#include <osg/io_utils>
#include <osg/Version>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/FileNameUtils>
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
    static TreeView::TreeData* applyItem(TreeView::TreeData* parent, osg::Object* obj)
    {
        TreeView::TreeData* treeItem = new TreeView::TreeData;
        treeItem->name = obj->getName().empty() ? obj->className() : A2U(obj->getName());
        treeItem->id = "##node_" + nanoid::generate(8);
        treeItem->name += treeItem->id;
        treeItem->tooltip = obj->libraryName() + std::string("::") + obj->className();
        treeItem->userData = obj;
        parent->children.push_back(treeItem);
        return treeItem;
    }

    virtual void apply(osg::Drawable& node) {}
    virtual void apply(osg::Geometry& node) {}

    virtual void apply(osg::Node& node)
    {
        osg::Referenced* topNode = _itemStack.top()->userData.get();
        if (topNode != &node)
        {
            TreeView::TreeData* treeItem = applyItem(_itemStack.top(), &node);
            _itemStack.push(treeItem); traverse(node); _itemStack.pop();
        }
        else traverse(node);
    }

    virtual void apply(osg::Geode& node)
    {
        osg::Referenced* topNode = _itemStack.top()->userData.get();
        if (topNode != &node)
        {
            TreeView::TreeData* treeItem = applyItem(_itemStack.top(), &node);
            for (unsigned int i = 0; i < node.getNumDrawables(); ++i)
                applyItem(treeItem, node.getDrawable(i));
            traverse(node);
        }
        else traverse(node);
    }
};

Hierarchy::Hierarchy(EditorContentHandler* ech)
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
        _camTreeData->name = TR("Main Camera"); _camTreeData->id = "##ed01main_camera";
        _camTreeData->userData = g_data.mainCamera.get();
        _treeView->treeDataList.push_back(_camTreeData);

        _sceneTreeData = new TreeView::TreeData;
        _sceneTreeData->name = TR("Scene Root"); _sceneTreeData->id = "##ed01scene_root";
        _sceneTreeData->userData = g_data.sceneRoot.get();
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
            CommandBuffer::instance()->add(SelectCommand,
                static_cast<osg::Object*>(_selectedItem->userData.get()), g_data.selector.get(), 0);
            CommandBuffer::instance()->add(RefreshProperties,
                static_cast<osg::Object*>(_selectedItem->userData.get()), "");
        }
        else
            CommandBuffer::instance()->add(GoHomeCommand,
                g_data.view.get(), static_cast<osg::Object*>(_selectedItem->userData.get()), 0);
    };

    _treeView->callbackR = [&](ImGuiManager*, ImGuiContentHandler*,
                               ImGuiComponentBase* me, const std::string& id)
    {
        TreeView* treeView = static_cast<TreeView*>(me);
        _selectedItem = treeView->findByID(id);
        if (_selectedItem.valid()) _selectedItemPopupTriggered = true;
    };

    // Popup menu: active, center | new {} | cut, copy, paste, delete | share, unshare
    {
        osgVerse::MenuBar::MenuItemData activeItem(osgVerse::MenuBar::TR("Activate##ed01m01"));
        _popupMenus.push_back(activeItem);

        osgVerse::MenuBar::MenuItemData centerItem(osgVerse::MenuBar::TR("Make Central##ed01m02"));
        centerItem.callback = [&](osgVerse::ImGuiManager*, osgVerse::ImGuiContentHandler*,
                                  osgVerse::ImGuiComponentBase* me)
        {
            if (!_selectedItem) return;
            CommandBuffer::instance()->add(GoHomeCommand,
                g_data.view.get(), static_cast<osg::Object*>(_selectedItem->userData.get()), 0);
        };
        _popupMenus.push_back(centerItem);

        _popupMenus.push_back(osgVerse::MenuBar::MenuItemData::separator);

        osgVerse::MenuBar::MenuItemData transNodeItem(osgVerse::MenuBar::TR("New Node##ed01m03"));
        transNodeItem.callback = ech->getMainMenu()->getItemCallback("New Node");
        _popupMenus.push_back(transNodeItem);

        osgVerse::MenuBar::MenuItemData new3dItem(osgVerse::MenuBar::TR("New Drawable##ed01m04"));
        {
            osgVerse::MenuBar::MenuItemData boxItem(osgVerse::MenuBar::TR("Box##ed01m0401"));
            boxItem.callback = ech->getMainMenu()->getItemCallback("Box");
            new3dItem.subItems.push_back(boxItem);

            osgVerse::MenuBar::MenuItemData sphereItem(osgVerse::MenuBar::TR("Sphere##ed01m0402"));
            sphereItem.callback = ech->getMainMenu()->getItemCallback("Sphere");
            new3dItem.subItems.push_back(sphereItem);

            osgVerse::MenuBar::MenuItemData cylinderItem(osgVerse::MenuBar::TR("Cylinder##ed01m0403"));
            cylinderItem.callback = ech->getMainMenu()->getItemCallback("Cylinder");
            new3dItem.subItems.push_back(cylinderItem);

            osgVerse::MenuBar::MenuItemData coneItem(osgVerse::MenuBar::TR("Cone##ed01m0404"));
            coneItem.callback = ech->getMainMenu()->getItemCallback("Cone");
            new3dItem.subItems.push_back(coneItem);

            osgVerse::MenuBar::MenuItemData capsuleItem(osgVerse::MenuBar::TR("Capsule##ed01m0405"));
            new3dItem.subItems.push_back(capsuleItem);

            osgVerse::MenuBar::MenuItemData quadItem(osgVerse::MenuBar::TR("Quad##ed01m0406"));
            new3dItem.subItems.push_back(quadItem);
        }
        _popupMenus.push_back(new3dItem);

        osgVerse::MenuBar::MenuItemData newFxItem(osgVerse::MenuBar::TR("New Object##ed01m05"));
        {
            osgVerse::MenuBar::MenuItemData camItem(osgVerse::MenuBar::TR("Camera##ed01m0501"));
            newFxItem.subItems.push_back(camItem);

            osgVerse::MenuBar::MenuItemData lightItem(osgVerse::MenuBar::TR("Light##ed01m0502"));
            newFxItem.subItems.push_back(lightItem);
        }
        _popupMenus.push_back(newFxItem);

        _popupMenus.push_back(osgVerse::MenuBar::MenuItemData::separator);

        osgVerse::MenuBar::MenuItemData cutItem(osgVerse::MenuBar::TR("Cut##ed01m06"));
        _popupMenus.push_back(cutItem);

        osgVerse::MenuBar::MenuItemData copyItem(osgVerse::MenuBar::TR("Copy##ed01m07"));
        _popupMenus.push_back(copyItem);

        osgVerse::MenuBar::MenuItemData shareItem(osgVerse::MenuBar::TR("Copy As Share##ed01m08"));
        _popupMenus.push_back(shareItem);

        osgVerse::MenuBar::MenuItemData pasteItem(osgVerse::MenuBar::TR("Paste##ed01m09"));
        _popupMenus.push_back(pasteItem);

        osgVerse::MenuBar::MenuItemData unshareItem(osgVerse::MenuBar::TR("Unshare##ed01m10"));
        _popupMenus.push_back(unshareItem);

        _popupMenus.push_back(osgVerse::MenuBar::MenuItemData::separator);

        osgVerse::MenuBar::MenuItemData deleteItem(osgVerse::MenuBar::TR("Delete##ed01m11"));
        _popupMenus.push_back(deleteItem);
    }
}

bool Hierarchy::handleCommand(CommandData* cmd)
{
    /* Refresh hierarchy:
       - cmd->object (parent) must be found in hierarchy
       - If cmd->value (node) found, it may be a new subgraph, or already recorded
       - cmd->valueEx (int): delete/share/... action (TODO)
    */
    osg::Node* node = NULL; osg::Drawable* drawable = NULL;
    osg::Group* parent = static_cast<osg::Group*>(cmd->object.get());
    if (!parent) return false;

    // See if node is newly created or existed, and update
    std::vector<TreeView::TreeData*> pItems = _treeView->findByUserData(parent);
    if (pItems.empty()) return false;

    HierarchyVisitor hv;
    if (cmd->get(node, 0, false))
    {
        std::vector<TreeView::TreeData*> nItems = _treeView->findByUserData(node);
        if (!nItems.empty()) return true;  // already recorded in hierarchy
        hv._itemStack.push(pItems[0]); node->accept(hv);
    }
    else if (cmd->get(drawable, 0, false))
    {
        std::vector<TreeView::TreeData*> dItems = _treeView->findByUserData(drawable);
        if (dItems.empty()) hv.applyItem(pItems[0], drawable);
    }
    return true;
}

bool Hierarchy::handleItemCommand(osgVerse::CommandData* cmd)
{
    /* Refresh hierarchy item:
       - cmd->object (item) must be found in hierarchy
    */
    std::string name = A2U(cmd->object->getName());
#if OSG_VERSION_GREATER_THAN(3, 3, 9)
    osg::Node* node = cmd->object->asNode();
#else
    osg::Node* node = dynamic_cast<osg::Node*>(cmd->object.get());
#endif

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
    // Popup menu: active (check), center | new {} | cut, copy, paste, delete | share (check), unshare (check)
    static osg::ref_ptr<osgVerse::MenuBar> s_popup = new osgVerse::MenuBar;
    for (size_t i = 0; i < _popupMenus.size(); ++i)
        s_popup->showMenuItem(_popupMenus[i], mgr, content);
}

void Hierarchy::addCreatedNode(osg::Node* node)
{
    osg::Group* parent = getSelectedGroup();
    if (parent != NULL)
        osgVerse::CommandBuffer::instance()->add(osgVerse::SetNodeCommand, parent, node, false);
    else
        osgVerse::CommandBuffer::instance()->add(osgVerse::SetNodeCommand, g_data.sceneRoot.get(), node, false);
    osgVerse::CommandBuffer::instance()->add(
        osgVerse::RefreshSceneCommand, g_data.view.get(), g_data.pipeline.get(), false);
}

void Hierarchy::addCreatedDrawable(osg::Drawable* drawable)
{
    osg::Geode* parent = getOrCreateSelectedGeode()->asGeode();
    if (parent != NULL)
        osgVerse::CommandBuffer::instance()->add(osgVerse::SetNodeCommand, parent, drawable, false);
    osgVerse::CommandBuffer::instance()->add(
        osgVerse::RefreshSceneCommand, g_data.view.get(), g_data.pipeline.get(), false);
}

void Hierarchy::addModelFromUrl(const std::string& url)
{
    osg::ref_ptr<osg::Node> loadedModel = osgDB::readNodeFile(url);
    std::string simpleName = U2A(osgDB::getSimpleFileName(url));
    if (!loadedModel) return;

    osg::MatrixTransform* modelRoot = new osg::MatrixTransform;
    modelRoot->setName(simpleName); modelRoot->addChild(loadedModel.get());

    osg::Node* n = modelRoot; osg::Group* parent = getSelectedGroup();
    if (parent != NULL)
        osgVerse::CommandBuffer::instance()->add(osgVerse::SetNodeCommand, parent, n, false);
    else
        osgVerse::CommandBuffer::instance()->add(osgVerse::SetNodeCommand, g_data.sceneRoot.get(), n, false);
    osgVerse::CommandBuffer::instance()->add(
        osgVerse::RefreshSceneCommand, g_data.view.get(), g_data.pipeline.get(), true);
}

osg::Group* Hierarchy::getSelectedGroup()
{
    if (_selectedItem.valid())
    {
        osg::Group* parent = dynamic_cast<osg::Group*>(_selectedItem->userData.get());
        osg::Geode* parentG = dynamic_cast<osg::Geode*>(_selectedItem->userData.get());
        if (parent == NULL || parentG != NULL)
        {
            if (parentG != NULL && parentG->getNumParents() > 0) parent = parentG->getParent(0);
            if (parentG == NULL)
            {
                osg::Drawable* parentD = dynamic_cast<osg::Drawable*>(_selectedItem->userData.get());
#if OSG_VERSION_GREATER_THAN(3, 2, 2)
                if (parentD != NULL && parentD->getNumParents() > 0) parent = parentD->getParent(0);
#else
                if (parentD != NULL && parentD->getNumParents() > 0)
                {
                    osg::Node* geode = parentD->getParent(0);
                    if (geode->getNumParents() > 0) parent = geode->getParent(0);
                }
#endif
            }
        }
        return parent;
    }
    return NULL;
}

osg::Node* Hierarchy::getOrCreateSelectedGeode()
{
    osg::Group* parentGroup = NULL;
    if (_selectedItem.valid())
    {
        osg::Node* parent = dynamic_cast<osg::Node*>(_selectedItem->userData.get());
        if (parent != NULL)
        {
            osg::Geode* parentG = parent->asGeode();
            if (parentG != NULL) return parentG; else parentGroup = parent->asGroup();
        }
        else
        {
            osg::Drawable* parentD = dynamic_cast<osg::Drawable*>(_selectedItem->userData.get());
            if (parentD != NULL && parentD->getNumParents() > 0) return parentD->getParent(0);
        }
    }

    // Create a new geode for geometry to use
    osg::Node* geode = new osg::Geode;
    if (parentGroup == NULL) parentGroup = g_data.sceneRoot.get();
    osgVerse::CommandBuffer::instance()->add(osgVerse::SetNodeCommand, parentGroup, geode, false);
    return geode;
}
