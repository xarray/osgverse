#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( ScriptNodeCallback,
                         new osg::ScriptNodeCallback,
                         osg::ScriptNodeCallback,
                         "osg::Object osg::Callback osg::CallbackObject osg::ScriptNodeCallback" )
{
    ADD_OBJECT_SERIALIZER( Script, osg::Script, NULL );  // _script
    ADD_STRING_SERIALIZER( EntryPoint, "" );  // _entrypoint
}
