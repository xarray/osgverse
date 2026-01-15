#include <GenericReserializer.h>
using namespace osgVerse;

// _filenameList
static bool readFileNames(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        std::string value;
        is.readWrappedString( value );
        ud.add("setFileName", i, value);
    }
    is >> is.END_BRACKET;
    return true;
}

// _children
static bool readChildren(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size;
    if (size > 0)
    {
        is >> is.BEGIN_BRACKET;
        for ( unsigned int i=0; i<size; ++i )
        {
            ObjectTypeAndID child = ud.readObjectFromStream(is, "osg::Node");
            if ( child.valid() ) ud.add("addChild", child);
        }
        is >> is.END_BRACKET;
    }
    return true;
}

// _userDefinedCenter, _radius
static bool readUserCenter(InputStream& is, InputUserData& ud)
{
    osg::Vec3d center; double radius;
    is >> center >> radius;
    ud.add("setCenter", center);
    ud.add("setRadius", radius);
    return true;
}

REGISTER_OBJECT_WRAPPER( ProxyNode,
                         new osg::ProxyNode,
                         osg::ProxyNode,
                         "osg::Object osg::Node osg::ProxyNode" )
{
    // Note: osg::Group is not in the list to prevent recording dynamic loaded children

    ADD_USER_SERIALIZER( FileNames );  // _filenameList
    ADD_USER_SERIALIZER( Children );  // _children (which are not loaded from external)
    ADD_STRING_SERIALIZER( DatabasePath, "" );  // _databasePath

    BEGIN_ENUM_SERIALIZER( LoadingExternalReferenceMode, LOAD_IMMEDIATELY );
        ADD_ENUM_VALUE( LOAD_IMMEDIATELY );
        ADD_ENUM_VALUE( DEFER_LOADING_TO_DATABASE_PAGER );
        ADD_ENUM_VALUE( NO_AUTOMATIC_LOADING );
    END_ENUM_SERIALIZER();  // _loadingExtReference

    BEGIN_ENUM_SERIALIZER( CenterMode, USE_BOUNDING_SPHERE_CENTER );
        ADD_ENUM_VALUE( USE_BOUNDING_SPHERE_CENTER );
        ADD_ENUM_VALUE( USER_DEFINED_CENTER );
        ADD_ENUM_VALUE( UNION_OF_BOUNDING_SPHERE_AND_USER_DEFINED );
    END_ENUM_SERIALIZER();  // _centerMode

    ADD_USER_SERIALIZER( UserCenter );  // _userDefinedCenter, _radius

    //wrapper->addFinishedObjectReadCallback(new ProxyNodeFinishedObjectReadCallback());
}
