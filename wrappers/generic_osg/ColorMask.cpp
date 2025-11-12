#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( ColorMask,
                         new osg::ColorMask,
                         osg::ColorMask,
                         "osg::Object osg::StateAttribute osg::ColorMask" )
{
    ADD_BOOL_SERIALIZER( RedMask, true );  // _red
    ADD_BOOL_SERIALIZER( GreenMask, true );  // _green
    ADD_BOOL_SERIALIZER( BlueMask, true );  // _blue
    ADD_BOOL_SERIALIZER( AlphaMask, true );  // _alpha
}
