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

    btCollisionShape* createBox(const osg::Vec3& halfSize)
    { return new btBoxShape(btVector3(halfSize[0], halfSize[1], halfSize[2])); }
    
    btCollisionShape* createCylinder(const osg::Vec3& halfSize)
    { return new btCylinderShape(btVector3(halfSize[0], halfSize[1], halfSize[2])); }

    btCollisionShape* createCone(float radius, float height)
    { return new btConeShape(radius, height); }

    btCollisionShape* createSphere(float radius)
    { return new btSphereShape(radius); }
    
}
