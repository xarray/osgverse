/* -*-c++-*- osg2max - Copyright (C) 2010 by Wang Rui <wangray84 at gmail dot com>
* The max scene enumerator, which builds OSG scene graph from MAX scene
*/

#ifndef OSG2MAX_MAXPARSER
#define OSG2MAX_MAXPARSER

#include <osg/Group>
#include "ExportImplementor.h"

class MaxEnumerator
{
public:
    MaxEnumerator(ExpInterface* ei=0, Interface* gi=0, DWORD options=0);
    virtual ~MaxEnumerator() {}
    
    void traverseMaterials(Interface* gi);
    void traverseNodes(Interface* gi);
    
    bool hasError() const;
    osg::Node* output();
    
    void setExportSelected( bool b ) { _exportSelected = b; }
    bool getExportSelected() const { return _exportSelected; }
    
    virtual bool traverse(INode* maxNode, Mtl* maxMtl, Interface* gi);
    virtual bool traverse(INode* maxNode, osg::Group* parent, Interface* gi);
    
protected:
    osg::ref_ptr<ExportImplementor> _implementor;
    osg::ref_ptr<osg::Node> _root;
    bool _hasError;
    
    bool _exportSelected;
};

#endif
