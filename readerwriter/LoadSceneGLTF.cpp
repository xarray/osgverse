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

#include "modeling/Utilities.h"
#include "modeling/GaussianGeometry.h"
#include "animation/TweenAnimation.h"
#include "animation/BlendShapeAnimation.h"
#include "pipeline/Utilities.h"
#include "MaterialGraph.h"
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
#include "LoadSceneGLTF.h"
#include "Utilities.h"

namespace osgVerse
{
    static std::set<std::string> g_extensions =
    {
        // "KHR_audio",
        // "KHR_lights_punctual",
        // "MSFT_lod",
#ifdef VERSE_USE_DRACO
        "KHR_draco_mesh_compression",
#endif
        "EXT_texture_webp",
        "KHR_texture_basisu",
        "MSFT_texture_dds",
        "KHR_materials_specular",
        "KHR_materials_unlit",
        "KHR_gaussian_splatting",
        "KHR_gaussian_splatting_compression_spz_2"
    };

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

static float linearToSRGB(float x)
{
    if (x <= 0.0031308f) return 12.92f * x;
    else return 1.055f * pow(x, 1.0f / 2.4f) - 0.055f;
}

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
        bool succeed = wf->httpGet(fileName);
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

    static bool GetFileSizeInBytes(size_t* filesize_out, std::string* err,
                                   const std::string& filepath, void* userData)
    {
        osgDB::ReaderWriter* rw = (osgDB::ReaderWriter*)userData;
        if (rw) { filesize_out = 0; return true; }
        return tinygltf::GetFileSizeInBytes(filesize_out, err, filepath, userData);
    }

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
                    cValues[0].get<double>(), cValues[1].get<double>(), cValues[2].get<double>());
            }
        }
        return osg::Vec3d();
    }

    unsigned int ReadB3dmHeader(std::vector<char>& data, osg::Vec3d* rtcCenter = NULL)
    {
        // https://github.com/CesiumGS/3d-tiles/blob/main/specification/TileFormats/Batched3DModel/README.adoc#tileformats-batched3dmodel-batched-3d-model
        // magic(h0) + version(h1) + length(h2) + featureTableJsonLength(h3) + featureTableBinLength(h4) +
        // batchTableJsonLength(h5) + batchTableBinLength(h6) + <Real feature table> + <Real batch table> + GLTF body
        int header[7], hSize = 7 * sizeof(int); memcpy(header, data.data(), hSize);
        if (rtcCenter && header[3] > 0) *rtcCenter = ReadRtcCenterFeatureTable(data, hSize, header[3]);

        int extraSize = header[3] + header[4] + header[5] + header[6];
        if (hSize + extraSize >= header[2]) extraSize = 0;  // unexpected behaviour
        return hSize + extraSize;
    }

    unsigned int ReadI3dmHeader(std::vector<char>& data, unsigned int& format)
    {
        // https://github.com/CesiumGS/3d-tiles/blob/main/specification/TileFormats/Instanced3DModel/README.adoc#tileformats-instanced3dmodel-instanced-3d-model
        // magic(h0) + version(h1) + length(h2) + featureTableJsonLength(h3) + featureTableBinLength(h4) +
        // batchTableJsonLength(h5) + batchTableBinLength(h6) + gltfFormat(h7) +
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
                    std::stringstream dataIn(std::ios::in | std::ios::out | std::ios::binary);
                    dataIn.write((char*)bytes, size);

                    osg::ref_ptr<osg::Image> img = rw->readImage(dataIn).getImage();
                    if (img.valid())
                    {
                        size_t s = img->getTotalSizeInBytes(); w = img->s(); h = img->t();
                        comp = osg::Image::computeNumComponents(img->getPixelFormat());
                        data = (unsigned char*)stbi__malloc(s);
                        std::copy(img->data(), img->data() + s, data);
                    }
                }
            }

            if (!data)
            {
                if (err) (*err) += "Unknown image format. Failed to decode image: " + image->name +
                                   ", ID = " + std::to_string(image_idx) + ", Mimetype = " + image->mimeType +
                                   ", Size = " + std::to_string(size) + "\n";
                return false;
            }
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

    LoaderGLTF::LoaderGLTF(std::istream& in, const std::string& d, bool isBinary,
                           int pbr, bool yUp) : _usingMaterialPBR(pbr), _3dtilesFormat(false)
    {
        std::string protocol = osgDB::getServerProtocol(d);
        osgDB::ReaderWriter* rwWeb = (protocol.empty()) ? NULL
                                   : osgDB::Registry::instance()->getReaderWriterForExtension("verse_web");
        tinygltf::FsCallbacks fs = {
            &osgVerse::FileExists, &tinygltf::ExpandFilePath,
            &osgVerse::ReadWholeFile, &tinygltf::WriteWholeFile,
            &osgVerse::GetFileSizeInBytes, rwWeb };

        std::string err, warn; bool loaded = false;
        std::istreambuf_iterator<char> eos; osg::Vec3d rtcCenter;
        std::vector<char> data(std::istreambuf_iterator<char>(in), eos);
        if (data.empty()) { OSG_WARN << "[LoaderGLTF] Unable to read from stream\n"; return; }

        tinygltf::TinyGLTF loader;
        loader.SetParseStrictness(tinygltf::Permissive);
        loader.SetStoreOriginalJSONForExtrasAndExtensions(true);
        loader.SetImageLoader(&LoadImageDataEx, this);
        loader.SetFsCallbacks(fs, &err);
        if (!err.empty()) OSG_WARN << "[LoaderGLTF] SetFsCallbacks: " << err << std::endl;

        if (isBinary)
        {
            unsigned int version = 2, offset = 0, format = 0;  // 0: url, 1: raw GLTF
            std::string externalFileURI;
            if (data.size() > 4)
            {
                if (data[0] == 'b' && data[1] == '3' && data[2] == 'd' && data[3] == 'm')
                {
                    offset = ReadB3dmHeader(data, &rtcCenter); _3dtilesFormat = true;
                    memcpy(&version, &data[0] + offset + 4, 4); tinygltf::swap4(&version);
                }
                else if (data[0] == 'i' && data[1] == '3' && data[2] == 'd' && data[3] == 'm')
                {
                    offset = ReadI3dmHeader(data, format); _3dtilesFormat = true;
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
        
        if (!_modelDef.extensionsRequired.empty() || !_modelDef.extensionsUsed.empty())
        {
            OSG_INFO << "[LoaderGLTF] Found GLTF extensions: \n";
            for (size_t i = 0; i < _modelDef.extensionsUsed.size(); ++i)
            {
                std::string ex = _modelDef.extensionsUsed[i]; bool ok = (g_extensions.find(ex) != g_extensions.end());
                OSG_INFO << "  - " << ex << ": " << ok << "\n"; if (!ok) warn += " " + ex + " not supported.";
            }
        }

        if (!err.empty()) OSG_WARN << "[LoaderGLTF] Errors found: " << err << std::endl;
        if (!warn.empty()) OSG_WARN << "[LoaderGLTF] Warnings found: " << warn << std::endl;
        if (!loaded) { OSG_WARN << "[LoaderGLTF] Unable to load GLTF scene" << std::endl; return; }
        _root = new osg::MatrixTransform;

        // Preload skin data
        for (size_t i = 0; i < _modelDef.skins.size(); ++i)
        {
            const tinygltf::Skin& skin = _modelDef.skins[i];
            SkinningData sd; sd.skeletonBaseIndex = skin.skeleton;
            sd.invBindPoseAccessor = skin.inverseBindMatrices;
            sd.player = new PlayerAnimation; sd.player->setName(skin.name);
            sd.player->setModelRoot(_root.get());
            sd.joints.assign(skin.joints.begin(), skin.joints.end());
            _skinningDataList.push_back(sd);
        }

        // Read and construct scene graph
		if (_modelDef.defaultScene < 0) _modelDef.defaultScene = 0;
        if (_modelDef.scenes.size() <= _modelDef.defaultScene) return;

        const tinygltf::Scene& defScene = _modelDef.scenes[_modelDef.defaultScene];
        for (size_t i = 0; i < defScene.nodes.size(); ++i)
        {
            osg::ref_ptr<osg::Node> child = createNode(
                defScene.nodes[i], _modelDef.nodes[defScene.nodes[i]]);
            if (child.valid()) _root->addChild(child.get());
            _root->addDescription(defScene.extras_json_string);
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

        // Update Y-to-Z and RTC center transformations
        osg::ref_ptr<osg::MatrixTransform> rtcRoot;
        if (rtcCenter.length2() > 0.0)
        {
            rtcRoot = new osg::MatrixTransform;
            rtcRoot->setMatrix(osg::Matrix::translate(rtcCenter));
        }
        else if (!_modelDef.extensions.empty() &&
                 _modelDef.extensions.find("CESIUM_RTC") != _modelDef.extensions.end())
        {
            if (_modelDef.extensions["CESIUM_RTC"].Has("center"))
            {
                const tinygltf::Value& center = _modelDef.extensions["CESIUM_RTC"].Get("center");
                if (center.IsArray())
                {
                    const tinygltf::Value::Array& cData = center.Get<tinygltf::Value::Array>();
                    if (cData.size() > 2) rtcCenter.set(
                        cData[0].GetNumberAsDouble(), cData[1].GetNumberAsDouble(), cData[2].GetNumberAsDouble());
                    rtcRoot = new osg::MatrixTransform;
                    rtcRoot->setMatrix(osg::Matrix::translate(rtcCenter));
                }
            }
        }

        if (!_modelDef.extras_json_string.empty()) _root->addDescription(_modelDef.extras_json_string);
        if (yUp) _root->setMatrix(osg::Matrix::rotate(osg::Y_AXIS, osg::Z_AXIS));
        if (rtcRoot.valid()) { rtcRoot->addChild(_root.get()); _root = rtcRoot; }

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
                
                osg::Group* g = (itr->first) ? itr->first->asGroup() : NULL;
                osg::Transform* t = g ? g->asTransform() : NULL; if (!t) continue;
                if (belongsToSkeleton >= 0)
                    skeletonAnimMap[t] = playerAnim;
                else
                {   // non-skeleton animations
                    TweenAnimation* tween = dynamic_cast<TweenAnimation*>(t->getUpdateCallback());
                    if (!tween) { tween = new TweenAnimation; t->addUpdateCallback(tween); }
                    tween->addAnimation(animName, playerAnim.toAnimationPath());
                }
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
            if (!node.extras_json_string.empty()) geode->addDescription(node.extras_json_string);
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
            struct GaussianPreparedData
            {
                osg::ref_ptr<osg::Vec4Array> quat, red[4], green[4], blue[4];
                osg::ref_ptr<osg::Vec3Array> scale;
                osg::ref_ptr<osg::FloatArray> alpha;
                int shDegrees; bool enabled;
                GaussianPreparedData() : shDegrees(0), enabled(false) {}
            } gsData;

            struct SkeletonPreparedData
            {
                std::vector<unsigned char> jointList0;
                std::vector<unsigned short> jointList1;
                std::vector<float> weightList;
            } skData;

            std::vector<unsigned char> bufferData; std::map<std::string, int> extBufferViews;
            tinygltf::Primitive primitive = mesh.primitives[i];
            for (std::map<std::string, tinygltf::Value>::iterator it = primitive.extensions.begin();
                 it != primitive.extensions.end(); ++it)
            {
                const std::string& ext = it->first;
                std::vector<std::string> keys = it->second.Keys();
                if (ext == "KHR_gaussian_splatting") gsData.enabled = true;

                for (size_t i = 0; i < keys.size(); ++i)
                {
                    if (keys[i] == "extensions")
                    {
                        const tinygltf::Value& val = it->second.Get("extensions");
                        std::string subExt = "KHR_gaussian_splatting_compression_spz_2";
                        if (val.Has(subExt))
                        {
                            const tinygltf::Value& spz = val.Get(subExt);
                            if (spz.Has("bufferView")) extBufferViews[subExt] = spz.Get("bufferView").GetNumberAsInt();
                        }
                    }
                    else {}
                }
            }

            osg::ref_ptr<osg::Geometry> geom = gsData.enabled ? new GaussianGeometry : new osg::Geometry;
            if (gsData.enabled) static_cast<GaussianGeometry*>(geom.get())->setShDegrees(gsData.shDegrees);
            geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
            geom->setName(mesh.name + "_" + std::to_string(i));
            if (primitive.material >= 0)
            {
                tinygltf::Material& material = _modelDef.materials[primitive.material];
                createMaterial(geom->getOrCreateStateSet(), material);  // add material
            }

            if (gsData.enabled && extBufferViews.find("KHR_gaussian_splatting_compression_spz_2") != extBufferViews.end())
            {
                osg::ref_ptr<osg::Geometry> geom2 =
                    createFromExtGaussianSplattingSPZ2(mesh.name, extBufferViews["KHR_gaussian_splatting_compression_spz_2"]);
                if (geom2.valid()) { geom2->setName(geom->getName()); geom = geom2; }
                geode->addDrawable(geom.get()); continue;
            }

            for (std::map<std::string, int>::iterator attrib = primitive.attributes.begin();
                 attrib != primitive.attributes.end(); ++attrib)
            {
                tinygltf::Accessor& attrAccessor = _modelDef.accessors[attrib->second];
                int size = attrAccessor.count; if (!size) continue;
                int compNum = (attrAccessor.type != TINYGLTF_TYPE_SCALAR) ? attrAccessor.type : 1;
                int compSize = tinygltf::GetComponentSizeInBytes(attrAccessor.componentType);
                int copySize = size * (compSize * compNum);

                size_t offset = 0, stride = 0;
                if (attrAccessor.bufferView >= 0)
                {
                    const tinygltf::BufferView& attrView = _modelDef.bufferViews[attrAccessor.bufferView];
                    if (attrView.buffer >= 0)
                    {
                        const tinygltf::Buffer& buffer = _modelDef.buffers[attrView.buffer];
                        bufferData = buffer.data;
                    }
                    offset = attrView.byteOffset + attrAccessor.byteOffset;
                    stride = (attrView.byteStride > 0 && attrView.byteStride != (compSize * compNum))
                           ? attrView.byteStride : 0;
                }
                else
                    {}  // TODO: process certain extension buffer?

                if (bufferData.empty())
                    { OSG_WARN << "[LoaderGLTF] No data buffer for " << attrib->first << std::endl; continue; }
                //std::cout << attrib->first << ": Size = " << size << ", Components = " << compNum
                //          << ", ComponentBytes = " << compSize << std::endl;

                if (attrib->first.compare("POSITION") == 0 && compSize == 4 && compNum == 3)
                {
                    osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array(size);
                    copyBufferData(&(*va)[0], &bufferData[offset], copySize, stride, size);
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
                    va->setNormalize(attrAccessor.normalized);
#endif
                    if (!gsData.enabled) geom->setVertexArray(va.get());
                    else static_cast<GaussianGeometry*>(geom.get())->setPosition(va.get());
                }
                else if (attrib->first.compare("NORMAL") == 0 && compSize == 4 && compNum == 3)
                {
                    osg::ref_ptr<osg::Vec3Array> na = new osg::Vec3Array(size);
                    copyBufferData(&(*na)[0], &bufferData[offset], copySize, stride, size);
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
                    na->setNormalize(attrAccessor.normalized);
#endif
                    if (!gsData.enabled) geom->setNormalArray(na.get());
                    geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
                }
                else if (attrib->first.compare("TANGENT") == 0 && compSize == 4 && compNum == 4)
                {
                    osg::ref_ptr<osg::Vec4Array> ta = new osg::Vec4Array(size);
                    copyBufferData(&(*ta)[0], &bufferData[offset], copySize, stride, size);
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
                    ta->setNormalize(attrAccessor.normalized);
#endif
                    if (!gsData.enabled) geom->setVertexAttribArray(6, ta.get());
                    geom->setVertexAttribBinding(6, osg::Geometry::BIND_PER_VERTEX);
                }
                else if (attrib->first.find("COLOR") == 0 && compNum == 4)
                {
                    osg::ref_ptr<osg::Array> ca;
                    if (compSize == 1)
                    {
                        osg::Vec4ubArray* ca4ub = new osg::Vec4ubArray(size); ca = ca4ub;
                        copyBufferData(&(*ca4ub)[0], &bufferData[offset], copySize, stride, size);
                    }
                    else if (compSize == 4)
                    {
                        osg::Vec4Array* ca4f = new osg::Vec4Array(size); ca = ca4f;
                        copyBufferData(&(*ca4f)[0], &bufferData[offset], copySize, stride, size);
                    }

                    if (ca.valid())
                    {
                        if (gsData.enabled)
                        {
                            gsData.alpha = new osg::FloatArray(size);
                            if (!gsData.red[0]) gsData.red[0] = new osg::Vec4Array(size);
                            if (!gsData.green[0]) gsData.green[0] = new osg::Vec4Array(size);
                            if (!gsData.blue[0]) gsData.blue[0] = new osg::Vec4Array(size);

                            if (compSize == 1)
                            {
                                osg::Vec4ubArray* ca4ub = static_cast<osg::Vec4ubArray*>(ca.get());
                                for (size_t c = 0; c < size; ++c)
                                {
                                    const osg::Vec4ub& v = (*ca4ub)[c]; (*gsData.alpha)[c] = v.a() / 255.0f;
                                    (*(gsData.red[0]))[c] = osg::Vec4(v[0] / 255.0f, 0.0f, 0.0f, 0.0f);
                                    (*(gsData.green[0]))[c] = osg::Vec4(v[1] / 255.0f, 0.0f, 0.0f, 0.0f);
                                    (*(gsData.blue[0]))[c] = osg::Vec4(v[2] / 255.0f, 0.0f, 0.0f, 0.0f);
                                }
                            }
                            else if (compSize == 4)
                            {
                                osg::Vec4Array* ca4f = static_cast<osg::Vec4Array*>(ca.get());
                                for (size_t c = 0; c < size; ++c)
                                {
                                    const osg::Vec4& v = (*ca4f)[c]; (*gsData.alpha)[c] = v.a();
                                    (*(gsData.red[0]))[c] = osg::Vec4(v[0], 0.0f, 0.0f, 0.0f);
                                    (*(gsData.green[0]))[c] = osg::Vec4(v[1], 0.0f, 0.0f, 0.0f);
                                    (*(gsData.blue[0]))[c] = osg::Vec4(v[2], 0.0f, 0.0f, 0.0f);
                                }
                            }
                        }
                        else
                        {
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
                            ca->setNormalize(attrAccessor.normalized);
#endif
                            if (!gsData.enabled) geom->setColorArray(ca.get());
                            geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
                        }
                    }
                }
                else if (attrib->first.find("TEXCOORD_") != std::string::npos && compSize == 4 && compNum == 2)
                {
                    int texUnit = atoi(attrib->first.substr(9).c_str());
                    if (texUnit > 7)
                    {
                        OSG_WARN << "[LoaderGLTF] Tex-coord index too large and will be ignored: "
                                 << attrib->first << std::endl;
                    }
                    else
                    {
                        osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array(size);
                        copyBufferData(&(*ta)[0], &bufferData[offset], copySize, stride, size);
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
                        ta->setNormalize(attrAccessor.normalized);
#endif
                        if (!gsData.enabled) geom->setTexCoordArray(texUnit, ta.get());
                    }
                }
                else if (attrib->first.find("JOINTS_") != std::string::npos && compNum == 4)
                {
                    int jID = atoi(attrib->first.substr(7).c_str());
                    if (jID == 0)  // FIXME: joints group > 0?
                    {
                        if (compSize == 1)
                        {
                            skData.jointList0.resize(size * compNum);
                            copyBufferData(&skData.jointList0[0], &bufferData[offset], copySize, stride, size);
                        }
                        else
                        {
                            skData.jointList1.resize(size * compNum);
                            copyBufferData(&skData.jointList1[0], &bufferData[offset], copySize, stride, size);
                        }
                    }
                }
                else if (attrib->first.find("WEIGHTS_") != std::string::npos && compSize == 4 && compNum == 4)
                {
                    int wID = atoi(attrib->first.substr(8).c_str());
                    if (wID == 0)  // FIXME: weights group > 0?
                    {
                        skData.weightList.resize(size * compNum);
                        copyBufferData(&skData.weightList[0], &bufferData[offset], copySize, stride, size);
                    }
                }
                else if (attrib->first.compare("_ROTATION") == 0 && compSize == 4 && compNum == 4)
                {
                    gsData.quat = new osg::Vec4Array(size);
                    copyBufferData(&(*gsData.quat)[0], &bufferData[offset], copySize, stride, size);
                }
                else if (attrib->first.compare("_SCALE") == 0 && compSize == 4 && compNum == 3)
                {
                    gsData.scale = new osg::Vec3Array(size);
                    copyBufferData(&(*gsData.scale)[0], &bufferData[offset], copySize, stride, size);
                }
                else if (attrib->first.compare("_BATCHID") == 0 && compSize == 2 && compNum == 1)
                {
                    // TODO: read from b3dm batch table?
                }
                else
                {
                    OSG_WARN << "[LoaderGLTF] Unsupported attribute " << attrib->first << " with "
                             << compNum << "-components and dataSize=" << compSize << std::endl;
                }
            }  // for (std::map<std::string, int>::iterator attrib ...)

            osg::Vec3Array* va = NULL;
            if (gsData.enabled)
            {
                GaussianGeometry* gaussian = static_cast<GaussianGeometry*>(geom.get());
                if (gsData.scale.valid() && gsData.quat.valid() && gsData.alpha.valid())
                    gaussian->setScaleAndRotation(gsData.scale.get(), gsData.quat.get(), gsData.alpha.get());
                for (int k = 0; k < 4; ++k)
                {
                    if (gsData.red[k].valid()) gaussian->setShRed(k, gsData.red[k].get());
                    if (gsData.green[k].valid()) gaussian->setShGreen(k, gsData.green[k].get());
                    if (gsData.blue[k].valid()) gaussian->setShBlue(k, gsData.blue[k].get());
                }
            }
            else
            {
                va = static_cast<osg::Vec3Array*>(geom->getVertexArray());
                if (!va || (va && va->empty())) continue;
            }

            // Configure primitive index array
            osg::ref_ptr<osg::PrimitiveSet> p;
            if (primitive.indices < 0)
                p = new osg::DrawArrays(GL_POINTS, 0, va ? va->size() : 0);
            else
            {
                tinygltf::Accessor indexAccessor = _modelDef.accessors[primitive.indices];
                const tinygltf::BufferView& indexView = _modelDef.bufferViews[indexAccessor.bufferView];
                if (indexView.target == 0)
                    p = new osg::DrawArrays(GL_POINTS, 0, va ? va->size() : 0);
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
            if (!gsData.enabled) geom->addPrimitiveSet(p.get());
            else static_cast<GaussianGeometry*>(geom.get())->finalize();
            geode->addDrawable(geom.get());

            // Handle skinning data
            if (sd != NULL && !skData.weightList.empty())
            {
                typedef std::pair<osg::Transform*, float> JointWeightPair;
                PlayerAnimation::GeometryJointData gjData;
                for (size_t w = 0; w < skData.weightList.size(); w += 4)
                {
                    PlayerAnimation::GeometryJointData::JointWeights jwMap;
                    osg::Transform* tList[4] = { NULL };
                    for (int k = 0; k < 4; ++k)
                    {
                        int jID = skData.jointList0.empty() ? (int)skData.jointList1[w + k]
                                                            : (int)skData.jointList0[w + k];
                        if (jID < 0 || jID >= sd->joints.size())
                        {
                            OSG_WARN << "[LoaderGLTF] Invalid joint index " << jID
                                     << " for weight index " << (w + k) << std::endl;
                            continue;
                        }

                        osg::Node* n = _nodeCreationMap[sd->joints[jID]];
                        if (n && n->asGroup()) tList[k] = n->asGroup()->asTransform();
                        if (tList[k]) jwMap.push_back(JointWeightPair(tList[k], skData.weightList[w + k]));
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
        }  // for (size_t i = 0; i < mesh.primitives.size(); ++i)

        bool withNames = mesh.extras.Has("targetNames");
        if (!mesh.weights.empty()) applyBlendshapeWeights(geode, mesh.weights,
            withNames ? mesh.extras.Get("targetNames") : tinygltf::Value());
        return true;
    }

	static osg::Texture2D* createDefaultTextureForColor(const osg::Vec4& color)
    {
        osg::ref_ptr<osg::Image> image = new osg::Image;
        image->allocateImage(1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE);
        image->setInternalTextureFormat(GL_RGBA8);

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
            ss->setTextureAttributeAndModes(0, createTexture(uniformNames[0], _modelDef.textures[baseID]));
        else
        {
            osg::Vec4 baseColor(linearToSRGB(material.pbrMetallicRoughness.baseColorFactor[0]),
                                linearToSRGB(material.pbrMetallicRoughness.baseColorFactor[1]),
                                linearToSRGB(material.pbrMetallicRoughness.baseColorFactor[2]),
                                material.pbrMetallicRoughness.baseColorFactor[3]);
            osg::Texture2D* tex2D = createDefaultTextureForColor(baseColor);
            if (tex2D) ss->setTextureAttributeAndModes(0, tex2D);
        }

        if (normalID >= 0)
            ss->setTextureAttributeAndModes(1, createTexture(uniformNames[1], _modelDef.textures[normalID]));
        if (_usingMaterialPBR > 1 || (normalID >= 0 && _usingMaterialPBR > 0))
        {
            // Load or create Occlusion-Roughnes-Metallic texture
            std::pair<int, int> ormKey(occlusionID, roughnessID);
            if (_ormImageMap.find(ormKey) != _ormImageMap.end() && (occlusionID >= 0 || roughnessID >= 0))
            {
                osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
                tex2D->setResizeNonPowerOfTwoHint(false);
                tex2D->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT);
                tex2D->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT);

                osg::Image* sharedORM = _ormImageMap[ormKey].get();
                tex2D->setImage(sharedORM); tex2D->setName(sharedORM->getName());
                tex2D->setFilter(osg::Texture2D::MIN_FILTER,
                    sharedORM->isCompressed() ? osg::Texture2D::LINEAR: osg::Texture2D::LINEAR_MIPMAP_LINEAR);
                tex2D->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
                ss->setTextureAttributeAndModes(3, tex2D.get());
            }
            else
            {
                // Load occlusion texture and combine ORM
                osg::ref_ptr<osg::Texture> ormNewInput; bool compressed = false;
                if (occlusionID >= 0)
                    ormNewInput = createTexture(uniformNames[4], _modelDef.textures[occlusionID]);
                if (ormNewInput.valid())
                {
                    // https://github.com/KhronosGroup/glTF/blob/main/specification/2.0/schema/material.occlusionTextureInfo.schema.json
                    osg::Texture* t = static_cast<osg::Texture*>(ss->getTextureAttribute(3, osg::StateAttribute::TEXTURE));
                    ss->setTextureAttributeAndModes(3, constructOcclusionRoughnessMetallic(t, ormNewInput.get(), 0, -1, -1));
                    if (!compressed && ormNewInput->getNumImages() > 0) compressed = ormNewInput->getImage(0)->isCompressed();
                }

                // Load metallic-roughness texture and combine ORM
                if (roughnessID >= 0 && roughnessID < _modelDef.textures.size())
                    ormNewInput = createTexture(uniformNames[3], _modelDef.textures[roughnessID]);
                else
                    ormNewInput = createDefaultTextureForColor(osg::Vec4(
                        1.0f, material.pbrMetallicRoughness.roughnessFactor, material.pbrMetallicRoughness.metallicFactor, 1.0f));
                if (ormNewInput.valid())
                {
                    // https://github.com/KhronosGroup/glTF/blob/main/specification/2.0/schema/material.pbrMetallicRoughness.schema.json
                    if (occlusionID < 0)
                        ss->setTextureAttributeAndModes(3, ormNewInput.get());  // directly use GLTF's XRM texture?
                    else
                    {
                        osg::Texture* t = static_cast<osg::Texture*>(ss->getTextureAttribute(3, osg::StateAttribute::TEXTURE));
                        ss->setTextureAttributeAndModes(3, constructOcclusionRoughnessMetallic(t, ormNewInput.get(), -1, 1, 2));
                        if (!compressed && ormNewInput->getNumImages() > 0) compressed = ormNewInput->getImage(0)->isCompressed();
                    }
                }

                // Compress ORM output if necessary
                osg::Texture* ormOutput = static_cast<osg::Texture*>(ss->getTextureAttribute(3, osg::StateAttribute::TEXTURE));
                if (ormOutput && compressed)
                {
#if true
                    if (ormOutput->getNumImages() > 0) ormOutput->setImage(0, compressImage(*ormOutput->getImage(0)));
#else
                    ormOutput->setInternalFormatMode(osg::Texture::USE_S3TC_DXT1_COMPRESSION);
#endif
                    ormOutput->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
                }
                if (ormOutput && ormOutput->getNumImages() > 0) _ormImageMap[ormKey] = ormOutput->getImage(0);
            }

            // Load emission texture
            if (emissiveID >= 0)
                ss->setTextureAttributeAndModes(5, createTexture(uniformNames[5], _modelDef.textures[emissiveID]));
            else
            {
                osg::Vec3 emission(material.emissiveFactor[0], material.emissiveFactor[1], material.emissiveFactor[2]);
                if (emission.length2() > 0.0f)
                {
                    osg::Texture2D* tex2D = createDefaultTextureForColor(
                        osg::Vec4(emission[0], emission[1], emission[2], emission.length()));
                    if (tex2D) ss->setTextureAttributeAndModes(5, tex2D);
                }
            }
        }

        // Setup other material attributes
        if (material.alphaMode.compare("BLEND") == 0) ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        else ss->setRenderingHint(osg::StateSet::OPAQUE_BIN);

        if (material.doubleSided) ss->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
        else ss->setMode(GL_CULL_FACE, osg::StateAttribute::ON);

#if defined(OSG_GLES1_AVAILABLE) || defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
        // Do nothing?
#else
        if (!material.extras_json_string.empty())
            MaterialGraph::instance()->readFromBlender(material.extras_json_string, *ss);
#endif
    }

    osg::Texture* LoaderGLTF::createTexture(const std::string& name, tinygltf::Texture& tex)
    {
        if (tex.source < 0 || tex.source >= _modelDef.images.size())
        {
            const std::string& ext = tex.extensions_json_string;
            if (!ext.empty())
            {
                size_t pos0 = ext.find("\"source\":");
                if (pos0 != std::string::npos)
                {
                    size_t pos1 = ext.find_first_of("},", pos0 + 9);
                    tex.source = atoi(ext.substr(pos0 + 9, pos1 - pos0 - 9).c_str());
                }
            }
            if (tex.source < 0 || tex.source >= _modelDef.images.size())
            {
                OSG_WARN << "[LoaderGLTF] Invalid texture source index: " << tex.source
                         << " for " << name << ", ext = " << ext << std::endl; return NULL;
            }
        }

        tinygltf::Image& imageSrc = _modelDef.images[tex.source];
        if (imageSrc.image.empty()) return NULL;

        osg::ref_ptr<osg::Image> image2D = _imageMap[tex.source].get();
        if (!image2D)
        {
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
                if (!image) return NULL;
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
            image2D->setUserValue("Loader", std::string("LoaderGLTF:" + imageSrc.mimeType));
            if (!imageSrc.name.empty())
                OSG_INFO << "[LoaderGLTF] " << imageSrc.name << " loaded for " << name << std::endl;
        }

        osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
        tex2D->setResizeNonPowerOfTwoHint(false);
        tex2D->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT);
        tex2D->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT);
        tex2D->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR_MIPMAP_LINEAR);
        tex2D->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        tex2D->setImage(image2D.get()); tex2D->setName(imageSrc.name); return tex2D.release();
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

    osg::ref_ptr<osg::Geometry> LoaderGLTF::createFromExtGaussianSplattingSPZ2(const std::string& name, int bufferViewID)
    {
        std::vector<unsigned char> bufferData;
        const tinygltf::BufferView& extView = _modelDef.bufferViews[bufferViewID];
        if (extView.buffer >= 0) { bufferData = _modelDef.buffers[extView.buffer].data; }

        osgDB::ReaderWriter* rw = osgDB::Registry::instance()->getReaderWriterForExtension("verse_3dgs");
        if (!rw)
            { OSG_WARN << "[LoaderGLTF] 3DGS plugin not found. Cannot parse SPZ in " << name << std::endl; }
        else
        {
            std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
            ss.write((char*)bufferData.data(), bufferData.size());

            osg::ref_ptr<osgDB::Options> opt = new osgDB::Options("extension=spz");
            osg::ref_ptr<osg::Node> spzNode = rw->readNode(ss, opt.get()).getNode();
            FindGeometryVisitor fgv(true); if (spzNode.valid()) spzNode->accept(fgv);

            std::vector<std::pair<osg::Geometry*, osg::Matrix>>& geomList = fgv.getGeometries();
            if (geomList.empty())
                { OSG_WARN << "[LoaderGLTF] Failed to load SPZ gaussian geometry in " << name << std::endl; }
            else
                return geomList.front().first;  // SPZ always creates only 1 geometry
        }
        return NULL;
    }

    osg::ref_ptr<osg::Group> loadGltf(const std::string& file, bool isBinary, int usingPBR, bool yUp)
    {
        std::string workDir = osgDB::getFilePath(file), http = osgDB::getServerProtocol(file);
        if (!http.empty() && http.find("file") == std::string::npos) return NULL;

        std::ifstream in(file.c_str(), std::ios::in | std::ios::binary);
        if (!in)
        {
            OSG_WARN << "[LoaderGLTF] file " << file << " not readable" << std::endl;
            return NULL;
        }

        osg::ref_ptr<LoaderGLTF> loader = new LoaderGLTF(in, workDir, isBinary, usingPBR, yUp);
        if (loader->getRoot()) loader->getRoot()->setName(file);
        return loader->getRoot();
    }

    osg::ref_ptr<osg::Group> loadGltf2(std::istream& in, const std::string& dir,
                                       bool isBinary, int usingPBR, bool yUp)
    {
        osg::ref_ptr<LoaderGLTF> loader = new LoaderGLTF(in, dir, isBinary, usingPBR, yUp);
        return loader->getRoot();
    }
}
