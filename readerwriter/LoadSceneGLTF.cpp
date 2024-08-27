#include <osg/io_utils>
#include <osg/Version>
#include <osg/ValueObject>
#include <osg/AnimationPath>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osgDB/ConvertUTF>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "animation/BlendShapeAnimation.h"
#include "pipeline/Utilities.h"
#include "LoadTextureKTX.h"
#include <libhv/all/client/requests.h>
#include <picojson.h>
#define DISABLE_SKINNING_DATA 0

#define TINYGLTF_IMPLEMENTATION
#ifdef VERSE_USE_DRACO
#   define TINYGLTF_ENABLE_DRACO
#endif
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "LoadSceneGLTF.h"
#include "Utilities.h"

namespace osgVerse
{
    extern bool LoadBinaryV1(std::vector<char>& data, const std::string& baseDir);
}

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

static std::string trimString(const std::string& str)
{
    if (!str.size()) return str;
    std::string::size_type first = str.find_first_not_of(" \t");
    std::string::size_type last = str.find_last_not_of("  \t\r\n");
    if ((first == str.npos) || (last == str.npos)) return std::string("");
    return str.substr(first, last - first + 1);
}

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

    /*class RTCCenterCallback : public osg::NodeCallback
    {
    public:
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            osg::Transform* transform = node->asGroup()->asTransform();
            if (transform != NULL && transform->asMatrixTransform())
            {
                osg::Vec3d rtcCenter;
                if (node->getUserValue("RTC_CENTER", rtcCenter))
                {
                    osg::MatrixList matrices = node->getWorldMatrices(getRtcRoot(node));
                    if (!matrices.empty())
                    {
                        osg::Vec3d offset = rtcCenter - matrices[0].getTrans();
                        if (!osg::equivalent(offset.length2(), 0.0))
                            transform->asMatrixTransform()->setMatrix(osg::Matrix::translate(offset));
                    }
                }
            }
            traverse(node, nv);
        }

        osg::Node* getRtcRoot(osg::Node* node)
        {
            bool rtcRoot = false;
            if (node->getUserValue("RTC_ROOT", rtcRoot)) { if (rtcRoot) return node; }
            if (node->getNumParents() == 0) return NULL;
            return getRtcRoot(node->getParent(0));
        }
    };*/

    static osg::Vec3d ReadRtcCenterFeatureTable(std::vector<char>& data, int offset, int size)
    {
        std::string json; json.assign(data.begin() + offset, data.begin() + size + offset);
        picojson::value root; std::string err = picojson::parse(root, json);
        if (err.empty() && root.contains("RTC_CENTER"))
        {
            picojson::value center = root.get<picojson::object>().at("RTC_CENTER");
            if (center.is<picojson::array>())
            {
                picojson::array cValues = center.get<picojson::array>();
                if (cValues.size() > 2) return osg::Vec3d(
                    cValues[0].get<double>(), cValues[2].get<double>(), -cValues[1].get<double>());
            }
        }
        return osg::Vec3d();
    }

    unsigned int ReadB3dmHeader(std::vector<char>& data, osg::Vec3d* rtcCenter = NULL)
    {
        // https://github.com/CesiumGS/3d-tiles/blob/main/specification/TileFormats/Batched3DModel/README.adoc#tileformats-batched3dmodel-batched-3d-model
        // magic + version + length + featureTableJsonLength + featureTableBinLength +
        // batchTableJsonLength + batchTableBinLength + <Real feature table> + <Real batch table> + GLTF body
        int header[7], hSize = 7 * sizeof(int); memcpy(header, data.data(), hSize);
        if (rtcCenter && header[3] > 0) *rtcCenter = ReadRtcCenterFeatureTable(data, hSize, header[3]);
        return hSize + header[3] + header[4] + header[5] + header[6];
    }

    unsigned int ReadI3dmHeader(std::vector<char>& data, unsigned int& format)
    {
        // https://github.com/CesiumGS/3d-tiles/blob/main/specification/TileFormats/Instanced3DModel/README.adoc#tileformats-instanced3dmodel-instanced-3d-model
        // magic + version + length + featureTableJsonLength + featureTableBinLength +
        // batchTableJsonLength + batchTableBinLength + gltfFormat +
        // <Real feature table> + <Real batch table> + GLTF body
        int header[8]; memcpy(header, data.data(), 8 * sizeof(int)); format = header[7];
        return 8 * sizeof(int) + header[3] + header[4] + header[5] + header[6];
    }

    bool LoadImageDataEx(tinygltf::Image* image, const int image_idx, std::string* err,
                         std::string* warn, int req_width, int req_height,
                         const unsigned char* bytes, int size, void* user_data)
    {
        // Most copied from tinygltf::LoadImageData()
        int w = 0, h = 0, comp = 0, req_comp = 0;
        unsigned char* data = nullptr;
        tinygltf::LoadImageDataOption option;
        if (user_data)
            option = *reinterpret_cast<tinygltf::LoadImageDataOption*>(user_data);

        // preserve_channels true: Use channels stored in the image file.
        req_comp = option.preserve_channels ? 0 : 4;
        int bits = 8, pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;

        // It is possible that the image we want to load is a 16bit per channel image
        if (stbi_is_16_bit_from_memory(bytes, size))
        {
            data = reinterpret_cast<unsigned char*>(
                stbi_load_16_from_memory(bytes, size, &w, &h, &comp, req_comp));
            if (data) { bits = 16; pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT; }
        }

        // Load it as a normal 8bit per channel if not 16bit
        if (!data) data = stbi_load_from_memory(bytes, size, &w, &h, &comp, req_comp);
        if (!data)
        {
            if (image->mimeType.find("ktx") != std::string::npos)
            {
                image->image.resize(size);
                std::copy(bytes, bytes + size, image->image.begin());
                return true;  // parsing KTX later
            }
            else if (image->mimeType.find("image/") != std::string::npos)
            {
                std::string ext = image->mimeType.substr(6);  // image/ext
                osgDB::ReaderWriter* rw = osgDB::Registry::instance()->getReaderWriterForExtension(ext);
                if (rw == NULL) rw = osgDB::Registry::instance()->getReaderWriterForExtension("verse_" + ext);
                if (rw != NULL)
                {
                    image->image.resize(size); image->extensions_json_string = ext;
                    std::copy(bytes, bytes + size, image->image.begin());
                    return true;  // parsing by plugin later
                }
            }

            if (err) (*err) += "Unknown image format. STB cannot decode image data for image[" +
                               std::to_string(image_idx) + "] name = \"" + image->name + "\", type = " +
                               image->mimeType + ".\n";
            return false;
        }

        if ((w < 1) || (h < 1))
        {
            stbi_image_free(data);
            if (err) (*err) += "Invalid image data for image[" + std::to_string(image_idx) +
                               "] name = \"" + image->name + "\"\n";
            return false;
        }

        if (req_width > 0)
        {
            if (req_width != w)
            {
                stbi_image_free(data);
                if (err) (*err) += "Image width mismatch for image[" +
                                   std::to_string(image_idx) + "] name = \"" + image->name + "\"\n";
                return false;
            }
        }

        if (req_height > 0)
        {
            if (req_height != h)
            {
                stbi_image_free(data);
                if (err) (*err) += "Image height mismatch. for image[" +
                                   std::to_string(image_idx) + "] name = \"" + image->name + "\"\n";
                return false;
            }
        }

        // loaded data has `req_comp` channels(components)
        if (req_comp != 0) comp = req_comp;
        image->width = w; image->height = h;
        image->component = comp; image->bits = bits;
        image->pixel_type = pixel_type;
        image->image.resize(static_cast<size_t>(w * h * comp) * size_t(bits / 8));
        std::copy(data, data + w * h * comp * (bits / 8), image->image.begin());
        stbi_image_free(data); return true;
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
        //_rtcCenterCallback = new RTCCenterCallback;

        std::string err, warn; bool loaded = false;
        std::istreambuf_iterator<char> eos; osg::Vec3d rtcCenter;
        std::vector<char> data(std::istreambuf_iterator<char>(in), eos);
        if (data.empty()) { OSG_WARN << "[LoaderGLTF] Unable to read from stream\n"; return; }

        tinygltf::TinyGLTF loader;
        loader.SetImageLoader(&LoadImageDataEx, this);
        loader.SetFsCallbacks(fs);
        if (isBinary)
        {
            unsigned int version = 2, offset = 0, format = 0;  // 0: url, 1: raw GLTF
            std::string externalFileURI;
            if (data.size() > 4)
            {
                if (data[0] == 'b' && data[1] == '3' && data[2] == 'd' && data[3] == 'm')
                {
                    offset = ReadB3dmHeader(data, &rtcCenter);
                    memcpy(&version, &data[0] + offset + 4, 4); tinygltf::swap4(&version);
                }
                else if (data[0] == 'i' && data[1] == '3' && data[2] == 'd' && data[3] == 'm')
                {
                    offset = ReadI3dmHeader(data, format);
                    if (format == 0)
                    {
                        std::vector<char> uri(data.size() - offset);
                        memcpy(&uri[0], &data[0] + offset, uri.size()); char* p = &uri[0];
                        externalFileURI = std::string(p, p + uri.size());
                    }
                    else
                        { memcpy(&version, &data[0] + offset + 4, 4); tinygltf::swap4(&version); }
                }
            }

            if (!externalFileURI.empty())
            {
                std::string fileName = d + "/" + trimString(externalFileURI);
                std::string ext = osgDB::getFileExtension(fileName);
                if (ext == "glb") loaded = loader.LoadBinaryFromFile(&_modelDef, &err, &warn, fileName);
                else loaded = loader.LoadASCIIFromFile(&_modelDef, &err, &warn, fileName);
            }
            else if (version >= 2)
            {
                loaded = loader.LoadBinaryFromMemory(
                    &_modelDef, &err, &warn, (unsigned char*)&data[0] + offset, data.size() - offset, d);
            }
            else loaded = LoadBinaryV1(data, d);
        }
        else
            loaded = loader.LoadASCIIFromString(&_modelDef, &err, &warn, &data[0], data.size(), d);
        
        if (!err.empty()) OSG_WARN << "[LoaderGLTF] Errors found: " << err << std::endl;
        if (!warn.empty()) OSG_WARN << "[LoaderGLTF] Warnings found: " << warn << std::endl;
        if (!loaded) { OSG_WARN << "[LoaderGLTF] Unable to load GLTF scene" << std::endl; return; }

        if (rtcCenter.length2() > 0.0)
        {
            osg::MatrixTransform* mt = new osg::MatrixTransform;
            mt->setMatrix(osg::Matrix::translate(rtcCenter));
            _root = mt;  //_root->setUserValue("RTC_CENTER", rtcCenter);
        }
        else
        {
            if (!_modelDef.extensions.empty() &&
                _modelDef.extensions.find("CESIUM_RTC") != _modelDef.extensions.end())
            {
                if (_modelDef.extensions["CESIUM_RTC"].Has("center"))
                {
                    const tinygltf::Value& center = _modelDef.extensions["CESIUM_RTC"].Get("center");
                    if (center.IsArray())
                    {
                        const tinygltf::Value::Array& centerData = center.Get<tinygltf::Value::Array>();
                        if (centerData.size() > 2) rtcCenter.set(
                            centerData[0].GetNumberAsDouble(), centerData[2].GetNumberAsDouble(),
                            -centerData[1].GetNumberAsDouble());

                        osg::MatrixTransform* mt = new osg::MatrixTransform;
                        mt->setMatrix(osg::Matrix::translate(rtcCenter));
                        _root = mt;  //_root->setUserValue("RTC_CENTER", rtcCenter);
                    }
                }
            }
            if (!_root) _root = new osg::Group;
        }

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

        // Load geometries to geodes (after all nodes have registered with an ID)
        for (size_t i = 0; i < _deferredMeshList.size(); ++i)
        {
            DeferredMeshData& mData = _deferredMeshList[i];
            createMesh(mData.meshRoot.get(), mData.mesh, mData.skinIndex);
        }

        // Configure skinning data and player objects
        std::map<size_t, std::vector<osg::Transform*>> boneListMap;
        for (size_t i = 0; i < _skinningDataList.size(); ++i)
        {
            SkinningData& sd = _skinningDataList[i];
            std::vector<osg::Transform*>& boneList = boneListMap[i];
            for (size_t b = 0; b < sd.joints.size(); ++b)
            {
                osg::Node* n = _nodeCreationMap[sd.joints[b]];
                if (n && n->asGroup() && n->asGroup()->asTransform())
                    boneList.push_back(n->asGroup()->asTransform());
                else if (n != NULL)
                    { OSG_WARN << "[LoaderGLTF] Invalid bone: " << n->getName() << std::endl; }
                else
                    { OSG_WARN << "[LoaderGLTF] Invalid empty bone: " << sd.joints[b] << std::endl; }
            }

            createInvBindMatrices(sd, boneList, _modelDef.accessors[sd.invBindPoseAccessor]);
            if (!sd.meshList.empty()) sd.player->initialize(boneList, sd.meshList, sd.jointData);

            osg::Node* skeletonRoot = _nodeCreationMap[sd.skeletonBaseIndex];
            sd.skeletonRoot = skeletonRoot ? skeletonRoot->asGroup() : NULL;
#if !DISABLE_SKINNING_DATA
            sd.meshRoot = new osg::Geode; sd.meshRoot->setName("CharacterGeode");
            sd.meshRoot->addUpdateCallback(sd.player.get());
            if (sd.skeletonRoot.valid()) sd.skeletonRoot->addChild(sd.meshRoot.get());
            else _root->addChild(sd.meshRoot.get());
#endif
        }

        // Configure animations
        for (size_t i = 0; i < _modelDef.animations.size(); ++i)
        {
            tinygltf::Animation& anim = _modelDef.animations[i];
            std::string animName = anim.name; if (animName.empty()) animName = "Take001";
            int belongsToSkeleton = -1;

            typedef std::pair<std::string, int> PathAndSampler;
            std::map<osg::Node*, std::vector<PathAndSampler>> samplers;
            for (size_t j = 0; j < anim.channels.size(); ++j)
            {
                tinygltf::AnimationChannel& ch = anim.channels[j];
                if (ch.sampler < 0 || ch.target_node < 0) continue;

                osg::Node* node = _nodeCreationMap[ch.target_node];
                samplers[node].push_back(PathAndSampler(ch.target_path, ch.sampler));
                for (size_t k = 0; k < _skinningDataList.size(); ++k)
                {
                    std::vector<int>& joints = _skinningDataList[k].joints;
                    if (std::find(joints.begin(), joints.end(), ch.target_node) != joints.end())
                    { belongsToSkeleton = k; break; }
                    else if (node == _skinningDataList[k].skeletonRoot)
                    { belongsToSkeleton = k; break; }
                }
            }

            std::map<osg::Transform*, PlayerAnimation::AnimationData> skeletonAnimMap;
            for (std::map<osg::Node*, std::vector<PathAndSampler>>::iterator
                 itr = samplers.begin(); itr != samplers.end(); ++itr)
            {
                PlayerAnimation::AnimationData playerAnim;
                std::vector<PathAndSampler>& pathList = itr->second;
                for (size_t j = 0; j < pathList.size(); ++j)
                {
                    tinygltf::AnimationSampler& sp = anim.samplers[pathList[j].second];
                    if (sp.input < 0 || sp.output < 0) continue;
                    playerAnim._interpolations[pathList[j].first] = sp.interpolation;
                    createAnimationSampler(playerAnim, pathList[j].first,
                            _modelDef.accessors[sp.input], _modelDef.accessors[sp.output]);
                }

                if (belongsToSkeleton >= 0)
                {
                    osg::Group* g = (itr->first) ? itr->first->asGroup() : NULL;
                    osg::Transform* t = g ? g->asTransform() : NULL;
                    if (t) skeletonAnimMap[t] = playerAnim;
                }
                else {}  // TODO: non-skeleton animations
            }

            if (belongsToSkeleton >= 0 && !skeletonAnimMap.empty())
            {
                std::vector<osg::Transform*>& boneList = boneListMap[belongsToSkeleton];
                _skinningDataList[belongsToSkeleton].player->loadAnimation(
                    animName, boneList, skeletonAnimMap);
            }
        }  // end of for (animations)
    }

    osg::Node* LoaderGLTF::createNode(int id, tinygltf::Node& node)
    {
        osg::ref_ptr<osg::Geode> geode = (node.mesh >= 0) ? new osg::Geode : NULL;
        bool emptyTRS = (node.translation.empty() && node.rotation.empty()
                     && node.scale.empty()), emptyM = node.matrix.empty();
        if (geode.valid())
        {
            geode->setName(node.name + "_Geode");
            _deferredMeshList.push_back(
                DeferredMeshData(geode.get(), _modelDef.meshes[node.mesh], node.skin));
        }
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
            geom->setName(mesh.name + "_" + std::to_string(i));
            geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);

            tinygltf::Primitive primitive = mesh.primitives[i];
            for (std::map<std::string, int>::iterator attrib = primitive.attributes.begin();
                attrib != primitive.attributes.end(); ++attrib)
            {
                tinygltf::Accessor& attrAccessor = _modelDef.accessors[attrib->second];
                const tinygltf::BufferView& attrView = _modelDef.bufferViews[attrAccessor.bufferView];
                int size = attrAccessor.count; if (!size || attrView.buffer < 0) continue;

                const tinygltf::Buffer& buffer = _modelDef.buffers[attrView.buffer];
                int compNum = (attrAccessor.type != TINYGLTF_TYPE_SCALAR) ? attrAccessor.type : 1;
                int compSize = tinygltf::GetComponentSizeInBytes(attrAccessor.componentType);
                int copySize = size * (compSize * compNum);
                size_t offset = attrView.byteOffset + attrAccessor.byteOffset;
                size_t stride = (attrView.byteStride > 0 && attrView.byteStride != (compSize * compNum))
                              ? attrView.byteStride : 0;

                //std::cout << attrib->first << ": Size = " << size << ", Components = " << compNum
                //          << ", ComponentBytes = " << compSize << std::endl;
                if (attrib->first.compare("POSITION") == 0 && compSize == 4 && compNum == 3)
                {
                    osg::Vec3Array* va = new osg::Vec3Array(size);
                    copyBufferData(&(*va)[0], &buffer.data[offset], copySize, stride, size);
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
                    va->setNormalize(attrAccessor.normalized);
#endif
                    geom->setVertexArray(va);
                }
                else if (attrib->first.compare("NORMAL") == 0 && compSize == 4 && compNum == 3)
                {
                    osg::Vec3Array* na = new osg::Vec3Array(size);
                    copyBufferData(&(*na)[0], &buffer.data[offset], copySize, stride, size);
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
                    na->setNormalize(attrAccessor.normalized);
                    geom->setNormalArray(na, osg::Array::BIND_PER_VERTEX);
#else
                    geom->setNormalArray(na); geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
#endif
                }
                else if (attrib->first.compare("COLOR") == 0 && compNum == 4 && compSize == 4)
                {
                    osg::Vec4Array* ca = new osg::Vec4Array(size);
                    copyBufferData(&(*ca)[0], &buffer.data[offset], copySize, stride, size);
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
                    ca->setNormalize(attrAccessor.normalized);
                    geom->setColorArray(ca, osg::Array::BIND_PER_VERTEX);
#else
                    geom->setColorArray(ca); geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
#endif
                }
                else if (attrib->first.compare("TANGENT") == 0 && compSize == 4 && compNum == 4)
                {
                    osg::Vec4Array* ta = new osg::Vec4Array(size);
                    copyBufferData(&(*ta)[0], &buffer.data[offset], copySize, stride, size);
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
                    ta->setNormalize(attrAccessor.normalized);
                    geom->setVertexAttribArray(6, ta, osg::Array::BIND_PER_VERTEX);
#endif
                    geom->setVertexAttribArray(6, ta);
                    geom->setVertexAttribBinding(6, osg::Geometry::BIND_PER_VERTEX);
                }
                else if (attrib->first.find("TEXCOORD_") != std::string::npos && compSize == 4 && compNum == 2)
                {
                    osg::Vec2Array* ta = new osg::Vec2Array(size);
                    copyBufferData(&(*ta)[0], &buffer.data[offset], copySize, stride, size);
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
                    ta->setNormalize(attrAccessor.normalized);
#endif
                    geom->setTexCoordArray(atoi(attrib->first.substr(9).c_str()), ta);
                }
                else if (attrib->first.find("JOINTS_") != std::string::npos && compNum == 4)
                {
                    int jID = atoi(attrib->first.substr(7).c_str());
                    if (jID == 0)  // FIXME: joints group > 0?
                    {
                        if (compSize == 1)
                        {
                            jointList0.resize(size * compNum);
                            copyBufferData(&jointList0[0], &buffer.data[offset], copySize, stride, size);
                        }
                        else
                        {
                            jointList1.resize(size * compNum);
                            copyBufferData(&jointList1[0], &buffer.data[offset], copySize, stride, size);
                        }
                    }
                }
                else if (attrib->first.find("WEIGHTS_") != std::string::npos &&
                         compSize == 4 && compNum == 4)
                {
                    int wID = atoi(attrib->first.substr(8).c_str());
                    if (wID == 0)  // FIXME: weights group > 0?
                    {
                        weightList.resize(size * compNum);
                        copyBufferData(&weightList[0], &buffer.data[offset], copySize, stride, size);
                    }
                }
                else
                    OSG_WARN << "[LoaderGLTF] Unsupported primitive " << attrib->first << " with "
                             << compNum << "-components and dataSize=" << compSize << std::endl;
            }

            // Configure primitive index array
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
                size_t stride = (indexView.byteStride > 0 && indexView.byteStride != compSize)
                              ? indexView.byteStride : 0;

                size_t offset = indexView.byteOffset + indexAccessor.byteOffset;
                switch (compSize)
                {
                case 1:
                    {
                        osg::DrawElementsUByte* de = new osg::DrawElementsUByte(GL_POINTS, size); p = de;
                        copyBufferData(&(*de)[0], &indexBuffer.data[offset], size * compSize, stride, size);
                    }
                    break;
                case 2:
                    {
                        osg::DrawElementsUShort* de = new osg::DrawElementsUShort(GL_POINTS, size); p = de;
                        copyBufferData(&(*de)[0], &indexBuffer.data[offset], size * compSize, stride, size);
                    }
                    break;
                case 4:
                    {
                        osg::DrawElementsUInt* de = new osg::DrawElementsUInt(GL_POINTS, size); p = de;
                        copyBufferData(&(*de)[0], &indexBuffer.data[offset], size * compSize, stride, size);
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

            // Apply to geode and create material
            geom->addPrimitiveSet(p.get());
            geode->addDrawable(geom.get());
            if (primitive.material >= 0)
            {
                tinygltf::Material& material = _modelDef.materials[primitive.material];
                createMaterial(geom->getOrCreateStateSet(), material);
            }

            // Handle skinning data
            if (sd != NULL && !weightList.empty())
            {
                typedef std::pair<osg::Transform*, float> JointWeightPair;
                PlayerAnimation::GeometryJointData gjData;
                for (size_t w = 0; w < weightList.size(); w += 4)
                {
                    PlayerAnimation::GeometryJointData::JointWeights jwMap;
                    osg::Transform* tList[4] = { NULL };
                    for (int k = 0; k < 4; ++k)
                    {
                        int jID = jointList0.empty() ? (int)jointList1[w + k] : (int)jointList0[w + k];
                        if (jID < 0 || jID >= sd->joints.size())
                        {
                            OSG_WARN << "[LoaderGLTF] Invalid joint index " << jID
                                     << " for weight index " << (w + k) << std::endl;
                            continue;
                        }

                        osg::Node* n = _nodeCreationMap[sd->joints[jID]];
                        if (n && n->asGroup()) tList[k] = n->asGroup()->asTransform();
                        if (tList[k]) jwMap.push_back(JointWeightPair(tList[k], weightList[w + k]));
                        else OSG_WARN << "[LoaderGLTF] No joint with ID = " << sd->joints[jID]
                                      << " for weight index " << (w + k) << std::endl;
                    }
                    gjData._weightList.push_back(jwMap);
                }

                gjData._stateset = geom->getStateSet();
                sd->jointData[geom.get()] = gjData;
                sd->meshList.push_back(geom.get());
            }

            // Handle blendshapes
            for (size_t j = 0; j < primitive.targets.size(); ++j)
                createBlendshapeData(geom.get(), primitive.targets[j]);
        }

        bool withNames = mesh.extras.Has("targetNames");
        if (!mesh.weights.empty()) applyBlendshapeWeights(geode, mesh.weights,
            withNames ? mesh.extras.Get("targetNames") : tinygltf::Value());
        return true;
    }

	static osg::Texture2D* createDefaultTextureForColor(const osg::Vec4& color)
    {
        osg::ref_ptr<osg::Image> image = new osg::Image;
        image->allocateImage(1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE);
        image->setInternalTextureFormat(GL_RGBA);

        osg::Vec4ub* ptr = (osg::Vec4ub*)image->data();
        *ptr = osg::Vec4ub(color[0] * 255, color[1] * 255, color[2] * 255, color[3] * 255);

        osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
        tex2D->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::NEAREST);
        tex2D->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::NEAREST);
        tex2D->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT);
        tex2D->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT);
        tex2D->setImage(image.get()); return tex2D.release();
    }

    void LoaderGLTF::createMaterial(osg::StateSet* ss, tinygltf::Material material)
    {
        // Shininess(RGB) = Occlusion/Roughness/Metallic, Ambient = Occlusion
        int baseID = material.pbrMetallicRoughness.baseColorTexture.index;
        int roughnessID = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
        int normalID = material.normalTexture.index;
        int emissiveID = material.emissiveTexture.index;
        int occlusionID = material.occlusionTexture.index;

        if (baseID >= 0 && baseID < _modelDef.textures.size())
            createTexture(ss, 0, uniformNames[0], _modelDef.textures[baseID]);
        else
        {
            osg::Texture2D* tex2D = createDefaultTextureForColor(osg::Vec4(
                material.pbrMetallicRoughness.baseColorFactor[0], material.pbrMetallicRoughness.baseColorFactor[1],
                material.pbrMetallicRoughness.baseColorFactor[2], material.pbrMetallicRoughness.baseColorFactor[3]));
            if (tex2D) ss->setTextureAttributeAndModes(0, tex2D);
        }

        if (normalID >= 0) createTexture(ss, 1, uniformNames[1], _modelDef.textures[normalID]);
        if (roughnessID >= 0) createTexture(ss, 3, uniformNames[3], _modelDef.textures[roughnessID]);
        if (occlusionID >= 0) createTexture(ss, 4, uniformNames[4], _modelDef.textures[occlusionID]);
        if (emissiveID >= 0) createTexture(ss, 5, uniformNames[5], _modelDef.textures[emissiveID]);

        if (material.alphaMode.compare("BLEND") == 0)
            ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        else
            ss->setRenderingHint(osg::StateSet::OPAQUE_BIN);
    }

    void LoaderGLTF::createTexture(osg::StateSet* ss, int u,
                                   const std::string& name, tinygltf::Texture& tex)
    {
        if (tex.source < 0) tex.source = 0; if (tex.source >= _modelDef.images.size()) return;
        tinygltf::Image& imageSrc = _modelDef.images[tex.source];
        if (imageSrc.image.empty()) return;

        osg::ref_ptr<osg::Image> image2D = _imageMap[tex.source].get();
        if (!image2D)
        {
            //std::cout << name << ": " << imageSrc.uri << ", Size = "
            //          << imageSrc.width << "x" << imageSrc.height << "\n";
            osg::ref_ptr<osg::Image> image;
            if (imageSrc.width < 1 || imageSrc.height < 1)
            {
                std::stringstream dataIn(std::ios::in | std::ios::out | std::ios::binary);
                dataIn.write((char*)&imageSrc.image[0], imageSrc.image.size());
                if (imageSrc.mimeType.find("ktx") != std::string::npos)
                {
                    std::vector<osg::ref_ptr<osg::Image>> imageList = loadKtx2(dataIn, NULL);
                    if (!imageList.empty()) image = imageList[0];
                }
                else if (!imageSrc.extensions_json_string.empty())
                {
                    std::string ext = imageSrc.extensions_json_string;
                    osgDB::ReaderWriter* rw = osgDB::Registry::instance()->getReaderWriterForExtension(ext);
                    if (!rw) rw = osgDB::Registry::instance()->getReaderWriterForExtension("verse_" + ext);
                    if (rw) image = rw->readImage(dataIn).getImage();
                }
                if (!image) return;
            }
            else
            {
                GLenum format = GL_RGBA, type = GL_UNSIGNED_BYTE;
                if (imageSrc.bits == 16) type = GL_UNSIGNED_SHORT;
                if (imageSrc.component == 1) format = GL_RED;
                else if (imageSrc.component == 2) format = GL_RG;
                else if (imageSrc.component == 3) format = GL_RGB;

                image = new osg::Image; image->setFileName(imageSrc.uri);
                image->allocateImage(imageSrc.width, imageSrc.height, 1, format, type);
                switch (imageSrc.component)
                {
                case 1: image->setInternalTextureFormat(GL_R8); break;
                case 2: image->setInternalTextureFormat(GL_RG8); break;
                case 3: image->setInternalTextureFormat(GL_RGB8); break;
                default: image->setInternalTextureFormat(GL_RGBA8); break;
                }
                memcpy(image->data(), &imageSrc.image[0], image->getTotalSizeInBytes());
            }

            image2D = image.get(); _imageMap[tex.source] = image2D;
            image2D->setFileName(imageSrc.uri); image2D->setName(imageSrc.name);
            if (!imageSrc.name.empty())
                OSG_NOTICE << "[LoaderGLTF] " << imageSrc.name << " loaded for " << name << std::endl;
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

        size_t stride = (attrView.byteStride > 0 && attrView.byteStride != (compSize * 4 * sizeof(float)))
                      ? attrView.byteStride : 0;
        size_t offset = accessor.byteOffset + attrView.byteOffset;
        size_t matSize = compSize * 4; std::vector<float> matrices(size * matSize);
        copyBufferData(&matrices[0], &buffer.data[offset], matrices.size() * sizeof(float), stride, size);
        for (int i = 0; i < size; ++i)
        {
            osg::Matrixf matrix(&matrices[i * matSize]);
            for (size_t j = 0; j < sd.meshList.size(); ++j)
            {
                PlayerAnimation::GeometryJointData& jData = sd.jointData[sd.meshList[j]];
                jData._invBindPoseMap[bones[i]] = matrix;
                //std::cout << bones[i]->getName().c_str() << matrix;
            }
        }
    }

    void LoaderGLTF::createAnimationSampler(
            PlayerAnimation::AnimationData& anim, const std::string& path,
            tinygltf::Accessor& in, tinygltf::Accessor& out)
    {
        const tinygltf::BufferView& inView = _modelDef.bufferViews[in.bufferView];
        const tinygltf::BufferView& outView = _modelDef.bufferViews[out.bufferView];
        if (inView.buffer < 0 || outView.buffer < 0) return;

        const tinygltf::Buffer& inBuffer = _modelDef.buffers[inView.buffer];
        const tinygltf::Buffer& outBuffer = _modelDef.buffers[outView.buffer];
        int inCompSize = tinygltf::GetComponentSizeInBytes(in.componentType);
        int outCompSize = tinygltf::GetComponentSizeInBytes(out.componentType);
        size_t inSize = in.count, outSize = out.count;

        size_t inStride = (inView.byteStride > 0 && inView.byteStride != sizeof(float))
                        ? inView.byteStride : 0, inOffset = in.byteOffset + inView.byteOffset;
        std::vector<float> timeList(inSize), weightList;
        copyBufferData(&timeList[0], &inBuffer.data[inOffset],
                       inSize * sizeof(float), inStride, inSize);

        int outCompNum = (out.type != TINYGLTF_TYPE_SCALAR) ? out.type : 1;
        size_t outStride = (outView.byteStride > 0 && outView.byteStride != (outCompNum * outCompSize))
                         ? outView.byteStride : 0, outOffset = out.byteOffset + outView.byteOffset;
        std::vector<osg::Vec3> vec3List; std::vector<osg::Vec4> vec4List;
        switch (outCompNum)
        {
        case 1:
            if (outCompSize != 4)
            {
                OSG_WARN << "[LoaderGLTF] Unsupported component size " << outCompSize
                         << " for weight animation sampler" << std::endl;
                // TODO
            }
            else
            {
                weightList.resize(outSize);
                copyBufferData(&weightList[0], &outBuffer.data[outOffset],
                               outSize * sizeof(float), outStride, outSize);
            }
            break;
        case 3:
            vec3List.resize(outSize);
            copyBufferData(&vec3List[0], &outBuffer.data[outOffset],
                outSize * sizeof(osg::Vec3f), outStride, outSize); break;
        case 4:
            if (outCompSize != 4)
            {
                OSG_WARN << "[LoaderGLTF] Unsupported component size " << outCompSize
                         << " for rotation animation sampler" << std::endl;
                // TODO
            }
            else
            {
                vec4List.resize(outSize);
                copyBufferData(&vec4List[0], &outBuffer.data[outOffset],
                    outSize * sizeof(osg::Vec4f), outStride, outSize);
            }
            break;
        default:
            OSG_WARN << "[LoaderGLTF] Unsupported animation sampler " << out.name << " with "
                     << outCompNum << "-components and dataSize=" << outCompSize << std::endl;
            break;
        }

        if (!weightList.empty())
        {
            for (size_t i = 0; i < timeList.size(); ++i)
            {
                float w = (i < weightList.size()) ? weightList[i] : weightList.back();
                anim._morphFrames.push_back(std::pair<float, float>(timeList[i], w));
            }
            std::sort(anim._morphFrames.begin(), anim._morphFrames.end(),
                      [](std::pair<float, float>& l, std::pair<float, float>& r) { return l.first < r.first; });
        }
        else if (!vec3List.empty())
        {
            std::vector<std::pair<float, osg::Vec3>>& frames =
                (path.find("scale") != path.npos) ? anim._scaleFrames : anim._positionFrames;
            for (size_t i = 0; i < timeList.size(); ++i)
            {
                const osg::Vec3& w = (i < vec3List.size()) ? vec3List[i] : vec3List.back();
                frames.push_back(std::pair<float, osg::Vec3>(timeList[i], w));
            }
            std::sort(frames.begin(), frames.end(),
                      [](std::pair<float, osg::Vec3>& l, std::pair<float, osg::Vec3>& r) { return l.first < r.first; });
        }
        else if (!vec4List.empty())
        {
            for (size_t i = 0; i < timeList.size(); ++i)
            {
                const osg::Vec4& w = (i < vec4List.size()) ? vec4List[i] : vec4List.back();
                anim._rotationFrames.push_back(std::pair<float, osg::Vec4>(timeList[i], w));
            }
            std::sort(anim._rotationFrames.begin(), anim._rotationFrames.end(),
                      [](std::pair<float, osg::Vec4>& l, std::pair<float, osg::Vec4>& r) { return l.first < r.first; });
        }
    }

    void LoaderGLTF::createBlendshapeData(osg::Geometry* geom, std::map<std::string, int>& target)
    {
        osg::Vec3Array *va = NULL, *na = NULL; osg::Vec4Array *ta = NULL;
        for (std::map<std::string, int>::iterator attrib = target.begin();
             attrib != target.end(); ++attrib)
        {
            tinygltf::Accessor& attrAccessor = _modelDef.accessors[attrib->second];
            const tinygltf::BufferView& attrView = _modelDef.bufferViews[attrAccessor.bufferView];
            int size = attrAccessor.count; if (!size || attrView.buffer < 0) continue;

            const tinygltf::Buffer& buffer = _modelDef.buffers[attrView.buffer];
            int compNum = (attrAccessor.type != TINYGLTF_TYPE_SCALAR) ? attrAccessor.type : 1;
            int compSize = tinygltf::GetComponentSizeInBytes(attrAccessor.componentType);
            int copySize = size * (compSize * compNum);
            size_t offset = attrView.byteOffset + attrAccessor.byteOffset;
            size_t stride = (attrView.byteStride > 0 && attrView.byteStride != (compSize * compNum))
                          ? attrView.byteStride : 0;

            if (attrib->first.compare("POSITION") == 0 && compSize == 4 && compNum == 3)
            {
                va = new osg::Vec3Array(size);
                copyBufferData(&(*va)[0], &buffer.data[offset], copySize, stride, size);
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
                va->setNormalize(attrAccessor.normalized);
#endif
            }
            else if (attrib->first.compare("NORMAL") == 0 && compSize == 4 && compNum == 3)
            {
                na = new osg::Vec3Array(size);
                copyBufferData(&(*na)[0], &buffer.data[offset], copySize, stride, size);
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
                na->setNormalize(attrAccessor.normalized);
#endif
            }
            else if (attrib->first.compare("TANGENT") == 0 &&
                     compSize == 4 && compNum == 4)
            {   // Do nothing as we calculate tangent/binormal by ourselves
#if 0
                ta = new osg::Vec4Array(size);
                copyBufferData(&(*ta)[0], &buffer.data[offset], copySize, stride, size);
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
                ta->setNormalize(attrAccessor.normalized);
#endif
#endif
            }
            else
                OSG_WARN << "[LoaderGLTF] Unsupported target " << attrib->first << " with "
                         << compNum << "-components and dataSize=" << compSize << std::endl;
        }

        // Save targets to specified geometry callback
        BlendShapeAnimation* bsa = dynamic_cast<BlendShapeAnimation*>(geom->getUpdateCallback());
        if (!bsa)
        {
            bsa = new BlendShapeAnimation; bsa->setName(geom->getName() + "BsCallback");
            geom->setUpdateCallback(bsa);
        }

        BlendShapeAnimation::BlendShapeData* bsd = new BlendShapeAnimation::BlendShapeData;
        bsd->vertices = va; bsd->normals = na; bsd->tangents = ta;
        bsa->addBlendShapeData(bsd);
    }

    void LoaderGLTF::applyBlendshapeWeights(osg::Geode* geode, const std::vector<double>& weights,
                                            const tinygltf::Value& targetNames)
    {
        std::vector<std::string> names;
        if (targetNames.IsArray())
        {
            for (size_t i = 0; i < targetNames.ArrayLen(); ++i)
                names.push_back(targetNames.Get(i).Get<std::string>());
        }

        // Save names and weights to specified geometry callback
        for (unsigned int i = 0; i < geode->getNumDrawables(); ++i)
        {
            BlendShapeAnimation* bsa = dynamic_cast<BlendShapeAnimation*>(
                geode->getDrawable(i)->getUpdateCallback());
            if (bsa) bsa->apply(names, weights);
        }
    }

    osg::ref_ptr<osg::Group> loadGltf(const std::string& file, bool isBinary)
    {
        std::string workDir = osgDB::getFilePath(file), http = osgDB::getServerProtocol(file);
        if (!http.empty()) return NULL;
        std::ifstream in(file.c_str(), std::ios::in | std::ios::binary);
        if (!in)
        {
            OSG_WARN << "[LoaderGLTF] file " << file << " not readable" << std::endl;
            return NULL;
        }

        osg::ref_ptr<LoaderGLTF> loader = new LoaderGLTF(in, workDir, isBinary);
        if (loader->getRoot()) loader->getRoot()->setName(file);
        return loader->getRoot();
    }

    osg::ref_ptr<osg::Group> loadGltf2(std::istream& in, const std::string& dir, bool isBinary)
    {
        osg::ref_ptr<LoaderGLTF> loader = new LoaderGLTF(in, dir, isBinary);
        return loader->getRoot();
    }
}
