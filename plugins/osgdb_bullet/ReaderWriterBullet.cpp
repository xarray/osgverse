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
#include <set>
using namespace osgVerse;

namespace btHelpers
{
    struct CollisionShape : public osgVerse::CollisionShapeBase
    {
        CollisionShape(btCollisionShape* b, const osg::Vec3& off = osg::Vec3())
        :   offset(off), owner(NULL), compound(NULL) { internal = b; }

        btCollisionShape* getShape() { return (btCollisionShape*)internal; }
        osg::Vec3 offset;            // local offset of the shape geometry
        btCollisionObject* owner;    // the body this shape is attached to
        btCompoundShape* compound;   // wrapper for offset shapes (only used by getBodyShape())
    };

    struct TypedConstraint : public osgVerse::ConstraintBase
    {
        TypedConstraint(btTypedConstraint* b, osgVerse::PhysicsEngine::ConstraintType t
                        = osgVerse::PhysicsEngine::CONSTRAINT_P2P) : type(t) { internal = b; }
        btTypedConstraint* getConstraint() { return (btTypedConstraint*)internal; }
        osgVerse::PhysicsEngine::ConstraintType type;  // the Box3D joint type it emulates
    };

    struct RigidBody : public osgVerse::RigidBodyBase
    {
        RigidBody(btRigidBody* b)
        : compound(NULL), totalMass(0.0f), collisionGroup(0), gravityScale(1.0f) { internal = b; }

        btRigidBody* getBody() { return (btRigidBody*)internal; }
        btCompoundShape* compound;  // owned, for bodies which may have multiple shapes
        std::vector<btCollisionShape*> ownedShapes;  // shapes attached by addShapeToBody()
        float totalMass; int collisionGroup; float gravityScale;
    };

    // Bullet doesn't provide all features of Box3D joints and springs, warn once for each of them
    static void warnUnsupported(const std::string& feature)
    {
        static std::set<std::string> s_known;
        if (s_known.find(feature) != s_known.end()) return;
        s_known.insert(feature);
        OSG_WARN << "[PhysicsEngine] Bullet doesn't support " << feature << ", which will be ignored"
                 << std::endl;
    }

    // Motor limits of Bullet are impulses, so torques are converted with an assumed time step
    static const float motorTimeStep = 1.0f / 60.0f;

    /** Bullet has no springs on hinges and spherical joints, so they are driven to their target
        pose by a motor: this returns the torque limit of the motor as an impulse */
    static btScalar motorImpulse(const osgVerse::PhysicsEngine::ConstraintSetting& cs)
    {
        float torque = (cs.maxMotorTorque > 0.0f) ? cs.maxMotorTorque : cs.maxSpringForce;
        return torque * motorTimeStep;
    }

    /** Bullet uses a spring stiffness (N/m or Nm/rad) instead of a frequency, so it is derived
        from the requested frequency and the mass / inertia of the driven body */
    static btScalar springStiffness(btRigidBody* body, int axis, float hertz)
    {
        btScalar omega = SIMD_2_PI * btMax(hertz, 0.1f);
        if (axis < 3) return btMax(1.0f, body->getMass() * omega * omega);

        btVector3 inv = body->getInvInertiaDiagLocal();
        btScalar inertia = 1.0f / btMax(0.01f, btMax(inv.x(), btMax(inv.y(), inv.z())));
        return btMax(1.0f, inertia * omega * omega);
    }

    /** Bullet's damping is reversed compared with the damping ratio of Box3D: 1 means no damping */
    static btScalar springDamping(float dampingRatio)
    { return btMax(btScalar(0.0f), btMin(btScalar(1.0f), btScalar(1.0f - dampingRatio))); }

    /** Bullet has no joints without any effect: a 6DOF constraint with all axes free generates no
        force, but it still makes Bullet disable the collisions between the two linked bodies */
    static btGeneric6DofConstraint* createEmptyConstraint(btRigidBody* a, btRigidBody* b,
                                                          const btTransform& tA, const btTransform& tB)
    {
        btGeneric6DofConstraint* empty = new btGeneric6DofConstraint(*a, *b, tA, tB, false);
        empty->setLinearLowerLimit(btVector3(1.0f, 1.0f, 1.0f));
        empty->setLinearUpperLimit(btVector3(-1.0f, -1.0f, -1.0f));
        empty->setAngularLowerLimit(btVector3(1.0f, 1.0f, 1.0f));
        empty->setAngularUpperLimit(btVector3(-1.0f, -1.0f, -1.0f));
        return empty;
    }
    
    static btTransform toBtTransform(const osg::Matrix& m)
    {
        osg::Quat q = m.getRotate(); osg::Vec3 p = m.getTrans();
        btTransform t; t.setRotation(btQuaternion(q.x(), q.y(), q.z(), q.w()));
        t.setOrigin(btVector3(p[0], p[1], p[2])); return t;
    }

    /** Box3D's target rotation of a spherical joint is the rotation of joint frame B related to
        joint frame A, while Bullet wants the opposite one (frame A related to frame B) */
    static btQuaternion toBtQuaternion(const osg::Quat& q)
    { return btQuaternion(q.x(), q.y(), q.z(), q.w()).inverse(); }

    // Bullet stores material properties on bodies rather than shapes, so they are applied once
    static void applyShapeSetting(btRigidBody* body, const osgVerse::PhysicsEngine::ShapeSetting* setting)
    {
        if (!setting) return;
        body->setFriction(setting->friction);
        body->setRestitution(setting->restitution);
        body->setRollingFriction(setting->rollingResistance);
        if (setting->restitution > 0.0f)
            warnUnsupported("the Box3D way of mixing restitution, both bodies need a restitution in Bullet");
    }

    // Bullet filtering uses group and mask bits: bodies in the same group won't collide with each
    // other, which emulates the negative collision group of Box3D shapes
    static void applyCollisionGroup(btDiscreteDynamicsWorld* world, RigidBody* container, int group)
    {
        if (group == 0 || container->collisionGroup == group) return;
        if (container->collisionGroup != 0)
        { warnUnsupported("multiple collision groups on one body"); return; }

        int groupBits = 1 << (1 + (group > 0 ? group : -group) % 30), maskBits = ~groupBits;
        btRigidBody* body = container->getBody();
        world->removeRigidBody(body);
        world->addRigidBody(body, groupBits, maskBits);
        body->setGravity(world->getGravity() * container->gravityScale);  // reset by addRigidBody()
        container->collisionGroup = group;
    }

    // Get the shape used to create a rigid body, which may be a compound wrapper of an offset shape
    static btCollisionShape* getBodyShape(CollisionShape* cs)
    {
        if (cs->offset.length2() <= 0.0f) return cs->getShape();
        if (!cs->compound)
        {
            cs->compound = new btCompoundShape;
            btTransform local; local.setIdentity();
            local.setOrigin(btVector3(cs->offset[0], cs->offset[1], cs->offset[2]));
            cs->compound->addChildShape(local, cs->getShape());
        }
        return cs->compound;
    }

    static void destroyShape(CollisionShape* cs)
    {
        if (!cs) return;
        if (cs->compound) delete cs->compound;  // children shapes are not owned by the compound
        if (cs->internal) delete cs->getShape();
        cs->compound = NULL; cs->internal = NULL;
    }

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

    struct SweepResultCallback : public btCollisionWorld::ClosestConvexResultCallback
    {
        SweepResultCallback(const btVector3& from, const btVector3& to, const btCollisionObject* ignored)
        :   btCollisionWorld::ClosestConvexResultCallback(from, to), ignoredBody(ignored) {}

        virtual bool needsCollision(btBroadphaseProxy* proxy) const
        {
            if (proxy->m_clientObject == (void*)ignoredBody) return false;  // skip the ignored body
            return ClosestConvexResultCallback::needsCollision(proxy);
        }

        const btCollisionObject* ignoredBody;
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
                                        const osg::Matrix& m = osg::Matrix(), bool kinematic = false,
                                        const ShapeSetting* setting = NULL);
    virtual void removeBody(const std::string& name);
    virtual bool isDynamicBody(const std::string& name, bool& isKinematic);

    // Setting/getting transform and velocity functions
    virtual void setTransform(const std::string& name, const osg::Matrix& matrix);
    virtual osg::Matrix getTransform(const std::string& name, bool& valid);

    virtual void setVelocity(const std::string& name, const osg::Vec3& v, bool linearOrAngular);
    virtual osg::Vec3 getVelocity(const std::string& name, bool linearOrAngular);

    virtual void setGravity(const osg::Vec3& gravity);
    virtual osg::Vec3 getGravity() const;

    // Constraint functions
    virtual void addConstraint(const std::string& name, ConstraintBase* constraint,
                                bool noCollisionsBetweenLinked = true);
    virtual void removeConstraint(const std::string& name);
    virtual ConstraintBase* createConstraint(RigidBodyBase* bodyA, const osg::Matrix& frameA,
                                             RigidBodyBase* bodyB, const osg::Matrix& frameB,
                                             ConstraintType type = CONSTRAINT_P2P,
                                             const ConstraintSetting* setting = NULL);
    virtual void setConstraintSetting(const std::string& name, const ConstraintSetting& setting);
    virtual float getConstraintAngle(const std::string& name);
    virtual float getConstraintTwistAngle(const std::string& name);

    // Applying force functions
    virtual void applyImpulse(const std::string& name, const osg::Vec3& point,
                                const osg::Vec3& impulse, bool wake = true, bool linearOrAngular = true);
    
    // get*() functions
    virtual float getInverseMass(const std::string& name);
    virtual osg::Matrix getInverseInertia(const std::string& name);
    virtual osg::Vec3 getCenterOfMass(const std::string& name);

    // Extended rigid-body functions
    virtual RigidBodyBase* createBody(const std::string& name, const BodySetting& setting,
                                      const osg::Matrix& matrix = osg::Matrix());
    virtual CollisionShapeBase* addShapeToBody(RigidBodyBase* body, CollisionShapeBase* shape,
                                               float mass, const ShapeSetting* setting = NULL);
    virtual void setShapeFriction(CollisionShapeBase* shape, float friction);
    virtual void setGravityScale(const std::string& name, float scale);
    virtual void setLinearDamping(const std::string& name, float damping);
    virtual void setBullet(const std::string& name, bool flag);
    virtual void setMassCenter(const std::string& name, const osg::Vec3& center);

    // Collision and raycast functions
    virtual bool raycast(const osg::Vec3& start, const osg::Vec3& end,
                         RaycastHit& result, const QueryFilter& filter = QueryFilter(), bool getNameFromBody = true);
    virtual std::vector<RaycastHit> raycastAll(const osg::Vec3& start, const osg::Vec3& end,
                                               const QueryFilter& filter = QueryFilter(), bool getNameFromBody = true);
    virtual SweepResult sweep(const osg::Vec3& start, const osg::Vec3& translation, CollisionShapeBase* shape,
                              const QueryFilter& filter = QueryFilter(), RigidBodyBase* ignoredBody = NULL);

    /* Physics creation functions */
    virtual CollisionShapeBase* createPhysicsPoint();  // for kinematic use only
    virtual CollisionShapeBase* createPhysicsBox(const osg::Vec3& halfSize);
    virtual CollisionShapeBase* createPhysicsCylinder(const osg::Vec3& halfSize);
    virtual CollisionShapeBase* createPhysicsCone(float radius, float height);
    virtual CollisionShapeBase* createPhysicsCapsule(float radius, float height);
    virtual CollisionShapeBase* createPhysicsSphere(float radius);
    virtual CollisionShapeBase* createPhysicsHull(osg::Node* node, bool optimized = true);
    virtual CollisionShapeBase* createPhysicsTriangleMesh(osg::Node* node, bool compressed = true);
    virtual CollisionShapeBase* createPhysicsHeightField(osg::HeightField* hf, bool filpQuad = false);
    virtual CollisionShapeBase* createPhysicsBox(const osg::Vec3& halfSize, const osg::Vec3& offset);
    virtual CollisionShapeBase* createPhysicsCapsule(float radius, const osg::Vec3& c0, const osg::Vec3& c1);
    
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
        btHelpers::RigidBody* container = static_cast<btHelpers::RigidBody*>(itr->second.get());
        btRigidBody* body = container->getBody();
        if (container->compound) { delete container->compound; container->compound = NULL; }
        for (size_t i = 0; i < container->ownedShapes.size(); ++i) delete container->ownedShapes[i];
        container->ownedShapes.clear();
        if (body->getMotionState()) delete body->getMotionState();
        PHY_WORLD()->removeCollisionObject(body); delete body;
    }
    for (std::map<std::string, osg::ref_ptr<CollisionShapeBase>>::iterator itr = _shapes.begin();
         itr != _shapes.end(); ++itr) { btHelpers::destroyShape((btHelpers::CollisionShape*)itr->second.get()); }
    _constraints.clear(); _shapes.clear(); _bodies.clear(); _core = NULL;
}

RigidBodyBase* BulletPhysicsEngine::createBody(const std::string& name, const BodySetting& setting,
                                               const osg::Matrix& matrix)
{
    osg::Quat q = matrix.getRotate();
    osg::Vec3 p = matrix.getTrans();
    if (_bodies.find(name) != _bodies.end()) removeBody(name);  // remove existing body

    btTransform transform; transform.setIdentity();
    transform.setOrigin(btVector3(p.x(), p.y(), p.z()));
    transform.setRotation(btQuaternion(q.x(), q.y(), q.z(), q.w()));
    btDefaultMotionState* motionState = new btDefaultMotionState(transform);

    // The body starts with an empty compound shape. A temporary mass is used here so that it is
    // not treated as a static body when being added to the world; the actual mass is set later
    // when shapes are attached by addShapeToBody().
    btCompoundShape* compound = new btCompoundShape;
    btRigidBody::btRigidBodyConstructionInfo rbInfo(1.0f, motionState, compound, btVector3(0, 0, 0));
    btRigidBody* body = new btRigidBody(rbInfo);
    body->setCollisionFlags(body->getCollisionFlags() & (~btCollisionObject::CF_STATIC_OBJECT));
    body->setDamping(setting.linearDamping, setting.angularDamping);
    body->setAngularFactor(btVector3(
        setting.lockAngularX ? 0.0f : 1.0f, setting.lockAngularY ? 0.0f : 1.0f,
        setting.lockAngularZ ? 0.0f : 1.0f));
    if (setting.kinematic)
    {
        body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        body->setActivationState(DISABLE_DEACTIVATION);
    }
    if (!setting.allowSleep) body->setActivationState(DISABLE_DEACTIVATION);

    btHelpers::RigidBody* container = new btHelpers::RigidBody(body);
    container->compound = compound; container->gravityScale = setting.gravityScale;
    _bodies[name] = container;
    PHY_WORLD()->addRigidBody(body);

    // The world sets the gravity of a body when it is added, so the scale must be applied after
    body->setGravity(PHY_WORLD()->getGravity() * setting.gravityScale);
    return container;
}

CollisionShapeBase* BulletPhysicsEngine::addShapeToBody(RigidBodyBase* body, CollisionShapeBase* csb,
                                                        float mass, const ShapeSetting* setting)
{
    btHelpers::RigidBody* container = static_cast<btHelpers::RigidBody*>(body);
    btHelpers::CollisionShape* cs = static_cast<btHelpers::CollisionShape*>(csb);
    if (!container || !cs || !container->compound)
    { OSG_NOTICE << "[PhysicsEngine] Failed to add shape to body\n"; return NULL; }

    // Bullet has no per-shape material and filtering, so they are set on the body as an approximation
    btRigidBody* btBody = container->getBody();
    btTransform local; local.setIdentity();
    local.setOrigin(btVector3(cs->offset[0], cs->offset[1], cs->offset[2]));
    container->compound->addChildShape(local, cs->getShape());
    container->ownedShapes.push_back(cs->getShape());
    container->totalMass += mass; cs->owner = btBody;

    btVector3 inertia(0.0f, 0.0f, 0.0f);
    if (container->totalMass > 0.0f)
        container->compound->calculateLocalInertia(container->totalMass, inertia);
    btBody->setMassProps(container->totalMass, inertia);
    btBody->updateInertiaTensor();

    // Bullet forgets the body gravity when the mass changes, so it is applied again here
    btBody->setGravity(PHY_WORLD()->getGravity() * container->gravityScale);
    btHelpers::applyShapeSetting(btBody, setting);
    btHelpers::applyCollisionGroup(PHY_WORLD(), container, setting ? setting->collisionGroup : 0);
    PHY_WORLD()->updateAabbs(); return cs;
}

void BulletPhysicsEngine::setShapeFriction(CollisionShapeBase* shape, float friction)
{
    btHelpers::CollisionShape* cs = static_cast<btHelpers::CollisionShape*>(shape);
    if (cs && cs->owner) cs->owner->setFriction(friction);
}

void BulletPhysicsEngine::setGravityScale(const std::string& name, float scale)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        btHelpers::RigidBody* container = static_cast<btHelpers::RigidBody*>(itr->second.get());
        container->gravityScale = scale;
        container->getBody()->setGravity(PHY_WORLD()->getGravity() * scale);
    }
}

void BulletPhysicsEngine::setLinearDamping(const std::string& name, float damping)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        btRigidBody* body = static_cast<btHelpers::RigidBody*>(itr->second.get())->getBody();
        body->setDamping(damping, body->getAngularDamping());
    }
}

void BulletPhysicsEngine::setBullet(const std::string& name, bool flag)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr == _bodies.end()) return;
    btRigidBody* body = static_cast<btHelpers::RigidBody*>(itr->second.get())->getBody();
    if (flag)
    {
        // Bullet uses continuous collision detection instead of "bullet" bodies. The swept sphere
        // radius is estimated from the body bounds, and may need tuning for very thin bodies
        btVector3 minBound, maxBound;
        body->getCollisionShape()->getAabb(body->getWorldTransform(), minBound, maxBound);
        btVector3 extent = (maxBound - minBound) * 0.5f;
        body->setCcdMotionThreshold(0.01f);
        body->setCcdSweptSphereRadius(0.25f * btMin(extent.x(), btMin(extent.y(), extent.z())));
    }
    else
    {
        body->setCcdMotionThreshold(0.0f);
        body->setCcdSweptSphereRadius(0.0f);
    }
}

void BulletPhysicsEngine::setMassCenter(const std::string& name, const osg::Vec3& center)
{
    // Bullet always uses the shape based center of mass and provides no way to offset it
    if (center.length2() > 0.0f) btHelpers::warnUnsupported("local mass center offsets");
}

RigidBodyBase* BulletPhysicsEngine::addRigidBody(const std::string& name, CollisionShapeBase* csb, float mass,
                                                 const osg::Matrix& matrix, bool kinematic,
                                                 const ShapeSetting* setting)
{
    bool isDynamic = (mass > 0.0f);
    osg::Quat q = matrix.getRotate();
    osg::Vec3 p = matrix.getTrans();
    if (_shapes.find(name) != _shapes.end()) removeBody(name);  // remove existing shape

    btTransform transform; transform.setIdentity();
    transform.setOrigin(btVector3(p.x(), p.y(), p.z()));
    transform.setRotation(btQuaternion(q.x(), q.y(), q.z(), q.w()));
    btHelpers::CollisionShape* cs = static_cast<btHelpers::CollisionShape*>(csb);
    btCollisionShape* shape = cs ? btHelpers::getBodyShape(cs) : NULL;
    if (!shape) { OSG_NOTICE << "[PhysicsEngine] Failed to get input shape\n"; return NULL; }

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
    PHY_WORLD()->addRigidBody(body);
    btHelpers::applyShapeSetting(body, setting);
    btHelpers::applyCollisionGroup(PHY_WORLD(), container, setting ? setting->collisionGroup : 0);
    return container;
}

void BulletPhysicsEngine::removeBody(const std::string& name)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        btHelpers::RigidBody* container = static_cast<btHelpers::RigidBody*>(itr->second.get());
        btRigidBody* body = container->getBody();
        if (container->compound) { delete container->compound; container->compound = NULL; }
        for (size_t i = 0; i < container->ownedShapes.size(); ++i) delete container->ownedShapes[i];
        container->ownedShapes.clear();
        if (body->getMotionState()) delete body->getMotionState();
        PHY_WORLD()->removeCollisionObject(body);
        _bodies.erase(itr);  // handle deleted by itself if no other reference exists
    }

    std::map<std::string, osg::ref_ptr<CollisionShapeBase>>::iterator itr2 = _shapes.find(name);
    if (itr2 != _shapes.end())
    { btHelpers::destroyShape((btHelpers::CollisionShape*)itr2->second.get()); _shapes.erase(itr2); }
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
        // NOTE: the motion state is interpolated and may lag behind one physics step, so the
        // world transform of the body is used here instead to report the accurate pose
        btRigidBody* body = itr->second->get<btRigidBody>();
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
        if (!body->isActive()) body->activate();  // setting a velocity wakes a sleeping body
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

void BulletPhysicsEngine::applyImpulse(const std::string& name, const osg::Vec3& point,
                                       const osg::Vec3& impulse, bool wake, bool linearOrAngular)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        btRigidBody* body = itr->second->get<btRigidBody>();
        if (body->isStaticOrKinematicObject()) return;

        btVector3 btImpulse(impulse[0], impulse[1], impulse[2]);
        if (linearOrAngular)
        {
            btVector3 btPoint(point[0], point[1], point[2]);
            btVector3 relPos = btPoint - body->getCenterOfMassPosition();
            body->applyImpulse(btImpulse, relPos);
        }
        else
            body->applyTorqueImpulse(btImpulse);  // Bullet names angular impulse as torque impulse
        if (wake && !body->isActive()) body->activate();
    }
}

float BulletPhysicsEngine::getInverseMass(const std::string& name)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        btRigidBody* body = itr->second->get<btRigidBody>();
        return body->getInvMass();
    }
    return 0.0f;
}

osg::Matrix BulletPhysicsEngine::getInverseInertia(const std::string& name)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        btRigidBody* body = itr->second->get<btRigidBody>();
        btMatrix3x3 m = body->getInvInertiaTensorWorld();
        return osg::Matrix(m[0][0], m[0][1], m[0][2], 0.0f, m[1][0], m[1][1], m[1][2], 0.0f,
                           m[2][0], m[2][1], m[2][2], 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    }
    return osg::Matrix();
}

osg::Vec3 BulletPhysicsEngine::getCenterOfMass(const std::string& name)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        btRigidBody* body = itr->second->get<btRigidBody>();
        btVector3 c = body->getCenterOfMassPosition(); return osg::Vec3(c[0], c[1], c[2]);
    }
    return osg::Vec3();
}

bool BulletPhysicsEngine::raycast(const osg::Vec3& s, const osg::Vec3& e,
                                  RaycastHit& result, const QueryFilter& f, bool getNameFromBody)
{
    btVector3 from(s.x(), s.y(), s.z()), to(e.x(), e.y(), e.z());
    btCollisionWorld::ClosestRayResultCallback rayCallback(from, to);
    //rayCallback.m_flags |= btTriangleRaycastCallback::kF_UseGjkConvexCastRaytest;
    rayCallback.m_collisionFilterMask = f.maskBits;
    rayCallback.m_collisionFilterGroup = f.categoryBits;

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
        const osg::Vec3& s, const osg::Vec3& e, const QueryFilter& f, bool getNameFromBody)
{
    btVector3 from(s.x(), s.y(), s.z()), to(e.x(), e.y(), e.z());
    btCollisionWorld::AllHitsRayResultCallback rayCallback(from, to);
    //rayCallback.m_flags |= btTriangleRaycastCallback::kF_UseGjkConvexCastRaytest;
    rayCallback.m_collisionFilterMask = f.maskBits;
    rayCallback.m_collisionFilterGroup = f.categoryBits;

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

BulletPhysicsEngine::SweepResult BulletPhysicsEngine::sweep(const osg::Vec3& s, const osg::Vec3& t,
                                                            CollisionShapeBase* csb, const QueryFilter& f,
                                                            RigidBodyBase* ignoredBody)
{
    SweepResult result;
    btHelpers::CollisionShape* cs = static_cast<btHelpers::CollisionShape*>(csb);
    btConvexShape* shape = cs ? dynamic_cast<btConvexShape*>(cs->getShape()) : NULL;
    if (!shape)
    { OSG_NOTICE << "[PhysicsEngine] Unsupported shape for sweeping\n"; return result; }

    // Shape geometry may be offset from the shape origin, so shift the trace instead
    btVector3 offset(cs->offset[0], cs->offset[1], cs->offset[2]);
    btVector3 from(s.x(), s.y(), s.z()), to(s.x() + t.x(), s.y() + t.y(), s.z() + t.z());
    btTransform fromTrans, toTrans; fromTrans.setIdentity(); toTrans.setIdentity();
    fromTrans.setOrigin(from + offset); toTrans.setOrigin(to + offset);

    btCollisionObject* ignored = ignoredBody ? ignoredBody->get<btRigidBody>() : NULL;
    btHelpers::SweepResultCallback callback(from + offset, to + offset, ignored);
    callback.m_collisionFilterGroup = f.categoryBits;
    callback.m_collisionFilterMask = f.maskBits;
    PHY_WORLD()->convexSweepTest(shape, fromTrans, toTrans, callback);

    if (callback.hasHit())
    {
        result.hit = true; result.fraction = callback.m_closestHitFraction;
        result.point = osg::Vec3(callback.m_hitPointWorld.x(), callback.m_hitPointWorld.y(),
                                 callback.m_hitPointWorld.z()) - cs->offset;
        result.normal = osg::Vec3(callback.m_hitNormalWorld.x(), callback.m_hitNormalWorld.y(),
                                  callback.m_hitNormalWorld.z());
        result.startedSolid = (callback.m_closestHitFraction <= 0.0f);

        const btRigidBody* hitBody = callback.m_hitCollisionObject ?
            btRigidBody::upcast(callback.m_hitCollisionObject) : NULL;
        if (hitBody)
        {
            result.rigidBody = new btHelpers::RigidBody(const_cast<btRigidBody*>(hitBody));
            for (std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator
                 itr = _bodies.begin(); itr != _bodies.end(); ++itr)
            { if (itr->second->equals(result.rigidBody)) { result.name = itr->first; break; } }
        }
    }
    return result;
}

CollisionShapeBase* BulletPhysicsEngine::createPhysicsPoint()
{ return new btHelpers::CollisionShape(new btEmptyShape()); }

CollisionShapeBase* BulletPhysicsEngine::createPhysicsBox(const osg::Vec3& halfSize)
{ return new btHelpers::CollisionShape(new btBoxShape(btVector3(halfSize[0], halfSize[1], halfSize[2]))); }

CollisionShapeBase* BulletPhysicsEngine::createPhysicsBox(const osg::Vec3& halfSize, const osg::Vec3& offset)
{
    return new btHelpers::CollisionShape(
        new btBoxShape(btVector3(halfSize[0], halfSize[1], halfSize[2])), offset);
}

CollisionShapeBase* BulletPhysicsEngine::createPhysicsCylinder(const osg::Vec3& halfSize)
{ return new btHelpers::CollisionShape(new btCylinderShape(btVector3(halfSize[0], halfSize[1], halfSize[2]))); }

CollisionShapeBase* BulletPhysicsEngine::createPhysicsCone(float radius, float height)
{ return new btHelpers::CollisionShape(new btConeShape(radius, height)); }

CollisionShapeBase* BulletPhysicsEngine::createPhysicsCapsule(float radius, float height)
{ return new btHelpers::CollisionShape(new btCapsuleShape(radius, height)); }

CollisionShapeBase* BulletPhysicsEngine::createPhysicsCapsule(float radius, const osg::Vec3& c0, const osg::Vec3& c1)
{
    osg::Vec3 axis = c1 - c0, center = (c0 + c1) * 0.5f;
    if (fabsf(axis.x()) > 1e-4f || fabsf(axis.y()) > 1e-4f)
        OSG_NOTICE << "[PhysicsEngine] Bullet only supports Z-aligned capsules, "
                   << "the given capsule will be treated as a vertical one\n";
    return new btHelpers::CollisionShape(new btCapsuleShapeZ(radius, axis.length()), center);
}

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

ConstraintBase* BulletPhysicsEngine::createConstraint(RigidBodyBase* bodyA, const osg::Matrix& frameA,
                                                      RigidBodyBase* bodyB, const osg::Matrix& frameB,
                                                      ConstraintType type, const ConstraintSetting* setting)
{
    if (!bodyA || !bodyB) return NULL;
    btRigidBody *btA = bodyA->get<btRigidBody>(), *btB = bodyB->get<btRigidBody>();
    if (!btA || !btB) return NULL;

    ConstraintSetting cs; if (setting) cs = *setting;
    btTransform tA = btHelpers::toBtTransform(frameA), tB = btHelpers::toBtTransform(frameB);
    if (cs.useWorldPivots)
    {
        // Constraint frames are related to the centers of mass of the two bodies
        tA = btA->getCenterOfMassTransform().inverse() * tA;
        tB = btB->getCenterOfMassTransform().inverse() * tB;
    }
    btTypedConstraint* constraint = NULL;

    switch (type)
    {
    case CONSTRAINT_HINGE:
        {
            btHingeConstraint* hinge = new btHingeConstraint(*btA, *btB, tA, tB, false);
            // Bullet measures the hinge angle in the opposite direction of Box3D, so the limits are
            // mirrored and the motor speed is negated to keep both backends behaving the same
            if (cs.enableLimit) hinge->setLimit(-cs.upperLimit, -cs.lowerLimit);
            if (cs.enableSpring)
            {
                // Bullet has no hinge spring: the joint is driven to its target angle by the motor,
                // which is set as a velocity and therefore has to be updated in every step
                btScalar impulse = btHelpers::motorImpulse(cs);
                if (impulse > 0.0f)
                {
                    hinge->enableAngularMotor(true, 0.0f, impulse);
                    hinge->setMotorTarget(-cs.targetAngle, btHelpers::motorTimeStep);
                }
                else btHelpers::warnUnsupported("driving joints without a torque limit");
                btHelpers::warnUnsupported("spring stiffness of hinge joints, a motor is used instead");
            }
            else if (cs.enableMotor)
                hinge->enableAngularMotor(true, -cs.motorSpeed, cs.maxMotorTorque * btHelpers::motorTimeStep);
            constraint = hinge;
        }
        break;
    case CONSTRAINT_CONE_TWIST:
        {
            btConeTwistConstraint* cone = new btConeTwistConstraint(*btA, *btB, tA, tB);
            if (cs.enableLimit)
            {
                if (btFabs(cs.lowerLimit + cs.upperLimit) > 0.001f)
                    btHelpers::warnUnsupported("asymmetric twist limits, symmetric ones are used");
                float twistSpan = btMax(btFabs(cs.lowerLimit), btFabs(cs.upperLimit));
                cone->setLimit(cs.coneLimit, cs.coneLimit, twistSpan);
            }
            if (cs.enableSpring)
            {
                /* Bullet has no spring on cone-twist joints and the motor of its cone-twist
                   constraint can only drive the twist of the joint, so the spring is approximated
                   by the motor: the rotation of the joint is driven by the impulse of the target */
                btScalar impulse = btHelpers::motorImpulse(cs);
                if (impulse > 0.0f)
                {
                    cone->enableMotor(true); cone->setMaxMotorImpulse(impulse);
                    cone->setMotorTargetInConstraintSpace(btHelpers::toBtQuaternion(cs.targetRotation));
                }
                else btHelpers::warnUnsupported("driving joints without a torque limit");
                btHelpers::warnUnsupported("spring stiffness of cone-twist joints, a motor is used instead");
            }
            if (cs.enableMotor) btHelpers::warnUnsupported("motors of cone-twist constraints");
            constraint = cone;
        }
        break;
    case CONSTRAINT_PARALLEL:
        {
            // Bullet has no joint to align body frames, so a 6DOF spring constraint with free axes
            // and spring driven rotations is used as an approximation. A parallel joint only aligns
            // the z axes, so its twist (the 3rd rotation) stays free.
            btGeneric6DofSpringConstraint* spring = new btGeneric6DofSpringConstraint(*btA, *btB, tA, tB, false);
            for (int i = 0; i < 6; ++i)
            {
                spring->setLimit(i, 1.0f, -1.0f);  // all translations and rotations are free
                if (i == 3 || i == 4)
                {
                    spring->enableSpring(i, true);
                    spring->setStiffness(i, btHelpers::springStiffness(btB, i, cs.hertz));
                    spring->setDamping(i, btHelpers::springDamping(cs.dampingRatio));
                }
            }
            // The parallel joint always drives the z axes of the two joint frames to be aligned
            spring->setEquilibriumPoint(3, 0.0f); spring->setEquilibriumPoint(4, 0.0f);
            if (cs.maxSpringForce > 0.0f)
                btHelpers::warnUnsupported("maximum spring force / torque limits of joints");
            constraint = spring;
        }
        break;
    case CONSTRAINT_MOTOR:
        {
            // Bullet has no motor joint to drive a body to the pose of a kinematic anchor, and an
            // approximation with a 6DOF spring makes the driven body jitter all the time, so the
            // joint is not emulated at all: an empty constraint only keeps the linked bodies from
            // colliding with each other, which is what a Box3D motor joint does as well
            btHelpers::warnUnsupported("motor joints, an empty constraint is used instead");
            constraint = btHelpers::createEmptyConstraint(btA, btB, tA, tB);
        }
        break;
    case CONSTRAINT_FILTER:
        {
            // Bullet has no filter joint: an empty constraint is created, which only disables the
            // collisions between the two linked bodies just like a real filter joint does
            btHelpers::warnUnsupported("filter joints, an empty constraint is used instead");
            constraint = btHelpers::createEmptyConstraint(btA, btB, tA, tB);
        }
        break;
    default:  // CONSTRAINT_P2P
        {
            btPoint2PointConstraint* p2p = new btPoint2PointConstraint(
                *btA, *btB, tA.getOrigin(), tB.getOrigin());
            p2p->m_setting.m_tau = cs.tau;
            p2p->m_setting.m_damping = cs.damping;
            p2p->m_setting.m_impulseClamp = cs.impulseClamp;
            if (cs.enableLimit || cs.enableSpring || cs.enableMotor)
                btHelpers::warnUnsupported("limits, springs and motors of point-to-point constraints");
            constraint = p2p;
        }
        break;
    }
    if (!constraint) return NULL;
    return new btHelpers::TypedConstraint(constraint, type);
}

void BulletPhysicsEngine::setConstraintSetting(const std::string& name, const ConstraintSetting& cs)
{
    std::map<std::string, ConstraintAndState>::iterator itr = _constraints.find(name);
    if (itr == _constraints.end()) return;
    btTypedConstraint* constraint = itr->second.first->get<btTypedConstraint>();
    if (!constraint) return;

    switch (constraint->getConstraintType())
    {
    case HINGE_CONSTRAINT_TYPE:
        {
            btHingeConstraint* hinge = static_cast<btHingeConstraint*>(constraint);
            if (cs.enableLimit) hinge->setLimit(-cs.upperLimit, -cs.lowerLimit);
            else hinge->setLimit(1.0f, -1.0f);  // higher lower-limit means no limit
            if (cs.enableSpring)
            {
                // Drives the hinge to the target angle, see the note in createConstraint()
                btScalar impulse = btHelpers::motorImpulse(cs);
                if (impulse > 0.0f)
                {
                    hinge->enableAngularMotor(true, 0.0f, impulse);
                    hinge->setMotorTarget(-cs.targetAngle, btHelpers::motorTimeStep);
                }
                else btHelpers::warnUnsupported("driving joints without a torque limit");
            }
            else
                hinge->enableAngularMotor(cs.enableMotor, -cs.motorSpeed,
                                          cs.maxMotorTorque * btHelpers::motorTimeStep);
        }
        break;
    case CONETWIST_CONSTRAINT_TYPE:
        {
            btConeTwistConstraint* cone = static_cast<btConeTwistConstraint*>(constraint);
            if (cs.enableLimit)
            {
                float twistSpan = btMax(btFabs(cs.lowerLimit), btFabs(cs.upperLimit));
                cone->setLimit(cs.coneLimit, cs.coneLimit, twistSpan);
            }
            else
                cone->setLimit(SIMD_PI, SIMD_PI, SIMD_PI);  // spans are clamped, so this is free enough
            if (cs.enableSpring)
            {
                /* The motor of a cone-twist constraint can only drive its twist, so a joint which
                   has to be driven is created as a 6DOF spring constraint instead (see the note in
                   createConstraint()); this only happens for joints which already exist */
                btScalar impulse = btHelpers::motorImpulse(cs);
                if (impulse > 0.0f)
                {
                    cone->enableMotor(true); cone->setMaxMotorImpulse(impulse);
                    cone->setMotorTargetInConstraintSpace(btHelpers::toBtQuaternion(cs.targetRotation));
                }
                else btHelpers::warnUnsupported("driving joints without a torque limit");
                btHelpers::warnUnsupported("springs of existing cone-twist joints, only their twist "
                                           "is driven by a motor");
            }
            else
                cone->enableMotor(false);
            if (cs.enableMotor) btHelpers::warnUnsupported("motors of cone-twist constraints");
        }
        break;
    case D6_SPRING_CONSTRAINT_TYPE:
        {
            // The parallel joint is the only one using this spring constraint: it always drives the
            // z axes of the two joint frames to be aligned, while its twist stays free
            btGeneric6DofSpringConstraint* spring = static_cast<btGeneric6DofSpringConstraint*>(constraint);
            for (int i = 0; i < 6; ++i)
            {
                bool enabled = (i == 3 || i == 4);  // the parallel joint always uses these springs
                spring->enableSpring(i, enabled);
                if (enabled)
                {
                    spring->setStiffness(i, btHelpers::springStiffness(&spring->getRigidBodyB(), i, cs.hertz));
                    spring->setDamping(i, btHelpers::springDamping(cs.dampingRatio));
                }
            }
            spring->setEquilibriumPoint(3, 0.0f);
            spring->setEquilibriumPoint(4, 0.0f);
            if (cs.maxSpringForce > 0.0f)
                btHelpers::warnUnsupported("maximum spring force / torque limits of joints");
        }
        break;
    default:
        {
            btHelpers::TypedConstraint* container = static_cast<btHelpers::TypedConstraint*>(itr->second.first.get());
            // A filter joint and a motor joint have no parameter in Bullet
            if (container->type == CONSTRAINT_FILTER || container->type == CONSTRAINT_MOTOR) break;
            if (cs.enableLimit || cs.enableSpring || cs.enableMotor)
                btHelpers::warnUnsupported("limits, springs and motors of point-to-point constraints");
        }
        break;
    }
    if (cs.collideConnected)
        btHelpers::warnUnsupported("changing collision states of linked bodies, use addConstraint() instead");
}

float BulletPhysicsEngine::getConstraintAngle(const std::string& name)
{
    std::map<std::string, ConstraintAndState>::iterator itr = _constraints.find(name);
    if (itr == _constraints.end()) return 0.0f;
    btTypedConstraint* constraint = itr->second.first->get<btTypedConstraint>();
    if (!constraint) return 0.0f;

    switch (constraint->getConstraintType())
    {
    case HINGE_CONSTRAINT_TYPE:
        {
            // Bullet measures the hinge angle in the opposite direction of Box3D
            btHingeConstraint* hinge = static_cast<btHingeConstraint*>(constraint);
            return -(float)hinge->getHingeAngle();
        }
    case CONETWIST_CONSTRAINT_TYPE:
        {
            // Box3D's cone angle is the angle between the z axes of the two joint frames
            btConeTwistConstraint* cone = static_cast<btConeTwistConstraint*>(constraint);
            btVector3 axisA = (cone->getRigidBodyA().getCenterOfMassTransform()
                               * cone->getFrameOffsetA()).getBasis().getColumn(2);
            btVector3 axisB = (cone->getRigidBodyB().getCenterOfMassTransform()
                               * cone->getFrameOffsetB()).getBasis().getColumn(2);
            btScalar dot = btMin(btMax(axisA.dot(axisB), btScalar(-1.0f)), btScalar(1.0f));
            return (float)btAcos(dot);
        }
    default: return 0.0f;  // other constraints have no angle
    }
}

float BulletPhysicsEngine::getConstraintTwistAngle(const std::string& name)
{
    std::map<std::string, ConstraintAndState>::iterator itr = _constraints.find(name);
    if (itr == _constraints.end()) return 0.0f;
    btTypedConstraint* constraint = itr->second.first->get<btTypedConstraint>();
    if (!constraint) return 0.0f;
    if (constraint->getConstraintType() != CONETWIST_CONSTRAINT_TYPE) return 0.0f;

    // Bullet only updates its own twist angle while solving, so it is computed the Box3D way:
    // the twist is the rotation about the z axis of joint frame B related to joint frame A
    btConeTwistConstraint* cone = static_cast<btConeTwistConstraint*>(constraint);
    btQuaternion quatA = cone->getRigidBodyA().getCenterOfMassTransform().getRotation()
                       * cone->getFrameOffsetA().getRotation();
    btQuaternion quatB = cone->getRigidBodyB().getCenterOfMassTransform().getRotation()
                       * cone->getFrameOffsetB().getRotation();
    btQuaternion relQ = quatA.inverse() * quatB;

    // Account for polarity to keep the twist angle in the range of [-pi, pi]
    float twist = (relQ.getW() < 0.0f) ? btAtan2(-relQ.getZ(), -relQ.getW())
                                       : btAtan2(relQ.getZ(), relQ.getW());
    return 2.0f * twist;
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
