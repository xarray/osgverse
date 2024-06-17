#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>

#include <webp/encode.h>
#include <webp/decode.h>

class ReaderWriterWebP : public osgDB::ReaderWriter
{
public:
    ReaderWriterWebP()
    {
        supportsExtension("verse_webp", "osgVerse pseudo-loader");
        supportsExtension("webp", "WEBP image file");
    }

    virtual const char* className() const
    {
        return "[osgVerse] WEBP image format reader";
    }

    virtual ReadResult readImage(const std::string& path, const Options* options) const
    {
        std::string fileName(path);
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

        bool usePseudo = (ext == "verse_webp");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getLowerCaseFileExtension(fileName);
        }

        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_FOUND;
        return readImage(in, options);
    }

    virtual ReadResult readImage(std::istream& fin, const Options* options) const
    {
        std::string buffer((std::istreambuf_iterator<char>(fin)),
                           std::istreambuf_iterator<char>());
        if (buffer.empty()) return ReadResult::ERROR_IN_READING_FILE;

        WebPConfig config; int width = 0, height = 0;
        WebPConfigInit(&config);

        uint8_t* rgba = WebPDecodeRGBA((const uint8_t*)buffer.data(), buffer.size(), &width, &height);
        if (!rgba) return ReadResult::ERROR_IN_READING_FILE;

        osg::ref_ptr<osg::Image> image = new osg::Image;
        image->allocateImage(width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE);
        image->setInternalTextureFormat(GL_RGBA8);
        memcpy(image->data(), rgba, image->getTotalSizeInBytes());
        return image;
    }

protected:
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_webp, ReaderWriterWebP)
