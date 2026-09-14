#include <osg/Notify>
#include "Export.h"

extern "C" OSGVERSE_RW_EXPORT void graphicswindow_GLFW(void)
{
    OSG_WARN << "[GraphicsWindowGLFW] No GLFW graphics window found. Maybe you are using OpenHarmony?" << std::endl;
}
