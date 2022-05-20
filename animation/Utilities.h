#ifndef MANA_ANIM_UTILITIES_HPP
#define MANA_ANIM_UTILITIES_HPP

#include <osg/Version>
#include <osg/Texture2D>
#include <osg/Geometry>
#include "PhysicsEngine.h"
class btCollisionShape;

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

    extern btCollisionShape* createBox(const osg::Vec3& halfSize);
    extern btCollisionShape* createCylinder(const osg::Vec3& halfSize);
    extern btCollisionShape* createCone(float radius, float height);
    extern btCollisionShape* createSphere(float radius);

}

#endif
