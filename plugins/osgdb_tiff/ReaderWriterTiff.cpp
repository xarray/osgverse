#include <osg/io_utils>
#include <osg/Version>
#include <osg/Image>
#include <osg/ImageSequence>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <tiffio.h>

class ReaderWriterTiff : public osgDB::ReaderWriter
{
public:
    ReaderWriterTiff()
    {
        supportsExtension("verse_tiff", "osgVerse pseudo-loader");
        supportsExtension("tiff", "Tiff image format");
        supportsExtension("tif", "Tiff image format");
    }

    virtual const char* className() const
    {
        return "[osgVerse] Tiff format reader with 3D texture support";
    }

    virtual ReadResult readImage(const std::string& path, const Options* options) const
    {
        std::string fileName(path);
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

        bool usePseudo = (ext == "verse_image");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getLowerCaseFileExtension(fileName);
        }

        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        return readImage(in, options);
    }

    virtual WriteResult writeImage(const osg::Image& image, const std::string& path,
                                   const Options* options) const
    {
        std::string fileName(path);
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return WriteResult::FILE_NOT_HANDLED;

        bool usePseudo = (ext == "verse_image");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getLowerCaseFileExtension(fileName);
        }

        std::ofstream out(fileName, std::ios::out | std::ios::binary);
        // TODO
        return WriteResult::NOT_IMPLEMENTED;
    }

    virtual ReadResult readImage(std::istream& fin, const Options* options) const
    {
        // TODO
        return NULL;
    }

protected:
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_tiff, ReaderWriterTiff)
