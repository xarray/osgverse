/* -*-c++-*- osg2max - Copyright (C) 2010 by Wang Rui <wangray84 at gmail dot com>
* The texture-related StateAttribute export implementation
*/

#include <osg/BlendFunc>
#include <osg/Material>
#include <osg/Texture2D>
#include <osg/TexEnv>
#include <osgDB/ReadFile>
#include <osgDB/ConvertUTF>
#include "ExportImplementor.h"
#include <stdmat.h>

void ExportImplementor::createStateSet( osg::StateSet* ss, Mtl* mtl, bool hasMaterial, bool hasTexture, int envMode )
{
    if ( !mtl ) return;
    else if ( mtl->ClassID()==Class_ID(DMTL_CLASS_ID, 0) )  // Standard material
    {
        if ( hasMaterial ) createSurfaceMaterial( ss, mtl );
        if ( hasTexture ) createTexture2D( ss, mtl, ID_DI, envMode );  // Diffuse map
    }
    else if ( mtl->ClassID()==Class_ID(MULTI_CLASS_ID, 0) )  // Multi-Object material
    {
        for ( int i=0; i<mtl->NumSubMtls(); ++i )
        {
            Mtl* subMtl = mtl->GetSubMtl(i);
            exportMaterial( subMtl, BAKE_NONE );  // No need to check if sub-material are still shelled
        }
    }
}

void ExportImplementor::createSurfaceMaterial( osg::StateSet* ss, Mtl* mtl )
{
    StdMat* stdMat = dynamic_cast<StdMat*>(mtl);
    if ( !stdMat ) return;
    
    float alpha = stdMat->GetOpacity(0);
    float shinstr = stdMat->GetShinStr(0);
    osg::Vec4 diffuse = convertColor(stdMat->GetDiffuse(0), alpha);
    osg::Vec4 blackColor(0.0f, 0.0f, 0.0f, alpha);
    
    // Basic properties
    osg::ref_ptr<osg::Material> material = new osg::Material;
    material->setColorMode( osg::Material::OFF );
    material->setDiffuse( osg::Material::FRONT_AND_BACK, diffuse );
    material->setAmbient( osg::Material::FRONT_AND_BACK, convertColor(stdMat->GetAmbient(0), alpha) );
    material->setSpecular( osg::Material::FRONT_AND_BACK, convertColor(stdMat->GetSpecular(0), alpha) );
    if ( stdMat->GetSelfIllumColorOn() )
        material->setEmission( osg::Material::FRONT_AND_BACK, convertColor(stdMat->GetSelfIllumColor(0), alpha) );
    else
        material->setEmission( osg::Material::FRONT_AND_BACK, blackColor );
    if ( shinstr>0.0f )
    {
        material->setSpecular( osg::Material::FRONT_AND_BACK,
                               blackColor + (convertColor(stdMat->GetSpecular(0), 0.0f) * (shinstr / 9.99f)) );
        material->setShininess( osg::Material::FRONT_AND_BACK, stdMat->GetShininess(0) * 128.0f );
    }
    
    // Add blend operation if necessary
    if ( alpha<1.0f )
    {
        osg::ref_ptr<osg::BlendFunc> blendFunc = new osg::BlendFunc;
        blendFunc->setFunction( osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA );
        
        ss->setAttributeAndModes( blendFunc.get() );
        ss->setRenderingHint( osg::StateSet::TRANSPARENT_BIN );
    }
    ss->setAttributeAndModes( material.get() );
}

void ExportImplementor::createTexture2D( osg::StateSet* ss, Mtl* mtl, int type, int envMode )
{
    Texmap* texMap = mtl->GetSubTexmap(type);
    if ( !(mtl->SubTexmapOn(type) && texMap) ) return;
    
    unsigned int unit = texMap->GetTheUVGen()->GetMapChannel() - 1;
    osg::ref_ptr<osg::Texture> texture = getTexture(texMap);
    if ( !texture )
    {
        if ( texMap->ClassID()==Class_ID(BMTEX_CLASS_ID, 0) )
        {
            // Bitmap texture
            BitmapTex* bitmap = dynamic_cast<BitmapTex*>(texMap);
            if ( !bitmap ) return;
            
            osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
            tex2D->setWrap( osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE );
            tex2D->setWrap( osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE );
            
            if ( bitmap->GetMapName() )
            {
                // Load image file from disk directly
                osg::Image* image = osgDB::readImageFile( ws2s(bitmap->GetMapName()) );
                tex2D->setImage( image );
            }
            texture = tex2D;
        }
        
        if ( texture.valid() )
            _textureMap[texMap] = texture.get();
    }
    ss->setTextureAttributeAndModes( unit, texture.get() );
    
    // Texture environment mode
    if ( envMode )
    {
        osg::ref_ptr<osg::TexEnv> texenv = new osg::TexEnv( (osg::TexEnv::Mode)envMode );
        ss->setTextureAttributeAndModes( unit, texenv.get() );
    }
}

osg::StateSet* ExportImplementor::exportMaterial( Mtl* maxMtl, BakeElementType bakeType )
{
    // FIXME: Exporting shell material can only work for LightMap->DiffuseMap now
    Mtl* currentMtl = maxMtl;
    if ( bakeType!=BAKE_NONE ) currentMtl = maxMtl->GetSubMtl(1);
    if ( !currentMtl ) return NULL;
    
    osg::ref_ptr<osg::StateSet> ss = getStateSet(currentMtl);
    if ( ss.valid() ) return ss.get();
    else ss = new osg::StateSet;
    
    int envMode = 0;
    if ( bakeType==BAKE_LIGHTING_MAP )
    {
        // Export origin image for lighting-map baking
        Mtl* originMtl = maxMtl->GetSubMtl(0);
        createStateSet( ss.get(), originMtl, false, true, envMode );
        envMode = GL_MODULATE;
    }
    
    createStateSet( ss.get(), currentMtl, true, true, envMode );
    _materialMap[currentMtl] = ss.get();
    return ss.get();
}
