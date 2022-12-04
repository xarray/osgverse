#include "UserComponent.h"
using namespace osgVerse;

DefaultComponent::DefaultComponent(const std::string& clsName, osg::Object* target, osg::Camera* cam)
{
    PropertyItem* p = PropertyItemManager::instance()->getExtendedItem(clsName);
    if (p) { p->setTarget(target, PropertyItem::UnknownType); p->setCamera(cam); setPropertyUI(p); }
}

StandardComponent::StandardComponent(PropertyItemManager::StandardItemType st,
                                     PropertyItem::TargetType t, osg::Object* target, osg::Camera* cam)
{
    PropertyItem* p = PropertyItemManager::instance()->getStandardItem(st);
    if (p) { p->setTarget(target, t); p->setCamera(cam); setPropertyUI(p); }
}

UserComponentManager* UserComponentManager::instance()
{
    static osg::ref_ptr<UserComponentManager> s_instance = new UserComponentManager;
    return s_instance.get();
}

UserComponent* UserComponentManager::createStandard(
        PropertyItemManager::StandardItemType st, PropertyItem::TargetType t,
        osg::Object* target, osg::Camera* cam)
{
    ComponentTarget comTarget(target, st);
    if (_stdComponents.find(comTarget) == _stdComponents.end())
        _stdComponents[comTarget] = new StandardComponent(st, t, target, cam);
    return _stdComponents[comTarget].get();
}

UserComponent* UserComponentManager::createExtended(const std::string& className,
                                                    osg::Object* target, osg::Camera* cam)
{
    static std::string s_standardCls[] = {
        "osg::Geometry", "osg::Texture1D", "osg::Texture2D", "osg::Texture2DArray", "osg::Texture3D",
        "osg::Texture3D", "osg::TextureCubeMap", "osg::StateSet", "osg::Program", "osg::Geode",
        "osg::Group", "osg::Camera", "osg::MatrixTransform", "osg::PositionAttitudeTransform"
    };
    for (int i = 0; i < 14; ++i) { if (className == s_standardCls[i]) return NULL; }

    if (_extComponents.find(className) == _extComponents.end())
        _extComponents[className] = new DefaultComponent(className, target, cam);
    return _extComponents[className].get();
}

void UserComponentManager::registerComponent(const std::string& className, UserComponent* uc)
{
    PropertyItemManager::instance()->registerExtendedItem(className, uc->getPropertyUI());
    _extComponents[className] = uc;
}

void UserComponentManager::registerComponents(UserComponentGroup* ucg)
{
    std::map<std::string, osg::ref_ptr<UserComponent>>& comps = ucg->getAll();
    for (std::map<std::string, osg::ref_ptr<UserComponent>>::iterator itr = comps.begin();
         itr != comps.end(); ++itr)
    {
        PropertyItemManager::instance()->registerExtendedItem(
            itr->first, itr->second->getPropertyUI());
        _extComponents[itr->first] = itr->second;
    }
}

void UserComponentManager::unregisterComponent(const std::string& className)
{
    if (_extComponents.find(className) != _extComponents.end())
    {
        PropertyItemManager::instance()->unregisterExtendedItem(className);
        _extComponents.erase(_extComponents.find(className));
    }
}

void UserComponentManager::unregisterComponent(osg::Object* target, PropertyItemManager::StandardItemType st)
{
    ComponentTarget comTarget(target, st);
    if (_stdComponents.find(comTarget) != _stdComponents.end())
        _stdComponents.erase(_stdComponents.find(comTarget));
}
