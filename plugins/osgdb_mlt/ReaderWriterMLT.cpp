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

#include <mlt/decoder.hpp>
#include <mlt/geometry.hpp>
#include <mlt/metadata/tileset.hpp>
#include <mlt/util/buffer_stream.hpp>
#include <mlt/projection.hpp>

// Find example data at: https://github.com/maplibre/demotiles
class ReaderWriterMLT : public osgDB::ReaderWriter
{
public:
    ReaderWriterMLT()
    {
        supportsExtension("verse_mlt", "osgVerse pseudo-loader");
        supportsExtension("mlt", "MapLibre vector tile file");
        supportsOption("IncludeFeatures", "Add FeatureCollection as UserData of the result Geometry/Image. Default: 0");
        supportsOption("ImageWidth", "Image resolution. Default: 512");
        supportsOption("ImageHeight", "Image resolution. Default: 512");
    }

    virtual const char* className() const
    {
        return "[osgVerse] MapLibre vector tile format reader";
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

        mlt::MapLibreTile tile = mlt::Decoder().decode({ buffer.data(), buffer.size() });
        osg::ref_ptr<osgVerse::FeatureCollection> collection = processTile(tile);
        if (!collection) return ReadResult::ERROR_IN_READING_FILE; else return collection.get();
    }

    virtual ReadResult readNode(std::istream& fin, const Options* options) const
    {
        std::string buffer((std::istreambuf_iterator<char>(fin)),
                           std::istreambuf_iterator<char>());
        if (buffer.empty()) return ReadResult::ERROR_IN_READING_FILE;

        mlt::MapLibreTile tile = mlt::Decoder().decode({ buffer.data(), buffer.size() });
        osg::ref_ptr<osgVerse::FeatureCollection> collection = processTile(tile);
        if (!collection) return ReadResult::ERROR_IN_READING_FILE;

        osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
        geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
        for (size_t i = 0; i < collection->features.size(); ++i)
        {
            osgVerse::Feature* feature = collection->features[i];
            osgVerse::addFeatureToGeometry(*feature, geom.get(), true);
        }

        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        if (options)
        {
            int toInc = atoi(options->getPluginStringData("IncludeFeatures").c_str());
            if (toInc > 0) geom->setUserData(collection.get());
        }
        geode->addDrawable(geom.get()); return geode.get();
    }

    virtual ReadResult readImage(std::istream& fin, const Options* options) const
    {
        std::string buffer((std::istreambuf_iterator<char>(fin)),
                           std::istreambuf_iterator<char>());
        if (buffer.empty()) return ReadResult::ERROR_IN_READING_FILE;

        mlt::MapLibreTile tile = mlt::Decoder().decode({ buffer.data(), buffer.size() });
        osg::ref_ptr<osgVerse::FeatureCollection> collection = processTile(tile);
        if (!collection) return ReadResult::ERROR_IN_READING_FILE;

        std::string wStr = options ? options->getPluginStringData("ImageWidth") : "512";
        std::string hStr = options ? options->getPluginStringData("ImageHeight") : "512";
        int w = atoi(wStr.c_str()), h = atoi(hStr.c_str()); if (w < 1) w = 512; if (h < 1) h = 512;

        osg::ref_ptr<osgVerse::Drawer2D> drawer = new osgVerse::Drawer2D;
        if (options)
        {
            int toInc = atoi(options->getPluginStringData("IncludeFeatures").c_str());
            if (toInc > 0) drawer->setUserData(collection.get());
        }
        drawer->allocateImage(w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE);
        drawer->setPixelBufferObject(new osg::PixelBufferObject(drawer.get()));
        drawer->start(false); drawer->fillBackground(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));

        const osg::BoundingBox& bb = collection->bound;
        osg::Vec2 off(-bb.xMin(), -bb.yMin()),
                  sc((float)w / (bb.xMax() - bb.xMin()), (float)h / (bb.yMax() - bb.yMin()));
        osgVerse::DrawerStyleData fillStyle(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f), true);
        for (size_t i = 0; i < collection->features.size(); ++i)
        {
            osgVerse::Feature* feature = collection->features[i];
            osgVerse::drawFeatureToImage(*feature, drawer.get(), off, sc, &fillStyle);
        }
        drawer->finish(); return drawer.get();
    }

protected:
    osgVerse::FeatureCollection* processTile(const mlt::MapLibreTile& tile) const
    {
        osg::ref_ptr<osgVerse::FeatureCollection> collection = new osgVerse::FeatureCollection;
        const std::vector<mlt::Layer>& layers = tile.getLayers();
        for (size_t i = 0; i < layers.size(); ++i)
        {
            const mlt::Layer& layer = layers[i];
            const std::vector<mlt::Feature>& features = layer.getFeatures();
            std::map<size_t, std::vector<osg::ref_ptr<osgVerse::Feature>>> savedList;

            for (size_t j = 0; j < features.size(); ++j)
            {
                const mlt::Feature::Geometry& geom = features[j].getGeometry();
                const nonstd::span<const uint32_t> tri = geom.getTriangles();

                osg::ref_ptr<osgVerse::Feature> f = new osgVerse::Feature;
                switch (geom.type)
                {
                case mlt::metadata::tileset::GeometryType::POINT:
                    {
                        const mlt::geometry::Point* g = static_cast<const mlt::geometry::Point*>(&geom);
                        f->addPoint(osg::Vec3(g->getCoordinate().x, g->getCoordinate().y, 0.0f));
                    }
                    f->setType(GL_POINTS); savedList[i].push_back(f); break;
                case mlt::metadata::tileset::GeometryType::LINESTRING:
                    {
                        const mlt::geometry::MultiPoint* g = static_cast<const mlt::geometry::MultiPoint*>(&geom);
                        const mlt::CoordVec& coords = g->getCoordinates();
                        for (size_t n = 0; n < coords.size(); ++n) f->addPoint(osg::Vec3(coords[n].x, coords[n].y, 0.0f));
                    }
                    f->setType(GL_LINE_STRIP); savedList[i].push_back(f); break;
                case mlt::metadata::tileset::GeometryType::POLYGON:
                    {
                        const mlt::geometry::Polygon* g = static_cast<const mlt::geometry::Polygon*>(&geom);
                        const mlt::geometry::Polygon::RingVec& rings = g->getRings();
                        for (size_t m = 0; m < rings.size(); ++m)
                        {
                            const mlt::geometry::Polygon::Ring& coords = rings[m]; std::vector<osg::Vec3> rData;
                            for (size_t n = 0; n < coords.size(); ++n) rData.push_back(osg::Vec3(coords[n].x, coords[n].y, 0.0f));
                            f->addPoints(new osg::Vec3Array(rData.begin(), rData.end()), m == 0);
                        }
                    }
                    f->setType(GL_POLYGON); savedList[i].push_back(f); break;
                case mlt::metadata::tileset::GeometryType::MULTIPOINT:
                    {
                        const mlt::geometry::MultiPoint* g = static_cast<const mlt::geometry::MultiPoint*>(&geom);
                        const mlt::CoordVec& coords = g->getCoordinates();
                        for (size_t n = 0; n < coords.size(); ++n) f->addPoint(osg::Vec3(coords[n].x, coords[n].y, 0.0f));
                    }
                    f->setType(GL_POINTS); savedList[i].push_back(f); break;
                case mlt::metadata::tileset::GeometryType::MULTILINESTRING:
                    {
                        const mlt::geometry::MultiLineString* g = static_cast<const mlt::geometry::MultiLineString*>(&geom);
                        const std::vector<mlt::CoordVec>& lineList = g->getLineStrings();
                        for (size_t m = 0; m < lineList.size(); ++m)
                        {
                            const mlt::CoordVec& coords = lineList[m];
                            for (size_t n = 0; n < coords.size(); ++n) f->addPoint(osg::Vec3(coords[n].x, coords[n].y, 0.0f));
                            f->setType(GL_LINE_STRIP); savedList[i].push_back(f); f = new osgVerse::Feature;
                        }
                    }
                    break;
                case mlt::metadata::tileset::GeometryType::MULTIPOLYGON:
                    {
                        const mlt::geometry::MultiPolygon* g = static_cast<const mlt::geometry::MultiPolygon*>(&geom);
                        const std::vector<mlt::geometry::Polygon::RingVec>& ringList = g->getPolygons();
                        for (size_t r = 0; r < ringList.size(); ++r)
                        {
                            const mlt::geometry::Polygon::RingVec& rings = ringList[r];
                            for (size_t m = 0; m < rings.size(); ++m)
                            {
                                const mlt::geometry::Polygon::Ring& coords = rings[m]; std::vector<osg::Vec3> rData;
                                for (size_t n = 0; n < coords.size(); ++n)
                                    rData.push_back(osg::Vec3(coords[n].x, coords[n].y, 0.0f));
                                f->addPoints(new osg::Vec3Array(rData.begin(), rData.end()), m == 0);
                            }
                            f->setType(GL_POLYGON); savedList[i].push_back(f); f = new osgVerse::Feature;
                        }
                    }
                    break;
                }
            }

            const mlt::PropertyVecMap& properties = layer.getProperties();
            for (mlt::PropertyVecMap::const_iterator it = properties.begin();
                 it != properties.end(); ++it)
            {
                for (size_t j = 0; j < it->second.getPropertyCount(); ++j)
                {
                    std::optional<mlt::Property> prop = it->second.getProperty(j);
                    std::vector<osg::ref_ptr<osgVerse::Feature>>& fList = savedList[j];
                    for (size_t m = 0; m < fList.size(); ++m)
                    {
                        osgVerse::Feature* f = fList[m].get(); f->setName(layer.getName());
                        if (prop.has_value()) std::visit(PropertyVisitor(it->first, f), prop.value());
                    }
                }
            }

            for (std::map<size_t, std::vector<osg::ref_ptr<osgVerse::Feature>>>::iterator it = savedList.begin();
                 it != savedList.end(); ++it)
            { for (size_t j = 0; j < it->second.size(); ++j) collection->push_back(it->second[j].get()); }
        }
        return collection.release();
    }

    struct PropertyVisitor
    {
        PropertyVisitor(const std::string& p, osg::Object* o) : parent(o), name(p) {}
        osg::Object* parent; std::string name;

        void operator()(std::nullptr_t) const {}
        void operator()(bool v) const { parent->setUserValue(name, v); }
        void operator()(const std::optional<bool>& v) const { if (v) parent->setUserValue(name, *v); }
        void operator()(std::int32_t v) const { parent->setUserValue(name, v); }
        void operator()(const std::optional<std::int32_t>& v) const { if (v) parent->setUserValue(name, *v); }
        void operator()(std::int64_t v) const { parent->setUserValue(name, (int32_t)v); }
        void operator()(const std::optional<std::int64_t>& v) const { if (v) parent->setUserValue(name, (int32_t)*v); }
        void operator()(std::uint32_t v) const { parent->setUserValue(name, v); }
        void operator()(const std::optional<uint32_t>& v) const { if (v) parent->setUserValue(name, *v); }
        void operator()(std::uint64_t v) const { parent->setUserValue(name, (uint32_t)v); }
        void operator()(const std::optional<std::uint64_t>& v) const { if (v) parent->setUserValue(name, (uint32_t)*v); }
        void operator()(float v) const { parent->setUserValue(name, v); }
        void operator()(const std::optional<float>& v) const { if (v) parent->setUserValue(name, *v); }
        void operator()(double v) const { parent->setUserValue(name, v); }
        void operator()(const std::optional<double>& v) const { if (v) parent->setUserValue(name, *v); }
        void operator()(std::string_view v) const { parent->setUserValue(name, std::string(v.data(), v.size())); }
    };

    /*void updateProperty(osg::Object& obj, const std::string& prefix,
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
    }*/

    std::string getRealFileName(const std::string& path, std::string& ext) const
    {
        std::string fileName(path); ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return "";

        bool usePseudo = (ext == "verse_mlt");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }
        return fileName;
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_mlt, ReaderWriterMLT)
