#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( NodeTrackerCallback,
                         new osg::NodeTrackerCallback,
                         osg::NodeTrackerCallback,
                         "osg::Object osg::NodeCallback osg::NodeTrackerCallback" )
{
    ADD_OBJECT_SERIALIZER( TrackNode, osg::Node, NULL );  // _trackNodePath
}
