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
        supportsOption("DisabledPBR", "Use PBR materials or not");
    }

    virtual const char* className() const
    {
        return "[osgVerse] FBX scene reader";
    }

    virtual ReadResult readNode(const std::string& path, const osgDB::Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        if (fileName.empty()) return ReadResult::FILE_NOT_HANDLED;
        int noPBR = options ? atoi(options->getPluginStringData("DisabledPBR").c_str()) : 0;
        return osgVerse::loadFbx(fileName, noPBR == 0).get();
    }

    virtual ReadResult readNode(std::istream& fin, const osgDB::Options* options) const
    {
        std::string dir = "";
        if (options) dir = options->getPluginStringData("Directory");
        if (dir.empty() && options && !options->getDatabasePathList().empty())
            dir = options->getDatabasePathList().front();

        int noPBR = options ? atoi(options->getPluginStringData("DisabledPBR").c_str()) : 0;
        return osgVerse::loadFbx2(fin, dir, noPBR == 0).get();
    }

protected:
    std::string getRealFileName(const std::string& path, std::string& ext) const
    {
        std::string fileName(path); ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return "";

        bool usePseudo = (ext == "verse_fbx");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }
        return fileName;
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_fbx, ReaderWriterFBX)
