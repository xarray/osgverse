#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( CallbackObject,
                         new osg::CallbackObject,
                         osg::CallbackObject,
                         "osg::Object osg::Callback osg::CallbackObject" )
{
}
