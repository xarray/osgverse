#include <osg/io_utils>
#include <osg/LightSource>
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

#include <ai/RecastManager.h>
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments(&argc, argv);
    osg::ref_ptr<osg::Node> terrain = osgDB::readNodeFiles(arguments);
    if (!terrain) terrain = osgDB::readNodeFile("lz.osg");

    osg::ref_ptr<osgVerse::RecastManager> recast = new osgVerse::RecastManager;
    std::ifstream dataIn("recast_terrain.bin", std::ios::in | std::ios::binary);
    if (!dataIn)
    {
        std::ofstream dataOut("recast_terrain.bin", std::ios::out | std::ios::binary);
        recast->build(terrain.get()); recast->save(dataOut);
    }
    else
    { recast->read(dataIn); OSG_NOTICE << "Read recast data from file." << std::endl; }
    recast->initializeAgents();

    osg::ref_ptr<osg::MatrixTransform> player = new osg::MatrixTransform;
    player->setMatrix(osg::Matrix::translate(0.0f, 0.0f, 100.0f));
    player->addChild(osgDB::readNodeFile("dumptruck.osgt"));

    osg::ref_ptr<osgVerse::RecastManager::Agent> agent =
        new osgVerse::RecastManager::Agent(player.get(), osg::Vec3(100.0f, 100.0f, 140.0f));
    recast->updateAgent(agent.get());

    osg::ref_ptr<osg::MatrixTransform> debugNode = new osg::MatrixTransform;
    debugNode->addChild(recast->getDebugMesh());
    debugNode->setMatrix(osg::Matrix::translate(0.0f, 0.0f, 1.0f));
    debugNode->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    debugNode->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    debugNode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(terrain.get());
    root->addChild(player.get());
    root->addChild(debugNode.get());

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    while (!viewer.done())
    {
        recast->advance(viewer.getFrameStamp()->getSimulationTime());
        viewer.frame();
    }
    return 0;
}
