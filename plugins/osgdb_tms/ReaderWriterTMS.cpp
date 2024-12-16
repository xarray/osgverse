#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include "pipeline/Utilities.h"

class ReaderWriterTMS : public osgDB::ReaderWriter
{
public:
    ReaderWriterTMS()
    {
        // osgviewer 0-0-0.verse_tms -O "https://webst01.is.autonavi.com/appmaptile?style=6&x={x}&y={y}&z={z}"
        supportsExtension("verse_tms", "osgVerse pseudo-loader");
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
            const std::string& pseudoAddr = options->getOptionString();
            int x = atoi(values[0].c_str()) * 2, y = atoi(values[1].c_str()) * 2,
                z = atoi(values[2].c_str()) + 1;

            osg::ref_ptr<osg::Group> group = new osg::Group;
            for (int yy = 0; yy < 2; ++yy)
                for (int xx = 0; xx < 2; ++xx)
                {
                    osg::ref_ptr<osg::PagedLOD> plod = new osg::PagedLOD;
                    plod->addChild(createTile(pseudoAddr, x + xx, y + yy, z));
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
    osg::Node* createTile(const std::string& pseudoPath, int x, int y, int z) const
    {
        osg::Vec3d extentMin = osg::Vec3d(-180.0, -90.0, 0.0);
        osg::Vec3d extentMax = osg::Vec3d(180.0, 90.0, 0.0);  // FIXME

        double multiplier = pow(0.5, double(z));
        double tileWidth = multiplier * (extentMax.x() - extentMin.x());
        double tileHeight = multiplier * (extentMax.y() - extentMin.y());
#if true
        osg::Vec3d origin(extentMin.x(), extentMax.y(), extentMin.z());
        osg::Vec3d tileMin = origin + osg::Vec3d(double(x) * tileWidth, -double(y + 1) * tileHeight, 0.0);
        osg::Vec3d tileMax = origin + osg::Vec3d(double(x + 1) * tileWidth, -double(y) * tileHeight, 1.0);
#else
        osg::Vec3d tileMin = extentMin + osg::Vec3d(double(x) * tileWidth, double(y) * tileHeight, 0.0);
        osg::Vec3d tileMax = extentMin + osg::Vec3d(double(x + 1) * tileWidth, double(y + 1) * tileHeight, 1.0);
#endif

        osg::ref_ptr<osg::Geometry> geom =
            osg::createTexturedQuadGeometry(tileMin, osg::X_AXIS * tileWidth, osg::Y_AXIS * tileHeight);
        geom->getOrCreateStateSet()->setTextureAttributeAndModes(0, osgVerse::createTexture2D(
            osgDB::readImageFile(createPath(pseudoPath, x, y, z) + ".verse_web"), osg::Texture::MIRROR));

        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable(geom.get()); return geode.release();
    }

    std::string createPath(const std::string& pseudoPath, int x, int y, int z) const
    {
        std::string path = pseudoPath;
        path = replace(path, "{z}", std::to_string(z));
        path = replace(path, "{x}", std::to_string(x));
        path = replace(path, "{y}", std::to_string(y)); return path;
    }

    std::string replace(std::string& src, const std::string& match, const std::string& value) const
    {
        size_t levelPos = src.find(match); if (levelPos == std::string::npos) return src;
        src.replace(levelPos, match.length(), value); return src;
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_tms, ReaderWriterTMS)
