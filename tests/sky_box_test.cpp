#include <osg/io_utils>
#include <osg/TriangleIndexFunctor>
#include <osg/Texture2D>
#include <osg/PagedLOD>
#include <osg/ProxyNode>
#include <osg/MatrixTransform>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <VerseCommon.h>
#include <pipeline/SkyBox.h>
#include <iostream>
#include <sstream>

#ifdef OSG_LIBRARY_STATIC
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
USE_SERIALIZER_WRAPPER(DracoGeometry)
#endif

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());
    osg::ref_ptr<osg::Group> root = new osg::Group;
    osgVerse::updateOsgBinaryWrappers();

#if true
    osg::ref_ptr<osg::ProxyNode> scene = new osg::ProxyNode;
    scene->setFileName(0, (argc < 2) ? "cessna.osg" : argv[1]);
#else
    osg::ref_ptr<osg::Node> scene =
        (argc < 2) ? osgDB::readNodeFile("cessna.osg") : osgDB::readNodeFiles(arguments);
    if (!scene) { OSG_WARN << "Failed to load " << (argc < 2) ? "" : argv[1]; return 1; }
#endif
    root->addChild(scene.get());

    // Post-HUD display
    osg::ref_ptr<osg::Camera> postCamera = osgVerse::SkyBox::createSkyCamera();
    root->addChild(postCamera.get());

    osg::ref_ptr<osgVerse::SkyBox> skybox = new osgVerse::SkyBox;
    {
        skybox->setSkyShaders(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "skybox.vert.glsl"),
                              osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "skybox.frag.glsl"));
        skybox->setEnvironmentMap(osgDB::readImageFile(SKYBOX_DIR + "sunset.png"));
        postCamera->addChild(skybox.get());
    }

    // Start the main loop
    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);

    std::cout << ".... Phase 0: system init\n";
    osg::Timer_t t0 = osg::Timer::instance()->tick();
    for (int i = 0; i < 10; ++i) viewer.frame();

    std::cout << ".... Phase 1: data loading\n";
    osg::Timer_t t1 = osg::Timer::instance()->tick();
    while (viewer.getDatabasePager()->getRequestsInProgress() ||
           viewer.getDatabasePager()->getDataToMergeListSize() > 0) viewer.frame();
    viewer.getCameraManipulator()->home(0.0);

    std::cout << ".... Phase 2: data merging\n";
    osg::Timer_t t2 = osg::Timer::instance()->tick();
    for (int i = 0; i < 10; ++i) viewer.frame();

    osg::Timer_t t3 = osg::Timer::instance()->tick();
    std::cout << ".... P0 .... P1 .... P2 ....\n ";
    std::cout << osg::Timer::instance()->delta_m(t0, t1) << ", "
              << osg::Timer::instance()->delta_m(t1, t2) << ", "
              << osg::Timer::instance()->delta_m(t2, t3) << "\n";
    return viewer.run();
}
