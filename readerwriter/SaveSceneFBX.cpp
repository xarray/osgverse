#include <osg/io_utils>
#include <osg/Version>
#include <osg/ValueObject>
#include <osg/TriangleIndexFunctor>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osg/Material>
#include <osgDB/ConvertUTF>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <algorithm>
#include <iomanip>
#include <memory>
#include <sstream>

#include "pipeline/Utilities.h"
#include "modeling/Utilities.h"
#include "Utilities.h"
#include "SaveSceneFBX.h"
#include "3rdparty/ufbx.h"

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

namespace
{
    const uint32_t FBX_VERSION = 7400;

    std::string toNum(double v)
    {
        if (v == 0.0) return "0";
        std::ostringstream os; os << std::setprecision(10) << v;
        return os.str();
    }

    std::string escapeString(const std::string& s)
    {
        std::string s2 = osgVerse::StringAuxiliary::convertNativePath(s);  // use '/' as separator
        std::string result; result.reserve(s2.size());
        for (size_t i = 0; i < s2.size(); ++i)
        { if (s2[i] == '\"') result += "\\\""; else result += s2[i]; }
        return result;
    }

    // Generic FBX document node, shared by the ASCII and binary serializers
    struct FbxProp
    {
        enum Type { INT32, INT64, DOUBLE, STRING, RAW_BYTES, ARR_INT32, ARR_FLOAT, ARR_DOUBLE };
        Type type; int64_t iv; double dv; std::string sv;
        std::vector<int32_t> i32; std::vector<float> f32; std::vector<double> f64;

        FbxProp() : type(INT32), iv(0), dv(0.0) {}
        static FbxProp I32(int64_t v) { FbxProp p; p.type = INT32; p.iv = v; return p; }
        static FbxProp I64(int64_t v) { FbxProp p; p.type = INT64; p.iv = v; return p; }
        static FbxProp D(double v) { FbxProp p; p.type = DOUBLE; p.dv = v; return p; }
        static FbxProp S(const std::string& v) { FbxProp p; p.type = STRING; p.sv = v; return p; }
        static FbxProp R(const std::vector<unsigned char>& v)
        { FbxProp p; p.type = RAW_BYTES; p.sv.assign((const char*)v.data(), v.size()); return p; }
        static FbxProp ARi(const std::vector<int32_t>& v) { FbxProp p; p.type = ARR_INT32; p.i32 = v; return p; }
        static FbxProp ARf(const std::vector<float>& v) { FbxProp p; p.type = ARR_FLOAT; p.f32 = v; return p; }
        static FbxProp ARd(const std::vector<double>& v) { FbxProp p; p.type = ARR_DOUBLE; p.f64 = v; return p; }
    };

    struct FbxNode
    {
        std::string name;
        std::vector<FbxProp> props;
        std::vector<std::shared_ptr<FbxNode>> children;
        FbxNode() {}
        explicit FbxNode(const std::string& n) : name(n) {}
        FbxNode& prop(const FbxProp& p) { props.push_back(p); return *this; }
    };

    FbxNode* addNode(FbxNode& parent, const std::string& name)
    {
        parent.children.push_back(std::shared_ptr<FbxNode>(new FbxNode(name)));
        return parent.children.back().get();
    }

    void addProp70(FbxNode& props70, const std::string& name, const std::string& type,
                   const std::string& subtype, const std::string& flags, const std::vector<double>& values)
    {
        FbxNode* p = addNode(props70, "P");
        p->prop(FbxProp::S(name)).prop(FbxProp::S(type)).prop(FbxProp::S(subtype)).prop(FbxProp::S(flags));
        for (size_t i = 0; i < values.size(); ++i) p->prop(FbxProp::D(values[i]));
    }

    void addProp70i(FbxNode& props70, const std::string& name, const std::string& type,
                    const std::string& subtype, const std::string& flags, int64_t value)
    {
        FbxNode* p = addNode(props70, "P");
        p->prop(FbxProp::S(name)).prop(FbxProp::S(type)).prop(FbxProp::S(subtype)).prop(FbxProp::S(flags));
        p->prop(FbxProp::I32(value));
    }

    void addDefinition(FbxNode& defs, const std::string& type, int64_t count)
    {
        FbxNode* ot = addNode(defs, "ObjectType");
        ot->prop(FbxProp::S(type));
        addNode(*ot, "Count")->prop(FbxProp::I32(count));
    }

    // ---- ASCII serializer ----
    size_t arrayCount(const FbxProp& p)
    {
        if (p.type == FbxProp::ARR_INT32) return p.i32.size();
        if (p.type == FbxProp::ARR_FLOAT) return p.f32.size();
        if (p.type == FbxProp::ARR_DOUBLE) return p.f64.size();
        return 0;
    }

    void writeAsciiProp(std::ostream& os, const FbxProp& p)
    {
        switch (p.type)
        {
        case FbxProp::INT32: case FbxProp::INT64: os << p.iv; break;
        case FbxProp::DOUBLE: os << toNum(p.dv); break;
        case FbxProp::STRING: os << "\"" << escapeString(p.sv) << "\""; break;
        default: break;
        }
    }

    void writeAsciiNode(std::ostream& os, const FbxNode& n, int indent)
    {
        std::string tab(indent, '\t');
        os << tab << n.name << ":";

        if (n.props.size() == 1 && n.props[0].type == FbxProp::RAW_BYTES)
        {   // embedded media content: base64 string
            const std::string& s = n.props[0].sv;
            std::vector<unsigned char> data(s.begin(), s.end());
            os << " ,\"" << osgVerse::WebAuxiliary::encodeBase64(data) << "\"\n";
            return;
        }
        if (n.props.size() == 1 &&
            (n.props[0].type == FbxProp::ARR_INT32 || n.props[0].type == FbxProp::ARR_FLOAT ||
             n.props[0].type == FbxProp::ARR_DOUBLE))
        {
            const FbxProp& a = n.props[0];
            os << " *" << arrayCount(a) << " {\n" << tab << "\ta: ";
            for (size_t i = 0; i < arrayCount(a); ++i)
            { if (i) os << ",";
              if (a.type == FbxProp::ARR_INT32) os << a.i32[i];
              else if (a.type == FbxProp::ARR_FLOAT) os << toNum(a.f32[i]);
              else os << toNum(a.f64[i]); }
            os << "\n" << tab << "}\n";
            return;
        }

        for (size_t i = 0; i < n.props.size(); ++i)
        { os << ((i == 0) ? " " : ","); writeAsciiProp(os, n.props[i]); }

        if (n.children.empty()) { os << "\n"; return; }
        os << " {\n";
        for (size_t i = 0; i < n.children.size(); ++i) writeAsciiNode(os, *n.children[i], indent + 1);
        os << tab << "}\n";
    }

    // ---- Binary serializer ----
    inline void putU8(std::string& out, unsigned v) { out.push_back((char)(unsigned char)v); }
    inline void putU32(std::string& out, uint32_t v)
    {
        out.push_back((char)(v & 0xff)); out.push_back((char)((v >> 8) & 0xff));
        out.push_back((char)((v >> 16) & 0xff)); out.push_back((char)((v >> 24) & 0xff));
    }
    inline void putU64(std::string& out, uint64_t v)
    { putU32(out, (uint32_t)(v & 0xffffffffu)); putU32(out, (uint32_t)(v >> 32)); }
    inline void putF64(std::string& out, double v)
    { uint64_t u; memcpy(&u, &v, sizeof(u)); putU64(out, u); }

    void appendBinaryArray(std::string& out, osg::Referenced* compressor, unsigned char typeCode,
                           size_t count, size_t elemSize, const std::string& data)
    {
        // Try DEFLATE (zlib) compression first: FBX binary arrays may be stored either way
        std::vector<unsigned char> compressed;
        if (!data.empty())
        {
            std::vector<unsigned char> raw(data.begin(), data.end());
            compressed = osgVerse::CompressAuxiliary::compress(compressor, raw);
        }

        putU8(out, typeCode); putU32(out, (uint32_t)count);
        if (!compressed.empty() && compressed.size() < data.size())
        {   // encoding = 1 (zlib compressed), encoded size = compressed length
            putU32(out, 1); putU32(out, (uint32_t)compressed.size());
            out.append((const char*)compressed.data(), compressed.size());
        }
        else
        {   // encoding = 0 (plain data), encoded size = raw length
            putU32(out, 0); putU32(out, (uint32_t)(count * elemSize)); out += data;
        }
    }

    void appendBinaryProp(std::string& out, osg::Referenced* compressor, const FbxProp& p)
    {
        switch (p.type)
        {
        case FbxProp::INT32: putU8(out, 'I'); putU32(out, (uint32_t)(int32_t)p.iv); break;
        case FbxProp::INT64: putU8(out, 'L'); putU64(out, (uint64_t)p.iv); break;
        case FbxProp::DOUBLE: putU8(out, 'D'); putF64(out, p.dv); break;
        case FbxProp::STRING:
            putU8(out, 'S'); putU32(out, (uint32_t)p.sv.size()); out += p.sv; break;
        case FbxProp::RAW_BYTES:
            putU8(out, 'R'); putU32(out, (uint32_t)p.sv.size()); out += p.sv; break;
        case FbxProp::ARR_INT32:
        {
            std::string data; data.reserve(p.i32.size() * 4);
            for (size_t i = 0; i < p.i32.size(); ++i) putU32(data, (uint32_t)p.i32[i]);
            appendBinaryArray(out, compressor, 'i', p.i32.size(), 4, data); break;
        }
        case FbxProp::ARR_FLOAT:
        {
            std::string data; data.reserve(p.f32.size() * 4);
            for (size_t i = 0; i < p.f32.size(); ++i)
            { uint32_t u; float f = p.f32[i]; memcpy(&u, &f, 4); putU32(data, u); }
            appendBinaryArray(out, compressor, 'f', p.f32.size(), 4, data); break;
        }
        case FbxProp::ARR_DOUBLE:
        {
            std::string data; data.reserve(p.f64.size() * 8);
            for (size_t i = 0; i < p.f64.size(); ++i) putF64(data, p.f64[i]);
            appendBinaryArray(out, compressor, 'd', p.f64.size(), 8, data); break;
        }
        }
    }

    std::string serializeBinaryNode(const FbxNode& n, osg::Referenced* compressor, uint32_t startOffset)
    {
        std::string propBuf;
        for (size_t i = 0; i < n.props.size(); ++i) appendBinaryProp(propBuf, compressor, n.props[i]);

        std::string childBuf;
        uint32_t childOffset = startOffset + 13 + (uint32_t)n.name.size() + (uint32_t)propBuf.size();
        for (size_t i = 0; i < n.children.size(); ++i)
        {
            std::string cs = serializeBinaryNode(*n.children[i], compressor, childOffset);
            childBuf += cs; childOffset += (uint32_t)cs.size();
        }
        if (!n.children.empty()) { childBuf.append(13, '\0'); childOffset += 13; }  // NULL-record

        std::string out;
        putU32(out, childOffset);                       // EndOffset (absolute)
        putU32(out, (uint32_t)n.props.size());          // NumProperties
        putU32(out, (uint32_t)propBuf.size());          // PropertyListLen
        putU8(out, (unsigned char)n.name.size());
        out += n.name; out += propBuf; out += childBuf;
        return out;
    }

    void writeBinaryDocument(std::ostream& out, const FbxNode& root)
    {
        // A zlib compressor handle is reused for all arrays of this document
        osg::ref_ptr<osg::Referenced> compressor = osgVerse::CompressAuxiliary::createHandle(
            osgVerse::CompressAuxiliary::ZLIB);

        std::string data;
        data.append("Kaydara FBX Binary  ", 20);
        putU8(data, 0x00); putU8(data, 0x1A); putU8(data, 0x00);
        putU32(data, FBX_VERSION);

        for (size_t i = 0; i < root.children.size(); ++i)
            data += serializeBinaryNode(*root.children[i], compressor.get(), (uint32_t)data.size());
        data.append(13, '\0');                          // top-level NULL-record

        // Footer (as written by the official FBX SDK for version 7.4)
        const unsigned char footerId[16] = { 0xfa, 0xbc, 0xae, 0x0a, 0xd7, 0xca, 0xd3, 0x66,
                                             0xb6, 0x75, 0xf8, 0x86, 0x1a, 0xfe, 0x2a, 0x78 };
        const unsigned char footerSig[16] = { 0xf8, 0x5a, 0x8c, 0x6a, 0xde, 0xf5, 0xd9, 0x7e,
                                              0xec, 0xe9, 0x0c, 0xe3, 0x75, 0x8f, 0x29, 0x0b };
        data.append((const char*)footerId, 16);
        size_t pad = (16 - (data.size() % 16)) % 16;
        data.append(pad, '\0');
        putU32(data, 0); putU32(data, FBX_VERSION);
        data.append(120, '\0');
        data.append((const char*)footerSig, 16);
        out.write(data.data(), data.size());
    }

    void writeAsciiDocument(std::ostream& out, const FbxNode& root)
    {
        out << "; FBX 7.4.0 project file\n";
        out << "; Generated by osgVerse::SaverFBX\n";
        out << "; ----------------------------------------------------\n\n";
        for (size_t i = 0; i < root.children.size(); ++i) writeAsciiNode(out, *root.children[i], 0);
    }
}

class FbxSceneWriter : public osgVerse::NodeVisitorEx
{
public:
    FbxSceneWriter(const std::string& dir, const std::string& imgHint)
    :   osgVerse::NodeVisitorEx(), _folder(dir), _imageHint(imgHint), _nextID(1000000) {}

    struct TriangleCollector
    {
        std::vector<unsigned int> triangles;
        void operator()(unsigned int i1, unsigned int i2, unsigned int i3)
        {
            if (i1 == i2 || i2 == i3 || i1 == i3) return;
            triangles.push_back(i1); triangles.push_back(i2); triangles.push_back(i3);
        }
    };

    virtual void apply(osg::Node* node, osg::Drawable* drawable, osg::StateSet& ss)
    {
        bool hasMaterial = (ss.getAttribute(osg::StateAttribute::MATERIAL) != NULL);
        if (!hasMaterial && ss.getTextureAttributeList().empty())
        { osgVerse::NodeVisitorEx::apply(node, drawable, ss); return; }

        if (_statesets.find(&ss) == _statesets.end())
        {
            MaterialDef mat; mat.id = _nextID++; mat.name = ss.getName();
            osg::Material* mtl = dynamic_cast<osg::Material*>(
                ss.getAttribute(osg::StateAttribute::MATERIAL));
            if (mtl)
            {
                mat.baseColor = mtl->getDiffuse(osg::Material::FRONT); mat.hasBaseColor = true;
                mat.emissive = mtl->getEmission(osg::Material::FRONT); mat.hasEmissive = true;
                float shininess = mtl->getShininess(osg::Material::FRONT);
                mat.roughness = osg::clampBetween(1.0f - shininess * 0.01f, 0.0f, 1.0f);
                mat.hasRoughness = true;
            }
            if (ss.getRenderingHint() == osg::StateSet::TRANSPARENT_BIN)
            { mat.opacity = mat.baseColor.a(); mat.hasOpacity = true; }

            _statesets[&ss] = (int)_materials.size(); _materials.push_back(mat);
        }
        osgVerse::NodeVisitorEx::apply(node, drawable, ss);
    }

    virtual void apply(osg::Node* node, osg::Drawable* drawable, osg::Texture* tex, int unit)
    {
        osg::StateSet* ss = (tex->getNumParents() > 0) ? tex->getParent(0) : NULL;
        if (!ss || dynamic_cast<osg::Texture2D*>(tex) == NULL) return;
        if (_statesets.find(ss) == _statesets.end()) return;

        // /*0*/"base_color", /*1*/"norm", /*3*/"ao", /*5*/"emit_color"
        std::string key;
        switch (unit)
        {
        case 0: key = "base_color"; break;
        case 1: key = "norm"; break;
        case 3: key = "ao"; break;
        case 5: key = "emit_color"; break;
        default:
            OSG_NOTICE << "[SaverFBX] Texture unit " << unit << " is not exported by FBX writer\n";
            return;
        }

        int texID = createTexture(tex);
        if (texID >= 0) _materials[_statesets[ss]].textureKeys[key] = texID;
    }

    virtual void apply(osg::Geometry& geometry)
    {
        osgVerse::NodeVisitorEx::apply(geometry);

        // Get vertex attribute arrays
        osg::Vec3Array* va = dynamic_cast<osg::Vec3Array*>(geometry.getVertexArray());
        osg::Vec3Array* na = dynamic_cast<osg::Vec3Array*>(geometry.getNormalArray());
        osg::Vec4Array* ca = dynamic_cast<osg::Vec4Array*>(geometry.getColorArray());
        osg::Vec2Array* ta0 = dynamic_cast<osg::Vec2Array*>(geometry.getTexCoordArray(0));
        if (!va || va->empty()) return;

        osg::TriangleIndexFunctor<TriangleCollector> f; geometry.accept(f);
        if (f.triangles.empty()) return;

        size_t vSize = va->size();
        GeometryDef def; def.id = _nextID++; def.name = geometry.getName();
        if (def.name.empty()) def.name = geometry.className();

        // Vertices and polygon indices
        def.vertices.resize(vSize * 3);
        for (size_t i = 0; i < vSize; ++i)
        { const osg::Vec3& p = (*va)[i]; def.vertices[i * 3 + 0] = p[0]; def.vertices[i * 3 + 1] = p[1]; def.vertices[i * 3 + 2] = p[2]; }

        def.polygons.resize(f.triangles.size());
        for (size_t i = 0; i < f.triangles.size(); i += 3)
        {
            def.polygons[i + 0] = (int)f.triangles[i + 0];
            def.polygons[i + 1] = (int)f.triangles[i + 1];
            def.polygons[i + 2] = ~((int)f.triangles[i + 2]);  // last index of a polygon is XOR'ed
        }

        if (na && na->size() == vSize)
        {
            def.normals.resize(vSize * 3);
            for (size_t i = 0; i < vSize; ++i)
            { const osg::Vec3& n = (*na)[i]; def.normals[i * 3 + 0] = n[0]; def.normals[i * 3 + 1] = n[1]; def.normals[i * 3 + 2] = n[2]; }
        }
        if (ta0 && ta0->size() == vSize)
        {
            def.uvs.resize(vSize * 2);
            for (size_t i = 0; i < vSize; ++i)
            { const osg::Vec2& t = (*ta0)[i]; def.uvs[i * 2 + 0] = t[0]; def.uvs[i * 2 + 1] = t[1]; }
        }
        if (ca && ca->size() == vSize)
        {
            def.colors.resize(vSize * 4);
            for (size_t i = 0; i < vSize; ++i)
            { const osg::Vec4& c = (*ca)[i]; def.colors[i * 4 + 0] = c[0]; def.colors[i * 4 + 1] = c[1]; def.colors[i * 4 + 2] = c[2]; def.colors[i * 4 + 3] = c[3]; }
        }

        // Model matrix: decompose the accumulated world matrix into TRS
        osg::Vec3 trans, scale; osg::Quat rot, so;
        if (_matrixStack.size() > 0) _matrixStack.back().decompose(trans, rot, scale, so);
        else { trans.set(0.0f, 0.0f, 0.0f); rot = osg::Quat(); scale.set(1.0f, 1.0f, 1.0f); }

        osg::StateSet* ss = geometry.getStateSet();
        if (!ss && geometry.getNumParents() > 0) ss = geometry.getParent(0)->getStateSet();
        int matIndex = (ss && _statesets.find(ss) != _statesets.end()) ? _statesets[ss] : -1;

        def.modelID = _nextID++; def.modelName = def.name;
        def.translation = trans; def.scale = scale;
        def.rotation = ufbxQuatToEuler(rot);
        def.materialID = (matIndex >= 0) ? _materials[matIndex].id : -1;

        _geometries.push_back(def);
    }

    bool write(std::ostream& out, bool isBinary)
    {
        // Create 1x1 fallback textures so that pure material colors survive a round trip
        for (size_t i = 0; i < _materials.size(); ++i) createFallbackTexture(_materials[i]);

        FbxNode root; buildDocument(root);
        if (isBinary) writeBinaryDocument(out, root);
        else writeAsciiDocument(out, root);
        return out.good();
    }

protected:
    struct TextureDef
    {
        int id, videoID;
        std::string name, relativeFileName;
        std::vector<unsigned char> content;
        bool embedded;
        int wrapS, wrapT;
        TextureDef() : id(0), videoID(-1), embedded(false), wrapS(0), wrapT(0) {}
    };
    struct MaterialDef
    {
        int id; std::string name;
        std::map<std::string, int> textureKeys;  // FBX prop key ("base_color") -> texture id
        osg::Vec4 baseColor, emissive;
        float roughness, metalness, ao, opacity;
        bool hasBaseColor, hasEmissive, hasRoughness, hasMetalness, hasAO, hasOpacity;
        MaterialDef()
        :   baseColor(1.0f, 1.0f, 1.0f, 1.0f), emissive(0.0f, 0.0f, 0.0f, 0.0f),
            roughness(0.5f), metalness(0.0f), ao(1.0f), opacity(1.0f),
            hasBaseColor(false), hasEmissive(false), hasRoughness(false), hasMetalness(false),
            hasAO(false), hasOpacity(false) {}
    };
    struct GeometryDef
    {
        int id, modelID, materialID; std::string name, modelName;
        osg::Vec3 translation, rotation, scale;
        std::vector<double> vertices, normals, uvs, colors;
        std::vector<int> polygons;
        GeometryDef() : id(0), modelID(0), materialID(-1),
            translation(0.0f, 0.0f, 0.0f), rotation(0.0f, 0.0f, 0.0f), scale(1.0f, 1.0f, 1.0f) {}
    };

    static osg::Vec3 ufbxQuatToEuler(const osg::Quat& q)
    {
        ufbx_quat uq; uq.x = q.x(); uq.y = q.y(); uq.z = q.z(); uq.w = q.w();
        ufbx_vec3 e = ufbx_quat_to_euler(uq, UFBX_ROTATION_ORDER_XYZ);
        return osg::Vec3(e.x, e.y, e.z);
    }

    int createTexture(osg::Texture* tex)
    {
        osg::Image* image = tex->getImage(0);
        if (!image || !image->valid()) return -1;
        if (_images.find(image) != _images.end()) return _textures[_images[image]].id;

        TextureDef def; def.id = _nextID++;
        def.wrapS = toWrap(tex->getWrap(osg::Texture::WRAP_S));
        def.wrapT = toWrap(tex->getWrap(osg::Texture::WRAP_T));

        std::string ext, baseName = osgDB::getNameLessExtension(
            osgDB::getSimpleFileName(image->getFileName()));
        if (baseName.empty()) baseName = "texture";
        std::vector<unsigned char> data; bool ok = encodeImage(image, data, ext);
        if (!ok) { OSG_WARN << "[SaverFBX] Unable to encode image of texture " << tex->getName() << "\n"; }
        else
        {
            def.name = makeUniqueName(baseName, ext, def.id);
            bool useExternal = (_imageHint == "UseExternal") || _imageHint.find("External") != std::string::npos;
            if (!image->isCompressed() && !useExternal)
            { def.embedded = true; def.videoID = _nextID++; def.content.swap(data); }
            else if (!writeExternalFile(def.name, data))
                { OSG_WARN << "[SaverFBX] Failed writing external image: " << def.name << "\n"; }
        }
        def.relativeFileName = def.name;

        _images[image] = (int)_textures.size(); _textures.push_back(def);
        return def.id;
    }

    void createFallbackTexture(MaterialDef& mat)
    {
        if (mat.textureKeys.find("base_color") != mat.textureKeys.end() || !mat.hasBaseColor) return;
        osg::Vec4ub c((unsigned char)(mat.baseColor[0] * 255.0f + 0.5f), (unsigned char)(mat.baseColor[1] * 255.0f + 0.5f),
                      (unsigned char)(mat.baseColor[2] * 255.0f + 0.5f), (unsigned char)(mat.baseColor[3] * 255.0f + 0.5f));
        osg::ref_ptr<osg::Image> image = new osg::Image;
        image->allocateImage(1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE); image->setInternalTextureFormat(GL_RGBA8);
        osg::Vec4ub* ptr = (osg::Vec4ub*)image->data(); *ptr = c;
        image->setFileName("Color_" + mat.name + ".png");

        osg::ref_ptr<osg::Texture2D> tex = new osg::Texture2D;
        tex->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT);
        tex->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT);
        tex->setImage(image.get());
        int texID = createTexture(tex.get());
        if (texID >= 0) mat.textureKeys["base_color"] = texID;
    }

    bool encodeImage(osg::Image* image, std::vector<unsigned char>& data, std::string& ext)
    {
        if (image->isCompressed())
        {
            data.assign(image->data(), image->data() + image->getTotalSizeInBytes());
            ext = osgDB::getLowerCaseFileExtension(image->getFileName());
            if (ext.empty()) ext = "dds"; return true;
        }

        int w = image->s(), h = image->t(), comp = osg::Image::computeNumComponents(image->getPixelFormat());
        if ((w <= 0) || (h <= 0) || (comp < 1) || (comp > 4)) return false;
        if ((image->getDataType() != GL_UNSIGNED_BYTE) && (image->getDataType() != GL_BYTE)) return false;

        if (image->isImageTranslucent() || comp < 3)
        {
            ext = "png";
            stbi_write_png_to_func(FbxSceneWriter::writeToMemory, &data, w, h, comp, image->data(), 0);
        }
        else
        {
            ext = "jpg";
            stbi_write_jpg_to_func(FbxSceneWriter::writeToMemory, &data, w, h, comp, image->data(), 98);
        }
        return !data.empty();
    }

    static void writeToMemory(void* context, void* data, int size)
    {
        std::vector<unsigned char>* buffer = reinterpret_cast<std::vector<unsigned char>*>(context);
        unsigned char* pData = reinterpret_cast<unsigned char*>(data);
        buffer->insert(buffer->end(), pData, pData + size);
    }

    bool writeExternalFile(const std::string& name, const std::vector<unsigned char>& data)
    {
        std::string fileName = _folder.empty() ? name : (_folder + "/" + name);
        std::ofstream out(fileName.c_str(), std::ios::out | std::ios::binary);
        if (!out) return false;
        if (!data.empty()) out.write((const char*)&data[0], data.size());
        return out.good();
    }

    std::string makeUniqueName(const std::string& baseName, const std::string& ext, int id)
    {
        std::string result = baseName + "." + ext;
        if (_usedNames.find(result) != _usedNames.end())
            result = baseName + "_" + std::to_string(id) + "." + ext;
        _usedNames.insert(result); return result;
    }

    static int toWrap(osg::Texture::WrapMode mode)  // ufbx: 0 = repeat, 1 = clamp
    {
        switch (mode)
        {
        case osg::Texture::CLAMP: case osg::Texture::CLAMP_TO_EDGE:
        case osg::Texture::CLAMP_TO_BORDER: return 1;
        default: return 0;  // REPEAT / MIRROR are both exported as repeating
        }
    }

    // ---- Build the generic FBX node tree ----
    void buildDocument(FbxNode& root)
    {
        // FBXHeaderExtension
        FbxNode* header = addNode(root, "FBXHeaderExtension");
        addNode(*header, "FBXHeaderVersion")->prop(FbxProp::I32(1003));
        addNode(*header, "FBXVersion")->prop(FbxProp::I32(FBX_VERSION));
        FbxNode* cts = addNode(*header, "CreationTimeStamp");
        addNode(*cts, "Version")->prop(FbxProp::I32(1000));
        addNode(*cts, "Year")->prop(FbxProp::I32(2024));
        addNode(*cts, "Month")->prop(FbxProp::I32(1));
        addNode(*cts, "Day")->prop(FbxProp::I32(1));
        addNode(*cts, "Hour")->prop(FbxProp::I32(0));
        addNode(*cts, "Minute")->prop(FbxProp::I32(0));
        addNode(*cts, "Second")->prop(FbxProp::I32(0));
        addNode(*cts, "Millisecond")->prop(FbxProp::I32(0));
        addNode(*header, "Creator")->prop(FbxProp::S("osgVerse::SaverFBX"));

        // GlobalSettings (Z-up, 1 unit = 1 meter to match LoadSceneFBX's target axes)
        FbxNode* gs = addNode(root, "GlobalSettings");
        addNode(*gs, "Version")->prop(FbxProp::I32(1000));
        FbxNode* gsp = addNode(*gs, "Properties70");
        addProp70i(*gsp, "UpAxis", "int", "Integer", "", 2);
        addProp70i(*gsp, "UpAxisSign", "int", "Integer", "", 1);
        addProp70i(*gsp, "FrontAxis", "int", "Integer", "", 1);
        addProp70i(*gsp, "FrontAxisSign", "int", "Integer", "", -1);
        addProp70i(*gsp, "CoordAxis", "int", "Integer", "", 0);
        addProp70i(*gsp, "CoordAxisSign", "int", "Integer", "", 1);
        addProp70i(*gsp, "OriginalUpAxis", "int", "Integer", "", 2);
        addProp70i(*gsp, "OriginalUpAxisSign", "int", "Integer", "", 1);
        addProp70(*gsp, "UnitScaleFactor", "double", "Number", "", std::vector<double>{100.0});
        addProp70(*gsp, "OriginalUnitScaleFactor", "double", "Number", "", std::vector<double>{100.0});

        // Definitions
        FbxNode* defs = addNode(root, "Definitions");
        size_t total = 1 + _geometries.size() * 2 + _materials.size() + _textures.size() + countVideos();
        addNode(*defs, "Version")->prop(FbxProp::I32(100));
        addNode(*defs, "Count")->prop(FbxProp::I32((int64_t)total));
        addDefinition(*defs, "GlobalSettings", 1);
        addDefinition(*defs, "Model", (int64_t)_geometries.size());
        addDefinition(*defs, "Geometry", (int64_t)_geometries.size());
        addDefinition(*defs, "Material", (int64_t)_materials.size());
        addDefinition(*defs, "Texture", (int64_t)_textures.size());
        addDefinition(*defs, "Video", (int64_t)countVideos());

        // Objects
        FbxNode* objects = addNode(root, "Objects");
        for (size_t i = 0; i < _geometries.size(); ++i) buildGeometry(*objects, _geometries[i]);
        for (size_t i = 0; i < _geometries.size(); ++i) buildModel(*objects, _geometries[i]);
        for (size_t i = 0; i < _materials.size(); ++i) buildMaterial(*objects, _materials[i]);
        for (size_t i = 0; i < _textures.size(); ++i) buildTexture(*objects, _textures[i]);
        for (size_t i = 0; i < _textures.size(); ++i) buildVideo(*objects, _textures[i]);

        // Connections
        FbxNode* conns = addNode(root, "Connections");
        for (size_t i = 0; i < _geometries.size(); ++i)
        {
            const GeometryDef& def = _geometries[i];
            addNode(*conns, "C")->prop(FbxProp::S("OO")).prop(FbxProp::I32(def.modelID)).prop(FbxProp::I32(0));
            addNode(*conns, "C")->prop(FbxProp::S("OO")).prop(FbxProp::I32(def.id)).prop(FbxProp::I32(def.modelID));
            if (def.materialID >= 0)
                addNode(*conns, "C")->prop(FbxProp::S("OO")).prop(FbxProp::I32(def.materialID)).prop(FbxProp::I32(def.modelID));
        }
        for (size_t i = 0; i < _materials.size(); ++i)
        {
            const MaterialDef& mat = _materials[i];
            for (std::map<std::string, int>::const_iterator it = mat.textureKeys.begin();
                 it != mat.textureKeys.end(); ++it)
                addNode(*conns, "C")->prop(FbxProp::S("OP")).prop(FbxProp::I32(it->second)).prop(FbxProp::I32(mat.id))
                    .prop(FbxProp::S("3dsMax|main|" + it->first + "_map"));
        }
        for (size_t i = 0; i < _textures.size(); ++i)
        {
            if (_textures[i].embedded)
                addNode(*conns, "C")->prop(FbxProp::S("OO")).prop(FbxProp::I32(_textures[i].videoID)).prop(FbxProp::I32(_textures[i].id));
        }
    }

    void buildGeometry(FbxNode& parent, const GeometryDef& def)
    {
        FbxNode* n = addNode(parent, "Geometry");
        n->prop(FbxProp::I32(def.id)).prop(FbxProp::S("Geometry::" + def.name)).prop(FbxProp::S("Mesh"));
        addNode(*n, "Vertices")->prop(FbxProp::ARd(def.vertices));
        addNode(*n, "PolygonVertexIndex")->prop(FbxProp::ARi(std::vector<int32_t>(def.polygons.begin(), def.polygons.end())));
        addNode(*n, "GeometryVersion")->prop(FbxProp::I32(124));
        if (!def.normals.empty())
        {
            FbxNode* le = addNode(*n, "LayerElementNormal");
            addNode(*le, "Version")->prop(FbxProp::I32(101));
            addNode(*le, "Name")->prop(FbxProp::S(""));
            addNode(*le, "MappingInformationType")->prop(FbxProp::S("ByVertice"));
            addNode(*le, "ReferenceInformationType")->prop(FbxProp::S("Direct"));
            addNode(*le, "Normals")->prop(FbxProp::ARd(def.normals));
        }
        if (!def.uvs.empty())
        {
            FbxNode* le = addNode(*n, "LayerElementUV");
            addNode(*le, "Version")->prop(FbxProp::I32(101));
            addNode(*le, "Name")->prop(FbxProp::S("UVMap"));
            addNode(*le, "MappingInformationType")->prop(FbxProp::S("ByVertice"));
            addNode(*le, "ReferenceInformationType")->prop(FbxProp::S("Direct"));
            addNode(*le, "UV")->prop(FbxProp::ARd(def.uvs));
        }
        if (!def.colors.empty())
        {
            FbxNode* le = addNode(*n, "LayerElementColor");
            addNode(*le, "Version")->prop(FbxProp::I32(101));
            addNode(*le, "Name")->prop(FbxProp::S(""));
            addNode(*le, "MappingInformationType")->prop(FbxProp::S("ByVertice"));
            addNode(*le, "ReferenceInformationType")->prop(FbxProp::S("Direct"));
            addNode(*le, "Colors")->prop(FbxProp::ARd(def.colors));
        }
        FbxNode* lem = addNode(*n, "LayerElementMaterial");
        addNode(*lem, "Version")->prop(FbxProp::I32(101));
        addNode(*lem, "Name")->prop(FbxProp::S(""));
        addNode(*lem, "MappingInformationType")->prop(FbxProp::S("AllSame"));
        addNode(*lem, "ReferenceInformationType")->prop(FbxProp::S("IndexToDirect"));
        addNode(*lem, "Materials")->prop(FbxProp::ARi(std::vector<int32_t>{0}));
    }

    void buildModel(FbxNode& parent, const GeometryDef& def)
    {
        FbxNode* n = addNode(parent, "Model");
        n->prop(FbxProp::I32(def.modelID)).prop(FbxProp::S("Model::" + def.modelName)).prop(FbxProp::S("Mesh"));
        addNode(*n, "Version")->prop(FbxProp::I32(232));
        FbxNode* p70 = addNode(*n, "Properties70");
        addProp70(*p70, "Lcl Translation", "Lcl Translation", "", "A",
                  std::vector<double>{def.translation[0], def.translation[1], def.translation[2]});
        addProp70(*p70, "Lcl Rotation", "Lcl Rotation", "", "A",
                  std::vector<double>{def.rotation[0], def.rotation[1], def.rotation[2]});
        addProp70(*p70, "Lcl Scaling", "Lcl Scaling", "", "A",
                  std::vector<double>{def.scale[0], def.scale[1], def.scale[2]});
        addNode(*n, "Shading")->prop(FbxProp::I32(1));
        addNode(*n, "Culling")->prop(FbxProp::S("CullingOff"));
    }

    void buildMaterial(FbxNode& parent, const MaterialDef& mat)
    {
        FbxNode* n = addNode(parent, "Material");
        n->prop(FbxProp::I32(mat.id)).prop(FbxProp::S("Material::" + mat.name)).prop(FbxProp::S(""));
        addNode(*n, "Version")->prop(FbxProp::I32(102));
        addNode(*n, "ShadingModel")->prop(FbxProp::S("unknown"));
        addNode(*n, "MultiLayer")->prop(FbxProp::I32(0));
        FbxNode* p70 = addNode(*n, "Properties70");
        // 3ds Max PBR (metal-rough) class IDs make ufbx map properties to pbr.* maps
        addProp70i(*p70, "3dsMax|ClassIDa", "int", "Integer", "", (int64_t)(int32_t)(uint32_t)0xD00F1E00u);
        addProp70i(*p70, "3dsMax|ClassIDb", "int", "Integer", "", (int64_t)(int32_t)(uint32_t)0xBE77E500u);
        if (mat.hasBaseColor)
            addProp70(*p70, "3dsMax|main|base_color", "Color", "", "A",
                      std::vector<double>{mat.baseColor[0], mat.baseColor[1], mat.baseColor[2]});
        if (mat.hasRoughness)
            addProp70(*p70, "3dsMax|main|roughness", "Number", "", "A", std::vector<double>{mat.roughness});
        if (mat.hasMetalness)
            addProp70(*p70, "3dsMax|main|metalness", "Number", "", "A", std::vector<double>{mat.metalness});
        if (mat.hasAO)
            addProp70(*p70, "3dsMax|main|ao", "Number", "", "A", std::vector<double>{mat.ao});
        if (mat.hasEmissive)
            addProp70(*p70, "3dsMax|main|emit_color", "Color", "", "A",
                      std::vector<double>{mat.emissive[0], mat.emissive[1], mat.emissive[2]});
        if (mat.hasOpacity)
            addProp70(*p70, "3dsMax|main|opacity", "Number", "", "A", std::vector<double>{mat.opacity});
    }

    void buildTexture(FbxNode& parent, const TextureDef& def)
    {
        FbxNode* n = addNode(parent, "Texture");
        n->prop(FbxProp::I32(def.id)).prop(FbxProp::S("Texture::" + def.name)).prop(FbxProp::S(""));
        addNode(*n, "Type")->prop(FbxProp::S("TextureVideoClip"));
        addNode(*n, "Version")->prop(FbxProp::I32(202));
        addNode(*n, "TextureName")->prop(FbxProp::S("Texture::" + def.name));
        if (def.embedded) addNode(*n, "Media")->prop(FbxProp::S("Video::" + def.name));
        addNode(*n, "FileName")->prop(FbxProp::S(def.relativeFileName));
        addNode(*n, "RelativeFilename")->prop(FbxProp::S(def.relativeFileName));
        addNode(*n, "ModelUVTranslation")->prop(FbxProp::D(0.0)).prop(FbxProp::D(0.0));
        addNode(*n, "ModelUVScaling")->prop(FbxProp::D(1.0)).prop(FbxProp::D(1.0));
        addNode(*n, "Texture_Alpha_Source")->prop(FbxProp::S("None"));
        FbxNode* crop = addNode(*n, "Cropping");
        crop->prop(FbxProp::D(0.0)).prop(FbxProp::D(0.0)).prop(FbxProp::D(0.0)).prop(FbxProp::D(0.0));
        FbxNode* p70 = addNode(*n, "Properties70");
        addProp70i(*p70, "WrapModeU", "enum", "", "", def.wrapS);
        addProp70i(*p70, "WrapModeV", "enum", "", "", def.wrapT);
    }

    void buildVideo(FbxNode& parent, const TextureDef& def)
    {
        if (!def.embedded) return;
        FbxNode* n = addNode(parent, "Video");
        n->prop(FbxProp::I32(def.videoID)).prop(FbxProp::S("Video::" + def.name)).prop(FbxProp::S("Clip"));
        addNode(*n, "Type")->prop(FbxProp::S("Clip"));
        addNode(*n, "Version")->prop(FbxProp::I32(202));
        addNode(*n, "UseMipMap")->prop(FbxProp::I32(0));
        addNode(*n, "Filename")->prop(FbxProp::S(def.relativeFileName));
        addNode(*n, "RelativeFilename")->prop(FbxProp::S(def.relativeFileName));
        addNode(*n, "Content")->prop(FbxProp::R(def.content));
    }

    size_t countVideos() const
    { size_t n = 0; for (size_t i = 0; i < _textures.size(); ++i) { if (_textures[i].embedded) n++; } return n; }

    std::map<osg::StateSet*, int> _statesets;     // stateset -> index in _materials
    std::map<osg::Image*, int> _images;           // image -> index in _textures
    std::set<std::string> _usedNames;
    std::vector<MaterialDef> _materials;
    std::vector<TextureDef> _textures;
    std::vector<GeometryDef> _geometries;
    std::string _folder, _imageHint;
    int _nextID;
};

namespace osgVerse
{
    SaverFBX::SaverFBX(const osg::Node& node, std::ostream& out, const std::string& dir,
                       const std::string& imgHint, bool isBinary)
    {
        FbxSceneWriter writer(dir, imgHint);
        const_cast<osg::Node&>(node).accept(writer);
        _done = writer.write(out, isBinary);
    }

    SaverFBX::~SaverFBX() {}

    bool saveFbx(const osg::Node& node, const std::string& file,
                 const std::string& imgHint, bool isBinary)
    {
        std::string workDir = osgDB::getFilePath(file);
        std::ofstream out(file.c_str(), std::ios::out | std::ios::binary);
        if (!out)
        {
            OSG_WARN << "[SaverFBX] file " << file << " not writable" << std::endl;
            return false;
        }

        osg::ref_ptr<SaverFBX> saver = new SaverFBX(node, out, workDir, imgHint, isBinary);
        return saver->getResult();
    }

    bool saveFbx2(const osg::Node& node, std::ostream& out, const std::string& dir,
                  const std::string& imgHint, bool isBinary)
    {
        osg::ref_ptr<SaverFBX> saver = new SaverFBX(node, out, dir, imgHint, isBinary);
        return saver->getResult();
    }
}
