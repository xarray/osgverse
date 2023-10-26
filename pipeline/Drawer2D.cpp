#include <osg/Version>
#include <osg/io_utils>

#define BL_STATIC
#include <blend2d/blend2d.h>
#include "Drawer2D.h"

using namespace osgVerse;

struct BlendCore : public osg::Referenced
{
    BLImage image;
    BLContext* context;
    std::map<std::string, BLFontFace> fonts;
};

Drawer2D::Drawer2D()
    : _drawing(false)
{ _b2dData = new BlendCore; }

bool Drawer2D::start(bool useCurrentPixels)
{
    BLFormat format = BLFormat::BL_FORMAT_PRGB32;
    int w = 1024; if (s() > 1) w = s();
    int h = 1024; if (t() > 1) h = t();

    unsigned char* pixels = NULL;
    if (useCurrentPixels)
    {
        if (_dataType != GL_UNSIGNED_BYTE)
        {
            OSG_WARN << "[Drawer2D] Invalid data type" << std::endl;
            return false;
        }

        switch (_pixelFormat)
        {
        case GL_LUMINANCE: case GL_ALPHA: case GL_R:
            format = BLFormat::BL_FORMAT_A8; pixels = data(); break;
        case GL_RGBA:
            format = BLFormat::BL_FORMAT_PRGB32; pixels = data(); break;
        case GL_RGB:
            {
                // TODO: convert data
            }
            format = BLFormat::BL_FORMAT_XRGB32; break;
        default:
            OSG_WARN << "[Drawer2D] Unsupported pixel format"
                     << std::endl; return false;
        }
    }

    BlendCore* core = (BlendCore*)_b2dData.get();
    if (!core) return false;
    if (useCurrentPixels && pixels != NULL)
    {
        BLResult r = core->image.createFromData(w, h, format, pixels, 0);
        if (r != BL_SUCCESS)
        {
            OSG_WARN << "[Drawer2D] Failed to use current data" << std::endl;
            core->image.create(w, h, format);
        }
    }
    else
        core->image.create(w, h, format);
    core->context = new BLContext(core->image);
    _drawing = true; return true;
}

bool Drawer2D::finish()
{
    BlendCore* core = (BlendCore*)_b2dData.get();
    bool contextClosed = false; _drawing = false;
    if (core && core->context != NULL)
    {
        BLImageData dataOut;
        core->context->end();
        if (core->image.getData(&dataOut) == BL_SUCCESS)
        {
            GLenum dType = GL_UNSIGNED_BYTE;
            if (dataOut.format == BLFormat::BL_FORMAT_A8)
            {
                if (_pixelFormat != GL_R && _pixelFormat != GL_ALPHA &&
                    _pixelFormat != GL_LUMINANCE)
                { allocateImage(dataOut.size.w, dataOut.size.h, 1, GL_LUMINANCE, dType); }
            }
            else if (dataOut.format == BLFormat::BL_FORMAT_PRGB32)
            {
                if (_pixelFormat != GL_RGBA)
                    allocateImage(dataOut.size.w, dataOut.size.h, 1, GL_RGBA, dType);
            }
            else if (dataOut.format == BLFormat::BL_FORMAT_XRGB32)
            {
                // TODO: convert rgb data
                allocateImage(dataOut.size.w, dataOut.size.h, 1, GL_RGBA, dType);
            }

            if (dataOut.size.w < 1 || dataOut.size.h < 1)
            {
                OSG_WARN << "[Drawer2D] Failed to get result data" << std::endl;
                return false;
            }
            memcpy(data(), dataOut.pixelData, getTotalSizeInBytes());
            dirty(); contextClosed = true;
        }
        else
            OSG_WARN << "[Drawer2D] Failed to get result data" << std::endl;
        delete core->context;
    }
    _b2dData = NULL; return contextClosed;
}

bool Drawer2D::loadFont(const std::string& name, const std::string& file)
{
    BlendCore* core = (BlendCore*)_b2dData.get();
    if (core && core->context)
    {
        BLFontFace& fontFace = core->fonts[name];
        BLArray<uint8_t> dataBuffer;
        if (BLFileSystem::readFile(file.c_str(), dataBuffer) == BL_SUCCESS)
        {
            BLFontData fontData;
            if (fontData.createFromData(dataBuffer) == BL_SUCCESS)
            {
                fontFace.reset();
                fontFace.createFromData(fontData, 0);
                return true;
            }
        }
    }
    return false;
}

#define VALID_B2D() BlendCore* core = (BlendCore*)_b2dData.get(); \
                    if (core && core->context && _drawing)

void Drawer2D::drawText(const osg::Vec2f pos, float size, const std::wstring& text,
                        const std::string& fontName, const osg::Vec4f& c)
{
    VALID_B2D()
    {
        if (core->fonts.empty())
        {
            OSG_WARN << "[Drawer2D] Unable to draw text without any font" << std::endl;
            return;
        }

        BLFontFace& fontFace = core->fonts.begin()->second;
        if (core->fonts.find(fontName) != core->fonts.end())
            fontFace = core->fonts[fontName];

        BLFont font; font.createFromFace(fontFace, size);
        BLRgba32 color(c[0] * 255, c[1] * 255, c[2] * 255, c[3] * 255);
        core->context->strokeUtf16Text(
            BLPoint(pos[0], pos[1]), font, (uint16_t*)text.data(), text.length(), color);
    }
}

void Drawer2D::clear(const osg::Vec4f& r)
{
    VALID_B2D()
    {
        if (r[2] > 0.0f && r[3] > 0.0f)
            core->context->clearRect(BLRect(r[0], r[1], r[2], r[3]));
        else
            core->context->clearAll();
    }
}

void Drawer2D::fillBackground(const osg::Vec4f& c)
{
    VALID_B2D()
    {
        BLRgba32 bg(c[0] * 255, c[1] * 255, c[2] * 255, c[3] * 255);
        core->context->fillAll(bg);
    }
}
