#include <osg/io_utils>
#include <osg/Version>
#include <osg/Image>
#include <osg/ImageSequence>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>

#define STB_IMAGE_STATIC
#define STB_IMAGE_WRITE_STATIC
#define STBI_WINDOWS_UTF8
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "3rdparty/stb/stb_image.h"
#include "3rdparty/stb/stb_image_write.h"

static const int s_rawHeader1 = 0xF1259E55;
static const int s_rawHeader2 = 0x42F2E926;

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
        supportsExtension("rseq", "Customized raw image file");
    }

    virtual const char* className() const
    {
        return "[osgVerse] Common image format reader";
    }

    virtual ReadResult readImage(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_FOUND;
        return (ext == "rseq") ? readRaw(in, options) : readImage(in, options);
    }

    virtual WriteResult writeImage(const osg::Image& image, const std::string& path,
                                   const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        std::ofstream out(fileName, std::ios::out | std::ios::binary);
        if (ext == "rseq") return writeRaw(out, image, options);
        // TODO
        return WriteResult::NOT_IMPLEMENTED;
    }

    virtual ReadResult readImage(std::istream& fin, const Options* options) const
    {
        if (options)
        {
            std::string filename = options->getPluginStringData("STREAM_FILENAME");
            std::string ext = osgDB::getLowerCaseFileExtension(filename);
            if (ext == "rseq") return readRaw(fin, options);
        }

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

protected:
    std::string getRealFileName(const std::string& path, std::string& ext) const
    {
        std::string fileName(path); ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return fileName;

        bool usePseudo = (ext == "verse_image");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }
        return fileName;
    }

    ReadResult readRaw(std::istream& fin, const Options* options) const
    {
        int header1 = 0, header2 = 0; long long imgCount = 0;
        fin.read((char*)&header1, sizeof(int));
        fin.read((char*)&header2, sizeof(int));
        fin.read((char*)&imgCount, sizeof(long long));
        if (header1 != s_rawHeader1 || header2 != s_rawHeader2)
        {
            OSG_WARN << "[ReaderWriterImage] header mismatched." << std::endl;
            return ReadResult::ERROR_IN_READING_FILE;
        }

        std::vector<osg::ref_ptr<osg::Image>> images;
        for (long long i = 0; i < imgCount; ++i)
        {
            long long imgW = 0, imgH = 0, imgSize = 0;
            fin.read((char*)&imgSize, sizeof(long long));
            if (imgSize == 0) continue;

            GLenum internalFmt = 0, pixelFmt = 0, dataType = 0;
            fin.read((char*)&imgW, sizeof(long long));
            fin.read((char*)&imgH, sizeof(long long));
            fin.read((char*)&internalFmt, sizeof(GLenum));
            fin.read((char*)&pixelFmt, sizeof(GLenum));
            fin.read((char*)&dataType, sizeof(GLenum));

            osg::Image* image = new osg::Image;
            image->allocateImage(imgW, imgH, 1, pixelFmt, dataType);
            image->setInternalTextureFormat(internalFmt);
            if (image->getTotalSizeInBytes() != imgSize)
            {
                OSG_WARN << "[ReaderWriterImage] Raw image size mismatched: "
                         << "(" << imgW << " x " << imgH << ") Current size "
                         << image->getTotalSizeInBytes() << " != " << imgSize << std::endl;
                continue;
            }

            char* ptr = (char*)image->data();
            fin.read(ptr, imgSize); images.push_back(image);
        }

        if (images.empty()) return ReadResult::FILE_LOADED;
        else if (images.size() == 1) return images.front().get();

        osg::ref_ptr<osg::ImageSequence> seq = new osg::ImageSequence;
        for (size_t i = 0; i < images.size(); ++i) seq->addImage(images[i]);
        return seq.get();
    }

    WriteResult writeRaw(std::ostream& fout, const osg::Image& image,
                         const Options* options) const
    {
        const osg::ImageSequence* seq = dynamic_cast<const osg::ImageSequence*>(&image);
#if OSG_VERSION_GREATER_THAN(3, 2, 0)
        osg::ImageSequence::ImageDataList images;
        if (!seq)
        {
            osg::ImageSequence::ImageData imgData;
            imgData._image = const_cast<osg::Image*>(&image);
            images.push_back(imgData);
        }
        else
            images = seq->getImageDataList();
#else
        std::vector<osg::ref_ptr<osg::Image>> images;
        if (!seq) images.push_back(const_cast<osg::Image*>(&image));
        else images = seq->getImages();
#endif

        long long imgCount = (long long)images.size();
        fout.write((char*)&s_rawHeader1, sizeof(int));
        fout.write((char*)&s_rawHeader2, sizeof(int));
        fout.write((char*)&imgCount, sizeof(long long));

        long long imgW = 0, imgH = 0, imgSize = 0;
        for (long long i = 0; i < imgCount; ++i)
        {
#if OSG_VERSION_GREATER_THAN(3, 2, 0)
            osg::ImageSequence::ImageData& imgData = images[i];
            osg::Image* img = imgData._image.get();
#else
            osg::Image* img = images[i].get();
#endif
            if (!img)
            {
                imgSize = 0;
                fout.write((char*)&imgSize, sizeof(long long));
                continue;
            }

            GLenum internalFmt = img->getInternalTextureFormat();
            GLenum pixelFmt = img->getPixelFormat();
            GLenum dataType = img->getDataType();
            imgW = img->s(); imgH = img->t();
            imgSize = img->getTotalSizeInBytes();
            fout.write((char*)&imgSize, sizeof(long long));
            fout.write((char*)&imgW, sizeof(long long));
            fout.write((char*)&imgH, sizeof(long long));
            fout.write((char*)&internalFmt, sizeof(GLenum));
            fout.write((char*)&pixelFmt, sizeof(GLenum));
            fout.write((char*)&dataType, sizeof(GLenum));
            fout.write((char*)img->data(), imgSize);
        }
        return WriteResult::FILE_SAVED;
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_image, ReaderWriterImage)
