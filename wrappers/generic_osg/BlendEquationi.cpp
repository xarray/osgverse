#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( BlendEquationi,
                         new osg::BlendEquationi,
                         osg::BlendEquationi,
                         "osg::Object osg::StateAttribute osg::BlendEquation osg::BlendEquationi" )
{
    ADD_UINT_SERIALIZER( Index, 0 );
}
