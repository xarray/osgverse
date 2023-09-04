#include <osg/io_utils>
#include <osg/Version>
#include <osg/Notify>
#include <osg/Geometry>
#include <osg/Geode>

#define OZZ_INCLUDE_PRIVATE_HEADER
#include <ozz/animation/runtime/animation_keyframe.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/ik_aim_job.h>
#include <ozz/animation/runtime/ik_two_bone_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/skeleton_utils.h>
#include <ozz/base/log.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/platform.h>
#include <ozz/base/span.h>
#include <ozz/base/io/archive.h>
#include <ozz/base/io/stream.h>
#include <ozz/base/maths/box.h>
#include <ozz/base/maths/math_ex.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/simd_quaternion.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/maths/vec_float.h>
#include <ozz/base/memory/allocator.h>
#include <ozz/geometry/runtime/skinning_job.h>
#include <ozz/mesh.h>
#include <fstream>

typedef ozz::sample::Mesh OzzMesh;
class OzzAnimation : public osg::Referenced
{
public:
    bool loadSkeleton(const char* filename, ozz::animation::Skeleton* skeleton);
    bool loadAnimation(const char* filename, ozz::animation::Animation* anim);
    bool loadMesh(const char* filename, ozz::vector<ozz::sample::Mesh>* meshes);

    bool applyMesh(osg::Geometry& geom, const OzzMesh& mesh);
    bool applySkinningMesh(osg::Geometry& geom, const OzzMesh& mesh);
    void multiplySoATransformQuaternion(int index, const ozz::math::SimdQuaternion& quat,
                                        const ozz::span<ozz::math::SoaTransform>& transforms);

    struct AnimationSampler
    {
        AnimationSampler() : weight(0.0f), playbackSpeed(1.0f), timeRatio(-1.0f),
            startTime(0.0f), resetTimeRatio(true), looping(false) {}
        ozz::animation::Animation animation;
        //ozz::animation::SamplingCache cache;
        ozz::vector<ozz::math::SoaTransform> locals;
        ozz::vector<ozz::math::SimdFloat4> jointWeights;
        float weight, playbackSpeed, timeRatio, startTime;
        bool resetTimeRatio, looping;
    };

    std::map<std::string, AnimationSampler> _animations;
    ozz::animation::Skeleton _skeleton;
    ozz::animation::SamplingJob::Context _context;
    ozz::vector<ozz::math::SoaTransform> _blended_locals;
    ozz::vector<ozz::math::Float4x4> _models;
    ozz::vector<ozz::math::Float4x4> _skinning_matrices;
    ozz::vector<OzzMesh> _meshes;
};
