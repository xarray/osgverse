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

        /** Initialize the image as usual, and start as a drawer here */
        bool start(bool useCurrentPixels);

        /** Finish drawing work and copy back to the image itself */
        bool finish();

        bool loadFont(const std::string& name, const std::string& file);
        void drawText(const osg::Vec2f pos, float size, const std::wstring& text,
                      const std::string& font = std::string(),
                      const osg::Vec4f& color = osg::Vec4f(1.0f, 1.0f, 1.0f, 1.0f));

        void clear(const osg::Vec4f& rect = osg::Vec4());
        void fillBackground(const osg::Vec4f& color);

    protected:
        osg::ref_ptr<osg::Referenced> _b2dData;
        bool _drawing;
    };
}

#endif
