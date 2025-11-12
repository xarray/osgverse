#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( ClipPlane,
                         new osg::ClipPlane,
                         osg::ClipPlane,
                         "osg::Object osg::StateAttribute osg::ClipPlane" )
{
    ADD_VEC4D_SERIALIZER( ClipPlane, osg::Vec4d() );  // _clipPlane
    ADD_UINT_SERIALIZER( ClipPlaneNum, 0 );  // _clipPlaneNum
}
