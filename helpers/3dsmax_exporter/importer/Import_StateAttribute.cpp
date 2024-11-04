/* -*-c++-*- osg2max - Copyright (C) 2010 by Wang Rui <wangray84 at gmail dot com>
* The StateAttribute and mode import implementation
*/

#include <osg/BlendFunc>
#include <osg/Material>
#include <osg/PolygonMode>
#include "ImportImplementor.h"

void ImportImplementor::importRenderingMode( MtlBase* maxMtl,
                                             osg::StateAttribute::GLMode mode,
                                             osg::StateAttribute::GLModeValue value )
{
}

void ImportImplementor::importRenderingAttribute( MtlBase* maxMtl,
                                                  osg::StateAttribute::TypeMemberPair type,
                                                  osg::StateSet::RefAttributePair attr )
{
    StdMat* stdMtl = dynamic_cast<StdMat*>( maxMtl );
    if ( !stdMtl ) return;
    
    osg::StateAttribute* sa = attr.first.get();
    switch ( type.first )
    {
    case osg::StateAttribute::BLENDFUNC:
        // TODO
        stdMtl->SetTransparencyType(TRANSP_FILTER);
        break;
    case osg::StateAttribute::MATERIAL:
        {
            osg::Material* mat = static_cast<osg::Material*>(sa);
            stdMtl->SetAmbient( Color(convertPoint4(mat->getAmbient(osg::Material::FRONT))), 0 );
            stdMtl->SetDiffuse( Color(convertPoint4(mat->getDiffuse(osg::Material::FRONT))), 0 );
            stdMtl->SetSpecular( Color(convertPoint4(mat->getSpecular(osg::Material::FRONT))), 0 );
            stdMtl->SetFilter( Color(convertPoint4(mat->getEmission(osg::Material::FRONT))), 0 );
            stdMtl->SetShininess( mat->getShininess(osg::Material::FRONT), 0 );
            
            float alpha = mat->getDiffuse(osg::Material::FRONT).a();
            stdMtl->SetOpacity( alpha, 0 );
        }
        break;
    case osg::StateAttribute::POLYGONMODE:
        {
            osg::PolygonMode* pm = static_cast<osg::PolygonMode*>(sa);
            switch ( pm->getMode(osg::PolygonMode::FRONT_AND_BACK) )
            {
            case osg::PolygonMode::LINE: stdMtl->SetWire(TRUE); break;
            default: stdMtl->SetWire(FALSE); break;
            }
        }
        break;
    default:
        //NotifyBox( GetString(IDS_NOTIMPLEMENTED), L"importRenderingAttribute", s2ws(attr.first->className()).c_str() );
        return;
    }
}
