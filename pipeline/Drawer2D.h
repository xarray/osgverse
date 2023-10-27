#ifndef MANA_PP_DRAWER2D_HPP
#define MANA_PP_DRAWER2D_HPP

#include <osg/Version>
#include <osg/Image>
#include <osg/Texture2D>

namespace osgVerse
{
    class Drawer2D : public osg::Image
    {
    public:
        Drawer2D();
        Drawer2D(const Drawer2D& img, const osg::CopyOp& op = osg::CopyOp::SHALLOW_COPY);
        Drawer2D(const osg::Image& img, const osg::CopyOp& op = osg::CopyOp::SHALLOW_COPY);

        /** Initialize the image as usual, and start as a drawer here */
        bool start(bool useCurrentPixels);

        /** Finish drawing work and copy back to the image itself */
        bool finish();

        struct StyleData
        {
            StyleData(const osg::Vec4f& c = osg::Vec4f(1.0f, 1.0f, 1.0f, 1.0f),
                      bool f = false) : color(c), filled(f), type(COLOR) {}
            StyleData(osg::Image* im) : image(im), filled(true), type(IMAGE) {}
            StyleData(const osg::Vec2f& s, const osg::Vec2f& e,
                      const std::map<float, osg::Vec4f>& stops, bool f = false)
                : gradientStops(stops), filled(f), type(LINEAR_GRADIENT)
            { linearGradient.set(s[0], s[1], e[0], e[1]); }

            osg::ref_ptr<osg::Image> image;
            std::map<float, osg::Vec4f> gradientStops;
            osg::Vec4f color, linearGradient; bool filled;
            enum Type { COLOR, IMAGE, LINEAR_GRADIENT } type;
        };

        bool loadFont(const std::string& name, const std::string& file);
        void drawText(const osg::Vec2f pos, float size, const std::wstring& text,
                      const std::string& font = std::string(), const StyleData& sd = StyleData());

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

        void translate(const osg::Vec2& pos);
        void scale(const osg::Vec2& scale);
        void rotate(float angle);
        void clear(const osg::Vec4f& rect = osg::Vec4());
        void fillBackground(const osg::Vec4f& color);

        static unsigned char* convertImage(osg::Image* image, int& format, int& components);

    protected:
        osg::ref_ptr<osg::Referenced> _b2dData;
        bool _drawing;
    };
}

#endif
