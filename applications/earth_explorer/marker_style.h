#ifndef EARTH_MARKER_STYLE_H
#define EARTH_MARKER_STYLE_H
#include <string>
#include <osg/Vec4>

// Single source of truth for the marker system (vision-enhanced): shape enum + per-layer
// representative-color lookup + shared GLSL shape library + ImGui shape drawing. The earth
// marker shader, layer catalog icons and detail-card chips all read from here -> change one place,
// three stay in sync.
// Red line: this module only defines "what it looks like"; it never touches data/thread/picking/registration logic.
namespace earthmark
{
    enum class MarkerShape { Circle=0, Arrow=1, Ship=2, StarBurst=3, Ring=4,
                             WarnTriangle=5, Diamond=6, Hexagon=7, Teardrop=8, SatBox=9, Square=10 };
    struct MarkerVisual { MarkerShape shape; osg::Vec4 color; };   // color = representative color
    // Look up the representative badge by layer id; unregistered falls back to {Circle, neutral gray}.
    MarkerVisual visualForLayer(const std::string& layerId);
    // Shared GLSL shape library (spliced into each point-sprite FS). Provides
    // float markerCoverage(int,vec2,float), returning coverage 0..1 (0 = outside the shape, should
    // discard; edges anti-aliased with smoothstep). pc=gl_PointCoord.
    const char* markerShapeGLSL();
    // Draw the corresponding shape on the ImGui side (layer catalog icon / detail card chip).
    // imDrawList is passed as ImDrawList* (void* to avoid header dependency);
    // (cx,cy)=center screen coordinates, sizePx=bounding size, colorU32=ImU32.
    void drawMarkerIcon(void* imDrawList, MarkerShape shape, float cx, float cy, float sizePx,
                        unsigned int colorU32);
}
#endif
