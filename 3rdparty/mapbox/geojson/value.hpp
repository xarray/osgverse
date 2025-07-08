#pragma once

#include <mapbox/geojson.hpp>

namespace mapbox {
namespace geojson {

// Converts Value to GeoJSON type.
geojson convert(const mapbox::geojson::value&);

// Converts GeoJSON type to Value.
mapbox::geojson::value convert(const geojson&);

} // namespace geojson
} // namespace mapbox
