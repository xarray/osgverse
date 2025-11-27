#include <osg/io_utils>
#include <osg/ValueObject>
#include <osg/TriangleIndexFunctor>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <pipeline/Drawer2D.h>
#include <readerwriter/FeatureDefinition.h>
#include <vtzero/vector_tile.hpp>

struct GeometryVisitor
{
    std::vector<osg::Vec3> ringData;
    osg::ref_ptr<osgVerse::FeatureCollection> collection;
    osg::ref_ptr<osgVerse::Feature> current;
    GeometryVisitor() { collection = new osgVerse::FeatureCollection; }

    void points_begin(const uint32_t count) { current = new osgVerse::Feature; }
    void points_point(const vtzero::point pt) { current->addPoint(osg::Vec3(pt.x, pt.y, 0.0f)); }
    void points_end() { current->setType(GL_POINTS); collection->push_back(current.get()); }

    void linestring_begin(const uint32_t count) { current = new osgVerse::Feature; }
    void linestring_point(const vtzero::point pt) { current->addPoint(osg::Vec3(pt.x, pt.y, 0.0f)); }
    void linestring_end() { current->setType(GL_LINE_STRIP); collection->push_back(current.get()); }

    void ring_begin(const uint32_t count) { current = new osgVerse::Feature; ringData.clear(); }
    void ring_point(const vtzero::point pt) { ringData.push_back(osg::Vec3(pt.x, pt.y, 0.0f)); }
    void ring_end(const vtzero::ring_type rt)
    {
        bool first = (rt == vtzero::ring_type::outer);
        current->addPoints(new osg::Vec3Array(ringData.begin(), ringData.end()), first);
        current->setType(GL_POLYGON); collection->push_back(current.get());
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
        supportsOption("IncludeFeatures", "Add FeatureCollection as UserData of the result Geometry/Image. Default: 0");
        supportsOption("ImageWidth", "Image resolution. Default: 512");
        supportsOption("ImageHeight", "Image resolution. Default: 512");
    }

    virtual const char* className() const
    {
        return "[osgVerse] MVT vector tile format reader";
    }

    virtual ReadResult readObject(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        if (fileName.empty()) return ReadResult::FILE_NOT_HANDLED;

        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_HANDLED;
        return readObject(in, options);
    }

    virtual ReadResult readNode(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        if (fileName.empty()) return ReadResult::FILE_NOT_HANDLED;

        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_HANDLED;
        return readNode(in, options);
    }

    virtual ReadResult readImage(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        if (fileName.empty()) return ReadResult::FILE_NOT_HANDLED;

        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_HANDLED;
        return readImage(in, options);
    }

    virtual ReadResult readObject(std::istream& fin, const Options* options) const
    {
        std::string buffer((std::istreambuf_iterator<char>(fin)),
                           std::istreambuf_iterator<char>());
        if (buffer.empty()) return ReadResult::ERROR_IN_READING_FILE;

        GeometryVisitor gv;
        try
        {
            vtzero::vector_tile tile(buffer);
            while (vtzero::layer layer = tile.next_layer()) processLayer(gv, layer);
        }
        catch (const std::exception& e)
            { OSG_WARN << "[ReaderWriterMVT] Parse failed: " << e.what() << std::endl; }
        return gv.collection.get();
    }

    virtual ReadResult readNode(std::istream& fin, const Options* options) const
    {
        std::string buffer((std::istreambuf_iterator<char>(fin)),
                           std::istreambuf_iterator<char>());
        if (buffer.empty()) return ReadResult::ERROR_IN_READING_FILE;

        GeometryVisitor gv;
        try
        {
            vtzero::vector_tile tile(buffer);
            while (vtzero::layer layer = tile.next_layer()) processLayer(gv, layer);
        }
        catch (const std::exception& e)
            { OSG_WARN << "[ReaderWriterMVT] Parse failed: " << e.what() << std::endl; }

        osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
        geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
        for (size_t i = 0; i < gv.collection->features.size(); ++i)
        {
            osgVerse::Feature* feature = gv.collection->features[i];
            osgVerse::addFeatureToGeometry(*feature, geom.get(), true);
        }

        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        if (options)
        {
            int toInc = atoi(options->getPluginStringData("IncludeFeatures").c_str());
            if (toInc > 0) geom->setUserData(gv.collection.get());
        }
        geode->addDrawable(geom.get()); return geode.get();
    }

    virtual ReadResult readImage(std::istream& fin, const Options* options) const
    {
        std::string buffer((std::istreambuf_iterator<char>(fin)),
                           std::istreambuf_iterator<char>());
        if (buffer.empty()) return ReadResult::ERROR_IN_READING_FILE;

        GeometryVisitor gv;
        try
        {
            vtzero::vector_tile tile(buffer);
            while (vtzero::layer layer = tile.next_layer()) processLayer(gv, layer);
        }
        catch (const std::exception& e)
            { OSG_WARN << "[ReaderWriterMVT] Parse failed: " << e.what() << std::endl; }

        std::string wStr = options ? options->getPluginStringData("ImageWidth") : "512";
        std::string hStr = options ? options->getPluginStringData("ImageHeight") : "512";
        int w = atoi(wStr.c_str()), h = atoi(hStr.c_str()); if (w < 1) w = 512; if (h < 1) h = 512;

        osg::ref_ptr<osgVerse::Drawer2D> drawer = new osgVerse::Drawer2D;
        if (options)
        {
            int toInc = atoi(options->getPluginStringData("IncludeFeatures").c_str());
            if (toInc > 0) drawer->setUserData(gv.collection.get());
        }
        drawer->allocateImage(w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE);
        drawer->setPixelBufferObject(new osg::PixelBufferObject(drawer.get()));
        drawer->start(false); drawer->fillBackground(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));

        const osg::BoundingBox& bb = gv.collection->bound;
        osg::Vec2 off(-bb.xMin(), -bb.yMin()),
                  sc((float)w / (bb.xMax() - bb.xMin()), (float)h / (bb.yMax() - bb.yMin()));
        osgVerse::DrawerStyleData fillStyle(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f), true);
        for (size_t i = 0; i < gv.collection->features.size(); ++i)
        {
            osgVerse::Feature* feature = gv.collection->features[i];
            osgVerse::drawFeatureToImage(*feature, drawer.get(), off, sc, &fillStyle);
        }
        drawer->finish(); return drawer.get();
    }

protected:
    void processLayer(GeometryVisitor& gv, vtzero::layer& layer) const
    {
        while (vtzero::feature feature = layer.next_feature())
        {
            vtzero::decode_geometry(feature.geometry(), gv);
            if (!gv.current) continue; gv.current->setName(std::to_string(feature.id()));
            while (vtzero::property prop = feature.next_property())
                updateProperty(*gv.current, "", prop.key(), prop.value());
        }

        const std::vector<vtzero::data_view>& keys = layer.key_table();
        const std::vector<vtzero::property_value>& values = layer.value_table();
        size_t numKeyValues = osg::minimum(keys.size(), values.size());
        gv.collection->setName(layer.name().to_string());
        for (size_t i = 0; i < numKeyValues; ++i) updateProperty(*gv.collection, "", keys[i], values[i]);
    }

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
