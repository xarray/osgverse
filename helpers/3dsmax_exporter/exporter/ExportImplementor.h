/* -*-c++-*- osg2max - Copyright (C) 2010 by Wang Rui <wangray84 at gmail dot com>
* The export implementation
*/

#ifndef OSG2MAX_EXPORTIMPLEMENTOR
#define OSG2MAX_EXPORTIMPLEMENTOR

#include <osg/StateAttribute>
#include <osg/Drawable>
#include <osg/Node>
#include "exporter.h"
#include "utilities.h"

namespace osg
{
    class MatrixTransform;
    class Texture;
}

class ExportImplementor : public osg::Referenced
{
public:
    enum BakeElementType { BAKE_NONE=0, BAKE_COMPLETE_MAP=1, BAKE_LIGHTING_MAP=2, BAKE_UNKNOWN=99 };
    
    ExportImplementor(ExpInterface* ei=0, Interface* gi=0);
    
    bool hasError() const { return _hasError; }
    BakeElementType checkBakeType(INode* maxNode);
    
    osg::MatrixTransform* exportTransform(INode* maxNode);
    osg::MatrixTransform* exportGeomObject(INode* maxNode, Object* obj);
    osg::StateSet* exportMaterial(Mtl* maxMtl, BakeElementType bakeType);
    
    inline osg::Geode* getGeode(Object* obj);
    inline osg::StateSet* getStateSet(Mtl* mtl);
    inline osg::Texture* getTexture(Texmap* texmap);
    
    static inline osg::Matrix getNodeMatrix(INode* node, TimeValue t=0);
    static inline osg::Matrix getObjectMatrix(INode* node, TimeValue t=0);
    
protected:
    virtual ~ExportImplementor() {}
    
    void createMultiMeshes(osg::Geode* geode, Mesh* mesh, Mtl* mtl, Mtl* originMtl,
                           int numSubMeshes, int i1, int i2, int i3);
    void createStateSet(osg::StateSet* ss, Mtl* mtl, bool hasMaterial, bool hasTexture, int envMode);
    void createSurfaceMaterial(osg::StateSet* ss, Mtl* mtl);
    void createTexture2D(osg::StateSet* ss, Mtl* mtl, int type, int envMode);
    
    typedef std::map<Object*, osg::Geode*> GeodeMap;
    GeodeMap _geodeMap;
    
    typedef std::map<Mtl*, osg::ref_ptr<osg::StateSet> > MeterialMap;
    MeterialMap _materialMap;
    
    typedef std::map<Texmap*, osg::ref_ptr<osg::Texture> > TextureMap;
    TextureMap _textureMap;
    
    ExpInterface* _ei;
    Interface* _gi;
    bool _hasError;
};

// INLINE METHODS

inline osg::Geode* ExportImplementor::getGeode( Object* obj )
{
    GeodeMap::iterator itr = _geodeMap.find(obj);
    if ( itr!=_geodeMap.end() ) return itr->second;
    return NULL;
}

inline osg::StateSet* ExportImplementor::getStateSet( Mtl* mtl )
{
    MeterialMap::iterator itr = _materialMap.find(mtl);
    if ( itr!=_materialMap.end() ) return itr->second.get();
    return NULL;
}

inline osg::Texture* ExportImplementor::getTexture( Texmap* texmap )
{
    TextureMap::iterator itr = _textureMap.find(texmap);
    if ( itr!=_textureMap.end() ) return itr->second.get();
    return NULL;
}

inline osg::Matrix ExportImplementor::getNodeMatrix( INode* node, TimeValue t )
{
    Matrix3 parentTM = node->GetParentTM(t);
    Matrix3 nodeTM = node->GetNodeTM(t);
    return convertMatrix( nodeTM * Inverse(parentTM) );
}

inline osg::Matrix ExportImplementor::getObjectMatrix( INode* node, TimeValue t )
{
    Matrix3 nodeTM = node->GetNodeTM(t);
    Matrix3 objectTM = node->GetObjectTM(t);
    if ( nodeTM==objectTM ) return osg::Matrix::identity();
    
    Matrix3 invParentTM = Inverse(node->GetParentTM(t));
    nodeTM = nodeTM * invParentTM;
    objectTM = (objectTM * invParentTM) * Inverse(nodeTM);
    return convertMatrix( objectTM );
}

#endif
