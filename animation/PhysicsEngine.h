#ifndef MANA_ANIM_PHYSICSENGINE_HPP
#define MANA_ANIM_PHYSICSENGINE_HPP

#include <osg/Version>
#include <osg/MatrixTransform>
#include <map>

class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
class btCollisionShape;
class btRigidBody;

namespace osgVerse
{

    class PhysicsEngine : public osg::Referenced
    {
    public:
        PhysicsEngine();

        // Rigid-body functions
        btRigidBody* addRigidBody(const std::string& name, btCollisionShape* s, float mass = 0.0f,
                                  const osg::Matrix& m = osg::Matrix(), bool kinematic = false);
        void removeBody(const std::string& name);

        // Setting transform and velocity functions
        void setTransform(const std::string& name, const osg::Matrix& matrix);
        osg::Matrix getTransform(const std::string& name, bool& valid);

        void setVelocity(const std::string& name, const osg::Vec3& v, bool linearOrAngular);
        osg::Vec3 getVelocity(const std::string& name, bool linearOrAngular);

        // Misc functions
        btCollisionShape* getShape(const std::string& name);
        btRigidBody* getRigidBody(const std::string& name);
        void setGravity(const osg::Vec3& gravity);

        // Raycast functions
        struct RaycastHit
        {
            btRigidBody* rigidBody;
            osg::Vec3 position, normal;
            std::string name;
            RaycastHit() : rigidBody(NULL) {}
        };
        bool raycast(const osg::Vec3& start, const osg::Vec3& end,
                     RaycastHit& result, bool getNameFromBody = false);
        std::vector<RaycastHit> raycastAll(const osg::Vec3& start, const osg::Vec3& end,
                                           bool getNameFromBody = false);

        // Advance the world
        void advance(float timeStep, int maxSubSteps = 1);

    protected:
        virtual ~PhysicsEngine();

        btDefaultCollisionConfiguration* _collisionCfg;
        btCollisionDispatcher* _collisionDispatcher;
        btBroadphaseInterface* _overlappingPairCache;
        btSequentialImpulseConstraintSolver* _solver;
        btDiscreteDynamicsWorld* _world;
        std::map<std::string, btCollisionShape*> _shapes;
        std::map<std::string, btRigidBody*> _bodies;
    };

}

#endif
