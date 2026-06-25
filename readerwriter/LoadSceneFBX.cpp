#include <osg/io_utils>
#include <osg/Version>
#include <osg/ValueObject>
#include <osg/AnimationPath>
#include <osg/Texture2D>
#include <osg/Material>
#include <osg/Geometry>
#include <osgDB/ConvertUTF>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgUtil/SmoothingVisitor>
#include <iostream>
#include <sstream>

#include "pipeline/Utilities.h"
#include "readerwriter/Utilities.h"
#include "animation/TweenAnimation.h"
#include "animation/BlendShapeAnimation.h"
#include "LoadSceneFBX.h"

#define DISABLE_SKINNING_DATA 0

static osg::Image* createDefaultImageForColor(const osg::Vec4& color)
{
    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->allocateImage(1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    image->setInternalTextureFormat(GL_RGBA8);

    osg::Vec4ub* ptr = (osg::Vec4ub*)image->data();
    *ptr = osg::Vec4ub(color[0] * 255, color[1] * 255, color[2] * 255, color[3] * 255);
    return image.release();
}

namespace osgVerse
{
    LoaderFBX::LoaderFBX(std::istream& in, const std::string& d, int pbr)
        : _scene(NULL), _workingDir(d + "/"), _usingMaterialPBR(pbr)
    {
        std::istreambuf_iterator<char> eos;
        std::vector<char> data(std::istreambuf_iterator<char>(in), eos);
        if (data.empty()) { OSG_WARN << "[LoaderFBX] Unable to read from stream\n"; return; }

        ufbx_error error; ufbx_load_opts opts = {};
        opts.load_external_files = true;
        opts.ignore_missing_external_files = true;
        opts.generate_missing_normals = true;
        opts.pivot_handling = UFBX_PIVOT_HANDLING_RETAIN;
        opts.target_axes.right = UFBX_COORDINATE_AXIS_POSITIVE_X;
        opts.target_axes.up = UFBX_COORDINATE_AXIS_POSITIVE_Z;
        opts.target_axes.front = UFBX_COORDINATE_AXIS_NEGATIVE_Y;
        //opts.space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS;
        opts.target_unit_meters = 1.0f;
        _scene = ufbx_load_memory((void*)&data[0], data.size(), &opts, &error);
        if (error.type != UFBX_ERROR_NONE)
        {
            std::string desc(error.description.data, error.description.length);
            OSG_NOTICE << "[LoaderFBX] Error " << error.type << ": " << desc << "\n";
        }
        if (!_scene)
            { OSG_WARN << "[LoaderFBX] Unable to parse FBX scene\n"; return; }
#if false
        OSG_NOTICE << "\n============ AnimLayers:  " << _scene->anim_layers.count << "\n";
        OSG_NOTICE << "============ Characters:  " << _scene->characters.count << "\n";
        OSG_NOTICE << "============ Bones:       " << _scene->bones.count << "\n";
        OSG_NOTICE << "============ BlendShapes: " << _scene->blend_shapes.count << "\n";
        OSG_NOTICE << "============ TexFiles:    " << _scene->texture_files.count << "\n";
        OSG_NOTICE << "============ Cameras:     " << _scene->cameras.count << "\n";
        OSG_NOTICE << "============ Lights:      " << _scene->lights.count << "\n";
#endif

        _root = new osg::MatrixTransform;
        createNode(NULL, _root.get(), _scene->root_node);
#if !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE) && !defined(OSG_GL3_AVAILABLE)
        _root->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
#endif

        // Handle character skeletons
        osg::Matrix invRootMatrix = osg::Matrix::inverse(_root->getMatrix());
        std::map<osg::Transform*, osg::observer_ptr<PlayerAnimation>> boneToAnimationMap;
        std::map<PlayerAnimation*, std::vector<osg::Transform*>> animatorBoneListMap;
        for (std::map<osg::Geode*, SkinningData>::iterator it = _skinningDataMap.begin();
             it != _skinningDataMap.end(); ++it)
        {
            SkinningData& sd = it->second; ufbx_skin_deformer* skin = sd.deformer;
            std::map<osg::Geometry*, PlayerAnimation::GeometryJointData> jointDataMap;
            
            // Traverse all meshes of current player
            std::set<osg::Transform*> nodeUsed; std::vector<osg::Geometry*> meshList;
            for (std::map<osg::Geometry*, std::vector<SkinningData::JointWeights>>::iterator
                 it2 = sd.skinningDataList.begin(); it2 != sd.skinningDataList.end(); ++it2)
            {
                PlayerAnimation::GeometryJointData& jointData = jointDataMap[it2->first];
                std::map<osg::Transform*, osg::Matrixf>& invPoses = jointData._invBindPoseMap;
                jointData._stateset = it2->first->getStateSet(); meshList.push_back(it2->first);

                // Compute skinning weights
                std::vector<SkinningData::JointWeights>& jWeights = it2->second;
                jointData._weightList.resize(jWeights.size());
                for (size_t i = 0; i < jWeights.size(); ++i)
                {
                    SkinningData::JointWeights& jw0 = jWeights[i];
                    PlayerAnimation::GeometryJointData::JointWeights& jw1 = jointData._weightList[i];
                    for (size_t j = 0; j < jw0.size(); ++j)
                    {
                        unsigned int clusterId = jw0[j].first; float w = jw0[j].second;
                        ufbx_skin_cluster* cluster = skin->clusters.data[clusterId];
                        ufbx_node* boneNode = cluster ? cluster->bone_node : NULL;
                        if (!boneNode)
                        {
                            OSG_NOTICE << "[LoaderFBX] Failed to get bone from cluster ID " << clusterId << " of "
                                       << "skeletal character " << it->first->getName() << "\n"; continue;
                        }

                        unsigned int boneId = boneNode->typed_id;
                        if (_boneToNodeMap.find(boneId) == _boneToNodeMap.end())
                        {
                            ufbx_bone* bone = _scene->bones[boneId]; std::string bName(bone->name.data, bone->name.length);
                            OSG_NOTICE << "[LoaderFBX] Failed to match bone " << bName << " (ID " << boneId << ") to any node\n";
                        }
                        else
                        {
                            osg::Transform* t = _boneToNodeMap[boneId].get(); nodeUsed.insert(t);
                            jw1.push_back(std::pair<osg::Transform*, float>(t, w));
                            invPoses[t] = osg::Matrix::inverse(toMatrix(cluster->bind_to_world));
                        }
                    }
                }  // for (size_t i = 0; i < jWeights.size(); ++i)
            }

            // Find all bone transforms of current player
            std::vector<osg::Transform*> nodeList;
            for (size_t k = 0; k < _scene->nodes.count; ++k)
            {
                unsigned int boneId = _scene->nodes[k]->typed_id;
                if (_boneToNodeMap.find(boneId) == _boneToNodeMap.end()) continue;

                osg::Transform* t = _boneToNodeMap[boneId].get();
                if (nodeUsed.find(t) != nodeUsed.end()) nodeList.push_back(t);
            }
            
            // Create animation callback of current player
            if (!nodeList.empty() && !meshList.empty())
            {
#if !DISABLE_SKINNING_DATA
                osg::ref_ptr<PlayerAnimation> player = new PlayerAnimation;
                player->setName(it->first->getName()); player->setModelRoot(_root.get());
                player->initialize(nodeList, meshList, jointDataMap);
                for (size_t k = 0; k < nodeList.size(); ++k) boneToAnimationMap[nodeList[k]] = player.get();
                animatorBoneListMap[player.get()] = nodeList;

                osg::ref_ptr<osg::Geode> meshRoot = new osg::Geode;
                meshRoot->setName("CharacterGeode_" + it->first->getName());
                meshRoot->addUpdateCallback(player.get());

                osg::ref_ptr<osg::MatrixTransform> meshRootMT = new osg::MatrixTransform;
                meshRootMT->setMatrix(invRootMatrix);  // will not be affected by root
                meshRootMT->addChild(meshRoot.get()); _root->addChild(meshRootMT.get());
                it->first->setNodeMask(0);  // FIXME: ugly to hide original meshes
                it->first->setUserValue("OriginalPlayerMesh", true);
#endif
            }

#if false
            std::cout << "Skeletal character " << it->first->getName() << ":\n";
            for (std::map<osg::Geometry*, PlayerAnimation::GeometryJointData>::iterator it2 = jointDataMap.begin();
                 it2 != jointDataMap.end(); ++it2)
            {
                PlayerAnimation::GeometryJointData& gjd = it2->second;
                std::cout << "  Mesh: " << it2->first->getName() << ": V = " << gjd._weightList.size() << "\n";
                for (size_t i = 0; i < gjd._weightList.size(); ++i)
                {
                    std::vector<std::pair<osg::Transform*, float>>& weights = gjd._weightList[i];
                    std::stringstream ss; float total = 0.0f; bool invalid = false;
                    for (size_t j = 0; j < weights.size(); ++j)
                    {
                        std::pair<osg::Transform*, float>& w = weights[j]; total += w.second; if (!w.first) invalid = true;
                        ss << (w.first ? w.first->getName() : "NULL") << "/" << w.second << "; ";
                    }
                    ss << "TOTAL = " << total << "; NUM = " << weights.size(); if (total < 0.99f) invalid = true;

                    if (invalid) std::cout << " (?)" << i << ": " << ss.str() << " (INVALID BONE WEIGHTS)\n";
                    else if (i < 10) std::cout << "    " << i << ": " << ss.str() << (i == 9 ? "\n    ...\n" : "\n");
                }
            }
#endif
        }

        // Handle animations
        for (size_t i = 0; i < _scene->anim_stacks.count; ++i)
        {
            ufbx_anim_stack* st = _scene->anim_stacks[i]; std::string name(st->name.data, st->name.length);
            std::map<osg::Transform*, PlayerAnimation::AnimationData> boneAnimMap;
            for (size_t j = 0; j < st->layers.count; ++j)
            {
                if (j > 0) { OSG_NOTICE << "[LoaderFBX] Unsupported animation layer "<< j << " of " << name << "\n"; }  // TODO
                else createAnimation(st->layers[j], name, st->time_begin, st->time_end, boneAnimMap);
            }

            if (!boneToAnimationMap.empty()&& !boneAnimMap.empty())
            {
                osg::observer_ptr<PlayerAnimation> player;
                if (boneToAnimationMap.size() > 1)
                {
                    std::map<osg::Transform*, PlayerAnimation::AnimationData>::iterator it = boneAnimMap.begin();
                    for (; it != boneAnimMap.end(); ++it)
                    { if (boneToAnimationMap.find(it->first) != boneToAnimationMap.end()) player = boneToAnimationMap[it->first]; }
                }
                else
                    player = boneToAnimationMap.begin()->second.get();
                
                // Add animation to player
                if (player.valid() && animatorBoneListMap.find(player.get()) != animatorBoneListMap.end())
                    player->loadAnimation(name, animatorBoneListMap[player.get()], boneAnimMap);
            }
        }
    }

    LoaderFBX::~LoaderFBX()
    {
        if (_scene != NULL) ufbx_free_scene(_scene);
    }

    void LoaderFBX::createNode(osg::Group* parent, osg::MatrixTransform* node, ufbx_node* srcNode)
    {
        if (_nodes.find(srcNode->typed_id) == _nodes.end())
        {
            if (srcNode->mesh)
                node->addChild(createMesh(toMatrix(srcNode->geometry_to_node), srcNode->mesh));
            node->setMatrix(toMatrix(srcNode->node_to_parent));
            node->setName(std::string(srcNode->name.data, srcNode->name.length));
            _boneToNodeMap[srcNode->typed_id] = node;
            _nodes[srcNode->typed_id] = node;
            
            for (size_t i = 0; i < srcNode->children.count; ++i)
            {
                osg::ref_ptr<osg::MatrixTransform> child = new osg::MatrixTransform;
                createNode(node, child.get(), srcNode->children[i]);
            }
        }
        else
            node = _nodes[srcNode->typed_id].get();
        if (parent) parent->addChild(node);
    }

    void LoaderFBX::createAnimation(ufbx_anim_layer* layer, const std::string& group, double t0, double t1,
                                    std::map<osg::Transform*, PlayerAnimation::AnimationData>& boneAnimMap)
    {
        typedef std::pair<std::vector<ufbx_anim_prop>, std::pair<int, bool>> AnimationChannel;
        std::map<osg::MatrixTransform*, AnimationChannel> nodeAnimChannels;
        std::map<osg::MatrixTransform*, osg::Vec3> nodePivots;
        for (size_t i = 0; i < layer->anim_props.count; ++i)
        {
            const ufbx_anim_prop& prop = layer->anim_props[i];
            switch (prop.element->type)
            {
            case UFBX_ELEMENT_NODE:
                if (_nodes.find(prop.element->typed_id) != _nodes.end())
                {
                    ufbx_node* srcNode = _scene->nodes[prop.element->typed_id]; osg::Vec3 pivot;
                    for (size_t p = 0; p < srcNode->props.props.count; ++p)
                    {
                        ufbx_prop& prop = srcNode->props.props[p];
                        std::string pName(prop.name.data, prop.name.length);
                        if (pName == UFBX_RotationPivot) pivot = toVec3(prop.value_vec3);
                    }

                    osg::MatrixTransform* mt = _nodes[prop.element->typed_id].get(); nodePivots[mt] = pivot;
                    AnimationChannel& ch = nodeAnimChannels[mt]; int order = (int)srcNode->rotation_order;
                    ch.first.push_back(prop); ch.second = std::pair<int, bool>(order, srcNode->bone != NULL);
                }
                break;
            case UFBX_ELEMENT_MESH:
                OSG_NOTICE << "[LoaderFBX] Animation on 'mesh' not implemented: " << prop.prop_name.data << "\n"; break;  // TODO
            case UFBX_ELEMENT_CAMERA:
                OSG_NOTICE << "[LoaderFBX] Animation on 'camera' not implemented: " << prop.prop_name.data << "\n"; break;  // TODO
            case UFBX_ELEMENT_BLEND_CHANNEL:
                OSG_NOTICE << "[LoaderFBX] Animation on 'blend-shape' not implemented: " << prop.prop_name.data << "\n"; break;  // TODO
            default:
                OSG_NOTICE << "[LoaderFBX] Animation on " << prop.element->type << " not implemented: " << prop.prop_name.data << "\n"; break;
            }
        }

        // Handle per-node animation data
        std::string layerName(layer->name.data, layer->name.length);
        for (std::map<osg::MatrixTransform*, AnimationChannel>::iterator it = nodeAnimChannels.begin();
             it != nodeAnimChannels.end(); ++it)
        {
            PlayerAnimation::AnimationData animData; std::vector<ufbx_anim_prop>& props = it->second.first;
            osg::MatrixTransform* mt = it->first; std::pair<int, bool>& orderAndBone = it->second.second;
            for (size_t i = 0; i < props.size(); ++i) createAnimation(animData, layer, props[i], orderAndBone.first);

            osg::Vec3 pivot = nodePivots[mt];
            if (!orderAndBone.second)
            {   // non-skeleton animations
                TweenAnimation* tween = dynamic_cast<TweenAnimation*>(mt->getUpdateCallback());
                if (!tween) { tween = new TweenAnimation; mt->addUpdateCallback(tween); }
                tween->setPivotPoint(pivot); tween->addAnimation(layerName, animData.toAnimationPath());
            }
            else
                boneAnimMap[mt] = animData;
        }
    }

    void LoaderFBX::createAnimation(PlayerAnimation::AnimationData& anim,
                                    ufbx_anim_layer* layer, const ufbx_anim_prop& prop, int order)
    {
        ufbx_anim_value* animValues = prop.anim_value; std::map<double, osg::Vec3> kfMap;
        std::string propName(prop.prop_name.data, prop.prop_name.length);
        osg::Vec3 def = toVec3(animValues->default_value);
        for (int i = 0; i < 3; ++i)
        {
            ufbx_anim_curve* animCurve = animValues->curves[i]; if (!animCurve) continue;
            for (size_t k = 0; k < animCurve->keyframes.count; ++k)
            {
                double t = animCurve->keyframes[k].time;
                if (kfMap.find(t) == kfMap.end()) kfMap[t] = def;
                kfMap[t][i] = animCurve->keyframes[k].value;
            }
        }

        std::vector<std::pair<float, osg::Vec3>> keyframes; if (kfMap.empty()) return;
        for (std::map<double, osg::Vec3>::iterator it = kfMap.begin(); it != kfMap.end(); ++it)
            keyframes.push_back(std::pair<float, osg::Vec3>((float)it->first, it->second));
        
        ufbx_rotation_order rotOrder = (ufbx_rotation_order)order;
        if (propName.find(UFBX_Lcl_Translation) != std::string::npos) anim._positionFrames.swap(keyframes);
        else if (propName.find(UFBX_Lcl_Scaling) != std::string::npos) anim._scaleFrames.swap(keyframes);
        else if (propName.find(UFBX_Lcl_Rotation) != std::string::npos)
        {
            anim._rotationFrames.resize(keyframes.size());
            for (size_t i = 0; i < keyframes.size(); ++i)
            {
                std::pair<float, osg::Vec3>& kf = keyframes[i]; osg::Vec3& v = kf.second;
                ufbx_quat q = ufbx_euler_to_quat({v[0], v[1], v[2]}, rotOrder);
                anim._rotationFrames[i] = std::pair<float, osg::Vec4>(kf.first, toQuat(q).asVec4());
            }
        }
        else OSG_NOTICE << "[LoaderFBX] Unknown animation prop-name: " << propName << "\n";
    }

    osg::Node* LoaderFBX::createMesh(const osg::Matrix& matrix, ufbx_mesh* srcMesh)
    {
        size_t max_parts = 0, max_triangles = 0, num_blend_shapes = 0, num_bones = 0;
        for (size_t pi = 0; pi < srcMesh->material_parts.count; pi++)
        {
            ufbx_mesh_part *part = &(srcMesh->material_parts.data[pi]);
            if (part->num_triangles == 0) continue;
            max_parts += 1; max_triangles = osg::maximum(max_triangles, part->num_triangles);
        }

        struct MeshVertex
        {
            osg::Vec3 position, normal, tangent;
            osg::Vec2 uv; osg::Vec4 color; unsigned int index;
        };
        struct SkinVertex { uint32_t bone_index[4]; float bone_weight[4]; };

        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->setName(std::string(srcMesh->name.data, srcMesh->name.length));
        bool withColors = srcMesh->vertex_color.exists;
        bool withNormals = srcMesh->vertex_normal.exists;
        bool withTangents = srcMesh->vertex_tangent.exists;
        bool withUVs = srcMesh->vertex_uv.exists;
        
        // Temporary buffers
        size_t num_tri_indices = srcMesh->max_face_triangles * 3;
        std::vector<uint32_t> tri_indices(num_tri_indices);
        std::vector<uint32_t> indices(max_triangles * 3);
        std::vector<MeshVertex> vertices(max_triangles * 3);
        std::vector<SkinVertex> skin_vertices(max_triangles * 3);
        std::vector<SkinVertex> mesh_skin_vertices(srcMesh->num_vertices);

        // Instance handling
        if (srcMesh->instances.count > 1) {}  // TODO

        // Pre-calculate skinned vertex data
        ufbx_skin_deformer* skin = NULL;
        if (srcMesh->skin_deformers.count > 0)
        {
            SkinningData& sd = _skinningDataMap[geode.get()];
            skin = srcMesh->skin_deformers.data[0]; sd.deformer = skin;
            for (size_t ci = 0; ci < skin->clusters.count; ci++)
            {
                ufbx_skin_cluster* cluster = skin->clusters.data[ci];
                sd.boneIndexAndMatrices.push_back(
                    std::pair<int, osg::Matrix>(cluster->bone_node->typed_id, toMatrix(cluster->geometry_to_bone)));
                num_bones++;
            }

            // Pre-calculate the skinned vertex bones/weights for each vertex
            for (size_t vi = 0; vi < srcMesh->num_vertices; vi++)
            {
                size_t num_weights = 0; uint32_t clusters[4] = { 0 };
                float total_weight = 0.0f, weights[4] = { 0.0f };

                // Pick the first N weights to use and get a reasonable approximation of the skinning
                ufbx_skin_vertex vertex_weights = skin->vertices.data[vi];
                for (size_t wi = 0; wi < vertex_weights.num_weights; wi++)
                {
                    if (num_weights >= 4) break;
                    ufbx_skin_weight weight = skin->weights.data[vertex_weights.weight_begin + wi];
                    //if (weight.cluster_index >= MAX_BONES) continue;

                    ufbx_skin_cluster* cluster = skin->clusters.data[weight.cluster_index];
                    total_weight += (float)weight.weight;
                    clusters[num_weights] = (uint32_t)weight.cluster_index;
                    weights[num_weights] = (float)weight.weight; num_weights++;
                }

                // Normalize and quantize the weights to 8 bits
                if (total_weight > 0.0f)
                {
                    SkinVertex& skin_vert = mesh_skin_vertices[vi]; float sum = 0.0f;
                    for (size_t i = 0; i < 4; i++)
                    {
                        float weight = (float)weights[i] / total_weight; sum += weight;
                        skin_vert.bone_weight[i] = weight;
                        skin_vert.bone_index[i] = clusters[i];
                    }
                    skin_vert.bone_weight[0] += 1.0 - sum;
                }
            }
        }

        // Split the mesh into parts by material
        ufbx_vec4 default_v4 = {0}; ufbx_vec3 default_v3 = {0}; ufbx_vec2 default_v2 = {0};
        std::map<osg::Geometry*, std::vector<unsigned int>> srcMeshIndexMap;
        for (size_t pi = 0; pi < srcMesh->material_parts.count; pi++)
        {
            ufbx_mesh_part* mesh_part = &(srcMesh->material_parts.data[pi]);
            if (mesh_part->num_triangles == 0) continue;

            // First fetch all vertices into a flat non-indexed buffer, we also need to triangulate the faces
            size_t num_indices = 0;
            for (size_t fi = 0; fi < mesh_part->num_faces; fi++)
            {
                ufbx_face face = srcMesh->faces.data[mesh_part->face_indices.data[fi]];
                size_t num_tris = ufbx_triangulate_face(tri_indices.data(), num_tri_indices, srcMesh, face);

                // Iterate through every vertex of every triangle in the triangulated result
                for (size_t vi = 0; vi < num_tris * 3; vi++)
                {
                    uint32_t ix = tri_indices[vi];
                    MeshVertex& vert = vertices[num_indices];
                    ufbx_vec3 pos = ufbx_get_vertex_vec3(&srcMesh->vertex_position, ix);
                    ufbx_vec3 normal = withNormals ? ufbx_get_vertex_vec3(&srcMesh->vertex_normal, ix) : default_v3;
                    ufbx_vec3 tangent = withTangents ? ufbx_get_vertex_vec3(&srcMesh->vertex_tangent, ix) : default_v3;
                    ufbx_vec4 color = withColors ? ufbx_get_vertex_vec4(&srcMesh->vertex_color, ix) : default_v4;
                    ufbx_vec2 uv = withUVs ? ufbx_get_vertex_vec2(&srcMesh->vertex_uv, ix) : default_v2;

                    if (skin) skin_vertices[num_indices] = mesh_skin_vertices[srcMesh->vertex_indices.data[ix]];
                    vert.position = toVec3(pos); vert.uv = toVec2(uv); vert.color = toVec4(color);
                    vert.normal = toVec3(normal); vert.normal.normalize();
                    vert.tangent = toVec3(tangent); vert.tangent.normalize();
                    vert.index = srcMesh->vertex_indices.data[ix]; num_indices++;
                }
            }

            // Optimize the flat vertex buffer into an indexed one
            ufbx_vertex_stream streams[2]; ufbx_error error;
            size_t num_streams = skin ? 2 : 1;
            streams[0].data = vertices.data();
            streams[0].vertex_count = num_indices;
            streams[0].vertex_size = sizeof(MeshVertex);
            if (skin)
            {
                streams[1].data = skin_vertices.data();
                streams[1].vertex_count = num_indices;
                streams[1].vertex_size = sizeof(SkinVertex);
            }
            
            size_t num_vertices = ufbx_generate_indices(
                streams, num_streams, indices.data(), num_indices, NULL, &error);
            if (error.type != UFBX_ERROR_NONE)
            {
                std::string desc(error.description.data, error.description.length);
                OSG_NOTICE << "[LoaderFBX] Error " << error.type << ": " << desc << "\n";
            }
            indices.resize(num_indices);

            // Handle material
            osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
            if (mesh_part->index < srcMesh->materials.count)
            {
                ufbx_material* mtl =  srcMesh->materials.data[mesh_part->index];
                if (mtl) geom->setStateSet(createMaterial(mtl));
            }

            osg::Vec3Array* va = new osg::Vec3Array(num_vertices);
            osg::Vec3Array* na = withNormals ? new osg::Vec3Array(num_vertices) : NULL;
            osg::Vec3Array* ta = withTangents ? new osg::Vec3Array(num_vertices) : NULL;
            osg::Vec4Array* ca = withColors ? new osg::Vec4Array(num_vertices) : NULL;
            osg::Vec2Array* uv = withUVs ? new osg::Vec2Array(num_vertices) : NULL;
            std::vector<unsigned int>& idx = srcMeshIndexMap[geom.get()]; idx.resize(num_vertices);
            for (size_t i = 0; i < num_vertices; ++i)
            {
                const MeshVertex& mv = vertices[i]; (*va)[i] = mv.position * matrix;
                if (withNormals) (*na)[i] = mv.normal; if (withTangents) (*ta)[i] = mv.tangent;
                if (withColors) (*ca)[i] = mv.color; if (withUVs) (*uv)[i] = mv.uv; idx[i] = mv.index;
            }
            
            geom->setName(geode->getName() + "_" + std::to_string(pi));
            geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
            geom->setVertexArray(va); if (withUVs) geom->setTexCoordArray(0, uv);
            if (withNormals) { geom->setNormalArray(na); geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX); }
            if (withColors) { geom->setColorArray(ca); geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX); }
            if (withTangents) { geom->setVertexAttribArray(6, ta); geom->setVertexAttribBinding(6, osg::Geometry::BIND_PER_VERTEX); }

            if (num_indices < 65535)
                geom->addPrimitiveSet(new osg::DrawElementsUShort(GL_TRIANGLES, indices.begin(), indices.end()));
            else
                geom->addPrimitiveSet(new osg::DrawElementsUInt(GL_TRIANGLES, indices.begin(), indices.end()));

            if (skin)
            {
                SkinningData& sd = _skinningDataMap[geode.get()];
                std::vector<SkinningData::JointWeights>& weightList = sd.skinningDataList[geom.get()];
                weightList.resize(num_vertices);
                for (size_t i = 0; i < num_vertices; ++i)
                {
                    const SkinVertex& skv = skin_vertices[i];
                    SkinningData::JointWeights& jw = weightList[i]; jw.resize(4);
                    for (int k = 0; k < 4; ++k) { jw[k].first = skv.bone_index[k]; jw[k].second = skv.bone_weight[k]; }
                }
            }
            geode->addDrawable(geom.get());
        }  // for (size_t pi = 0; pi < srcMesh->material_parts.count; pi++)


        // Fetch blend channels from all attached blend deformers
        for (size_t di = 0; di < srcMesh->blend_deformers.count; di++)
        {
            ufbx_blend_deformer* deformer = srcMesh->blend_deformers.data[di];
            if (deformer->channels.count > 0)
            {   // Set blendshape callback for every geometries
                std::string bsName(deformer->name.data, deformer->name.length);
                for (size_t i = 0; i < geode->getNumDrawables(); ++i)
                {
                    osg::Geometry* geom = static_cast<osg::Geometry*>(geode->getDrawable(i));
                    BlendShapeAnimation* bsa = dynamic_cast<BlendShapeAnimation*>(geom->getUpdateCallback());
                    if (!bsa)
                    {
                        bsa = new BlendShapeAnimation; bsa->setName(bsName + "_BsCallback");
                        geom->setUpdateCallback(bsa);
                    }
                }
            }

            for (size_t ci = 0; ci < deformer->channels.count; ci++)
            {   // Get offsets from blend channels and apply them to the callback
                ufbx_blend_channel* chan = deformer->channels.data[ci];
                if (chan->keyframes.count == 0) continue; num_blend_shapes++;

                ufbx_blend_shape* shape = chan->keyframes.data[chan->keyframes.count - 1].shape;
                std::map<unsigned int, osg::Vec3> deltaP, deltaN;
                for (size_t i = 0; i < shape->offset_vertices.count; ++i)
                {
                    unsigned int srcIdx = shape->offset_vertices.data[i];
                    if (shape->position_offsets.count > 0) deltaP[srcIdx] = toVec3(shape->position_offsets.data[i]);
                    if (shape->normal_offsets.count > 0) deltaN[srcIdx] = toVec3(shape->normal_offsets.data[i]);
                }
                
                std::string chName(chan->name.data, chan->name.length);
                for (size_t i = 0; i < geode->getNumDrawables(); ++i)
                {
                    osg::Geometry* geom = static_cast<osg::Geometry*>(geode->getDrawable(i));
                    std::vector<unsigned int>& idx = srcMeshIndexMap[geom]; if (idx.empty()) continue;

                    BlendShapeAnimation::BlendShapeData* bsd = new BlendShapeAnimation::BlendShapeData;
                    {
                        if (!deltaP.empty()) bsd->vertices = new osg::Vec3Array(idx.size());
                        if (!deltaN.empty()) bsd->normals = new osg::Vec3Array(idx.size());
                        for (size_t j = 0; j < idx.size(); ++j)
                        {
                            unsigned int srcID = idx[j];
                            if (deltaP.find(srcID) != deltaP.end()) (*bsd->vertices)[j] = deltaP[srcID];
                            if (deltaN.find(srcID) != deltaN.end()) (*bsd->normals)[j] = deltaN[srcID];
                        }
                    }

                    BlendShapeAnimation* bsa = dynamic_cast<BlendShapeAnimation*>(geom->getUpdateCallback());
                    bsd->weight = chan->weight; bsa->addBlendShapeData(bsd); bsa->registerBlendShape(chName, bsd);
                }
            }
        }
        return geode.release();
    }

    osg::StateSet* LoaderFBX::createMaterial(ufbx_material* mtl)
    {
        if (_materials.find(mtl) == _materials.end())
        {
            osg::ref_ptr<osg::StateSet> ss = new osg::StateSet;
            ss->setName(std::string(mtl->name.data, mtl->name.length));
#if false
            std::stringstream sstream;
#define CHECK_TEX_STATE(val) if (mtl-> val .texture) sstream << " " << #val
            CHECK_TEX_STATE(pbr.base_color); CHECK_TEX_STATE(pbr.base_factor); CHECK_TEX_STATE(pbr.roughness); CHECK_TEX_STATE(pbr.metalness);
			CHECK_TEX_STATE(pbr.diffuse_roughness); CHECK_TEX_STATE(pbr.specular_factor); CHECK_TEX_STATE(pbr.specular_color);
			CHECK_TEX_STATE(pbr.specular_ior); CHECK_TEX_STATE(pbr.specular_anisotropy); CHECK_TEX_STATE(pbr.specular_rotation);
			CHECK_TEX_STATE(pbr.transmission_factor); CHECK_TEX_STATE(pbr.transmission_color); CHECK_TEX_STATE(pbr.transmission_depth);
			CHECK_TEX_STATE(pbr.transmission_scatter); CHECK_TEX_STATE(pbr.transmission_scatter_anisotropy);
			CHECK_TEX_STATE(pbr.transmission_dispersion); CHECK_TEX_STATE(pbr.transmission_roughness); CHECK_TEX_STATE(pbr.transmission_extra_roughness);
			CHECK_TEX_STATE(pbr.transmission_priority); CHECK_TEX_STATE(pbr.transmission_enable_in_aov); CHECK_TEX_STATE(pbr.subsurface_factor);
			CHECK_TEX_STATE(pbr.subsurface_color); CHECK_TEX_STATE(pbr.subsurface_radius); CHECK_TEX_STATE(pbr.subsurface_scale);
			CHECK_TEX_STATE(pbr.subsurface_anisotropy); CHECK_TEX_STATE(pbr.subsurface_tint_color); CHECK_TEX_STATE(pbr.subsurface_type);
			CHECK_TEX_STATE(pbr.sheen_factor); CHECK_TEX_STATE(pbr.sheen_color); CHECK_TEX_STATE(pbr.sheen_roughness);
			CHECK_TEX_STATE(pbr.coat_factor); CHECK_TEX_STATE(pbr.coat_color); CHECK_TEX_STATE(pbr.coat_roughness); CHECK_TEX_STATE(pbr.coat_ior);
			CHECK_TEX_STATE(pbr.coat_anisotropy); CHECK_TEX_STATE(pbr.coat_rotation); CHECK_TEX_STATE(pbr.coat_normal);
			CHECK_TEX_STATE(pbr.coat_affect_base_color); CHECK_TEX_STATE(pbr.coat_affect_base_roughness); CHECK_TEX_STATE(pbr.thin_film_factor);
			CHECK_TEX_STATE(pbr.thin_film_thickness); CHECK_TEX_STATE(pbr.thin_film_ior); CHECK_TEX_STATE(pbr.emission_factor);
			CHECK_TEX_STATE(pbr.emission_color); CHECK_TEX_STATE(pbr.opacity); CHECK_TEX_STATE(pbr.indirect_diffuse);
            CHECK_TEX_STATE(pbr.indirect_specular); CHECK_TEX_STATE(pbr.normal_map); CHECK_TEX_STATE(pbr.tangent_map);
			CHECK_TEX_STATE(pbr.displacement_map); CHECK_TEX_STATE(pbr.matte_factor); CHECK_TEX_STATE(pbr.matte_color);
			CHECK_TEX_STATE(pbr.ambient_occlusion); CHECK_TEX_STATE(pbr.glossiness); CHECK_TEX_STATE(pbr.coat_glossiness);
			CHECK_TEX_STATE(pbr.transmission_glossiness); CHECK_TEX_STATE(fbx.diffuse_factor); CHECK_TEX_STATE(fbx.diffuse_color);
			CHECK_TEX_STATE(fbx.specular_factor); CHECK_TEX_STATE(fbx.specular_color); CHECK_TEX_STATE(fbx.specular_exponent);
			CHECK_TEX_STATE(fbx.reflection_factor); CHECK_TEX_STATE(fbx.reflection_color); CHECK_TEX_STATE(fbx.transparency_factor);
            CHECK_TEX_STATE(fbx.transparency_color); CHECK_TEX_STATE(fbx.emission_factor); CHECK_TEX_STATE(fbx.emission_color);
			CHECK_TEX_STATE(fbx.ambient_factor); CHECK_TEX_STATE(fbx.ambient_color); CHECK_TEX_STATE(fbx.normal_map); CHECK_TEX_STATE(fbx.bump);
			CHECK_TEX_STATE(fbx.bump_factor); CHECK_TEX_STATE(fbx.displacement_factor); CHECK_TEX_STATE(fbx.displacement);
			CHECK_TEX_STATE(fbx.vector_displacement_factor); CHECK_TEX_STATE(fbx.vector_displacement);
            OSG_NOTICE << "[LoaderFBX] " << ss->getName() << " has textures:" << sstream.str() << "\n";
#endif
            //if (mtl->features.pbr.enabled)
            {
                osg::ref_ptr<osg::Texture> tD = applyMaterialData(&(mtl->pbr.base_color), &(mtl->pbr.base_factor));
                osg::ref_ptr<osg::Texture> tN = applyMaterialData(&(mtl->pbr.normal_map), NULL);
                osg::ref_ptr<osg::Texture> tS = applyMaterialData(&(mtl->pbr.specular_color), &(mtl->pbr.specular_factor));
                osg::ref_ptr<osg::Texture> tE = applyMaterialData(&(mtl->pbr.emission_color), &(mtl->pbr.emission_factor));
                osg::ref_ptr<osg::Texture> tORM;

                if (_usingMaterialPBR > 1 || (tN.valid() && _usingMaterialPBR > 0))
                {   // PBR materials: read and combine O-R-M
                    if (_ormTextureMap.find(mtl) == _ormTextureMap.end())
                    {
                        osg::ref_ptr<osg::Texture> tO = applyMaterialData(&(mtl->pbr.ambient_occlusion), NULL);
                        osg::ref_ptr<osg::Texture> tR = applyMaterialData(&(mtl->pbr.roughness), NULL);
                        osg::ref_ptr<osg::Texture> tM = applyMaterialData(&(mtl->pbr.metalness), NULL);
                        bool compressed = tO.valid() ? tO->getImage(0)->isCompressed() : false;

                        if (tR.valid())
                        {
                            compressed = tR->getImage(0)->isCompressed(); tORM = tR;
                            if (tM.valid()) tORM = constructOcclusionRoughnessMetallic(tORM, tM.get(), -1, -1, 0);
                            if (tO.valid()) tORM = constructOcclusionRoughnessMetallic(tORM, tO.get(), 0, -1, -1);
                        }
                        else if (tM.valid())
                        {
                            compressed = tM->getImage(0)->isCompressed(); tORM = tM;
                            if (tO.valid()) tORM = constructOcclusionRoughnessMetallic(tORM, tO.get(), 0, -1, -1);
                        }
                        else if (tO.valid()) tORM = tO;
                        
                        if (tORM.valid())
                        {
                            if (compressed)
                            {
                                if (tORM->getNumImages() > 0) tORM->setImage(0, compressImage(*tORM->getImage(0)));
                                tORM->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
                            }
                            _ormTextureMap[mtl] = tORM;
                        }
                    }
                    else tORM = _ormTextureMap[mtl];
                }

                if (tD.valid()) ss->setTextureAttributeAndModes(0, tD.get());
                if (tN.valid()) ss->setTextureAttributeAndModes(1, tN.get());
                if (tS.valid()) ss->setTextureAttributeAndModes(2, tS.get());
                if (tORM.valid()) ss->setTextureAttributeAndModes(3, tORM.get());
                if (tE.valid()) ss->setTextureAttributeAndModes(5, tE.get());
            }

            if (_usingMaterialPBR <= 0)
            {
                //if (mtl->features.opacity.enabled) ss->setRenderingHint(osg::StateSet::OPAQUE_BIN);
                //else ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
            }
            if (mtl->features.double_sided.enabled) ss->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
            else ss->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
#if !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE) && !defined(OSG_GL3_AVAILABLE)
            if (_usingMaterialPBR <= 0)
            { if (mtl->features.unlit.enabled) ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF); }
#endif
            _materials[mtl] = ss;
        }
        return _materials[mtl].get();
    }

    osg::Texture* LoaderFBX::applyMaterialData(ufbx_material_map* color, ufbx_material_map* factor)
    {
        osg::ref_ptr<osg::Image> image;
        ufbx_wrap_mode wrapU = UFBX_WRAP_REPEAT, wrapV = UFBX_WRAP_REPEAT;
        if (color->texture)
        {
            std::string realName, fileName = StringAuxiliary::convertNativePath(
                std::string(color->texture->relative_filename.data, color->texture->relative_filename.length));
            if (_images.find(color->texture->file_index) != _images.end()) image = _images[color->texture->file_index];

            // Get readerwriter first
            std::string simpleFile = osgDB::getSimpleFileName(fileName), ext = osgDB::getFileExtension(fileName);
            osgDB::ReaderWriter* rw = osgDB::Registry::instance()->getReaderWriterForExtension(ext);
            if (!rw) rw = osgDB::Registry::instance()->getReaderWriterForExtension("verse_image");
            if (!rw) { OSG_WARN << "[LoaderFBX] No image reading plugin found for: " << fileName << "\n"; return NULL; }

            if (!image && color->texture->content.size > 0)
            {   // Read from internal media data
                const char* ptr = (const char*)color->texture->content.data;
                std::vector<char> data(ptr, ptr + color->texture->content.size);
                std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
                stream.write((char*)data.data(), data.size());
                image = rw->readImage(stream).takeImage(); realName = "embedded media";
            }
            
            if (!color->texture->has_file) return NULL;
            if (!image)
            {   // Read from external file
                std::string fullName = StringAuxiliary::convertNativePath(
                    std::string(color->texture->absolute_filename.data, color->texture->absolute_filename.length));
                image = rw->readImage(fullName).takeImage();
                if (!image) image = rw->readImage(_workingDir + fileName).takeImage(); else realName = fullName;
                if (!image) image = rw->readImage(_workingDir + simpleFile).takeImage(); else realName = _workingDir + fileName;
                if (!image) OSG_NOTICE << "[LoaderFBX] Failed to load texture image: " << fileName << "\n";
                else realName = _workingDir + simpleFile;
            }

            wrapU = color->texture->wrap_u; wrapV = color->texture->wrap_v;
            if (image.valid())
            {
                _images[color->texture->file_index] = image;
                OSG_INFO << "[LoaderFBX] Loaded image from " << realName << std::endl;
            }
        }
        else return NULL;

        // TODO: handle factor in a more detailed way?
        if (!image)
        {
            osg::Vec4 col = color->has_value ? toColorValue(*color) : osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
            osg::Vec4 fac = factor ? toColorValue(*factor) : osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
            image = createDefaultImageForColor(osg::Vec4(col[0] * fac[0], col[1] * fac[1], col[2] * fac[2], col[3] * fac[3]));
        }

        osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
        tex2D->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR_MIPMAP_LINEAR);
        tex2D->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        tex2D->setWrap(osg::Texture2D::WRAP_S, wrapU == UFBX_WRAP_REPEAT ? osg::Texture2D::REPEAT : osg::Texture2D::CLAMP);
        tex2D->setWrap(osg::Texture2D::WRAP_T, wrapV == UFBX_WRAP_REPEAT ? osg::Texture2D::REPEAT : osg::Texture2D::CLAMP);
        tex2D->setImage(image.get()); return tex2D.release();
    }

    osg::ref_ptr<osg::Group> loadFbx(const std::string& file, int pbr)
    {
        std::string workDir = osgDB::getFilePath(file), http = osgDB::getServerProtocol(file);
        if (!http.empty()) return NULL;
        std::ifstream in(file.c_str(), std::ios::in | std::ios::binary);
        if (!in)
        {
            OSG_WARN << "[LoaderFBX] file " << file << " not readable" << std::endl;
            return NULL;
        }

        osg::ref_ptr<LoaderFBX> loader = new LoaderFBX(in, workDir, pbr > 0);
        if (loader->getRoot()) loader->getRoot()->setName(file);
        return loader->getRoot();
    }

    osg::ref_ptr<osg::Group> loadFbx2(std::istream& in, const std::string& dir, int pbr)
    {
        osg::ref_ptr<LoaderFBX> loader = new LoaderFBX(in, dir, pbr > 0);
        return loader->getRoot();
    }
}
