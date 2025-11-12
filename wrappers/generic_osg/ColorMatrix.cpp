#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( ColorMatrix,
                         new osg::ColorMatrix,
                         osg::ColorMatrix,
                         "osg::Object osg::StateAttribute osg::ColorMatrix" )
{
    ADD_MATRIX_SERIALIZER( Matrix, osg::Matrix() );  // _matrix
}
