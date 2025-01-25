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
    osgVerse::globalInitialize(argc, argv);
    osg::setNotifyHandler(new osgVerse::ConsoleHandler);
    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile(
        argc > 1 ? argv[1] : BASE_DIR + "/models/Sponza/Sponza.gltf");
    if (!scene) scene = new osg::Group;

    // Add tangent/bi-normal arrays for normal mapping
    osgVerse::TangentSpaceVisitor tsv; scene->accept(tsv);
    osgVerse::FixedFunctionOptimizer ffo; scene->accept(ffo);

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->addChild(scene.get());
    sceneRoot->setMatrix(osg::Matrix::rotate(osg::PI_2, osg::X_AXIS));
    osgVerse::Pipeline::setPipelineMask(*sceneRoot, DEFERRED_SCENE_MASK | SHADOW_CASTER_MASK);

    osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osg.(0.15,0.15,0.15).scale.0,0,-20.trans");
    if (otherSceneRoot.valid())
        osgVerse::Pipeline::setPipelineMask(*otherSceneRoot, FORWARD_SCENE_MASK);

    osg::ref_ptr<osg::Group> root = new osg::Group;
    if (argc < 2) root->addChild(otherSceneRoot.get());
    root->addChild(sceneRoot.get());
    return root.release();
}

osg::Group* OsgSceneWidget::initializeScene(int argc, char** argv, osg::Group* sharedScene)
{
    osg::ref_ptr<osg::Group> root = (sharedScene != NULL)
                                  ? sharedScene : loadBasicScene(argc, argv);

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
    _viewer->getCamera()->setViewport(0, 0, this->width(), this->height());
    _viewer->getCamera()->setGraphicsContext(_graphicsWindow.get());

    // Setup the pipeline
    _params = osgVerse::StandardPipelineParameters(SHADER_DIR, SKYBOX_DIR + "sunset.png");
#if true
    setupStandardPipeline(pipeline.get(), _viewer, _params);
#else
    std::ifstream ppConfig(SHADER_DIR "/standard_pipeline.json");
    pipeline->load(ppConfig, _viewer);
#endif

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

    // Start the embedded viewer
    _viewer->addEventHandler(new osgViewer::StatsHandler);
    _viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    _viewer->setKeyEventSetsDone(0);
    _viewer->setSceneData(root.get());
    _viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    return root.get();
}

#if USE_QMAINWINDOW
void MainWindow::addNewView()
{
    OsgSceneWidget* w = new OsgSceneWidget;
    _sceneRoot = w->initializeScene(0, NULL, _sceneRoot.get());  // FIXME
    connect(this, SIGNAL(updateRequired()), w, SLOT(update()));
    _allocatedWidgets.push_back(w);

    QDockWidget* dock = new QDockWidget(QObject::tr("OsgDock"), this);
    dock->setAllowedAreas(Qt::DockWidgetArea::AllDockWidgetAreas);
    dock->setWidget(w);
    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, dock);
}

void MainWindow::removeLastView()
{
    if (_allocatedWidgets.size() < 2) return;
    OsgSceneWidget* w = _allocatedWidgets.back();
    w->deleteLater();
    _allocatedWidgets.pop_back();
}
#endif

int main(int argc, char** argv)
{
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    QApplication app(argc, argv);

#if USE_QMAINWINDOW
    MainWindow mw;
    {
        QMenu* menu = mw.menuBar()->addMenu(QObject::tr("&Operation"));
        menu->addAction(QObject::tr("&Add View"), &mw, SLOT(addNewView()));
        menu->addAction(QObject::tr("&Remove View"), &mw, SLOT(removeLastView()));
    }

    mw.addNewView();
    mw.setGeometry(50, 50, 1280, 720);
    mw.show();
#else
    OsgSceneWidget w;
    w.initializeScene(argc, argv);
    w.setGeometry(50, 50, 1280, 720);
    w.show();
#endif

    RenderingThread thread(&app);
#if USE_QMAINWINDOW
    app.connect(&thread, SIGNAL(updateRequired()), &mw, SIGNAL(updateRequired()));
#else
    app.connect(&thread, SIGNAL(updateRequired()), &w, SLOT(update()));
#endif
    app.connect(&app, SIGNAL(lastWindowClosed()), &thread, SLOT(setDone()));
    app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

    thread.start();
    app.exec();
    while (!thread.isFinished()) {}
    return 0;
}
