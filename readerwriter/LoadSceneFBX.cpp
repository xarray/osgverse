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

#define MAX_BONES 64
#define MAX_BLEND_SHAPES 64
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

        _root = new osg::MatrixTransform;
#if !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE) && !defined(OSG_GL3_AVAILABLE)
        _root->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
#endif
        createNode(NULL, _root.get(), _scene->root_node);
    }

    void LoaderFBX::createNode(osg::Group* parent, osg::MatrixTransform* node, ufbx_node* srcNode)
    {
        ufbx_mesh* srcMesh = srcNode->mesh;
        if (srcMesh) node->addChild(createMesh(toMatrix(srcNode->geometry_to_node), srcMesh));
        node->setMatrix(toMatrix(srcNode->node_to_parent));
        node->setName(std::string(srcNode->name.data, srcNode->name.length));
        
        for (size_t i = 0; i < srcNode->children.count; ++i)
        {
            osg::ref_ptr<osg::MatrixTransform> child = new osg::MatrixTransform;
            createNode(node, child.get(), srcNode->children[i]);
        }
        if (parent && node->getNumChildren() > 0) parent->addChild(node);
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
        struct SkinVertex { uint8_t bone_index[4]; uint8_t bone_weight[4]; };

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
                if (num_bones < MAX_BONES)
                {
                    _boneIndexAndMatrices.push_back(
                        std::pair<int, osg::Matrix>(cluster->bone_node->typed_id, toMatrix(cluster->geometry_to_bone)));
                    num_bones++;
                }
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
                    if (weight.cluster_index >= MAX_BONES) continue;

                    total_weight += (float)weight.weight;
                    clusters[num_weights] = (uint8_t)weight.cluster_index;
                    weights[num_weights] = (float)weight.weight; num_weights++;
                }

                // Normalize and quantize the weights to 8 bits
                if (total_weight > 0.0f)
                {
                    SkinVertex& skin_vert = mesh_skin_vertices[vi]; uint32_t quantized_sum = 0;
                    for (size_t i = 0; i < 4; i++)
                    {
                        uint8_t quantized_weight = (uint8_t)((float)weights[i] / total_weight * 255.0f);
                        quantized_sum += quantized_weight;
                        skin_vert.bone_weight[i] = quantized_weight;
                        skin_vert.bone_index[i] = clusters[i];
                    }
                    skin_vert.bone_weight[0] += 255 - quantized_sum;
                }
            }
        }

        // Fetch blend channels from all attached blend deformers
        ufbx_blend_channel *blend_channels[MAX_BLEND_SHAPES];
        for (size_t di = 0; di < srcMesh->blend_deformers.count; di++)
        {
            ufbx_blend_deformer *deformer = srcMesh->blend_deformers.data[di];
            for (size_t ci = 0; ci < deformer->channels.count; ci++)
            {
                ufbx_blend_channel *chan = deformer->channels.data[ci];
                if (chan->keyframes.count == 0) continue;
                if (num_blend_shapes < MAX_BLEND_SHAPES)
                {
                    blend_channels[num_blend_shapes] = chan;
                    num_blend_shapes++;  // TODO
                }
            }

            if (num_blend_shapes > 0) {}  // TODO
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
            std::string fileName(color->texture->relative_filename.data, color->texture->relative_filename.length);
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
            
            if (!image)
            {   // Read from external file
                image = rw->readImage(std::string(color->texture->filename.data, color->texture->filename.length)).takeImage();
                if (!image) image = rw->readImage(fileName).takeImage();
                if (!image) image = rw->readImage(_workingDir + "/" + simpleFile).takeImage();
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
