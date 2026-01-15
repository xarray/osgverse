#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( PointSprite,
                         new osg::PointSprite,
                         osg::PointSprite,
                         "osg::Object osg::StateAttribute osg::PointSprite" )
{
    BEGIN_ENUM_SERIALIZER( CoordOriginMode, UPPER_LEFT );
        ADD_ENUM_VALUE( UPPER_LEFT );
        ADD_ENUM_VALUE( LOWER_LEFT );
    END_ENUM_SERIALIZER();  // _coordOriginMode
}
