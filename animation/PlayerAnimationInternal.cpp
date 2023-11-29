#include "PlayerAnimation.h"
#include "PlayerAnimationInternal.h"
#include "BlendShapeAnimation.h"
#include <osg/io_utils>
#include <osg/Version>
#include <osg/Notify>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <osg/ShapeDrawable>
#include <osgUtil/SmoothingVisitor>
using namespace osgVerse;

bool OzzAnimation::loadSkeleton(const char* filename, ozz::animation::Skeleton* skeleton)
{
    ozz::io::File file(filename, "rb");
    if (!file.opened())
        ozz::log::Err() << "[PlayerAnimation] Failed to open skeleton file " << filename << std::endl;
    else
    {
        ozz::io::IArchive archive(&file);
        if (!archive.TestTag<ozz::animation::Skeleton>())
            ozz::log::Err() << "[PlayerAnimation] Failed to load skeleton instance from file "
                            << filename << std::endl;
        else { archive >> *skeleton; return true; }
    }
    return false;
}
    
bool OzzAnimation::loadAnimation(const char* filename, ozz::animation::Animation* anim)
{
    ozz::io::File file(filename, "rb");
    if (!file.opened())
        ozz::log::Err() << "[PlayerAnimation] Failed to open animation file " << filename << std::endl;
    else
    {
        ozz::io::IArchive archive(&file);
        if (!archive.TestTag<ozz::animation::Animation>())
            ozz::log::Err() << "[PlayerAnimation] Failed to load animation instance from file "
                            << filename << std::endl;
        else { archive >> *anim; return true; }
    }
    return false;
}

bool OzzAnimation::loadMesh(const char* filename, ozz::vector<ozz::sample::Mesh>* meshes)
{
    ozz::io::File file(filename, "rb");
    if (!file.opened())
    {
        ozz::log::Err() << "[PlayerAnimation] Failed to open mesh file " << filename << std::endl;
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

bool OzzAnimation::applyMesh(osg::Geometry& geom, const OzzMesh& mesh)
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

bool OzzAnimation::applySkinningMesh(osg::Geometry& geom, const OzzMesh& mesh)
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
            ozz::log::Err() << "[PlayerAnimation] Failed with skinning job" << std::endl;

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

void OzzAnimation::multiplySoATransformQuaternion(
        int index, const ozz::math::SimdQuaternion& quat,
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

        float timeRatio = (sampler.timeRatio < 0.0f) ? 1.0f : sampler.timeRatio;
        if (sampler.weight <= 0.0f) continue;

        ozz::animation::BlendingJob::Layer layer;
        layer.transform = ozz::make_span(sampler.locals);
        layer.weight = sampler.weight;
        if (!sampler.jointWeights.empty())
            layer.joint_weights = ozz::make_span(sampler.jointWeights);
        layers.push_back(layer);

        // Sample animation data to its local space
        ozz::animation::SamplingJob samplingJob;
        samplingJob.animation = &(sampler.animation);
        samplingJob.context = &(ozz->_context);
        samplingJob.ratio = osg::clampBetween(timeRatio, 0.0f, 1.0f);
        samplingJob.output = ozz::make_span(sampler.locals);
        if (!samplingJob.Run())
        {
            OSG_WARN << "[PlayerAnimation] sampling job failed." << std::endl;
            return false;
        }
    }

    // Set-up blending job
    ozz::animation::BlendingJob blendJob;
    blendJob.threshold = _blendingThreshold;
    blendJob.layers = ozz::make_span(layers);
    blendJob.rest_pose = ozz->_skeleton.joint_rest_poses();
    blendJob.output = ozz::make_span(ozz->_blended_locals);
    if (!blendJob.Run())
    {
        OSG_WARN << "[PlayerAnimation] blending job failed." << std::endl;
        return false;
    }

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
    size_t numMeshes = ozz->_meshes.size() + (_drawSkeleton ? 1 : 0);
    if (meshDataRoot.getNumDrawables() != numMeshes)
    {
        meshDataRoot.removeDrawables(0, meshDataRoot.getNumDrawables());
        for (size_t i = 0; i < numMeshes; ++i)
        {
            osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
            geom->setUseDisplayList(false);
            geom->setUseVertexBufferObjects(true);
            if (i < _blendshapes.size()) geom->addUpdateCallback(_blendshapes[i].get());
            if (i < _meshStateSetList.size()) geom->setStateSet(_meshStateSetList[i].get());
            else if (_drawSkeleton && i == numMeshes - 1)
                geom->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
            meshDataRoot.addDrawable(geom.get());
        }
    }

    for (size_t i = 0; i < ozz->_meshes.size(); ++i)
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
    if (_drawSkeleton)
        updateSkeletonMesh(*(meshDataRoot.getDrawable(numMeshes - 1)->asGeometry()));
    return true;
}

static bool applyTransform(osg::Transform& node, const ozz::math::Float4x4& m, const osg::Matrix& parentM)
{
    osg::Matrix matrix(
        ozz::math::GetX(m.cols[0]), ozz::math::GetY(m.cols[0]), ozz::math::GetZ(m.cols[0]), ozz::math::GetW(m.cols[0]),
        ozz::math::GetX(m.cols[1]), ozz::math::GetY(m.cols[1]), ozz::math::GetZ(m.cols[1]), ozz::math::GetW(m.cols[1]),
        ozz::math::GetX(m.cols[2]), ozz::math::GetY(m.cols[2]), ozz::math::GetZ(m.cols[2]), ozz::math::GetW(m.cols[2]),
        ozz::math::GetX(m.cols[3]), ozz::math::GetY(m.cols[3]), ozz::math::GetZ(m.cols[3]), ozz::math::GetW(m.cols[3]));
    matrix = matrix * osg::Matrix::inverse(parentM);
    if (!osg::equivalent(matrix(3, 3), 1.0)) return false;

    osg::MatrixTransform* mt = node.asMatrixTransform();
    if (mt) { mt->setMatrix(matrix); return true; }

    osg::PositionAttitudeTransform* pat = node.asPositionAttitudeTransform();
    if (pat)
    {
        osg::Vec3 pos, scale; osg::Quat rot, so; matrix.decompose(pos, rot, scale, so);
        pat->setPosition(pos); pat->setScale(scale); pat->setAttitude(rot); return true;
    }
    return false;
}

bool PlayerAnimation::applyTransforms(osg::Transform& skeletonRoot,
                                      bool createIfMissing, bool createWithShape)
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    ozz::span<const int16_t> parents = ozz->_skeleton.joint_parents();
    ozz::span<const char* const> joints = ozz->_skeleton.joint_names();
    const ozz::vector<ozz::math::Float4x4>& matrices = ozz->_models;
    if (parents.empty() || joints.size() != matrices.size()) return false;

    std::vector<osg::Group*> createdNodes; osg::ref_ptr<osg::Geode> geode;
    createdNodes.push_back(&skeletonRoot); skeletonRoot.setName(joints[0]);
    applyTransform(skeletonRoot, matrices[0], osg::Matrix());
    for (size_t i = 0; i < parents.size(); ++i)
    {
        std::string jointName = joints[i];
        int16_t idx = parents[i]; if (idx < 0) continue;
        if (createdNodes.size() <= idx) continue;  // FIXME: will this happen?

        osg::Group* parent = createdNodes[idx]; bool found = false;
        for (size_t j = 0; j < parent->getNumChildren(); ++j)
        {
            osg::Group* child = parent->getChild(j)->asGroup();
            if (child && child->getName() == jointName)
            { createdNodes.push_back(child); found = true; break; }
        }

        // Display a sphere of bone joint
        if (createIfMissing && createWithShape && !geode)
        {
            geode = new osg::Geode;
            geode->addDrawable(new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(), 0.01f)));
        }

        if (found)
        {
            osg::Transform* childT = createdNodes.back()->asTransform();
            applyTransform(*childT, matrices[i], parent->getWorldMatrices(&skeletonRoot)[0]);
        }
        else if (createIfMissing)
        {
            osg::MatrixTransform* newT = new osg::MatrixTransform;
            if (geode.valid()) newT->addChild(geode.get());
            newT->setName(jointName); parent->addChild(newT); createdNodes.push_back(newT);
            applyTransform(*newT, matrices[i], parent->getWorldMatrices(&skeletonRoot)[0]);
        }
        else
        {
            OSG_WARN << "[PlayerAnimation] Joint node not found: " << jointName
                     << ", while applying transforms to scene graph" << std::endl;
            return false;
        }
    }
    return true;
}

void PlayerAnimation::updateSkeletonMesh(osg::Geometry& geom)
{
    OzzAnimation* ozz = static_cast<OzzAnimation*>(_internal.get());
    ozz::span<const int16_t> parents = ozz->_skeleton.joint_parents();
    const ozz::vector<ozz::math::Float4x4>& matrices = ozz->_models;
    size_t vCount = parents.size();
    if (vCount < 1 || vCount != matrices.size()) return;

    osg::Vec3Array* va = static_cast<osg::Vec3Array*>(geom.getVertexArray());
    if (!va) { va = new osg::Vec3Array(vCount); geom.setVertexArray(va); }
    else if (va->size() != vCount) va->resize(vCount);

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
    else if (ca->size() != vCount) { ca->resize(vCount); ca->dirty(); }

    osg::DrawElementsUShort* de = (geom.getNumPrimitiveSets() == 0) ? NULL
                                : static_cast<osg::DrawElementsUShort*>(geom.getPrimitiveSet(0));
    if (!de) { de = new osg::DrawElementsUShort(GL_LINES, vCount * 2); geom.addPrimitiveSet(de); }
    else if (de->size() != vCount * 2) { de->resize(vCount * 2); de->dirty(); }

    for (size_t i = 0; i < parents.size(); ++i)
    {
        int16_t pID = parents[i]; if (pID < 0) pID = 0;  // make an invalid line
        const ozz::math::Float4x4& m = matrices[i];
        osg::Matrix matrix(
            ozz::math::GetX(m.cols[0]), ozz::math::GetY(m.cols[0]), ozz::math::GetZ(m.cols[0]), ozz::math::GetW(m.cols[0]),
            ozz::math::GetX(m.cols[1]), ozz::math::GetY(m.cols[1]), ozz::math::GetZ(m.cols[1]), ozz::math::GetW(m.cols[1]),
            ozz::math::GetX(m.cols[2]), ozz::math::GetY(m.cols[2]), ozz::math::GetZ(m.cols[2]), ozz::math::GetW(m.cols[2]),
            ozz::math::GetX(m.cols[3]), ozz::math::GetY(m.cols[3]), ozz::math::GetZ(m.cols[3]), ozz::math::GetW(m.cols[3]));
        (*ca)[i] = (i == 0) ? osg::Vec4ub(255, 0, 0, 255) : osg::Vec4ub(255, 255, 255, 255);
        (*va)[i] = matrix.getTrans(); (*de)[i * 2] = i; (*de)[i * 2 + 1] = pID;
    }
    va->dirty();
}

void PlayerAnimation::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
    osg::Geode* geode = node->asGeode();
    if (nv->getFrameStamp()) update(*nv->getFrameStamp(), !_animated);
    if (node->asGeode()) applyMeshes(*node->asGeode(), true);
    else OSG_WARN << "[PlayerAnimation] Callback should set to a geode to animate meshes" << std::endl;

    //node->getParent(0)->asTransform()->asMatrixTransform()->setMatrix(osg::Matrix());
    //applyTransforms(*(node->getParent(0)->asTransform()), true, true);
    traverse(node, nv);
}
