#include <osg/io_utils>
#include <osg/TriangleIndexFunctor>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgUtil/Tessellator>
#include <vtzero/vector_tile.hpp>

struct GeometryVisitor
{
    osg::observer_ptr<osg::Geometry> geom;
    osg::ref_ptr<osg::Vec3Array> vertices;
    osg::ref_ptr<osg::Vec4Array> colors;
    std::map<int, int> ringTypes;
    osg::Vec4 defaultColor;
    unsigned int vStart;

    GeometryVisitor(osg::Geometry* g) : geom(g), vStart(0)
    {
        vertices = dynamic_cast<osg::Vec3Array*>(geom->getVertexArray());
        colors = dynamic_cast<osg::Vec4Array*>(geom->getColorArray());
        defaultColor = osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
        if (!vertices)
        {
            vertices = new osg::Vec3Array; colors = new osg::Vec4Array;
            geom->setVertexArray(vertices.get()); geom->setColorArray(colors.get());
            geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        }
    }

    void points_begin(const uint32_t count) { vStart = vertices->size(); }
    void points_point(const vtzero::point point)
    { vertices->push_back(osg::Vec3(point.x, point.y, 0.0f)); colors->push_back(defaultColor); }
    void points_end()
    { geom->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, vStart, vertices->size() - vStart)); }

    void linestring_begin(const uint32_t count) { vStart = vertices->size(); }
    void linestring_point(const vtzero::point point)
    { vertices->push_back(osg::Vec3(point.x, point.y, 0.0f)); colors->push_back(defaultColor); }
    void linestring_end()
    { geom->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, vStart, vertices->size() - vStart)); }

    void ring_begin(const uint32_t count) { vStart = vertices->size(); }
    void ring_point(const vtzero::point point)
    { vertices->push_back(osg::Vec3(point.x, point.y, 0.0f)); colors->push_back(defaultColor); }
    void ring_end(const vtzero::ring_type rt)
    {
        ringTypes[geom->getNumPrimitiveSets()] = (int)rt;
        if (vertices->at(vStart) == vertices->back()) { vertices->pop_back(); colors->pop_back(); }
        geom->addPrimitiveSet(new osg::DrawArrays(GL_POLYGON, vStart, vertices->size() - vStart));
    }
};

class ReaderWriterMVT : public osgDB::ReaderWriter
{
public:
    ReaderWriterMVT()
    {
        supportsExtension("verse_mvt", "osgVerse pseudo-loader");
        supportsExtension("mvt", "MVT vector tile file");
    }

    virtual const char* className() const
    {
        return "[osgVerse] MVT vector tile format reader";
    }

    virtual ReadResult readNode(const std::string& path, const Options* options) const
    {
        std::string fileName(path);
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

        bool usePseudo = (ext == "verse_mvt");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getLowerCaseFileExtension(fileName);
        }

        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_FOUND;
        return readNode(in, options);
    }

    virtual ReadResult readNode(std::istream& fin, const Options* options) const
    {
        std::string buffer((std::istreambuf_iterator<char>(fin)),
                           std::istreambuf_iterator<char>());
        if (buffer.empty()) return ReadResult::ERROR_IN_READING_FILE;

        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        try
        {
            vtzero::vector_tile tile(buffer);
            while (vtzero::layer layer = tile.next_layer())
            {
                osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
                geom->setName(layer.name().to_string());
                geom->setUseDisplayList(false);
                geom->setUseVertexBufferObjects(true);
                geode->addDrawable(geom.get());

                const std::vector<vtzero::data_view>& keys = layer.key_table();
                const std::vector<vtzero::property_value>& values = layer.value_table();
                size_t numKeyValues = osg::minimum(keys.size(), values.size());
                for (size_t i = 0; i < numKeyValues; ++i) updateProperty(*geode, "", keys[i], values[i]);

                GeometryVisitor gv(geom.get());
                while (vtzero::feature feature = layer.next_feature())
                {
                    vtzero::decode_geometry(feature.geometry(), gv);
                    osg::PrimitiveSet* p = geom->getNumPrimitiveSets() > 0 ?
                        geom->getPrimitiveSet(geom->getNumPrimitiveSets() - 1) : NULL;
                    while (vtzero::property prop = feature.next_property())
                    {
                        std::string id = std::to_string(feature.id());
                        if (p) { p->setName(id); updateProperty(*p, "", prop.key(), prop.value()); }
                        else updateProperty(*geom, id + "_", prop.key(), prop.value());
                    }
                }

                if (!gv.ringTypes.empty())
                {
                    unsigned int lastNumPrimitives = geom->getNumPrimitiveSets();
                    std::vector<osg::ref_ptr<osg::DrawArrays>> polygonsToTess;

                    osg::ref_ptr<osg::Geometry> geomToTess = new osg::Geometry;
                    geomToTess->setVertexArray(gv.vertices.get());
                    for (unsigned int i = 0; i < lastNumPrimitives; ++i)
                    {
                        osg::DrawArrays* da = static_cast<osg::DrawArrays*>(geom->getPrimitiveSet(i));
                        if (da->getMode() != GL_POLYGON) continue;

                        bool outer = (gv.ringTypes[i] == 0);
                        if (outer && !polygonsToTess.empty())
                        {   // Handle past data
                            osg::PrimitiveSet* p = createDelaunayTriangulation(*geomToTess, polygonsToTess);
                            if (p) geom->addPrimitiveSet(p); polygonsToTess.clear();
                        }
                        polygonsToTess.push_back(da);
                    }

                    if (!polygonsToTess.empty())
                    {   // Handle past data
                        osg::ref_ptr<osg::PrimitiveSet> p = createDelaunayTriangulation(*geomToTess, polygonsToTess);
                        if (p.valid()) geom->addPrimitiveSet(p.get());
                    }
                    for (int i = (int)lastNumPrimitives - 1; i >= 0; --i)
                    { if (geom->getPrimitiveSet(i)->getMode() == GL_POLYGON) geom->removePrimitiveSet(i); }
                }
            }  // while (vtzero::layer layer = tile.next_layer())
        }
        catch (const std::exception& e)
            { OSG_WARN << "[ReaderWriterMVT] " << e.what() << std::endl; }
        return geode.get();
    }

protected:
    struct TriangleCollector
    {
        std::vector<unsigned int> triangles;
        void operator()(unsigned int i1, unsigned int i2, unsigned int i3)
        { triangles.push_back(i1); triangles.push_back(i2); triangles.push_back(i3); }
    };

    osg::PrimitiveSet* createDelaunayTriangulation(
            osg::Geometry& geom, const std::vector<osg::ref_ptr<osg::DrawArrays>>& primitives) const
    {
        geom.removePrimitiveSet(0, geom.getNumPrimitiveSets());
        for (size_t i = 0; i < primitives.size(); ++i)
            geom.addPrimitiveSet(primitives[i].get());

        osg::ref_ptr<osgUtil::Tessellator> tscx = new osgUtil::Tessellator;
        tscx->setWindingType(osgUtil::Tessellator::TESS_WINDING_ODD);
        tscx->setTessellationType(osgUtil::Tessellator::TESS_TYPE_POLYGONS);
        tscx->setTessellationNormal(osg::Z_AXIS);
        tscx->retessellatePolygons(geom);

        osg::TriangleIndexFunctor<TriangleCollector> f; geom.accept(f);
        if (f.triangles.size() < 65535)
            return new osg::DrawElementsUShort(GL_TRIANGLES, f.triangles.begin(), f.triangles.end());
        else
            return new osg::DrawElementsUInt(GL_TRIANGLES, f.triangles.begin(), f.triangles.end());
    }

    void updateProperty(osg::Object& obj, const std::string& prefix,
                        const vtzero::data_view& key, const vtzero::property_value& v) const
    {
        std::string name = prefix + key.to_string();
        switch (v.type())
        {
        case vtzero::property_value_type::float_value: obj.setUserValue(name, v.float_value()); break;
        case vtzero::property_value_type::double_value: obj.setUserValue(name, v.double_value()); break;
        case vtzero::property_value_type::int_value: obj.setUserValue(name, (int)v.int_value()); break;
        case vtzero::property_value_type::uint_value: obj.setUserValue(name, (unsigned int)v.uint_value()); break;
        case vtzero::property_value_type::sint_value: obj.setUserValue(name, (signed int)v.sint_value()); break;
        case vtzero::property_value_type::bool_value: obj.setUserValue(name, v.bool_value()); break;
        default: obj.setUserValue(name, v.string_value().to_string()); break;
        }
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_mvt, ReaderWriterMVT)
