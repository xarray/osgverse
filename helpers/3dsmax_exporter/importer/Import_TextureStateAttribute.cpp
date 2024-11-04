/* -*-c++-*- osg2max - Copyright (C) 2010 by Wang Rui <wangray84 at gmail dot com>
* The texture-related StateAttribute and mode import implementation
*/

#include <osg/Texture2D>
#include "ImportImplementor.h"

BitmapTex* ImportImplementor::createBitmapTexture( osg::Texture2D* tex2D, const std::string& prefix )
{
    BitmapTex* bm = dynamic_cast<BitmapTex*>( getMaxMaterial(tex2D) );
    if ( bm ) return bm;
    
    osg::Image* image = tex2D->getImage();
    if ( !image )
    {
        //NotifyBox( GetString(IDS_NOTFOUND), L"createBitmapTexture", L"Image object" );
        return NULL;
    }
    
    WStr fileName( s2ws(prefix + "/" + image->getFileName()).c_str() );
    if ( !image->data() )
    {
        NotifyBox( GetString(IDS_NOTFOUND), L"createBitmapTexture", fileName.data() );
        return NULL;
    }
    
    bm = NewDefaultBitmapTex();
    bm->SetMapName( fileName.data() );
    bm->ReloadBitmapAndUpdate();
    _materialMap[tex2D] = bm;
    return bm;
}

void ImportImplementor::importTextureMode( MtlBase* maxMtl,
                                           osg::StateAttribute::GLMode mode,
                                           std::vector<UnitModeValuePair>& values )
{
}

void ImportImplementor::importTextureAttribute( MtlBase* maxMtl,
                                                osg::StateAttribute::TypeMemberPair type,
                                                std::vector<UnitAttributePair>& attrs )
{
    StdMat* stdMtl = dynamic_cast<StdMat*>( maxMtl );
    if ( !stdMtl ) return;
    
    Texmap* texMap = NULL;
    switch ( type.first )
    {
    case osg::StateAttribute::TEXTURE:
        // FIXME: Don't know how to control layers in composite texture
        /*if ( attrs.size()>1 )
        {
            MultiTex* comp = NewDefaultCompositeTex();
            comp->SetNumSubTexmaps( attrs.size() );
            for ( unsigned int i=0; i<attrs.size(); ++i )
            {
                osg::StateAttribute* sa = attrs[i].second.first.get();
                
                osg::Texture2D* tex2D = dynamic_cast<osg::Texture2D*>(sa);
                if ( !tex2D )
                    NotifyBox( GetString(IDS_NOTIMPLEMENTED), L"importTextureAttribute", s2ws(sa->className()).c_str() );
                else
                {
                    BitmapTex* bm = createBitmapTexture( tex2D, getPrefix() );
                    bm->GetTheUVGen()->SetMapChannel( i+1 );
                    comp->SetSubTexmap( i, bm );
                }
            }
            texMap = comp;
        }
        else*/
        {
            osg::StateAttribute* sa = attrs[0].second.first.get();
            
            osg::Texture2D* tex2D = dynamic_cast<osg::Texture2D*>(sa);
            if ( !tex2D )
                NotifyBox( GetString(IDS_NOTIMPLEMENTED), L"importTextureAttribute", s2ws(sa->className()).c_str() );
            else
            {
                BitmapTex* bm = createBitmapTexture( tex2D, getPrefix() );
                texMap = bm;
            }
        }
        break;
    default:
        NotifyBox( GetString(IDS_NOTIMPLEMENTED), L"importTextureAttribute", s2ws(attrs[0].second.first->className()).c_str());
        return;
    }
    
    if ( texMap )
    {
        stdMtl->SetSubTexmap( ID_DI, texMap );
        stdMtl->EnableMap( ID_DI, TRUE );
        _gi->ActivateTexture( texMap, stdMtl );
    }
}
