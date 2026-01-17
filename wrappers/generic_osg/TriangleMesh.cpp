#include <GenericReserializer.h>
using namespace osgVerse;

#define ARRAY_FUNCTIONS( PROP, TYPE ) \
    static bool read##PROP(InputStream& is, InputUserData& ud) { \
        ObjectTypeAndID array = ud.readObjectFromStream(is, #TYPE); \
        ud.add("set" #PROP, &array); \
        return true; \
    }

ARRAY_FUNCTIONS( Vertices, osg::Vec3Array )
ARRAY_FUNCTIONS( Indices, osg::IndexArray )

REGISTER_OBJECT_WRAPPER( TriangleMesh,
                         new osg::TriangleMesh,
                         osg::TriangleMesh,
                         "osg::Object osg::Shape osg::TriangleMesh" )
{
    ADD_USER_SERIALIZER( Vertices );  // _vertices
    ADD_USER_SERIALIZER( Indices );  // _indices
}
