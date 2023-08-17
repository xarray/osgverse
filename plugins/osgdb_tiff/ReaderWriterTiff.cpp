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

#define CVT(x)      (((x) * 255L) / ((1L << 16) - 1))
#define PACK(a, b)  ((a) << 8 | (b))

static osg::Image* tiffLoad(std::istream& fin, const osgDB::Options* options)
{
    TIFFSetErrorHandler(tiffError);
    TIFFSetWarningHandler(tiffWarn);
    TIFF* in = TIFFClientOpen("inputstream", "r", (thandle_t)&fin,
                              tiffStreamReadProc, tiffStreamWriteProc,
                              tiffStreamSeekProc, tiffStreamCloseProc,
                              tiffStreamSizeProc, tiffStreamMapProc, tiffStreamUnmapProc);
    if (in == NULL) { OSG_WARN << "[ReaderWriterTiff] Unable to open stream" << std::endl; return NULL; }

    uint16 photometric = 0;
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

    uint16 samplesperpixel = 0;
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

    uint16 bitspersample = 0;
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

    uint32 w = 0, h = 0, d = 1; uint16 config = 0, dataType = 0;
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

    if (d > 1)
    {
        // TODO...
    }
    else
    {
        int dirCount = 0, imgSize = w * h * format;
        do
        {
            unsigned char* buffer = new unsigned char[imgSize];
            memset(buffer, 0, imgSize); dirCount++;

            unsigned char* currPtr = buffer + (h - 1) * w * format;
        } while (TIFFReadDirectory(in));
    }
    return NULL;
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
        tiffLoad(fin, options);
        return NULL;
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
