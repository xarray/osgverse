#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( ComputeBoundingBoxCallback,
                         new osg::Drawable::ComputeBoundingBoxCallback,
                         osg::Drawable::ComputeBoundingBoxCallback,
                         "osg::Object osg::ComputeBoundingBoxCallback" )
{
}
