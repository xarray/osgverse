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

#include <animation/ParticleEngine.h>
#include <pipeline/Utilities.h>
#include <readerwriter/Utilities.h>
#include <iostream>
#include <sstream>

#ifdef OSG_LIBRARY_STATIC
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
#endif

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osg::ref_ptr<osg::Group> root = new osg::Group;

    osg::ref_ptr<osg::Node> scene =
        (argc < 2) ? osgDB::readNodeFile("cessna.osg") : osgDB::readNodeFiles(arguments);
    if (!scene) { OSG_WARN << "Failed to load " << (argc < 2) ? "" : argv[1]; return 1; }
    root->addChild(scene.get());

    osg::ref_ptr<osgVerse::ParticleDrawable> particle = new osgVerse::ParticleDrawable;
    particle->createEffect("test", BASE_DIR + "/models/Particles/Fireworks.efkefc");
    particle->playEffect("test", osgVerse::ParticleDrawable::PLAYING);
    particle->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    particle->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

    osg::ref_ptr<osg::Geode> particleNode = new osg::Geode;
    particleNode->addDrawable(particle.get());

    osg::ref_ptr<osg::MatrixTransform> particleMT = new osg::MatrixTransform;
    particleMT->setMatrix(osg::Matrix::translate(0.0f, 0.0f, 10.0f));
    particleMT->addChild(particleNode.get());
    root->addChild(particleMT.get());

    // Start the main loop
    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}
