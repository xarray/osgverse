#include <osg/io_utils>
#include <osg/Version>
#include <osg/Image>
#include <osg/ImageSequence>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8
#include <stb/stb_image.h>

class ReaderWriterImage : public osgDB::ReaderWriter
{
public:
    ReaderWriterImage()
    {
        supportsExtension("verse_image", "osgVerse pseudo-loader");
        supportsExtension("jpg", "JPEG image file");
        supportsExtension("jpeg", "JPEG image file");
        supportsExtension("png", "PNG image file");
        supportsExtension("psd", "PSD image file");
        supportsExtension("hdr", "HDR image file");
    }

    virtual const char* className() const
    {
        return "[osgVerse] Common image format reader";
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
            ext = osgDB::getFileExtension(fileName);
        }

        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        return !in ? ReadResult::FILE_NOT_FOUND : readImage(in, options);
    }

    virtual ReadResult readImage(std::istream& fin, const Options* = NULL) const
    {
        std::string buffer((std::istreambuf_iterator<char>(fin)),
                           std::istreambuf_iterator<char>());
        if (buffer.empty()) return ReadResult::ERROR_IN_READING_FILE;

        int x = 0, y = 0, channels = 0;
        stbi_uc* data = stbi_load_from_memory(
            (const unsigned char*)&buffer[0], buffer.size(), &x, &y, &channels, 0);
        
        GLenum format = GL_RGBA;
        switch (channels)
        {
        case 1: format = GL_LUMINANCE; break;
        case 2: format = GL_LUMINANCE_ALPHA; break;
        case 3: format = GL_RGB; break;
        default: break;
        }

        osg::ref_ptr<osg::Image> image = new osg::Image;
        image->setImage(x, y, 1, format, format, GL_UNSIGNED_BYTE,
                        data, osg::Image::USE_MALLOC_FREE);
        image->flipVertical();
        return image.get();
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_image, ReaderWriterImage)
