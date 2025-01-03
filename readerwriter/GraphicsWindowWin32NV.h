#ifndef MANA_READERWRITER_GRAPHICSWINDOWWIN32NV_HPP
#define MANA_READERWRITER_GRAPHICSWINDOWWIN32NV_HPP

#include <osg/Version>
#include <osgDB/ReaderWriter>
#include <osgViewer/api/Win32/GraphicsWindowWin32>
#include "Export.h"

struct SDL_Window;

namespace osgVerse
{
    class OSGVERSE_RW_EXPORT GraphicsWindowWin32NV : public osgViewer::GraphicsWindowWin32
    {
    public:
        struct WindowDataNV : public osgViewer::GraphicsWindowWin32::WindowData
        {
            WindowDataNV(HWND window, int gpu = 0, bool installEventHandler = true)
                : osgViewer::GraphicsWindowWin32::WindowData(window, installEventHandler)
            { selectedGPU = gpu; }

            int selectedGPU;
        };

        GraphicsWindowWin32NV(osg::GraphicsContext::Traits* traits);
        virtual const char* libraryName() const { return "osgVerse"; }
        virtual const char* className() const { return "GraphicsWindowWin32NV"; }

        virtual bool createWindow();
        virtual void destroyWindow(bool deleteNativeWindow = true);

    protected:
        virtual ~GraphicsWindowWin32NV();
        bool _dcCreatedForSpecGPU;
    };
}

#endif
