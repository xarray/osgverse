/* -*-c++-*- osg2max - Copyright (C) 2010 by Wang Rui <wangray84 at gmail dot com>
* The max scene constructor, which is actually a node visitor traversing OSG scene graph
*/

#include <osg/Geometry>
#include <osg/ShapeDrawable>
#include <osg/Geode>
#include <osg/Group>
#include <osgDB/ReadFile>
#include <osgDB/FileNameUtils>
#include "MaxConstructor.h"
#include <dummy.h>

MaxConstructor::MaxConstructor( ImpInterface* ii, Interface* gi )
:   osg::NodeVisitor(),
    _convertGeodeToMesh(false),
    _hideDummyNode(true)
{
    setTraversalMode( TRAVERSE_ALL_CHILDREN );
    _implementor = new ImportImplementor(ii, gi);
}

bool MaxConstructor::hasError() const
{ return _implementor->hasError(); }

void MaxConstructor::pushPrefix( const std::string& prefix )
{ _implementor->pushPrefix(prefix); }

void MaxConstructor::popPrefix()
{ _implementor->popPrefix(); }

void MaxConstructor::finalize()
{ _implementor->finalize(_hideDummyNode); }

void MaxConstructor::apply( osg::Node& node )
{
    osg::NodePath currentPath = _nodePath;
    
    // Import node
    // Here we must avoid reusing of instanced nodes, because 3dsmax doesn't support it
    INode* maxNode = _implementor->getOrCreateNode(&node, true);
    _implementor->importNode( maxNode, &node, currentPath );
    
    // Don't import dummies and statesets of unknown node-types
    traverse(node);
}

void MaxConstructor::apply(osg::ProxyNode& node)
{
    osg::NodePath currentPath = _nodePath;
    /*for (unsigned int i = 0; i < node.getNumFileNames(); ++i)
    {
        osg::ref_ptr<osg::Node> child = osgDB::readNodeFile(node.getFileName(i));
        if (child.valid()) node.addChild( child.get() );
    }*/

    // Import node
    // Here we must avoid reusing of instanced nodes, because 3dsmax doesn't support it
    INode* maxNode = _implementor->getOrCreateNode(&node, true);
    _implementor->importNode(maxNode, &node, currentPath);

    // Don't import dummies and statesets of unknown node-types
    if (node.getNumChildren() > 0)
    {
        for (unsigned int i = 0; i < node.getNumChildren(); ++i)
        {
            _proxyName = osgDB::getStrippedName(node.getFileName(i));
            //pushPrefix( node.getFileName(i) );
            node.getChild(i)->setName(osgDB::getStrippedName(node.getFileName(i)));
            node.getChild(i)->accept( *this );
            //popPrefix();
            _proxyName = "";
        }
    }
    else
        traverse(node);
}

void MaxConstructor::apply( osg::Geode& node )
{
    std::vector<const osg::Geometry*> geometries;
    
    // We will never directly add geodes to max to reduce the complexity of the scene
    osg::NodePath currentPath = _nodePath;
    currentPath.pop_back();  // Remove the geode from the path
    
    const osg::StateSet* ss = node.getStateSet();
    for ( unsigned int i=0; i<node.getNumDrawables(); ++i )
    {
        osg::Drawable* drawable = node.getDrawable(i);
        
        // Check if we should collect all geometries for merging geode later
        osg::Geometry* geom = drawable->asGeometry();
        if ( geom && _convertGeodeToMesh )
        {
            geometries.push_back( geom );
            continue;
        }
        
        // Import drawable as max node
        INode* maxDrawableNode = _implementor->getOrCreateNode(drawable, true);
        _implementor->importObjectAsNode( maxDrawableNode, drawable, currentPath );
        
        // Import geometry objects if possible
        if ( geom )
            _implementor->importGeometry( maxDrawableNode, geom );
        else
        {
            // Import shape objects if possible
            osg::ShapeDrawable* shape = dynamic_cast<osg::ShapeDrawable*>(drawable);
            if ( shape ) _implementor->importShape( maxDrawableNode, shape );
        }
        
        // Import rendering states of the drawable only when there is an object reference
        osg::StateSet* ssDrawable = const_cast<osg::StateSet*>( drawable->getStateSet() );
        if ( ssDrawable )
        {
            if ( ss ) ssDrawable->merge( *ss );
            _implementor->importStateSet( maxDrawableNode, ssDrawable );
        }
        else if ( ss )
            _implementor->importStateSet( maxDrawableNode, ss );
    }
    
    if ( geometries.size()>0 && _convertGeodeToMesh )
    {
        if ( geometries.size()==1 )
        {
            // If only one drawable exists in the geode, treat it normally
            const osg::Geometry* geom = geometries.front();
            INode* maxDrawableNode = _implementor->getOrCreateNode(geom, true);
            _implementor->importObjectAsNode( maxDrawableNode, geom, currentPath );
            _implementor->importGeometry( maxDrawableNode, geom );
            
            osg::StateSet* ssDrawable = const_cast<osg::StateSet*>( geom->getStateSet() );
            if ( ssDrawable )
            {
                if ( ss ) ssDrawable->merge( *ss );
                _implementor->importStateSet( maxDrawableNode, ssDrawable );
            }
            else if ( ss )
                _implementor->importStateSet( maxDrawableNode, ss );
            maxDrawableNode->SetName( _implementor->createName(&node) );  // Set geode's name to the maxNode
            if (!_proxyName.empty()) maxDrawableNode->SetName(s2ws(_proxyName + "_Mesh").c_str());
        }
        else
        {
            // Merge all drawables in the geode into one 3dsmax mesh
            INode* maxNode = _implementor->getOrCreateNode(&node, true);
            _implementor->importObjectAsNode( maxNode, &node, currentPath );
            _implementor->importMultiGeometry( maxNode, &node, geometries );
            if (!_proxyName.empty()) maxNode->SetName(s2ws(_proxyName + "_Mesh").c_str());
        }
    }
    traverse(node);
}

void MaxConstructor::apply( osg::Group& node )
{
    osg::NodePath currentPath = _nodePath;
    
    // Import node
    INode* maxNode = _implementor->getOrCreateNode(&node, true);
    _implementor->importNode( maxNode, &node, currentPath );
    
    // Apply an empty dummy to the node
    osg::BoundingBox bb = ImportImplementor::createBoxForDummy(&node);
    DummyObject* dummy = new DummyObject;
    dummy->SetBox( Box3(convertPoint(bb._min), convertPoint(bb._max)) );
    if ( _hideDummyNode ) dummy->DisableDisplay();
    else dummy->EnableDisplay();
    maxNode->SetObjectRef( dummy );
    
    // Import rendering states only when there is an object reference
    const osg::StateSet* ss = node.getStateSet();
    if ( ss ) _implementor->importStateSet( maxNode, ss );
    
    traverse(node);
}
