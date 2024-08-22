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
       </command>
   - EDIT angle/scripts/registry_xml.py:
     - Add to the last of variable 'supported_egl_extensions':
       "EGL_VERSE_vulkan_objects",
   - RUN: python scripts/run_code_generation.py
   - EDIT src/libANGLE/Display.h:
        void *GetVkObjectsFromDisplayVERSE(Display* display);
   - EDIT src/libANGLE/Display.cpp:
        void* GetVkObjectsFromDisplayVERSE(Display* display) {
        #if defined(ANGLE_ENABLE_VULKAN)
            return rx::GetVulkanObjectsVERSE(display);
        #else
            return nullptr;
        #endif
        }
   - EDIT src/libANGLE/renderer/vulkan/DisplayVk_api.h:
        void *GetVulkanObjectsVERSE(egl::Display *display);
   - EDIT src/libANGLE/renderer/vulkan/DisplayVk.cpp:
        struct VulkanObjectInfo { ... };  // must be same with declaration here
        void *GetVulkanObjectsVERSE(egl::Display *display) {
            DisplayVk *displayVk = vk::GetImpl(display);
            vk::Renderer *renderer = displayVk->getRenderer();
            VulkanObjectInfo* info = new VulkanObjectInfo();
            ...
            return info;
       }
   - EDIT include/EGL/eglext_angle.h:
       #ifndef EGL_VERSE_vulkan_objects
       #define EGL_VERSE_vulkan_objects
       typedef void* (EGLAPIENTRYP PFNEGLGETVKOBJECTSVERSEPROC)(EGLDisplay dpy);
       #ifdef EGL_EGLEXT_PROTOTYPES
       EGLAPI void *EGLAPIENTRY eglGetVkObjectsVERSE(EGLDisplay dpy);
       #endif
       #endif // EGL_VERSE_vulkan_objects
   - EDIT src/libGLESv2/egl_ext_stubs.cpp:
       void *GetVkObjectsVERSE(Thread *thread, egl::Display *display) {
           ANGLE_EGL_TRY_RETURN(thread, display->prepareForCall(), "eglGetVkObjectsVERSE",
                                GetDisplayIfValid(display), EGL_FALSE);
           void *returnValue = GetVkObjectsFromDisplayVERSE(display);
           thread->setSuccess(); return returnValue;
       }
   - EDIT libANGLE/validationEGL.cpp:
       bool ValidateGetVkObjectsVERSE(const ValidationContext *val, const egl::Display *dpy) {
           ANGLE_VALIDATION_TRY(ValidateDisplay(val, dpy));
           if (!dpy->getExtensions().vulkanImageANGLE) {
               val->setError(EGL_BAD_ACCESS); return false;
           }
           return true;
       }
   - EDIT libGLESv2/egl_context_lock_impl.h:
       ANGLE_INLINE ScopedContextMutexLock GetContextLock_GetVkObjectsVERSE(Thread *thread, egl::Display *dpyPacked)
       { return TryLockCurrentContext(thread); }
  - Re-MAKE the project and copy result libraries to VERSE binary folder!
*/

struct VulkanObjectInfo
{
    VkInstance instance;
    VkPhysicalDevice physicsDevice;
    VkDevice device;
};

class VulkanManager : public osg::Referenced
{
public:
    VulkanManager(VulkanObjectInfo* info) : _infoData(info) {}

protected:
    virtual ~VulkanManager() { if (_infoData != NULL) delete _infoData; }
    VulkanObjectInfo* _infoData;
};

#endif
