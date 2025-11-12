#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( ClipNode,
                         new osg::ClipNode,
                         osg::ClipNode,
                         "osg::Object osg::Node osg::Group osg::ClipNode" )
{
    // TODO ////ADD_LIST_SERIALIZER( ClipPlaneList, osg::ClipNode::ClipPlaneList );  // _planes

    BEGIN_ENUM_SERIALIZER( ReferenceFrame, RELATIVE_RF );
        ADD_ENUM_VALUE( RELATIVE_RF );
        ADD_ENUM_VALUE( ABSOLUTE_RF );
    END_ENUM_SERIALIZER();  // _referenceFrame
}
