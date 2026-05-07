#include "gf/io/reader.h"
#include "gf/io/spz.h"

#include <cstddef>
#include <cstdint>
#include <utility>

#include "load-spz.h"
#include "splat-types.h"

#include "gf/core/errors.h"
#include "gf/core/gauss_ir.h"
#include "gf/core/metadata.h"
#include "gf/core/validate.h"

namespace gf {

namespace {

GaussianCloudIR ToIR(const spz::GaussianCloud &g) {
  GaussianCloudIR ir;
  ir.numPoints = g.numPoints;
  ir.meta.shDegree = g.shDegree;
  ir.meta.antialiased = g.antialiased;
  ir.meta.sourceFormat = "spz";
  ir.positions = g.positions;
  ir.scales = g.scales;
  ir.alphas = g.alphas;
  ir.colors = g.colors;
  ir.sh = g.sh;

  ir.rotations.resize(g.rotations.size());
  for (size_t i = 0; i < static_cast<size_t>(g.numPoints); ++i) {
    const size_t idx = i * 4;
    float x = g.rotations[idx + 0];
    float y = g.rotations[idx + 1];
    float z = g.rotations[idx + 2];
    float w = g.rotations[idx + 3];

    ir.rotations[idx + 0] = w;
    ir.rotations[idx + 1] = x;
    ir.rotations[idx + 2] = y;
    ir.rotations[idx + 3] = z;
  }
  return ir;
}

} // namespace

class SpzReader : public IGaussReader {
public:
  Expected<GaussianCloudIR> Read(const uint8_t *data, size_t size,
                                 const ReadOptions &options) override {
    try {
      if (data == nullptr || size == 0) {
        return Expected<GaussianCloudIR>(
            MakeError("spz read failed: empty input"));
      }

      // Use memory buffer version of loadSpz directly
      spz::UnpackOptions unpack;
      spz::GaussianCloud g =
          spz::loadSpz(data, static_cast<int32_t>(size), unpack);

      GaussianCloudIR ir = ToIR(g);
      const auto err = ValidateBasic(ir, options.strict);
      if (!err.message.empty() && options.strict)
        return Expected<GaussianCloudIR>(err);
      return Expected<GaussianCloudIR>(std::move(ir));
    } catch (const std::exception &e) {
      return Expected<GaussianCloudIR>(
          MakeError(std::string("spz read failed: ") + e.what()));
    }
  }
};

std::unique_ptr<IGaussReader> MakeSpzReader() {
  return std::make_unique<SpzReader>();
}

} // namespace gf
