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
        supportsOption("WriteQuality=<q>", "Quality option: 0-100");
        supportsExtension("verse_webp", "osgVerse pseudo-loader");
        supportsExtension("webp", "WEBP image file");
    }

    virtual const char* className() const
    {
        return "[osgVerse] WEBP image format reader";
    }

    virtual ReadResult readImage(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_HANDLED;
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
        WebPFree(rgba);
        image->flipVertical();
        return image.get();
    }

    virtual WriteResult writeImage(const osg::Image& image, const std::string& path,
                                   const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        std::ofstream out(fileName, std::ios::out | std::ios::binary);
        if (!out) return WriteResult::ERROR_IN_WRITING_FILE;
        osg::ref_ptr<osg::Image> tmp_img = new osg::Image(image);
        tmp_img->flipVertical();
        return writeImage(*tmp_img, out, options);
    }

    virtual WriteResult writeImage(const osg::Image& image, std::ostream& out,
                                   const Options* options) const
    {
        std::string quality = options ? options->getPluginStringData("WriteQuality") : "";
        int q = quality.empty() ? 80 : atoi(quality.c_str());

        uint8_t* result = NULL; size_t size = 0;
        switch (image.getInternalTextureFormat())
        {
        case GL_RGBA: case GL_RGBA8:
            size = WebPEncodeRGBA(image.data(), image.s(), image.t(),
                                  image.s() * 4, q, &result); break;
        case GL_RGB: case GL_RGB8:
            size = WebPEncodeRGB(image.data(), image.s(), image.t(),
                                  image.s() * 3, q, &result); break;
        case GL_BGR:
            size = WebPEncodeBGR(image.data(), image.s(), image.t(),
                image.s() * 3, q, &result); break;
        default:
            OSG_NOTICE << "[ReaderWriterWebP] Unsupported image type: " << std::hex
                       << image.getInternalTextureFormat() << std::endl;
            return WriteResult::NOT_IMPLEMENTED;
        }
        
        if (!result || !size) return WriteResult::ERROR_IN_WRITING_FILE;
        else { out.write((char*)result, size); WebPFree(result); }
        return WriteResult::FILE_SAVED;
    }

protected:
    std::string getRealFileName(const std::string& path, std::string& ext) const
    {
        std::string fileName(path); ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return "";

        bool usePseudo = (ext == "verse_webp");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }
        return fileName;
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_webp, ReaderWriterWebP)
