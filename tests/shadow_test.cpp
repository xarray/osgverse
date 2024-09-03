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
#include <readerwriter/Utilities.h>
#include <iostream>
#include <sstream>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

class MyViewer : public osgViewer::Viewer
{
public:
    MyViewer(osgVerse::Pipeline* p) : osgViewer::Viewer(), _pipeline(p) {}
    osg::ref_ptr<osgVerse::Pipeline> _pipeline;

protected:
    virtual osg::GraphicsOperation* createRenderer(osg::Camera* camera)
    {
        if (_pipeline.valid()) return _pipeline->createRenderer(camera);
        else return osgViewer::Viewer::createRenderer(camera);
    }
};

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osg::ref_ptr<osg::Node> scene = (argc > 1) ? osgDB::readNodeFiles(arguments)
                                  : osgDB::readNodeFile(BASE_DIR "/models/Sponza.osgb");
    if (!scene) { OSG_WARN << "Failed to load GLTF model"; return 1; }

    // Add tangent/bi-normal arrays for normal mapping
    osgVerse::TangentSpaceVisitor tsv; scene->accept(tsv);
    osgVerse::FixedFunctionOptimizer ffo; scene->accept(ffo);

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->addChild(scene.get());
    sceneRoot->setMatrix(osg::Matrix::rotate(osg::PI_2, osg::X_AXIS));
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
    osgVerse::StandardPipelineViewer viewer(false, true, true);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);
    viewer.realize();

    // Config shadow settings
    osgVerse::Pipeline* pipeline = viewer.getPipeline();
    osgVerse::LightDrawable* light0 =
        static_cast<osgVerse::LightDrawable*>(viewer.getLightRoot()->getDrawable(0));

    osgVerse::ShadowModule* shadow = static_cast<osgVerse::ShadowModule*>(pipeline->getModule("Shadow"));
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
        for (int i = 0; i < shadow->getShadowNumber(); ++i)
        {
            osg::Node* quad = osgVerse::createScreenQuad(
                osg::Vec3(0.0f, quadY, 0.0f), 0.2f, 0.2f, osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
            quad->getOrCreateStateSet()->setTextureAttributeAndModes(0, shadow->getTexture(i));
            postCamera->addChild(quad); quadY += 0.21f;
        }
    }

    float lightX = 0.02f; bool lightD = true, animated = arguments.read("--animated");
    std::cout << "Shadow testing started..." << std::endl;
    while (!viewer.done())
    {
        if (animated)
        {
            if (lightD) { if (lightX > 0.8f) lightD = false; else lightX += 0.001f; }
            else { if (lightX < -0.8f) lightD = true; else lightX -= 0.001f; }
        }
        light0->setDirection(osg::Vec3(lightX, 0.1f, -1.0f));
        viewer.frame();
    }
    return 0;
}
