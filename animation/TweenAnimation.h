#ifndef MANA_ANIM_TWEENANIMATION_HPP
#define MANA_ANIM_TWEENANIMATION_HPP

#include <osg/Version>
#include <osg/AnimationPath>
#include <osg/Texture2D>
#include <osg/Geometry>

namespace osgVerse
{

    /** The tweening animation support class */
    class TweenAnimation : public osg::NodeCallback
    {
    public:
        TweenAnimation();
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

        enum PlayingMode
        {
            Inherited = 0, Forwarding, Reversing, Looping,
            ReversedLooping, PingPong, Invalid
        };

        enum TweenMode
        {
            NoTweening = 0, Linear, CubicInOut
        };

        void setPivotPoint(const osg::Vec3d& pivot) { _pivotPoint = pivot; }
        const osg::Vec3d& getPivotPoint() const { return _pivotPoint; }

        void setUseInverseMatrix(bool invMatrix) { _useInverseMatrix = invMatrix; }
        bool getUseInverseMatrix() const { return _useInverseMatrix; }

        bool addAnimation(const std::string& name, osg::AnimationPath* path);
        bool removeAnimation(const std::string& name);
        bool setProperty(const std::string& name, float offset, float multiplier = 1.0f);

        osg::AnimationPath* getAnimation(const std::string& name);
        PlayingMode getProperty(const std::string& name, float& offset, float& multiplier) const;
        bool getTimeProperty(const std::string& name, double& start, double& duration) const;
        std::vector<std::string> getAnimationNames() const;

        bool play(const std::string& name, PlayingMode pm = Inherited, TweenMode tw = NoTweening);
        bool seek(double timestamp, bool asTimeRatio = true);
        void stop() { _playingState = 0; }
        void pause();

        bool isPlaying(bool& isPaused) const
        { isPaused = (_playingState == 2); return _playingState > 0; }

        std::string getCurrentAnimation() const { return _currentName; }
        double getCurrentTime() const { return _currentAnimationTime; }
        double getCurrentTimeRatio() const;

        struct AnimationCallback : public osg::Referenced
        {
            virtual void onStart(TweenAnimation* anim, const std::string& name) {}
            virtual void onEnd(TweenAnimation* anim, const std::string& name, bool toLoop) {}
            virtual void interpolate(osg::Node* node, const osg::AnimationPath::ControlPoint& cp,
                                     const osg::Vec3d& pivotPoint, bool invMatrix);
        };
        void setAnimationCallback(AnimationCallback* acb) { _animationCallback = acb; }
        AnimationCallback* getAnimationCallback() { return _animationCallback.get(); }

    protected:
        bool getInterpolatedControlPoint(osg::AnimationPath* path, double time,
                                         osg::AnimationPath::ControlPoint& controlPoint) const;

        struct Property
        {
            osg::ref_ptr<osg::Referenced> easing;
            float timeOffset, timeMultiplier, weight;
            PlayingMode mode; int direction;
            Property() : timeOffset(0.0f), timeMultiplier(1.0f),
                         mode(Forwarding), direction(0) {}
        };

        typedef std::pair<osg::ref_ptr<osg::AnimationPath>, Property> Animation;
        std::map<std::string, Animation> _animations;

        osg::ref_ptr<AnimationCallback> _animationCallback;
        std::string _currentName;
        osg::Vec3d _pivotPoint;
        double _currentAnimationTime, _referenceTime;
        int _playingState;  // 0: idle, 1: playing, 2: paused
        bool _useInverseMatrix;
    };

}

#endif
