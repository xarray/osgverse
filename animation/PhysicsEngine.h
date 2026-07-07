#ifndef MANA_ANIM_PHYSICSENGINE_HPP
#define MANA_ANIM_PHYSICSENGINE_HPP

#include <osg/Version>
#include <osg/MatrixTransform>
#include <map>

namespace osgVerse
{
    struct PhysicsItemBase : public osg::Referenced
    {
        template<typename T> T* get() { return (T*)internal; }
        template<typename T> const T* get() const { return (const T*)internal; }
        bool equals(PhysicsItemBase* item) const { return internal == item->internal; }
        virtual ~PhysicsItemBase() {} void* internal;
    };
    struct CollisionShapeBase : public PhysicsItemBase {};
    struct RigidBodyBase : public PhysicsItemBase {};
    struct ConstraintBase : public PhysicsItemBase {};
    class PhysicsCoreBase : public osg::Referenced {};

    /** The default physics engine */
    class PhysicsEngine : public osg::Object
    {
    public:
        PhysicsEngine();
        PhysicsEngine(const PhysicsEngine& copy, const osg::CopyOp& op = osg::CopyOp::SHALLOW_COPY);
        META_Object(osgVerse, PhysicsEngine)

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

        // get*() functions
        CollisionShapeBase* getShape(const std::string& name);
        RigidBodyBase* getRigidBody(const std::string& name);
        ConstraintBase* getConstraint(const std::string& name);

        typedef std::pair<osg::ref_ptr<ConstraintBase>, int> ConstraintAndState;
        const std::map<std::string, ConstraintAndState>& getConstraints() const { return _constraints; }
        const std::map<std::string, osg::ref_ptr<CollisionShapeBase>>& getShapes() const { return _shapes; }
        const std::map<std::string, osg::ref_ptr<RigidBodyBase>>& getBodies() const { return _bodies; }

        virtual void setGravity(const osg::Vec3& gravity);
        virtual osg::Vec3 getGravity() const;

        // Raycast functions
        struct RaycastHit
        {
            osg::ref_ptr<RigidBodyBase> rigidBody;
            osg::Vec3 position, normal; std::string name;
            RaycastHit() : rigidBody(NULL) {}
        };
        virtual bool raycast(const osg::Vec3& start, const osg::Vec3& end,
                             RaycastHit& result, bool getNameFromBody = true);
        virtual std::vector<RaycastHit> raycastAll(const osg::Vec3& start, const osg::Vec3& end,
                                                   bool getNameFromBody = true);

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

        struct ConstraintSetting
        {
            ConstraintSetting() : tau(0.3f), damping(1.0f), impulseClamp(0.0f), useWorldPivots(false) {}
            float tau, damping, impulseClamp; bool useWorldPivots;
        };
        virtual ConstraintBase* createConstraintP2P(RigidBodyBase* bodyA, const osg::Vec3& pivotA,
                                                    RigidBodyBase* bodyB, const osg::Vec3& pivotB,
                                                    const ConstraintSetting* setting = NULL);
        
        // Advance the world
        virtual void advance(float timeStep, int maxSubSteps = 1);

        PhysicsCoreBase* core() { return _core.get(); }
        const PhysicsCoreBase* core() const { return _core.get(); }

    protected:
        virtual ~PhysicsEngine();

        std::map<std::string, ConstraintAndState> _constraints;
        std::map<std::string, osg::ref_ptr<CollisionShapeBase>> _shapes;
        std::map<std::string, osg::ref_ptr<RigidBodyBase>> _bodies;
        osg::ref_ptr<PhysicsCoreBase> _core;
    };
}

#endif
