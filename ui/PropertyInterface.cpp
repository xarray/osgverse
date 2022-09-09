#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "PropertyInterface.h"
using namespace osgVerse;

PropertyItemManager* PropertyItemManager::instance()
{
    static osg::ref_ptr<PropertyItemManager> s_instance = new PropertyItemManager;
    return s_instance.get();
}

PropertyItemManager::PropertyItemManager()
{

}

PropertyItem* PropertyItemManager::getStandardItem(StandardItemType t)
{
    if (_standardItemMap.find(t) == _standardItemMap.end()) return NULL;
    else return _standardItemMap[t];
}

PropertyItem* PropertyItemManager::getExtendedItem(const std::string& t)
{
    if (_extendedItemMap.find(t) == _extendedItemMap.end()) return NULL;
    else return _extendedItemMap[t];
}
