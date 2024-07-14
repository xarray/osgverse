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
#include <pipeline/Utilities.h>
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
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        if (ea.getEventType() == osgGA::GUIEventAdapter::RELEASE &&
            (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL))
        {
            osgVerse::IntersectionResult result = osgVerse::findNearestIntersection(
                view->getCamera(), ea.getXnormalized(), ea.getYnormalized());
            osg::Node* agent = result.findNode([](osg::Node* node)
            { if (node->getName() == "RecastAgent") return true; return false; });

            if (!result.drawable) return false;
            if (agent == NULL)  // click on ground to create new agent
            {
                // TODO
            }
            else  // click on an agent to select it
            {
                // TODO: show a bounding box?
                _selectedAgent = agent->asGroup();
                std::cout << "Selected an agent " << _selectedAgent.get() << std::endl;
            }
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::DOUBLECLICK)
        {
            if (_selectedAgent != NULL)
            {
                osgVerse::IntersectionResult result = osgVerse::findNearestIntersection(
                    view->getCamera(), ea.getXnormalized(), ea.getYnormalized());
                osg::Node* agent = result.findNode([](osg::Node* node)
                { if (node->getName() == "RecastAgent") return true; return false; });
                if (agent != NULL) return false;  // nothing to do

                // Select a new target for position current agent
                osgVerse::RecastManager::Agent* aData = static_cast<osgVerse::RecastManager::Agent*>(
                    _recast->getAgentFromNode(_selectedAgent.get()));
                if (aData != NULL)
                {
                    aData->target = result.getWorldIntersectPoint(); _recast->updateAgent(aData);
                    std::cout << "Set new position of " << _selectedAgent.get() << std::endl;
                }
            }
        }
        return false;
    }

protected:
    osg::observer_ptr<osg::Group> _root, _selectedAgent;
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
    player->addChild(osgDB::readNodeFile("dumptruck.osgt.0,0,2.trans"));
    player->setName("RecastAgent");

    osg::ref_ptr<osgVerse::RecastManager::Agent> agent =
        new osgVerse::RecastManager::Agent(player.get(), osg::Vec3(100.0f, 100.0f, 140.0f));
    agent->maxSpeed = 15.0f; recast->updateAgent(agent.get());

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

    //osg::Geode* geode = new osg::Geode;
    //geode->addDrawable(new osg::ShapeDrawable(
    //    osgVerse::createHeightField(terrain.get(), 4096, 4096)));
    //root->addChild(geode);

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
