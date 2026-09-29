#include "marker_style.h"
#include <map>
#include <cmath>
#include <ui/ImGuiComponents.h>

namespace earthmark
{
    // ---- 1) 单一真源:每层代表徽章登记表 ----
    MarkerVisual visualForLayer(const std::string& id)
    {
        static const osg::Vec4 kShipCol(0.208f,0.878f,0.816f,1.f);
        static const osg::Vec4 kStratCol(0.604f,0.643f,0.698f,1.f);
        static const osg::Vec4 kSatCol(1.000f,0.824f,0.227f,1.f);
        static const std::map<std::string, MarkerVisual> kReg = {
            { "ais",         { MarkerShape::Ship,         kShipCol } },
            { "ships",       { MarkerShape::Ship,         kShipCol } },
            { "flights",     { MarkerShape::Arrow,        osg::Vec4(0.302f,0.659f,1.000f,1.f) } },
            { "fires",       { MarkerShape::StarBurst,    osg::Vec4(1.000f,0.353f,0.188f,1.f) } },
            { "quakes",      { MarkerShape::Ring,         osg::Vec4(0.690f,0.420f,1.000f,1.f) } },
            { "gdacs",       { MarkerShape::WarnTriangle, osg::Vec4(1.000f,0.231f,0.278f,1.f) } },
            { "gdelt",       { MarkerShape::Diamond,      osg::Vec4(1.000f,0.761f,0.227f,1.f) } },
            { "gpsjam",      { MarkerShape::Hexagon,      osg::Vec4(1.000f,0.353f,0.784f,1.f) } },
            { "eonet",       { MarkerShape::Teardrop,     osg::Vec4(0.294f,0.847f,0.420f,1.f) } },
            { "unhcr",       { MarkerShape::Circle,       osg::Vec4(0.239f,0.482f,1.000f,1.f) } },
            { "nhc",         { MarkerShape::Ring,         osg::Vec4(0.498f,0.816f,0.847f,1.f) } },
            { "satstations", { MarkerShape::SatBox,       kSatCol } },
            { "satnav",      { MarkerShape::SatBox,       kSatCol } },
            { "satwx",       { MarkerShape::SatBox,       kSatCol } },
            { "starlink",    { MarkerShape::SatBox,       kSatCol } },
            // 战略各源(id 以 strategic_feed.cpp kDatasets[] 实际值为准,全部 Square):
            { "bases",       { MarkerShape::Square,       kStratCol } },
            { "ports",       { MarkerShape::Square,       kStratCol } },
            { "nuclear",     { MarkerShape::Square,       kStratCol } },
            { "spaceports",  { MarkerShape::Square,       kStratCol } },
            { "datacenters", { MarkerShape::Square,       kStratCol } },
        };
        std::map<std::string, MarkerVisual>::const_iterator it = kReg.find(id);
        if (it != kReg.end()) return it->second;
        return MarkerVisual{ MarkerShape::Circle, osg::Vec4(0.700f,0.700f,0.720f,1.f) };
    }

    // ---- 2) 共享 GLSL 形状库 ----
    // 约定:d=pc-0.5。可旋转形状先按 headingRad 旋转。每个形状返回 signed margin(>0 内、<0 外);
    // markerCoverage 统一 AA:cov=smoothstep(-AA,AA,margin),AA≈0.035;调用方对 cov<=0 discard。
    const char* markerShapeGLSL()
    {
        return
        "float _sdCircle(vec2 d){ return 0.45 - length(d); }\n"
        "float _sdDiamond(vec2 d){ return 0.44 - (abs(d.x)+abs(d.y)); }\n"
        "float _sdSquare(vec2 d){ return 0.38 - max(abs(d.x),abs(d.y)); }\n"
        "float _sdHex(vec2 d){ d=abs(d); return 0.44 - max(d.x*0.8660254+d.y*0.5, d.y); }\n"
        "float _sdTriUp(vec2 d){ float mb=d.y+0.34; float hw=0.5*(0.42-d.y); return min(mb, hw-abs(d.x)); }\n"
        "float _sdArrow(vec2 r){ float mb=r.y+0.32; float hw=0.30*(0.42-r.y)/0.74; return min(mb, hw-abs(r.x)); }\n"
        "float _sdShip(vec2 r){\n"
        "  float m = 1.0;\n"
        "  m = min(m, r.y + 0.40);                              // 艉底 y>-0.40\n"
        "  m = min(m, (0.45 - r.y)*0.28 - (r.x)*0.53 + 0.045);  // 右艏斜边\n"
        "  m = min(m, (0.45 - r.y)*0.28 + (r.x)*0.53 + 0.045);  // 左艏斜边\n"
        "  m = min(m, 0.24 - r.x);                              // 右舷\n"
        "  m = min(m, 0.24 + r.x);                              // 左舷\n"
        "  return m;\n"
        "}\n"
        "float _sdStar(vec2 d){ float a=atan(d.y,d.x); float rr=0.45*(0.52+0.48*abs(cos(2.0*a))); return rr-length(d); }\n"
        "float _sdRing(vec2 d){ float r=length(d); float band=0.085-abs(r-0.33); float dot=0.14-r; return max(band,dot); }\n"
        "float _sdDrop(vec2 d){ float circ=0.33-length(d-vec2(0.0,-0.08));\n"
        "  float hw=0.33*(0.46-d.y)/0.54; float tri=min(min(hw-abs(d.x), d.y+0.08), 0.46-d.y); return max(circ,tri); }\n"
        "float _sdSat(vec2 d){ float cheb=max(abs(d.x),abs(d.y));\n"
        "  float frame=min(cheb-0.26, 0.38-cheb); float arm=min(0.055-min(abs(d.x),abs(d.y)), 0.48-cheb); return max(frame,arm); }\n"
        "float markerCoverage(int shapeId, vec2 pc, float headingRad){\n"
        "  vec2 d = pc - vec2(0.5);\n"
        "  vec2 r = d;\n"
        "  if (shapeId==1 || shapeId==2){ float s=sin(headingRad),c=cos(headingRad);\n"
        "     r=vec2(c*d.x - s*d.y, s*d.x + c*d.y); r.y=-r.y; }\n"
        "  float m;\n"
        "  if      (shapeId==1) m=_sdArrow(r);\n"
        "  else if (shapeId==2) m=_sdShip(r);\n"
        "  else if (shapeId==3) m=_sdStar(d);\n"
        "  else if (shapeId==4) m=_sdRing(d);\n"
        "  else if (shapeId==5) m=_sdTriUp(d);\n"
        "  else if (shapeId==6) m=_sdDiamond(d);\n"
        "  else if (shapeId==7) m=_sdHex(d);\n"
        "  else if (shapeId==8) m=_sdDrop(d);\n"
        "  else if (shapeId==9) m=_sdSat(d);\n"
        "  else if (shapeId==10) m=_sdSquare(d);\n"
        "  else m=_sdCircle(d);\n"
        "  return smoothstep(-0.035, 0.035, m);\n"
        "}\n";
    }

    // ---- 3) ImGui 形状绘制(与 GLSL 形状对应) ----
    void drawMarkerIcon(void* imDrawList, MarkerShape shape, float cx, float cy, float sizePx,
                        unsigned int colorU32)
    {
        ImDrawList* dl = (ImDrawList*)imDrawList;
        ImU32 col = (ImU32)colorU32;
        float R = sizePx * 0.5f;
        ImVec2 c(cx, cy);
        #define MP(px,py) ImVec2(c.x + (px)*sizePx, c.y - (py)*sizePx)
        switch (shape)
        {
        case MarkerShape::Ship: {
            ImVec2 p[5] = { MP(0.0f,0.45f), MP(0.26f,0.12f), MP(0.20f,-0.40f), MP(-0.20f,-0.40f), MP(-0.26f,0.12f) };
            dl->AddConvexPolyFilled(p, 5, col); break; }
        case MarkerShape::Arrow: {
            ImVec2 p[3] = { MP(0.0f,0.45f), MP(0.30f,-0.35f), MP(-0.30f,-0.35f) };
            dl->AddConvexPolyFilled(p, 3, col); break; }
        case MarkerShape::StarBurst: {
            ImVec2 p[8]; for (int i=0;i<8;++i){ float a=(float)i*3.14159265f/4.0f;
                float rr=(i%2==0)?R:R*0.5f; p[i]=ImVec2(c.x+cosf(a)*rr, c.y-sinf(a)*rr); }
            dl->AddConvexPolyFilled(p, 8, col); break; }
        case MarkerShape::Ring:
            dl->AddCircle(c, R*0.82f, col, 24, R*0.20f); dl->AddCircleFilled(c, R*0.28f, col, 12); break;
        case MarkerShape::WarnTriangle: {
            ImVec2 p[3] = { MP(0.0f,0.42f), MP(0.42f,-0.38f), MP(-0.42f,-0.38f) };
            dl->AddConvexPolyFilled(p, 3, col); break; }
        case MarkerShape::Diamond: {
            ImVec2 p[4] = { MP(0.0f,0.45f), MP(0.45f,0.0f), MP(0.0f,-0.45f), MP(-0.45f,0.0f) };
            dl->AddConvexPolyFilled(p, 4, col); break; }
        case MarkerShape::Hexagon: {
            ImVec2 p[6]; for (int i=0;i<6;++i){ float a=3.14159265f/2.0f + (float)i*3.14159265f/3.0f;
                p[i]=ImVec2(c.x+cosf(a)*R, c.y-sinf(a)*R); }
            dl->AddConvexPolyFilled(p, 6, col); break; }
        case MarkerShape::Teardrop: {
            dl->AddCircleFilled(ImVec2(c.x, c.y + R*0.16f), R*0.66f, col, 16);
            ImVec2 p[3] = { MP(0.0f,0.46f), MP(0.30f,-0.05f), MP(-0.30f,-0.05f) };
            dl->AddConvexPolyFilled(p, 3, col); break; }
        case MarkerShape::SatBox:
            dl->AddRect(MP(-0.30f,0.30f), MP(0.30f,-0.30f), col, 1.5f, 0, R*0.18f);
            dl->AddLine(MP(0.0f,0.48f), MP(0.0f,-0.48f), col, R*0.12f);
            dl->AddLine(MP(-0.48f,0.0f), MP(0.48f,0.0f), col, R*0.12f); break;
        case MarkerShape::Square:
            dl->AddRectFilled(MP(-0.38f,0.38f), MP(0.38f,-0.38f), col, R*0.20f); break;
        case MarkerShape::Circle:
        default:
            dl->AddCircleFilled(c, R*0.9f, col, 16); break;
        }
        #undef MP
    }
}
