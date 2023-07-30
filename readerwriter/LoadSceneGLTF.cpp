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

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "LoadSceneGLTF.h"
#include "Utilities.h"

#ifndef GL_ARB_texture_rg
    #define GL_RG                             0x8227
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

        const tinygltf::Scene& defScene = _modelDef.scenes[_modelDef.defaultScene];
        for (size_t i = 0; i < defScene.nodes.size(); ++i)
        {
            osg::ref_ptr<osg::Node> child = createNode(_modelDef.nodes[defScene.nodes[i]]);
            if (child.valid()) _root->addChild(child.get());
        }
    }

    osg::Node* LoaderGLTF::createNode(tinygltf::Node& node)
    {
        osg::ref_ptr<osg::Geode> geode = (node.mesh >= 0) ? new osg::Geode : NULL;
        if (geode.valid()) createMesh(geode.get(), _modelDef.meshes[node.mesh]);  // TODO: skin
        if (node.matrix.empty() && node.children.empty()) return geode.release();

        osg::ref_ptr<osg::MatrixTransform> group = new osg::MatrixTransform;
        for (size_t i = 0; i < node.children.size(); ++i)
        {
            osg::ref_ptr<osg::Node> child = createNode(_modelDef.nodes[node.children[i]]);
            if (child.valid()) group->addChild(child.get());
        }

        if (geode.valid()) group->addChild(geode.get());
        if (!node.matrix.empty()) group->setMatrix(osg::Matrix(&node.matrix[0]));
        return group.release();
    }

    bool LoaderGLTF::createMesh(osg::Geode* geode, tinygltf::Mesh mesh)
    {
        for (size_t i = 0; i < mesh.primitives.size(); ++i)
        {
            osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
            geom->setUseDisplayList(false);
            geom->setUseVertexBufferObjects(true);

            tinygltf::Primitive primitive = mesh.primitives[i];
            for (auto& attrib : primitive.attributes)
            {
                tinygltf::Accessor attrAccessor = _modelDef.accessors[attrib.second];
                const tinygltf::BufferView& attrView = _modelDef.bufferViews[attrAccessor.bufferView];
                if (attrView.buffer < 0) continue;

                const tinygltf::Buffer& buffer = _modelDef.buffers[attrView.buffer];
                int compNum = (attrAccessor.type != TINYGLTF_TYPE_SCALAR) ? attrAccessor.type : 1;
                int compSize = tinygltf::GetComponentSizeInBytes(attrAccessor.componentType);
                int size = attrAccessor.count; if (!size) continue;

                int copySize = size * (compSize * compNum);
                //std::cout << attrib.first << ": Size = " << size << ", Components = " << compNum
                //          << ", ComponentBytes = " << compSize << std::endl;
                if (attrib.first.compare("POSITION") == 0 && compSize == 4 && compNum == 3)
                {
                    osg::Vec3Array* va = new osg::Vec3Array(size);
                    memcpy(&(*va)[0], &buffer.data[attrView.byteOffset], copySize);
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
                    va->setNormalize(attrAccessor.normalized);
#endif
                    geom->setVertexArray(va);
                }
                else if (attrib.first.compare("NORMAL") == 0 && compSize == 4 && compNum == 3)
                {
                    osg::Vec3Array* na = new osg::Vec3Array(size);
                    memcpy(&(*na)[0], &buffer.data[attrView.byteOffset], copySize);
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
                    memcpy(&(*ta)[0], &buffer.data[attrView.byteOffset], copySize);
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
                    memcpy(&(*ta)[0], &buffer.data[attrView.byteOffset], copySize);
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
                    ta->setNormalize(attrAccessor.normalized);
#endif
                    geom->setTexCoordArray(atoi(attrib.first.substr(9).c_str()), ta);
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
            else
            {
                const tinygltf::Buffer& indexBuffer = _modelDef.buffers[indexView.buffer];
                int compSize = tinygltf::GetComponentSizeInBytes(indexAccessor.componentType);
                int size = indexAccessor.count; if (!size) continue;

                switch (compSize)
                {
                case 1:
                    {
                        osg::DrawElementsUByte* de = new osg::DrawElementsUByte(GL_POINTS, size); p = de;
                        memcpy(&(*de)[0], &indexBuffer.data[indexView.byteOffset], size * compSize);
                    }
                    break;
                case 2:
                    {
                        osg::DrawElementsUShort* de = new osg::DrawElementsUShort(GL_POINTS, size); p = de;
                        memcpy(&(*de)[0], &indexBuffer.data[indexView.byteOffset], size * compSize);
                    }
                    break;
                case 4:
                    {
                        osg::DrawElementsUInt* de = new osg::DrawElementsUInt(GL_POINTS, size); p = de;
                        memcpy(&(*de)[0], &indexBuffer.data[indexView.byteOffset], size * compSize);
                    }
                    break;
                default: OSG_WARN << "[LoaderGLTF] Unknown size " << compSize << std::endl; continue;
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
            default: OSG_WARN << "[LoaderGLTF] Unknown type " << primitive.mode << std::endl; continue;
            }

            geom->addPrimitiveSet(p.get());
            geode->addDrawable(geom.get());
            if (primitive.material >= 0)
            {
                tinygltf::Material& material = _modelDef.materials[primitive.material];
                createMaterial(geom->getOrCreateStateSet(), material);
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
