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

    BLContextCreateInfo info; info.threadCount = 0;
    core->context = new BLContext(core->image, &info);
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

namespace osgVerse_Drawer
{
    template<typename T>
    void drawText(BLContext* context, const osg::Vec2f pos, BLFont& font,
                  const std::wstring& text, bool filled, const T& style)
    {
        if (filled)
            context->fillUtf16Text(
                BLPoint(pos[0], pos[1]), font, (uint16_t*)text.data(), text.length(), style);
        else
            context->strokeUtf16Text(
                BLPoint(pos[0], pos[1]), font, (uint16_t*)text.data(), text.length(), style);
    }

    template<typename T>
    void drawPolyline(BLContext* context, const std::vector<BLPoint>& blPts,
                      bool filled, bool closed, const T& style)
    {
        if (closed)
        {
            if (filled) context->fillPolygon(&blPts[0], blPts.size(), style);
            else context->strokePolygon(&blPts[0], blPts.size(), style);
        }
        else context->strokePolyline(&blPts[0], blPts.size(), style);
    }

    template<typename T>
    void drawCircle(BLContext* context, const osg::Vec2f pos0, float r1, float r2,
                    bool filled, const T& style)
    {
        if (filled)
        {
            if (r2 > 0.0f && !osg::equivalent(r2, r1))
                context->fillEllipse(pos0[0], pos0[1], r1, r2, style);
            else
                context->fillCircle(pos0[0], pos0[1], r1, style);
        }
        else
        {
            if (r2 > 0.0f && !osg::equivalent(r2, r1))
                context->strokeEllipse(pos0[0], pos0[1], r1, r2, style);
            else
                context->strokeCircle(pos0[0], pos0[1], r1, style);
        }
    }

    template<typename T>
    void drawRectangle(BLContext* context, const osg::Vec4f r, float rx, float ry,
                       bool filled, const T& style)
    {
        BLRect rect{ r[0], r[1], r[2], r[3] };
        if (filled)
        {
            if (!(rx > 0.0f || ry > 0.0f)) context->fillRect(rect, style);
            else context->fillRoundRect(rect, rx, ry, style);
        }
        else
        {
            if (!(rx > 0.0f || ry > 0.0f)) context->strokeRect(rect, style);
            else context->strokeRoundRect(rect, rx, ry, style);
        }
    }

    template<typename T>
    void drawPath(BLContext* context, const BLPath& path, bool filled, const T& style)
    {
        if (filled) context->fillPath(path, style);
        else context->strokePath(path, style);
    }
}

static BLRgba32 asColor(const Drawer2D::StyleData& sd)
{
    return BLRgba32(sd.color[0] * 255, sd.color[1] * 255,
                    sd.color[2] * 255, sd.color[3] * 255);
}

void Drawer2D::drawText(const osg::Vec2f pos, float size, const std::wstring& text,
                        const std::string& fontName, const StyleData& sd)
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
        osgVerse_Drawer::drawText(core->context, pos, font, text, sd.filled, asColor(sd));
    }
}

void Drawer2D::drawLine(const osg::Vec2f p0, const osg::Vec2f p1, const StyleData& sd)
{
    VALID_B2D()
    {
        core->context->strokeLine(BLPoint(p0[0], p0[1]),
                                  BLPoint(p1[0], p1[1]), asColor(sd));
    }
}

void Drawer2D::drawPolyline(const std::vector<osg::Vec2f>& points,
                            bool closed, const StyleData& sd)
{
    VALID_B2D()
    {
        if (points.size() < 2)
        {
            OSG_WARN << "[Drawer2D] Too few points for drawing polyline" << std::endl;
            return;
        }
        else if (points.size() == 2) closed = false;

        std::vector<BLPoint> blPts;
        for (size_t i = 0; i < points.size(); ++i)
            blPts.push_back(BLPoint(points[i].x(), points[i].y()));
        osgVerse_Drawer::drawPolyline(core->context, blPts, sd.filled, closed, asColor(sd));
    }
}

void Drawer2D::drawCircle(const osg::Vec2f pos0, float r1, float r2, const StyleData& sd)
{
    VALID_B2D()
    {
        osgVerse_Drawer::drawCircle(core->context, pos0, r1, r2, sd.filled, asColor(sd));
    }
}

void Drawer2D::drawRectangle(const osg::Vec4f r, float rx, float ry, const StyleData& sd)
{
    VALID_B2D()
    {
        osgVerse_Drawer::drawRectangle(core->context, r, rx, ry, sd.filled, asColor(sd));
    }
}

void Drawer2D::drawPath(const std::vector<PathData>& path, const StyleData& sd)
{
    VALID_B2D()
    {
        if (path.size() < 2)
        {
            OSG_WARN << "[Drawer2D] Too few points for drawing path" << std::endl;
            return;
        }

        BLPath blPath;
        for (size_t i = 0; i < path.size(); ++i)
        {
            const PathData& pd = path[i];
            if (pd.isCubic)
            {
                blPath.cubicTo(BLPoint(pd.pos[0], pd.pos[1]),
                               BLPoint(pd.control0[0], pd.control0[1]),
                               BLPoint(pd.control1[0], pd.control1[1]));
            }
            else if (pd.isMoving) blPath.moveTo(BLPoint(pd.pos[0], pd.pos[1]));
            else blPath.lineTo(BLPoint(pd.pos[0], pd.pos[1]));
        }
        osgVerse_Drawer::drawPath(core->context, blPath, sd.filled, asColor(sd));
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
