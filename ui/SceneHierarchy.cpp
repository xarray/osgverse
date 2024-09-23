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
    _treeWindow = new Window(TR("Hierarchy") + _postfix);
    _treeWindow->pos = osg::Vec2(0.0f, 0.0f);
    _treeWindow->size = osg::Vec2(0.15f, 0.75f);
    _treeWindow->alpha = 0.9f;
    _treeWindow->useMenuBar = false;
    _treeWindow->flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;
    _treeWindow->userData = this;

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
    // 1. LMB selects item = show highlighter in scene + show data in properties
    // 2. RMB selects item/empty = show popup window
    // 3. LMB double clicks item = focus on this item
    _treeView->callback = [&](ImGuiManager*, ImGuiContentHandler*,
                              ImGuiComponentBase* me, const std::string& id)
    {
        TreeView* treeView = static_cast<TreeView*>(me);
        _selectedItem = treeView->findByID(id); _selectedItemPopupTriggered = false;
        if (!_selectedItem) return;

        if (!ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            // TODO: select in scene and show properties
        }
        else
        {
            // TODO: show it in center
        }
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

bool SceneHierarchy::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    bool done = _treeWindow->show(mgr, content);
    if (done)
    {
        _treeView->show(mgr, content);
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
        _treeWindow->showEnd();
    }
    return done;
}

void SceneHierarchy::showPopupMenu(osgVerse::TreeView::TreeData* item, osgVerse::ImGuiManager* mgr,
                                   osgVerse::ImGuiContentHandler* content)
{
    // TODO
}
