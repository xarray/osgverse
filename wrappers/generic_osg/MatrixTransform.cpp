#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( MatrixTransform,
                         new osg::MatrixTransform,
                         osg::MatrixTransform,
                         "osg::Object osg::Node osg::Group osg::Transform osg::MatrixTransform" )
{
    ADD_MATRIX_SERIALIZER( Matrix, osg::Matrix() );  // _matrix
}
