#ifndef EARTH_MARKER_STYLE_H
#define EARTH_MARKER_STYLE_H
#include <string>
#include <osg/Vec4>

// 标记系统单一真源(vision 加强版):形状枚举 + 每层代表色查表 + 共享 GLSL 形状库 +
// ImGui 形状绘制。地球标记着色器、图层目录 icon、详情卡 chip 都读这里 → 改一处三处同步。
// 红线:本模块只负责"长什么样",不碰任何数据/线程/拾取/注册逻辑。
namespace earthmark
{
    enum class MarkerShape { Circle=0, Arrow=1, Ship=2, StarBurst=3, Ring=4,
                             WarnTriangle=5, Diamond=6, Hexagon=7, Teardrop=8, SatBox=9, Square=10 };
    struct MarkerVisual { MarkerShape shape; osg::Vec4 color; };   // color=代表色
    // 按图层 id 查代表徽章;未登记回退 {Circle, 中性灰}。
    MarkerVisual visualForLayer(const std::string& layerId);
    // 共享 GLSL 形状库(拼进各点精灵 FS)。提供 float markerCoverage(int,vec2,float),返回
    // 覆盖率 0..1(0=形状外应 discard;边缘 smoothstep 抗锯齿)。pc=gl_PointCoord。
    const char* markerShapeGLSL();
    // ImGui 侧画对应形状(图层目录 icon / 详情卡 chip)。imDrawList 传 ImDrawList*(void* 免头依赖);
    // (cx,cy)=中心屏幕坐标,sizePx=外接尺寸,colorU32=ImU32。
    void drawMarkerIcon(void* imDrawList, MarkerShape shape, float cx, float cy, float sizePx,
                        unsigned int colorU32);
}
#endif
