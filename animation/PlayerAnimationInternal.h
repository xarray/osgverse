#include <osg/io_utils>
#include <osg/Version>
#include <osg/Notify>
#include <osg/Geometry>
#include <osg/Geode>

#define OZZ_INCLUDE_PRIVATE_HEADER
#include "3rdparty/ozz/animation/runtime/animation_keyframe.h"
#include "3rdparty/ozz/animation/runtime/animation_utils.h"
#include "3rdparty/ozz/animation/runtime/animation.h"
#include "3rdparty/ozz/animation/runtime/sampling_job.h"
#include "3rdparty/ozz/animation/runtime/blending_job.h"
#include "3rdparty/ozz/animation/runtime/local_to_model_job.h"
#include "3rdparty/ozz/animation/runtime/ik_aim_job.h"
#include "3rdparty/ozz/animation/runtime/ik_two_bone_job.h"
#include "3rdparty/ozz/animation/runtime/skeleton.h"
#include "3rdparty/ozz/animation/runtime/skeleton_utils.h"
#include "3rdparty/ozz/base/log.h"
#include "3rdparty/ozz/base/containers/vector.h"
#include "3rdparty/ozz/base/platform.h"
#include "3rdparty/ozz/base/span.h"
#include "3rdparty/ozz/base/encode/group_varint.h"
#include "3rdparty/ozz/base/io/archive.h"
#include "3rdparty/ozz/base/io/stream.h"
#include "3rdparty/ozz/base/maths/box.h"
#include "3rdparty/ozz/base/maths/math_ex.h"
#include "3rdparty/ozz/base/maths/simd_math.h"
#include "3rdparty/ozz/base/maths/simd_quaternion.h"
#include "3rdparty/ozz/base/maths/soa_transform.h"
#include "3rdparty/ozz/base/maths/vec_float.h"
#include "3rdparty/ozz/base/memory/allocator.h"
#include "3rdparty/ozz/geometry/runtime/skinning_job.h"
#include "3rdparty/ozz/mesh.h"
#include <fstream>

typedef ozz::sample::Mesh OzzMesh;
class OzzAnimation : public osg::Referenced
{
public:
    typedef void (*ValidateSkinningFunc)(osg::Vec3*, osg::Vec3*, int, int);
    OzzAnimation() : _validator(NULL), _allocatedBuffer(NULL) {}

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
    ozz::vector<std::string> _meshNames;
    ValidateSkinningFunc _validator;
    void* _allocatedBuffer;

    static osg::Matrix convertMatrix(const ozz::math::Float4x4& m)
    {
        return osg::Matrix(
            ozz::math::GetX(m.cols[0]), ozz::math::GetY(m.cols[0]), ozz::math::GetZ(m.cols[0]), ozz::math::GetW(m.cols[0]),
            ozz::math::GetX(m.cols[1]), ozz::math::GetY(m.cols[1]), ozz::math::GetZ(m.cols[1]), ozz::math::GetW(m.cols[1]),
            ozz::math::GetX(m.cols[2]), ozz::math::GetY(m.cols[2]), ozz::math::GetZ(m.cols[2]), ozz::math::GetW(m.cols[2]),
            ozz::math::GetX(m.cols[3]), ozz::math::GetY(m.cols[3]), ozz::math::GetZ(m.cols[3]), ozz::math::GetW(m.cols[3]));
    }

    static std::vector<osg::Vec3> skinVertices(const OzzMesh::Part& part, const ozz::span<ozz::math::Float4x4> skinningMat)
    {
        const float* in_positions = part.positions.data();
        const float* joint_weights = part.joint_weights.data();
        const uint16_t* joint_indices = part.joint_indices.data();
        int vertex_count = part.vertex_count(), influencesCount = part.influences_count(),
            numMatrices = (int)skinningMat.size();
        std::vector<osg::Matrix> matrices(numMatrices);
        for (size_t i = 0; i < numMatrices; ++i) matrices[i] = convertMatrix(skinningMat[i]); 

        std::vector<osg::Vec3> out_positions(vertex_count);
        for (int v = 0; v < vertex_count; ++v)
        {
            uint16_t j0 = joint_indices[v * 4 + 0], j1 = joint_indices[v * 4 + 1],
                     j2 = joint_indices[v * 4 + 2], j3 = joint_indices[v * 4 + 3];
            if (numMatrices <= j0 || numMatrices <= j1 || numMatrices <= j2 || numMatrices <= j3)
            {
                std::cout << "[OzzAnimation] Invalid joint-group: " << j0 << ", " << j1 << ", " << j2 << ", " << j3
                          << " (NumMatrices = " << numMatrices << ")\n"; continue;
            }
            
            float w0 = joint_weights[v * 3 + 0], w1 = joint_weights[v * 3 + 1], w2 = joint_weights[v * 3 + 2];
            osg::Vec3 pos(in_positions[v * 3 + 0], in_positions[v * 3 + 1], in_positions[v * 3 + 2]);
            out_positions[v] = matrices[j0] * pos * w0 + matrices[j1] * pos * w1 +
                               matrices[j2] * pos * w2 + matrices[j3] * pos * (1.0f - (w0 + w1 + w2));
        }
        return out_positions;
    }
};
