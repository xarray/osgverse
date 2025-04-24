#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( Box,
                         new osg::Box,
                         osg::Box,
                         "osg::Object osg::Shape osg::Box" )
{
    ADD_VEC3_SERIALIZER( Center, osg::Vec3() );  // _center
    ADD_VEC3_SERIALIZER( HalfLengths, osg::Vec3() );  // _halfLengths
    ADD_QUAT_SERIALIZER( Rotation, osg::Quat() );  // _rotation
}
