#include <osg/io_utils>
#include <osg/MatrixTransform>
#include <osg/ShapeDrawable>
#include <osg/Geode>
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

/* A handler which releases the human to let it fall down as a ragdoll when any key is pressed (and
   puts it back on its feet when a key is pressed again), and which allows to drag one of its bones
   with the left mouse button to throw it away, in the same way as the basic physics example. */
class HumanHandler : public osgGA::GUIEventHandler
{
public:
    HumanHandler(osgVerse::PhysicsEngine* pe, osgVerse::PhysicsHuman* h)
    : _physics(pe), _human(h), _pickingDistance(0.0f)
    {
        // Create a point/empty kinematic body for dragging use
        _physics->addRigidBody("dragger", _physics->createPhysicsPoint(), 0.0f, osg::Matrix(), true);
    }

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        if (ea.getEventType() == osgGA::GUIEventAdapter::PUSH)
        {
            if (ea.getButtonMask() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
                dragBone(view, ea.getXnormalized(), ea.getYnormalized());
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::RELEASE)
        {
            if (!_pickedRigidName.empty()) releaseBone();
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::DRAG)
        {
            if (!_pickedRigidName.empty())
            {
                if (dragBone(view, ea.getXnormalized(), ea.getYnormalized()))
                    return true;  // diable camera manipulator if dragging
            }
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::KEYUP)
        {
            // Any key releases the drive of the human, so that it falls down as a ragdoll, or
            // turns it back on, which makes the human stand up again
            _human->setRagdoll(!_human->isRagdoll());
            std::cout << "[Human] The human is now "
                      << (_human->isRagdoll() ? "a ragdoll" : "standing up again") << std::endl;
            return true;
        }
        return false;
    }

    void releaseBone()
    {
        _physics->removeConstraint("dragP2P");
        _pickedRigidName = "";
    }

    bool dragBone(osgViewer::View* view, float nx, float ny)
    {
        osg::Matrix invMVP = osg::Matrix::inverse(
            view->getCamera()->getViewMatrix() * view->getCamera()->getProjectionMatrix());
        osg::Vec3 start = osg::Vec3(nx, ny, -1.0f) * invMVP;
        osg::Vec3 end = osg::Vec3(nx, ny, 1.0f) * invMVP;

        if (_pickedRigidName.empty())
        {
            // Try to pick a bone of the human. The human is released at the same time, so that the
            // bone can be dragged far away from the body and the human is thrown around
            osgVerse::PhysicsEngine::RaycastHit result;
            if (_physics->raycast(start, end, result))
            {
                bool isKinematic = false;
                if (_physics->isDynamicBody(result.name, isKinematic))
                {
                    osgVerse::PhysicsEngine::ConstraintSetting setting;
                    setting.useWorldPivots = true;
                    setting.impulseClamp = 30.0f;
                    setting.tau = 0.001f;  // very weak constraint for picking

                    // Create p2p constraint between the empty kinematic body and the picked bone
                    _physics->setTransform("dragger", osg::Matrix::translate(result.position));
                    _physics->addConstraint("dragP2P", _physics->createConstraint(
                            _physics->getRigidBody("dragger"), osg::Matrix::translate(result.position),
                            result.rigidBody, osg::Matrix::translate(result.position),
                            osgVerse::PhysicsEngine::CONSTRAINT_P2P, &setting));
                    _pickingDistance = (result.position - start).length();
                    _pickedRigidName = result.name; _human->setRagdoll(true);
                    return true;
                }
            }
            return false;
        }
        else
        {
            // Drag the picked bone by moving the empty kinematic body
            osg::Vec3 dir = end - start;
            dir.normalize(); dir *= _pickingDistance;
            _physics->setTransform("dragger", osg::Matrix::translate(start + dir));
            return true;
        }
    }

protected:
    osg::observer_ptr<osgVerse::PhysicsEngine> _physics;
    osg::observer_ptr<osgVerse::PhysicsHuman> _human;
    std::string _pickedRigidName;
    float _pickingDistance;
};

int main(int argc, char** argv)
{
    const float groundSize = 20.0f, groundThickness = 0.1f, timeStep = 0.02f;
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());

    // Create a ground geometry
    osg::ref_ptr<osg::MatrixTransform> groundMT = new osg::MatrixTransform;
    groundMT->setMatrix(osg::Matrix::translate(0.0f, 0.0f, -groundThickness * 0.5f));
    {
        osg::ref_ptr<osg::Geode> ground = new osg::Geode;
        ground->addDrawable(new osg::ShapeDrawable(
            new osg::Box(osg::Vec3(), groundSize, groundSize, groundThickness)));
        groundMT->addChild(ground.get());
    }

    osg::ref_ptr<osgVerse::PhysicsEngine> physics;
    if (arguments.read("--bullet"))
        physics = dynamic_cast<osgVerse::PhysicsEngine*>(osgDB::readObjectFile("0.verse_bullet"));
    if (!physics) physics = new osgVerse::PhysicsEngine;
    OSG_NOTICE << "Using physics engine: " << physics->getName() << "\n";

    // Create the physics world and add the rigid body of the ground
    physics->addRigidBody("ground", physics->createPhysicsBox(
        osg::Vec3(groundSize * 0.5f, groundSize * 0.5f, groundThickness * 0.5f)), 0.0f);
    groundMT->setUpdateCallback(new osgVerse::PhysicsUpdateCallback(physics.get(), "ground"));

    // Create the simplest standing human: every bone of it is a box which is updated by physics
    osg::ref_ptr<osgVerse::PhysicsHuman> human = new osgVerse::PhysicsHuman(physics.get(), "human");
    human->collisionGroup = 1;  // bones of the same human don't collide with each other
    if (!human->create(osg::Vec3(0.0f, 0.0f, 0.0f)))
    {
        OSG_WARN << "[Human] Failed to create the human\n"; return 1;
    }

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(groundMT.get());
    for (unsigned int i = 0; i < human->getNumBones(); ++i)
    {
        const std::string& boneName = human->getBoneName(i);
        osg::ref_ptr<osg::MatrixTransform> boneMT = new osg::MatrixTransform;
        boneMT->setUpdateCallback(new osgVerse::PhysicsUpdateCallback(
            physics.get(), human->getBodyName(i)));
        {
            // The bones are colored from the torso to the limbs and the feet
            float f = (float)i / human->getNumBones();
            osg::Vec3 size = human->getBoneBoxSize(i) * 2.0f;
            osg::ref_ptr<osg::Geode> bone = new osg::Geode;
            osg::ref_ptr<osg::ShapeDrawable> drawable = new osg::ShapeDrawable(
                new osg::Box(osg::Vec3(), size[0], size[1], size[2]));
            drawable->setColor(osg::Vec4(0.3f + f * 0.5f, 0.6f - f * 0.3f, 1.0f - f * 0.6f, 1.0f));
            bone->addDrawable(drawable.get()); boneMT->addChild(bone.get());
        }
        root->addChild(boneMT.get());
    }

    // Start the viewer
    osgViewer::Viewer viewer;
    viewer.addEventHandler(new HumanHandler(physics.get(), human.get()));
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);

    std::cout << "[Human] Press any key to release the human and let it fall as a ragdoll,\n"
              << "        and press a key again to make it stand up again.\n"
              << "        Drag a bone with the left mouse button to throw the human away.\n";
    while (!viewer.done())
    {
        human->step(timeStep);
        physics->advance(timeStep, 4);  // the springs of the human need the sub-steps to be stable
        viewer.frame();
    }
    return 0;
}
