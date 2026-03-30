#pragma once
#ifndef SMPLX_UTIL_CNPY_9B22F575_CE1E_4694_9F5D_2A7868EBE2C4
#define SMPLX_UTIL_CNPY_9B22F575_CE1E_4694_9F5D_2A7868EBE2C4

#include "smplx/defs.hpp"
#include <cnpy.h>

namespace smplx {
namespace util {

// Matrix load helper; currently copies on return
// can modify cnpy to load into the Eigen matrix; not important for now
Matrix load_float_matrix(const cnpy::NpyArray& raw, size_t r, size_t c);

Eigen::Matrix<Index, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
load_uint_matrix(const cnpy::NpyArray& raw, size_t r, size_t c);

const size_t ANY_SHAPE = (size_t)-1;
void assert_shape(const cnpy::NpyArray& m,
                  std::initializer_list<size_t> shape);

}  // namespace util
}  // namespace smplx
#endif  // ifndef SMPLX_UTIL_CNPY_9B22F575_CE1E_4694_9F5D_2A7868EBE2C4
