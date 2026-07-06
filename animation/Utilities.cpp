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
