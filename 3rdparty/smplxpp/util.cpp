#include <fstream>
#include <iostream>

#include "smplx/util.hpp"
#include "smplx/util_cnpy.hpp"

namespace smplx {
namespace util {

const char* gender_to_str(Gender gender) {
    switch(gender) {
        case Gender::neutral: return "NEUTRAL";
        case Gender::male: return "MALE";
        case Gender::female: return "FEMALE";
        default: return "UNKNOWN";
    }
}

Gender parse_gender(std::string str) {
    for (auto& c : str) c = ::toupper(c);
    if (str == "NEUTRAL") return Gender::neutral;
    if (str == "MALE") return Gender::male;
    if (str == "FEMALE") return Gender::female;
    std::cerr << "WARNING: Gender '" << str << "' could not be parsed\n";
    return Gender::unknown;
}

std::string find_data_file(const std::string& data_path) {
    static const std::string TEST_PATH = "data/models/smplx/uv.txt";
    static const int MAX_LEVELS = 3;
    static std::string data_dir_saved = "\n";
    if (data_dir_saved == "\n") {
        data_dir_saved.clear();
        const char* env = std::getenv("SMPLX_DIR");
        if (env) {
            // use environmental variable if exists and works
            data_dir_saved = env;

            // auto append slash
            if (!data_dir_saved.empty() && data_dir_saved.back() != '/' &&
                data_dir_saved.back() != '\\')
                data_dir_saved.push_back('/');

            std::ifstream test_ifs(data_dir_saved + TEST_PATH);
            if (!test_ifs) data_dir_saved.clear();
        }

        // else check current directory and parents
        if (data_dir_saved.empty()) {
            for (int i = 0; i < MAX_LEVELS; ++i) {
                std::ifstream test_ifs(data_dir_saved + TEST_PATH);
                if (test_ifs) break;
                data_dir_saved.append("../");
            }
        }

        data_dir_saved.append("data/");
    }
    return data_dir_saved + data_path;
}

Eigen::Vector3f auto_color(size_t color_index) {
    static const Eigen::Vector3f palette[] = {
        Eigen::Vector3f{1.f, 0.2f, 0.3f},   Eigen::Vector3f{0.3f, 0.2f, 1.f},
        Eigen::Vector3f{0.3f, 1.2f, 0.2f},  Eigen::Vector3f{0.8f, 0.2f, 1.f},
        Eigen::Vector3f{0.7f, 0.7f, 0.7f},  Eigen::Vector3f{1.f, 0.45f, 0.f},
        Eigen::Vector3f{1.f, 0.17f, 0.54f}, Eigen::Vector3f{0.133f, 1.f, 0.37f},
        Eigen::Vector3f{1.f, 0.25f, 0.21f}, Eigen::Vector3f{1.f, 1.f, 0.25f},
        Eigen::Vector3f{0.f, 0.45f, 0.9f},  Eigen::Vector3f{0.105f, 0.522f, 1.f},
        Eigen::Vector3f{0.9f, 0.5f, 0.7f},  Eigen::Vector3f{1.f, 0.522f, 0.7f},
        Eigen::Vector3f{0.f, 1.0f, 0.8f},   Eigen::Vector3f{0.9f, 0.7f, 0.9f},
    };
    return palette[color_index % (sizeof palette / sizeof palette[0])];
}

Points auto_color_table(size_t num_colors) {
    Points colors(num_colors, 3);
    for (size_t i = 0; i < num_colors; ++i) {
        colors.row(i) = util::auto_color(i).transpose();
    }
    return colors;
}

Matrix load_float_matrix(const cnpy::NpyArray& raw, size_t r, size_t c) {
    size_t dwidth = raw.word_size;
    _SMPLX_ASSERT(dwidth == 4 || dwidth == 8);
    if (raw.fortran_order) {
        if (dwidth == 4) {
            return Eigen::template Map<const Eigen::Matrix<
                float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>>(
                       raw.data<float>(), r, c)
                .template cast<Scalar>();
        } else {
            return Eigen::template Map<const Eigen::Matrix<
                double, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>>(
                       raw.data<double>(), r, c)
                .template cast<Scalar>();
        }
    } else {
        if (dwidth == 4) {
            return Eigen::template Map<const Eigen::Matrix<
                float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>(
                       raw.data<float>(), r, c)
                .template cast<Scalar>();
        } else {
            return Eigen::template Map<const Eigen::Matrix<
                double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>(
                       raw.data<double>(), r, c)
                .template cast<Scalar>();
        }
    }
}
Eigen::Matrix<Index, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
load_uint_matrix(const cnpy::NpyArray& raw, size_t r, size_t c) {
    size_t dwidth = raw.word_size;
    _SMPLX_ASSERT(dwidth == 4 || dwidth == 8);
    if (raw.fortran_order) {
        if (dwidth == 4) {
            return Eigen::template Map<const Eigen::Matrix<
                uint32_t, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>>(
                       raw.data<uint32_t>(), r, c)
                .template cast<Index>();
        } else {
            return Eigen::template Map<const Eigen::Matrix<
                uint64_t, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>>(
                       raw.data<uint64_t>(), r, c)
                .template cast<Index>();
        }
    } else {
        if (dwidth == 4) {
            return Eigen::template Map<const Eigen::Matrix<
                uint32_t, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>(
                       raw.data<uint32_t>(), r, c)
                .template cast<Index>();
        } else {
            return Eigen::template Map<const Eigen::Matrix<
                uint64_t, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>(
                       raw.data<uint64_t>(), r, c)
                .template cast<Index>();
        }
    }
}

void assert_shape(const cnpy::NpyArray& m,
                  std::initializer_list<size_t> shape) {
    _SMPLX_ASSERT_EQ(m.shape.size(), shape.size());
    size_t idx = 0;
    for (auto& dim : shape) {
        if (dim != ANY_SHAPE)
            _SMPLX_ASSERT_EQ(m.shape[idx], dim);
        ++idx;
    }
}

}  // namespace util
}  // namespace smplx
