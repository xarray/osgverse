#include <osg/io_utils>
#include <osg/Version>
#include <osg/BlendFunc>
#include <osg/Material>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "modeling/Utilities.h"
#include <unordered_map>

#include <osg/Geometry>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <stdexcept>

namespace
{
    class PlyReader
    {
    public:
        enum Type { CHAR, UCHAR, SHORT, USHORT, INT, UINT, FLOAT, DOUBLE, LIST };
        struct Property
        {
            Type type, listCountType, listItemType;
            std::string name; bool isList = false;
        };

        struct Element
        {
            std::string name; size_t count = 0;
            std::vector<Property> properties;
        };

        struct Vertex
        {
            osg::Vec3 position;
            osg::Vec3 normal;
            osg::Vec4 color;
            osg::Vec2 texcoord;
            bool hasNormal = false;
            bool hasColor = false;
            bool hasTexcoord = false;
        };

        osg::ref_ptr<osg::Geometry> read(std::istream& file)
        {
            bool isBinary = false, isBigEndian = false;
            std::vector<Element> elements;
            std::map<std::string, std::map<std::string, size_t>> propIndexMap; // element->property->index
            
            std::string line; std::getline(file, line);
            if (line.find("ply") != 0) return NULL;  // first line

            // Header data
            Element* currentElement = nullptr;
            while (std::getline(file, line))
            {
                std::istringstream iss(line);
                std::string keyword; iss >> keyword;

                if (keyword == "format")
                {
                    std::string format, version; iss >> format >> version;
                    if (format == "ascii") isBinary = false;
                    else if (format == "binary_little_endian") { isBinary = true; isBigEndian = false; }
                    else if (format == "binary_big_endian") { isBinary = true; isBigEndian = true; }
                    else { throw std::runtime_error("Unsupported format: " + format); }
                }
                else if (keyword == "element")
                {
                    std::string name; size_t count = 0; iss >> name >> count;
                    elements.push_back({name, count, {}});
                    currentElement = &elements.back(); propIndexMap[name] = {};
                }
                else if (keyword == "property")
                {
                    if (!currentElement) { throw std::runtime_error("Invalid property before any elements"); }
                    parseProperty(iss, *currentElement, propIndexMap[currentElement->name]);
                }
                else if (keyword == "end_header")
                    break;
            }

            // Find vertex and face elements
            const Element* vertexElem = nullptr;
            const Element* faceElem = nullptr;
            size_t vertexElemIdx = 0, faceElemIdx = 0;
            for (size_t i = 0; i < elements.size(); ++i)
            {
                if (elements[i].name == "vertex") { vertexElem = &elements[i]; vertexElemIdx = i; }
                else if (elements[i].name == "face") { faceElem = &elements[i]; faceElemIdx = i; }
            }
            if (!vertexElem) { throw std::runtime_error("No vertex found"); }

            // Load real data
            std::vector<Vertex> vertices;
            std::vector<std::vector<unsigned int>> faces;
            if (isBinary)
                vertices = readVerticesBinary(file, *vertexElem, propIndexMap["vertex"], isBigEndian);
            else
                vertices = readVerticesAscii(file, *vertexElem, propIndexMap["vertex"]);

            if (faceElem)
            {
                if (isBinary)
                    faces = readFacesBinary(file, *faceElem, propIndexMap["face"], isBigEndian);
                else
                    faces = readFacesAscii(file, *faceElem, propIndexMap["face"]);
            }
            return buildGeometry(vertices, faces);
        }

    private:
        Type stringToType(const std::string& s)
        {
            if (s == "char" || s == "int8") return Type::CHAR;
            if (s == "uchar" || s == "uint8") return Type::UCHAR;
            if (s == "short") return Type::SHORT;
            if (s == "ushort") return Type::USHORT;
            if (s == "int" || s == "int32") return Type::INT;
            if (s == "uint" || s == "uint32") return Type::UINT;
            if (s == "float" || s == "float32") return Type::FLOAT;
            if (s == "double" || s == "float64") return Type::DOUBLE;
            throw std::runtime_error("Unknown type: " + s);
        }

        void parseProperty(std::istringstream& iss, Element& elem, std::map<std::string, size_t>& propMap)
        {
            Property prop; std::string typeOrList; iss >> typeOrList;
            if (typeOrList == "list")
            {
                std::string countType, itemType; prop.isList = true;
                iss >> countType >> itemType >> prop.name;
                prop.listCountType = stringToType(countType);
                prop.listItemType = stringToType(itemType);
            }
            else
                { prop.type = stringToType(typeOrList); iss >> prop.name; }
            propMap[prop.name] = elem.properties.size();
            elem.properties.push_back(prop);
        }

        template<typename T> T readBinaryValue(std::istream& file, bool isBigEndian)
        {
            T value; file.read(reinterpret_cast<char*>(&value), sizeof(T));
            if (isBigEndian)
            {
                char* bytes = reinterpret_cast<char*>(&value);
                std::reverse(bytes, bytes + sizeof(T));
            }
            return value;
        }

        std::vector<Vertex> readVerticesAscii(std::istream& file, const Element& elem,
                                              const std::map<std::string, size_t>& propMap)
        {
            std::vector<Vertex> vertices;
            vertices.reserve(elem.count);

            auto getPropIdx = [&](const std::string& name) -> int
            {
                auto it = propMap.find(name);
                return (it != propMap.end()) ? static_cast<int>(it->second) : -1;
            };
            int xIdx = getPropIdx("x"), yIdx = getPropIdx("y"), zIdx = getPropIdx("z");
            int nxIdx = getPropIdx("nx"), nyIdx = getPropIdx("ny"), nzIdx = getPropIdx("nz");
            int rIdx = getPropIdx("red"), gIdx = getPropIdx("green"), bIdx = getPropIdx("blue"), aIdx = getPropIdx("alpha");
            int uIdx = getPropIdx("s") != -1 ? getPropIdx("s") : getPropIdx("u");
            int vIdx = getPropIdx("t") != -1 ? getPropIdx("t") : getPropIdx("v");

            std::string line;
            for (size_t i = 0; i < elem.count && std::getline(file, line); ++i)
            {
                std::istringstream iss(line); std::vector<double> values;
                double val = 0; while (iss >> val) values.push_back(val);
                if (values.size() < elem.properties.size()) continue;

                Vertex v;
                if (xIdx >= 0 && xIdx < (int)values.size()) v.position.x() = values[xIdx];
                if (yIdx >= 0 && yIdx < (int)values.size()) v.position.y() = values[yIdx];
                if (zIdx >= 0 && zIdx < (int)values.size()) v.position.z() = values[zIdx];

                if (nxIdx >= 0 && nyIdx >= 0 && nzIdx >= 0 && 
                    nxIdx < (int)values.size() && nyIdx < (int)values.size() && nzIdx < (int)values.size())
                {
                    v.normal.set(values[nxIdx], values[nyIdx], values[nzIdx]);
                    v.hasNormal = true;
                }

                if (rIdx >= 0 && gIdx >= 0 && bIdx >= 0 &&
                    rIdx < (int)values.size() && gIdx < (int)values.size() && bIdx < (int)values.size())
                {
                    float r = values[rIdx] / 255.0f, g = values[gIdx] / 255.0f, b = values[bIdx] / 255.0f;
                    float a = (aIdx >= 0 && aIdx < (int)values.size()) ? values[aIdx] / 255.0f : 1.0f;
                    v.color.set(r, g, b, a); v.hasColor = true;
                }

                if (uIdx >= 0 && vIdx >= 0 && 
                    uIdx < (int)values.size() && vIdx < (int)values.size())
                {
                    v.texcoord.set(values[uIdx], values[vIdx]);
                    v.hasTexcoord = true;
                }
                vertices.push_back(v);
            }
            return vertices;
        }

        std::vector<Vertex> readVerticesBinary(std::istream& file, const Element& elem,
                                               const std::map<std::string, size_t>& propMap,
                                               bool isBigEndian)
        {
            std::vector<Vertex> vertices; vertices.reserve(elem.count);
            auto getPropIdx = [&](const std::string& name) -> int
            {
                auto it = propMap.find(name);
                return (it != propMap.end()) ? static_cast<int>(it->second) : -1;
            };

            int xIdx = getPropIdx("x"), yIdx = getPropIdx("y"), zIdx = getPropIdx("z");
            int nxIdx = getPropIdx("nx"), nyIdx = getPropIdx("ny"), nzIdx = getPropIdx("nz");
            int rIdx = getPropIdx("red"), gIdx = getPropIdx("green"), bIdx = getPropIdx("blue"), aIdx = getPropIdx("alpha");
            int uIdx = getPropIdx("s") != -1 ? getPropIdx("s") : getPropIdx("u");
            int vIdx = getPropIdx("t") != -1 ? getPropIdx("t") : getPropIdx("v");
            for (size_t i = 0; i < elem.count; ++i)
            {
                std::vector<double> values(elem.properties.size(), 0.0);
                for (size_t p = 0; p < elem.properties.size(); ++p)
                {
                    const auto& prop = elem.properties[p];
                    switch (prop.type)
                    {
                        case Type::CHAR: values[p] = static_cast<double>(readBinaryValue<int8_t>(file, isBigEndian)); break;
                        case Type::UCHAR: values[p] = static_cast<double>(readBinaryValue<uint8_t>(file, isBigEndian)); break;
                        case Type::SHORT: values[p] = static_cast<double>(readBinaryValue<int16_t>(file, isBigEndian)); break;
                        case Type::USHORT: values[p] = static_cast<double>(readBinaryValue<uint16_t>(file, isBigEndian)); break;
                        case Type::INT: values[p] = static_cast<double>(readBinaryValue<int32_t>(file, isBigEndian)); break;
                        case Type::UINT: values[p] = static_cast<double>(readBinaryValue<uint32_t>(file, isBigEndian)); break;
                        case Type::FLOAT: values[p] = static_cast<double>(readBinaryValue<float>(file, isBigEndian)); break;
                        case Type::DOUBLE: values[p] = readBinaryValue<double>(file, isBigEndian); break;
                        default: break;
                    }
                }

                Vertex v;
                if (xIdx >= 0) v.position.x() = values[xIdx];
                if (yIdx >= 0) v.position.y() = values[yIdx];
                if (zIdx >= 0) v.position.z() = values[zIdx];
                if (nxIdx >= 0 && nyIdx >= 0 && nzIdx >= 0)
                {
                    v.normal.set(values[nxIdx], values[nyIdx], values[nzIdx]);
                    v.hasNormal = true;
                }

                if (rIdx >= 0 && gIdx >= 0 && bIdx >= 0)
                {
                    float r = values[rIdx] / 255.0f, g = values[gIdx] / 255.0f, b = values[bIdx] / 255.0f;
                    float a = (aIdx >= 0) ? values[aIdx] / 255.0f : 1.0f;
                    v.color.set(r, g, b, a); v.hasColor = true;
                }

                if (uIdx >= 0 && vIdx >= 0)
                {
                    v.texcoord.set(values[uIdx], values[vIdx]);
                    v.hasTexcoord = true;
                }
                vertices.push_back(v);
            }
            return vertices;
        }
        
        std::vector<std::vector<unsigned int>> readFacesAscii(std::istream& file, const Element& elem,
                                                              const std::map<std::string, size_t>& propMap)
        {
            std::string line; std::vector<std::vector<unsigned int>> faces; faces.reserve(elem.count);
            for (size_t i = 0; i < elem.count && std::getline(file, line); ++i)
            {
                std::vector<unsigned int> indices; std::istringstream iss(line);
                int count = 0; iss >> count; indices.reserve(count);

                unsigned int idx = 0;
                while (iss >> idx) indices.push_back(idx);
                if (indices.size() == static_cast<size_t>(count))
                    faces.push_back(indices);
            }
            return faces;
        }

        std::vector<std::vector<unsigned int>> readFacesBinary(std::istream& file, const Element& elem,
                                                               const std::map<std::string, size_t>& propMap,
                                                               bool isBigEndian)
        {
            std::vector<std::vector<unsigned int>> faces; faces.reserve(elem.count);
            for (size_t i = 0; i < elem.count; ++i)
            {
                uint8_t count = readBinaryValue<uint8_t>(file, isBigEndian);
                std::vector<unsigned int> indices; indices.reserve(count);
                for (uint8_t j = 0; j < count; ++j)
                    indices.push_back(readBinaryValue<uint32_t>(file, isBigEndian));
                faces.push_back(indices);
            }
            return faces;
        }

        osg::ref_ptr<osg::Geometry> buildGeometry(const std::vector<Vertex>& vertices,
                                                  const std::vector<std::vector<unsigned int>>& faces)
        {
            osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array;
            verts->reserve(vertices.size());
            
            bool hasNormal = false, hasColor = false, hasTexcoord = false;
            for (const auto& v : vertices)
            {
                if (v.hasNormal) hasNormal = true;
                if (v.hasColor) hasColor = true;
                if (v.hasTexcoord) hasTexcoord = true;
                verts->push_back(v.position);
            }
            
            osg::ref_ptr<osg::Vec3Array> norms;
            if (hasNormal)
            {
                norms = new osg::Vec3Array; norms->reserve(vertices.size());
                for (const auto& v : vertices)
                    norms->push_back(v.hasNormal ? v.normal : osg::Vec3(0.0f, 1.0f, 0.0f));
            }

            osg::ref_ptr<osg::Vec4Array> colors;
            if (hasColor)
            {
                colors = new osg::Vec4Array; colors->reserve(vertices.size());
                for (const auto& v : vertices)
                    colors->push_back(v.hasColor ? v.color : osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
            }

            osg::ref_ptr<osg::Vec2Array> uvs;
            if (hasTexcoord)
            {
                uvs = new osg::Vec2Array; uvs->reserve(vertices.size());
                for (const auto& v : vertices)
                    uvs->push_back(v.hasTexcoord ? v.texcoord : osg::Vec2(0.0f, 0.0f));
            }

            osg::ref_ptr<osg::Geometry> geometry;
            if (faces.empty())
            {
                geometry = osgVerse::createGeometry(verts.get(), norms.get(), uvs.get(),
                                                    new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices.size()));
            }
            else
            {
                osg::ref_ptr<osg::DrawElementsUInt> triangles = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES);
                for (const auto& face : faces)
                {
                    if (face.size() == 3)
                        { triangles->push_back(face[0]); triangles->push_back(face[1]); triangles->push_back(face[2]); }
                    else if (face.size() == 4)
                    {
                        triangles->push_back(face[0]); triangles->push_back(face[1]); triangles->push_back(face[2]);
                        triangles->push_back(face[0]); triangles->push_back(face[2]); triangles->push_back(face[3]);
                    }
                    else if (face.size() > 4)
                    {
                        for (size_t i = 2; i < face.size(); ++i)
                        { triangles->push_back(face[0]); triangles->push_back(face[i-1]); triangles->push_back(face[i]); }
                    }
                }
                geometry = osgVerse::createGeometry(verts.get(), norms.get(), uvs.get(), triangles.get());
            }

            if (geometry.valid() && hasColor)
            {
                geometry->setColorArray(colors.get());
                geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
            }
            return geometry;
        }
    };

    class PlyWriter
    {
    public:
        enum Format { ASCII, BINARY_LITTLE_ENDIAN, BINARY_BIG_ENDIAN };
        struct WriteOptions
        {
            Format format; bool writeAlpha = false;
            WriteOptions() : format(BINARY_LITTLE_ENDIAN), writeAlpha(false) {}
        };

        bool write(std::ostream& file, osg::Node* node, const WriteOptions& options = WriteOptions())
        {
            osgVerse::MeshCollector mc;
            mc.setWeldingVertices(true); mc.setUseGlobalVertices(true);
            if (node) node->accept(mc); else return false;

            const std::vector<osg::Vec3>& positions = mc.getVertices();
            const std::vector<unsigned int>& indices = mc.getTriangles();
            std::vector<osg::Vec4>& nAttr = mc.getAttributes(osgVerse::MeshCollector::NormalAttr);
            std::vector<osg::Vec4>& cAttr = mc.getAttributes(osgVerse::MeshCollector::ColorAttr);
            std::vector<osg::Vec4>& tAttr = mc.getAttributes(osgVerse::MeshCollector::UvAttr);
            bool hasNormals = !nAttr.empty(), hasColors = !cAttr.empty(), hasTexcoords = !tAttr.empty();

            std::vector<std::vector<unsigned int>> faces;
            for (size_t i = 0; i < indices.size(); i += 3)
            {
                std::vector<unsigned int> t;
                for (int k = 0; k < 3; ++k) t.push_back(indices[i + k]);
                faces.push_back(t);
            }

            // Write header and body
            writeHeader(file, positions.size(), faces.size(), hasNormals, hasColors, hasTexcoords, options);
            if (options.format == ASCII)
            {
                writeVerticesAscii(file, positions, nAttr, cAttr, tAttr,
                                   hasNormals, hasColors, hasTexcoords, options);
                writeFacesAscii(file, faces);
            }
            else
            {
                writeVerticesBinary(file, positions, nAttr, cAttr, tAttr,
                                    hasNormals, hasColors, hasTexcoords, options);
                writeFacesBinary(file, faces, options.format == BINARY_BIG_ENDIAN);
            }
            return true;
        }

    private:
        void writeHeader(std::ostream& file, size_t nVerts, size_t nFaces,
                         bool hasN, bool hasC, bool hasT, const WriteOptions& opt)
        {
            file << "ply\n";
            switch (opt.format)
            {
                case ASCII: file << "format ascii 1.0\n"; break;
                case BINARY_LITTLE_ENDIAN: file << "format binary_little_endian 1.0\n"; break;
                case BINARY_BIG_ENDIAN: file << "format binary_big_endian 1.0\n"; break;
            }
            file << "comment OSG PlyWriter\n";
            file << "element vertex " << nVerts << "\n";
            file << "property float x\nproperty float y\nproperty float z\n";
            if (hasN)
                file << "property float nx\nproperty float ny\nproperty float nz\n";
            if (hasC)
            {
                file << "property uchar red\nproperty uchar green\nproperty uchar blue\n";
                if (opt.writeAlpha) file << "property uchar alpha\n";
            }
            if (hasT)
                file << "property float s\nproperty float t\n";
            file << "element face " << nFaces << "\n";
            file << "property list uchar uint vertex_indices\n";
            file << "end_header\n";
        }

        void writeVerticesAscii(std::ostream& file, const std::vector<osg::Vec3>& pos,
                                const std::vector<osg::Vec4>& norm, const std::vector<osg::Vec4>& col,
                                const std::vector<osg::Vec4>& uv, bool hasN, bool hasC, bool hasT,
                                const WriteOptions& opt)
        {
            for (size_t i = 0; i < pos.size(); ++i)
            {
                file << pos[i].x() << " " << pos[i].y() << " " << pos[i].z();
                if (hasN) file << " " << norm[i].x() << " " << norm[i].y() << " " << norm[i].z();
                if (hasC)
                {
                    file << " " << (int)(col[i].r() * 255 + 0.5f)
                         << " " << (int)(col[i].g() * 255 + 0.5f)
                         << " " << (int)(col[i].b() * 255 + 0.5f);
                    if (opt.writeAlpha) file << " " << (int)(col[i].a() * 255 + 0.5f);
                }
                if (hasT) file << " " << uv[i].x() << " " << uv[i].y(); file << "\n";
            }
        }

        void writeVerticesBinary(std::ostream& file, const std::vector<osg::Vec3>& pos,
                                 const std::vector<osg::Vec4>& norm, const std::vector<osg::Vec4>& col,
                                 const std::vector<osg::Vec4>& uv, bool hasN, bool hasC, bool hasT,
                                 const WriteOptions& opt)
        {
            for (size_t i = 0; i < pos.size(); ++i)
            {
                writeBin(file, pos[i].x()); writeBin(file, pos[i].y()); writeBin(file, pos[i].z());
                if (hasN)
                    writeBin(file, norm[i].x()); writeBin(file, norm[i].y()); writeBin(file, norm[i].z());
                if (hasC)
                {
                    writeBin(file, (uint8_t)(col[i].r() * 255 + 0.5f));
                    writeBin(file, (uint8_t)(col[i].g() * 255 + 0.5f));
                    writeBin(file, (uint8_t)(col[i].b() * 255 + 0.5f));
                    if (opt.writeAlpha) writeBin(file, (uint8_t)(col[i].a() * 255 + 0.5f));
                }
                if (hasT)
                    writeBin(file, uv[i].x()); writeBin(file, uv[i].y());
            }
        }

        void writeFacesAscii(std::ostream& file, const std::vector<std::vector<unsigned int>>& faces)
        {
            for (const auto& f : faces)
            { file << f.size(); for (auto idx : f) file << " " << idx; file << "\n"; }
        }

        void writeFacesBinary(std::ostream& file, const std::vector<std::vector<unsigned int>>& faces, bool bigEndian)
        {
            for (const auto& f : faces)
            {
                uint8_t count = (uint8_t)f.size(); writeBin(file, count);
                for (auto idx : f)
                {
                    if (bigEndian)
                    {
                        uint32_t be = swapEndian(idx);
                        file.write((char*)&be, sizeof(be));
                    }
                    else
                        writeBin(file, idx);
                }
            }
        }

        template<typename T> void writeBin(std::ostream& file, T val)
        { file.write((char*)&val, sizeof(T)); }

        uint32_t swapEndian(uint32_t val)
        {
            return ((val >> 24) & 0xFF) | ((val >> 8) & 0xFF00) |
                   ((val << 8) & 0xFF0000) | ((val << 24) & 0xFF000000);
        }
    };
}

osg::Node* readMeshScenePLY(std::istream& fin, const std::string& dir, const osgDB::Options* options)
{
    PlyReader reader;
    try
    {
        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable(reader.read(fin));
        return geode.release();
    }
    catch (const std::exception& e)
    {
        OSG_NOTICE << "[ReaderWriterMesh] " << e.what() << std::endl;
    }
    return NULL;
}

bool writeMeshScenePLY(const osg::Node& node, std::ostream& fout, const osgDB::Options* options)
{ PlyWriter writer; return writer.write(fout, const_cast<osg::Node*>(&node)); }
