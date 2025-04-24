#include <GenericReserializer.h>
using namespace osgVerse;

static bool readPositionList(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize();
    is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        osg::Vec3d pos; is >> pos;
        ud.add("readPositionList", i, pos);
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( Billboard,
                         new osg::Billboard,
                         osg::Billboard,
                         "osg::Object osg::Node osg::Geode osg::Billboard" )
{
    BEGIN_ENUM_SERIALIZER( Mode, AXIAL_ROT );
        ADD_ENUM_VALUE( POINT_ROT_EYE );
        ADD_ENUM_VALUE( POINT_ROT_WORLD );
        ADD_ENUM_VALUE( AXIAL_ROT );
    END_ENUM_SERIALIZER();  // _mode

    ADD_VEC3_SERIALIZER( Axis, osg::Vec3f() );  // _axis
    ADD_VEC3_SERIALIZER( Normal, osg::Vec3f() );  // _normal
    ADD_USER_SERIALIZER( PositionList );  // _positionList
}
