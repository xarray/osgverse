#include "gf/io/sog.h"
#include "zip_internal.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <webp/encode.h>
#include <zlib.h>

#ifdef __EMSCRIPTEN__
#include <wasm_simd128.h>
#endif

#include "gf/core/errors.h"
#include "gf/core/gauss_ir.h"
#include "gf/core/validate.h"

using json = nlohmann::json;

namespace gf {

namespace {

#if defined(_OPENMP)
#define GF_OMP_PARALLEL_FOR _Pragma("omp parallel for schedule(static)")
#else
#define GF_OMP_PARALLEL_FOR
#endif

// --- Simple ZIP Writer ---

class SimpleZipWriter {
public:
  void AddFile(const std::string &name, const std::vector<uint8_t> &data) {
    FileEntry e;
    e.name = name;
    e.offset = static_cast<uint32_t>(buffer_.size());
    e.size = static_cast<uint32_t>(data.size());
    e.crc = crc32(0, data.data(), static_cast<uInt>(data.size()));

    zip::LocalFileHeader lh;
    lh.signature = zip::kLocalFileHeaderSig;
    lh.versionNeeded = 20;
    lh.flags = 0;
    lh.compression = 0; // 0 = stored
    lh.modTime = 0;
    lh.modDate = 0;
    lh.crc32 = e.crc;
    lh.compressedSize = e.size;
    lh.uncompressedSize = e.size;
    lh.fileNameLength = static_cast<uint16_t>(name.size());
    lh.extraFieldLength = 0;

    const uint8_t *lhp = reinterpret_cast<const uint8_t *>(&lh);
    buffer_.insert(buffer_.end(), lhp, lhp + sizeof(lh));
    buffer_.insert(buffer_.end(), name.begin(), name.end());
    buffer_.insert(buffer_.end(), data.begin(), data.end());

    entries_.push_back(e);
  }

  std::vector<uint8_t> Finalize() {
    uint32_t cdOffset = static_cast<uint32_t>(buffer_.size());
    for (const auto &e : entries_) {
      zip::CentralDirHeader ch;
      ch.signature = zip::kCentralDirHeaderSig;
      ch.versionMade = 20;
      ch.versionNeeded = 20;
      ch.flags = 0;
      ch.compression = 0;
      ch.modTime = 0;
      ch.modDate = 0;
      ch.crc32 = e.crc;
      ch.compressedSize = e.size;
      ch.uncompressedSize = e.size;
      ch.fileNameLength = static_cast<uint16_t>(e.name.size());
      ch.extraFieldLength = 0;
      ch.commentLength = 0;
      ch.diskStart = 0;
      ch.internalAttrs = 0;
      ch.externalAttrs = 0;
      ch.localHeaderOffset = e.offset;

      const uint8_t *chp = reinterpret_cast<const uint8_t *>(&ch);
      buffer_.insert(buffer_.end(), chp, chp + sizeof(ch));
      buffer_.insert(buffer_.end(), e.name.begin(), e.name.end());
    }
    uint32_t cdSize = static_cast<uint32_t>(buffer_.size()) - cdOffset;

    zip::EndOfCentralDir eocd;
    eocd.signature = zip::kEndOfCentralDirSig;
    eocd.diskNumber = 0;
    eocd.diskWithCentralDir = 0;
    eocd.numEntriesThisDisk = static_cast<uint16_t>(entries_.size());
    eocd.numEntriesTotal = static_cast<uint16_t>(entries_.size());
    eocd.centralDirSize = cdSize;
    eocd.centralDirOffset = cdOffset;
    eocd.commentLength = 0;

    const uint8_t *ep = reinterpret_cast<const uint8_t *>(&eocd);
    buffer_.insert(buffer_.end(), ep, ep + sizeof(eocd));

    return std::move(buffer_);
  }

private:
  struct FileEntry {
    std::string name;
    uint32_t offset;
    uint32_t size;
    uint32_t crc;
  };
  std::vector<FileEntry> entries_;
  std::vector<uint8_t> buffer_;
};

// --- First-Principles K-Means (1D) ---

std::vector<float> Generate1DCodebook(const std::vector<float> &data,
                                      int centers,
                                      std::vector<uint8_t> &indices) {
  if (data.empty()) {
    indices.clear();
    return std::vector<float>(static_cast<size_t>(centers), 0.0f);
  }

  const size_t n = data.size();
  std::vector<float> centroids(static_cast<size_t>(centers));

  // Linear initialization
  auto [min_it, max_it] = std::minmax_element(data.begin(), data.end());
  float min_v = *min_it;
  float max_v = *max_it;
  float range = max_v - min_v;
  for (int i = 0; i < centers; ++i) {
    centroids[static_cast<size_t>(i)] =
        min_v + (static_cast<float>(i) /
                 static_cast<float>(centers > 1 ? centers - 1 : 1)) *
                    range;
  }

  indices.resize(n);
  std::vector<float> next_centroids(static_cast<size_t>(centers));
  std::vector<int> counts(static_cast<size_t>(centers));

  for (int iter = 0; iter < 10; ++iter) {
    std::fill(counts.begin(), counts.end(), 0);
    std::fill(next_centroids.begin(), next_centroids.end(), 0.0f);

    for (size_t i = 0; i < n; ++i) {
      float min_d = 1e30f;
      int best_k = 0;
      for (int k = 0; k < centers; ++k) {
        float d = std::abs(data[i] - centroids[static_cast<size_t>(k)]);
        if (d < min_d) {
          min_d = d;
          best_k = k;
        }
      }
      indices[i] = static_cast<uint8_t>(best_k);
      next_centroids[static_cast<size_t>(best_k)] += data[i];
      counts[static_cast<size_t>(best_k)]++;
    }

    for (int k = 0; k < centers; ++k) {
      if (counts[static_cast<size_t>(k)] > 0) {
        centroids[static_cast<size_t>(k)] =
            next_centroids[static_cast<size_t>(k)] /
            static_cast<float>(counts[static_cast<size_t>(k)]);
      }
    }
  }

  return centroids;
}

void SortCodebookAndRemap(std::vector<float> &codebook,
                          std::vector<uint8_t> &indices) {
  std::vector<size_t> order(codebook.size());
  std::iota(order.begin(), order.end(), 0);
  std::sort(order.begin(), order.end(),
            [&](size_t a, size_t b) { return codebook[a] < codebook[b]; });

  std::vector<uint8_t> remap(codebook.size(), 0);
  std::vector<float> sorted(codebook.size(), 0.0f);
  for (size_t i = 0; i < order.size(); ++i) {
    sorted[i] = codebook[order[i]];
    remap[order[i]] = static_cast<uint8_t>(i);
  }

  for (auto &idx : indices) {
    idx = remap[idx];
  }
  codebook = std::move(sorted);
}

std::vector<float> GenerateSorted1DCodebook(const std::vector<float> &data,
                                            int centers,
                                            std::vector<uint8_t> &indices) {
  std::vector<float> codebook = Generate1DCodebook(data, centers, indices);
  SortCodebookAndRemap(codebook, indices);
  return codebook;
}

uint32_t NextPowerOfTwo(uint32_t value) {
  if (value <= 1) {
    return 1;
  }
  value--;
  value |= value >> 1;
  value |= value >> 2;
  value |= value >> 4;
  value |= value >> 8;
  value |= value >> 16;
  return value + 1;
}

uint32_t SelectSogShPaletteSize(uint32_t count) {
  constexpr uint32_t kMinPaletteSize = 1024u;
#ifdef __EMSCRIPTEN__
  constexpr uint32_t kMaxPaletteSize = 16384u;
#else
  constexpr uint32_t kMaxPaletteSize = 65536u;
#endif
  return std::min(kMaxPaletteSize,
                  std::max(kMinPaletteSize, NextPowerOfTwo(count)));
}

uint32_t GetPaddedDims(uint32_t dims) {
  switch (dims) {
  case 9:
    return 12;
  case 24:
    return 24;
  case 45:
    return 48;
  default:
    return dims;
  }
}

#ifdef __EMSCRIPTEN__

inline float DotProductHorizontalSum(v128_t acc) {
  v128_t shuf = wasm_i32x4_shuffle(acc, acc, 2, 3, 0, 1);
  acc = wasm_f32x4_add(acc, shuf);
  shuf = wasm_i32x4_shuffle(acc, acc, 1, 0, 3, 2);
  acc = wasm_f32x4_add(acc, shuf);
  return wasm_f32x4_extract_lane(acc, 0);
}

template <uint32_t D>
inline float DotProductFixed(const float *a, const float *b) {
  v128_t acc = wasm_f32x4_splat(0.0f);
  for (uint32_t d = 0; d < D; d += 4) {
    acc = wasm_f32x4_add(
        acc, wasm_f32x4_mul(wasm_v128_load(a + d), wasm_v128_load(b + d)));
  }
  return DotProductHorizontalSum(acc);
}

inline float DotProductDynamic(const float *a, const float *b, uint32_t dims) {
  v128_t acc = wasm_f32x4_splat(0.0f);
  uint32_t d = 0;
  for (; d + 3 < dims; d += 4) {
    acc = wasm_f32x4_add(
        acc, wasm_f32x4_mul(wasm_v128_load(a + d), wasm_v128_load(b + d)));
  }
  float tail = 0.0f;
  for (; d < dims; ++d) {
    tail += a[d] * b[d];
  }
  return DotProductHorizontalSum(acc) + tail;
}

#else

template <uint32_t D>
inline float DotProductFixed(const float *a, const float *b) {
  float sum0 = 0.0f;
  float sum1 = 0.0f;
  float sum2 = 0.0f;
  float sum3 = 0.0f;
  uint32_t d = 0;
  for (; d + 3 < D; d += 4) {
    sum0 += a[d + 0] * b[d + 0];
    sum1 += a[d + 1] * b[d + 1];
    sum2 += a[d + 2] * b[d + 2];
    sum3 += a[d + 3] * b[d + 3];
  }
  float sum = (sum0 + sum1) + (sum2 + sum3);
  for (; d < D; ++d) {
    sum += a[d] * b[d];
  }
  return sum;
}

inline float DotProductDynamic(const float *a, const float *b, uint32_t dims) {
  float sum0 = 0.0f;
  float sum1 = 0.0f;
  float sum2 = 0.0f;
  float sum3 = 0.0f;
  uint32_t d = 0;
  for (; d + 3 < dims; d += 4) {
    sum0 += a[d + 0] * b[d + 0];
    sum1 += a[d + 1] * b[d + 1];
    sum2 += a[d + 2] * b[d + 2];
    sum3 += a[d + 3] * b[d + 3];
  }
  float sum = (sum0 + sum1) + (sum2 + sum3);
  for (; d < dims; ++d) {
    sum += a[d] * b[d];
  }
  return sum;
}

#endif

template <uint32_t D>
void ComputeNormsFixed(const float *data, uint32_t rows, float *norms) {
  GF_OMP_PARALLEL_FOR
  for (int64_t row = 0; row < static_cast<int64_t>(rows); ++row) {
    const float *row_ptr = data + static_cast<size_t>(row) * D;
    norms[row] = DotProductFixed<D>(row_ptr, row_ptr);
  }
}

void ComputeNormsDynamic(const float *data, uint32_t rows, uint32_t dims,
                         float *norms) {
  GF_OMP_PARALLEL_FOR
  for (int64_t row = 0; row < static_cast<int64_t>(rows); ++row) {
    const float *row_ptr = data + static_cast<size_t>(row) * dims;
    norms[row] = DotProductDynamic(row_ptr, row_ptr, dims);
  }
}

void ComputeNorms(const float *data, uint32_t rows, uint32_t dims,
                  float *norms) {
  switch (dims) {
  case 12:
    ComputeNormsFixed<12>(data, rows, norms);
    break;
  case 24:
    ComputeNormsFixed<24>(data, rows, norms);
    break;
  case 48:
    ComputeNormsFixed<48>(data, rows, norms);
    break;
  default:
    ComputeNormsDynamic(data, rows, dims, norms);
    break;
  }
}

template <uint32_t D>
void AssignLabelsFixed(const float *data, const float *centroids, uint32_t rows,
                       uint32_t centers, const float *centroid_norms,
                       uint16_t *labels) {
  GF_OMP_PARALLEL_FOR
  for (int64_t row = 0; row < static_cast<int64_t>(rows); ++row) {
    const float *row_ptr = data + static_cast<size_t>(row) * D;
    uint32_t best = 0;
    float best_score =
        centroid_norms[best] -
        2.0f * DotProductFixed<D>(row_ptr,
                                  centroids + static_cast<size_t>(best) * D);

    for (uint32_t k = 1; k < centers; ++k) {
      const float score =
          centroid_norms[k] -
          2.0f * DotProductFixed<D>(row_ptr,
                                    centroids + static_cast<size_t>(k) * D);
      if (score < best_score) {
        best_score = score;
        best = k;
      }
    }

    labels[row] = static_cast<uint16_t>(best);
  }
}

void AssignLabelsDynamic(const float *data, const float *centroids,
                         uint32_t rows, uint32_t dims, uint32_t centers,
                         const float *centroid_norms, uint16_t *labels) {
  GF_OMP_PARALLEL_FOR
  for (int64_t row = 0; row < static_cast<int64_t>(rows); ++row) {
    const float *row_ptr = data + static_cast<size_t>(row) * dims;
    uint32_t best = 0;
    float best_score =
        centroid_norms[best] -
        2.0f * DotProductDynamic(
                   row_ptr, centroids + static_cast<size_t>(best) * dims, dims);

    for (uint32_t k = 1; k < centers; ++k) {
      const float score =
          centroid_norms[k] -
          2.0f * DotProductDynamic(
                     row_ptr, centroids + static_cast<size_t>(k) * dims, dims);
      if (score < best_score) {
        best_score = score;
        best = k;
      }
    }

    labels[row] = static_cast<uint16_t>(best);
  }
}

void AssignLabels(const float *data, const float *centroids, uint32_t rows,
                  uint32_t dims, uint32_t centers, const float *centroid_norms,
                  uint16_t *labels) {
  switch (dims) {
  case 12:
    AssignLabelsFixed<12>(data, centroids, rows, centers, centroid_norms,
                          labels);
    break;
  case 24:
    AssignLabelsFixed<24>(data, centroids, rows, centers, centroid_norms,
                          labels);
    break;
  case 48:
    AssignLabelsFixed<48>(data, centroids, rows, centers, centroid_norms,
                          labels);
    break;
  default:
    AssignLabelsDynamic(data, centroids, rows, dims, centers, centroid_norms,
                        labels);
    break;
  }
}

std::vector<float> PadRows(const std::vector<float> &data, uint32_t rows,
                           uint32_t dims, uint32_t padded_dims) {
  if (dims == padded_dims) {
    return data;
  }

  std::vector<float> padded(static_cast<size_t>(rows) * padded_dims, 0.0f);
  GF_OMP_PARALLEL_FOR
  for (int64_t row = 0; row < static_cast<int64_t>(rows); ++row) {
    const size_t src_offset = static_cast<size_t>(row) * dims;
    const size_t dst_offset = static_cast<size_t>(row) * padded_dims;
    std::copy_n(data.data() + src_offset, dims, padded.data() + dst_offset);
  }
  return padded;
}

struct VectorKMeansResult {
  std::vector<float> centroids;
  std::vector<uint16_t> labels;
};

VectorKMeansResult ClusterVectors(const std::vector<float> &data, uint32_t rows,
                                  uint32_t dims, uint32_t centers,
                                  int iterations = 10) {
  VectorKMeansResult result;
  if (rows == 0 || dims == 0 || centers == 0) {
    return result;
  }

  const uint32_t padded_dims = GetPaddedDims(dims);
  const std::vector<float> padded_data = PadRows(data, rows, dims, padded_dims);

  std::vector<float> padded_centroids(
      static_cast<size_t>(centers) * padded_dims, 0.0f);
  result.labels.resize(rows, 0);

  const auto copy_row = [&](uint32_t src_row, uint32_t dst_row,
                            std::vector<float> &dst) {
    const size_t src_offset = static_cast<size_t>(src_row) * padded_dims;
    const size_t dst_offset = static_cast<size_t>(dst_row) * padded_dims;
    std::copy_n(padded_data.data() + src_offset, padded_dims,
                dst.data() + dst_offset);
  };

  if (centers >= rows) {
    for (uint32_t i = 0; i < rows; ++i) {
      copy_row(i, i, padded_centroids);
      result.labels[i] = static_cast<uint16_t>(i);
    }
    for (uint32_t i = rows; i < centers; ++i) {
      copy_row(rows - 1, i, padded_centroids);
    }
    result.centroids.resize(static_cast<size_t>(centers) * dims, 0.0f);
    for (uint32_t i = 0; i < centers; ++i) {
      std::copy_n(
          padded_centroids.data() + static_cast<size_t>(i) * padded_dims, dims,
          result.centroids.data() + static_cast<size_t>(i) * dims);
    }
    return result;
  }

  for (uint32_t k = 0; k < centers; ++k) {
    const uint32_t sample =
        static_cast<uint32_t>((static_cast<uint64_t>(k) * rows) / centers);
    copy_row(std::min(sample, rows - 1), k, padded_centroids);
  }

  std::vector<float> next_centroids(static_cast<size_t>(centers) * padded_dims,
                                    0.0f);
  std::vector<uint32_t> counts(centers, 0);
  std::vector<float> centroid_norms(centers, 0.0f);
  std::vector<uint16_t> prev_labels(rows, 0);

  for (int iter = 0; iter < iterations; ++iter) {
    std::fill(next_centroids.begin(), next_centroids.end(), 0.0f);
    std::fill(counts.begin(), counts.end(), 0);
    std::swap(prev_labels, result.labels);

    ComputeNorms(padded_centroids.data(), centers, padded_dims,
                 centroid_norms.data());
    AssignLabels(padded_data.data(), padded_centroids.data(), rows, padded_dims,
                 centers, centroid_norms.data(), result.labels.data());

    uint32_t changed = 0;
    for (uint32_t row = 0; row < rows; ++row) {
      if (result.labels[row] != prev_labels[row]) {
        changed++;
      }
      const uint16_t best = result.labels[row];
      counts[best]++;

      const size_t row_offset = static_cast<size_t>(row) * padded_dims;
      const size_t next_offset = static_cast<size_t>(best) * padded_dims;
      for (uint32_t d = 0; d < padded_dims; ++d) {
        next_centroids[next_offset + d] += padded_data[row_offset + d];
      }
    }

    for (uint32_t k = 0; k < centers; ++k) {
      const size_t centroid_offset = static_cast<size_t>(k) * padded_dims;
      if (counts[k] == 0) {
        const uint32_t sample =
            static_cast<uint32_t>((static_cast<uint64_t>(k) * rows) / centers);
        copy_row(std::min(sample, rows - 1), k, padded_centroids);
        continue;
      }

      const float inv_count = 1.0f / static_cast<float>(counts[k]);
      for (uint32_t d = 0; d < padded_dims; ++d) {
        padded_centroids[centroid_offset + d] =
            next_centroids[centroid_offset + d] * inv_count;
      }
    }

    if (iter > 0 &&
        static_cast<float>(changed) / static_cast<float>(rows) < 0.01f) {
      break;
    }
  }

  result.centroids.resize(static_cast<size_t>(centers) * dims, 0.0f);
  GF_OMP_PARALLEL_FOR
  for (int64_t k = 0; k < static_cast<int64_t>(centers); ++k) {
    std::copy_n(padded_centroids.data() + static_cast<size_t>(k) * padded_dims,
                dims, result.centroids.data() + static_cast<size_t>(k) * dims);
  }

  return result;
}

// --- Morton Encoding (64-bit single-pass) ---
// Reference: https://fgiesen.wordpress.com/2009/12/13/decoding-morton-codes/
// Uses 21 bits per axis for near-zero collision probability (2M^3 grid)

// Bit expansion: 21-bit -> 63-bit for Morton code calculation
inline uint64_t ExpandBits21(uint64_t x) {
  x &= 0x1FFFFF; // Keep only the lower 21 bits
  x = (x | (x << 32)) & 0x001F00000000FFFF;
  x = (x | (x << 16)) & 0x001F0000FF0000FF;
  x = (x | (x << 8)) & 0x100F00F00F00F00F;
  x = (x | (x << 4)) & 0x10C30C30C30C30C3;
  x = (x | (x << 2)) & 0x1249249249249249;
  return x;
}

// Encode 3D coordinates into a 64-bit Morton code (Z-order curve)
inline uint64_t EncodeMorton3_64(uint32_t x, uint32_t y, uint32_t z) {
  return (ExpandBits21(z) << 2) | (ExpandBits21(y) << 1) | ExpandBits21(x);
}

// Generate Morton-sorted index array (single-pass, no recursion)
std::vector<uint32_t> GenerateMortonOrder(const std::vector<float> &positions,
                                          int32_t numPoints) {
  if (numPoints <= 0)
    return {};

  const size_t n = static_cast<size_t>(numPoints);

  // 1. Calculate global bounding box
  float mx = 1e30f, my = 1e30f, mz = 1e30f;
  float Mx = -1e30f, My = -1e30f, Mz = -1e30f;

  for (size_t i = 0; i < n; ++i) {
    float x = positions[i * 3 + 0];
    float y = positions[i * 3 + 1];
    float z = positions[i * 3 + 2];
    mx = std::min(mx, x);
    Mx = std::max(Mx, x);
    my = std::min(my, y);
    My = std::max(My, y);
    mz = std::min(mz, z);
    Mz = std::max(Mz, z);
  }

  // 2. Calculate quantization factors (21-bit precision)
  constexpr float kMax21 = 2097151.0f; // 2^21 - 1
  float xmul = (Mx - mx > 1e-8f) ? kMax21 / (Mx - mx) : 0.0f;
  float ymul = (My - my > 1e-8f) ? kMax21 / (My - my) : 0.0f;
  float zmul = (Mz - mz > 1e-8f) ? kMax21 / (Mz - mz) : 0.0f;

  // 3. Compute 64-bit Morton codes
  std::vector<std::pair<uint64_t, uint32_t>> morton_pairs(n);
  for (size_t i = 0; i < n; ++i) {
    uint32_t ix = std::min(
        2097151u, static_cast<uint32_t>((positions[i * 3 + 0] - mx) * xmul));
    uint32_t iy = std::min(
        2097151u, static_cast<uint32_t>((positions[i * 3 + 1] - my) * ymul));
    uint32_t iz = std::min(
        2097151u, static_cast<uint32_t>((positions[i * 3 + 2] - mz) * zmul));

    morton_pairs[i] = {EncodeMorton3_64(ix, iy, iz), static_cast<uint32_t>(i)};
  }

  // 4. Single-pass sort by Morton code
  std::sort(morton_pairs.begin(), morton_pairs.end(),
            [](const auto &a, const auto &b) { return a.first < b.first; });

  // 5. Extract sorted indices
  std::vector<uint32_t> indices(n);
  for (size_t i = 0; i < n; ++i) {
    indices[i] = morton_pairs[i].second;
  }

  return indices;
}

// --- SOG Encoding Helpers ---

inline float LogTransform(float v) {
  return (v < 0.0f) ? -std::log(std::abs(v) + 1.0f) : std::log(v + 1.0f);
}

std::vector<uint8_t> EncodeWebPLossless(const std::vector<uint8_t> &rgba,
                                        int width, int height) {
  uint8_t *output = nullptr;
  size_t size =
      WebPEncodeLosslessRGBA(rgba.data(), width, height, width * 4, &output);
  if (size == 0 || !output)
    return {};
  std::vector<uint8_t> result(output, output + size);
  WebPFree(output);
  return result;
}

void EncodeQuaternion(float w, float x, float y, float z, uint8_t out[4]) {
  float q[4] = {w, x, y, z};
  int max_idx = 0;
  float max_val = std::abs(q[0]);
  for (int i = 1; i < 4; ++i) {
    if (std::abs(q[i]) > max_val) {
      max_val = std::abs(q[i]);
      max_idx = i;
    }
  }
  if (q[max_idx] < 0.0f) {
    for (int i = 0; i < 4; ++i)
      q[i] = -q[i];
  }

  const float inv_sqrt2 = 1.0f / std::sqrt(2.0f);
  int count = 0;
  for (int i = 0; i < 4; ++i) {
    if (i == max_idx)
      continue;
    float val = std::clamp((q[i] / inv_sqrt2 + 1.0f) * 0.5f, 0.0f, 1.0f);
    out[count++] = static_cast<uint8_t>(std::round(val * 255.0f));
  }
  out[3] = static_cast<uint8_t>(252 + max_idx);
}

class SogWriter : public IGaussWriter {
public:
  Expected<std::vector<uint8_t>> Write(const GaussianCloudIR &ir,
                                       const WriteOptions &options) override {
    const auto err = ValidateBasic(ir, options.strict);
    if (!err.message.empty()) {
      return Expected<std::vector<uint8_t>>(err);
    }

    if (ir.numPoints <= 0)
      return MakeError("SOG: Empty cloud");
    if (ir.meta.shDegree < 0 || ir.meta.shDegree > 3) {
      return MakeError("SOG: Only SH degrees 0-3 are supported");
    }

    uint32_t count = static_cast<uint32_t>(ir.numPoints);
    const uint32_t sh_bands = static_cast<uint32_t>(ir.meta.shDegree);
    const uint32_t sh_coeffs =
        (sh_bands == 0) ? 0 : (sh_bands + 1) * (sh_bands + 1) - 1;
    // Texture dimensions: multiples of 4 for WebP/GPU alignment.
    // Padding pixels (i > count) remain zero-initialized.
    int width = static_cast<int>(
                    std::ceil(std::sqrt(static_cast<double>(count)) / 4.0)) *
                4;
    int height =
        static_cast<int>(
            std::ceil(static_cast<double>(count) / width / 4.0)) *
        4;
    size_t tex_size = static_cast<size_t>(width) * static_cast<size_t>(height);

    SimpleZipWriter zip;
    json meta_json;
    meta_json["version"] = 2;
    meta_json["count"] = count;
    meta_json["antialias"] = ir.meta.antialiased;
    meta_json["asset"]["generator"] = "GaussForge";

    // Morton sort: spatially adjacent points become texture-adjacent, improving
    // compression
    std::vector<uint32_t> morton_indices =
        GenerateMortonOrder(ir.positions, ir.numPoints);

    // 1. Positions (Log + 16-bit)
    std::vector<float> log_pos(static_cast<size_t>(count) * 3);
    float mins[3] = {1e30f, 1e30f, 1e30f}, maxs[3] = {-1e30f, -1e30f, -1e30f};
    for (uint32_t i = 0; i < count; ++i) {
      uint32_t idx = morton_indices[i];
      for (int d = 0; d < 3; ++d) {
        float v = LogTransform(ir.positions[static_cast<size_t>(idx) * 3 +
                                            static_cast<size_t>(d)]);
        log_pos[static_cast<size_t>(i) * 3 + static_cast<size_t>(d)] = v;
        mins[d] = std::min(mins[d], v);
        maxs[d] = std::max(maxs[d], v);
      }
    }
    meta_json["means"]["mins"] = {mins[0], mins[1], mins[2]};
    meta_json["means"]["maxs"] = {maxs[0], maxs[1], maxs[2]};
    meta_json["means"]["files"] = {"means_l.webp", "means_u.webp"};

    std::vector<uint8_t> means_l(tex_size * 4, 0), means_u(tex_size * 4, 0);
    for (uint32_t i = 0; i < count; ++i) {
      for (int d = 0; d < 3; ++d) {
        float range = maxs[d] - mins[d];
        float n = (range > 1e-8f) ? (log_pos[static_cast<size_t>(i) * 3 +
                                             static_cast<size_t>(d)] -
                                     mins[d]) /
                                        range
                                  : 0.0f;
        uint16_t q =
            static_cast<uint16_t>(std::clamp(n * 65535.0f, 0.0f, 65535.0f));
        means_l[static_cast<size_t>(i) * 4 + static_cast<size_t>(d)] =
            static_cast<uint8_t>(q & 0xFF);
        means_u[static_cast<size_t>(i) * 4 + static_cast<size_t>(d)] =
            static_cast<uint8_t>((q >> 8) & 0xFF);
      }
      means_l[static_cast<size_t>(i) * 4 + 3] = 255;
      means_u[static_cast<size_t>(i) * 4 + 3] = 255;
    }
    zip.AddFile("means_l.webp", EncodeWebPLossless(means_l, width, height));
    zip.AddFile("means_u.webp", EncodeWebPLossless(means_u, width, height));

    // 2. Quats
    std::vector<uint8_t> quats_rgba(tex_size * 4, 0);
    for (uint32_t i = 0; i < count; ++i) {
      uint32_t idx = morton_indices[i];
      EncodeQuaternion(ir.rotations[static_cast<size_t>(idx) * 4 + 0],
                       ir.rotations[static_cast<size_t>(idx) * 4 + 1],
                       ir.rotations[static_cast<size_t>(idx) * 4 + 2],
                       ir.rotations[static_cast<size_t>(idx) * 4 + 3],
                       &quats_rgba[static_cast<size_t>(i) * 4]);
    }
    meta_json["quats"]["files"] = {"quats.webp"};
    zip.AddFile("quats.webp", EncodeWebPLossless(quats_rgba, width, height));

    // 3. Scales (1D Codebook)
    std::vector<uint8_t> scale_indices;
    std::vector<float> scale_cb =
        GenerateSorted1DCodebook(ir.scales, 256, scale_indices);
    meta_json["scales"]["codebook"] = scale_cb;
    meta_json["scales"]["files"] = {"scales.webp"};
    std::vector<uint8_t> scales_rgba(tex_size * 4, 0);
    for (uint32_t i = 0; i < count; ++i) {
      uint32_t idx = morton_indices[i];
      scales_rgba[static_cast<size_t>(i) * 4 + 0] =
          scale_indices[static_cast<size_t>(idx) * 3 + 0];
      scales_rgba[static_cast<size_t>(i) * 4 + 1] =
          scale_indices[static_cast<size_t>(idx) * 3 + 1];
      scales_rgba[static_cast<size_t>(i) * 4 + 2] =
          scale_indices[static_cast<size_t>(idx) * 3 + 2];
      scales_rgba[static_cast<size_t>(i) * 4 + 3] = 255;
    }
    zip.AddFile("scales.webp", EncodeWebPLossless(scales_rgba, width, height));

    // 4. SH0 + Opacity (1D Codebook)
    std::vector<uint8_t> sh0_indices;
    std::vector<float> sh0_cb =
        GenerateSorted1DCodebook(ir.colors, 256, sh0_indices);
    meta_json["sh0"]["codebook"] = sh0_cb;
    meta_json["sh0"]["files"] = {"sh0.webp"};
    std::vector<uint8_t> sh0_rgba(tex_size * 4, 0);
    for (uint32_t i = 0; i < count; ++i) {
      uint32_t idx = morton_indices[i];
      sh0_rgba[static_cast<size_t>(i) * 4 + 0] =
          sh0_indices[static_cast<size_t>(idx) * 3 + 0];
      sh0_rgba[static_cast<size_t>(i) * 4 + 1] =
          sh0_indices[static_cast<size_t>(idx) * 3 + 1];
      sh0_rgba[static_cast<size_t>(i) * 4 + 2] =
          sh0_indices[static_cast<size_t>(idx) * 3 + 2];
      float op = 1.0f / (1.0f + std::exp(-ir.alphas[static_cast<size_t>(idx)]));
      // Clamp to [1/255, 254/255] to avoid alpha=0 or alpha=255,
      // which may cause WebP or renderers to mishandle edge pixels.
      op = std::clamp(op, 1.0f / 255.0f, 254.0f / 255.0f);
      sh0_rgba[static_cast<size_t>(i) * 4 + 3] =
          static_cast<uint8_t>(op * 255.0f + 0.5f);
    }
    zip.AddFile("sh0.webp", EncodeWebPLossless(sh0_rgba, width, height));

    // 5. Higher-order SH (palette + shared scalar codebook)
    if (sh_bands > 0 && !ir.sh.empty()) {
      const uint32_t palette_size = SelectSogShPaletteSize(count);
      const uint32_t sh_dims = sh_coeffs * 3;

      std::vector<float> sh_vectors(static_cast<size_t>(count) * sh_dims, 0.0f);
      GF_OMP_PARALLEL_FOR
      for (int64_t i = 0; i < static_cast<int64_t>(count); ++i) {
        const uint32_t idx = morton_indices[static_cast<size_t>(i)];
        const size_t src_offset = static_cast<size_t>(idx) * sh_dims;
        const size_t dst_offset = static_cast<size_t>(i) * sh_dims;
        std::copy_n(ir.sh.data() + src_offset, sh_dims,
                    sh_vectors.data() + dst_offset);
      }

      VectorKMeansResult palette =
          ClusterVectors(sh_vectors, count, sh_dims, palette_size);

      std::vector<uint8_t> shn_scalar_indices;
      std::vector<float> shn_codebook =
          GenerateSorted1DCodebook(palette.centroids, 256, shn_scalar_indices);

      const uint32_t centroid_width = 64u * sh_coeffs;
      const uint32_t centroid_height = (palette_size + 63u) / 64u;
      std::vector<uint8_t> centroids_rgba(
          static_cast<size_t>(centroid_width) * centroid_height * 4, 0);

      GF_OMP_PARALLEL_FOR
      for (int64_t palette_idx_i = 0;
           palette_idx_i < static_cast<int64_t>(palette_size);
           ++palette_idx_i) {
        const uint32_t palette_idx = static_cast<uint32_t>(palette_idx_i);
        const uint32_t base_x = (palette_idx % 64u) * sh_coeffs;
        const uint32_t y = palette_idx / 64u;
        const size_t centroid_base = static_cast<size_t>(palette_idx) * sh_dims;

        for (uint32_t coeff = 0; coeff < sh_coeffs; ++coeff) {
          const size_t pixel =
              (static_cast<size_t>(y) * centroid_width + base_x + coeff) * 4;
          const size_t scalar = centroid_base + static_cast<size_t>(coeff) * 3;
          centroids_rgba[pixel + 0] = shn_scalar_indices[scalar + 0];
          centroids_rgba[pixel + 1] = shn_scalar_indices[scalar + 1];
          centroids_rgba[pixel + 2] = shn_scalar_indices[scalar + 2];
          centroids_rgba[pixel + 3] = 255;
        }
      }

      std::vector<uint8_t> labels_rgba(tex_size * 4, 0);
      GF_OMP_PARALLEL_FOR
      for (int64_t i = 0; i < static_cast<int64_t>(count); ++i) {
        const uint16_t label = palette.labels[static_cast<size_t>(i)];
        labels_rgba[static_cast<size_t>(i) * 4 + 0] =
            static_cast<uint8_t>(label & 0xFF);
        labels_rgba[static_cast<size_t>(i) * 4 + 1] =
            static_cast<uint8_t>((label >> 8) & 0xFF);
        labels_rgba[static_cast<size_t>(i) * 4 + 3] = 255;
      }

      meta_json["shN"]["count"] = palette_size;
      meta_json["shN"]["bands"] = sh_bands;
      meta_json["shN"]["codebook"] = shn_codebook;
      meta_json["shN"]["files"] = {"shN_centroids.webp", "shN_labels.webp"};

      zip.AddFile("shN_centroids.webp",
                  EncodeWebPLossless(centroids_rgba,
                                     static_cast<int>(centroid_width),
                                     static_cast<int>(centroid_height)));
      zip.AddFile("shN_labels.webp",
                  EncodeWebPLossless(labels_rgba, width, height));
    }

    std::string meta_str = meta_json.dump(2);
    std::vector<uint8_t> meta_bytes(meta_str.begin(), meta_str.end());
    zip.AddFile("meta.json", meta_bytes);

    return zip.Finalize();
  }
};

} // namespace

std::unique_ptr<IGaussWriter> MakeSogWriter() {
  return std::make_unique<SogWriter>();
}

} // namespace gf
