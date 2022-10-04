#include <osgDB/ReadFile>
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

class SceneManipulator : public osgGA::TrackballManipulator
{
public:
    SceneManipulator() : osgGA::TrackballManipulator() {}
    virtual bool performMovement()
    {
        if (_ga_t0.get() == NULL || _ga_t1.get() == NULL) return false;
        double eventTimeDelta = _ga_t0->getTime() - _ga_t1->getTime();
        if (eventTimeDelta < 0.0) eventTimeDelta = 0.0;

        float dx = _ga_t0->getXnormalized() - _ga_t1->getXnormalized();
        float dy = _ga_t0->getYnormalized() - _ga_t1->getYnormalized();
        if (dx == 0.0 && dy == 0.0) return false;

        unsigned int bm = _ga_t1->getButtonMask(), mk = _ga_t1->getModKeyMask();
        bool modKeyDown = (mk & osgGA::GUIEventAdapter::MODKEY_ALT);
        if (bm == osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON && modKeyDown)
        { return performMovementRightMouseButton(eventTimeDelta, dx, dy); }
        else if ((bm == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON && modKeyDown) ||
                 bm == osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON)
        { return performMovementLeftMouseButton(eventTimeDelta, dx, dy); }
        else if (bm == osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON)
        { return performMovementMiddleMouseButton(eventTimeDelta, dx, dy); }
        return false;
    }
};

EditorContentHandler::EditorContentHandler()
    : _uiFrameNumber(0)
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
    if (_uiFrameNumber > 0)
    {
        // Wait for the first frame to initialize ImGui work-size
        if (_hierarchy.valid()) _hierarchy->show(mgr, this);
        if (_properties.valid()) _properties->show(mgr, this);
        if (_sceneLogic.valid()) _sceneLogic->show(mgr, this);
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        // TODO: auto layout
        osg::Vec4 hSize = _hierarchy->getWindow()->getCurrentRectangle();
        osg::Vec4 pSize = _properties->getWindow()->getCurrentRectangle();
        osg::Vec4 lSize = _sceneLogic->getWindow()->getCurrentRectangle();
    }

    // Dialog management
    { std::string r; osgVerse::ImGuiComponentBase::showFileDialog(r); }
    { bool r = false; osgVerse::ImGuiComponentBase::showConfirmDialog(r); }

    ImGui::PopFont();
    _uiFrameNumber++;
}

void EditorContentHandler::handleCommands()
{
    osgVerse::CommandData cmd;
    if (osgVerse::CommandBuffer::instance()->take(cmd, false))
    {
        switch (cmd.type)
        {
        case osgVerse::ResizeEditor:
            if (_hierarchy.valid()) _hierarchy->getWindow()->sizeApplied = false;
            if (_properties.valid()) _properties->getWindow()->sizeApplied = false;
            if (_sceneLogic.valid()) _sceneLogic->getWindow()->sizeApplied = false;
            break;
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
        //skybox->setEnvironmentMap("../skyboxes/default/", "jpg");
        skybox->setEnvironmentMap(osgDB::readImageFile("../skyboxes/barcelona/barcelona.hdr"));
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
    g_data.view = &viewer;

    osg::ref_ptr<osgVerse::ImGuiManager> imgui = new osgVerse::ImGuiManager;
    imgui->setChineseSimplifiedFont("../misc/SourceHanSansHWSC-Regular.otf");
    imgui->initialize(new EditorContentHandler);
    imgui->addToView(&viewer);

    viewer.addEventHandler(new osgVerse::CommandHandler);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    //viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setCameraManipulator(new SceneManipulator);
    viewer.setSceneData(root.get());
    //viewer.setKeyEventSetsDone(0);
    return viewer.run();
}