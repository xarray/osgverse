#include <osg/io_utils>
#include <osg/Version>
#include <osg/Image>
#include <osg/ImageSequence>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>

#include <tiffio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>

static std::string formattedErrorMessage(const char* fmt, va_list ap)
{
    static const int MSG_BUFSIZE = 256;
    static const int MAX_BUFSIZE = 256 * 1024;
    for (int size = MSG_BUFSIZE; size < MAX_BUFSIZE;)
    {
        char * p = new char[size];
        int n = vsnprintf(p, size, fmt, ap);
        if (n >= 0 && n < size)
        { std::string res(p); delete[] p; return res; }

        if (n > 0) size = n+1; else size *= 2;
        delete[] p;
    }
    return std::string(fmt, fmt + MSG_BUFSIZE) + "...";
}

static void tiffError(const char*, const char* fmt, va_list ap)
{ OSG_WARN << "[ReaderWriterTiff] Error: " << formattedErrorMessage(fmt, ap) << std::endl; }

static void tiffWarn(const char*, const char* fmt, va_list ap)
{ OSG_NOTICE << "[ReaderWriterTiff] Warn: " << formattedErrorMessage(fmt, ap) << std::endl; }

static tsize_t tiffStreamReadProc(thandle_t fd, tdata_t buf, tsize_t size)
{
    std::istream *fin = (std::istream*)fd;
    fin->read((char*)buf, size);
    if (fin->bad()) return -1;
    if (fin->gcount() < size) return 0;
    return size;
}

static tsize_t tiffStreamWriteProc(thandle_t, tdata_t, tsize_t)
{
    return 0;
}

static toff_t tiffStreamSeekProc(thandle_t fd, toff_t off, int i)
{
    std::istream *fin = (std::istream*)fd; toff_t ret = 0;
    switch(i)
    {
    case SEEK_SET:
        fin->seekg(off, std::ios::beg); ret = fin->tellg();
        if (fin->bad()) ret = 0; break;
    case SEEK_CUR:
        fin->seekg(off, std::ios::cur); ret = fin->tellg();
        if (fin->bad()) ret = 0; break;
    case SEEK_END:
        fin->seekg(off, std::ios::end); ret = fin->tellg();
        if (fin->bad()) ret = 0; break;
    default: break;
    }
    return ret;
}

static int tiffStreamCloseProc(thandle_t)
{
    return 0;
}

static toff_t tiffStreamSizeProc(thandle_t fd)
{
    std::istream *fin = (std::istream*)fd;
    std::streampos curPos = fin->tellg();
    fin->seekg(0, std::ios::end);

    toff_t size = fin->tellg();
    fin->seekg(curPos, std::ios::beg);
    return size;
}

static int tiffStreamMapProc(thandle_t, tdata_t*, toff_t*)
{
    return 0;
}

static void tiffStreamUnmapProc(thandle_t, tdata_t, toff_t)
{
}

static void invertRow(unsigned char* ptr, unsigned char* data, int n, int invert, uint16_t bitspersample)
{
    if (bitspersample == 8)
    {
        while (n--)
        {
            if (invert) *ptr++ = 255 - *data++;
            else *ptr++ = *data++;
        }
    }
    else if (bitspersample == 16)
    {
        unsigned short *ptr1 = (unsigned short*)ptr;
        unsigned short *data1 = (unsigned short*)data;
        while (n--)
        {
            if (invert) *ptr1++ = 65535 - *data1++;
            else *ptr1++ = *data1++;
        }
    }
    else if (bitspersample == 32)
    {
        float *ptr1 = (float*)ptr, *data1 = (float*)data;
        while (n--)
        {
            if (invert) *ptr1++ = 1.0 - *data1++;
            else *ptr1++ = *data1++;
        }
    }
}

static void remapRow(unsigned char* ptr, unsigned char* data, int n,
                     unsigned short* rmap, unsigned short* gmap, unsigned short* bmap)
{
    unsigned int ix = 0;
    while (n--)
    {
        ix = *data++;
        *ptr++ = (unsigned char) rmap[ix];
        *ptr++ = (unsigned char) gmap[ix];
        *ptr++ = (unsigned char) bmap[ix];
    }
}

static void interleaveRow(unsigned char *ptr, unsigned char *red, unsigned char *green, unsigned char *blue,
                          int n, int numSamples, uint16_t bitspersample)
{
    if (bitspersample == 8)
    {
        while (n--)
        {
            *ptr++ = *red++; *ptr++ = *green++; *ptr++ = *blue++;
            if (numSamples==4) *ptr++ = 255;
        }
    }
    else if (bitspersample == 16)
    {
        unsigned short *ptr1 = (unsigned short *)ptr;
        unsigned short *red1 = (unsigned short *)red;
        unsigned short *green1 = (unsigned short *)green;
        unsigned short *blue1 = (unsigned short *)blue;
        while (n--)
        {
            *ptr1++ = *red1++; *ptr1++ = *green1++; *ptr1++ = *blue1++;
            if (numSamples==4) *ptr1++ = 65535;
        }
    }
    else if (bitspersample == 32)
    {
        float *ptr1 = (float *)ptr; float *red1 = (float *)red;
        float *green1 = (float *)green; float *blue1 = (float *)blue;
        while (n--)
        {
            *ptr1++ = *red1++; *ptr1++ = *green1++; *ptr1++ = *blue1++;
            if (numSamples==4) *ptr1++ = 1.0f;
        }
    }
}

static void interleaveRow(unsigned char* ptr, unsigned char* red, unsigned char* green, unsigned char* blue,
                          unsigned char* alpha, int n, int numSamples, uint16_t bitspersample)
{
    if (bitspersample == 8)
    {
        while (n--)
        {
            *ptr++ = *red++; *ptr++ = *green++; *ptr++ = *blue++;
            if (numSamples==4) *ptr++ = *alpha++;
        }
    }
    else if (bitspersample == 16)
    {
        unsigned short *ptr1 = (unsigned short *)ptr;
        unsigned short *red1 = (unsigned short *)red;
        unsigned short *green1 = (unsigned short *)green;
        unsigned short *blue1 = (unsigned short *)blue;
        unsigned short *alpha1 = (unsigned short *)alpha;
        while (n--)
        {
            *ptr1++ = *red1++; *ptr1++ = *green1++; *ptr1++ = *blue1++;
            if (numSamples==4) *ptr1++ = *alpha1++;
        }
    }
    else if (bitspersample == 32)
    {
        float *ptr1 = (float *)ptr;
        float *red1 = (float *)red; float *green1 = (float *)green;
        float *blue1 = (float *)blue; float *alpha1 = (float *)alpha;
        while (n--)
        {
            *ptr1++ = *red1++; *ptr1++ = *green1++; *ptr1++ = *blue1++;
            if (numSamples==4) *ptr1++ = *alpha1++;
        }
    }
}

static int checkColormap(int n, uint16_t* r, uint16_t* g, uint16_t* b)
{
    while (n-- > 0)
    { if (*r++ >= 256 || *g++ >= 256 || *b++ >= 256) return (16); }
    return (8);  /* Assuming 8-bit colormap */
}

static unsigned int computeInternalFormat(unsigned int pixelFormat, unsigned int dataType)
{
    switch (pixelFormat)
    {
    case GL_LUMINANCE:
        switch (dataType)
        {
        case GL_UNSIGNED_BYTE: return GL_LUMINANCE8;
        case GL_UNSIGNED_SHORT: return GL_LUMINANCE16;
        case GL_FLOAT: return GL_LUMINANCE32F_ARB;
        }
        break;
    case GL_LUMINANCE_ALPHA:
        switch (dataType)
        {
        case GL_UNSIGNED_BYTE: return GL_LUMINANCE_ALPHA8UI_EXT;
        case GL_UNSIGNED_SHORT: return GL_LUMINANCE_ALPHA16UI_EXT;
        case GL_FLOAT: return GL_LUMINANCE_ALPHA32F_ARB;
        }
        break;
    case GL_RGB:
        switch (dataType)
        {
        case GL_UNSIGNED_BYTE: return GL_RGB8;
        case GL_UNSIGNED_SHORT: return GL_RGB16;
        case GL_FLOAT: return GL_RGB32F_ARB;
        }
        break;
    case GL_RGBA:
        switch (dataType)
        {
        case GL_UNSIGNED_BYTE: return GL_RGBA8;
        case GL_UNSIGNED_SHORT: return GL_RGBA16;
        case GL_FLOAT: return GL_RGBA32F_ARB;
        }
        break;
    }
    return -1;
}

#define CVT(x)      (((x) * 255L) / ((1L << 16) - 1))
#define PACK(a, b)  ((a) << 8 | (b))

static osg::ImageSequence* tiffLoad(std::istream& fin, const osgDB::Options* options)
{
    TIFFSetErrorHandler(tiffError);
    TIFFSetWarningHandler(tiffWarn);
    TIFF* in = TIFFClientOpen("inputstream", "r", (thandle_t)&fin,
                              tiffStreamReadProc, tiffStreamWriteProc,
                              tiffStreamSeekProc, tiffStreamCloseProc,
                              tiffStreamSizeProc, tiffStreamMapProc, tiffStreamUnmapProc);
    if (in == NULL) { OSG_WARN << "[ReaderWriterTiff] Unable to open stream" << std::endl; return NULL; }

    uint16_t photometric = 0;
    if (TIFFGetField(in, TIFFTAG_PHOTOMETRIC, &photometric) == 1)
    {
        if (photometric != PHOTOMETRIC_RGB && photometric != PHOTOMETRIC_PALETTE &&
            photometric != PHOTOMETRIC_MINISWHITE && photometric != PHOTOMETRIC_MINISBLACK)
        {
            OSG_WARN << "[ReaderWriterTiff] Photometric type " << photometric << " not handled" << std::endl;
            TIFFClose(in); return NULL;
        }
    }
    else
    {
        OSG_WARN << "[ReaderWriterTiff] Unable to get photometric type" << std::endl;
        TIFFClose(in); return NULL;
    }

    uint16_t samplesperpixel = 0;
    if (TIFFGetField(in, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel) == 1)
    {
        if (samplesperpixel != 1 && samplesperpixel != 2 &&
            samplesperpixel != 3 && samplesperpixel != 4)
        {
            OSG_WARN << "[ReaderWriterTiff] Bad samples per pixel: " << samplesperpixel << std::endl;
            TIFFClose(in); return NULL;
        }
    }
    else
    {
        OSG_WARN << "[ReaderWriterTiff] Unable to get samples per pixel" << std::endl;
        TIFFClose(in); return NULL;
    }

    uint16_t bitspersample = 0;
    if (TIFFGetField(in, TIFFTAG_BITSPERSAMPLE, &bitspersample) == 1)
    {
        if (bitspersample != 8 && bitspersample != 16 && bitspersample != 32)
        {
            OSG_WARN << "[ReaderWriterTiff] Can only handle 8, 16 and 32 bit samples" << std::endl;
            TIFFClose(in); return NULL;
        }
    }
    else
    {
        OSG_WARN << "[ReaderWriterTiff] Unable to get bits per sample" << std::endl;
        TIFFClose(in); return NULL;
    }

    uint32_t w = 0, h = 0, d = 1; uint16_t config = 0, dataType = 0;
    if (TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &w) != 1 || TIFFGetField(in, TIFFTAG_IMAGELENGTH, &h) != 1 ||
        TIFFGetField(in, TIFFTAG_PLANARCONFIG, &config) != 1)
    {
        OSG_WARN << "[ReaderWriterTiff] Unable to get width / height / depth parameters" << std::endl;
        TIFFClose(in); return NULL;
    }

    // if it has a palette, data returned is 3 byte rgb
    int format = (photometric == PHOTOMETRIC_PALETTE) ? 3 : (samplesperpixel * bitspersample / 8);
    int bytespersample = bitspersample / 8;
    int bytesperpixel = bytespersample * samplesperpixel;
    TIFFGetField(in, TIFFTAG_DATATYPE, &dataType);
    TIFFGetField(in, TIFFTAG_IMAGEDEPTH, &d);

    osg::ref_ptr<osg::ImageSequence> seq = new osg::ImageSequence;
    if (d > 1)
    {
        // TODO...
        OSG_WARN << "[ReaderWriterTiff] Unsupported dimension" << std::endl;
    }
    else
    {
        int dirCount = 0, imgSize = w * h * format;
        do
        {
            unsigned char* inBuffer = NULL; bool hasError = false;
            unsigned char* buffer = new unsigned char[imgSize];
            memset(buffer, 0, imgSize); dirCount++;

            unsigned char* currPtr = buffer + (h - 1) * w * format;
            uint16_t *red = NULL, *green = NULL, *blue = NULL;
            size_t rowSize = TIFFScanlineSize(in);
            switch (PACK(photometric, config))
            {
            case PACK(PHOTOMETRIC_MINISWHITE, PLANARCONFIG_CONTIG):
            case PACK(PHOTOMETRIC_MINISBLACK, PLANARCONFIG_CONTIG):
            case PACK(PHOTOMETRIC_MINISWHITE, PLANARCONFIG_SEPARATE):
            case PACK(PHOTOMETRIC_MINISBLACK, PLANARCONFIG_SEPARATE):
                inBuffer = new unsigned char[rowSize];
                for (uint32_t row = 0; row < h; row++)
                {
                    if (TIFFReadScanline(in, inBuffer, row, 0) < 0) { hasError = true; break; }
                    invertRow(currPtr, inBuffer, samplesperpixel * w,
                              photometric == PHOTOMETRIC_MINISWHITE, bitspersample);
                    currPtr -= format * w;
                }
                break;

            case PACK(PHOTOMETRIC_PALETTE, PLANARCONFIG_CONTIG):
            case PACK(PHOTOMETRIC_PALETTE, PLANARCONFIG_SEPARATE):
                if (TIFFGetField(in, TIFFTAG_COLORMAP, &red, &green, &blue) != 1)
                { hasError = true; break; }
                else if (!hasError && bitspersample != 32 && checkColormap(1<<bitspersample, red, green, blue) == 16)
                {
                    for (int i = (1 << bitspersample) - 1; i >= 0; --i)
                    { red[i] = CVT(red[i]); green[i] = CVT(green[i]); blue[i] = CVT(blue[i]); }
                }

                inBuffer = new unsigned char[rowSize];
                for (uint32_t row = 0; row < h; row++)
                {
                    if (TIFFReadScanline(in, inBuffer, row, 0) < 0) { hasError = true; break; }
                    remapRow(currPtr, inBuffer, w, red, green, blue); currPtr -= format * w;
                }
                break;

            case PACK(PHOTOMETRIC_RGB, PLANARCONFIG_CONTIG):
                inBuffer = new unsigned char[rowSize];
                for (uint32_t row = 0; row < h; row++)
                {
                    if (TIFFReadScanline(in, inBuffer, row, 0) < 0) { hasError = true; break; }
                    memcpy(currPtr, inBuffer, format * w); currPtr -= format * w;
                }
                break;

            case PACK(PHOTOMETRIC_RGB, PLANARCONFIG_SEPARATE):
                inBuffer = new unsigned char[format * rowSize];
                for (uint32_t row = 0; !hasError && row < h; row++)
                {
                    for (int s = 0; s < format; s++)
                    {
                        if (TIFFReadScanline(in, (tdata_t)(inBuffer + s * rowSize), (uint32_t)row, (tsample_t)s) < 0)
                        { hasError = true; break; }
                    }

                    if (!hasError)
                    {
                        unsigned char *inBuffer2 = inBuffer + rowSize, *inBuffer3 = inBuffer + 2 * rowSize,
                                      *inBuffer4 = inBuffer + 3 * rowSize;
                        if (format == 4) interleaveRow(currPtr, inBuffer, inBuffer2, inBuffer3, inBuffer4, w, format, bitspersample);
                        else if (format == 3) interleaveRow(currPtr, inBuffer, inBuffer2, inBuffer3, w, format, bitspersample);
                        currPtr -= format * w;
                    }
                }
                break;
            default:
                OSG_WARN << "[ReaderWriterTiff] Unsupported Packing: " << photometric << ", " << config << std::endl;
                hasError = true; break;
            }

            if (inBuffer) delete[] inBuffer;
            if (hasError)
            {
                OSG_WARN << "[ReaderWriterTiff] Failed to read with packing: " << photometric << ", " << config << std::endl;
                if (buffer) delete[] buffer; continue;
            }

            int numComponents = (photometric == PHOTOMETRIC_PALETTE) ? format : samplesperpixel;
            unsigned int pixelFormat =
                (numComponents) == 1 ? GL_LUMINANCE :
                (numComponents) == 2 ? GL_LUMINANCE_ALPHA :
                (numComponents) == 3 ? GL_RGB :
                (numComponents) == 4 ? GL_RGBA : (GLenum)-1;
            unsigned int dataType =
                (bitspersample == 8) ? GL_UNSIGNED_BYTE :
                (bitspersample == 16) ? GL_UNSIGNED_SHORT :
                (bitspersample == 32) ? GL_FLOAT : (GLenum)-1;
            unsigned int internalFormat = computeInternalFormat(pixelFormat, dataType);
            if (internalFormat <= 0)
            {
                OSG_WARN << "[ReaderWriterTiff] Unsupported image format" << std::endl;
                continue;
            }

            osg::Image* image = new osg::Image;
            image->setImage(w, h, d, internalFormat, pixelFormat, dataType,
                            buffer, osg::Image::USE_NEW_DELETE);
            seq->addImage(image);
        } while (TIFFReadDirectory(in));
    }
    TIFFClose(in);
    return seq.release();
}

#undef CVT
#undef PACK

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
        return "[osgVerse] Tiff format reader with 2D/3D image support";
    }

    virtual ReadResult readImage(const std::string& path, const Options* options) const
    {
        std::string fileName(path);
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

        bool usePseudo = (ext == "verse_tiff");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getLowerCaseFileExtension(fileName);
        }

        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        return readImage(in, options);
    }

    virtual ReadResult readImage(std::istream& fin, const Options* options) const
    {
        osg::ref_ptr<osg::ImageSequence> seq = tiffLoad(fin, options);
#if OSG_VERSION_GREATER_THAN(3, 2, 2)
        osg::ImageSequence::ImageDataList images = seq->getImageDataList();
        return images.empty() ? NULL : ((images.size() == 1) ?
                                        images[0]._image.get() : static_cast<osg::Image*>(seq.get()));
#else
        std::vector<osg::ref_ptr<osg::Image>> images = seq->getImages();
        return images.empty() ? NULL : ((images.size() == 1) ?
                                        images[0].get() : static_cast<osg::Image*>(seq.get()));
#endif
    }

    virtual WriteResult writeImage(const osg::Image& image, const std::string& path,
                                   const Options* options) const
    {
        std::string fileName(path);
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return WriteResult::FILE_NOT_HANDLED;

        bool usePseudo = (ext == "verse_tiff");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getLowerCaseFileExtension(fileName);
        }

        std::ofstream out(fileName, std::ios::out | std::ios::binary);
        // TODO
        return WriteResult::NOT_IMPLEMENTED;
    }

protected:
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_tiff, ReaderWriterTiff)
