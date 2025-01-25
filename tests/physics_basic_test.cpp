#include <osg/io_utils>
#include <osg/MatrixTransform>
#include <osg/ShapeDrawable>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <animation/PhysicsEngine.h>
#include <animation/Utilities.h>
#include <iostream>
#include <sstream>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

class ShootSphereHandler : public osgGA::GUIEventHandler
{
public:
    ShootSphereHandler(osgVerse::PhysicsEngine* pe, osg::Group* s)
    : _physics(pe), _scene(s), _pickingDistance(0.0f), _sphereCount(0)
    {
        // Create a point/empty kinematic body for dragging use
        _physics->addRigidBody("dragger", osgVerse::createPhysicsPoint(), 0.0f, osg::Matrix(), true);
    }

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        if (ea.getEventType() == osgGA::GUIEventAdapter::PUSH)
        {
            if (ea.getButtonMask() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
                dragObject(view, ea.getXnormalized(), ea.getYnormalized());
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::RELEASE)
        {
            if (!_pickedRigidName.empty()) releaseDraggingObject();
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::DRAG)
        {
            if (!_pickedRigidName.empty())
            {
                if (dragObject(view, ea.getXnormalized(), ea.getYnormalized()))
                    return true;  // diable camera manipulator if dragging
            }
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::KEYUP)
        {
            if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Return)
                shoot(view, 0.4f, 2.0f, 50.0f);
        }
        return false;
    }

    void releaseDraggingObject()
    {
        // Remove p2p constraint
        _physics->removeConstraint("dragP2P");
        _pickedRigidName = "";
    }

    bool dragObject(osgViewer::View* view, float nx, float ny)
    {
        osg::Matrix invMVP = osg::Matrix::inverse(
            view->getCamera()->getViewMatrix() * view->getCamera()->getProjectionMatrix());
        osg::Vec3 start = osg::Vec3(nx, ny, -1.0f) * invMVP;
        osg::Vec3 end = osg::Vec3(nx, ny, 1.0f) * invMVP;

        if (_pickedRigidName.empty())
        {
            // Try to pick a rigid object
            osgVerse::PhysicsEngine::RaycastHit result;
            if (_physics->raycast(start, end, result, true))
            {
                bool isKinematic = false;
                if (_physics->isDynamicBody(result.name, isKinematic))
                {
                    osgVerse::ConstraintSetting setting;
                    setting.useWorldPivots = true;
                    setting.impulseClamp = 30.0f;
                    setting.tau = 0.001f;  // very weak constraint for picking

                    // Create p2p constraint between the empty kinematic body and the picked one
                    _physics->setTransform("dragger", osg::Matrix::translate(result.position));
                    _physics->addConstraint("dragP2P", osgVerse::createConstraintP2P(
                            _physics->getRigidBody("dragger"), result.position,
                            result.rigidBody, result.position, &setting));
                    _pickingDistance = (result.position - start).length();
                    _pickedRigidName = result.name;
                    return true;
                }
            }
            return false;
        }
        else
        {
            // Drag the picked object by moving the empty kinematic body
            osg::Vec3 dir = end - start;
            dir.normalize(); dir *= _pickingDistance;
            _physics->setTransform("dragger", osg::Matrix::translate(start + dir));
            return true;
        }
    }

    void shoot(osgViewer::View* view, float sphereRadius, float sphereMass, float speed)
    {
        // Compute player position and forward
        osg::Vec3 eye, target, up, forward;
        view->getCamera()->getViewMatrixAsLookAt(eye, target, up);
        forward = target - eye; forward.normalize();

        // Create a sphere as a bullet
        osg::ref_ptr<osg::MatrixTransform> sphereMT = new osg::MatrixTransform;
        sphereMT->setMatrix(osg::Matrix::translate(eye));
        {
            osg::ref_ptr<osg::Geode> sphere = new osg::Geode;
            sphere->addDrawable(new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(), sphereRadius)));
            sphereMT->addChild(sphere.get());
        }

        // Add the sphere to scene and physics world
        std::string name = "sphere" + std::to_string(++_sphereCount);
        _physics->addRigidBody(
            name, osgVerse::createPhysicsSphere(sphereRadius), sphereMass, sphereMT->getMatrix());
        _physics->setVelocity(name, forward * speed, true);
        
        sphereMT->setUpdateCallback(new osgVerse::PhysicsUpdateCallback(_physics.get(), name));
        _scene->addChild(sphereMT.get());
    }

protected:
    osg::observer_ptr<osgVerse::PhysicsEngine> _physics;
    osg::observer_ptr<osg::Group> _scene;
    std::string _pickedRigidName;
    float _pickingDistance;
    int _sphereCount;
};

int main(int argc, char** argv)
{
    const float groundSize = 40.0f, groundThickness = 0.1f;
    const float boxHalfSize = 0.49f, boxMass = 2.0f;

    // Create a ground geometry
    osg::ref_ptr<osg::MatrixTransform> groundMT = new osg::MatrixTransform;
    {
        osg::ref_ptr<osg::Geode> ground = new osg::Geode;
        ground->addDrawable(new osg::ShapeDrawable(
            new osg::Box(osg::Vec3(), groundSize, groundSize, groundThickness)));
        groundMT->addChild(ground.get());
    }

    // Create a model from file
    osg::ref_ptr<osg::MatrixTransform> cessnaMT = new osg::MatrixTransform;
    cessnaMT->setMatrix(osg::Matrix::rotate(osg::PI_4, osg::X_AXIS) *
                        osg::Matrix::translate(0.0f, -5.0f, 10.0f));
    
    osg::ref_ptr<osg::Node> cessnaModel = osgDB::readNodeFile("cessna.osg");
    if (cessnaModel.valid())
    {
        // Scale can't be handled with rotation & position in the same matrix
        osg::ref_ptr<osg::MatrixTransform> cessna = new osg::MatrixTransform;
        cessna->setMatrix(osg::Matrix::scale(0.1f, 0.1f, 0.1f));
#if !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE) && !defined(OSG_GL3_AVAILABLE)
        cessna->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
#endif
        cessna->addChild(cessnaModel.get());
        cessnaMT->addChild(cessna.get());
    }

    // Create 50 boxes in scene
    osg::ref_ptr<osg::MatrixTransform> boxMT[50];
    for (int y = 0; y < 5; ++y)
        for (int x = -5; x < 5; ++x)
        {
            int id = (x + 5) + y * 10;
            boxMT[id] = new osg::MatrixTransform;
            boxMT[id]->setMatrix(osg::Matrix::translate((float)x, 0.0f, (float)y + 0.5f));
            
            osg::ref_ptr<osg::Geode> box = new osg::Geode;
            box->addDrawable(new osg::ShapeDrawable(new osg::Box(osg::Vec3(), boxHalfSize * 2.0f)));
            boxMT[id]->addChild(box.get());
        }

    // Add all to scene graph
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(groundMT.get()); root->addChild(cessnaMT.get());
    for (int i = 0; i < 50; ++i) root->addChild(boxMT[i].get());

    // Create the physics world and add the rigid body of every scene object
    osg::ref_ptr<osgVerse::PhysicsEngine> physics = new osgVerse::PhysicsEngine;
    physics->addRigidBody("ground", osgVerse::createPhysicsBox(
        osg::Vec3(groundSize * 0.5f, groundSize * 0.5f, groundThickness * 0.5f)), 0.0f);
    if (cessnaModel.valid())
    {
        physics->addRigidBody("cessna", osgVerse::createPhysicsHull(
            cessnaMT->getChild(0)), 15.0f, cessnaMT->getMatrix());
    }

    for (int i = 0; i < 50; ++i)
        physics->addRigidBody("box" + std::to_string(i), osgVerse::createPhysicsBox(
            osg::Vec3(boxHalfSize, boxHalfSize, boxHalfSize)), boxMass, boxMT[i]->getMatrix());

    // Setup callbacks for scene object to update its pose
    groundMT->setUpdateCallback(new osgVerse::PhysicsUpdateCallback(physics.get(), "ground"));
    if (cessnaModel.valid()) cessnaMT->setUpdateCallback(new osgVerse::PhysicsUpdateCallback(physics.get(), "cessna"));
    for (int i = 0; i < 50; ++i)
        boxMT[i]->setUpdateCallback(
            new osgVerse::PhysicsUpdateCallback(physics.get(), "box" + std::to_string(i)));

    // Start the viewer
    osgViewer::Viewer viewer;
    viewer.addEventHandler(new ShootSphereHandler(physics.get(), root.get()));
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);
    while (!viewer.done())
    {
        physics->advance(0.02f);
        viewer.frame();
    }
    return 0;
}
