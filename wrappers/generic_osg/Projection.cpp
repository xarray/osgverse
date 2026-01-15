#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( Projection,
                         new osg::Projection,
                         osg::Projection,
                         "osg::Object osg::Node osg::Group osg::Projection" )
{
    ADD_MATRIX_SERIALIZER( Matrix, osg::Matrix() );  // _matrix
}
