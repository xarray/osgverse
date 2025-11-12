#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( Callback,
                         new osg::Callback,
                         osg::Callback,
                         "osg::Object osg::Callback" )
{
    ADD_OBJECT_SERIALIZER( NestedCallback, osg::Callback, NULL );  // _nestedCallback
}
