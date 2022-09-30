#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include "hierarchy.h"
#include "properties.h"
#include "scenelogic.h"
#include "defines.h"
GlobalData g_data;

EditorContentHandler::EditorContentHandler()
{
    _hierarchy = new Hierarchy;
    _properties = new Properties;
    _sceneLogic = new SceneLogic;
    _mainMenu = new osgVerse::MainMenuBar;
    _mainMenu->userData = this;

    createEditorMenu1();
    createEditorMenu2();
    createEditorMenu3();
}

void EditorContentHandler::runInternal(osgVerse::ImGuiManager* mgr)
{
    ImGui::PushFont(ImGuiFonts["SourceHanSansHWSC-Regular"]);
    handleCommands();

    _mainMenu->show(mgr, this);
    ImGui::Separator();

    // TODO: auto layout
    if (_hierarchy.valid()) _hierarchy->show(mgr, this);
    if (_properties.valid()) _properties->show(mgr, this);
    if (_sceneLogic.valid()) _sceneLogic->show(mgr, this);

    if (!_currentDialogName.empty())
    {
        std::string result;
        if (osgVerse::ImGuiComponentBase::showFileDialog(_currentDialogName, result))
        {
            _hierarchy->addModelFromUrl(result);  // FIXME: open other files?
            _currentDialogName = "";
        }
    }
    ImGui::PopFont();
}

void EditorContentHandler::handleCommands()
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
        case osgVerse::RefreshHierarchyItem:
            if (!_hierarchy->handleItemCommand(&cmd))
                OSG_WARN << "[EditorContentHandler] Failed to refresh hierarchy item" << std::endl;
            break;
        case osgVerse::RefreshProperties:
            if (!_properties->handleCommand(&cmd))
                OSG_WARN << "[EditorContentHandler] Failed to refresh properties" << std::endl;
            break;
        }
    }
}

int main(int argc, char** argv)
{
    osgVerse::globalInitialize(argc, argv);
    osgViewer::Viewer viewer;

    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    {
        osg::StateSet* ss = sceneRoot->getOrCreateStateSet();
        ss->setTextureAttributeAndModes(0, osgVerse::createDefaultTexture(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f)));
    }

    osg::ref_ptr<osg::Group> auxRoot = new osg::Group;
    {
        osg::ref_ptr<osgVerse::SkyBox> skybox = new osgVerse::SkyBox;
        skybox->setEnvironmentMap("../skyboxes/default/", "jpg");
        auxRoot->addChild(skybox.get());
    }

    osg::ref_ptr<osgVerse::NodeSelector> selector = new osgVerse::NodeSelector;
    {
        selector->setMainCamera(viewer.getCamera());
        auxRoot->addChild(selector->getAuxiliaryRoot());
        // TODO: add to hud root?
    }

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(sceneRoot.get());
    root->addChild(auxRoot.get());

    g_data.mainCamera = viewer.getCamera();
    g_data.sceneRoot = sceneRoot.get();
    g_data.auxiliaryRoot = auxRoot.get();
    g_data.selector = selector.get();

    osg::ref_ptr<osgVerse::ImGuiManager> imgui = new osgVerse::ImGuiManager;
    imgui->setChineseSimplifiedFont("../misc/SourceHanSansHWSC-Regular.otf");
    imgui->initialize(new EditorContentHandler);
    imgui->addToView(&viewer);

    viewer.addEventHandler(new osgVerse::CommandHandler);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}