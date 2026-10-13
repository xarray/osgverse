#include <osg/io_utils>
#include <osg/Version>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osg/Geometry>
#include <osg/Array>
#include <osgDB/ReadFile>
#include <osgDB/FileNameUtils>
#include <modeling/Utilities.h>
#include <nanoid/nanoid.h>
#include "SceneHierarchy.h"
#include <iostream>
#include <stack>
using namespace osgVerse;

// Build a solid box centered at the origin (4 vertices per face, so each face gets a flat normal)
static osg::Geometry* createBoxGeometry(float size)
{
    const float h = size * 0.5f;
    const osg::Vec3 normals[6] =
    {
        osg::Vec3(0.0f, 0.0f, 1.0f), osg::Vec3(0.0f, 0.0f, -1.0f),
        osg::Vec3(1.0f, 0.0f, 0.0f), osg::Vec3(-1.0f, 0.0f, 0.0f),
        osg::Vec3(0.0f, 1.0f, 0.0f), osg::Vec3(0.0f, -1.0f, 0.0f)
    };
    const osg::Vec3 corners[6][4] =
    {
        { osg::Vec3(-h, -h,  h), osg::Vec3( h, -h,  h), osg::Vec3( h,  h,  h), osg::Vec3(-h,  h,  h) },  // +Z
        { osg::Vec3(-h, -h, -h), osg::Vec3(-h,  h, -h), osg::Vec3( h,  h, -h), osg::Vec3( h, -h, -h) },  // -Z
        { osg::Vec3( h, -h, -h), osg::Vec3( h,  h, -h), osg::Vec3( h,  h,  h), osg::Vec3( h, -h,  h) },  // +X
        { osg::Vec3(-h, -h, -h), osg::Vec3(-h, -h,  h), osg::Vec3(-h,  h,  h), osg::Vec3(-h,  h, -h) },  // -X
        { osg::Vec3(-h,  h, -h), osg::Vec3(-h,  h,  h), osg::Vec3( h,  h,  h), osg::Vec3( h,  h, -h) },  // +Y
        { osg::Vec3(-h, -h, -h), osg::Vec3( h, -h, -h), osg::Vec3( h, -h,  h), osg::Vec3(-h, -h,  h) }   // -Y
    };
    const osg::Vec2 texcoords[4] =
    {
        osg::Vec2(0.0f, 0.0f), osg::Vec2(1.0f, 0.0f),
        osg::Vec2(1.0f, 1.0f), osg::Vec2(0.0f, 1.0f)
    };

    osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> na = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array;
    osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_TRIANGLES);
    for (int f = 0; f < 6; ++f)
    {
        unsigned int base = (unsigned int)va->size();
        for (int i = 0; i < 4; ++i)
        {
            va->push_back(corners[f][i]); na->push_back(normals[f]); ta->push_back(texcoords[i]);
        }
        de->push_back(base); de->push_back(base + 1); de->push_back(base + 2);
        de->push_back(base); de->push_back(base + 2); de->push_back(base + 3);
    }
    return osgVerse::createGeometry(va.get(), na.get(), ta.get(), de.get());
}

// Build a quad on the XY plane, centered at the origin
static osg::Geometry* createPlaneGeometry(float size)
{
    const float h = size * 0.5f;
    osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
    va->push_back(osg::Vec3(-h, -h, 0.0f)); va->push_back(osg::Vec3(h, -h, 0.0f));
    va->push_back(osg::Vec3(h, h, 0.0f)); va->push_back(osg::Vec3(-h, h, 0.0f));
    osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array;
    ta->push_back(osg::Vec2(0.0f, 0.0f)); ta->push_back(osg::Vec2(1.0f, 0.0f));
    ta->push_back(osg::Vec2(1.0f, 1.0f)); ta->push_back(osg::Vec2(0.0f, 1.0f));
    osg::ref_ptr<osg::DrawArrays> da = new osg::DrawArrays(GL_QUADS, 0, 4);
    return osgVerse::createGeometry(va.get(), NULL, ta.get(), da.get());
}

// Build a capsule (cylindrical body + two hemisphere caps) aligned with the Z axis, centered at
// the origin. There is no helper for this shape in modeling/Utilities.h, so the profile is revolved
// here: the radius/z/normal of every ring is listed and then turned around the Z axis.
static osg::Geometry* createCapsuleGeometry(float radius, float height, int segments)
{
    const float halfPi = 1.5707963268f, twoPi = 6.2831853072f;
    const float half = height * 0.5f; const int rings = 8;
    std::vector<osg::Vec4> profile;  // (radius, z, normalRadius, normalZ) for each ring
    for (int i = 0; i <= rings; ++i)  // bottom hemisphere: bottom pole -> equator
    {
        float a = halfPi * (float)i / (float)rings;
        profile.push_back(osg::Vec4(radius * sinf(a), -half - radius * cosf(a), sinf(a), -cosf(a)));
    }
    for (int i = 0; i <= rings; ++i)  // top hemisphere: equator -> top pole
    {
        float a = halfPi * (1.0f - (float)i / (float)rings);
        profile.push_back(osg::Vec4(radius * sinf(a), half + radius * cosf(a), sinf(a), cosf(a)));
    }

    osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> na = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array;
    const int P = (int)profile.size();
    for (int j = 0; j < P; ++j)
    {
        const osg::Vec4& p = profile[j];
        for (int i = 0; i <= segments; ++i)
        {
            float theta = twoPi * (float)i / (float)segments;
            float ct = cosf(theta), st = sinf(theta);
            va->push_back(osg::Vec3(p[0] * ct, p[0] * st, p[1]));
            na->push_back(osg::Vec3(p[2] * ct, p[2] * st, p[3]));
            ta->push_back(osg::Vec2((float)i / (float)segments, (float)j / (float)(P - 1)));
        }
    }

    osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_TRIANGLES);
    for (int j = 0; j < P - 1; ++j)
    {
        for (int i = 0; i < segments; ++i)
        {
            unsigned int a = j * (segments + 1) + i, b = a + 1,
                         c = (j + 1) * (segments + 1) + i, d = c + 1;
            de->push_back(a); de->push_back(b); de->push_back(c);
            de->push_back(b); de->push_back(d); de->push_back(c);
        }
    }
    return osgVerse::createGeometry(va.get(), na.get(), ta.get(), de.get());
}

// Create a primitive geometry for the "Add Child > Geometry" menu. Shapes that already have a
// builder in modeling/Utilities.h are created directly; Box/Plane/Capsule are built above.
static osg::Geometry* createPrimitiveGeometry(const std::string& type)
{
    const int segments = 32;
    if (type == "Box") return createBoxGeometry(2.0f);
    if (type == "Plane") return createPlaneGeometry(2.0f);
    if (type == "Capsule") return createCapsuleGeometry(1.0f, 2.0f, segments);
    if (type == "Sphere") return osgVerse::createEllipsoid(osg::Vec3(), 1.0f, 1.0f, 1.0f, segments);
    if (type == "Cylinder")
        return osgVerse::createPrism(osg::Vec3(0.0f, 0.0f, -1.0f), 1.0f, 1.0f, 2.0f, segments, true);
    if (type == "Cone")
        return osgVerse::createPyramid(osg::Vec3(0.0f, 0.0f, -1.0f), 1.0f, 2.0f, segments, true);
    return NULL;
}

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
                { if (!addGeometryOperation(_selectedItem.get(), "Box")) OSG_WARN << "[SceneHierarchy] Failed adding Box" << std::endl; }));
                newGeomItem.subItems.push_back(createPopupMenu("Sphere",
                    [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
                { if (!addGeometryOperation(_selectedItem.get(), "Sphere")) OSG_WARN << "[SceneHierarchy] Failed adding Sphere" << std::endl; }));
                newGeomItem.subItems.push_back(createPopupMenu("Cylinder",
                    [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
                { if (!addGeometryOperation(_selectedItem.get(), "Cylinder")) OSG_WARN << "[SceneHierarchy] Failed adding Cylinder" << std::endl; }));
                newGeomItem.subItems.push_back(createPopupMenu("Cone",
                    [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
                { if (!addGeometryOperation(_selectedItem.get(), "Cone")) OSG_WARN << "[SceneHierarchy] Failed adding Cone" << std::endl; }));
                newGeomItem.subItems.push_back(createPopupMenu("Capsule",
                    [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
                { if (!addGeometryOperation(_selectedItem.get(), "Capsule")) OSG_WARN << "[SceneHierarchy] Failed adding Capsule" << std::endl; }));
                newGeomItem.subItems.push_back(createPopupMenu("Plane",
                    [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
                { if (!addGeometryOperation(_selectedItem.get(), "Plane")) OSG_WARN << "[SceneHierarchy] Failed adding Plane" << std::endl; }));
            }
            newItem.subItems.push_back(newGeomItem);

            newItem.subItems.push_back(createPopupMenu("Light",
                [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
            { OSG_NOTICE << "TODO: add Light" << std::endl; }));

            newItem.subItems.push_back(createPopupMenu("Camera",
                [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
            {
                if (!addOperation(_selectedItem.get(), "osg::Camera"))
                    OSG_WARN << "[SceneHierarchy] Failed adding camera: " << _selectedItem->name << std::endl;
            }));
        }
        _popupMenus.push_back(newItem);

        _popupMenus.push_back(createPopupMenu("Add from ...",
            [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
        { OSG_NOTICE << "TODO: add from ..." << std::endl; }));

        _popupMenus.push_back(createPopupMenu("Remove",
            [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
        {
            if (!removeOperation(_selectedItem.get()))
                OSG_WARN << "[SceneHierarchy] Failed removing child: "
                         << (_selectedItem.valid() ? _selectedItem->name : std::string("?")) << std::endl;
        }));

        _popupMenus.push_back(osgVerse::MenuBar::MenuItemData::separator);

        _popupMenus.push_back(createPopupMenu("Move Up",
            [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
        {
            if (!moveOperation(_selectedItem.get(), true))
                OSG_WARN << "[SceneHierarchy] Failed moving up: "
                         << (_selectedItem.valid() ? _selectedItem->name : std::string("?")) << std::endl;
        }));

        _popupMenus.push_back(createPopupMenu("Move Down",
            [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
        {
            if (!moveOperation(_selectedItem.get(), false))
                OSG_WARN << "[SceneHierarchy] Failed moving down: "
                         << (_selectedItem.valid() ? _selectedItem->name : std::string("?")) << std::endl;
        }));

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

TreeView::TreeData* SceneHierarchy::findItem(osg::Object* obj) const
{
    NodeToItemMapper::const_iterator itr = _nodeToItemMap.find(obj);
    return (itr != _nodeToItemMap.end()) ? itr->second.get() : NULL;
}

void SceneHierarchy::selectItem(TreeView::TreeData* item, bool notify)
{
    if (!item) return;
    _selectedItem = item;
    if (_treeView.valid())
    {
        _treeView->selectedItemID = item->id;
        item->flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (notify && _clickAction) _clickAction(_treeView.get(), item);
}

void SceneHierarchy::clearSelection(bool notify)
{
    if (_selectedItem.valid())
        _selectedItem->flags &= ~ImGuiTreeNodeFlags_Selected;
    _selectedItem = NULL;
    if (_treeView.valid()) _treeView->selectedItemID.clear();
    if (notify && _clickAction) _clickAction(_treeView.get(), NULL);
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

bool SceneHierarchy::addOperation(TreeView::TreeData* item, const std::string& childType)
{
    if (!item || !item->userData.valid()) return false;
    osg::ref_ptr<osg::Object> parent = SceneDataProxy::get<osg::Object*>(item->userData.get());
    osg::ref_ptr<osg::Object> child = _entries["osg"]->create(childType);
    if (!parent.valid() || !child.valid()) return false;

    bool finished = _entries["osg"]->callMethod(parent.get(), "addChild", child.get());
    if (finished) addItem(item, child.get(), true);
    return finished;
}

bool SceneHierarchy::removeOperation(TreeView::TreeData* subItem)
{
    osg::ref_ptr<osg::Object> child = SceneDataProxy::get<osg::Object*>(
        subItem ? subItem->userData.get() : NULL);
    if (!child.valid()) return false;

    bool selected = (subItem == _selectedItem.get()), finishedAny = false;
    std::vector<TreeView::TreeData*> parents = _treeView->findParents(subItem);
    for (size_t i = 0; i < parents.size(); ++i)
    {
        osg::ref_ptr<osg::Object> parent = SceneDataProxy::get<osg::Object*>(parents[i]->userData.get());
        if (!parent.valid()) continue;  // invalid parent: skip instead of passing NULL to the serializer

        bool finished = false;
        if (dynamic_cast<osg::Geode*>(parent.get()) != NULL)
            finished = _entries["osg"]->callMethod(parent.get(), "removeDrawable", child.get());
        else
            finished = _entries["osg"]->callMethod(parent.get(), "removeChild", child.get());
        if (finished && removeItem(parents[i], child.get(), true)) finishedAny = true;
    }

    // The removed object may be destroyed right after this call (the scene graph was its last
    // owner), so the selection and any panel depending on it must be cleared immediately -
    // otherwise the properties panel would dereference a dangling object next frame.
    if (finishedAny)
    {
        osg::Object* selObj = _selectedItem.valid() ? getObject(_selectedItem.get()) : NULL;
        if (selected || selObj == NULL) clearSelection(true);
    }
    return finishedAny;
}

osg::Object* SceneHierarchy::getObject(TreeView::TreeData* item) const
{
    return item ? SceneDataProxy::get<osg::Object*>(item->userData.get()) : NULL;
}

TreeView::TreeData* SceneHierarchy::getParentItem(TreeView::TreeData* item) const
{
    if (!item || !_treeView.valid()) return NULL;
    std::vector<TreeView::TreeData*> parents = _treeView->findParents(item);
    return parents.empty() ? NULL : parents[0];
}

bool SceneHierarchy::moveOperation(TreeView::TreeData* item, bool moveUp)
{
    TreeView::TreeData* parentItem = getParentItem(item);
    if (!parentItem) return false;

    int index = -1, count = (int)parentItem->children.size();
    for (int i = 0; i < count; ++i)
        if (parentItem->children[i].get() == item) { index = i; break; }
    int newIndex = index + (moveUp ? -1 : 1);
    if (index < 0 || newIndex < 0 || newIndex >= count) return false;

    osg::ref_ptr<osg::Object> parentObj = getObject(parentItem);
    if (!parentObj.valid()) return false;

    // Gather the child objects in their current order; the strong references keep them alive while
    // they are temporarily detached from the scene graph during the rebuild below.
    std::vector<osg::ref_ptr<osg::Object>> objects;
    for (int i = 0; i < count; ++i)
    {
        osg::ref_ptr<osg::Object> obj = getObject(parentItem->children[i].get());
        if (!obj.valid()) return false;
        objects.push_back(obj);
    }
    std::swap(objects[index], objects[newIndex]);

    // Reorder through the serializer method layer only. The OSG wrappers expose no "insertChild",
    // and setChild/setDrawable *replace* the node at the given index (they do not move it), so the
    // only safe reorder is to remove every child and add them back in the requested order, which
    // also keeps every node's parent list consistent.
    bool isGeode = (dynamic_cast<osg::Geode*>(parentObj.get()) != NULL);
    const std::string adder = isGeode ? "addDrawable" : "addChild";
    const std::string remover = isGeode ? "removeDrawable" : "removeChild";
    for (int i = 0; i < count; ++i)
        _entries["osg"]->callMethod(parentObj.get(), remover, objects[i].get());

    bool ok = true;
    for (int i = 0; i < count; ++i)
        ok = _entries["osg"]->callMethod(parentObj.get(), adder, objects[i].get()) && ok;

    if (ok) std::swap(parentItem->children[index], parentItem->children[newIndex]);
    return ok;
}

bool SceneHierarchy::addGeometryOperation(TreeView::TreeData* item, const std::string& primitive)
{
    osg::ref_ptr<osg::Object> target = item ? getObject(item) : NULL; if (!target.valid()) return false;
    osg::ref_ptr<osg::Geometry> geometry = createPrimitiveGeometry(primitive);
    if (!geometry.valid()) return false;

    // Find the Geode that will hold the new geometry; create one (under a group) if needed
    osg::ref_ptr<osg::Object> geodeOwner; osg::ref_ptr<osg::Geode> geode;
    if (dynamic_cast<osg::Geode*>(target.get()) != NULL)
        geode = static_cast<osg::Geode*>(target.get());
    else if (dynamic_cast<osg::Group*>(target.get()) != NULL)
        geodeOwner = target;
    else  // target is a drawable: attach to its parent's Geode/Group
    {
        osg::Object* parentObj = getObject(getParentItem(item));
        if (dynamic_cast<osg::Geode*>(parentObj) != NULL)
            geode = static_cast<osg::Geode*>(parentObj);
        else if (parentObj && dynamic_cast<osg::Group*>(parentObj) != NULL)
            geodeOwner = parentObj;
    }

    if (!geode.valid())
    {
        if (!geodeOwner.valid()) return false;
        osg::ref_ptr<osg::Object> newGeode = _entries["osg"]->create("osg::Geode");
        geode = dynamic_cast<osg::Geode*>(newGeode.get());
        if (!geode || !_entries["osg"]->callMethod(geodeOwner.get(), "addChild", geode.get()))
            return false;
        addItem(findItem(geodeOwner.get()), geode.get(), true);
    }

    if (!_entries["osg"]->callMethod(geode.get(), "addDrawable", geometry.get())) return false;
    addItem(findItem(geode.get()), geometry.get(), false);
    return true;
}
