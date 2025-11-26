#include <osg/io_utils>
#include <osg/ValueObject>
#include <osg/TriangleIndexFunctor>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgUtil/Tessellator>

#include <readerwriter/FeatureDefinition.h>
#include <mapbox/geojson.hpp>
#include <mapbox/geojson/rapidjson.hpp>
#include <mapbox/geometry.hpp>

struct GeometryVisitor
{
    void operator()(const mapbox::geojson::point& point) const
    {
        if (!typeSet) current->setType(GL_POINTS); typeSet = true;
        current->addPoint(osg::Vec3(point.x, point.y, 0.0f), index);
    }

    void operator()(const mapbox::geojson::line_string& line) const
    {
        if (!typeSet) current->setType(GL_LINES); typeSet = true;
        for (size_t i = 1; i < line.size(); ++i)
        {
            const mapbox::geojson::point &pt0 = line[i - 1], &pt1 = line[i];
            current->addPoint(osg::Vec3(pt0.x, pt0.y, 0.0f), index);
            current->addPoint(osg::Vec3(pt1.x, pt1.y, 0.0f), index);
        }
    }

    void operator()(const mapbox::geojson::polygon& polygon) const
    {
        if (!typeSet) current->setType(GL_POLYGON); typeSet = true;
        for (size_t i = 0; i < polygon.size(); ++i)
        {
            const auto& poly = polygon[i];
            for (size_t j = 0; j < poly.size(); ++j)
            {
                const mapbox::geojson::point& pt = poly[j];
                current->addPoint(osg::Vec3(pt.x, pt.y, 0.0f), index);
            }

            osg::Vec3Array* va = current->getPoints(index);
            if (va->front() == va->back()) va->pop_back();
        }
    }

    void operator()(const mapbox::geojson::multi_point& points) const
    {
        if (!typeSet) current->setType(GL_POINTS); typeSet = true;
        for (size_t i = 0; i < points.size(); ++i)
        {
            const mapbox::geojson::point& pt = points[i];
            current->addPoint(osg::Vec3(pt.x, pt.y, 0.0f), index);
        }
    }

    void operator()(const mapbox::geojson::multi_line_string& lines) const
    {
        if (!typeSet) current->setType(GL_LINES); typeSet = true;
        for (size_t i = 0; i < lines.size(); ++i) (*this)(lines[i]);
    }

    void operator()(const mapbox::geojson::multi_polygon& polygons) const
    {
        if (!typeSet) current->setType(GL_POLYGON); typeSet = true;
        for (size_t i = 0; i < polygons.size(); ++i) { (*this)(polygons[i]); index++; }
    }

    void operator()(const mapbox::geojson::geometry_collection& collection) const
    {
        OSG_WARN << "[ReaderWriterGeoJson] Geometry collection not implemented" << std::endl; // TODO
        /*for (const auto& geom : collection)
            mapbox::util::apply_visitor(*this, geom);*/
    }

    template <typename T> void operator()(const T& g) const
    { OSG_NOTICE << "[ReaderWriterGeoJson] Unknown type: " << typeid(g).name() << "\n"; }

    GeometryVisitor(osgVerse::Feature* c) : index(0), typeSet(false) { current = c; }
    osg::observer_ptr<osgVerse::Feature> current;
    mutable int index; mutable bool typeSet;
};

struct GeojsonVisitor
{
    void operator()(const mapbox::geojson::geometry& gData)
    {
        if (!current.valid()) current = new osgVerse::Feature;
        GeometryVisitor visitor(current.get());
        mapbox::util::apply_visitor(visitor, gData);
        features.push_back(current); current = NULL;
    }

    void operator()(const mapbox::geojson::feature& feature)
    {
        current = new osgVerse::Feature;
        if (feature.id.is<std::string>()) current->setName(feature.id.get<std::string>());
        for (std::unordered_map<std::string, mapbox::geojson::value>::const_iterator
             it = feature.properties.begin(); it != feature.properties.end(); ++it)
        {
            if (it->second.is<std::string>())
                current->setUserValue(it->first, it->second.get<std::string>());
            else if (it->second.is<double>())
                current->setUserValue(it->first, it->second.get<double>());
            else if (it->second.is<bool>())
                current->setUserValue(it->first, it->second.get<bool>());
        }
        (*this)(feature.geometry);
    }

    void operator()(const mapbox::geojson::feature_collection& collection)
    { for (const auto& feature : collection) (*this)(feature); }

    GeojsonVisitor() {}
    std::vector<osg::ref_ptr<osgVerse::Feature>> features;
    osg::ref_ptr<osgVerse::Feature> current;
};

class ReaderWriterGeoJson : public osgDB::ReaderWriter
{
public:
    ReaderWriterGeoJson()
    {
        supportsExtension("verse_geojson", "osgVerse pseudo-loader");
        supportsExtension("geojson", "GEOJSON feature data file");
        supportsExtension("json", "GEOJSON feature data file");
    }

    virtual const char* className() const
    {
        return "[osgVerse] GEOJSON feature data format reader";
    }

    virtual ReadResult readNode(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_HANDLED;;

        osg::ref_ptr<Options> localOptions = NULL;
        if (options) localOptions = options->cloneOptions();
        else localOptions = new osgDB::Options();

        localOptions->setPluginStringData("prefix", osgDB::getFilePath(path));
        localOptions->setPluginStringData("extension", ext);
        return readNode(in, localOptions.get());
    }

    virtual ReadResult readNode(std::istream& fin, const Options* options) const
    {
        std::string buffer((std::istreambuf_iterator<char>(fin)),
                           std::istreambuf_iterator<char>());
        if (buffer.empty()) return ReadResult::ERROR_IN_READING_FILE;

        GeojsonVisitor visitor;
        try
        {
            mapbox::geojson::geojson data = mapbox::geojson::parse(buffer);
            mapbox::util::apply_visitor(visitor, data);
        }
        catch (const std::runtime_error& err)
        {
            OSG_WARN << "[ReaderWriterGeoJson] Parse failed: " << err.what() << std::endl;
            return ReadResult::ERROR_IN_READING_FILE;
        }

        osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
        geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
        for (size_t i = 0; i < visitor.features.size(); ++i)
        {
            osgVerse::Feature* feature = visitor.features[i];
            osgVerse::addFeatureToGeometry(*feature, geom.get(), true);
        }

        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable(geom.get()); return geode.get();
    }

protected:
    std::string getRealFileName(const std::string& path, std::string& ext) const
    {
        std::string fileName(path); ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return "";

        bool usePseudo = (ext == "verse_geojson");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }
        return fileName;
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_geojson, ReaderWriterGeoJson)
