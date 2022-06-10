#include <osg/io_utils>
#include <osg/Version>
#include <osg/Notify>
#include <osg/Geometry>
#include <osg/Geode>
#include <osgDB/ReadFile>
#include <osgUtil/SmoothingVisitor>

#include <btBulletDynamicsCommon.h>
#include "PhysicsEngine.h"
using namespace osgVerse;

PhysicsEngine::PhysicsEngine()
{
    // FIXME: use a parallel processing dispatcher?
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

PhysicsEngine::~PhysicsEngine()
{
    for (std::map<std::string, btRigidBody*>::iterator itr = _bodies.begin();
         itr != _bodies.end(); ++itr)
    {
        if (itr->second->getMotionState()) delete itr->second->getMotionState();
        _world->removeCollisionObject(itr->second);
        delete itr->second;
    }
    for (std::map<std::string, btCollisionShape*>::iterator itr = _shapes.begin();
         itr != _shapes.end(); ++itr) { delete itr->second; }

    delete _world; delete _solver;
    delete _overlappingPairCache;
    delete _collisionDispatcher;
    delete _collisionCfg;
}

btRigidBody* PhysicsEngine::addRigidBody(const std::string& name, btCollisionShape* shape, float mass,
                                         const osg::Matrix& matrix, bool kinematic)
{
    bool isDynamic = (mass > 0.0f);
    osg::Quat q = matrix.getRotate();
    osg::Vec3 p = matrix.getTrans();
    if (_shapes.find(name) != _shapes.end()) removeBody(name);  // remove existing shape

    btTransform transform; transform.setIdentity();
    transform.setOrigin(btVector3(p.x(), p.y(), p.z()));
    transform.setRotation(btQuaternion(q.x(), q.y(), q.z(), q.w()));

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

    _world->addRigidBody(body);
    _shapes[name] = shape; _bodies[name] = body;
    return body;
}

void PhysicsEngine::removeBody(const std::string& name)
{
    std::map<std::string, btRigidBody*>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        if (itr->second->getMotionState()) delete itr->second->getMotionState();
        _world->removeCollisionObject(itr->second);
        delete itr->second; _bodies.erase(itr);
    }

    std::map<std::string, btCollisionShape*>::iterator itr2 = _shapes.find(name);
    if (itr2 != _shapes.end()) { delete itr2->second; _shapes.erase(itr2); }
}

void PhysicsEngine::setTransform(const std::string& name, const osg::Matrix& matrix)
{
    std::map<std::string, btRigidBody*>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        osg::Quat q = matrix.getRotate();
        osg::Vec3 p = matrix.getTrans();

        btTransform transform; transform.setIdentity();
        transform.setOrigin(btVector3(p.x(), p.y(), p.z()));
        transform.setRotation(btQuaternion(q.x(), q.y(), q.z(), q.w()));

        btRigidBody* body = itr->second;
        if (body->getMotionState())
            body->getMotionState()->setWorldTransform(transform);
        else
            body->setWorldTransform(transform);
    }
}

osg::Matrix PhysicsEngine::getTransform(const std::string& name, bool& valid)
{
    std::map<std::string, btRigidBody*>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        btTransform transform; valid = true;
        btRigidBody* body = itr->second;
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

void PhysicsEngine::setVelocity(const std::string& name, const osg::Vec3& v, bool linearOrAngular)
{
    std::map<std::string, btRigidBody*>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        btRigidBody* body = itr->second;
        if (linearOrAngular) body->setLinearVelocity(btVector3(v[0], v[1], v[2]));
        else body->setAngularVelocity(btVector3(v[0], v[1], v[2]));
    }
}

osg::Vec3 PhysicsEngine::getVelocity(const std::string& name, bool linearOrAngular)
{
    std::map<std::string, btRigidBody*>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        btRigidBody* body = itr->second; btVector3 vel;
        if (linearOrAngular) vel = body->getLinearVelocity();
        else vel = body->getAngularVelocity();
        return osg::Vec3(vel.x(), vel.y(), vel.z());
    }
    return osg::Vec3();
}

btCollisionShape* PhysicsEngine::getShape(const std::string& name)
{
    if (_shapes.find(name) == _shapes.end()) return NULL;
    return _shapes[name];
}

btRigidBody* PhysicsEngine::getRigidBody(const std::string& name)
{
    if (_bodies.find(name) == _bodies.end()) return NULL;
    return _bodies[name];
}

void PhysicsEngine::setGravity(const osg::Vec3& gravity)
{ _world->setGravity(btVector3(gravity[0], gravity[1], gravity[2])); }

void PhysicsEngine::advance(float timeStep, int maxSubSteps)
{ _world->stepSimulation(timeStep, maxSubSteps); }
