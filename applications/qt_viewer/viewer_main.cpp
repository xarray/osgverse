#include <osg/ComputeBoundsVisitor>
#include <osg/Texture2D>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/ViewerEventHandlers>

#include <pipeline/SkyBox.h>
#include <pipeline/Pipeline.h>
#include <pipeline/LightModule.h>
#include <pipeline/ShadowModule.h>
#include <pipeline/Utilities.h>
#include "qt_header.h"
#include <iostream>
#include <sstream>

void OsgSceneWidget::initializeScene(int argc, char** argv)
{
    osgVerse::globalInitialize(argc, argv);
    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile(
        argc > 1 ? argv[1] : BASE_DIR "/models/Sponza/Sponza.gltf");
    if (!scene) scene = new osg::Group;

    // Add tangent/bi-normal arrays for normal mapping
    osgVerse::TangentSpaceVisitor tsv;
    scene->accept(tsv);

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->addChild(scene.get());
    sceneRoot->setNodeMask(DEFERRED_SCENE_MASK | SHADOW_CASTER_MASK);
    sceneRoot->setMatrix(osg::Matrix::rotate(osg::PI_2, osg::X_AXIS));

    osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osgt.15,15,1.scale.0,0,-300.trans");
    //osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osgt.0,0,-250.trans");
    otherSceneRoot->setNodeMask(~DEFERRED_SCENE_MASK);

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(otherSceneRoot.get());
    root->addChild(sceneRoot.get());

    // Main light
    osg::ref_ptr<osgVerse::LightDrawable> light0 = new osgVerse::LightDrawable;
    light0->setColor(osg::Vec3(4.0f, 4.0f, 3.8f));
    light0->setDirection(osg::Vec3(0.02f, 0.1f, -1.0f));
    light0->setDirectional(true);

    osg::ref_ptr<osg::Geode> lightGeode = new osg::Geode;
    lightGeode->addDrawable(light0.get());
    root->addChild(lightGeode.get());

    // Start the pipeline, with camera assoicated with embedded Qt window
    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline;
    _viewer = new MyViewer(pipeline.get());
    _viewer->getCamera()->setViewport(0, 0, this->width(), this->height());
    _viewer->getCamera()->setGraphicsContext(_graphicsWindow.get());

    osgVerse::StandardPipelineParameters params(SHADER_DIR, SKYBOX_DIR "sunset.png");
    setupStandardPipeline(pipeline.get(), _viewer.get(), params);

    // Setup shadow & light module
    osgVerse::ShadowModule* shadow = static_cast<osgVerse::ShadowModule*>(pipeline->getModule("Shadow"));
    if (shadow && shadow->getFrustumGeode())
    {
        shadow->getFrustumGeode()->setNodeMask(FORWARD_SCENE_MASK);
        root->addChild(shadow->getFrustumGeode());
    }

    osgVerse::LightModule* light = static_cast<osgVerse::LightModule*>(pipeline->getModule("Light"));
    if (light) light->setMainLight(light0.get(), "Shadow");

    // Post-HUD display
    osg::ref_ptr<osg::Camera> postCamera = osgVerse::SkyBox::createSkyCamera();
    root->addChild(postCamera.get());

    osg::ref_ptr<osgVerse::SkyBox> skybox = new osgVerse::SkyBox;
    {
        skybox->setEnvironmentMap(params.skyboxMap.get(), false);
        skybox->setNodeMask(~DEFERRED_SCENE_MASK);
        postCamera->addChild(skybox.get());
    }

    // Start the viewer
    _viewer->addEventHandler(new osgViewer::StatsHandler);
    _viewer->addEventHandler(new osgViewer::WindowSizeHandler);
    _viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    _viewer->setKeyEventSetsDone(0);
    _viewer->setSceneData(root.get());

    // FIXME: how to avoid shadow problem...
    // If renderer->setGraphicsThreadDoesCull(false), which is used by DrawThreadPerContext & ThreadPerCamera,
    // Shadow will go jigger because the output texture is not sync-ed before lighting...
    // For SingleThreaded & CullDrawThreadPerContext it seems OK
    _viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

    OsgSceneWidget w;
    w.initializeScene(argc, argv);
    w.setGeometry(50, 50, 1280, 720);
    w.show();
    return app.exec();
}
