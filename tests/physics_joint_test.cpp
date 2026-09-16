#include <osg/io_utils>
#include <osg/MatrixTransform>
#include <osg/ShapeDrawable>
#include <osg/Geode>
#include <osg/Math>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <animation/PhysicsEngine.h>
#include <animation/Utilities.h>
#include <readerwriter/Utilities.h>
#include <pipeline/Global.h>
#include <iostream>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

// This example shows every supported joint type side by side, with each station animated in a loop
// so that the behaviors of the joints can be compared directly. A human ragdoll will be added here
// later as the final station.
using namespace osgVerse;

static const float timeStep = 1.0f / 60.0f;
static const int chainGroup = 1;  // shapes sharing the same group won't collide with each other

// Positions of all stations, they are placed from -15m to +15m along the X axis with an interval
// of 5 meters, so that the animated parts of neighboring stations never touch each other
static const float chainX = -15.0f, chainHandHeight = 5.0f;
static const float windmillX = -10.0f;
static const float swingX = -5.0f, swingLowerLimit = -0.9f, swingUpperLimit = 0.9f;
static const float swingMotorSpeed = 1.4f;
static const float coneX = 0.0f, coneAnchorHeight = 4.0f;
static const float parallelX = 5.0f, parallelHeight = 2.6f;
static const float motorX = 10.0f, motorHeight = 2.2f, motorCircleRadius = 1.1f;
static const float filterX = 15.0f;

// Create the visual shape of a body, which will follow the transform of the physics body
static osg::MatrixTransform* addVisual(PhysicsEngine* physics, osg::Group* root, const std::string& name,
                                       osg::Shape* shape, const osg::Vec4& color, const osg::Matrix& matrix)
{
    osg::ref_ptr<osg::ShapeDrawable> drawable = new osg::ShapeDrawable(shape);
    drawable->setColor(color);
    osg::ref_ptr<osg::Geode> geode = new osg::Geode; geode->addDrawable(drawable.get());

    osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
    mt->setMatrix(matrix); mt->addChild(geode.get()); root->addChild(mt.get());
    mt->setUpdateCallback(new PhysicsUpdateCallback(physics, name));
    return mt.release();
}

// Create a rigid body together with its visual shape
static osg::MatrixTransform* addBody(PhysicsEngine* physics, osg::Group* root, const std::string& name,
                                     osg::Shape* shape, CollisionShapeBase* collision, float mass,
                                     const osg::Matrix& matrix, const osg::Vec4& color,
                                     bool kinematic = false, const PhysicsEngine::ShapeSetting* setting = NULL)
{
    osg::MatrixTransform* mt = addVisual(physics, root, name, shape, color, matrix);
    physics->addRigidBody(name, collision, mass, matrix, kinematic, setting);
    return mt;
}

// Station 1: a chain of P2P joints, which is dragged by a kinematic hand moving along a circle
static void createChainStation(PhysicsEngine* physics, osg::Group* root)
{
    const int boneCount = 5;
    const float boneLength = 0.6f, boneRadius = 0.12f, half = boneLength * 0.5f;
    addBody(physics, root, "chain.hand", new osg::Sphere(osg::Vec3(), 0.25f),
            physics->createPhysicsSphere(0.25f), 0.0f,
            osg::Matrix::translate(chainX, 0.0f, chainHandHeight),
            osg::Vec4(1.0f, 0.7f, 0.25f, 1.0f), true);

    // Bones overlap a little at their ends, so they are put into one collision group to ignore
    // each other. Otherwise they would push themselves apart.
    PhysicsEngine::ShapeSetting setting;
    setting.friction = 0.4f; setting.collisionGroup = chainGroup;
    for (int i = 0; i < boneCount; ++i)
    {
        std::string name = "chain.bone" + std::to_string(i);
        osg::Vec3 center(chainX, 0.0f, chainHandHeight - 0.2f - half - boneLength * i);
        addBody(physics, root, name, new osg::Capsule(osg::Vec3(), boneRadius, boneLength),
                physics->createPhysicsCapsule(boneRadius, osg::Vec3(0.0f, 0.0f, -half),
                                              osg::Vec3(0.0f, 0.0f, half)),
                1.0f, osg::Matrix::translate(center), osg::Vec4(0.5f, 0.75f, 0.9f, 1.0f), false, &setting);
    }

    // Connect the hand and all bones with point-to-point joints
    std::string previous = "chain.hand"; osg::Vec3 pivot(0.0f, 0.0f, -0.2f);
    for (int i = 0; i < boneCount; ++i)
    {
        std::string name = "chain.bone" + std::to_string(i);
        physics->addConstraint("chain.joint" + std::to_string(i), physics->createConstraint(
            physics->getRigidBody(previous), osg::Matrix::translate(pivot),
            physics->getRigidBody(name), osg::Matrix::translate(0.0f, 0.0f, half),
            PhysicsEngine::CONSTRAINT_P2P));
        previous = name; pivot = osg::Vec3(0.0f, 0.0f, -half);
    }
}

// Station 2: a hinge joint with a motor, which keeps the arm rotating forever
static void createWindmillStation(PhysicsEngine* physics, osg::Group* root)
{
    const float postHeight = 3.0f, armLength = 2.0f, armHalf = armLength * 0.5f;
    addBody(physics, root, "windmill.post",
            new osg::Box(osg::Vec3(0.0f, 0.0f, postHeight * 0.5f), 0.3f, 0.3f, postHeight),
            physics->createPhysicsBox(osg::Vec3(0.15f, 0.15f, postHeight * 0.5f)), 0.0f,
            osg::Matrix::translate(windmillX, 0.0f, 0.0f), osg::Vec4(0.45f, 0.45f, 0.5f, 1.0f));

    // The arm is hinged at the top of the post. The hinge axis is the Z axis of the joint frames,
    // so they are rotated to make the arm spin around the world +Y axis like a windmill.
    osg::Matrix hingeFrame = osg::Matrix::rotate(osg::DegreesToRadians(-90.0f), osg::X_AXIS);
    addBody(physics, root, "windmill.arm",
            new osg::Box(osg::Vec3(armHalf, 0.0f, 0.0f), armLength, 0.24f, 0.24f),
            physics->createPhysicsBox(osg::Vec3(armHalf, 0.12f, 0.12f), osg::Vec3(armHalf, 0.0f, 0.0f)),
            4.0f, osg::Matrix::translate(windmillX, 0.0f, postHeight), osg::Vec4(0.9f, 0.4f, 0.3f, 1.0f));

    PhysicsEngine::ConstraintSetting cs;
    cs.enableMotor = true; cs.motorSpeed = 2.5f; cs.maxMotorTorque = 300.0f;
    physics->addConstraint("windmill.hinge", physics->createConstraint(
        physics->getRigidBody("windmill.post"),
        hingeFrame * osg::Matrix::translate(0.0f, 0.0f, postHeight),
        physics->getRigidBody("windmill.arm"), hingeFrame, PhysicsEngine::CONSTRAINT_HINGE, &cs));
}

// Station 3: a hinge joint with angle limits, whose motor speed is reversed in a smooth way
static void createSwingStation(PhysicsEngine* physics, osg::Group* root)
{
    const float postHeight = 2.2f, armLength = 1.6f, armHalf = armLength * 0.5f;
    addBody(physics, root, "swing.post",
            new osg::Box(osg::Vec3(0.0f, 0.0f, postHeight * 0.5f), 0.3f, 0.3f, postHeight),
            physics->createPhysicsBox(osg::Vec3(0.15f, 0.15f, postHeight * 0.5f)), 0.0f,
            osg::Matrix::translate(swingX, 0.0f, 0.0f), osg::Vec4(0.45f, 0.45f, 0.5f, 1.0f));

    // The arm swings around the vertical axis, and its angle is limited by the joint
    addBody(physics, root, "swing.arm",
            new osg::Box(osg::Vec3(armHalf, 0.0f, 0.0f), armLength, 0.24f, 0.24f),
            physics->createPhysicsBox(osg::Vec3(armHalf, 0.12f, 0.12f), osg::Vec3(armHalf, 0.0f, 0.0f)),
            3.0f, osg::Matrix::translate(swingX, 0.0f, postHeight), osg::Vec4(0.85f, 0.75f, 0.3f, 1.0f));

    PhysicsEngine::ConstraintSetting cs;
    cs.enableLimit = true; cs.lowerLimit = swingLowerLimit; cs.upperLimit = swingUpperLimit;
    cs.enableMotor = true; cs.motorSpeed = swingMotorSpeed; cs.maxMotorTorque = 200.0f;
    physics->addConstraint("swing.hinge", physics->createConstraint(
        physics->getRigidBody("swing.post"), osg::Matrix::translate(0.0f, 0.0f, postHeight),
        physics->getRigidBody("swing.arm"), osg::Matrix(), PhysicsEngine::CONSTRAINT_HINGE, &cs));
}

// Station 4: a cone-twist (spherical) joint, which limits both the cone angle and the twist
static void createConeTwistStation(PhysicsEngine* physics, osg::Group* root)
{
    const float poleRadius = 0.1f, poleHalf = 0.8f;
    addBody(physics, root, "cone.anchor", new osg::Sphere(osg::Vec3(), 0.2f),
            physics->createPhysicsSphere(0.2f), 0.0f,
            osg::Matrix::translate(coneX, 0.0f, coneAnchorHeight),
            osg::Vec4(1.0f, 0.7f, 0.25f, 1.0f), true);

    // The pendulum may only swing inside the cone and twist around its own axis by a small angle
    addBody(physics, root, "cone.pendulum", new osg::Capsule(osg::Vec3(), poleRadius, poleHalf * 2.0f),
            physics->createPhysicsCapsule(poleRadius, osg::Vec3(0.0f, 0.0f, -poleHalf),
                                          osg::Vec3(0.0f, 0.0f, poleHalf)),
            2.0f, osg::Matrix::translate(coneX, 0.0f, coneAnchorHeight - 0.2f - poleHalf),
            osg::Vec4(0.4f, 0.7f, 0.95f, 1.0f));

    PhysicsEngine::ConstraintSetting cs;
    cs.enableLimit = true; cs.coneLimit = osg::DegreesToRadians(30.0f);
    cs.lowerLimit = osg::DegreesToRadians(-25.0f); cs.upperLimit = osg::DegreesToRadians(25.0f);
    physics->addConstraint("cone.joint", physics->createConstraint(
        physics->getRigidBody("cone.anchor"), osg::Matrix::translate(0.0f, 0.0f, -0.2f),
        physics->getRigidBody("cone.pendulum"), osg::Matrix::translate(0.0f, 0.0f, poleHalf),
        PhysicsEngine::CONSTRAINT_CONE_TWIST, &cs));
}

// Station 5: a parallel joint, which aligns the orientations of two bodies without fixing positions
static void createParallelStation(PhysicsEngine* physics, osg::Group* root)
{
    const float needleHalf = 0.5f, needleRadius = 0.08f, offset = 1.6f;
    addBody(physics, root, "parallel.ref",
            new osg::Box(osg::Vec3(0.0f, 0.0f, needleHalf), needleRadius * 2.0f, needleRadius * 2.0f,
                         needleHalf * 2.0f),
            physics->createPhysicsBox(osg::Vec3(needleRadius, needleRadius, needleHalf)), 0.0f,
            osg::Matrix::translate(parallelX, 0.0f, parallelHeight),
            osg::Vec4(0.45f, 0.45f, 0.5f, 1.0f), true);

    // The follower has no gravity, so that only the orientation alignment can be observed. Sleeping
    // is disabled because it moves slowly and would be stopped by the engine otherwise.
    PhysicsEngine::BodySetting bodySetting;
    bodySetting.gravityScale = 0.0f; bodySetting.allowSleep = false;
    RigidBodyBase* follower = physics->createBody(
        "parallel.follower", bodySetting, osg::Matrix::translate(parallelX + offset, 0.0f, parallelHeight));
    PhysicsEngine::ShapeSetting shapeSetting;
    physics->addShapeToBody(follower, physics->createPhysicsBox(
        osg::Vec3(needleRadius, needleRadius, needleHalf)), 1.0f, &shapeSetting);
    addVisual(physics, root, "parallel.follower",
              new osg::Box(osg::Vec3(0.0f, 0.0f, needleHalf), needleRadius * 2.0f, needleRadius * 2.0f,
                           needleHalf * 2.0f),
              osg::Vec4(0.5f, 0.85f, 0.55f, 1.0f),
              osg::Matrix::translate(parallelX + offset, 0.0f, parallelHeight));

    PhysicsEngine::ConstraintSetting cs;
    cs.hertz = 5.0f; cs.dampingRatio = 0.7f;
    cs.maxSpringForce = 2000.0f;  // the spring torque limit of the alignment
    physics->addConstraint("parallel.joint", physics->createConstraint(
        physics->getRigidBody("parallel.ref"), osg::Matrix(), follower, osg::Matrix(),
        PhysicsEngine::CONSTRAINT_PARALLEL, &cs));
}

// Station 6: a motor joint, which drives a dynamic body to the pose of a kinematic anchor
static void createMotorStation(PhysicsEngine* physics, osg::Group* root)
{
    const float size = 0.45f;
    addBody(physics, root, "motor.anchor", new osg::Box(osg::Vec3(), size * 2.0f),
            physics->createPhysicsBox(osg::Vec3(size, size, size)), 0.0f,
            osg::Matrix::translate(motorX, 0.0f, motorHeight), osg::Vec4(1.0f, 0.7f, 0.25f, 1.0f), true);

    PhysicsEngine::BodySetting bodySetting; bodySetting.allowSleep = false;
    RigidBodyBase* follower = physics->createBody(
        "motor.follower", bodySetting, osg::Matrix::translate(motorX, 0.0f, motorHeight));
    PhysicsEngine::ShapeSetting shapeSetting;
    physics->addShapeToBody(follower, physics->createPhysicsBox(
        osg::Vec3(size, size, size)), 2.0f, &shapeSetting);
    addVisual(physics, root, "motor.follower", new osg::Box(osg::Vec3(), size * 2.0f),
              osg::Vec4(0.85f, 0.45f, 0.8f, 1.0f), osg::Matrix::translate(motorX, 0.0f, motorHeight));

    PhysicsEngine::ConstraintSetting cs;
    cs.hertz = 4.0f; cs.dampingRatio = 0.7f; cs.maxSpringForce = 5000.0f;
    physics->addConstraint("motor.joint", physics->createConstraint(
        physics->getRigidBody("motor.anchor"), osg::Matrix(), follower, osg::Matrix(),
        PhysicsEngine::CONSTRAINT_MOTOR, &cs));
}

// Station 7: a filter joint, which disables the collisions between two linked bodies
static void createFilterStation(PhysicsEngine* physics, osg::Group* root)
{
    const float postHeight = 2.2f, rodLength = 1.6f, rodHalf = rodLength * 0.5f;
    addBody(physics, root, "filter.post",
            new osg::Box(osg::Vec3(0.0f, 0.0f, postHeight * 0.5f), 0.3f, 0.3f, postHeight),
            physics->createPhysicsBox(osg::Vec3(0.15f, 0.15f, postHeight * 0.5f)), 0.0f,
            osg::Matrix::translate(filterX, 0.0f, 0.0f), osg::Vec4(0.45f, 0.45f, 0.5f, 1.0f));

    // The wall stands in the way of the rotating rod, but their collision should be disabled
    addBody(physics, root, "filter.wall", new osg::Box(osg::Vec3(), 0.2f, 1.6f, 1.6f),
            physics->createPhysicsBox(osg::Vec3(0.1f, 0.8f, 0.8f)), 0.0f,
            osg::Matrix::translate(filterX + 1.0f, 0.0f, postHeight), osg::Vec4(0.4f, 0.4f, 0.45f, 1.0f));

    addBody(physics, root, "filter.rod",
            new osg::Box(osg::Vec3(rodHalf, 0.0f, 0.0f), rodLength, 0.2f, 0.2f),
            physics->createPhysicsBox(osg::Vec3(rodHalf, 0.1f, 0.1f), osg::Vec3(rodHalf, 0.0f, 0.0f)),
            3.0f, osg::Matrix::translate(filterX, 0.0f, postHeight), osg::Vec4(0.9f, 0.4f, 0.3f, 1.0f));

    PhysicsEngine::ConstraintSetting cs;
    cs.enableMotor = true; cs.motorSpeed = 1.2f; cs.maxMotorTorque = 300.0f;
    physics->addConstraint("filter.hinge", physics->createConstraint(
        physics->getRigidBody("filter.post"), osg::Matrix::translate(0.0f, 0.0f, postHeight),
        physics->getRigidBody("filter.rod"), osg::Matrix(), PhysicsEngine::CONSTRAINT_HINGE, &cs));

    // Without this filter joint the rod would be blocked by the wall instead of passing through it
    physics->addConstraint("filter.wallJoint", physics->createConstraint(
        physics->getRigidBody("filter.rod"), osg::Matrix(),
        physics->getRigidBody("filter.wall"), osg::Matrix(), PhysicsEngine::CONSTRAINT_FILTER));
}

// Drive the kinematic bodies of all stations, so that every joint is animated in a loop
static void animateStations(PhysicsEngine* physics, float time)
{
    // Move the hand of the chain station along a circle
    float handAngle = time * 0.9f;
    physics->setTransform("chain.hand", osg::Matrix::translate(
        chainX + cosf(handAngle) * 1.2f, sinf(handAngle) * 1.2f, chainHandHeight));

    // Let the motor speed follow a sine wave, so the arm swings between its angle limits
    PhysicsEngine::ConstraintSetting swing;
    swing.enableLimit = true; swing.lowerLimit = swingLowerLimit; swing.upperLimit = swingUpperLimit;
    swing.enableMotor = true; swing.motorSpeed = swingMotorSpeed * sinf(time * 1.2f);
    swing.maxMotorTorque = 200.0f;
    physics->setConstraintSetting("swing.hinge", swing);

    // Sway the anchor of the cone-twist station, which makes the pendulum swing inside its cone
    physics->setTransform("cone.anchor", osg::Matrix::translate(
        coneX, sinf(time * 1.5f) * 0.7f, coneAnchorHeight));

    // Tilt the reference body of the parallel station: the follower should align with it
    physics->setTransform("parallel.ref", osg::Matrix::rotate(sinf(time * 0.6f) * 0.6f, osg::Y_AXIS) *
        osg::Matrix::translate(parallelX, 0.0f, parallelHeight));

    // Move the anchor of the motor station along a circle: the follower should follow its pose
    float motorAngle = time * 0.8f;
    physics->setTransform("motor.anchor", osg::Matrix::rotate(motorAngle, osg::Z_AXIS) *
        osg::Matrix::translate(motorX + cosf(motorAngle) * motorCircleRadius,
                               sinf(motorAngle) * motorCircleRadius, motorHeight));
}

int main(int argc, char** argv)
{
    const float groundHalfX = 22.0f, groundHalfY = 10.0f;
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());

    osg::ref_ptr<PhysicsEngine> physics;
    if (arguments.read("--bullet"))
        physics = dynamic_cast<PhysicsEngine*>(osgDB::readObjectFile("0.verse_bullet"));
    if (!physics) physics = new PhysicsEngine;
    OSG_NOTICE << "[osgVerse] Using physics engine: " << physics->getName() << "\n";

    // Bullet has no motor joint to drive a body to the pose of a kinematic anchor: its plugin
    // reports a warning about the missing feature, and the station is not shown here
    bool hasMotorJoint = (physics->getName() != "Bullet3");

    // Build the scene: the ground and all stations of joints
    osg::ref_ptr<osg::Group> root = new osg::Group;
    addBody(physics.get(), root.get(), "ground",
            new osg::Box(osg::Vec3(), groundHalfX * 2.0f, groundHalfY * 2.0f, 0.2f),
            physics->createPhysicsBox(osg::Vec3(groundHalfX, groundHalfY, 0.1f)), 0.0f,
            osg::Matrix::translate(0.0f, 0.0f, -0.1f), osg::Vec4(0.55f, 0.55f, 0.6f, 1.0f));

    createChainStation(physics.get(), root.get());
    createWindmillStation(physics.get(), root.get());
    createSwingStation(physics.get(), root.get());
    createConeTwistStation(physics.get(), root.get());
    createParallelStation(physics.get(), root.get());
    if (hasMotorJoint) createMotorStation(physics.get(), root.get());
    createFilterStation(physics.get(), root.get());

    std::cout << "[osgVerse] Joint stations from -X to +X:\n"
              << "  1. P2P: a 5-bone chain hangs on a hand which moves along a circle\n"
              << "  2. Hinge with motor: the arm keeps rotating like a windmill\n"
              << "  3. Hinge with limits: the arm swings between its two angle limits\n"
              << "  4. Cone-twist: the pendulum swings inside a 30-degree cone\n"
              << "  5. Parallel: the free body only aligns its orientation with the reference\n";
    if (hasMotorJoint)
        std::cout << "  6. Motor: the body follows the pose of a kinematic anchor moving in a circle\n";
    std::cout << (hasMotorJoint ? "  7. " : "  6. ") << "Filter: the rod passes through the wall "
              << "without any collision\n";

    // Start the viewer
    osg::ref_ptr<osgGA::TrackballManipulator> manipulator = new osgGA::TrackballManipulator;
    manipulator->setHomePosition(osg::Vec3(0.0f, -34.0f, 14.0f), osg::Vec3(0.0f, 0.0f, 2.5f), osg::Z_AXIS);

    osgViewer::Viewer viewer;
    viewer.setUpViewOnSingleScreen(0);
    viewer.setSceneData(root.get());
    viewer.setCameraManipulator(manipulator.get());
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);

    float simTime = 0.0f;
    while (!viewer.done())
    {
        animateStations(physics.get(), simTime);
        physics->advance(timeStep, 4);
        simTime += timeStep; viewer.frame();
    }
    return 0;
}
