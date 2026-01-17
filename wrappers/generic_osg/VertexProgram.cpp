#include <GenericReserializer.h>
using namespace osgVerse;

// _programLocalParameters
static bool readLocalParameters(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        unsigned int key; osg::Vec4d value;
        is >> key >> value;
        ud.add("setProgramLocalParameter", key, value);
    }
    is >> is.END_BRACKET;
    return true;
}

// _matrixList
static bool readMatrices(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        unsigned int key; osg::Matrixd value;
        is >> key >> value;
        ud.add("setMatrix", key, value);
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( VertexProgram,
                         new osg::VertexProgram,
                         osg::VertexProgram,
                         "osg::Object osg::StateAttribute osg::VertexProgram" )
{
    ADD_STRING_SERIALIZER( VertexProgram, "" );  // _fragmentProgram
    ADD_USER_SERIALIZER( LocalParameters );  // _programLocalParameters
    ADD_USER_SERIALIZER( Matrices );  // _matrixList
}
