#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <readerwriter/LoadSceneFBX.h>

class ReaderWriterFBX : public osgDB::ReaderWriter
{
public:
    ReaderWriterFBX()
    {
        supportsExtension("verse_fbx", "osgVerse pseudo-loader");
        supportsExtension("fbx", "FBX scene file");
        supportsOption("Directory", "Setting the working directory");
    }

    virtual const char* className() const
    {
        return "[osgVerse] FBX scene reader";
    }

    virtual ReadResult readNode(const std::string& path, const osgDB::Options* options) const
    {
        std::string fileName(path);
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

        bool usePseudo = (ext == "verse_fbx");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }
        return osgVerse::loadFbx(fileName).get();
    }

    virtual ReadResult readNode(std::istream& fin, const osgDB::Options* options) const
    {
        std::string dir = ".";
        if (options) dir = options->getPluginStringData("Directory");
        return osgVerse::loadFbx2(fin, dir).get();
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_fbx, ReaderWriterFBX)
