#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osg/LightSource>
#include <osg/Texture2D>
#include <osg/ShapeDrawable>
#include <osg/MatrixTransform>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgUtil/CullVisitor>
#include <osgUtil/Optimizer>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <pipeline/Pipeline.h>
#include <pipeline/ShadowModule.h>
#include <pipeline/LightModule.h>
#include <pipeline/Utilities.h>
#include <modeling/Utilities.h>
#include <readerwriter/Utilities.h>
#include <iostream>
#include <sstream>

//#include <microprofile.h>
#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

#ifdef OSG_LIBRARY_STATIC
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
#endif
USE_GRAPICSWINDOW_IMPLEMENTATION(SDL)
USE_GRAPICSWINDOW_IMPLEMENTATION(GLFW)

namespace
{
    // Small helpers used to assemble a procedural acne-friendly test scene. Mixing
    // osgVerse::createPrism/createPyramid with osg::ShapeDrawable also exercises the
    // modeling utilities on shapes that highlight different shadow-bias pathologies.
    osg::ref_ptr<osg::Geode> makeBoxGeode(const osg::Vec3& center, const osg::Vec3& halfSize)
    {
        osg::ref_ptr<osg::Box> box = new osg::Box(
            center, halfSize.x() * 2.0f, halfSize.y() * 2.0f, halfSize.z() * 2.0f);
        osg::ref_ptr<osg::ShapeDrawable> sd = new osg::ShapeDrawable(box.get());
        osg::ref_ptr<osg::Geode> g = new osg::Geode;
        g->addDrawable(sd.get());
        return g.get();
    }

    osg::ref_ptr<osg::Node> placeNode(osg::Node* child, const osg::Vec3& pos,
                                       const osg::Quat& rot = osg::Quat())
    {
        osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
        mt->setMatrix(osg::Matrix::rotate(rot) * osg::Matrix::translate(pos));
        mt->addChild(child);
        return mt.get();
    }

    // Default scene covering the shadow-acne cases that a single real model tends to hide:
    //   - tall thin pole  : peter-panning vs precision trade-off at the base
    //   - 4-sided pyramid : slanted faces where the old slope-bias alone fails
    //   - 8-sided prism   : cap/edge shading on a near-cylinder, plus the octagonal cap
    //                        transitions where adjacent face normals change a lot
    //   - small box       : ordinary flat-face self-shadow acne
    //   - sphere          : continuously varying surface slope (plane bias matters)
    //   - tilted plank    : 30 deg surface, prone to "swimming" shadow edges
    //   - row of poles    : 5 heights stepped across cascade boundaries, so the bias
    //                        transition between cascades is visible at one glance
    //   - tilted back wall: a large receiver at 60 deg, a classic peter-panning zone
    //   - ground plane    : receiver for everything above; large enough to keep most
    //                        shadows on it rather than on tiny casters
    osg::ref_ptr<osg::Node> createAcneTestScene()
    {
        osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
        root->setMatrix(osg::Matrix::rotate(osg::PI_2, osg::X_AXIS));

        // Ground (wide flat box: a thin slab avoids the single-sided-plane problem)
        root->addChild(placeNode(
            makeBoxGeode(osg::Vec3(), osg::Vec3(15.0f, 0.05f, 15.0f)),
            osg::Vec3(0.0f, -0.05f, 0.0f)));
        // Cluster A (left of origin)
        // Tall thin pole: high aspect ratio, base acne / peter-panning test
        root->addChild(placeNode(
            makeBoxGeode(osg::Vec3(), osg::Vec3(0.08f, 2.5f, 0.08f)),
            osg::Vec3(-4.5f, 2.5f, 0.0f)));
        // Pyramid: slanted faces where the slope-bias-only scheme fails
        if (osg::Geometry* p = osgVerse::createPyramid(
                osg::Vec3(-2.5f, 0.0f, -1.5f), 0.6f, 1.6f, 4, true))
        {
            osg::ref_ptr<osg::Geode> g = new osg::Geode; g->addDrawable(p);
            root->addChild(g.get());
        }
        // Small box: ordinary flat-face self-shadow acne
        root->addChild(placeNode(
            makeBoxGeode(osg::Vec3(), osg::Vec3(0.6f, 0.6f, 0.6f)),
            osg::Vec3(-2.5f, 0.6f, 0.8f)));
        // Cluster B (right of origin)
        // Octagonal prism: cap/edge shading + 8-sided cylinder self-shadow
        if (osg::Geometry* pr = osgVerse::createPrism(
                osg::Vec3(2.0f, 0.0f, -1.5f), 0.7f, 0.7f, 1.4f, 8, true))
        {
            osg::ref_ptr<osg::Geode> g = new osg::Geode; g->addDrawable(pr);
            root->addChild(g.get());
        }
        // Sphere: continuously varying slope, the case plane bias was designed for
        {
            osg::ref_ptr<osg::ShapeDrawable> sd = new osg::ShapeDrawable(
                new osg::Sphere(osg::Vec3(2.0f, 0.6f, 0.8f), 0.6f));
            osg::ref_ptr<osg::Geode> g = new osg::Geode; g->addDrawable(sd.get());
            root->addChild(g.get());
        }
        // Tilted thin plank: 30 deg surface, prone to "swimming" shadow edges
        root->addChild(placeNode(
            makeBoxGeode(osg::Vec3(), osg::Vec3(2.0f, 0.04f, 0.4f)),
            osg::Vec3(2.0f, 1.2f, -3.5f),
            osg::Quat(osg::PI / 6.0f, osg::Vec3(1.0f, 0.0f, 0.0f))));
        // Row of stepped-height poles along +X: their bases land in different cascades,
        // so the cascade-boundary bias discontinuity is visible side by side
        for (int i = 0; i < 5; ++i)
        {
            float x = 5.0f + i * 1.2f, h = 0.4f + i * 0.8f;
            root->addChild(placeNode(
                makeBoxGeode(osg::Vec3(), osg::Vec3(0.12f, h * 0.5f, 0.12f)),
                osg::Vec3(x, h * 0.5f, 5.0f)));
        }
        // Back wall (large receiver tilted 60 deg): classic peter-panning / acne zone
        root->addChild(placeNode(
            makeBoxGeode(osg::Vec3(), osg::Vec3(4.0f, 1.5f, 0.1f)),
            osg::Vec3(-4.0f, 1.5f, -7.0f),
            osg::Quat(osg::PI / 3.0f, osg::Vec3(0.0f, 1.0f, 0.0f))));
        return root.get();
    }
}

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());
    osg::setNotifyHandler(new osgVerse::ConsoleHandler);

    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFiles(arguments);
    if (!scene) scene = createAcneTestScene();  // procedural acne-friendly default
    if (!scene) scene = osgDB::readNodeFile(BASE_DIR + "/models/Sponza.osgb");  // fallback
    if (!scene) { OSG_WARN << "Failed to load any scene" << std::endl; return 1; }

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->addChild(scene.get());
    osgVerse::Pipeline::setPipelineMask(*sceneRoot, DEFERRED_SCENE_MASK | SHADOW_CASTER_MASK);

    // Post-HUD display
    osg::ref_ptr<osg::Camera> postCamera = new osg::Camera;
    postCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    postCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    postCamera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0));
    postCamera->setViewMatrix(osg::Matrix::identity());
    postCamera->setRenderOrder(osg::Camera::POST_RENDER, 10000);
    postCamera->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
    osgVerse::Pipeline::setPipelineMask(*postCamera, FORWARD_SCENE_MASK);

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(sceneRoot.get());
    root->addChild(postCamera.get());

    // Start the viewer
    osg::ref_ptr<osgViewer::Viewer> viewer;
    bool useForwardShadow = arguments.read("--forward");
    if (useForwardShadow)
    {
        viewer = new osgViewer::Viewer;
        // TODO: forward shadowing?
    }
    else
    {
        osgVerse::StandardPipelineViewer* plViewer = new osgVerse::StandardPipelineViewer(false, true, true);
        plViewer->getParameters().enableAO = false;
        plViewer->getParameters().enablePostEffects = false;
        if (arguments.read("--vsm"))
            plViewer->getParameters().shadowTechnique |= osgVerse::ShadowModule::VarianceSM;
        if (arguments.read("--esm"))
            plViewer->getParameters().shadowTechnique |= osgVerse::ShadowModule::ExponentialSM;
        if (arguments.read("--evsm"))
            plViewer->getParameters().shadowTechnique |= osgVerse::ShadowModule::ExponentialVarianceSM;
        if (arguments.read("--band-pcf"))
            plViewer->getParameters().shadowTechnique |= osgVerse::ShadowModule::BandPCF;
        if (arguments.read("--no-pcf"))
            plViewer->getParameters().shadowTechnique &= ~osgVerse::ShadowModule::PossionPCF;
        if (arguments.read("--no-plane-bias"))
            plViewer->getParameters().shadowPlaneFixedBias =
            plViewer->getParameters().shadowPlaneBiasClamp = 0.0f;
        // Blur radius is in world units (not shadow-map texels). The digit in the flag is just a
        // preset index for the strength of the effect; -3 is mild, -7 is strong. A 4096² shadow
        // map has ~0.003 world units per texel, so a value of 0.1 already gives a clearly
        // visible soft edge. See ShadowModule::setShadowFilterSize() for the per-cascade
        // texel conversion
        if (arguments.read("--blur-3")) plViewer->getParameters().shadowFilterSize = 0.1f;
        if (arguments.read("--blur-5")) plViewer->getParameters().shadowFilterSize = 0.3f;
        if (arguments.read("--blur-7")) plViewer->getParameters().shadowFilterSize = 1.0f;
        // Negative arguments override the cast exponent directly:
        //   --esm-alpha X         -> ShadowESMExponents.z
        //   --evsm-positive X     -> ShadowEVSMExponents.x (clamped to 5.54 on half-float)
        //   --evsm-negative X     -> ShadowEVSMExponents.y
        float argF = 0.0f;
        if (arguments.read("--esm-alpha", argF)) plViewer->getParameters().shadowESMDepthScale = argF;
        if (arguments.read("--evsm-positive", argF)) plViewer->getParameters().shadowEVSMPositiveExponent = argF;
        if (arguments.read("--evsm-negative", argF)) plViewer->getParameters().shadowEVSMNegativeExponent = argF;
        viewer = plViewer;
    }

    viewer->addEventHandler(new osgViewer::StatsHandler);
    viewer->addEventHandler(new osgViewer::WindowSizeHandler);
    viewer->addEventHandler(new osgGA::StateSetManipulator(viewer->getCamera()->getOrCreateStateSet()));
    viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    viewer->setSceneData(root.get());
    viewer->setUpViewOnSingleScreen(0);
    viewer->realize();

    osgVerse::Pipeline::Stage* shadowCombine = NULL;
    osg::ref_ptr<osgVerse::ShadowModule> shadow;
    osg::ref_ptr<osgVerse::LightDrawable> light0;
    if (useForwardShadow)
    {
        shadow = new osgVerse::ShadowModule("Shadow", NULL, true);
        std::vector<osgVerse::Pipeline::Stage*> stages = shadow->createStages(2048, 3,
            osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "std_shadow_cast.vert.glsl"),
            osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "std_shadow_cast.frag.glsl"), SHADOW_CASTER_MASK);
        std::vector<osg::ref_ptr<osgVerse::Pipeline::Stage>> refStages(stages.begin(), stages.end());

        osg::ref_ptr<osg::Group> shadowRoot = new osg::Group;
        for (size_t i = 0; i < refStages.size(); ++i)
        {
            refStages[i]->camera->setCullMask(SHADOW_CASTER_MASK);
            refStages[i]->camera->addChild(sceneRoot.get());
            shadowRoot->addChild(refStages[i]->camera.get());
        }
        root->addChild(shadowRoot.get());

        osg::ref_ptr<osgVerse::ShadowDrawCallback> shadowCallback =new osgVerse::ShadowDrawCallback(shadow.get());
        shadowCallback->setup(viewer->getCamera(), PRE_DRAW);
        viewer->getCamera()->addUpdateCallback(shadow.get());
    }
    else
    {
        osgVerse::StandardPipelineViewer* plViewer = static_cast<osgVerse::StandardPipelineViewer*>(viewer.get());
        osgVerse::Pipeline* pipeline = plViewer->getPipeline();
        shadowCombine = pipeline->getStage("Shadowing");
        light0 = static_cast<osgVerse::LightDrawable*>(plViewer->getLightRoot()->getDrawable(0));
        shadow = static_cast<osgVerse::ShadowModule*>(pipeline->getModule("Shadow"));
    }

    // Config shadow settings
    if (shadow)
    {
        if (arguments.read("--vhacd"))
        {
            // Effective Shadow Culling Method 1:
            //    Convert geometries into V-HACD polygons and thus enable fast rendering
            shadow->createCasterGeometries(sceneRoot.get(), SHADOW_CASTER_MASK, 0.1f);
        }

        if (arguments.read("--small-culling"))
        {
            // Effective Shadow Culling Method 2:
            //    Use main camera VPW for small-pixels-culling under shadow cameras
            shadow->setSmallPixelsToCull(0, 5);
            shadow->setSmallPixelsToCull(1, 10);
            shadow->setSmallPixelsToCull(2, 30);
        }

        if (shadow->getFrustumGeode())
        {
            osgVerse::Pipeline::setPipelineMask(*shadow->getFrustumGeode(), FORWARD_SCENE_MASK);
            root->addChild(shadow->getFrustumGeode());
        }

        float quadY = 0.0f;
        if (shadowCombine)
        {
            osg::Node* quad = osgVerse::createScreenQuad(
                osg::Vec3(0.0f, quadY, 0.0f), 0.2f, 0.2f, osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
            quad->getOrCreateStateSet()->setTextureAttributeAndModes(
                0, shadowCombine->getBufferTexture("DebugDepthBuffer"));
            postCamera->addChild(quad); quadY += 0.201f;
        }
        for (int i = 0; i < shadow->getShadowNumber(); ++i)
        {
            // Show the map the combine pass actually samples, so a broken blur chain is visible
            // in the corner quads instead of only in the final image
            osg::Node* quad = osgVerse::createScreenQuad(
                osg::Vec3(0.0f, quadY, 0.0f), 0.2f, 0.2f, osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
            quad->getOrCreateStateSet()->setTextureAttributeAndModes(0, shadow->getFilterableTexture(i));
            postCamera->addChild(quad); quadY += 0.201f;
        }
    }

    float lightX = 0.02f; bool lightD = true, animated = arguments.read("--animated");
    std::cout << "Shadow testing started..." << std::endl;
    while (!viewer->done())
    {
        if (animated)
        {
            if (lightD) { if (lightX > 0.8f) lightD = false; else lightX += 0.001f; }
            else { if (lightX < -0.8f) lightD = true; else lightX -= 0.001f; }
        }

        if (light0.valid()) light0->setDirection(osg::Vec3(lightX, 0.1f, -1.0f));
        else if (shadow.valid()) shadow->setLightState(osg::Vec3(0.0f, 0.0f, 1.0f), osg::Vec3(lightX, 0.1f, -1.0f));
        viewer->frame(); //MicroProfileFlip(NULL);  // see profile at: localhost:1338
    }
    return 0;
}
