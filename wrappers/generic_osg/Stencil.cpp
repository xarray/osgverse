#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( Stencil,
                         new osg::Stencil,
                         osg::Stencil,
                         "osg::Object osg::StateAttribute osg::Stencil" )
{
    BEGIN_ENUM_SERIALIZER( Function, ALWAYS );
        ADD_ENUM_VALUE( NEVER );
        ADD_ENUM_VALUE( LESS );
        ADD_ENUM_VALUE( EQUAL );
        ADD_ENUM_VALUE( LEQUAL );
        ADD_ENUM_VALUE( GREATER );
        ADD_ENUM_VALUE( NOTEQUAL );
        ADD_ENUM_VALUE( GEQUAL );
        ADD_ENUM_VALUE( ALWAYS );
    END_ENUM_SERIALIZER();  // _func

    ADD_INT_SERIALIZER( FunctionRef, 0 );  // _funcRef
    ADD_HEXINT_SERIALIZER( FunctionMask, ~0u );  // _funcMask

    BEGIN_ENUM_SERIALIZER( StencilFailOperation, KEEP );
        ADD_ENUM_VALUE( KEEP );
        ADD_ENUM_VALUE( ZERO );
        ADD_ENUM_VALUE( REPLACE );
        ADD_ENUM_VALUE( INCR );
        ADD_ENUM_VALUE( DECR );
        ADD_ENUM_VALUE( INVERT );
        ADD_ENUM_VALUE( INCR_WRAP );
        ADD_ENUM_VALUE( DECR_WRAP );
    END_ENUM_SERIALIZER();  // _sfail

    BEGIN_ENUM_SERIALIZER( StencilPassAndDepthFailOperation, KEEP );
        ADD_ENUM_VALUE( KEEP );
        ADD_ENUM_VALUE( ZERO );
        ADD_ENUM_VALUE( REPLACE );
        ADD_ENUM_VALUE( INCR );
        ADD_ENUM_VALUE( DECR );
        ADD_ENUM_VALUE( INVERT );
        ADD_ENUM_VALUE( INCR_WRAP );
        ADD_ENUM_VALUE( DECR_WRAP );
    END_ENUM_SERIALIZER();  // _zfail

    BEGIN_ENUM_SERIALIZER( StencilPassAndDepthPassOperation, KEEP );
        ADD_ENUM_VALUE( KEEP );
        ADD_ENUM_VALUE( ZERO );
        ADD_ENUM_VALUE( REPLACE );
        ADD_ENUM_VALUE( INCR );
        ADD_ENUM_VALUE( DECR );
        ADD_ENUM_VALUE( INVERT );
        ADD_ENUM_VALUE( INCR_WRAP );
        ADD_ENUM_VALUE( DECR_WRAP );
    END_ENUM_SERIALIZER();  // _zpass

    ADD_HEXINT_SERIALIZER( WriteMask, ~0u );  // _writeMask
}
