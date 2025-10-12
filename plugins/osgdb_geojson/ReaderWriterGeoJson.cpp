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

struct TriangleCollector
{
    std::vector<unsigned int> triangles;
    unsigned int start;

    void operator()(unsigned int i1, unsigned int i2, unsigned int i3)
    { triangles.push_back(start + i1); triangles.push_back(start + i2); triangles.push_back(start + i3); }
};

struct GeometryVisitor
{
    void operator()(const mapbox::geojson::point& point) const
    {
        unsigned int vStart = vertices->size();
        vertices->push_back(osg::Vec3(point.x, point.y, 0.0f)); colors->push_back(defaultColor);
        geom->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, vStart, 1));
    }

    void operator()(const mapbox::geojson::line_string& line) const
    {
        unsigned int vStart = vertices->size();
        for (size_t i = 0; i < line.size(); ++i)
        {
            const mapbox::geojson::point& pt = line[i];
            vertices->push_back(osg::Vec3(pt.x, pt.y, 0.0f)); colors->push_back(defaultColor);
        }
        geom->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, vStart, vertices->size() - vStart));
    }

    void operator()(const mapbox::geojson::polygon& polygon) const
    {
        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array; unsigned int vStart = 0;
        osg::ref_ptr<osg::Geometry> temp = new osg::Geometry; temp->setVertexArray(va.get());
        for (size_t i = 0; i < polygon.size(); ++i)
        {
            const auto& poly = polygon[i]; vStart = va->size();
            for (size_t j = 0; j < poly.size(); ++j)
            {
                const mapbox::geojson::point& pt = poly[j];
                va->push_back(osg::Vec3(pt.x, pt.y, 0.0f));
            }
            if (va->at(vStart) == va->back()) va->pop_back();
            temp->addPrimitiveSet(new osg::DrawArrays(GL_POLYGON, vStart, va->size() - vStart));
        }
        
        osg::ref_ptr<osgUtil::Tessellator> tscx = new osgUtil::Tessellator;
        tscx->setWindingType(osgUtil::Tessellator::TESS_WINDING_ODD);
        tscx->setTessellationType(osgUtil::Tessellator::TESS_TYPE_POLYGONS);
        tscx->setTessellationNormal(osg::Z_AXIS);
        tscx->retessellatePolygons(*temp);

        vStart = vertices->size();
        vertices->insert(vertices->end(), va->begin(), va->end());
        colors->insert(colors->end(), va->size(), defaultColor);
        
        osg::TriangleIndexFunctor<TriangleCollector> f; f.start = vStart; temp->accept(f);
        if (f.triangles.size() < 65535)
            geom->addPrimitiveSet(new osg::DrawElementsUShort(GL_TRIANGLES, f.triangles.begin(), f.triangles.end()));
        else
            geom->addPrimitiveSet(new osg::DrawElementsUInt(GL_TRIANGLES, f.triangles.begin(), f.triangles.end()));
    }

    void operator()(const mapbox::geojson::multi_point& points) const
    {
        unsigned int vStart = vertices->size();
        for (size_t i = 0; i < points.size(); ++i)
        {
            const mapbox::geojson::point& pt = points[i];
            vertices->push_back(osg::Vec3(pt.x, pt.y, 0.0f)); colors->push_back(defaultColor);
        }
        geom->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, vStart, vertices->size() - vStart));
    }

    void operator()(const mapbox::geojson::multi_line_string& lines) const
    {
        for (size_t i = 0; i < lines.size(); ++i) (*this)(lines[i]);
    }

    void operator()(const mapbox::geojson::multi_polygon& polygons) const
    {
        for (size_t i = 0; i < polygons.size(); ++i) (*this)(polygons[i]);
    }

    void operator()(const mapbox::geojson::geometry_collection& collection) const
    {
        for (const auto& geom : collection)
            mapbox::util::apply_visitor(*this, geom);
    }

    template <typename T> void operator()(const T& g) const
    { std::cout << "Unknown type: " << typeid(g).name() << "\n"; }

    GeometryVisitor(osg::Geometry* g) : geom(g)
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

    osg::observer_ptr<osg::Geometry> geom;
    osg::ref_ptr<osg::Vec3Array> vertices;
    osg::ref_ptr<osg::Vec4Array> colors;
    osg::Vec4 defaultColor;
};

struct GeojsonVisitor
{
    void operator()(const mapbox::geojson::geometry& gData)
    {
        osg::ref_ptr<osg::Geometry> geom;
        if (!current)
        {
            geom = new osg::Geometry; current = geom.get();
            geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
        }

        GeometryVisitor visitor(current.get());
        mapbox::util::apply_visitor(visitor, gData);
        if (current.valid() && current->getVertexArray())
            geode->addDrawable(current.get());
        current = NULL;
    }

    void operator()(const mapbox::geojson::feature& feature)
    {
        osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
        geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
        if (feature.id.is<std::string>()) geom->setName(feature.id.get<std::string>());

        for (std::unordered_map<std::string, mapbox::geojson::value>::const_iterator
             it = feature.properties.begin(); it != feature.properties.end(); ++it)
        {
            if (it->second.is<std::string>())
                geom->setUserValue(it->first, it->second.get<std::string>());
            else if (it->second.is<double>())
                geom->setUserValue(it->first, it->second.get<double>());
            else if (it->second.is<bool>())
                geom->setUserValue(it->first, it->second.get<bool>());
        }
        current = geom.get(); (*this)(feature.geometry);
    }

    void operator()(const mapbox::geojson::feature_collection& collection)
    {
        for (const auto& feature : collection) (*this)(feature);
    }

    GeojsonVisitor() { geode = new osg::Geode; }
    osg::observer_ptr<osg::Geometry> current;
    osg::ref_ptr<osg::Geode> geode;
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
        return visitor.geode;
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
