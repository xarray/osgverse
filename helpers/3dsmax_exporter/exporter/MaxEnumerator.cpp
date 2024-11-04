/* -*-c++-*- osg2max - Copyright (C) 2010 by Wang Rui <wangray84 at gmail dot com>
* The max scene enumerator, which is actually a node visitor traversing OSG scene graph
*/

#include <osg/MatrixTransform>
#include "MaxEnumerator.h"
#include <dummy.h>

MaxEnumerator::MaxEnumerator( ExpInterface* ei, Interface* gi, DWORD options )
:   _hasError(false),
    _exportSelected(false)
{
    _implementor = new ExportImplementor(ei, gi);
}

void MaxEnumerator::traverseMaterials( Interface* gi )
{
    INode* maxRoot = gi->GetRootNode();
    if ( !maxRoot )
    {
        _hasError = true;
        return;
    }
    
    // Traverse the MAX nodes and export materials
    Mtl* maxMtl = maxRoot->GetMtl();
    if ( !traverse(maxRoot, maxMtl, gi) )
    {
        _hasError = true;
        return;
    }
}

void MaxEnumerator::traverseNodes( Interface* gi )
{
    INode* maxRoot = gi->GetRootNode();
    if ( !maxRoot )
    {
        _hasError = true;
        return;
    }
    
    osg::MatrixTransform* transformRoot = _implementor->exportTransform(maxRoot);
    _root = transformRoot;
    
    // Traverse the MAX nodes and export the whole scene
    int numChildren = maxRoot->NumberOfChildren();
    for ( int i=0; i<numChildren; ++i )
    {
        if ( gi->GetCancel() ) return;
        if ( !traverse(maxRoot->GetChildNode(i), transformRoot, gi) )
        {
            _hasError = true;
            return;
        }
    }
}

bool MaxEnumerator::hasError() const
{ return _hasError || _implementor->hasError(); }

osg::Node* MaxEnumerator::output()
{ return _root.get(); }

bool MaxEnumerator::traverse( INode* maxNode, Mtl* maxMtl, Interface* gi )
{
    if ( !maxNode ) return false;
    if ( !_exportSelected || maxNode->Selected() )
    {
        // maxNode==NULL means we are handling sub-items of a multi-material
        ExportImplementor::BakeElementType bakeType = ExportImplementor::BAKE_NONE;
        
        // Get bake element type if this is shell material
        if ( maxMtl && maxMtl->ClassID()==Class_ID(BAKE_SHELL_CLASS_ID, 0) )
            bakeType = _implementor->checkBakeType(maxNode);
        
        // Export current material
        _implementor->exportMaterial(maxMtl, bakeType);
        if ( bakeType!=ExportImplementor::BAKE_NONE )
            maxMtl = maxMtl->GetSubMtl(1);
    }
    
    // Go to child nodes if we are not traversing sub-materials
    int numChildren = maxNode->NumberOfChildren();
    for ( int i=0; i<numChildren; ++i )
    {
        if ( gi->GetCancel() ) return true;
        
        INode* childNode = maxNode->GetChildNode(i);
        if ( !childNode ) continue;
        
        Mtl* childMtl = childNode->GetMtl();
        if ( !traverse(childNode, childMtl, gi) ) return false;
    }
    return true;
}

bool MaxEnumerator::traverse( INode* maxNode, osg::Group* parent, Interface* gi )
{
    osg::Group* subParent = NULL;
    if ( !_exportSelected || maxNode->Selected() )
    {
        Object* obj = maxNode->EvalWorldState(0).obj;
        if ( maxNode->IsGroupHead() || obj->ClassID()==Class_ID(DUMMY_CLASS_ID, 0) )
        {
            osg::MatrixTransform* mt = _implementor->exportTransform(maxNode);
            subParent = mt;
        }
        else
        {
            switch ( obj->SuperClassID() )
            {
            case GEOMOBJECT_CLASS_ID:
                subParent = _implementor->exportGeomObject(maxNode, obj);
                break;
            case SHAPE_CLASS_ID:
                // TODO: export shapes
                break;
            case CAMERA_CLASS_ID:
                // TODO: export cameras
                break;
            case LIGHT_CLASS_ID:
                // TODO: export lights
                break;
            case HELPER_CLASS_ID:
                // TODO: export helpers
                break;
            default:
                {
                    WStr className; obj->GetClassName(className);
                    NotifyBox( GetString(IDS_NOTIMPLEMENTED), L"MaxEnumerator", className.data() );
                }
                return true;
            }
        }
        
        if ( subParent )
            parent->addChild( subParent );
    }
    
    int numChildren = maxNode->NumberOfChildren();
    if ( !subParent ) subParent = parent;
    for ( int i=0; i<numChildren; ++i )
    {
        if ( gi->GetCancel() ) return true;
        if ( !traverse(maxNode->GetChildNode(i), subParent, gi) ) return false;
    }
    return true;
}
