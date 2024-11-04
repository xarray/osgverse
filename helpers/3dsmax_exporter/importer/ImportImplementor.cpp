/* -*-c++-*- osg2max - Copyright (C) 2010 by Wang Rui <wangray84 at gmail dot com>
* The import implementation
*/

#include <osg/Transform>
#include "ImportImplementor.h"
#include <dummy.h>

ImportImplementor::ImportImplementor( ImpInterface* ii, Interface* gi )
:   _ii(ii), _gi(gi), _hasError(false)
{
}

void ImportImplementor::finalize( bool hideDummyNode )
{
    // Add object reference for unset nodes if any; otherwise 3DSMAX will crash when initializing...
    for ( NodeMap::iterator itr=_nodeMap.begin(); itr!=_nodeMap.end(); ++itr )
    {
        INode* maxNode = itr->second;
        if ( maxNode->GetObjectRef() ) continue;
        
        // Calculate the bounding box of the helper dummy object
        osg::BoundingBox bb = createBoxForDummy(itr->first);
        
        // Apply the dummy to the node
        DummyObject* dummy = new DummyObject;
        dummy->SetBox( Box3(convertPoint(bb._min), convertPoint(bb._max)) );
        if ( hideDummyNode ) dummy->DisableDisplay();
        else dummy->EnableDisplay();
        maxNode->SetObjectRef( dummy );
    }
}

void ImportImplementor::importObjectAsNode( INode* maxNode, const osg::Object* obj, const osg::NodePath& nodePath )
{
    if ( nodePath.size()>0 )
    {
        osg::Node* parent = nodePath.back();
        if ( obj==parent )
        {
            parent = NULL;
            if ( nodePath.size()>1 )
                parent = nodePath.at( nodePath.size()-2 );
        }
        
        if ( parent )
        {
            INode* maxParent = getOrCreateNode( parent );
            if ( maxParent ) maxParent->AttachChild( maxNode );
        }
    }
    
    // Transformation
    osg::Matrix matrix = osg::computeLocalToWorld( nodePath );
    maxNode->SetNodeTM( 0, convertMatrix(matrix) );
    
    // Basic properties
    maxNode->SetName( createName(obj) );
}

void ImportImplementor::importNode( INode* maxNode, const osg::Node* node, const osg::NodePath& nodePath )
{
    importObjectAsNode( maxNode, node, nodePath );
    maxNode->Hide( node->getNodeMask()==0 );
}

MtlBase* ImportImplementor::createStandardMaterial( const osg::StateSet* ss )
{
    MtlBase* maxMtl = getMaxMaterial(ss);
    if ( maxMtl ) return maxMtl;
    
    static int ss_count = 1;
    StdMat* stdMtl = NewDefaultStdMat();
    stdMtl->SetName( createName(ss, "Mtl" + std::to_string(ss_count++) + "_") );
    if ( !ss ) return stdMtl;
    
    // Convert rendering mdoes and attributes into the material
    const osg::StateSet::ModeList& modeList = ss->getModeList();
    for ( osg::StateSet::ModeList::const_iterator itr=modeList.begin();
          itr!=modeList.end(); ++itr )
    {
        importRenderingMode( stdMtl, itr->first, itr->second );
    }
    
    const osg::StateSet::AttributeList& attrList = ss->getAttributeList();
    for ( osg::StateSet::AttributeList::const_iterator itr=attrList.begin();
          itr!=attrList.end(); ++itr )
    {
        importRenderingAttribute( stdMtl, itr->first, itr->second );
    }
    
    typedef std::map<osg::StateAttribute::GLMode, std::vector<UnitModeValuePair> > UnitModeValueMap;
    UnitModeValueMap unitModeValueMap;
    const osg::StateSet::TextureModeList& tmodeList = ss->getTextureModeList();
    for ( unsigned int u=0; u<tmodeList.size(); ++u )
    {
        const osg::StateSet::ModeList& modeList = tmodeList[u];
        for ( osg::StateSet::ModeList::const_iterator itr=modeList.begin();
              itr!=modeList.end(); ++itr )
        {
            unitModeValueMap[itr->first].push_back( UnitModeValuePair(u, itr->second) );
        }
    }
    for ( UnitModeValueMap::iterator itr=unitModeValueMap.begin(); itr!=unitModeValueMap.end(); ++itr )
        importTextureMode( stdMtl, itr->first, itr->second );
    
    typedef std::map<osg::StateAttribute::TypeMemberPair, std::vector<UnitAttributePair> > UnitAttrMap;
    UnitAttrMap unitAttrMap;
    const osg::StateSet::TextureAttributeList& tattrList = ss->getTextureAttributeList();
    for ( unsigned int u=0; u<tattrList.size(); ++u )
    {
        const osg::StateSet::AttributeList& attrList = tattrList[u];
        for ( osg::StateSet::AttributeList::const_iterator itr=attrList.begin();
              itr!=attrList.end(); ++itr )
        {
            unitAttrMap[itr->first].push_back( UnitAttributePair(u, itr->second) );
        }
    }
    for ( UnitAttrMap::iterator itr=unitAttrMap.begin(); itr!=unitAttrMap.end(); ++itr )
        importTextureAttribute( stdMtl, itr->first, itr->second );
    
    maxMtl = stdMtl;
    _materialMap[ss] = maxMtl;
    return maxMtl;
}

void ImportImplementor::importStateSet( INode* maxNode, const osg::StateSet* ss )
{
    // Must check if object reference is set; otherwise configure a node's material will cause crash
    if ( !maxNode->GetObjectRef() )
    {
        NotifyBox( GetString(IDS_INTERNALERROR), L"ImportImplementor::importStateSet" );
        return;
    }
    
    // FIXME: Create a standard material directly?
    Mtl* maxMtl = static_cast<Mtl*>( createStandardMaterial(ss) );
    maxNode->SetMtl( maxMtl );
    //_gi->PutMtlToMtlEditor( maxMtl );
}
