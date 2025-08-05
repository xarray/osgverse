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

// WebMercatorTiles: (0-0-0) Lv1 = 00,01,10,11, ...; (0-0-x) Lv0 = 00,01,10,11, ...
// WGS84Tiles: (0-0-0) Lv1 = 00,10, Lv2 = 00,01,10,11,20,21,30,31, ...; (0-0-x) Lv0 = 00,10, ...

// osgviewer 0-0-0.verse_tms -O "Orthophoto=https://xxxx.com/tile/{z}/{y}/{x} ..."
// osgviewer 0-0-x.verse_tms -O "Orthophoto=E:\testTMS\{z}\{x}\{y}.png ..."
// osgviewer 0-0-x.verse_tms -O "Orthophoto=E:\testTMS\all_in_pack.mbtiles ..."

/* Tile map servers:
     Arcgis Map:
     - Satellite: https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}
     - Roadmap: https://server.arcgisonline.com/arcgis/rest/services/World_Street_Map/MapServer/tile/{z}/{y}/{x}
     - Terrain: https://server.arcgisonline.com/arcgis/rest/services/WorldElevation3D/Terrain3D/ImageServer/tile/{z}/{y}/{x}
       - ArcGIS Terrain 3D format (application/octet-stream, gzip)
       - Info: https://server.arcgisonline.com/arcgis/rest/services/WorldElevation3D/Terrain3D/ImageServer
     GapDe Map: https://webst01.is.autonavi.com/appmaptile?style%3d{s}&x%3d{x}&y%3d{y}&z%3d{z}
     - {s}: 6 = satellite, 7 = roadmap
     TengXun Map: (https://blog.csdn.net/mygisforum/article/details/22997879)
     - http://p0.map.gtimg.com/demTiles/{z}/{x/16}/{y/16}/{x}_{y}.jpg
     - http://p0.map.gtimg.com/sateTiles/{z}/{x/16}/{y/16}/{x}_{y}.jpg
     - options->setPluginData("UrlPathFunction", (void*)createTengXunPath);
     - Custom function:
       static std::string createTengXunPath(int type, const std::string& prefix, int x, int y, int z) {
           std::string path = prefix; bool changed = false;
           path = replace(path, "{x16}", std::to_string(x / 16), changed);
           path = replace(path, "{y16}", std::to_string(y / 16), changed);
           y = (int)pow((double)z, 2.0) - y - 1; }
     Baidu Map: http://shangetu{s}.map.bdimg.com/it/u=x={x};y={y};z={z};v=009;type=sate&fm=46
     - {s}: 0, 1, 2, 3
     Google Map: http://{s}.google.com/vt/lyrs={t}&x={x}&y={y}&z={z}
     - {s}: mt0, mt1, mt2, mt3
     - {t}: h = roads only, m = standard roadmap, p = colored terrain, s = satellite only, t = terrain only, y = hybrid
     Carto Voyager: https://{s}.basemaps.cartocdn.com/rastertiles/voyager/{z}/{x}/{y}{r}.png
     - {s}: a, b, c; {r}: no set or @2x
     Custom TMS from GlobalMapper:
     - options->setPluginData("UrlPathFunction", (void*)createGlobalMapperPath);
     - Custom function:
       static std::string createGlobalMapperPath(int type, const std::string& prefix, int x, int y, int z) {
           int newY = pow(2, z) + y, newZ = z + 1;  // if "OriginBottomLeft=1"
           //int newY = pow(2, z + 1) - y, newZ = z + 1;  // if "OriginBottomLeft=0"
           return osgVerse::TileCallback::createPath(prefix, x, newY, newZ); }
*/
class ReaderWriterTMS : public osgDB::ReaderWriter
{
public:
    ReaderWriterTMS()
    {
        supportsExtension("verse_tms", "osgVerse pseudo-loader");
        supportsExtension("tms", "TMS tile indices");
        supportsOption("URL", "The TMS server URL with wildcards, applied as orthophoto layer");
        supportsOption("Orthophoto", "TMS server URL with wildcards or .mbtiles, applied as orthophoto layer");
        supportsOption("Elevation", "TMS server URL with wildcards or .mbtiles, applied as elevation layer");
        supportsOption("OceanMask", "TMS server URL with wildcards or .mbtiles, applied as ocean mask layer");
        supportsOption("UrlPathFunction", "The custom function from setPluginData() to compute tile URL");
        supportsOption("UseEarth3D", "Display TMS tiles as a real earth: default=0");
        supportsOption("UseWebMercator", "Use Web Mercator (Level-0 has 4 tiles): default=0");
        supportsOption("OriginBottomLeft", "Use bottom-left as every tile's origin point: default=0");
        supportsOption("FlatExtentMinX", "Flat earth extent X0: default -180");
        supportsOption("FlatExtentMinY", "Flat earth extent Y0: default -90");
        supportsOption("FlatExtentMaxX", "Flat earth extent X1: default 180");
        supportsOption("FlatExtentMaxY", "Flat earth extent Y1: default 90");
        supportsOption("MaximumLevel", "Set maximum level (Z) to load: default 0 (infinite)");
        supportsOption("TileSkirtRatio", "Create skirts for every tile: default 0.02");
        supportsOption("TileElevationScale", "Set elevation scale for every tile: default 1.0");
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

        bool usePseudo = (ext == "verse_tms"), useWM = true;
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

            std::string maxLvStr = options->getPluginStringData("MaximumLevel");
            int maxZ = atoi(maxLvStr.c_str());  // return OK but not loading anything
            if (maxZ > 0 && maxZ < z) return ReadResult::FILE_LOADED;

            std::string maskAddr = osgVerse::urlDecode(options->getPluginStringData("OceanMask"));
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
                if (z == 0) countY = 1; useWM = false;
            }

            std::string useEarth = options->getPluginStringData("UseEarth3D");
            if (!useEarth.empty()) std::transform(useEarth.begin(), useEarth.end(), useEarth.begin(), tolower);
            bool flatten = (useEarth == "false" || atoi(useEarth.c_str()) <= 0);

            osg::ref_ptr<osg::Group> group = new osg::Group;
            for (int yy = 0; yy < countY; ++yy)
                for (int xx = 0; xx < 2; ++xx)
                {
                    osg::ref_ptr<osg::PagedLOD> plod = new osg::PagedLOD;
                    osg::ref_ptr<osg::Node> node = createTile(
                        plod.get(), elevAddr, orthoAddr, maskAddr, x + xx, y + yy, z,
                        extentMin, extentMax, options, useWM, flatten);
                    if (!node) continue;

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
    osg::Node* createTile(osg::LOD* parent, const std::string& elevPath, const std::string& orthPath,
                          const std::string& maskPath, int x, int y, int z,
                          const osg::Vec3d& extentMin, const osg::Vec3d& extentMax,
                          const Options* opt, bool useWM, bool flatten) const
    {
        CreatePathFunc pathFunc = (CreatePathFunc)opt->getPluginData("UrlPathFunction");
        std::string name = "TMS_" + std::to_string(x) + "_" + std::to_string(y) + "_" + std::to_string(z),
                    botLeft = opt->getPluginStringData("OriginBottomLeft");
        if (!botLeft.empty()) std::transform(botLeft.begin(), botLeft.end(), botLeft.begin(), tolower);

        std::string elevPath1 = elevPath, ext = osgDB::getFileExtensionIncludingDot(elevPath), extWithVerse;
        size_t queryInExt = ext.find("?");  // remove query string if mixed with extension
        if (queryInExt != std::string::npos) ext = ext.substr(0, queryInExt);

        osgVerse::TileManager* mgr = osgVerse::TileManager::instance();
        bool elevH = mgr->isHandlerExtension(ext, extWithVerse), changed = false;
        if (!extWithVerse.empty() && osgDB::getServerProtocol(elevPath1).empty())  // auto-change local file ext
            osgVerse::TileCallback::replace(elevPath1, ext, extWithVerse, changed);

        osg::ref_ptr<osgVerse::TileCallback> tileCB = new osgVerse::TileCallback;
        tileCB->setLayerPath(osgVerse::TileCallback::ELEVATION, elevPath1);
        tileCB->setLayerPath(osgVerse::TileCallback::ORTHOPHOTO, orthPath);
        tileCB->setLayerPath(osgVerse::TileCallback::OCEAN_MASK, maskPath);
        tileCB->setTotalExtent(extentMin, extentMax); tileCB->setTileNumber(x, y, z);
        tileCB->setBottomLeft(botLeft == "true" || atoi(botLeft.c_str()) > 0);
        tileCB->setUseWebMercator(useWM); tileCB->setFlatten(flatten);
        tileCB->setCreatePathFunction(pathFunc);
        if (opt)
        {
            std::string skirt = opt->getPluginStringData("TileSkirtRatio");
            if (!skirt.empty()) tileCB->setSkirtRatio((float)atof(skirt.c_str()));

            std::string scale = opt->getPluginStringData("TileElevationScale");
            if (!scale.empty()) tileCB->setElevationScale((float)atof(scale.c_str()));
        }

        osg::Vec3d tileMin, tileMax; double tileWidth = 0.0, tileHeight;
        tileCB->computeTileExtent(tileMin, tileMax, tileWidth, tileHeight);

        osg::Matrix localMatrix; osg::ref_ptr<osg::Texture> elevImage;
        osg::ref_ptr<osgVerse::TileGeometryHandler> elevHandler; bool emptyPath0 = false, emptyPath1 = false;
        if (elevH)
            elevHandler = tileCB->createLayerHandler(osgVerse::TileCallback::ELEVATION, emptyPath0);
        else
            elevImage = tileCB->createLayerImage(osgVerse::TileCallback::ELEVATION, emptyPath0);
        if (!elevHandler && !elevImage && !emptyPath0)  // try to find parent data
            elevImage = tileCB->findAndUseParentData(osgVerse::TileCallback::ELEVATION, parent);

        osg::ref_ptr<osg::Texture> orthImage = tileCB->createLayerImage(osgVerse::TileCallback::ORTHOPHOTO, emptyPath0);
        osg::ref_ptr<osg::Texture> maskImage = tileCB->createLayerImage(osgVerse::TileCallback::OCEAN_MASK, emptyPath1);
        if (!orthImage && !emptyPath0)  // try to find parent data
            orthImage = tileCB->findAndUseParentData(osgVerse::TileCallback::ORTHOPHOTO, parent);
        if (!maskImage && !emptyPath1)  // try to find parent data
            maskImage = tileCB->findAndUseParentData(osgVerse::TileCallback::OCEAN_MASK, parent);
        if (!orthImage) return NULL;

        osg::ref_ptr<osg::Geometry> geom = elevHandler.valid() ?
            tileCB->createTileGeometry(localMatrix, elevHandler.get(), tileMin, tileMax, tileWidth, tileHeight) :
            tileCB->createTileGeometry(localMatrix, elevImage.get(), tileMin, tileMax, tileWidth, tileHeight);
        geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true); geom->setName(name + "_Geom");
        if (orthImage.valid())
        {
#if defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE) || defined(OSG_GL3_AVAILABLE)
            geom->getOrCreateStateSet()->setTextureAttribute(0, orthImage.get());
#else
            geom->getOrCreateStateSet()->setTextureAttributeAndModes(0, orthImage.get());
#endif
            geom->getOrCreateStateSet()->getOrCreateUniform("UvOffset1", osg::Uniform::FLOAT_VEC4)
                                       ->set(osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
        }

        if (maskImage.valid())
        {
#if defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE) || defined(OSG_GL3_AVAILABLE)
            geom->getOrCreateStateSet()->setTextureAttribute(1, maskImage.get());
#else
            geom->getOrCreateStateSet()->setTextureAttributeAndModes(1, maskImage.get());
#endif
            geom->getOrCreateStateSet()->getOrCreateUniform("UvOffset2", osg::Uniform::FLOAT_VEC4)
                                       ->set(osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
        }
        // FIXME: handle more layers? using tex2d array?

        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable(geom.get()); geode->setName(name + "_Geode");

        osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
        mt->setUpdateCallback(tileCB.get());
        mt->setName(name); mt->addChild(geode.get());
        mt->setMatrix(localMatrix); return mt.release();
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_tms, ReaderWriterTMS)
