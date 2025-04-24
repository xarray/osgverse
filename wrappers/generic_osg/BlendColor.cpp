#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( BlendColor,
                         new osg::BlendColor,
                         osg::BlendColor,
                         "osg::Object osg::StateAttribute osg::BlendColor" )
{
    ADD_VEC4_SERIALIZER( ConstantColor, osg::Vec4() );  // _constantColor
}
