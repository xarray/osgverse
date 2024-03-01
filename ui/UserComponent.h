#ifndef MANA_UI_USERCOMPONENT_HPP
#define MANA_UI_USERCOMPONENT_HPP

#include <osg/Camera>
#include <osg/MatrixTransform>
#include "../pipeline/Global.h"
#include "PropertyInterface.h"
#include "ImGui.h"
#include "3rdparty/any.hpp"
#include <map>

namespace osgVerse
{
    class UserComponent : public Component
    {
    public:
        UserComponent() : Component() {}
        UserComponent(const UserComponent& c, const osg::CopyOp& op)
        :   Component(c, op), _variables(c._variables), _setters(c._setters),
            _getters(c._getters), _propertyUI(c._propertyUI) {}
        META_Object(osgVerse, UserComponent);
        
        enum ValueType
        {
            AnyValue = 0, StringValue, BoolValue, IntValue, UIntValue,
            FloatValue, Float2Value, Float3Value, Float4Value,
            DoubleValue, Double2Value, Double3Value, Double4Value,
            ColorValue, MatrixValue, NodeValue, DrawableValue, ComponentValue,
            MaxValueEnd
        };
        typedef std::function<bool(const linb::any& a0, const linb::any& a1,
                                   const linb::any& a2)> SetCallback;
        typedef std::function<bool(const linb::any& inA, linb::any& a0,
                                   linb::any& a1, linb::any& a2)> GetCallback;
        
        /** Variable methods */
        struct VariableData
        {
            ValueType type; linb::any value;
            VariableData() : type(MaxValueEnd) {}
        };
        void addVariableData(const std::string& n, const VariableData& vd) { _variables[n] = vd; }
        void getVariable(const std::string& n, VariableData& vd) { vd = _variables[n]; }
        std::map<std::string, VariableData>& getAllVariables() { return _variables; }

        /** Setter methods */
        struct SetterData
        {
            SetterData() : count(0) {}
            SetCallback callback[3];
            ValueType type[3]; int count;
        };
        void addSetterData(const std::string& n, const SetterData& sd) { _setters[n] = sd; }
        void getSetter(const std::string& n, SetterData& sd) { sd = _setters[n]; }
        std::map<std::string, SetterData>& getAllSetters() { return _setters; }

        /** Getter methods */
        struct GetterData
        {
            GetterData() : inType(MaxValueEnd), count(0) {}
            GetCallback callback[3];
            ValueType type[3], inType; int count;
        };
        void addGetterData(const std::string& n, const GetterData& gd) { _getters[n] = gd; }
        void getGetter(const std::string& n, GetterData& gd) { gd = _getters[n]; }
        std::map<std::string, GetterData>& getAllGetters() { return _getters; }

        /** Set a custom property UI if needed */
        void setPropertyUI(PropertyItem* item) { _propertyUI = item; }
        PropertyItem* getPropertyUI() { return _propertyUI.get(); }

        /** Actual update work in node's callback */
        virtual void run(osg::Object* object, osg::Referenced* nv) {}

    protected:
        std::map<std::string, VariableData> _variables;
        std::map<std::string, SetterData> _setters;
        std::map<std::string, GetterData> _getters;
        osg::ref_ptr<PropertyItem> _propertyUI;
    };

    class UserComponentGroup : public Component
    {
    public:
        UserComponentGroup() : Component() {}
        UserComponentGroup(const UserComponentGroup& c, const osg::CopyOp& op)
            : Component(c, op), _components(c._components) {}
        META_Object(osgVerse, UserComponentGroup);

        void add(const std::string& clsName, UserComponent* uc) { _components[clsName] = uc; }
        void remove(const std::string& clsName)
        { if (_components.find(clsName) != _components.end()) _components.erase(_components.find(clsName)); }

        std::map<std::string, osg::ref_ptr<UserComponent>>& getAll() { return _components; }
        const std::map<std::string, osg::ref_ptr<UserComponent>>& getAll() const { return _components; }

        UserComponent* get(const std::string& clsName) { return _components[clsName]; }
        virtual void run(osg::Object* object, osg::Referenced* nv) {}  // do nothing

    protected:
        std::map<std::string, osg::ref_ptr<UserComponent>> _components;
    };

    class DefaultComponent : public UserComponent
    {
    public:
        DefaultComponent(const std::string& clsName = "", osg::Object* target = NULL, osg::Camera* cam = NULL);
        DefaultComponent(const DefaultComponent& c, const osg::CopyOp& op) : UserComponent(c, op) {}
        META_Object(osgVerse, DefaultComponent);
    };

    class StandardComponent : public UserComponent
    {
    public:
        StandardComponent(PropertyItemManager::StandardItemType st = PropertyItemManager::BasicNodeItem,
                          PropertyItem::TargetType t = PropertyItem::UnknownType,
                          osg::Object* target = NULL, osg::Camera* cam = NULL);
        StandardComponent(const StandardComponent& c, const osg::CopyOp& op) : UserComponent(c, op) {}
        META_Object(osgVerse, StandardComponent);
    };

    class UserComponentManager : public osg::Referenced
    {
    public:
        static UserComponentManager* instance();

        UserComponent* createStandard(PropertyItemManager::StandardItemType st, PropertyItem::TargetType t,
                                      osg::Object* target, osg::Camera* cam);
        UserComponent* createExtended(const std::string& className, osg::Object* target, osg::Camera* cam);
        
        void registerComponent(const std::string& className, UserComponent* uc);
        void registerComponents(UserComponentGroup* ucg);
        void unregisterComponent(const std::string& className);
        void unregisterComponent(osg::Object* target, PropertyItemManager::StandardItemType st);

    protected:
        typedef std::pair<osg::Object*, PropertyItemManager::StandardItemType> ComponentTarget;
        std::map<ComponentTarget, osg::ref_ptr<UserComponent>> _stdComponents;
        std::map<std::string, osg::ref_ptr<UserComponent>> _extComponents;
    };
}

#endif
