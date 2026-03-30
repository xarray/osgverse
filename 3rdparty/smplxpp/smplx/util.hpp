#pragma once
#ifndef SMPLX_UTIL_63B0803D_E0C7_4529_A796_9F6ED269E89F
#define SMPLX_UTIL_63B0803D_E0C7_4529_A796_9F6ED269E89F
#include "smplx/defs.hpp"

#include <random>

#define _SMPLX_ASSERT(x)                                                 \
    do {                                                                 \
        if (!(x)) {                                                      \
            std::cerr << "smplx assertion FAILED: \"" << #x << "\" ("    \
                      << (bool)(x) << ")\n  at " << __FILE__ << " line " \
                      << __LINE__ << "\n";                               \
            std::exit(1);                                                \
        }                                                                \
    } while (0)
#define _SMPLX_ASSERT_EQ(x, y)                                            \
    do {                                                                  \
        if ((x) != (y)) {                                                 \
            std::cerr << "smplx assertion FAILED: " << #x << " == " << #y \
                      << " (" << (x) << " != " << (y) << ")\n  at "       \
                      << __FILE__ << " line " << __LINE__ << "\n";        \
            std::exit(1);                                                 \
        }                                                                 \
    } while (0)
#define _SMPLX_ASSERT_NE(x, y)                                            \
    do {                                                                  \
        if ((x) == (y)) {                                                 \
            std::cerr << "smplx assertion FAILED: " << #x << " != " << #y \
                      << " (" << (x) << " == " << (y) << ")\n  at "       \
                      << __FILE__ << " line " << __LINE__ << "\n";        \
            std::exit(1);                                                 \
        }                                                                 \
    } while (0)
#define _SMPLX_ASSERT_LE(x, y)                                                 \
    do {                                                                       \
        if ((x) > (y)) {                                                       \
            std::cerr << "smplx assertion FAILED: " << #x << " <= " << #y      \
                      << " (" << (x) << " > " << (y) << ")\n  at " << __FILE__ \
                      << " line " << __LINE__ << "\n";                         \
            std::exit(1);                                                      \
        }                                                                      \
    } while (0)
#define _SMPLX_ASSERT_LT(x, y)                                           \
    do {                                                                 \
        if ((x) >= (y)) {                                                \
            std::cerr << "smplx assertion FAILED: " << #x << " < " << #y \
                      << " (" << (x) << " >= " << (y) << ")\n  at "      \
                      << __FILE__ << " line " << __LINE__ << "\n";       \
            std::exit(1);                                                \
        }                                                                \
    } while (0)

#include <chrono>
#define _SMPLX_BEGIN_PROFILE \
    auto start = std::chrono::high_resolution_clock::now()
#define _SMPLX_PROFILE(x)                                                      \
    do {                                                                       \
        double _delta = std::chrono::duration<double, std::milli>(             \
                            std::chrono::high_resolution_clock::now() - start) \
                            .count();                                          \
        printf("%s: %f ms = %f fps\n", #x, _delta, 1e3f / _delta);             \
        start = std::chrono::high_resolution_clock::now();                     \
    } while (false)
#define _SMPLX_PROFILE_STEPS(x, stp)                                  \
    do {                                                              \
        printf("%s: %f ms / step\n", #x,                              \
               std::chrono::duration<double, std::milli>(             \
                   std::chrono::high_resolution_clock::now() - start) \
                       .count() /                                     \
                   (stp));                                            \
        start = std::chrono::high_resolution_clock::now();            \
    } while (false)

#include <Eigen/Geometry>

namespace smplx {
namespace util {

const char* gender_to_str(Gender gender);
Gender parse_gender(std::string str);  // Copy intended

// Angle-axis to rotation matrix using custom implementation
template <class T, int Option = Eigen::ColMajor>
inline Eigen::Matrix<T, 3, 3, Option> rodrigues(
    const Eigen::Ref<const Eigen::Matrix<T, 3, 1>>& vec) {
    const T theta = vec.norm();
    const Eigen::Matrix<T, 3, 3, Option> eye =
        Eigen::Matrix<T, 3, 3, Option>::Identity();

    if (std::fabs(theta) < 1e-5f)
        return eye;
    else {
        const T c = std::cos(theta);
        const T s = std::sin(theta);
        const Eigen::Matrix<T, 3, 1> r = vec / theta;
        Eigen::Matrix<T, 3, 3, Option> skew;
        skew << 0, -r.z(), r.y(), r.z(), 0, -r.x(), -r.y(), r.x(), 0;
        return c * eye + (1 - c) * r * r.transpose() + s * skew;
    }
}

// Angle-axis to rotation matrix through Eigen quaternion
// (slightly slower than rodrigues, not useful)
template <class T, int Option = Eigen::ColMajor>
inline Eigen::Matrix<T, 3, 3, Option> rodrigues_eigen(
    const Eigen::Ref<const Eigen::Matrix<T, 3, 1>>& vec) {
    return Eigen::template AngleAxis<T>(vec.norm(), vec / vec.norm())
        .toRotationMatrix();
}

// Affine transformation matrix (hopefully) faster multiplication
// bottom row omitted
template <class T, int Option = Eigen::ColMajor>
inline void mul_affine(
    const Eigen::Ref<const Eigen::Matrix<T, 3, 4, Option>>& a,
    Eigen::Ref<Eigen::Matrix<T, 3, 4, Option>> b) {
    b.template leftCols<3>() =
        a.template leftCols<3>() * b.template leftCols<3>();
    b.template rightCols<1>() =
        a.template rightCols<1>() +
        a.template leftCols<3>() * b.template rightCols<1>();
}

// Affine transformation matrix 'in-place' inverse
template <class T, int Option = Eigen::ColMajor>
inline void inv_affine(Eigen::Ref<Eigen::Matrix<T, 3, 4, Option>> a) {
    a.template leftCols<3>() = a.template leftCols<3>().inverse();
    a.template rightCols<1>() =
        -a.template leftCols<3>() * a.template rightCols<1>();
}

// Homogeneous transformation matrix in-place inverse
template <class T, int Option = Eigen::ColMajor>
inline void inv_homogeneous(Eigen::Ref<Eigen::Matrix<T, 3, 4, Option>> a) {
    a.template leftCols<3>().transposeInPlace();
    a.template rightCols<1>() =
        -a.template leftCols<3>() * a.template rightCols<1>();
}

// Path resolve helper: return a valid path to file in data/
std::string find_data_file(const std::string& data_path);

// Create color from integer
Eigen::Vector3f auto_color(size_t color_index);

// Create table of num_colors colors, shape (num_colors, 3) row-major
Points auto_color_table(size_t num_colors);

// Set matrix to iid multivariate normal
template <class Mat>
void set_randn(Mat& m, float mean = 0.0f, float variance = 1.0f) {
    thread_local std::mt19937 rg{std::random_device{}()};
    std::normal_distribution<float> dist(mean, variance);
    for (size_t i = 0; i < m.rows(); ++i) {
        for (size_t j = 0; j < m.cols(); ++j) {
            m(i, j) = dist(rg);
        }
    }
}

}  // namespace util
}  // namespace smplx

#endif  // ifndef SMPLX_UTIL_63B0803D_E0C7_4529_A796_9F6ED269E89F
