#include "gf/io/spz.h"

#include <vector>

#include "load-spz.h"
#include "splat-types.h"

#include "gf/core/errors.h"
#include "gf/core/gauss_ir.h"
#include "gf/core/validate.h"

namespace gf {

namespace {

spz::GaussianCloud ToSpz(const GaussianCloudIR &ir) {
  spz::GaussianCloud g;
  g.numPoints = ir.numPoints;
  g.shDegree = ir.meta.shDegree;
  g.antialiased = ir.meta.antialiased;
  g.positions = ir.positions;
  g.scales = ir.scales;
  g.alphas = ir.alphas;
  g.colors = ir.colors;
  g.sh = ir.sh;

  g.rotations.resize(ir.rotations.size());
  for (int i = 0; i < ir.numPoints; ++i) {
    const size_t idx = i * 4;
    float w = ir.rotations[idx + 0];
    float x = ir.rotations[idx + 1];
    float y = ir.rotations[idx + 2];
    float z = ir.rotations[idx + 3];

    g.rotations[idx + 0] = x;
    g.rotations[idx + 1] = y;
    g.rotations[idx + 2] = z;
    g.rotations[idx + 3] = w;
  }
  return g;
}

} // namespace

class SpzWriter : public IGaussWriter {
public:
  Expected<std::vector<uint8_t>> Write(const GaussianCloudIR &ir,
                                       const WriteOptions &options) override {
    const auto err = ValidateBasic(ir, options.strict);
    if (!err.message.empty() && options.strict)
      return Expected<std::vector<uint8_t>>(err);

    spz::GaussianCloud g = ToSpz(ir);
    spz::PackOptions pack;
    std::vector<uint8_t> result;
    bool ok = spz::saveSpz(g, pack, &result);
    if (!ok) {
      return Expected<std::vector<uint8_t>>(MakeError("spz write failed"));
    }

    return Expected<std::vector<uint8_t>>(std::move(result));
  }
};

std::unique_ptr<IGaussWriter> MakeSpzWriter() {
  return std::make_unique<SpzWriter>();
}

} // namespace gf
