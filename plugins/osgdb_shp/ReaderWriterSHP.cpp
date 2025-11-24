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
#include <shapelib/shapefil.h>

class ReaderWriterSHP : public osgDB::ReaderWriter
{
public:
    ReaderWriterSHP()
    {
        supportsExtension("verse_shp", "osgVerse pseudo-loader");
        supportsExtension("shp", "Shapefile data file");
    }

    virtual const char* className() const
    {
        return "[osgVerse] ESRI Shapefile data format reader";
    }

    virtual ReadResult readNode(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        SHPHandle shp = SHPOpen(fileName.c_str(), "rb");
        if (shp == NULL) return ReadResult::FILE_NOT_HANDLED;

        int numEntities = 0, numShapeType = 0;
        double minBound[4], maxBound[4];
        SHPGetInfo(shp, &numEntities, &numShapeType, minBound, maxBound);
        for (int i = 0; i < numEntities; ++i)
        {
            SHPObject* shape = SHPReadObject(shp, i);
            if (shape == NULL) continue;

            switch (shape->nSHPType)
            {
            case SHPT_POINT:
                break;
            case SHPT_MULTIPOINT:
                break;
            case SHPT_ARC:
                break;
            case SHPT_POLYGON:
                break;
            default: break;
            }
            SHPDestroyObject(shape);
        }
        SHPClose(shp);

        // TODO
        return ReadResult::FILE_NOT_HANDLED;
    }

protected:
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
