#include <GenericReserializer.h>
using namespace osgVerse;

// _userDefinedCenter, _radius
static bool readUserCenter(InputStream& is, InputUserData& ud)
{
    osg::Vec3d center; double radius;
    is >> center >> radius;
    ud.add("setCenter", center);
    ud.add("setRadius", radius);
    return true;
}

// _rangeList
static bool readRangeList(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        float min, max;
        is >> min >> max;
        ud.add("setRange", i, min, max);
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( LOD,
                         new osg::LOD,
                         osg::LOD,
                         "osg::Object osg::Node osg::Group osg::LOD" )
{
    BEGIN_ENUM_SERIALIZER( CenterMode, USE_BOUNDING_SPHERE_CENTER );
        ADD_ENUM_VALUE( USE_BOUNDING_SPHERE_CENTER );
        ADD_ENUM_VALUE( USER_DEFINED_CENTER );
        ADD_ENUM_VALUE( UNION_OF_BOUNDING_SPHERE_AND_USER_DEFINED );
    END_ENUM_SERIALIZER();  // _centerMode

    ADD_USER_SERIALIZER( UserCenter );  // _userDefinedCenter, _radius

    BEGIN_ENUM_SERIALIZER( RangeMode, DISTANCE_FROM_EYE_POINT );
        ADD_ENUM_VALUE( DISTANCE_FROM_EYE_POINT );
        ADD_ENUM_VALUE( PIXEL_SIZE_ON_SCREEN );
    END_ENUM_SERIALIZER();  // _rangeMode

    ADD_USER_SERIALIZER( RangeList );  // _rangeList
}
