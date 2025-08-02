#include <osg/io_utils>
#include <osg/ValueObject>
#include <osg/TriangleIndexFunctor>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgUtil/Tessellator>

#include <mapbox/geojson.hpp>
#include <mapbox/geojson/rapidjson.hpp>
#include <mapbox/geometry.hpp>

struct GeometryVisitor
{
    void operator()(const mapbox::geojson::point& point) const
    {
        std::cout << "Point at (" << point.x << ", " << point.y << ")\n";
    }

    void operator()(const mapbox::geojson::line_string& line) const
    {
        std::cout << "LineString with points:\n";
        for (const auto& pt : line)
            std::cout << "  (" << pt.x << ", " << pt.y << ")\n";
    }

    void operator()(const mapbox::geojson::polygon& polygon) const
    {
        std::cout << "Polygon with " << polygon.size() << " rings\n";
        for (size_t i = 0; i < polygon.size(); ++i)
            std::cout << "  Ring " << i << " has " << polygon[i].size() << " points\n";
    }

    void operator()(const mapbox::geojson::geometry_collection& collection) const
    {
        std::cout << "GeometryCollection with " << collection.size() << " geometries:\n";
        for (const auto& geom : collection)
            mapbox::util::apply_visitor(*this, geom);
    }

    void operator()(const mapbox::geojson::multi_point&) const { /*...*/ }
    void operator()(const mapbox::geojson::multi_line_string&) const { /*...*/ }
    void operator()(const mapbox::geojson::multi_polygon&) const { /*...*/ }

    template <typename T> void operator()(const T& g) const
    { std::cout << "Unknown type: " << typeid(g).name() << "\n"; }
};

struct GeojsonVisitor
{
    void operator()(const mapbox::geojson::geometry& geom)
    {
        GeometryVisitor visitor;
        mapbox::util::apply_visitor(visitor, geom);
    }

    void operator()(const mapbox::geojson::feature& feature)
    {
        std::cout << "Processing Feature\n";
        // TODO
    }

    void operator()(const mapbox::geojson::feature_collection& collection)
    {
        std::cout << "Processing FeatureCollection with " << collection.size() << " features\n";
        for (const auto& feature : collection) (*this)(feature);
    }
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
        return ReadResult::FILE_NOT_HANDLED;
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
