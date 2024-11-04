/* -*-c++-*- osg2max - Copyright (C) 2010 by Wang Rui <wangray84 at gmail dot com>
* The export implementation
*/

#include <osg/MatrixTransform>
#include <osgDB/ConvertUTF>
#include "ExportImplementor.h"
#include <INodeBakeProperties.h>

ExportImplementor::ExportImplementor( ExpInterface* ei, Interface* gi )
:   _ei(ei), _gi(gi), _hasError(false)
{
}

ExportImplementor::BakeElementType ExportImplementor::checkBakeType( INode* maxNode )
{
    if ( !maxNode ) return BAKE_NONE;
    
    INodeBakeProperties* bakeProps =
        static_cast<INodeBakeProperties*>( maxNode->GetInterface(NODE_BAKE_PROPERTIES_INTERFACE) );
    if ( bakeProps && bakeProps->GetNBakeElements()>0 )
    {
        WStr elementName = bakeProps->GetBakeElement(0)->GetName();
        if ( elementName == WStr(L"CompleteMap") )
            return BAKE_COMPLETE_MAP;
        else if ( elementName == WStr(L"LightingMap") )
            return BAKE_LIGHTING_MAP;
        else
        {
            NotifyBox( GetString(IDS_NOTIMPLEMENTED), L"MaxEnumerator", elementName.data() );
            return BAKE_UNKNOWN;
        }
    }
    return BAKE_NONE;
}

osg::MatrixTransform* ExportImplementor::exportTransform( INode* maxNode )
{
    osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
    mt->setName( ws2s(maxNode->GetName()) );
    mt->setMatrix( getNodeMatrix(maxNode, 0) );
    return mt.release();
}
