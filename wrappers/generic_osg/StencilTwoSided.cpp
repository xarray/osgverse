#include <GenericReserializer.h>
using namespace osgVerse;

#ifndef GL_INCR_WRAP
#   define GL_INCR_WRAP 0x8507
#endif
#ifndef GL_DECR_WRAP
#   define GL_DECR_WRAP 0x8508
#endif

enum osg_StencilTwoSided_Function
{
    NEVER = GL_NEVER, LESS = GL_LESS, EQUAL = GL_EQUAL, LEQUAL = GL_LEQUAL,
    GREATER = GL_GREATER, NOTEQUAL = GL_NOTEQUAL, GEQUAL = GL_GEQUAL, ALWAYS = GL_ALWAYS
};

BEGIN_USER_TABLE(Function, osg_StencilTwoSided);
    ADD_USER_VALUE( NEVER );
    ADD_USER_VALUE( LESS );
    ADD_USER_VALUE( EQUAL );
    ADD_USER_VALUE( LEQUAL );
    ADD_USER_VALUE( GREATER );
    ADD_USER_VALUE( NOTEQUAL );
    ADD_USER_VALUE( GEQUAL );
    ADD_USER_VALUE( ALWAYS );
END_USER_TABLE()
USER_READ_FUNC( Function, readFunction1 )

enum osg_StencilTwoSided_Operation
{
    KEEP = GL_KEEP, ZERO = GL_ZERO, REPLACE = GL_REPLACE, INCR = GL_INCR,
    DECR = GL_DECR, INVERT = GL_INVERT, INCR_WRAP = GL_INCR_WRAP, DECR_WRAP = GL_DECR_WRAP
};

BEGIN_USER_TABLE(Operation, osg_StencilTwoSided);
    ADD_USER_VALUE( KEEP );
    ADD_USER_VALUE( ZERO );
    ADD_USER_VALUE( REPLACE );
    ADD_USER_VALUE( INCR );
    ADD_USER_VALUE( DECR );
    ADD_USER_VALUE( INVERT );
    ADD_USER_VALUE( INCR_WRAP );
    ADD_USER_VALUE( DECR_WRAP );
END_USER_TABLE()
USER_READ_FUNC( Operation, readOperation )

#define STENCIL_INT_VALUE_FUNC( PROP, TYPE ) \
    static bool read##PROP(InputStream& is, InputUserData& ud) { \
        TYPE value1; is >> is.PROPERTY("Front") >> value1; \
        TYPE value2; is >> is.PROPERTY("Back") >> value2; \
        ud.add("set" #PROP, GL_FRONT, value1); \
        ud.add("set" #PROP, GL_BACK, value2); return true; }

#define STENCIL_USER_VALUE_FUNC( PROP, TYPE ) \
    static bool read##PROP(InputStream& is, InputUserData& ud) { \
        is >> is.PROPERTY("Front"); int value1 = read##TYPE(is); \
        is >> is.PROPERTY("Back"); int value2 = read##TYPE(is); \
        ud.add("set" #PROP, GL_FRONT, value1); \
        ud.add("set" #PROP, GL_BACK, value2); return true; }

STENCIL_USER_VALUE_FUNC( Function, Function1 )
STENCIL_INT_VALUE_FUNC( FunctionRef, int )
STENCIL_INT_VALUE_FUNC( FunctionMask, unsigned int )
STENCIL_USER_VALUE_FUNC( StencilFailOperation, Operation )
STENCIL_USER_VALUE_FUNC( StencilPassAndDepthFailOperation, Operation )
STENCIL_USER_VALUE_FUNC( StencilPassAndDepthPassOperation, Operation )
STENCIL_INT_VALUE_FUNC( WriteMask, unsigned int )

REGISTER_OBJECT_WRAPPER( StencilTwoSided,
                         new osg::StencilTwoSided,
                         osg::StencilTwoSided,
                         "osg::Object osg::StateAttribute osg::StencilTwoSided" )
{
    ADD_USER_SERIALIZER( Function );  // _func
    ADD_USER_SERIALIZER( FunctionRef );  // _funcRef
    ADD_USER_SERIALIZER( FunctionMask );  // _funcMask
    ADD_USER_SERIALIZER( StencilFailOperation );  // _sfail
    ADD_USER_SERIALIZER( StencilPassAndDepthFailOperation );  // _zfail
    ADD_USER_SERIALIZER( StencilPassAndDepthPassOperation );  // _zpass
    ADD_USER_SERIALIZER( WriteMask );  // _writeMask
}
