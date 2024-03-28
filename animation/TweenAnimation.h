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
            Forwarding = 0, Reversing, Looping,
            ReversedLooping, PingPong
        };

        void setPivotPoint(const osg::Vec3d& pivot) { _pivotPoint = pivot; }
        const osg::Vec3d& getPivotPoint() const { return _pivotPoint; }

        void setUseInverseMatrix(bool invMatrix) { _useInverseMatrix = invMatrix; }
        bool getUseInverseMatrix() const { return _useInverseMatrix; }

        bool addAnimation(const std::string& name, osg::AnimationPath* path);
        bool removeAnimation(const std::string& name);
        bool setProperty(const std::string& name, float offset, float multiplier = 1.0f);

        osg::AnimationPath* getAnimation(const std::string& name);
        bool getProperty(const std::string& name, float& offset, float& multiplier) const;
        bool getTimeProperty(const std::string& name, double& start, double& duration) const;
        std::vector<std::string> getAnimationNames() const;

        bool play(const std::string& name, PlayingMode mode = Forwarding);
        bool seek(double timestamp, bool asTimeRatio = true);
        void stop() { _playingState = 0; }
        void pause();

        bool isPlaying(bool& isPaused) const
        { isPaused = (_playingState == 2); return _playingState > 0; }

        std::string getCurrentAnimation() const { return _currentName; }
        double getCurrentTime() const { return _currentAnimationTime; }
        double getCurrentTimeRatio() const;

    protected:
        struct Property
        {
            float timeOffset, timeMultiplier, weight;
            PlayingMode mode; int direction;
            Property() : timeOffset(0.0f), timeMultiplier(1.0f),
                         mode(Forwarding), direction(0) {}
        };

        struct TimelineNode
        {
            std::string animationName;
            double start, duration;
            TimelineNode() : start(0.0), duration(0.0) {}
        };

        typedef std::pair<osg::ref_ptr<osg::AnimationPath>, Property> Animation;
        std::map<std::string, Animation> _animations;
        std::vector<TimelineNode> _timeline;  // TODO

        std::string _currentName;
        osg::Vec3d _pivotPoint;
        double _currentAnimationTime, _referenceTime;
        int _playingState;  // 0: idle, 1: playing, 2: paused
        bool _useInverseMatrix;
    };

}

#endif
