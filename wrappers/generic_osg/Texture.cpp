#include <GenericReserializer.h>
#include <GL/gl.h>
using namespace osgVerse;

enum osg_Texture_WrapParameter { WRAP_S, WRAP_T, WRAP_R };
enum osg_Texture_FilterParameter { MIN_FILTER, MAG_FILTER };

#define WRAP_FUNCTIONS( PROP, VALUE ) \
    static bool read##PROP(InputStream& is, InputUserData& ud) { \
        DEF_GLENUM(mode); is >> mode; \
        ud.add( "setWrap", VALUE, mode.get() ); \
        return true; \
    }

WRAP_FUNCTIONS( WRAP_S, osg_Texture_WrapParameter::WRAP_S )
WRAP_FUNCTIONS( WRAP_T, osg_Texture_WrapParameter::WRAP_T )
WRAP_FUNCTIONS( WRAP_R, osg_Texture_WrapParameter::WRAP_R )

#define FILTER_FUNCTIONS( PROP, VALUE ) \
    static bool read##PROP(InputStream& is, InputUserData& ud) { \
        DEF_GLENUM(mode); is >> mode; \
        ud.add( "setFilter", VALUE, mode.get() ); \
        return true; \
    }

FILTER_FUNCTIONS( MIN_FILTER, osg_Texture_FilterParameter::MIN_FILTER )
FILTER_FUNCTIONS( MAG_FILTER, osg_Texture_FilterParameter::MAG_FILTER )

#define GL_FORMAT_FUNCTIONS( PROP ) \
    static bool read##PROP( InputStream& is, InputUserData& ud ) { \
        DEF_GLENUM(mode); is >> mode; ud.add( "set" #PROP, mode.get() ); \
        return true; \
    }

GL_FORMAT_FUNCTIONS( SourceFormat )
GL_FORMAT_FUNCTIONS( SourceType )

static bool readInternalFormat(InputStream& is, InputUserData& ud)
{
    DEF_GLENUM(mode); is >> mode;
    ud.add("setInternalFormat", mode.get());
    return true;
}

// _imageAttachment
struct DummyImageAttachment
{
DummyImageAttachment(): unit(0), level(0), layered(GL_FALSE), layer(0), access(0), format(0){}
    GLuint unit;
    GLint level;
    GLboolean layered;
    GLint layer;
    GLenum access;
    GLenum format;
};

static bool readImageAttachment(InputStream& is, InputUserData& ud)
{
    DummyImageAttachment attachment;
    is >> attachment.unit >> attachment.level >> attachment.layered
       >> attachment.layer >> attachment.access >> attachment.format;
    return true;
}

// _swizzle
static unsigned char swizzleToCharacter(GLint swizzle, unsigned char defaultCharacter)
{
    switch (swizzle)
    {
    case GL_RED:
        return 'R';
    case GL_GREEN:
        return 'G';
    case GL_BLUE:
        return 'B';
    case GL_ALPHA:
        return 'A';
    case GL_ZERO:
        return '0';
    case GL_ONE:
        return '1';
    default:
        break;
    }

    return defaultCharacter;
}

static GLint characterToSwizzle(unsigned char character, GLint defaultSwizzle)
{
    switch (character)
    {
    case 'R':
        return GL_RED;
    case 'G':
        return GL_GREEN;
    case 'B':
        return GL_BLUE;
    case 'A':
        return GL_ALPHA;
    case '0':
        return GL_ZERO;
    case '1':
        return GL_ONE;
    default:
        break;
    }

    return defaultSwizzle;
}

static std::string swizzleToString(const osg::Vec4i& swizzle)
{
    std::string result;

    result.push_back(swizzleToCharacter(swizzle.r(), 'R'));
    result.push_back(swizzleToCharacter(swizzle.g(), 'G'));
    result.push_back(swizzleToCharacter(swizzle.b(), 'B'));
    result.push_back(swizzleToCharacter(swizzle.a(), 'A'));

    return result;
}

static osg::Vec4i stringToSwizzle(const std::string& swizzleString)
{
    osg::Vec4i swizzle;

    swizzle.r() = characterToSwizzle(swizzleString[0], GL_RED);
    swizzle.g() = characterToSwizzle(swizzleString[1], GL_GREEN);
    swizzle.b() = characterToSwizzle(swizzleString[2], GL_BLUE);
    swizzle.a() = characterToSwizzle(swizzleString[3], GL_ALPHA);

    return swizzle;
}

static bool readSwizzle(InputStream& is, InputUserData& ud)
{
    std::string swizzleString;
    is >> swizzleString;
    ud.add("setSwizzle", stringToSwizzle(swizzleString));

    return true;
}

REGISTER_OBJECT_WRAPPER( Texture,
                         /*new osg::Texture*/NULL,
                         osg::Texture,
                         "osg::Object osg::StateAttribute osg::Texture" )
{
    ADD_USER_SERIALIZER( WRAP_S );  // _wrap_s
    ADD_USER_SERIALIZER( WRAP_T );  // _wrap_t
    ADD_USER_SERIALIZER( WRAP_R );  // _wrap_r
    ADD_USER_SERIALIZER( MIN_FILTER );  // _min_filter
    ADD_USER_SERIALIZER( MAG_FILTER );  // _mag_filter
    ADD_FLOAT_SERIALIZER( MaxAnisotropy, 1.0f );  // _maxAnisotropy
    ADD_BOOL_SERIALIZER( UseHardwareMipMapGeneration, true );  // _useHardwareMipMapGeneration
    ADD_BOOL_SERIALIZER( UnRefImageDataAfterApply, false );  // _unrefImageDataAfterApply
    ADD_BOOL_SERIALIZER( ClientStorageHint, false );  // _clientStorageHint
    ADD_BOOL_SERIALIZER( ResizeNonPowerOfTwoHint, true );  // _resizeNonPowerOfTwoHint
    ADD_VEC4D_SERIALIZER( BorderColor, osg::Vec4d(0.0,0.0,0.0,0.0) );  // _borderColor
    ADD_GLINT_SERIALIZER( BorderWidth, 0 );  // _borderWidth

    BEGIN_ENUM_SERIALIZER( InternalFormatMode, USE_IMAGE_DATA_FORMAT );
        ADD_ENUM_VALUE( USE_IMAGE_DATA_FORMAT );
        ADD_ENUM_VALUE( USE_USER_DEFINED_FORMAT );
        ADD_ENUM_VALUE( USE_ARB_COMPRESSION );
        ADD_ENUM_VALUE( USE_S3TC_DXT1_COMPRESSION );
        ADD_ENUM_VALUE( USE_S3TC_DXT3_COMPRESSION );
        ADD_ENUM_VALUE( USE_S3TC_DXT5_COMPRESSION );
        ADD_ENUM_VALUE( USE_PVRTC_2BPP_COMPRESSION );
        ADD_ENUM_VALUE( USE_PVRTC_4BPP_COMPRESSION );
        ADD_ENUM_VALUE( USE_ETC_COMPRESSION );
        ADD_ENUM_VALUE( USE_RGTC1_COMPRESSION );
        ADD_ENUM_VALUE( USE_RGTC2_COMPRESSION );
        ADD_ENUM_VALUE( USE_S3TC_DXT1c_COMPRESSION );
        ADD_ENUM_VALUE( USE_S3TC_DXT1a_COMPRESSION );
    END_ENUM_SERIALIZER();  // _internalFormatMode

    ADD_USER_SERIALIZER( InternalFormat );  // _internalFormat
    ADD_USER_SERIALIZER( SourceFormat );  // _sourceFormat
    ADD_USER_SERIALIZER( SourceType );  // _sourceType
    ADD_BOOL_SERIALIZER( ShadowComparison, false );  // _use_shadow_comparison

    BEGIN_ENUM_SERIALIZER( ShadowCompareFunc, LEQUAL );
        ADD_ENUM_VALUE( NEVER );
        ADD_ENUM_VALUE( LESS );
        ADD_ENUM_VALUE( EQUAL );
        ADD_ENUM_VALUE( LEQUAL );
        ADD_ENUM_VALUE( GREATER );
        ADD_ENUM_VALUE( NOTEQUAL );
        ADD_ENUM_VALUE( GEQUAL );
        ADD_ENUM_VALUE( ALWAYS );
    END_ENUM_SERIALIZER();  // _shadow_compare_func

    BEGIN_ENUM_SERIALIZER( ShadowTextureMode, LUMINANCE );
        ADD_ENUM_VALUE( LUMINANCE );
        ADD_ENUM_VALUE( INTENSITY );
        ADD_ENUM_VALUE( ALPHA );
        ADD_ENUM_VALUE( NONE );
    END_ENUM_SERIALIZER();  // _shadow_texture_mode

    ADD_FLOAT_SERIALIZER( ShadowAmbient, 0.0f );  // _shadow_ambient

    {
        UPDATE_TO_VERSION_SCOPED( 95 )
        ADD_USER_SERIALIZER( ImageAttachment );  // _imageAttachment
    }
    {
        UPDATE_TO_VERSION_SCOPED( 154 )
        REMOVE_SERIALIZER( ImageAttachment );
    }
    {
        UPDATE_TO_VERSION_SCOPED( 98 )
        ADD_USER_SERIALIZER( Swizzle );  // _swizzle
    }
    {
        UPDATE_TO_VERSION_SCOPED( 155 )
        ADD_FLOAT_SERIALIZER( MinLOD, 0.0f );
        ADD_FLOAT_SERIALIZER( MaxLOD, -1.0f );
        ADD_FLOAT_SERIALIZER( LODBias, 0.0f );
    }
}
