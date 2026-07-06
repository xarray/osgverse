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
    struct ShapeData
    {
        virtual ~ShapeData() {}
        virtual b3ShapeId createOnBody(b3WorldId world, b3BodyId body, const b3ShapeDef& def) = 0;
    };

    struct BoxData : public ShapeData
    {
        b3BoxHull hull;
        BoxData(const osg::Vec3& hs) { hull = b3MakeBoxHull(hs.x(), hs.y(), hs.z()); }
        virtual b3ShapeId createOnBody(b3WorldId, b3BodyId body, const b3ShapeDef& def)
        { return b3CreateHullShape(body, &def, &hull.base); }
    };

    /*struct HullData : public ShapeData
    {
        std::vector<b3Vec3> vertices;
        HullData(const std::vector<osg::Vec3>& verts)
        {
            vertices.reserve(verts.size());
            for (const auto& v : verts) vertices.push_back((b3Vec3){v.x(), v.y(), v.z()});
        }
        virtual b3ShapeId createOnBody(b3WorldId, b3BodyId body, const b3ShapeDef& def)
        {
            b3HullData hull;
            hull.vertexCount = vertices.size();
            hull.vertices = vertices.data();  // FIXME: b3ComputeHull?
            return b3CreateHullShape(body, &def, &hull);
        }
    };

    struct MeshData : public ShapeData
    {
        std::vector<b3Vec3> vertices;
        std::vector<b3MeshTriangle> triangles;
        MeshData(const std::vector<osg::Vec3>& verts, const std::vector<unsigned int>& tris)
        {
            vertices.reserve(verts.size());
            for (const auto& v : verts) vertices.push_back((b3Vec3){v.x(), v.y(), v.z()});
            int nTri = tris.size() / 3;
            triangles.reserve(nTri);
            for (int i = 0; i < nTri; ++i)
                triangles.push_back((b3MeshTriangle){tris[i*3], tris[i*3+1], tris[i*3+2]});
        }
        virtual b3ShapeId createOnBody(b3WorldId, b3BodyId body, const b3ShapeDef& def)
        {
            b3MeshData mesh;
            mesh.vertexCount = vertices.size();
            mesh.vertices = vertices.data();
            mesh.triangleCount = triangles.size();
            mesh.triangles = triangles.data();
            return b3CreateMeshShape(body, &def, &mesh);
        }
    };*/

    struct CollisionShape : public osgVerse::CollisionShapeBase
    {
        CollisionShape() { shapeDef = b3DefaultShapeDef(); }
        b3ShapeDef shapeDef; std::unique_ptr<ShapeData> shapeData;
    };

    struct RigidBody : public osgVerse::RigidBodyBase { RigidBody(b3BodyId b) : _b(b) { internal = &_b; } b3BodyId _b; };
    struct Constraint : public osgVerse::ConstraintBase { Constraint(b3JointId j) : _j(j) { internal = &_j; } b3JointId _j; };

    class PhysicsCore : public PhysicsCoreBase
    {
    public:
        PhysicsCore()
        {
            b3WorldDef worldDef = b3DefaultWorldDef();
            worldDef.gravity = (b3Vec3){ 0.0f, 0.0f, -9.8f };
            _worldId = b3CreateWorld(&worldDef);
        }

        static void fromMatrix(const osg::Matrix& m, b3Vec3& pos, b3Quat& rot)
        {
            osg::Vec4 q = m.getRotate().asVec4(); osg::Vec3 p = m.getTrans();
            pos = (b3Vec3){ p.x(), p.y(), p.z() };
            rot = (b3Quat){ (float)q.x(), q.y(), q.z(), q.w() };
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
        bool getNameFromBody, singleHit;
    };

    static float raycastCallback(b3ShapeId shapeId, b3Pos point, b3Vec3 normal, float fraction,
							     uint64_t materialId, int triangleIndex, int childIndex, void* context)
    {
        RaycastCallbackData* data = (RaycastCallbackData*)context;
        osgVerse::PhysicsEngine::RaycastHit result;
        result.position = osg::Vec3(point.x, point.y, point.z);
        result.normal = osg::Vec3(normal.x, normal.y, normal.z);
        result.rigidBody = NULL; // Will be filled later if needed
        
        // Find body from shape
        b3BodyId bodyId = b3Shape_GetBody(shapeId);
        if (B3_IS_NON_NULL(bodyId))
        {
            result.rigidBody = new b3Helpers::RigidBody(bodyId);
            if (data->getNameFromBody)
            {
                const std::map<std::string, osg::ref_ptr<RigidBodyBase>>& bodies = data->engine->getBodies();
                for (std::map<std::string, osg::ref_ptr<RigidBodyBase>>::const_iterator
                    itr = bodies.begin(); itr != bodies.end(); ++itr)
                {
                    b3BodyId* body = itr->second->get<b3BodyId>();
                    if (B3_ID_EQUALS((*body), bodyId)) { result.name = itr->first; break; }
                }
            }
        }

        if (data->singleHit)
        {
            data->hits->clear();
            data->hits->push_back(result);
            return 0.0f; // Terminate ray
        }
        else
        {
            data->hits->push_back(result);
            return 1.0f; // Continue ray
        }
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

    b3ShapeId* shapeId = csb ? csb->get<b3ShapeId>() : NULL;
    if (!shapeId) return NULL;

    // TODO: shape?
    b3BodyDef bodyDef = b3DefaultBodyDef();
    bodyDef.position = pos; bodyDef.rotation = rot;
    if (kinematic) { bodyDef.type = b3_kinematicBody; bodyDef.isAwake = true; }
    else if (isDynamic) bodyDef.type = b3_dynamicBody;
    else bodyDef.type = b3_staticBody;

    // Note: In Box3D, shapes are created with body and definition. We need to re-create or attach.
    // Since Box3D shapes are tied to bodies at creation, we handle this differently.
    // The shape was already created but not attached - we need to use the shape definition approach.
    // Actually, in Box3D, shapes are created via b3CreateHullShape etc. which takes bodyId.
    // So our createPhysics* functions need to be revised, or we create a new shape here.
    b3BodyId bodyId = b3CreateBody(PHY_WORLD(), &bodyDef);
    
    b3Helpers::RigidBody* container = new b3Helpers::RigidBody(bodyId);
    _shapes[name] = csb; _bodies[name] = container;
    return container;
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
        b3Body_SetTransform(*body, pos, rot);
    }
}

osg::Matrix PhysicsEngine::getTransform(const std::string& name, bool& valid)
{
    std::map<std::string, osg::ref_ptr<RigidBodyBase>>::iterator itr = _bodies.find(name);
    if (itr != _bodies.end())
    {
        b3BodyId* body = itr->second->get<b3BodyId>(); valid = true;
        b3Vec3 pos = b3Body_GetPosition(*body);
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
        b3Vec3 vel = (b3Vec3){ v[0], v[1], v[2] };
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
{ b3World_SetGravity(PHY_WORLD(), (b3Vec3){ gravity[0], gravity[1], gravity[2] }); }

osg::Vec3 PhysicsEngine::getGravity() const
{ b3Vec3 g = b3World_GetGravity(PHY_WORLD()); return osg::Vec3(g.x, g.y, g.z); }

bool PhysicsEngine::raycast(const osg::Vec3& s, const osg::Vec3& e,
                            RaycastHit& result, bool getNameFromBody)
{
    b3Vec3 origin = (b3Vec3){ s.x(), s.y(), s.z() };
    b3Vec3 translation = (b3Vec3){ e.x() - s.x(), e.y() - s.y(), e.z() - s.z() };
    b3QueryFilter filter = b3DefaultQueryFilter();
    std::vector<RaycastHit> hits;

    b3Helpers::RaycastCallbackData data = { this, &hits, getNameFromBody, true };
    b3World_CastRay(PHY_WORLD(), origin, translation, filter, b3Helpers::raycastCallback, &data);
    if (!hits.empty()) { result = hits[0]; return true; } else return false;
}

std::vector<PhysicsEngine::RaycastHit> PhysicsEngine::raycastAll(const osg::Vec3& s, const osg::Vec3& e,
                                                                 bool getNameFromBody)
{
    b3Vec3 origin = (b3Vec3){ s.x(), s.y(), s.z() };
    b3Vec3 translation = (b3Vec3){ e.x() - s.x(), e.y() - s.y(), e.z() - s.z() };
    b3QueryFilter filter = b3DefaultQueryFilter();
    std::vector<RaycastHit> hitList;

    b3Helpers::RaycastCallbackData data = { this, &hitList, getNameFromBody, false };
    b3World_CastRay(PHY_WORLD(), origin, translation, filter, b3Helpers::raycastCallback, &data);
    return hitList;
}

void PhysicsEngine::advance(float timeStep, int maxSubSteps)
{ b3World_Step(PHY_WORLD(), timeStep, maxSubSteps); }

CollisionShapeBase* PhysicsEngine::createPhysicsPoint()
{ 
    b3Helpers::CollisionShape* cs = new b3Helpers::CollisionShape;
    cs->shapeData.reset(new b3Helpers::BoxData(osg::Vec3(0.001f, 0.001f, 0.001f)));
    cs->shapeDef.density = 0.0f; return cs;
}

CollisionShapeBase* PhysicsEngine::createPhysicsBox(const osg::Vec3& halfSize)
{ return NULL; /* TODO */ }

CollisionShapeBase* PhysicsEngine::createPhysicsCylinder(const osg::Vec3& halfSize)
{ return NULL; /* TODO */ }

CollisionShapeBase* PhysicsEngine::createPhysicsCone(float radius, float height)
{ return NULL; /* TODO */ }

CollisionShapeBase* PhysicsEngine::createPhysicsSphere(float radius)
{ return NULL; /* TODO */ }

CollisionShapeBase* PhysicsEngine::createPhysicsHull(osg::Node* node, bool optimized)
{
    osgVerse::MeshCollector bvv; if (node != NULL) node->accept(bvv);
    const std::vector<osg::Vec3>& vertices = bvv.getVertices();
    if (vertices.empty()) return NULL;

    // TODO
    return NULL;
}

CollisionShapeBase* PhysicsEngine::createPhysicsTriangleMesh(osg::Node* node, bool compressed)
{
    osgVerse::MeshCollector bvv; if (node != NULL) node->accept(bvv);
    const std::vector<osg::Vec3>& vertices = bvv.getVertices();
    const std::vector<unsigned int>& triangles = bvv.getTriangles();
    if (vertices.empty() || triangles.empty()) return NULL;

    // TODO
    return NULL;
}

CollisionShapeBase* PhysicsEngine::createPhysicsHeightField(osg::HeightField* hf, bool filpQuad)
{
    const osg::HeightField::HeightList& heights = hf->getHeightList();
    // TODO
    return NULL;
}

ConstraintBase* PhysicsEngine::createConstraintP2P(RigidBodyBase* bodyA, const osg::Vec3& pA,
                                                   RigidBodyBase* bodyB, const osg::Vec3& pB,
                                                   const ConstraintSetting* setting)
{
    // TODO
    return NULL;
}
