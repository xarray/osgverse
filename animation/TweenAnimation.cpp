#include <osg/io_utils>
#include <osg/CameraView>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <osgViewer/View>
#include "3rdparty/tweeny/tweeny.h"
#include "3rdparty/tweeny/easing.h"
#include "TweenAnimation.h"
#include <iostream>
using namespace osgVerse;

class EasingType : public osg::Referenced
{
public:
    EasingType(double s, double e, TweenAnimation::TweenMode tm)
    {
        duration = e - s; num = (duration > 0.0) ? osg::maximum(100 * (int)duration, 2000) : 1;
        if (tm == TweenAnimation::CubicInOut)
            tween = tweeny::from(s).to(e).during(num).via(tweeny::easing::cubicInOut);
        else
            tween = tweeny::from(s).to(e).during(num).via(tweeny::easing::linear);
    }

    double value(double dt) { return tween.peek((uint32_t)(dt * num / duration)); }
    tweeny::tween<double> tween; int num; double duration;
};

class AnimationPathVisitor : public osg::NodeVisitor
{
public:

    AnimationPathVisitor(const osg::AnimationPath::ControlPoint& cp,
                         const osg::Vec3d& pivotPoint, bool invMatrix)
    :   osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _cp(cp), _pivotPoint(pivotPoint), _useInverseMatrix(invMatrix) {}

    virtual void apply(osg::Camera& camera)
    {
        osg::Matrix matrix;
        if (_useInverseMatrix) _cp.getInverse(matrix); else _cp.getMatrix(matrix);

        osg::View* view = camera.getView();
        if (view != NULL)
        {
            osgViewer::View* view1 = dynamic_cast<osgViewer::View*>(view);
            if (view1 && view1->getCameraManipulator())
            {
                view1->getCameraManipulator()->setByInverseMatrix(
                    osg::Matrix::translate(-_pivotPoint) * matrix); return;
            }
        }
        camera.setViewMatrix(osg::Matrix::translate(-_pivotPoint) * matrix);
    }

    virtual void apply(osg::CameraView& cv)
    {
        if (_useInverseMatrix)
        {
            osg::Matrix matrix; _cp.getInverse(matrix);
            cv.setPosition(matrix.getTrans());
            cv.setAttitude(_cp.getRotation().inverse());
            cv.setFocalLength(1.0f / _cp.getScale().x());
        }
        else
        {
            cv.setPosition(_cp.getPosition());
            cv.setAttitude(_cp.getRotation());
            cv.setFocalLength(_cp.getScale().x());
        }
    }

    virtual void apply(osg::MatrixTransform& mt)
    {
        osg::Matrix matrix;
        if (_useInverseMatrix) _cp.getInverse(matrix);
        else _cp.getMatrix(matrix);
        mt.setMatrix(osg::Matrix::translate(-_pivotPoint) * matrix);
    }

    virtual void apply(osg::PositionAttitudeTransform& pat)
    {
        if (_useInverseMatrix)
        {
            osg::Matrix matrix; _cp.getInverse(matrix);
            pat.setPosition(matrix.getTrans());
            pat.setAttitude(_cp.getRotation().inverse());
            pat.setScale(osg::Vec3(
                1.0f / _cp.getScale().x(), 1.0f / _cp.getScale().y(), 1.0f / _cp.getScale().z()));
            pat.setPivotPoint(_pivotPoint);
        }
        else
        {
            pat.setPosition(_cp.getPosition());
            pat.setAttitude(_cp.getRotation());
            pat.setScale(_cp.getScale());
            pat.setPivotPoint(_pivotPoint);
        }
    }

    osg::AnimationPath::ControlPoint _cp;
    osg::Vec3d _pivotPoint;
    bool _useInverseMatrix;
};

void TweenAnimation::AnimationCallback::interpolate(osg::Node* node, const osg::AnimationPath::ControlPoint& cp,
                                                    const osg::Vec3d& pivotPoint, bool invMatrix)
{ AnimationPathVisitor apcv(cp, pivotPoint, invMatrix); node->accept(apcv); }

TweenAnimation::TweenAnimation()
:   _currentAnimationTime(0.0), _referenceTime(-1.0),
    _playingState(0), _useInverseMatrix(false)
{
}

void TweenAnimation::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
    double delta = 0.02, timestamp = _currentAnimationTime;
    if (_playingState > 0 && nv->getFrameStamp())
    {
        if (_referenceTime < 0.0)
        {
            if (_animationCallback.valid()) _animationCallback->onStart(this, _currentName);
            delta = 0.0;
        }
        else
            delta = nv->getFrameStamp()->getSimulationTime() - _referenceTime;
        _referenceTime = nv->getFrameStamp()->getSimulationTime();
    }

    if (_playingState == 1)
    {
        Animation& animationPair = _animations.find(_currentName)->second;
        Property& prop = animationPair.second; bool atEnd = false;
        osg::AnimationPath* path = animationPair.first.get();
        if (prop.mode == Inherited)
        {
            if (path->getLoopMode() == osg::AnimationPath::LOOP) prop.mode = Looping;
            else if (path->getLoopMode() == osg::AnimationPath::SWING) prop.mode = PingPong;
        }

        double startT = path->getFirstTime(), endT = path->getLastTime();
        switch (prop.mode)
        {
        case Reversing:
            timestamp -= delta * prop.timeMultiplier;
            if (timestamp <= startT) { timestamp = startT; _playingState = 0; atEnd = true; } break;
            break;
        case Looping:
            timestamp += delta * prop.timeMultiplier;
            if (timestamp >= endT) { timestamp = prop.timeOffset; atEnd = true; } break;
        case ReversedLooping:
            timestamp -= delta * prop.timeMultiplier;
            if (timestamp <= startT) { timestamp = prop.timeOffset; atEnd = true; } break;
        case PingPong:
            if (prop.direction == 0)
            {
                timestamp += delta * prop.timeMultiplier;
                if (timestamp >= endT) { timestamp = endT; prop.direction = 1; atEnd = true; }
            }
            else if (prop.direction > 0)
            {
                timestamp -= delta * prop.timeMultiplier;
                if (timestamp <= startT) { timestamp = startT; prop.direction = 0; atEnd = true; }
            }
            break;
        default:
            timestamp += delta * prop.timeMultiplier;
            if (timestamp >= endT)
            {
                if (prop.mode != DynamicData) _playingState = 0;
                timestamp = endT; atEnd = true;
            }
            break;
        }

        double realTimestamp = timestamp;
        if (atEnd && _animationCallback.valid())
            _animationCallback->onEnd(this, _currentName, _playingState > 0);
        if (prop.easing.valid())
        {
            EasingType* easing = static_cast<EasingType*>(prop.easing.get());
            realTimestamp = easing->value(timestamp - startT);
        }
        //std::cout << "State-" << _playingState << ": " << timestamp << " => " << realTimestamp << "\n";
        
        osg::AnimationPath::ControlPoint cp;
        if (path && getInterpolatedControlPoint(path, realTimestamp, cp))
        {
            if (!_animationCallback)
                { AnimationPathVisitor apcv(cp, _pivotPoint, _useInverseMatrix); node->accept(apcv); }
            else
                _animationCallback->interpolate(node, cp, _pivotPoint, _useInverseMatrix);
            _currentAnimationTime = timestamp;
        }
    }
    traverse(node, nv);
}

bool TweenAnimation::addAnimation(const std::string& name, osg::AnimationPath* path)
{
    if (_animations.find(name) != _animations.end() || !path) return false;
    _animations[name] = Animation(path, Property()); return true;
}

bool TweenAnimation::removeAnimation(const std::string& name)
{
    if (_animations.find(name) == _animations.end()) return false;
    _animations.erase(_animations.find(name)); return true;
}

bool TweenAnimation::setProperty(const std::string& name, float offset, float multiplier)
{
    if (_animations.find(name) == _animations.end()) return false;
    Property& prop = _animations[name].second;
    prop.timeOffset = offset; prop.timeMultiplier = multiplier; return true;
}

bool TweenAnimation::addControlPoint(const std::string& name, double time,
                                     const osg::AnimationPath::ControlPoint& cp, bool relativeToEnd)
{
    osg::AnimationPath* path = getAnimation(name); if (!path) return false;
    double realT = relativeToEnd ? (path->getLastTime() + time) : time;
    path->insert(realT, cp); return true;
}

osg::AnimationPath* TweenAnimation::getAnimation(const std::string& name)
{
    if (_animations.find(name) == _animations.end()) return NULL;
    return _animations[name].first.get();
}

TweenAnimation::PlayingMode TweenAnimation::getProperty(
        const std::string& name, float& offset, float& multiplier) const
{
    if (_animations.find(name) == _animations.end()) return TweenAnimation::Invalid;
    const Property& prop = _animations.find(name)->second.second;
    offset = prop.timeOffset; multiplier = prop.timeMultiplier; return prop.mode;
}

bool TweenAnimation::getTimeProperty(const std::string& name, double& start, double& duration) const
{
    if (_animations.find(name) == _animations.end()) return false;
    const osg::AnimationPath* p = _animations.find(name)->second.first.get();
    start = p->getFirstTime(); duration = p->getPeriod(); return true;
}

std::vector<std::string> TweenAnimation::getAnimationNames() const
{
    std::vector<std::string> names;
    for (std::map<std::string, Animation>::const_iterator itr = _animations.begin();
         itr != _animations.end(); ++itr) names.push_back(itr->first);
    return names;
}

bool TweenAnimation::play(const std::string& name0, PlayingMode pm, TweenMode tw)
{
    std::string name = (name0.empty() && !_animations.empty()) ? _animations.begin()->first : name0;
    if (_animations.find(name) == _animations.end()) return false;

    Property& prop = _animations[name].second;
    prop.mode = pm; prop.direction = 0;
    _currentName = name; _playingState = 1; _referenceTime = -1.0;
    _currentAnimationTime = prop.timeOffset;

    double start = 0.0, duration = 0.0;
    if (getTimeProperty(_currentName, start, duration))
    {
        prop.easing = (pm != DynamicData && tw != NoTweening)
                    ? new EasingType(start, start + duration, tw) : NULL;
        if (pm == Reversing || pm == ReversedLooping)
            _currentAnimationTime = start + duration - prop.timeOffset;
    }
    return true;
}

bool TweenAnimation::seek(double timestamp, bool asTimeRatio)
{
    if (_currentName.empty()) return false;
    if (asTimeRatio)
    {
        double start = 0.0, duration = 0.0;
        if (getTimeProperty(_currentName, start, duration))
            _currentAnimationTime = timestamp * duration + start;
    }
    else _currentAnimationTime = timestamp;
    return true;
}

void TweenAnimation::pause()
{
    if (_playingState == 1) _playingState = 2;
    else if (_playingState > 0) _playingState = 1;
}

double TweenAnimation::getCurrentTimeRatio() const
{
    if (_playingState > 0)
    {
        double start = 0.0, duration = 0.0;
        if (getTimeProperty(_currentName, start, duration))
            return (_currentAnimationTime - start) / duration;
    }
    return -1.0;
}

bool TweenAnimation::getInterpolatedControlPoint(osg::AnimationPath* path, double time,
                                                 osg::AnimationPath::ControlPoint& controlPoint) const
{
    const osg::AnimationPath::TimeControlPointMap& controlMap = path->getTimeControlPointMap();
    if (controlMap.empty()) return false;

    osg::AnimationPath::TimeControlPointMap::const_iterator second = controlMap.lower_bound(time);
    if (second == controlMap.begin())
        controlPoint = second->second;
    else if (second != controlMap.end())
    {
        // we have both a lower bound and the next item.
        osg::AnimationPath::TimeControlPointMap::const_iterator first = second; --first;

        // delta_time = second.time - first.time
        double delta_time = second->first - first->first;
        if (delta_time == 0.0)
            controlPoint = first->second;
        else
        {
            controlPoint.interpolate((time - first->first) / delta_time,
                                     first->second, second->second);
        }
    }
    else // (second == controlMap.end())
        controlPoint = controlMap.rbegin()->second;
    return true;
}

namespace osgVerse
{
    class FindAnimationVisitor : public osg::NodeVisitor
    {
    public:
        FindAnimationVisitor()
            : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}
        std::vector<NodeAnimationPair> animations;

        virtual void apply(osg::Transform& node)
        {
            osg::NodeCallback* cb = dynamic_cast<osg::NodeCallback*>(node.getUpdateCallback());
            while (cb)
            {
                TweenAnimation* tween = dynamic_cast<TweenAnimation*>(cb);
                if (tween) animations.push_back(NodeAnimationPair(&node, tween));
                cb = dynamic_cast<osg::NodeCallback*>(cb->getNestedCallback());
            }
            traverse(node);
        }
    };

    std::vector<NodeAnimationPair> obtainAnimations(osg::Node* node)
    { FindAnimationVisitor fav; if (node) node->accept(fav); return fav.animations; }

    struct QuickAnimationCallback : public TweenAnimation::AnimationCallback
    {
        virtual void onStart(TweenAnimation* anim, const std::string& name) {}
        virtual void onEnd(TweenAnimation* anim, const std::string& name, bool toLoop)
        { if (_endFunc != NULL) (*_endFunc)(); }

        QuickAnimationCallback(AnimationEndFunction func) : _endFunc(func) {}
        AnimationEndFunction _endFunc;
    };

    bool doAnimation(osg::Node* n, osg::AnimationPath* anim, AnimationEndFunction func,
                     TweenAnimation::TweenMode tw)
    {
        osg::NodeCallback* cb = dynamic_cast<osg::NodeCallback*>(n ? n->getUpdateCallback() : NULL);
        if (!n || !anim) return false;

        TweenAnimation* tween = dynamic_cast<TweenAnimation*>(cb);
        while (tween == NULL && cb != NULL)
        {
            cb = dynamic_cast<osg::NodeCallback*>(cb->getNestedCallback());
            tween = dynamic_cast<TweenAnimation*>(cb);
        }

        if (tween == NULL) { tween = new TweenAnimation; n->addUpdateCallback(tween); }
        tween->removeAnimation("quick"); tween->addAnimation("quick", anim);

        QuickAnimationCallback* aniCB = dynamic_cast<QuickAnimationCallback*>(tween->getAnimationCallback());
        if (aniCB != NULL) aniCB->_endFunc = func;
        else tween->setAnimationCallback(new QuickAnimationCallback(func));
        return tween->play("quick", TweenAnimation::Inherited, tw);
    }

    bool doMove(osg::Node* n, const osg::Vec3d& end, double duration, bool localSpace, bool incr,
                AnimationEndFunction func, TweenAnimation::TweenMode tw)
    {
        osg::Vec3d start, scale; osg::Quat rot, so;
        if (n != NULL && n->getNumParents() > 0)
        {
            osg::Matrix l2w = n->getWorldMatrices(localSpace ? n->getParent(0) : NULL)[0];
            l2w.decompose(start, rot, scale, so);
        }

        osg::AnimationPath* path = new osg::AnimationPath;
        path->setLoopMode(osg::AnimationPath::NO_LOOPING);
        path->insert(0.0, osg::AnimationPath::ControlPoint(start, rot, scale));
        path->insert(duration, osg::AnimationPath::ControlPoint(incr ? (start + end) : end, rot, scale));
        return doAnimation(n, path, func, tw);
    }

    bool doRotate(osg::Node* n, const osg::Quat& end, double duration, bool localSpace, bool incr,
                  AnimationEndFunction func, TweenAnimation::TweenMode tw)
    {
        osg::Vec3d pos, scale; osg::Quat start, so;
        if (n != NULL && n->getNumParents() > 0)
        {
            osg::Matrix l2w = n->getWorldMatrices(localSpace ? n->getParent(0) : NULL)[0];
            l2w.decompose(pos, start, scale, so);
        }

        osg::AnimationPath* path = new osg::AnimationPath;
        path->setLoopMode(osg::AnimationPath::NO_LOOPING);
        path->insert(0.0, osg::AnimationPath::ControlPoint(pos, start, scale));
        path->insert(duration, osg::AnimationPath::ControlPoint(pos, incr ? (start * end) : end, scale));
        return doAnimation(n, path, func, tw);
    }

    bool doScale(osg::Node* n, const osg::Vec3d& end, double duration, bool localSpace, bool incr,
                 AnimationEndFunction func, TweenAnimation::TweenMode tw)
    {
        osg::Vec3d pos, start; osg::Quat rot, so;
        if (n != NULL && n->getNumParents() > 0)
        {
            osg::Matrix l2w = n->getWorldMatrices(localSpace ? n->getParent(0) : NULL)[0];
            l2w.decompose(pos, rot, start, so);
        }

        osg::AnimationPath* path = new osg::AnimationPath;
        path->setLoopMode(osg::AnimationPath::NO_LOOPING);
        path->insert(0.0, osg::AnimationPath::ControlPoint(pos, rot, start));
        path->insert(duration, osg::AnimationPath::ControlPoint(pos, rot, incr ? (start + end) : end));
        return doAnimation(n, path, func, tw);
    }
}
