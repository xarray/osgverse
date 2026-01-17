#include <GenericReserializer.h>
using namespace osgVerse;

static bool readImages(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        ObjectTypeAndID image = ud.readObjectFromStream(is, "osg::Image");
        if ( image.valid() ) ud.add("setImage", i, &image);
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( Texture2DArray,
                         new osg::Texture2DArray,
                         osg::Texture2DArray,
                         "osg::Object osg::StateAttribute osg::Texture osg::Texture2DArray" )
{
    ADD_USER_SERIALIZER( Images );  // _images
    ADD_INT_SERIALIZER( TextureWidth, 0 );  // _textureWidth
    ADD_INT_SERIALIZER( TextureHeight, 0 );  // _textureHeight
    ADD_INT_SERIALIZER( TextureDepth, 0 );  // _textureDepth
}
