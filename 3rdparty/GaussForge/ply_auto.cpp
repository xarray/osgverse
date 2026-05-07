#include "gf/io/ply_auto.h"

#include "gf/core/errors.h"
#include "gf/core/gauss_ir.h"
#include "gf/io/ply.h"
#include "gf/io/ply_compressed.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <unordered_set>
#include <vector>

namespace gf {

namespace {

constexpr int CHUNK_SIZE = 256;

// Helper function to read lines and skip comments from memory buffer
bool getlineSkipComment(const uint8_t *&data, size_t &remaining,
                        std::string &line) {
  while (remaining > 0) {
    // Find line end
    size_t lineEnd = 0;
    bool found = false;
    for (size_t i = 0; i < remaining; ++i) {
      if (data[i] == '\n') {
        lineEnd = i;
        found = true;
        break;
      }
    }
    if (!found) {
      lineEnd = remaining;
    }

    // Extract line content
    line.assign(reinterpret_cast<const char *>(data), lineEnd);
    data += lineEnd + (found ? 1 : 0);
    remaining -= lineEnd + (found ? 1 : 0);

    // Remove trailing whitespace
    auto end = line.find_last_not_of(" \t\r\n");
    if (end != std::string::npos) {
      line = line.substr(0, end + 1);
    } else {
      line.clear();
    }

    auto start = line.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
      continue;
    std::string trimmed = line.substr(start);
    if (trimmed.rfind("comment", 0) == 0)
      continue;
    line = trimmed;
    return true;
  }
  return false;
}

// Check if PLY file is compressed format based on header structure
bool IsCompressedPly(const uint8_t *data, size_t size) {
  if (data == nullptr || size == 0)
    return false;

  const uint8_t *current = data;
  size_t remaining = size;
  std::string line;

  // Check PLY magic
  if (!getlineSkipComment(current, remaining, line) || line != "ply")
    return false;

  // Check format
  if (!getlineSkipComment(current, remaining, line) ||
      line != "format binary_little_endian 1.0")
    return false;

  // Expected chunk properties (18 float32 properties)
  const std::vector<std::string> chunkProperties = {
      "min_x",       "min_y",       "min_z",       "max_x",       "max_y",
      "max_z",       "min_scale_x", "min_scale_y", "min_scale_z", "max_scale_x",
      "max_scale_y", "max_scale_z", "min_r",       "min_g",       "min_b",
      "max_r",       "max_g",       "max_b"};

  // Expected vertex properties (4 uint32 properties)
  const std::vector<std::string> vertexProperties = {
      "packed_position", "packed_rotation", "packed_scale", "packed_color"};

  // Valid SH coefficient counts
  const std::unordered_set<int> validShCoeffs = {9, 24, 45};

  // Parse header to collect element information
  struct ElementInfo {
    std::string name;
    int count;
    std::vector<std::pair<std::string, std::string>> properties; // (type, name)
  };

  std::vector<ElementInfo> elements;
  ElementInfo *currentElement = nullptr;

  while (getlineSkipComment(current, remaining, line)) {
    if (line == "end_header")
      break;

    if (line.rfind("element ", 0) == 0) {
      std::string rest = line.substr(8);
      size_t spacePos = rest.find(' ');
      if (spacePos == std::string::npos)
        return false;
      std::string name = rest.substr(0, spacePos);
      int count = std::stoi(rest.substr(spacePos + 1));
      elements.push_back({name, count, {}});
      currentElement = &elements.back();
    } else if (line.rfind("property ", 0) == 0 && currentElement) {
      std::string rest = line.substr(9);
      size_t firstSpace = rest.find(' ');
      if (firstSpace == std::string::npos)
        return false;
      std::string type = rest.substr(0, firstSpace);
      std::string name = rest.substr(firstSpace + 1);
      currentElement->properties.push_back({type, name});
    }
  }

  // Check element count (should be 2 or 3)
  if (elements.size() != 2 && elements.size() != 3)
    return false;

  // Find chunk element
  auto chunkIt =
      std::find_if(elements.begin(), elements.end(),
                   [](const ElementInfo &e) { return e.name == "chunk"; });
  if (chunkIt == elements.end())
    return false;

  // Check chunk properties (check existence and type, not order)
  if (chunkIt->properties.size() != chunkProperties.size())
    return false;
  for (const auto &expectedProp : chunkProperties) {
    bool found = false;
    for (const auto &prop : chunkIt->properties) {
      if (prop.first == "float" && prop.second == expectedProp) {
        found = true;
        break;
      }
    }
    if (!found)
      return false;
  }

  // Find vertex element
  auto vertexIt =
      std::find_if(elements.begin(), elements.end(),
                   [](const ElementInfo &e) { return e.name == "vertex"; });
  if (vertexIt == elements.end())
    return false;

  // Check vertex properties (check existence and type, not order)
  if (vertexIt->properties.size() != vertexProperties.size())
    return false;
  for (const auto &expectedProp : vertexProperties) {
    bool found = false;
    for (const auto &prop : vertexIt->properties) {
      if (prop.first == "uint" && prop.second == expectedProp) {
        found = true;
        break;
      }
    }
    if (!found)
      return false;
  }

  // Check chunk count matches: ceil(vertex_count / CHUNK_SIZE) == chunk_count
  int expectedChunks = static_cast<int>(std::ceil(
      static_cast<double>(vertexIt->count) / static_cast<double>(CHUNK_SIZE)));
  if (chunkIt->count != expectedChunks)
    return false;

  // Check optional SH element if present
  if (elements.size() == 3) {
    auto shIt =
        std::find_if(elements.begin(), elements.end(),
                     [](const ElementInfo &e) { return e.name == "sh"; });
    if (shIt == elements.end())
      return false;

    // Check SH coefficient count
    if (validShCoeffs.find(shIt->properties.size()) == validShCoeffs.end())
      return false;

    // Check all SH properties are uint8 and named f_rest_0, f_rest_1, etc.
    // Build set of expected property names
    std::unordered_set<std::string> expectedShNames;
    for (size_t i = 0; i < shIt->properties.size(); ++i) {
      expectedShNames.insert("f_rest_" + std::to_string(i));
    }

    // Check each property exists, has correct type, and no duplicates
    std::unordered_set<std::string> foundShNames;
    for (const auto &prop : shIt->properties) {
      if (prop.first != "uchar")
        return false;
      if (expectedShNames.find(prop.second) == expectedShNames.end())
        return false;
      // Check for duplicates
      if (foundShNames.find(prop.second) != foundShNames.end())
        return false;
      foundShNames.insert(prop.second);
    }

    // Ensure all expected properties were found
    if (foundShNames.size() != expectedShNames.size())
      return false;

    // Check SH row count matches vertex count
    if (shIt->count != vertexIt->count)
      return false;
  }

  return true;
}

} // namespace

class PlyAutoReader : public IGaussReader {
public:
  Expected<GaussianCloudIR> Read(const uint8_t *data, size_t size,
                                 const ReadOptions &options) override {
    // Detect format by checking header structure
    if (IsCompressedPly(data, size)) {
      // Use compressed reader
      auto reader = MakePlyCompressedReader();
      return reader->Read(data, size, options);
    } else {
      // Use standard reader
      auto reader = MakePlyReader();
      return reader->Read(data, size, options);
    }
  }
};

std::unique_ptr<IGaussReader> MakePlyAutoReader() {
  return std::make_unique<PlyAutoReader>();
}

} // namespace gf
