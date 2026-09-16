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
    float halfHeight = totalHeight * 0.5f, feetHeight = totalHeight * 0.5f;
    float halfX = bodyRadius * 0.5f, halfY = bodyRadius * 0.5f, halfZ = feetHeight * 0.5f;
    osg::Vec3 feetOffset(0.0f, 0.0f, -halfHeight + halfZ);
    _feetShape = _engine->createPhysicsBox(osg::Vec3(halfX, halfY, halfZ), feetOffset);
    if (_engine->addShapeToBody(_body.get(), _feetShape.get(), characterMass * 0.4f, 0.0f) == NULL)
        OSG_WARN << "[PhysicsCharacter] Failed to create feet box shape\n";

    // Body capsule (upper half): zero friction so we can slide along walls
    float radius = bodyRadius * 0.707f;
    float bottom = -halfHeight + feetHeight * 0.5f + radius;
    float top = halfHeight - radius;
    if (top > bottom)
    {
        _bodyShape = _engine->createPhysicsCapsule(
            radius, osg::Vec3(0.0f, 0.0f, bottom), osg::Vec3(0.0f, 0.0f, top));
        if (_engine->addShapeToBody(_body.get(), _bodyShape.get(), characterMass * 0.6f, 0.0f) == NULL)
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

    // Phase 2 - UP: trace straight up from the hit position. The tracing box is moved a little
    // backward, otherwise it will start inside the obstacle the character is just facing.
    osg::Vec3 hitPos = trForward.endPosition - moveDir * (bodyRadius * 0.1f);
    osg::Vec3 upFrom = hitPos, upTo(hitPos.x(), hitPos.y(), hitPos.z() + maxStepHeight);
    TraceResult trUp = traceBody(upFrom, upTo, radiusScale, 1.0f);
    if (trUp.startedSolid) return false;

    osg::Vec3 topPos = trUp.hit ? trUp.endPosition : upTo;
    if (topPos.z() - upFrom.z() < 0.005f) return false;  // too tight to step up

    // Phase 3 - ACROSS: from the top position, trace in the move direction
    float acrossDist = forwardDist * (1.0f - trForward.fraction) + bodyRadius * 0.5f;
    osg::Vec3 acrossFrom = topPos, acrossTo = topPos + moveDir * acrossDist;
    TraceResult trAcross = traceBody(acrossFrom, acrossTo, radiusScale, 1.0f);
    if (trAcross.startedSolid) return false;
    osg::Vec3 acrossPos = trAcross.hit ? trAcross.endPosition : acrossTo;

    // Phase 4 - DOWN: from the across position, trace straight down to find a standable surface
    osg::Vec3 downFrom = acrossPos, downTo(acrossPos.x(), acrossPos.y(), acrossPos.z() - maxStepHeight);
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
