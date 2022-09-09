#ifndef MANA_UI_PROPERTYINTERFACE_HPP
#define MANA_UI_PROPERTYINTERFACE_HPP

#include <osg/Camera>
#include <osg/MatrixTransform>
#include "ImGui.h"
#include <map>
#include <any>

namespace osgVerse
{
    class PropertyItem : public osg::Referenced
    {
    public:
        enum TargetType
        {
            UnknownType, NodeType, DrawableType, GeometryType,
            MatrixType, PoseType, StateSetType, ComponentType
        };
        void setTarget(osg::Object* o, TargetType t) { _target = o; _type = t; }
        
        virtual const char* componentName() const = 0;
        virtual const char* title() const = 0;
        virtual bool updateGui(ImGuiManager* mgr, ImGuiContentHandler* content) { return false; }

    protected:
        osg::observer_ptr<osg::Object> _target;
        TargetType _type;
    };

    class PropertyItemManager : public osg::Referenced
    {
    public:
        static PropertyItemManager* instance();

        enum StandardItemType
        {
            BasicNodeItem, BasicDrawableItem, TransformItem, GeometryItem,
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
