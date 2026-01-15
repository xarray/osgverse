#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( Script,
                         new osg::Script,
                         osg::Script,
                         "osg::Object osg::Script" )
{
    ADD_STRING_SERIALIZER( Script, "" );  // _script
    ADD_STRING_SERIALIZER( Language, "" );  // _script
}



