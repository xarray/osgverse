#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( Switch,
                         new osg::Switch,
                         osg::Switch,
                         "osg::Object osg::Node osg::Group osg::Switch" )
{
    ADD_BOOL_SERIALIZER( NewChildDefaultValue, true );  // _newChildDefaultValue
    ADD_LIST_SERIALIZER( ValueList, bool );  // _values
}
