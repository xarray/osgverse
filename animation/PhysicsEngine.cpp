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
    };

    struct BoxData : public ShapeData
    {
        b3BoxHull hull; BoxData(const osg::Vec3& hs)
        { hull = b3MakeBoxHull(hs.x(), hs.y(), hs.z()); volume = 8.0f * hs.x() * hs.y() * hs.z(); }
        virtual b3ShapeId createOnBody(b3WorldId, b3BodyId body, const b3ShapeDef& def)
        { return b3CreateHullShape(body, &def, &hull.base); }
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
            float h = abs(c1.z() - c0.z()); float cyVol = osg::PI * r * r * h;
            volume = (4.0f / 3.0f) * osg::PI * r * r * r + cyVol;
        }
        virtual b3ShapeId createOnBody(b3WorldId, b3BodyId body, const b3ShapeDef& def)
        { return b3CreateCapsuleShape(body, &def, &sh); }
    };

    struct SphereData : public ShapeData
    {
        b3Sphere sh; SphereData(const osg::Vec3& c, float r)
        { sh = b3Sphere { {c.x(), c.y(), c.z()}, r }; float h = r * 0.57735f; volume = 8.0f * h * h * h;  }
        virtual b3ShapeId createOnBody(b3WorldId, b3BodyId body, const b3ShapeDef& def)
        { return b3CreateSphereShape(body, &def, &sh); }
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
        CollisionShape() { shapeDef = b3DefaultShapeDef(); }
        b3ShapeDef shapeDef; osg::ref_ptr<ShapeData> shapeData;
    };

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

    static void raycastSetResult(osgVerse::PhysicsEngine::RaycastHit& result,
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
            if (getNameFromBody)
            {
                const std::map<std::string, osg::ref_ptr<RigidBodyBase>>& bodies = engine->getBodies();
                for (std::map<std::string, osg::ref_ptr<RigidBodyBase>>::const_iterator
                    itr = bodies.begin(); itr != bodies.end(); ++itr)
                {
                    b3BodyId* body = itr->second->get<b3BodyId>();
                    if (B3_ID_EQUALS((*body), bodyId)) { result.name = itr->first; break; }
                }
            }
        }
    }

    static float raycastCallback(b3ShapeId shapeId, b3Pos point, b3Vec3 normal, float fraction,
							     uint64_t materialId, int triangleIndex, int childIndex, void* context)
    {
        RaycastCallbackData* data = (RaycastCallbackData*)context;
        osgVerse::PhysicsEngine::RaycastHit result;
        raycastSetResult(result, data->engine, data->getNameFromBody, shapeId, point, normal);
        data->hits->push_back(result); return 1.0f; // Continue ray
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
                                           const osg::Matrix& matrix, bool kinematic)
{
    bool isDynamic = (mass > 0.0f); b3Vec3 pos; b3Quat rot;
    b3Helpers::PhysicsCore::fromMatrix(matrix, pos, rot);
    if (_shapes.find(name) != _shapes.end()) removeBody(name);  // remove existing shape

    b3Helpers::CollisionShape* cs = static_cast<b3Helpers::CollisionShape*>(csb);
    if (!cs || (cs && !cs->shapeData)) { OSG_NOTICE << "[PhysicsEngine] Failed to get input shape\n"; return NULL; }
    cs->shapeDef.density = (mass > 0.0f) ? (mass / cs->shapeData->volume) : 0.0f;

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
        delete itr->second; _bodies.erase(itr);
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
                            RaycastHit& result, bool getNameFromBody)
{
    b3Vec3 origin = b3Vec3{ s.x(), s.y(), s.z() };
    b3Vec3 translation = b3Vec3{ e.x() - s.x(), e.y() - s.y(), e.z() - s.z() };
    b3QueryFilter filter = b3DefaultQueryFilter();

    b3RayResult r = b3World_CastRayClosest(PHY_WORLD(), b3ToPos(origin), translation, filter);
    if (r.hit)
        b3Helpers::raycastSetResult(result, this, getNameFromBody, r.shapeId, r.point, r.normal);
    return r.hit;
}

std::vector<PhysicsEngine::RaycastHit> PhysicsEngine::raycastAll(const osg::Vec3& s, const osg::Vec3& e,
                                                                 bool getNameFromBody)
{
    b3Vec3 origin = b3Vec3{ s.x(), s.y(), s.z() };
    b3Vec3 translation = b3Vec3{ e.x() - s.x(), e.y() - s.y(), e.z() - s.z() };
    b3QueryFilter filter = b3DefaultQueryFilter();
    std::vector<RaycastHit> hitList;

    b3Helpers::RaycastCallbackData data = { this, &hitList, getNameFromBody };
    b3World_CastRay(PHY_WORLD(), b3ToPos(origin), translation, filter, b3Helpers::raycastCallback, &data);
    return hitList;
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

ConstraintBase* PhysicsEngine::createConstraintP2P(RigidBodyBase* bodyA, const osg::Vec3& pA,
                                                   RigidBodyBase* bodyB, const osg::Vec3& pB,
                                                   const ConstraintSetting* setting)
{
    if (!bodyA || !bodyB) return NULL;
    b3BodyId* bA = bodyA->get<b3BodyId>();
    b3BodyId* bB = bodyB->get<b3BodyId>();
    if (B3_IS_NULL((*bA)) || B3_IS_NULL((*bB))) return NULL;

    b3Vec3 anchorA = b3Vec3{ pA[0], pA[1], pA[2] };
    b3Vec3 anchorB = b3Vec3{ pB[0], pB[1], pB[2] };
    b3Transform localFrameA, localFrameB;
    localFrameA.q = b3Quat{ 0, 0, 0, 1 };
    localFrameB.q = b3Quat{ 0, 0, 0, 1 };

    // Box3D uses spherical joint for point-to-point constraint
    b3SphericalJointDef jointDef = b3DefaultSphericalJointDef();
    jointDef.base.bodyIdA = *bA; jointDef.base.bodyIdB = *bB;

    if (setting && setting->useWorldPivots)
    {
        b3Pos posA = b3Body_GetPosition(*bA), posB = b3Body_GetPosition(*bB);
        b3Quat rotA = b3Body_GetRotation(*bA), rotB = b3Body_GetRotation(*bB);
        b3Quat invRotA = b3Quat{ -rotA.v.x, -rotA.v.y, -rotA.v.z, rotA.s };
        b3Quat invRotB = b3Quat{ -rotB.v.x, -rotB.v.y, -rotB.v.z, rotB.s };
        localFrameA.p = b3Vec3{ anchorA.x - (float)posA.x, anchorA.y - (float)posA.y, anchorA.z - (float)posA.z };
        localFrameB.p = b3Vec3{ anchorB.x - (float)posB.x, anchorB.y - (float)posB.y, anchorB.z - (float)posB.z };
    }
    else
        { localFrameA.p = anchorA; localFrameB.p = anchorB; }
    jointDef.base.localFrameA = localFrameA; jointDef.base.localFrameB = localFrameB;

    // Box3D spherical joint doesn't have tau/damping/impulseClamp directly
    // These are solver parameters set on the world or body level
    if (setting)
    {
        // Apply settings if Box3D supports them through other mechanisms
        // TODO
    }
    b3JointId joint = b3CreateSphericalJoint(PHY_WORLD(), &jointDef);
    return new b3Helpers::Constraint(joint);
}
