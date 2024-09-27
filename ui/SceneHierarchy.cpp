#include <osg/io_utils>
#include <osg/Version>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/FileNameUtils>
#include <nanoid/nanoid.h>
#include "SceneHierarchy.h"
#include <iostream>
#include <stack>
using namespace osgVerse;

SceneHierarchy::SceneHierarchy()
    : _selectedItemPopupTriggered(false)
{
    _postfix = "##" + nanoid::generate(8);
    _treeView = new TreeView;
    _treeView->userData = this;
    {
        _camTreeData = new TreeView::TreeData;
        _camTreeData->name = TR("Viewer"); _camTreeData->id = "##viewer" + _postfix;
        _treeView->treeDataList.push_back(_camTreeData);

        _sceneTreeData = new TreeView::TreeData;
        _sceneTreeData->name = TR("Scene Root"); _sceneTreeData->id = "##scene_root" + _postfix;
        _treeView->treeDataList.push_back(_sceneTreeData);
    }

    // Selected events:
    // 1. LMB selects/double-clicks item: callbacks
    // 2. RMB selects item/empty = show popup window
    _treeView->callback = [&](ImGuiManager*, ImGuiContentHandler*,
                              ImGuiComponentBase* me, const std::string& id)
    {
        TreeView* treeView = static_cast<TreeView*>(me);
        _selectedItem = treeView->findByID(id); _selectedItemPopupTriggered = false;
        if (!_selectedItem) return;

        if (!ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            { if (_clickAction) _clickAction(treeView, _selectedItem.get()); }
        else if (_dbClickAction)
            _dbClickAction(treeView, _selectedItem.get());
    };

    _treeView->callbackR = [&](ImGuiManager*, ImGuiContentHandler*,
                               ImGuiComponentBase* me, const std::string& id)
    {
        TreeView* treeView = static_cast<TreeView*>(me);
        _selectedItem = treeView->findByID(id);
        if (_selectedItem.valid()) _selectedItemPopupTriggered = true;
    };
}

void SceneHierarchy::setViewer(osgViewer::View* view)
{
    _camTreeData->userData = view;
    _sceneTreeData->userData = view->getSceneData();
}

TreeView::TreeData* SceneHierarchy::addItem(TreeView::TreeData* parent, osg::Object* obj, bool asSubGraph)
{
    TreeView::TreeData* treeItem = new TreeView::TreeData;
    treeItem->name = obj->getName().empty() ? obj->className() : obj->getName();
    treeItem->id = "##node_" + nanoid::generate(8);
    treeItem->name += treeItem->id;
    treeItem->tooltip = obj->libraryName() + std::string("::") + obj->className();
    treeItem->userData = obj;

    if (!parent) parent = _sceneTreeData.get();
    parent->children.push_back(treeItem);

    // TODO: asSubGraph
    return treeItem;
}

bool SceneHierarchy::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    bool done = _treeView->show(mgr, content);
    if (_selectedItem.valid())
    {
        if (_selectedItemPopupTriggered)
        {
            ImGui::OpenPopup((_selectedItem->id + "Popup").c_str());
            _selectedItemPopupTriggered = false;
        }

        if (ImGui::BeginPopup((_selectedItem->id + "Popup").c_str()))
        {
            showPopupMenu(_selectedItem.get(), mgr, content);
            ImGui::EndPopup();
        }
    }
    return done;
}

void SceneHierarchy::showPopupMenu(osgVerse::TreeView::TreeData* item, osgVerse::ImGuiManager* mgr,
                                   osgVerse::ImGuiContentHandler* content)
{
    // TODO
}
