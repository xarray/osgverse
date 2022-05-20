#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <readerwriter/LoadSceneGLTF.h>

class ReaderWriterGLTF : public osgDB::ReaderWriter
{
public:
    ReaderWriterGLTF()
    {
        supportsExtension("gltf", "GLTF ascii scene file");
        supportsExtension("glb", "GLTF binary scene file");
        supportsOption("Directory", "Setting the working directory");
        supportsOption("Mode", "Set to 'ascii/binary' to read specific GLTF data");
    }

    virtual const char* className() const
    {
        return "GLTF scene Reader";
    }

    virtual ReadResult readNode(const std::string& path, const osgDB::Options* options) const
    {
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;
        if (ext == "glb") return osgVerse::loadGltf(path, true);
        else return osgVerse::loadGltf(path, false);
    }

    virtual ReadResult readNode(std::istream& fin, const osgDB::Options* options) const
    {
        std::string dir = ".", mode; bool isBinary = false;
        if (options)
        {
            dir = options->getPluginStringData("Directory");
            mode = options->getPluginStringData("Mode");
            std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
            if (mode == "binary") isBinary = true;
        }
        osg::ref_ptr<osg::Node> scene = osgVerse::loadGltf2(fin, dir, isBinary);
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(gltf, ReaderWriterGLTF)
