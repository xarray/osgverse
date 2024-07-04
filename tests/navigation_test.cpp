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

#include <pipeline/IntersectionManager.h>
#include <readerwriter/Utilities.h>
#include <ai/RecastManager.h>
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

class InteractiveHandler : public osgGA::GUIEventHandler
{
public:
    InteractiveHandler(osg::Group* root, osgVerse::RecastManager* rm)
        : _root(root), _recast(rm) {}

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        if (ea.getEventType() == osgGA::GUIEventAdapter::RELEASE &&
            (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL))
        {

        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::DOUBLECLICK)
        {

        }
        return false;
    }

protected:
    osg::observer_ptr<osg::Group> _root;
    osg::observer_ptr<osgVerse::RecastManager> _recast;
};

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments(&argc, argv);
    osgVerse::fixOsgBinaryWrappers();

    osg::ref_ptr<osg::Node> terrain = osgDB::readNodeFiles(arguments);
    if (!terrain) terrain = osgDB::readNodeFile("lz.osg");

    osg::ref_ptr<osgVerse::RecastManager> recast = new osgVerse::RecastManager;
    std::ifstream dataIn("recast_terrain.bin", std::ios::in | std::ios::binary);
    if (!dataIn)
    {
        std::ofstream dataOut("recast_terrain.bin", std::ios::out | std::ios::binary);
        if (recast->build(terrain.get(), true)) recast->save(dataOut);
        else { OSG_WARN << "Failed to build nav-mesh." << std::endl; return -1; }
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
    //debugNode->setMatrix(osg::Matrix::translate(0.0f, 0.0f, 1.0f));
    debugNode->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    debugNode->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    debugNode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    debugNode->getOrCreateStateSet()->setMode(GL_DEPTH, osg::StateAttribute::OFF);

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(terrain.get());
    root->addChild(player.get());
    root->addChild(debugNode.get());

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new InteractiveHandler(root.get(), recast.get()));
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
