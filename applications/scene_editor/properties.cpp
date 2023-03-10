#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include "properties.h"

#include <imgui/ImGuizmo.h>
#include <ui/CommandHandler.h>
#include <ui/UserComponent.h>
#include <pipeline/Global.h>
using namespace osgVerse;

Properties::Properties(EditorContentHandler* ech)
    : _selectedProperty(-1)
{
    _propWindow = new Window(TR("Properties##ed02"));
    _propWindow->pos = osg::Vec2(0.8f, 0.0f);
    _propWindow->size = osg::Vec2(0.2f, 0.75f);
    _propWindow->alpha = 0.9f;
    _propWindow->useMenuBar = false;
    _propWindow->flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;
    _propWindow->userData = this;

    // Popup menu: enable, up, down | copy, paste, delete | edit
    {
        osgVerse::MenuBar::MenuItemData enableItem(osgVerse::MenuBar::TR("Enable##ed02m01"));
        _popupMenus.push_back(enableItem);

        osgVerse::MenuBar::MenuItemData upItem(osgVerse::MenuBar::TR("Move Up##ed02m02"));
        _popupMenus.push_back(upItem);

        osgVerse::MenuBar::MenuItemData downItem(osgVerse::MenuBar::TR("Move Down##ed02m03"));
        _popupMenus.push_back(downItem);

        _popupMenus.push_back(osgVerse::MenuBar::MenuItemData::separator);

        osgVerse::MenuBar::MenuItemData copyItem(osgVerse::MenuBar::TR("Copy##ed02m04"));
        _popupMenus.push_back(copyItem);

        osgVerse::MenuBar::MenuItemData pasteItem(osgVerse::MenuBar::TR("Paste##ed02m05"));
        _popupMenus.push_back(pasteItem);

        osgVerse::MenuBar::MenuItemData deleteItem(osgVerse::MenuBar::TR("Delete##ed02m06"));
        _popupMenus.push_back(deleteItem);

        _popupMenus.push_back(osgVerse::MenuBar::MenuItemData::separator);

        osgVerse::MenuBar::MenuItemData editItem(osgVerse::MenuBar::TR("Edit##ed02m07"));
        _popupMenus.push_back(editItem);
    }
}

bool Properties::handleCommand(CommandData* cmd)
{
    /* Refresh properties:
       - cmd->object (parent) is the node/drawable whose properties are updated
       - cmd->value (string): to-be-updated component's name, or empty to update all (TODO)
    */
    UserComponentManager* ucm = UserComponentManager::instance();
    osg::Camera* mainCam = g_data.mainCamera.get();
    osg::StateSet* stateSet = NULL; ComponentCallback* callback = NULL;
    _properties.clear(); _selectedProperty = -1;

    osg::Drawable* targetD = dynamic_cast<osg::Drawable*>(cmd->object.get());
    if (targetD)
    {
        osg::Geometry* targetG = targetD->asGeometry();
        _properties.push_back(ucm->createStandard(
            PropertyItemManager::BasicDrawableItem, PropertyItem::DrawableType, targetD, mainCam));
        if (targetG) _properties.push_back(ucm->createStandard(
            PropertyItemManager::GeometryItem, PropertyItem::GeometryType, targetG, mainCam));

        stateSet = targetD->getStateSet();
        callback = dynamic_cast<ComponentCallback*>(targetD->getUpdateCallback());
    }

    osg::Node* targetN = dynamic_cast<osg::Node*>(cmd->object.get());
    if (!targetD && targetN)
    {
        osg::Transform* targetT = targetN->asTransform();
        _properties.push_back(ucm->createStandard(
            PropertyItemManager::BasicNodeItem, PropertyItem::NodeType, targetN, mainCam));
        
        if (targetT)
        {
            osg::MatrixTransform* targetMT = targetT->asMatrixTransform();
            if (targetMT) _properties.push_back(ucm->createStandard(
                PropertyItemManager::TransformItem, PropertyItem::MatrixType, targetMT, mainCam));

            osg::PositionAttitudeTransform* targetPT = targetT->asPositionAttitudeTransform();
            if (targetPT) _properties.push_back(ucm->createStandard(
                PropertyItemManager::TransformItem, PropertyItem::PoseType, targetPT, mainCam));
        }

        osg::Camera* targetCam = targetN->asCamera();
        if (targetCam) _properties.push_back(ucm->createStandard(
            PropertyItemManager::CameraItem, PropertyItem::CameraType, targetCam, mainCam));

        stateSet = targetN->getStateSet();
        callback = dynamic_cast<ComponentCallback*>(targetN->getUpdateCallback());
    }

    if (stateSet != NULL)
    {
        _properties.push_back(ucm->createStandard(
            PropertyItemManager::TextureItem, PropertyItem::StateSetType, stateSet, mainCam));
        _properties.push_back(ucm->createStandard(
            PropertyItemManager::ShaderItem, PropertyItem::StateSetType, stateSet, mainCam));
        _properties.push_back(ucm->createStandard(
            PropertyItemManager::AttributeItem, PropertyItem::StateSetType, stateSet, mainCam));
    }

    // For extended class, find their component from the fullname
    // For any other (not registered) class, add an indicating component here
    if (cmd->object.valid())
    {
        std::string clsName = std::string(cmd->object->libraryName())
                            + std::string("::") + cmd->object->className();
        UserComponent* userNodeData = ucm->createExtended(clsName, cmd->object.get(), mainCam);
        if (userNodeData) _properties.push_back(userNodeData);
    }
    else return false;

    if (callback != NULL)
    {
        for (size_t i = 0; i < callback->getNumComponents(); ++i)
        {
            Component* c = callback->getComponent(i);
            std::string compName = std::string(c->libraryName()) + std::string("::") + c->className();
            UserComponent* userCallbackData = ucm->createExtended(compName, c, mainCam);
            if (userCallbackData) _properties.push_back(userCallbackData);
        }
    }
    return true;
}

bool Properties::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    bool done = _propWindow->show(mgr, content);
    if (done)
    {
        int headerFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Bullet
                        | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        for (size_t i = 0; i < _properties.size(); ++i)
        {
            osgVerse::UserComponent* component = _properties[i];
            osgVerse::PropertyItem* item = component->getPropertyUI();
            if (!item) continue;

            std::string title = TR(item->title()) + "##prop" + std::to_string(i + 1);
            if (ImGui::ArrowButton((title + "Arrow").c_str(), ImGuiDir_Down))  // TODO: disabled = ImGuiDir_None
            {
                // Select the item and also open popup menu
                ImGui::OpenPopup((title + "Popup").c_str());
                _selectedProperty = (int)i;
            }
            ImGui::SameLine();

            // Show property item (selected or not)
            if (i == _selectedProperty)
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.4f, 0.4f, 0.0f, 1.0f));
            bool toOpen = ImGui::CollapsingHeader(title.c_str(), headerFlags);
            if (i == _selectedProperty) ImGui::PopStyleColor();

            if (ImGui::BeginPopup((title + "Popup").c_str()))
            {
                showPopupMenu(item, mgr, content);
                ImGui::EndPopup();
            }

            if (toOpen)
            {
                if (item->show(mgr, content))
                {
                    if (item->needRefreshUI())
                        CommandBuffer::instance()->add(RefreshHierarchyItem, item->getTarget(), "");
                }
            }
        }
        _propWindow->showEnd();
    }
    return done;
}

void Properties::showPopupMenu(osgVerse::PropertyItem* item, osgVerse::ImGuiManager* mgr,
                               osgVerse::ImGuiContentHandler* content)
{
    // Popup menu: enable (check), up (check), down (check) | copy, paste, delete | edit (check)
    static osg::ref_ptr<osgVerse::MenuBar> s_popup = new osgVerse::MenuBar;
    for (size_t i = 0; i < _popupMenus.size(); ++i)
        s_popup->showMenuItem(_popupMenus[i], mgr, content);
}
