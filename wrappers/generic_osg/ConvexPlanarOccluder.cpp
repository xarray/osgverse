#include <GenericReserializer.h>
using namespace osgVerse;

static void readConvexPlanarPolygon(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        osg::Vec3d vertex; is >> vertex;
        ud.add("polygon", vertex);
    }
    is >> is.END_BRACKET;
}

static bool readOccluder(InputStream& is, InputUserData& ud)
{
    readConvexPlanarPolygon( is, ud);
    return true;
}

static bool readHoles(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        is >> is.PROPERTY("Polygon");
        readConvexPlanarPolygon(is, ud);
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( ConvexPlanarOccluder,
                         new osg::ConvexPlanarOccluder,
                         osg::ConvexPlanarOccluder,
                         "osg::Object osg::ConvexPlanarOccluder" )
{
    ADD_USER_SERIALIZER( Occluder );  // _occluder
    ADD_USER_SERIALIZER( Holes );  // _holeList
}
