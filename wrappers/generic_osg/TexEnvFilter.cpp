#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( TexEnvFilter,
                         new osg::TexEnvFilter,
                         osg::TexEnvFilter,
                         "osg::Object osg::StateAttribute osg::TexEnvFilter" )
{
    ADD_FLOAT_SERIALIZER( LodBias, 0.0f );  // _lodBias
}
