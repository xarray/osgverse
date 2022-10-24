#ifndef MANA_DEFINES_HPP
#define MANA_DEFINES_HPP

#include <osg/io_utils>
#include <osg/MatrixTransform>
#include <osg/NodeVisitor>
#include <osgDB/ConvertUTF>

#include <imgui/imgui.h>
#include <imgui/ImGuizmo.h>
#include <ui/ImGui.h>
#include <ui/ImGuiComponents.h>
#include <ui/CommandHandler.h>
#include <pipeline/SkyBox.h>
#include <pipeline/NodeSelector.h>
#include <pipeline/Pipeline.h>
#include <pipeline/LightModule.h>
#include <pipeline/ShadowModule.h>
#include <pipeline/IntersectionManager.h>
#include <pipeline/Utilities.h>
#include <iostream>
#include <sstream>

#define A2U(s) osgDB::convertStringFromCurrentCodePageToUTF8(s)
#define U2A(s) osgDB::convertStringFromUTF8toCurrentCodePage(s)
#define W2U(s) osgDB::convertUTF16toUTF8(s)
#define U2W(s) osgDB::convertUTF8toUTF16(s)
class Hierarchy;
class Properties;
class SceneLogic;

class EditorContentHandler : public osgVerse::ImGuiContentHandler
{
public:
    EditorContentHandler();
    Hierarchy* getHierarchy() { return _hierarchy.get(); }
    Properties* getProperties() { return _properties.get(); }
    SceneLogic* getSceneLogic() { return _sceneLogic.get(); }
    osgVerse::MainMenuBar* getMainMenu() { return _mainMenu.get(); }

    void handleCommands();
    virtual void runInternal(osgVerse::ImGuiManager* mgr);

protected:
    void createEditorMenu1();
    void createEditorMenu2();
    void createEditorMenu3();

    osg::ref_ptr<osgVerse::MainMenuBar> _mainMenu;
    osg::ref_ptr<Hierarchy> _hierarchy;
    osg::ref_ptr<Properties> _properties;
    osg::ref_ptr<SceneLogic> _sceneLogic;
    unsigned int _uiFrameNumber;
};

struct GlobalData
{
    osg::observer_ptr<osg::Camera> mainCamera;
    osg::observer_ptr<osg::Group> sceneRoot, auxiliaryRoot;
    osg::observer_ptr<osgVerse::NodeSelector> selector;
    osg::observer_ptr<osgVerse::Pipeline> pipeline;
    osg::observer_ptr<osgVerse::ShadowModule> shadow;
    osg::observer_ptr<osgViewer::View> view;
};
extern GlobalData g_data;

#endif
