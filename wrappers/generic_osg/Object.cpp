#include <GenericReserializer.h>
using namespace osgVerse;

static bool readUserData(InputStream& is, InputUserData& ud)
{
    is >> is.BEGIN_BRACKET;
    ObjectTypeAndID object = ud.readObjectFromStream(is, "osg::Object");
    if (object.valid()) ud.add("setUserData", &object);
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( Object,
                         new osg::DummyObject,
                         osg::Object,
                         "osg::Object" )
{
    ADD_STRING_SERIALIZER( Name, "" );  // _name

    BEGIN_ENUM_SERIALIZER( DataVariance, UNSPECIFIED );
        ADD_ENUM_VALUE( STATIC );
        ADD_ENUM_VALUE( DYNAMIC );
        ADD_ENUM_VALUE( UNSPECIFIED );
    END_ENUM_SERIALIZER();  // _dataVariance

    ADD_USER_SERIALIZER( UserData );  // _userData, deprecated
    {
        UPDATE_TO_VERSION_SCOPED( 77 )
        REMOVE_SERIALIZER( UserData );
        ADD_OBJECT_SERIALIZER( UserDataContainer, osg::UserDataContainer, NULL );
    }
}
