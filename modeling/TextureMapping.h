#ifndef MANA_MODELING_TEXTUREMAPPING
#define MANA_MODELING_TEXTUREMAPPING

#include <osg/Image>
#include <osg/Vec2>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/Matrix>
#include <vector>
#include <string>
#include <memory>

namespace osgVerse
{

    struct TextureMapping
    {
        struct Vertex
        {
            float x, y, z;
            Vertex() : x(0), y(0), z(0) {}
            Vertex(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
  
            osg::Vec3 toVec3() const { return osg::Vec3(x, y, z); }
            static Vertex fromVec3(const osg::Vec3& v) { return Vertex(v.x(), v.y(), v.z()); }
        };

        struct Face
        {
            size_t v0, v1, v2;
            Face() : v0(0), v1(0), v2(0) {}
            Face(size_t a, size_t b, size_t c) : v0(a), v1(b), v2(c) {}
        };

        struct Mesh
        {
            std::vector<Vertex> vertices;
            std::vector<Face> faces;
            bool empty() const { return vertices.empty() || faces.empty(); }
            void clear() { vertices.clear(); faces.clear(); }

            static void convertFromColmap(std::vector<Vertex>& vertices);
        };

        template<typename T> struct Color
        {
            T r, g, b, a;
            Color() : r(0), g(0), b(0), a(255) {}
            Color(T r_, T g_, T b_) : r(r_), g(g_), b(b_), a(255) {}
            Color(T r_, T g_, T b_, T a_) : r(r_), g(g_), b(b_), a(a_) {}

            template<typename U> Color<U> cast() const
            {
                return cast_impl<U>(
                    std::integral_constant<bool, std::is_same<T, U>::value>(),
                    std::integral_constant<bool, std::is_floating_point<T>::value && std::is_integral<U>::value>(),
                    std::integral_constant<bool, std::is_integral<T>::value && std::is_floating_point<U>::value>());
            }

        private:
            template<typename U>
            Color<U> cast_impl(std::true_type, std::false_type, std::false_type) const
            { return Color<U>(r, g, b, a); }

            template<typename U>  // float -> int
            Color<U> cast_impl(std::false_type, std::true_type, std::false_type) const
            {
                return Color<U>(static_cast<U>(r * 255), static_cast<U>(g * 255),
                                static_cast<U>(b * 255), static_cast<U>(a * 255));
            }

            template<typename U>  // int -> float
            Color<U> cast_impl(std::false_type, std::false_type, std::true_type) const
            {
                return Color<U>(static_cast<U>(r) / 255.0f, static_cast<U>(g) / 255.0f,
                                static_cast<U>(b) / 255.0f, static_cast<U>(a) / 255.0f);
            }

            template<typename U>
            Color<U> cast_impl(std::false_type, std::false_type, std::false_type) const
            {
                return Color<U>(static_cast<U>(r), static_cast<U>(g),
                                static_cast<U>(b), static_cast<U>(a));
            }
        };
        using Colorf = Color<float>;
        using Colorub = Color<unsigned char>;

        class Bitmap
        {
        public:
            Bitmap();
            Bitmap(int width, int height, bool as_rgb = true);
            explicit Bitmap(osg::Image* image);
  
            bool read(const std::string& path);
            bool write(const std::string& path) const;
            int width() const;
            int height() const;
            int channels() const;

            template<typename T>
            bool getPixel(int x, int y, Color<T>* color) const;

            template<typename T>
            void setPixel(int x, int y, const Color<T>& color);

            bool interpolateBilinear(double x, double y, Colorf* color) const;
            void fill(const Colorub& color);

            osg::Image* getImage() { return image_.get(); }
            const osg::Image* getImage() const { return image_.get(); }

            unsigned char* data() { return image_->data(); }
            const unsigned char* data() const { return image_->data(); }
  
        private:
            osg::ref_ptr<osg::Image> image_;
            bool as_rgb_ = true;
        };

        struct Image {
            osg::Matrixf K;  // camera-intrinsics 3x3
            osg::Matrixf R;  // rotation
            osg::Vec3f T;    // translation

            osg::Matrixf P;  // Projection = K * [R | t]
            Bitmap bitmap;
            int width = 0;
            int height = 0;

            Image() = default;
            Image(const osg::Matrixf& K_, const osg::Matrixf& R_, 
                  const osg::Vec3f& T_, const Bitmap& bmp);
            void computeProjectionMatrix();
            osg::Vec3f getCameraCenter() const;

            int getWidth() const { return width; }
            int getHeight() const { return height; }
            const Bitmap& getBitmap() const { return bitmap; }
            const float* getP() const { return P.ptr(); }
            const float* getR() const { return R.ptr(); }
            const float* getT() const { return T.ptr(); }

            static void convertFromColmap(const osg::Matrixf& R_colmap, const osg::Vec3f& T_colmap,
                                          osg::Matrixf& R_osg, osg::Vec3f& T_osg);
        };

        struct MeshTextureMappingOptions
        {
            double min_cos_normal_angle = 0.1;  // cosine threshold between face-normal and view-dir
            int min_visible_vertices = 3;  // minimum vertex count projected into image
            int view_selection_smoothing_iterations = 3;  // neighbor smoothing iterations
            int atlas_patch_padding = 2;  // atlas texture padding
            int inpaint_radius = 5;  // extended pixels around baking area
            bool apply_color_correction = true;  // enable color correction (Waechter et al. 2014)
            double color_correction_regularization = 0.1;  // correction regularization
            int num_threads = -1;  // -1 means use all posible threads
            double texture_scale_factor = 1.0;  // atlas image resolution scale
            bool enable_occlusion_testing = true;

            bool check() const;
        };

        struct MeshTextureMappingResult
        {
            // face_uvs[face_idx * 6 + vertex_in_face * 2 + 0] = u
            // face_uvs[face_idx * 6 + vertex_in_face * 2 + 1] = v
            std::vector<float> face_uvs;
            std::vector<int> face_view_ids;

            Bitmap texture_atlas;
            int atlas_width = 0;
            int atlas_height = 0;
        };

        // Waechter, M., Moehrle, N., and Goesele, M.,
        // "Let there be color! Large-scale texturing of 3D reconstructions,"
        // European Conference on Computer Vision (ECCV), 2014.
        static MeshTextureMappingResult process(
            const Mesh& mesh, const std::vector<Image>& images, const MeshTextureMappingOptions& options);

        using FaceAdjacencyMap = std::vector<std::vector<size_t>>;
        static std::vector<osg::Vec3f> computeFaceNormals(const Mesh& mesh);
        static FaceAdjacencyMap buildFaceAdjacency(const Mesh& mesh);
        static osg::Vec2f projectPoint(const float* P, const osg::Vec3f& point);
        static float projectPointDepth(const float* P, const osg::Vec3f& point);

        static std::vector<int> selectViews(const Mesh& mesh,
            const std::vector<osg::Vec3f>& face_normals, const std::vector<Image>& images,
            const FaceAdjacencyMap& adjacency, const MeshTextureMappingOptions& options);
    };  // TextureMapping

}

#endif
