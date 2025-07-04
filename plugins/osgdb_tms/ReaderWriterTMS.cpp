#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>

#include <pipeline/Utilities.h>
#include <readerwriter/Utilities.h>
#include <readerwriter/TileCallback.h>

// osgviewer 0-0-0.verse_tms -O "Orthophoto=https://webst01.is.autonavi.com/appmaptile?style%3d6&x%3d{x}&y%3d{y}&z%3d{z} UseWebMercator=1"
// osgviewer 0-0-x.verse_tms -O "Orthophoto=E:\testTMS\{z}\{x}\{y}.png OriginBottomLeft=1"
/* More tile map servers:
     TengXun Map: (https://blog.csdn.net/mygisforum/article/details/22997879)
     - http://p0.map.gtimg.com/demTiles/{z}/{x/16}/{y/16}/{x}_{y}.jpg
     - http://p0.map.gtimg.com/sateTiles/{z}/{x/16}/{y/16}/{x}_{y}.jpg
     Baidu Map: http://shangetu{s}.map.bdimg.com/it/u=x={x};y={y};z={z};v=009;type=sate&fm=46
     - {s}: 0, 1, 2, 3
     Google Map: http://{s}.google.com/vt/lyrs={t}&x={x}&y={y}&z={z}
     - {s}: mt0, mt1, mt2, mt3
     - {t}: h = roads only, m = standard roadmap, p = colored terrain, s = satellite only, t = terrain only, y = hybrid
     Carto Voyager: https://{s}.basemaps.cartocdn.com/rastertiles/voyager/{z}/{x}/{y}{r}.png
     - {s}: a, b, c; {r}: no set or @2x
     ArcGIS: https://services.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{x}/{y}
*/
class ReaderWriterTMS : public osgDB::ReaderWriter
{
public:
    ReaderWriterTMS()
    {
        supportsExtension("verse_tms", "osgVerse pseudo-loader");
        supportsExtension("tms", "TMS tile indices");
        supportsOption("URL", "The TMS server URL with wildcards, applied as orthophoto layer");
        supportsOption("Orthophoto", "The TMS server URL with wildcards, applied as orthophoto layer");
        supportsOption("Elevation", "The TMS server URL with wildcards, applied as elevation layer");
        supportsOption("Vector", "The TMS server URL with wildcards, applied as line graph layer");
        supportsOption("UrlPathFunction", "The custom function from setPluginData() to compute tile URL");
        supportsOption("UseEarth3D", "Display TMS tiles as a real earth: default=0");
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
            std::string vectAddr = osgVerse::urlDecode(options->getPluginStringData("Vector"));
            std::string elevAddr = osgVerse::urlDecode(options->getPluginStringData("Elevation"));
            std::string orthoAddr = osgVerse::urlDecode(options->getPluginStringData("Orthophoto"));
            if (orthoAddr.empty()) orthoAddr = osgVerse::urlDecode(options->getPluginStringData("URL"));

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
            
            std::string useEarth = options->getPluginStringData("UseEarth3D");
            if (!useEarth.empty()) std::transform(useEarth.begin(), useEarth.end(), useEarth.begin(), tolower);
            bool flatten = (useEarth == "false" || atoi(useEarth.c_str()) <= 0);

            osg::ref_ptr<osg::Group> group = new osg::Group;
            for (int yy = 0; yy < countY; ++yy)
                for (int xx = 0; xx < 2; ++xx)
                {
                    osg::ref_ptr<osg::Node> node = createTile(
                        elevAddr, orthoAddr, vectAddr, x + xx, y + yy, z, extentMin, extentMax, options, flatten);
                    if (!node) continue;

                    osg::ref_ptr<osg::PagedLOD> plod = new osg::PagedLOD;
                    plod->setDatabaseOptions(options->cloneOptions());
                    plod->addChild(node.get()); plod->setName("TMSLod:" + fileName);
                    plod->setFileName(1, std::to_string(x + xx) + "-" + std::to_string(y + yy) +
                                         "-" + std::to_string(z) + ".verse_tms");
                    plod->setRangeMode(osg::LOD::PIXEL_SIZE_ON_SCREEN);
                    if (flatten)
                        { plod->setRange(0, 0.0f, 500.0f); plod->setRange(1, 500.0f, FLT_MAX); }
                    else
                        { plod->setRange(0, 0.0f, 1000.0f); plod->setRange(1, 1000.0f, FLT_MAX); }
                    group->addChild(plod.get());
                }
            group->setName("TMSGroup:" + fileName);
            return (group->getNumChildren() > 0) ? group.get() : NULL;
        }
        return ReadResult::FILE_NOT_FOUND;
    }

protected:
    osg::Node* createTile(const std::string& elevPath, const std::string& orthPath,
                          const std::string& vectPath, int x, int y, int z,
                          const osg::Vec3d& extentMin, const osg::Vec3d& extentMax,
                          const Options* opt, bool flatten) const
    {
        CreatePathFunc pathFunc = (CreatePathFunc)opt->getPluginData("UrlPathFunction");
        std::string name = "TMS_" + std::to_string(z) + "_" + std::to_string(x) + "_" + std::to_string(y),
                    botLeft = opt->getPluginStringData("OriginBottomLeft");
        if (!botLeft.empty()) std::transform(botLeft.begin(), botLeft.end(), botLeft.begin(), tolower);

        osg::ref_ptr<osgVerse::TileCallback> tileCB = new osgVerse::TileCallback;
        tileCB->setLayerPath(osgVerse::TileCallback::ELEVATION, elevPath);
        tileCB->setLayerPath(osgVerse::TileCallback::ORTHOPHOTO, orthPath);
        tileCB->setLayerPath(osgVerse::TileCallback::VECTOR, vectPath);
        tileCB->setTotalExtent(extentMin, extentMax); tileCB->setTileNumber(x, y, z);
        tileCB->setBottomLeft(botLeft == "true" || atoi(botLeft.c_str()) > 0);
        tileCB->setFlatten(flatten); tileCB->setCreatePathFunction(pathFunc);

        osg::Vec3d tileMin, tileMax; double tileWidth = 0.0, tileHeight;
        tileCB->computeTileExtent(tileMin, tileMax, tileWidth, tileHeight);

        osg::Matrix localMatrix;
        osgVerse::TileManager* mgr = osgVerse::TileManager::instance();
        bool elevH = mgr->isHandlerExtension(osgDB::getFileExtension(elevPath));

        osg::ref_ptr<osg::Image> elevImage; osg::ref_ptr<osgVerse::TileGeometryHandler> elevHandler;
        if (elevH) elevHandler = tileCB->createLayerHandler(osgVerse::TileCallback::ELEVATION);
        else elevImage = tileCB->createLayerImage(osgVerse::TileCallback::ELEVATION);

        osg::ref_ptr<osg::Image> orthImage = tileCB->createLayerImage(osgVerse::TileCallback::ORTHOPHOTO);
        osg::ref_ptr<osg::Image> vectImage = tileCB->createLayerImage(osgVerse::TileCallback::VECTOR);

        osg::ref_ptr<osg::Geometry> geom = elevHandler.valid() ?
            tileCB->createTileGeometry(localMatrix, elevHandler.get(), tileMin, tileMax, tileWidth, tileHeight) :
            tileCB->createTileGeometry(localMatrix, elevImage.get(), tileMin, tileMax, tileWidth, tileHeight);
        geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
        if (orthImage.valid())
        {
            geom->getOrCreateStateSet()->setTextureAttributeAndModes(
                0, osgVerse::createTexture2D(orthImage.get(), osg::Texture::CLAMP_TO_EDGE));
        }
        if (vectImage.valid())
        {
            geom->getOrCreateStateSet()->setTextureAttributeAndModes(
                1, osgVerse::createTexture2D(vectImage.get(), osg::Texture::CLAMP_TO_EDGE));
        }

        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable(geom.get());

        osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
        mt->setUpdateCallback(tileCB.get());
        mt->setName(name); mt->addChild(geode.get());
        mt->setMatrix(localMatrix); return mt.release();
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_tms, ReaderWriterTMS)
