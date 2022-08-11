#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include "hierarchy.h"

#include <imgui/ImGuizmo.h>
#include <ui/CommandHandler.h>

class HierarchyVisitor : public osg::NodeVisitor
{
public:
    HierarchyVisitor() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}
    std::map<osg::Node*, osgVerse::TreeView::TreeData> _treeItemMap;
    osgVerse::TreeView::TreeData* _rootItem;

    virtual void apply(osg::Node& node)
    {
        if (_rootItem->userData != &node)
        {
            osgVerse::TreeView::TreeData treeItem;
            static int kkk = 1; treeItem.id = "node_" + std::to_string(kkk++);  // TODO
            treeItem.name = node.className() + std::string("_") + node.getName();  // TODO
            treeItem.userData = &node;
            _treeItemMap[&node] = treeItem;
        }
        traverse(node);
    }

    void rearrange()
    {
        for (std::map<osg::Node*, osgVerse::TreeView::TreeData>::iterator itr = _treeItemMap.begin();
             itr != _treeItemMap.end(); ++itr)
        {
            osg::Node* parent = itr->first->getParent(0);
            if (_rootItem->userData != parent)
                _treeItemMap[parent].children.push_back(itr->second);
            else
                _rootItem->children.push_back(itr->second);
        }
    }
};

Hierarchy::Hierarchy(osg::Camera* cam, osg::MatrixTransform* mt)
    : _camera(cam), _sceneRoot(mt)
{
    _treeWindow = new osgVerse::Window(TR("Hierarchy##ed01"));
    _treeWindow->pos = osg::Vec2(0, 0);
    _treeWindow->sizeMin = osg::Vec2(300, 780);
    _treeWindow->alpha = 0.8f;
    _treeWindow->useMenuBar = true;
    _treeWindow->flags = ImGuiWindowFlags_NoCollapse;
    _treeWindow->userData = this;

    _treeMenuBar = new osgVerse::MenuBar;
    _treeMenuBar->userData = this;
    {
        osgVerse::MenuBar::MenuData projMenu(TR("Project##menu01"));
        {
            osgVerse::MenuBar::MenuItemData newItem(TR("New##menu0101"));
            projMenu.items.push_back(newItem);

            osgVerse::MenuBar::MenuItemData openItem(TR("Open##menu0102"));
            projMenu.items.push_back(openItem);

            osgVerse::MenuBar::MenuItemData saveItem(TR("Save##menu0103"));
            projMenu.items.push_back(saveItem);

            osgVerse::MenuBar::MenuItemData settingItem(TR("Settings##menu0104"));
            projMenu.items.push_back(settingItem);
        }
        _treeMenuBar->menuDataList.push_back(projMenu);

        osgVerse::MenuBar::MenuData assetMenu(TR("Assets##menu02"));
        {
            osgVerse::MenuBar::MenuItemData importItem(TR("Import Model##menu0201"));
            importItem.callback = [](osgVerse::ImGuiManager*, osgVerse::ImGuiContentHandler*,
                                     ImGuiComponentBase* me)
            {
                Hierarchy* h = static_cast<Hierarchy*>(me->userData.get());
                osgVerse::CommandBuffer::instance()->add(osgVerse::LoadModelCommand,
                    h->_sceneRoot.get(), std::string("spaceship.osgt"));  // TODO: test only
            };
            assetMenu.items.push_back(importItem);
        }
        _treeMenuBar->menuDataList.push_back(assetMenu);
    }

    _treeView = new osgVerse::TreeView;
    _treeView->userData = this;
    {
        _camTreeData.name = TR("Main Camera"); _camTreeData.id = "main_camera";
        _camTreeData.userData = _camera.get();
        _treeView->treeDataList.push_back(_camTreeData);

        _sceneTreeData.name = TR("Scene Root"); _sceneTreeData.id = "scene_root";
        _sceneTreeData.userData = _sceneRoot.get();
        _treeView->treeDataList.push_back(_sceneTreeData);
    }
}

bool Hierarchy::handleCommand(osgVerse::CommandData* cmd)
{
    osg::Node* node = NULL;
    osg::Group* parent = static_cast<osg::Group*>(cmd->object.get());
    if (!cmd->get(node) || !parent) return false;

    // Find parent, and see if node is new or existed, and update
    HierarchyVisitor hv;
    hv._rootItem = &_treeView->treeDataList[1];  // Scene-root
    // TODO: find parent & node in tree-view
    parent->accept(hv); hv.rearrange();
    return true;
}

bool Hierarchy::show(osgVerse::ImGuiManager* mgr, osgVerse::ImGuiContentHandler* content)
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
