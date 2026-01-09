#include "GraphicsWindowWin32NV.h"
#include <osg/DeleteHandler>
#include <osgViewer/View>
#include <windowsx.h>
#include <iostream>
#include <sstream>
using namespace osgVerse;

/// https://www.opengl.org/registry/specs/NV/gpu_affinity.txt
typedef void* HGPUNV;
typedef struct _GPU_DEVICE
{
    DWORD cb;
    CHAR DeviceName[32];
    CHAR DeviceString[128];
    DWORD Flags;
    RECT rcVirtualScreen;
} GPU_DEVICE, *PGPU_DEVICE;
typedef BOOL(*PFNWGLENUMGPUSNV)(UINT, HGPUNV*);
typedef BOOL(*PFNWGLENUMGPUDEVICESNV)(HGPUNV, UINT, PGPU_DEVICE);
typedef HDC(*PFNWGLCREATEAFFINITYDCNV)(const HGPUNV*);
typedef BOOL(*PFNWGLENUMGPUSFROMAFFINITYDCNV)(HDC, UINT, HGPUNV*);
typedef BOOL(*PFNWGLDELETEDCNV)(HDC);
////////////////////////////////////////

static PFNWGLENUMGPUSNV wglEnumGpusNV = NULL;
static PFNWGLENUMGPUDEVICESNV wglEnumGpuDevicesNV = NULL;
static PFNWGLCREATEAFFINITYDCNV wglCreateAffinityDCNV = NULL;
static PFNWGLENUMGPUSFROMAFFINITYDCNV wglEnumGpusFromAffinityDCNV = NULL;
static PFNWGLDELETEDCNV wglDeleteDCNV = NULL;

namespace
{
    class Win32WindowingSystemNV : public osg::GraphicsContext::WindowingSystemInterface
    {
    public:
        typedef std::vector<DISPLAY_DEVICE> DisplayDevices;
        typedef std::map<HWND, osgViewer::GraphicsWindowWin32*> WindowHandles;
        typedef std::pair<HWND, osgViewer::GraphicsWindowWin32*> WindowHandleEntry;
        static std::string osgGraphicsWindowWithCursorClass;
        static std::string osgGraphicsWindowWithoutCursorClass;

        Win32WindowingSystemNV()
        {
            _windowClassesRegistered = false;
            getInterface() = this;
        }

        void registerWindow(HWND hwnd, osgViewer::GraphicsWindowWin32* window)
        { if (hwnd) _activeWindows.insert(WindowHandleEntry(hwnd, window)); }

        void unregisterWindow(HWND hwnd)
        { if (hwnd) _activeWindows.erase(hwnd); }

        osgViewer::GraphicsWindowWin32* getGraphicsWindowFor(HWND hwnd)
        {
            WindowHandles::const_iterator entry = _activeWindows.find(hwnd);
            return entry == _activeWindows.end() ? 0 : entry->second;
        }

        static osg::observer_ptr<Win32WindowingSystemNV>& getInterface()
        {
            static osg::observer_ptr<Win32WindowingSystemNV> s_win32Interface;
            return s_win32Interface;
        }

        virtual unsigned int getNumScreens(const osg::GraphicsContext::ScreenIdentifier& si =
                                           osg::GraphicsContext::ScreenIdentifier())
        { return si.displayNum == 0 ? ::GetSystemMetrics(SM_CMONITORS) : 0; }

        virtual void getScreenSettings(const osg::GraphicsContext::ScreenIdentifier& si,
                                       osg::GraphicsContext::ScreenSettings& resolution)
        {
            DISPLAY_DEVICE displayDevice; DEVMODE deviceMode;
            if (!getScreenInformation(si, displayDevice, deviceMode))
                deviceMode.dmFields = 0;        // Set the fields to 0 so that it says 'nothing'.
            if ((deviceMode.dmFields & (DM_PELSWIDTH | DM_PELSHEIGHT)) != 0)  // Get resolution
            {
                resolution.width = deviceMode.dmPelsWidth;
                resolution.height = deviceMode.dmPelsHeight;
            }
            else
                { resolution.width = 0; resolution.height = 0; }

            if ((deviceMode.dmFields & DM_DISPLAYFREQUENCY) != 0)  // Get refersh rate
            {
                resolution.refreshRate = deviceMode.dmDisplayFrequency;
                if (resolution.refreshRate == 0 || resolution.refreshRate == 1)
                    resolution.refreshRate = 0;  // Windows specific: 0 and 1 represent the hardware's default refresh rate
            }
            else
                resolution.refreshRate = 0;

            if ((deviceMode.dmFields & DM_BITSPERPEL) != 0) resolution.colorDepth = deviceMode.dmBitsPerPel;
            else resolution.colorDepth = 0;  // Get bits per pixel for color buffer
        }

        virtual bool setScreenSettings(const osg::GraphicsContext::ScreenIdentifier& si,
                                       const osg::GraphicsContext::ScreenSettings& resolution)
        {
            OSG_WARN << "[Win32WindowingSystemNV] setScreenSettings() not implemented" << std::endl;
            return false;
        }

        bool getScreenInformation(const osg::GraphicsContext::ScreenIdentifier& si,
                                  DISPLAY_DEVICE& displayDevice, DEVMODE& deviceMode)
        {
            if (si.displayNum > 0) return false;
            DisplayDevices displayDevices; enumerateDisplayDevices(displayDevices);
            if (si.screenNum >= static_cast<int>(displayDevices.size())) return false;

            displayDevice = displayDevices[si.screenNum];
            deviceMode.dmSize = sizeof(deviceMode); deviceMode.dmDriverExtra = 0;
            if (!::EnumDisplaySettings(displayDevice.DeviceName, ENUM_CURRENT_SETTINGS, &deviceMode)) return false;
            return true;
        }

        virtual void enumerateScreenSettings(const osg::GraphicsContext::ScreenIdentifier& si,
                                             osg::GraphicsContext::ScreenSettingsList& resolutionList)
        {
            resolutionList.clear();
            if (si.displayNum > 0) return;

            DisplayDevices displayDevices;
            enumerateDisplayDevices(displayDevices);
            if (si.screenNum >= static_cast<int>(displayDevices.size())) return;

            DISPLAY_DEVICE displayDevice = displayDevices[si.screenNum]; DEVMODE deviceMode;
            static const unsigned int MAX_RESOLUTIONS = 4046;
            for (unsigned int i = 0; i < MAX_RESOLUTIONS; ++i)
            {
                if (!::EnumDisplaySettings(displayDevice.DeviceName, i, &deviceMode)) break;
                deviceMode.dmSize = sizeof(deviceMode); deviceMode.dmDriverExtra = 0;
                resolutionList.push_back(osg::GraphicsContext::ScreenSettings(
                    deviceMode.dmPelsWidth, deviceMode.dmPelsHeight, deviceMode.dmDisplayFrequency, deviceMode.dmBitsPerPel));
            }
        }

        virtual osg::GraphicsContext* createGraphicsContext(osg::GraphicsContext::Traits* traits)
        {
            registerWindowClasses();
            return new GraphicsWindowWin32NV(traits);
        }

    protected:
        virtual ~Win32WindowingSystemNV()
        {
            if (osg::Referenced::getDeleteHandler())
            {
                osg::Referenced::getDeleteHandler()->setNumFramesToRetainObjects(0);
                osg::Referenced::getDeleteHandler()->flushAll();
            }
            unregisterWindowClasses();
        }

        void enumerateDisplayDevices(DisplayDevices& displayDevices) const
        {
            for (unsigned int deviceNum = 0;; ++deviceNum)
            {
                DISPLAY_DEVICE displayDevice; displayDevice.cb = sizeof(displayDevice);
                if (!::EnumDisplayDevices(NULL, deviceNum, &displayDevice, 0)) break;

                if (displayDevice.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) continue;
                if (!(displayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) continue;
                displayDevices.push_back(displayDevice);
            }
        }

        void registerWindowClasses();
        void unregisterWindowClasses();
        WindowHandles _activeWindows;
        bool _windowClassesRegistered;
    };
}

std::string Win32WindowingSystemNV::osgGraphicsWindowWithCursorClass;
std::string Win32WindowingSystemNV::osgGraphicsWindowWithoutCursorClass;
static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    osgViewer::GraphicsWindowWin32* window = Win32WindowingSystemNV::getInterface()->getGraphicsWindowFor(hwnd);
    return window ? window->handleNativeWindowingEvent(hwnd, uMsg, wParam, lParam) :
                    ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void Win32WindowingSystemNV::registerWindowClasses()
{
    std::ostringstream str; HINSTANCE hinst = ::GetModuleHandle(NULL);
    if (_windowClassesRegistered) return;
    str << "OSG Graphics Window for Win32NV [" << ::GetCurrentProcessId() << "]";
    osgGraphicsWindowWithCursorClass = str.str() + "{ with cursor }";
    osgGraphicsWindowWithoutCursorClass = str.str() + "{ without cursor }";

    WNDCLASSEX wc; wc.cbSize = sizeof(wc);
    wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WindowProc; wc.cbClsExtra = 0; wc.cbWndExtra = 0;
    wc.hInstance = hinst; wc.hIcon = ::LoadIcon(hinst, "OSG_ICON");
    wc.hCursor = ::LoadCursor(NULL, IDC_ARROW); wc.hIconSm = NULL;
    wc.hbrBackground = NULL; wc.lpszMenuName = 0;
    wc.lpszClassName = osgGraphicsWindowWithCursorClass.c_str();
    if (::RegisterClassEx(&wc) == 0)
    {
        unsigned int lastError = ::GetLastError();
        if (lastError != ERROR_CLASS_ALREADY_EXISTS)
        {
            OSG_WARN << "[Win32WindowingSystemNV] Unable to register first window class: "
                     << lastError << std::endl; return;
        }
    }

    wc.hCursor = NULL; wc.lpszClassName = osgGraphicsWindowWithoutCursorClass.c_str();
    if (::RegisterClassEx(&wc) == 0)
    {
        unsigned int lastError = ::GetLastError();
        if (lastError != ERROR_CLASS_ALREADY_EXISTS)
        {
            OSG_WARN << "[Win32WindowingSystemNV] Unable to register second window class"
                     << lastError << std::endl; return;
        }
    }
    _windowClassesRegistered = true;
}

void Win32WindowingSystemNV::unregisterWindowClasses()
{
    if (_windowClassesRegistered)
    {
        ::UnregisterClass(osgGraphicsWindowWithCursorClass.c_str(), ::GetModuleHandle(NULL));
        ::UnregisterClass(osgGraphicsWindowWithoutCursorClass.c_str(), ::GetModuleHandle(NULL));
        _windowClassesRegistered = false;
    }
}

///////////////// GraphicsWindowWin32NV ////////////////////

GraphicsWindowWin32NV::GraphicsWindowWin32NV(osg::GraphicsContext::Traits* traits)
    : osgViewer::GraphicsWindowWin32(traits), _dcCreatedForSpecGPU(false)
{
    wglEnumGpusNV = (PFNWGLENUMGPUSNV)wglGetProcAddress("wglEnumGpusNV");
    wglEnumGpuDevicesNV = (PFNWGLENUMGPUDEVICESNV)wglGetProcAddress("wglEnumGpuDevicesNV");
    wglCreateAffinityDCNV = (PFNWGLCREATEAFFINITYDCNV)wglGetProcAddress("wglCreateAffinityDCNV");
    wglEnumGpusFromAffinityDCNV = (PFNWGLENUMGPUSFROMAFFINITYDCNV)wglGetProcAddress("wglEnumGpusFromAffinityDCNV");
    wglDeleteDCNV = (PFNWGLDELETEDCNV)wglGetProcAddress("wglDeleteDCNV");
}

GraphicsWindowWin32NV::~GraphicsWindowWin32NV()
{}

bool GraphicsWindowWin32NV::createWindow()
{
    unsigned int extendedStyle = 0, windowStyle = 0;
    if (!determineWindowPositionAndStyle(_traits->screenNum,
        _traits->x, _traits->y, _traits->width, _traits->height,
        _traits->windowDecoration, _windowOriginXToRealize, _windowOriginYToRealize,
        _windowWidthToRealize, _windowHeightToRealize, windowStyle, extendedStyle))
    {
        OSG_WARN << "[GraphicsWindowWin32NV] Unable to determine the window position and style: "
                 << ::GetLastError() << std::endl; return false;
    }

    _hwnd = ::CreateWindowEx(extendedStyle,
        _traits->useCursor ? Win32WindowingSystemNV::osgGraphicsWindowWithCursorClass.c_str() :
                             Win32WindowingSystemNV::osgGraphicsWindowWithoutCursorClass.c_str(),
        _traits->windowName.c_str(), windowStyle,
        _windowOriginXToRealize, _windowOriginYToRealize,
        _windowWidthToRealize, _windowHeightToRealize,
        NULL, NULL, ::GetModuleHandle(NULL), NULL);
    if (_hwnd == 0)
    {
        OSG_WARN << "[GraphicsWindowWin32NV] Unable to create window: "
                 << ::GetLastError() << std::endl; return false;
    }

    WindowDataNV* wData = _traits.valid()
                        ? dynamic_cast<WindowDataNV*>(_traits->inheritedWindowData.get()) : NULL;
    _dcCreatedForSpecGPU = false;
    if (wData != NULL && wglCreateAffinityDCNV != NULL)
    {
        std::vector<HGPUNV> gpuHandles;
        if (wglEnumGpusNV != NULL)
        {
            int gpuIndex = 0; bool hasGPU = false;
            do
            {
                HGPUNV gpuHandle = NULL;
                hasGPU = wglEnumGpusNV(gpuIndex++, &gpuHandle);
                if (hasGPU) gpuHandles.push_back(gpuHandle);
            } while (hasGPU);
        }

        int selectedGPU = wData->selectedGPU; _hdc = 0;
        if (selectedGPU >= 0 && selectedGPU < (int)gpuHandles.size())
        {
            HGPUNV gpuMask[2] = { gpuHandles[selectedGPU], NULL };
            _hdc = wglCreateAffinityDCNV(gpuMask);
            _dcCreatedForSpecGPU = true;
        }
    }

    if (_hdc == 0)
        _hdc = ::GetDC(_hwnd);
    if (_hdc == 0)
    {
        OSG_WARN << "[GraphicsWindowWin32NV] Unable to get window device context: "
                 << ::GetLastError() << std::endl;
        destroyWindow(); _hwnd = 0; return false;
    }

    if (!setPixelFormat())
    {
        if (_dcCreatedForSpecGPU && wglDeleteDCNV != NULL) wglDeleteDCNV(_hdc);
        else ::ReleaseDC(_hwnd, _hdc); _hdc = 0;
        destroyWindow(); return false;
    }

    _hglrc = createContextImplementation();
    if (_hglrc == 0)
    {
        OSG_WARN << "[GraphicsWindowWin32NV] Unable to create OpenGL rendering context: "
                 << ::GetLastError() << std::endl;
        if (_dcCreatedForSpecGPU && wglDeleteDCNV != NULL) wglDeleteDCNV(_hdc);
        else ::ReleaseDC(_hwnd, _hdc); _hdc = 0;
        destroyWindow(); return false;
    }

    Win32WindowingSystemNV::getInterface()->registerWindow(_hwnd, this);
    return true;
}

void GraphicsWindowWin32NV::destroyWindow(bool deleteNativeWindow)
{
    if (_destroying) return; _destroying = true;
    if (_graphicsThread && _graphicsThread->isRunning())
    {   // find all the viewers that might own use this graphics context
        osg::GraphicsContext::Cameras cameras = getCameras();
        for (osg::GraphicsContext::Cameras::iterator it = cameras.begin(); it != cameras.end(); ++it)
        {
            osgViewer::View* view = dynamic_cast<osgViewer::View*>((*it)->getView());
            osgViewer::ViewerBase* viewerBase = view ? view->getViewerBase() : 0;
            if (viewerBase && viewerBase->areThreadsRunning())
                viewerBase->stopThreading();
        }
    }

    if (_hdc)
    {
        releaseContext();
#ifdef OSG_USE_EGL
        //
#else
        if (_hglrc) { ::wglDeleteContext(_hglrc); _hglrc = 0; }
#endif
        if (_dcCreatedForSpecGPU && wglDeleteDCNV != NULL) wglDeleteDCNV(_hdc);
        else ::ReleaseDC(_hwnd, _hdc); _hdc = 0;
    }

    unregisterWindowProcedure();
    if (_hwnd)
    {
        Win32WindowingSystemNV::getInterface()->unregisterWindow(_hwnd);
        if (_ownsWindow && deleteNativeWindow) ::DestroyWindow(_hwnd);
        _hwnd = 0;
    }
    _initialized = false; _realized = false;
    _valid = false; _destroying = false;
}

extern "C" OSGVERSE_RW_EXPORT void graphicswindow_Win32NV(void) {}
#if OSG_VERSION_GREATER_THAN(3, 5, 1)
static osg::WindowingSystemInterfaceProxy<Win32WindowingSystemNV> s_proxy_Win32WindowingSystemNV("Win32NV");
#endif
