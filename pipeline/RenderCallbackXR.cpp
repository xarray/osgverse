#include <osg/io_utils>
#include <osg/Version>
#include <osg/FrameBufferObject>
#include <osg/RenderInfo>
#include <osg/GLExtensions>
#include <osg/PolygonMode>
#include <osg/Geode>
#if OSG_VERSION_GREATER_THAN(3, 5, 1)
    #include <osg/ContextData>
#endif
#include <osgDB/DynamicLibrary>
#include <sstream>
#include <iostream>

#if defined(__EMSCRIPTEN__)
#   define XR_DISABLED 1
#elif defined(__unix__) || defined(__linux__)
# if defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
#   include <EGL/egl.h>
#   define XR_USE_PLATFORM_EGL 1
//  XrGraphicsBindingEGLMNDX
# else
#   include <osgViewer/api/X11/GraphicsWindowX11>
#   define XR_USE_PLATFORM_XLIB 1
//  XrGraphicsBindingOpenGLXlibKHR

// TODO: XR_USE_PLATFORM_XCB (XrGraphicsBindingOpenGLXcbKHR)?
// TODO: XR_USE_PLATFORM_WAYLAND (XrGraphicsBindingOpenGLWaylandKHR)?
# endif
#elif defined(WIN32) || defined(_WIN32)
# if defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
#   include <EGL/egl.h>
#   define XR_USE_PLATFORM_EGL 1
//  XrGraphicsBindingEGLMNDX
# else
#   include <windows.h>
#   include <osgViewer/api/Win32/GraphicsWindowWin32>
#   define XR_USE_PLATFORM_WIN32 1
//  XrGraphicsBindingOpenGLWin32KHR
# endif
#elif defined(__ANDROID__)
#   define XR_USE_PLATFORM_ANDROID 1
//  XrGraphicsBindingOpenGLESAndroidKHR
#else
#   define XR_DISABLED 1
#endif

#if defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
#   define XR_USE_GRAPHICS_API_OPENGL_ES 1
#else
#   define XR_USE_GRAPHICS_API_OPENGL 1
#endif

#include "RenderCallbackXR.h"
#include "Utilities.h"
using namespace osgVerse;

#ifdef XR_DISABLED

RenderCallbackXR::RenderCallbackXR() : _beganFrame(false) {}
RenderCallbackXR::~RenderCallbackXR() {}
bool RenderCallbackXR::begin(osg::Matrix& view, osg::Matrix& projL, osg::Matrix& projR,
                             double znear, double zfar) { return false; }

void RenderCallbackXR::operator()(osg::RenderInfo& renderInfo) const
{
    OSG_NOTICE << "[RenderCallbackXR] Current platform is unsupported" << std::endl;
    if (_subCallback.valid()) _subCallback.get()->run(renderInfo);
}

void RenderCallbackXR::releaseGLObjects(osg::State* state) const
{ CameraDrawCallback::releaseGLObjects(state); }

#else

#include "3rdparty/openxr/openxr.h"
#include "3rdparty/openxr/openxr_platform.h"

struct LoaderXR : public osg::Referenced
{
    osg::ref_ptr<osgDB::DynamicLibrary> moduleHandle;
    PFN_xrGetInstanceProcAddr xrGetInstanceProcAddr = nullptr;
    PFN_xrEnumerateInstanceExtensionProperties xrEnumerateInstanceExtensionProperties = nullptr;
    PFN_xrCreateInstance xrCreateInstance = nullptr;
    PFN_xrDestroyInstance xrDestroyInstance = nullptr;
    PFN_xrGetInstanceProperties xrGetInstanceProperties = nullptr;
    PFN_xrGetSystem xrGetSystem = nullptr;
    PFN_xrGetSystemProperties xrGetSystemProperties = nullptr;
    PFN_xrEnumerateViewConfigurations xrEnumerateViewConfigurations = nullptr;
    PFN_xrEnumerateViewConfigurationViews xrEnumerateViewConfigurationViews = nullptr;
    PFN_xrCreateSession xrCreateSession = nullptr;
    PFN_xrDestroySession xrDestroySession = nullptr;
    PFN_xrBeginSession xrBeginSession = nullptr;
    PFN_xrEndSession xrEndSession = nullptr;
    PFN_xrRequestExitSession xrRequestExitSession = nullptr;
    PFN_xrWaitFrame xrWaitFrame = nullptr;
    PFN_xrBeginFrame xrBeginFrame = nullptr;
    PFN_xrEndFrame xrEndFrame = nullptr;
    PFN_xrLocateViews xrLocateViews = nullptr;
    PFN_xrCreateSwapchain xrCreateSwapchain = nullptr;
    PFN_xrDestroySwapchain xrDestroySwapchain = nullptr;
    PFN_xrEnumerateSwapchainImages xrEnumerateSwapchainImages = nullptr;
    PFN_xrEnumerateSwapchainFormats xrEnumerateSwapchainFormats = nullptr;
    PFN_xrAcquireSwapchainImage xrAcquireSwapchainImage = nullptr;
    PFN_xrWaitSwapchainImage xrWaitSwapchainImage = nullptr;
    PFN_xrReleaseSwapchainImage xrReleaseSwapchainImage = nullptr;
    PFN_xrCreateReferenceSpace xrCreateReferenceSpace = nullptr;
    PFN_xrDestroySpace xrDestroySpace = nullptr;
    PFN_xrPollEvent xrPollEvent = nullptr;
    PFN_xrResultToString xrResultToString = nullptr;
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
    PFN_xrGetOpenGLESGraphicsRequirementsKHR xrGetOpenGLESGraphicsRequirementsKHR = nullptr;
#else
    PFN_xrGetOpenGLGraphicsRequirementsKHR xrGetOpenGLGraphicsRequirementsKHR = nullptr;
#endif

    bool load()
    {
#ifdef _WIN32
        moduleHandle = osgDB::DynamicLibrary::loadLibrary("openxr_loader.dll");
#else
        moduleHandle = osgDB::DynamicLibrary::loadLibrary("libopenxr_loader.so");
#endif
        if (!moduleHandle)
        {
            OSG_WARN << "[RenderCallbackXR] Load 'openxr_loader' library failed" << std::endl;
            return false;
        }

        xrGetInstanceProcAddr = (PFN_xrGetInstanceProcAddr)moduleHandle->getProcAddress("xrGetInstanceProcAddr");
        xrCreateInstance = (PFN_xrCreateInstance)moduleHandle->getProcAddress("xrCreateInstance");
        return xrGetInstanceProcAddr && xrCreateInstance;
    }

    void loadInstanceFunctions(XrInstance instance)
    {
        PFN_xrVoidFunction func = nullptr;
#define REGISTER_XR_FUNCTION(type, var) \
        xrGetInstanceProcAddr(instance, #var, &func); var = (type)func; \
        if (var == nullptr) { OSG_NOTICE << "[RenderCallbackXR] Function not found: " #var << std::endl; }

        REGISTER_XR_FUNCTION(PFN_xrEnumerateInstanceExtensionProperties, xrEnumerateInstanceExtensionProperties);
        REGISTER_XR_FUNCTION(PFN_xrCreateInstance, xrCreateInstance);
        REGISTER_XR_FUNCTION(PFN_xrDestroyInstance, xrDestroyInstance);
        REGISTER_XR_FUNCTION(PFN_xrGetInstanceProperties, xrGetInstanceProperties);
        REGISTER_XR_FUNCTION(PFN_xrGetSystem, xrGetSystem);
        REGISTER_XR_FUNCTION(PFN_xrGetSystemProperties, xrGetSystemProperties);
        REGISTER_XR_FUNCTION(PFN_xrEnumerateViewConfigurations, xrEnumerateViewConfigurations);
        REGISTER_XR_FUNCTION(PFN_xrEnumerateViewConfigurationViews, xrEnumerateViewConfigurationViews);
        REGISTER_XR_FUNCTION(PFN_xrCreateSession, xrCreateSession);
        REGISTER_XR_FUNCTION(PFN_xrDestroySession, xrDestroySession);
        REGISTER_XR_FUNCTION(PFN_xrBeginSession, xrBeginSession);
        REGISTER_XR_FUNCTION(PFN_xrEndSession, xrEndSession);
        REGISTER_XR_FUNCTION(PFN_xrRequestExitSession, xrRequestExitSession);
        REGISTER_XR_FUNCTION(PFN_xrWaitFrame, xrWaitFrame);
        REGISTER_XR_FUNCTION(PFN_xrBeginFrame, xrBeginFrame);
        REGISTER_XR_FUNCTION(PFN_xrEndFrame, xrEndFrame);
        REGISTER_XR_FUNCTION(PFN_xrLocateViews, xrLocateViews);
        REGISTER_XR_FUNCTION(PFN_xrCreateSwapchain, xrCreateSwapchain);
        REGISTER_XR_FUNCTION(PFN_xrDestroySwapchain, xrDestroySwapchain);
        REGISTER_XR_FUNCTION(PFN_xrEnumerateSwapchainImages, xrEnumerateSwapchainImages);
        REGISTER_XR_FUNCTION(PFN_xrEnumerateSwapchainFormats, xrEnumerateSwapchainFormats);
        REGISTER_XR_FUNCTION(PFN_xrAcquireSwapchainImage, xrAcquireSwapchainImage);
        REGISTER_XR_FUNCTION(PFN_xrWaitSwapchainImage, xrWaitSwapchainImage);
        REGISTER_XR_FUNCTION(PFN_xrReleaseSwapchainImage, xrReleaseSwapchainImage);
        REGISTER_XR_FUNCTION(PFN_xrCreateReferenceSpace, xrCreateReferenceSpace);
        REGISTER_XR_FUNCTION(PFN_xrDestroySpace, xrDestroySpace);
        REGISTER_XR_FUNCTION(PFN_xrPollEvent, xrPollEvent);
        REGISTER_XR_FUNCTION(PFN_xrResultToString, xrResultToString);
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
        REGISTER_XR_FUNCTION(PFN_xrGetOpenGLESGraphicsRequirementsKHR, xrGetOpenGLESGraphicsRequirementsKHR);
#else
        REGISTER_XR_FUNCTION(PFN_xrGetOpenGLGraphicsRequirementsKHR, xrGetOpenGLGraphicsRequirementsKHR);
#endif
    }
};

struct SessionXR : public osg::Referenced
{
    XrInstance instance = XR_NULL_HANDLE;
    XrSwapchain swapchain = XR_NULL_HANDLE;
    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    XrSession session = XR_NULL_HANDLE;
    XrSpace referenceSpace = XR_NULL_HANDLE;
    XrViewConfigurationType viewConfigType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    XrTime frameLastTime;
    int recommendedWidth = 0, recommendedHeight = 0;

#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
    std::vector<XrSwapchainImageOpenGLESKHR> swapchainImages;
#else
    std::vector<XrSwapchainImageOpenGLKHR> swapchainImages;
#endif
    std::vector<XrView> views;
    uint32_t viewCount = 2;

    static std::string parseXrVersion(XrVersion version)
    {
        std::stringstream ss;
        ss << static_cast<uint16_t>((version >> 48) & 0xFFFF) << "."
           << static_cast<uint16_t>((version >> 32) & 0xFFFF) << "."
           << static_cast<uint32_t>(version & 0xFFFFFFFF);
        return ss.str();
    }

    static osg::Matrix poseToView(const XrPosef& pose)
    {
        osg::Vec3d p(pose.position.x, pose.position.y, pose.position.z);
        osg::Quat q(pose.orientation.x, pose.orientation.y, pose.orientation.z, pose.orientation.w);
        osg::Matrixd mat; mat.setTrans(p); mat.setRotate(q);
        return osg::Matrixd::inverse(mat);
    }

    static osg::Matrix fovToProjection(const XrFovf& fov, double nearZ, double farZ)
    {
        if (farZ <= nearZ)
        {
            float tanLeft = tanf(fov.angleLeft), tanRight = tanf(fov.angleRight);
            float tanUp = tanf(fov.angleUp), tanDown = tanf(fov.angleDown);
            float tanWidth = tanRight - tanLeft, tanHeight = tanUp - tanDown;

            osg::Matrix proj;
            proj(0, 0) = 2.0 / tanWidth; proj(1, 1) = 2.0 / tanHeight;
            proj(0, 2) = (tanRight + tanLeft) / tanWidth;
            proj(1, 2) = (tanUp + tanDown) / tanHeight;
            proj(2, 2) = -1.0; proj(2, 3) = -2.0 * nearZ;
            proj(3, 2) = -1.0; proj(3, 3) = 0.0; return proj;
        }
        return osg::Matrix::frustum(
            tanf(fov.angleLeft) * nearZ, tanf(fov.angleRight) * nearZ,
            tanf(fov.angleDown) * nearZ, tanf(fov.angleUp) * nearZ, nearZ, farZ);
    }

    bool createInstance(LoaderXR& loader)
    {
        const char* extensions[] =
        {
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
            XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME,
#else
            XR_KHR_OPENGL_ENABLE_EXTENSION_NAME,
#endif
            XR_EXT_HAND_TRACKING_EXTENSION_NAME,
        };

        XrApplicationInfo appInfo{};
        strcpy(appInfo.applicationName, "osgVerseXR");
        strcpy(appInfo.engineName, "osgVerse");
        appInfo.engineVersion = OSG_VERSION_MAJOR * 100 + OSG_VERSION_MINOR * 10 + OSG_VERSION_PATCH;
        appInfo.applicationVersion = 1;
        appInfo.apiVersion = XR_CURRENT_API_VERSION;

        XrInstanceCreateInfo createInfo{ XR_TYPE_INSTANCE_CREATE_INFO };
        createInfo.applicationInfo = appInfo;
        createInfo.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]);
        createInfo.enabledExtensionNames = extensions;

        XrResult result = loader.xrCreateInstance(&createInfo, &instance);
        if (XR_FAILED(result))
        {
            OSG_NOTICE << "[RenderCallbackXR] Failed to create OpenXR instance: " << result << std::endl;
            return false;
        }
        return true;
    }

    bool createViewSystem(LoaderXR& loader)
    {
        XrSystemGetInfo systemInfo{ XR_TYPE_SYSTEM_GET_INFO };
        systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

        XrResult result = loader.xrGetSystem(instance, &systemInfo, &systemId);
        if (XR_FAILED(result))
        {
            OSG_NOTICE << "[RenderCallbackXR] Failed to get OpenXR system: " << result << std::endl;
            return false;
        }

        // Get view config information
        uint32_t viewConfigCount = 0, viewCount = 0; bool foundStereo = false;
        loader.xrEnumerateViewConfigurations(instance, systemId, 0, &viewConfigCount, nullptr);
        std::vector<XrViewConfigurationType> viewConfigs(viewConfigCount);
        loader.xrEnumerateViewConfigurations(instance, systemId, viewConfigCount, &viewConfigCount, viewConfigs.data());

        viewConfigType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        for (auto config : viewConfigs)
        {
            if (config == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO)
            { foundStereo = true; break; }
        }

        if (!foundStereo)
        {
            OSG_NOTICE << "[RenderCallbackXR] No stereo view configuration found" << std::endl;
            return false;
        }

        // Get preferred view resolution
        loader.xrEnumerateViewConfigurationViews(instance, systemId, viewConfigType, 0, &viewCount, nullptr);
        std::vector<XrViewConfigurationView> configViews(viewCount);
        for (auto& view : configViews) view.type = XR_TYPE_VIEW_CONFIGURATION_VIEW;

        loader.xrEnumerateViewConfigurationViews(instance, systemId, viewConfigType,
                                                 viewCount, &viewCount, configViews.data());
        recommendedWidth = configViews[0].recommendedImageRectWidth;
        recommendedHeight = configViews[0].recommendedImageRectHeight;
        OSG_NOTICE << "[RenderCallbackXR] View count: " << viewCount << "; Recommended XR resolution: "
                   << recommendedWidth << "x" << recommendedHeight << std::endl;
        return true;
    }

    bool createSession(LoaderXR& loader, osg::GraphicsContext* gc)
    {
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
#   if defined(XR_USE_PLATFORM_ANDROID)
        // TODO
        XrGraphicsBindingOpenGLESAndroidKHR graphicsBinding{ XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR };
        OSG_WARN << "[RenderCallbackXR] createSession() not implemented\n"; return false;
#   else
        // TODO
        XrGraphicsBindingEGLMNDX graphicsBinding{ XR_TYPE_GRAPHICS_BINDING_EGL_MNDX };
        OSG_WARN << "[RenderCallbackXR] createSession() not implemented\n"; return false;
#   endif

        XrGraphicsRequirementsOpenGLESKHR requirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR };
        loader.xrGetOpenGLESGraphicsRequirementsKHR(instance, systemId, &requirements);
#else
#   if defined(WIN32) || defined(_WIN32)
        osgViewer::GraphicsWindowWin32* gw = dynamic_cast<osgViewer::GraphicsWindowWin32*>(gc);
        if (!gw) { OSG_NOTICE << "[RenderCallbackXR] No valid graphics window found\n"; return false; }

        XrGraphicsBindingOpenGLWin32KHR graphicsBinding
        { XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR, NULL, gw->getHDC(), gw->getWGLContext() };
#   else
        // TODO
        XrGraphicsBindingOpenGLXlibKHR graphicsBinding{ XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR };
        OSG_WARN << "[RenderCallbackXR] createSession() not implemented\n"; return false;
#   endif

        XrGraphicsRequirementsOpenGLKHR requirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
        loader.xrGetOpenGLGraphicsRequirementsKHR(instance, systemId, &requirements);
#endif

        XrSessionCreateInfo sessionCreateInfo{ XR_TYPE_SESSION_CREATE_INFO };
        sessionCreateInfo.systemId = systemId;
        sessionCreateInfo.next = &graphicsBinding;
        OSG_NOTICE << "[RenderCallbackXR] Requiring graphics API version: "
                   << "Minimum = " << parseXrVersion(requirements.minApiVersionSupported) << "; "
                   << "Maximum = " << parseXrVersion(requirements.maxApiVersionSupported) << std::endl;

        XrResult result = loader.xrCreateSession(instance, &sessionCreateInfo, &session);
        if (XR_FAILED(result))
        {
            OSG_NOTICE << "[RenderCallbackXR] Failed to create OpenXR session: " << result << std::endl;
            return false;
        }
        return true;
    }

    bool createReferenceSpace(LoaderXR& loader, const osg::Vec3& pos, const osg::Quat& q, int spaceType)
    {
        XrReferenceSpaceCreateInfo spaceCreateInfo{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
        spaceCreateInfo.referenceSpaceType = (XrReferenceSpaceType)spaceType;
        spaceCreateInfo.poseInReferenceSpace = { {(float)q[0], (float)q[1], (float)q[2], (float)q[3]},
                                                 {pos[0], pos[1], pos[2]}};

        XrResult result = loader.xrCreateReferenceSpace(session, &spaceCreateInfo, &referenceSpace);
        if (XR_FAILED(result))
        {
            OSG_NOTICE << "[RenderCallbackXR] Failed to create reference space: " << result << std::endl;
            return false;
        }
        return true;
    }

    bool createSwapchain(LoaderXR& loader)
    {
        XrSwapchainCreateInfo swapchainCreateInfo{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
        swapchainCreateInfo.format = GL_RGBA8;  // GL_SRGB8_ALPHA8
        swapchainCreateInfo.width = recommendedWidth * 2;  // our input should be a single-pass left+right image!
        swapchainCreateInfo.height = recommendedHeight;
        swapchainCreateInfo.mipCount = 1; swapchainCreateInfo.faceCount = 1;
        swapchainCreateInfo.sampleCount = 1; swapchainCreateInfo.arraySize = 1;
        swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;

        XrResult result = loader.xrCreateSwapchain(session, &swapchainCreateInfo, &swapchain);
        if (XR_FAILED(result))
        {
            OSG_NOTICE << "[RenderCallbackXR] Failed to create swapchain: " << result << std::endl;
            return false;
        }

        uint32_t imageCount = 0;
        loader.xrEnumerateSwapchainImages(swapchain, 0, &imageCount, nullptr);
        swapchainImages.resize(imageCount);
        for (auto& img : swapchainImages)
        {
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
            img.type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
#else
            img.type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
#endif
        }

        loader.xrEnumerateSwapchainImages(
            swapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)swapchainImages.data());
        OSG_NOTICE << "[RenderCallbackXR] Created swapchain with " << imageCount << " images" << std::endl;
        views.resize(viewCount); return true;
    }

    void destroy(const LoaderXR& loader) const
    {
        if (swapchain != XR_NULL_HANDLE) loader.xrDestroySwapchain(swapchain);
        if (referenceSpace != XR_NULL_HANDLE) loader.xrDestroySpace(referenceSpace);
        if (session != XR_NULL_HANDLE) loader.xrDestroySession(session);
        if (instance != XR_NULL_HANDLE) loader.xrDestroyInstance(instance);
    }

#if OSG_VERSION_GREATER_THAN(3, 3, 2)
    void blitToSwapchain(osg::State* state, osg::GLExtensions* ext, GLuint dstTexture)
#else
    void blitToSwapchain(osg::State* state, osg::FBOExtensions* ext, GLuint dstTexture)
#endif
    {
        // TODO
    }
};

RenderCallbackXR::RenderCallbackXR()
:   _beginFrameTick(0), _spaceType(LOCAL), _beganFrame(false)
{
    osg::ref_ptr<LoaderXR> loader = new LoaderXR;
    if (loader->load()) _xrLoader = loader; else return;

    osg::ref_ptr<SessionXR> xr = new SessionXR;
    if (xr->createInstance(*loader))
    {
        loader->loadInstanceFunctions(xr->instance);
        if (xr->createViewSystem(*loader)) _xrSession = xr;
        else xr->destroy(*loader);
    }
}

RenderCallbackXR::~RenderCallbackXR()
{
    if (_xrLoader.valid() && _xrSession.valid())
    {
        SessionXR* xr = static_cast<SessionXR*>(_xrSession.get());
        xr->destroy(static_cast<LoaderXR&>(*_xrLoader));
    }
}

bool RenderCallbackXR::begin(osg::Matrix& view, osg::Matrix& projL, osg::Matrix& projR, double znear, double zfar)
{
    LoaderXR* loader = static_cast<LoaderXR*>(_xrLoader.get());
    SessionXR* xr = static_cast<SessionXR*>(_xrSession.get());
    if (loader && xr)
    {
        if (!_beganFrame && xr->session)
        {
            XrFrameWaitInfo waitInfo{ XR_TYPE_FRAME_WAIT_INFO };
            XrFrameState frameState{ XR_TYPE_FRAME_STATE };
            loader->xrWaitFrame(xr->session, &waitInfo, &frameState);
            if (frameState.shouldRender)
            {
                // Start frame
                uint32_t viewCountOutput = 0;
                loader->xrBeginFrame(xr->session, nullptr);
                _beginFrameTick = osg::Timer::instance()->tick();
                xr->frameLastTime = frameState.predictedDisplayTime;

                // Get view location
                XrViewLocateInfo viewLocateInfo{ XR_TYPE_VIEW_LOCATE_INFO };
                viewLocateInfo.viewConfigurationType = xr->viewConfigType;
                viewLocateInfo.displayTime = frameState.predictedDisplayTime;
                viewLocateInfo.space = xr->referenceSpace;

                XrViewState viewState{ XR_TYPE_VIEW_STATE };
                loader->xrLocateViews(xr->session, &viewLocateInfo, &viewState,
                                      xr->viewCount, &viewCountOutput, xr->views.data());

                // Compute OSG view & projection matrices
                const XrView& left = xr->views.front();
                const XrView& right = xr->views.back();
                osg::Matrix leftProj = xr->fovToProjection(left.fov, znear, zfar);
                osg::Matrix rightProj = xr->fovToProjection(right.fov, znear, zfar);

                osg::Vec3d leftPos(left.pose.position.x, left.pose.position.y, left.pose.position.z);
                osg::Vec3d rightPos(right.pose.position.x, right.pose.position.y, right.pose.position.z);
                osg::Vec3d ipdOffset = rightPos - leftPos;
                osg::Matrix ipdMatrix; ipdMatrix.makeTranslate(-ipdOffset);
                osg::Matrix rightProjAdjusted = ipdMatrix * rightProj;

                view = xr->poseToView(left.pose);
                projL = leftProj; projR = rightProjAdjusted;
                _beganFrame = true; return true;
            }
        }
        else if (_beganFrame)
            { OSG_NOTICE << "[RenderCallbackXR] Current frame already began, should call xrEndFrame()\n"; }
    }
    return false;
}

void RenderCallbackXR::operator()(osg::RenderInfo& renderInfo) const
{
    osg::State* state = renderInfo.getState();
#if OSG_VERSION_GREATER_THAN(3, 3, 2)
    osg::GLExtensions* ext = state->get<osg::GLExtensions>();
    if (!ext->isFrameBufferObjectSupported)
#else
    osg::FBOExtensions* ext = osg::FBOExtensions::instance(renderInfo.getContextID(), true);
    if (!ext->isSupported())
#endif
    {
        OSG_WARN << "[RenderCallbackXR] No FBO support" << std::endl;
        if (_subCallback.valid()) _subCallback.get()->run(renderInfo); return;
    }

    LoaderXR* loader = static_cast<LoaderXR*>(_xrLoader.get());
    SessionXR* xr = static_cast<SessionXR*>(_xrSession.get());
    if (loader && xr)
    {
        // Initialize current session
        if (!xr->session)
        {
            if (xr->createSession(*loader, state->getGraphicsContext()))
            {
                xr->createReferenceSpace(*loader, _spacePosition, _spaceOrientation, (int)_spaceType);
                xr->createSwapchain(*loader);
            }
        }

        // Finish XR rendering
        uint32_t imageIndex = 0;
        if (xr->session && _beganFrame)
        {
            // Update swapchain images from single FBO input
            XrSwapchainImageAcquireInfo acquireInfo{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
            loader->xrAcquireSwapchainImage(xr->swapchain, &acquireInfo, &imageIndex);
            {
                // Blit FBO to each eye's image
                GLuint destFramebuffer = xr->swapchainImages[imageIndex].image;
                xr->blitToSwapchain(state, ext, destFramebuffer);
            }
            XrSwapchainImageReleaseInfo releaseInfo{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
            loader->xrReleaseSwapchainImage(xr->swapchain, &releaseInfo);

            // Configure projection view of each eye
            std::vector<XrCompositionLayerProjectionView> projectionViews;
            projectionViews.resize(xr->viewCount);
            for (uint32_t eye = 0; eye < xr->viewCount; ++eye)
            {
                XrCompositionLayerProjectionView& projectionView = projectionViews[eye];
                projectionView.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
                projectionView.pose = xr->views[eye].pose;
                projectionView.fov = xr->views[eye].fov;
                projectionView.next = nullptr;
                projectionView.subImage.swapchain = xr->swapchain;
                projectionView.subImage.imageRect.offset =
                    { (eye == 0 ? 0 : (int32_t)xr->recommendedWidth), 0 };
                projectionView.subImage.imageRect.extent =
                    { (int32_t)xr->recommendedWidth, (int32_t)xr->recommendedHeight };
                projectionView.subImage.imageArrayIndex = 0;
            }

            // Last composition layer
            XrCompositionLayerProjection projectionLayer{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };
            projectionLayer.space = xr->referenceSpace;
            projectionLayer.viewCount = static_cast<uint32_t>(projectionViews.size());
            projectionLayer.views = projectionViews.data();

            const XrCompositionLayerBaseHeader* layers[] =
            { reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projectionLayer) };
            osg::Timer_t delta = osg::Timer::instance()->tick() - _beginFrameTick;

            // End frame and present all layers
            XrFrameEndInfo endInfo{ XR_TYPE_FRAME_END_INFO };
            endInfo.displayTime = xr->frameLastTime + delta;
            endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
            endInfo.layerCount = 1; endInfo.layers = layers;
            loader->xrEndFrame(xr->session, &endInfo);
            _beganFrame = false;
        }
        else if (!_beganFrame)
            { OSG_NOTICE << "[RenderCallbackXR] Current frame not ready, should call begin() first\n"; }
    }
    if (_subCallback.valid()) _subCallback.get()->run(renderInfo);
}

void RenderCallbackXR::releaseGLObjects(osg::State* state) const
{
    if (_xrLoader.valid() && _xrSession.valid())
    {
        const SessionXR* xr = static_cast<const SessionXR*>(_xrSession.get());
        xr->destroy(static_cast<const LoaderXR&>(*_xrLoader));
        const_cast<RenderCallbackXR*>(this)->_xrSession = NULL;
    }
    CameraDrawCallback::releaseGLObjects(state);
}

#endif
