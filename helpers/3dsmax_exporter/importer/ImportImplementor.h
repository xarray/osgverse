/* -*-c++-*- osg2max - Copyright (C) 2010 by Wang Rui <wangray84 at gmail dot com>
* The import implementation
*/

#ifndef OSG2MAX_IMPORTIMPLEMENTOR
#define OSG2MAX_IMPORTIMPLEMENTOR

#include <osg/BoundingBox>
#include <osg/StateAttribute>
#include <osg/Drawable>
#include <osg/Node>
#include "importer.h"
#include "utilities.h"
#include <stdmat.h>

namespace osg
{

class Geometry;
class ShapeDrawable;
class Geode;
class Texture2D;

}

class ImportImplementor : public osg::Referenced
{
public:
    ImportImplementor(ImpInterface* ii=0, Interface* gi=0);
    
    bool hasError() const { return _hasError; }
    const std::string& getPrefix() const { return _prefix.back(); }
    void pushPrefix( const std::string& prefix ) { _prefix.push_back(prefix); }
    void popPrefix() { _prefix.pop_back(); }
    void finalize( bool hideDummyNode );
    
    void importObjectAsNode(INode* maxNode, const osg::Object* obj, const osg::NodePath& nodePath);
    void importNode(INode* maxNode, const osg::Node* node, const osg::NodePath& nodePath);
    void importStateSet(INode* maxNode, const osg::StateSet* ss);
    
    void importMultiGeometry(INode* maxNode, const osg::Node* geode, std::vector<const osg::Geometry*>& geoms);
    void importGeometry(INode* maxNode, const osg::Geometry* geom);
    void importShape(INode* maxNode, const osg::ShapeDrawable* shapeDrawable);
    
    void importRenderingMode(MtlBase* maxMtl, osg::StateAttribute::GLMode mode, osg::StateAttribute::GLModeValue value);
    void importRenderingAttribute(MtlBase* maxMtl, osg::StateAttribute::TypeMemberPair type,
                                  osg::StateSet::RefAttributePair attr);
    
    typedef std::pair<unsigned int, osg::StateAttribute::GLModeValue> UnitModeValuePair;
    typedef std::pair<unsigned int, osg::StateSet::RefAttributePair> UnitAttributePair;
    void importTextureMode(MtlBase* maxMtl, osg::StateAttribute::GLMode mode, std::vector<UnitModeValuePair>& values);
    void importTextureAttribute(MtlBase* maxMtl, osg::StateAttribute::TypeMemberPair type,
                                std::vector<UnitAttributePair>& attrs);
    
    inline INode* getOrCreateNode(const osg::Object* obj, bool forceCreate=false);
    inline Object* getMaxObject(const osg::Object* obj);
    inline MtlBase* getMaxMaterial(const osg::Object* obj);
    
    static inline WStr createName(const osg::Object* obj, const std::string& prefix=std::string(""));
    static inline osg::BoundingBox createBoxForDummy(const osg::Object* obj);
    
protected:
    virtual ~ImportImplementor() {}
    
    void createMeshContent(Mesh& mesh, MtlID mtlID, const osg::Geometry* geom,
                           std::vector<unsigned int>& faces, unsigned int totalUnits);
    MtlBase* createStandardMaterial(const osg::StateSet* ss);
    BitmapTex* createBitmapTexture(osg::Texture2D* tex2D, const std::string& prefix);
    
    typedef std::map<const osg::Object*, INode*> NodeMap;
    NodeMap _nodeMap;
    
    typedef std::map<const osg::Object*, Object*> ObjectMap;
    ObjectMap _objectMap;
    
    typedef std::map<const osg::Object*, MtlBase*> MaterialMap;
    MaterialMap _materialMap;

    std::vector<std::string> _prefix;
    ImpInterface* _ii;
    Interface* _gi;
    bool _hasError;
};

// INLINE METHODS

inline INode* ImportImplementor::getOrCreateNode( const osg::Object* obj, bool forceCreate )
{
    if ( !forceCreate )
    {
        NodeMap::iterator itr = _nodeMap.find(obj);
        if ( itr!=_nodeMap.end() ) return itr->second;
        else return NULL;
    }
    
    ImpNode * impNode = _ii->CreateNode();
    _ii->AddNodeToScene( impNode );
    
    INode* maxNode = impNode->GetINode();
    _nodeMap[obj] = maxNode;
    return maxNode;
}

inline Object* ImportImplementor::getMaxObject( const osg::Object* obj )
{
    ObjectMap::iterator itr = _objectMap.find(obj);
    if ( itr!=_objectMap.end() ) return itr->second;
    return NULL;
}

inline MtlBase* ImportImplementor::getMaxMaterial( const osg::Object* obj )
{
    MaterialMap::iterator itr = _materialMap.find(obj);
    if ( itr!=_materialMap.end() ) return itr->second;
    return NULL;
}

inline WStr ImportImplementor::createName( const osg::Object* obj, const std::string& prefix )
{
    if ( !obj ) return WStr(s2ws(prefix).c_str());
    else if ( obj->getName().empty() ) return WStr(s2ws(prefix).c_str()) + WStr(s2ws(obj->className()).c_str());
    else return WStr(s2ws(prefix).c_str()) + WStr(s2ws(obj->getName()).c_str());
}

inline osg::BoundingBox ImportImplementor::createBoxForDummy( const osg::Object* obj )
{
    osg::BoundingBox bb;
    const osg::Node* node = dynamic_cast<const osg::Node*>(obj);
    if ( node )
        bb.expandBy( node->getBound() );
    else
    {
        const osg::Drawable* drawable = dynamic_cast<const osg::Drawable*>(obj);
        if ( drawable ) bb.expandBy( drawable->getBound() );
    }
    osg::Vec3 center = bb.center();
    bb._min -= center; bb._max -= center;
    return bb;
}

#endif
