#include <osg/io_utils>
#include <osg/MatrixTransform>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <imgui/imgui.h>
#include <imgui/ImGuizmo.h>
#include <ui/ImGui.h>
#include <ui/ImGuiComponents.h>
#include <ui/CommandHandler.h>
#include <pipeline/Pipeline.h>
#include <pipeline/Utilities.h>

#include "hierarchy.h"
#include "properties.h"
#include "scenelogic.h"
#include <iostream>
#include <sstream>

class EditorContentHandler : public osgVerse::ImGuiContentHandler
{
public:
    EditorContentHandler(osg::Camera* camera, osg::MatrixTransform* mt)
    {
        _hierarchy = new Hierarchy(camera, mt);
        _properties = new Properties(camera, mt);
        _sceneLogic = new SceneLogic(camera, mt);
    }

    virtual void runInternal(osgVerse::ImGuiManager* mgr)
    {
        ImGui::PushFont(ImGuiFonts["LXGWWenKaiLite-Regular"]);
        handleCommands();

        // TODO: auto layout
        _hierarchy->show(mgr, this);
        _properties->show(mgr, this);
        _sceneLogic->show(mgr, this);

        ImGui::PopFont();
    }

    void handleCommands()
    {
        osgVerse::CommandData cmd;
        if (osgVerse::CommandBuffer::instance()->take(cmd, false))
        {
            switch (cmd.type)
            {
            case osgVerse::RefreshHierarchy:
                if (!_hierarchy->handleCommand(&cmd))
                    OSG_WARN << "[EditorContentHandler] Failed to refresh hierarchy" << std::endl;
                break;
            }
        }
    }

protected:
    osg::ref_ptr<Hierarchy> _hierarchy;
    osg::ref_ptr<Properties> _properties;
    osg::ref_ptr<SceneLogic> _sceneLogic;
};

int main(int argc, char** argv)
{
    osgVerse::globalInitialize(argc, argv);
    osgViewer::Viewer viewer;

    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    {
        osg::StateSet* ss = sceneRoot->getOrCreateStateSet();
        ss->setTextureAttributeAndModes(0, osgVerse::createDefaultTexture(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f)));
    }

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(sceneRoot.get());

    osg::ref_ptr<osgVerse::ImGuiManager> imgui = new osgVerse::ImGuiManager;
    imgui->setChineseSimplifiedFont("../misc/LXGWWenKaiLite-Regular.ttf");
    imgui->initialize(new EditorContentHandler(viewer.getCamera(), sceneRoot.get()));
    imgui->addToView(&viewer);

    viewer.addEventHandler(new osgVerse::CommandHandler());
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}