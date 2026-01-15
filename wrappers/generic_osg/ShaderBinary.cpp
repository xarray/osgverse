#include <GenericReserializer.h>
using namespace osgVerse;

static bool readData(InputStream& is, InputUserData& ud)
{
    unsigned int size; is >> size;
    char* data = new char[size]();
    if ( is.isBinary() )
    {
        is.readCharArray( data, size );
    }
    else
    {
        is >> is.BEGIN_BRACKET;
        for ( unsigned int i=0; i<size; ++i )
        {
            is >> std::hex >> data[i] >> std::dec;
        }
        is >> is.END_BRACKET;
    }

    if (size>0)
    {
        ud.add("assign", size, (unsigned char*)data);
    }

    delete [] data;
    return true;
}

REGISTER_OBJECT_WRAPPER( ShaderBinary,
                         new osg::ShaderBinary,
                         osg::ShaderBinary,
                         "osg::Object osg::ShaderBinary" )
{
    ADD_USER_SERIALIZER( Data );  // _data
}
