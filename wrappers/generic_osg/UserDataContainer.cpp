#include <GenericReserializer.h>
using namespace osgVerse;

static bool readUDC_UserData(InputStream& is, InputUserData& ud)
{
    is >> is.BEGIN_BRACKET;
    ObjectTypeAndID object = ud.readObjectFromStream(is, "osg::Object");
    if(object.valid()) ud.add("setUserData", &object);
    is >> is.END_BRACKET;
    return true;
}

// _descriptions
static bool readUDC_Descriptions(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        std::string value;
        is.readWrappedString( value );
        ud.add("addDescription", value);
    }
    is >> is.END_BRACKET;
    return true;
}

static bool readUDC_UserObjects(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for( unsigned int i=0; i<size; ++i )
    {
        ObjectTypeAndID object = ud.readObjectFromStream(is, "osg::Object");
        if (object.valid()) ud.add("addUserObject", &object);
    }
    is >> is.END_BRACKET;
    return true;
}

namespace UserDataContainerNamespace
{
    REGISTER_OBJECT_WRAPPER( UserDataContainer,
                            0,
                            osg::UserDataContainer,
                            "osg::Object osg::UserDataContainer" )
    {
    }
}

namespace DefaultUserDataContainerNamespace
{
    REGISTER_OBJECT_WRAPPER( DefaultUserDataContainer,
                            new osg::DefaultUserDataContainer,
                            osg::DefaultUserDataContainer,
                            "osg::Object osg::UserDataContainer osg::DefaultUserDataContainer" )
    {
        ADD_USER_SERIALIZER( UDC_UserData );  // _userData
        ADD_USER_SERIALIZER( UDC_Descriptions );  // _descriptions
        ADD_USER_SERIALIZER( UDC_UserObjects );  // _userData
    }
}
