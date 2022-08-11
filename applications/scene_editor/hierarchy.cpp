#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include "hierarchy.h"
#include <iostream>
#include <stack>

#include <nanoid/nanoid.h>
#include <imgui/ImGuizmo.h>
#include <ui/CommandHandler.h>
using namespace osgVerse;

class HierarchyVisitor : public osg::NodeVisitor
{
public:
    HierarchyVisitor() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}
    std::stack<osg::ref_ptr<TreeView::TreeData>> _itemStack;

    virtual void apply(osg::Node& node)
    {
        TreeView::TreeData* treeItem = new TreeView::TreeData;
        treeItem->name = node.getName().empty() ? node.className() : node.getName();
        treeItem->id = "##node_" + nanoid::generate(8);
        treeItem->name += treeItem->id;

        treeItem->tooltip = node.libraryName() + std::string("::") + node.className();
        treeItem->userData = &node;
        
        _itemStack.top()->children.push_back(treeItem);
        _itemStack.push(treeItem);
        traverse(node);
        _itemStack.pop();
    }
};

Hierarchy::Hierarchy(osg::Camera* cam, osg::MatrixTransform* mt)
    : _camera(cam), _sceneRoot(mt)
{
    _treeWindow = new Window(TR("Hierarchy##ed01"));
    _treeWindow->pos = osg::Vec2(0, 0);
    _treeWindow->sizeMin = osg::Vec2(200, 780);
    _treeWindow->sizeMax = osg::Vec2(600, 780);
    _treeWindow->alpha = 0.8f;
    _treeWindow->useMenuBar = true;
    _treeWindow->flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;
    _treeWindow->userData = this;

    _treeMenuBar = new MenuBar;
    _treeMenuBar->userData = this;
    {
        MenuBar::MenuData projMenu(TR("Project##menu01"));
        {
            MenuBar::MenuItemData newItem(TR("New##menu0101"));
            projMenu.items.push_back(newItem);

            MenuBar::MenuItemData openItem(TR("Open##menu0102"));
            projMenu.items.push_back(openItem);

            MenuBar::MenuItemData saveItem(TR("Save##menu0103"));
            projMenu.items.push_back(saveItem);

            MenuBar::MenuItemData settingItem(TR("Settings##menu0104"));
            projMenu.items.push_back(settingItem);
        }
        _treeMenuBar->menuDataList.push_back(projMenu);

        MenuBar::MenuData assetMenu(TR("Assets##menu02"));
        {
            MenuBar::MenuItemData importItem(TR("Import Model##menu0201"));
            importItem.callback = [](ImGuiManager*, ImGuiContentHandler*,
                                     ImGuiComponentBase* me)
            {
                Hierarchy* h = static_cast<Hierarchy*>(me->userData.get());
                CommandBuffer::instance()->add(LoadModelCommand,
                    h->_sceneRoot.get(), std::string("spaceship.osgt"));  // TODO: test only
            };
            assetMenu.items.push_back(importItem);
        }
        _treeMenuBar->menuDataList.push_back(assetMenu);
    }

    _treeView = new TreeView;
    _treeView->userData = this;
    {
        _camTreeData = new TreeView::TreeData;
        _camTreeData->name = TR("Main Camera"); _camTreeData->id = "main_camera";
        _camTreeData->userData = _camera.get();
        _treeView->treeDataList.push_back(_camTreeData);

        _sceneTreeData = new TreeView::TreeData;
        _sceneTreeData->name = TR("Scene Root"); _sceneTreeData->id = "scene_root";
        _sceneTreeData->userData = _sceneRoot.get();
        _treeView->treeDataList.push_back(_sceneTreeData);
    }
}

bool Hierarchy::handleCommand(CommandData* cmd)
{
    // Refresh hierarchy:
    // - cmd->object (parent) must be found in hierarchy
    // - If cmd->value (node) not found, it is a new subgraph
    // - If cmd->value (node) found, it is updated
    // TODO: how to decide DELETE
    osg::Node* node = NULL;
    osg::Group* parent = static_cast<osg::Group*>(cmd->object.get());
    if (!cmd->get(node) || !parent) return false;

    // See if node is newly created or existed, and update
    std::vector<TreeView::TreeData*> nItems = _treeView->findByUserData(node);
    std::vector<TreeView::TreeData*> pItems = _treeView->findByUserData(parent);
    if (pItems.empty()) return false;

    HierarchyVisitor hv;
    hv._itemStack.push(nItems.empty() ? pItems[0] : nItems[0]);
    parent->accept(hv); return true;
}

bool Hierarchy::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    bool done = _treeWindow->show(mgr, content);
    {
        _treeMenuBar->show(mgr, content);
        ImGui::Separator();
        _treeView->show(mgr, content);
    }
    _treeWindow->showEnd();
    return done;
}
