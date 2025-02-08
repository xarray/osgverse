#ifndef MANA_READERWRITER_VULKANEXTENSION_HPP
#define MANA_READERWRITER_VULKANEXTENSION_HPP

#include <vulkan/vulkan.h>
#include <osg/Referenced>

/* HOW TO MODIFY Google Angle to SUPPORT eglGetVkObjectsVERSE()
   - EDIT angle/scripts/egl_angle_ext.xml:
     - Add new extension:
       <extension name="EGL_VERSE_vulkan_objects" supported="egl">
           <require><command name="eglGetVkObjectsVERSE"/></require>
       </extension>
     - Add new command:
       <command>
           <proto><ptype>void *</ptype> <name>eglGetVkObjectsVERSE</name></proto>
           <param><ptype>EGLDisplay</ptype> <name>dpy</name></param>
           <param><ptype>EGLSurface</ptype> <name>surface</name></param>
       </command>
   - EDIT angle/scripts/registry_xml.py:
     - Add to the last of variable 'supported_egl_extensions':
       "EGL_VERSE_vulkan_objects",
   - RUN: python scripts/run_code_generation.py
   - EDIT src/libANGLE/Display.h: (namespace egl)
        void *GetVkObjectsFromDisplayVERSE(Display* display, Surface *surface);
   - EDIT src/libANGLE/Display.cpp: (namespace egl)
        void* GetVkObjectsFromDisplayVERSE(Display* display, Surface *surface) {
        #if defined(ANGLE_ENABLE_VULKAN)
            return rx::GetVulkanObjectsVERSE(display, surface);
        #else
            return nullptr;
        #endif
        }
   - EDIT src/libANGLE/renderer/vulkan/DisplayVk_api.h: (namespace rx)
        void *GetVulkanObjectsVERSE(egl::Display *display, egl::Surface *surface);
   - EDIT src/libANGLE/renderer/vulkan/SurfaceVk.h: (add to class WindowSurfaceVk)
        VkSurfaceKHR getSurface() const { return mSurface; }
        VkSwapchainKHR getSwapChain() const { return mSwapchain; }
   - EDIT src/libANGLE/renderer/vulkan/SurfaceVk.cpp: (namespace rx)
        struct VulkanObjectInfo { ... };  // must be same with declaration here
        void *GetVulkanObjectsVERSE(egl::Display *display, egl::Surface *surface) {
            DisplayVk *displayVk = vk::GetImpl(display);
            vk::Renderer *renderer = displayVk->getRenderer();
            const WindowSurfaceVk *windowSurface = GetImplAs<WindowSurfaceVk>(surface);
            VulkanObjectInfo* info = new VulkanObjectInfo();
            ...
            ///////// REFERENCE:
            info->instance = renderer->getInstance();
            info->physicsDevice = renderer->getPhysicalDevice();
            info->device = renderer->getDevice();
            for (int i = 0; i < 3; ++i) info->queues[i] = renderer->getQueue((egl::ContextPriority)i);
            info->surface = windowSurface->getSurface();
            info->swapChain = windowSurface->getSwapChain();
            /////////
            return info;
       }
   - EDIT include/EGL/eglext_angle.h:
       #ifndef EGL_VERSE_vulkan_objects
       #define EGL_VERSE_vulkan_objects
       typedef void* (EGLAPIENTRYP PFNEGLGETVKOBJECTSVERSEPROC)(EGLDisplay dpy, EGLSurface surface);
       #ifdef EGL_EGLEXT_PROTOTYPES
       EGLAPI void *EGLAPIENTRY eglGetVkObjectsVERSE(EGLDisplay dpy, EGLSurface surface);
       #endif
       #endif // EGL_VERSE_vulkan_objects
   - EDIT src/libGLESv2/egl_ext_stubs.cpp:
       void *GetVkObjectsVERSE(Thread *thread, egl::Display *display, SurfaceID surfaceID) {
           ANGLE_EGL_TRY_PREPARE_FOR_CALL_RETURN(thread, display->prepareForCall(),
                                                "eglGetVkObjectsVERSE", GetDisplayIfValid(display), nullptr);
            Surface *surface = display->getSurface(surfaceID);
            void *returnValue = GetVkObjectsFromDisplayVERSE(display, surface);
            thread->setSuccess(); return returnValue;
       }
   - EDIT libANGLE/validationEGL.cpp:
       bool ValidateGetVkObjectsVERSE(const ValidationContext *val, const egl::Display *dpy, SurfaceID sID) {
           ANGLE_VALIDATION_TRY(ValidateDisplay(val, dpy));
           ANGLE_VALIDATION_TRY(ValidateSurface(val, dpy, sID));
           if (!dpy->getExtensions().vulkanImageANGLE) {
               val->setError(EGL_BAD_ACCESS); return false;
           }
           return true;
       }
   - EDIT libGLESv2/egl_context_lock_impl.h:
       ANGLE_INLINE ScopedContextMutexLock GetContextLock_GetVkObjectsVERSE(Thread *thread, egl::Display *dpyPacked)
       { return TryLockCurrentContext(thread); }
  - Re-MAKE the project and copy result libraries to VERSE binary folder!
       ninja -j 10 -k1 -C out/Release
*/

struct VulkanObjectInfo
{
    VkInstance instance;
    VkPhysicalDevice physicsDevice;
    VkDevice device;
    VkQueue queues[3];

    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;
};

class VulkanManager : public osg::Referenced
{
public:
    VulkanManager(VulkanObjectInfo* info) : _infoData(info) {}

    void testValidation(void* procAddr)
    {
        if (!_infoData || !procAddr) return;
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)procAddr;
        PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)
            vkGetInstanceProcAddr(_infoData->instance, "vkGetPhysicalDeviceProperties");

        VkPhysicalDeviceProperties properties {};
        vkGetPhysicalDeviceProperties(_infoData->physicsDevice, &properties);
        OSG_NOTICE << "[VulkanManager] Get device data: " << properties.deviceName << "; "
                   << properties.driverVersion << ", " << properties.apiVersion << std::endl;
    }

protected:
    virtual ~VulkanManager() { if (_infoData != NULL) delete _infoData; }
    VulkanObjectInfo* _infoData;
};

#endif
