#ifndef MANA_READERWRITER_GRAPHICSWINDOWSDL_HPP
#define MANA_READERWRITER_GRAPHICSWINDOWSDL_HPP

#include <osg/Version>
#include <osgDB/ReaderWriter>
#include <osgViewer/GraphicsWindow>
#include "Export.h"

struct SDL_Window;

namespace osgVerse
{
    class OSGVERSE_RW_EXPORT GraphicsWindowSDL : public osgViewer::GraphicsWindow
    {
    public:
        GraphicsWindowSDL(osg::GraphicsContext::Traits* traits);
        virtual const char* libraryName() const { return "osgVerse"; }
        virtual const char* className() const { return "GraphicsWindowSDL"; }

        osg::Referenced* getVulkanObjects() { return _vulkanObjects.get(); }
        const osg::Referenced* getVulkanObjects() const { return _vulkanObjects.get(); }

        virtual bool isSameKindAs(const osg::Object* object) const
        { return dynamic_cast<const GraphicsWindowSDL*>(object) != 0; }

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

    protected:
        virtual ~GraphicsWindowSDL();
        void initialize();

        osg::ref_ptr<osg::Referenced> _vulkanObjects;
        SDL_Window* _sdlWindow;
        void *_glContext, * _glDisplay, *_glSurface;
        int _lastKey, _lastModKey;
        bool _valid, _realized;
    };
}

#endif
