#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( ConvexHull,
                         new osg::ConvexHull,
                         osg::ConvexHull,
                         "osg::Object osg::Shape osg::TriangleMesh osg::ConvexHull" )
{
}
