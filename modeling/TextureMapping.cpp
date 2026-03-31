#include "TextureMapping.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <queue>
#include <unordered_map>
#include <iostream>
#include <limits>

#include <osg/Version>
#include <osg/Math>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/SparseCholesky>
#include <Eigen/SparseCore>

#ifdef _OPENMP
#include <omp.h>
#endif

#define VERBOSE_PROCESSING 1
#if VERBOSE_PROCESSING
#   define COUT std::cout
#else
#   define COUT OSG_DEBUG
#endif

namespace osgVerse
{

    void TextureMapping::Mesh::convertFromColmap(std::vector<TextureMapping::Vertex>& vertices)
    { for (auto& v : vertices) {v.y = -v.y; v.z = -v.z;} }

    TextureMapping::Bitmap::Bitmap() : as_rgb_(true)
    { image_ = new osg::Image(); }

    TextureMapping::Bitmap::Bitmap(int width, int height, bool as_rgb) : as_rgb_(as_rgb)
    {
        int channels = as_rgb ? 3 : 4;
        image_ = new osg::Image;
        image_->allocateImage(width, height, 1, as_rgb ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE);
        image_->setPixelFormat(as_rgb ? GL_RGB : GL_RGBA);
    }

    TextureMapping::Bitmap::Bitmap(osg::Image* image) : image_(image), as_rgb_(true)
    { if (image_) as_rgb_ = (image_->getPixelFormat() == GL_RGB); }

    bool TextureMapping::Bitmap::read(const std::string& path)
    {
        image_ = osgDB::readImageFile(path); if (!image_) return false;
        as_rgb_ = (image_->getPixelFormat() == GL_RGB); return true;
    }

    bool TextureMapping::Bitmap::write(const std::string& path) const
    { if (!image_) return false; return osgDB::writeImageFile(*image_, path); }

    int TextureMapping::Bitmap::width() const { return image_ ? image_->s() : 0; }
    int TextureMapping::Bitmap::height() const { return image_ ? image_->t() : 0; }
    int TextureMapping::Bitmap::channels() const { if (!image_) return 0; return as_rgb_ ? 3 : 4; }

    template<>
    bool TextureMapping::Bitmap::getPixel<unsigned char>(int x, int y, TextureMapping::Color<unsigned char>* color) const
    {
        if (!image_ || x < 0 || x >= width() || y < 0 || y >= height()) return false;
        unsigned char* data = image_->data(x, y);
        color->r = data[0]; color->g = data[1]; color->b = data[2];
        color->a = as_rgb_ ? 255 : data[3]; return true;
    }

    template<>
    bool TextureMapping::Bitmap::getPixel<float>(int x, int y, TextureMapping::Color<float>* color) const
    {
        TextureMapping::Color<unsigned char> ub_color;
        if (!getPixel(x, y, &ub_color)) return false;
        *color = ub_color.cast<float>();
        return true;
    }

    template<>
    void TextureMapping::Bitmap::setPixel<unsigned char>(int x, int y, const TextureMapping::Color<unsigned char>& color)
    {
        if (!image_ || x < 0 || x >= width() || y < 0 || y >= height()) return;
        unsigned char* data = image_->data(x, y);
        data[0] = color.r; data[1] = color.g; data[2] = color.b;
        if (!as_rgb_) data[3] = color.a;
    }

    template<>
    void TextureMapping::Bitmap::setPixel<float>(int x, int y, const Color<float>& color)
    { setPixel(x, y, color.cast<unsigned char>()); }

    bool TextureMapping::Bitmap::interpolateBilinear(double x, double y, TextureMapping::Colorf* color) const
    {
        if (!image_ || x < 0 || x >= width() - 1 || y < 0 || y >= height() - 1) return false;
        int x0 = static_cast<int>(std::floor(x));
        int y0 = static_cast<int>(std::floor(y));
        int x1 = x0 + 1, y1 = y0 + 1;
        double fx = x - x0, fy = y - y0;
  
        TextureMapping::Colorub c00, c01, c10, c11;
        getPixel(x0, y0, &c00); getPixel(x0, y1, &c01);
        getPixel(x1, y0, &c10); getPixel(x1, y1, &c11);
  
        auto lerp = [](double a, double b, double t) { return a * (1 - t) + b * t; };
        color->r = static_cast<float>(lerp(lerp(c00.r, c10.r, fx), lerp(c01.r, c11.r, fx), fy)) / 255.0f;
        color->g = static_cast<float>(lerp(lerp(c00.g, c10.g, fx), lerp(c01.g, c11.g, fx), fy)) / 255.0f;
        color->b = static_cast<float>(lerp(lerp(c00.b, c10.b, fx), lerp(c01.b, c11.b, fx), fy)) / 255.0f;
        color->a = static_cast<float>(lerp(lerp(c00.a, c10.a, fx), lerp(c01.a, c11.a, fx), fy)) / 255.0f;
        return true;
    }

    void TextureMapping::Bitmap::fill(const TextureMapping::Colorub& color)
    {
        if (!image_) return;
        for (int y = 0; y < height(); ++y)
            for (int x = 0; x < width(); ++x) setPixel(x, y, color);
    }

    TextureMapping::Image::Image(const osg::Matrixf& K_, const osg::Matrixf& R_,
                                 const osg::Vec3f& T_, const TextureMapping::Bitmap& bmp)
    :   K(K_), R(R_), T(T_), bitmap(bmp)
    {
          width = bitmap.width();
          height = bitmap.height();
          computeProjectionMatrix();
    }

    void TextureMapping::Image::computeProjectionMatrix()
    {
        // P = K * [R | t]
        osg::Matrixf RT;
        RT.set(
            R(0,0), R(0,1), R(0,2), T.x(),
            R(1,0), R(1,1), R(1,2), T.y(),
            R(2,0), R(2,1), R(2,2), T.z(),
            0, 0, 0, 1);
        P = K * RT;
    }

    osg::Vec3f TextureMapping::Image::getCameraCenter() const
    {
        // C = -R^T * T
        osg::Matrixf RT = R;
        RT.transpose(RT); return -(RT * T);
    }

    void TextureMapping::Image::convertFromColmap(const osg::Matrixf& R_colmap, const osg::Vec3f& T_colmap,
                                                  osg::Matrixf& R_osg, osg::Vec3f& T_osg)
    {
        const static osg::Matrixf kColmapToOsgMatrix(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, -1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, -1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f);
        R_osg = kColmapToOsgMatrix * R_colmap * kColmapToOsgMatrix;
        T_osg = kColmapToOsgMatrix * T_colmap;
    }

    bool TextureMapping::MeshTextureMappingOptions::check() const
    {
        if (min_cos_normal_angle <= 0.0 || min_cos_normal_angle > 1.0)
        {
            OSG_NOTICE << "[TextureMapping] min_cos_normal_angle must be in (0, 1]" << std::endl;
            return false;
        }
        if (min_visible_vertices < 1 || min_visible_vertices > 3)
        {
            OSG_NOTICE << "[TextureMapping] min_visible_vertices must be 1, 2, or 3" << std::endl;
            return false;
        }
        if (view_selection_smoothing_iterations < 0)
        {
            OSG_NOTICE << "[TextureMapping] view_selection_smoothing_iterations must be >= 0" << std::endl;
            return false;
        }
        if (atlas_patch_padding < 0)
        {
            OSG_NOTICE << "[TextureMapping] atlas_patch_padding must be >= 0" << std::endl;
            return false;
        }
        if (inpaint_radius < 0)
        {
            OSG_NOTICE << "[TextureMapping] inpaint_radius must be >= 0" << std::endl;
            return false;
        }
        if (color_correction_regularization <= 0.0)
        {
            OSG_NOTICE << "[TextureMapping] color_correction_regularization must be > 0" << std::endl;
            return false;
        }
        if (texture_scale_factor <= 0.0)
        {
            OSG_NOTICE << "[TextureMapping] texture_scale_factor must be > 0" << std::endl;
            return false;
        }
        return true;
    }

    namespace
    {
        osg::Vec3f getVertex(const TextureMapping::Mesh& mesh, size_t idx)
        { return mesh.vertices[idx].toVec3(); }

        std::array<size_t, 3> getFaceIndices(const TextureMapping::Face& face)
        { return {face.v0, face.v1, face.v2}; }

        inline uint64_t edgeKey(size_t a, size_t b)
        {
            if (a > b) std::swap(a, b);
            return (static_cast<uint64_t>(a) << 32) | static_cast<uint64_t>(b);
        }
    }

    osg::Vec2f TextureMapping::projectPoint(const float* P, const osg::Vec3f& point)
    {
        Eigen::Map<const Eigen::Matrix<float, 3, 4, Eigen::RowMajor>> P_m(P);
        Eigen::Vector4f ph(point.x(), point.y(), point.z(), 1.0f);
        Eigen::Vector3f proj = P_m * ph;
        return osg::Vec2f(proj(0) / proj(2), proj(1) / proj(2));
    }

    float TextureMapping::projectPointDepth(const float* P, const osg::Vec3f& point)
    {
        Eigen::Map<const Eigen::Matrix<float, 3, 4, Eigen::RowMajor>> P_m(P);
        Eigen::Vector4f ph(point.x(), point.y(), point.z(), 1.0f);
        return (P_m.row(2) * ph)(0, 0);
    }

    std::vector<osg::Vec3f> TextureMapping::computeFaceNormals(const TextureMapping::Mesh& mesh)
    {
        const size_t num_faces = mesh.faces.size();
        std::vector<osg::Vec3f> normals(num_faces);
        for (size_t i = 0; i < num_faces; ++i)
        {
            const std::array<size_t, 3> idx = getFaceIndices(mesh.faces[i]);
            const osg::Vec3f v0 = getVertex(mesh, idx[0]);
            const osg::Vec3f v1 = getVertex(mesh, idx[1]);
            const osg::Vec3f v2 = getVertex(mesh, idx[2]);
            const osg::Vec3f n = (v1 - v0) ^ (v2 - v0);
            const float len = n.length();
            if (len > 1e-10f) normals[i] = n / len;
            else normals[i] = osg::Vec3f(0, 0, 0);
        }
        return normals;
    }

    TextureMapping::FaceAdjacencyMap TextureMapping::buildFaceAdjacency(const TextureMapping::Mesh& mesh)
    {
        const size_t num_faces = mesh.faces.size();
        std::unordered_map<uint64_t, std::vector<size_t>> edge_to_faces;
        edge_to_faces.reserve(num_faces * 3);

        for (size_t fi = 0; fi < num_faces; ++fi)
        {
            const std::array<size_t, 3> idx = getFaceIndices(mesh.faces[fi]);
            for (int e = 0; e < 3; ++e)
            {
                const uint64_t key = edgeKey(idx[e], idx[(e + 1) % 3]);
                edge_to_faces[key].push_back(fi);
            }
        }

        TextureMapping::FaceAdjacencyMap adjacency(num_faces);
        for (const auto& pair : edge_to_faces)
        {
            const std::vector<size_t>& face_list = pair.second;
            if (face_list.size() == 2)
            {
                adjacency[face_list[0]].push_back(face_list[1]);
                adjacency[face_list[1]].push_back(face_list[0]);
            }
        }

        for (auto& neighbors : adjacency)
        {
            std::sort(neighbors.begin(), neighbors.end());
            neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());
        }
        return adjacency;
    }

    namespace
    {
        bool rayTriangleIntersect(const osg::Vec3f& orig, const osg::Vec3f& dir,
                                  const osg::Vec3f& v0, const osg::Vec3f& v1,
                                  const osg::Vec3f& v2, float& t)
        {
            const float EPSILON = 1e-6f;
            osg::Vec3f edge1 = v1 - v0;
            osg::Vec3f edge2 = v2 - v0;
            osg::Vec3f h = dir ^ edge2;
            float a = edge1 * h;
            if (std::abs(a) < EPSILON) return false;

            float f = 1.0f / a;
            osg::Vec3f s = orig - v0;
            float u = f * (s * h);
            if (u < 0.0f || u > 1.0f) return false;

            osg::Vec3f q = s ^ edge1;
            float v = f * (dir * q);
            if (v < 0.0f || u + v > 1.0f) return false;
            t = f * (edge2 * q);
            return t > EPSILON;
        }

        struct OcclusionTester
        {
            const TextureMapping::Mesh* mesh = nullptr;
            void build(const TextureMapping::Mesh& m) { mesh = &m; }

            bool isOccluded(const osg::Vec3f& camera_center,
                            const osg::Vec3f& vertex, size_t face_idx) const
            {
                if (!mesh) return false;
                constexpr float kEps = 1e-4f;
                osg::Vec3f dir = vertex - camera_center;
                float dist = dir.length();
                if (dist < kEps) return false;

                dir /= dist;
                for (size_t i = 0; i < mesh->faces.size(); ++i)
                {
                    if (i == face_idx) continue;
                    const TextureMapping::Face& face = mesh->faces[i];
                    osg::Vec3f v0 = getVertex(*mesh, face.v0);
                    osg::Vec3f v1 = getVertex(*mesh, face.v1);
                    osg::Vec3f v2 = getVertex(*mesh, face.v2);

                    osg::Vec3f cross = (v1 - v0) ^ (v2 - v0);
                    float t = 0.0f; if (cross.length2() < 1e-20f) continue;
                    if (rayTriangleIntersect(camera_center, dir, v0, v1, v2, t))
                    { if (t < dist - kEps) return true; }
                }
                return false;
            }
        };

        struct FaceRegion
        {
            int view_id = -1;
            std::vector<size_t> face_ids;
        };

        struct PackRect
        {
            int x = 0;
            int y = 0;
            int width = 0;
            int height = 0;
            size_t region_idx = 0;
        };

        struct RegionProjection
        {
            std::vector<std::array<osg::Vec2f, 3>> face_projections;
            int bbox_x = 0;
            int bbox_y = 0;
            int bbox_width = 0;
            int bbox_height = 0;
        };

        struct AtlasLayout
        {
            int atlas_width = 0;
            int atlas_height = 0;
            std::vector<PackRect> placements;
        };
    }  // namespace

    std::vector<int> selectViews(const TextureMapping::Mesh& mesh,
                                 const std::vector<osg::Vec3f>& face_normals,
                                 const std::vector<TextureMapping::Image>& images,
                                 const TextureMapping::FaceAdjacencyMap& adjacency,
                                 const TextureMapping::MeshTextureMappingOptions& options)
    {
        const size_t num_faces = mesh.faces.size();
        const size_t num_images = images.size();
        if (num_faces == 0 || num_images == 0)
            return std::vector<int>(num_faces, -1);

        OcclusionTester occlusion_tester;
        if (options.enable_occlusion_testing)
            occlusion_tester.build(mesh);

        std::vector<double> scores(num_faces * num_images, -1.0);
#ifdef _OPENMP
        const int num_threads = options.num_threads > 0
            ? options.num_threads : std::max(1, static_cast<int>(omp_get_max_threads()));
#pragma omp parallel for schedule(dynamic) num_threads(num_threads)
#endif
        // Check every faces and images
        for (int64_t fi = 0; fi < static_cast<int64_t>(num_faces); ++fi)
        {
            const osg::Vec3f& normal = face_normals[fi];
            if (normal.length2() < 1e-10f) continue;

            const std::array<size_t, 3> idx = getFaceIndices(mesh.faces[fi]);
            const osg::Vec3f v0 = getVertex(mesh, idx[0]);
            const osg::Vec3f v1 = getVertex(mesh, idx[1]);
            const osg::Vec3f v2 = getVertex(mesh, idx[2]);
            const osg::Vec3f centroid = (v0 + v1 + v2) / 3.0f;
            const std::array<osg::Vec3f, 3> verts = { v0, v1, v2 };

            for (size_t ii = 0; ii < num_images; ++ii)
            {
                const TextureMapping::Image& img = images[ii];
                const osg::Vec3f cam_center = img.getCameraCenter();
                osg::Vec3f view_dir = cam_center - centroid; view_dir.normalize();

                float cos_angle = normal * view_dir;
                if (cos_angle < static_cast<float>(options.min_cos_normal_angle)) continue;

                int visible_count = 0; bool behind_camera = false;
                std::array<osg::Vec2f, 3> proj;
                for (int vi = 0; vi < 3; ++vi)
                {
                    const float depth = TextureMapping::projectPointDepth(img.getP(), verts[vi]);
                    if (depth <= 0) { behind_camera = true; break; }
                    proj[vi] = TextureMapping::projectPoint(img.getP(), verts[vi]);
                    if (proj[vi].x() >= 0 && proj[vi].x() < static_cast<float>(img.getWidth()) &&
                        proj[vi].y() >= 0 && proj[vi].y() < static_cast<float>(img.getHeight()))
                    { ++visible_count; }
                }

                if (behind_camera) continue;
                if (visible_count < options.min_visible_vertices) continue;
                if (options.enable_occlusion_testing)
                {
                    bool occluded = false;
                    for (int vi = 0; vi < 3; ++vi)
                    {
                        if (occlusion_tester.isOccluded(cam_center, verts[vi], fi))
                        { occluded = true; break; }
                    }
                    if (occluded) continue;
                }

                const osg::Vec2f e1 = proj[1] - proj[0];
                const osg::Vec2f e2 = proj[2] - proj[0];
                const double area = std::abs(static_cast<double>(e1.x()) * static_cast<double>(e2.y()) -
                                             static_cast<double>(e1.y()) * static_cast<double>(e2.x()));
                scores[fi * num_images + ii] = area;
            }
        }

        // Find best view for each face
        std::vector<int> view_per_face(num_faces, -1);
        for (size_t fi = 0; fi < num_faces; ++fi)
        {
            double best_score = -1.0;
            for (size_t ii = 0; ii < num_images; ++ii)
            {
                const double s = scores[fi * num_images + ii];
                if (s > best_score) { best_score = s; view_per_face[fi] = static_cast<int>(ii); }
            }
        }

        // Smoothing iterations
        for (int iter = 0; iter < options.view_selection_smoothing_iterations; ++iter)
        {
            std::vector<int> new_views = view_per_face;
            for (size_t fi = 0; fi < num_faces; ++fi)
            {
                if (view_per_face[fi] < 0) continue;
                std::unordered_map<int, int> label_counts;
                for (const size_t ni : adjacency[fi])
                {
                    if (view_per_face[ni] >= 0)
                        ++label_counts[view_per_face[ni]];
                }

                int best_label = view_per_face[fi];
                int best_count = label_counts.count(best_label) ? label_counts[best_label] : 0;
                for (const auto& pair : label_counts)
                {
                    const int label = pair.first; int count = pair.second;
                    if (count > best_count && scores[fi * num_images + label] > 0)
                    { best_count = count; best_label = label; }
                }
                new_views[fi] = best_label;
            }
            view_per_face = new_views;
        }
        return view_per_face;
    }

    std::vector<FaceRegion> extractFaceRegions(const std::vector<int>& view_per_face,
                                               const TextureMapping::FaceAdjacencyMap& adjacency, size_t num_faces)
    {
        std::vector<bool> visited(num_faces, false);
        std::vector<FaceRegion> regions;
        for (size_t fi = 0; fi < num_faces; ++fi)
        {
            if (visited[fi] || view_per_face[fi] < 0) continue;
            FaceRegion region; region.view_id = view_per_face[fi];
            std::queue<size_t> queue; queue.push(fi);
            visited[fi] = true;

            while (!queue.empty())
            {
                const size_t current = queue.front(); queue.pop();
                region.face_ids.push_back(current);
                for (const size_t ni : adjacency[current])
                {
                    if (!visited[ni] && view_per_face[ni] == region.view_id)
                    { visited[ni] = true; queue.push(ni); }
                }
            }
            regions.push_back(std::move(region));
        }
        return regions;
    }

    std::vector<RegionProjection> computeRegionProjections(
            const TextureMapping::Mesh& mesh,
            const std::vector<FaceRegion>& regions,
            const std::vector<TextureMapping::Image>& images)
    {
        std::vector<RegionProjection> projections(regions.size());
        for (size_t ri = 0; ri < regions.size(); ++ri)
        {
            const FaceRegion& region = regions[ri];
            const TextureMapping::Image& img = images[region.view_id];
            RegionProjection& rp = projections[ri];
            rp.face_projections.resize(region.face_ids.size());

            float min_x = std::numeric_limits<float>::max();
            float min_y = std::numeric_limits<float>::max();
            float max_x = std::numeric_limits<float>::lowest();
            float max_y = std::numeric_limits<float>::lowest();
            for (size_t i = 0; i < region.face_ids.size(); ++i)
            {
                const size_t fi = region.face_ids[i];
                const std::array<size_t, 3> idx = getFaceIndices(mesh.faces[fi]);
                for (int vi = 0; vi < 3; ++vi)
                {
                    const osg::Vec3f v = getVertex(mesh, idx[vi]);
                    const osg::Vec2f p = TextureMapping::projectPoint(img.getP(), v);
                    rp.face_projections[i][vi] = p;
                    min_x = osg::minimum(min_x, p.x());
                    min_y = osg::minimum(min_y, p.y());
                    max_x = std::max(max_x, p.x());
                    max_y = std::max(max_y, p.y());
                }
            }

            rp.bbox_x = static_cast<int>(std::floor(min_x));
            rp.bbox_y = static_cast<int>(std::floor(min_y));
            rp.bbox_width = static_cast<int>(std::ceil(max_x)) - rp.bbox_x + 1;
            rp.bbox_height = static_cast<int>(std::ceil(max_y)) - rp.bbox_y + 1;
        }
        return projections;
    }

    void scaleRegionProjections(std::vector<RegionProjection>& projections, double scale)
    {
        float sf = static_cast<float>(scale);
        for (auto& rp : projections)
        {
            for (auto& fp : rp.face_projections)
            { for (auto& p : fp) p *= sf; }

            float min_x = std::numeric_limits<float>::max();
            float min_y = std::numeric_limits<float>::max();
            float max_x = std::numeric_limits<float>::lowest();
            float max_y = std::numeric_limits<float>::lowest();
            for (const auto& fp : rp.face_projections)
                for (const auto& p : fp)
                {
                    min_x = osg::minimum(min_x, p.x());
                    min_y = osg::minimum(min_y, p.y());
                    max_x = std::max(max_x, p.x());
                    max_y = std::max(max_y, p.y());
                }

            rp.bbox_x = static_cast<int>(std::floor(min_x));
            rp.bbox_y = static_cast<int>(std::floor(min_y));
            rp.bbox_width = static_cast<int>(std::ceil(max_x)) - rp.bbox_x + 1;
            rp.bbox_height = static_cast<int>(std::ceil(max_y)) - rp.bbox_y + 1;
        }
    }

    AtlasLayout packAtlas(const std::vector<RegionProjection>& projections,
                          const std::vector<FaceRegion>& regions, int padding)
    {
        AtlasLayout layout;
        if (projections.empty()) return layout;

        struct RectEntry
        {
            int width, height;
            size_t region_idx;
        };
        std::vector<RectEntry> rects(projections.size());

        int max_rect_width = 0, atlas_width = 1;
        int64_t total_area = 0;
        for (size_t i = 0; i < projections.size(); ++i)
        {
            rects[i].width = projections[i].bbox_width + 2 * padding;
            rects[i].height = projections[i].bbox_height + 2 * padding;
            rects[i].region_idx = i;
            total_area += static_cast<int64_t>(rects[i].width) * rects[i].height;
            max_rect_width = std::max(max_rect_width, rects[i].width);
        }

        std::sort(rects.begin(), rects.end(), [](const RectEntry& a, const RectEntry& b)
            { return a.height > b.height; });

        const int atlas_side = std::max(static_cast<int>(std::ceil(std::sqrt(total_area * 1.3))), max_rect_width);
        while (atlas_width < atlas_side) atlas_width *= 2;
        int atlas_height = atlas_width;

        // Shelf packing
        auto TryPack = [&](int aw, int ah, std::vector<PackRect>& placements_out) -> bool
            {
                placements_out.resize(rects.size());
                int shelf_x = 0, shelf_y = 0, shelf_height = 0;
                for (size_t i = 0; i < rects.size(); ++i)
                {
                    if (shelf_x + rects[i].width > aw)
                    {
                        shelf_y += shelf_height;
                        shelf_x = 0;
                        shelf_height = 0;
                    }
                    if (shelf_y + rects[i].height > ah) return false;
                    placements_out[i].x = shelf_x + padding;
                    placements_out[i].y = shelf_y + padding;
                    placements_out[i].width = projections[rects[i].region_idx].bbox_width;
                    placements_out[i].height = projections[rects[i].region_idx].bbox_height;
                    placements_out[i].region_idx = rects[i].region_idx;
                    shelf_x += rects[i].width; shelf_height = std::max(shelf_height, rects[i].height);
                }
                return true;
            };

        std::vector<PackRect> placements;
        constexpr int kMaxAtlasDim = 1 << 16;  // 65536
        while (!TryPack(atlas_width, atlas_height, placements))
        {
            if (atlas_width > kMaxAtlasDim)
            {
                OSG_NOTICE << "[TextureMapping] Atlas dimensions exceeded maximum (" << kMaxAtlasDim << ")" << std::endl;
                break;
            }
            atlas_width *= 2; atlas_height *= 2;
        }

        int max_used_y = 0;
        for (const PackRect& p : placements)
            max_used_y = std::max(max_used_y, p.y + p.height + padding);
        atlas_height = std::max(max_used_y, 1);

        layout.placements.resize(projections.size());
        for (const PackRect& p : placements)
            layout.placements[p.region_idx] = p;
        layout.atlas_width = atlas_width;
        layout.atlas_height = atlas_height;
        return layout;
    }

    std::vector<float> computeFaceUVs(
            const std::vector<FaceRegion>& regions,
            const std::vector<RegionProjection>& projections,
            const AtlasLayout& layout, size_t num_faces)
    {
        std::vector<float> uvs(num_faces * 6, 0.0f);
        float inv_atlas_width = 1.0f / static_cast<float>(layout.atlas_width);
        float inv_atlas_height = 1.0f / static_cast<float>(layout.atlas_height);
        for (size_t ri = 0; ri < regions.size(); ++ri)
        {
            const FaceRegion& region = regions[ri];
            const RegionProjection& rp = projections[ri];
            const PackRect& placement = layout.placements[ri];
            for (size_t i = 0; i < region.face_ids.size(); ++i)
            {
                const size_t fi = region.face_ids[i];
                for (int vi = 0; vi < 3; ++vi)
                {
                    const osg::Vec2f& proj = rp.face_projections[i][vi];
                    float atlas_x = proj.x() - rp.bbox_x + placement.x;
                    float atlas_y = proj.y() - rp.bbox_y + placement.y;
                    uvs[fi * 6 + vi * 2 + 0] = atlas_x * inv_atlas_width;
                    uvs[fi * 6 + vi * 2 + 1] = 1.0f - atlas_y * inv_atlas_height;
                }
            }
        }
        return uvs;
    }

    osg::Vec3f barycentric(const osg::Vec2f& P,
                           const osg::Vec2f& A,
                           const osg::Vec2f& B,
                           const osg::Vec2f& C)
    {
        osg::Vec2f v0 = B - A;
        osg::Vec2f v1 = C - A;
        osg::Vec2f v2 = P - A;
        float d00 = v0 * v0, d01 = v0 * v1, d11 = v1 * v1;
        float d20 = v2 * v0, d21 = v2 * v1;
        float denom = d00 * d11 - d01 * d01;
        if (std::abs(denom) < 1e-10f) return osg::Vec3f(-1, -1, -1);

        float v = (d11 * d20 - d01 * d21) / denom;
        float w = (d00 * d21 - d01 * d20) / denom;
        float u = 1.0f - v - w;
        return osg::Vec3f(u, v, w);
    }

    std::array<osg::Vec2f, 3> computeAtlasVerts(const RegionProjection& rp,
                                                const PackRect& placement, size_t face_in_region)
    {
        std::array<osg::Vec2f, 3> atlas_verts;
        for (int vi = 0; vi < 3; ++vi)
        {
            const osg::Vec2f& proj = rp.face_projections[face_in_region][vi];
            atlas_verts[vi].x() = proj.x() - rp.bbox_x + placement.x;
            atlas_verts[vi].y() = proj.y() - rp.bbox_y + placement.y;
        }
        return atlas_verts;
}

    void bakeTexture(TextureMapping::Bitmap* atlas, std::vector<bool>* baked_mask,
                     const TextureMapping::Mesh& mesh, const std::vector<FaceRegion>& regions,
                     const std::vector<RegionProjection>& projections, const AtlasLayout& layout,
                     const std::vector<TextureMapping::Image>& images,
                     const TextureMapping::MeshTextureMappingOptions& options)
    {
        int aw = layout.atlas_width;
        int ah = layout.atlas_height;
        baked_mask->assign(static_cast<size_t>(aw) * ah, false);
        for (size_t ri = 0; ri < regions.size(); ++ri)
        {
            const FaceRegion& region = regions[ri];
            const RegionProjection& rp = projections[ri];
            const PackRect& placement = layout.placements[ri];
            const TextureMapping::Image& img = images[region.view_id];
            const TextureMapping::Bitmap& src_bmp = img.getBitmap();
            for (size_t i = 0; i < region.face_ids.size(); ++i)
            {
                const std::array<osg::Vec2f, 3> atlas_verts = computeAtlasVerts(rp, placement, i);
                int min_px = std::max(0,
                    static_cast<int>(std::floor(std::min({ atlas_verts[0].x(), atlas_verts[1].x(), atlas_verts[2].x() }))) - 1);
                int min_py = std::max(0,
                    static_cast<int>(std::floor(std::min({ atlas_verts[0].y(), atlas_verts[1].y(), atlas_verts[2].y() }))) - 1);
                int max_px = std::min(aw - 1,
                    static_cast<int>(std::ceil(std::max({ atlas_verts[0].x(), atlas_verts[1].x(), atlas_verts[2].x() }))) + 1);
                int max_py = std::min(ah - 1,
                    static_cast<int>(std::ceil(std::max({ atlas_verts[0].y(), atlas_verts[1].y(), atlas_verts[2].y() }))) + 1);

                float texture_inv_scale_factor = static_cast<float>(1.0 / options.texture_scale_factor);
                for (int py = min_py; py <= max_py; ++py)
                    for (int px = min_px; px <= max_px; ++px)
                    {
                        osg::Vec2f pixel_center(px + 0.5f, py + 0.5f);
                        osg::Vec3f bary = barycentric(pixel_center, atlas_verts[0], atlas_verts[1], atlas_verts[2]);
                        float min_bary = std::min({ bary.x(), bary.y(), bary.z() });
                        if (min_bary < -1e-4f) continue;

                        osg::Vec2f img_pos = (rp.face_projections[i][0] * bary.x() +
                                              rp.face_projections[i][1] * bary.y() +
                                              rp.face_projections[i][2] * bary.z()) * texture_inv_scale_factor;

                        TextureMapping::Colorf color;
                        if (!src_bmp.interpolateBilinear(static_cast<double>(img_pos.x()),
                                                         static_cast<double>(img_pos.y()), &color)) { continue; }
                        atlas->setPixel(px, py, color.cast<unsigned char>());
                        (*baked_mask)[static_cast<size_t>(py) * aw + px] = true;
                    }
            }  // for (size_t i = 0...
        }
    }

    void applyGlobalColorCorrection(
            TextureMapping::Bitmap* atlas, const TextureMapping::Mesh& mesh,
            const std::vector<FaceRegion>& regions, const std::vector<RegionProjection>& projections,
            const AtlasLayout& layout, const std::vector<TextureMapping::Image>& images,
            const TextureMapping::FaceAdjacencyMap& adjacency, const std::vector<int>& view_per_face,
            const std::vector<bool>& baked_mask, const TextureMapping::MeshTextureMappingOptions& options)
    {
        struct SeamEdge
        {
            size_t face_l;
            size_t face_r;
            size_t vert_a;
            size_t vert_b;
        };
        size_t num_faces = mesh.faces.size();
        std::vector<SeamEdge> seam_edges;
        seam_edges.reserve(num_faces);

        for (size_t fi = 0; fi < num_faces; ++fi)
        {
            if (view_per_face[fi] < 0) continue;
            const std::array<size_t, 3> idx = getFaceIndices(mesh.faces[fi]);
            for (int e = 0; e < 3; ++e)
            {
                const size_t va = idx[e];
                const size_t vb = idx[(e + 1) % 3];
                const uint64_t ekey = edgeKey(va, vb);
                for (const size_t ni : adjacency[fi])
                {
                    if (ni <= fi) continue;
                    if (view_per_face[ni] < 0) continue;
                    if (view_per_face[ni] == view_per_face[fi]) continue;
                    const std::array<size_t, 3> nidx = getFaceIndices(mesh.faces[ni]);

                    bool shares_edge = false;
                    for (int ne = 0; ne < 3; ++ne)
                    {
                        if (edgeKey(nidx[ne], nidx[(ne + 1) % 3]) == ekey)
                        { shares_edge = true; break; }
                    }
                    if (shares_edge)
                        seam_edges.push_back({ fi, ni, va, vb });
                }
            }
        }
        if (seam_edges.empty()) return;

        struct RegionVertexMap { std::unordered_map<size_t, size_t> vert_to_var; };
        std::vector<RegionVertexMap> region_vert_maps(regions.size());

        std::vector<int> face_to_region(num_faces, -1); size_t total_vars = 0;
        for (size_t ri = 0; ri < regions.size(); ++ri)
            for (const size_t fi : regions[ri].face_ids)
            {
                face_to_region[fi] = static_cast<int>(ri);
                const std::array<size_t, 3> idx = getFaceIndices(mesh.faces[fi]);
                for (int vi = 0; vi < 3; ++vi)
                {
                    if (region_vert_maps[ri].vert_to_var.count(idx[vi]) == 0)
                        region_vert_maps[ri].vert_to_var[idx[vi]] = total_vars++;
                }
            }
        if (total_vars == 0) return;

        int aw = layout.atlas_width, ah = layout.atlas_height;
        double beta = options.color_correction_regularization;
        std::vector<std::array<double, 3>> offsets(total_vars, { 0.0, 0.0, 0.0 });
        size_t estimated_triplets = seam_edges.size() * 8 + total_vars;
        for (int ch = 0; ch < 3; ++ch)
        {
            std::vector<Eigen::Triplet<double>> triplets;
            triplets.reserve(estimated_triplets);
            Eigen::VectorXd rhs = Eigen::VectorXd::Zero(total_vars);
            for (const SeamEdge& se : seam_edges)
            {
                int ri_l = face_to_region[se.face_l];
                int ri_r = face_to_region[se.face_r];
                if (ri_l < 0 || ri_r < 0) continue;

                for (const size_t sv : {se.vert_a, se.vert_b})
                {
                    auto it_l = region_vert_maps[ri_l].vert_to_var.find(sv);
                    auto it_r = region_vert_maps[ri_r].vert_to_var.find(sv);
                    if (it_l == region_vert_maps[ri_l].vert_to_var.end() ||
                        it_r == region_vert_maps[ri_r].vert_to_var.end())
                    { continue; }

                    osg::Vec3f vert = getVertex(mesh, sv);
                    size_t var_l = it_l->second, var_r = it_r->second;
                    const TextureMapping::Image& img_l = images[regions[ri_l].view_id];
                    const TextureMapping::Image& img_r = images[regions[ri_r].view_id];
                    osg::Vec2f proj_l = TextureMapping::projectPoint(img_l.getP(), vert);
                    osg::Vec2f proj_r = TextureMapping::projectPoint(img_r.getP(), vert);

                    TextureMapping::Colorf color_l, color_r;
                    if (!img_l.getBitmap().interpolateBilinear(proj_l.x(), proj_l.y(), &color_l) ||
                        !img_r.getBitmap().interpolateBilinear(proj_r.x(), proj_r.y(), &color_r))
                    { continue; }

                    double f_l = (ch == 0) ? color_l.r : (ch == 1) ? color_l.g : color_l.b;
                    double f_r = (ch == 0) ? color_r.r : (ch == 1) ? color_r.g : color_r.b;
                    triplets.emplace_back(var_l, var_l, 1.0);
                    triplets.emplace_back(var_r, var_r, 1.0);
                    triplets.emplace_back(var_l, var_r, -1.0);
                    triplets.emplace_back(var_r, var_l, -1.0);
                    rhs(var_l) += (f_r - f_l);
                    rhs(var_r) += (f_l - f_r);
                }
            }

            for (size_t i = 0; i < total_vars; ++i)
                triplets.emplace_back(i, i, beta);
            Eigen::SparseMatrix<double> A(total_vars, total_vars);
            A.setFromTriplets(triplets.begin(), triplets.end());

            Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> solver; solver.compute(A);
            if (solver.info() != Eigen::Success)
            {
                OSG_NOTICE << "[TextureMapping] Color correction: failed to factorize system" << std::endl;
                return;
            }

            Eigen::VectorXd x = solver.solve(rhs);
            if (solver.info() != Eigen::Success)
            {
                OSG_NOTICE << "[TextureMapping] Color correction: failed to solve system" << std::endl;
                return;
            }
            for (size_t i = 0; i < total_vars; ++i)
                offsets[i][ch] = x(i);
        }  // for (int ch = 0; ch < 3; ++ch)

        // Apply offsets to atlas
        for (size_t ri = 0; ri < regions.size(); ++ri)
        {
            const FaceRegion& region = regions[ri];
            const RegionProjection& rp = projections[ri];
            const PackRect& placement = layout.placements[ri];
            for (size_t i = 0; i < region.face_ids.size(); ++i)
            {
                const size_t fi = region.face_ids[i];
                const std::array<size_t, 3> idx = getFaceIndices(mesh.faces[fi]);
                std::array<std::array<double, 3>, 3> vert_offsets;
                for (int vi = 0; vi < 3; ++vi)
                {
                    size_t var_id = region_vert_maps[ri].vert_to_var[idx[vi]];
                    vert_offsets[vi] = offsets[var_id];
                }

                const std::array<osg::Vec2f, 3> atlas_verts = computeAtlasVerts(rp, placement, i);
                int min_px = std::max(0,
                    static_cast<int>(std::floor(std::min({ atlas_verts[0].x(), atlas_verts[1].x(), atlas_verts[2].x() }))));
                int min_py = std::max(0,
                    static_cast<int>(std::floor(std::min({ atlas_verts[0].y(), atlas_verts[1].y(), atlas_verts[2].y() }))));
                int max_px = std::min(aw - 1,
                    static_cast<int>(std::ceil(std::max({ atlas_verts[0].x(), atlas_verts[1].x(), atlas_verts[2].x() }))));
                int max_py = std::min(ah - 1,
                    static_cast<int>(std::ceil(std::max({ atlas_verts[0].y(), atlas_verts[1].y(), atlas_verts[2].y() }))));

                for (int py = min_py; py <= max_py; ++py)
                    for (int px = min_px; px <= max_px; ++px)
                    {
                        if (!baked_mask[static_cast<size_t>(py) * aw + px]) continue;
                        osg::Vec2f pixel_center(px + 0.5f, py + 0.5f);
                        osg::Vec3f bary = barycentric(pixel_center, atlas_verts[0], atlas_verts[1], atlas_verts[2]);
                        if (bary.x() < -0.01f || bary.y() < -0.01f || bary.z() < -0.01f) continue;

                        std::array<double, 3> offset_interp = { 0, 0, 0 };
                        for (int c = 0; c < 3; ++c)
                        {
                            offset_interp[c] = bary.x() * vert_offsets[0][c] +
                                               bary.y() * vert_offsets[1][c] +
                                               bary.z() * vert_offsets[2][c];
                        }

                        TextureMapping::Colorub color;
                        atlas->getPixel(px, py, &color);
                        color.r = static_cast<unsigned char>(
                            std::max(0.0, std::min(255.0, color.r + offset_interp[0])));
                        color.g = static_cast<unsigned char>(
                            std::max(0.0, std::min(255.0, color.g + offset_interp[1])));
                        color.b = static_cast<unsigned char>(
                            std::max(0.0, std::min(255.0, color.b + offset_interp[2])));
                        atlas->setPixel(px, py, color);
                    }
            }
        }  // for (size_t ri = 0...
    }

    void inpaintAtlas(TextureMapping::Bitmap* atlas,
                      const std::vector<bool>& baked_mask, int inpaint_radius)
    {
        int aw = atlas->width(), ah = atlas->height();
        if (inpaint_radius <= 0 || aw == 0 || ah == 0) return;

        size_t num_pixels = static_cast<size_t>(aw) * ah;
        std::vector<int> dist(num_pixels, std::numeric_limits<int>::max());
        std::vector<TextureMapping::Colorub> fill_colors(num_pixels);
        std::queue<std::pair<int, int>> queue;
        for (int y = 0; y < ah; ++y)
            for (int x = 0; x < aw; ++x)
            {
                size_t idx = static_cast<size_t>(y) * aw + x;
                if (baked_mask[idx])
                {
                    dist[idx] = 0;
                    atlas->getPixel(x, y, &fill_colors[idx]);
                    queue.push({ x, y });
                }
            }

        constexpr std::array<int, 4> kDx = { -1, 1, 0, 0 };
        constexpr std::array<int, 4> kDy = { 0, 0, -1, 1 };
        while (!queue.empty())
        {
            const auto& pair = queue.front();
            int cx = pair.first, cy = pair.second;
            queue.pop();

            size_t cidx = static_cast<size_t>(cy) * aw + cx;
            int cdist = dist[cidx];
            if (cdist >= inpaint_radius) continue;

            for (int d = 0; d < 4; ++d)
            {
                int nx = cx + kDx[d], ny = cy + kDy[d];
                if (nx < 0 || nx >= aw || ny < 0 || ny >= ah) continue;
                size_t nidx = static_cast<size_t>(ny) * aw + nx;
                if (dist[nidx] <= cdist + 1) continue;
                dist[nidx] = cdist + 1;
                fill_colors[nidx] = fill_colors[cidx];
                queue.push({ nx, ny });
            }
        }

        for (int y = 0; y < ah; ++y)
            for (int x = 0; x < aw; ++x)
            {
                size_t idx = static_cast<size_t>(y) * aw + x;
                if (!baked_mask[idx] && dist[idx] <= inpaint_radius)
                    atlas->setPixel(x, y, fill_colors[idx]);
            }
    }

    TextureMapping::MeshTextureMappingResult process(
        const TextureMapping::Mesh& mesh,
        const std::vector<TextureMapping::Image>& images,
        const TextureMapping::MeshTextureMappingOptions& options)
    {

        TextureMapping::MeshTextureMappingResult result;
        if (!options.check()) return result;

        if (mesh.faces.empty() || mesh.vertices.empty())
        {
            result.face_view_ids.assign(mesh.faces.size(), -1);
            result.face_uvs.assign(mesh.faces.size() * 6, 0.0f);
            return result;
        }

        if (!options.enable_occlusion_testing)
        {
            OSG_NOTICE << "[TextureMapping] Occlusion testing disabled; some faces may be textured from "
                       << "views where they are occluded by other geometry." << std::endl;
        }

        COUT << "Computing face normals..." << std::endl;
        const std::vector<osg::Vec3f> face_normals = TextureMapping::computeFaceNormals(mesh);

        COUT << "Building face adjacency..." << std::endl;
        const TextureMapping::FaceAdjacencyMap adjacency = TextureMapping::buildFaceAdjacency(mesh);

        COUT << "Selecting views for " << mesh.faces.size() << " faces from " << images.size() << " images..." << std::endl;
        const std::vector<int> view_per_face = selectViews(mesh, face_normals, images, adjacency, options);
        result.face_view_ids = view_per_face;

        size_t num_assigned = std::count_if(view_per_face.begin(), view_per_face.end(), [](int v) { return v >= 0; });
        if (num_assigned == 0)
        {
            OSG_NOTICE << "[TextureMapping] No faces were assigned to any view" << std::endl;
            result.face_uvs.assign(mesh.faces.size() * 6, 0.0f); return result;
        }
        COUT << num_assigned << " / " << mesh.faces.size() << " faces assigned to views" << std::endl;
        COUT << "Extracting face regions..." << std::endl;

        const std::vector<FaceRegion> regions = extractFaceRegions(view_per_face, adjacency, mesh.faces.size());
        COUT << "Found " << regions.size() << " regions" << std::endl;

        COUT << "Computing region projections..." << std::endl;
        std::vector<RegionProjection> projections = computeRegionProjections(mesh, regions, images);
        if (options.texture_scale_factor != 1.0)
        {
            COUT << "Scaling region projections by factor " << options.texture_scale_factor << "..." << std::endl;
            scaleRegionProjections(projections, options.texture_scale_factor);
        }

        COUT << "Packing texture atlas..." << std::endl;
        const AtlasLayout layout = packAtlas(projections, regions, options.atlas_patch_padding);
        result.atlas_width = layout.atlas_width;
        result.atlas_height = layout.atlas_height;

        COUT << "Atlas size: " << layout.atlas_width << " x " << layout.atlas_height << std::endl;
        COUT << "Computing face UVs..." << std::endl;
        result.face_uvs = computeFaceUVs(regions, projections, layout, mesh.faces.size());

        COUT << "Baking texture..." << std::endl;
        result.texture_atlas = TextureMapping::Bitmap(layout.atlas_width, layout.atlas_height, true);
        result.texture_atlas.fill(TextureMapping::Colorub(0, 0, 0));

        std::vector<bool> baked_mask;
        bakeTexture(&result.texture_atlas, &baked_mask, mesh, regions, projections, layout, images, options);
        if (options.apply_color_correction)
        {
            COUT << "Applying global color correction..." << std::endl;
            applyGlobalColorCorrection(&result.texture_atlas, mesh, regions, projections,
                                       layout, images, adjacency, view_per_face, baked_mask, options);
        }

        if (options.inpaint_radius > 0)
        {
            COUT << "Inpainting atlas..." << std::endl;
            inpaintAtlas(&result.texture_atlas, baked_mask, options.inpaint_radius);
        }
        COUT << "Surface texture mapping complete" << std::endl;
        return result;
    }

}  // namesapce osgVerse
