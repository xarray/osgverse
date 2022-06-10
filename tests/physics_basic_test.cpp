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

class ShootSphereHandler : public osgGA::GUIEventHandler
{
public:
    ShootSphereHandler(osgVerse::PhysicsEngine* pe, osg::Group* s)
    : _physics(pe), _scene(s), _sphereCount(0) {}

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        if (ea.getEventType() == osgGA::GUIEventAdapter::KEYUP)
        {
            if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Return)
                shoot(static_cast<osgViewer::View*>(&aa));
        }
        return false;
    }

    void shoot(osgViewer::View* view)
    {
        const float sphereRadius = 0.4f, sphereMass = 2.0f, speed = 50.0f;
        osg::Vec3 eye, target, up, forward;

        // Compute player position and forward
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
    int _sphereCount;
};

int main(int argc, char** argv)
{
    const float groundSize = 40.0f, groundThickness = 0.1f;
    const float boxHalfSize = 0.49f, boxMass = 2.0f;

    // Create ground geometry
    osg::ref_ptr<osg::MatrixTransform> groundMT = new osg::MatrixTransform;
    {
        osg::ref_ptr<osg::Geode> ground = new osg::Geode;
        ground->addDrawable(new osg::ShapeDrawable(
            new osg::Box(osg::Vec3(), groundSize, groundSize, groundThickness)));
        groundMT->addChild(ground.get());
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
    root->addChild(groundMT.get());
    for (int i = 0; i < 50; ++i) root->addChild(boxMT[i].get());

    // Create the physics world and add the rigid body of every scene object
    osg::ref_ptr<osgVerse::PhysicsEngine> physics = new osgVerse::PhysicsEngine;
    physics->addRigidBody("ground", osgVerse::createPhysicsBox(
        osg::Vec3(groundSize * 0.5f, groundSize * 0.5f, groundThickness * 0.5f)), 0.0f);
    for (int i = 0; i < 50; ++i)
        physics->addRigidBody("box" + std::to_string(i), osgVerse::createPhysicsBox(
            osg::Vec3(boxHalfSize, boxHalfSize, boxHalfSize)), boxMass, boxMT[i]->getMatrix());

    // Setup callbacks for scene object to update its pose
    groundMT->setUpdateCallback(new osgVerse::PhysicsUpdateCallback(physics.get(), "ground"));
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
    while (!viewer.done())
    {
        physics->advance(0.02f);
        viewer.frame();
    }
    return 0;
}
