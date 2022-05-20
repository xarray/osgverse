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

        btRigidBody* addShape(const std::string& name, btCollisionShape* s, float mass = 0.0f,
                              const osg::Matrix& m = osg::Matrix(), bool kinematic = false);
        void removeShape(const std::string& name);

        void setTransform(const std::string& name, const osg::Matrix& matrix);
        osg::Matrix getTransform(const std::string& name, bool& valid);

        void setVelocity(const std::string& name, const osg::Vec3& v, bool linearOrAngular);
        osg::Vec3 getVelocity(const std::string& name, bool linearOrAngular);

        btCollisionShape* getShape(const std::string& name);
        btRigidBody* getRigidBody(const std::string& name);

        void setGravity(const osg::Vec3& gravity);
        void advance(float timeStamp, int maxSubSteps = 1);

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
