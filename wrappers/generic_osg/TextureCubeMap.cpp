#include <GenericReserializer.h>
using namespace osgVerse;

enum osg_TextureCubeMap_Face {
    POSITIVE_X = 0, NEGATIVE_X = 1,
    POSITIVE_Y = 2, NEGATIVE_Y = 3,
    POSITIVE_Z = 4, NEGATIVE_Z = 5
};

#define FACE_IMAGE_FUNCTION( PROP, FACE ) \
    static bool read##PROP( InputStream& is, InputUserData& ud ) { \
        bool hasImage; is >> hasImage; \
        if ( hasImage ) { \
            is >> is.BEGIN_BRACKET; ud.add( "setImage" #FACE, is.readImage()); \
            is >> is.END_BRACKET; \
        } \
        return true; \
    }

FACE_IMAGE_FUNCTION( PosX, osg_TextureCubeMap_Face::POSITIVE_X )
FACE_IMAGE_FUNCTION( NegX, osg_TextureCubeMap_Face::NEGATIVE_X )
FACE_IMAGE_FUNCTION( PosY, osg_TextureCubeMap_Face::POSITIVE_Y )
FACE_IMAGE_FUNCTION( NegY, osg_TextureCubeMap_Face::NEGATIVE_Y )
FACE_IMAGE_FUNCTION( PosZ, osg_TextureCubeMap_Face::POSITIVE_Z )
FACE_IMAGE_FUNCTION( NegZ, osg_TextureCubeMap_Face::NEGATIVE_Z )

REGISTER_OBJECT_WRAPPER( TextureCubeMap,
                         new osg::TextureCubeMap,
                         osg::TextureCubeMap,
                         "osg::Object osg::StateAttribute osg::Texture osg::TextureCubeMap" )
{
    ADD_USER_SERIALIZER( PosX );
    ADD_USER_SERIALIZER( NegX );
    ADD_USER_SERIALIZER( PosY );
    ADD_USER_SERIALIZER( NegY );
    ADD_USER_SERIALIZER( PosZ );
    ADD_USER_SERIALIZER( NegZ );  // _images

    ADD_INT_SERIALIZER( TextureWidth, 0 );  // _textureWidth
    ADD_INT_SERIALIZER( TextureHeight, 0 );  // _textureHeight
}
