#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <algorithm>
#include <readerwriter/LoadSceneFBX.h>
#include <readerwriter/SaveSceneFBX.h>

class ReaderWriterFBX : public osgDB::ReaderWriter
{
public:
    ReaderWriterFBX()
    {
        supportsExtension("verse_fbx", "osgVerse pseudo-loader");
        supportsExtension("fbx", "FBX scene file");
        supportsOption("Directory", "Setting the working directory");
        supportsOption("DisabledPBR", "Use PBR materials or not");
        supportsOption("ForcedPBR", "Force using PBR materials or not");
        supportsOption("WriteImageHint=<hint>", "Export option: Hint of writing image to stream: "
                       "<IncludeData> writes embedded image data into the FBX (default); "
                       "<UseExternal> writes separate image files next to the FBX file");
        supportsOption("Mode", "Export option: Set to 'binary' (default) or 'ascii' to choose FBX flavour");
    }

    virtual const char* className() const
    {
        return "[osgVerse] FBX scene reader / writer";
    }

    virtual ReadResult readNode(const std::string& path, const osgDB::Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        if (fileName.empty()) return ReadResult::FILE_NOT_HANDLED;
        int noPBR = options ? atoi(options->getPluginStringData("DisabledPBR").c_str()) : 0;
        int forcedPBR = options ? atoi(options->getPluginStringData("ForcedPBR").c_str()) : 0;
        return osgVerse::loadFbx(fileName, (noPBR == 1) ? 0 : (forcedPBR == 0 ? 1 : 2)).get();
    }

    virtual ReadResult readNode(std::istream& fin, const osgDB::Options* options) const
    {
        std::string dir = "";
        if (options) dir = options->getPluginStringData("Directory");
        if (dir.empty() && options && !options->getDatabasePathList().empty())
            dir = options->getDatabasePathList().front();

        int noPBR = options ? atoi(options->getPluginStringData("DisabledPBR").c_str()) : 0;
        int forcedPBR = options ? atoi(options->getPluginStringData("ForcedPBR").c_str()) : 0;
        return osgVerse::loadFbx2(fin, dir, (noPBR == 1) ? 0 : (forcedPBR == 0 ? 1 : 2)).get();
    }

    virtual WriteResult writeNode(const osg::Node& node, const std::string& path,
                                  const osgDB::Options* options) const
    {
        std::string ext, imageHint; std::string fileName = getRealFileName(path, ext);
        if (fileName.empty()) return WriteResult::FILE_NOT_HANDLED;
        if (options) imageHint = options->getPluginStringData("WriteImageHint");

        bool success = osgVerse::saveFbx(node, fileName, imageHint, isBinaryOutput(options));
        return success ? WriteResult::FILE_SAVED : WriteResult::ERROR_IN_WRITING_FILE;
    }

    virtual WriteResult writeNode(const osg::Node& node, std::ostream& fout,
                                  const osgDB::Options* options) const
    {
        std::string dir = "", imageHint;
        if (options)
        {
            dir = options->getPluginStringData("Directory");
            if (dir.empty()) dir = options->getPluginStringData("prefix");
            imageHint = options->getPluginStringData("WriteImageHint");
        }

        if (dir.empty() && options && !options->getDatabasePathList().empty())
            dir = options->getDatabasePathList().front();
        bool success = osgVerse::saveFbx2(node, fout, dir, imageHint, isBinaryOutput(options));
        return success ? WriteResult::FILE_SAVED : WriteResult::ERROR_IN_WRITING_FILE;
    }

protected:
    // Binary FBX (version 7.4) is the default output flavour
    bool isBinaryOutput(const osgDB::Options* options) const
    {
        std::string mode = options ? options->getPluginStringData("Mode") : "";
        std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
        return (mode != "ascii");
    }

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
