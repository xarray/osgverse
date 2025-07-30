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

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

#define MIRROR_EXAMPLE 1
//#define CSG_EXAMPLE 1

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

int main(int argc, char** argv)
{
    osg::DisplaySettings::instance()->setMinimumNumStencilBits(8);
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    osgVerse::updateOsgBinaryWrappers();

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
#else
    osg::ref_ptr<osg::Node> rootNode;
#endif

    osgViewer::Viewer viewer;
    viewer.getCamera()->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(rootNode);
    return viewer.run();
}
