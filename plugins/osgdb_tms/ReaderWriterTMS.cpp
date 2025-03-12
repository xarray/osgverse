#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <modeling/Math.h>
#include <pipeline/Utilities.h>

// osgviewer 0-0-0.verse_tms -O "URL=https://webst01.is.autonavi.com/appmaptile?style%3d6&x%3d{x}&y%3d{y}&z%3d{z} UseWebMercator=1"
// osgviewer 0-0-x.verse_tms -O "URL=E:\testTMS\{z}\{x}\{y}.png OriginBottomLeft=1"
class ReaderWriterTMS : public osgDB::ReaderWriter
{
public:
    ReaderWriterTMS()
    {
        supportsExtension("verse_tms", "osgVerse pseudo-loader");
        supportsExtension("tms", "TMS tile indices");
        supportsOption("URL", "The TMS server URL with wildcards");
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
            
            std::string useEarth = options->getPluginStringData("UseEarth3D");
            if (!useEarth.empty()) std::transform(useEarth.begin(), useEarth.end(), useEarth.begin(), tolower);
            bool flatten = (useEarth == "false" || atoi(useEarth.c_str()) <= 0);

            osg::ref_ptr<osg::Group> group = new osg::Group;
            for (int yy = 0; yy < countY; ++yy)
                for (int xx = 0; xx < 2; ++xx)
                {
                    osg::ref_ptr<osg::Node> node = createTile(
                        pseudoAddr, x + xx, y + yy, z, extentMin, extentMax, options, flatten);
                    if (!node) continue;

                    osg::ref_ptr<osg::PagedLOD> plod = new osg::PagedLOD;
                    plod->setDatabaseOptions(options->cloneOptions());
                    plod->addChild(node.get());
                    plod->setFileName(1, std::to_string(x + xx) + "-" + std::to_string(y + yy) +
                                         "-" + std::to_string(z) + ".verse_tms");
                    plod->setRangeMode(osg::LOD::PIXEL_SIZE_ON_SCREEN);
                    if (flatten)
                        { plod->setRange(0, 0.0f, 500.0f); plod->setRange(1, 500.0f, FLT_MAX); }
                    else
                        { plod->setRange(0, 0.0f, 1000.0f); plod->setRange(1, 1000.0f, FLT_MAX); }
                    group->addChild(plod.get());
                }
            group->setName(fileName);
            return (group->getNumChildren() > 0) ? group.get() : NULL;
        }
        return ReadResult::FILE_NOT_FOUND;
    }

protected:
    osg::Node* createTile(const std::string& pseudoPath, int x, int y, int z,
                          const osg::Vec3d& extentMin, const osg::Vec3d& extentMax,
                          const Options* opt, bool flatten) const
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

        std::string url = createPath(pseudoPath, x, y, z);
        std::string postfix = osgDB::getServerProtocol(url).empty() ? "" : ".verse_web";
        osg::ref_ptr<osg::Image> image = osgDB::readImageFile(url + postfix);
        if (image.valid())
        {
            osg::Matrix localMatrix;
            osg::ref_ptr<osg::Geometry> geom = createTileGeometry(
                localMatrix, tileMin, tileMax, tileWidth, tileHeight, flatten);
            geom->getOrCreateStateSet()->setTextureAttributeAndModes(
                0, osgVerse::createTexture2D(image.get(), osg::Texture::MIRROR));
            geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);

            osg::ref_ptr<osg::Geode> geode = new osg::Geode;
            geode->addDrawable(geom.get());
            if (!flatten)
            {
                osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
                mt->setMatrix(localMatrix); mt->setName(url);
                mt->addChild(geode.get()); return mt.release();
            }
            else
                { geode->setName(url); return geode.release(); }
        }
        else return NULL;
    }

    osg::Geometry* createTileGeometry(osg::Matrix& outMatrix,
                                      const osg::Vec3d& tileMin, const osg::Vec3d& tileMax,
                                      double width, double height, bool flatten) const
    {
        if (!flatten)
        {
            osg::Vec3d center = adjustLatitudeLongitudeAltitude((tileMin + tileMax) * 0.5, true);
            osg::Matrix localToWorld = osgVerse::Coordinate::convertLLAtoENU(center);
            osg::Matrix worldToLocal = osg::Matrix::inverse(localToWorld);
            osg::Matrix normalMatrix(localToWorld(0, 0), localToWorld(0, 1), localToWorld(0, 2), 0.0,
                                     localToWorld(1, 0), localToWorld(1, 1), localToWorld(1, 2), 0.0,
                                     localToWorld(2, 0), localToWorld(2, 1), localToWorld(2, 2), 0.0,
                                     0.0, 0.0, 0.0, 1.0);
            unsigned int numRows = 16, numCols = 16;
            outMatrix = localToWorld;

            osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array(numCols * numRows);
            osg::ref_ptr<osg::Vec3Array> na = new osg::Vec3Array(numCols * numRows);
            osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array(numCols * numRows);
            double invW = width / (float)(numCols - 1), invH = height / (float)(numRows - 1);
            for (unsigned int y = 0; y < numRows; ++y)
                for (unsigned int x = 0; x < numCols; ++x)
                {
                    unsigned int vi = x + y * numCols;
                    osg::Vec3d lla = adjustLatitudeLongitudeAltitude(
                        tileMin + osg::Vec3d((double)x * invW, (double)y * invH, 0.0), true);
                    osg::Vec3d ecef = osgVerse::Coordinate::convertLLAtoECEF(lla);
                    (*va)[vi] = osg::Vec3(ecef * worldToLocal);
                    (*na)[vi] = osg::Vec3(ecef * normalMatrix);
                    (*ta)[vi] = osg::Vec2((double)x * invW / width, (double)y * invH / height);
                }

            osg::ref_ptr<osg::DrawElementsUShort> de = new osg::DrawElementsUShort(GL_TRIANGLES);
            for (unsigned int y = 0; y < numRows - 1; ++y)
                for (unsigned int x = 0; x < numCols - 1; ++x)
                {
                    unsigned int vi = x + y * numCols;
                    de->push_back(vi); de->push_back(vi + 1); de->push_back(vi + numCols);
                    de->push_back(vi + numCols); de->push_back(vi + 1); de->push_back(vi + numCols + 1);
                    //de->push_back(vi); de->push_back(vi + 1);
                    //de->push_back(vi + numCols + 1); de->push_back(vi + numCols);
                }

            osg::Geometry* geom = new osg::Geometry;
            geom->setVertexArray(va.get()); geom->setTexCoordArray(0, ta.get());
            geom->setNormalArray(na.get()); geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
            geom->addPrimitiveSet(de.get()); return geom;
        }
        else
            return osg::createTexturedQuadGeometry(tileMin, osg::X_AXIS * width, osg::Y_AXIS * height);
    }

    osg::Vec3d adjustLatitudeLongitudeAltitude(const osg::Vec3d& extent, bool useSphericalMercator) const
    {
        osg::Vec3d lla(osg::inDegrees(extent[1]), osg::inDegrees(extent[0]), 0.0);
        if (useSphericalMercator)
        {
            double n = 2.0 * lla.x();
            double adjustedLatitude = atan(0.5 * (exp(n) - exp(-n)));
            return osg::Vec3d(adjustedLatitude, lla.y(), lla.z());
        }
        return lla;
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
