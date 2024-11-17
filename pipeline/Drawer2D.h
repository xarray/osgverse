#ifndef MANA_PP_DRAWER2D_HPP
#define MANA_PP_DRAWER2D_HPP

#include <osg/Version>
#include <osg/Image>
#include <osg/Texture2D>

namespace osgVerse
{
    struct DrawerStyleData
    {
        enum ExtendMode { PAD = 0, REPEAT, MIRROR, PAD_REPEAT, PAD_MIRROR,
                          REPEAT_PAD, REPEAT_MIRROR, MIRROR_PAD, MIRROR_REPEAT };
        enum Type { COLOR, IMAGE, LINEAR_GRADIENT, RADIAL_GRADIENT };

        DrawerStyleData(const osg::Vec4f& c = osg::Vec4f(1.0f, 1.0f, 1.0f, 1.0f), bool f = false)
        : color(c), filled(f), extending(PAD), type(COLOR) { transform.makeIdentity(); }

        DrawerStyleData(osg::Image* im, ExtendMode e = PAD)
        : image(im), filled(true), extending(e), type(IMAGE) { transform.makeIdentity(); }

        DrawerStyleData(const osg::Vec2f& s, const osg::Vec2f& e,
                    const std::map<float, osg::Vec4f>& stops, bool f = false)
        : gradientStops(stops), filled(f), extending(PAD), type(LINEAR_GRADIENT)
        { gradient.set(s[0], s[1], e[0], e[1]); transform.makeIdentity(); }

        DrawerStyleData(const osg::Vec2f& s, float r0, const osg::Vec2f& e, float r1,
                    const std::map<float, osg::Vec4f>& stops, bool f = false)
        : gradientStops(stops), filled(f), extending(PAD), type(RADIAL_GRADIENT)
        { gradient.set(s[0], s[1], e[0], e[1]); gradient2.set(r0, r1, 0.0f, 0.0f); transform.makeIdentity(); }

        osg::ref_ptr<osg::Image> image;
        std::map<float, osg::Vec4f> gradientStops;
        osg::Vec4f color, gradient, gradient2;
        osg::Matrix3 transform; bool filled;
        ExtendMode extending; Type type;
    };

    class Drawer2D : public osg::Image
    {
    public:
        using StyleData = DrawerStyleData;

        Drawer2D();
        Drawer2D(const Drawer2D& img, const osg::CopyOp& op = osg::CopyOp::SHALLOW_COPY);
        Drawer2D(const osg::Image& img, const osg::CopyOp& op = osg::CopyOp::SHALLOW_COPY);

        /** Initialize the image as usual, and start as a drawer here */
        bool start(bool useCurrentPixels, int threads = 0);

        /** Finish drawing work and copy back to the image itself */
        bool finish();

        bool loadFont(const std::string& name, const std::string& file);
        void drawText(const osg::Vec2f pos, float size, const std::wstring& text,
                      const std::string& font = std::string(), const StyleData& sd = StyleData());
        void drawUtf8Text(const osg::Vec2f pos, float size, const std::string& text,
                          const std::string& font = std::string(), const StyleData& sd = StyleData());
        osg::Vec4 getTextBoundingBox(const std::wstring& text, float size, const std::string& font = std::string());
        osg::Vec4 getUtf8TextBoundingBox(const std::string& text, float size, const std::string& font = std::string());

        void drawLine(const osg::Vec2f pos0, const osg::Vec2f pos1,
                      const StyleData& sd = StyleData());
        void drawPolyline(const std::vector<osg::Vec2f>& points, bool closed,
                          const StyleData& sd = StyleData());
        void drawCircle(const osg::Vec2f pos0, float r1, float r2 = 0.0f,
                        const StyleData& sd = StyleData());
        void drawArc(const osg::Vec2f pos0, float r1, float r2, float start, float sweep,
                     int asChordOrPie = 0, const StyleData& sd = StyleData());
        void drawRectangle(const osg::Vec4f rect, float rx = 0.0f, float ry = 0.0f,
                           const StyleData& sd = StyleData());

        struct PathData
        {
            PathData(const osg::Vec2& pt, bool onlyMove)
                : pos(pt), isMoving(onlyMove), isCubic(false) {}
            PathData(const osg::Vec2& pt, const osg::Vec2& c0, const osg::Vec2& c1)
                : pos(pt), control0(c0), control1(c1), isMoving(false), isCubic(true) {}

            osg::Vec2 pos, control0, control1;
            bool isMoving, isCubic;
        };
        void drawPath(const std::vector<PathData>& path, const StyleData& sd = StyleData());

        enum StrokeOption
        {
            WIDTH, START_CAP, END_CAP, JOIN, DASH_OFFSET
        };
        void setStrokeOption(StrokeOption opt, int value);

        void translate(const osg::Vec2& pos, bool postMult);
        void scale(const osg::Vec2& scale, bool postMult);
        void rotate(float angle, bool postMult);
        void setTransform(const osg::Matrix3& transform);
        osg::Matrix3 getTransform() const;

        void clear(const osg::Vec4f& rect = osg::Vec4());
        void fillBackground(const osg::Vec4f& color);

        static unsigned char* convertImage(osg::Image* image, int& format, int& components);

    protected:
        osg::ref_ptr<osg::Referenced> _b2dData;
        bool _drawing;
    };
}

#endif
