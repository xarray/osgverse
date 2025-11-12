#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( ComputeBoundingSphereCallback,
                         new osg::Node::ComputeBoundingSphereCallback,
                         osg::Node::ComputeBoundingSphereCallback,
                         "osg::Object osg::ComputeBoundingSphereCallback" )
{
}
