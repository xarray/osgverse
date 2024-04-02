#include "PlayerAnimation.h"
#include "PlayerAnimationInternal.h"
#include "BlendShapeAnimation.h"
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

                    matrices[soaIndex].makeIdentity();
                    t->computeLocalToWorldMatrix(matrices[soaIndex], NULL);
                    //matrices[soaIndex] = t->getWorldMatrices()[0];
                    if ((++soaIndex) == 4)
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
                for (int j = numSoa; j < 4; ++j) s[j] = osg::Vec3(1.0f, 1.0f, 1.0f);

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
            std::vector<osg::ref_ptr<osgVerse::BlendShapeAnimation>>& getBS() { return _blendshapes; }
            std::vector<osg::ref_ptr<osg::StateSet>>& getStateSets() { return _stateSetList; }

            virtual void apply(osg::Geode& node)
            {
#if OSG_VERSION_LESS_OR_EQUAL(3, 4, 1)
                for (size_t i = 0; i < node.getNumDrawables(); ++i)
                {
                    osg::Geometry* geom = node.getDrawable(i)->asGeometry();
                    if (geom && _jointMap.find(geom) != _jointMap.end())
                        apply(*geom, _jointMap.find(geom)->second);
                }
#endif
                traverse(node);
            }

            virtual void apply(osg::Geometry& geom)
            {
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
                if (_jointMap.find(&geom) != _jointMap.end())
                    apply(geom, _jointMap.find(&geom)->second);
                traverse(geom);
#endif
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
                osg::Vec4Array* ta = dynamic_cast<osg::Vec4Array*>(geom.getVertexAttribArray(6));
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
                    meshPart.tangents.resize(vCount * 4);
                    memcpy(&meshPart.tangents[0], &(*ta)[0], vCount * sizeof(float) * 4);
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
                    std::vector<float> weightValues; float weightSum = 0.0f;
                    std::vector<std::pair<osg::Transform*, float>>& jMap = jData._weightList[i];
                    for (std::vector<std::pair<osg::Transform*, float>>::iterator itr = jMap.begin();
                         itr != jMap.end(); ++itr, ++count)
                    {
                        if (_nodeMap.find(itr->first) == _nodeMap.end())
                        {
                            OSG_WARN << "[PlayerAnimation] Invalid joint linked to mesh: "
                                     << (itr->first ? itr->first->getName() : std::string("NULL"))
                                     << std::endl;
                        }

                        meshPart.joint_indices.push_back(_nodeMap[itr->first]);
                        weightValues.push_back(itr->second); weightSum += itr->second;
                        if (count == numJointsToWeight - 1) break;
                    }

                    while (count < numJointsToWeight - 1)
                    {
                        meshPart.joint_indices.push_back(0);
                        weightValues.push_back(0.0f); count++;
                    }

                    // Recompute and remove last weight as computed at runtime
                    for (size_t j = 0; j < weightValues.size() - 1; ++j)
                        meshPart.joint_weights.push_back(weightValues[j] / weightSum);
                    count = 0;
                }

                // Handle blendshapes
                BlendShapeAnimation* bsa =
                    dynamic_cast<osgVerse::BlendShapeAnimation*>(geom.getUpdateCallback());
                _blendshapes.push_back(bsa != NULL ? bsa : NULL);

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
            std::vector<osg::ref_ptr<osgVerse::BlendShapeAnimation>> _blendshapes;
        };

        struct AnimationConverter
        {
            typedef std::map<osg::Transform*, PlayerAnimation::AnimationData> AnimationMap;
            void build(OzzAnimation::AnimationSampler& sampler,
                       std::vector<osg::Transform*> nodes, const AnimationMap& dataMap)
            {
                ozz::animation::Animation& anim = sampler.animation;
                anim.Deallocate(); anim.num_tracks_ = (int)nodes.size();

                // Get max/min time range
                float minTime = 9999.0f, maxTime = 0.0f;
                std::map<osg::Transform*, std::pair<int, int>> trackMap;  // node: index, has tracks
                for (size_t i = 0; i < nodes.size(); ++i)
                    trackMap[nodes[i]] = std::pair<int, int>(i, 0);

                for (AnimationMap::const_iterator itr = dataMap.begin();
                     itr != dataMap.end(); ++itr)
                {
                    const PlayerAnimation::AnimationData& ad = itr->second;
                    if (!ad._positionFrames.empty())
                    {
                        minTime = osg::minimum(ad._positionFrames.front().first, minTime);
                        maxTime = osg::maximum(ad._positionFrames.back().first, maxTime);
                    }
                    if (!ad._rotationFrames.empty())
                    {
                        minTime = osg::minimum(ad._rotationFrames.front().first, minTime);
                        maxTime = osg::maximum(ad._rotationFrames.back().first, maxTime);
                    }
                    if (!ad._scaleFrames.empty())
                    {
                        minTime = osg::minimum(ad._scaleFrames.front().first, minTime);
                        maxTime = osg::maximum(ad._scaleFrames.back().first, maxTime);
                    }
                }
                if (osg::equivalent(maxTime, 0.0f)) return;

                // Get and use every track's default pose as keyframe data
                std::map<osg::Transform*, std::vector<std::pair<float, osg::Vec3>>> defPosTracks, defScaleTracks;
                std::map<osg::Transform*, std::vector<std::pair<float, osg::Vec4>>> defRotTracks;
                std::map<int, osg::Transform*> candicates0;
                for (size_t i = 0; i < nodes.size(); ++i)
                {
                    osg::Transform* t = nodes[i]; candicates0[i] = t;
                    osg::Matrix matrix; osg::Vec3 pos, scale; osg::Quat rot, so;
                    t->computeLocalToWorldMatrix(matrix, NULL);

                    matrix.decompose(pos, rot, scale, so);
                    defPosTracks[t].push_back(std::pair<float, osg::Vec3>(0.0f, pos));
                    defRotTracks[t].push_back(std::pair<float, osg::Vec4>(0.0f, rot.asVec4()));
                    defScaleTracks[t].push_back(std::pair<float, osg::Vec3>(0.0f, scale));
                }

                // Record T/R/S keyframes
                float invD = 1.0f / maxTime; osg::Vec3 defScale(1.0f, 1.0f, 1.0f);
                std::vector<Float3Key> positions, scales; std::vector<QuaternionKey> rotations;
                for (AnimationMap::const_iterator itr = dataMap.begin(); itr != dataMap.end(); ++itr)
                {
                    const PlayerAnimation::AnimationData& ad = itr->second;
                    std::map<std::string, std::string> ipMap = ad._interpolations;
                    std::pair<int, int>& trackData = trackMap[itr->first]; trackData.second += 1;

                    int track = trackData.first; osg::Transform* tr = candicates0[track];
                    sampleData(positions, ipMap["translation"], 0, track, ad._positionFrames, invD,
                               true, defPosTracks[tr].front().second);
                    sampleData(rotations, ipMap["rotation"], 1, track, ad._rotationFrames, invD,
                               true, defRotTracks[tr].front().second);
                    sampleData(scales, ipMap["scale"], 2, track, ad._scaleFrames, invD,
                               true, defScaleTracks[tr].front().second);
                }

                // Fill empty tracks of each frame
                sortData(positions, rotations, scales);
                fillTracksPerFrame(positions, defPosTracks, candicates0);
                fillTracksPerFrame(rotations, defRotTracks, candicates0);
                fillTracksPerFrame(scales, defScaleTracks, candicates0);

                // Record extra tracks for SoaTransform use
                int numSoaTracks = ozz::Align(anim.num_tracks_, 4);
                osg::Transform* tr0 = nodes[0];  // clear default to generate empty keyframes for extra
                defPosTracks[tr0].clear(); defRotTracks[tr0].clear(); defScaleTracks[tr0].clear();
                for (int j = anim.num_tracks_; j < numSoaTracks; ++j)
                {
                    sampleData(positions, "LINEAR", 0, j, defPosTracks[tr0], 1.0f);
                    sampleData(rotations, "LINEAR", 0, j, defRotTracks[tr0], 1.0f);
                    sampleData(scales, "LINEAR", 0, j, defScaleTracks[tr0], 1.0f, true, defScale);
                }

                // Allocate animation buffer
                int tCount = (int)positions.size(), rCount = (int)rotations.size(),
                    sCount = (int)scales.size();
                size_t bufferSize = tCount * sizeof(Float3Key) + rCount * sizeof(QuaternionKey) +
                                    sCount * sizeof(Float3Key);
                span<byte> buffer = { static_cast<byte*>(memory::default_allocator()->Allocate
                                      (bufferSize, alignof(Float3Key))), bufferSize };
                anim.translations_ = ozz::fill_span<Float3Key>(buffer, tCount);
                anim.rotations_ = ozz::fill_span<QuaternionKey>(buffer, rCount);
                anim.scales_ = ozz::fill_span<Float3Key>(buffer, sCount);
                anim.duration_ = maxTime;

                // Sort by ratio/track and fill the animation data
                sortData(positions, rotations, scales);
                if (tCount > 0) memcpy(anim.translations_.data(), &positions[0], tCount * sizeof(Float3Key));
                if (rCount > 0) memcpy(anim.rotations_.data(), &rotations[0], rCount * sizeof(QuaternionKey));
                if (sCount > 0) memcpy(anim.scales_.data(), &scales[0], sCount * sizeof(Float3Key));
            }

            template<typename T1, typename T2>
            void fillTracksPerFrame(T1& inputs, T2& defTrackValues,
                                    const std::map<int, osg::Transform*>& candicates0)
            {
                std::map<int, osg::Transform*> c1;
                float lastR = -1.0f; size_t count = inputs.size();
                for (size_t i = 0; i < count; ++i)
                {
                    float r = inputs[i].ratio; int track = inputs[i].track;
                    if (r != lastR)
                    {
                        for (std::map<int, osg::Transform*>::iterator it = c1.begin(); it != c1.end(); ++it)
                        {
                            osg::Transform* tr = it->second; defTrackValues[tr].front().first = lastR;
                            sampleData(inputs, "LINEAR", 0, it->first, defTrackValues[tr], 1.0f, false);
                        }
                        c1 = candicates0; lastR = r;
                    }

                    std::map<int, osg::Transform*>::iterator itr = c1.find(track);
                    if (itr != c1.end()) c1.erase(itr);
                }

                for (std::map<int, osg::Transform*>::iterator it = c1.begin(); it != c1.end(); ++it)
                {
                    osg::Transform* tr = it->second; defTrackValues[tr].front().first = lastR;
                    sampleData(inputs, "LINEAR", 0, it->first, defTrackValues[tr], 1.0f, false);
                }
            }

            void sortData(std::vector<Float3Key>& positions, std::vector<QuaternionKey>& rotations,
                          std::vector<Float3Key>& scales)
            {
                std::sort(positions.begin(), positions.end(), [](Float3Key& l, Float3Key& r)
                { if (osg::equivalent(l.ratio, r.ratio)) return l.track < r.track; return l.ratio < r.ratio; });
                std::sort(rotations.begin(), rotations.end(), [](QuaternionKey& l, QuaternionKey& r)
                { if (osg::equivalent(l.ratio, r.ratio)) return l.track < r.track; return l.ratio < r.ratio; });
                std::sort(scales.begin(), scales.end(), [](Float3Key& l, Float3Key& r)
                { if (osg::equivalent(l.ratio, r.ratio)) return l.track < r.track; return l.ratio < r.ratio; });
            }

            void sampleData(std::vector<Float3Key>& values, const std::string& interpo, int type,
                            int track, const std::vector<std::pair<float, osg::Vec3>>& frames, float invD,
                            bool autoFill01 = true, const osg::Vec3& def01 = osg::Vec3())
            {
                if (frames.empty() && autoFill01)
                {
                    Float3Key k0; k0.ratio = 0.0f; k0.track = track;
                    Float3Key k1; k1.ratio = 1.0f; k1.track = track;
                    for (int k = 0; k < 3; ++k)
                    {
                        k0.value[k] = ozz::math::FloatToHalf(def01[k]);
                        k1.value[k] = ozz::math::FloatToHalf(def01[k]);
                    }
                    values.push_back(k0); values.push_back(k1); return;
                }

                // TODO: interpo = LINEAR / STEP / CUBICSPLINE
                for (size_t i = 0; i < frames.size(); ++i)
                {
                    float t = frames[i].first * invD;
                    const osg::Vec3& v = frames[i].second;
                    Float3Key key; key.ratio = t; key.track = track;
                    key.value[0] = ozz::math::FloatToHalf(v[0]);
                    key.value[1] = ozz::math::FloatToHalf(v[1]);
                    key.value[2] = ozz::math::FloatToHalf(v[2]);

                    if (i == 0 && t > 0.0f && autoFill01)
                    {
                        Float3Key key0 = key; key0.ratio = 0.0f;
                        values.push_back(key0);
                    }
                    values.push_back(key);
                }

                if (values.back().ratio < 1.0f && autoFill01)
                {
                    Float3Key key1 = values.back(); key1.ratio = 1.0f;
                    values.push_back(key1);
                }
            }

            void sampleData(std::vector<QuaternionKey>& values, const std::string& interpo, int type,
                            int track, const std::vector<std::pair<float, osg::Vec4>>& frames, float invD,
                            bool autoFill01 = true, const osg::Vec4& def01 = osg::Vec4())
            {
                if (frames.empty() && autoFill01)
                {
                    QuaternionKey k0; k0.ratio = 0.0f; k0.track = track;
                    QuaternionKey k1; k1.ratio = 1.0f; k1.track = track;
                    compressQuat(def01, &k0); compressQuat(def01, &k1);
                    values.push_back(k0); values.push_back(k1); return;
                }

                // TODO: interpo = LINEAR / STEP / CUBICSPLINE
                for (size_t i = 0; i < frames.size(); ++i)
                {
                    float t = frames[i].first * invD;
                    QuaternionKey key; key.ratio = t; key.track = track;
                    compressQuat(frames[i].second, &key);

                    if (i == 0 && t > 0.0f && autoFill01)
                    {
                        QuaternionKey key0 = key; key0.ratio = 0.0f;
                        values.push_back(key0);
                    }
                    values.push_back(key);
                }

                if (values.back().ratio < 1.0f && autoFill01)
                {
                    QuaternionKey key1 = values.back(); key1.ratio = 1.0f;
                    values.push_back(key1);
                }
            }

            static void compressQuat(const osg::Vec4& src, QuaternionKey* dest)
            {
                // Finds the largest quaternion component.
                const float quat[4] = { src.x(), src.y(), src.z(), src.w() };
                const long long largest = std::max_element(quat, quat + 4, lessAbs) - quat;
                dest->largest = largest & 0x3; dest->sign = quat[largest] < 0.f;

                // Quantize the 3 smallest components on 16 bits signed integers.
                const float kFloat2Int = 32767.f * ozz::math::kSqrt2;
                const int kMapping[4][3] = { {1, 2, 3}, {0, 2, 3}, {0, 1, 3}, {0, 1, 2} };
                const int* map = kMapping[largest];
                const int a = static_cast<int>(floor(quat[map[0]] * kFloat2Int + .5f));
                const int b = static_cast<int>(floor(quat[map[1]] * kFloat2Int + .5f));
                const int c = static_cast<int>(floor(quat[map[2]] * kFloat2Int + .5f));
                dest->value[0] = ozz::math::Clamp(-32767, a, 32767) & 0xffff;
                dest->value[1] = ozz::math::Clamp(-32767, b, 32767) & 0xffff;
                dest->value[2] = ozz::math::Clamp(-32767, c, 32767) & 0xffff;
            }

            static bool lessAbs(float _left, float _right)
            { return std::abs(_left) < std::abs(_right); }
        };
    }
}

static void printPlayerData(OzzAnimation* ozz)
{
    ozz::span<const char* const> names = ozz->_skeleton.joint_names();
    ozz::span<const int16_t> parents = ozz->_skeleton.joint_parents();
    ozz::span<const ozz::math::SoaTransform> restPoses = ozz->_skeleton.joint_rest_poses();
    for (size_t i = 0; i < names.size(); ++i)
    {
        int16_t pid = parents[i], soaID = i % 4;
        const ozz::math::SoaTransform soaT = restPoses[i / 4];
        std::cout << "B" << i << ": " << names[i] << ", Parent = "
                  << (pid >= 0 ? names[pid] : "(null)") << std::endl;

        osg::Vec3 pos, scale;
        switch (soaID)
        {
        case 0:
            scale[0] = ozz::math::GetX(soaT.scale.x); scale[1] = ozz::math::GetX(soaT.scale.y);
            scale[2] = ozz::math::GetX(soaT.scale.z);
            pos[0] = ozz::math::GetX(soaT.translation.x); pos[1] = ozz::math::GetX(soaT.translation.y);
            pos[2] = ozz::math::GetX(soaT.translation.z); break;
        case 1:
            scale[0] = ozz::math::GetY(soaT.scale.x); scale[1] = ozz::math::GetY(soaT.scale.y);
            scale[2] = ozz::math::GetY(soaT.scale.z);
            pos[0] = ozz::math::GetY(soaT.translation.x); pos[1] = ozz::math::GetY(soaT.translation.y);
            pos[2] = ozz::math::GetY(soaT.translation.z); break;
        case 2:
            scale[0] = ozz::math::GetZ(soaT.scale.x); scale[1] = ozz::math::GetZ(soaT.scale.y);
            scale[2] = ozz::math::GetZ(soaT.scale.z);
            pos[0] = ozz::math::GetZ(soaT.translation.x); pos[1] = ozz::math::GetZ(soaT.translation.y);
            pos[2] = ozz::math::GetZ(soaT.translation.z); break;
        case 3:
            scale[0] = ozz::math::GetW(soaT.scale.x); scale[1] = ozz::math::GetW(soaT.scale.y);
            scale[2] = ozz::math::GetW(soaT.scale.z);
            pos[0] = ozz::math::GetW(soaT.translation.x); pos[1] = ozz::math::GetW(soaT.translation.y);
            pos[2] = ozz::math::GetW(soaT.translation.z); break;
        }
        std::cout << "  Position = " << pos << ", Scale = " << scale << std::endl;
    }
    std::cout << std::endl;

    for (size_t i = 0; i < ozz->_meshes.size(); ++i)
    {
        OzzMesh& mesh = ozz->_meshes[i];
        std::cout << "Mesh-" << i << ": Parts = " << mesh.parts.size() << std::endl;
        for (size_t j = 0; j < mesh.parts.size(); ++j)
        {
            OzzMesh::Part& part = mesh.parts[j];
            int numVertices = part.vertex_count(), numInfluences = part.influences_count();
            ozz::vector<uint16_t> jointIds = part.joint_indices;
            ozz::vector<float> jointWeights = part.joint_weights;
            std::cout << "  Part-" << j << ": Vertices = " << numVertices << ", Influences = "
                      << numInfluences << ", JointIDs = " << jointIds.size()
                      << ", Weights = " << jointWeights.size() << std::endl;

            for (size_t n = 0; n < numVertices; ++n)
            {
                size_t n0 = n * 4, n1 = n * 3;
                float w = jointWeights[n1] + jointWeights[n1 + 1] + jointWeights[n1 + 2];
                std::cout << "    Vec-" << n << ": B" << jointIds[n0] << " / " << jointWeights[n1]
                          << ", B" << jointIds[n0 + 1] << " / " << jointWeights[n1 + 1]
                          << ", B" << jointIds[n0 + 2] << " / " << jointWeights[n1 + 2]
                          << ", B" << jointIds[n0 + 3] << " / " << (1.0f - w) << std::endl;
            }
        }
        std::cout << std::endl;
    }
}

static void printAnimationData(OzzAnimation* ozz, const std::string& key)
{
    ozz::span<const char* const> names = ozz->_skeleton.joint_names();
    OzzAnimation::AnimationSampler& sampler = ozz->_animations[key];
    ozz::animation::Animation& animation = sampler.animation;

    ozz::span<const ozz::animation::Float3Key> posList = sampler.animation.translations();
    ozz::span<const ozz::animation::QuaternionKey> rotList = sampler.animation.rotations();
    ozz::span<const ozz::animation::Float3Key> scaleList = sampler.animation.scales();
    std::cout << "Anim " << key << ": Duration = " << sampler.animation.duration() << ", T/R/S = "
              << posList.size() << "/" << rotList.size() << "/" << scaleList.size() << std::endl;

    for (size_t i = 0; i < posList.size(); ++i)
    {
        const ozz::animation::Float3Key& value = posList[i];
        std::cout << "  T-" << i << " (" << value.ratio << "): Track = " << value.track << ", Vec = "
                  << ozz::math::HalfToFloat(value.value[0]) << ", " << ozz::math::HalfToFloat(value.value[1])
                  << ", " << ozz::math::HalfToFloat(value.value[2]) << std::endl;
    }
}

PlayerAnimation::PlayerAnimation()
{
    _internal = new OzzAnimation; _animated = true; _drawSkeleton = true; _restPose = false;
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
    _meshStateSetList = cmv.getStateSets(); _blendshapes = cmv.getBS();
#if 0
    printPlayerData(ozz);
#endif
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
    _meshStateSetList = cmv.getStateSets(); _blendshapes = cmv.getBS();
#if 1
    printPlayerData(ozz);
#endif
    return initializeInternal();
}

bool PlayerAnimation::initialize(const std::string& skeleton, const std::string& mesh)
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    if (!ozz->loadSkeleton(skeleton.c_str(), &(ozz->_skeleton))) return false;
    if (!ozz->loadMesh(mesh.c_str(), &(ozz->_meshes))) return false;
#if 0
    printPlayerData(ozz);
#endif
    return initializeInternal();
}

bool PlayerAnimation::loadAnimation(const std::string& key, const std::string& animation)
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    OzzAnimation::AnimationSampler& sampler = ozz->_animations[key];
    if (!ozz->loadAnimation(animation.c_str(), &(sampler.animation))) return false;
    return loadAnimationInternal(key);
}

bool PlayerAnimation::loadAnimation(const std::string& key, const std::vector<osg::Transform*>& nodes,
                                    const std::map<osg::Transform*, AnimationData>& animDataMap)
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    OzzAnimation::AnimationSampler& sampler = ozz->_animations[key];

    ozz::animation::AnimationConverter ac;
    ac.build(sampler, nodes, animDataMap); sampler.looping = true;
    return loadAnimationInternal(key);
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

std::vector<std::string> PlayerAnimation::getAnimationNames() const
{
    std::vector<std::string> names;
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    std::map<std::string, OzzAnimation::AnimationSampler>::iterator itr;
    for (itr = ozz->_animations.begin(); itr != ozz->_animations.end(); ++itr) names.push_back(itr->first);
    return names;
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

std::vector<std::string> PlayerAnimation::getBlendshapeNames() const
{
    std::vector<std::string> names;
    for (size_t i = 0; i < _blendshapes.size(); ++i)
    {
        osgVerse::BlendShapeAnimation* bs = _blendshapes[i].get();
        if (!bs) continue;  // It is common to have an empty BS callback...
        for (size_t j = 0; j < bs->getNumBlendShapes(); ++j)
            names.push_back(bs->getBlendShapeData(j)->name);
    }
    return names;
}

void PlayerAnimation::setBlendShape(const std::string& key, float weight)
{
    for (size_t i = 0; i < _blendshapes.size(); ++i)
    {
        BlendShapeAnimation::BlendShapeData* bsd = (!_blendshapes[i]) ? NULL
                                                 : _blendshapes[i]->getBlendShapeData(key);
        if (bsd != NULL) bsd->weight = weight;
    }
}

void PlayerAnimation::clearAllBlendShapes()
{
    for (size_t i = 0; i < _blendshapes.size(); ++i)
    {
        if (!_blendshapes[i]) continue;
        std::vector<osg::ref_ptr<BlendShapeAnimation::BlendShapeData>>& bsList =
            _blendshapes[i]->getAllBlendShapes();
        for (size_t j = 0; j < bsList.size(); ++j) bsList[j]->weight = 0.0f;
    }
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

bool PlayerAnimation::loadAnimationInternal(const std::string& key)
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    OzzAnimation::AnimationSampler& sampler = ozz->_animations[key];
    if (sampler.animation.translations().empty() && sampler.animation.rotations().empty() &&
        sampler.animation.scales().empty())
    {
        OSG_WARN << "[PlayerAnimation] Invalid animation data: " << key << std::endl;
        ozz->_animations.erase(ozz->_animations.find(key)); return false;
    }

    const int num_joints = ozz->_skeleton.num_joints();
    if (num_joints != sampler.animation.num_tracks())
    {
        OSG_WARN << "[PlayerAnimation] The animation " << key << " failed to match skeleton. "
                 << "Joint count (" << num_joints << ") doesn't equal to animation tracks ("
                 << sampler.animation.num_tracks() << ")" << std::endl;
        ozz->_animations.erase(ozz->_animations.find(key)); return false;
    }

    sampler.locals.resize(ozz->_skeleton.num_soa_joints());
    ozz->_context.Resize(num_joints);
    if (ozz->_animations.size() > 1) sampler.weight = 0.0f;
    else sampler.weight = 1.0f;  // by default only the first animation is full weighted

#if 0
    printAnimationData(ozz, key);
#endif
    return true;
}
