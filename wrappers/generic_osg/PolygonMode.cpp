#include <GenericReserializer.h>
using namespace osgVerse;

enum osg_PolygonMode_Mode { POINT = GL_POINT, LINE = GL_LINE, FILL = GL_FILL };
BEGIN_USER_TABLE( Mode, osg_PolygonMode );
    ADD_USER_VALUE( POINT );
    ADD_USER_VALUE( LINE );
    ADD_USER_VALUE( FILL );
END_USER_TABLE()

USER_READ_FUNC( Mode, readModeValue )

// _modeFront, _modeBack
static bool readMode(InputStream& is, InputUserData& ud)
{
    bool frontAndBack;
    is >> is.PROPERTY("UseFrontAndBack") >> frontAndBack;
    is >> is.PROPERTY("Front"); int value1 = readModeValue(is);
    is >> is.PROPERTY("Back"); int value2 = readModeValue(is);

    if ( frontAndBack )
        ud.add("setMode", GL_FRONT_AND_BACK, value1);
    else
    {
        ud.add("setMode", GL_FRONT, value1);
        ud.add("setMode", GL_BACK, value2);
    }
    return true;
}

REGISTER_OBJECT_WRAPPER( PolygonMode,
                         new osg::PolygonMode,
                         osg::PolygonMode,
                         "osg::Object osg::StateAttribute osg::PolygonMode" )
{
    ADD_USER_SERIALIZER( Mode );  // _modeFront, _modeBack
}
