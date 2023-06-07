#include <osg/io_utils>
#include <osg/Image>
#include <osg/Geometry>
#include <osg/Geode>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <openvdb/openvdb.h>

class ReaderWriterVDB : public osgDB::ReaderWriter
{
public:
    ReaderWriterVDB()
    {
        supportsExtension("verse_vdb", "osgVerse pseudo-loader");
        supportsExtension("vdb", "VDB point cloud and texture file");
    }

    virtual const char* className() const
    {
        return "[osgVerse] VDB point cloud and texture reader";
    }

    virtual ReadResult readImage(const std::string& path, const Options* options) const
    {
        std::string fileName(path);
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

        bool usePseudo = (ext == "verse_vdb");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }
        return NULL;
    }

    virtual ReadResult readImage(std::istream& fin, const Options* = NULL) const
    {
        return NULL;
    }

    virtual WriteResult writeImage(const osg::Image& image, const std::string& path,
                                   const Options* options) const
    {
        std::string fileName(path);
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return WriteResult::FILE_NOT_HANDLED;

        bool usePseudo = (ext == "verse_vdb");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }
        return WriteResult::ERROR_IN_WRITING_FILE;
    }

    virtual WriteResult writeImage(const osg::Image& image, std::ostream& fout,
                                   const Options* options) const
    {
        return WriteResult::ERROR_IN_WRITING_FILE;
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_vdb, ReaderWriterVDB)
