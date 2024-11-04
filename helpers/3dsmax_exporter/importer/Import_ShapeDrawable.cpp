/* -*-c++-*- osg2max - Copyright (C) 2010 by Wang Rui <wangray84 at gmail dot com>
* The ShapeDrawable import implementation
*/

#include <osg/ShapeDrawable>
#include "ImportImplementor.h"
#include <simpobj.h>

class GenShapeVisitor : public osg::ConstShapeVisitor
{
public:
    GenShapeVisitor(Interface* gi, INode* node)
    : _gi(gi), _node(node), _result(0), _segments(32) {}
    
    Object* getResult() { return _result; }
    
    void addTranslation( const osg::Vec3& trans, const osg::Vec3& offset )
    {
        // FIXME: shape rotation is not handled here
        Matrix3 maxMatrix = _node->GetNodeTM(0);
        maxMatrix.PreTranslate( convertPoint(trans-offset) );
        _node->SetNodeTM( 0, maxMatrix );
    }
    
    virtual void apply(const osg::Sphere& sphere)
    {
        GenSphere* gs = (GenSphere*)_gi->CreateInstance( GEOMOBJECT_CLASS_ID, Class_ID(SPHERE_CLASS_ID,0) );
        gs->SetParams( sphere.getRadius(), _segments );
        
        addTranslation( sphere.getCenter(), osg::Vec3() );
        _result = gs;
    }
    
    virtual void apply(const osg::Box& box)
    {
        const osg::Vec3& lenghts = box.getHalfLengths() * 2.0f;
        GenBoxObject* gb = (GenBoxObject*)_gi->CreateInstance( GEOMOBJECT_CLASS_ID, Class_ID(BOXOBJ_CLASS_ID,0) );
        gb->SetParams( lenghts[0], lenghts[1], lenghts[2] );
        
        addTranslation( box.getCenter(), osg::Vec3(0.0f, 0.0f, lenghts[2]*0.5f) );
        _result = gb;
    }
    
    virtual void apply(const osg::Cone& cone)
    {
        Object* obj = (Object*)CreateInstance( SHAPE_CLASS_ID, Class_ID(CONE_CLASS_ID,0) );
        IParamArray* pblock = obj->GetParamBlock();
        pblock->SetValue( CONE_RADIUS1, 0, cone.getRadius() );
        pblock->SetValue( CONE_RADIUS2, 0, 0.0f );
        pblock->SetValue( CONE_HEIGHT, 0, cone.getHeight() );
        
        addTranslation( cone.getCenter(), osg::Vec3() );
        _result = obj;
    }
    
    virtual void apply(const osg::Cylinder& cylinder)
    {
        GenCylinder* gc = (GenCylinder*)_gi->CreateInstance( GEOMOBJECT_CLASS_ID, Class_ID(CYLINDER_CLASS_ID,0) );
        gc->SetParams( cylinder.getRadius(), cylinder.getHeight(), _segments, _segments );
        
        addTranslation( cylinder.getCenter(), osg::Vec3(0.0f, 0.0f, cylinder.getHeight()*0.5f) );
        _result = gc;
    }
    
    virtual void apply(const osg::Capsule&)
    {
    }
    
    virtual void apply(const osg::TriangleMesh&)
    {
    }
    
    virtual void apply(const osg::ConvexHull&)
    {
    }
    
    virtual void apply(const osg::HeightField&)
    {
    }
    
    virtual void apply(const osg::CompositeShape&)
    {
    }
    
protected:
    Interface* _gi;
    INode* _node;
    Object* _result;
    int _segments;
};

void ImportImplementor::importShape( INode* maxNode, const osg::ShapeDrawable* shapeDrawable )
{
    Object* maxObj = getMaxObject(shapeDrawable);
    if ( !maxObj )
    {
        const osg::Shape* shape = shapeDrawable->getShape();
        if ( shape )
        {
            // FIXME: How to deal with tessellation hints here?
            GenShapeVisitor gsv(_gi, maxNode);
            shape->accept( gsv );
            
            maxObj = gsv.getResult();
        }
        
        maxNode->SetWireColor( convertColorRef(shapeDrawable->getColor()) );
        _objectMap[shapeDrawable] = maxObj;
    }
    maxNode->SetObjectRef( maxObj );
}
