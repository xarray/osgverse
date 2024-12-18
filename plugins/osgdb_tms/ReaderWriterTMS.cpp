#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <pipeline/Utilities.h>

// osgviewer 0-0-0.verse_tms -O "URL=https://webst01.is.autonavi.com/appmaptile?style%3d6&x%3d{x}&y%3d{y}&z%3d{z} UseWebMercator=1"
// osgviewer 0-0-x.verse_tms -O "URL=http://192.168.2.7:8088/freexserver/htc/service/tms/1.0.0/YTH_DT@EPSG%3A4326@png/{z}/{x}/{y}.png OriginBottomLeft=1"
class ReaderWriterTMS : public osgDB::ReaderWriter
{
public:
    ReaderWriterTMS()
    {
        supportsExtension("verse_tms", "osgVerse pseudo-loader");
        supportsExtension("tms", "TMS tile indices");
        supportsOption("URL", "The TMS server URL with wildcards");
        supportsOption("UseWebMercator", "Use Web Mercator (Level-0 has 4 tiles): default=0");
        supportsOption("OriginBottomLeft", "Use bottom-left as every tile's origin point: default=0");
        supportsOption("FlatExtentMinX", "Flat earth extent X0: default -180");
        supportsOption("FlatExtentMinY", "Flat earth extent Y0: default -90");
        supportsOption("FlatExtentMaxX", "Flat earth extent X1: default 180");
        supportsOption("FlatExtentMaxY", "Flat earth extent Y1: default 90");
    }

    virtual const char* className() const
    {
        return "[osgVerse] TMS tile reader";
    }

    virtual ReadResult readNode(const std::string& path, const Options* options) const
    {
        std::string fileName(path);
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

        bool usePseudo = (ext == "verse_tms");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getLowerCaseFileExtension(fileName);
        }

        std::vector<std::string> values; osgDB::split(fileName, values, '-');
        if (options && values.size() > 2)
        {
            int x = atoi(values[0].c_str()) * 2, y = atoi(values[1].c_str()) * 2,
                z = (values[2] == "x") ? 0 : atoi(values[2].c_str()) + 1, countY = 2;
            std::string pseudoAddr = options->getPluginStringData("URL"); bool changed = true;
            while (changed) pseudoAddr = replace(pseudoAddr, "%3d", "=", changed);

            osg::Vec3d extentMin = osg::Vec3d(-180.0, -90.0, 0.0), extentMax = osg::Vec3d(180.0, 90.0, 0.0);
            std::string strX = options->getPluginStringData("FlatExtentMinX"),
                        strY = options->getPluginStringData("FlatExtentMinY");
            if (!strX.empty()) extentMin[0] = atof(strX.c_str()); if (!strY.empty()) extentMin[1] = atof(strY.c_str());
            strX = options->getPluginStringData("FlatExtentMaxX"); strY = options->getPluginStringData("FlatExtentMaxY");
            if (!strX.empty()) extentMax[0] = atof(strX.c_str()); if (!strY.empty()) extentMax[1] = atof(strY.c_str());

            std::string use4T = options->getPluginStringData("UseWebMercator");
            if (!use4T.empty()) std::transform(use4T.begin(), use4T.end(), use4T.begin(), tolower);
            if (use4T == "false" || atoi(use4T.c_str()) <= 0)
            {
                extentMax[0] = (extentMax[0] + extentMin[0]) * 0.5;
                if (z == 0) countY = 1;
            }

            osg::ref_ptr<osg::Group> group = new osg::Group;
            for (int yy = 0; yy < countY; ++yy)
                for (int xx = 0; xx < 2; ++xx)
                {
                    osg::ref_ptr<osg::PagedLOD> plod = new osg::PagedLOD;
                    plod->setDatabaseOptions(options->cloneOptions());
                    plod->addChild(createTile(pseudoAddr, x + xx, y + yy, z, extentMin, extentMax, options));
                    plod->setFileName(1, std::to_string(x + xx) + "-" + std::to_string(y + yy) +
                                         "-" + std::to_string(z) + ".verse_tms");
                    plod->setRangeMode(osg::LOD::PIXEL_SIZE_ON_SCREEN);
                    plod->setRange(0, 0.0f, 500.0f); plod->setRange(1, 500.0f, FLT_MAX);
                    group->addChild(plod.get());
                }
            group->setName(fileName); return group.get();
        }
        return ReadResult::FILE_NOT_FOUND;
    }

protected:
    osg::Node* createTile(const std::string& pseudoPath, int x, int y, int z,
                          const osg::Vec3d& extentMin, const osg::Vec3d& extentMax, const Options* opt) const
    {
        std::string botLeft = opt->getPluginStringData("OriginBottomLeft");
        if (!botLeft.empty()) std::transform(botLeft.begin(), botLeft.end(), botLeft.begin(), tolower);

        osg::Vec3d tileMin, tileMax; double multiplier = pow(0.5, double(z));
        double tileWidth = multiplier * (extentMax.x() - extentMin.x());
        double tileHeight = multiplier * (extentMax.y() - extentMin.y());
        if (botLeft == "false" || atoi(botLeft.c_str()) <= 0)
        {
            osg::Vec3d origin(extentMin.x(), extentMax.y(), extentMin.z());
            tileMin = origin + osg::Vec3d(double(x) * tileWidth, -double(y + 1) * tileHeight, 0.0);
            tileMax = origin + osg::Vec3d(double(x + 1) * tileWidth, -double(y) * tileHeight, 1.0);
        }
        else
        {
            tileMin = extentMin + osg::Vec3d(double(x) * tileWidth, double(y) * tileHeight, 0.0);
            tileMax = extentMin + osg::Vec3d(double(x + 1) * tileWidth, double(y + 1) * tileHeight, 1.0);
        }

        osg::ref_ptr<osg::Geometry> geom =
            osg::createTexturedQuadGeometry(tileMin, osg::X_AXIS * tileWidth, osg::Y_AXIS * tileHeight);
        geom->getOrCreateStateSet()->setTextureAttributeAndModes(0, osgVerse::createTexture2D(
            osgDB::readImageFile(createPath(pseudoPath, x, y, z) + ".verse_web"), osg::Texture::MIRROR));

        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable(geom.get()); return geode.release();
    }

    std::string createPath(const std::string& pseudoPath, int x, int y, int z) const
    {
        std::string path = pseudoPath; bool changed = false;
        path = replace(path, "{z}", std::to_string(z), changed);
        path = replace(path, "{x}", std::to_string(x), changed);
        path = replace(path, "{y}", std::to_string(y), changed); return path;
    }

    std::string replace(std::string& src, const std::string& match, const std::string& v, bool& c) const
    {
        size_t levelPos = src.find(match); if (levelPos == std::string::npos) { c = false; return src; }
        src.replace(levelPos, match.length(), v); c = true; return src;
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_tms, ReaderWriterTMS)
