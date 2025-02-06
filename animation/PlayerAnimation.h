#ifndef MANA_ANIM_PLAYERANIMATION_HPP
#define MANA_ANIM_PLAYERANIMATION_HPP

#include <osg/Version>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osg/AnimationPath>

namespace osgVerse
{
    class BlendShapeAnimation;

    /** The player animation support class */
    class PlayerAnimation : public osg::NodeCallback
    {
    public:
        typedef float (*SetJointWeightFunc)(int, int, void*);
        PlayerAnimation();
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

        void setPlaying(bool b, bool rp = false) { _animated = b; _restPose = rp; }
        void setDrawingSkeleton(bool b) { _drawSkeleton = b; }
        void setDrawingSkinning(bool b) { _drawSkinning = b; }

        bool getPlaying(bool* rp = NULL) const { if (rp) *rp = _restPose; return _animated; }
        bool getDrawingSkeleton() const { return _drawSkeleton; }
        bool getDrawingSkinning() const { return _drawSkinning; }

        struct GeometryJointData
        {
            typedef std::vector<std::pair<osg::Transform*, float>> JointWeights;  // [joint, weight]
            std::vector<JointWeights> _weightList;  // size equals to vertex count
            std::map<osg::Transform*, osg::Matrixf> _invBindPoseMap;  // [joint, invBindPose]
            osg::ref_ptr<osg::StateSet> _stateset;
        };

        struct AnimationData
        {
            std::map<std::string, std::string> _interpolations;
            std::vector<std::pair<float, float>> _morphFrames;
            std::vector<std::pair<float, osg::Vec3>> _positionFrames, _scaleFrames;
            std::vector<std::pair<float, osg::Vec4>> _rotationFrames;

            osg::AnimationPath* toAnimationPath() const;
            void fromAnimationPath(const osg::AnimationPath* path);
        };

        /** Initialize the player skeleton and mesh from OSG scene graph
        *   - skeletonRoot: node_name = joint_name/parent_idx, matrix = rest_pose
        *   - meshRoot: geometry = ozz_mesh (part.pos/normal/uv/tangents/colors)
        *   - jointDataMap: joint_idx/weight (per vertex), inverse_bind_poses (per joint-in-geom)
        */
        bool initialize(osg::Node& skeletonRoot, osg::Node& meshRoot,
                        const std::map<osg::Geometry*, GeometryJointData>& jointDataMap);
        bool initialize(const std::vector<osg::Transform*>& nodes,
                        const std::vector<osg::Geometry*>& meshList,
                        const std::map<osg::Geometry*, GeometryJointData>& jointDataMap);

        void setModelRoot(osg::Node* node) { _modelRoot = node; }
        osg::Node* getModelRoot() const { return _modelRoot.get(); }
        osg::Node* getSkeletonRootInModel() const { return _skeletonRoot.get(); }

        /// Initialize the player from ozz skeleton and mesh files
        bool initialize(const std::string& skeleton, const std::string& mesh);

        /// Load animation data from ozz files
        bool loadAnimation(const std::string& key, const std::string& animation);

        /// Load animation data from structure
        bool loadAnimation(const std::string& key, const std::vector<osg::Transform*>& nodes,
                           const std::map<osg::Transform*, AnimationData>& animDataMap);
        bool loadAnimation(const std::string& key, const std::vector<osg::Transform*>& nodes,
                           const std::map<osg::Transform*, osg::ref_ptr<osg::AnimationPath>>& animDataMap);
        void unloadAnimation(const std::string& key);

        /* Update functions */
        bool update(const osg::FrameStamp& fs, bool paused);
        bool applyMeshes(osg::Geode& meshDataRoot, bool withSkinning);
        bool applyTransforms(osg::Transform& root, bool createIfMissing, bool withShape = false,
                             osg::Node* shapeNode = NULL);

        /* Update IK functions */
        struct JointIkData { int joint; float weight; osg::Vec3 localUp; osg::Vec3 localForward; };
        bool updateAimIK(const osg::Vec3& target, const std::vector<JointIkData>& chain,
                         const osg::Vec3& aimOffset = osg::Vec3(), const osg::Vec3& pole = osg::Y_AXIS);
        bool updateTwoBoneIK(const osg::Vec3& target, int start, int mid, int end, bool& reached,
                             float weight = 1.0f, float soften = 1.0f, float twist = 0.0f,
                             const osg::Vec3& midAxis = osg::Z_AXIS, const osg::Vec3& pole = osg::Y_AXIS);

        /* Skeleton set/get functions */
        typedef std::pair<int, int> ThisAndParent;
        std::vector<ThisAndParent> getSkeletonIndices(int from = -1) const;
        std::string getSkeletonJointName(int joint) const;
        int getSkeletonJointIndex(const std::string& joint) const;

        typedef std::pair<int, float> JointAndWeight;
        typedef std::map<osg::Vec3, std::vector<JointAndWeight>> VertexWeights;
        VertexWeights getSkeletonVertexWeights() const;

        void setModelSpaceJointMatrix(int joint, const osg::Matrix& m);
        osg::Matrix getModelSpaceJointMatrix(int joint) const;
        osg::BoundingBox computeSkeletonBounds() const;

        /* Animation set/get functions */
        std::vector<std::string> getAnimationNames() const;
        float getAnimationStartTime(const std::string& key);
        float getTimeRatio(const std::string& key) const;
        float getDuration(const std::string& key) const;

        float getPlaybackSpeed(const std::string& key) const;
        void setPlaybackSpeed(const std::string& key, float s);

        void select(const std::string& key, float weight, bool looping);
        void selectPartial(const std::string& key, float weight, bool looping,
                           SetJointWeightFunc func, void* userData);
        void seek(const std::string& key, float timeRatio);

        /* Blendshape set/get functions */
        void setBlendShape(const std::string& key, float weight);
        void clearAllBlendShapes();
        std::vector<std::string> getBlendshapeNames() const;

        BlendShapeAnimation* getBlendShapeCallback(int i) { return _blendshapes[i].get(); }
        const BlendShapeAnimation* getBlendShapeCallback(int i) const { return _blendshapes[i].get(); }
        unsigned int getNumBlendShapeCallbacks() const { return _blendshapes.size(); }

    protected:
        virtual ~PlayerAnimation();
        bool initializeInternal();
        bool loadAnimationInternal(const std::string& key);
        void updateSkeletonMesh(osg::Geometry& geom);

        std::vector<osg::ref_ptr<BlendShapeAnimation>> _blendshapes;
        std::vector<osg::ref_ptr<osg::StateSet>> _meshStateSetList;
        osg::observer_ptr<osg::Node> _modelRoot, _skeletonRoot;
        osg::ref_ptr<osg::Referenced> _internal;
        float _blendingThreshold;
        bool _animated, _drawSkeleton, _drawSkinning, _restPose;
    };

}

#endif
