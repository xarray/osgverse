#include <GenericReserializer.h>
using namespace osgVerse;

#ifndef GL_FIXED_ONLY
#   define GL_FIXED_ONLY 0x891D
#endif

REGISTER_OBJECT_WRAPPER( ClampColor,
                         new osg::ClampColor,
                         osg::ClampColor,
                         "osg::Object osg::StateAttribute osg::ClampColor" )
{
    ADD_GLENUM_SERIALIZER( ClampVertexColor, GLenum, GL_FIXED_ONLY );  // _clampVertexColor
    ADD_GLENUM_SERIALIZER( ClampFragmentColor, GLenum, GL_FIXED_ONLY );  // _clampFragmentColor
    ADD_GLENUM_SERIALIZER( ClampReadColor, GLenum, GL_FIXED_ONLY );  // _clampReadColor
}
