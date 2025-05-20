#include <osg/io_utils>
#include <osg/ValueObject>
#include <osg/TriangleIndexFunctor>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgUtil/Tessellator>
#include "spz/load-spz.h"

class ReaderWriter3DGS : public osgDB::ReaderWriter
{
public:
    ReaderWriter3DGS()
    {
        supportsExtension("verse_3dgs", "osgVerse pseudo-loader");
        supportsExtension("ply", "PLY point cloud file");
        supportsExtension("splat", "Gaussian splat data file");
        supportsExtension("ksplat", "Mark Kellogg's splat file");
        supportsExtension("spz", "NianticLabs' splat file");
    }

    virtual const char* className() const
    {
        return "[osgVerse] 3D Gaussian Scattering data format reader";
    }

    virtual ReadResult readNode(const std::string& path, const Options* options) const
    {
        std::string fileName(path);
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

        bool usePseudo = (ext == "verse_3dgs");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getLowerCaseFileExtension(fileName);
        }

        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_FOUND;

        osg::ref_ptr<Options> localOptions = NULL;
        if (options) localOptions = options->cloneOptions();
        else localOptions = new osgDB::Options();

        localOptions->setPluginStringData("prefix", osgDB::getFilePath(path));
        localOptions->setPluginStringData("extension", ext);
        return readNode(in, localOptions.get());
    }

    virtual ReadResult readNode(std::istream& fin, const Options* options) const
    {
        spz::UnpackOptions unpackOpt;  // TODO: convert coordinates
        if (options)
        {
            std::string prefix = options->getPluginStringData("prefix");
            std::string ext = options->getPluginStringData("extension");
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            spz::GaussianCloud cloud;
            if (ext == "ply")
            {
                cloud = spz::loadSplatFromPly(fin, prefix, unpackOpt);
                // TODO
            }
            else
            {
                std::string buffer((std::istreambuf_iterator<char>(fin)),
                                   std::istreambuf_iterator<char>());
                if (buffer.empty()) return ReadResult::ERROR_IN_READING_FILE;

                if (ext == "spz")
                {
                    std::vector<uint8_t> dataSrc(buffer.begin(), buffer.end());
                    cloud = spz::loadSpz(dataSrc, unpackOpt);
                    // TODO
                }
                else if (ext == "splat")
                {
                    // TODO
                }
                else if (ext == "ksplat")
                {
                    // TODO
                }
            }
        }
        return NULL;
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_3dgs, ReaderWriter3DGS)
