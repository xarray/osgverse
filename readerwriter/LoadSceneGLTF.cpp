#include <osg/io_utils>
#include <osg/Version>
#include <osg/AnimationPath>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osgDB/ConvertUTF>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "pipeline/Utilities.h"
#include <libhv/all/client/requests.h>
#define DISABLE_SKINNING_DATA 0

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "LoadSceneGLTF.h"
#include "Utilities.h"

#ifndef GL_ARB_texture_rg
#define GL_RG                             0x8227
#define GL_R8                             0x8229
#define GL_R16                            0x822A
#define GL_RG8                            0x822B
#define GL_RG16                           0x822C
#define GL_R16F                           0x822D
#define GL_R32F                           0x822E
#define GL_RG16F                          0x822F
#define GL_RG32F                          0x8230
#endif

class HttpRequester : public osg::Referenced
{
public:
    static HttpRequester* instance()
    {
        static osg::ref_ptr<HttpRequester> s_ins = new HttpRequester;
        return s_ins.get();
    }

    bool read(const std::string& fileName, std::vector<unsigned char>& data)
    {
#ifdef __EMSCRIPTEN__
        osg::ref_ptr<osgVerse::WebFetcher> wf = new osgVerse::WebFetcher;
        bool succeed = wf->httpGet(osgDB::getServerFileName(fileName));
        if (!succeed) return false;
        else data.assign(wf->buffer.begin(), wf->buffer.end());
#else
        HttpRequest req;
        req.method = HTTP_GET; req.url = fileName;
        req.scheme = osgDB::getServerProtocol(fileName);

        HttpResponse response;
        int result = _client->send(&req, &response);
        if (result != 0) return false;
        data.assign(response.body.begin(), response.body.end());
#endif
        return true;
    }

protected:
    HttpRequester() { _client = new hv::HttpClient; }
    virtual ~HttpRequester() { delete _client; }
    hv::HttpClient* _client;
};

namespace osgVerse
{
    static bool FileExists(const std::string& absFilename, void* userData)
    {
        osgDB::ReaderWriter* rw = (osgDB::ReaderWriter*)userData;
        if (rw) return true;  // FIXME: always believe remote file exists?
        return tinygltf::FileExists(absFilename, userData);
    }

    static bool ReadWholeFile(std::vector<unsigned char>* out, std::string* err,
                              const std::string& filepath, void* userData)
    {
        osgDB::ReaderWriter* rw = (osgDB::ReaderWriter*)userData;
        if (rw && HttpRequester::instance()->read(filepath, *out)) return true;
        return tinygltf::ReadWholeFile(out, err, filepath, userData);
    }

    LoaderGLTF::LoaderGLTF(std::istream& in, const std::string& d, bool isBinary)
    {
        std::string protocol = osgDB::getServerProtocol(d);
        osgDB::ReaderWriter* rwWeb = (protocol.empty()) ? NULL
                : osgDB::Registry::instance()->getReaderWriterForExtension("verse_web");
        tinygltf::FsCallbacks fs = {
            &osgVerse::FileExists, &tinygltf::ExpandFilePath,
            &osgVerse::ReadWholeFile, &tinygltf::WriteWholeFile,
            (rwWeb ? NULL : &tinygltf::GetFileSizeInBytes), rwWeb };

        std::string err, warn; bool loaded = false;
        std::istreambuf_iterator<char> eos;
        std::vector<char> data(std::istreambuf_iterator<char>(in), eos);
        if (data.empty()) { OSG_WARN << "[LoaderGLTF] Unable to read from stream\n"; return; }

        tinygltf::TinyGLTF loader;
        loader.SetFsCallbacks(fs);
        if (isBinary)
        {
            loaded = loader.LoadBinaryFromMemory(
                &_modelDef, &err, &warn, (unsigned char*)&data[0], data.size(), d);
        }
        else
            loaded = loader.LoadASCIIFromString(&_modelDef, &err, &warn, &data[0], data.size(), d);
        
        if (!err.empty()) OSG_WARN << "[LoaderGLTF] Errors found: " << err << "\n";
        if (!warn.empty()) OSG_WARN << "[LoaderGLTF] Warnings found: " << warn << "\n";
        if (!loaded) { OSG_WARN << "[LoaderGLTF] Unable to load GLTF scene\n"; return; }
        _root = new osg::Group;

        // Preload skin data
        for (size_t i = 0; i < _modelDef.skins.size(); ++i)
        {
            const tinygltf::Skin& skin = _modelDef.skins[i];
            SkinningData sd; sd.skeletonBaseIndex = skin.skeleton;
            sd.invBindPoseAccessor = skin.inverseBindMatrices;
            sd.player = new PlayerAnimation; sd.player->setName(skin.name);
            sd.joints.assign(skin.joints.begin(), skin.joints.end());
            _skinningDataList.push_back(sd);
        }

        // Read and construct scene graph
        const tinygltf::Scene& defScene = _modelDef.scenes[_modelDef.defaultScene];
        for (size_t i = 0; i < defScene.nodes.size(); ++i)
        {
            osg::ref_ptr<osg::Node> child = createNode(
                defScene.nodes[i], _modelDef.nodes[defScene.nodes[i]]);
            if (child.valid()) _root->addChild(child.get());
        }

        // Load geometries to geodes of the scene graph
        for (size_t i = 0; i < _deferredMeshList.size(); ++i)
        {
            DeferredMeshData& mData = _deferredMeshList[i];
            createMesh(mData.meshRoot.get(), mData.mesh, mData.skinIndex);
        }

        // Configure skinning data and player objects
        for (size_t i = 0; i < _skinningDataList.size(); ++i)
        {
            SkinningData& sd = _skinningDataList[i];
            std::vector<osg::Transform*> boneList;
            for (size_t b = 0; b < sd.joints.size(); ++b)
            {
                osg::Node* n = _nodeCreationMap[sd.joints[b]];
                if (n && n->asGroup() && n->asGroup()->asTransform())
                    boneList.push_back(n->asGroup()->asTransform());
                else if (n)
                    OSG_WARN << "[LoaderGLTF] Invalid bone: " << n->getName() << std::endl;
                else
                    OSG_WARN << "[LoaderGLTF] Invalid empty bone: " << sd.joints[b] << std::endl;
            }

            createInvBindMatrices(sd, boneList, _modelDef.accessors[sd.invBindPoseAccessor]);
            if (!sd.meshList.empty()) sd.player->initialize(boneList, sd.meshList, sd.jointData);

            osg::Node* skeletonRoot = _nodeCreationMap[sd.skeletonBaseIndex];
            sd.skeletonRoot = skeletonRoot ? skeletonRoot->asGroup() : NULL;
#if !DISABLE_SKINNING_DATA
            sd.meshRoot = new osg::Geode; sd.meshRoot->addUpdateCallback(sd.player.get());
            if (sd.skeletonRoot.valid()) sd.skeletonRoot->addChild(sd.meshRoot.get());
            else _root->addChild(sd.meshRoot.get());
#endif
        }
    }

    osg::Node* LoaderGLTF::createNode(int id, tinygltf::Node& node)
    {
        osg::ref_ptr<osg::Geode> geode = (node.mesh >= 0) ? new osg::Geode : NULL;
        bool emptyTRS = (node.translation.empty() && node.rotation.empty()
                     && node.scale.empty()), emptyM = node.matrix.empty();
        if (geode.valid()) _deferredMeshList.push_back(
            DeferredMeshData(geode.get(), _modelDef.meshes[node.mesh], node.skin));
        /*if (emptyTRS && emptyM && node.children.empty())
        {
            geode->setName(node.name); _nodeCreationMap[id] = geode.get();
            return geode.release();
        }*/

        osg::ref_ptr<osg::MatrixTransform> group = new osg::MatrixTransform;
        for (size_t i = 0; i < node.children.size(); ++i)
        {
            osg::ref_ptr<osg::Node> child = createNode(
                node.children[i], _modelDef.nodes[node.children[i]]);
            if (child.valid()) group->addChild(child.get());
        }

        if (geode.valid()) group->addChild(geode.get());
        //else group->addChild(osgDB::readNodeFile("axes.osgt.(0.1,0.1,0.1).scale"));
        group->setName(node.name); _nodeCreationMap[id] = group.get();

        osg::Matrix matrix; std::vector<double> &t = node.translation, &r = node.rotation;
        if (!emptyTRS)
        {
            if (node.scale.size() == 3)
                matrix *= osg::Matrix::scale(node.scale[0], node.scale[1], node.scale[2]);
            if (r.size() == 4) matrix *= osg::Matrix::rotate(osg::Quat(r[0], r[1], r[2], r[3]));
            if (t.size() == 3) matrix *= osg::Matrix::translate(t[0], t[1], t[2]);
        }
        else if (!emptyM)
            matrix = osg::Matrix(&node.matrix[0]);
        group->setMatrix(matrix); return group.release();
    }

    bool LoaderGLTF::createMesh(osg::Geode* geode, tinygltf::Mesh& mesh, int skinIndex)
    {
        SkinningData* sd = (skinIndex < 0) ? NULL : &_skinningDataList[skinIndex];
#if !DISABLE_SKINNING_DATA
        if (sd != NULL) geode->setNodeMask(0);  // FIXME: ugly to hide original meshes
#endif

        for (size_t i = 0; i < mesh.primitives.size(); ++i)
        {
            std::vector<unsigned char> jointList0; std::vector<unsigned short> jointList1;
            std::vector<float> weightList;

            osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
            geom->setUseDisplayList(false);
            geom->setUseVertexBufferObjects(true);

            tinygltf::Primitive primitive = mesh.primitives[i];
            for (auto& attrib : primitive.attributes)
            {
                tinygltf::Accessor& attrAccessor = _modelDef.accessors[attrib.second];
                const tinygltf::BufferView& attrView = _modelDef.bufferViews[attrAccessor.bufferView];
                int size = attrAccessor.count; if (!size || attrView.buffer < 0) continue;

                const tinygltf::Buffer& buffer = _modelDef.buffers[attrView.buffer];
                int compNum = (attrAccessor.type != TINYGLTF_TYPE_SCALAR) ? attrAccessor.type : 1;
                int compSize = tinygltf::GetComponentSizeInBytes(attrAccessor.componentType);
                int copySize = size * (compSize * compNum);
                size_t offset = attrView.byteOffset + attrAccessor.byteOffset;
                if (attrView.byteStride > 0 && attrView.byteStride != (compSize * compNum))
                {
                    OSG_WARN << "[LoaderGLTF] Unsupported byte-stride: " << attrView.byteStride
                             << " for " << attrib.first << std::endl;
                }

                //std::cout << attrib.first << ": Size = " << size << ", Components = " << compNum
                //          << ", ComponentBytes = " << compSize << std::endl;
                if (attrib.first.compare("POSITION") == 0 && compSize == 4 && compNum == 3)
                {
                    osg::Vec3Array* va = new osg::Vec3Array(size);
                    memcpy(&(*va)[0], &buffer.data[offset], copySize);
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
                    va->setNormalize(attrAccessor.normalized);
#endif
                    geom->setVertexArray(va);
                }
                else if (attrib.first.compare("NORMAL") == 0 && compSize == 4 && compNum == 3)
                {
                    osg::Vec3Array* na = new osg::Vec3Array(size);
                    memcpy(&(*na)[0], &buffer.data[offset], copySize);
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
                    na->setNormalize(attrAccessor.normalized);
                    geom->setNormalArray(na, osg::Array::BIND_PER_VERTEX);
#else
                    geom->setNormalArray(na); geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
#endif
                }
                else if (attrib.first.compare("TANGENT") == 0 && compSize == 4 && compNum == 4)
                {   // Do nothing as we calculate tangent/binormal by ourselves
#if 0
                    osg::Vec4Array* ta = new osg::Vec4Array(size);
                    memcpy(&(*ta)[0], &buffer.data[offset], copySize);
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
                    ta->setNormalize(attrAccessor.normalized);
                    geom->setVertexAttribArray(6, ta, osg::Array::BIND_PER_VERTEX);
#endif
                    geom->setVertexAttribArray(6, ta);
                    geom->setVertexAttribBinding(6, osg::Geometry::BIND_PER_VERTEX);
#endif
                }
                else if (attrib.first.find("TEXCOORD_") != std::string::npos && compSize == 4 && compNum == 2)
                {
                    osg::Vec2Array* ta = new osg::Vec2Array(size);
                    memcpy(&(*ta)[0], &buffer.data[offset], copySize);
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
                    ta->setNormalize(attrAccessor.normalized);
#endif
                    geom->setTexCoordArray(atoi(attrib.first.substr(9).c_str()), ta);
                }
                else if (attrib.first.find("JOINTS_") != std::string::npos && compNum == 4)
                {
                    int jID = atoi(attrib.first.substr(7).c_str());
                    if (jID == 0)  // FIXME: joints group > 0?
                    {
                        if (compSize == 1)
                        {
                            jointList0.resize(size * compNum);
                            memcpy(&jointList0[0], &buffer.data[offset], copySize);
                        }
                        else
                        {
                            jointList1.resize(size * compNum);
                            memcpy(&jointList1[0], &buffer.data[offset], copySize);
                        }
                    }
                }
                else if (attrib.first.find("WEIGHTS_") != std::string::npos && compSize == 4 && compNum == 4)
                {
                    int wID = atoi(attrib.first.substr(8).c_str());
                    if (wID == 0)  // FIXME: weights group > 0?
                    {
                        weightList.resize(size * compNum);
                        memcpy(&weightList[0], &buffer.data[offset], copySize);
                    }
                }
                else
                    OSG_WARN << "[LoaderGLTF] Unsupported primitive " << attrib.first << " with "
                             << compNum << "-components and dataSize=" << compSize << std::endl;
            }

            tinygltf::Accessor indexAccessor = _modelDef.accessors[primitive.indices];
            const tinygltf::BufferView& indexView = _modelDef.bufferViews[indexAccessor.bufferView];
            osg::Vec3Array* va = static_cast<osg::Vec3Array*>(geom->getVertexArray());
            if (!va || (va && va->empty())) continue;

            osg::ref_ptr<osg::PrimitiveSet> p;
            if (indexView.target == 0)
                p = new osg::DrawArrays(GL_POINTS, 0, va->size());
            else  // ELEMENT_ARRAY_BUFFER = 34963
            {
                const tinygltf::Buffer& indexBuffer = _modelDef.buffers[indexView.buffer];
                int compSize = tinygltf::GetComponentSizeInBytes(indexAccessor.componentType);
                int size = indexAccessor.count; if (!size) continue;
                if (indexView.byteStride > 0 && indexView.byteStride != compSize)
                {
                    OSG_WARN << "[LoaderGLTF] Unsupported byte-stride: " << indexView.byteStride
                             << " for primitive indices" << std::endl;
                }

                size_t offset = indexView.byteOffset + indexAccessor.byteOffset;
                switch (compSize)
                {
                case 1:
                    {
                        osg::DrawElementsUByte* de = new osg::DrawElementsUByte(GL_POINTS, size); p = de;
                        memcpy(&(*de)[0], &indexBuffer.data[offset], size * compSize);
                    }
                    break;
                case 2:
                    {
                        osg::DrawElementsUShort* de = new osg::DrawElementsUShort(GL_POINTS, size); p = de;
                        memcpy(&(*de)[0], &indexBuffer.data[offset], size * compSize);
                    }
                    break;
                case 4:
                    {
                        osg::DrawElementsUInt* de = new osg::DrawElementsUInt(GL_POINTS, size); p = de;
                        memcpy(&(*de)[0], &indexBuffer.data[offset], size * compSize);
                    }
                    break;
                default:
                    OSG_WARN << "[LoaderGLTF] Unknown size " << compSize << std::endl; break;
                }
            }

            switch (primitive.mode)
            {
            case TINYGLTF_MODE_POINTS: p->setMode(GL_POINTS); break;
            case TINYGLTF_MODE_LINE: p->setMode(GL_LINES); break;
            case TINYGLTF_MODE_LINE_LOOP: p->setMode(GL_LINE_LOOP); break;
            case TINYGLTF_MODE_LINE_STRIP: p->setMode(GL_LINE_STRIP); break;
            case TINYGLTF_MODE_TRIANGLES: p->setMode(GL_TRIANGLES); break;
            case TINYGLTF_MODE_TRIANGLE_STRIP: p->setMode(GL_TRIANGLE_STRIP); break;
            case TINYGLTF_MODE_TRIANGLE_FAN: p->setMode(GL_TRIANGLE_FAN); break;
            default: OSG_WARN << "[LoaderGLTF] Unknown primitive " << primitive.mode << std::endl; continue;
            }

            geom->addPrimitiveSet(p.get());
            geode->addDrawable(geom.get());
            if (primitive.material >= 0)
            {
                tinygltf::Material& material = _modelDef.materials[primitive.material];
                createMaterial(geom->getOrCreateStateSet(), material);
            }

            if (sd != NULL && !weightList.empty())  // handle skinning data
            {
                typedef std::pair<osg::Transform*, float> JointWeightPair;
                PlayerAnimation::GeometryJointData gjData;
                for (size_t w = 0; w < weightList.size(); w += 4)
                {
                    PlayerAnimation::GeometryJointData::JointWeights jwMap;
                    osg::Transform* tList[4] = { NULL };
                    for (int k = 0; k < 4; ++k)
                    {
                        osg::Node* n = _nodeCreationMap[
                            !jointList0.empty() ? (int)jointList0[w + k] : (int)jointList1[w + k]];
                        if (n && n->asGroup()) tList[k] = n->asGroup()->asTransform();
                        if (tList[k]) jwMap.push_back(JointWeightPair(tList[k], weightList[w + k]));
                        //else OSG_WARN << "[LoaderGLTF] No joint found for weight " << (w + k) << std::endl;
                    }
                    gjData._weightList.push_back(jwMap);
                }

                gjData._stateset = geom->getStateSet();
                sd->jointData[geom.get()] = gjData;
                sd->meshList.push_back(geom.get());
            }
        }
        return true;
    }

    void LoaderGLTF::createMaterial(osg::StateSet* ss, tinygltf::Material material)
    {
        // Shininess(RGB) = Occlusion/Roughness/Metallic, Ambient = Occlusion
        int baseID = material.pbrMetallicRoughness.baseColorTexture.index;
        int roughnessID = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
        int normalID = material.normalTexture.index;
        int emissiveID = material.emissiveTexture.index;
        int occlusionID = material.occlusionTexture.index;

        if (baseID >= 0) createTexture(ss, 0, uniformNames[0], _modelDef.textures[baseID]);
        if (normalID >= 0) createTexture(ss, 1, uniformNames[1], _modelDef.textures[normalID]);
        if (roughnessID >= 0) createTexture(ss, 3, uniformNames[3], _modelDef.textures[roughnessID]);
        if (occlusionID >= 0) createTexture(ss, 4, uniformNames[4], _modelDef.textures[occlusionID]);
        if (emissiveID >= 0) createTexture(ss, 5, uniformNames[5], _modelDef.textures[emissiveID]);

#if 0
        if (material.alphaMode.compare("OPAQUE") == 0)  // FIXME: handle transparent
            ss->setRenderingHint(osg::StateSet::OPAQUE_BIN);
        else
            ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
#endif
    }

    void LoaderGLTF::createTexture(osg::StateSet* ss, int u,
                                   const std::string& name, tinygltf::Texture& tex)
    {
        tinygltf::Image& imageSrc = _modelDef.images[tex.source];
        if (imageSrc.image.empty()) return;

        GLenum format = GL_RGBA, type = GL_UNSIGNED_BYTE;
        if (imageSrc.bits == 16) type = GL_UNSIGNED_SHORT;
        if (imageSrc.component == 1) format = GL_RED;
        else if (imageSrc.component == 2) format = GL_RG;
        else if (imageSrc.component == 3) format = GL_RGB;

        osg::ref_ptr<osg::Image> image2D = _imageMap[tex.source].get();
        if (!image2D)
        {
            //std::cout << name << ": " << imageSrc.uri << ", Size = "
            //          << imageSrc.width << "x" << imageSrc.height << "\n";

            osg::ref_ptr<osg::Image> image = new osg::Image;
            image->setFileName(imageSrc.uri);
            image->allocateImage(imageSrc.width, imageSrc.height, 1, format, type);
            switch (imageSrc.component)
            {
            case 1: image->setInternalTextureFormat(GL_R8); break;
            case 2: image->setInternalTextureFormat(GL_RG8); break;
            case 3: image->setInternalTextureFormat(GL_RGB8); break;
            default: image->setInternalTextureFormat(GL_RGBA8); break;
            }
            memcpy(image->data(), &imageSrc.image[0], image->getTotalSizeInBytes());
            
            image2D = image.get(); image2D->setName(imageSrc.uri);
            _imageMap[tex.source] = image2D;
            OSG_NOTICE << "[LoaderGLTF] " << imageSrc.uri << " loaded for " << name << std::endl;
        }

        osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
        tex2D->setResizeNonPowerOfTwoHint(false);
        tex2D->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT);
        tex2D->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT);
        tex2D->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR_MIPMAP_LINEAR);
        tex2D->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        tex2D->setImage(image2D.get()); tex2D->setName(imageSrc.uri);
        ss->setTextureAttributeAndModes(u, tex2D.get());
        //ss->addUniform(new osg::Uniform(name.c_str(), u));
    }

    void LoaderGLTF::createInvBindMatrices(SkinningData& sd, const std::vector<osg::Transform*>& bones,
                                           tinygltf::Accessor& accessor)
    {
        const tinygltf::BufferView& attrView = _modelDef.bufferViews[accessor.bufferView];
        if (attrView.buffer < 0) return;

        const tinygltf::Buffer& buffer = _modelDef.buffers[attrView.buffer];
        int compSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
        int compNum = accessor.type, size = accessor.count;
        if (compNum != TINYGLTF_TYPE_MAT4 || size != (int)bones.size()) return;
        if (attrView.byteStride > 0 && attrView.byteStride != (compSize * 4 * sizeof(float)))
        {
            OSG_WARN << "[LoaderGLTF] Unsupported byte-stride: " << attrView.byteStride
                     << " for inv-bind matrices" << std::endl;
        }

        size_t offset = accessor.byteOffset + attrView.byteOffset;
        size_t matSize = compSize * 4; std::vector<float> matrices(size * matSize);
        memcpy(&matrices[0], &buffer.data[offset], matrices.size() * sizeof(float));
        for (int i = 0; i < size; ++i)
        {
            osg::Matrixf matrix(&matrices[i * matSize]);
            for (size_t j = 0; j < sd.meshList.size(); ++j)
            {
                PlayerAnimation::GeometryJointData& jData = sd.jointData[sd.meshList[j]];
                jData._invBindPoseMap[bones[i]] = matrix;
            }
        }
    }

    osg::ref_ptr<osg::Group> loadGltf(const std::string& file, bool isBinary)
    {
        std::string workDir = osgDB::getFilePath(file);
        std::ifstream in(file.c_str(), std::ios::in | std::ios::binary);
        if (!in)
        {
            OSG_WARN << "[LoaderGLTF] file " << file << " not readable" << std::endl;
            return NULL;
        }
        
        osg::ref_ptr<LoaderGLTF> loader = new LoaderGLTF(in, workDir, isBinary);
        return loader->getRoot();
    }

    osg::ref_ptr<osg::Group> loadGltf2(std::istream& in, const std::string& dir, bool isBinary)
    {
        osg::ref_ptr<LoaderGLTF> loader = new LoaderGLTF(in, dir, isBinary);
        return loader->getRoot();
    }
}
