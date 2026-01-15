#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( ClipNode,
                         new osg::ClipNode,
                         osg::ClipNode,
                         "osg::Object osg::Node osg::Group osg::ClipNode" )
{
    ADD_OBJECT_LIST_SERIALIZER( ClipPlaneList, osg::ClipPlane );  // _planes

    BEGIN_ENUM_SERIALIZER( ReferenceFrame, RELATIVE_RF );
        ADD_ENUM_VALUE( RELATIVE_RF );
        ADD_ENUM_VALUE( ABSOLUTE_RF );
    END_ENUM_SERIALIZER();  // _referenceFrame
}
