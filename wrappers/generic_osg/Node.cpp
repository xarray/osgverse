#include <GenericReserializer.h>
using namespace osgVerse;

// _initialBound
static bool readInitialBound(InputStream& is, InputUserData& ud)
{
    osg::Vec3d center;
    double radius;
    is >> is.BEGIN_BRACKET;
    is >> is.PROPERTY("Center") >> center;
    is >> is.PROPERTY("Radius") >> radius;
    is >> is.END_BRACKET;
    ud.add("setInitialBound", osg::BoundingSphere(center, radius));
    return true;
}

// _descriptions
static bool readDescriptions(InputStream& is, InputUserData& ud)
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

REGISTER_OBJECT_WRAPPER( Node,
                         new osg::Node,
                         osg::Node,
                         "osg::Object osg::Node" )
{
    ADD_USER_SERIALIZER( InitialBound );  // _initialBound
    ADD_OBJECT_SERIALIZER( ComputeBoundingSphereCallback,
                           osg::Node::ComputeBoundingSphereCallback, NULL );  // _computeBoundCallback
    ADD_OBJECT_SERIALIZER( UpdateCallback, osg::Callback, NULL );  // _updateCallback
    ADD_OBJECT_SERIALIZER( EventCallback, osg::Callback, NULL );  // _eventCallback
    ADD_OBJECT_SERIALIZER( CullCallback, osg::Callback, NULL );  // _cullCallback
    ADD_BOOL_SERIALIZER( CullingActive, true );  // _cullingActive
    ADD_HEXINT_SERIALIZER( NodeMask, 0xffffffff );  // _nodeMask

    ADD_USER_SERIALIZER( Descriptions );  // _descriptions, deprecated
    {
        UPDATE_TO_VERSION_SCOPED( 77 )
        REMOVE_SERIALIZER( Descriptions );
    }

    ADD_OBJECT_SERIALIZER( StateSet, osg::StateSet, NULL );  // _stateset
}
