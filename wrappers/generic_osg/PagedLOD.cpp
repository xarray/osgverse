#include <GenericReserializer.h>
using namespace osgVerse;

// _databasePath
static bool readDatabasePath(InputStream& is, InputUserData& ud)
{
    bool hasPath; is >> hasPath;
    if ( !hasPath )
    {
        /*if (is.getOptions() && !is.getOptions()->getDatabasePathList().empty())
        {
            const std::string& optionPath = is.getOptions()->getDatabasePathList().front();
            if ( !optionPath.empty() ) ud.add("setDatabasePath", optionPath);
        }*/
    }
    else
    {
        std::string path; is.readWrappedString( path );
        ud.add( "setDatabasePath", path );
    }
    return true;
}

// _perRangeDataList
static bool readRangeDataList(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        std::string name; is.readWrappedString( name );
        ud.add("setFileName", i, name);
    }
    is >> is.END_BRACKET;

    size = 0; is >> is.PROPERTY("PriorityList") >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        float offset, scale;
        is >> offset >> scale;
        ud.add("setPriorityOffset", i, offset);
        ud.add("setPriorityScale", i, scale);
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
            if ( child.valid() ) ud.add("addChild", &child);
        }
        is >> is.END_BRACKET;
    }
    return true;
}

REGISTER_OBJECT_WRAPPER( PagedLOD,
                         new osg::PagedLOD,
                         osg::PagedLOD,
                         "osg::Object osg::Node osg::LOD osg::PagedLOD" )
{
    // Note: osg::Group is not in the list to prevent recording dynamic loaded children

    ADD_USER_SERIALIZER( DatabasePath );  // _databasePath
    ADD_UINT_SERIALIZER( FrameNumberOfLastTraversal, 0 );  // _frameNumberOfLastTraversal, note, not required, removed from soversion 70 onwwards, see below
    ADD_UINT_SERIALIZER( NumChildrenThatCannotBeExpired, 0 );  // _numChildrenThatCannotBeExpired
    ADD_BOOL_SERIALIZER( DisableExternalChildrenPaging, false );  // _disableExternalChildrenPaging
    ADD_USER_SERIALIZER( RangeDataList );  // _perRangeDataList
    ADD_USER_SERIALIZER( Children );  // _children (which are not loaded from external)

    {
        UPDATE_TO_VERSION_SCOPED( 70 )
        REMOVE_SERIALIZER( FrameNumberOfLastTraversal );
    }



}
