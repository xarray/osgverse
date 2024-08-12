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
#include <pipeline/Pipeline.h>
#include <readerwriter/Utilities.h>
#include <ai/RecastManager.h>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

class InteractiveHandler : public osgGA::GUIEventHandler
{
public:
    InteractiveHandler(osg::Group* root, osg::Node* ag, osgVerse::RecastManager* rm)
        : _agentNode(ag), _root(root), _recast(rm)
    { _axesNode = osgDB::readNodeFile("axes.osgt.5,5,5.scale"); }

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        if (ea.getEventType() == osgGA::GUIEventAdapter::RELEASE &&
            (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL))
        {
            osgVerse::IntersectionResult result = osgVerse::findNearestIntersection(
                view->getCamera(), ea.getXnormalized(), ea.getYnormalized());
            if (!result.drawable) return false;

            osg::Node* agent = result.findNode([](osg::Node* node)
            { return (node->getName() == "RecastAgent"); });
            select(agent ? agent->asGroup() : NULL);
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::DOUBLECLICK)
        {
            osgVerse::IntersectionResult result = osgVerse::findNearestIntersection(
                view->getCamera(), ea.getXnormalized(), ea.getYnormalized());
            if (!result.drawable) return false;

            if (_selectedAgent != NULL)
            {
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
            else  // click on ground to create new agent
            {
                osg::ref_ptr<osg::MatrixTransform> player = new osg::MatrixTransform;
                player->setMatrix(osg::Matrix::translate(result.getWorldIntersectPoint()));
                player->addChild(_agentNode.get()); player->setName("RecastAgent");
                osgVerse::Pipeline::setPipelineMask(*player, DEFERRED_SCENE_MASK & (~SHADOW_CASTER_MASK));
                select(player.get()); _root->addChild(player.get());

                osg::ref_ptr<osgVerse::RecastManager::Agent> agent =
                    new osgVerse::RecastManager::Agent(player.get(), result.getWorldIntersectPoint());
                agent->maxSpeed = 5.0f; agent->maxAcceleration = 8.0f;
                _recast->updateAgent(agent.get());
            }
        }
        return false;
    }

    void select(osg::Group* agent)
    {
        if (_selectedAgent.valid()) _selectedAgent->removeChild(_axesNode.get());
        _selectedAgent = agent; if (_selectedAgent.valid()) _selectedAgent->addChild(_axesNode.get());
    }

protected:
    osg::ref_ptr<osg::Node> _agentNode, _axesNode;
    osg::observer_ptr<osg::Group> _root, _selectedAgent;
    osg::observer_ptr<osgVerse::RecastManager> _recast;
};

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osgVerse::fixOsgBinaryWrappers();

    std::string agentPath = "dumptruck.osgt"; arguments.read("--agent", agentPath);
    std::string recastData = "recast_terrain.bin"; arguments.read("--recast", recastData);
    osg::ref_ptr<osg::Node> agentNode = osgDB::readNodeFile(agentPath);
    osg::ref_ptr<osg::Node> terrain = osgVerse::readNodeFiles(arguments);
    if (!terrain) terrain = osgDB::readNodeFile("lz.osg");

    osg::ref_ptr<osgVerse::RecastManager> recast = new osgVerse::RecastManager;
    if (agentNode.valid())
    {
        osgVerse::RecastSettings settings = recast->getSettings();
        settings.agentRadius = agentNode->getBound().radius();
        recast->setSettings(settings);
    }

    std::ifstream dataIn(recastData, std::ios::in | std::ios::binary);
    if (!dataIn)
    {
        std::ofstream dataOut(recastData, std::ios::out | std::ios::binary);
        if (recast->build(terrain.get(), true)) recast->save(dataOut);
        else { OSG_WARN << "Failed to build nav-mesh." << std::endl; return -1; }
    }
    else
    { recast->read(dataIn); OSG_NOTICE << "Read recast data from file." << std::endl; }
    recast->initializeAgents();

    osg::ref_ptr<osg::MatrixTransform> debugNode = new osg::MatrixTransform;
    debugNode->addChild(recast->getDebugMesh());
    //debugNode->setMatrix(osg::Matrix::translate(0.0f, 0.0f, 1.0f));
    debugNode->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    debugNode->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    debugNode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    debugNode->getOrCreateStateSet()->setMode(GL_DEPTH, osg::StateAttribute::OFF);

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(terrain.get()); root->addChild(debugNode.get());
    osgVerse::Pipeline::setPipelineMask(*terrain, DEFERRED_SCENE_MASK & (~SHADOW_CASTER_MASK));

    osg::Geode* geode = new osg::Geode;
    //geode->addDrawable(new osg::ShapeDrawable(
    //    osgVerse::createHeightField(terrain.get(), 4096, 4096)));
    //root->addChild(geode);

#if true
    osgVerse::StandardPipelineViewer viewer;
#else
    osgViewer::Viewer viewer;
#endif
    viewer.addEventHandler(new InteractiveHandler(root.get(), agentNode.get(), recast.get()));
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    while (!viewer.done())
    {
        recast->advance(viewer.getFrameStamp()->getSimulationTime(), 2.5f);
        viewer.frame();
    }
    return 0;
}
