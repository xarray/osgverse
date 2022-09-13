#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "PropertyInterface.h"
#include "ImGuiComponents.h"
#include <imgui/ImGuizmo.h>
using namespace osgVerse;

extern PropertyItem* createBasicPropertyItem();
extern PropertyItem* createTransformPropertyItem();
extern PropertyItem* createGeometryPropertyItem();
extern PropertyItem* createTexturePropertyItem();

///////////////////////////

PropertyItemManager* PropertyItemManager::instance()
{
    static osg::ref_ptr<PropertyItemManager> s_instance = new PropertyItemManager;
    return s_instance.get();
}

PropertyItemManager::PropertyItemManager()
{
    _standardItemMap[BasicNodeItem] = createBasicPropertyItem();
    _standardItemMap[BasicDrawableItem] = createBasicPropertyItem();
    _standardItemMap[TransformItem] = createTransformPropertyItem();
    _standardItemMap[GeometryItem] = createGeometryPropertyItem();
    _standardItemMap[TextureItem] = createTexturePropertyItem();
    // TODO: light/lod/camera/shader+uniform/attributes...
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
