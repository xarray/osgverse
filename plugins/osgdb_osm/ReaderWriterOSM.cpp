#include <osg/io_utils>
#include <osg/ValueObject>
#include <osg/TriangleIndexFunctor>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgUtil/Tessellator>

#include <pipeline/Drawer2D.h>
#include <readerwriter/FeatureDefinition.h>
#include <osmium/io/any_input.hpp>
#include <osmium/visitor.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/index/map/flex_mem.hpp>
#include <cstdlib>

class ReaderWriterOSM : public osgDB::ReaderWriter
{
public:
    ReaderWriterOSM()
    {
#if WIN32
        _putenv("OSMIUM_POOL_THREADS=1");
#else
        setenv("OSMIUM_POOL_THREADS", "1", 1);
#endif

        supportsExtension("verse_osm", "osgVerse pseudo-loader");
        supportsExtension("osm", "OpenStreetMap XML format");
        supportsExtension("pbf", "OpenStreetMap PBF format");
        supportsOption("IncludeFeatures", "Add FeatureCollection as UserData of the result Geometry/Image. Default: 0");
        supportsOption("ImageWidth", "Image resolution. Default: 512");
        supportsOption("ImageHeight", "Image resolution. Default: 512");
        supportsOption("FilterKey", "Filter objects by key");
        supportsOption("FilterValue", "Filter objects by value (used with FilterKey)");
    }

    virtual const char* className() const
    {
        return "[osgVerse] OpenStreetMap data format reader";
    }

    virtual ReadResult readObject(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        if (fileName.empty()) return ReadResult::FILE_NOT_HANDLED;

        try
        {
            osg::ref_ptr<osgVerse::FeatureCollection> fc = parseOSMData(fileName, options);
            return fc.get();
        }
        catch (const std::exception& e)
        {
            OSG_WARN << "[ReaderWriterOSM] Error parsing OSM data: " << e.what() << std::endl;
            return ReadResult::FILE_NOT_HANDLED;
        }
    }

    virtual ReadResult readNode(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        if (fileName.empty()) return ReadResult::FILE_NOT_HANDLED;

        try
        {
            osg::ref_ptr<osgVerse::FeatureCollection> fc = parseOSMData(fileName, options);

            osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
            geom->setUseDisplayList(false);
            geom->setUseVertexBufferObjects(true);
            for (size_t i = 0; i < fc->features.size(); ++i)
            {
                osgVerse::Feature* feature = fc->features[i];
                osgVerse::addFeatureToGeometry(*feature, geom.get(), true);
            }

            osg::ref_ptr<osg::Geode> geode = new osg::Geode;
            if (options)
            {
                int toInc = atoi(options->getPluginStringData("IncludeFeatures").c_str());
                if (toInc > 0) geom->setUserData(fc.get());
            }
            geode->addDrawable(geom.get());
            return geode.get();
        }
        catch (const std::exception& e)
        {
            OSG_WARN << "[ReaderWriterOSM] Error parsing OSM data as node: " << e.what() << std::endl;
            return ReadResult::FILE_NOT_HANDLED;
        }
    }

    virtual ReadResult readImage(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        if (fileName.empty()) return ReadResult::FILE_NOT_HANDLED;

        try
        {
            osg::ref_ptr<osgVerse::FeatureCollection> fc = parseOSMData(fileName, options);
            std::string wStr = options ? options->getPluginStringData("ImageWidth") : "512";
            std::string hStr = options ? options->getPluginStringData("ImageHeight") : "512";
            int w = atoi(wStr.c_str()), h = atoi(hStr.c_str());
            if (w < 1) w = 512; if (h < 1) h = 512;

            osg::ref_ptr<osgVerse::Drawer2D> drawer = new osgVerse::Drawer2D;
            if (options)
            {
                int toInc = atoi(options->getPluginStringData("IncludeFeatures").c_str());
                if (toInc > 0) drawer->setUserData(fc.get());
            }
            drawer->allocateImage(w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE);
            drawer->setPixelBufferObject(new osg::PixelBufferObject(drawer.get()));
            drawer->start(false);
            drawer->fillBackground(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));

            const osg::BoundingBox& bb = fc->bound;
            osg::Vec2 off(-bb.xMin(), -bb.yMin()), sc((float)w / (bb.xMax() - bb.xMin()), (float)h / (bb.yMax() - bb.yMin()));
            osgVerse::DrawerStyleData fillStyle(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f), true);
            for (size_t i = 0; i < fc->features.size(); ++i)
            {
                osgVerse::Feature* feature = fc->features[i];
                osgVerse::drawFeatureToImage(*feature, drawer.get(), off, sc, &fillStyle);
            }
            drawer->finish();
            return drawer.get();
        }
        catch (const std::exception& e)
        {
            OSG_WARN << "[ReaderWriterOSM] Error parsing OSM data as image: " << e.what() << std::endl;
            return ReadResult::FILE_NOT_HANDLED;
        }
    }

protected:
    /** OSM data handler using libosmium */
    class OSMDataHandler : public osmium::handler::Handler
    {
    public:
        OSMDataHandler(osgVerse::FeatureCollection* fc, const std::string& filterKey,
                       const std::string& filterValue) : _fc(fc), _filterKey(filterKey), _filterValue(filterValue)
        {}  //{ _points = new osgVerse::Feature; _points->setType(GL_POINTS); _fc->push_back(_points.get()); }

        void node(const osmium::Node& node)
        {
            if (!passesFilter(node)) return;
            const auto& loc = node.location();
            _locations.set(node.id(), loc);
            //_points->addPoint(osg::Vec3(loc.lat(), loc.lon(), 0.0f));

            // Store OSM tags as user values
            //storeTagsAsUserValues(node, feature.get());
            //feature->setUserValue("osm_id", (unsigned int)node.id());
        }

        void way(const osmium::Way& way)
        {
            if (!passesFilter(way)) return;
            osg::ref_ptr<osgVerse::Feature> feature = new osgVerse::Feature;

            // Determine geometry type from tags
            std::string geomType = getGeometryType(way);
            if (geomType == "polygon") feature->setType(GL_POLYGON);
            else if (geomType == "line") feature->setType(GL_LINE_STRIP);
            else feature->setType(GL_LINE_STRIP);

            // Store way nodes as points
            osg::ref_ptr<osg::Vec3Array> points = new osg::Vec3Array;
            for (const auto& nodeRef : way.nodes())
            {
                try
                {
                    osmium::Location loc = _locations.get(nodeRef.ref());
                    points->push_back(osg::Vec3(loc.lat(), loc.lon(), 0.0f));
                }
                catch (const osmium::not_found&)
                {
                    if (nodeRef.location().valid())
                    {
                        const auto& loc = nodeRef.location();
                        points->push_back(osg::Vec3(loc.lat(), loc.lon(), 0.0f));
                    }
                }
            }

            if (!points->empty())
            {
                feature->addPoints(points.get());
                storeTagsAsUserValues(way, feature.get());
                feature->setUserValue("osm_id", (unsigned int)way.id());
                _fc->push_back(feature.get());
            }
        }

        void relation(const osmium::Relation& relation)
        {
            if (!passesFilter(relation)) return;
            // TODO
        }

    private:
        template <typename T> bool passesFilter(const T& obj) const
        {
            if (_filterKey.empty()) return true;
            const osmium::TagList& tags = obj.tags();
            const char* value = tags[_filterKey.c_str()];

            if (!value) return false;
            if (_filterValue.empty()) return true;
            return _filterValue == value;
        }

        template <typename T> void storeTagsAsUserValues(const T& obj, osgVerse::Feature* feature) const
        {
            const osmium::TagList& tags = obj.tags();
            for (const auto& tag : tags)
            {
                std::string key = std::string("tag_") + tag.key();
                feature->setUserValue(key, std::string(tag.value()));
            }
        }

        std::string getGeometryType(const osmium::Way& way) const
        {
            const osmium::TagList& tags = way.tags();

            // Check for area/polygon indicators
            if (const char* area = tags["area"])
            { if (std::string(area) == "yes") return "polygon"; }

            // Check for common polygon tags
            static const char* polygonTags[] = {
                "building", "landuse", "natural", "waterway", "amenity",
                "leisure", "tourism", "shop", "office", "craft",
                "historic", "military", "aeroway", "boundary", "place"
            };

            for (const char* tag : polygonTags) { if (tags[tag]) return "polygon"; }
            return "line";
        }

        using IndexType = osmium::index::map::FlexMem<osmium::unsigned_object_id_type, osmium::Location>;
        IndexType _locations;

        osgVerse::FeatureCollection* _fc;
        //osg::ref_ptr<osgVerse::Feature> _points;
        std::string _filterKey;
        std::string _filterValue;
    };

    osgVerse::FeatureCollection* parseOSMData(const std::string& fileName,
                                              const osgDB::ReaderWriter::Options* options) const
    {
        // Get filter options
        std::string filterKey, filterValue;
        if (options)
        {
            filterKey = options->getPluginStringData("FilterKey");
            filterValue = options->getPluginStringData("FilterValue");
        }

        osg::ref_ptr<osgVerse::FeatureCollection> fc = new osgVerse::FeatureCollection;
        try
        {
            // Open OSM file using libosmium
            osmium::io::File inputFile(fileName);
            osmium::io::Reader reader(inputFile,
                osmium::osm_entity_bits::node | osmium::osm_entity_bits::way/* | osmium::osm_entity_bits::relation*/);

            OSMDataHandler handler(fc.get(), filterKey, filterValue);
            osmium::apply(reader, handler);
            reader.close();
        }
        catch (const std::exception& e)
        {
            OSG_NOTICE << "[ReaderWriterOSM] Error reading OSM data: " << e.what() << std::endl;
        }
        return fc.release();
    }

    std::string getRealFileName(const std::string& path, std::string& ext) const
    {
        std::string fileName(path); ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return "";

        bool usePseudo = (ext == "verse_osm");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }

        // Handle .osm.pbf compound extension
        if (ext == "pbf")
        {
            std::string baseName = osgDB::getNameLessExtension(fileName);
            std::string baseExt = osgDB::getLowerCaseFileExtension(baseName);
            if (baseExt == "osm") ext = "osm.pbf";
        }
        return fileName;
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_osm, ReaderWriterOSM)
