#include <osg/io_utils>
#include <osg/Version>
#include <osg/AnimationPath>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osgDB/ConvertUTF>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include "pipeline/Utilities.h"

#include <mutex>
#include <ktx/texture.h>
#include <ktx/gl_format.h>
#include "LoadTextureKTX.h"

static inline VkFormat glGetVkFormatFromInternalFormat(GLint glFormat)
{
    switch (glFormat)
    {
    //
    // 8 bits per component
    //
    case GL_R8: return VK_FORMAT_R8_UNORM;                // 1-component, 8-bit unsigned normalized
    case GL_RG8: return VK_FORMAT_R8G8_UNORM;               // 2-component, 8-bit unsigned normalized
    case GL_RGB8: return VK_FORMAT_R8G8B8_UNORM;              // 3-component, 8-bit unsigned normalized
    case GL_RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;             // 4-component, 8-bit unsigned normalized

    case GL_ALPHA: return VK_FORMAT_R8_UNORM;                 // 1-component, 8-bit unsigned normalized
    case GL_LUMINANCE: return VK_FORMAT_R8_UNORM;             // 1-component, 8-bit unsigned normalized
    case GL_LUMINANCE_ALPHA: return VK_FORMAT_R8G8_UNORM;     // 2-component, 8-bit unsigned normalized
    case GL_RGB: return VK_FORMAT_R8G8B8_UNORM;               // 3-component, 8-bit unsigned normalized
    case GL_RGBA: return VK_FORMAT_R8G8B8A8_UNORM;            // 4-component, 8-bit unsigned normalized

    case GL_R8_SNORM: return VK_FORMAT_R8_SNORM;          // 1-component, 8-bit signed normalized
    case GL_RG8_SNORM: return VK_FORMAT_R8G8_SNORM;         // 2-component, 8-bit signed normalized
    case GL_RGB8_SNORM: return VK_FORMAT_R8G8B8_SNORM;        // 3-component, 8-bit signed normalized
    case GL_RGBA8_SNORM: return VK_FORMAT_R8G8B8A8_SNORM;       // 4-component, 8-bit signed normalized

    case GL_R8UI: return VK_FORMAT_R8_UINT;              // 1-component, 8-bit unsigned integer
    case GL_RG8UI: return VK_FORMAT_R8G8_UINT;             // 2-component, 8-bit unsigned integer
    case GL_RGB8UI: return VK_FORMAT_R8G8B8_UINT;            // 3-component, 8-bit unsigned integer
    case GL_RGBA8UI: return VK_FORMAT_R8G8B8A8_UINT;           // 4-component, 8-bit unsigned integer

    case GL_R8I: return VK_FORMAT_R8_SINT;               // 1-component, 8-bit signed integer
    case GL_RG8I: return VK_FORMAT_R8G8_SINT;              // 2-component, 8-bit signed integer
    case GL_RGB8I: return VK_FORMAT_R8G8B8_SINT;             // 3-component, 8-bit signed integer
    case GL_RGBA8I: return VK_FORMAT_R8G8B8A8_SINT;            // 4-component, 8-bit signed integer

    case GL_SR8: return VK_FORMAT_R8_SRGB;               // 1-component, 8-bit sRGB
    case GL_SRG8: return VK_FORMAT_R8G8_SRGB;              // 2-component, 8-bit sRGB
    case GL_SRGB8: return VK_FORMAT_R8G8B8_SRGB;             // 3-component, 8-bit sRGB
    case GL_SRGB8_ALPHA8: return VK_FORMAT_R8G8B8A8_SRGB;      // 4-component, 8-bit sRGB

    //
    // 16 bits per component
    //
    case GL_R16: return VK_FORMAT_R16_UNORM;               // 1-component, 16-bit unsigned normalized
    case GL_RG16: return VK_FORMAT_R16G16_UNORM;              // 2-component, 16-bit unsigned normalized
    case GL_RGB16: return VK_FORMAT_R16G16B16_UNORM;             // 3-component, 16-bit unsigned normalized
    case GL_RGBA16: return VK_FORMAT_R16G16B16A16_UNORM;            // 4-component, 16-bit unsigned normalized

    case GL_R16_SNORM: return VK_FORMAT_R16_SNORM;         // 1-component, 16-bit signed normalized
    case GL_RG16_SNORM: return VK_FORMAT_R16G16_SNORM;        // 2-component, 16-bit signed normalized
    case GL_RGB16_SNORM: return VK_FORMAT_R16G16B16_SNORM;       // 3-component, 16-bit signed normalized
    case GL_RGBA16_SNORM: return VK_FORMAT_R16G16B16A16_SNORM;      // 4-component, 16-bit signed normalized

    case GL_R16UI: return VK_FORMAT_R16_UINT;             // 1-component, 16-bit unsigned integer
    case GL_RG16UI: return VK_FORMAT_R16G16_UINT;            // 2-component, 16-bit unsigned integer
    case GL_RGB16UI: return VK_FORMAT_R16G16B16_UINT;           // 3-component, 16-bit unsigned integer
    case GL_RGBA16UI: return VK_FORMAT_R16G16B16A16_UINT;          // 4-component, 16-bit unsigned integer

    case GL_R16I: return VK_FORMAT_R16_SINT;              // 1-component, 16-bit signed integer
    case GL_RG16I: return VK_FORMAT_R16G16_SINT;             // 2-component, 16-bit signed integer
    case GL_RGB16I: return VK_FORMAT_R16G16B16_SINT;            // 3-component, 16-bit signed integer
    case GL_RGBA16I: return VK_FORMAT_R16G16B16A16_SINT;           // 4-component, 16-bit signed integer

    case GL_R16F: return VK_FORMAT_R16_SFLOAT;              // 1-component, 16-bit floating-point
    case GL_RG16F: return VK_FORMAT_R16G16_SFLOAT;             // 2-component, 16-bit floating-point
    case GL_RGB16F: return VK_FORMAT_R16G16B16_SFLOAT;            // 3-component, 16-bit floating-point
    case GL_RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;           // 4-component, 16-bit floating-point

    //
    // 32 bits per component
    //
    case GL_R32UI: return VK_FORMAT_R32_UINT;             // 1-component, 32-bit unsigned integer
    case GL_RG32UI: return VK_FORMAT_R32G32_UINT;            // 2-component, 32-bit unsigned integer
    case GL_RGB32UI: return VK_FORMAT_R32G32B32_UINT;           // 3-component, 32-bit unsigned integer
    case GL_RGBA32UI: return VK_FORMAT_R32G32B32A32_UINT;          // 4-component, 32-bit unsigned integer

    case GL_R32I: return VK_FORMAT_R32_SINT;              // 1-component, 32-bit signed integer
    case GL_RG32I: return VK_FORMAT_R32G32_SINT;             // 2-component, 32-bit signed integer
    case GL_RGB32I: return VK_FORMAT_R32G32B32_SINT;            // 3-component, 32-bit signed integer
    case GL_RGBA32I: return VK_FORMAT_R32G32B32A32_SINT;           // 4-component, 32-bit signed integer

    case GL_R32F: return VK_FORMAT_R32_SFLOAT;              // 1-component, 32-bit floating-point
    case GL_RG32F: return VK_FORMAT_R32G32_SFLOAT;             // 2-component, 32-bit floating-point
    case GL_RGB32F: return VK_FORMAT_R32G32B32_SFLOAT;            // 3-component, 32-bit floating-point
    case GL_RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;           // 4-component, 32-bit floating-point

    //
    // Packed
    //
    case GL_RGB5: return VK_FORMAT_R5G5B5A1_UNORM_PACK16;              // 3-component 5:5:5,       unsigned normalized
    case GL_RGB565: return VK_FORMAT_R5G6B5_UNORM_PACK16;            // 3-component 5:6:5,       unsigned normalized
    case GL_RGBA4: return VK_FORMAT_R4G4B4A4_UNORM_PACK16;             // 4-component 4:4:4:4,     unsigned normalized
    case GL_RGB5_A1: return VK_FORMAT_A1R5G5B5_UNORM_PACK16;           // 4-component 5:5:5:1,     unsigned normalized
    case GL_RGB10_A2: return VK_FORMAT_A2R10G10B10_UNORM_PACK32;          // 4-component 10:10:10:2,  unsigned normalized
    case GL_RGB10_A2UI: return VK_FORMAT_A2R10G10B10_UINT_PACK32;        // 4-component 10:10:10:2,  unsigned integer
    case GL_R11F_G11F_B10F: return VK_FORMAT_B10G11R11_UFLOAT_PACK32;    // 3-component 11:11:10,    floating-point
    case GL_RGB9_E5: return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;           // 3-component/exp 9:9:9/5, floating-point

    //
    // S3TC/DXT/BC
    //
    case GL_COMPRESSED_RGB_S3TC_DXT1_EXT: return VK_FORMAT_BC1_RGB_UNORM_BLOCK;                  // line through 3D space, 4x4 blocks, unsigned normalized
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;                 // line through 3D space plus 1-bit alpha, 4x4 blocks, unsigned normalized
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT: return VK_FORMAT_BC2_UNORM_BLOCK;                 // line through 3D space plus line through 1D space, 4x4 blocks, unsigned normalized
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: return VK_FORMAT_BC3_UNORM_BLOCK;                 // line through 3D space plus 4-bit alpha, 4x4 blocks, unsigned normalized

    case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT: return VK_FORMAT_BC1_RGB_SRGB_BLOCK;                 // line through 3D space, 4x4 blocks, sRGB
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT: return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;           // line through 3D space plus 1-bit alpha, 4x4 blocks, sRGB
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT: return VK_FORMAT_BC2_SRGB_BLOCK;           // line through 3D space plus line through 1D space, 4x4 blocks, sRGB
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT: return VK_FORMAT_BC3_SRGB_BLOCK;           // line through 3D space plus 4-bit alpha, 4x4 blocks, sRGB

    case GL_COMPRESSED_RED_RGTC1: return VK_FORMAT_BC4_UNORM_BLOCK;                          // line through 1D space, 4x4 blocks, unsigned normalized
    case GL_COMPRESSED_RG_RGTC2: return VK_FORMAT_BC5_UNORM_BLOCK;                           // two lines through 1D space, 4x4 blocks, unsigned normalized
    case GL_COMPRESSED_SIGNED_RED_RGTC1: return VK_FORMAT_BC4_SNORM_BLOCK;                   // line through 1D space, 4x4 blocks, signed normalized
    case GL_COMPRESSED_SIGNED_RG_RGTC2: return VK_FORMAT_BC5_SNORM_BLOCK;                    // two lines through 1D space, 4x4 blocks, signed normalized

    case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT: return VK_FORMAT_BC6H_UFLOAT_BLOCK;            // 3-component, 4x4 blocks, unsigned floating-point
    case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT: return VK_FORMAT_BC6H_SFLOAT_BLOCK;              // 3-component, 4x4 blocks, signed floating-point
    case GL_COMPRESSED_RGBA_BPTC_UNORM: return VK_FORMAT_BC7_UNORM_BLOCK;                    // 4-component, 4x4 blocks, unsigned normalized
    case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM: return VK_FORMAT_BC7_SRGB_BLOCK;              // 4-component, 4x4 blocks, sRGB

    //
    // ETC
    //
    case GL_COMPRESSED_RGB8_ETC2: return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;                          // 3-component ETC2, 4x4 blocks, unsigned normalized
    case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2: return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;      // 4-component ETC2 with 1-bit alpha, 4x4 blocks, unsigned normalized
    case GL_COMPRESSED_RGBA8_ETC2_EAC: return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;                     // 4-component ETC2, 4x4 blocks, unsigned normalized

    case GL_COMPRESSED_SRGB8_ETC2: return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;                         // 3-component ETC2, 4x4 blocks, sRGB
    case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2: return VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;     // 4-component ETC2 with 1-bit alpha, 4x4 blocks, sRGB
    case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC: return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;              // 4-component ETC2, 4x4 blocks, sRGB

    case GL_COMPRESSED_R11_EAC: return VK_FORMAT_EAC_R11_UNORM_BLOCK;                            // 1-component ETC, 4x4 blocks, unsigned normalized
    case GL_COMPRESSED_RG11_EAC: return VK_FORMAT_EAC_R11G11_UNORM_BLOCK;                           // 2-component ETC, 4x4 blocks, unsigned normalized
    case GL_COMPRESSED_SIGNED_R11_EAC: return VK_FORMAT_EAC_R11_SNORM_BLOCK;                     // 1-component ETC, 4x4 blocks, signed normalized
    case GL_COMPRESSED_SIGNED_RG11_EAC: return VK_FORMAT_EAC_R11G11_SNORM_BLOCK;                    // 2-component ETC, 4x4 blocks, signed normalized

    //
    // PVRTC
    //
    case GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG: return VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG;           // 3- or 4-component PVRTC, 16x8 blocks, unsigned normalized
    case GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG: return VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG;           // 3- or 4-component PVRTC,  8x8 blocks, unsigned normalized
    case GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG: return VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG;           // 3- or 4-component PVRTC, 16x8 blocks, unsigned normalized
    case GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG: return VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG;           // 3- or 4-component PVRTC,  4x4 blocks, unsigned normalized

    case GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT: return VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG;     // 4-component PVRTC, 16x8 blocks, sRGB
    case GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT: return VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG;     // 4-component PVRTC,  8x8 blocks, sRGB
    case GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV2_IMG: return VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG;     // 4-component PVRTC,  8x4 blocks, sRGB
    case GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV2_IMG: return VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG;     // 4-component PVRTC,  4x4 blocks, sRGB

    //
    // ASTC
    //
    case GL_COMPRESSED_RGBA_ASTC_4x4_KHR: return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;                // 4-component ASTC, 4x4 blocks, unsigned normalized
    case GL_COMPRESSED_RGBA_ASTC_5x4_KHR: return VK_FORMAT_ASTC_5x4_UNORM_BLOCK;                // 4-component ASTC, 5x4 blocks, unsigned normalized
    case GL_COMPRESSED_RGBA_ASTC_5x5_KHR: return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;                // 4-component ASTC, 5x5 blocks, unsigned normalized
    case GL_COMPRESSED_RGBA_ASTC_6x5_KHR: return VK_FORMAT_ASTC_6x5_UNORM_BLOCK;                // 4-component ASTC, 6x5 blocks, unsigned normalized
    case GL_COMPRESSED_RGBA_ASTC_6x6_KHR: return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;                // 4-component ASTC, 6x6 blocks, unsigned normalized
    case GL_COMPRESSED_RGBA_ASTC_8x5_KHR: return VK_FORMAT_ASTC_8x5_UNORM_BLOCK;                // 4-component ASTC, 8x5 blocks, unsigned normalized
    case GL_COMPRESSED_RGBA_ASTC_8x6_KHR: return VK_FORMAT_ASTC_8x6_UNORM_BLOCK;                // 4-component ASTC, 8x6 blocks, unsigned normalized
    case GL_COMPRESSED_RGBA_ASTC_8x8_KHR: return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;                // 4-component ASTC, 8x8 blocks, unsigned normalized
    case GL_COMPRESSED_RGBA_ASTC_10x5_KHR: return VK_FORMAT_ASTC_10x5_UNORM_BLOCK;               // 4-component ASTC, 10x5 blocks, unsigned normalized
    case GL_COMPRESSED_RGBA_ASTC_10x6_KHR: return VK_FORMAT_ASTC_10x6_UNORM_BLOCK;               // 4-component ASTC, 10x6 blocks, unsigned normalized
    case GL_COMPRESSED_RGBA_ASTC_10x8_KHR: return VK_FORMAT_ASTC_10x8_UNORM_BLOCK;               // 4-component ASTC, 10x8 blocks, unsigned normalized
    case GL_COMPRESSED_RGBA_ASTC_10x10_KHR: return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;              // 4-component ASTC, 10x10 blocks, unsigned normalized
    case GL_COMPRESSED_RGBA_ASTC_12x10_KHR: return VK_FORMAT_ASTC_12x10_UNORM_BLOCK;              // 4-component ASTC, 12x10 blocks, unsigned normalized
    case GL_COMPRESSED_RGBA_ASTC_12x12_KHR: return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;              // 4-component ASTC, 12x12 blocks, unsigned normalized

    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR: return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;        // 4-component ASTC, 4x4 blocks, sRGB
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR: return VK_FORMAT_ASTC_5x4_SRGB_BLOCK;        // 4-component ASTC, 5x4 blocks, sRGB
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR: return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;        // 4-component ASTC, 5x5 blocks, sRGB
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR: return VK_FORMAT_ASTC_6x5_SRGB_BLOCK;        // 4-component ASTC, 6x5 blocks, sRGB
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR: return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;        // 4-component ASTC, 6x6 blocks, sRGB
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR: return VK_FORMAT_ASTC_8x5_SRGB_BLOCK;        // 4-component ASTC, 8x5 blocks, sRGB
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR: return VK_FORMAT_ASTC_8x6_SRGB_BLOCK;        // 4-component ASTC, 8x6 blocks, sRGB
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR: return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;        // 4-component ASTC, 8x8 blocks, sRGB
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR: return VK_FORMAT_ASTC_10x5_SRGB_BLOCK;       // 4-component ASTC, 10x5 blocks, sRGB
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR: return VK_FORMAT_ASTC_10x6_SRGB_BLOCK;       // 4-component ASTC, 10x6 blocks, sRGB
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR: return VK_FORMAT_ASTC_10x8_SRGB_BLOCK;       // 4-component ASTC, 10x8 blocks, sRGB
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR: return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;      // 4-component ASTC, 10x10 blocks, sRGB
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR: return VK_FORMAT_ASTC_12x10_SRGB_BLOCK;      // 4-component ASTC, 12x10 blocks, sRGB
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR: return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;      // 4-component ASTC, 12x12 blocks, sRGB

    //
    // Depth/stencil
    //
    case GL_DEPTH_COMPONENT16: return VK_FORMAT_D16_UNORM;
    case GL_DEPTH_COMPONENT24: return VK_FORMAT_X8_D24_UNORM_PACK32;
    case GL_DEPTH_COMPONENT32F: return VK_FORMAT_D32_SFLOAT;
    case GL_STENCIL_INDEX8: return VK_FORMAT_S8_UINT;
    case GL_DEPTH24_STENCIL8: return VK_FORMAT_D24_UNORM_S8_UINT;
    case GL_DEPTH32F_STENCIL8: return VK_FORMAT_D32_SFLOAT_S8_UINT;
    default: return VK_FORMAT_UNDEFINED;
    }
}

static std::map<osgVerse::ReadingKtxFlag, int> g_readKtxFlags;
static std::mutex g_readKtxMutex;

namespace osgVerse
{
    static osg::ref_ptr<osg::Image> loadImageFromKtx(ktxTexture* texture, const osgDB::Options* opt,
                                                     int layer, int face, ktx_size_t imgDataSize)
    {
        bool transcoded = false, compressed = false;
        ktx_error_code_e result = KTX_SUCCESS;
        ktx_uint32_t w = texture->baseWidth, h = texture->baseHeight, d = texture->baseDepth;
        ktx_uint32_t w2 = osg::Image::computeNearestPowerOfTwo(w), h2 = osg::Image::computeNearestPowerOfTwo(h);
        if (ktxTexture_NeedsTranscoding(texture))
        {
            bool noCompress = false, supportsDXT = false, supportsETC = false;
            g_readKtxMutex.lock();
            noCompress = (g_readKtxFlags[ReadKtx_ToRGBA] > 0);
            supportsDXT = (g_readKtxFlags[ReadKtx_NoDXT] == 0);
            g_readKtxMutex.unlock();

            if (opt != NULL)
            {
                if (!opt->getPluginStringData("UseDXT").empty())
                    supportsDXT = atoi(opt->getPluginStringData("UseDXT").c_str()) > 0;
                if (!opt->getPluginStringData("UseETC").empty())
                    supportsETC = atoi(opt->getPluginStringData("UseETC").c_str()) > 0;
                if (!opt->getPluginStringData("UseRGBA").empty())
                    noCompress = atoi(opt->getPluginStringData("UseRGBA").c_str()) > 0;
            }

            ktx_transcode_fmt_e fmt = ktx_transcode_fmt_e::KTX_TTF_RGBA32;
            if (w2 != w || h2 != h)
            {
                if (!noCompress)
                {
                    OSG_NOTICE << "[LoaderKTX] Found NPOT image: " << w << " x " << h
                               << ", will not transcode to compressing texture format" << std::endl;
                }
            }
            else if (!noCompress)
            {
                compressed = true;
                if (supportsETC) fmt = ktx_transcode_fmt_e::KTX_TTF_ETC;
                else if (supportsDXT) fmt = ktx_transcode_fmt_e::KTX_TTF_BC1_OR_3;
                else compressed = false;
            }

            result = ktxTexture2_TranscodeBasis((ktxTexture2*)texture, fmt, 0);
            transcoded = (result == KTX_SUCCESS);
        }

        ktx_size_t offset = 0; ktxTexture_GetImageOffset(texture, 0, layer, face, &offset);
        ktx_uint8_t* imgData = ktxTexture_GetData(texture) + offset;

        osg::ref_ptr<osg::Image> image = new osg::Image;
        if (texture->classId == ktxTexture2_c)
        {
            ktxTexture2* tex = (ktxTexture2*)texture;
            GLint glInternalformat = glGetInternalFormatFromVkFormat((VkFormat)tex->vkFormat);
            switch (glInternalformat)  // FIXME: compressed SRGB DXT not working directly...
            {
            case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT: case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
                glInternalformat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT; compressed = true; break;
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT: case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
                glInternalformat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; compressed = true; break;
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT: case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
                glInternalformat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; compressed = true; break;
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT: case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
                glInternalformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; compressed = true; break;
            }

            GLenum glFormat = compressed ? glInternalformat  /* Use compressed format */
                            : glGetFormatFromInternalFormat(glInternalformat);
            GLenum glType = glGetTypeFromInternalFormat(glInternalformat);
            if (glFormat == GL_INVALID_VALUE || glType == GL_INVALID_VALUE)
            {
                OSG_WARN << "[LoaderKTX] Failed to get KTX2 file format: VkFormat = "
                         << std::hex << tex->vkFormat << std::dec << std::endl;
                return NULL;
            }
            else if (transcoded)
            {   // FIXME: KTX1 transcoding?
                OSG_INFO << "[LoaderKTX] Transcoded format: internalFmt = " << std::hex
                         << glInternalformat << ", pixelFmt = " << glFormat << ", type = "
                         << glType << std::dec << std::endl;
                imgDataSize = ktxTexture_GetImageSize(texture, 0);
            }

            image->allocateImage(w, h, d, glFormat, glType, 4);
            image->setInternalTextureFormat(glInternalformat);
            if (image->getTotalSizeInBytes() < imgDataSize)
            {
                OSG_WARN << "[LoaderKTX] Failed to copy image data, size mismatched: "
                         << imgDataSize << " != " << image->getTotalSizeInBytes() << std::endl;
                OSG_WARN << "[LoaderKTX] Source format: internalFmt = " << std::hex
                         << glInternalformat << ", pixelFmt = " << glFormat << ", type = "
                         << glType << std::dec << ", res = " << w << "x" << h << std::endl;
            }
        }
        else
        {
            ktxTexture1* tex = (ktxTexture1*)texture;
            image->allocateImage(w, h, d, tex->glFormat, tex->glType, 4);
            image->setInternalTextureFormat(tex->glInternalformat);
        }
        memcpy(image->data(), imgData, imgDataSize);
        return image;
    }

    static std::vector<osg::ref_ptr<osg::Image>> loadKtxFromObject(
            ktxTexture* texture, const osgDB::Options* opt)
    {
        std::vector<osg::ref_ptr<osg::Image>> resultArray;
        ktx_uint32_t numLevels = texture->numLevels; ktx_size_t imgSize = 0;
        if (texture->classId == ktxTexture2_c)
            imgSize = ktxTexture_calcImageSize(texture, 0, KTX_FORMAT_VERSION_TWO);
        else
            imgSize = ktxTexture_calcImageSize(texture, 0, KTX_FORMAT_VERSION_ONE);

        if (texture->isArray || texture->numFaces > 1)
        {
            if (texture->isArray)
            {
                for (ktx_uint32_t i = 0; i < texture->numLayers; ++i)
                {
                    osg::ref_ptr<osg::Image> img = loadImageFromKtx(texture, opt, i, 0, imgSize);
                    if (img.valid()) resultArray.push_back(img);
                }
            }
            else
            {
                for (ktx_uint32_t i = 0; i < texture->numFaces; ++i)
                {
                    osg::ref_ptr<osg::Image> img = loadImageFromKtx(texture, opt, 0, i, imgSize);
                    if (img.valid()) resultArray.push_back(img);
                }
            }
        }
        else
        {
            osg::ref_ptr<osg::Image> image = loadImageFromKtx(texture, opt, 0, 0, imgSize);
            if (image.valid()) resultArray.push_back(image);
        }
        ktxTexture_Destroy(texture);
        return resultArray;
    }

    std::vector<osg::ref_ptr<osg::Image>> loadKtx(const std::string& file, const osgDB::Options* opt)
    {
        ktxTexture* texture = NULL;
        ktx_error_code_e result = ktxTexture_CreateFromNamedFile(
            file.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);
        if (result != KTX_SUCCESS)
        {
            OSG_WARN << "[LoaderKTX] Unable to read from KTX file: " << file << "\n";
            return std::vector<osg::ref_ptr<osg::Image>>();
        }
        return loadKtxFromObject(texture, opt);
    }

    std::vector<osg::ref_ptr<osg::Image>> loadKtx2(std::istream& in, const osgDB::Options* opt)
    {
        std::string data((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
        if (data.empty()) return std::vector<osg::ref_ptr<osg::Image>>();

        ktxTexture* texture = NULL;
        ktx_error_code_e result = ktxTexture_CreateFromMemory(
            (const ktx_uint8_t*)data.data(), data.size(),
            KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);
        if (result != KTX_SUCCESS)
        {
            OSG_WARN << "[LoaderKTX] Unable to read from KTX stream\n";
            return std::vector<osg::ref_ptr<osg::Image>>();
        }
        return loadKtxFromObject(texture, opt);
    }

    static ktxTexture* saveImageToKtx(const std::vector<osg::Image*>& images, bool asCubeMap,
                                      bool useBASISU, bool useUASTC = false, int basisuThreadCount = 1,
                                      int basisuCompressLv = 2, int basisuQualityLv = 128)
    {
        ktxTexture* texture = NULL;
        if (images.empty()) return NULL;
        if (asCubeMap && images.size() < 6) return NULL;

        osg::Image* image0 = images[0];
        ktxTextureCreateInfo createInfo;

        createInfo.glInternalformat = image0->getInternalTextureFormat();
        createInfo.vkFormat = glGetVkFormatFromInternalFormat(createInfo.glInternalformat);
        createInfo.baseWidth = image0->s();
        createInfo.baseHeight = image0->t();
        createInfo.baseDepth = image0->r();
        createInfo.numDimensions = (image0->r() > 1) ? 3 : ((image0->t() > 1) ? 2 : 1);
        createInfo.numLevels = 1;  // FIXME: always omit mipmaps?
        createInfo.numLayers = asCubeMap ? 1 : images.size();
        createInfo.numFaces = asCubeMap ? images.size() : 1;
        createInfo.isArray = (images.size() > 1) ? KTX_TRUE : KTX_FALSE;
        createInfo.generateMipmaps = KTX_FALSE;
        if (createInfo.vkFormat == 0)
        {
            OSG_WARN << "[LoaderKTX] No VkFormat for GL internal format: "
                     << std::hex << createInfo.glInternalformat << std::dec << std::endl;
            return NULL;
        }

        KTX_error_code result = ktxTexture2_Create(
            &createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, (ktxTexture2**)&texture);
        if (result != KTX_SUCCESS)
        {
            OSG_WARN << "[LoaderKTX] Unable to create KTX for saving" << std::endl;
            return NULL;
        }

        for (size_t i = 0; i < images.size(); ++i)
        {
            osg::Image* img = images[i];
            if (img->s() != image0->s() || img->t() != image0->t() ||
                img->getInternalTextureFormat() != image0->getInternalTextureFormat())
            {
                OSG_WARN << "[LoaderKTX] Mismatched image format while saving "
                         << "KTX texture" << std::endl; continue;
            }

            ktx_uint8_t* src = (ktx_uint8_t*)img->data();
            result = ktxTexture_SetImageFromMemory(
                texture, 0, (asCubeMap ? 0 : i), (asCubeMap ? i : 0),
                src, img->getTotalSizeInBytes());
            if (result != KTX_SUCCESS)
            {
                OSG_WARN << "[LoaderKTX] Unable to save image " << i
                         << " to KTX texture: " << ktxErrorString(result) << std::endl;
                ktxTexture_Destroy(texture); return NULL;
            }
        }

        if (useBASISU)
        {
            ktxBasisParams params = { 0 };
            params.structSize = sizeof(params);
            params.uastc = useUASTC ? KTX_TRUE : KTX_FALSE;
            params.compressionLevel = basisuCompressLv;
            params.qualityLevel = basisuQualityLv;
            params.threadCount = basisuThreadCount;
            result = ktxTexture2_CompressBasisEx((ktxTexture2*)texture, &params);
            if (result != KTX_SUCCESS)
            {
                OSG_WARN << "[LoaderKTX] Failed to compress ktxTexture2: "
                         << ktxErrorString(result) << std::endl;
            }
        }
        return texture;
    }

    static ktxTexture* saveImageToKtx(const std::vector<osg::Image*>& images,
                                      bool asCubeMap, const osgDB::Options* opt)
    {
        if (opt != NULL)
        {
            std::string useBASISU = opt->getPluginStringData("UseBASISU");
            std::string useUASTC = opt->getPluginStringData("UseUASTC");
            std::string threadCount = opt->getPluginStringData("ThreadCount");
            std::string compressLv = opt->getPluginStringData("CompressLevel");
            std::string qualityLv = opt->getPluginStringData("QualityLevel");
            return saveImageToKtx(images, asCubeMap, atoi(useBASISU.c_str()) > 0,
                                  atoi(useUASTC.c_str()) > 0, atoi(threadCount.c_str()),
                                  atoi(compressLv.c_str()), atoi(qualityLv.c_str()));
        }
        else
            return saveImageToKtx(images, asCubeMap, false, false);
    }

    bool saveKtx(const std::string& file, bool asCubeMap, const osgDB::Options* opt,
                 const std::vector<osg::Image*>& images)
    {
        ktxTexture* texture = saveImageToKtx(images, asCubeMap, opt);
        if (texture == NULL) return false;

        KTX_error_code result = ktxTexture_WriteToNamedFile(texture, file.c_str());
        ktxTexture_Destroy(texture);
        return result == KTX_SUCCESS;
    }

    bool saveKtx2(std::ostream& out, bool asCubeMap, const osgDB::Options* opt,
                  const std::vector<osg::Image*>& images)
    {
        ktxTexture* texture = saveImageToKtx(images, asCubeMap, opt);
        if (texture == NULL) return false;

        ktx_uint8_t* buffer = NULL; ktx_size_t length = 0;
        KTX_error_code result = ktxTexture_WriteToMemory(texture, &buffer, &length);
        if (result == KTX_SUCCESS)
        {
            out.write((char*)buffer, length);
            delete buffer;
        }
        ktxTexture_Destroy(texture);
        return result == KTX_SUCCESS;
    }

    void setReadingKtxFlag(ReadingKtxFlag flag, int value)
    { g_readKtxMutex.lock(); g_readKtxFlags[flag] = value; g_readKtxMutex.unlock(); }
}
