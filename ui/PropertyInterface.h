#ifndef MANA_UI_PROPERTYINTERFACE_HPP
#define MANA_UI_PROPERTYINTERFACE_HPP

#include <osg/Camera>
#include <osg/MatrixTransform>
#include "ImGui.h"
#include <map>
#include <any>

namespace osgVerse
{
    struct ImGuiComponentBase;

    class PropertyItem : public osg::Referenced
    {
    public:
        enum TargetType
        {
            UnknownType, NodeType, DrawableType, CameraType, GeometryType,
            MatrixType, PoseType, StateSetType, ComponentType
        };
        void setTarget(osg::Object* o, TargetType t) { _target = o; _type = t; updateTarget(); }
        osg::Object* getTarget() { return _target.get(); }

        void setCamera(osg::Camera* c) { _camera = c; }
        osg::Camera* getCamera() { return _camera.get(); }

        virtual std::string componentName() const
        { return std::string(!_target ? "?" : _target->className()) + "_PropertyItem"; }
        
        virtual bool needRefreshUI() const { return false; }
        virtual std::string title() const = 0;
        virtual void updateTarget(ImGuiComponentBase* c = NULL) = 0;
        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content) { return false; }

    protected:
        osg::observer_ptr<osg::Object> _target;
        osg::observer_ptr<osg::Camera> _camera;
        TargetType _type;
    };

    class PropertyItemManager : public osg::Referenced
    {
    public:
        static PropertyItemManager* instance();

        enum StandardItemType
        {
            BasicNodeItem, BasicDrawableItem, CameraItem, TransformItem, GeometryItem,
            TextureItem, ShaderItem, UniformItem, AttributeItem
        };
        PropertyItem* getStandardItem(StandardItemType t);
        PropertyItem* getExtendedItem(const std::string& t);

    protected:
        PropertyItemManager();

        std::map<StandardItemType, osg::ref_ptr<PropertyItem>> _standardItemMap;
        std::map<std::string, osg::ref_ptr<PropertyItem>> _extendedItemMap;
    };
}

#endif
