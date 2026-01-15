#include <GenericReserializer.h>
using namespace osgVerse;

static bool readShaderSource(InputStream& is, InputUserData& ud)
{
    std::string code;
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        std::string line;
        is.readWrappedString( line );
        code.append( line ); code.append( 1, '\n' );
    }
    is >> is.END_BRACKET;
    ud.add("setShaderSource", code);
    return true;
}

REGISTER_OBJECT_WRAPPER( Shader,
                         new osg::Shader,
                         osg::Shader,
                         "osg::Object osg::Shader" )
{
    BEGIN_ENUM_SERIALIZER( Type, UNDEFINED);
        ADD_ENUM_VALUE( VERTEX );
        ADD_ENUM_VALUE( TESSCONTROL );
        ADD_ENUM_VALUE( TESSEVALUATION );
        ADD_ENUM_VALUE( FRAGMENT );
        ADD_ENUM_VALUE( GEOMETRY );
        ADD_ENUM_VALUE( COMPUTE );
        ADD_ENUM_VALUE( UNDEFINED );
    END_ENUM_SERIALIZER();  // _type

    ADD_USER_SERIALIZER( ShaderSource );  // _shaderSource
    ADD_OBJECT_SERIALIZER( ShaderBinary, osg::ShaderBinary, NULL );  // _shaderBinary
}
