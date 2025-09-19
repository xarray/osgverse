#include <osg/io_utils>
#include <osg/ValueObject>
#include <osg/TriangleIndexFunctor>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgUtil/Tessellator>

#include <pipeline/Drawer2D.h>
#include <vtzero/vector_tile.hpp>

struct ImageVisitor
{
    osg::observer_ptr<osgVerse::Drawer2D> drawer;
    std::vector<std::pair<std::vector<osg::Vec2>, int>> ringList;
    std::vector<osg::Vec2> lineStrip; float scale;
    ImageVisitor(osgVerse::Drawer2D* d, float s) : drawer(d), scale(s) {}

    void points_begin(const uint32_t count) {}
    void points_point(const vtzero::point point) { drawer->drawCircle(osg::Vec2(point.x, point.y) * scale, 1.0f); }
    void points_end() {}

    void linestring_begin(const uint32_t count) { lineStrip.clear(); }
    void linestring_point(const vtzero::point point) { lineStrip.push_back(osg::Vec2(point.x, point.y) * scale); }
    void linestring_end() { drawer->drawPolyline(lineStrip, false); }

    void ring_begin(const uint32_t count) { lineStrip.clear(); }
    void ring_point(const vtzero::point point) { lineStrip.push_back(osg::Vec2(point.x, point.y) * scale); }
    void ring_end(const vtzero::ring_type rt) { ringList.push_back(std::pair<std::vector<osg::Vec2>, int>(lineStrip, (int)rt)); }
};

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
        supportsExtension("pbf", "PBF vector tile file");

        supportsOption("ImageWidth", "Image resolution. Default 512");
        supportsOption("ImageHeight", "Image resolution. Default 512");
    }

    virtual const char* className() const
    {
        return "[osgVerse] MVT vector tile format reader";
    }

    virtual ReadResult readNode(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_HANDLED;
        return readNode(in, options);
    }

    virtual ReadResult readImage(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_HANDLED;
        return readImage(in, options);
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
                uint32_t extent = layer.extent();  // FIXME: coordinate scale?
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

    virtual ReadResult readImage(std::istream& fin, const Options* options) const
    {
        std::string buffer((std::istreambuf_iterator<char>(fin)),
                           std::istreambuf_iterator<char>());
        if (buffer.empty()) return ReadResult::ERROR_IN_READING_FILE;

        std::string wStr = options ? options->getPluginStringData("ImageWidth") : "512";
        std::string hStr = options ? options->getPluginStringData("ImageHeight") : "512";
        int w = atoi(wStr.c_str()), h = atoi(hStr.c_str()); if (w < 1) w = 512; if (h < 1) h = 512;

        osg::ref_ptr<osgVerse::Drawer2D> drawer = new osgVerse::Drawer2D;
        drawer->allocateImage(w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE);
        drawer->setPixelBufferObject(new osg::PixelBufferObject(drawer.get()));
        drawer->start(false); drawer->fillBackground(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
        try
        {
            vtzero::vector_tile tile(buffer);
            while (vtzero::layer layer = tile.next_layer())
            {
                uint32_t extent = layer.extent(); float scale = (float)w / (float)extent;
                const std::vector<vtzero::data_view>& keys = layer.key_table();
                const std::vector<vtzero::property_value>& values = layer.value_table();
                size_t numKeyValues = osg::minimum(keys.size(), values.size());
                for (size_t i = 0; i < numKeyValues; ++i) updateProperty(*drawer, "", keys[i], values[i]);

                ImageVisitor iv(drawer.get(), scale);
                while (vtzero::feature feature = layer.next_feature())
                {
                    vtzero::decode_geometry(feature.geometry(), iv);
                    while (vtzero::property prop = feature.next_property())
                    {
                        std::string id = std::to_string(feature.id());
                        updateProperty(*drawer, id + "_", prop.key(), prop.value());
                    }
                }

                if (!iv.ringList.empty())
                {
                    osgVerse::DrawerStyleData fillStyle(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f), true);
#if false
                    for (std::vector<std::pair<std::vector<osg::Vec2>, int>>::iterator it = iv.ringList.begin();
                         it != iv.ringList.end(); ++it) { drawer->drawPolyline(it->first, true); }
#else
                    std::vector<std::vector<osg::Vec2f>> polygon;
                    for (std::vector<std::pair<std::vector<osg::Vec2>, int>>::iterator it = iv.ringList.begin();
                         it != iv.ringList.end(); ++it)
                    {
                        if (it->second == (int)vtzero::ring_type::outer)
                            { if (!polygon.empty()) drawer->drawPolygon(polygon, fillStyle); polygon.clear(); }
                        polygon.push_back(it->first);
                    }
                    if (!polygon.empty()) drawer->drawPolygon(polygon, fillStyle);
#endif
                    iv.ringList.clear();
                }
            }  // while (vtzero::layer layer = tile.next_layer())
        }
        catch (const std::exception& e)
            { OSG_WARN << "[ReaderWriterMVT] " << e.what() << std::endl; }
        drawer->finish(); return drawer.get();
    }

protected:
    struct TriangleCollector
    {
        std::vector<unsigned int> triangles;
        void operator()(unsigned int i1, unsigned int i2, unsigned int i3)
        { triangles.push_back(i1); triangles.push_back(i2); triangles.push_back(i3); }
    };

    std::string getRealFileName(const std::string& path, std::string& ext) const
    {
        std::string fileName(path); ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return "";

        bool usePseudo = (ext == "verse_mvt");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }
        return fileName;
    }

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
