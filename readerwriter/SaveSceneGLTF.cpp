#include <osg/io_utils>
#include <osg/Version>
#include <osg/ValueObject>
#include <osg/TriangleIndexFunctor>
#include <osg/AnimationPath>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osgDB/ConvertUTF>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "modeling/Utilities.h"
#include "pipeline/Utilities.h"
#include "LoadTextureKTX.h"
#include <picojson.h>
#include <algorithm>

#ifdef VERSE_USE_DRACO
#   define TINYGLTF_ENABLE_DRACO
#endif
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#include "SaveSceneGLTF.h"
#include "Utilities.h"

class GltfSceneWriter : public osgVerse::NodeVisitorEx
{
public:
    GltfSceneWriter(tinygltf::Model* m, bool yUp) : osgVerse::NodeVisitorEx(), _model(m)
    { if (yUp) pushMatrix(osg::Matrix::rotate(osg::Z_AXIS, osg::Y_AXIS)); }

    struct TriangleCollector
    {
        std::vector<unsigned int> triangles;
        void operator()(unsigned int i1, unsigned int i2, unsigned int i3)
        {
            if (i1 == i2 || i2 == i3 || i1 == i3) return;
            triangles.push_back(i1); triangles.push_back(i2); triangles.push_back(i3);
        }
    };
    tinygltf::Scene& scene() { return _scene; }

    void pushDescriptions(osg::Node& node)
    {
        const std::vector<std::string>& d = node.getDescriptions();
        _extraStringList.insert(_extraStringList.end(), d.begin(), d.end());
    }

    void popDescriptions(osg::Node& node)
    {
        const std::vector<std::string>& d = node.getDescriptions();
        for (size_t i = 0; i < d.size(); ++i) { if (!_extraStringList.empty()) _extraStringList.pop_back(); }
    }

    virtual void apply(osg::Node& node)
    { pushDescriptions(node); osgVerse::NodeVisitorEx::apply(node); popDescriptions(node); }

    virtual void apply(osg::Transform& node)
    { pushDescriptions(node); osgVerse::NodeVisitorEx::apply(node); popDescriptions(node); }

    virtual void apply(osg::Geode& node)
    { pushDescriptions(node); osgVerse::NodeVisitorEx::apply(node); popDescriptions(node); }

    virtual void apply(osg::Geometry& geometry)
    {
        osgVerse::NodeVisitorEx::apply(geometry);  // traverse first to generate materials

#define NEW_BUFFER_EX(id, src, srcType, gltfType, gltfComp, gltfTarget) \
    id = (int)_model->accessors.size(); _model->buffers.push_back(tinygltf::Buffer()); tinygltf::Buffer& buf = _model->buffers.back(); \
    _model->bufferViews.push_back(tinygltf::BufferView()); tinygltf::BufferView& view = _model->bufferViews.back(); \
    _model->accessors.push_back(tinygltf::Accessor()); tinygltf::Accessor& acc = _model->accessors.back(); \
    size_t dSize = src.size() * sizeof(srcType); buf.data.resize(dSize); memcpy(buf.data.data(), &src[0], dSize); \
    view.buffer = _model->buffers.size() - 1; view.byteOffset = 0; view.byteLength = dSize; view.target = gltfTarget; \
    acc.bufferView = _model->bufferViews.size() - 1; acc.byteOffset = 0; acc.count = (int)src.size(); acc.type = gltfType; \
    acc.componentType = gltfComp; acc.minValues.clear(); acc.maxValues.clear(); acc.sparse.isSparse = 0;

#define NEW_V_BUFFER(id, src, srcType, gltfType, gltfComp) \
    NEW_BUFFER_EX(id, src, srcType, gltfType, gltfComp, TINYGLTF_TARGET_ARRAY_BUFFER)
#define NEW_E_BUFFER(id, src, srcType, gltfType, gltfComp) \
    NEW_BUFFER_EX(id, src, srcType, gltfType, gltfComp, TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER)

        // Get vertex attribute buffers/accessors
        int posID = -1, normID = -1, colID = -1, uv0ID = -1, uv1ID = -1; size_t vSize = 0;
        osg::Vec3Array* va = dynamic_cast<osg::Vec3Array*>(geometry.getVertexArray());
        osg::Vec3Array* na = dynamic_cast<osg::Vec3Array*>(geometry.getNormalArray());
        osg::Vec4Array* ca = dynamic_cast<osg::Vec4Array*>(geometry.getColorArray());
        osg::Vec2Array* ta0 = dynamic_cast<osg::Vec2Array*>(geometry.getTexCoordArray(0));
        osg::Vec2Array* ta1 = dynamic_cast<osg::Vec2Array*>(geometry.getTexCoordArray(1));
        if (va && !va->empty()) { vSize = va->size(); NEW_V_BUFFER(posID, (*va), osg::Vec3, TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT) }
        if (na && na->size() == vSize) { NEW_V_BUFFER(normID, (*na), osg::Vec3, TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT) }
        if (ca && ca->size() == vSize) { NEW_V_BUFFER(colID, (*ca), osg::Vec4, TINYGLTF_TYPE_VEC4, TINYGLTF_COMPONENT_TYPE_FLOAT) }
        if (ta0 && ta0->size() == vSize) { NEW_V_BUFFER(uv0ID, (*ta0), osg::Vec2, TINYGLTF_TYPE_VEC2, TINYGLTF_COMPONENT_TYPE_FLOAT) }
        if (ta1 && ta1->size() == vSize) { NEW_V_BUFFER(uv1ID, (*ta1), osg::Vec2, TINYGLTF_TYPE_VEC2, TINYGLTF_COMPONENT_TYPE_FLOAT) }

        // Create new node and mesh
        _scene.nodes.push_back(_model->nodes.size()); _model->nodes.push_back(tinygltf::Node());
        tinygltf::Node& gltfNode = _model->nodes.back(); gltfNode.name = geometry.getName();
        if (geometry.getName().empty()) gltfNode.name = geometry.className();

        std::vector<tinygltf::Value> extraStrings; std::map<std::string, tinygltf::Value> extraObj;
        for (size_t i = 0; i < _extraStringList.size(); ++i) extraStrings.push_back(tinygltf::Value(_extraStringList[i]));
        if (!extraStrings.empty()) { extraObj["descriptions"] = tinygltf::Value(extraStrings); gltfNode.extras = tinygltf::Value(extraObj); }

        osg::Matrix matrix; if (_matrixStack.size() > 0) matrix = _matrixStack.back();
        gltfNode.matrix.assign(matrix.ptr(), matrix.ptr() + 16);
        if (_geometries.find(&geometry) != _geometries.end())
            gltfNode.mesh = _geometries[&geometry];  // shared geometry
        else
        {
            gltfNode.mesh = _model->meshes.size(); _geometries[&geometry] = gltfNode.mesh;
            _model->meshes.push_back(tinygltf::Mesh());
        }

        tinygltf::Mesh& gltfMesh = _model->meshes.back(); gltfMesh.name = geometry.getName();
        if (geometry.getName().empty()) gltfMesh.name = geometry.className();
        gltfMesh.primitives.clear(); gltfMesh.weights.clear();

        // Create primitives
#if false
        for (unsigned int i = 0; i < geometry.getNumPrimitiveSets(); ++i)
        {
            osg::PrimitiveSet* p = geometry.getPrimitiveSet(i);
            int indexID = -1; gltfMesh.primitives.push_back(tinygltf::Primitive());

            tinygltf::Primitive& primitive = gltfMesh.primitives.back();
            switch (p->getMode())
            {
            case GL_POINTS: primitive.mode = TINYGLTF_MODE_POINTS; break;
            case GL_LINES: primitive.mode = TINYGLTF_MODE_LINE; break;
            case GL_LINE_LOOP: primitive.mode = TINYGLTF_MODE_LINE_LOOP; break;
            case GL_LINE_STRIP: primitive.mode = TINYGLTF_MODE_LINE_STRIP; break;
            case GL_TRIANGLES: primitive.mode = TINYGLTF_MODE_TRIANGLES; break;
            case GL_TRIANGLE_STRIP: primitive.mode = TINYGLTF_MODE_TRIANGLE_STRIP; break;
            case GL_TRIANGLE_FAN: primitive.mode = TINYGLTF_MODE_TRIANGLE_FAN; break;
            }

            osg::DrawElementsUInt* de0 = dynamic_cast<osg::DrawElementsUInt*>(p);
            if (de0) { NEW_E_BUFFER(indexID, (*de0), unsigned int,
                                    TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) }
            osg::DrawElementsUShort* de1 = dynamic_cast<osg::DrawElementsUShort*>(p);
            if (de1) { NEW_E_BUFFER(indexID, (*de1), unsigned short,
                                    TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) }
            osg::DrawElementsUByte* de2 = dynamic_cast<osg::DrawElementsUByte*>(p);
            if (de2) { NEW_E_BUFFER(indexID, (*de2), unsigned char,
                                    TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) }
            if (indexID < 0)
            {
                osg::TriangleIndexFunctor<TriangleCollector> f; p->accept(f);
                { NEW_E_BUFFER(indexID, (f.triangles), unsigned int,
                               TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) }
                primitive.mode = TINYGLTF_MODE_TRIANGLES;
            }
#else
        osg::TriangleIndexFunctor<TriangleCollector> f; geometry.accept(f);
        int indexID = -1; gltfMesh.primitives.push_back(tinygltf::Primitive());
        { NEW_E_BUFFER(indexID, (f.triangles), unsigned int, TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) }

        tinygltf::Primitive& primitive = gltfMesh.primitives.back();
        primitive.mode = TINYGLTF_MODE_TRIANGLES;
        {
#endif
            primitive.attributes.clear(); primitive.targets.clear();
            if (posID >= 0) primitive.attributes["POSITION"] = posID;
            if (normID >= 0) primitive.attributes["NORMAL"] = normID;
            if (colID >= 0) primitive.attributes["COLOR"] = colID;
            if (uv0ID >= 0) primitive.attributes["TEXCOORD_0"] = uv0ID;
            if (uv1ID >= 0) primitive.attributes["TEXCOORD_1"] = uv1ID;
            if (indexID >= 0) primitive.indices = indexID;
            if (!_model->materials.empty())
                primitive.material = (int)_model->materials.size() - 1;
        }
    }

    virtual void apply(osg::Node* node, osg::Drawable* drawable, osg::StateSet& ss)
    {
        osgVerse::NodeVisitorEx::apply(node, drawable, ss);
    }

    virtual void apply(osg::Node* node, osg::Drawable* drawable, osg::Texture* tex, int unit)
    {
        osg::StateSet* ss = (tex->getNumParents() > 0) ? tex->getParent(0) : NULL;
        unsigned int mtlIndex = 0; if (!ss || dynamic_cast<osg::Texture2D*>(tex) == NULL) return;
        if (_statesets.find(ss) == _statesets.end())
        {
            mtlIndex = _model->materials.size(); _statesets[ss] = (int)mtlIndex;
            _model->materials.push_back(tinygltf::Material());
        }
        else
            mtlIndex = (unsigned int)_statesets[ss];

        tinygltf::Material& material = _model->materials[mtlIndex]; material.name = ss->getName();
        if (ss->getRenderingHint() == osg::StateSet::TRANSPARENT_BIN) material.alphaMode = "BLEND";
        material.doubleSided = true;  // FIXME

        // /*0*/"DiffuseMap", /*1*/"NormalMap", /*2*/"SpecularMap", /*3*/"ShininessMap",
        // /*4*/"AmbientMap", /*5*/"EmissiveMap", /*6*/"ReflectionMap", /*7*/"CustomMap"
        osg::Referenced* tc = drawable->asGeometry() ? drawable->asGeometry()->getTexCoordArray(unit) : NULL;
        switch (unit)
        {
        case 0:
            if (tc) material.pbrMetallicRoughness.baseColorTexture.texCoord = unit;
            material.pbrMetallicRoughness.baseColorTexture.index = createGltfTexture(tex); break;
        case 1:
            if (tc) material.normalTexture.texCoord = unit;
            material.normalTexture.index = createGltfTexture(tex); break;
        case 3:  // FIXME: ORM texture to GLTF?
            if (tc) material.pbrMetallicRoughness.metallicRoughnessTexture.texCoord = unit;
            material.pbrMetallicRoughness.metallicRoughnessTexture.index = createGltfTexture(tex); break;
        case 5:
            if (tc) material.emissiveTexture.texCoord = unit;
            material.emissiveTexture.index = createGltfTexture(tex); break;
        default: break;
        }
    }

    static bool writeImageImplementation(const std::string* basepath, const std::string* filename,
                                         const tinygltf::Image* image, bool embedImages,
                                         const tinygltf::FsCallbacks* fs_cb, const tinygltf::URICallbacks* uri_cb,
                                         std::string* out_uri, void* user_pointer)
    {
        GltfSceneWriter* writer = (GltfSceneWriter*)user_pointer;
        if (image->image.empty()) { *out_uri = *filename; return true; }

        // If the image data is already encoded, take it as is
        std::vector<unsigned char> data; if (image->as_is) data = image->image;

        const std::string ext = osgDB::getFileExtension(*filename), mimeType = image->mimeType;
        if (mimeType == "image/png")
        {
            if (!image->as_is)
            {
                if ((image->bits != 8) || (image->pixel_type != TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE))
                {
                    OSG_WARN << "[SaverGLTF] Unsupported pixel format while encoding PNG of "
                             << *filename << std::endl; return false;
                }
                if (!stbi_write_png_to_func(GltfSceneWriter::WriteToMemory_stbi, &data, image->width,
                                            image->height, image->component, &image->image[0], 0))
                { OSG_WARN << "[SaverGLTF] Failed writing PNG of " << *filename << std::endl; return false; }
            }
        }
        else if (mimeType == "image/jpeg")
        {
            if (!image->as_is)
            {
                if (!stbi_write_jpg_to_func(GltfSceneWriter::WriteToMemory_stbi, &data, image->width,
                                            image->height, image->component, &image->image[0], 100))
                { OSG_WARN << "[SaverGLTF] Failed writing JPG of " << *filename << std::endl; return false; }
            }
        }
        else if (!embedImages)
        {
            // Error: can't output requested format to file
            OSG_WARN << "[SaverGLTF] Unsupported file format: " << *filename << std::endl; return false;
        }

        std::string writeError, header = "data:" + mimeType + ";base64,";
        if (embedImages)
        {
            if (data.size())   // Embed base64-encoded image into URI
                *out_uri = header + osgVerse::WebAuxiliary::encodeBase64(data);
            else
                { OSG_WARN << "[SaverGLTF] Empty data while writing file: " << *filename << std::endl; }
        }
        else
        {
            if ((fs_cb != nullptr) && (fs_cb->WriteWholeFile != nullptr))
            {   // Write image to disc
                std::string imageFilepath = *basepath;
                if (imageFilepath.empty()) imageFilepath = *filename;
                else
                {
                    if (*imageFilepath.rbegin() == '/') imageFilepath += *filename;
                    else imageFilepath += "/" + *filename;
                }
                
                if (!fs_cb->WriteWholeFile(&writeError, imageFilepath, data, fs_cb->user_data))
                {
                    // Could not write image file to disc; Throw error ?
                    OSG_WARN << "[SaverGLTF] Failed writing file to disk: " << *filename << std::endl;
                    return false;
                }
            }
            else
                { OSG_WARN << "[SaverGLTF] No method to write file: " << *filename << std::endl; }

            if (!uri_cb->encode) *out_uri = *filename;
            else
            {
                if (!uri_cb->encode(*filename, "image", out_uri, uri_cb->user_data))
                { OSG_WARN << "[SaverGLTF] Failed encode uri of: " << *filename << std::endl; return false; }
            }
        }
        return true;
    }

    static void WriteToMemory_stbi(void* context, void* data, int size)
    {
        std::vector<unsigned char>* buffer = reinterpret_cast<std::vector<unsigned char> *>(context);
        unsigned char* pData = reinterpret_cast<unsigned char*>(data);
        buffer->insert(buffer->end(), pData, pData + size);
    }

protected:
    int createGltfTexture(osg::Texture* tex)
    {
        osg::Image* image = tex->getImage(0);  // FIXME: consider only 2D tex?
        if (!image) return -1;

        int resultID = (int)_model->textures.size();
        _model->images.push_back(tinygltf::Image());
        _model->textures.push_back(tinygltf::Texture());
        _model->samplers.push_back(tinygltf::Sampler());

        tinygltf::Image& gltfImage = _model->images.back(); gltfImage.name = image->getName();
        gltfImage.width = image->s(); gltfImage.height = image->t();
        gltfImage.component = osg::Image::computeNumComponents(image->getPixelFormat());
        gltfImage.bits = osg::Image::computePixelSizeInBits(image->getPixelFormat(), image->getDataType())
                       / gltfImage.component;  // bits per channel
        switch (gltfImage.bits)
        {
        case 16: gltfImage.pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT; break;
        case 32: gltfImage.pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT; break;
        default: gltfImage.pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE; break;
        }

        std::string ext = osgDB::getFileExtension(image->getFileName()), newExt;
        std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
        if (image->valid())
        {
            gltfImage.image.resize(image->getTotalSizeInBytes());
            memcpy(gltfImage.image.data(), image->data(), image->getTotalSizeInBytes());
            if (image->isCompressed())  // FIXME: assumed as DDS?
            {
                gltfImage.mimeType = "image/dds"; gltfImage.as_is = true;
                if (ext != "dds") newExt = ".dds";
            }
            else
            {   // FIXME: consider KTX?
                if (image->isImageTranslucent()) { gltfImage.mimeType = "image/png"; if (ext != "png") newExt = ".png"; }
                else { gltfImage.mimeType = "image/jpeg"; if (ext != "jpg" && ext != "jpeg") newExt = ".jpg"; }
            }
        }
        gltfImage.uri = image->getFileName() + newExt;

        tinygltf::Texture& gltfTex = _model->textures.back();
        gltfTex.source = _model->images.size() - 1;
        gltfTex.sampler = _model->samplers.size() - 1;
        gltfTex.name = tex->getName();

        tinygltf::Sampler& sampler = _model->samplers.back();
        sampler.minFilter = setFilter(tex->getFilter(osg::Texture::MIN_FILTER));
        sampler.magFilter = setFilter(tex->getFilter(osg::Texture::MAG_FILTER));
        sampler.wrapS = setWrap(tex->getWrap(osg::Texture::WRAP_S));
        sampler.wrapT = setWrap(tex->getWrap(osg::Texture::WRAP_T));
        return resultID;
    }

    int setFilter(osg::Texture::FilterMode mode)
    {
        switch (mode)
        {
        case osg::Texture::LINEAR: return TINYGLTF_TEXTURE_FILTER_LINEAR;
        case osg::Texture::LINEAR_MIPMAP_LINEAR: return TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR;
        case osg::Texture::LINEAR_MIPMAP_NEAREST: return TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST;
        case osg::Texture::NEAREST_MIPMAP_LINEAR: return TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR;
        case osg::Texture::NEAREST_MIPMAP_NEAREST: return TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST;
        default: return TINYGLTF_TEXTURE_FILTER_NEAREST;
        }
    }

    int setWrap(osg::Texture::WrapMode mode)
    {
        switch (mode)
        {
        case osg::Texture::REPEAT: return TINYGLTF_TEXTURE_WRAP_REPEAT;
        case osg::Texture::MIRROR: return TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT;
        default: return TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE;
        }
    }

    std::map<osg::Geometry*, int> _geometries;
    std::map<osg::StateSet*, int> _statesets;
    std::vector<std::string> _extraStringList;
    tinygltf::Model* _model;
    tinygltf::Scene _scene;
};

namespace osgVerse
{
    SaverGLTF::SaverGLTF(const osg::Node& node, std::ostream& out, const std::string& d, bool isBinary)
    {
        GltfSceneWriter sceneWriter(&_modelDef, true);
        const_cast<osg::Node&>(node).accept(sceneWriter); _done = true;

        sceneWriter.scene().name = "Scene: " + node.getName();
        _modelDef.scenes.push_back(sceneWriter.scene());
        _modelDef.defaultScene = 0;
        _modelDef.asset.version = "2.0";
        _modelDef.asset.generator = "osgVerse::SaverGLTF";

        tinygltf::TinyGLTF writer;
        writer.SetImageWriter(GltfSceneWriter::writeImageImplementation, &sceneWriter);

        bool success = writer.WriteGltfSceneToStream(&_modelDef, out, true, isBinary);
        if (!success) { OSG_WARN << "[SaverGLTF] Unable to write GLTF scene\n"; _done = false; }
    }

    bool saveGltf(const osg::Node& node, const std::string& file, bool isBinary)
    {
        std::string workDir = osgDB::getFilePath(file);
        std::ofstream out(file.c_str(), std::ios::out | std::ios::binary);
        if (!out)
        {
            OSG_WARN << "[SaverGLTF] file " << file << " not writable" << std::endl;
            return false;
        }

        osg::ref_ptr<SaverGLTF> saver = new SaverGLTF(node, out, workDir, isBinary);
        return saver->getResult();
    }

    bool saveGltf2(const osg::Node& node, std::ostream& out, const std::string& dir, bool isBinary)
    {
        osg::ref_ptr<SaverGLTF> saver = new SaverGLTF(node, out, dir, isBinary);
        return saver->getResult();
    }
}
