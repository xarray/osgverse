#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( OccluderNode,
                         new osg::OccluderNode,
                         osg::OccluderNode,
                         "osg::Object osg::Node osg::Group osg::OccluderNode" )
{
    ADD_OBJECT_SERIALIZER( Occluder, osg::ConvexPlanarOccluder, NULL );  // _occluder
}
