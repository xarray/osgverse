#ifndef MANA_ANIM_PHYSICSENGINE_HPP
#define MANA_ANIM_PHYSICSENGINE_HPP

#include <osg/Version>
#include <osg/MatrixTransform>
#include <osg/observer_ptr>
#include <osg/Shape>
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

        /** Body settings for creating compound bodies (see createBody() and addShapeToBody()) */
        struct BodySetting
        {
            BodySetting()
            :   kinematic(false), allowSleep(true), enableContactRecycling(true),
                lockAngularX(false), lockAngularY(false), lockAngularZ(false),
                gravityScale(1.0f), linearDamping(0.0f), angularDamping(0.0f) {}
            bool kinematic, allowSleep, enableContactRecycling;
            bool lockAngularX, lockAngularY, lockAngularZ;
            float gravityScale, linearDamping, angularDamping;
        };

        /** Shape settings for the collision material and filtering */
        struct ShapeSetting
        {
            ShapeSetting() : friction(0.6f), restitution(0.0f), rollingResistance(0.0f), collisionGroup(0) {}
            float friction, restitution, rollingResistance;
            int collisionGroup;  // shapes with the same non-zero group won't collide with each other
        };

        /** Constraint types used by createConstraint() */
        enum ConstraintType
        {
            CONSTRAINT_P2P,         // point-to-point joint without limits
            CONSTRAINT_HINGE,       // revolute joint: one rotation axis with limit, spring and motor
            CONSTRAINT_CONE_TWIST,  // spherical joint with cone and twist limits
            CONSTRAINT_PARALLEL,    // angular spring which keeps two bodies aligned
            CONSTRAINT_MOTOR,       // spring which drives a body to a kinematic anchor's pose
            CONSTRAINT_FILTER       // only disables collision between the two linked bodies
        };

        /** Constraint settings shared by createConstraint() and setConstraintSetting(). Limits and
            motor speeds are in radians (per second), motor torque and spring force limits are in
            newton-meters / newtons. */
        struct ConstraintSetting
        {
            ConstraintSetting()
            :   tau(0.3f), damping(1.0f), impulseClamp(0.0f), useWorldPivots(false),
                enableLimit(false), lowerLimit(0.0f), upperLimit(0.0f), coneLimit(0.0f),
                enableSpring(false), hertz(0.0f), dampingRatio(0.7f),
                enableMotor(false), motorSpeed(0.0f), maxMotorTorque(0.0f), maxSpringForce(0.0f),
                collideConnected(false) {}
            float tau, damping, impulseClamp; bool useWorldPivots;  // tau/damping/clamp are P2P only
            bool enableLimit; float lowerLimit, upperLimit;         // hinge or twist angle range
            float coneLimit;                                        // cone angle of spherical joints
            bool enableSpring; float hertz, dampingRatio;           // joint spring or alignment spring
            bool enableMotor; float motorSpeed, maxMotorTorque;     // speed is around the main axis
            float maxSpringForce;                                   // force/torque limit of springs
            bool collideConnected;
        };

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

        /** Create a constraint (joint) between two bodies. `frameA` and `frameB` are local frames
            related to body A and body B respectively, unless ConstraintSetting::useWorldPivots is
            set and they are given in world space. The returned constraint should be registered
            by addConstraint() before it takes effect. */
        virtual ConstraintBase* createConstraint(RigidBodyBase* bodyA, const osg::Matrix& frameA,
                                                 RigidBodyBase* bodyB, const osg::Matrix& frameB,
                                                 ConstraintType type = CONSTRAINT_P2P,
                                                 const ConstraintSetting* setting = NULL);

        /** Update limits, spring and motor parameters of a registered constraint */
        virtual void setConstraintSetting(const std::string& name, const ConstraintSetting& setting);

        // Applying force functions (point is ignored when applying an angular impulse)
        virtual void applyImpulse(const std::string& name, const osg::Vec3& point,
                                  const osg::Vec3& impulse, bool wake = true, bool linearOrAngular = true);

        // get*() functions
        virtual float getInverseMass(const std::string& name);
        virtual osg::Matrix getInverseInertia(const std::string& name);
        virtual osg::Vec3 getCenterOfMass(const std::string& name);

        /** Create an empty dynamic body (unless kinematic is set) without any shape.
            Shapes can be attached later by addShapeToBody(), which makes it possible to
            build compound bodies like a character controller (feet box + body capsule). */
        virtual RigidBodyBase* createBody(const std::string& name, const BodySetting& setting,
                                          const osg::Matrix& matrix = osg::Matrix());

        /** Attach an exist shape to a body created by createBody(). The shape (which may contain
            local offset geometry, see createPhysicsBox() / createPhysicsCapsule()) will be owned
            by the body. The mass is the mass contributed by this shape, and is converted to
            density internally. */
        virtual CollisionShapeBase* addShapeToBody(RigidBodyBase* body, CollisionShapeBase* shape,
                                                   float mass, const ShapeSetting* setting = NULL);

        virtual void setShapeFriction(CollisionShapeBase* shape, float friction);
        virtual void setGravityScale(const std::string& name, float scale);
        virtual void setLinearDamping(const std::string& name, float damping);

        /** Enable/disable continuous collision detection for a body, which prevents fast moving
            bodies (such as ragdoll bones) from going through other objects. */
        virtual void setBullet(const std::string& name, bool flag);

        /** Override the local center of mass of a body. Used by character controllers to lower
            the mass center for better stability. Not supported by all engines. */
        virtual void setMassCenter(const std::string& name, const osg::Vec3& center);

        CollisionShapeBase* getShape(const std::string& name);
        RigidBodyBase* getRigidBody(const std::string& name);
        ConstraintBase* getConstraint(const std::string& name);

        typedef std::pair<osg::ref_ptr<ConstraintBase>, int> ConstraintAndState;
        const std::map<std::string, ConstraintAndState>& getConstraints() const { return _constraints; }
        const std::map<std::string, osg::ref_ptr<CollisionShapeBase>>& getShapes() const { return _shapes; }
        const std::map<std::string, osg::ref_ptr<RigidBodyBase>>& getBodies() const { return _bodies; }

        // Collision and raycast functions
        struct QueryFilter
        {
            unsigned int categoryBits, maskBits; std::string name;
            QueryFilter() : categoryBits(0xFFFFFFFF), maskBits(0xFFFFFFFF) {}
        };

        struct ContactPlane
        {
            osg::ref_ptr<RigidBodyBase> rigidBody;
            osg::Plane plane; osg::Vec3 point; float penetration;
            ContactPlane() : penetration(0.0f) {}
        };

        struct RaycastHit
        {
            osg::ref_ptr<RigidBodyBase> rigidBody;
            osg::Vec3 position, normal; std::string name;
            RaycastHit() : rigidBody(NULL) {}
        };

        struct SweepResult
        {
            osg::ref_ptr<RigidBodyBase> rigidBody;
            osg::Vec3 point, normal; float fraction; bool hit, startedSolid;
            std::string name;
            SweepResult() : fraction(1.0f), hit(false), startedSolid(false) {}
        };

        /** Cast a shape (sweep) through the world and return the closest hit. Only convex shapes
            created by createPhysicsBox()/createPhysicsSphere()/createPhysicsCapsule() are guaranteed
            to work here. `ignoredBody` is used to skip the shapes of a given body (usually the
            character's own body). */
        virtual SweepResult sweep(const osg::Vec3& start, const osg::Vec3& translation,
                                  CollisionShapeBase* shape, const QueryFilter& filter = QueryFilter(),
                                  RigidBodyBase* ignoredBody = NULL);
        //virtual std::vector<ContactPlane> collide(const osg::Vec3& position, CollisionShapeBase* shape,
        //                                          const QueryFilter& filter = QueryFilter());  // TODO
        virtual bool raycast(const osg::Vec3& start, const osg::Vec3& end,
                             RaycastHit& result, const QueryFilter& filter = QueryFilter(), bool getNameFromBody = true);
        virtual std::vector<RaycastHit> raycastAll(const osg::Vec3& start, const osg::Vec3& end,
                                                   const QueryFilter& filter = QueryFilter(), bool getNameFromBody = true);

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

        /** Create a box shape with geometry moved away from its own origin. It is useful for
            building compound bodies like a character controller. */
        virtual CollisionShapeBase* createPhysicsBox(const osg::Vec3& halfSize, const osg::Vec3& offset);

        /** Create a capsule shape defined by its two hemisphere centers (in local space) */
        virtual CollisionShapeBase* createPhysicsCapsule(float radius, const osg::Vec3& c0, const osg::Vec3& c1);

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

    /** Rigid-body based character controller, supporting walking, jumping and stepping up.
        The character is built as a compound body (a feet box with dynamic friction and a
        low-friction body capsule) and all its movement is done by shape casts (sweeps) plus
        velocity manipulation, so it works with any PhysicsEngine backend which implements
        the required shape/sweep interfaces. Typical usage:
            char* = new PhysicsCharacter(engine, "character");
            character->create(startPosition);
            // for every frame:
            character->setWishVelocity(forward * throttle);  // horizontal direction, length <= 1
            character->jump();                               // if jump key pressed
            character->step(timeStep);   engine->advance(timeStep);   character->lateStep();
    */
    class PhysicsCharacter : public osg::Referenced
    {
    public:
        PhysicsCharacter(PhysicsEngine* engine, const std::string& name);

        // Character parameters (in meters per second, meters and degrees), tune before create()
        float walkSpeed, runSpeed, jumpSpeed;
        float maxSlopeAngle, characterGravity, characterMass, jumpCooldownTime;
        float stepUpHeight, stepDownHeight, skin;
        float brakePower, surfaceFriction, airFriction;
        float bodyRadius, totalHeight;

        bool create(const osg::Vec3& position);
        void destroy();
        bool valid() const { return _body.valid(); }

        /** Set the horizontal wish velocity direction (world space, length usually <= 1.0).
            It will be scaled by walkSpeed or runSpeed in step() automatically. */
        void setWishVelocity(const osg::Vec3& v);
        void setSprinting(bool b) { _sprintRequest = b; }
        void jump();

        void step(float timeStep);  // call this before PhysicsEngine::advance()
        void lateStep();            // call this after PhysicsEngine::advance()

        bool isOnGround() const { return _onGround; }
        bool isSprinting() const { return _sprint; }
        const osg::Vec3& getGroundNormal() const { return _groundNormal; }
        const osg::Vec3& getWishVelocity() const { return _wishVelocity; }
        osg::Vec3 getPosition() const;
        osg::Vec3 getFeetPosition() const;
        osg::Vec3 getVelocity() const;
        osg::Vec3 getMassCenter() const { return _massCenter; }
        const std::string& getName() const { return _name; }
        RigidBodyBase* getBody() { return _body.get(); }
        const RigidBodyBase* getBody() const { return _body.get(); }
        PhysicsEngine* getEngine() { return _engine.get(); }

    protected:
        virtual ~PhysicsCharacter();

        struct TraceResult
        {
            osg::Vec3 endPosition, hitPoint, normal;
            float fraction; bool hit, startedSolid;
            TraceResult() : fraction(1.0f), hit(false), startedSolid(false) {}
        };

        // s&box unit conversion (1 unit = 1 inch = 0.0254m, 40 units per meter)
        static const float SRC;

        TraceResult traceBody(const osg::Vec3& from, const osg::Vec3& to,
                              float radiusScale, float heightScale);
        CollisionShapeBase* getTraceShape(float radiusScale, float heightScale);
        bool isStandableSurface(const osg::Vec3& normal) const;
        void updateGround(bool onGround, const osg::Vec3& normal);
        static osg::Vec3 addClamped(const osg::Vec3& current, const osg::Vec3& add, float maxAddLength);

        void updateMassCenter(float wishSpeed);
        void updateBody(const osg::Vec3& wishVelocity);
        void addVelocity(const osg::Vec3& wishVelocity);
        bool tryStep(float maxStepHeight);
        void restoreStep();
        void reground(float stepSize);
        void categorizeGround();

        osg::observer_ptr<PhysicsEngine> _engine;
        std::string _name;
        osg::ref_ptr<RigidBodyBase> _body;
        osg::ref_ptr<CollisionShapeBase> _feetShape, _bodyShape;
        std::map<int, osg::ref_ptr<CollisionShapeBase>> _traceShapes;

        osg::Vec3 _wishInput, _wishVelocity, _groundNormal, _massCenter, _stepPosition;
        float _jumpCooldown; bool _onGround, _sprintRequest, _sprint, _didStep;
    };
}

#endif
