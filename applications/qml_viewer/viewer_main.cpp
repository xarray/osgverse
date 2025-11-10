#include "qt_header.h"
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
#include <readerwriter/Utilities.h>
#include <iostream>
#include <sstream>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()

static osg::Group* loadBasicScene(int argc, char** argv)
{
    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile(
        argc > 1 ? argv[1] : BASE_DIR + "/models/Sponza/Sponza.gltf");
    if (!scene) scene = new osg::Group;

    // Add tangent/bi-normal arrays for normal mapping
    osgVerse::TangentSpaceVisitor tsv; scene->accept(tsv);
    osgVerse::FixedFunctionOptimizer ffo; scene->accept(ffo);

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->addChild(scene.get());
    osgVerse::Pipeline::setPipelineMask(*sceneRoot, DEFERRED_SCENE_MASK | SHADOW_CASTER_MASK);

    osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osg.(0.15,0.15,0.15).scale.0,0,-20.trans");
    if (otherSceneRoot.valid())
        osgVerse::Pipeline::setPipelineMask(*otherSceneRoot, FORWARD_SCENE_MASK);

    osg::ref_ptr<osg::Group> root = new osg::Group;
    if (argc < 2) root->addChild(otherSceneRoot.get());
    root->addChild(sceneRoot.get());
    return root.release();
}

void OsgFramebufferObject::initializeScene(bool pbrMode)
{
    osg::ref_ptr<osg::Group> root = loadBasicScene(0, NULL);
    if (!pbrMode)
    {
        _view = new MyView(NULL);
        _view->getCamera()->setViewport(
            0, 0, _graphicsWindow->getTraits()->width, _graphicsWindow->getTraits()->height);
        _view->getCamera()->setGraphicsContext(_graphicsWindow.get());
    }
    else
    {
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
        _view = new MyView(pipeline.get());
        _view->getCamera()->setViewport(
            0, 0, _graphicsWindow->getTraits()->width, _graphicsWindow->getTraits()->height);
        _view->getCamera()->setGraphicsContext(_graphicsWindow.get());

        // Setup the pipeline
        _params = osgVerse::StandardPipelineParameters(SHADER_DIR, SKYBOX_DIR + "sunset.png");
        setupStandardPipeline(pipeline.get(), _view, _params);

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
            skybox->setSkyShaders(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "skybox.vert.glsl"),
                                  osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "skybox.frag.glsl"));
            skybox->setEnvironmentMap(_params.skyboxMap.get(), false);
            postCamera->addChild(skybox.get());
            osgVerse::Pipeline::setPipelineMask(*skybox, FORWARD_SCENE_MASK);
        }
    }

    // Start the embedded viewer
    _view->addEventHandler(new osgViewer::StatsHandler);
    _view->setCameraManipulator(new osgGA::TrackballManipulator);
    _view->setSceneData(root.get());
}

class ViewerHelper
{
public:
    static void updateViewer(QApplication& app, MyCompositeViewer& viewer)
    { viewer.frameNoRendering(); }

    static void createViewer(QApplication& app, QQmlApplicationEngine& engine,
                             MyCompositeViewer& viewer)
    {
        QObject* root = engine.rootObjects().first();
        if (!root) { qWarning() << "No QML root"; return; }

        QQuickItem* item0 = root->findChild<QQuickItem*>("main_view");
        QQuickItem* item1 = root->findChild<QQuickItem*>("sub_view");

        OsgFramebufferObject* osgView = qobject_cast<OsgFramebufferObject*>(item0);
        if (osgView && osgView->getView()) { osgView->setParentViewer(&viewer); viewer.addView(osgView->getView()); }

        osgView = qobject_cast<OsgFramebufferObject*>(item1);
        if (osgView && osgView->getView()) { osgView->setParentViewer(&viewer); viewer.addView(osgView->getView()); }

        if (osgView && osgView->window())
        {
            QObject::connect(osgView->window(), &QQuickWindow::beforeRendering, &app, [&app, &viewer]()
            { ViewerHelper::updateViewer(app, viewer); }, Qt::DirectConnection);
        }
        else
            qWarning() << "No window found for main 3D view item. Viewer won't work.";
    }
};

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    qmlRegisterType<OsgFramebufferObject>("osgVerse", 1, 0, "OsgFramebufferObject");

    osgVerse::globalInitialize(argc, argv);
    osg::setNotifyHandler(new osgVerse::ConsoleHandler);

    MyCompositeViewer viewer;
    viewer.setKeyEventSetsDone(0);
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [&app, &engine, &viewer](QObject* obj, const QUrl& objUrl)
    { if (obj) ViewerHelper::createViewer(app, engine, viewer); }, Qt::QueuedConnection);

    engine.load(QUrl(QStringLiteral("qrc:/qt_interface.qml")));
    return app.exec();
}
