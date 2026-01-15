#include <GenericReserializer.h>
using namespace osgVerse;

static bool readInitialBound(InputStream& is, InputUserData& ud)
{
    osg::Vec3d min, max;
    is >> is.BEGIN_BRACKET;
    is >> is.PROPERTY("Minimum") >> min;
    is >> is.PROPERTY("Maximum") >> max;
    is >> is.END_BRACKET;
    ud.add("setInitialBound", osg::BoundingBox(min, max));
    return true;
}

REGISTER_OBJECT_WRAPPER( Drawable,
                         /*new osg::Drawable*/NULL,
                         osg::Drawable,
                         "osg::Object osg::Node osg::Drawable" )
{
    {
         UPDATE_TO_VERSION_SCOPED( 154 )
         ADDED_ASSOCIATE("osg::Node")
    }

    ADD_OBJECT_SERIALIZER( StateSet, osg::StateSet, NULL );  // _stateset
    ADD_USER_SERIALIZER( InitialBound );  // _initialBound
    ADD_OBJECT_SERIALIZER( ComputeBoundingBoxCallback,
                           osg::Drawable::ComputeBoundingBoxCallback, NULL );  // _computeBoundCallback
    ADD_OBJECT_SERIALIZER( Shape, osg::Shape, NULL );  // _shape
    ADD_BOOL_SERIALIZER( SupportsDisplayList, true );  // _supportsDisplayList
    ADD_BOOL_SERIALIZER( UseDisplayList, true );  // _useDisplayList
    ADD_BOOL_SERIALIZER( UseVertexBufferObjects, false );  // _useVertexBufferObjects
    ADD_OBJECT_SERIALIZER( UpdateCallback, osg::Callback, NULL );  // _updateCallback
    ADD_OBJECT_SERIALIZER( EventCallback, osg::Callback, NULL );  // _eventCallback
    ADD_OBJECT_SERIALIZER( CullCallback, osg::Callback, NULL );  // _cullCallback
    ADD_OBJECT_SERIALIZER( DrawCallback, osg::Drawable::DrawCallback, NULL );  // _drawCallback
    {
         //now provided by Node's serialization
         UPDATE_TO_VERSION_SCOPED( 156 )
         REMOVE_SERIALIZER( StateSet )
         REMOVE_SERIALIZER( UpdateCallback )
         REMOVE_SERIALIZER( EventCallback )
         REMOVE_SERIALIZER( CullCallback )
         REMOVE_SERIALIZER( DrawCallback )
    }

    {
        UPDATE_TO_VERSION_SCOPED( 142 )
        ADD_HEXINT_SERIALIZER( NodeMask, 0xffffffff );  // _nodeMask
    }

    {
        UPDATE_TO_VERSION_SCOPED( 145 )
        ADD_BOOL_SERIALIZER( CullingActive, true );  // _cullingActive
    }

}
