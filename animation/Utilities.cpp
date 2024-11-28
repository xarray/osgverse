#include <osg/io_utils>
#include <osg/Version>
#include <osg/ShapeDrawable>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <osgDB/ReadFile>
#include <osgUtil/SmoothingVisitor>

#define TIMEOUT_CB_OVERRIDE
extern "C"
{
    struct timeout_cb
    {
        void (*fn)(osg::Referenced*);
        osg::observer_ptr<osg::Referenced> arg;
    };
    #include <3rdparty/timeout.h>
}

#ifdef VERSE_WITH_BULLET
#   include <btBulletDynamicsCommon.h>
#   include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#endif
#include <3rdparty/filters/Butterworth.h>
#include <modeling/Utilities.h>
#include "Utilities.h"
#include <chrono>
#include <mutex>
using namespace osgVerse;

/// TimeOut ///
static std::mutex g_timeOutMutex;
static void timeOutLoop(TimeOut* timeOut)
{
    timeouts* to = (timeouts*)timeOut->getTimeWheel();
    while (timeOut->running())
    {
        std::this_thread::sleep_for(std::chrono::nanoseconds(500));
        std::chrono::steady_clock::time_point t0 = timeOut->getStart();
        std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();

        std::unique_lock<std::mutex> lk(g_timeOutMutex);
        timeouts_update(to, std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    }
}

TimeOut::TimeOut()
{
    int err = 0; timeouts* to = timeouts_open(0, &err);
    _start = std::chrono::steady_clock::now(); _running = true;
    _timeWheel = to; _thread = std::thread(timeOutLoop, this);
}

TimeOut::~TimeOut()
{
    struct timeout* toResult = NULL; timeouts* to = (timeouts*)_timeWheel;
    _running = false; _thread.join();
    timeouts_update(to, std::numeric_limits<uint64_t>::max());

    while ((toResult = timeouts_get(to)))  //FIXME: check if any memory leak?
    { timeout_del(toResult); delete toResult; }
    if (to != NULL) timeouts_close(to);
}

void TimeOut::set(osg::Referenced* sender, TimeOutCallback cb, double seconds, bool repeated)
{
    timeouts* to = (timeouts*)_timeWheel;
    struct timeout* toData = new struct timeout;
    timeout_init(toData, repeated ? TIMEOUT_INT : 0);
    toData->callback.fn = cb; toData->callback.arg = sender;

    std::unique_lock<std::mutex> lk(g_timeOutMutex);
    std::chrono::milliseconds t((long long)(seconds * 1000.0));
    timeouts_add(to, toData, std::chrono::duration_cast<std::chrono::nanoseconds>(t).count());
}

void TimeOut::set(osg::Referenced* sender, TimeOutCallback cb, std::chrono::steady_clock::time_point t)
{
    timeouts* to = (timeouts*)_timeWheel;
    struct timeout* toData = new struct timeout;
    timeout_init(toData, TIMEOUT_ABS);
    toData->callback.fn = cb; toData->callback.arg = sender;

    std::unique_lock<std::mutex> lk(g_timeOutMutex);
    timeouts_add(to, toData, std::chrono::duration_cast<std::chrono::nanoseconds>(t - _start).count());
}

bool TimeOut::checkTimeouts()
{
    std::vector<timeout_cb> callbacks;
    {
        struct timeout* toResult = NULL; timeouts* to = (timeouts*)_timeWheel;
        std::unique_lock<std::mutex> lk(g_timeOutMutex);
        while ((toResult = timeouts_get(to)))
            callbacks.push_back(toResult->callback);
    }

    for (size_t i = 0; i < callbacks.size(); ++i)
        callbacks[i].fn(callbacks[i].arg.get());
    return !callbacks.empty();
}

/// VectorSmoother ///
struct ButterworthFilter : public FilterBase
{
    ButterworthFilter(double sampleRate, double cutoff)
    { f.setup(2, sampleRate, cutoff); f.reset(); }

    virtual double filter(double input) { return f.filter(input); }
    Iir::Butterworth::LowPass<2> f;
};

VectorSmoother::VectorSmoother(double sampleRate)
{
    _filter[0] = new ButterworthFilter(sampleRate, 5);
    _filter[1] = new ButterworthFilter(sampleRate, 5);
    _filter[2] = new ButterworthFilter(sampleRate, 5);
}

#ifdef VERSE_WITH_BULLET

/// PhysicsUpdateCallback ///
PhysicsUpdateCallback::PhysicsUpdateCallback(PhysicsEngine* e, const std::string& n)
{ _engine = e; _bodyName = n; }

void PhysicsUpdateCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
    if (_engine.valid())
    {
        bool isValid = false;
        osg::Matrix m = _engine->getTransform(_bodyName, isValid);

        osg::Group* group = node->asGroup();
        if (group && isValid)
        {
            osg::Transform* transform = group->asTransform();
            if (transform)
            {
                osg::MatrixTransform* mt = transform->asMatrixTransform();
                if (mt) mt->setMatrix(m);

                osg::PositionAttitudeTransform* pat =
                    transform->asPositionAttitudeTransform();
                if (pat) { pat->setAttitude(m.getRotate()); pat->setPosition(m.getTrans()); }
            }
        }
    }
    traverse(node, nv);
}

namespace osgVerse
{

    btCollisionShape* createPhysicsPoint()
    { return new btEmptyShape(); }

    btCollisionShape* createPhysicsBox(const osg::Vec3& halfSize)
    { return new btBoxShape(btVector3(halfSize[0], halfSize[1], halfSize[2])); }

    btCollisionShape* createPhysicsCylinder(const osg::Vec3& halfSize)
    { return new btCylinderShape(btVector3(halfSize[0], halfSize[1], halfSize[2])); }

    btCollisionShape* createPhysicsCone(float radius, float height)
    { return new btConeShape(radius, height); }

    btCollisionShape* createPhysicsSphere(float radius)
    { return new btSphereShape(radius); }

    btCollisionShape* createPhysicsHull(osg::Node* node, bool optimized)
    {
        osgVerse::MeshCollector bvv; if (node != NULL) node->accept(bvv);
        const std::vector<osg::Vec3>& vertices = bvv.getVertices();
        if (vertices.empty()) return NULL;

        btConvexHullShape* shape = new btConvexHullShape(
            (const btScalar*)&vertices[0], vertices.size(), sizeof(btScalar) * 3);
        if (optimized) { shape->optimizeConvexHull(); shape->initializePolyhedralFeatures(); }
        return shape;
    }

    btCollisionShape* createPhysicsTriangleMesh(osg::Node* node, bool compressed)
    {
        osgVerse::MeshCollector bvv; if (node != NULL) node->accept(bvv);
        const std::vector<osg::Vec3>& vertices = bvv.getVertices();
        const std::vector<unsigned int>& triangles = bvv.getTriangles();
        if (vertices.empty() || triangles.empty()) return NULL;

        btIndexedMesh meshPart;
        meshPart.m_numTriangles = triangles.size() / 3;
        meshPart.m_numVertices = vertices.size();
        meshPart.m_indexType = PHY_INTEGER;
        meshPart.m_triangleIndexStride = 3 * sizeof(int);
        meshPart.m_vertexType = PHY_FLOAT;
        meshPart.m_vertexStride = sizeof(btVector3FloatData);

        int* indexArray = (int*)btAlignedAlloc(sizeof(int) * 3 * meshPart.m_numTriangles, 16);
        for (int j = 0; j < 3 * meshPart.m_numTriangles; j++) indexArray[j] = triangles[j];
        meshPart.m_triangleIndexBase = (const unsigned char*)indexArray;

        btVector3FloatData* btVertices = (btVector3FloatData*)btAlignedAlloc(
            sizeof(btVector3FloatData) * meshPart.m_numVertices, 16);
        for (int j = 0; j < meshPart.m_numVertices; j++)
        {
            btVertices[j].m_floats[0] = vertices[j][0];
            btVertices[j].m_floats[1] = vertices[j][1];
            btVertices[j].m_floats[2] = vertices[j][2];
            btVertices[j].m_floats[3] = 0.f;
        }
        meshPart.m_vertexBase = (const unsigned char*)btVertices;

        btTriangleIndexVertexArray* meshInterface = new btTriangleIndexVertexArray();
        meshInterface->addIndexedMesh(meshPart, meshPart.m_indexType);
        return new btBvhTriangleMeshShape(meshInterface, compressed);
    }

    btCollisionShape* createPhysicsHeightField(osg::HeightField* hf, bool filpQuad)
    {
        const osg::HeightField::HeightList& heights = hf->getHeightList();
        btScalar minHeight = FLT_MAX, maxHeight = -FLT_MAX;
        for (size_t i = 0; i < heights.size(); ++i)
        {
            float h = heights[i];
            if (h < minHeight) minHeight = h;
            if (h > maxHeight) maxHeight = h;
        }  // TODO: check if correct

        btHeightfieldTerrainShape* shape = new btHeightfieldTerrainShape(
            hf->getNumRows(), hf->getNumColumns(), &heights[0],
            minHeight, maxHeight, 2, filpQuad);
        shape->setLocalScaling(btVector3(hf->getXInterval(), hf->getYInterval(), 1.0f));
        shape->setUseDiamondSubdivision(true); return shape;
    }

    btTypedConstraint* createConstraintP2P(btRigidBody* bodyA, const osg::Vec3& pA,
                                           btRigidBody* bodyB, const osg::Vec3& pB,
                                           const ConstraintSetting* setting)
    {
        btVector3 pivotA(pA[0], pA[1], pA[2]), pivotB(pB[0], pB[1], pB[2]);
        if (!bodyA || !bodyB) return NULL;
        if (setting && setting->useWorldPivots)
        {
            pivotA = bodyA->getCenterOfMassTransform().inverse() * pivotA;
            pivotB = bodyB->getCenterOfMassTransform().inverse() * pivotB;
        }

        btPoint2PointConstraint* p2p = new btPoint2PointConstraint(*bodyA, *bodyB, pivotA, pivotB);
        if (setting)
        {
            p2p->m_setting.m_tau = setting->tau;
            p2p->m_setting.m_damping = setting->damping;
            p2p->m_setting.m_impulseClamp = setting->impulseClamp;
        }
        return p2p;
    }
}
#endif
