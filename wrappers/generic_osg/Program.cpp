#include <GenericReserializer.h>
using namespace osgVerse;

#define PROGRAM_LIST_FUNC( PROP, TYPE, DATA ) \
    static bool read##PROP(InputStream& is, InputUserData& ud) { \
        unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET; \
        for ( unsigned int i=0; i<size; ++i ) { \
            std::string key; unsigned int value; \
            is >> key >> value; ud.add("add" #DATA, key, value); \
        } \
        is >> is.END_BRACKET; \
        return true; \
    }

PROGRAM_LIST_FUNC( AttribBinding, AttribBindingList, BindAttribLocation )
PROGRAM_LIST_FUNC( FragDataBinding, FragDataBindingList, BindFragDataLocation )

#define PROGRAM_PARAMETER_FUNC( PROP, NAME ) \
    static bool read##PROP(InputStream& is, InputUserData& ud) { \
        int value; is >> is.PROPERTY(#NAME) >> value; \
        ud.add("setParameter", NAME, value); \
        return true; \
    }

PROGRAM_PARAMETER_FUNC( GeometryVerticesOut, GL_GEOMETRY_VERTICES_OUT_EXT )
PROGRAM_PARAMETER_FUNC( GeometryInputType, GL_GEOMETRY_INPUT_TYPE_EXT )
PROGRAM_PARAMETER_FUNC( GeometryOutputType, GL_GEOMETRY_OUTPUT_TYPE_EXT )

// _shaderList
static bool readShaders(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        osg::ref_ptr<osg::Shader> shader = is.readObjectOfType<osg::Shader>();
        if ( shader ) ud.add("addShader", shader);
    }
    is >> is.END_BRACKET;
    return true;
}

// feedBackVaryings
static bool readFeedBackVaryingsName(InputStream& is, InputUserData& ud)
{
	unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
	for ( unsigned int i=0; i<size; ++i )
	{
		std::string str; is>> str;
        ud.add("addTransformFeedBackVarying", str);
	}
	is >> is.END_BRACKET;
	return true;
}

// feedBack mode
static bool readFeedBackMode(InputStream& is, InputUserData& ud)
{
	unsigned int size = 0; is>>size;
    ud.add("setTransformFeedBackMode", size);
	return true;
}

// _numGroupsX/Y/Z
static bool readComputeGroups(InputStream& is, InputUserData& ud)
{
    GLint numX = 0, numY = 0, numZ = 0;
    is >> numX >> numY >> numZ;
    return true;
}

static bool readBindUniformBlock(InputStream& is, InputUserData& ud)
{
    unsigned int  size = 0; is >> size >> is.BEGIN_BRACKET;
    std::string name; unsigned int index;
    for ( unsigned int i=0; i<size; ++i )
    {
        is >>name; is >>index;    
        ud.add("addBindUniformBlock", name, index);
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( Program,
                         new osg::Program,
                         osg::Program,
                         "osg::Object osg::StateAttribute osg::Program" )
{
    ADD_USER_SERIALIZER( AttribBinding );  // _attribBindingList
    ADD_USER_SERIALIZER( FragDataBinding );  // _fragDataBindingList
    ADD_USER_SERIALIZER( Shaders );  // _shaderList
    ADD_USER_SERIALIZER( GeometryVerticesOut );  // _geometryVerticesOut
    ADD_USER_SERIALIZER( GeometryInputType );  // _geometryInputType
    ADD_USER_SERIALIZER( GeometryOutputType );  // _geometryOutputType

    {
        UPDATE_TO_VERSION_SCOPED( 95 )
        ADD_USER_SERIALIZER( ComputeGroups );  // _numGroupsX/Y/Z
    }

    {
        UPDATE_TO_VERSION_SCOPED( 153 )
        REMOVE_SERIALIZER( ComputeGroups );
    }

    {
        UPDATE_TO_VERSION_SCOPED( 116 )
        ADD_USER_SERIALIZER( FeedBackVaryingsName );
        ADD_USER_SERIALIZER( FeedBackMode );
    }
    {
        UPDATE_TO_VERSION_SCOPED( 150 )
        ADD_USER_SERIALIZER( BindUniformBlock );
    }
}
