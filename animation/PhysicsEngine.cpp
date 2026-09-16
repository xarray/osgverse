#include <osg/io_utils>
#include <osg/Version>
#include <osg/Notify>
#include <osg/Geometry>
#include <osg/Geode>
#include <osgDB/ReadFile>
#include <osgUtil/SmoothingVisitor>
#include <memory>

#include "3rdparty/box3d/box3d.h"
#include "modeling/Utilities.h"
#include "PhysicsEngine.h"
using namespace osgVerse;

namespace b3Helpers
{
    struct ShapeData : public osg::Referenced
    {
        float volume = 1.0f; virtual ~ShapeData() {}
        virtual b3ShapeId createOnBody(b3WorldId world, b3BodyId body, const b3ShapeDef& def) = 0;

        // Get a convex point-cloud proxy for shape casting (sweep). Return false if unsupported
        virtual bool getProxy(std::vector<b3Vec3>& points, float& radius) const { return false; }
    };

    struct BoxData : public ShapeData
    {
        b3BoxHull hull; osg::Vec3 halfSize, offset;
        BoxData(const osg::Vec3& hs, const osg::Vec3& off = osg::Vec3()) : halfSize(hs), offset(off)
        {
            b3Transform xf; xf.p = b3Vec3{off.x(), off.y(), off.z()};
            xf.q = b3Quat{0.0f, 0.0f, 0.0f, 1.0f};
            hull = b3MakeTransformedBoxHull(hs.x(), hs.y(), hs.z(), xf);
            volume = 8.0f * hs.x() * hs.y() * hs.z();
        }
        virtual b3ShapeId createOnBody(b3WorldId, b3BodyId body, const b3ShapeDef& def)
        { return b3CreateHullShape(body, &def, &hull.base); }

        virtual bool getProxy(std::vector<b3Vec3>& points, float& radius) const
        {
            points.resize(8);
            for (int i = 0; i < 8; ++i)
            {
                float sx = (i & 1) ? halfSize.x() : -halfSize.x();
                float sy = (i & 2) ? halfSize.y() : -halfSize.y();
                float sz = (i & 4) ? halfSize.z() : -halfSize.z();
                points[i] = b3Vec3{ sx + offset.x(), sy + offset.y(), sz + offset.z() };
            }
            radius = 0.0f; return true;
        }
    };

    struct CylinderData : public ShapeData
    {
        b3HullData* hull; virtual ~CylinderData() { if (hull) b3DestroyHull(hull); }
        CylinderData(const osg::Vec3& hry, int n)
        { hull = b3CreateCylinder(hry.x(), hry.y(), hry.z(), n); volume = osg::PI * hry.y() * hry.y() * hry.x(); }
        virtual b3ShapeId createOnBody(b3WorldId, b3BodyId body, const b3ShapeDef& def)
        { return hull ? b3CreateHullShape(body, &def, hull) : b3_nullShapeId; }
    };

    struct ConeData : public ShapeData
    {
        b3HullData* hull; virtual ~ConeData() { if (hull) b3DestroyHull(hull); }
        ConeData(const osg::Vec3& hrr, int n)
        { hull = b3CreateCone(hrr.x(), hrr.y(), hrr.z(), n); volume = osg::PI * hrr.y() * hrr.y() * hrr.x() / 3.0f; }
        virtual b3ShapeId createOnBody(b3WorldId, b3BodyId body, const b3ShapeDef& def)
        { return hull ? b3CreateHullShape(body, &def, hull) : b3_nullShapeId; }
    };

    struct CapsuleData : public ShapeData
    {
        b3Capsule sh; CapsuleData(const osg::Vec3& c0, const osg::Vec3& c1, float r)
        {
            sh = b3Capsule { {c0.x(), c0.y(), c0.z()}, {c1.x(), c1.y(), c1.z()}, r };
            float h = (c1 - c0).length(); float cyVol = osg::PI * r * r * h;
            volume = (4.0f / 3.0f) * osg::PI * r * r * r + cyVol;
        }
        virtual b3ShapeId createOnBody(b3WorldId, b3BodyId body, const b3ShapeDef& def)
        { return b3CreateCapsuleShape(body, &def, &sh); }

        virtual bool getProxy(std::vector<b3Vec3>& points, float& radius) const
        {
            points.resize(2); points[0] = sh.center1; points[1] = sh.center2;
            radius = sh.radius; return true;
        }
    };

    struct SphereData : public ShapeData
    {
        b3Sphere sh; SphereData(const osg::Vec3& c, float r)
        { sh = b3Sphere { {c.x(), c.y(), c.z()}, r }; float h = r * 0.57735f; volume = 8.0f * h * h * h;  }
        virtual b3ShapeId createOnBody(b3WorldId, b3BodyId body, const b3ShapeDef& def)
        { return b3CreateSphereShape(body, &def, &sh); }

        virtual bool getProxy(std::vector<b3Vec3>& points, float& radius) const
        {
            points.resize(1); points[0] = sh.center; radius = sh.radius; return true;
        }
    };

    struct HullData : public ShapeData
    {
        b3HullData* hull; virtual ~HullData() { if (hull) b3DestroyHull(hull); }
        HullData(const b3Vec3* pt, int count) { hull = b3CreateHull(pt, count, count); }
        virtual b3ShapeId createOnBody(b3WorldId, b3BodyId body, const b3ShapeDef& def)
        { return hull ? b3CreateHullShape(body, &def, hull) : b3_nullShapeId; }
    };

    struct TriangleData : public ShapeData
    {
        b3MeshData* mesh; std::vector<int> idx; virtual ~TriangleData() { if (mesh) b3DestroyMesh(mesh); }
        TriangleData(const b3MeshDef& def) { idx.resize(64); mesh = b3CreateMesh(&def, idx.data(), idx.size()); }
        virtual b3ShapeId createOnBody(b3WorldId, b3BodyId body, const b3ShapeDef& def)
        { return mesh ? b3CreateMeshShape(body, &def, mesh, {1.0f, 1.0f, 1.0f}) : b3_nullShapeId; }
    };

    struct HeightData : public ShapeData
    {
        b3HeightFieldData* hf; virtual ~HeightData() { if (hf) b3DestroyHeightField(hf); }
        HeightData(const b3HeightFieldDef& def) { hf = b3CreateHeightField(&def); }
        virtual b3ShapeId createOnBody(b3WorldId, b3BodyId body, const b3ShapeDef& def)
        { return hf ? b3CreateHeightFieldShape(body, &def, hf) : b3_nullShapeId; }
    };

    struct CollisionShape : public osgVerse::CollisionShapeBase
    {
        CollisionShape() : shapeId(b3_nullShapeId) { shapeDef = b3DefaultShapeDef(); }
        b3ShapeDef shapeDef; osg::ref_ptr<ShapeData> shapeData;
        b3ShapeId shapeId;  // valid after the shape is created on a body
    };

    static void applyShapeSetting(b3ShapeDef& def, const osgVerse::PhysicsEngine::ShapeSetting* setting)
    {
        if (!setting) return;
        def.baseMaterial.friction = setting->friction;
        def.baseMaterial.restitution = setting->restitution;
        def.baseMaterial.rollingResistance = setting->rollingResistance;
        // Box3D only skips collisions between shapes which share the same negative group index
        def.filter.groupIndex = (setting->collisionGroup == 0) ?
            0 : -(setting->collisionGroup > 0 ? setting->collisionGroup : -setting->collisionGroup);
    }

    static b3Quat toB3Quat(const osg::Quat& q)
    { return b3Quat{ (float)q.x(), (float)q.y(), (float)q.z(), (float)q.w() }; }

    struct RigidBody : public osgVerse::RigidBodyBase { RigidBody(b3BodyId b) : _b(b) { internal = &_b; } b3BodyId _b; };
    struct Constraint : public osgVerse::ConstraintBase { Constraint(b3JointId j) : _j(j) { internal = &_j; } b3JointId _j; };

    class PhysicsCore : public PhysicsCoreBase
    {
    public:
        PhysicsCore()
        {
            b3WorldDef worldDef = b3DefaultWorldDef();
            worldDef.gravity = b3Vec3{ 0.0f, 0.0f, -9.8f };
            _worldId = b3CreateWorld(&worldDef);
        }

        static void fromMatrix(const osg::Matrix& m, b3Vec3& pos, b3Quat& rot)
        {
            osg::Vec4 q = m.getRotate().asVec4(); osg::Vec3 p = m.getTrans();
            pos = b3Vec3{ p.x(), p.y(), p.z() };
            rot = b3Quat{ (float)q.x(), q.y(), q.z(), q.w() };
        }

        static osg::Matrix toMatrix(const b3Vec3& pos, const b3Quat& rot)
        {
            osg::Quat q(rot.v.x, rot.v.y, rot.v.z, rot.s);
            osg::Vec3 p(pos.x, pos.y, pos.z);
            return osg::Matrix(osg::Matrix::rotate(q) * osg::Matrix::translate(p));
        }

        static b3Transform toTransform(const osg::Matrix& m)
        {
            b3Vec3 pos; b3Quat rot;
            PhysicsCore::fromMatrix(m, pos, rot);
            b3Transform xf; xf.p = pos; xf.q = rot; return xf;
        }

        /** Convert a world space frame to the local space of a body with the given transform */
        static void toLocal(const b3Vec3& bodyPos, const b3Quat& bodyRot, osg::Matrix& frame)
        { frame = frame * osg::Matrix::inverse(toMatrix(bodyPos, bodyRot)); }

        b3WorldId _worldId;

    protected:
        virtual ~PhysicsCore() { b3DestroyWorld(_worldId); }
    };

    struct RaycastCallbackData
    {
        osgVerse::PhysicsEngine* engine;
        std::vector<osgVerse::PhysicsEngine::RaycastHit>* hits;
        bool getNameFromBody;
    };

    static void findBodyName(osgVerse::PhysicsEngine* engine, b3BodyId bodyId, std::string& name)
    {
        const std::map<std::string, osg::ref_ptr<RigidBodyBase>>& bodies = engine->getBodies();
        for (std::map<std::string, osg::ref_ptr<RigidBodyBase>>::const_iterator
             itr = bodies.begin(); itr != bodies.end(); ++itr)
        {
            b3BodyId* body = itr->second->get<b3BodyId>();
            if (B3_ID_EQUALS((*body), bodyId)) { name = itr->first; break; }
        }
    }

    static void setBodyFromShape(osgVerse::PhysicsEngine::RaycastHit& result,
                                 osgVerse::PhysicsEngine* engine, bool getNameFromBody,
                                 b3ShapeId shapeId, b3Pos point, b3Vec3 normal)
    {
        result.position = osg::Vec3(point.x, point.y, point.z);
        result.normal = osg::Vec3(normal.x, normal.y, normal.z);
        result.rigidBody = NULL; // Will be filled later if needed

        // Find body from shape
        b3BodyId bodyId = b3Shape_GetBody(shapeId);
        if (B3_IS_NON_NULL(bodyId))
        {
            result.rigidBody = new b3Helpers::RigidBody(bodyId);
            if (getNameFromBody) b3Helpers::findBodyName(engine, bodyId, result.name);
        }
    }

    static float raycastCallback(b3ShapeId shapeId, b3Pos point, b3Vec3 normal, float fraction,
                                 uint64_t materialId, int triangleIndex, int childIndex, void* context)
    {
        RaycastCallbackData* data = (RaycastCallbackData*)context;
        osgVerse::PhysicsEngine::RaycastHit result;
        setBodyFromShape(result, data->engine, data->getNameFromBody, shapeId, point, normal);
        data->hits->push_back(result); return 1.0f; // Continue ray
    }

    struct ShapeCastContext
    {
        osgVerse::PhysicsEngine* engine;
        b3BodyId ignoredBody; bool hasIgnoredBody;
        bool startedSolid, hit;
        float closestFraction; b3Vec3 closestNormal; b3Pos closestPoint; b3ShapeId closestShape;

        ShapeCastContext(osgVerse::PhysicsEngine* e, b3BodyId ignored)
        :   engine(e), ignoredBody(ignored), hasIgnoredBody(B3_IS_NON_NULL(ignored)), startedSolid(false),
            hit(false), closestFraction(1.0f), closestShape(b3_nullShapeId)
        { closestNormal = b3Vec3_zero; closestPoint = b3Pos_zero; }
    };

    static float shapeCastCallback(b3ShapeId shapeId, b3Pos point, b3Vec3 normal, float fraction,
                                   uint64_t materialId, int triangleIndex, int childIndex, void* context)
    {
        ShapeCastContext* data = (ShapeCastContext*)context;
        if (data->hasIgnoredBody && B3_ID_EQUALS(b3Shape_GetBody(shapeId), data->ignoredBody))
            return -1.0f;  // skip the shapes of the ignored body (usually character itself)

        if (fraction == 0.0f)
        { data->startedSolid = true; return -1.0f; }  // already inside something

        if (fraction < data->closestFraction)
        {
            data->closestFraction = fraction; data->closestNormal = normal;
            data->closestPoint = point; data->closestShape = shapeId; data->hit = true;
        }
        return data->closestFraction;
    }
}
#define PHY_WORLD() (((b3Helpers::PhysicsCore*)_core.get())->_worldId)

PhysicsEngine::PhysicsEngine()
{ _core = new b3Helpers::PhysicsCore; setName("Box3D"); }

PhysicsEngine::PhysicsEngine(const PhysicsEngine& copy, const osg::CopyOp& op)
:   osg::Object(copy, op), _constraints(copy._constraints), _shapes(copy._shapes),
    _bodies(copy._bodies), _core(copy._core) {}

PhysicsEngine::~PhysicsEngine()
{
    for (std::map<std::string, ConstraintAndState>::iterator itr = _constraints.begin();
         itr != _constraints.end(); ++itr)
    {
        b3JointId* joint = itr->second.first->get<b3JointId>();
        b3DestroyJoint(*joint, false);
    }
    for (std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.begin();
         itr != _bodies.end(); ++itr)
    {
        b3BodyId* body = itr->second->get<b3BodyId>();
        b3DestroyBody(*body);
    }
    _constraints.clear(); _shapes.clear(); _bodies.clear(); _core = NULL;
}

RigidBodyBase* PhysicsEngine::addRigidBody(const std::string& name, CollisionShapeBase* csb, float mass,
                                           const osg::Matrix& matrix, bool kinematic,
                                           const ShapeSetting* setting)
{
    bool isDynamic = (mass > 0.0f); b3Vec3 pos; b3Quat rot;
    b3Helpers::PhysicsCore::fromMatrix(matrix, pos, rot);
    if (_shapes.find(name) != _shapes.end()) removeBody(name);  // remove existing shape

    b3Helpers::CollisionShape* cs = static_cast<b3Helpers::CollisionShape*>(csb);
    if (!cs || (cs && !cs->shapeData)) { OSG_NOTICE << "[PhysicsEngine] Failed to get input shape\n"; return NULL; }
    cs->shapeDef.density = (mass > 0.0f) ? (mass / cs->shapeData->volume) : 0.0f;
    b3Helpers::applyShapeSetting(cs->shapeDef, setting);

    b3BodyDef bodyDef = b3DefaultBodyDef();
    bodyDef.position = b3ToPos(pos); bodyDef.rotation = rot;
    bodyDef.type = kinematic ? b3_kinematicBody : (mass > 0.0f ? b3_dynamicBody : b3_staticBody);

    b3BodyId bodyId = b3CreateBody(PHY_WORLD(), &bodyDef);
    if (B3_IS_NULL(bodyId)) { OSG_NOTICE << "[PhysicsEngine] Failed to create body\n"; return NULL; }
    b3ShapeId shapeId = cs->shapeData->createOnBody(PHY_WORLD(), bodyId, cs->shapeDef);
    if (B3_IS_NULL(shapeId)) { b3DestroyBody(bodyId); OSG_NOTICE << "[PhysicsEngine] Failed to create shape\n"; return NULL; }

    b3Helpers::RigidBody* container = new b3Helpers::RigidBody(bodyId);
    _shapes[name] = csb; _bodies[name] = container;
    cs->shapeData = NULL; return container;
}

void PhysicsEngine::removeBody(const std::string& name)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        b3BodyId* body = itr->second->get<b3BodyId>(); b3DestroyBody(*body);
        _bodies.erase(itr);  // the body handle will be deleted by itself if no other reference exists
    }

    std::map<std::string, osg::ref_ptr<CollisionShapeBase>>::iterator itr2 = _shapes.find(name);
    if (itr2 != _shapes.end()) { _shapes.erase(itr2); }
}

bool PhysicsEngine::isDynamicBody(const std::string& name, bool& isKinematic)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        b3BodyId* body = itr->second->get<b3BodyId>();
        b3BodyType type = b3Body_GetType(*body);
        isKinematic = (type == b3_kinematicBody);
        return (type != b3_staticBody);
    }
    return false;
}

void PhysicsEngine::setTransform(const std::string& name, const osg::Matrix& matrix)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        b3Vec3 pos; b3Quat rot;
        b3Helpers::PhysicsCore::fromMatrix(matrix, pos, rot);
        b3BodyId* body = itr->second->get<b3BodyId>();
        b3Body_SetTransform(*body, b3ToPos(pos), rot);
    }
}

osg::Matrix PhysicsEngine::getTransform(const std::string& name, bool& valid)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        b3BodyId* body = itr->second->get<b3BodyId>(); valid = true;
        b3Vec3 pos = b3ToVec3(b3Body_GetPosition(*body));
        b3Quat rot = b3Body_GetRotation(*body);
        return b3Helpers::PhysicsCore::toMatrix(pos, rot);
    }
    valid = false; return osg::Matrix();
}

void PhysicsEngine::setVelocity(const std::string& name, const osg::Vec3& v, bool linearOrAngular)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        b3BodyId* body = itr->second->get<b3BodyId>();
        b3Vec3 vel = b3Vec3{ v[0], v[1], v[2] };
        if (linearOrAngular) b3Body_SetLinearVelocity(*body, vel);
        else b3Body_SetAngularVelocity(*body, vel);
    }
}

osg::Vec3 PhysicsEngine::getVelocity(const std::string& name, bool linearOrAngular)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        b3BodyId* body = itr->second->get<b3BodyId>(); b3Vec3 vel;
        if (linearOrAngular) vel = b3Body_GetLinearVelocity(*body);
        else vel = b3Body_GetAngularVelocity(*body);
        return osg::Vec3(vel.x, vel.y, vel.z);
    }
    return osg::Vec3();
}

void PhysicsEngine::applyImpulse(const std::string& name, const osg::Vec3& point,
                                 const osg::Vec3& impulse, bool wake, bool linearOrAngular)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        b3BodyId* body = itr->second->get<b3BodyId>();
        b3Vec3 v = b3Vec3{ impulse[0], impulse[1], impulse[2] };
        if (linearOrAngular)
        {
            b3Pos p = b3Pos({ point[0], point[1], point[2] });
            b3Body_ApplyLinearImpulse(*body, v, p, wake);
        }
        else
            b3Body_ApplyAngularImpulse(*body, v, wake);
    }
}

void PhysicsEngine::setBullet(const std::string& name, bool flag)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end()) b3Body_SetBullet(*itr->second->get<b3BodyId>(), flag);
}

float PhysicsEngine::getInverseMass(const std::string& name)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        b3BodyId* body = itr->second->get<b3BodyId>();
        return b3Body_GetInverseMass(*body);
    }
    return 0.0f;
}

osg::Matrix PhysicsEngine::getInverseInertia(const std::string& name)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        b3BodyId* body = itr->second->get<b3BodyId>();
        b3Matrix3 m = b3Body_GetWorldInverseRotationalInertia(*body);
        return osg::Matrix(m.cx.x, m.cy.x, m.cz.x, 0.0f, m.cx.y, m.cy.y, m.cz.y, 0.0f,
                           m.cx.z, m.cy.z, m.cz.z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    }
    return osg::Matrix();
}

osg::Vec3 PhysicsEngine::getCenterOfMass(const std::string& name)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        b3BodyId* body = itr->second->get<b3BodyId>();
        b3Pos c = b3Body_GetWorldCenter(*body); return osg::Vec3(c.x, c.y, c.z);
    }
    return osg::Vec3();
}

RigidBodyBase* PhysicsEngine::createBody(const std::string& name, const BodySetting& setting,
                                         const osg::Matrix& matrix)
{
    b3Vec3 pos; b3Quat rot;
    b3Helpers::PhysicsCore::fromMatrix(matrix, pos, rot);
    if (_bodies.find(name) != _bodies.end()) removeBody(name);  // remove existing body

    b3BodyDef bodyDef = b3DefaultBodyDef();
    bodyDef.position = b3ToPos(pos); bodyDef.rotation = rot;
    bodyDef.type = setting.kinematic ? b3_kinematicBody : b3_dynamicBody;
    bodyDef.gravityScale = setting.gravityScale;
    bodyDef.linearDamping = setting.linearDamping; bodyDef.angularDamping = setting.angularDamping;
    bodyDef.enableSleep = setting.allowSleep;
    bodyDef.enableContactRecycling = setting.enableContactRecycling;
    bodyDef.motionLocks.angularX = setting.lockAngularX;
    bodyDef.motionLocks.angularY = setting.lockAngularY;
    bodyDef.motionLocks.angularZ = setting.lockAngularZ;
    bodyDef.name = name.c_str();

    b3BodyId bodyId = b3CreateBody(PHY_WORLD(), &bodyDef);
    if (B3_IS_NULL(bodyId)) { OSG_NOTICE << "[PhysicsEngine] Failed to create body\n"; return NULL; }

    b3Helpers::RigidBody* container = new b3Helpers::RigidBody(bodyId);
    _bodies[name] = container; return container;
}

CollisionShapeBase* PhysicsEngine::addShapeToBody(RigidBodyBase* body, CollisionShapeBase* csb,
                                                  float mass, const ShapeSetting* setting)
{
    if (!body || !csb) return NULL;
    b3Helpers::CollisionShape* cs = static_cast<b3Helpers::CollisionShape*>(csb);
    b3BodyId* bodyId = body->get<b3BodyId>();
    if (!cs->shapeData || B3_IS_NULL((*bodyId)))
    { OSG_NOTICE << "[PhysicsEngine] Failed to add shape to body\n"; return NULL; }

    cs->shapeDef.density = (mass > 0.0f) ? (mass / cs->shapeData->volume) : 0.0f;
    b3Helpers::applyShapeSetting(cs->shapeDef, setting);
    cs->shapeId = cs->shapeData->createOnBody(PHY_WORLD(), *bodyId, cs->shapeDef);
    if (B3_IS_NULL(cs->shapeId))
    { OSG_NOTICE << "[PhysicsEngine] Failed to create shape on body\n"; return NULL; }
    return cs;
}

void PhysicsEngine::setShapeFriction(CollisionShapeBase* shape, float friction)
{
    b3Helpers::CollisionShape* cs = static_cast<b3Helpers::CollisionShape*>(shape);
    if (cs && B3_IS_NON_NULL(cs->shapeId)) b3Shape_SetFriction(cs->shapeId, friction);
}

void PhysicsEngine::setGravityScale(const std::string& name, float scale)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
        b3Body_SetGravityScale(*itr->second->get<b3BodyId>(), scale);
}

void PhysicsEngine::setLinearDamping(const std::string& name, float damping)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
        b3Body_SetLinearDamping(*itr->second->get<b3BodyId>(), damping);
}

void PhysicsEngine::setMassCenter(const std::string& name, const osg::Vec3& center)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        b3BodyId* body = itr->second->get<b3BodyId>();
        b3MassData massData = b3Body_GetMassData(*body);
        massData.center = b3Vec3{ center[0], center[1], center[2] };
        b3Body_SetMassData(*body, massData);
    }
}

void PhysicsEngine::addConstraint(const std::string& name, ConstraintBase* cBase,
                                  bool noCollisionsBetweenLinked)
{
    // Box3D joints are created with collideConnected flag in the definition
    // We store the original state for restoration on removal
    b3JointId* joint = cBase->get<b3JointId>();
    b3BodyId bodyA = b3Joint_GetBodyA(*joint);
    b3BodyId bodyB = b3Joint_GetBodyB(*joint);
    
    int constraintedState = b3Body_IsAwake(bodyB) ? 1 : 0;
    if (b3Body_GetType(bodyA) == b3_kinematicBody || b3Body_GetType(bodyA) == b3_staticBody)
        b3Body_SetAwake(bodyB, true);
    else
        b3Body_SetAwake(bodyA, true);
    _constraints[name] = ConstraintAndState(cBase, constraintedState);
}

void PhysicsEngine::removeConstraint(const std::string& name)
{
    std::map<std::string, ConstraintAndState>::iterator itr = _constraints.find(name);
    if (itr != _constraints.end())
    {
        b3JointId* joint = itr->second.first->get<b3JointId>();
        b3BodyId bodyA = b3Joint_GetBodyA(*joint);
        b3BodyId bodyB = b3Joint_GetBodyB(*joint);

        if (b3Body_GetType(bodyA) == b3_kinematicBody || b3Body_GetType(bodyA) == b3_staticBody)
            { if (itr->second.second == 0) b3Body_SetAwake(bodyB, false); }
        else
            { if (itr->second.second == 0) b3Body_SetAwake(bodyA, false); }
        b3DestroyJoint(*joint, true); _constraints.erase(itr);
    }
}

CollisionShapeBase* PhysicsEngine::getShape(const std::string& name)
{
    if (_shapes.find(name) == _shapes.end()) return NULL;
    return _shapes[name].get();
}

RigidBodyBase* PhysicsEngine::getRigidBody(const std::string& name)
{
    if (_bodies.find(name) == _bodies.end()) return NULL;
    return _bodies[name].get();
}

ConstraintBase* PhysicsEngine::getConstraint(const std::string& name)
{
    if (_constraints.find(name) == _constraints.end()) return NULL;
    return _constraints[name].first.get();
}

void PhysicsEngine::setGravity(const osg::Vec3& gravity)
{ b3World_SetGravity(PHY_WORLD(), b3Vec3{ gravity[0], gravity[1], gravity[2] }); }

osg::Vec3 PhysicsEngine::getGravity() const
{ b3Vec3 g = b3World_GetGravity(PHY_WORLD()); return osg::Vec3(g.x, g.y, g.z); }

bool PhysicsEngine::raycast(const osg::Vec3& s, const osg::Vec3& e,
                            RaycastHit& result, const QueryFilter& f, bool getNameFromBody)
{
    b3Vec3 origin = b3Vec3{ s.x(), s.y(), s.z() };
    b3Vec3 translation = b3Vec3{ e.x() - s.x(), e.y() - s.y(), e.z() - s.z() };
    b3QueryFilter filter = b3DefaultQueryFilter();
    filter.categoryBits = f.categoryBits; filter.maskBits = f.maskBits;
    if (!f.name.empty()) filter.name = f.name.c_str();

    b3RayResult r = b3World_CastRayClosest(PHY_WORLD(), b3ToPos(origin), translation, filter);
    if (r.hit)
        b3Helpers::setBodyFromShape(result, this, getNameFromBody, r.shapeId, r.point, r.normal);
    return r.hit;
}

std::vector<PhysicsEngine::RaycastHit> PhysicsEngine::raycastAll(const osg::Vec3& s, const osg::Vec3& e,
                                                                 const QueryFilter& f, bool getNameFromBody)
{
    b3Vec3 origin = b3Vec3{ s.x(), s.y(), s.z() };
    b3Vec3 translation = b3Vec3{ e.x() - s.x(), e.y() - s.y(), e.z() - s.z() };
    b3QueryFilter filter = b3DefaultQueryFilter();
    filter.categoryBits = f.categoryBits; filter.maskBits = f.maskBits;
    if (!f.name.empty()) filter.name = f.name.c_str();

    std::vector<RaycastHit> hitList;
    b3Helpers::RaycastCallbackData data = { this, &hitList, getNameFromBody };
    b3World_CastRay(PHY_WORLD(), b3ToPos(origin), translation, filter, b3Helpers::raycastCallback, &data);
    return hitList;
}

PhysicsEngine::SweepResult PhysicsEngine::sweep(const osg::Vec3& s, const osg::Vec3& t, CollisionShapeBase* csb,
                                                const QueryFilter& f, RigidBodyBase* ignoredBody)
{
    SweepResult result;
    b3Helpers::CollisionShape* cs = static_cast<b3Helpers::CollisionShape*>(csb);
    std::vector<b3Vec3> points; float radius = 0.0f;
    if (!cs || !cs->shapeData || !cs->shapeData->getProxy(points, radius))
    { OSG_NOTICE << "[PhysicsEngine] Unsupported shape for sweeping\n"; return result; }

    b3ShapeProxy proxy; proxy.points = points.data();
    proxy.count = (int)points.size(); proxy.radius = radius;
    b3Vec3 translation = b3Vec3{ t.x(), t.y(), t.z() };

    b3QueryFilter filter = b3DefaultQueryFilter();
    filter.categoryBits = f.categoryBits; filter.maskBits = f.maskBits;
    if (!f.name.empty()) filter.name = f.name.c_str();

    b3BodyId ignoredId = b3_nullBodyId;
    if (ignoredBody) ignoredId = *ignoredBody->get<b3BodyId>();
    b3Helpers::ShapeCastContext context(this, ignoredId);
    b3World_CastShape(PHY_WORLD(), b3ToPos(b3Vec3{ s.x(), s.y(), s.z() }), &proxy, translation,
                      filter, b3Helpers::shapeCastCallback, &context);

    result.startedSolid = context.startedSolid; result.hit = context.hit;
    if (context.hit)
    {
        b3BodyId bodyId = b3Shape_GetBody(context.closestShape);
        result.fraction = context.closestFraction;
        result.point = osg::Vec3(context.closestPoint.x, context.closestPoint.y, context.closestPoint.z);
        result.normal = osg::Vec3(context.closestNormal.x, context.closestNormal.y, context.closestNormal.z);
        if (B3_IS_NON_NULL(bodyId))
        {
            result.rigidBody = new b3Helpers::RigidBody(bodyId);
            b3Helpers::findBodyName(this, bodyId, result.name);
        }
    }
    return result;
}

void PhysicsEngine::advance(float timeStep, int maxSubSteps)
{ b3World_Step(PHY_WORLD(), timeStep, maxSubSteps); }

CollisionShapeBase* PhysicsEngine::createPhysicsPoint()
{ 
    b3Helpers::CollisionShape* cs = new b3Helpers::CollisionShape;
    cs->shapeData = new b3Helpers::BoxData(osg::Vec3(0.001f, 0.001f, 0.001f)); return cs;
}

CollisionShapeBase* PhysicsEngine::createPhysicsBox(const osg::Vec3& halfSize)
{
    b3Helpers::CollisionShape* cs = new b3Helpers::CollisionShape;
    cs->shapeData = new b3Helpers::BoxData(halfSize); return cs;
}

CollisionShapeBase* PhysicsEngine::createPhysicsBox(const osg::Vec3& halfSize, const osg::Vec3& offset)
{
    b3Helpers::CollisionShape* cs = new b3Helpers::CollisionShape;
    cs->shapeData = new b3Helpers::BoxData(halfSize, offset); return cs;
}

CollisionShapeBase* PhysicsEngine::createPhysicsCylinder(const osg::Vec3& halfSize)
{
    float h = 2.0f * halfSize.z(), r = osg::minimum(halfSize.x(), halfSize.y());
    b3Helpers::CollisionShape* cs = new b3Helpers::CollisionShape;
    cs->shapeData = new b3Helpers::CylinderData(osg::Vec3(h, r, 0.0f), 16); return cs;
}

CollisionShapeBase* PhysicsEngine::createPhysicsCone(float radius, float height)
{
    b3Helpers::CollisionShape* cs = new b3Helpers::CollisionShape;
    cs->shapeData = new b3Helpers::ConeData(osg::Vec3(height, radius, 0.0f), 16); return cs;
}

CollisionShapeBase* PhysicsEngine::createPhysicsCapsule(float radius, float height)
{
    osg::Vec3 c0(0.0f, 0.0f, -0.5f * height), c1(0.0f, 0.0f, 0.5f * height);
    b3Helpers::CollisionShape* cs = new b3Helpers::CollisionShape;
    cs->shapeData = new b3Helpers::CapsuleData(c0, c1, radius); return cs;
}

CollisionShapeBase* PhysicsEngine::createPhysicsCapsule(float radius, const osg::Vec3& c0, const osg::Vec3& c1)
{
    b3Helpers::CollisionShape* cs = new b3Helpers::CollisionShape;
    cs->shapeData = new b3Helpers::CapsuleData(c0, c1, radius); return cs;
}

CollisionShapeBase* PhysicsEngine::createPhysicsSphere(float radius)
{
    b3Helpers::CollisionShape* cs = new b3Helpers::CollisionShape;
    cs->shapeData = new b3Helpers::SphereData(osg::Vec3(), radius); return cs;
}

CollisionShapeBase* PhysicsEngine::createPhysicsHull(osg::Node* node, bool optimized)
{
    osgVerse::MeshCollector bvv; if (node != NULL) node->accept(bvv);
    const std::vector<osg::Vec3>& vertices = bvv.getVertices();
    const osg::BoundingBoxd& bb = bvv.getBoundingBox(); osg::Vec3 l = bb._max - bb._min;
    if (vertices.empty()) return NULL;

    std::vector<b3Vec3> points(vertices.size());
    memcpy(points.data(), vertices.data(), sizeof(b3Vec3) * vertices.size());
    
    osg::ref_ptr<b3Helpers::HullData> hullShape = new b3Helpers::HullData(points.data(), points.size());
    if (!hullShape->hull)
    {   // hull creation failed? Try VHACD instead
        osgVerse::BoundingVolumeVisitor vhacd; node->accept(vhacd);
        osg::ref_ptr<osg::Geode> geode = new osg::Geode; geode->addDrawable(vhacd.computeVHACD());

        osgVerse::MeshCollector bvv2; geode->accept(bvv2);
        const std::vector<osg::Vec3>& vertices2 = bvv2.getVertices(); points.resize(vertices2.size());
        memcpy(points.data(), vertices2.data(), sizeof(b3Vec3) * vertices2.size());
        hullShape = new b3Helpers::HullData(points.data(), points.size());
    }
    b3Helpers::CollisionShape* cs = new b3Helpers::CollisionShape; cs->shapeData = hullShape;
    cs->shapeData->volume = (l[0] * l[1] * l[2]); return cs;
}

CollisionShapeBase* PhysicsEngine::createPhysicsTriangleMesh(osg::Node* node, bool compressed)
{
    osgVerse::MeshCollector bvv; if (node != NULL) node->accept(bvv);
    const std::vector<osg::Vec3>& vertices = bvv.getVertices();
    const std::vector<unsigned int>& triangles = bvv.getTriangles();
    const osg::BoundingBoxd& bb = bvv.getBoundingBox(); osg::Vec3 l = bb._max - bb._min;
    if (vertices.empty() || triangles.empty()) return NULL;

    std::vector<b3Vec3> points(vertices.size());
    memcpy(points.data(), vertices.data(), sizeof(b3Vec3) * vertices.size());
    std::vector<int32_t> indices(triangles.size());
    memcpy(indices.data(), triangles.data(), sizeof(int32_t) * triangles.size());

    b3MeshDef def = {};
    def.vertices = points.data(); def.vertexCount = (int)points.size();
    def.indices = indices.data(); def.triangleCount = (int)indices.size() / 3;
    def.materialIndices = NULL; def.useMedianSplit = false;
    def.identifyEdges = true; def.weldVertices = true;
    def.weldTolerance = 0.001f * 1.5f;  // weldToleranceMillimeters

    b3Helpers::CollisionShape* cs = new b3Helpers::CollisionShape;
    cs->shapeData = new b3Helpers::TriangleData(def);
    cs->shapeData->volume = (l[0] * l[1] * l[2]); return cs;
}

CollisionShapeBase* PhysicsEngine::createPhysicsHeightField(osg::HeightField* hf, bool filpQuad)
{
    const osg::HeightField::HeightList& heights = hf->getHeightList();
    float minHeight = FLT_MAX, maxHeight = -FLT_MAX;
    for (size_t i = 0; i < heights.size(); ++i)
    {
        float h = heights[i];
        if (h < minHeight) minHeight = h;
        if (h > maxHeight) maxHeight = h;
    }  // TODO: check if correct

    b3HeightFieldDef def; def.heights = (float*)heights.data();
    def.countX = hf->getNumColumns(); def.countZ = hf->getNumRows();
    def.scale = b3Vec3{1.0f, 1.0f, 1.0f};
    def.globalMinimumHeight = minHeight;
    def.globalMaximumHeight = maxHeight;

    b3Helpers::CollisionShape* cs = new b3Helpers::CollisionShape;
    cs->shapeData = new b3Helpers::HeightData(def);
    cs->shapeData->volume = (hf->getXInterval() * hf->getNumColumns()) * (hf->getYInterval() * hf->getNumRows())
                          * (maxHeight - minHeight); return cs;
}

ConstraintBase* PhysicsEngine::createConstraint(RigidBodyBase* bodyA, const osg::Matrix& frameA,
                                                RigidBodyBase* bodyB, const osg::Matrix& frameB,
                                                ConstraintType type, const ConstraintSetting* setting)
{
    if (!bodyA || !bodyB) return NULL;
    b3BodyId* bA = bodyA->get<b3BodyId>();
    b3BodyId* bB = bodyB->get<b3BodyId>();
    if (B3_IS_NULL((*bA)) || B3_IS_NULL((*bB))) return NULL;

    ConstraintSetting cs; if (setting) cs = *setting;

    // World frames should be converted to local ones of the two bodies first
    osg::Matrix fA = frameA, fB = frameB;
    if (cs.useWorldPivots)
    {
        b3Helpers::PhysicsCore::toLocal(b3ToVec3(b3Body_GetPosition(*bA)), b3Body_GetRotation(*bA), fA);
        b3Helpers::PhysicsCore::toLocal(b3ToVec3(b3Body_GetPosition(*bB)), b3Body_GetRotation(*bB), fB);
    }
    b3Transform tA = b3Helpers::PhysicsCore::toTransform(fA);
    b3Transform tB = b3Helpers::PhysicsCore::toTransform(fB);
    b3JointId joint = b3_nullJointId;

    // Box3D joints are created with all parameters in the definition, while tau/damping/impulseClamp
    // of P2P constraints are Bullet-only parameters and simply ignored here
    switch (type)
    {
    case CONSTRAINT_HINGE:
        {
            b3RevoluteJointDef def = b3DefaultRevoluteJointDef();
            def.base.bodyIdA = *bA; def.base.bodyIdB = *bB;
            def.base.localFrameA = tA; def.base.localFrameB = tB;
            def.base.collideConnected = cs.collideConnected;
            def.enableLimit = cs.enableLimit;
            def.lowerAngle = cs.lowerLimit; def.upperAngle = cs.upperLimit;
            def.enableSpring = cs.enableSpring;
            def.hertz = cs.hertz; def.dampingRatio = cs.dampingRatio;
            def.targetAngle = cs.targetAngle;
            def.enableMotor = cs.enableMotor; def.maxMotorTorque = cs.maxMotorTorque;
            def.motorSpeed = cs.motorSpeed;
            joint = b3CreateRevoluteJoint(PHY_WORLD(), &def);
        }
        break;
    case CONSTRAINT_CONE_TWIST:
        {
            b3SphericalJointDef def = b3DefaultSphericalJointDef();
            def.base.bodyIdA = *bA; def.base.bodyIdB = *bB;
            def.base.localFrameA = tA; def.base.localFrameB = tB;
            def.base.collideConnected = cs.collideConnected;
            def.enableConeLimit = cs.enableLimit; def.coneAngle = cs.coneLimit;
            def.enableTwistLimit = cs.enableLimit;
            def.lowerTwistAngle = cs.lowerLimit; def.upperTwistAngle = cs.upperLimit;
            def.enableSpring = cs.enableSpring;
            def.hertz = cs.hertz; def.dampingRatio = cs.dampingRatio;
            def.targetRotation = b3Helpers::toB3Quat(cs.targetRotation);
            def.enableMotor = cs.enableMotor; def.maxMotorTorque = cs.maxMotorTorque;
            def.motorVelocity = b3Vec3{ 0.0f, 0.0f, cs.motorSpeed };
            joint = b3CreateSphericalJoint(PHY_WORLD(), &def);
        }
        break;
    case CONSTRAINT_PARALLEL:
        {
            b3ParallelJointDef def = b3DefaultParallelJointDef();
            def.base.bodyIdA = *bA; def.base.bodyIdB = *bB;
            def.base.localFrameA = tA; def.base.localFrameB = tB;
            def.base.collideConnected = cs.collideConnected;
            def.hertz = cs.hertz; def.dampingRatio = cs.dampingRatio;
            if (cs.maxSpringForce > 0.0f) def.maxTorque = cs.maxSpringForce;
            joint = b3CreateParallelJoint(PHY_WORLD(), &def);
        }
        break;
    case CONSTRAINT_MOTOR:
        {
            b3MotorJointDef def = b3DefaultMotorJointDef();
            def.base.bodyIdA = *bA; def.base.bodyIdB = *bB;
            def.base.localFrameA = tA; def.base.localFrameB = tB;
            def.base.collideConnected = cs.collideConnected;
            def.linearHertz = cs.hertz; def.angularHertz = cs.hertz;
            def.linearDampingRatio = cs.dampingRatio; def.angularDampingRatio = cs.dampingRatio;
            if (cs.maxSpringForce > 0.0f)
            { def.maxSpringForce = cs.maxSpringForce; def.maxSpringTorque = cs.maxSpringForce; }
            joint = b3CreateMotorJoint(PHY_WORLD(), &def);
        }
        break;
    case CONSTRAINT_FILTER:
        {
            b3FilterJointDef def = b3DefaultFilterJointDef();
            def.base.bodyIdA = *bA; def.base.bodyIdB = *bB;
            joint = b3CreateFilterJoint(PHY_WORLD(), &def);
        }
        break;
    default:  // CONSTRAINT_P2P: spherical joint, with optional limits, spring and motor
        {
            b3SphericalJointDef def = b3DefaultSphericalJointDef();
            def.base.bodyIdA = *bA; def.base.bodyIdB = *bB;
            def.base.localFrameA = tA; def.base.localFrameB = tB;
            def.base.collideConnected = cs.collideConnected;
            def.enableSpring = cs.enableSpring;
            def.hertz = cs.hertz; def.dampingRatio = cs.dampingRatio;
            def.targetRotation = b3Helpers::toB3Quat(cs.targetRotation);
            def.enableMotor = cs.enableMotor; def.maxMotorTorque = cs.maxMotorTorque;
            def.motorVelocity = b3Vec3{ 0.0f, 0.0f, cs.motorSpeed };
            joint = b3CreateSphericalJoint(PHY_WORLD(), &def);
        }
        break;
    }

    if (B3_IS_NULL(joint))
    { OSG_NOTICE << "[PhysicsEngine] Failed to create constraint\n"; return NULL; }
    return new b3Helpers::Constraint(joint);
}

void PhysicsEngine::setConstraintSetting(const std::string& name, const ConstraintSetting& cs)
{
    std::map<std::string, ConstraintAndState>::iterator itr = _constraints.find(name);
    if (itr == _constraints.end()) return;
    b3JointId* joint = itr->second.first->get<b3JointId>();
    b3Joint_SetCollideConnected(*joint, cs.collideConnected);

    switch (b3Joint_GetType(*joint))
    {
    case b3_revoluteJoint:
        b3RevoluteJoint_EnableLimit(*joint, cs.enableLimit);
        if (cs.enableLimit) b3RevoluteJoint_SetLimits(*joint, cs.lowerLimit, cs.upperLimit);
        b3RevoluteJoint_EnableSpring(*joint, cs.enableSpring);
        if (cs.enableSpring)
        {
            b3RevoluteJoint_SetSpringHertz(*joint, cs.hertz);
            b3RevoluteJoint_SetSpringDampingRatio(*joint, cs.dampingRatio);
            b3RevoluteJoint_SetTargetAngle(*joint, cs.targetAngle);
        }
        b3RevoluteJoint_EnableMotor(*joint, cs.enableMotor);
        if (cs.enableMotor)
        {
            b3RevoluteJoint_SetMaxMotorTorque(*joint, cs.maxMotorTorque);
            b3RevoluteJoint_SetMotorSpeed(*joint, cs.motorSpeed);
        }
        break;
    case b3_sphericalJoint:
        b3SphericalJoint_EnableConeLimit(*joint, cs.enableLimit);
        b3SphericalJoint_EnableTwistLimit(*joint, cs.enableLimit);
        if (cs.enableLimit)
        {
            b3SphericalJoint_SetConeLimit(*joint, cs.coneLimit);
            b3SphericalJoint_SetTwistLimits(*joint, cs.lowerLimit, cs.upperLimit);
        }
        b3SphericalJoint_EnableSpring(*joint, cs.enableSpring);
        if (cs.enableSpring)
        {
            b3SphericalJoint_SetSpringHertz(*joint, cs.hertz);
            b3SphericalJoint_SetSpringDampingRatio(*joint, cs.dampingRatio);
            b3SphericalJoint_SetTargetRotation(*joint, b3Helpers::toB3Quat(cs.targetRotation));
        }
        b3SphericalJoint_EnableMotor(*joint, cs.enableMotor);
        if (cs.enableMotor)
        {
            b3SphericalJoint_SetMaxMotorTorque(*joint, cs.maxMotorTorque);
            b3SphericalJoint_SetMotorVelocity(*joint, b3Vec3{ 0.0f, 0.0f, cs.motorSpeed });
        }
        break;
    case b3_parallelJoint:
        b3ParallelJoint_SetSpringHertz(*joint, cs.hertz);
        b3ParallelJoint_SetSpringDampingRatio(*joint, cs.dampingRatio);  // no maxTorque setter here
        break;
    case b3_motorJoint:
        b3MotorJoint_SetLinearHertz(*joint, cs.hertz);
        b3MotorJoint_SetAngularHertz(*joint, cs.hertz);
        b3MotorJoint_SetLinearDampingRatio(*joint, cs.dampingRatio);
        b3MotorJoint_SetAngularDampingRatio(*joint, cs.dampingRatio);
        if (cs.maxSpringForce > 0.0f)
        {
            b3MotorJoint_SetMaxSpringForce(*joint, cs.maxSpringForce);
            b3MotorJoint_SetMaxSpringTorque(*joint, cs.maxSpringForce);
        }
        break;
    default: break;  // filter joints have no parameter to update
    }
    b3Joint_WakeBodies(*joint);
}

float PhysicsEngine::getConstraintAngle(const std::string& name)
{
    std::map<std::string, ConstraintAndState>::iterator itr = _constraints.find(name);
    if (itr == _constraints.end()) return 0.0f;
    b3JointId* joint = itr->second.first->get<b3JointId>();

    switch (b3Joint_GetType(*joint))
    {
    case b3_revoluteJoint: return b3RevoluteJoint_GetAngle(*joint);
    case b3_sphericalJoint: return b3SphericalJoint_GetConeAngle(*joint);
    default: return 0.0f;  // parallel / motor / filter joints have no angle
    }
}

float PhysicsEngine::getConstraintTwistAngle(const std::string& name)
{
    std::map<std::string, ConstraintAndState>::iterator itr = _constraints.find(name);
    if (itr == _constraints.end()) return 0.0f;
    b3JointId* joint = itr->second.first->get<b3JointId>();

    if (b3Joint_GetType(*joint) == b3_sphericalJoint)
        return b3SphericalJoint_GetTwistAngle(*joint);
    return 0.0f;
}
