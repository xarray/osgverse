#include <GenericReserializer.h>
using namespace osgVerse;

enum osg_TexGen_Coord { S, T, R, Q };
#define PLANE_FUNCTION( PROP, COORD ) \
    static bool read##PROP(InputStream& is, InputUserData& ud) { \
        osg::Plane plane; is >> plane; \
        ud.add("setPlane", COORD, plane); \
        return true; \
    }

PLANE_FUNCTION( PlaneS, osg_TexGen_Coord::S )
PLANE_FUNCTION( PlaneT, osg_TexGen_Coord::T )
PLANE_FUNCTION( PlaneR, osg_TexGen_Coord::R )
PLANE_FUNCTION( PlaneQ, osg_TexGen_Coord::Q )

REGISTER_OBJECT_WRAPPER( TexGen,
                         new osg::TexGen,
                         osg::TexGen,
                         "osg::Object osg::StateAttribute osg::TexGen" )
{
    BEGIN_ENUM_SERIALIZER( Mode, OBJECT_LINEAR );
        ADD_ENUM_VALUE( OBJECT_LINEAR );
        ADD_ENUM_VALUE( EYE_LINEAR );
        ADD_ENUM_VALUE( SPHERE_MAP );
        ADD_ENUM_VALUE( NORMAL_MAP );
        ADD_ENUM_VALUE( REFLECTION_MAP );
    END_ENUM_SERIALIZER();  // _mode

    ADD_USER_SERIALIZER( PlaneS );
    ADD_USER_SERIALIZER( PlaneT );
    ADD_USER_SERIALIZER( PlaneR );
    ADD_USER_SERIALIZER( PlaneQ );  //_plane_s, _plane_t, _plane_r, _plane_q
}
