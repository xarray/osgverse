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
#include <shapelib/shapefil.h>

class ReaderWriterSHP : public osgDB::ReaderWriter
{
public:
    ReaderWriterSHP()
    {
        supportsExtension("verse_shp", "osgVerse pseudo-loader");
        supportsExtension("shp", "Shapefile data file");
        supportsOption("IncludeFeatures", "Add FeatureCollection as UserData of the result Geometry/Image. Default: 0");
        supportsOption("ImageWidth", "Image resolution. Default: 512");
        supportsOption("ImageHeight", "Image resolution. Default: 512");
    }

virtual const char* className() const
{
    return "[osgVerse] ESRI Shapefile data format reader";
}

virtual ReadResult readObject(const std::string& path, const Options* options) const
{
    std::string ext; std::string fileName = getRealFileName(path, ext);
    if (fileName.empty()) return ReadResult::FILE_NOT_HANDLED;

    SHPHandle shp = SHPOpen(fileName.c_str(), "rb");
    if (shp == NULL) return ReadResult::FILE_NOT_HANDLED;

    osg::ref_ptr<osgVerse::FeatureCollection> fc = parseShapeData(fileName, shp);
    SHPClose(shp); return fc.get();
}

virtual ReadResult readNode(const std::string& path, const Options* options) const
{
    std::string ext; std::string fileName = getRealFileName(path, ext);
    if (fileName.empty()) return ReadResult::FILE_NOT_HANDLED;

    SHPHandle shp = SHPOpen(fileName.c_str(), "rb");
    if (shp == NULL) return ReadResult::FILE_NOT_HANDLED;
    osg::ref_ptr<osgVerse::FeatureCollection> fc = parseShapeData(fileName, shp); SHPClose(shp);

    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
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
    geode->addDrawable(geom.get()); return geode.get();
}

virtual ReadResult readImage(const std::string& path, const Options* options) const
{
    std::string ext; std::string fileName = getRealFileName(path, ext);
    if (fileName.empty()) return ReadResult::FILE_NOT_HANDLED;

    SHPHandle shp = SHPOpen(fileName.c_str(), "rb");
    if (shp == NULL) return ReadResult::FILE_NOT_HANDLED;

    osg::ref_ptr<osgVerse::FeatureCollection> fc = parseShapeData(fileName, shp); SHPClose(shp);
    std::string wStr = options ? options->getPluginStringData("ImageWidth") : "512";
    std::string hStr = options ? options->getPluginStringData("ImageHeight") : "512";
    int w = atoi(wStr.c_str()), h = atoi(hStr.c_str()); if (w < 1) w = 512; if (h < 1) h = 512;

    osg::ref_ptr<osgVerse::Drawer2D> drawer = new osgVerse::Drawer2D;
    if (options)
    {
        int toInc = atoi(options->getPluginStringData("IncludeFeatures").c_str());
        if (toInc > 0) drawer->setUserData(fc.get());
    }
    drawer->allocateImage(w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    drawer->setPixelBufferObject(new osg::PixelBufferObject(drawer.get()));
    drawer->start(false); drawer->fillBackground(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));

    const osg::BoundingBox& bb = fc->bound;
    osg::Vec2 off(-bb.xMin(), -bb.yMin()),
        sc((float)w / (bb.xMax() - bb.xMin()), (float)h / (bb.yMax() - bb.yMin()));
    osgVerse::DrawerStyleData fillStyle(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f), true);
    for (size_t i = 0; i < fc->features.size(); ++i)
    {
        osgVerse::Feature* feature = fc->features[i];
        osgVerse::drawFeatureToImage(*feature, drawer.get(), off, sc, &fillStyle);
    }
    drawer->finish(); return drawer.get();
}

protected:
    osgVerse::FeatureCollection* parseShapeData(const std::string& fileName, SHPHandle shp) const
    {
        std::string dbfFile(osgDB::getNameLessExtension(fileName) + ".dbf");
        DBFHandle hDBF = DBFOpen(dbfFile.c_str(), "rb");

        int numEntities = 0, numShapeType = 0; double minBound[4], maxBound[4];
        int numFields = (hDBF != NULL) ? DBFGetFieldCount(hDBF) : 0,
            numRecords = (hDBF != NULL) ? DBFGetRecordCount(hDBF) : 0;
        SHPGetInfo(shp, &numEntities, &numShapeType, minBound, maxBound);

        osg::ref_ptr<osgVerse::FeatureCollection> fc = new osgVerse::FeatureCollection;
        for (int i = 0; i < numEntities; ++i)
        {
            SHPObject* shape = SHPReadObject(shp, i);
            if (shape == NULL) continue;

            osg::ref_ptr<osgVerse::Feature> feature = new osgVerse::Feature;
            if (i < numRecords)
            {
                for (int f = 0; f < numFields; ++f)
                {
                    char field[256] = ""; int width = 0, dec = 0;
                    switch (DBFGetFieldInfo(hDBF, f, field, &width, &dec))
                    {
                    case FTInteger:
                        feature->setUserValue(field, DBFReadIntegerAttribute(hDBF, i, f)); break;
                    case FTDouble:
                        feature->setUserValue(field, DBFReadDoubleAttribute(hDBF, i, f)); break;
                    case FTString:
                        {
                            const char* value = DBFReadStringAttribute(hDBF, i, f);
                            feature->setUserValue(field, std::string(value ? value : "")); break;
                        }
                    default:
                        OSG_NOTICE << "[ReaderWriterSHP] Unknown field: " << field << "\n"; break;
                    }
                }
            }

            switch (shape->nSHPType)
            {
            case SHPT_POINT: case SHPT_MULTIPOINT: feature->setType(GL_POINTS); break;
            case SHPT_ARC: feature->setType(GL_LINE_STRIP); break;
            case SHPT_POLYGON: feature->setType(GL_POLYGON); break;
            default: OSG_NOTICE << "[ReaderWriterSHP] Unknown type: " << shape->nSHPType << "\n"; break;
            }

            std::vector<osg::Vec3> vList;
            for (int v = 0; v < shape->nVertices; ++v)
            {
                osg::Vec3 pt(shape->padfX[v], shape->padfY[v], 0.0f);
                if (shape->padfZ) pt[2] = shape->padfZ[v]; vList.push_back(pt);
            }

            int numParts = shape->nParts;
            if (numParts > 0)
            {
                for (int p = 0; p < numParts; ++p)
                {
                    int start = shape->panPartStart[p],
                        end = ((p == numParts - 1) ? -1 : shape->panPartStart[p + 1]);
                    feature->addPoints(new osg::Vec3Array(
                        vList.begin() + start, end > 0 ? vList.begin() + end : vList.end()));
                }
            }
            else
                feature->addPoints(new osg::Vec3Array(vList.begin(), vList.end()));
            fc->push_back(feature.get()); SHPDestroyObject(shape);
        }

        if (hDBF != NULL) DBFClose(hDBF);
        return fc.release();
    }

    std::string getRealFileName(const std::string& path, std::string& ext) const
    {
        std::string fileName(path); ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return "";

        bool usePseudo = (ext == "verse_shp");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }
        return fileName;
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_shp, ReaderWriterSHP)
