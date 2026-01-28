#include <osg/io_utils>
#include <osg/Stencil>
#include <osg/BlendFunc>
#include <osg/CullFace>
#include <osg/ClipNode>
#include <osg/Depth>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>

#include <VerseCommon.h>
#include <modeling/Utilities.h>
#include <pipeline/Utilities.h>
#include <pipeline/ResourceManager.h>
#include <pipeline/Pipeline.h>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

//#define MIRROR_EXAMPLE 1
//#define CSG_EXAMPLE 1
#define EARTH_CROSS_EXAMPLE 1

#ifdef MIRROR_EXAMPLE
osg::Drawable* createMirrorSurface(float xMin, float xMax, float yMin, float yMax, float z)
{
    osg::Vec3Array* coords = new osg::Vec3Array(4);
    (*coords)[0].set(xMin, yMax, z); (*coords)[1].set(xMin, yMin, z);
    (*coords)[2].set(xMax, yMin, z); (*coords)[3].set(xMax, yMax, z);

    osg::Vec2Array* tcoords = new osg::Vec2Array(4);
    (*tcoords)[0].set(0.0f, 1.0f); (*tcoords)[1].set(0.0f, 0.0f);
    (*tcoords)[2].set(1.0f, 0.0f); (*tcoords)[3].set(1.0f, 1.0f);

    osg::Vec3Array* norms = new osg::Vec3Array(4);
    for (int i = 0; i < 4; ++i) (*norms)[i].set(0.0f, 0.0f, 1.0f);

    osg::Vec4Array* colors = new osg::Vec4Array(4);
    for (int i = 0; i < 4; ++i) (*colors)[i].set(1.0f, 1.0f, 1.0, 1.0f);

    osg::Geometry* geom = new osg::Geometry;
    geom->setVertexArray(coords); geom->setTexCoordArray(0, tcoords);
    geom->setNormalArray(norms); geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geom->setColorArray(colors); geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));
    return geom;
}

osg::Node* createMirroredScene(osg::Node* model)
{
    const osg::BoundingSphere& bs = model->getBound();
    float width_factor = 1.5, height_factor = 0.3;
    float xMin = bs.center().x() - bs.radius() * width_factor;
    float xMax = bs.center().x() + bs.radius() * width_factor;
    float yMin = bs.center().y() - bs.radius() * width_factor;
    float yMax = bs.center().y() + bs.radius() * width_factor;
    float z = bs.center().z() - bs.radius() * height_factor;
    osg::Drawable* mirror = createMirrorSurface(xMin, xMax, yMin, yMax, z);

    // Create root stateset to define basic color mask and depth testing method
    osg::StateSet* rootStateSet = new osg::StateSet;
    {
        osg::ColorMask* rootColorMask = new osg::ColorMask;
        rootColorMask->setMask(true, true, true, true);

        osg::Depth* rootDepth = new osg::Depth;
        rootDepth->setFunction(osg::Depth::LESS);
        rootDepth->setRange(0.0, 1.0);

        rootStateSet->setAttribute(rootColorMask);
        rootStateSet->setAttribute(rootDepth);
    }

    osg::MatrixTransform* rootNode = new osg::MatrixTransform;
    rootNode->setMatrix(osg::Matrix::rotate(osg::inDegrees(45.0f), 1.0f, 0.0f, 0.0f));
    rootNode->setStateSet(rootStateSet);

    // Bin1: set STENCIL of mirror surface (both sides) region to 1, not rendering it
    osg::StateSet* statesetBin1 = new osg::StateSet;
    {
        osg::Stencil* stencil = new osg::Stencil;
        stencil->setFunction(osg::Stencil::ALWAYS, 1, ~0u);
        stencil->setOperation(osg::Stencil::KEEP, osg::Stencil::KEEP, osg::Stencil::REPLACE);

        osg::ColorMask* colorMask = new osg::ColorMask;
        colorMask->setMask(false, false, false, false);

        statesetBin1->setRenderBinDetails(1, "RenderBin");
        statesetBin1->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
        statesetBin1->setAttributeAndModes(stencil, osg::StateAttribute::ON);
        statesetBin1->setAttribute(colorMask);
    }

    osg::Geode* geode1 = new osg::Geode;
    geode1->addDrawable(mirror);
    geode1->setStateSet(statesetBin1);
    rootNode->addChild(geode1);

    // Bin2: set STENCIL of model region to 1
    osg::StateSet* statesetBin2 = new osg::StateSet;
    {
        osg::Stencil* stencil = new osg::Stencil;
        stencil->setFunction(osg::Stencil::ALWAYS, 0, ~0u);
        stencil->setOperation(osg::Stencil::KEEP, osg::Stencil::KEEP, osg::Stencil::REPLACE);

        statesetBin2->setRenderBinDetails(2, "RenderBin");
        statesetBin2->setAttributeAndModes(stencil, osg::StateAttribute::ON);
    }

    osg::Group* groupBin2 = new osg::Group;
    groupBin2->addChild(model);
    groupBin2->setStateSet(statesetBin2);
    rootNode->addChild(groupBin2);

    // Bin3: Clear depth inside regions where stencil = 1 for rendering mirrored model later
    osg::StateSet* statesetBin3 = new osg::StateSet;
    {
        osg::Stencil* stencil = new osg::Stencil;
        stencil->setFunction(osg::Stencil::EQUAL, 1, ~0u);
        stencil->setOperation(osg::Stencil::KEEP, osg::Stencil::KEEP, osg::Stencil::KEEP);

        osg::ColorMask* colorMask = new osg::ColorMask;
        colorMask->setMask(false, false, false, false);

        osg::Depth* depth = new osg::Depth;
        depth->setFunction(osg::Depth::ALWAYS);
        depth->setRange(1.0, 1.0);

        statesetBin3->setRenderBinDetails(3, "RenderBin");
        statesetBin3->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
        statesetBin3->setAttributeAndModes(stencil, osg::StateAttribute::ON);
        statesetBin3->setAttribute(colorMask);
        statesetBin3->setAttribute(depth);
    }

    osg::Geode* geode3 = new osg::Geode;
    geode3->addDrawable(mirror);
    geode3->setStateSet(statesetBin3);
    rootNode->addChild(geode3);

    // Bin4: draw mirror of the original model at regions where stencil = 1
    osg::ClipNode* clipNode = new osg::ClipNode;
    {
        osg::ClipPlane* clipplane = new osg::ClipPlane;
        clipplane->setClipPlane(0.0, 0.0, -1.0, z);
        clipplane->setClipPlaneNum(0);
        clipNode->addClipPlane(clipplane);
    }

    osg::StateSet* dstate = clipNode->getOrCreateStateSet();
    {
        osg::Stencil* stencil = new osg::Stencil;
        stencil->setFunction(osg::Stencil::EQUAL, 1, ~0u);
        stencil->setOperation(osg::Stencil::KEEP, osg::Stencil::KEEP, osg::Stencil::KEEP);

        dstate->setRenderBinDetails(4, "RenderBin");
        dstate->setMode(GL_CULL_FACE, osg::StateAttribute::OVERRIDE | osg::StateAttribute::OFF);
        dstate->setAttributeAndModes(stencil, osg::StateAttribute::ON);
    }

    osg::MatrixTransform* reverseMatrix = new osg::MatrixTransform;
    reverseMatrix->setStateSet(dstate);
    reverseMatrix->preMult(osg::Matrix::translate(0.0f, 0.0f, -z) *
                            osg::Matrix::scale(1.0f, 1.0f, -1.0f) *
                            osg::Matrix::translate(0.0f, 0.0f, z));
    reverseMatrix->addChild(model);
    clipNode->addChild(reverseMatrix);
    rootNode->addChild(clipNode);

    // Bin5: draw the mirror and reset stencil value to 0
    osg::StateSet* statesetBin5 = new osg::StateSet;
    {
        osg::Depth* depth = new osg::Depth;
        depth->setFunction(osg::Depth::ALWAYS);

        osg::Stencil* stencil = new osg::Stencil;
        stencil->setFunction(osg::Stencil::EQUAL, 1, ~0u);
        stencil->setOperation(osg::Stencil::KEEP, osg::Stencil::KEEP, osg::Stencil::ZERO);

        osg::BlendFunc* trans = new osg::BlendFunc;
        trans->setFunction(osg::BlendFunc::ONE, osg::BlendFunc::ONE);

        statesetBin5->setRenderBinDetails(5, "RenderBin");
        statesetBin5->setMode(GL_CULL_FACE, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);
        statesetBin5->setAttributeAndModes(stencil, osg::StateAttribute::ON);
        statesetBin5->setAttributeAndModes(trans, osg::StateAttribute::ON);
        statesetBin5->setAttribute(depth);
    }

    osg::Geode* geode5 = new osg::Geode;
    geode5->addDrawable(mirror);
    geode5->setStateSet(statesetBin5);
    rootNode->addChild(geode5);
    return rootNode;
}
#endif

#ifdef CSG_EXAMPLE
osg::Node* createCsgScene(osg::Node* nodeA, osg::Node* nodeB)
{
    // Create root stateset to define basic color mask and depth testing method
    osg::StateSet* rootStateSet = new osg::StateSet;
    rootStateSet->setAttributeAndModes(new osg::ColorMask(true, true, true, true));
    rootStateSet->setAttributeAndModes(new osg::Depth(osg::Depth::LESS, 0.0, 1.0, true));
    rootStateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);

    osg::Group* rootNode = new osg::Group;
    rootNode->setStateSet(rootStateSet);

    // Bin0: create depth buffer of A
    osg::StateSet* statesetBin0 = new osg::StateSet;
    {
        statesetBin0->setRenderBinDetails(0, "RenderBin");
        statesetBin0->setAttributeAndModes(new osg::Depth(osg::Depth::LESS, 0.0, 1.0, true));
        statesetBin0->setAttributeAndModes(new osg::CullFace(osg::CullFace::FRONT));
        statesetBin0->setAttributeAndModes(new osg::ColorMask(false, false, false, false));
    }
    osg::Group* groupBin0 = new osg::Group;
    groupBin0->addChild(nodeA);
    groupBin0->setStateSet(statesetBin0);

    // Bin1: find regions of back faces of A, set stencil = stencil + 1
    osg::StateSet* statesetBin1 = new osg::StateSet;
    {
        osg::Stencil* stencil = new osg::Stencil;
        stencil->setFunction(osg::Stencil::ALWAYS, 0, ~0u);
        stencil->setOperation(osg::Stencil::KEEP, osg::Stencil::KEEP, osg::Stencil::INCR_WRAP);

        statesetBin1->setRenderBinDetails(1, "RenderBin");
        statesetBin1->setAttributeAndModes(stencil, osg::StateAttribute::ON);
        statesetBin1->setAttributeAndModes(new osg::Depth(osg::Depth::LESS, 0.0, 1.0, false));
        statesetBin1->setAttributeAndModes(new osg::CullFace(osg::CullFace::FRONT));
        statesetBin1->setAttributeAndModes(new osg::ColorMask(false, false, false, false));
    }
    osg::Group* groupBin1 = new osg::Group;
    groupBin1->addChild(nodeA);
    groupBin1->setStateSet(statesetBin1);

    // Bin2: find regions of front faces of A, set stencil = stencil - 1
    // So only internal regions of A will keep stencil > 0 after this bin
    osg::StateSet* statesetBin2 = new osg::StateSet;
    {
        osg::Stencil* stencil = new osg::Stencil;
        stencil->setFunction(osg::Stencil::ALWAYS, 0, ~0u);
        stencil->setOperation(osg::Stencil::KEEP, osg::Stencil::KEEP, osg::Stencil::DECR_WRAP);

        statesetBin2->setRenderBinDetails(2, "RenderBin");
        statesetBin2->setAttributeAndModes(stencil, osg::StateAttribute::ON);
        statesetBin2->setAttributeAndModes(new osg::Depth(osg::Depth::LESS, 0.0, 1.0, false));
        statesetBin2->setAttributeAndModes(new osg::CullFace(osg::CullFace::BACK));
        statesetBin2->setAttributeAndModes(new osg::ColorMask(false, false, false, false));
    }
    osg::Group* groupBin2 = new osg::Group;
    groupBin2->addChild(nodeA);
    groupBin2->setStateSet(statesetBin2);

    // Bin3: find regions where stencil > 0 AND regions of back faces of B, replace stencil values
    // That will set stencil values of both 'internal regions of A' AND 'back faces of B' to 1
    osg::StateSet* statesetBin3 = new osg::StateSet;
    {
        osg::Stencil* stencil = new osg::Stencil;
        stencil->setFunction(osg::Stencil::GREATER, 0, ~0u);
        stencil->setOperation(osg::Stencil::KEEP, osg::Stencil::KEEP, osg::Stencil::REPLACE);

        statesetBin3->setRenderBinDetails(3, "RenderBin");
        statesetBin3->setAttributeAndModes(stencil, osg::StateAttribute::ON);
        statesetBin3->setAttributeAndModes(new osg::Depth(osg::Depth::LESS, 0.0, 1.0, false));
        statesetBin3->setAttributeAndModes(new osg::CullFace(osg::CullFace::FRONT));
        statesetBin3->setAttributeAndModes(new osg::ColorMask(false, false, false, false));
    }
    osg::Group* groupBin3 = new osg::Group;
    groupBin3->addChild(nodeB);
    groupBin3->setStateSet(statesetBin3);

    // Bin4: find regions where stencil = 1 AND regions of front faces of B, set stencil = 0
    // This will only keep regions that 'internal regions of A' AND 'back faces of B' AND 'not front faces of B'
    osg::StateSet* statesetBin4 = new osg::StateSet;
    {
        osg::Stencil* stencil = new osg::Stencil;
        stencil->setFunction(osg::Stencil::EQUAL, 1, ~0u);
        stencil->setOperation(osg::Stencil::KEEP, osg::Stencil::KEEP, osg::Stencil::ZERO);

        statesetBin4->setRenderBinDetails(4, "RenderBin");
        statesetBin4->setAttributeAndModes(stencil, osg::StateAttribute::ON);
        statesetBin4->setAttributeAndModes(new osg::Depth(osg::Depth::LESS, 0.0, 1.0, false));
        statesetBin4->setAttributeAndModes(new osg::CullFace(osg::CullFace::BACK));
        statesetBin4->setAttributeAndModes(new osg::ColorMask(false, false, false, false));
    }
    osg::Group* groupBin4 = new osg::Group;
    groupBin4->addChild(nodeB);
    groupBin4->setStateSet(statesetBin4);

    // Bin5: render front faces of A
    osg::StateSet* statesetBin5 = new osg::StateSet;
    {
        osg::Stencil* stencil = new osg::Stencil;
        stencil->setFunction(osg::Stencil::EQUAL, 0, ~0u);
        stencil->setOperation(osg::Stencil::KEEP, osg::Stencil::KEEP, osg::Stencil::KEEP);

        statesetBin5->setRenderBinDetails(5, "RenderBin");
        statesetBin5->setAttributeAndModes(stencil, osg::StateAttribute::ON);
        statesetBin5->setAttributeAndModes(new osg::CullFace(osg::CullFace::BACK));
        statesetBin5->setAttributeAndModes(new osg::ColorMask(true, true, true, true));
    }
    osg::Group* groupBin5 = new osg::Group;
    groupBin5->addChild(nodeA);
    groupBin5->setStateSet(statesetBin5);

    // Bin6: render back faces of B
    osg::StateSet* statesetBin6 = new osg::StateSet;
    {
        osg::Stencil* stencil = new osg::Stencil;
        stencil->setFunction(osg::Stencil::EQUAL, 1, ~0u);
        stencil->setOperation(osg::Stencil::KEEP, osg::Stencil::KEEP, osg::Stencil::ZERO);

        statesetBin6->setRenderBinDetails(6, "RenderBin");
        statesetBin6->setAttributeAndModes(stencil, osg::StateAttribute::ON);
        statesetBin6->setAttributeAndModes(new osg::CullFace(osg::CullFace::FRONT));
        statesetBin6->setAttributeAndModes(new osg::ColorMask(true, true, true, true));
        statesetBin6->setAttributeAndModes(new osg::BlendFunc(GL_ONE, GL_ONE));
    }
    osg::Group* groupBin6 = new osg::Group;
    groupBin6->addChild(nodeB);
    groupBin6->setStateSet(statesetBin6);

    ///////////////////////////////
    rootNode->addChild(groupBin0);
    rootNode->addChild(groupBin1);
    rootNode->addChild(groupBin2);
    rootNode->addChild(groupBin3);
    rootNode->addChild(groupBin4);
    rootNode->addChild(groupBin5);
    rootNode->addChild(groupBin6);
    return rootNode;
}
#endif

#ifdef EARTH_CROSS_EXAMPLE
const char* commonVertCode = {
    "uniform mat4 osg_ViewMatrixInverse; \n"
    "VERSE_VS_OUT vec4 worldPos, texCoord, color; \n"
    "void main() {\n"
    "    texCoord = osg_MultiTexCoord0; color = osg_Color; \n"
    "    worldPos = osg_ViewMatrixInverse * osg_ModelViewMatrix * osg_Vertex; \n"
    "    gl_Position = osg_ModelViewProjectionMatrix * osg_Vertex; \n"
    "}\n"
};

const char* sphereFragCode = {
    "uniform sampler2D baseTexture; \n"
    "uniform vec4 clipPlane0, clipPlane1, clipPlane2; \n"
    "VERSE_FS_IN vec4 worldPos, texCoord; \n"
    "VERSE_FS_OUT vec4 fragColor; \n"
    "void main() {\n"
    "    float clipD0 = dot(worldPos, clipPlane0); bool valid0 = any(notEqual(clipPlane0, vec4(0.0))); \n"
    "    float clipD1 = dot(worldPos, clipPlane1); bool valid1 = any(notEqual(clipPlane1, vec4(0.0))); \n"
    "    float clipD2 = dot(worldPos, clipPlane2); bool valid2 = any(notEqual(clipPlane2, vec4(0.0))); \n"
    "    int passed = ((clipD0 <= 0.0 && valid0) ? 1 : 0) + ((clipD1 <= 0.0 && valid1) ? 1 : 0) + \n"
    "                 ((clipD2 <= 0.0 && valid2) ? 1 : 0); \n"
    "    if (passed == 0 && (valid0 || valid1 || valid2)) discard; \n"
    "    fragColor = VERSE_TEX2D(baseTexture, texCoord.st); \n"
    "    VERSE_FS_FINAL(fragColor);\n"
    "}\n"
};

const char* mentalFragCode = {
    "uniform sampler2D noiseTexture; \n"
    "uniform vec3 colorScale; \n"
    "uniform float osg_SimulationTime; \n"
    "VERSE_FS_IN vec4 texCoord, color; \n"
    "VERSE_FS_OUT vec4 fragColor; \n"

    "float hash21(in vec2 n) { return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453); }\n"
    "mat2 makem2(in float theta) { float c = cos(theta); float s = sin(theta); return mat2(c, -s, s, c); }\n"
    "float noise(in vec2 x) { return VERSE_TEX2D(noiseTexture, x * .01).x; }\n"

    "vec2 gradn(vec2 p) {\n"
    "    float ep = .09; \n"
    "    float gradx = noise(vec2(p.x + ep, p.y)) - noise(vec2(p.x - ep, p.y)); \n"
    "    float grady = noise(vec2(p.x, p.y + ep)) - noise(vec2(p.x, p.y - ep)); \n"
    "    return vec2(gradx, grady); \n"
    "}\n"

    "float flow(in vec2 p, in float time) {\n"
    "    float z = 2., rz = 0.; vec2 bp = p; \n"
    "    for (float i = 1.; i < 7.; i++) {\n"
    "        p += time * .6;  //primary flow speed \n"
    "        bp += time * 1.9;  //secondary flow speed \n"
    "        vec2 gr = gradn(i * p * .34 + time * 1.);  //displacement field \n"
    "        gr *= makem2(time * 6. - (0.05 * p.x + 0.03 * p.y) * 40.); \n"
    "        p += gr * .5;  //displace the system \n"
    "        rz += (sin(noise(p) * 7.) * 0.5 + 0.5) / z; \n"
    "        p = mix(bp, p, .77); bp *= 1.9; \n"
    "        z *= 1.4; p *= 2.;  //intensity scaling & octave scaling \n"
    "    }\n"
    "    return rz; \n"
    "}\n"

    "void main() {\n"
    "    vec2 p = texCoord.st; p *= 30.; float rz = flow(p, -osg_SimulationTime * 0.02); \n"
    "    vec3 col = colorScale / rz; col = pow(col, vec3(1.4)); \n"
    "    fragColor = color * vec4(col, 1.0); \n"
    "    VERSE_FS_FINAL(fragColor);\n"
    "}\n"
};

class EnvironmentHandler : public osgGA::GUIEventHandler
{
public:
    EnvironmentHandler(osg::Geode* box) : _box(box) {}
    void setUniform(int i, osg::Uniform* u) { _uniform[i] = u; }

    void updateClip(int i, const osg::Vec4& plane)
    {
        osg::Vec3 vx, vy, n(plane[0], plane[1], plane[2]); n.normalize();
        if (abs(n[0]) <= abs(n[1]) && abs(n[0]) <= abs(n[2])) vx = osg::X_AXIS;
        else if (abs(n[1]) <= abs(n[2])) vx = osg::Y_AXIS;
        else vx = osg::Z_AXIS; _uniform[i]->set(plane);

        vx = vx - n * (n * vx); vx.normalize(); vy = n ^ vx; vy.normalize();
        vx = vx * 10.0f; vy = vy * 10.0f;
        //std::cout << plane << ": " << vx << "; " << vy << "\n";

        osg::ref_ptr<osg::Geometry> geomOfPlane =
            osg::createTexturedQuadGeometry(osg::Vec3(-vx - vy), vx * 2.0f, vy * 2.0f);
        if (plane.length2() > 0.0f) _box->setDrawable(i, geomOfPlane.get());
        else _box->setDrawable(i, new osg::Geometry);
    }

    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        if (ea.getEventType() == osgGA::GUIEventAdapter::KEYUP)
        {
            osg::Vec4 plane0, plane1, plane2;
            _uniform[0]->get(plane0); _uniform[1]->get(plane1); _uniform[2]->get(plane2);
            switch (ea.getKey())
            {
            case '1':
                if (plane0.length2() > 0.0f) plane0.set(0.0f, 0.0f, 0.0f, 0.0f);
                else plane0.set(0.0f, 0.0f, 1.0f, 0.0f); updateClip(0, plane0); break;
            case '2':
                if (plane1.length2() > 0.0f) plane1.set(0.0f, 0.0f, 0.0f, 0.0f);
                else plane1.set(1.0f, 0.0f, 0.0f, 0.0f); updateClip(1, plane1); break;
            case '3':
                if (plane2.length2() > 0.0f) plane2.set(0.0f, 0.0f, 0.0f, 0.0f);
                else plane2.set(0.0f, 1.0f, 0.0f, 0.0f); updateClip(2, plane2); break;
            }
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN)
        {
            osg::Vec4 plane0, plane1, plane2;
            _uniform[0]->get(plane0); _uniform[1]->get(plane1); _uniform[2]->get(plane2);
            switch (ea.getKey())
            {
            case 'n':
                plane0 = plane0 * osg::Matrix::rotate(-0.01f, osg::X_AXIS); updateClip(0, plane0); break;
            case 'm':
                plane0 = plane0 * osg::Matrix::rotate(0.01f, osg::X_AXIS); updateClip(0, plane0); break;
            case ',':
                plane1 = plane1 * osg::Matrix::rotate(-0.01f, osg::Z_AXIS); updateClip(1, plane1); break;
            case '.':
                plane1 = plane1 * osg::Matrix::rotate(0.01f, osg::Z_AXIS); updateClip(1, plane1); break;
            }
        }
        return false;
    }

protected:
    osg::observer_ptr<osg::Geode> _box;
    osg::observer_ptr<osg::Uniform> _uniform[3];
};

osg::Node* createCrossScene(EnvironmentHandler* env, osg::Node* nodeA, osg::Node* nodeB)
{
    osg::StateSet* rootStateSet = new osg::StateSet;
    rootStateSet->setAttributeAndModes(new osg::ColorMask(true, true, true, true));
    rootStateSet->setAttributeAndModes(new osg::Depth(osg::Depth::LESS, 0.0, 1.0, true));
    rootStateSet->setAttributeAndModes(new osg::CullFace(osg::CullFace::BACK));

    int cxtVer = 0, glslVer = 0; osgVerse::guessOpenGLVersions(cxtVer, glslVer);
    osg::Shader* vs = new osg::Shader(osg::Shader::VERTEX, commonVertCode);
    osg::Shader* fs0 = new osg::Shader(osg::Shader::FRAGMENT, sphereFragCode);
    osg::Shader* fs2 = new osg::Shader(osg::Shader::FRAGMENT, mentalFragCode);
    osgVerse::Pipeline::createShaderDefinitions(vs, cxtVer, glslVer);
    osgVerse::Pipeline::createShaderDefinitions(fs0, cxtVer, glslVer);
    osgVerse::Pipeline::createShaderDefinitions(fs2, cxtVer, glslVer);
    
    vs->setName("Common_VS"); osgVerse::ResourceManager::instance()->shareShader(vs, true);
    fs0->setName("Sphere_FS"); osgVerse::ResourceManager::instance()->shareShader(fs0, true);
    fs2->setName("Mental_FS"); osgVerse::ResourceManager::instance()->shareShader(fs2, true);

    osg::StateSet* statesetBin0 = new osg::StateSet;
    {
        osg::Program* prog = new osg::Program;
        prog->addShader(vs); prog->addShader(fs0);

        osg::Stencil* stencil = new osg::Stencil;
        stencil->setFunction(osg::Stencil::ALWAYS, 1, ~0u);
        stencil->setOperation(osg::Stencil::KEEP, osg::Stencil::KEEP, osg::Stencil::INCR);

        statesetBin0->setAttributeAndModes(prog); statesetBin0->setAttributeAndModes(stencil);
        statesetBin0->setTextureAttributeAndModes(
            0, osgVerse::createTexture2D(osgDB::readImageFile("Images/land_shallow_topo_2048.jpg")));
        osg::Uniform* cp0 = statesetBin0->getOrCreateUniform("clipPlane0", osg::Uniform::FLOAT_VEC4);
        osg::Uniform* cp1 = statesetBin0->getOrCreateUniform("clipPlane1", osg::Uniform::FLOAT_VEC4);
        osg::Uniform* cp2 = statesetBin0->getOrCreateUniform("clipPlane2", osg::Uniform::FLOAT_VEC4);
        statesetBin0->getOrCreateUniform("baseTexture", osg::Uniform::INT)->set((int)0);
        statesetBin0->setRenderBinDetails(0, "RenderBin");

        cp0->set(osg::Vec4(0.0f, 0.0f, 1.0f, 0.0f)); env->setUniform(0, cp0);  // XOY
        cp1->set(osg::Vec4(1.0f, 0.0f, 0.0f, 0.0f)); env->setUniform(1, cp1);  // YOZ
        cp2->set(osg::Vec4(0.0f, 1.0f, 0.0f, 0.0f)); env->setUniform(2, cp2);  // ZOX
    }
    osg::Group* groupBin0 = new osg::Group;
    groupBin0->addChild(nodeA);
    groupBin0->setStateSet(statesetBin0);

    osg::StateSet* statesetBin1 = new osg::StateSet;
    {
        osg::Stencil* stencil = new osg::Stencil;
        stencil->setFunction(osg::Stencil::ALWAYS, 1, ~0u);
        stencil->setOperation(osg::Stencil::KEEP, osg::Stencil::KEEP, osg::Stencil::INCR);

        statesetBin1->setAttributeAndModes(stencil);
        statesetBin1->setAttributeAndModes(new osg::ColorMask(false, false, false, false));
        statesetBin1->setAttributeAndModes(new osg::Depth(osg::Depth::LESS, 0.0, 1.0, false));
        statesetBin0->setRenderBinDetails(1, "RenderBin");
    }
    osg::Group* groupBin1 = new osg::Group;
    groupBin1->addChild(nodeA);
    groupBin1->setStateSet(statesetBin1);

    osg::StateSet* statesetBin2 = new osg::StateSet;
    {
        osg::Program* prog = new osg::Program;
        prog->addShader(vs); prog->addShader(fs2);

        osg::Stencil* stencil = new osg::Stencil;
        stencil->setFunction(osg::Stencil::EQUAL, 1, ~0u);
        stencil->setOperation(osg::Stencil::KEEP, osg::Stencil::KEEP, osg::Stencil::KEEP);
        statesetBin2->setAttributeAndModes(prog); statesetBin2->setAttributeAndModes(stencil);
        statesetBin2->setTextureAttributeAndModes(
            0, osgVerse::createTexture2D(osgDB::readImageFile(BASE_DIR + "/textures/noise.jpg")));
        statesetBin2->getOrCreateUniform("noiseTexture", osg::Uniform::INT)->set((int)0);
        statesetBin2->getOrCreateUniform("colorScale", osg::Uniform::FLOAT_VEC3)->set(osg::Vec3(0.2f, 0.07f, 0.01f));
        statesetBin2->setRenderBinDetails(2, "RenderBin");
    }
    osg::Group* groupBin2 = new osg::Group;
    groupBin2->addChild(nodeB);
    groupBin2->setStateSet(statesetBin2);

    osg::Group* rootNode = new osg::Group;
    rootNode->setStateSet(rootStateSet);

    ///////////////////////////////
    rootNode->addChild(groupBin0);
    rootNode->addChild(groupBin1);
    rootNode->addChild(groupBin2);
    return rootNode;
}
#endif

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());
    osgVerse::updateOsgBinaryWrappers();

    osg::DisplaySettings::instance()->setMinimumNumStencilBits(8);
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    osgViewer::Viewer viewer;

    osg::ref_ptr<osg::Node> loadedModel = osgDB::readNodeFile("cessna.osgt");
    if (!loadedModel) return 1;

    osg::ref_ptr<osg::MatrixTransform> modelTransform = new osg::MatrixTransform;
    modelTransform->addChild(loadedModel);
    modelTransform->setUpdateCallback(new osg::AnimationPathCallback(
        modelTransform->getBound().center(), osg::Vec3(0.0f, 0.0f, 1.0f), osg::inDegrees(45.0f)));

#if defined(MIRROR_EXAMPLE)
    osg::ref_ptr<osg::Node> rootNode = createMirroredScene(modelTransform.get());
#elif defined(CSG_EXAMPLE)
    osg::Geode* sphere = new osg::Geode;
    sphere->addDrawable(new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(), 4.0f)));
    osg::ref_ptr<osg::Node> rootNode = createCsgScene(modelTransform.get(), sphere);
#elif defined(EARTH_CROSS_EXAMPLE)
    osg::Geode* sphere = new osg::Geode;
    sphere->addDrawable(new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(), 5.0f)));

    osg::Geode* box = new osg::Geode;
    osg::Vec3 vx = osg::X_AXIS * 10.0f, vy = osg::Y_AXIS * 10.0f, vz = osg::Z_AXIS * 10.0f;
    box->addDrawable(osg::createTexturedQuadGeometry(osg::Vec3(-vx - vy), vx * 2.0f, vy * 2.0f));
    box->addDrawable(osg::createTexturedQuadGeometry(osg::Vec3(-vy - vz), vy * 2.0f, vz * 2.0f));
    box->addDrawable(osg::createTexturedQuadGeometry(osg::Vec3(-vz - vx), vz * 2.0f, vx * 2.0f));

    osg::Geode* core = new osg::Geode;
    core->addDrawable(new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(), 2.0f)));
    {
        osg::Program* prog = new osg::Program;
        prog->addShader(osgVerse::ResourceManager::instance()->getShader("Mental_FS"));
        core->getOrCreateStateSet()->setAttributeAndModes(prog);
        core->getOrCreateStateSet()->setTextureAttributeAndModes(
            0, osgVerse::createTexture2D(osgDB::readImageFile(BASE_DIR + "/textures/noise.jpg")));
        core->getOrCreateStateSet()->getOrCreateUniform("noiseTexture", osg::Uniform::INT)->set((int)0);
        core->getOrCreateStateSet()->getOrCreateUniform("colorScale", osg::Uniform::FLOAT_VEC3)->set(osg::Vec3(0.8f, 0.8f, 0.0f));
    }

    EnvironmentHandler* env = new EnvironmentHandler(box);
    viewer.addEventHandler(env);

    osg::ref_ptr<osg::Node> rootNode = createCrossScene(env, sphere, box);
    rootNode->asGroup()->addChild(core);
#else
    osg::ref_ptr<osg::Node> rootNode = new osg::Group;
#endif

#if defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE) || defined(OSG_GL3_AVAILABLE)
    rootNode->getOrCreateStateSet()->setAttribute(osgVerse::createDefaultProgram("baseTexture"));
    rootNode->getOrCreateStateSet()->addUniform(new osg::Uniform("baseTexture", (int)0));
#endif
    viewer.getCamera()->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(rootNode);
    return viewer.run();
}
