#ifndef MANA_PP_IMAGECHECK_HPP
#define MANA_PP_IMAGECHECK_HPP

#include <osg/GL>
#include <osg/GLU>
#include <osg/Texture>
#include <osg/Image>

#if !defined(GL_ARB_ES3_compatibility) || !defined(GL_COMPRESSED_SRGB8_ETC2)
#   define GL_COMPRESSED_RGB8_ETC2                             0x9274
#   define GL_COMPRESSED_SRGB8_ETC2                            0x9275
#   define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2         0x9276
#   define GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2        0x9277
#   define GL_COMPRESSED_RGBA8_ETC2_EAC                        0x9278
#   define GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC                 0x9279
#   define GL_COMPRESSED_R11_EAC                               0x9270
#   define GL_COMPRESSED_SIGNED_R11_EAC                        0x9271
#   define GL_COMPRESSED_RG11_EAC                              0x9272
#   define GL_COMPRESSED_SIGNED_RG11_EAC                       0x9273
#endif

#if !defined(GL_KHR_texture_compression_astc_hdr) || !defined(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR)
#   define GL_KHR_texture_compression_astc_hdr 1
#   define GL_COMPRESSED_RGBA_ASTC_4x4_KHR   0x93B0
#   define GL_COMPRESSED_RGBA_ASTC_5x4_KHR   0x93B1
#   define GL_COMPRESSED_RGBA_ASTC_5x5_KHR   0x93B2
#   define GL_COMPRESSED_RGBA_ASTC_6x5_KHR   0x93B3
#   define GL_COMPRESSED_RGBA_ASTC_6x6_KHR   0x93B4
#   define GL_COMPRESSED_RGBA_ASTC_8x5_KHR   0x93B5
#   define GL_COMPRESSED_RGBA_ASTC_8x6_KHR   0x93B6
#   define GL_COMPRESSED_RGBA_ASTC_8x8_KHR   0x93B7
#   define GL_COMPRESSED_RGBA_ASTC_10x5_KHR  0x93B8
#   define GL_COMPRESSED_RGBA_ASTC_10x6_KHR  0x93B9
#   define GL_COMPRESSED_RGBA_ASTC_10x8_KHR  0x93BA
#   define GL_COMPRESSED_RGBA_ASTC_10x10_KHR 0x93BB
#   define GL_COMPRESSED_RGBA_ASTC_12x10_KHR 0x93BC
#   define GL_COMPRESSED_RGBA_ASTC_12x12_KHR 0x93BD
#   define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR 0x93D0
#   define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR 0x93D1
#   define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR 0x93D2
#   define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR 0x93D3
#   define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR 0x93D4
#   define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR 0x93D5
#   define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR 0x93D6
#   define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR 0x93D7
#   define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR 0x93D8
#   define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR 0x93D9
#   define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR 0x93DA
#   define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR 0x93DB
#   define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR 0x93DC
#   define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR 0x93DD
#endif

#if !defined(GL_EXT_texture_compression_s3tc) || !defined(GL_COMPRESSED_SRGB_S3TC_DXT1_EXT)
#   define GL_COMPRESSED_RGB_S3TC_DXT1_EXT         0x83F0
#   define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT        0x83F1
#   define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT        0x83F2
#   define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT        0x83F3
#   define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT        0x8C4C
#   define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT  0x8C4D
#   define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT  0x8C4E
#   define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT  0x8C4F
#endif

namespace osgVerse
{
    struct ImageHelper
    {
        static bool hasAlpha(osg::Image& image)
        {
            switch (image.getPixelFormat())
            {
            case(GL_COMPRESSED_RGB_S3TC_DXT1_EXT): return false;
            case(GL_COMPRESSED_SRGB_S3TC_DXT1_EXT): return false;
            case(GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT): return true;
            case(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT): return true;
            case(GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT): return true;
            case(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT): return true;
            case(GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT): return true;
            case(GL_COMPRESSED_SIGNED_RED_RGTC1_EXT): return false;
            case(GL_COMPRESSED_RED_RGTC1_EXT):   return false;
            case(GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT): return false;
            case(GL_COMPRESSED_RED_GREEN_RGTC2_EXT): return false;
            case(GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG): return false;
            case(GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG): return false;
            case(GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG): return true;
            case(GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG): return true;
            case(GL_ETC1_RGB8_OES): return false;
            case(GL_COMPRESSED_RGB8_ETC2): return false;
            case(GL_COMPRESSED_SRGB8_ETC2): return false;
            case(GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2): return true;
            case(GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2): return true;
            case(GL_COMPRESSED_RGBA8_ETC2_EAC): return true;
            case(GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC): return true;
            case(GL_COMPRESSED_R11_EAC): return false;
            case(GL_COMPRESSED_SIGNED_R11_EAC): return false;
            case(GL_COMPRESSED_RG11_EAC): return false;
            case(GL_COMPRESSED_SIGNED_RG11_EAC): return false;
            case(GL_COLOR_INDEX): return false;
            case(GL_STENCIL_INDEX): return false;
            case(GL_DEPTH_COMPONENT): return false;
            case(GL_DEPTH_COMPONENT16): return false;
            case(GL_DEPTH_COMPONENT24): return false;
            case(GL_DEPTH_COMPONENT32): return false;
            case(GL_DEPTH_COMPONENT32F): return false;
            case(GL_DEPTH_COMPONENT32F_NV): return false;
            case(GL_RED): return false;
            case(GL_GREEN): return false;
            case(GL_BLUE): return false;
            case(GL_ALPHA): return true;
            case(GL_ALPHA8I_EXT): return true;
            case(GL_ALPHA8UI_EXT): return true;
            case(GL_ALPHA16I_EXT): return true;
            case(GL_ALPHA16UI_EXT): return true;
            case(GL_ALPHA32I_EXT): return true;
            case(GL_ALPHA32UI_EXT): return true;
            case(GL_ALPHA16F_ARB): return true;
            case(GL_ALPHA32F_ARB): return true;
            case(GL_R16F): return false;
            case(GL_R32F): return false;
            case(GL_R8): return false;
            //case(GL_R8_SNORM): return false;
            case(GL_R16): return false;
            //case(GL_R16_SNORM): return false;
            //case(GL_R8I): return false;
            //case(GL_R8UI): return false;
            //case(GL_R16I): return false;
            //case(GL_R16UI): return false;
            //case(GL_R32I): return false;
            //case(GL_R32UI): return false;
            case(GL_RG): return false;
            case(GL_RG16F): return false;
            case(GL_RG32F): return false;
            case(GL_RG8): return false;
            //case(GL_RG8_SNORM): return false;
            case(GL_RG16): return false;
            //case(GL_RG16_SNORM): return false;
            //case(GL_RG8I): return false;
            //case(GL_RG8UI): return false;
            //case(GL_RG16I): return false;
            //case(GL_RG16UI): return false;
            //case(GL_RG32I): return false;
            //case(GL_RG32UI): return false;
            case(GL_RGB): return false;
            case(GL_BGR): return false;
            case(GL_RGB8I_EXT): return false;
            case(GL_RGB8UI_EXT): return false;
            case(GL_RGB16I_EXT): return false;
            case(GL_RGB16UI_EXT): return false;
            case(GL_RGB32I_EXT): return false;
            case(GL_RGB32UI_EXT): return false;
            case(GL_RGB16F_ARB): return false;
            case(GL_RGB32F_ARB): return false;
            case(GL_RGBA16F_ARB): return true;
            case(GL_RGBA32F_ARB): return true;
            case(GL_RGBA): return true;
            case(GL_BGRA): return true;
            case(GL_RGBA8): return true;
            case(GL_LUMINANCE): return false;
            case(GL_LUMINANCE4): return false;
            case(GL_LUMINANCE8): return false;
            case(GL_LUMINANCE12): return false;
            case(GL_LUMINANCE16): return false;
            case(GL_LUMINANCE8I_EXT): return false;
            case(GL_LUMINANCE8UI_EXT): return false;
            case(GL_LUMINANCE16I_EXT): return false;
            case(GL_LUMINANCE16UI_EXT): return false;
            case(GL_LUMINANCE32I_EXT): return false;
            case(GL_LUMINANCE32UI_EXT): return false;
            case(GL_LUMINANCE16F_ARB): return false;
            case(GL_LUMINANCE32F_ARB): return false;
            case(GL_LUMINANCE4_ALPHA4): return true;
            case(GL_LUMINANCE6_ALPHA2): return true;
            case(GL_LUMINANCE8_ALPHA8): return true;
            case(GL_LUMINANCE12_ALPHA4): return true;
            case(GL_LUMINANCE12_ALPHA12): return true;
            case(GL_LUMINANCE16_ALPHA16): return true;
            case(GL_INTENSITY): return false;
            case(GL_INTENSITY4): return false;
            case(GL_INTENSITY8): return false;
            case(GL_INTENSITY12): return false;
            case(GL_INTENSITY16): return false;
            case(GL_INTENSITY8UI_EXT): return false;
            case(GL_INTENSITY8I_EXT): return false;
            case(GL_INTENSITY16I_EXT): return false;
            case(GL_INTENSITY16UI_EXT): return false;
            case(GL_INTENSITY32I_EXT): return false;
            case(GL_INTENSITY32UI_EXT): return false;
            case(GL_INTENSITY16F_ARB): return false;
            case(GL_INTENSITY32F_ARB): return false;
            case(GL_LUMINANCE_ALPHA): return true;
            case(GL_LUMINANCE_ALPHA8I_EXT): return true;
            case(GL_LUMINANCE_ALPHA8UI_EXT): return true;
            case(GL_LUMINANCE_ALPHA16I_EXT): return true;
            case(GL_LUMINANCE_ALPHA16UI_EXT): return true;
            case(GL_LUMINANCE_ALPHA32I_EXT): return true;
            case(GL_LUMINANCE_ALPHA32UI_EXT): return true;
            case(GL_LUMINANCE_ALPHA16F_ARB): return true;
            case(GL_LUMINANCE_ALPHA32F_ARB): return true;
            case(GL_HILO_NV): return false;
            case(GL_DSDT_NV): return false;
            case(GL_DSDT_MAG_NV): return false;
            case(GL_DSDT_MAG_VIB_NV): return false;
            case(GL_RED_INTEGER_EXT): return false;
            case(GL_GREEN_INTEGER_EXT): return false;
            case(GL_BLUE_INTEGER_EXT): return false;
            case(GL_ALPHA_INTEGER_EXT): return true;
            case(GL_RGB_INTEGER_EXT): return false;
            case(GL_RGBA_INTEGER_EXT): return true;
            case(GL_BGR_INTEGER_EXT): return false;
            case(GL_BGRA_INTEGER_EXT): return true;
            case(GL_LUMINANCE_INTEGER_EXT): return false;
            case(GL_LUMINANCE_ALPHA_INTEGER_EXT): return true;
            case(GL_SRGB8): return false;
            case(GL_SRGB8_ALPHA8): return true;
            case (GL_COMPRESSED_RGBA_ASTC_4x4_KHR): return true;
            case (GL_COMPRESSED_RGBA_ASTC_5x4_KHR): return true;
            case (GL_COMPRESSED_RGBA_ASTC_5x5_KHR): return true;
            case (GL_COMPRESSED_RGBA_ASTC_6x5_KHR): return true;
            case (GL_COMPRESSED_RGBA_ASTC_6x6_KHR): return true;
            case (GL_COMPRESSED_RGBA_ASTC_8x5_KHR): return true;
            case (GL_COMPRESSED_RGBA_ASTC_8x6_KHR): return true;
            case (GL_COMPRESSED_RGBA_ASTC_8x8_KHR): return true;
            case (GL_COMPRESSED_RGBA_ASTC_10x5_KHR): return true;
            case (GL_COMPRESSED_RGBA_ASTC_10x6_KHR): return true;
            case (GL_COMPRESSED_RGBA_ASTC_10x8_KHR): return true;
            case (GL_COMPRESSED_RGBA_ASTC_10x10_KHR): return true;
            case (GL_COMPRESSED_RGBA_ASTC_12x10_KHR): return true;
            case (GL_COMPRESSED_RGBA_ASTC_12x12_KHR): return true;
            case (GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR): return true;
            case (GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR): return true;
            case (GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR): return true;
            case (GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR): return true;
            case (GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR): return true;
            case (GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR): return true;
            case (GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR): return true;
            case (GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR): return true;
            case (GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR): return true;
            case (GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR): return true;
            case (GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR): return true;
            case (GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR): return true;
            case (GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR): return true;
            case (GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR): return true;
            default: return false;
            }
        }
    };
}

#endif
