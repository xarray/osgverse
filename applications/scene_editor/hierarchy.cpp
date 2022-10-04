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

    // TODO: display shared nodes (like prefab in Unity)
    //       display locked/hidden
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

Hierarchy::Hierarchy()
    : _selectedItemPopupTriggered(false)
{
    _treeWindow = new Window(TR("Hierarchy##ed01"));
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
        _camTreeData->name = TR("Main Camera"); _camTreeData->id = "main_camera##ed02";
        _camTreeData->userData = g_data.mainCamera.get();
        _treeView->treeDataList.push_back(_camTreeData);

        _sceneTreeData = new TreeView::TreeData;
        _sceneTreeData->name = TR("Scene Root"); _sceneTreeData->id = "scene_root##ed03";
        _sceneTreeData->userData = g_data.sceneRoot.get();
        _treeView->treeDataList.push_back(_sceneTreeData);
    }

    // Selected events:
    // 1. LMB selects item = show highlighter in scene + show data in properties
    // 2. RMB selects item/empty = show popup window
    // 3. LMB double clicks item = expanded or not
    _treeView->callback = [&](ImGuiManager*, ImGuiContentHandler*,
                              ImGuiComponentBase* me, const std::string& id)
    {
        TreeView* treeView = static_cast<TreeView*>(me);
        _selectedItem = treeView->findByID(id); _selectedItemPopupTriggered = false;
        if (!_selectedItem) return;

        CommandBuffer::instance()->add(SelectCommand,
            static_cast<osg::Object*>(_selectedItem->userData.get()), g_data.selector.get(), 0);
        CommandBuffer::instance()->add(RefreshProperties,
            static_cast<osg::Object*>(_selectedItem->userData.get()), "");
    };

    _treeView->callbackR = [&](ImGuiManager*, ImGuiContentHandler*,
                               ImGuiComponentBase* me, const std::string& id)
    {
        TreeView* treeView = static_cast<TreeView*>(me);
        _selectedItem = treeView->findByID(id);
        if (_selectedItem.valid()) _selectedItemPopupTriggered = true;
    };
}

bool Hierarchy::handleCommand(CommandData* cmd)
{
    /* Refresh hierarchy:
       - cmd->object (parent) must be found in hierarchy
       - If cmd->value (node) not found, it is a new subgraph
       - If cmd->value (node) found, it is shared from somewhere else (TODO)
       - cmd->valueEx (bool): 'node' should be deleted from 'parent' (TODO)
    */
    osg::Node* node = NULL;
    osg::Group* parent = static_cast<osg::Group*>(cmd->object.get());
    if (!cmd->get(node) || !parent) return false;

    // See if node is newly created or existed, and update
    std::vector<TreeView::TreeData*> nItems = _treeView->findByUserData(node);
    std::vector<TreeView::TreeData*> pItems = _treeView->findByUserData(parent);
    if (pItems.empty()) return false;

    HierarchyVisitor hv;
    hv._itemStack.push(pItems[0]);
    node->accept(hv); return true;
}

bool Hierarchy::handleItemCommand(osgVerse::CommandData* cmd)
{
    /* Refresh hierarchy item:
       - cmd->object (item) must be found in hierarchy
    */
    std::string name = cmd->object->getName();
    osg::Node* node = cmd->object->asNode();

    std::vector<TreeView::TreeData*> nItems = _treeView->findByUserData(cmd->object.get());
    for (size_t i = 0; i < nItems.size(); ++i)
    {
        TreeView::TreeData* td = nItems[i];
        td->name = (name.empty() ? cmd->object->className() : name) + td->id;
        if (node)
        {
            unsigned int mask = node->getNodeMask();
            // TODO: show visible state on item
        }
    }
    return true;
}

bool Hierarchy::show(ImGuiManager* mgr, ImGuiContentHandler* content)
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

void Hierarchy::showPopupMenu(osgVerse::TreeView::TreeData* item, osgVerse::ImGuiManager* mgr,
                              osgVerse::ImGuiContentHandler* content)
{
    // Popup menu: active, new {} | cut, copy, paste, delete | share, unshare
    // TODO
}

void Hierarchy::addModelFromUrl(const std::string& url)
{
    if (_selectedItem.valid())
    {
        osg::Group* parent = static_cast<osg::Group*>(_selectedItem->userData.get());
        if (parent != NULL)
        {
            osgVerse::CommandBuffer::instance()->add(osgVerse::LoadModelCommand, parent, url);
            return;  // Finish loading to selected item; otherwise load this model to scene root
        }
    }
    osgVerse::CommandBuffer::instance()->add(
        osgVerse::LoadModelCommand, g_data.sceneRoot.get(), url);
}
