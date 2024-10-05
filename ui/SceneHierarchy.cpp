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

class HierarchyCreatingVisitor : public osg::NodeVisitor
{
public:
    HierarchyCreatingVisitor(SceneHierarchy* h, TreeView::TreeData* parent, bool m)
        : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
    {
        _itemStack.push(parent); _hierarchy = h; _removingMode = m;
        setNodeMaskOverride(0xffffffff);
    }

    virtual void apply(osg::Drawable& node)
    {
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
        push(&node); traverse(node); pop(&node);
#endif
    }

    virtual void apply(osg::Geometry& node)
    {
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
        push(&node); traverse(node); pop(&node);
#endif
    }

    virtual void apply(osg::Node& node)
    { push(&node); traverse(node); pop(&node); }

    virtual void apply(osg::Geode& node)
    {
#if OSG_VERSION_LESS_OR_EQUAL(3, 4, 1)
        for (unsigned int i = 0; i < node.getNumDrawables(); ++i)
            apply(*node.getDrawable(i));
#endif
        push(&node); traverse(node); pop(&node);
    }

    void push(osg::Object* obj)
    {
        if (_removingMode)
        {
            SceneHierarchy::NodeToItemMapper& mapper = _hierarchy->getNodeToItemMap();
            SceneHierarchy::NodeToItemMapper::iterator it = mapper.find(obj);
            if (it != mapper.end()) _itemStack.push(it->second.get());
        }
        else
        {
            TreeView::TreeData* item = _hierarchy->addItem(_itemStack.top(), obj, false);
            if (item != NULL) _itemStack.push(item);
        }
    }

    void pop(osg::Object* obj)
    {
        if (_removingMode)
        {
            TreeView::TreeData* parent = _itemStack.top();
            _hierarchy->removeItem(parent, obj, false);
        }
        _itemStack.pop();
    }

protected:
    std::stack<TreeView::TreeData*> _itemStack;
    osg::observer_ptr<SceneHierarchy> _hierarchy;
    bool _removingMode;
};

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
        _sceneTreeData->name = TR("Scene"); _sceneTreeData->id = "##scene_root" + _postfix;
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
    if (!obj) return NULL; else if (!parent) parent = _sceneTreeData.get();
    if (asSubGraph)
    {
        osg::Node* node = dynamic_cast<osg::Node*>(obj);
        if (node != NULL) { HierarchyCreatingVisitor hv(this, parent, false); node->accept(hv); }
    }

    if (_nodeToItemMap.find(obj) == _nodeToItemMap.end())
    {
        TreeView::TreeData* treeItem = new TreeView::TreeData;
        treeItem->name = (obj->getName().empty() ? obj->className() : obj->getName());
        treeItem->id = "##node_" + nanoid::generate(8);
        treeItem->name += treeItem->id;
        treeItem->tooltip = obj->libraryName() + std::string("::") + obj->className();
        treeItem->userData = obj; _nodeToItemMap[obj] = treeItem;
        parent->children.push_back(treeItem);
    }
    return _nodeToItemMap[obj].get();
}

bool SceneHierarchy::removeItem(TreeView::TreeData* parent, osg::Object* obj, bool asSubGraph)
{
    NodeToItemMapper::iterator itr = _nodeToItemMap.find(obj);
    if (!parent) parent = _sceneTreeData.get();
    if (itr != _nodeToItemMap.end())
    {
        TreeView::TreeData* current = itr->second.get();
        std::vector<osg::ref_ptr<TreeView::TreeData>>::iterator childIt =
            std::find(parent->children.begin(), parent->children.end(), current);
        if (childIt != parent->children.end())
        {
            if (asSubGraph)
            {
                osg::Node* node = dynamic_cast<osg::Node*>(obj);
                if (node != NULL) { HierarchyCreatingVisitor hv(this, parent, true); node->accept(hv); }
            }
            else
                { parent->children.erase(childIt); _nodeToItemMap.erase(obj); }
            return true;
        }
        else
            OSG_WARN << "[SceneHierarchy] Object to remove not belong to " << parent->name << std::endl;
    }
    return false;
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
