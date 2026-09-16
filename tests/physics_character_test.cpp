#include <osg/io_utils>
#include <osg/MatrixTransform>
#include <osg/ShapeDrawable>
#include <osg/Geode>
#include <osg/Math>
#include <osgDB/ReadFile>
#include <osgGA/GUIEventHandler>
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

// The test scene is Z-up: the character walks along +Y, and the staircase ascends along +Y
static const float groundHalfSize = 40.0f;
static const float characterStartHeight = 1.0f;

static osg::MatrixTransform* addBox(osgVerse::PhysicsEngine* physics, osg::Group* root, const std::string& name,
                                    const osg::Vec3& halfSize, const osg::Matrix& matrix, float mass,
                                    const osg::Vec4& color)
{
    osg::ref_ptr<osg::ShapeDrawable> box = new osg::ShapeDrawable(new osg::Box(
        osg::Vec3(), halfSize.x() * 2.0f, halfSize.y() * 2.0f, halfSize.z() * 2.0f));
    box->setColor(color);
    osg::ref_ptr<osg::Geode> geode = new osg::Geode; geode->addDrawable(box.get());

    osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
    mt->setMatrix(matrix); mt->addChild(geode.get()); root->addChild(mt.get());
    physics->addRigidBody(name, physics->createPhysicsBox(halfSize), mass, matrix);
    mt->setUpdateCallback(new osgVerse::PhysicsUpdateCallback(physics, name));
    return mt.release();
}

static osg::MatrixTransform* addCharacter(osgVerse::PhysicsEngine* physics,
                                          osgVerse::PhysicsCharacter* character, osg::Group* root)
{
    float halfHeight = character->totalHeight * 0.5f, feetHeight = character->totalHeight * 0.5f;
    float halfZ = feetHeight * 0.5f, radius = character->bodyRadius * 0.707f;
    float bottom = -halfHeight + feetHeight * 0.5f + radius, top = halfHeight - radius;

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    osg::ref_ptr<osg::ShapeDrawable> feet = new osg::ShapeDrawable(new osg::Box(
        osg::Vec3(0.0f, 0.0f, -halfHeight + halfZ),
        character->bodyRadius, character->bodyRadius, feetHeight));
    feet->setColor(osg::Vec4(0.35f, 0.75f, 0.35f, 1.0f)); geode->addDrawable(feet.get());

    if (top > bottom)
    {
        osg::ref_ptr<osg::ShapeDrawable> capsule = new osg::ShapeDrawable(new osg::Capsule(
            osg::Vec3(0.0f, 0.0f, (bottom + top) * 0.5f), radius, top - bottom));
        capsule->setColor(osg::Vec4(0.35f, 0.55f, 0.9f, 1.0f)); geode->addDrawable(capsule.get());
    }

    osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
    mt->addChild(geode.get()); root->addChild(mt.get());
    mt->setUpdateCallback(new osgVerse::PhysicsUpdateCallback(physics, character->getName()));
    return mt.release();
}

// Build the testing scene: ground, staircase, ramps, wall and some dynamic props
static void buildScene(osgVerse::PhysicsEngine* physics, osg::Group* root)
{
    addBox(physics, root, "ground", osg::Vec3(groundHalfSize, groundHalfSize, 0.05f),
           osg::Matrix::translate(0.0f, 0.0f, -0.05f), 0.0f, osg::Vec4(0.5f, 0.5f, 0.55f, 1.0f));

    // Staircase: 8 steps with a rise of 0.2m and a depth of 0.4m each
    for (int i = 0; i < 8; ++i)
    {
        float height = 0.2f * (i + 1), halfDepth = 0.2f;
        addBox(physics, root, "step" + std::to_string(i), osg::Vec3(2.0f, halfDepth, height * 0.5f),
               osg::Matrix::translate(0.0f, 5.0f + 0.4f * i, height * 0.5f), 0.0f,
               osg::Vec4(0.72f, 0.68f, 0.5f, 1.0f));
    }

    // Landing at the top of the staircase (top surface at the same height of the last step)
    addBox(physics, root, "landing", osg::Vec3(2.0f, 1.0f, 0.8f),
           osg::Matrix::translate(0.0f, 9.0f, 0.8f), 0.0f, osg::Vec4(0.72f, 0.68f, 0.5f, 1.0f));

    // Gentle ramp (about 20 degrees, the character should walk on it)
    addBox(physics, root, "ramp", osg::Vec3(1.5f, 3.0f, 0.1f),
           osg::Matrix::rotate(osg::DegreesToRadians(-20.0f), osg::X_AXIS)
         * osg::Matrix::translate(-6.0f, 4.0f, 1.0f), 0.0f, osg::Vec4(0.4f, 0.65f, 0.4f, 1.0f));

    // Steep ramp (about 50 degrees, too steep to stand on)
    addBox(physics, root, "steepRamp", osg::Vec3(1.5f, 2.5f, 0.1f),
           osg::Matrix::rotate(osg::DegreesToRadians(-50.0f), osg::X_AXIS)
         * osg::Matrix::translate(-6.0f, -4.0f, 2.0f), 0.0f, osg::Vec4(0.65f, 0.4f, 0.4f, 1.0f));

    // Wall: the character should be blocked by it instead of passing through
    addBox(physics, root, "wall", osg::Vec3(3.0f, 0.3f, 1.5f),
           osg::Matrix::translate(0.0f, 16.0f, 1.5f), 0.0f, osg::Vec4(0.35f, 0.35f, 0.45f, 1.0f));

    // Dynamic boxes: the character should be able to push them around
    for (int i = 0; i < 3; ++i)
        addBox(physics, root, "box" + std::to_string(i), osg::Vec3(0.4f, 0.4f, 0.4f),
               osg::Matrix::translate(-3.0f + 1.2f * i, 6.0f, 0.4f), 8.0f,
               osg::Vec4(0.85f, 0.75f, 0.3f, 1.0f));
}

// Input and third-person camera handler
class CharacterHandler : public osgGA::GUIEventHandler
{
public:
    CharacterHandler() : _yaw(0.0f), _pitch(-0.25f), _distance(5.0f), _mouseX(0), _mouseY(0),
                         _dragging(false), _sprint(false), _jumpRequest(false), _resetRequest(false)
    { for (int i = 0; i < 4; ++i) _keys[i] = false; }

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        if (ea.getEventType() == osgGA::GUIEventAdapter::PUSH)
        {
            if (ea.getButtonMask() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
            { _dragging = true; _mouseX = ea.getX(); _mouseY = ea.getY(); return true; }
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::RELEASE)
            _dragging = false;
        else if (ea.getEventType() == osgGA::GUIEventAdapter::DRAG && _dragging)
        {
            _yaw -= (ea.getX() - _mouseX) * 0.002f;
            _pitch = osg::minimum(osg::maximum(_pitch - (ea.getY() - _mouseY) * 0.002f, -1.2f), 1.2f);
            _mouseX = ea.getX(); _mouseY = ea.getY(); return true;
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::SCROLL)
        {
            _distance = osg::minimum(osg::maximum(
                _distance - (ea.getScrollingMotion() == osgGA::GUIEventAdapter::SCROLL_UP ? 0.5f : -0.5f), 1.5f), 20.0f);
            return true;
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN)
        {
            switch (ea.getKey())
            {
            case osgGA::GUIEventAdapter::KEY_W: _keys[0] = true; break;
            case osgGA::GUIEventAdapter::KEY_S: _keys[1] = true; break;
            case osgGA::GUIEventAdapter::KEY_A: _keys[2] = true; break;
            case osgGA::GUIEventAdapter::KEY_D: _keys[3] = true; break;
            case osgGA::GUIEventAdapter::KEY_Shift_L: _sprint = true; break;
            case osgGA::GUIEventAdapter::KEY_Space: _jumpRequest = true; break;
            }
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::KEYUP)
        {
            switch (ea.getKey())
            {
            case osgGA::GUIEventAdapter::KEY_W: _keys[0] = false; break;
            case osgGA::GUIEventAdapter::KEY_S: _keys[1] = false; break;
            case osgGA::GUIEventAdapter::KEY_A: _keys[2] = false; break;
            case osgGA::GUIEventAdapter::KEY_D: _keys[3] = false; break;
            case osgGA::GUIEventAdapter::KEY_Shift_L: _sprint = false; break;
            case osgGA::GUIEventAdapter::KEY_R: _resetRequest = true; break;
            }
        }
        return false;
    }

    // Wish velocity direction (length <= 1) in world space, computed from the camera yaw
    osg::Vec3 getWishDirection() const
    {
        osg::Vec3 forward(cosf(_yaw), sinf(_yaw), 0.0f), right(forward.y(), -forward.x(), 0.0f);
        osg::Vec3 wish = forward * (float)(_keys[0] ? 1 : (_keys[1] ? -1 : 0))
                       + right * (float)(_keys[3] ? 1 : (_keys[2] ? -1 : 0));
        return wish;
    }

    osg::Vec3 getCameraDirection() const
    {
        float cp = cosf(_pitch);
        return osg::Vec3(cosf(_yaw) * cp, sinf(_yaw) * cp, sinf(_pitch));
    }

    bool consumeJump() { bool b = _jumpRequest; _jumpRequest = false; return b; }
    bool consumeReset() { bool b = _resetRequest; _resetRequest = false; return b; }
    bool isSprinting() const { return _sprint; }
    float getDistance() const { return _distance; }

private:
    float _yaw, _pitch, _distance; int _mouseX, _mouseY;
    bool _dragging, _keys[4], _sprint, _jumpRequest, _resetRequest;
};

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());

    osg::ref_ptr<osgVerse::PhysicsEngine> physics;
    if (arguments.read("--bullet"))
        physics = dynamic_cast<osgVerse::PhysicsEngine*>(osgDB::readObjectFile("0.verse_bullet"));
    if (!physics) physics = new osgVerse::PhysicsEngine;
    OSG_NOTICE << "[osgVerse] Using physics engine: " << physics->getName() << "\n";

    // Create the scene and the physics rigid bodies
    osg::ref_ptr<osg::Group> root = new osg::Group;
    buildScene(physics.get(), root.get());

    // Create the character at the start position and add it to the scene
    osg::ref_ptr<osgVerse::PhysicsCharacter> character = new osgVerse::PhysicsCharacter(physics.get(), "character");
    if (!character->create(osg::Vec3(0.0f, 0.0f, characterStartHeight)))
    { OSG_WARN << "[osgVerse] Failed to create the character\n"; return 1; }

    addCharacter(physics.get(), character.get(), root.get());
    std::cout << "[osgVerse] Character: radius=" << character->bodyRadius << "m, height="
              << character->totalHeight << "m, walk speed=" << character->walkSpeed << "m/s\n";
    std::cout << "[osgVerse] Controls: W/A/S/D = move, Shift = sprint, Space = jump, "
              << "R = reset, Mouse = rotate camera\n";

    osg::ref_ptr<CharacterHandler> handler = new CharacterHandler;
    osgViewer::Viewer viewer;
    viewer.setUpViewOnSingleScreen(0);
    viewer.setSceneData(root.get());
    viewer.setCameraManipulator(NULL);  // the camera is updated manually to follow the character
    viewer.addEventHandler(handler.get());
    //viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.getCamera()->setClearColor(osg::Vec4(0.55f, 0.65f, 0.8f, 1.0f));

    const float timeStep = 1.0f / 60.0f;
    int frameCount = 0;
    while (!viewer.done())
    {
        if (handler->consumeReset())
        {
            physics->setTransform(character->getName(), osg::Matrix::translate(0.0f, 0.0f, characterStartHeight));
            character->setWishVelocity(osg::Vec3());
        }

        character->setWishVelocity(handler->getWishDirection() * 0.5f);
        character->setSprinting(handler->isSprinting());
        if (handler->consumeJump()) character->jump();
        character->step(timeStep); physics->advance(timeStep, 4); character->lateStep();

        // Follow the character with a third-person camera
        osg::Vec3 pivot = character->getPosition() + osg::Vec3(0.0f, 0.0f, 0.4f);
        osg::Vec3 direction = handler->getCameraDirection();
        viewer.getCamera()->setViewMatrixAsLookAt(
            pivot - direction * handler->getDistance(), pivot, osg::Z_AXIS);

        // Print the state of the character to console
        /*osg::Vec3 velocity = character->getVelocity();
        if ((frameCount % 30) == 0)
        {
            std::cout << "[State] position: " << character->getPosition() << ", feet: "
                      << character->getFeetPosition() << ", velocity: " << velocity << ", on ground: "
                      << (character->isOnGround() ? "yes" : "no") << std::endl;
        }*/
        frameCount++; viewer.frame();
    }
    return 0;
}
