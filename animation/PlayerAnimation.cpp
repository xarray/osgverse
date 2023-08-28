#include "PlayerAnimation.h"
#include "PlayerAnimationInternal.h"
#include <osg/io_utils>
#include <osg/Version>
#include <osg/TriangleIndexFunctor>
#include <osgDB/ReadFile>
#include <nanoid/nanoid.h>
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

namespace ozz
{
    namespace animation
    {
        class CreateSkeletonVisitor : public osg::NodeVisitor
        {
        public:
            CreateSkeletonVisitor()
                : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}
            const std::vector<osg::Transform*>& getSkeletonNodes() const { return _nodeList; }

            virtual void apply(osg::Transform& node)
            {
                if (node.getName().empty())
                {
                    OSG_WARN << "[PlayerAnimation] Find possible skeleton node without name. "
                             << "Will set a default one but will not work with animations." << std::endl;
                    node.setName("Joint_" + nanoid::generate(8));
                }

                if (node.getNumParents() > 0)
                {
                    if (_parentMap.find(&node) == _parentMap.end())
                    {
                        _parentMap[&node] = node.getParent(0);
                        _nodeList.push_back(&node);
                        _namesToStore.append(node.getName() + " ");
                        _namesToStore.back() = '\0';
                    }
                }
                traverse(node);
            }

            void initialize(const std::vector<osg::Transform*>& skeletonList)
            {   // call this if not use me as a visitor
                for (size_t i = 0; i < skeletonList.size(); ++i)
                {
                    osg::Transform* node = skeletonList[i];
                    if (node->getNumParents() > 0)
                        _parentMap[node] = node->getParent(0);
                    _namesToStore.append(node->getName() + " ");
                    _namesToStore.back() = '\0';
                }
                _nodeList = skeletonList;
            }

            void build(ozz::animation::Skeleton& skeleton)
            {
                int32_t charsCount = (int32_t)_namesToStore.size();
                int32_t numJoints = (int32_t)_nodeList.size();
                skeleton.Deallocate(); if (numJoints < 1) return;

                char* cursor = skeleton.Allocate(charsCount, numJoints);
                memcpy(cursor, _namesToStore.data(), charsCount);
                for (int i = 0; i < numJoints - 1; ++i)
                {
                    skeleton.joint_names_[i] = cursor;
                    cursor += std::strlen(skeleton.joint_names_[i]) + 1;
                }
                skeleton.joint_names_[numJoints - 1] = cursor;

                osg::Matrix matrices[4]; int soaIndex = 0;
                for (int i = 0; i < numJoints; ++i)
                {
                    osg::Transform* t = _nodeList[i];
                    skeleton.joint_parents_[i] = -1;
                    if (_parentMap.find(t) != _parentMap.end())
                    {
                        osg::Transform* parent = dynamic_cast<osg::Transform*>(_parentMap.find(t)->second);
                        if (parent != NULL)
                        {
                            std::vector<osg::Transform*>::iterator itr =
                                std::find(_nodeList.begin(), _nodeList.end(), parent);
                            if (itr != _nodeList.end())
                                skeleton.joint_parents_[i] = std::distance(_nodeList.begin(), itr);
                        }
                    }
                    
                    t->computeLocalToWorldMatrix(matrices[soaIndex++], NULL);
                    if (soaIndex == 4)
                    { applySoaTransform(skeleton, matrices, i / 4, soaIndex); soaIndex = 0; }
                }
                if (soaIndex != 0)
                    applySoaTransform(skeleton, matrices, (numJoints + 3) / 4 - 1, soaIndex);
            }

        protected:
            void applySoaTransform(ozz::animation::Skeleton& skeleton, osg::Matrix* matrices,
                                   int idx, int numSoa)
            {
                osg::Vec3 p[4], s[4]; osg::Quat q[4], so;
                for (int j = 0; j < numSoa; ++j) matrices[j].decompose(p[j], q[j], s[j], so);
                for (int j = numSoa; j < 4; ++j) q[j] = osg::Quat(0.0f, 0.0f, 0.0f, 0.0f);

                skeleton.joint_rest_poses_[idx] = ozz::math::SoaTransform{
                    ozz::math::SoaFloat3 {
                        ozz::math::simd_float4::Load(p[0].x(), p[1].x(), p[2].x(), p[3].x()),
                        ozz::math::simd_float4::Load(p[0].y(), p[1].y(), p[2].y(), p[3].y()),
                        ozz::math::simd_float4::Load(p[0].z(), p[1].z(), p[2].z(), p[3].z()) },
                    ozz::math::SoaQuaternion {
                        ozz::math::simd_float4::Load(q[0].x(), q[1].x(), q[2].x(), q[3].x()),
                        ozz::math::simd_float4::Load(q[0].y(), q[1].y(), q[2].y(), q[3].y()),
                        ozz::math::simd_float4::Load(q[0].z(), q[1].z(), q[2].z(), q[3].z()),
                        ozz::math::simd_float4::Load(q[0].w(), q[1].w(), q[2].w(), q[3].w()) },
                    ozz::math::SoaFloat3 {
                        ozz::math::simd_float4::Load(s[0].x(), s[1].x(), s[2].x(), s[3].x()),
                        ozz::math::simd_float4::Load(s[0].y(), s[1].y(), s[2].y(), s[3].y()),
                        ozz::math::simd_float4::Load(s[0].z(), s[1].z(), s[2].z(), s[3].z()) } };
            }

            std::map<osg::Transform*, osg::Node*> _parentMap;
            std::vector<osg::Transform*> _nodeList;
            std::string _namesToStore;
        };

        struct CollectTriangleOperator
        {
            void operator()(unsigned int i1, unsigned int i2, unsigned int i3)
            {
                if (i1 == i2 || i2 == i3 || i1 == i3) return;
                triangles.push_back(startIndex + i1);
                triangles.push_back(startIndex + i2);
                triangles.push_back(startIndex + i3);
            }

            std::vector<uint16_t> triangles;
            uint16_t startIndex;
        };

        class CreateMeshVisitor : public osg::NodeVisitor
        {
        public:
            CreateMeshVisitor(const std::vector<osg::Transform*>& nodes,
                              const std::map<osg::Geometry*, PlayerAnimation::GeometryJointData>& jd)
                : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), _jointMap(jd)
            { for (size_t i = 0; i < nodes.size(); ++i) _nodeMap[nodes[i]] = i; }

            ozz::vector<OzzMesh>& getMeshes() { return _meshes; }
            std::vector<osg::ref_ptr<osg::StateSet>>& getStateSets() { return _stateSetList; }

            virtual void apply(osg::Geode& node)
            {
                for (size_t i = 0; i < node.getNumDrawables(); ++i)
                {
                    osg::Geometry* geom = node.getDrawable(i)->asGeometry();
                    if (geom && _jointMap.find(geom) != _jointMap.end())
                        apply(*geom, _jointMap.find(geom)->second);
                }
                traverse(node);
            }

            void initialize(const std::vector<osg::Geometry*>& meshList)
            {   // call this if not use me as a visitor
                for (size_t i = 0; i < meshList.size(); ++i)
                {
                    osg::Geometry* geom = meshList[i];
                    if (geom && _jointMap.find(geom) != _jointMap.end())
                        apply(*geom, _jointMap.find(geom)->second);
                }
            }

            void apply(osg::Geometry& geom, PlayerAnimation::GeometryJointData& jData)
            {
                osg::Vec3Array* va = static_cast<osg::Vec3Array*>(geom.getVertexArray());
                osg::Vec3Array* na = static_cast<osg::Vec3Array*>(geom.getNormalArray());
                osg::Vec3Array* ta = dynamic_cast<osg::Vec3Array*>(geom.getVertexAttribArray(6));
                osg::Vec4Array* ca = dynamic_cast<osg::Vec4Array*>(geom.getColorArray());
                osg::Vec2Array* uv = dynamic_cast<osg::Vec2Array*>(geom.getTexCoordArray(0));
                if (!va || (va && va->empty())) return;

                size_t vCount = va->size(), wCount = jData._weightList.size();
                if (vCount != wCount)
                {
                    OSG_WARN << "[PlayerAnimation] Imported joint-weight list size mismatched: "
                             << wCount << " != (vertex count) " << vCount << std::endl;
                    return;
                }

                // Apply to mesh part
                OzzMesh::Part meshPart; meshPart.positions.resize(vCount * 3);
                memcpy(&meshPart.positions[0], &(*va)[0], vCount * sizeof(float) * 3);
                if (ta && ta->size() == vCount)
                {
                    for (size_t i = 0; i < vCount; ++i)
                    {
                        const osg::Vec3& t = (*ta)[i]; meshPart.tangents.push_back(t[0]);
                        meshPart.tangents.push_back(t[1]); meshPart.tangents.push_back(t[2]);
                        meshPart.tangents.push_back(1.0f);  // LH = -1, RH = +1
                    }
                }

                if (na && na->size() == vCount)
                {
                    meshPart.normals.resize(vCount * 3);
                    memcpy(&meshPart.normals[0], &(*na)[0], vCount * sizeof(float) * 3);
                }
                if (ca && ca->size() == vCount)
                {
                    meshPart.colors.resize(vCount * 4);
                    memcpy(&meshPart.colors[0], &(*ca)[0], vCount * sizeof(float) * 4);
                }
                if (uv && uv->size() == vCount)
                {
                    meshPart.uvs.resize(vCount * 2);
                    memcpy(&meshPart.uvs[0], &(*uv)[0], vCount * sizeof(float) * 2);
                }

                // Compute joint weights
                size_t numJointsToWeight = jData._weightList[0].size(), count = 0;
                if (numJointsToWeight != 4)
                {
                    OSG_WARN << "[PlayerAnimation] Unsupported joint-weight size: "
                             << numJointsToWeight << std::endl;
                    return;  // FIXME: a valid range besides [1, 4]?
                }

                for (size_t i = 0; i < wCount; ++i)
                {
                    std::vector<std::pair<osg::Transform*, float>>& jMap = jData._weightList[i];
                    for (std::vector<std::pair<osg::Transform*, float>>::iterator itr = jMap.begin();
                         itr != jMap.end(); ++itr, ++count)
                    {
                        meshPart.joint_indices.push_back(_nodeMap[itr->first]);
                        meshPart.joint_weights.push_back(itr->second);
                        if (count == numJointsToWeight - 1) break;
                    }

                    while (count < numJointsToWeight - 1)
                    {
                        meshPart.joint_indices.push_back(0); count++;
                        meshPart.joint_weights.push_back(0.0f);
                    }
                    count = 0;
                }

                // Apply to OZZ mesh
                osg::TriangleIndexFunctor<CollectTriangleOperator> functor;
                functor.startIndex = 0; geom.accept(functor);

                OzzMesh mesh; mesh.parts.push_back(meshPart);
                mesh.triangle_indices.assign(functor.triangles.begin(), functor.triangles.end());
                for (std::map<osg::Transform*, osg::Matrixf>::iterator itr = jData._invBindPoseMap.begin();
                     itr != jData._invBindPoseMap.end(); ++itr)
                { mesh.joint_remaps.push_back(_nodeMap[itr->first]); }

                std::sort(mesh.joint_remaps.begin(), mesh.joint_remaps.end());  // required by OZZ
                for (size_t j = 0; j < mesh.joint_remaps.size(); ++j)
                {
                    uint16_t id = mesh.joint_remaps[j];
                    std::map<osg::Transform*, osg::Matrixf>::iterator itr = jData._invBindPoseMap.begin();
                    for (; itr != jData._invBindPoseMap.end(); ++itr)
                    { if (_nodeMap[itr->first] == id) break; }

                    const osg::Matrixf& m = itr->second;
                    ozz::math::Float4x4 pose = ozz::math::Float4x4::identity();
                    pose.cols[0] = ozz::math::simd_float4::Load(m(0, 0), m(0, 1), m(0, 2), m(0, 3));
                    pose.cols[1] = ozz::math::simd_float4::Load(m(1, 0), m(1, 1), m(1, 2), m(1, 3));
                    pose.cols[2] = ozz::math::simd_float4::Load(m(2, 0), m(2, 1), m(2, 2), m(2, 3));
                    pose.cols[3] = ozz::math::simd_float4::Load(m(3, 0), m(3, 1), m(3, 2), m(3, 3));
                    mesh.inverse_bind_poses.push_back(pose);
                }
                _meshes.push_back(mesh);
                _stateSetList.push_back(jData._stateset);
            }

        protected:
            std::map<osg::Geometry*, PlayerAnimation::GeometryJointData> _jointMap;
            std::map<osg::Transform*, uint16_t> _nodeMap;
            std::vector<osg::ref_ptr<osg::StateSet>> _stateSetList;
            ozz::vector<OzzMesh> _meshes;
        };
    }
}

PlayerAnimation::PlayerAnimation()
{
    _internal = new OzzAnimation; _animated = true;
    _blendingThreshold = ozz::animation::BlendingJob().threshold;
}

bool PlayerAnimation::initialize(osg::Node& skeletonRoot, osg::Node& meshRoot,
                                 const std::map<osg::Geometry*, GeometryJointData>& jointDataMap)
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());

    // Load skeleton data from 'skeletonRoot'
    ozz::animation::CreateSkeletonVisitor csv;
    skeletonRoot.accept(csv); csv.build(ozz->_skeleton);

    // Load mesh data from 'meshRoot' and 'jointDataMap'
    ozz::animation::CreateMeshVisitor cmv(csv.getSkeletonNodes(), jointDataMap);
    meshRoot.accept(cmv); ozz->_meshes = cmv.getMeshes();
    _meshStateSetList = cmv.getStateSets();
    return initializeInternal();
}

bool PlayerAnimation::initialize(const std::vector<osg::Transform*>& nodes,
                                 const std::vector<osg::Geometry*>& meshList,
                                 const std::map<osg::Geometry*, GeometryJointData>& jointDataMap)
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    ozz::animation::CreateSkeletonVisitor csv;
    csv.initialize(nodes); csv.build(ozz->_skeleton);

    ozz::animation::CreateMeshVisitor cmv(csv.getSkeletonNodes(), jointDataMap);
    cmv.initialize(meshList); ozz->_meshes = cmv.getMeshes();
    _meshStateSetList = cmv.getStateSets();
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
