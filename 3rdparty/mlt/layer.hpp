#pragma once

#include <mlt/feature.hpp>
#include <mlt/properties.hpp>
#include <mlt/util/noncopyable.hpp>

#include <vector>

namespace mlt {

namespace geometry {
struct GeometryVector;
}

class Layer : public util::noncopyable {
public:
    using extent_t = std::uint32_t;

    Layer() = delete;
    Layer(std::string name_,
          extent_t extent_,
          std::unique_ptr<geometry::GeometryVector>&& geometryVector_,
          std::vector<Feature> features_,
          PropertyVecMap properties_) noexcept;
    ~Layer();

    // Hand-written instead of "= default": MSVC 14.16 (VS2017) STL's unordered_map move constructor
    // is not marked noexcept, which makes the defaulted noexcept move ctor "defined as deleted"
    // and breaks std::vector<Layer>::push_back (C2280). Newer STLs are noexcept, so this only
    // restores the intended behavior on old toolsets.
    Layer(Layer&&) noexcept;
    Layer& operator=(Layer&&) = default;

    const std::string& getName() const noexcept { return name; }
    extent_t getExtent() const noexcept { return extent; }
    const std::vector<Feature>& getFeatures() const noexcept { return features; }
    const PropertyVecMap& getProperties() const { return properties; }

private:
    std::string name;
    extent_t extent;

    // Retain the geometry vector because features may reference data from it rather than making copies
    std::unique_ptr<geometry::GeometryVector> geometryVector;

    std::vector<Feature> features;
    PropertyVecMap properties;
};

} // namespace mlt
