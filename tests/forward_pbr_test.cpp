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

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <pipeline/Pipeline.h>
#include <pipeline/LightModule.h>
#include <pipeline/Utilities.h>
#include <readerwriter/Utilities.h>
#include <iostream>
#include <sstream>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

#ifdef OSG_LIBRARY_STATIC
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
#endif
USE_GRAPICSWINDOW_IMPLEMENTATION(SDL)

osg::StateSet* createPbrStateSet(osgVerse::Pipeline* pipeline)
{
    osg::ref_ptr<osg::StateSet> forwardSS = pipeline->createForwardStateSet(
        osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "std_forward_render.vert.glsl"),
        osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "std_forward_render.frag.glsl"));

    osgVerse::LightModule* lm = static_cast<osgVerse::LightModule*>(pipeline->getModule("Light"));
    if (forwardSS.valid() && lm)
    {
        forwardSS->setTextureAttributeAndModes(7, lm->getParameterTable());
        forwardSS->addUniform(new osg::Uniform("LightParameterMap", 7));
        forwardSS->addUniform(lm->getLightNumber());
    }
    return forwardSS.release();
}

int main(int argc, char** argv)
{
    osgVerse::globalInitialize(argc, argv);
    osg::setNotifyHandler(new osgVerse::ConsoleHandler);

    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile(
        argc > 1 ? argv[1] : BASE_DIR + "/models/Sponza/Sponza.gltf.125,125,125.scale");
    if (!scene) { OSG_WARN << "Failed to load GLTF model"; return 1; }

    // Add tangent/bi-normal arrays for normal mapping
    osgVerse::TangentSpaceVisitor tsv; scene->accept(tsv);
    osgVerse::FixedFunctionOptimizer ffo; scene->accept(ffo);

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->addChild(scene.get());

    osg::ref_ptr<osgVerse::LightDrawable> light0 = new osgVerse::LightDrawable;
    light0->setColor(osg::Vec3(1.5f, 1.5f, 1.2f));
    light0->setDirection(osg::Vec3(0.02f, 0.1f, -1.0f));
    light0->setDirectional(true);

    osg::ref_ptr<osgVerse::LightDrawable> light1 = new osgVerse::LightDrawable;
    light1->setColor(osg::Vec3(0.9f, 0.9f, 1.1f));
    light1->setDirection(osg::Vec3(-0.4f, 0.6f, 0.1f));
    light1->setDirectional(true);

    osg::ref_ptr<osg::Geode> lightGeode = new osg::Geode;
    lightGeode->addDrawable(light0.get());  // Main light
    lightGeode->addDrawable(light1.get());

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(sceneRoot.get());
    root->addChild(lightGeode.get());

    // The pipeline only for shader construction and lighting
    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline;
    osg::ref_ptr<osgVerse::LightModule> lightModule = new osgVerse::LightModule("Light", pipeline.get());
    lightModule->setMainLight(light0.get(), "");  // no shadow module
    sceneRoot->setStateSet(createPbrStateSet(pipeline.get()));

    // Start the viewer
    osgViewer::Viewer viewer;
    viewer.getCamera()->addUpdateCallback(lightModule.get());
    viewer.setUpViewOnSingleScreen(0);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    return viewer.run();
}
