#pragma once
#ifndef SMPLX_CUDA_UTIL_A2261F55_FC88_4379_AADF_81E399304BB7
#define SMPLX_CUDA_UTIL_A2261F55_FC88_4379_AADF_81E399304BB7

#ifndef RESTRICT
#define RESTRICT __restrict__
#endif
namespace smplx {

#define cudaCheck(func)                                               \
    {                                                                  \
        cudaError_t status = (func);                                   \
        if (status != cudaSuccess) {                                   \
            printf("CUDA API failed at line %d with error: %s (%d)\n", \
                   __LINE__, cudaGetErrorString(status), status);      \
            return;                                                    \
        }                                                              \
    }


namespace cuda_util {
namespace device {
const unsigned int BLOCK_SIZE = 512;

template <class T, bool KeepYValue>
__global__ void matvec_kernel(const T* RESTRICT d_a, const T* RESTRICT d_x,
                              T* RESTRICT d_y, const unsigned int n_rows,
                              const unsigned int n_cols) {
    unsigned int bid = blockIdx.x;
    unsigned int row = threadIdx.x;
    unsigned int idx_y;
    const T* Asub;

    /* Only `x` is copied to shared memory */
    __shared__ T x_shared[BLOCK_SIZE];

    idx_y = bid * BLOCK_SIZE;

    T* y_sub = d_y + idx_y;
    T y_val = 0.0;
    Asub = d_a + BLOCK_SIZE * bid;

    if (row < n_cols) x_shared[row] = d_x[row];
    __syncthreads();
#pragma unroll
    for (unsigned int e = 0; e < n_cols; ++e) {
        y_val += Asub[row + e * n_rows] * x_shared[e];
    }
    __syncthreads();
    if (row + idx_y < n_rows) {
        if (KeepYValue)
            y_sub[row] += y_val;
        else
            y_sub[row] = y_val;
    }
}

template <class T>
__global__ void vecadd_kernel(const T* RESTRICT d_x, T* RESTRICT d_y,
                             const unsigned int n_rows) {
    unsigned int row = blockIdx.x * blockDim.x + threadIdx.x;
    if (row < n_rows) d_y[row] += d_x[row];
}

template <class T>
__global__ void vecadd3_kernel(const T* RESTRICT d_x, const T* RESTRICT d_y, T* RESTRICT d_z,
                             const unsigned int n_rows) {
    unsigned int row = blockIdx.x * blockDim.x + threadIdx.x;
    if (row < n_rows) d_z[row] += d_x[row] + d_y[row];
}
}  // namespace device

// Matrix-vector multiply; only works for vector d_y of size BLOCK_SIZE
template <class T, bool KeepYValue = true>
__host__ void mmv_block(const T* RESTRICT d_a, const T* RESTRICT d_x, T* RESTRICT d_y,
                     const unsigned int n_rows, const unsigned int n_cols,
                        cudaStream_t stream = nullptr) {
    dim3 dim_grid((n_rows + device::BLOCK_SIZE - 1) / device::BLOCK_SIZE);
    device::matvec_kernel<T, KeepYValue><<<dim_grid, device::BLOCK_SIZE, 0, stream>>>(
            d_a, d_x, d_y, n_rows, n_cols);
}

template <class T>
__host__ void vec_add(const T* RESTRICT d_x, T* RESTRICT d_y, const unsigned int n_rows,
        cudaStream_t stream = nullptr) {
    dim3 dim_grid((n_rows + device::BLOCK_SIZE - 1) / device::BLOCK_SIZE);
    device::vecadd_kernel<T><<<dim_grid, device::BLOCK_SIZE, 0, stream>>>(
            d_x, d_y, n_rows);
}

template <class T>
__host__ void vec_add3(const T* RESTRICT d_x, const T* RESTRICT d_y, T* RESTRICT d_z, const unsigned int n_rows,
        cudaStream_t stream = nullptr) {
    dim3 dim_grid((n_rows + device::BLOCK_SIZE - 1) / device::BLOCK_SIZE);
    device::vecadd3_kernel<T><<<dim_grid, device::BLOCK_SIZE, 0, stream>>>(
            d_x, d_y, d_z, n_rows);
}

template <class ScalarType, class MatType>
__host__ void to_host_eigen_matrix(ScalarType* d_data, int rows, int cols,
                              MatType& out) {
    out.resize(rows, cols);
    cudaMemcpy(out.data(), d_data, out.size() * sizeof(ScalarType),
               cudaMemcpyDeviceToHost);
}

template <class Scalar, class EigenType>
__host__ void from_host_eigen_matrix(Scalar*& d_data, const EigenType& src) {
    const size_t dsize = src.size() * sizeof(src.data()[0]);
    cudaMalloc((void**)&d_data, dsize);
    cudaMemcpy(d_data, src.data(), dsize, cudaMemcpyHostToDevice);
}
template <int Option>
__host__ void from_host_eigen_sparse_matrix(internal::GPUSparseMatrix& d_data,
                            const Eigen::SparseMatrix<float, Option>& src) {
    const size_t nnz = src.nonZeros();
    d_data.nnz = (int) nnz;
    d_data.cols = (int) src.cols();
    d_data.rows = (int) src.rows();
    cudaMalloc((void**)&d_data.values, nnz * sizeof(float));
    cudaMemcpy(d_data.values, src.valuePtr(), nnz * sizeof(float),
               cudaMemcpyHostToDevice);

    cudaMalloc((void**)&d_data.inner, nnz * sizeof(int));
    cudaMemcpy(d_data.inner, src.innerIndexPtr(), nnz * sizeof(int),
               cudaMemcpyHostToDevice);

    cudaMalloc((void**)&d_data.outer, (src.outerSize() + 1) * sizeof(int));
    cudaMemcpy(d_data.outer, src.outerIndexPtr(),
               (src.outerSize() + 1) * sizeof(int), cudaMemcpyHostToDevice);
}
}  // namespace util
}  // namespace smplx
#endif  // ifndef SMPLX_CUDA_UTIL_A2261F55_FC88_4379_AADF_81E399304BB7
