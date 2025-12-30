#include <osg/io_utils>
#include <osg/Version>
#include <osg/ValueObject>
#include <osg/TriangleIndexFunctor>
#include <osg/Geode>
#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/Multisample>
#include <osg/Material>
#include <osg/PolygonOffset>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <sstream>
#include <iomanip>
#include <cctype>

#include "Utilities.h"
#include <libhv/all/client/requests.h>
#include <libhv/all/base64.h>
#include <xxYUV/rgb2yuv.h>
#include <avir/avir.h>
#include <nanoid/nanoid.h>
#include <miniz.h>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#define RGBCX_IMPLEMENTATION
#include <bc7_rdo/rgbcx.h>

using namespace osgVerse;
#define ALIGN(v, a) ((v) + ((a) - 1) & ~((a) - 1))

static std::string trimString(const std::string& str)
{
    if (!str.size()) return str;
    std::string::size_type first = str.find_first_not_of(" \t");
    std::string::size_type last = str.find_last_not_of("  \t\r\n");
    if ((first == str.npos) || (last == str.npos)) return std::string("");
    return str.substr(first, last - first + 1);
}

#pragma pack(push, 1)
struct DDS_PIXELFORMAT
{
    uint32_t dwSize = 32, dwFlags = 0x4;   // DDPF_FOURCC
    uint32_t dwFourCC = 0x31545844; // "DXT1"
    uint32_t dwRGBBitCount = 0;
    uint32_t dwRBitMask = 0, dwGBitMask = 0;
    uint32_t dwBBitMask = 0, dwABitMask = 0;
};

struct DDS_HEADER
{
    uint32_t dwMagic = 0x20534444; // "DDS "
    uint32_t dwSize = 124;
    uint32_t dwFlags = 0x00001007; // DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT|DDSD_LINEARSIZE
    uint32_t dwHeight, dwWidth, dwPitchOrLinearSize;
    uint32_t dwDepth = 0, dwMipMapCount = 0;
    uint32_t dwReserved1[11] = {};
    DDS_PIXELFORMAT ddspf;
    uint32_t dwCaps = 0x00001000; // DDSCAPS_TEXTURE
    uint32_t dwCaps2 = 0, dwCaps3 = 0, dwCaps4 = 0, dwReserved2 = 0;
};
#pragma pack(pop)

struct MipmapHelpers
{
    static inline float log2(float x) { static float inv2 = 1.f / logf(2.f); return logf(x) * inv2; }
    static inline int log2Int(float v) { return (int)floorf(log2(v)); }
    static inline bool isPowerOf2(int x) { return (x & (x - 1)) == 0; }

    static inline float sincf(float x)
    {
        if (fabsf(x) >= 0.0001f) return sinf(x) / x;
        else return 1.0f + x * x * (-1.0f / 6.0f + x * x * 1.0f / 120.0f);
    }

    struct Box
    {
        static constexpr float width = 0.5f;
        static float eval(float x) { if (fabsf(x) <= width) return 1.0f; else return 0.0f; }
    };

    struct Lanczos
    {
        static constexpr float width = 3.0f;
        static float eval(float x)
        {
            if (fabsf(x) >= width) return 0.0f;
            return sincf(osg::PI * x) * sincf(osg::PI * x / width);
        }
    };

    struct Kaiser
    {
        static constexpr float width = 7.0f, alpha = 4.0f, stretch = 1.0f;
        static float eval(float x)
        {
            float t = x / width; float t2 = t * t; if (t2 >= 1.0f) return 0.0f;
            return sincf(osg::PI * x * stretch) * bessel_0(alpha * sqrtf(1.0f - t2)) / bessel_0(alpha);
        }

        static inline float bessel_0(float x, float EPSILON = 1e-6f)
        {
            float xh = 0.5f * x, sum = 1.0f, pow = 1.0f, ds = 1.0f, k = 0.0f;
            while (ds > sum * EPSILON)
            { k += 1.0f; pow = pow * (xh / k); ds = pow * pow; sum = sum + ds; } return sum;
        }
    };

    template<typename Filter>
    static float filter_sample(float x, float scale)
    {
        constexpr int SAMPLE_COUNT = 32;
        constexpr float SAMPLE_COUNT_INV = 1.0f / float(SAMPLE_COUNT);
        float sample = 0.5f, sum = 0.0f;
        for (int i = 0; i < SAMPLE_COUNT; i++, sample += 1.0f)
            sum += Filter::eval((x + sample * SAMPLE_COUNT_INV) * scale);
        return sum * SAMPLE_COUNT_INV;
    }

    template<typename Filter>
    static void downsample(const std::vector<osg::Vec4>& source, int w0, int h0,
                           std::vector<osg::Vec4>& target, int w1, int h1, std::vector<osg::Vec4>& temp)
    {
        float scale_x = float(w1) / float(w0), scale_y = float(h1) / float(h0);
        float inv_scale_x = 1.0f / scale_x, inv_scale_y = 1.0f / scale_y;
        float filter_width_x = Filter::width * inv_scale_x, sum_x = 0.0f;
        float filter_width_y = Filter::width * inv_scale_y, sum_y = 0.0f;
        int window_size_x = int(ceilf(filter_width_x * 2.0f)) + 1;
        int window_size_y = int(ceilf(filter_width_y * 2.0f)) + 1;

        std::vector<float> kernel_x(window_size_x), kernel_y(window_size_y);
        memset(kernel_x.data(), 0, window_size_x * sizeof(float));
        memset(kernel_y.data(), 0, window_size_y * sizeof(float));
        for (int x = 0; x < window_size_x; x++)  // Fill horizontal kernel
        {
            float sample = filter_sample<Filter>(float(x - window_size_x / 2), scale_x);
            kernel_x[x] = sample; sum_x += sample;
        }
        for (int y = 0; y < window_size_y; y++)  // Fill vertical kernel
        {
            float sample = filter_sample<Filter>(float(y - window_size_y / 2), scale_y);
            kernel_y[y] = sample; sum_y += sample;
        }
        for (int x = 0; x < window_size_x; x++) kernel_x[x] /= sum_x;
        for (int y = 0; y < window_size_y; y++) kernel_y[y] /= sum_y;

#pragma omp parallel for schedule(dynamic, 1)
        for (int y = 0; y < h0; y++)  // Apply horizontal kernel
        {
            for (int x = 0; x < w1; x++)
            {
                float center = (float(x) + 0.5f) * inv_scale_x;
                int ll = int(floorf(center - filter_width_x));
                osg::Vec4 sum(0.0f, 0.0f, 0.0f, 0.0f);
                for (int i = 0; i < window_size_x; i++)
                    sum += source[osg::clampBetween(ll + i, 0, w0 - 1) + y * w0] * kernel_x[i];
                temp[x * h0 + y] = sum;
            }
        }

#pragma omp parallel for schedule(dynamic, 1)
        for (int x = 0; x < w1; x++)  // Apply vertical kernel
        {
            for (int y = 0; y < h1; y++)
            {
                float center = (float(y) + 0.5f) * inv_scale_y;
                int tt = int(floorf(center - filter_width_y));
                osg::Vec4 sum(0.0f, 0.0f, 0.0f, 0.0f);
                for (int i = 0; i < window_size_y; i++)
                    sum += temp[x * h0 + osg::clampBetween(tt + i, 0, h0 - 1)] * kernel_y[i];
                target[x + y * w1] = sum;
            }
        }
    }
};

namespace osgVerse
{
    bool copyImageChannel(osg::Image& src, int srcChannel, osg::Image& dst, int dstChannel)
    {
        if (src.isCompressed() || dst.isCompressed()) return false;
        if (src.s() != dst.s() || src.t() != dst.t()) return false;

#if OSG_VERSION_GREATER_THAN(3, 2, 3)
#pragma omp parallel for schedule(dynamic, 1)
        for (int y = 0; y < src.t(); ++y)
            for (int x = 0; x < src.s(); ++x)
            {
                osg::Vec4 c0 = src.getColor(x, y), c1 = dst.getColor(x, y);
                c1[dstChannel] = c0[srcChannel]; dst.setColor(c1, x, y);
            }
#else
        OSG_WARN << "[copyImageChannel] Image::setColor() not implemented." << std::endl;
#endif
        return true;
    }

    osg::Texture* constructOcclusionRoughnessMetallic(osg::Texture* origin, osg::Texture* input,
                                                      int chO, int chR, int chM)
    {
        osg::Texture2D* texO = static_cast<osg::Texture2D*>(origin);
        osg::Texture2D* texI = static_cast<osg::Texture2D*>(input);
        if (!texI || (texI && !texI->getImage())) return texO;

        osg::Image* imgI = texI->getImage(); int s = imgI->s(), t = imgI->t();
        if (!texO)  // no origin texture, create one referring to current input
        {
            osg::Image* imgO = new osg::Image;
            imgO->allocateImage(s, t, 1, GL_RGBA, GL_UNSIGNED_BYTE);
            imgO->setInternalTextureFormat(GL_RGBA8);
            imgO->setUserValue("Loader", std::string("ConstructORM"));

            texO = new osg::Texture2D; texO->setName("OcclusionRoughnessMetallic_" + texI->getName());
            texO->setResizeNonPowerOfTwoHint(false); texO->setImage(imgO);
            texO->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT);
            texO->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT);
            texO->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR_MIPMAP_LINEAR);
            texO->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        }
        else
        {
            osg::Image* imgO = texO->getImage();
            imgO->scaleImage(s, t, 1);
        }
        
        osg::Vec4ub* ptr = (osg::Vec4ub*)texO->getImage()->data();
#pragma omp parallel for schedule(dynamic, 1)
        for (int y = 0; y < t; ++y)
            for (int x = 0; x < s; ++x)
            {
                osg::Vec4 c0 = imgI->getColor(x, y);
                unsigned char valO = (chO < 0) ? 255 : (unsigned char)(c0[chO] * 255.0f);
                unsigned char valR = (chR < 0) ? 255 : (unsigned char)(c0[chR] * 255.0f);
                unsigned char valM = (chM < 0) ? 0 : (unsigned char)(c0[chM] * 255.0f);
                *(ptr + x + y * s) = osg::Vec4ub(valO, valR, valM, 255);
            }
        return texO;
    }

    osg::Image* compressImage(osg::Image& img, osgDB::ReaderWriter* rw, bool forceDXT1)
    {
        int w = img.s(), h = img.t(); unsigned char* data = img.data();
        if (!data || w == 0 || h == 0 || (w % 4) || (h % 4)) return NULL;

        static bool rgbxInited = false;
        if (!rgbxInited) { rgbcx::init(); rgbxInited = true; }
        int dataType = (img.getDataType() == GL_UNSIGNED_BYTE) ? 0 : -1;
        int components = osg::Image::computeNumComponents(img.getPixelFormat());
        bool useDXT5 = (!forceDXT1 && components == 4);

        const unsigned int blocksX = w / 4, blocksY = h / 4;
        const unsigned int bcSize = blocksX * blocksY * (useDXT5 ? 16 : 8);
        std::vector<unsigned char> bcData(bcSize);
        for (unsigned int by = 0; by < blocksY; ++by)
            for (unsigned int bx = 0; bx < blocksX; ++bx)
            {
                unsigned int block[16];
                for (unsigned int y = 0; y < 4; ++y)
                    for (unsigned int x = 0; x < 4; ++x)
                    {
                        unsigned int px = bx * 4 + x, py = by * 4 + y;
                        if (dataType == 0)
                        {
                            unsigned int idx = (py * w + px) * components, rgba = 0;
                            for (int i = components; i < 4; ++i) rgba |= 0xFF << (8 * i);
                            for (int i = 0; i < components; ++i) rgba |= (unsigned int)data[idx + i] << (8 * i);
                            block[y * 4 + x] = rgba;
                        }
                        else {}  // TODO
                    }
                unsigned char* dst = bcData.data() + (by * blocksX + bx) * (useDXT5 ? 16 : 8);
                if (useDXT5) rgbcx::encode_bc3(dst, (unsigned char*)block);
                else rgbcx::encode_bc1(dst, (unsigned char*)block, false);
            }

        DDS_HEADER hdr{}; std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
        hdr.dwWidth = w; hdr.dwHeight = h; hdr.dwPitchOrLinearSize = bcSize;
        hdr.ddspf.dwFourCC = useDXT5 ? 0x35545844 : 0x31545844;
        ss.write((char*)&hdr, sizeof(hdr)); ss.write((char*)bcData.data(), bcData.size());

        if (!rw) rw = osgDB::Registry::instance()->getReaderWriterForExtension("dds");
        if (rw) return rw->readImage(ss).takeImage(); else return NULL;
    }

    bool resizeImage(osg::Image& img, int rWidth, int rHeight, bool autoCompress)
    {
        int w = img.s(), h = img.t(); unsigned char *data = img.data(), *newData = NULL;
        if (!data || w == 0 || h == 0 || rWidth == 0 || rHeight == 0) return false;

        bool compressed = img.isCompressed();
        int depth = (img.getDataType() == GL_UNSIGNED_BYTE) ? 8
                  : ((img.getDataType() == GL_UNSIGNED_SHORT) ? 16 : 0);
        int comp = osg::Image::computeNumComponents(img.getPixelFormat());
        if (!depth || compressed)
        {
            if (compressed)
            {   // Maybe compressed format? Try a stupid method...
                newData = new unsigned char[w * h * comp];  // normal ubyte array
#pragma omp parallel for schedule(dynamic, 1)
                for (int y = 0; y < h; ++y)
                    for (int x = 0; x < w; ++x)
                    {
                        osg::Vec4 c = img.getColor(x, y); int idx = comp * (x + y * w);
                        for (int n = 0; n < comp; ++n) *(newData + idx + n) = (unsigned char)(c[n] * 255.0f);
                    }

                osg::Image::AllocationMode m = osg::Image::USE_NEW_DELETE;
                switch (comp)
                {
                case 1: img.setImage(w, h, 1, GL_LUMINANCE8, GL_LUMINANCE, GL_UNSIGNED_BYTE, newData, m); break;
                case 2: img.setImage(w, h, 1, GL_RG8, GL_RG, GL_UNSIGNED_BYTE, newData, m); break;
                case 3: img.setImage(w, h, 1, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, newData, m); break;
                default: img.setImage(w, h, 1, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, newData, m); break;
                }
            }

            img.scaleImage(rWidth, rHeight, 1);
            if (compressed && autoCompress)
            {
                osg::ref_ptr<osg::Image> dds = compressImage(img);
                img.allocateImage(dds->s(), dds->t(), 1, dds->getPixelFormat(), dds->getDataType());
                img.setInternalTextureFormat(dds->getInternalTextureFormat());
                memcpy(img.data(), dds->data(), dds->getTotalSizeInBytes());
            }
            return (img.s() == rWidth && img.t() == rHeight);
        }

        avir::CImageResizer<> resizer(8, depth); newData = new unsigned char[rWidth * rHeight * comp];
        resizer.resizeImage(data, img.s(), img.t(), 0, newData, rWidth, rHeight, comp, 0.0);
        img.setImage(rWidth, rHeight, 1, img.getInternalTextureFormat(), img.getPixelFormat(), GL_UNSIGNED_BYTE,
                     newData, osg::Image::USE_NEW_DELETE); return true;
    }

    bool generateMipmaps(osg::Image& image, bool useKaiser)
    {
        int w0 = image.s(), h0 = image.t(); int w = w0, h = h0;
        if (!image.valid() || image.isCompressed()) return false;
        if ((w0 < 2 && h0 < 2) || image.r() > 1) return false;

        std::vector<osg::Vec4> source(w0 * h0), temp(w0 * h0 / 2), level0;
#pragma omp parallel for schedule(dynamic, 1)
        for (int i = 0; i < h0; ++i)
            for (int j = 0; j < w0; ++j) source[i * w0 + j] = image.getColor(j, i);

        typedef std::pair<osg::Vec2, std::vector<osg::Vec4>> MipmapData;
        std::vector<MipmapData> mipmapDataList(1); bool hasLevel0 = false;
        if (!(MipmapHelpers::isPowerOf2(w0) && MipmapHelpers::isPowerOf2(h0)))
        {
            w = osg::Image::computeNearestPowerOfTwo(w0); if (w > w0) w >>= 2;
            h = osg::Image::computeNearestPowerOfTwo(h0); if (h > h0) h >>= 2;
            level0.resize(w * h); hasLevel0 = true;
            if (useKaiser)
                MipmapHelpers::downsample<MipmapHelpers::Kaiser>(source, w0, h0, level0, w, h, temp);
            else
                MipmapHelpers::downsample<MipmapHelpers::Box>(source, w0, h0, level0, w, h, temp);
        }
        else level0 = source;

        std::vector<osg::Vec4>& data0 = level0;
        int numLevels = MipmapHelpers::log2Int(w > h ? w : h) + 1; mipmapDataList.resize(numLevels);
        mipmapDataList[0] = MipmapData(osg::Vec2(w, h), level0);
        for (int i = 1; i < numLevels; ++i)
        {
            int prevW = (int)mipmapDataList[i - 1].first[0];
            int prevH = (int)mipmapDataList[i - 1].first[1];
            int ww = (w >> i); ww = ww > 1 ? ww : 1;
            int hh = (h >> i); hh = hh > 1 ? hh : 1;
            mipmapDataList[i] = MipmapData(osg::Vec2(ww, hh), std::vector<osg::Vec4>(ww * hh));

            std::vector<osg::Vec4>& data = mipmapDataList[i].second;
            if (useKaiser)
                MipmapHelpers::downsample<MipmapHelpers::Kaiser>(data0, prevW, prevH, data, ww, hh, temp);
            else
                MipmapHelpers::downsample<MipmapHelpers::Box>(data0, prevW, prevH, data, ww, hh, temp);
            data0 = mipmapDataList[i].second;
        }

        std::vector<unsigned char> totalData;
        osg::Image::MipmapDataType mipmapInfo;
        if (!hasLevel0) mipmapDataList.erase(mipmapDataList.begin());

        GLenum pf = image.getPixelFormat(), dt = image.getDataType();
        totalData.insert(totalData.begin(), image.data(), image.data() + image.getTotalSizeInBytes());
        for (size_t i = 0; i < mipmapDataList.size(); ++i)
        {
            std::vector<osg::Vec4>& data = mipmapDataList[i].second;
            int ww = (int)mipmapDataList[i].first[0], hh = (int)mipmapDataList[i].first[1];
            if (i > 0)
            {
                int ww0 = (int)mipmapDataList[i - 1].first[0];
                int hh0 = (int)mipmapDataList[i - 1].first[1];
                size_t rowSize = osg::Image::computeRowWidthInBytes(ww0, pf, dt, image.getPacking());
                mipmapInfo.push_back(mipmapInfo.back() + rowSize * hh0);
            }
            else
                mipmapInfo.push_back(image.getTotalSizeInBytes());

            osg::ref_ptr<osg::Image> subImage = new osg::Image;
            subImage->allocateImage(ww, hh, 1, pf, dt, image.getPacking());
            subImage->setInternalTextureFormat(image.getInternalTextureFormat());
#if OSG_VERSION_GREATER_THAN(3, 2, 3)
#pragma omp parallel for schedule(dynamic, 1)
            for (int j = 0; j < hh; ++j)
                for (int k = 0; k < ww; ++k) subImage->setColor(data[j * ww + k], k, j);
#else
            OSG_WARN << "[generateMipmaps] Image::setColor() not implemented." << std::endl;
#endif
            totalData.insert(totalData.end(), subImage->data(),
                             subImage->data() + subImage->getTotalSizeInBytes());
        }

        unsigned char* totalData1 = new unsigned char[totalData.size()];
        memcpy(totalData1, &totalData[0], totalData.size());
        image.setImage(w0, h0, 1, image.getInternalTextureFormat(), pf, dt, totalData1,
                       osg::Image::USE_NEW_DELETE, image.getPacking());
        image.setMipmapLevels(mipmapInfo);
        return true;
    }

    std::vector<std::vector<unsigned char>> convertRGBtoYUV(osg::Image* image, YUVFormat format)
    {
        std::vector<std::vector<unsigned char>> yuvData;
        if (!image) return yuvData;
        if (image->getDataType() != GL_UNSIGNED_BYTE)
        { OSG_WARN << "[convertRGBtoYUV] Invalid data type. Must be GL_UNSIGNED_BYTE\n"; return yuvData; }

        GLenum pixelFormat = image->getPixelFormat();
        if (pixelFormat != GL_RGB && pixelFormat != GL_RGBA && pixelFormat != GL_BGR && pixelFormat != GL_BGRA)
        { OSG_WARN << "[convertRGBtoYUV] Invalid pixel format. Must be RGB/RGBA/BGR/BGRA\n"; return yuvData; }

        rgb2yuv_parameter rgb2yuv; std::vector<char> yuvBuffer;
        memset(&rgb2yuv, 0, sizeof(rgb2yuv_parameter));
        rgb2yuv.width = image->s(); rgb2yuv.height = image->t(); rgb2yuv.rgb = image->data();
        rgb2yuv.componentRGB = osg::Image::computeNumComponents(pixelFormat);
        rgb2yuv.swizzleRGB = (pixelFormat == GL_RGB || pixelFormat == GL_RGBA) ? false : true;
        rgb2yuv.alignWidth = 16; rgb2yuv.alignHeight = 1; rgb2yuv.alignSize = 1;
        rgb2yuv.strideRGB = 0; rgb2yuv.videoRange = false;

        int strideY = ALIGN(rgb2yuv.width, rgb2yuv.alignWidth);
        int strideU = strideY / 2, strideV = strideY / 2;
        int sizeY = ALIGN(strideY * ALIGN(rgb2yuv.height, rgb2yuv.alignHeight), rgb2yuv.alignSize);
        int sizeU = ALIGN(strideU * ALIGN(rgb2yuv.height, rgb2yuv.alignHeight) / 2, rgb2yuv.alignSize);
        size_t yuvSize = sizeY + sizeU + sizeU, lastSize = yuvBuffer.size();
        if (yuvSize != lastSize) yuvBuffer.resize(yuvSize);
        if (yuvSize == 0) { OSG_NOTICE << "Failed to execute convertRGBtoYUV()\n"; return yuvData; }

        char* ptr = &yuvBuffer[0]; rgb2yuv.y = ptr;
        rgb2yuv.u = ptr + sizeY; rgb2yuv.v = ptr + sizeY + sizeU;
        switch (format)
        {
        case YU12:
            rgb2yuv_yu12(&rgb2yuv); yuvData.resize(3);
            yuvData[0].resize(sizeY); memcpy(yuvData[0].data(), rgb2yuv.y, sizeY);
            yuvData[1].resize(sizeU); memcpy(yuvData[1].data(), rgb2yuv.u, sizeU);
            yuvData[2].resize(sizeU); memcpy(yuvData[2].data(), rgb2yuv.v, sizeU); break;
        case YV12:
            rgb2yuv_yv12(&rgb2yuv); yuvData.resize(3);
            yuvData[0].resize(sizeY); memcpy(yuvData[0].data(), rgb2yuv.y, sizeY);
            yuvData[1].resize(sizeU); memcpy(yuvData[1].data(), rgb2yuv.v, sizeU);
            yuvData[2].resize(sizeU); memcpy(yuvData[2].data(), rgb2yuv.u, sizeU); break;
        case NV12:
            rgb2yuv_nv12(&rgb2yuv); yuvData.resize(2);
            yuvData[0].resize(sizeY); memcpy(yuvData[0].data(), rgb2yuv.y, sizeY);
            yuvData[1].resize(sizeU * 2); memcpy(yuvData[1].data(), rgb2yuv.u, sizeU * 2); break;
            break;
        case NV21:
            rgb2yuv_nv21(&rgb2yuv); yuvData.resize(2);
            yuvData[0].resize(sizeY); memcpy(yuvData[0].data(), rgb2yuv.y, sizeY);
            yuvData[1].resize(sizeU * 2); memcpy(yuvData[1].data(), rgb2yuv.v, sizeU * 2); break;
        default: OSG_NOTICE << "Unsupported for convertRGBtoYUV()\n"; return yuvData;
        }
        return yuvData;
    }

    std::vector<unsigned char> loadFileData(const std::string& url, std::string& mimeType, std::string& encodingType,
                                            const std::vector<std::string>& reqHeaders)
    {
        std::vector<unsigned char> buffer;
        std::string scheme = osgDB::getServerProtocol(url);
        if (!scheme.empty())
        {
#ifdef __EMSCRIPTEN__
            osg::ref_ptr<osgVerse::WebFetcher> wf = new osgVerse::WebFetcher;
            bool succeed = wf->httpGet(url, NULL, NULL, NULL, reqHeaders);
            if (!succeed)
            {
                OSG_WARN << "[loadFileData] Failed getting " << url << ": "
                         << wf->status << std::endl; return buffer;
            }

            size_t size = wf->buffer.size(); buffer.resize(size);
            memcpy(buffer.data(), wf->buffer.data(), size);
            for (size_t i = 0; i < wf->resHeaders.size(); i += 2)
            {
                std::string key = trimString(wf->resHeaders[i]);
                std::transform(key.begin(), key.end(), key.begin(), tolower);
                if (key == "content-type") mimeType = trimString(wf->resHeaders[i + 1]);
                else if (key == "content-encoding") encodingType = trimString(wf->resHeaders[i + 1]);
            }
#else
            HttpRequest req; req.method = HTTP_GET;
            req.url = osgVerse::WebAuxiliary::normalizeUrl(url); req.scheme = scheme;
            req.headers["User-Agent"] = "Mozilla/5.0"; req.headers["Accept"] = "*/*";
            for (size_t i = 0; i < reqHeaders.size(); i += 2)
            {
                if (i == reqHeaders.size() - 1) break;
                req.headers[reqHeaders[i + 0]] = reqHeaders[i + 1];
            }

            HttpResponse response; hv::HttpClient client;
            int result = client.send(&req, &response);
            if (result != 0)
                { OSG_WARN << "[loadFileData] Failed getting " << url << ": " << result << std::endl; }
            else if (response.status_code > 200 || response.body.empty())
            {
                OSG_WARN << "[loadFileData] Failed getting " << url << ": Code = "
                         << response.status_code << ", Size = " << response.body.size() << std::endl;
            }
            else
            {
                size_t size = response.body.size(); buffer.resize(size);
                memcpy(buffer.data(), response.body.data(), size);
            }

            for (http_headers::iterator itr = response.headers.begin(); itr != response.headers.end(); ++itr)
            {
                std::string key = trimString(itr->first);
                std::transform(key.begin(), key.end(), key.begin(), tolower);
                if (key == "content-type") mimeType = trimString(itr->second);
                else if (key == "content-encoding") encodingType = trimString(itr->second);
            }
#endif
        }
        else
        {
            std::ifstream fin(url.c_str(), std::ios::in | std::ios::binary);
            std::string str((std::istreambuf_iterator<char>(fin)),
                            std::istreambuf_iterator<char>());
            if (!str.empty())
                { buffer.resize(str.size()); memcpy(buffer.data(), str.data(), str.size()); }
        }
        return buffer;
    }
}

/// CompressAuxiliary ///
struct CompressHandleData : osg::Referenced
{
    CompressHandleData(CompressAuxiliary::CompressorType t) : type(t) {}
    std::vector<unsigned char> buffer;
    CompressAuxiliary::CompressorType type;

    mz_zip_archive zipArchive;
};

osg::Referenced* CompressAuxiliary::createHandle(CompressorType type, std::istream& fin)
{
    std::string data((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());
    osg::ref_ptr<CompressHandleData> H = new CompressHandleData(type);
    if (type == ZIP)
    {
        memset(&(H->zipArchive), 0, sizeof(mz_zip_archive));
        H->buffer.assign(data.begin(), data.end());
        if (mz_zip_reader_init_mem(&(H->zipArchive), (void*)H->buffer.data(), H->buffer.size(), 0))
            return H.release();
    }
    return NULL;
}

void CompressAuxiliary::destroyHandle(osg::Referenced* handle)
{
    CompressHandleData* H = (CompressHandleData*)handle; if (!H) return;
    if (H->type == ZIP) mz_zip_reader_end(&(H->zipArchive));
}

std::vector<std::string> CompressAuxiliary::listContents(osg::Referenced* handle)
{
    std::vector<std::string> fileList;
    CompressHandleData* H = (CompressHandleData*)handle; if (!H) return fileList;
    if (H->type == ZIP)
    {
        mz_uint count = mz_zip_reader_get_num_files(&(H->zipArchive));
        for (mz_uint i = 0; i < count; ++i)
        {
            mz_zip_archive_file_stat fileStat;
            if (mz_zip_reader_file_stat(&(H->zipArchive), i, &fileStat))
                fileList.push_back(fileStat.m_filename);
        }
    }
    return fileList;
}

std::vector<unsigned char> CompressAuxiliary::extract(osg::Referenced* handle, const std::string& fileName)
{
    size_t uncompSize = 0; std::vector<unsigned char> data;
    CompressHandleData* H = (CompressHandleData*)handle; if (!H) return data;
    if (H->type == ZIP)
    {
        void* p = mz_zip_reader_extract_file_to_heap(&(H->zipArchive), fileName.c_str(), &uncompSize, 0);
        if (!p) return data; data.assign((unsigned char*)p, (unsigned char*)p + uncompSize); mz_free(p);
    }
    return data;
}

/// AudioPlayer ///
struct AudioPlayingMixer
{
    static void dataCallback(ma_device* device, void* output, const void* input, ma_uint32 frameCount)
    {
        AudioPlayingMixer* mixer = (AudioPlayingMixer*)device->pUserData;
        float* outputF = (float*)output; int channels = device->playback.channels;
        memset(outputF, 0, frameCount * channels * sizeof(float));

        float data[4096]; ma_mutex_lock(&mixer->lock);
        std::map<std::string, osg::ref_ptr<AudioPlayer::Clip>>& clips = mixer->player->getClips();
        for (std::map<std::string, osg::ref_ptr<AudioPlayer::Clip>>::iterator it = clips.begin();
             it != clips.end(); ++it)
        {
            AudioPlayer::Clip* clip = it->second.get();
            if (clip->state != AudioPlayer::Clip::PLAYING) continue;

            ma_uint64 framesRead = 0; memset(data, 0, 4096 * sizeof(float));
            ma_result result = ma_decoder_read_pcm_frames(clip->decoder, (void*)data, frameCount, &framesRead);
            if (result == MA_SUCCESS && framesRead > 0)
            {
                for (ma_uint32 frame = 0; frame < framesRead; ++frame)
                    for (ma_uint32 ch = 0; ch < channels; ++ch)
                    {
                        float sample = data[frame * channels + ch] * clip->volume;
                        outputF[frame * channels + ch] += sample;
                    }
            }

            if (framesRead < frameCount)
            {
                if (clip->looping)
                {
                    ma_uint64 remainingFrames = frameCount - framesRead;
                    ma_decoder_seek_to_pcm_frame(clip->decoder, 0);
                }
                else
                    clip->state = AudioPlayer::Clip::STOPPED;
            }
        }
        ma_mutex_unlock(&mixer->lock);
    }

    AudioPlayer* player;
    ma_mutex lock;
};

AudioPlayer* AudioPlayer::instance()
{
    static osg::ref_ptr<AudioPlayer> s_instance = new AudioPlayer;
    return s_instance.get();
}

AudioPlayer::AudioPlayer()
{
    _mixer = new AudioPlayingMixer;
    ma_mutex_init(&(_mixer->lock)); _mixer->player = this;

    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = ma_format_f32;
    deviceConfig.playback.channels = 2;
    deviceConfig.sampleRate = 48000;
    deviceConfig.dataCallback = AudioPlayingMixer::dataCallback;
    deviceConfig.pUserData = _mixer;

    _device = new ma_device;
    if (ma_device_init(NULL, &deviceConfig, _device) != MA_SUCCESS)
        { OSG_FATAL << "[AudioPlayer] Failed to open playback device\n"; }
    else if (ma_device_start(_device) != MA_SUCCESS)
        { OSG_FATAL << "[AudioPlayer] Failed to start device\n"; }

    ma_context* context = ma_device_get_context(_device);
    ma_device_state state = ma_device_get_state(_device);
    ma_device_info info; ma_device_get_info(_device, ma_device_type_playback, &info);
    OSG_NOTICE << "[AudioPlayer] Backend: " << (context != NULL ? ma_get_backend_name(context->backend) : "(failed)")
               << "; Device: " << info.name << ", State: " << (state == ma_device_state_started ? "started\n" : "idle\n");
}

AudioPlayer::~AudioPlayer()
{
    for (std::map<std::string, osg::ref_ptr<Clip>>::iterator it = _clips.begin();
         it != _clips.end(); ++it)
    { ma_decoder_uninit(it->second->decoder); delete it->second->decoder; }
    ma_device_uninit(_device);
    ma_mutex_uninit(&(_mixer->lock));
    delete _device; delete _mixer;
}

bool AudioPlayer::addFile(const std::string& file, bool autoPlay, bool looping)
{
    if (_clips.find(file) != _clips.end()) return false;
    else ma_mutex_lock(&_mixer->lock);

    osg::ref_ptr<Clip> clip = new Clip; clip->decoder = new ma_decoder;
    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 2, 48000);
    ma_result result = ma_decoder_init_file(file.c_str(), &decoderConfig, clip->decoder);
    if (result != MA_SUCCESS)
    {
        OSG_WARN << "[AudioPlayer] Failed to create clip: " << result << "\n";
        ma_mutex_unlock(&_mixer->lock); return false;
    }

    if (autoPlay) clip->state = Clip::PLAYING; clip->looping = looping;
    _clips[file] = clip; ma_mutex_unlock(&_mixer->lock); return true;
}

bool AudioPlayer::removeFile(const std::string& file)
{
    std::map<std::string, osg::ref_ptr<Clip>>::iterator it = _clips.find(file);
    if (it != _clips.end())
    {
        ma_mutex_lock(&_mixer->lock); ma_decoder_uninit(it->second->decoder);
        delete it->second->decoder; _clips.erase(it);
        ma_mutex_unlock(&_mixer->lock); return true;
    }
    return false;
}

AudioPlayer::Clip* AudioPlayer::getClip(const std::string& file)
{
    std::map<std::string, osg::ref_ptr<Clip>>::iterator it = _clips.find(file);
    return it == _clips.end() ? NULL : it->second.get();
}

const AudioPlayer::Clip* AudioPlayer::getClip(const std::string& file) const
{
    std::map<std::string, osg::ref_ptr<Clip>>::const_iterator it = _clips.find(file);
    return it == _clips.end() ? NULL : it->second.get();
}
