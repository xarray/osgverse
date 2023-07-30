#include <osg/Geode>
#include <osg/Texture2D>
#include <osgDB/Registry>
#include "LoadTextureKTX.h"
#include "Utilities.h"
using namespace osgVerse;

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
        osg::ref_ptr<osg::Image> image1 = compressImage(tex, image0.get());
        if (!image1 || (image1.valid() && !image1->valid())) return;
        image0->allocateImage(image1->s(), image1->t(), image1->r(),
                              image1->getPixelFormat(), image1->getDataType(),
                              image1->getPacking());
        image0->setInternalTextureFormat(image1->getInternalTextureFormat());
        memcpy(image0->data(), image1->data(), image1->getTotalSizeInBytes());
    }
}

osg::Image* TextureOptimizer::compressImage(osg::Texture* tex, osg::Image* img)
{
    std::stringstream ss; if (!img->valid()) return NULL;
    if (img->isCompressed()) return NULL;

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

    std::vector<osg::ref_ptr<osg::Image>> outImages = loadKtx2(ss);
    return outImages.empty() ? NULL : outImages[0].release();
}
