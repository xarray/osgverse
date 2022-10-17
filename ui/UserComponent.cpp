#include "UserComponent.h"
using namespace osgVerse;

StandardComponent::StandardComponent(PropertyItemManager::StandardItemType st,
                                     PropertyItem::TargetType t, osg::Object* target, osg::Camera* cam)
{
    PropertyItem* p = PropertyItemManager::instance()->getStandardItem(st);
    if (p) { p->setTarget(target, t); p->setCamera(cam); setPropertyUI(p); }
}
