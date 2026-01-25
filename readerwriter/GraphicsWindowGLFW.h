#ifndef MANA_READERWRITER_GRAPHICSWINDOWGLFW_HPP
#define MANA_READERWRITER_GRAPHICSWINDOWGLFW_HPP

#include <osg/Version>
#include <osgDB/ReaderWriter>
#include <osgViewer/GraphicsWindow>
#include "Export.h"

struct GLFWwindow;

namespace osgVerse
{
    class OSGVERSE_RW_EXPORT GraphicsWindowGLFW : public osgViewer::GraphicsWindow
    {
    public:
        struct OSGVERSE_RW_EXPORT WindowData : public osg::Referenced
        {
            WindowData() : majorVersion(0), minorVersion(0) {}
            int majorVersion, minorVersion;  // Expected OpenGL version
        };

        GraphicsWindowGLFW(osg::GraphicsContext::Traits* traits);
        virtual const char* libraryName() const { return "osgVerse"; }
        virtual const char* className() const { return "GraphicsWindowGLFW"; }

        virtual bool isSameKindAs(const osg::Object* object) const
        { return dynamic_cast<const GraphicsWindowGLFW*>(object) != 0; }

        virtual bool valid() const { return _valid; }
        virtual bool isRealizedImplementation() const { return _realized; }

        virtual bool realizeImplementation();
        virtual void closeImplementation();
        virtual bool makeCurrentImplementation();
        virtual bool releaseContextImplementation();
        virtual void swapBuffersImplementation();
#if OSG_VERSION_GREATER_THAN(3, 1, 1)
        virtual bool checkEvents();
#else
        virtual void checkEvents();
#endif
        virtual void grabFocus();
        virtual void grabFocusIfPointerInWindow();
        virtual void raiseWindow();
        virtual void requestWarpPointer(float x, float y);
        virtual bool setWindowDecorationImplementation(bool flag);
        virtual bool setWindowRectangleImplementation(int x, int y, int width, int height);
        virtual void setWindowName(const std::string& name);
        virtual void setCursor(osgViewer::GraphicsWindow::MouseCursor cursor);
        virtual void setSyncToVBlank(bool on);

        GraphicsWindowHandle* getHandle() const;

    protected:
        virtual ~GraphicsWindowGLFW();
        void initialize();

        GLFWwindow* _window;
        osg::Vec2d _lastMousePosition;
        int _lastKey, _lastModKey, _lastChar;
        bool _valid, _realized;
    };
}

#endif
