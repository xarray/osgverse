#include <osg/io_utils>
#include <osg/Version>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
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
        _maskStack.push(0xffffffff); _itemStack.push(parent);
        _hierarchy = h; _removingMode = m;
        setNodeMaskOverride(0xffffffff);
    }

    virtual void apply(osg::Drawable& node)
    {
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
        _maskStack.push(_maskStack.top() & node.getNodeMask());
        push(&node); traverse(node); pop(&node);
        _maskStack.pop();
#endif
    }

    virtual void apply(osg::Geometry& node)
    {
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
        _maskStack.push(_maskStack.top() & node.getNodeMask());
        push(&node); traverse(node); pop(&node);
        _maskStack.pop();
#endif
    }

    virtual void apply(osg::Node& node)
    {
        _maskStack.push(_maskStack.top() & node.getNodeMask());
        push(&node); traverse(node); pop(&node);
        _maskStack.pop();
    }

    virtual void apply(osg::Geode& node)
    {
        _maskStack.push(_maskStack.top() & node.getNodeMask());
#if OSG_VERSION_LESS_OR_EQUAL(3, 4, 1)
        for (unsigned int i = 0; i < node.getNumDrawables(); ++i)
            apply(*node.getDrawable(i));
#endif
        push(&node); traverse(node); pop(&node);
        _maskStack.pop();
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
            TreeView::TreeData* item =
                _hierarchy->addItem(_itemStack.top(), obj, false, _maskStack.top());
            if (item != NULL) _itemStack.push(item);
        }
    }

    void pop(osg::Object* obj)
    {
        _itemStack.pop();
        if (_removingMode && !_itemStack.empty())
        {
            TreeView::TreeData* parent = _itemStack.top();
            _hierarchy->removeItem(parent, obj, false);
        }
    }

protected:
    std::stack<unsigned int> _maskStack;
    std::stack<TreeView::TreeData*> _itemStack;
    osg::observer_ptr<SceneHierarchy> _hierarchy;
    bool _removingMode;
};

SceneHierarchy::SceneHierarchy()
    : _stateFlags(0), _selectedItemPopupTriggered(false)
{
    _postfix = "##" + nanoid::generate(8);
    _entries["osg"] = new LibraryEntry("osg");

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

    _popupBar = new osgVerse::MenuBar;
    {
        MenuBar::MenuItemData newItem = createPopupMenu("Add Child");
        {
            newItem.subItems.push_back(createPopupMenu("Transform",
                [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
            {
                if (!addOperation(_selectedItem.get(), "osg::MatrixTransform"))
                    OSG_WARN << "[SceneHierarchy] Failed adding transform: " << _selectedItem->name << std::endl;
            }));

            MenuBar::MenuItemData newGeomItem = createPopupMenu("Geometry");
            {
                newGeomItem.subItems.push_back(createPopupMenu("Box",
                    [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
                { OSG_NOTICE << "TODO: add Box" << std::endl; }));
                newGeomItem.subItems.push_back(createPopupMenu("Sphere",
                    [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
                { OSG_NOTICE << "TODO: add Sphere" << std::endl; }));
                newGeomItem.subItems.push_back(createPopupMenu("Cylinder",
                    [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
                { OSG_NOTICE << "TODO: add Cylinder" << std::endl; }));
                newGeomItem.subItems.push_back(createPopupMenu("Cone",
                    [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
                { OSG_NOTICE << "TODO: add Cone" << std::endl; }));
                newGeomItem.subItems.push_back(createPopupMenu("Capsule",
                    [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
                { OSG_NOTICE << "TODO: add Capsule" << std::endl; }));
                newGeomItem.subItems.push_back(createPopupMenu("Plane",
                    [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
                { OSG_NOTICE << "TODO: add Plane" << std::endl; }));
            }
            newItem.subItems.push_back(newGeomItem);

            newItem.subItems.push_back(createPopupMenu("Light",
                [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
            { OSG_NOTICE << "TODO: add Light" << std::endl; }));

            newItem.subItems.push_back(createPopupMenu("Camera",
                [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
            { OSG_NOTICE << "TODO: add Camera" << std::endl; }));
        }
        _popupMenus.push_back(newItem);

        _popupMenus.push_back(createPopupMenu("Add from ...",
            [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
        { OSG_NOTICE << "TODO: add from ..." << std::endl; }));

        _popupMenus.push_back(createPopupMenu("Remove",
            [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
        {
            if (!removeOperation(_selectedItem.get()))
                OSG_WARN << "[SceneHierarchy] Failed removing child: " << _selectedItem->name << std::endl;
        }));

        _popupMenus.push_back(osgVerse::MenuBar::MenuItemData::separator);

        _popupMenus.push_back(createPopupMenu("Move Up",
            [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
        { OSG_NOTICE << "TODO: move-up" << std::endl; }));

        _popupMenus.push_back(createPopupMenu("Move Down",
            [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
        { OSG_NOTICE << "TODO: move-down" << std::endl; }));

        _popupMenus.push_back(createPopupMenu("Cut",
            [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
        { OSG_NOTICE << "TODO: cut" << std::endl; }));

        _popupMenus.push_back(createPopupMenu("Copy",
            [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
        { OSG_NOTICE << "TODO: copy" << std::endl; }));

        _popupMenus.push_back(createPopupMenu("Paste",
            [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
        { OSG_NOTICE << "TODO: paste" << std::endl; }));

        _popupMenus.push_back(osgVerse::MenuBar::MenuItemData::separator);

        MenuBar::MenuItemData stateItem = createPopupMenu("Information");
        {
            stateItem.subItems.push_back(createPopupMenu("Stateset",
                [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
            { _stateFlags = 0; refreshItem(_sceneTreeData.get()); }));

            stateItem.subItems.push_back(createPopupMenu("Callback",
                [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
            { _stateFlags = 1; refreshItem(_sceneTreeData.get()); }));

            stateItem.subItems.push_back(createPopupMenu("Ref Count",
                [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
            { _stateFlags = 2; refreshItem(_sceneTreeData.get()); }));
        }
        _popupMenus.push_back(stateItem);

        _popupMenus.push_back(createPopupMenu("Hide",
            [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
        { OSG_NOTICE << "TODO: hide" << std::endl; }));

        _popupMenus.push_back(createPopupMenu("Rename",
            [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
        { OSG_NOTICE << "TODO: rename" << std::endl; }));

        _popupMenus.push_back(createPopupMenu("Share",
            [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
        { OSG_NOTICE << "TODO: share" << std::endl; }));
    }
}

MenuBar::MenuItemData SceneHierarchy::createPopupMenu(const std::string& name,
                                                      ImGuiComponentBase::ActionCallback cb)
{
    MenuBar::MenuItemData item(osgVerse::MenuBar::TR(name) + "##popup" + _postfix);
    item.callback = cb; return item;
}

void SceneHierarchy::setViewer(osgViewer::View* view, osg::Node* rootNode)
{
    _camTreeData->userData = NULL;
    _sceneTreeData->userData = new SceneDataProxy(
        (rootNode != NULL) ? rootNode : view->getSceneData());
}

TreeView::TreeData* SceneHierarchy::addItem(TreeView::TreeData* parent, osg::Object* obj,
                                            bool asSubGraph, unsigned int parentMask)
{
    if (!obj) return NULL; else if (!parent) parent = _sceneTreeData.get();
    if (asSubGraph)
    {
        osg::Node* node = dynamic_cast<osg::Node*>(obj);
        if (node != NULL) { HierarchyCreatingVisitor hv(this, parent, false); node->accept(hv); }
    }

    NodeToItemMapper::iterator itr = _nodeToItemMap.find(obj);
    if (itr == _nodeToItemMap.end())
    {
        TreeView::TreeData* treeItem = new TreeView::TreeData;
        treeItem->id = "##node_" + nanoid::generate(8);
        treeItem->tooltip = obj->libraryName() + std::string("::") + obj->className();
        treeItem->userData = new SceneDataProxy(obj); _nodeToItemMap[obj] = treeItem;
        parent->children.push_back(treeItem);
    }
    updateStateInformation(_nodeToItemMap[obj].get(), obj, parentMask);
    return _nodeToItemMap[obj].get();
}

void SceneHierarchy::removeUnusedItem(TreeView::TreeData* parent, bool recursively)
{
    for (size_t i = 0; i < parent->children.size();)
    {
        TreeView::TreeData* child = parent->children[i].get();
        removeUnusedItem(child, true);
        if (child->userData.valid())
        {
            SceneDataProxy* proxy = static_cast<SceneDataProxy*>(child->userData.get());
            if (proxy && !proxy->data)  // if data is empty, scene is already deleted
            { parent->children.erase(parent->children.begin() + i); continue; }
        } ++i;
    }
}

void SceneHierarchy::refreshItem(TreeView::TreeData* parent)
{
    if (!parent) parent = _selectedItem.get(); if (!parent) parent = _sceneTreeData.get();
    if (!parent) return; else removeUnusedItem(parent, true);

    osg::Node* node = SceneDataProxy::get<osg::Node*>(parent->userData.get());
    if (node != NULL) { HierarchyCreatingVisitor hv(this, parent, false); node->accept(hv); }
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
            OSG_WARN << "[SceneHierarchy] Item to remove (" << current->name << ") not belong to "
                     << parent->name << std::endl;
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
    for (size_t i = 0; i < _popupMenus.size(); ++i)
        _popupBar->showMenuItem(_popupMenus[i], mgr, content);
}

void SceneHierarchy::updateStateInformation(TreeView::TreeData* item, osg::Object* obj,
                                            unsigned int parentMask)
{
    bool fixedColor = false; item->color = 0xFFFFFFFF;
    if (parentMask == 0) { item->color = 0xFF666666; fixedColor = true; }
    item->name = (obj->getName().empty() ? obj->className() : obj->getName()) + item->id;

#if OSG_VERSION_GREATER_THAN(3, 4, 1)
    osg::Drawable* drawable = obj->asDrawable(); 
#else
    osg::Drawable* drawable = dynamic_cast<osg::Drawable*>(obj);
#endif
    if (drawable)
    {
        if (!fixedColor) item->color = 0xFF00FFFF;

        switch (_stateFlags)
        {
        case 0: item->state = drawable->getStateSet() ? "SS" : ""; break;
        case 1: item->state = (drawable->getUpdateCallback() || drawable->getEventCallback()) ? "CB" : ""; break;
        default: item->state = std::to_string(drawable->referenceCount()); break;
        }
    }
#if OSG_VERSION_GREATER_THAN(3, 3, 0)
    else if (obj->asNode())
    {
        osg::Node* node = obj->asNode();
#else
    else
    {
        osg::Node* node = dynamic_cast<osg::Node*>(obj); if (!node) return;
#endif
        if (node->asGeode())
            { if (!fixedColor) item->color = 0xFFAAFFAA; }
        else if (node->asGroup())
        {
            if (node->asGroup()->asTransform())
                { if (!fixedColor) item->color = 0xFFFFAAAA; }
            else
            {
                osg::PagedLOD* plod = dynamic_cast<osg::PagedLOD*>(node);
                if (plod && !fixedColor) item->color = 0xFFAAAAFF;
            }
        }

        switch (_stateFlags)
        {
        case 0: item->state = node->getStateSet() ? "SS" : ""; break;
        case 1: item->state = (node->getUpdateCallback() || node->getEventCallback()) ? "CB" : ""; break;
        default: item->state = std::to_string(node->referenceCount()); break;
        }
    }
}

bool SceneHierarchy::addOperation(TreeView::TreeData* item, const std::string& childType, bool isGeom)
{
    bool finished = false;
    if (item && item->userData.valid())
    {
        osg::ref_ptr<osg::Object> parent = SceneDataProxy::get<osg::Object*>(item->userData.get());
        osg::ref_ptr<osg::Object> child = _entries["osg"]->create(childType);
        if (isGeom)
        {
            osg::ref_ptr<osg::Object> geode = dynamic_cast<osg::Geode*>(parent.get());
            if (!geode)
            {
                geode = _entries["osg"]->create("osg::Geode");
                if (_entries["osg"]->callMethod(parent.get(), "addChild", geode.get()))
                    finished = false;
            }
            finished = _entries["osg"]->callMethod(geode.get(), "addDrawable", child.get());
        }
        else if (parent.valid() && child.valid())
            finished = _entries["osg"]->callMethod(parent.get(), "addChild", child.get());
        if (finished) addItem(item, child.get(), true);
    }
    return finished;
}

bool SceneHierarchy::removeOperation(TreeView::TreeData* subItem)
{
    osg::ref_ptr<osg::Object> child = SceneDataProxy::get<osg::Object*>(subItem ? subItem->userData.get() : NULL);
    if (child.valid())
    {
        std::vector<TreeView::TreeData*> parents = _treeView->findParents(subItem);
        for (size_t i = 0; i < parents.size(); ++i)
        {
            bool finished = false;
            osg::ref_ptr<osg::Object> parent = SceneDataProxy::get<osg::Object*>(parents[i]->userData.get());
            if (dynamic_cast<osg::Geode*>(parent.get()) != NULL)
                finished = _entries["osg"]->callMethod(parent.get(), "removeDrawable", child.get());
            else
                finished = _entries["osg"]->callMethod(parent.get(), "removeChild", child.get());
            if (finished) removeItem(parents[i], child.get(), true);
        }
        return !parents.empty();
    }
    return false;
}
