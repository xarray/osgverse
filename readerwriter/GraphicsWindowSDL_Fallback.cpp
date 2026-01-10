#include <osg/Notify>
#include "Export.h"

extern "C" OSGVERSE_RW_EXPORT void graphicswindow_SDL(void)
{
    OSG_WARN << "[GraphicsWindowSDL] No SDL graphics window found. Consider use GLFW as fallback" << std::endl;
}
