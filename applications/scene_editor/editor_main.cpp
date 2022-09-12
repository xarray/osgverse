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
        _mainMenu = new osgVerse::MainMenuBar;
        _mainMenu->userData = this;
        {
            osgVerse::MenuBar::MenuData projMenu(osgVerse::MenuBar::TR("Project##menu01"));
            {
                osgVerse::MenuBar::MenuItemData newItem(osgVerse::MenuBar::TR("New##menu0101"));
                projMenu.items.push_back(newItem);

                osgVerse::MenuBar::MenuItemData openItem(osgVerse::MenuBar::TR("Open##menu0102"));
                projMenu.items.push_back(openItem);

                osgVerse::MenuBar::MenuItemData saveItem(osgVerse::MenuBar::TR("Save##menu0103"));
                projMenu.items.push_back(saveItem);

                osgVerse::MenuBar::MenuItemData settingItem(osgVerse::MenuBar::TR("Settings##menu0104"));
                projMenu.items.push_back(settingItem);
            }
            _mainMenu->menuDataList.push_back(projMenu);

            osgVerse::MenuBar::MenuData assetMenu(osgVerse::MenuBar::TR("Assets##menu02"));
            {
                osgVerse::MenuBar::MenuItemData importItem(osgVerse::MenuBar::TR("Import Model##menu0201"));
                importItem.callback = [&](osgVerse::ImGuiManager*, osgVerse::ImGuiContentHandler*,
                                          osgVerse::ImGuiComponentBase* me)
                {
                    // TODO: file dialog
                    _hierarchy->addModelFromUrl("UH-60A/UH-60A.osgb");
                };
                assetMenu.items.push_back(importItem);
            }
            _mainMenu->menuDataList.push_back(assetMenu);

            osgVerse::MenuBar::MenuData compMenu(osgVerse::MenuBar::TR("Components##menu03"));
            {
                osgVerse::MenuBar::MenuItemData newItem(osgVerse::MenuBar::TR("New##menu0301"));
                compMenu.items.push_back(newItem);
            }
            _mainMenu->menuDataList.push_back(compMenu);

            osgVerse::MenuBar::MenuData editMenu(osgVerse::MenuBar::TR("Utility##menu04"));
            {
            }
            _mainMenu->menuDataList.push_back(editMenu);
        }

        _hierarchy = new Hierarchy(camera, mt);
        _properties = new Properties(camera, mt);
        _sceneLogic = new SceneLogic(camera, mt);
    }

    virtual void runInternal(osgVerse::ImGuiManager* mgr)
    {
        ImGui::PushFont(ImGuiFonts["SourceHanSansHWSC-Regular"]);
        handleCommands();

        _mainMenu->show(mgr, this);
        ImGui::Separator();

        // TODO: auto layout
        if (_hierarchy.valid()) _hierarchy->show(mgr, this);
        if (_properties.valid()) _properties->show(mgr, this);
        if (_sceneLogic.valid()) _sceneLogic->show(mgr, this);

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

protected:
    osg::ref_ptr<osgVerse::MainMenuBar> _mainMenu;
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
    imgui->setChineseSimplifiedFont("../misc/SourceHanSansHWSC-Regular.otf");
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