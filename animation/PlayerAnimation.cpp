#include "PlayerAnimation.h"
#include "PlayerAnimationInternal.h"
#include <osg/io_utils>
#include <osg/Version>
#include <osg/Notify>
#include <osgDB/ReadFile>
using namespace osgVerse;

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

PlayerAnimation::PlayerAnimation()
{
    _internal = new OzzAnimation;
    _blendingThreshold = ozz::animation::BlendingJob().threshold;
}

bool PlayerAnimation::initialize(osg::Node& skeletonRoot, osg::Node& meshRoot,
                                 const std::map<std::string, GeometryJointData>& jointDataMap)
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    // TODO!!
    return initializeInternal();
}

bool PlayerAnimation::initialize(const std::string& skeleton, const std::string& mesh)
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    if (!ozz->loadSkeleton(skeleton.c_str(), &(ozz->_skeleton))) return false;
    if (!ozz->loadMesh(mesh.c_str(), &(ozz->_meshes))) return false;
    return initializeInternal();
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

    sampler.locals.resize(ozz->_skeleton.num_soa_joints());
    ozz->_context.Resize(num_joints);
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

    osg::BoundingBoxf bound;
    ozz::vector<ozz::math::Float4x4> models(numJoints);

    // Compute model space bind pose.
    ozz::animation::LocalToModelJob job;
    job.input = ozz->_skeleton.joint_rest_poses();
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

bool PlayerAnimation::initializeInternal()
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    ozz->_models.resize(ozz->_skeleton.num_joints());
    ozz->_blended_locals.resize(ozz->_skeleton.num_soa_joints());

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
