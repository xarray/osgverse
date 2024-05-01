#include <osg/Version>
#include <osg/TriangleIndexFunctor>
#include <osg/Geode>
#include <osg/Texture2D>
#include <osgDB/Registry>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include "LoadTextureKTX.h"

#include <ghc/filesystem.hpp>
#include <nanoid/nanoid.h>
#include "Utilities.h"
using namespace osgVerse;

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

struct ResortVertexOperator
{
    void operator()(unsigned int i1, unsigned int i2, unsigned int i3)
    {
        if (i1 == i2 || i2 == i3 || i1 == i3) return;
        indices.push_back(i1); indices.push_back(i2); indices.push_back(i3);
    }
    std::vector<unsigned int> indices;
};

void FixedFunctionOptimizer::apply(osg::Geometry& geom)
{
#if defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE) || defined(OSG_GL3_AVAILABLE)
    bool invalidMode = false;
    for (size_t i = 0; i < geom.getNumPrimitiveSets(); ++i)
    {
        // glDrawArrays() and glDrawElements() doesn't support GL_QUADS in GLES 2.0/3.x and GL3/4
        // https://docs.gl/gl3/glDrawArrays  // https://docs.gl/gl3/glDrawElements
        GLenum mode = geom.getPrimitiveSet(i)->getMode();
        invalidMode = true;
    }

    if (invalidMode)
    {
        osg::TriangleIndexFunctor<ResortVertexOperator> functor; geom.accept(functor);
        geom.removePrimitiveSet(0, geom.getNumPrimitiveSets());

        osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_TRIANGLES);
        de->assign(functor.indices.begin(), functor.indices.end());
        geom.addPrimitiveSet(de.get());
    }
#endif

    removeUnusedStateAttributes(geom.getStateSet());
    geom.setUseDisplayList(false);
    geom.setUseVertexBufferObjects(true);
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
    traverse(geom);
#endif
}

void FixedFunctionOptimizer::apply(osg::Geode& geode)
{
#if OSG_VERSION_LESS_OR_EQUAL(3, 4, 1)
    for (unsigned int i = 0; i < geode.getNumDrawables(); ++i)
    {
        osg::Geometry* geom = dynamic_cast<osg::Geometry*>(geode.getDrawable(i));
        if (geom) apply(*geom);
    }
#endif
    removeUnusedStateAttributes(geode.getStateSet());
    NodeVisitor::apply(geode);
}

void FixedFunctionOptimizer::apply(osg::Node& node)
{
    removeUnusedStateAttributes(node.getStateSet());
    NodeVisitor::apply(node);
}

void FixedFunctionOptimizer::removeUnusedStateAttributes(osg::StateSet* ssPtr)
{
    if (ssPtr == NULL) return;
    osg::StateSet& ss = *ssPtr;

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
    // Remove texture modes as they are not needed by glEnable() in GLES 2.0/3.x and GL3/4
    // https://docs.gl/es2/glEnable  // https://docs.gl/gl4/glEnable
    const osg::StateSet::TextureModeList& texModes = ss.getTextureModeList();
    for (size_t i = 0; i < texModes.size(); ++i)
    {
        ss.removeTextureAttribute(i, osg::StateAttribute::TEXENV);
        ss.removeTextureAttribute(i, osg::StateAttribute::TEXGEN);
        ss.removeTextureAttribute(i, osg::StateAttribute::TEXMAT);

        osg::StateSet::ModeList modes = texModes[i];
        for (osg::StateSet::ModeList::const_iterator itr = modes.begin();
            itr != modes.end(); ++itr) ss.removeTextureMode(i, itr->first);
    }
    ss.removeMode(GL_NORMALIZE);
#endif
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
    for (size_t i = 0; i < _savedTextures.size(); ++i)
    {
        ghc::filesystem::path path = _savedTextures[i];
        ghc::filesystem::remove(path);
    }
    ghc::filesystem::remove(_textureFolder);
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
    bool fixOsgBinaryWrappers(const std::string& libName) { return false; }
#endif
}
