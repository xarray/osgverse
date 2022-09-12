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

Hierarchy::Hierarchy(osg::Camera* cam, osg::MatrixTransform* mt)
    : _camera(cam), _sceneRoot(mt)
{
    _treeWindow = new Window(TR("Hierarchy##ed01"));
    _treeWindow->pos = osg::Vec2(0, 0);
    _treeWindow->sizeMin = osg::Vec2(200, 780);
    _treeWindow->sizeMax = osg::Vec2(600, 780);
    _treeWindow->alpha = 0.9f;
    _treeWindow->useMenuBar = false;
    _treeWindow->flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;
    _treeWindow->userData = this;

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

    // Selected events:
    // 1. LMB selects item = show highlighter in scene + show data in properties
    // 2. RMB selects item/empty = show popup window
    // 3. LMB double clicks item = expanded or not
    _treeView->callback = [](ImGuiManager*, ImGuiContentHandler*,
                             ImGuiComponentBase* me, const std::string& id)
    {
        TreeView* treeView = static_cast<TreeView*>(me);
        TreeView::TreeData* item = treeView->findByID(id);
        CommandBuffer::instance()->add(RefreshProperties,
            static_cast<osg::Object*>(item->userData.get()), "");
    };

    _treeView->callbackR = [](ImGuiManager*, ImGuiContentHandler*,
                              ImGuiComponentBase* me, const std::string& id)
    {
        std::cout << "RMB: " << id << "\n";
        // TODO
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
    parent->accept(hv); return true;
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
    {
        _treeView->show(mgr, content);
    }
    _treeWindow->showEnd();
    return done;
}

void Hierarchy::addModelFromUrl(const std::string& url)
{
    // TODO: find parent node
    osgVerse::CommandBuffer::instance()->add(
        osgVerse::LoadModelCommand, _sceneRoot.get(), url);
}
