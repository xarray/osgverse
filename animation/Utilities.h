#ifndef MANA_ANIM_UTILITIES_HPP
#define MANA_ANIM_UTILITIES_HPP

#include <osg/Version>
#include <osg/Texture2D>
#include <osg/Shape>
#include <osg/Geometry>
#include "PhysicsEngine.h"

class btCollisionShape;
class btRigidBody;
class btTypedConstraint;

namespace osgVerse
{

    class PhysicsUpdateCallback : public osg::NodeCallback
    {
    public:
        PhysicsUpdateCallback(PhysicsEngine* e, const std::string& n);
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

    protected:
        osg::observer_ptr<PhysicsEngine> _engine;
        std::string _bodyName;
    };

    extern btCollisionShape* createPhysicsPoint();  // for kinematic use only
    extern btCollisionShape* createPhysicsBox(const osg::Vec3& halfSize);
    extern btCollisionShape* createPhysicsCylinder(const osg::Vec3& halfSize);
    extern btCollisionShape* createPhysicsCone(float radius, float height);
    extern btCollisionShape* createPhysicsSphere(float radius);
    extern btCollisionShape* createPhysicsHull(osg::Node* node, bool optimized = true);
    extern btCollisionShape* createPhysicsTriangleMesh(osg::Node* node, bool compressed = true);
    extern btCollisionShape* createPhysicsHeightField(osg::HeightField* hf, bool filpQuad = false);

    struct ConstraintSetting
    {
        ConstraintSetting() : tau(0.3f), damping(1.0f),
                              impulseClamp(0.0f), useWorldPivots(false) {}
        float tau, damping, impulseClamp; bool useWorldPivots;
    };
    extern btTypedConstraint* createConstraintP2P(btRigidBody* bodyA, const osg::Vec3& pivotA,
                                                  btRigidBody* bodyB, const osg::Vec3& pivotB,
                                                  const ConstraintSetting* setting = NULL);

}

#endif
