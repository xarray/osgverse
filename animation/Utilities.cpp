#include <osg/io_utils>
#include <osg/Version>
#include <osg/ShapeDrawable>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <osgDB/ReadFile>
#include <osgUtil/SmoothingVisitor>

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <modeling/Utilities.h>
#include "Utilities.h"
using namespace osgVerse;

PhysicsUpdateCallback::PhysicsUpdateCallback(PhysicsEngine* e, const std::string& n)
{ _engine = e; _bodyName = n; }

void PhysicsUpdateCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
    if (_engine.valid())
    {
        bool isValid = false;
        osg::Matrix m = _engine->getTransform(_bodyName, isValid);

        osg::Group* group = node->asGroup();
        if (group && isValid)
        {
            osg::Transform* transform = group->asTransform();
            if (transform)
            {
                osg::MatrixTransform* mt = transform->asMatrixTransform();
                if (mt) mt->setMatrix(m);

                osg::PositionAttitudeTransform* pat =
                    transform->asPositionAttitudeTransform();
                if (pat) { pat->setAttitude(m.getRotate()); pat->setPosition(m.getTrans()); }
            }
        }
    }
    traverse(node, nv);
}

namespace osgVerse
{

    btCollisionShape* createPhysicsPoint()
    { return new btEmptyShape(); }

    btCollisionShape* createPhysicsBox(const osg::Vec3& halfSize)
    { return new btBoxShape(btVector3(halfSize[0], halfSize[1], halfSize[2])); }

    btCollisionShape* createPhysicsCylinder(const osg::Vec3& halfSize)
    { return new btCylinderShape(btVector3(halfSize[0], halfSize[1], halfSize[2])); }

    btCollisionShape* createPhysicsCone(float radius, float height)
    { return new btConeShape(radius, height); }

    btCollisionShape* createPhysicsSphere(float radius)
    { return new btSphereShape(radius); }

    btCollisionShape* createPhysicsHull(osg::Node* node, bool optimized)
    {
        osgVerse::MeshCollector bvv; if (node != NULL) node->accept(bvv);
        const std::vector<osg::Vec3>& vertices = bvv.getVertices();
        if (vertices.empty()) return NULL;

        btConvexHullShape* shape = new btConvexHullShape(
            (const btScalar*)&vertices[0], vertices.size(), sizeof(btScalar) * 3);
        if (optimized) { shape->optimizeConvexHull(); shape->initializePolyhedralFeatures(); }
        return shape;
    }

    btCollisionShape* createPhysicsTriangleMesh(osg::Node* node, bool compressed)
    {
        osgVerse::MeshCollector bvv; if (node != NULL) node->accept(bvv);
        const std::vector<osg::Vec3>& vertices = bvv.getVertices();
        const std::vector<unsigned int>& triangles = bvv.getTriangles();
        if (vertices.empty() || triangles.empty()) return NULL;

        btTriangleIndexVertexArray* triangleData = new btTriangleIndexVertexArray(
            triangles.size() / 3, (int*)&triangles[0], sizeof(int) * 3,
            vertices.size(), (btScalar*)&vertices[0], sizeof(btScalar) * 3);
        return new btBvhTriangleMeshShape(triangleData, compressed);
    }

    btCollisionShape* createPhysicsHeightField(osg::HeightField* hf, bool filpQuad)
    {
        const osg::HeightField::HeightList& heights = hf->getHeightList();
        btScalar minHeight = FLT_MAX, maxHeight = -FLT_MAX;
        for (size_t i = 0; i < heights.size(); ++i)
        {
            float h = heights[i];
            if (h < minHeight) minHeight = h;
            if (h > maxHeight) maxHeight = h;
        }  // TODO: check if correct

        btHeightfieldTerrainShape* shape = new btHeightfieldTerrainShape(
            hf->getNumRows(), hf->getNumColumns(), &heights[0],
            minHeight, maxHeight, 2, filpQuad);
        shape->setLocalScaling(btVector3(hf->getXInterval(), hf->getYInterval(), 1.0f));
        shape->setUseDiamondSubdivision(true); return shape;
    }

    btTypedConstraint* createConstraintP2P(btRigidBody* bodyA, const osg::Vec3& pA,
                                           btRigidBody* bodyB, const osg::Vec3& pB,
                                           const ConstraintSetting* setting)
    {
        btVector3 pivotA(pA[0], pA[1], pA[2]), pivotB(pB[0], pB[1], pB[2]);
        if (!bodyA || !bodyB) return NULL;
        if (setting && setting->useWorldPivots)
        {
            pivotA = bodyA->getCenterOfMassTransform().inverse() * pivotA;
            pivotB = bodyB->getCenterOfMassTransform().inverse() * pivotB;
        }

        btPoint2PointConstraint* p2p = new btPoint2PointConstraint(*bodyA, *bodyB, pivotA, pivotB);
        if (setting)
        {
            p2p->m_setting.m_tau = setting->tau;
            p2p->m_setting.m_damping = setting->damping;
            p2p->m_setting.m_impulseClamp = setting->impulseClamp;
        }
        return p2p;
    }
}
