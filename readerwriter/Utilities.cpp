#include <osg/Geode>
#include <osg/Texture2D>
#include <osgDB/Registry>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include "LoadTextureKTX.h"

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

void FixedFunctionOptimizer::apply(osg::Geode& geode)
{
    for (unsigned int i = 0; i < geode.getNumDrawables(); ++i)
    {
        osg::Geometry* geom = dynamic_cast<osg::Geometry*>(geode.getDrawable(i));
        if (geom)
        {
            removeUnusedStateAttributes(geom->getStateSet());
            geom->setUseDisplayList(false);
            geom->setUseVertexBufferObjects(true);
        }
    }
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

#if defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
    // Remove texture modes as they are not needed by glEnable() in GLES 2.0/3.x
    // https://docs.gl/es2/glEnable
    const osg::StateSet::TextureModeList& texModes = ss.getTextureModeList();
    for (size_t i = 0; i < texModes.size(); ++i)
    {
        osg::StateSet::ModeList modes = texModes[i];
        for (osg::StateSet::ModeList::const_iterator itr = modes.begin();
            itr != modes.end(); ++itr) ss.removeTextureMode(i, itr->first);
    }
#endif
}

TextureOptimizer::TextureOptimizer(bool inlineFile, const std::string& newTexFolder)
:   osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
{
    if (inlineFile) osgDB::makeDirectory(newTexFolder);
    _textureFolder = newTexFolder;
    _preparingForInlineFile = inlineFile;
}

TextureOptimizer::~TextureOptimizer()
{
}

void TextureOptimizer::apply(osg::Geode& geode)
{
    for (unsigned int i = 0; i < geode.getNumDrawables(); ++i)
        applyTextureAttributes(geode.getDrawable(i)->getStateSet());
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

    switch (img->getInternalTextureFormat())
    {
    case GL_LUMINANCE: case 1: img->setInternalTextureFormat(GL_R8); break;
    case GL_LUMINANCE_ALPHA: case 2: img->setInternalTextureFormat(GL_RG8); break;
    case GL_RGB: case 3: img->setInternalTextureFormat(GL_RGB8); break;
    case GL_RGBA: case 4: img->setInternalTextureFormat(GL_RGBA8); break;
    default: break;
    }

    std::vector<osg::Image*> images; images.push_back(img);
    if (!saveKtx2(ss, false, images)) return NULL;
    else OSG_NOTICE << "[TextureOptimizer] Compressed: " << img->getFileName()
                    << " (" << img->s() << " x " << img->t() << ")" << std::endl;

    if (!toLoad)
    {
        std::string fileName = img->getFileName(), id = "__" + nanoid::generate(8);
        if (fileName.empty()) fileName = "temp" + id + ".ktx.verse_ktx";
        else fileName = osgDB::getStrippedName(fileName) + id + ".ktx.verse_ktx";
        img->setFileName(_textureFolder + osgDB::getNativePathSeparator() + fileName);

        std::ofstream out(img->getFileName().c_str(), std::ios::out | std::ios::binary);
        out.write(ss.str().data(), ss.str().size()); return NULL;
    }
    else
    {
        std::vector<osg::ref_ptr<osg::Image>> outImages = loadKtx2(ss);
        return outImages.empty() ? NULL : outImages[0].release();
    }
}
