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

#ifdef VERSE_USE_DRACO
#   define TINYGLTF_ENABLE_DRACO
#endif
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "SaveSceneGLTF.h"
#include "Utilities.h"

class GltfSceneWriter : public osgVerse::NodeVisitorEx
{
public:
    GltfSceneWriter(tinygltf::Model* m) : osgVerse::NodeVisitorEx(), _model(m) {}
    tinygltf::Scene& scene() { return _scene; }

    struct TriangleCollector
    {
        std::vector<unsigned int> triangles;
        void operator()(unsigned int i1, unsigned int i2, unsigned int i3)
        { triangles.push_back(i1); triangles.push_back(i2); triangles.push_back(i3); }
    };

    virtual void apply(osg::Geode& node)
    {
        tinygltf::Node gltfNode; gltfNode.name = node.getName();
        if (node.getName().empty()) gltfNode.name = node.className();

        osg::Matrix matrix; if (_matrixStack.size() > 0) matrix = _matrixStack.back();
        gltfNode.matrix.assign(matrix.ptr(), matrix.ptr() + 16);

        _scene.nodes.push_back(_model->nodes.size()); _model->nodes.push_back(gltfNode);
        osgVerse::NodeVisitorEx::apply(node);
    }

    virtual void apply(osg::Geometry& geometry)
    {
        osgVerse::NodeVisitorEx::apply(geometry);
        if (_model->nodes.empty())
        {
            OSG_WARN << "[SaverGLTF] Found geometry without parent node\n";
            return;  // FIXME
        }

#define NEW_BUFFER_EX(id, src, srcType, num, gltfType, gltfComp, gltfTarget) \
    tinygltf::Buffer buf; tinygltf::BufferView view; tinygltf::Accessor acc; id = (int)_model->accessors.size(); \
    size_t dSize = src.size() * sizeof(srcType); buf.data.resize(dSize); memcpy(buf.data.data(), &src[0], dSize); \
    view.buffer = _model->buffers.size(); view.byteOffset = 0; view.byteLength = dSize; view.target = gltfTarget; \
    acc.bufferView = _model->bufferViews.size(); acc.byteOffset = 0; acc.count = num; acc.type = gltfType; acc.componentType = gltfComp; \
    _model->buffers.push_back(buf); _model->bufferViews.push_back(view); _model->accessors.push_back(acc);
#define NEW_VERTEX_BUFFER(id, src, srcType, num, gltfType, gltfComp) \
    NEW_BUFFER_EX(id, src, srcType, num, gltfType, gltfComp, TINYGLTF_TARGET_ARRAY_BUFFER)
#define NEW_ELEMENT_BUFFER(id, src, srcType, num, gltfType, gltfComp) \
    NEW_BUFFER_EX(id, src, srcType, num, gltfType, gltfComp, TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER)

        int posID = -1, normID = -1, colID = -1, uv0ID = -1, uv1ID = -1;
        osg::Vec3Array* va = dynamic_cast<osg::Vec3Array*>(geometry.getVertexArray());
        osg::Vec3Array* na = dynamic_cast<osg::Vec3Array*>(geometry.getNormalArray());
        osg::Vec4Array* ca = dynamic_cast<osg::Vec4Array*>(geometry.getColorArray());
        osg::Vec2Array* ta0 = dynamic_cast<osg::Vec2Array*>(geometry.getTexCoordArray(0));
        osg::Vec2Array* ta1 = dynamic_cast<osg::Vec2Array*>(geometry.getTexCoordArray(1));
        if (va && !va->empty()) { NEW_VERTEX_BUFFER(posID, (*va), osg::Vec3, 3, TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT) }
        if (na && !na->empty()) { NEW_VERTEX_BUFFER(normID, (*na), osg::Vec3, 3, TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT) }
        if (ca && !ca->empty()) { NEW_VERTEX_BUFFER(colID, (*ca), osg::Vec4, 4, TINYGLTF_TYPE_VEC4, TINYGLTF_COMPONENT_TYPE_FLOAT) }
        if (ta0 && !ta0->empty()) { NEW_VERTEX_BUFFER(uv0ID, (*ta0), osg::Vec2, 2, TINYGLTF_TYPE_VEC2, TINYGLTF_COMPONENT_TYPE_FLOAT) }
        if (ta1 && !ta1->empty()) { NEW_VERTEX_BUFFER(uv1ID, (*ta1), osg::Vec2, 2, TINYGLTF_TYPE_VEC2, TINYGLTF_COMPONENT_TYPE_FLOAT) }

        tinygltf::Mesh gltfMesh; gltfMesh.name = geometry.getName();
        if (geometry.getName().empty()) gltfMesh.name = geometry.className();
        for (unsigned int i = 0; i < geometry.getNumPrimitiveSets(); ++i)
        {
            osg::PrimitiveSet* p = geometry.getPrimitiveSet(i);
            tinygltf::Primitive primitive; int indexID = -1;
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
            if (de0) { NEW_ELEMENT_BUFFER(indexID, (*de0), unsigned int, 1,
                                          TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) }
            osg::DrawElementsUShort* de1 = dynamic_cast<osg::DrawElementsUShort*>(p);
            if (de1) { NEW_ELEMENT_BUFFER(indexID, (*de1), unsigned short, 1,
                                          TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) }
            osg::DrawElementsUByte* de2 = dynamic_cast<osg::DrawElementsUByte*>(p);
            if (de2) { NEW_ELEMENT_BUFFER(indexID, (*de2), unsigned char, 1,
                                          TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) }
            if (indexID < 0)
            {
                osg::TriangleIndexFunctor<TriangleCollector> f; p->accept(f);
                if (f.triangles.size() < 65535)
                {
                    NEW_ELEMENT_BUFFER(indexID, (f.triangles), unsigned short, 1,
                                       TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                }
                else
                {
                    NEW_ELEMENT_BUFFER(indexID, (f.triangles), unsigned int, 1,
                                       TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                }
                primitive.mode = TINYGLTF_MODE_TRIANGLES;
            }

            if (posID >= 0) primitive.attributes["POSITION"] = posID;
            if (normID >= 0) primitive.attributes["NORMAL"] = normID;
            if (colID >= 0) primitive.attributes["COLOR"] = colID;
            if (uv0ID >= 0) primitive.attributes["TEXCOORD_0"] = uv0ID;
            if (uv1ID >= 0) primitive.attributes["TEXCOORD_1"] = uv1ID;
            if (indexID >= 0) primitive.indices = indexID;
            if (!_model->materials.empty())
                primitive.material = (int)_model->materials.size() - 1;
            gltfMesh.primitives.push_back(primitive);
        }

        tinygltf::Node& gltfNode = _model->nodes.back();
        gltfNode.mesh = _model->meshes.size(); _model->meshes.push_back(gltfMesh);
    }

    virtual void apply(osg::Node* node, osg::Drawable* drawable, osg::StateSet& ss)
    {
        tinygltf::Material material; material.name = ss.getName();
        if (ss.getRenderingHint() == osg::StateSet::TRANSPARENT_BIN) material.alphaMode = "BLEND";
        material.doubleSided = true; _model->materials.push_back(material);
        osgVerse::NodeVisitorEx::apply(node, drawable, ss);
    }

    virtual void apply(osg::Node* node, osg::Drawable* drawable, osg::Texture* tex, int unit)
    {
        if (_model->materials.empty()) return;
        tinygltf::Material& material = _model->materials.back();
        osg::Referenced* tc = drawable->asGeometry() ? drawable->asGeometry()->getTexCoordArray(unit) : NULL;

        // /*0*/"DiffuseMap", /*1*/"NormalMap", /*2*/"SpecularMap", /*3*/"ShininessMap",
        // /*4*/"AmbientMap", /*5*/"EmissiveMap", /*6*/"ReflectionMap", /*7*/"CustomMap"
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

protected:
    int createGltfTexture(osg::Texture* tex)
    {
        osg::Image* image = tex->getImage(0);  // FIXME: consider only 2D tex?
        if (!image) return -1;

        tinygltf::Image gltfImage;
        gltfImage.name = image->getName();
        gltfImage.width = image->s(); gltfImage.height = image->t();
        gltfImage.component = osg::Image::computeNumComponents(image->getPixelFormat());
        gltfImage.bits = osg::Image::computePixelSizeInBits(image->getPixelFormat(), image->getDataType());
        switch (gltfImage.bits)
        {
        case 16: gltfImage.pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT; break;
        case 32: gltfImage.pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT; break;
        default: gltfImage.pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE; break;
        }
        //gltfImage.image = imageData;  // TODO
        //gltfImage.mimeType = "image/png";  // TODO
        gltfImage.uri = image->getFileName();

        tinygltf::Texture gltfTex;
        gltfTex.source = _model->images.size();
        gltfTex.sampler = _model->samplers.size();
        gltfTex.name = tex->getName();

        tinygltf::Sampler sampler;
        sampler.minFilter = setFilter(tex->getFilter(osg::Texture::MIN_FILTER));
        sampler.magFilter = setFilter(tex->getFilter(osg::Texture::MAG_FILTER));
        sampler.wrapS = setWrap(tex->getWrap(osg::Texture::WRAP_S));
        sampler.wrapT = setWrap(tex->getWrap(osg::Texture::WRAP_T));

        int resultID = (int)_model->textures.size();
        _model->images.push_back(gltfImage);
        _model->textures.push_back(gltfTex);
        _model->samplers.push_back(sampler);
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

    tinygltf::Model* _model;
    tinygltf::Scene _scene;
};

namespace osgVerse
{
    SaverGLTF::SaverGLTF(const osg::Node& node, std::ostream& out, const std::string& d, bool isBinary)
    {
        GltfSceneWriter sceneWriter(&_modelDef);
        const_cast<osg::Node&>(node).accept(sceneWriter); _done = true;

        sceneWriter.scene().name = "Scene: " + node.getName();
        _modelDef.scenes.push_back(sceneWriter.scene());
        _modelDef.defaultScene = 0;
        _modelDef.asset.version = "2.0";
        _modelDef.asset.generator = "osgVerse::SaverGLTF";
        
        tinygltf::TinyGLTF writer;
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
