#include <osg/io_utils>
#include <osg/Version>
#include <osg/Notify>
#include <osg/Geometry>
#include <osg/Geode>
#include <osgDB/ReadFile>
#include <osgUtil/SmoothingVisitor>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>

#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
//#include <BulletCollision/NarrowPhaseCollision/btRaycastCallback.h>
#include <modeling/Utilities.h>
#include <animation/PhysicsEngine.h>
using namespace osgVerse;

namespace btHelpers
{
    struct CollisionShape : public osgVerse::CollisionShapeBase { CollisionShape(btCollisionShape* b) {internal = b;} };
    struct RigidBody : public osgVerse::RigidBodyBase { RigidBody(btRigidBody* b) {internal = b;} };
    struct TypedConstraint : public osgVerse::ConstraintBase { TypedConstraint(btTypedConstraint* b) {internal = b;} };

    class PhysicsCore : public PhysicsCoreBase
    {
    public:
        PhysicsCore()
        {
            // FIXME: use a parallel processing dispatcher? (Extras/BulletMultiThreaded)
            _collisionCfg = new btDefaultCollisionConfiguration;
            _collisionDispatcher = new btCollisionDispatcher(_collisionCfg);

            // A good general purpose broadphase, may also try out btAxis3Sweep
            _overlappingPairCache = new btDbvtBroadphase;

            // FIXME: use a parallel processing solver?
            _solver = new btSequentialImpulseConstraintSolver;
            _world = new btDiscreteDynamicsWorld(_collisionDispatcher, _overlappingPairCache,
                                                _solver, _collisionCfg);
            _world->setGravity(btVector3(0, 0, -9.8));
        }

        btDefaultCollisionConfiguration* _collisionCfg;
        btCollisionDispatcher* _collisionDispatcher;
        btBroadphaseInterface* _overlappingPairCache;
        btSequentialImpulseConstraintSolver* _solver;
        btDiscreteDynamicsWorld* _world;

    protected:
        virtual ~PhysicsCore()
        {
            delete _world; delete _solver;
            delete _overlappingPairCache;
            delete _collisionDispatcher;
            delete _collisionCfg;
        }
    };
}

#define PHY_WORLD() (((btHelpers::PhysicsCore*)_core.get())->_world)
class BulletPhysicsEngine : public osgVerse::PhysicsEngine
{
public:
    BulletPhysicsEngine();
    BulletPhysicsEngine(const BulletPhysicsEngine& copy, const osg::CopyOp& op = osg::CopyOp::SHALLOW_COPY)
        : osgVerse::PhysicsEngine(copy, op) {}
    META_Object(osgVerse, BulletPhysicsEngine)

    // Rigid-body functions
    virtual RigidBodyBase* addRigidBody(const std::string& name, CollisionShapeBase* s, float mass = 0.0f,
                                        const osg::Matrix& m = osg::Matrix(), bool kinematic = false);
    virtual void removeBody(const std::string& name);
    virtual bool isDynamicBody(const std::string& name, bool& isKinematic);

    // Setting/getting transform and velocity functions
    virtual void setTransform(const std::string& name, const osg::Matrix& matrix);
    virtual osg::Matrix getTransform(const std::string& name, bool& valid);

    virtual void setVelocity(const std::string& name, const osg::Vec3& v, bool linearOrAngular);
    virtual osg::Vec3 getVelocity(const std::string& name, bool linearOrAngular);

    // Constraint functions
    virtual void addConstraint(const std::string& name, ConstraintBase* constraint,
                                bool noCollisionsBetweenLinked = true);
    virtual void removeConstraint(const std::string& name);

    virtual void setGravity(const osg::Vec3& gravity);
    virtual osg::Vec3 getGravity() const;

    // Raycast functions
    virtual bool raycast(const osg::Vec3& start, const osg::Vec3& end,
                            RaycastHit& result, bool getNameFromBody = true);
    virtual std::vector<RaycastHit> raycastAll(const osg::Vec3& start, const osg::Vec3& end,
                                                bool getNameFromBody = true);

    /* Physics creation functions */
    virtual CollisionShapeBase* createPhysicsPoint();  // for kinematic use only
    virtual CollisionShapeBase* createPhysicsBox(const osg::Vec3& halfSize);
    virtual CollisionShapeBase* createPhysicsCylinder(const osg::Vec3& halfSize);
    virtual CollisionShapeBase* createPhysicsCone(float radius, float height);
    virtual CollisionShapeBase* createPhysicsSphere(float radius);
    virtual CollisionShapeBase* createPhysicsHull(osg::Node* node, bool optimized = true);
    virtual CollisionShapeBase* createPhysicsTriangleMesh(osg::Node* node, bool compressed = true);
    virtual CollisionShapeBase* createPhysicsHeightField(osg::HeightField* hf, bool filpQuad = false);
    virtual ConstraintBase* createConstraintP2P(RigidBodyBase* bodyA, const osg::Vec3& pivotA,
                                                RigidBodyBase* bodyB, const osg::Vec3& pivotB,
                                                const ConstraintSetting* setting = NULL);
    
    virtual void advance(float timeStep, int maxSubSteps = 1);

protected:
    virtual ~BulletPhysicsEngine();
};

BulletPhysicsEngine::BulletPhysicsEngine()
{ _core = new btHelpers::PhysicsCore; setName("Bullet3"); }

BulletPhysicsEngine::~BulletPhysicsEngine()
{
    for (std::map<std::string, ConstraintAndState>::iterator itr = _constraints.begin();
         itr != _constraints.end(); ++itr)
    {
        btTypedConstraint* constraint = itr->second.first->get<btTypedConstraint>();
        PHY_WORLD()->removeConstraint(constraint); delete constraint;
    }
    for (std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.begin();
         itr != _bodies.end(); ++itr)
    {
        btRigidBody* body = itr->second->get<btRigidBody>();
        if (body->getMotionState()) delete body->getMotionState();
        PHY_WORLD()->removeCollisionObject(body); delete body;
    }
    for (std::map<std::string, osg::ref_ptr<CollisionShapeBase>>::iterator itr = _shapes.begin();
         itr != _shapes.end(); ++itr) { delete itr->second->get<btCollisionShape>(); }
    _constraints.clear(); _shapes.clear(); _bodies.clear(); _core = NULL;
}

RigidBodyBase* BulletPhysicsEngine::addRigidBody(const std::string& name, CollisionShapeBase* csb, float mass,
                                                 const osg::Matrix& matrix, bool kinematic)
{
    bool isDynamic = (mass > 0.0f);
    osg::Quat q = matrix.getRotate();
    osg::Vec3 p = matrix.getTrans();
    if (_shapes.find(name) != _shapes.end()) removeBody(name);  // remove existing shape

    btTransform transform; transform.setIdentity();
    transform.setOrigin(btVector3(p.x(), p.y(), p.z()));
    transform.setRotation(btQuaternion(q.x(), q.y(), q.z(), q.w()));
    btCollisionShape* shape = csb ? csb->get<btCollisionShape>() : NULL;
    if (!shape) return NULL;

    btVector3 localInertia(0, 0, 0);
    if (isDynamic) shape->calculateLocalInertia(mass, localInertia);
    btDefaultMotionState* motionState = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, shape, localInertia);
    
    btRigidBody* body = new btRigidBody(rbInfo);
    if (kinematic)
    {
        body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        body->setActivationState(DISABLE_DEACTIVATION);
    }
    else if (mass <= 0.0f)
        body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);

    btHelpers::RigidBody* container = new btHelpers::RigidBody(body);
    _shapes[name] = csb; _bodies[name] = container;
    PHY_WORLD()->addRigidBody(body); return container;
}

void BulletPhysicsEngine::removeBody(const std::string& name)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        btRigidBody* body = itr->second->get<btRigidBody>();
        if (body->getMotionState()) delete body->getMotionState();
        PHY_WORLD()->removeCollisionObject(body);
        delete itr->second; _bodies.erase(itr);
    }

    std::map<std::string, osg::ref_ptr<CollisionShapeBase>>::iterator itr2 = _shapes.find(name);
    if (itr2 != _shapes.end()) { delete itr2->second->get<btCollisionShape>(); _shapes.erase(itr2); }
}

bool BulletPhysicsEngine::isDynamicBody(const std::string& name, bool& isKinematic)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        btRigidBody* body = itr->second->get<btRigidBody>();
        int flags = body ? body->getCollisionFlags() : 0;
        if (flags & btCollisionObject::CF_KINEMATIC_OBJECT) isKinematic = true;
        return (flags & btCollisionObject::CF_STATIC_OBJECT) == 0;
    }
    return false;
}

void BulletPhysicsEngine::setTransform(const std::string& name, const osg::Matrix& matrix)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        osg::Quat q = matrix.getRotate();
        osg::Vec3 p = matrix.getTrans();
        btTransform transform; transform.setIdentity();
        transform.setOrigin(btVector3(p.x(), p.y(), p.z()));
        transform.setRotation(btQuaternion(q.x(), q.y(), q.z(), q.w()));

        btRigidBody* body = itr->second->get<btRigidBody>();
        if (body->getMotionState())
            body->getMotionState()->setWorldTransform(transform);
        body->setWorldTransform(transform);
    }
}

osg::Matrix BulletPhysicsEngine::getTransform(const std::string& name, bool& valid)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        btTransform transform; valid = true;
        btRigidBody* body = itr->second->get<btRigidBody>();
        if (body->getMotionState())
            body->getMotionState()->getWorldTransform(transform);
        else
            transform = body->getWorldTransform();

        const btVector3& p = transform.getOrigin();
        btQuaternion q = transform.getRotation();
        return osg::Matrix(osg::Matrix::rotate(osg::Quat(q.x(), q.y(), q.z(), q.w()))
                         * osg::Matrix::translate(p.x(), p.y(), p.z()));
    }
    valid = false;
    return osg::Matrix();
}

void BulletPhysicsEngine::setVelocity(const std::string& name, const osg::Vec3& v, bool linearOrAngular)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        btRigidBody* body = itr->second->get<btRigidBody>();
        if (linearOrAngular) body->setLinearVelocity(btVector3(v[0], v[1], v[2]));
        else body->setAngularVelocity(btVector3(v[0], v[1], v[2]));
    }
}

osg::Vec3 BulletPhysicsEngine::getVelocity(const std::string& name, bool linearOrAngular)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        btRigidBody* body = itr->second->get<btRigidBody>(); btVector3 vel;
        if (linearOrAngular) vel = body->getLinearVelocity();
        else vel = body->getAngularVelocity();
        return osg::Vec3(vel.x(), vel.y(), vel.z());
    }
    return osg::Vec3();
}

void BulletPhysicsEngine::addConstraint(const std::string& name, ConstraintBase* cBase,
                                        bool noCollisionsBetweenLinked)
{
    btTypedConstraint* constraint = cBase->get<btTypedConstraint>();
    const btRigidBody& bodyA = constraint->getRigidBodyA();
    const btRigidBody& bodyB = constraint->getRigidBodyB();
    int flagsA = bodyA.getCollisionFlags(), constraintedState = bodyA.getActivationState();
    if ((flagsA & btCollisionObject::CF_KINEMATIC_OBJECT) ||
        (flagsA & btCollisionObject::CF_STATIC_OBJECT))
    {
        bodyB.setActivationState(DISABLE_DEACTIVATION);
        constraintedState = bodyB.getActivationState();
    }
    else
        bodyA.setActivationState(DISABLE_DEACTIVATION);

    PHY_WORLD()->addConstraint(constraint, noCollisionsBetweenLinked);
    _constraints[name] = ConstraintAndState(cBase, constraintedState);
}

void BulletPhysicsEngine::removeConstraint(const std::string& name)
{
    std::map<std::string, ConstraintAndState>::iterator itr = _constraints.find(name);
    if (itr != _constraints.end())
    {
        btTypedConstraint* constraint = itr->second.first->get<btTypedConstraint>();
        const btRigidBody& bodyA = constraint->getRigidBodyA();
        const btRigidBody& bodyB = constraint->getRigidBodyB();

        int flagsA = bodyA.getCollisionFlags(), constraintedState = bodyA.getActivationState();
        if ((flagsA & btCollisionObject::CF_KINEMATIC_OBJECT) ||
            (flagsA & btCollisionObject::CF_STATIC_OBJECT))
        { bodyB.forceActivationState(itr->second.second); bodyB.activate(); }
        else { bodyA.forceActivationState(itr->second.second); bodyA.activate(); }

        PHY_WORLD()->removeConstraint(constraint);
        delete constraint; _constraints.erase(itr);
    }
}

void BulletPhysicsEngine::setGravity(const osg::Vec3& gravity)
{ PHY_WORLD()->setGravity(btVector3(gravity[0], gravity[1], gravity[2])); }

osg::Vec3 BulletPhysicsEngine::getGravity() const
{ const btVector3& v = PHY_WORLD()->getGravity(); return osg::Vec3(v.x(), v.y(), v.z()); }

bool BulletPhysicsEngine::raycast(const osg::Vec3& s, const osg::Vec3& e,
                                  RaycastHit& result, bool getNameFromBody)
{
    btVector3 from(s.x(), s.y(), s.z()), to(e.x(), e.y(), e.z());
    btCollisionWorld::ClosestRayResultCallback rayCallback(from, to);
    //rayCallback.m_flags |= btTriangleRaycastCallback::kF_UseGjkConvexCastRaytest;

    PHY_WORLD()->rayTest(from, to, rayCallback);
    if (rayCallback.hasHit())
    {
        btVector3 pos = rayCallback.m_hitPointWorld, norm = rayCallback.m_hitNormalWorld;
        result.position = osg::Vec3(pos.x(), pos.y(), pos.z());
        result.normal = osg::Vec3(norm.x(), norm.y(), norm.z());
        result.rigidBody = new btHelpers::RigidBody(
            (btRigidBody*)btRigidBody::upcast(rayCallback.m_collisionObject));

        if (getNameFromBody)
        {
            for (std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator
                 itr = _bodies.begin(); itr != _bodies.end(); ++itr)
            { if (itr->second->equals(result.rigidBody)) {result.name = itr->first; break;} }
        }
        return true;
    }
    return false;
}

std::vector<BulletPhysicsEngine::RaycastHit> BulletPhysicsEngine::raycastAll(
        const osg::Vec3& s, const osg::Vec3& e, bool getNameFromBody)
{
    btVector3 from(s.x(), s.y(), s.z()), to(e.x(), e.y(), e.z());
    btCollisionWorld::AllHitsRayResultCallback rayCallback(from, to);
    //rayCallback.m_flags |= btTriangleRaycastCallback::kF_UseGjkConvexCastRaytest;

    std::vector<BulletPhysicsEngine::RaycastHit> hitList;
    PHY_WORLD()->rayTest(from, to, rayCallback);
    if (rayCallback.hasHit())
    {
        for (int i = 0; i < rayCallback.m_collisionObjects.size(); ++i)
        {
            btVector3 pos = rayCallback.m_hitPointWorld[i],
                      norm = rayCallback.m_hitNormalWorld[i];
            BulletPhysicsEngine::RaycastHit result;
            result.position = osg::Vec3(pos.x(), pos.y(), pos.z());
            result.normal = osg::Vec3(norm.x(), norm.y(), norm.z());
            result.rigidBody = new btHelpers::RigidBody(
                (btRigidBody*)btRigidBody::upcast(rayCallback.m_collisionObjects[i]));

            if (getNameFromBody)
            {
                for (std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator
                     itr = _bodies.begin(); itr != _bodies.end(); ++itr)
                { if (itr->second->equals(result.rigidBody)) {result.name = itr->first; break;} }
            }
            hitList.push_back(result);
        }
    }
    return hitList;
}

void BulletPhysicsEngine::advance(float timeStep, int maxSubSteps)
{ PHY_WORLD()->stepSimulation(timeStep, maxSubSteps); }

CollisionShapeBase* BulletPhysicsEngine::createPhysicsPoint()
{ return new btHelpers::CollisionShape(new btEmptyShape()); }

CollisionShapeBase* BulletPhysicsEngine::createPhysicsBox(const osg::Vec3& halfSize)
{ return new btHelpers::CollisionShape(new btBoxShape(btVector3(halfSize[0], halfSize[1], halfSize[2]))); }

CollisionShapeBase* BulletPhysicsEngine::createPhysicsCylinder(const osg::Vec3& halfSize)
{ return new btHelpers::CollisionShape(new btCylinderShape(btVector3(halfSize[0], halfSize[1], halfSize[2]))); }

CollisionShapeBase* BulletPhysicsEngine::createPhysicsCone(float radius, float height)
{ return new btHelpers::CollisionShape(new btConeShape(radius, height)); }

CollisionShapeBase* BulletPhysicsEngine::createPhysicsSphere(float radius)
{ return new btHelpers::CollisionShape(new btSphereShape(radius)); }

CollisionShapeBase* BulletPhysicsEngine::createPhysicsHull(osg::Node* node, bool optimized)
{
    osgVerse::MeshCollector bvv; if (node != NULL) node->accept(bvv);
    const std::vector<osg::Vec3>& vertices = bvv.getVertices();
    if (vertices.empty()) return NULL;

    btConvexHullShape* shape = new btConvexHullShape(
        (const btScalar*)&vertices[0], vertices.size(), sizeof(btScalar) * 3);
    if (optimized) { shape->optimizeConvexHull(); shape->initializePolyhedralFeatures(); }
    return new btHelpers::CollisionShape(shape);
}

CollisionShapeBase* BulletPhysicsEngine::createPhysicsTriangleMesh(osg::Node* node, bool compressed)
{
    osgVerse::MeshCollector bvv; if (node != NULL) node->accept(bvv);
    const std::vector<osg::Vec3>& vertices = bvv.getVertices();
    const std::vector<unsigned int>& triangles = bvv.getTriangles();
    if (vertices.empty() || triangles.empty()) return NULL;

    btIndexedMesh meshPart;
    meshPart.m_numTriangles = triangles.size() / 3;
    meshPart.m_numVertices = vertices.size();
    meshPart.m_indexType = PHY_INTEGER;
    meshPart.m_triangleIndexStride = 3 * sizeof(int);
    meshPart.m_vertexType = PHY_FLOAT;
    meshPart.m_vertexStride = sizeof(btVector3FloatData);

    int* indexArray = (int*)btAlignedAlloc(sizeof(int) * 3 * meshPart.m_numTriangles, 16);
    for (int j = 0; j < 3 * meshPart.m_numTriangles; j++) indexArray[j] = triangles[j];
    meshPart.m_triangleIndexBase = (const unsigned char*)indexArray;

    btVector3FloatData* btVertices = (btVector3FloatData*)btAlignedAlloc(
        sizeof(btVector3FloatData) * meshPart.m_numVertices, 16);
    for (int j = 0; j < meshPart.m_numVertices; j++)
    {
        btVertices[j].m_floats[0] = vertices[j][0];
        btVertices[j].m_floats[1] = vertices[j][1];
        btVertices[j].m_floats[2] = vertices[j][2];
        btVertices[j].m_floats[3] = 0.f;
    }
    meshPart.m_vertexBase = (const unsigned char*)btVertices;

    btTriangleIndexVertexArray* meshInterface = new btTriangleIndexVertexArray();
    meshInterface->addIndexedMesh(meshPart, meshPart.m_indexType);
    return new btHelpers::CollisionShape(new btBvhTriangleMeshShape(meshInterface, compressed));
}

CollisionShapeBase* BulletPhysicsEngine::createPhysicsHeightField(osg::HeightField* hf, bool filpQuad)
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
    shape->setUseDiamondSubdivision(true); return new btHelpers::CollisionShape(shape);
}

ConstraintBase* BulletPhysicsEngine::createConstraintP2P(RigidBodyBase* bodyA, const osg::Vec3& pA,
                                                         RigidBodyBase* bodyB, const osg::Vec3& pB,
                                                         const ConstraintSetting* setting)
{
    btVector3 pivotA(pA[0], pA[1], pA[2]), pivotB(pB[0], pB[1], pB[2]);
    if (!bodyA || !bodyB) return NULL;

    btRigidBody *btA = bodyA->get<btRigidBody>(), *btB = bodyB->get<btRigidBody>();
    if (setting && setting->useWorldPivots)
    {
        pivotA = btA->getCenterOfMassTransform().inverse() * pivotA;
        pivotB = btB->getCenterOfMassTransform().inverse() * pivotB;
    }

    btPoint2PointConstraint* p2p = new btPoint2PointConstraint(*btA, *btB, pivotA, pivotB);
    if (setting)
    {
        p2p->m_setting.m_tau = setting->tau;
        p2p->m_setting.m_damping = setting->damping;
        p2p->m_setting.m_impulseClamp = setting->impulseClamp;
    }
    return new btHelpers::TypedConstraint(p2p);
}

/// ReaderWriterBullet ///
class ReaderWriterBullet : public osgDB::ReaderWriter
{
public:
    ReaderWriterBullet()
    {
        supportsExtension("verse_bullet", "osgVerse pseudo-loader");
    }

    virtual const char* className() const
    { return "[osgVerse] Bullet physics engine"; }

    virtual ReadResult readObject(const std::string& path, const Options* options) const
    {
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;
        else return new BulletPhysicsEngine;
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_bullet, ReaderWriterBullet)
