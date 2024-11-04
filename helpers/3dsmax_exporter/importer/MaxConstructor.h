/* -*-c++-*- osg2max - Copyright (C) 2010 by Wang Rui <wangray84 at gmail dot com>
* The max scene constructor, which is actually a node visitor traversing OSG scene graph
*/

#ifndef OSG2MAX_MAXCONSTRUCTOR
#define OSG2MAX_MAXCONSTRUCTOR

#include <osg/NodeVisitor>
#include <osg/ProxyNode>
#include <osg/Drawable>
#include "ImportImplementor.h"

namespace osg
{

class Drawable;
class StateSet;

}

class MaxConstructor : public osg::NodeVisitor
{
public:
    MaxConstructor(ImpInterface* ii=0, Interface* gi=0);
    virtual ~MaxConstructor() {}
    
    bool hasError() const;
    void pushPrefix( const std::string& prefix );
    void popPrefix();
    void finalize();
    
    void setConvertGeodeToMesh( bool b ) { _convertGeodeToMesh = b; }
    bool getConvertGeodeToMesh() const { return _convertGeodeToMesh; }
    
    void setHideDummyNode( bool b ) { _hideDummyNode = b; }
    bool getHideDummyNode() const { return _hideDummyNode; }
    
    virtual void apply(osg::Node& node);
    virtual void apply(osg::ProxyNode& node);
    virtual void apply(osg::Geode& node);
    virtual void apply(osg::Group& node);
    virtual void apply(osg::Drawable& node) {}  // do nothing
    virtual void apply(osg::Geometry& geometry) {}  // do nothing
    
protected:
    osg::ref_ptr<ImportImplementor> _implementor;
    std::string _proxyName;
    
    bool _convertGeodeToMesh;
    bool _hideDummyNode;
};

#endif
