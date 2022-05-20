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
        supportsExtension("fbx", "FBX scene file");
        supportsOption("Directory", "Setting the working directory");
    }

    virtual const char* className() const
    {
        return "FBX scene Reader";
    }

    virtual ReadResult readNode(const std::string& path, const osgDB::Options* options) const
    {
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;
        return osgVerse::loadFbx(path);
    }

    virtual ReadResult readNode(std::istream& fin, const osgDB::Options* options) const
    {
        std::string dir = "."; if (options) dir = options->getPluginStringData("Directory");
        osg::ref_ptr<osg::Node> scene = osgVerse::loadFbx2(fin, dir);
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(fbx, ReaderWriterFBX)
