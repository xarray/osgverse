#ifndef MANA_ANIM_PLAYERANIMATION_HPP
#define MANA_ANIM_PLAYERANIMATION_HPP

#include <osg/Version>
#include <osg/Texture2D>
#include <osg/Geometry>

namespace osgVerse
{

    /** The player animation support class */
    class PlayerAnimation : public osg::Referenced
    {
    public:
        typedef float (*SetJointWeightFunc)(int, int, void*);
        PlayerAnimation();

        struct GeometryJointData
        {
            typedef std::vector<std::pair<uint16_t, float>> JointWeight;
            std::vector<JointWeight> _weights;  // size must equal to vertex count
            std::map<uint16_t, osg::Matrixf> _invBindPoses;
        };

        /** Initialize the player skeleton and mesh from OSG scene graph
        *   - skeletonRoot: node_name = joint_name/parent_idx, matrix = rest_pose
        *   - meshRoot: geometry = ozz_mesh (part.pos/normal/uv/tangents/colors)
        *   - jointDataMap: joint_idx/weight (per vertex), inverse_bind_poses (per joint-in-geom)
        */
        bool initialize(osg::Node& skeletonRoot, osg::Node& meshRoot,
                        const std::map<std::string, GeometryJointData>& jointDataMap);

        /// Initialize the player from ozz skeleton and mesh files
        bool initialize(const std::string& skeleton, const std::string& mesh);

        /// Load animation data from ozz files
        bool loadAnimation(const std::string& key, const std::string& animation);
        void unloadAnimation(const std::string& key);

        bool update(const osg::FrameStamp& fs, bool paused);
        bool applyMeshes(osg::Geode& meshDataRoot, bool withSkinning);

        struct JointIkData { int joint; float weight; osg::Vec3 localUp; osg::Vec3 localForward; };
        bool updateAimIK(const osg::Vec3& target, const std::vector<JointIkData>& chain,
                         const osg::Vec3& aimOffset = osg::Vec3(), const osg::Vec3& pole = osg::Y_AXIS);
        bool updateTwoBoneIK(const osg::Vec3& target, int start, int mid, int end, bool& reached,
                             float weight = 1.0f, float soften = 1.0f, float twist = 0.0f,
                             const osg::Vec3& midAxis = osg::Z_AXIS, const osg::Vec3& pole = osg::Y_AXIS);

        typedef std::pair<int, int> ThisAndParent;
        std::vector<ThisAndParent> getSkeletonIndices(int from = -1) const;
        std::string getSkeletonJointName(int joint) const;
        int getSkeletonJointIndex(const std::string& joint) const;

        void setModelSpaceJointMatrix(int joint, const osg::Matrix& m);
        osg::Matrix getModelSpaceJointMatrix(int joint) const;

        osg::BoundingBox computeSkeletonBounds() const;
        float getAnimationStartTime(const std::string& key);
        float getTimeRatio(const std::string& key) const;
        float getDuration(const std::string& key) const;

        float getPlaybackSpeed(const std::string& key) const;
        void setPlaybackSpeed(const std::string& key, float s);

        void select(const std::string& key, float weight, bool looping);
        void selectPartial(const std::string& key, float weight, bool looping,
                           SetJointWeightFunc func, void* userData);
        void seek(const std::string& key, float timeRatio);

    protected:
        bool initializeInternal();

        std::vector<osg::ref_ptr<osg::StateSet>> _meshStateSetList;
        osg::ref_ptr<osg::Referenced> _internal;
        float _blendingThreshold;
    };

}

#endif
