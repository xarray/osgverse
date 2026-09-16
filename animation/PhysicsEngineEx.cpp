#include <osg/io_utils>
#include <osg/Version>
#include <osg/Notify>
#include <osg/Math>
#include "PhysicsEngine.h"
using namespace osgVerse;

/* PhysicsCharacter: a rigid-body based character controller with walking, jumping and stepping.
   The algorithm is ported from Box3D's sample_character.cpp (RigidbodyCharacter, s&box style),
   with the Y-up convention of the sample converted to the Z-up convention of osgVerse. */

// s&box unit conversion: 1 unit = 1 inch = 0.0254m (40 units per meter)
const float PhysicsCharacter::SRC = 0.0254f;

PhysicsCharacter::PhysicsCharacter(PhysicsEngine* engine, const std::string& name)
:   _engine(engine), _name(name), _jumpCooldown(0.0f), _onGround(false),
    _sprintRequest(false), _sprint(false), _didStep(false)
{
    walkSpeed = 230.0f * SRC;       // walk speed in meters per second
    runSpeed = 350.0f * SRC;        // sprint speed in meters per second
    jumpSpeed = 300.0f * SRC;       // initial jumping speed in meters per second
    maxSlopeAngle = 45.0f;          // maximum angle (in degrees) of a standable surface
    characterGravity = 15.0f;       // gravity scale of the character (default gravity is 9.8)
    characterMass = 500.0f;         // mass of the character in kilograms
    jumpCooldownTime = 0.2f;

    stepUpHeight = 18.0f * SRC;     // maximum height of a step the character can climb
    stepDownHeight = 18.0f * SRC;   // maximum height for sticking the character to ground
    skin = 0.095f * SRC;            // skin width used to avoid starting a trace solid

    brakePower = 0.2f; surfaceFriction = 0.6f; airFriction = 0.1f;
    bodyRadius = 16.0f * SRC;       // radius of the character body (widely known as 16x72 in s&box)
    totalHeight = 72.0f * SRC;      // total height of the character body

    _groundNormal.set(0.0f, 0.0f, 1.0f);
    _wishInput.set(0.0f, 0.0f, 0.0f);
    _wishVelocity.set(0.0f, 0.0f, 0.0f);
    _massCenter.set(0.0f, 0.0f, 0.0f);
    _stepPosition.set(0.0f, 0.0f, 0.0f);
}

PhysicsCharacter::~PhysicsCharacter()
{ destroy(); }

bool PhysicsCharacter::create(const osg::Vec3& position)
{
    if (!_engine) return false;
    destroy();

    PhysicsEngine::BodySetting setting;
    setting.gravityScale = characterGravity / 10.0f;
    setting.allowSleep = false; setting.enableContactRecycling = false;
    setting.lockAngularX = setting.lockAngularY = setting.lockAngularZ = true;
    _body = _engine->createBody(_name, setting, osg::Matrix::translate(position));
    if (!_body.valid()) return false;
    _massCenter = position;

    // Feet box (lower half): dynamic friction for braking and sliding on the ground
    PhysicsEngine::ShapeSetting shapeSetting; shapeSetting.friction = 0.0f;
    float halfHeight = totalHeight * 0.5f, feetHeight = totalHeight * 0.5f;
    float halfX = bodyRadius * 0.5f, halfY = bodyRadius * 0.5f, halfZ = feetHeight * 0.5f;
    osg::Vec3 feetOffset(0.0f, 0.0f, -halfHeight + halfZ);
    _feetShape = _engine->createPhysicsBox(osg::Vec3(halfX, halfY, halfZ), feetOffset);
    if (_engine->addShapeToBody(_body.get(), _feetShape.get(), characterMass * 0.4f, &shapeSetting) == NULL)
        OSG_WARN << "[PhysicsCharacter] Failed to create feet box shape\n";

    // Body capsule (upper half): zero friction so we can slide along walls
    float radius = bodyRadius * 0.707f;
    float bottom = -halfHeight + feetHeight * 0.5f + radius;
    float top = halfHeight - radius;
    if (top > bottom)
    {
        _bodyShape = _engine->createPhysicsCapsule(
            radius, osg::Vec3(0.0f, 0.0f, bottom), osg::Vec3(0.0f, 0.0f, top));
        if (_engine->addShapeToBody(_body.get(), _bodyShape.get(), characterMass * 0.6f, &shapeSetting) == NULL)
            OSG_WARN << "[PhysicsCharacter] Failed to create body capsule shape\n";
    }

    _jumpCooldown = 0.0f; _didStep = false; _onGround = false;
    updateMassCenter(0.0f); _massCenter = _engine->getCenterOfMass(_name);
    return true;
}

void PhysicsCharacter::destroy()
{
    _traceShapes.clear();
    if (_body.valid() && _engine.valid()) _engine->removeBody(_name);
    _body = NULL; _feetShape = NULL; _bodyShape = NULL;
    _onGround = false; _didStep = false; _jumpCooldown = 0.0f;
}

void PhysicsCharacter::setWishVelocity(const osg::Vec3& v)
{
    _wishInput = v; _wishInput.z() = 0.0f;  // walking is a horizontal-only movement
}

void PhysicsCharacter::jump()
{
    if (!_body.valid() || !_engine) return;
    if (_onGround && _jumpCooldown <= 0.0f)
    {
        osg::Vec3 velocity = _engine->getVelocity(_name, true);
        velocity.z() = jumpSpeed;
        _engine->setVelocity(_name, velocity, true);
        _onGround = false; _jumpCooldown = jumpCooldownTime;
    }
}

void PhysicsCharacter::step(float timeStep)
{
    if (!_body.valid() || !_engine) return;
    if (_jumpCooldown > 0.0f) _jumpCooldown -= timeStep;
    _sprint = _sprintRequest && _onGround;

    // Compute wish velocity from the input direction and current movement speed
    float maxSpeed = _sprint ? runSpeed : walkSpeed;
    osg::Vec3 wishVelocity = _wishInput;
    float wishSpeed = wishVelocity.length();
    if (wishSpeed > 1.0f) wishVelocity *= 1.0f / wishSpeed;  // keep the input as a direction
    wishVelocity *= maxSpeed; _wishVelocity = wishVelocity;

    updateBody(wishVelocity);   // friction, mass center, gravity scale and damping
    addVelocity(wishVelocity);  // apply wish velocity with the s&box-like velocity model
    _didStep = tryStep(stepUpHeight);
    _massCenter = _engine->getCenterOfMass(_name);
}

void PhysicsCharacter::lateStep()
{
    if (!_body.valid() || !_engine) return;
    restoreStep();
    reground(stepDownHeight);
    categorizeGround();
}

osg::Vec3 PhysicsCharacter::getPosition() const
{
    if (!_body.valid() || !_engine) return osg::Vec3();
    bool valid = false; osg::Matrix m = _engine->getTransform(_name, valid);
    if (!valid) return osg::Vec3();
    osg::Vec3d p = m.getTrans();
    return osg::Vec3((float)p.x(), (float)p.y(), (float)p.z());
}

osg::Vec3 PhysicsCharacter::getFeetPosition() const
{ return getPosition() - osg::Vec3(0.0f, 0.0f, totalHeight * 0.5f); }

osg::Vec3 PhysicsCharacter::getVelocity() const
{
    if (!_body.valid() || !_engine) return osg::Vec3();
    return _engine->getVelocity(_name, true);
}

CollisionShapeBase* PhysicsCharacter::getTraceShape(float radiusScale, float heightScale)
{
    int key = int(radiusScale * 10.0f + 0.5f) * 100 + int(heightScale * 10.0f + 0.5f);
    std::map<int, osg::ref_ptr<CollisionShapeBase>>::iterator itr = _traceShapes.find(key);
    if (itr != _traceShapes.end()) return itr->second.get();

    // The tracing box spans from its bottom (the trace origin) to the given height
    osg::ref_ptr<CollisionShapeBase> shape = _engine->createPhysicsBox(osg::Vec3(
        bodyRadius * 0.5f * radiusScale, bodyRadius * 0.5f * radiusScale, totalHeight * heightScale * 0.5f));
    _traceShapes[key] = shape; return shape.get();
}

PhysicsCharacter::TraceResult PhysicsCharacter::traceBody(const osg::Vec3& from, const osg::Vec3& to,
                                                          float radiusScale, float heightScale)
{
    TraceResult result; result.endPosition = to; result.hitPoint = to;
    result.normal.set(0.0f, 0.0f, 1.0f);
    osg::Vec3 translation = to - from;
    if (translation.length() < 1e-6f) return result;

    // The trace box bottom is aligned with the given "from" position
    CollisionShapeBase* shape = getTraceShape(radiusScale, heightScale);
    if (!shape) return result;
    osg::Vec3 offset(0.0f, 0.0f, totalHeight * heightScale * 0.5f);
    PhysicsEngine::SweepResult sr = _engine->sweep(
        from + offset, translation, shape, PhysicsEngine::QueryFilter(), _body.get());

    result.startedSolid = sr.startedSolid;
    if (sr.hit)
    {
        result.hit = true; result.fraction = sr.fraction; result.normal = sr.normal;
        result.hitPoint = sr.point; result.endPosition = from + translation * sr.fraction;
    }
    return result;
}

bool PhysicsCharacter::isStandableSurface(const osg::Vec3& normal) const
{
    float maxSlopeCos = cosf(osg::DegreesToRadians(maxSlopeAngle));
    return (normal * osg::Vec3(0.0f, 0.0f, 1.0f)) >= maxSlopeCos;
}

void PhysicsCharacter::updateGround(bool onGround, const osg::Vec3& normal)
{ _onGround = onGround; _groundNormal = normal; }

void PhysicsCharacter::categorizeGround()
{
    osg::Vec3 feet = getFeetPosition();
    osg::Vec3 from = feet + osg::Vec3(0.0f, 0.0f, 4.0f * SRC);
    osg::Vec3 to = feet - osg::Vec3(0.0f, 0.0f, 2.0f * SRC);

    // Shrink the tracing radius while the character is stuck in geometry or facing a steep surface
    float radiusScale = 1.0f;
    TraceResult tr = traceBody(from, to, radiusScale, 0.5f);
    while (tr.startedSolid || (tr.hit && !isStandableSurface(tr.normal)))
    {
        radiusScale -= 0.1f;
        if (radiusScale < 0.7f) { updateGround(false, osg::Vec3(0.0f, 0.0f, 1.0f)); return; }
        tr = traceBody(from, to, radiusScale, 0.5f);
    }

    if (!tr.startedSolid && tr.hit && isStandableSurface(tr.normal) && _jumpCooldown <= 0.0f)
        updateGround(true, tr.normal);
    else
        updateGround(false, osg::Vec3(0.0f, 0.0f, 1.0f));
}

void PhysicsCharacter::reground(float stepSize)
{
    if (!_onGround) return;
    float halfHeight = totalHeight * 0.5f;
    osg::Vec3 pos = getPosition();
    osg::Vec3 feet = pos - osg::Vec3(0.0f, 0.0f, halfHeight);
    osg::Vec3 from = feet + osg::Vec3(0.0f, 0.0f, 0.05f);
    osg::Vec3 to = feet - osg::Vec3(0.0f, 0.0f, stepSize);

    float radiusScale = 1.0f;
    TraceResult tr = traceBody(from, to, radiusScale, 0.5f);
    while (tr.startedSolid)
    {
        radiusScale -= 0.1f;
        if (radiusScale < 0.7f) return;
        tr = traceBody(from, to, radiusScale, 0.5f);
    }
    if (!tr.hit) return;

    // Stick the character to the ground, kill the vertical velocity to prevent bouncing
    osg::Vec3 target(tr.endPosition.x(), tr.endPosition.y(), tr.endPosition.z() + 0.01f + halfHeight);
    bool valid = false; osg::Matrix m = _engine->getTransform(_name, valid);
    _engine->setTransform(_name, osg::Matrix::rotate(m.getRotate()) * osg::Matrix::translate(target));
    if (target.z() - pos.z() > 0.01f)
    {
        osg::Vec3 velocity = _engine->getVelocity(_name, true);
        velocity.z() = 0.0f; _engine->setVelocity(_name, velocity, true);
    }
}

void PhysicsCharacter::restoreStep()
{
    if (!_didStep || !_engine) return;
    bool valid = false; osg::Matrix m = _engine->getTransform(_name, valid);
    _engine->setTransform(_name, osg::Matrix::rotate(m.getRotate()) * osg::Matrix::translate(_stepPosition));
    _didStep = false;
}

osg::Vec3 PhysicsCharacter::addClamped(const osg::Vec3& current, const osg::Vec3& add, float maxAddLength)
{
    float addLength = add.length();
    if (addLength > maxAddLength && addLength > 0.0f)
        return current + add * (maxAddLength / addLength);
    return current + add;
}

void PhysicsCharacter::updateMassCenter(float wishSpeed)
{
    float halfHeight = totalHeight * 0.5f;
    if (_onGround)
    {
        // s&box: massCenter = clamp(WishSpeed, 0, halfHeight), which lowers the mass center for stability
        float centerOffset = osg::minimum(osg::maximum(wishSpeed, 0.0f), halfHeight);
        _engine->setMassCenter(_name, osg::Vec3(0.0f, 0.0f, centerOffset - halfHeight));
    }
    else
        _engine->setMassCenter(_name, osg::Vec3());
}

void PhysicsCharacter::updateBody(const osg::Vec3& wishVelocity)
{
    float wishLength = wishVelocity.length();
    osg::Vec3 velocity = _engine->getVelocity(_name, true);
    float velocityLength = velocity.length();

    // Feet friction: use brakes when the character wants to stop or slow down
    float feetFriction = 0.0f;
    if (_onGround)
    {
        bool wantsBrakes = wishLength < (5.0f * SRC) || wishLength < velocityLength * 0.9f;
        if (wantsBrakes) feetFriction = 1.0f + 100.0f * brakePower * surfaceFriction;
    }
    _engine->setShapeFriction(_feetShape.get(), feetFriction);
    updateMassCenter(wishLength);

    // Gravity is disabled when the character stands still on a stable ground
    bool wantsGravity = !_onGround || velocityLength > (1.0f * SRC);
    _engine->setGravityScale(_name, wantsGravity ? (characterGravity / 10.0f) : 0.0f);

    bool wantsDamping = _onGround && wishLength < (1.0f * SRC);
    _engine->setLinearDamping(_name, wantsDamping ? 10.0f * brakePower : airFriction);
}

void PhysicsCharacter::addVelocity(const osg::Vec3& wishVelocity)
{
    float wishLength = wishVelocity.length();
    if (wishLength < 0.001f) return;

    float groundFrictionFactor = 0.25f + surfaceFriction * 10.0f;
    osg::Vec3 velocity = _engine->getVelocity(_name, true);
    float savedZ = velocity.z();
    float speed = velocity.length();
    float maxSpeed = osg::maximum(wishLength, speed);

    if (_onGround)
    {
        float amount = 1.0f * groundFrictionFactor;
        velocity = addClamped(velocity, wishVelocity * amount, wishLength * amount);
    }
    else
        velocity = addClamped(velocity, wishVelocity * 0.05f, wishLength);

    float newSpeed = velocity.length();
    if (newSpeed > maxSpeed && newSpeed > 0.0f) velocity *= maxSpeed / newSpeed;
    if (_onGround) velocity.z() = savedZ;  // walking doesn't change the vertical velocity
    _engine->setVelocity(_name, velocity, true);
}

bool PhysicsCharacter::tryStep(float maxStepHeight)
{
    if (!_onGround) return false;
    float halfHeight = totalHeight * 0.5f;
    osg::Vec3 pos = getPosition();
    osg::Vec3 feet = pos - osg::Vec3(0.0f, 0.0f, halfHeight);
    osg::Vec3 velocity = _engine->getVelocity(_name, true);

    // Only step when moving
    osg::Vec3 hVelocity(velocity.x(), velocity.y(), 0.0f);
    float hSpeed = hVelocity.length();
    if (hSpeed < 0.01f) return false;
    osg::Vec3 moveDir = hVelocity * (1.0f / hSpeed);

    // Phase 1 - FORWARD: trace the whole body forward in the velocity direction.
    // The tracing box is lifted a little from the feet to avoid starting inside the ground.
    float forwardDist = hSpeed * (1.0f / 60.0f) + bodyRadius;  // one frame of movement + radius
    osg::Vec3 traceFeet = feet + osg::Vec3(0.0f, 0.0f, 0.05f);
    osg::Vec3 forwardFrom = traceFeet - moveDir * skin, forwardTo = traceFeet + moveDir * forwardDist;
    float radiusScale = 1.0f;
    TraceResult trForward = traceBody(forwardFrom, forwardTo, radiusScale, 1.0f);
    while (trForward.startedSolid)
    {
        radiusScale -= 0.1f;
        if (radiusScale < 0.6f) return false;
        trForward = traceBody(forwardFrom, forwardTo, radiusScale, 1.0f);
    }
    if (!trForward.hit) return false;  // no obstacle ahead, no step needed

    // Phase 2 - LIFT: the tracing box is lifted by the maximum step height and then moved forward
    // by at most one frame of walking. Limiting the crossing distance keeps the speed on stairs
    // and slopes the same as the walking speed on flat ground, and the character never gains
    // extra distance from a step.
    float acrossDist = forwardDist * (1.0f - trForward.fraction) + bodyRadius * 0.5f;
    acrossDist = osg::minimum(acrossDist, hSpeed * (1.0f / 60.0f));
    osg::Vec3 liftedFrom = traceFeet + osg::Vec3(0.0f, 0.0f, maxStepHeight);
    osg::Vec3 liftedTo = liftedFrom + moveDir * acrossDist;
    TraceResult trAcross = traceBody(liftedFrom, liftedTo, radiusScale, 1.0f);
    if (trAcross.startedSolid) return false;
    osg::Vec3 acrossPos = trAcross.hit ? trAcross.endPosition : liftedTo;

    // Phase 3 - DOWN: from the across position, trace straight down to find a standable surface
    osg::Vec3 downFrom = acrossPos;
    osg::Vec3 downTo = acrossPos - osg::Vec3(0.0f, 0.0f, maxStepHeight + 0.05f);
    TraceResult trDown = traceBody(downFrom, downTo, radiusScale, 1.0f);
    if (!trDown.hit || !isStandableSurface(trDown.normal)) return false;

    // Check that we actually stepped up instead of moving laterally
    if (trDown.endPosition.z() - traceFeet.z() < 0.01f) return false;

    // Teleport the body to the step position and adjust its velocity
    osg::Vec3 stepPos(trDown.endPosition.x(), trDown.endPosition.y(),
                      trDown.endPosition.z() + 0.01f + halfHeight);
    bool valid = false; osg::Matrix m = _engine->getTransform(_name, valid);
    _engine->setTransform(_name, osg::Matrix::rotate(m.getRotate()) * osg::Matrix::translate(stepPos));

    osg::Vec3 newVelocity = _engine->getVelocity(_name, true);
    newVelocity.x() *= 0.9f; newVelocity.y() *= 0.9f; newVelocity.z() = 0.0f;
    _engine->setVelocity(_name, newVelocity, true);

    _stepPosition = stepPos; return true;
}

/* PhysicsHuman: an articulated ragdoll made of bone boxes. Every joint is created at the rest pose,
   so a zero joint target always keeps the rest pose, and the drive can be released to let the human
   fall down as a limp ragdoll. The joint frames are the reference of the drive: the target of every
   joint is the relative rotation of the two joint frames. */

PhysicsHuman::PhysicsHuman(PhysicsEngine* engine, const std::string& name)
:   _engine(engine), _name(name), _totalMass(0.0f), _ragdoll(false), _driveReleased(true),
    collisionGroup(0)
{}

PhysicsHuman::~PhysicsHuman()
{ destroy(); }

osg::Matrix PhysicsHuman::makeBoneFrame(const osg::Vec3& pos, const osg::Vec3& xAxis, const osg::Vec3& zAxis)
{
    osg::Vec3 x = xAxis; x.normalize();
    osg::Vec3 z = zAxis - x * (zAxis * x); z.normalize();
    osg::Vec3 y = z ^ x;

    // In OSG's row vector convention the rows of a rotation matrix are the images of the x/y/z axes
    osg::Matrix rot(x[0], x[1], x[2], 0.0f, y[0], y[1], y[2], 0.0f, z[0], z[1], z[2], 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
    return rot * osg::Matrix::translate(pos);
}

std::vector<PhysicsHuman::BoneDefinition> PhysicsHuman::getSimpleBones()
{
    // A simple human of about 1.7m in T-pose: facing +Y, z axis is up, feet at z = 0. The joint
    // frames of hinges have their x axis along the bone (the hinge rotates around the z axis), while
    // spherical joints have their z axis along the bone (the cone is around the bone direction).
    std::vector<BoneDefinition> bones;
    struct BoneHelper
    {
        std::vector<BoneDefinition>& bones;
        int addBall(const std::string& name, int parent, const osg::Vec3& pos, const osg::Vec3& dir,
                    const osg::Vec3& twistRef, float radius, float mass, float cone, float twist,
                    float length = 0.2f)
        {
            BoneDefinition bone; bone.name = name; bone.parent = parent;
            bone.frame = makeBoneFrame(pos, twistRef, dir);
            bone.boneRadius = radius; bone.mass = mass; bone.boneLength = length;
            bone.jointType = osgVerse::PhysicsEngine::CONSTRAINT_CONE_TWIST;
            bone.coneLimit = cone; bone.lowerLimit = -twist; bone.upperLimit = twist;
            bones.push_back(bone); return (int)bones.size() - 1;
        }

        int addHinge(const std::string& name, int parent, const osg::Vec3& pos, const osg::Vec3& dir,
                     const osg::Vec3& axis, float radius, float mass, float lower, float upper,
                     float length = 0.2f)
        {
            BoneDefinition bone; bone.name = name; bone.parent = parent;
            bone.frame = makeBoneFrame(pos, dir, axis);
            bone.boneRadius = radius; bone.mass = mass; bone.boneLength = length;
            bone.jointType = osgVerse::PhysicsEngine::CONSTRAINT_HINGE;
            bone.lowerLimit = lower; bone.upperLimit = upper;
            bones.push_back(bone); return (int)bones.size() - 1;
        }
    } helper = { bones };

    const osg::Vec3 X_AXIS(1.0f, 0.0f, 0.0f), Y_AXIS(0.0f, 1.0f, 0.0f), Z_AXIS(0.0f, 0.0f, 1.0f);
    const osg::Vec3 up(0.0f, 0.0f, 1.0f), down(0.0f, 0.0f, -1.0f);

    // Spine: the head of the chain is the hip bone, which is also the root of the human
    int hip = helper.addBall("hip", -1, osg::Vec3(0.0f, 0.0f, 0.95f), up, X_AXIS, 0.10f, 10.0f, 0.0f, 0.0f);
    int spine1 = helper.addBall("spine1", hip, osg::Vec3(0.0f, 0.0f, 1.07f), up, X_AXIS, 0.10f, 8.0f, 0.25f, 0.30f);
    int spine2 = helper.addBall("spine2", spine1, osg::Vec3(0.0f, 0.0f, 1.19f), up, X_AXIS, 0.11f, 8.0f, 0.25f, 0.30f);
    int neck = helper.addBall("neck", spine2, osg::Vec3(0.0f, 0.0f, 1.31f), up, X_AXIS, 0.045f, 1.0f, 0.30f, 0.40f);
    helper.addBall("head", neck, osg::Vec3(0.0f, 0.0f, 1.39f), up, X_AXIS, 0.095f, 5.0f, 0.30f, 0.30f, 0.18f);

    // Arms: the shoulder is linked to the upper spine, and the elbow is a hinge
    for (int i = 0; i < 2; ++i)
    {
        float s = (i == 0) ? 1.0f : -1.0f;  // 0 = left, 1 = right
        std::string side = (i == 0) ? "L" : "R";
        osg::Vec3 dir(s, 0.0f, 0.0f);
        int shoulder = helper.addBall("shoulder" + side, spine2, osg::Vec3(s * 0.05f, 0.0f, 1.28f),
                                      dir, Z_AXIS, 0.055f, 2.0f, 0.25f, 0.20f);
        int upperArm = helper.addBall("upperArm" + side, shoulder, osg::Vec3(s * 0.19f, 0.0f, 1.28f),
                                      dir, Z_AXIS, 0.045f, 2.5f, 0.70f, 0.50f);
        int lowerArm = helper.addHinge("lowerArm" + side, upperArm, osg::Vec3(s * 0.44f, 0.0f, 1.28f),
                                       dir, osg::Vec3(0.0f, 0.0f, s), 0.04f, 1.5f, 0.0f, 2.10f);
        helper.addBall("hand" + side, lowerArm, osg::Vec3(s * 0.68f, 0.0f, 1.28f), dir, Z_AXIS,
                       0.045f, 0.5f, 0.40f, 0.30f, 0.10f);
    }

    // Legs: the knee and the ankle are hinges. Both legs use the same frames as the human is
    // symmetric around the yz plane, so a positive knee angle always bends the leg backwards.
    // The ankles are right under the body weight, so the human doesn't need to hold itself up.
    for (int i = 0; i < 2; ++i)
    {
        float s = (i == 0) ? 1.0f : -1.0f;
        std::string side = (i == 0) ? "L" : "R";
        int upperLeg = helper.addBall("upperLeg" + side, hip, osg::Vec3(s * 0.11f, 0.0f, 0.95f),
                                      down, X_AXIS, 0.07f, 8.0f, 0.80f, 0.35f);
        int lowerLeg = helper.addHinge("lowerLeg" + side, upperLeg, osg::Vec3(s * 0.11f, 0.0f, 0.53f),
                                       down, osg::Vec3(-1.0f, 0.0f, 0.0f), 0.055f, 3.5f, 0.0f, 2.00f);
        int foot = helper.addHinge("foot" + side, lowerLeg, osg::Vec3(s * 0.11f, 0.0f, 0.09f),
                                   Y_AXIS, X_AXIS, 0.09f, 1.2f, -0.50f, 0.60f, 0.16f);
        bones[foot].boxOffset.set(-0.08f, 0.0f, 0.0f);  // the foot box stands on the ankle
    }
    return bones;
}

bool PhysicsHuman::create(const osg::Vec3& position, const std::vector<BoneDefinition>& bones)
{
    if (!_engine || bones.empty()) return false;
    destroy(); _bones = bones;

    // The box of a bone is centered at the middle of the bone, so that the center of mass of every
    // body is at its own origin. The joint frames are then exactly the given rest pose frames, which
    // makes a zero drive target equal to the rest pose.
    std::vector<osg::Matrix> bodyMatrices(_bones.size()), frameBs(_bones.size());
    std::vector<int> boxAxes(_bones.size(), 0);
    for (size_t i = 0; i < _bones.size(); ++i)  // move the human to the given position first
        _bones[i].frame = _bones[i].frame * osg::Matrix::translate(position);

    for (size_t i = 0; i < _bones.size(); ++i)
    {
        BoneDefinition& bone = _bones[i];
        int child = -1;
        for (size_t j = i + 1; j < _bones.size(); ++j)
        { if (_bones[j].parent == (int)i) { child = (int)j; break; } }

        float halfLength = bone.boneLength * 0.5f;
        osg::Vec3 axis(0.0f, 0.0f, 0.0f);  // local direction of the bone, snapped to a box axis
        boxAxes[i] = (bone.jointType == PhysicsEngine::CONSTRAINT_HINGE) ? 0 : 2;
        axis[boxAxes[i]] = 1.0f;
        if (child >= 0)
        {
            // The direction to the child bone is known, and it should be along an axis of the frame
            osg::Vec3 dir = _bones[child].frame.getTrans() - bone.frame.getTrans();
            halfLength = dir.length() * 0.5f;
            dir = bone.frame.getRotate().inverse() * dir;  // direction in the bone's local space
            int axisIndex = 0;
            for (int k = 1; k < 3; ++k)
            { if (fabsf(dir[k]) > fabsf(dir[axisIndex])) axisIndex = k; }
            axis = osg::Vec3(); axis[axisIndex] = (dir[axisIndex] < 0.0f) ? -1.0f : 1.0f;
            boxAxes[i] = axisIndex;
        }

        // The body is placed so that its center is at the middle of the bone, while its joint frame
        // (the origin of the bone) is at the beginning of it
        osg::Vec3 boxCenter = axis * halfLength + bone.boxOffset;
        bodyMatrices[i] = osg::Matrix::translate(boxCenter) * bone.frame;
        frameBs[i] = osg::Matrix::translate(-boxCenter);
        bone.boneLength = halfLength * 2.0f;
    }

    // Create all bodies first, so that joints can link them in any order
    PhysicsEngine::BodySetting bodySetting;
    bodySetting.allowSleep = false; bodySetting.angularDamping = 0.1f;
    for (size_t i = 0; i < _bones.size(); ++i)
    {
        _bodyNames.push_back(_name + "." + _bones[i].name);
        _jointNames.push_back(_name + "." + _bones[i].name + ".joint");

        osg::Vec3 halfSize(_bones[i].boneRadius, _bones[i].boneRadius, _bones[i].boneRadius);
        halfSize[boxAxes[i]] = _bones[i].boneLength * 0.5f;
        _boxSizes.push_back(halfSize);

        PhysicsEngine::ShapeSetting shapeSetting;
        shapeSetting.friction = 0.8f; shapeSetting.restitution = 0.0f;
        shapeSetting.collisionGroup = collisionGroup;
        RigidBodyBase* body = _engine->createBody(_bodyNames[i], bodySetting, bodyMatrices[i]);
        _engine->addShapeToBody(body, _engine->createPhysicsBox(halfSize), _bones[i].mass, &shapeSetting);
        _bodies.push_back(body);
    }

    // Torque limits of the joints are estimated from the mass of the subtree below them, with a
    // floor taken from the whole human: joints like the ankles have to carry the body weight
    _totalMass = 0.0f;
    for (size_t i = 0; i < _bones.size(); ++i) _totalMass += _bones[i].mass;

    _jointTorques.assign(_bones.size(), 0.0f);
    for (int i = (int)_bones.size() - 1; i >= 0; --i)
    {
        _jointTorques[i] += _bones[i].mass;  // parents are always defined before their children
        int p = _bones[i].parent;
        if (p >= 0) _jointTorques[p] += _jointTorques[i];
    }
    for (size_t i = 0; i < _bones.size(); ++i)
    {
        _jointTorques[i] = osg::maximum(_jointTorques[i], _totalMass * 0.8f);  // a strong human
        _jointTorques[i] *= 9.8f;
    }

    // Link the bones: local frames are derived from the rest pose of the two bodies
    for (size_t i = 0; i < _bones.size(); ++i)
    {
        int parent = _bones[i].parent;
        if (parent < 0)
        { _localFramesA.push_back(osg::Matrix()); _localFramesB.push_back(osg::Matrix()); continue; }

        _localFramesA.push_back(_bones[i].frame * osg::Matrix::inverse(bodyMatrices[parent]));
        _localFramesB.push_back(frameBs[i]);

        PhysicsEngine::ConstraintSetting cs;
        cs.enableLimit = true; cs.coneLimit = _bones[i].coneLimit;
        cs.lowerLimit = _bones[i].lowerLimit; cs.upperLimit = _bones[i].upperLimit;

        /* The joints are created with their drive already turned on, so that a backend which has to
           build a driven joint in another way than a limp one (Bullet, for example) can do that.
           step() then updates the drive, or releases it to get a pure ragdoll. */
        cs.enableSpring = true;
        cs.hertz = _drive.frequency; cs.dampingRatio = _drive.dampingRatio;
        cs.maxSpringForce = cs.maxMotorTorque = (_drive.maxTorque > 0.0f) ? _drive.maxTorque
                                                : (_jointTorques[i] * _drive.torqueScale);

        ConstraintBase* joint = _engine->createConstraint(
            _bodies[parent].get(), _localFramesA[i], _bodies[i].get(), _localFramesB[i],
            _bones[i].jointType, &cs);
        if (joint) _engine->addConstraint(_jointNames[i], joint);
        else OSG_WARN << "[PhysicsHuman] Failed to create the joint of bone " << _bones[i].name << "\n";
    }

    // The root bone is balanced by the drive when it is enabled: joint drives alone can hold the
    // pose of the human, but they are not able to keep its whole body from toppling over
    _rootFrame = bodyMatrices[0];

    _poseMatrices.clear(); _ragdoll = false; _driveReleased = false;
    return true;
}

void PhysicsHuman::destroy()
{
    if (_engine.valid())
    {
        for (size_t i = 0; i < _jointNames.size(); ++i) _engine->removeConstraint(_jointNames[i]);
        for (size_t i = 0; i < _bodyNames.size(); ++i) _engine->removeBody(_bodyNames[i]);
    }

    _bones.clear(); _bodyNames.clear(); _jointNames.clear(); _bodies.clear(); _boxSizes.clear();
    _localFramesA.clear(); _localFramesB.clear(); _jointTorques.clear();
    _poseMatrices.clear(); _rootFrame = osg::Matrix(); _totalMass = 0.0f;
    _ragdoll = false; _driveReleased = true;
}

int PhysicsHuman::getBoneIndex(const std::string& boneName) const
{
    for (size_t i = 0; i < _bones.size(); ++i)
    { if (_bones[i].name == boneName) return (int)i; }
    return -1;
}

std::vector<osg::Matrix> PhysicsHuman::getPoseMatrices() const
{
    std::vector<osg::Matrix> result;
    for (size_t i = 0; i < _bodies.size(); ++i)
    {
        bool valid = false;
        osg::Matrix matrix = _engine->getTransform(_bodyNames[i], valid);
        if (!valid) { result.push_back(osg::Matrix()); continue; }

        // The joint frame is the beginning of the bone, and not the center of its body
        result.push_back((i < _localFramesB.size()) ? (_localFramesB[i] * matrix) : matrix);
    }
    return result;
}

void PhysicsHuman::step(float timeStep)
{
    if (!_engine || _bodies.empty()) return;
    if (_drive.enabled && !_ragdoll)
    {
        _driveReleased = false;
        for (size_t i = 0; i < _bones.size(); ++i)
        {
            if (_bones[i].parent < 0) holdRoot();
            else driveJoint((unsigned int)i, timeStep);
        }
        return;
    }

    // Release the drive of the joints, so that the human becomes a limp ragdoll
    if (_driveReleased) return;
    _driveReleased = true;
    for (size_t i = 0; i < _bones.size(); ++i)
    {
        PhysicsEngine::ConstraintSetting cs;
        cs.enableLimit = true; cs.coneLimit = _bones[i].coneLimit;
        cs.lowerLimit = _bones[i].lowerLimit; cs.upperLimit = _bones[i].upperLimit;
        _engine->setConstraintSetting(_jointNames[i], cs);
    }
}

void PhysicsHuman::holdRoot()
{
    /* The root bone is held at its target pose, like an invisible hand on the hips. The joint drives
       are able to hold the pose of the human, but they can't keep its whole body from toppling over:
       doing that with impulses would need a torque which the small inertia of a hip can't take at a
       time step of a few milliseconds. Note that holding the root doesn't make it kinematic: all the
       bones are still dynamic, and the human falls down as soon as the drive is released. */
    osg::Matrix target = (_poseMatrices.size() == _bones.size()) ? _poseMatrices[0] : _rootFrame;
    _engine->setTransform(_bodyNames[0], target);
    _engine->setVelocity(_bodyNames[0], osg::Vec3(), true);
    _engine->setVelocity(_bodyNames[0], osg::Vec3(), false);
}

void PhysicsHuman::driveJoint(unsigned int index, float timeStep)
{
    int parent = _bones[index].parent;
    const BoneDefinition& bone = _bones[index];

    /* The drive acts on the joint frames of the two bones: its target is the relative rotation of
       these frames, which is the identity rotation at the rest pose of the human. Note that the
       rotation of frame B related to frame A, in the space of frame A, is B * A^-1 as OSG applies
       the matrices from the left to the right. The pose matrices are model space joint matrices, so
       the relative rotation they give is the one the joint frames would have if the bones followed
       the animation (see create() for the frames). */
    osg::Quat targetQ;  // an empty pose list keeps the rest pose of the human
    if (_poseMatrices.size() == _bones.size())
    {
        osg::Matrix poseA = _localFramesA[index] * _poseMatrices[parent];
        osg::Matrix poseB = _localFramesB[index] * _poseMatrices[index];
        targetQ = poseB.getRotate() * poseA.getRotate().inverse();
    }

    /* The joints are driven by the springs of the engine, which are part of its constraint solver:
       the solver knows the mass and the inertia of the whole chain of bones, so its springs can be
       stiff enough to hold the human up while the joints of the chain are solved together. Engines
       without springs on their joints (Bullet, for example) drive them with torque limited motors
       instead, which are set by the same call. */
    PhysicsEngine::ConstraintSetting cs;
    cs.enableLimit = true; cs.coneLimit = bone.coneLimit;
    cs.lowerLimit = bone.lowerLimit; cs.upperLimit = bone.upperLimit;
    cs.enableSpring = true; cs.hertz = _drive.frequency; cs.dampingRatio = _drive.dampingRatio;
    float torqueLimit = (_drive.maxTorque > 0.0f) ? _drive.maxTorque
                                                  : (_jointTorques[index] * _drive.torqueScale);
    cs.maxSpringForce = cs.maxMotorTorque = torqueLimit;
    if (bone.jointType == PhysicsEngine::CONSTRAINT_HINGE)
        cs.targetAngle = 2.0f * atan2f((float)targetQ.z(), (float)targetQ.w());
    else
        cs.targetRotation = targetQ;
    _engine->setConstraintSetting(_jointNames[index], cs);

    /* A motor drives the joint as well: the wanted relative speed is derived from the error of the
       joint, and the impulse is the one which makes the two bones reach it, limited by the strength
       of the joint. It makes the drive stronger on the engines whose joints have no springs to be
       driven by, and it is simply added to the springs on the other engines. */
    bool valid = false;
    osg::Matrix bodyA = _engine->getTransform(_bodyNames[parent], valid); if (!valid) return;
    osg::Matrix bodyB = _engine->getTransform(_bodyNames[index], valid); if (!valid) return;
    osg::Matrix frameA = _localFramesA[index] * bodyA, frameB = _localFramesB[index] * bodyB;
    osg::Quat errorQ = targetQ * (frameB.getRotate() * frameA.getRotate().inverse()).inverse();
    if (errorQ.w() < 0.0f) errorQ = osg::Quat(-errorQ.x(), -errorQ.y(), -errorQ.z(), -errorQ.w());
    double angle = 2.0 * acos(osg::minimum((double)errorQ.w(), 1.0));
    osg::Vec3 axis(errorQ.x(), errorQ.y(), errorQ.z());  // the rotation axis in the space of frame A
    if (angle < 1.0e-6 || axis.normalize() <= 0.0f) return;  // the joint is already at its target
    osg::Vec3 axisUnit = frameA.getRotate() * axis;          // the same axis, but in world space

    // Inertia seen by the joint: an impulse along the axis changes the relative rotation speed of
    // the two bones by (invIA + invIB) multiplied by it, projected on the same axis
    osg::Matrix invA = _engine->getInverseInertia(_bodyNames[index]);
    osg::Matrix invB = _engine->getInverseInertia(_bodyNames[parent]);
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c) invA(r, c) = invA(r, c) + invB(r, c);

    float inertia = 1.0f / osg::maximum(axisUnit * (axisUnit * invA), 1.0e-6f);
    osg::Vec3 relVel = _engine->getVelocity(_bodyNames[index], false)
                     - _engine->getVelocity(_bodyNames[parent], false);
    float wantedSpeed = osg::clampTo((float)angle * _drive.speed,
                                     -_drive.maxSpeed, _drive.maxSpeed);
    osg::Vec3 impulse = axisUnit * ((wantedSpeed - (relVel * axisUnit)) * inertia);
    float impulseLimit = torqueLimit * timeStep;
    if (impulse.length() > impulseLimit) impulse *= impulseLimit / impulse.length();
    _engine->applyImpulse(_bodyNames[index], frameB.getTrans(), impulse, true, false);
    _engine->applyImpulse(_bodyNames[parent], frameA.getTrans(), -impulse, true, false);
}
