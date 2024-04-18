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

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()

static osg::Group* loadBasicScene(int argc, char** argv)
{
    osgVerse::globalInitialize(argc, argv);
#if true
    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(osgDB::readNodeFile("cow.osg"));
#else
    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile(
        argc > 1 ? argv[1] : BASE_DIR "/models/Sponza/Sponza.gltf");
    if (!scene) scene = new osg::Group;

    // Add tangent/bi-normal arrays for normal mapping
    osgVerse::TangentSpaceVisitor tsv;
    scene->accept(tsv);

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->addChild(scene.get());
    sceneRoot->setMatrix(osg::Matrix::rotate(osg::PI_2, osg::X_AXIS));
    osgVerse::Pipeline::setPipelineMask(*sceneRoot, DEFERRED_SCENE_MASK | SHADOW_CASTER_MASK);

    osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osg.15,15,1.scale.0,0,-300.trans");
    //osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osg.0,0,-250.trans");
    if (otherSceneRoot.valid())
        osgVerse::Pipeline::setPipelineMask(*otherSceneRoot, FORWARD_SCENE_MASK);

    osg::ref_ptr<osg::Group> root = new osg::Group;
    if (argc == 1) root->addChild(otherSceneRoot.get());
    root->addChild(sceneRoot.get());
#endif
    return root.release();
}

void OsgFramebufferObject::initializeScene()
{
    osg::ref_ptr<osg::Group> root = loadBasicScene(0, NULL);

#if true
    _viewer = new osgViewer::Viewer;
    _viewer->getCamera()->setViewport(0, 0, 640, 480);
    _viewer->getCamera()->setGraphicsContext(_graphicsWindow.get());
#else
    // Main light
    osg::ref_ptr<osgVerse::LightDrawable> light0 = new osgVerse::LightDrawable;
    light0->setColor(osg::Vec3(4.0f, 4.0f, 3.8f));
    light0->setDirection(osg::Vec3(0.02f, 0.1f, -1.0f));
    light0->setDirectional(true);

    osg::ref_ptr<osg::Geode> lightGeode = new osg::Geode;
    lightGeode->addDrawable(light0.get());
    root->addChild(lightGeode.get());

    // Start the pipeline, with camera assoicated with embedded Qt window
    static osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline;
    _viewer = new MyViewer(pipeline.get());
    _viewer->getCamera()->setViewport(0, 0, 640, 480);
    _viewer->getCamera()->setGraphicsContext(_graphicsWindow.get());

    // Setup the pipeline
    _params = osgVerse::StandardPipelineParameters(SHADER_DIR, SKYBOX_DIR "sunset.png");
    setupStandardPipeline(pipeline.get(), _viewer, _params);

    // Setup shadow & light module
    osgVerse::ShadowModule* shadow = static_cast<osgVerse::ShadowModule*>(pipeline->getModule("Shadow"));
    if (shadow && shadow->getFrustumGeode())
    {
        osgVerse::Pipeline::setPipelineMask(*shadow->getFrustumGeode(), FORWARD_SCENE_MASK);
        root->addChild(shadow->getFrustumGeode());
    }

    osgVerse::LightModule* light = static_cast<osgVerse::LightModule*>(pipeline->getModule("Light"));
    if (light) light->setMainLight(light0.get(), "Shadow");

    // Post-HUD display
    osg::ref_ptr<osg::Camera> postCamera = osgVerse::SkyBox::createSkyCamera();
    root->addChild(postCamera.get());

    osg::ref_ptr<osgVerse::SkyBox> skybox = new osgVerse::SkyBox(pipeline.get());
    {
        skybox->setSkyShaders(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "skybox.vert.glsl"),
                              osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "skybox.frag.glsl"));
        skybox->setEnvironmentMap(_params.skyboxMap.get(), false);
        postCamera->addChild(skybox.get());
        osgVerse::Pipeline::setPipelineMask(*skybox, FORWARD_SCENE_MASK);
    }
#endif

    // Start the embedded viewer
    _viewer->addEventHandler(new osgViewer::StatsHandler);
    _viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    _viewer->setKeyEventSetsDone(0);
    _viewer->setSceneData(root.get());
    _viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    qmlRegisterType<OsgFramebufferObject>("osgVerse", 1, 0, "OsgFramebufferObject");

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [](QObject* obj, const QUrl& objUrl)
    { if (!obj) QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.load(QUrl(QStringLiteral("qrc:/qt_interface.qml")));
    return app.exec();
}
