#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include "properties.h"

#include <imgui/ImGuizmo.h>
#include <ui/CommandHandler.h>
#include <ui/PropertyInterface.h>
#include <pipeline/Global.h>
using namespace osgVerse;

Properties::Properties(osg::Camera* cam, osg::MatrixTransform* mt)
    : _camera(cam), _sceneRoot(mt)
{
    _propWindow = new Window(TR("Properties##ed02"));
    _propWindow->pos = osg::Vec2(1600, 0);
    _propWindow->sizeMin = osg::Vec2(320, 780);
    _propWindow->sizeMax = osg::Vec2(640, 780);
    _propWindow->alpha = 0.8f;
    _propWindow->useMenuBar = true;
    _propWindow->flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;
    _propWindow->userData = this;
}

bool Properties::handleCommand(CommandData* cmd)
{
    /* Refresh properties:
       - cmd->object (parent) is the node/drawable whose properties are updated
       - cmd->value (string): to-be-updated component's name, or empty to update all
    */
    PropertyItemManager* propManager = PropertyItemManager::instance();
    osg::StateSet* stateSet = NULL; ComponentCallback* callback = NULL;
    _properties.clear();

    osg::Drawable* targetD = dynamic_cast<osg::Drawable*>(cmd->object.get());
    if (targetD)
    {
        PropertyItem* p0 = propManager->getStandardItem(PropertyItemManager::BasicDrawableItem);
        if (p0)
        {
            p0->setTarget(targetD, PropertyItem::DrawableType);
            p0->setCamera(_camera.get()); _properties.push_back(p0);
        }

        osg::Geometry* targetG = targetD->asGeometry();
        if (targetG)
        {
            PropertyItem* p1 = propManager->getStandardItem(PropertyItemManager::GeometryItem);
            if (p1) { p1->setTarget(targetG, PropertyItem::GeometryType); _properties.push_back(p1); }
        }

        stateSet = targetD->getStateSet();
        callback = dynamic_cast<ComponentCallback*>(targetD->getUpdateCallback());
    }

    osg::Node* targetN = dynamic_cast<osg::Node*>(cmd->object.get());
    if (!targetD && targetN)
    {
        PropertyItem* p0 = propManager->getStandardItem(PropertyItemManager::BasicNodeItem);
        if (p0)
        {
            p0->setTarget(targetN, PropertyItem::NodeType);
            p0->setCamera(_camera.get()); _properties.push_back(p0);
        }
        
        osg::Transform* targetT = targetN->asTransform();
        if (targetT)
        {
            osg::MatrixTransform* targetMT = targetT->asMatrixTransform();
            if (targetMT)
            {
                PropertyItem* p1 = propManager->getStandardItem(PropertyItemManager::TransformItem);
                if (p1)
                {
                    p1->setTarget(targetMT, PropertyItem::MatrixType);
                    p1->setCamera(_camera.get()); _properties.push_back(p1);
                }
            }

            osg::PositionAttitudeTransform* targetPT = targetT->asPositionAttitudeTransform();
            if (targetPT)
            {
                PropertyItem* p1 = propManager->getStandardItem(PropertyItemManager::TransformItem);
                if (p1)
                {
                    p1->setTarget(targetPT, PropertyItem::PoseType);
                    p1->setCamera(_camera.get()); _properties.push_back(p1);
                }
            }
        }

        osg::Camera* targetCam = targetN->asCamera();
        if (targetCam)
        {
            PropertyItem* p1 = propManager->getStandardItem(PropertyItemManager::CameraItem);
            if (p1) { p1->setTarget(targetCam, PropertyItem::CameraType); _properties.push_back(p1); }
        }

        stateSet = targetN->getStateSet();
        callback = dynamic_cast<ComponentCallback*>(targetN->getUpdateCallback());
    }

    if (stateSet != NULL)
    {
        PropertyItem* p2 = propManager->getStandardItem(PropertyItemManager::TextureItem);
        if (p2) { p2->setTarget(stateSet, PropertyItem::StateSetType); _properties.push_back(p2); }

        PropertyItem* p3 = propManager->getStandardItem(PropertyItemManager::ShaderItem);
        if (p3) { p3->setTarget(stateSet, PropertyItem::StateSetType); _properties.push_back(p3); }

        PropertyItem* p4 = propManager->getStandardItem(PropertyItemManager::UniformItem);
        if (p4) { p4->setTarget(stateSet, PropertyItem::StateSetType); _properties.push_back(p4); }

        PropertyItem* p5 = propManager->getStandardItem(PropertyItemManager::AttributeItem);
        if (p5) { p5->setTarget(stateSet, PropertyItem::StateSetType); _properties.push_back(p5); }
    }

    if (callback != NULL)
    {
        for (size_t i = 0; i < callback->getNumComponents(); ++i)
        {
            Component* c = callback->getComponent(i);
            std::string fullName = std::string(c->libraryName())
                                 + std::string("::") + c->className();
            PropertyItem* pC = propManager->getExtendedItem(fullName);
            if (pC) { pC->setTarget(c, PropertyItem::ComponentType); _properties.push_back(pC); }
        }
    }
    return true;
}

bool Properties::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    bool done = _propWindow->show(mgr, content);
    {
        for (size_t i = 0; i < _properties.size(); ++i)
        {
            osgVerse::PropertyItem* item = _properties[i];
            std::string title = TR(item->title()) + "##prop" + std::to_string(i + 1);
            if (ImGui::CollapsingHeader(title.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (item->show(mgr, content))
                {
                    if (item->needRefreshUI())
                        CommandBuffer::instance()->add(RefreshHierarchyItem, item->getTarget(), "");
                }
            }
        }
    }
    _propWindow->showEnd();
    return done;
}
