#include <osg/Version>
#include <osg/TriangleIndexFunctor>
#include <osg/Geode>
#include <osg/Texture2D>
#include <osg/Multisample>
#include <osg/Material>
#include <osg/PolygonOffset>
#include <osgDB/Registry>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <ghc/filesystem.hpp>
#include <nanoid/nanoid.h>
#include <libhv/all/base64.h>

#include "modeling/Utilities.h"
#include "LoadTextureKTX.h"
#include "Utilities.h"
using namespace osgVerse;

class ModeChecker : public osg::Referenced
{
public:
    static ModeChecker* instance()
    {
        static osg::ref_ptr<ModeChecker> s_instance = new ModeChecker;
        return s_instance.get();
    }

    bool check(osg::StateSet& ss)
    {
        if (modeSet.empty()) return true;
        osg::StateSet::ModeList modeList = ss.getModeList();
        for (osg::StateSet::ModeList::iterator it = modeList.begin(); it != modeList.end(); ++it)
        { if (!check(it->first)) ss.removeMode(it->first); } return true;
    }

    bool check(GLenum mode)
    {
        if (modeSet.empty()) return true;
        return modeSet.find(mode) != modeSet.end();
    }

protected:
    ModeChecker()
    {
#if defined(OSG_GL3_AVAILABLE)
        // https://docs.gl/gl4/glEnable
        modeSet.insert(GL_BLEND);
        //modeSet.insert(GL_CLIP_DISTANCE);
        modeSet.insert(GL_COLOR_LOGIC_OP);
        modeSet.insert(GL_CULL_FACE);
        modeSet.insert(GL_DEBUG_OUTPUT);
        modeSet.insert(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        modeSet.insert(GL_DEPTH_CLAMP);
        modeSet.insert(GL_DEPTH_TEST);
        modeSet.insert(GL_DITHER);
        modeSet.insert(GL_FRAMEBUFFER_SRGB);
        modeSet.insert(GL_LINE_SMOOTH);
        modeSet.insert(GL_MULTISAMPLE_ARB);
        modeSet.insert(GL_POLYGON_OFFSET_FILL);
        modeSet.insert(GL_POLYGON_OFFSET_LINE);
        modeSet.insert(GL_POLYGON_OFFSET_POINT);
        modeSet.insert(GL_POLYGON_SMOOTH);
        modeSet.insert(GL_PRIMITIVE_RESTART);
        //modeSet.insert(GL_PRIMITIVE_RESTART_FIXED_INDEX);
        modeSet.insert(GL_RASTERIZER_DISCARD);
        modeSet.insert(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB);
        modeSet.insert(GL_SAMPLE_ALPHA_TO_ONE_ARB);
        modeSet.insert(GL_SAMPLE_COVERAGE_ARB);
        //modeSet.insert(GL_SAMPLE_SHADING);
        //modeSet.insert(GL_SAMPLE_MASK);
        modeSet.insert(GL_SCISSOR_TEST);
        modeSet.insert(GL_STENCIL_TEST);
        modeSet.insert(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        modeSet.insert(GL_PROGRAM_POINT_SIZE);
#elif defined(OSG_GLES2_AVAILABLE)
        // https://docs.gl/es2/glEnable
        // https://developer.mozilla.org/en-US/docs/Web/API/WebGLRenderingContext/enable
        modeSet.insert(GL_BLEND);
        modeSet.insert(GL_CULL_FACE);
        modeSet.insert(GL_DEPTH_TEST);
        modeSet.insert(GL_DITHER);
        modeSet.insert(GL_POLYGON_OFFSET_FILL);
        modeSet.insert(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB);
        modeSet.insert(GL_SAMPLE_COVERAGE_ARB);
        modeSet.insert(GL_SCISSOR_TEST);
        modeSet.insert(GL_STENCIL_TEST);
#elif defined(OSG_GLES3_AVAILABLE)
        // https://docs.gl/es3/glEnable
        modeSet.insert(GL_BLEND);
        modeSet.insert(GL_CULL_FACE);
        modeSet.insert(GL_DEPTH_TEST);
        modeSet.insert(GL_DITHER);
        modeSet.insert(GL_POLYGON_OFFSET_FILL);
        modeSet.insert(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB);
        modeSet.insert(GL_SAMPLE_COVERAGE_ARB);
        modeSet.insert(GL_SCISSOR_TEST);
        modeSet.insert(GL_STENCIL_TEST);
        // https://developer.mozilla.org/en-US/docs/Web/API/WebGLRenderingContext/enable
        modeSet.insert(GL_RASTERIZER_DISCARD);
#  if !defined(VERSE_WEBGL2)
        //modeSet.insert(GL_PRIMITIVE_RESTART_FIXED_INDEX);
        //modeSet.insert(GL_SAMPLE_MASK);
#  endif
#endif
    }

    std::set<GLenum> modeSet;
};

#ifdef __EMSCRIPTEN__
// Reference: https://github.com/emscripten-core/emscripten/issues/9574
EM_JS(void, emscripten_sleep_using_raf, (),
{
    Asyncify.handleSleep(wakeUp =>
    { requestAnimationFrame(wakeUp); });
});

void emscripten_advance()
{
#if 1
    emscripten_sleep_using_raf();
#else
    emscripten_sleep(10);
#endif
}
#endif

void FixedFunctionOptimizer::apply(osg::Geometry& geom)
{
    bool added = removeUnusedStateAttributes(geom.getStateSet());
    osg::Vec3Array* va = static_cast<osg::Vec3Array*>(geom.getVertexArray());
    if (va && !va->empty() && !_materialStack.empty())
    {
        // Change material to color array
        osg::Material* mtl = static_cast<osg::Material*>(_materialStack.back().get());
        osg::ref_ptr<osg::Vec4Array> ca = new osg::Vec4Array;
        ca->assign(va->size(), mtl->getDiffuse(osg::Material::FRONT));
        geom.setColorArray(ca.get()); geom.setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    }

    optimizeIndices(geom);
    geom.setUseDisplayList(false);
    geom.setUseVertexBufferObjects(true);
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
    traverse(geom);
#endif
    if (added) _materialStack.pop_back();
}

void FixedFunctionOptimizer::apply(osg::Geode& geode)
{
    bool added = removeUnusedStateAttributes(geode.getStateSet());
#if OSG_VERSION_LESS_OR_EQUAL(3, 4, 1)
    for (unsigned int i = 0; i < geode.getNumDrawables(); ++i)
    {
        osg::Geometry* geom = dynamic_cast<osg::Geometry*>(geode.getDrawable(i));
        if (geom) apply(*geom);
    }
#endif
    NodeVisitor::apply(geode);
    if (added) _materialStack.pop_back();
}

void FixedFunctionOptimizer::apply(osg::Node& node)
{
    bool added = removeUnusedStateAttributes(node.getStateSet());
    NodeVisitor::apply(node);
    if (added) _materialStack.pop_back();
}

bool FixedFunctionOptimizer::removeUnusedStateAttributes(osg::StateSet* ssPtr)
{
    if (ssPtr == NULL) return false;
    osg::StateSet& ss = *ssPtr;

    osg::StateAttribute* sa = ss.getAttribute(osg::StateAttribute::MATERIAL);
    if (sa != NULL) _materialStack.push_back(sa);

    ss.removeAttribute(osg::StateAttribute::ALPHAFUNC);
    ss.removeAttribute(osg::StateAttribute::CLIPPLANE);
    ss.removeAttribute(osg::StateAttribute::COLORMATRIX);
    ss.removeAttribute(osg::StateAttribute::FOG);
    ss.removeAttribute(osg::StateAttribute::LIGHT);
    ss.removeAttribute(osg::StateAttribute::LIGHTMODEL);
    ss.removeAttribute(osg::StateAttribute::LINESTIPPLE);
    ss.removeAttribute(osg::StateAttribute::LOGICOP);
    ss.removeAttribute(osg::StateAttribute::MATERIAL);
    ss.removeAttribute(osg::StateAttribute::POINT);
    ss.removeAttribute(osg::StateAttribute::POLYGONSTIPPLE);
    ss.removeAttribute(osg::StateAttribute::SHADEMODEL);

#if defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE) || defined(OSG_GL3_AVAILABLE)
    osg::StateSet::TextureAttributeList texAttrs = ss.getTextureAttributeList();
    for (size_t i = 0; i < texAttrs.size(); ++i)
    {
        osg::Texture* tex = static_cast<osg::Texture*>(
            ss.getTextureAttribute(0, osg::StateAttribute::TEXTURE));
        if (tex && tex->getNumImages() > 0)
        {
            // Try to fix some old and wrong internal formats
            for (size_t j = 0; j < tex->getNumImages(); ++j)
            {
#  if !defined(VERSE_WEBGL1)
                GLenum internalFmt = tex->getImage(j)->getInternalTextureFormat();
                switch (internalFmt)
                {
                case GL_ALPHA: tex->getImage(j)->setInternalTextureFormat(GL_ALPHA8); break;
                case GL_LUMINANCE: tex->getImage(j)->setInternalTextureFormat(GL_LUMINANCE8); break;
                case GL_RGB: tex->getImage(j)->setInternalTextureFormat(GL_RGB8); break;
                case GL_RGBA: tex->getImage(j)->setInternalTextureFormat(GL_RGBA8); break;
                }
#  endif
            }
        }
        ss.removeTextureAttribute(i, osg::StateAttribute::TEXENV);
        ss.removeTextureAttribute(i, osg::StateAttribute::TEXGEN);
        ss.removeTextureAttribute(i, osg::StateAttribute::TEXMAT);
    }

    // Remove texture modes as they are not needed by glEnable() in GLES 2.0/3.x and GL3/4
    // https://docs.gl/es2/glEnable  // https://docs.gl/gl4/glEnable
    osg::StateSet::TextureModeList texModes = ss.getTextureModeList();
    for (size_t i = 0; i < texModes.size(); ++i)
    {
        osg::StateSet::ModeList modes = texModes[i];
        for (osg::StateSet::ModeList::const_iterator itr = modes.begin();
            itr != modes.end(); ++itr) ss.removeTextureMode(i, itr->first);
    }

    // Remove unused GL modes
    ModeChecker::instance()->check(ss);
#endif  // defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE) || defined(OSG_GL3_AVAILABLE)
    return (sa != NULL);
}

TextureOptimizer::TextureOptimizer(bool inlineFile, const std::string& newTexFolder)
:   osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
{
    if (inlineFile) osgDB::makeDirectory(newTexFolder);
    _textureFolder = newTexFolder;
    _preparingForInlineFile = inlineFile;
    _ktxOptions = new osgDB::Options("UseBASISU=1");
}

TextureOptimizer::~TextureOptimizer()
{
}

void TextureOptimizer::deleteSavedTextures()
{
    try
    {
        for (size_t i = 0; i < _savedTextures.size(); ++i)
        {
            ghc::filesystem::path path = _savedTextures[i];
            ghc::filesystem::remove(path);
        }
        ghc::filesystem::remove(_textureFolder);
    }
    catch (std::runtime_error& err)
    {
        OSG_WARN << "[TextureOptimizer] deleteSavedTextures(): " << err.what() << std::endl;
    }
}

void TextureOptimizer::apply(osg::Drawable& drawable)
{
    applyTextureAttributes(drawable.getStateSet());
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
    traverse(drawable);
#endif
}

void TextureOptimizer::apply(osg::Geode& geode)
{
#if OSG_VERSION_LESS_OR_EQUAL(3, 4, 1)
    for (unsigned int i = 0; i < geode.getNumDrawables(); ++i)
        applyTextureAttributes(geode.getDrawable(i)->getStateSet());
#endif
    applyTextureAttributes(geode.getStateSet());
    NodeVisitor::apply(geode);
}

void TextureOptimizer::apply(osg::Node& node)
{
    applyTextureAttributes(node.getStateSet());
    NodeVisitor::apply(node);
}

void TextureOptimizer::applyTextureAttributes(osg::StateSet* ssPtr)
{
    if (ssPtr == NULL) return;
    osg::StateSet& ss = *ssPtr;

    const osg::StateSet::TextureAttributeList& texAttrs = ss.getTextureAttributeList();
    for (size_t i = 0; i < texAttrs.size(); ++i)
    {
        const osg::StateSet::AttributeList& attrs = texAttrs[i];
        for (osg::StateSet::AttributeList::const_iterator itr = attrs.begin();
             itr != attrs.end(); ++itr)
        {
            if (itr->first.first == osg::StateAttribute::TEXTURE)
                applyTexture(static_cast<osg::Texture*>(itr->second.first.get()), i);
        }
    }
}

void TextureOptimizer::applyTexture(osg::Texture* tex, unsigned int unit)
{
    osg::Texture2D* tex2D = dynamic_cast<osg::Texture2D*>(tex);
    if (tex2D && tex2D->getImage())
    {
        // Copy to original image as it may be shared by other textures
        osg::ref_ptr<osg::Image> image0 = tex2D->getImage();
        osg::ref_ptr<osg::Image> image1 = compressImage(
                tex, image0.get(), !_preparingForInlineFile);
        if (!image1 || (image1.valid() && !image1->valid())) return;
        image0->allocateImage(image1->s(), image1->t(), image1->r(),
                              image1->getPixelFormat(), image1->getDataType(),
                              image1->getPacking());
        image0->setInternalTextureFormat(image1->getInternalTextureFormat());
        memcpy(image0->data(), image1->data(), image1->getTotalSizeInBytes());
    }
}

osg::Image* TextureOptimizer::compressImage(osg::Texture* tex, osg::Image* img, bool toLoad)
{
    std::stringstream ss; if (!img->valid()) return NULL;
    if (img->isCompressed()) return NULL;
    if (img->getFileName().find("verse_ktx") != std::string::npos) return NULL;
    if (img->s() < 4 || img->t() < 4) return NULL;

    int w = osg::Image::computeNearestPowerOfTwo(img->s());
    int h = osg::Image::computeNearestPowerOfTwo(img->t());
    if (w != img->s() || h != img->t()) img->scaleImage(w, h, 1);

    switch (img->getInternalTextureFormat())
    {
    case GL_LUMINANCE: case 1: img->setInternalTextureFormat(GL_R8); break;
    case GL_LUMINANCE_ALPHA: case 2: img->setInternalTextureFormat(GL_RG8); break;
    case GL_RGB: case 3: img->setInternalTextureFormat(GL_RGB8); break;
    case GL_RGBA: case 4: img->setInternalTextureFormat(GL_RGBA8); break;
    default: break;
    }

    std::vector<osg::Image*> images; images.push_back(img);
    if (!saveKtx2(ss, false, _ktxOptions.get(), images)) return NULL;
    else OSG_NOTICE << "[TextureOptimizer] Compressed: " << img->getFileName()
                    << " (" << img->s() << " x " << img->t() << ")" << std::endl;

    if (!toLoad)
    {
        std::string fileName = img->getFileName(), id = "__" + nanoid::generate(8);
        if (fileName.empty()) fileName = "temp" + id + ".ktx";
        else fileName = osgDB::getStrippedName(fileName) + id + ".ktx";
        fileName = _textureFolder + osgDB::getNativePathSeparator() + fileName;
        img->setFileName(fileName + ".verse_ktx");

        std::ofstream out(fileName.c_str(), std::ios::out | std::ios::binary);
        out.write(ss.str().data(), ss.str().size());
        _savedTextures.push_back(fileName); return NULL;
    }
    else
    {
        std::vector<osg::ref_ptr<osg::Image>> outImages = loadKtx2(ss, _ktxOptions.get());
        return outImages.empty() ? NULL : outImages[0].release();
    }
}

namespace osgVerse
{
#if OSG_VERSION_LESS_THAN(3, 5, 0)
    bool updateOsgBinaryWrappers(const std::string& libName) { return false; }
    bool fixOsgBinaryWrappers(const std::string& libName) { return false; }
#endif

    std::string encodeBase64(const std::vector<unsigned char>& buffer)
    { return buffer.empty() ? "" : hv::Base64Encode(&buffer[0], buffer.size()); }

    std::vector<unsigned char> decodeBase64(const std::string& data)
    {
        std::string result = hv::Base64Decode(data.data(), data.size());
        std::vector<unsigned char> out(result.size()); if (result.empty()) return out;
        memcpy(&out[0], result.data(), result.size()); return out;
    }
}
