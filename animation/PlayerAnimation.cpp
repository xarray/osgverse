#include "PlayerAnimation.h"
#include <osg/io_utils>
#include <osg/Version>
#include <osg/Notify>
#include <osg/Geometry>
#include <osg/Geode>
#include <osgDB/ReadFile>
#include <osgUtil/SmoothingVisitor>

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

using namespace osgVerse;
typedef ozz::sample::Mesh OzzMesh;

static std::string& trim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(),
            [](unsigned char ch) { return !isspace(ch); }).base(), s.end());
    return s;
}

static osg::Texture2D* createTexture(const std::string& fileName)
{
    osg::ref_ptr<osg::Texture2D> tex = new osg::Texture2D;
    tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
    tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    tex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP);
    tex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP);
    tex->setImage(osgDB::readImageFile(fileName));
    return tex.release();
}

class OzzAnimation : public osg::Referenced
{
public:
    bool loadSkeleton(const char* filename, ozz::animation::Skeleton* skeleton)
    {
        ozz::io::File file(filename, "rb");
        if (!file.opened())
            ozz::log::Err() << "Failed to open skeleton file " << filename << std::endl;
        else
        {
            ozz::io::IArchive archive(&file);
            if (!archive.TestTag<ozz::animation::Skeleton>())
                ozz::log::Err() << "Failed to load skeleton instance from file "
                                << filename << std::endl;
            else { archive >> *skeleton; return true; }
        }
        return false;
    }
    
    bool loadAnimation(const char* filename, ozz::animation::Animation* anim)
    {
        ozz::io::File file(filename, "rb");
        if (!file.opened())
            ozz::log::Err() << "Failed to open animation file " << filename << std::endl;
        else
        {
            ozz::io::IArchive archive(&file);
            if (!archive.TestTag<ozz::animation::Animation>())
                ozz::log::Err() << "Failed to load animation instance from file "
                                << filename << std::endl;
            else { archive >> *anim; return true; }
        }
        return false;
    }

    bool loadMesh(const char* filename, ozz::vector<ozz::sample::Mesh>* meshes)
    {
        ozz::io::File file(filename, "rb");
        if (!file.opened())
        {
            ozz::log::Err() << "Failed to open mesh file " << filename << std::endl;
            return false;
        }
        else
        {
            ozz::io::IArchive archive(&file);
            while (archive.TestTag<OzzMesh>())
            { meshes->resize(meshes->size() + 1); archive >> meshes->back(); }
        }
        return true;
    }

    bool applyMesh(osg::Geometry& geom, const OzzMesh& mesh)
    {
        int vCount = mesh.vertex_count(), vIndex = 0, dirtyVA = 4;
        int tCount = mesh.triangle_index_count();
        if (vCount <= 0) return false;

        osg::Vec3Array* va = static_cast<osg::Vec3Array*>(geom.getVertexArray());
        if (!va) { va = new osg::Vec3Array(vCount); geom.setVertexArray(va); }
        else if (va->size() != vCount) va->resize(vCount); else dirtyVA--;

        osg::Vec3Array* na = static_cast<osg::Vec3Array*>(geom.getNormalArray());
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
        if (!na) { na = new osg::Vec3Array(vCount); geom.setNormalArray(na, osg::Array::BIND_PER_VERTEX); }
#else
        if (!na)
        {
            na = new osg::Vec3Array(vCount); geom.setNormalArray(na);
            geom.setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
        }
#endif
        else if (na->size() != vCount) na->resize(vCount); else dirtyVA--;

        osg::Vec2Array* ta = static_cast<osg::Vec2Array*>(geom.getTexCoordArray(0));
        if (!ta) { ta = new osg::Vec2Array(vCount); geom.setTexCoordArray(0, ta); }
        else if (ta->size() != vCount) ta->resize(vCount); else dirtyVA--;

        osg::Vec4ubArray* ca = static_cast<osg::Vec4ubArray*>(geom.getColorArray());
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
        if (!ca) { ca = new osg::Vec4ubArray(vCount); geom.setColorArray(ca, osg::Array::BIND_PER_VERTEX); }
#else
        if (!ca)
        {
            ca = new osg::Vec4ubArray(vCount); geom.setColorArray(ca);
            geom.setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        }
#endif
        else if (ca->size() != vCount) ca->resize(vCount); else dirtyVA--;

        osg::DrawElementsUShort* de = (geom.getNumPrimitiveSets() == 0) ? NULL
                                  : static_cast<osg::DrawElementsUShort*>(geom.getPrimitiveSet(0));
        if (!de) { de = new osg::DrawElementsUShort(GL_TRIANGLES); geom.addPrimitiveSet(de); }
        if (de->size() != tCount)
        {
            if (tCount <= 0) return false; de->resize(tCount); de->dirty();
            memcpy(&((*de)[0]), &(mesh.triangle_indices[0]), tCount * sizeof(uint16_t));
        }
        if (!dirtyVA) return true;

        bool hasNormals = true, hasUVs = true, hasColors = true;
        for (unsigned int i = 0; i < mesh.parts.size(); ++i)
        {
            const OzzMesh::Part& part = mesh.parts[i]; int count = part.vertex_count();
            memcpy(&((*va)[vIndex]), &(part.positions[0]), count * sizeof(float) * 3);
            if (part.normals.size() != count * 3) hasNormals = false;
            else memcpy(&((*na)[vIndex]), &(part.normals[0]), count * sizeof(float) * 3);
            if (part.uvs.size() != count * 2) hasUVs = false;
            else memcpy(&((*ta)[vIndex]), &(part.uvs[0]), count * sizeof(float) * 2);
            if (part.colors.size() != count * 4) hasColors = false;
            else memcpy(&((*ca)[vIndex]), &(part.colors[0]), count * sizeof(uint8_t) * 4);
            vIndex += count;
        }

        if (!hasNormals) osgUtil::SmoothingVisitor::smooth(geom);
        if (!hasColors && ca->size() > 0) memset(&((*ca)[0]), 255, ca->size() * sizeof(uint8_t) * 4);
        va->dirty(); na->dirty(); ta->dirty(); ca->dirty();
        geom.dirtyBound();
        return true;
    }

    bool applySkinningMesh(osg::Geometry& geom, const OzzMesh& mesh)
    {
        const ozz::span<ozz::math::Float4x4> skinningMat = ozz::make_span(_skinning_matrices);
        int vCount = mesh.vertex_count(), vIndex = 0, dirtyVA = 2;
        int tCount = mesh.triangle_index_count();
        if (vCount <= 0) return false;

        osg::Vec3Array* va = static_cast<osg::Vec3Array*>(geom.getVertexArray());
        if (!va) { va = new osg::Vec3Array(vCount); geom.setVertexArray(va); }
        else if (va->size() != vCount) va->resize(vCount);

        osg::Vec3Array* na = static_cast<osg::Vec3Array*>(geom.getNormalArray());
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
        if (!na) { na = new osg::Vec3Array(vCount); geom.setNormalArray(na, osg::Array::BIND_PER_VERTEX); }
#else
        if (!na)
        {
            na = new osg::Vec3Array(vCount); geom.setNormalArray(na);
            geom.setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
        }
#endif
        else if (na->size() != vCount) na->resize(vCount);

        osg::Vec2Array* ta = static_cast<osg::Vec2Array*>(geom.getTexCoordArray(0));
        if (!ta) { ta = new osg::Vec2Array(vCount); geom.setTexCoordArray(0, ta); }
        else if (ta->size() != vCount) ta->resize(vCount); else dirtyVA--;

        osg::Vec4ubArray* ca = static_cast<osg::Vec4ubArray*>(geom.getColorArray());
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
        if (!ca) { ca = new osg::Vec4ubArray(vCount); geom.setColorArray(ca, osg::Array::BIND_PER_VERTEX); }
#else
        if (!ca)
        {
            ca = new osg::Vec4ubArray(vCount); geom.setColorArray(ca);
            geom.setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        }
#endif
        else if (ca->size() != vCount) ca->resize(vCount); else dirtyVA--;

        osg::DrawElementsUShort* de = (geom.getNumPrimitiveSets() == 0) ? NULL
            : static_cast<osg::DrawElementsUShort*>(geom.getPrimitiveSet(0));
        if (!de) { de = new osg::DrawElementsUShort(GL_TRIANGLES); geom.addPrimitiveSet(de); }
        if (de->size() != tCount)
        {
            if (tCount <= 0) return false; de->resize(tCount); de->dirty();
            memcpy(&((*de)[0]), &(mesh.triangle_indices[0]), tCount * sizeof(uint16_t));
        }

        bool hasNormals = true, hasUVs = true, hasColors = true;
        for (unsigned int i = 0; i < mesh.parts.size(); ++i)
        {
            const OzzMesh::Part& part = mesh.parts[i];
            int count = part.vertex_count(), influencesCount = part.influences_count();
            ozz::vector<float> outPositions; outPositions.resize(part.positions.size());
            ozz::vector<float> outNormals; outNormals.resize(part.normals.size());
            ozz::vector<float> outTangents; outTangents.resize(part.tangents.size());

            // Setup skinning job
            ozz::geometry::SkinningJob skinningJob;
            skinningJob.vertex_count = count;
            skinningJob.influences_count = influencesCount;
            skinningJob.joint_matrices = skinningMat;
            skinningJob.joint_indices = make_span(part.joint_indices);
            skinningJob.joint_indices_stride = sizeof(uint16_t) * influencesCount;
            if (influencesCount > 1)
            {
                skinningJob.joint_weights = ozz::make_span(part.joint_weights);
                skinningJob.joint_weights_stride = sizeof(float) * (influencesCount - 1);
            }

            skinningJob.in_positions = ozz::make_span(part.positions);
            skinningJob.in_positions_stride = sizeof(float) * 3;
            skinningJob.out_positions = ozz::make_span(outPositions);
            skinningJob.out_positions_stride = skinningJob.in_positions_stride;
            if (part.tangents.size() == count * 4)
            {
                skinningJob.in_tangents = ozz::make_span(part.tangents);
                skinningJob.in_tangents_stride = sizeof(float) * 4;
                skinningJob.out_tangents = ozz::make_span(outTangents);
                skinningJob.out_tangents_stride = skinningJob.in_tangents_stride;
            }
            if (part.normals.size() == count * 3)
            {
                skinningJob.in_normals = ozz::make_span(part.normals);
                skinningJob.in_normals_stride = sizeof(float) * 3;
                skinningJob.out_normals = ozz::make_span(outNormals);
                skinningJob.out_normals_stride = skinningJob.in_normals_stride;
            }
            else hasNormals = false;

            // Get skinning result
            if (skinningJob.Run())
            {
                memcpy(&((*va)[vIndex]), &(outPositions[0]), count * sizeof(float) * 3);
                if (hasNormals)
                    memcpy(&((*na)[vIndex]), &(outNormals[0]), count * sizeof(float) * 3);
            }
            else
                ozz::log::Err() << "Failed with skinning job" << std::endl;

            // Update non-skinning attributes
            if (dirtyVA > 0)
            {
                if (part.uvs.size() != count * 2) hasUVs = false;
                else memcpy(&((*ta)[vIndex]), &(part.uvs[0]), count * sizeof(float) * 2);
                if (part.colors.size() != count * 4) hasColors = false;
                else memcpy(&((*ca)[vIndex]), &(part.colors[0]), count * sizeof(uint8_t) * 4);
            }
            vIndex += count;
        }

        if (!hasNormals) osgUtil::SmoothingVisitor::smooth(geom);
        if (!hasColors && ca->size() > 0) memset(&((*ca)[0]), 255, ca->size() * sizeof(uint8_t) * 4);
        if (dirtyVA > 0) { ta->dirty(); ca->dirty(); }
        va->dirty(); na->dirty(); geom.dirtyBound();
        return true;
    }

    void multiplySoATransformQuaternion(int index, const ozz::math::SimdQuaternion& quat,
                                        const ozz::span<ozz::math::SoaTransform>& transforms)
    {
        // Convert soa to aos in order to perform quaternion multiplication, and get back to soa
        ozz::math::SoaTransform& soaTransformRef = transforms[index / 4];
        ozz::math::SimdQuaternion aosQuats[4];
        ozz::math::Transpose4x4(&soaTransformRef.rotation.x, &aosQuats->xyzw);

        ozz::math::SimdQuaternion& aosQuatsRef = aosQuats[index & 3];
        aosQuatsRef = aosQuatsRef * quat;
        ozz::math::Transpose4x4(&aosQuats->xyzw, &soaTransformRef.rotation.x);
    }

    struct AnimationSampler
    {
        AnimationSampler() : weight(0.0f), playbackSpeed(1.0f), timeRatio(-1.0f),
                             startTime(0.0f), resetTimeRatio(true), looping(false) {}
        ozz::animation::Animation animation;
        ozz::animation::SamplingCache cache;
        ozz::vector<ozz::math::SoaTransform> locals;
        ozz::vector<ozz::math::SimdFloat4> jointWeights;
        float weight, playbackSpeed, timeRatio, startTime;
        bool resetTimeRatio, looping;
    };

    std::map<std::string, AnimationSampler> _animations;
    ozz::animation::Skeleton _skeleton;
    ozz::vector<ozz::math::SoaTransform> _blended_locals;
    ozz::vector<ozz::math::Float4x4> _models;
    ozz::vector<ozz::math::Float4x4> _skinning_matrices;
    ozz::vector<OzzMesh> _meshes;
};

PlayerAnimation::PlayerAnimation()
{
    _internal = new OzzAnimation;
    _blendingThreshold = ozz::animation::BlendingJob().threshold;
}

bool PlayerAnimation::initialize(const std::string& skeleton, const std::string& mesh)
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    if (!ozz->loadSkeleton(skeleton.c_str(), &(ozz->_skeleton))) return false;
    if (!ozz->loadMesh(mesh.c_str(), &(ozz->_meshes))) return false;
    ozz->_models.resize(ozz->_skeleton.num_joints());
    ozz->_blended_locals.resize(ozz->_skeleton.num_soa_joints());

    std::ifstream mtlIn(mesh + ".mat");
    _meshTextureList.resize(ozz->_meshes.size());
    if (mtlIn)
    {
        std::string line, meshID, channelName, fileName;
        while (std::getline(mtlIn, line))
        {
            std::stringstream ss; ss << line;
            std::getline(ss, meshID, ','); int id = atoi(meshID.c_str());
            std::getline(ss, channelName, ','); std::getline(ss, fileName);
            _meshTextureList[id].channels[trim(channelName)] = trim(fileName);
        }
        mtlIn.close();
    }

    size_t num_skinning_matrices = 0, num_joints = ozz->_skeleton.num_joints();
    for (const OzzMesh& mesh : ozz->_meshes)
        num_skinning_matrices = ozz::math::Max(num_skinning_matrices, mesh.joint_remaps.size());
    ozz->_skinning_matrices.resize(num_skinning_matrices);
    for (const OzzMesh& mesh : ozz->_meshes)
    {
        if (num_joints < mesh.highest_joint_index())
        {
            ozz::log::Err() << "The provided mesh doesn't match skeleton "
                            << "(joint count mismatch)" << std::endl;
            return false;
        }
    }
    return true;
}

bool PlayerAnimation::loadAnimation(const std::string& key, const std::string& animation)
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    OzzAnimation::AnimationSampler& sampler = ozz->_animations[key];
    if (!ozz->loadAnimation(animation.c_str(), &(sampler.animation))) return false;

    const int num_joints = ozz->_skeleton.num_joints();
    if (num_joints != sampler.animation.num_tracks())
    {
        ozz::log::Err() << "The provided animation " << key << " doesn't match skeleton "
                        << "(joint count mismatch)" << std::endl;
        return false;
    }

    sampler.cache.Resize(num_joints);
    sampler.locals.resize(ozz->_skeleton.num_soa_joints());
    if (ozz->_animations.size() > 1) sampler.weight = 0.0f;
    else sampler.weight = 1.0f;  // by default only the first animation is full weighted
    return true;
}

void PlayerAnimation::unloadAnimation(const std::string& key)
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    std::map<std::string, OzzAnimation::AnimationSampler>::iterator itr = ozz->_animations.find(key);
    if (itr != ozz->_animations.end()) ozz->_animations.erase(itr);
}

bool PlayerAnimation::update(const osg::FrameStamp& fs, bool paused)
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    ozz::vector<ozz::animation::BlendingJob::Layer> layers;

    std::map<std::string, OzzAnimation::AnimationSampler>::iterator itr;
    for (itr = ozz->_animations.begin(); itr != ozz->_animations.end(); ++itr)
    {
        OzzAnimation::AnimationSampler& sampler = itr->second;
        if (!paused)
        {
            // Compute global playing time ratio
            if (sampler.timeRatio < 0.0f)
            {
                sampler.startTime = (float)fs.getSimulationTime();
                sampler.timeRatio = 0.0f;
            }
            else if (sampler.resetTimeRatio)
            {
                sampler.startTime =
                    (float)fs.getSimulationTime() -
                    (sampler.timeRatio * sampler.animation.duration() / sampler.playbackSpeed);
                sampler.resetTimeRatio = false;
            }
            else
            {
                sampler.timeRatio = ((float)fs.getSimulationTime() - sampler.startTime)
                                  * sampler.playbackSpeed / sampler.animation.duration();
                if (sampler.looping && sampler.timeRatio > 1.0f) sampler.timeRatio = -1.0f;
            }
        }

        if (sampler.weight <= 0.0f) continue;
        ozz::animation::BlendingJob::Layer layer;
        layer.transform = make_span(sampler.locals);
        layer.weight = sampler.weight;
        if (!sampler.jointWeights.empty())
            layer.joint_weights = ozz::make_span(sampler.jointWeights);
        layers.push_back(layer);

        // Sample animation data to its local space
        ozz::animation::SamplingJob samplingJob;
        samplingJob.animation = &(sampler.animation);
        samplingJob.cache = &(sampler.cache);
        samplingJob.ratio = osg::clampBetween(sampler.timeRatio, 0.0f, 1.0f);
        samplingJob.output = ozz::make_span(sampler.locals);
        if (!samplingJob.Run()) return false;
    }

    // Set-up blending job
    ozz::animation::BlendingJob blendJob;
    blendJob.threshold = _blendingThreshold;
    blendJob.layers = ozz::make_span(layers);
    blendJob.bind_pose = ozz->_skeleton.joint_bind_poses();
    blendJob.output = ozz::make_span(ozz->_blended_locals);
    if (!blendJob.Run()) return false;

    // Convert sampler data to world space for updating skeleton
    ozz::animation::LocalToModelJob ltmJob;
    ltmJob.skeleton = &(ozz->_skeleton);
    ltmJob.input = ozz::make_span(ozz->_blended_locals);
    ltmJob.output = ozz::make_span(ozz->_models);
    return ltmJob.Run();
}

bool PlayerAnimation::updateAimIK(const osg::Vec3& target, const std::vector<JointIkData>& chain,
                                  const osg::Vec3& offset, const osg::Vec3& pole)
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    ozz::math::SimdQuaternion correction;

    ozz::animation::IKAimJob aimIK;
    aimIK.pole_vector = ozz::math::simd_float4::Load(pole[0], pole[1], pole[2], 0.0f);
    aimIK.target = ozz::math::simd_float4::Load(target[0], target[1], target[2], 0.0f);
    aimIK.joint_correction = &correction;

    // 1. Rotate forward and offset position based on the result of previous joint IK.
    // 2. Bring forward and offset back in joint local-space
    // 3. Aim is iteratively applied up to the last selected joint of the hierarchy
    for (size_t i = 0; i < chain.size(); ++i)
    {
        const JointIkData& ikData = chain[i];
        const osg::Vec3& up = ikData.localUp, dir = ikData.localForward;
        if (ikData.joint < 0 || ikData.weight <= 0.0f) return false;

        aimIK.joint = &(ozz->_models[ikData.joint]);
        aimIK.up = ozz::math::simd_float4::Load(up[0], up[1], up[2], 0.0f);
        aimIK.weight = ikData.weight;
        if (i == 0)  // First joint, uses global forward and offset
        {
            aimIK.offset = ozz::math::simd_float4::Load(offset[0], offset[1], offset[2], 0.0f);
            aimIK.forward = ozz::math::simd_float4::Load(dir[0], dir[1], dir[2], 0.0f);
        }
        else
        {
            const ozz::math::SimdFloat4 correctedForward = ozz::math::TransformVector(
                ozz->_models[i - 1], ozz::math::TransformVector(correction, aimIK.forward));
            const ozz::math::SimdFloat4 correctedOffset = ozz::math::TransformPoint(
                ozz->_models[i - 1], ozz::math::TransformVector(correction, aimIK.offset));
            const ozz::math::Float4x4 invJoint = ozz::math::Invert(ozz->_models[i]);
            aimIK.forward = ozz::math::TransformVector(invJoint, correctedForward);
            aimIK.offset = ozz::math::TransformPoint(invJoint, correctedOffset);
        }

        // Compute and apply IK quaternion to its respective local-space transforms
        if (!aimIK.Run()) return false;
        ozz->multiplySoATransformQuaternion(i, correction, ozz::make_span(ozz->_blended_locals));
    }

    // Convert IK data to world space for updating skeleton
    ozz::animation::LocalToModelJob ltmJob;
    ltmJob.skeleton = &(ozz->_skeleton);
    ltmJob.from = chain.back().joint;
    ltmJob.input = ozz::make_span(ozz->_blended_locals);
    ltmJob.output = ozz::make_span(ozz->_models);
    return ltmJob.Run();
}

bool PlayerAnimation::updateTwoBoneIK(const osg::Vec3& target, int start, int mid, int end, bool& reached,
                                      float weight, float soften, float twist,
                                      const osg::Vec3& midAxis, const osg::Vec3& pole)
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    ozz::math::SimdQuaternion startCorrection, midCorrection;
    if (start < 0 || mid < 0 || end < 0) return false;

    ozz::animation::IKTwoBoneJob boneIK;
    boneIK.target = ozz::math::simd_float4::Load(target[0], target[1], target[2], 0.0f);
    boneIK.pole_vector = ozz::math::simd_float4::Load(pole[0], pole[1], pole[2], 0.0f);
    boneIK.mid_axis = ozz::math::simd_float4::Load(midAxis[0], midAxis[1], midAxis[2], 0.0f);
    boneIK.twist_angle = twist; boneIK.weight = weight; boneIK.soften = soften;
    boneIK.start_joint = &ozz->_models[start];
    boneIK.mid_joint = &ozz->_models[mid];
    boneIK.end_joint = &ozz->_models[end];
    boneIK.start_joint_correction = &startCorrection;
    boneIK.mid_joint_correction = &midCorrection;
    boneIK.reached = &reached;

    // Apply IK quaternions to their respective local-space transforms
    if (!boneIK.Run()) return false;
    ozz->multiplySoATransformQuaternion(start, startCorrection, ozz::make_span(ozz->_blended_locals));
    ozz->multiplySoATransformQuaternion(mid, midCorrection, ozz::make_span(ozz->_blended_locals));

    // Convert IK data to world space for updating skeleton
    ozz::animation::LocalToModelJob ltmJob;
    ltmJob.skeleton = &(ozz->_skeleton);
    ltmJob.from = start; //ltmJob.to = end;
    ltmJob.input = ozz::make_span(ozz->_blended_locals);
    ltmJob.output = ozz::make_span(ozz->_models);
    return ltmJob.Run();
}

bool PlayerAnimation::applyMeshes(osg::Geode& meshDataRoot, bool withSkinning)
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    if (meshDataRoot.getNumDrawables() != ozz->_meshes.size())
    {
        std::map<std::string, osg::ref_ptr<osg::Texture2D>> textures;
        meshDataRoot.removeDrawables(0, meshDataRoot.getNumDrawables());
        for (unsigned int i = 0; i < ozz->_meshes.size(); ++i)
        {
            osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
            geom->setUseDisplayList(false);
            geom->setUseVertexBufferObjects(true);

            TextureData& td = _meshTextureList[i];
            for (std::map<std::string, std::string>::iterator it = td.channels.begin();
                 it != td.channels.end(); ++it)
            {
                osg::Texture2D* tex = textures[it->second];
                if (it->first == "DiffuseColor")
                {
                    if (!tex) { tex = createTexture(it->second); textures[it->second] = tex; }
                    geom->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex);
                }
                else {}  // TODO: implement other channels
            }
            meshDataRoot.addDrawable(geom.get());
        }
    }

    for (unsigned int i = 0; i < ozz->_meshes.size(); ++i)
    {
        const ozz::sample::Mesh& mesh = ozz->_meshes[i];
        osg::Geometry* geom = meshDataRoot.getDrawable(i)->asGeometry();
        if (!withSkinning) { ozz->applyMesh(*geom, mesh); continue; }

        // Compute each mesh's poses from world space data
        for (size_t j = 0; j < mesh.joint_remaps.size(); ++j)
        {
            ozz->_skinning_matrices[j] =
                ozz->_models[mesh.joint_remaps[j]] * mesh.inverse_bind_poses[j];
        }
        if (!ozz->applySkinningMesh(*geom, mesh)) return false;
    }
    return true;
}

std::vector<PlayerAnimation::ThisAndParent> PlayerAnimation::getSkeletonIndices(int from) const
{
    struct NameIterator
    {
        void operator()(int j, int p) { names.push_back(ThisAndParent(j, p)); }
        std::vector<ThisAndParent> names;
    } itr;

    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    itr = ozz::animation::IterateJointsDF(ozz->_skeleton, itr, from);
    return itr.names;
}

std::string PlayerAnimation::getSkeletonJointName(int j) const
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    return (j < ozz->_skeleton.num_joints()) ? ozz->_skeleton.joint_names()[j] : "";
}

int PlayerAnimation::getSkeletonJointIndex(const std::string& joint) const
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    auto names = ozz->_skeleton.joint_names();
    for (size_t i = 0; i < ozz->_skeleton.num_joints(); ++i)
    { if (names[i] == joint) return i; } return -1;
}

void PlayerAnimation::setModelSpaceJointMatrix(int joint, const osg::Matrix& matrix)
{
    ozz::math::Float4x4 m;
    for (int i = 0; i < 4; ++i)
    {
        m.cols[i] = ozz::math::simd_float4::Load(
            matrix(i, 0), matrix(i, 1), matrix(i, 2), matrix(i, 3));
    }
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    ozz->_models[joint] = m;
}

osg::Matrix PlayerAnimation::getModelSpaceJointMatrix(int joint) const
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    const ozz::math::Float4x4& m = ozz->_models[joint];
    return osg::Matrix(
        ozz::math::GetX(m.cols[0]), ozz::math::GetY(m.cols[0]), ozz::math::GetZ(m.cols[0]), ozz::math::GetW(m.cols[0]),
        ozz::math::GetX(m.cols[1]), ozz::math::GetY(m.cols[1]), ozz::math::GetZ(m.cols[1]), ozz::math::GetW(m.cols[1]),
        ozz::math::GetX(m.cols[2]), ozz::math::GetY(m.cols[2]), ozz::math::GetZ(m.cols[2]), ozz::math::GetW(m.cols[2]),
        ozz::math::GetX(m.cols[3]), ozz::math::GetY(m.cols[3]), ozz::math::GetZ(m.cols[3]), ozz::math::GetW(m.cols[3]));
}

osg::BoundingBox PlayerAnimation::computeSkeletonBounds() const
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    const int numJoints = ozz->_skeleton.num_joints();
    if (numJoints <= 0) return osg::BoundingBox();

    osg::BoundingBox bound;
    ozz::vector<ozz::math::Float4x4> models(numJoints);

    // Compute model space bind pose.
    ozz::animation::LocalToModelJob job;
    job.input = ozz->_skeleton.joint_bind_poses();
    job.output = ozz::make_span(models);
    job.skeleton = &(ozz->_skeleton);
    if (job.Run())
    {
        ozz::span<const ozz::math::Float4x4> matrices = job.output;
        if (matrices.empty()) return bound;

        const ozz::math::Float4x4* current = matrices.begin();
        ozz::math::SimdFloat4 minV = current->cols[3];
        ozz::math::SimdFloat4 maxV = current->cols[3]; ++current;
        while (current < matrices.end())
        {
            minV = ozz::math::Min(minV, current->cols[3]);
            maxV = ozz::math::Max(maxV, current->cols[3]); ++current;
        }

        ozz::math::Store3PtrU(minV, bound._min.ptr());
        ozz::math::Store3PtrU(maxV, bound._max.ptr());
    }
    return bound;
}

float PlayerAnimation::getAnimationStartTime(const std::string& key)
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    OzzAnimation::AnimationSampler& sampler = ozz->_animations[key];
    return sampler.startTime;
}

float PlayerAnimation::getTimeRatio(const std::string& key) const
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    OzzAnimation::AnimationSampler& sampler = ozz->_animations[key];
    return osg::clampBetween(sampler.timeRatio, 0.0f, 1.0f);
}

float PlayerAnimation::getDuration(const std::string& key) const
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    OzzAnimation::AnimationSampler& sampler = ozz->_animations[key];
    return sampler.animation.duration();
}

float PlayerAnimation::getPlaybackSpeed(const std::string& key) const
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    OzzAnimation::AnimationSampler& sampler = ozz->_animations[key];
    return sampler.playbackSpeed;
}

void PlayerAnimation::setPlaybackSpeed(const std::string& key, float s)
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    OzzAnimation::AnimationSampler& sampler = ozz->_animations[key];
    sampler.playbackSpeed = s;
}

void PlayerAnimation::select(const std::string& key, float weight, bool looping)
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    OzzAnimation::AnimationSampler& sampler = ozz->_animations[key];
    sampler.weight = weight; sampler.looping = looping;
    sampler.jointWeights.clear();  // clear specific joint weights
}

void PlayerAnimation::selectPartial(const std::string& key, float weight, bool looping,
                                    PlayerAnimation::SetJointWeightFunc func, void* userData)
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    OzzAnimation::AnimationSampler& sampler = ozz->_animations[key];
    sampler.weight = weight; sampler.looping = looping;
    sampler.jointWeights.resize(sampler.locals.size());

    struct WeightSetIterator
    {
        typedef ozz::vector<ozz::math::SimdFloat4>* JointWeightPtr;
        JointWeightPtr weights; SetJointWeightFunc traveller; void* userData;
        WeightSetIterator(JointWeightPtr w, SetJointWeightFunc f, void* p)
            : weights(w), traveller(f), userData(p) {}

        void operator()(int j, int p)
        {
            float v = traveller(j, p, userData);
            ozz::math::SimdFloat4& soa = weights->at(j / 4);
            soa = ozz::math::SetI(soa, ozz::math::simd_float4::Load1(v), j % 4);
        }
    } itr(&(sampler.jointWeights), func, userData);
    ozz::animation::IterateJointsDF(ozz->_skeleton, itr, -1);
}

void PlayerAnimation::seek(const std::string& key, float timeRatio)
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    OzzAnimation::AnimationSampler& sampler = ozz->_animations[key];
    sampler.timeRatio = osg::clampBetween(timeRatio, 0.0f, 1.0f);
    sampler.resetTimeRatio = true;
}
