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

#include "pipeline/Utilities.h"
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
        opts.target_axes.right = UFBX_COORDINATE_AXIS_POSITIVE_X;
        opts.target_axes.up = UFBX_COORDINATE_AXIS_POSITIVE_Z;
        opts.target_axes.front = UFBX_COORDINATE_AXIS_NEGATIVE_Y;
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

        // Handle skeletons
        for (std::map<osg::Geometry*, std::vector<JointWeights>>::iterator it = _skinningDataList.begin();
             it != _skinningDataList.end(); ++it)
        {
            // TODO: <bone_id, weight> to <node, weight>
        }

        // Handle animations
        for (size_t i = 0; i < _scene->anim_stacks.count; ++i)
        {
            ufbx_anim_stack* st = _scene->anim_stacks[i]; std::string name(st->name.data, st->name.length);
            for (size_t j = 0; j < st->layers.count; ++j) createAnimation(st->layers[j], name, st->time_begin, st->time_end);
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
            if (srcNode->bone) _boneToNodeMap[srcNode->bone->typed_id] = node;
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

    void LoaderFBX::createAnimation(ufbx_anim_layer* layer, const std::string& group, double t0, double t1)
    {
        typedef std::pair<std::vector<ufbx_anim_prop>, std::pair<int, bool>> AnimationChannel;
        std::map<osg::MatrixTransform*, AnimationChannel> nodeAnimChannels;
        for (size_t i = 0; i < layer->anim_props.count; ++i)
        {
            const ufbx_anim_prop& prop = layer->anim_props[i];
            switch (prop.element->type)
            {
            case UFBX_ELEMENT_NODE:
                if (_nodes.find(prop.element->typed_id) != _nodes.end())
                {
                    ufbx_node* srcNode = _scene->nodes[prop.element->typed_id];
                    osg::MatrixTransform* mt = _nodes[prop.element->typed_id].get();
                    AnimationChannel& ch = nodeAnimChannels[mt]; int order = (int)srcNode->rotation_order;
                    ch.first.push_back(prop); ch.second = std::pair<int, bool>(order, srcNode->bone != NULL);
                }
                break;
            default:
                OSG_NOTICE << "[LoaderFBX] Animation target " << prop.element->type << " not implemeneted\n"; break;
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

            if (orderAndBone.second)  // MT is bone node
            {}  // TODO
            else
            {   // non-skeleton animations
                TweenAnimation* tween = dynamic_cast<TweenAnimation*>(mt->getUpdateCallback());
                if (!tween) { tween = new TweenAnimation; mt->addUpdateCallback(tween); }
                tween->addAnimation(layerName, animData.toAnimationPath());
            }
        }
    }

    void LoaderFBX::createAnimation(PlayerAnimation::AnimationData& anim,
                                    ufbx_anim_layer* layer, const ufbx_anim_prop& prop, int order)
    {
        ufbx_anim_value* animValues = prop.anim_value; std::map<double, osg::Vec3> kfMap;
        osg::Vec3 def = toVec3(animValues->default_value);
        for (int i = 0; i < 3; ++i)
        {
            ufbx_anim_curve* animCurve = animValues->curves[i]; if (!animCurve) continue;
            for (size_t k = 0; k < animCurve->keyframes.count; ++k)
            {
                double t = animCurve->keyframes[k].time;
                if (kfMap.find(t) == kfMap.end()) kfMap[t] = def;
                kfMap[t][i] = animCurve->keyframes[k].value;  // FIXME: not considering handles?
            }
        }

        std::string propName(prop.prop_name.data, prop.prop_name.length);
        std::vector<std::pair<float, osg::Vec3>> keyframes; if (kfMap.empty()) return;
        for (std::map<double, osg::Vec3>::iterator it = kfMap.begin(); it != kfMap.end(); ++it)
             keyframes.push_back(std::pair<float, osg::Vec3>((float)it->first, it->second));
        
        ufbx_rotation_order rotOrder = (ufbx_rotation_order)order;
        if (propName.find(UFBX_Lcl_Translation) != std::string::npos) anim._positionFrames = keyframes;
        else if (propName.find(UFBX_Lcl_Scaling) != std::string::npos) anim._scaleFrames = keyframes;
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
        struct SkinVertex { uint8_t bone_index[4]; float bone_weight[4]; };

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
            skin = srcMesh->skin_deformers.data[0];
            for (size_t ci = 0; ci < skin->clusters.count; ci++)
            {
                ufbx_skin_cluster* cluster = skin->clusters.data[ci];
                _boneIndexAndMatrices.push_back(
                    std::pair<int, osg::Matrix>(cluster->bone_node->typed_id, toMatrix(cluster->geometry_to_bone)));
                num_bones++;
            }

            // Pre-calculate the skinned vertex bones/weights for each vertex
            for (size_t vi = 0; vi < srcMesh->num_vertices; vi++)
            {
                size_t num_weights = 0; uint8_t clusters[4] = { 0 };
                float total_weight = 0.0f, weights[4] = { 0.0f };

                // Pick the first N weights to use and get a reasonable approximation of the skinning
                ufbx_skin_vertex vertex_weights = skin->vertices.data[vi];
                for (size_t wi = 0; wi < vertex_weights.num_weights; wi++)
                {
                    if (num_weights >= 4) break;
                    ufbx_skin_weight weight = skin->weights.data[vertex_weights.weight_begin + wi];
                    //if (weight.cluster_index >= MAX_BONES) continue;

                    total_weight += (float)weight.weight;
                    clusters[num_weights] = (uint8_t)weight.cluster_index;
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

        // Fetch blend channels from all attached blend deformers
        std::vector<ufbx_blend_channel*> blend_channels;
        for (size_t di = 0; di < srcMesh->blend_deformers.count; di++)
        {
            ufbx_blend_deformer* deformer = srcMesh->blend_deformers.data[di];
            for (size_t ci = 0; ci < deformer->channels.count; ci++)
            {
                ufbx_blend_channel* chan = deformer->channels.data[ci];
                if (chan->keyframes.count == 0) continue;
                blend_channels.push_back(chan); num_blend_shapes++;
            }

            // TODO: https://github.com/ufbx/ufbx/blob/main/examples/viewer/viewer.c
            if (deformer->channels.count > 0)
            { OSG_NOTICE << "[LoaderFBX] Blendshapes of " << deformer->name.data << " not handled\n"; }
        }

        // Split the mesh into parts by material
        ufbx_vec4 default_v4 = {0}; ufbx_vec3 default_v3 = {0}; ufbx_vec2 default_v2 = {0};
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
            for (size_t i = 0; i < num_vertices; ++i)
            {
                const MeshVertex& mv = vertices[i]; (*va)[i] = mv.position * matrix;
                if (withNormals) (*na)[i] = mv.normal; if (withTangents) (*ta)[i] = mv.tangent;
                if (withColors) (*ca)[i] = mv.color; if (withUVs) (*uv)[i] = mv.uv;
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
                std::vector<JointWeights>& weightList = _skinningDataList[geom.get()];
                weightList.resize(num_vertices);
                for (size_t i = 0; i < num_vertices; ++i)
                {
                    const SkinVertex& skv = skin_vertices[i]; JointWeights jw(4);
                    for (int k = 0; k < 4; ++k) { jw[k].first = skv.bone_index[k]; jw[k].second = skv.bone_weight[k]; }
                }
            }
            geode->addDrawable(geom.get());
        }  // for (size_t pi = 0; pi < srcMesh->material_parts.count; pi++)
        return geode.release();
    }

    osg::StateSet* LoaderFBX::createMaterial(ufbx_material* mtl)
    {
        if (_materials.find(mtl) == _materials.end())
        {
            osg::ref_ptr<osg::StateSet> ss = new osg::StateSet;
            ss->setName(std::string(mtl->name.data, mtl->name.length));
            //if (mtl->features.pbr.enabled)
            {
                osg::ref_ptr<osg::Texture> tD = applyMaterialData(&(mtl->pbr.base_color), &(mtl->pbr.base_factor));
                osg::ref_ptr<osg::Texture> tN = applyMaterialData(&(mtl->pbr.normal_map), NULL);
                osg::ref_ptr<osg::Texture> tS = applyMaterialData(&(mtl->pbr.specular_color), &(mtl->pbr.specular_factor));
                osg::ref_ptr<osg::Texture> tE = applyMaterialData(&(mtl->pbr.emission_color), &(mtl->pbr.emission_factor));
                if (_usingMaterialPBR > 1 || (tN.valid() && _usingMaterialPBR > 0))
                {   // PBR materials
                    // TODO: read and combine O-R-M
                }

                if (tD.valid()) ss->setTextureAttributeAndModes(0, tD.get());
                if (tN.valid()) ss->setTextureAttributeAndModes(1, tN.get());
                if (tS.valid()) ss->setTextureAttributeAndModes(4, tS.get());
                if (tE.valid()) ss->setTextureAttributeAndModes(5, tE.get());
            }

            if (mtl->features.opacity.enabled) ss->setRenderingHint(osg::StateSet::OPAQUE_BIN);
            else ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
            if (mtl->features.double_sided.enabled) ss->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
            else ss->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
#if !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE) && !defined(OSG_GL3_AVAILABLE)
            if (mtl->features.unlit.enabled) ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
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
            std::string fileName = StringAuxiliary::convertNativePath(
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
                image = rw->readImage(stream).takeImage();
            }
            
            if (!color->texture->has_file) return NULL;
            if (!image)
            {   // Read from external file
                std::string fullName = StringAuxiliary::convertNativePath(
                    std::string(color->texture->absolute_filename.data, color->texture->absolute_filename.length));
                image = rw->readImage(fullName).takeImage();
                if (!image) image = rw->readImage(_workingDir + fileName).takeImage();
                if (!image) image = rw->readImage(_workingDir + simpleFile).takeImage();
                if (!image) OSG_NOTICE << "[LoaderFBX] Failed to load texture image: " << fileName << "\n";
            }
            wrapU = color->texture->wrap_u; wrapV = color->texture->wrap_v;
            if (image.valid()) _images[color->texture->file_index] = image;
        }

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
