#pragma once

#include <mapbox/kdbush.hpp>
#include <mapbox/feature.hpp>
#include <mapbox/geometry/point_arithmetic.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <memory>
#include <sstream>
#include <vector>

#ifdef DEBUG_TIMER
#include <chrono>
#include <iostream>
#endif

#ifndef M_PI
#    define M_PI 3.14159265358979323846
#endif

namespace mapbox {
namespace supercluster {

using namespace mapbox::geometry;
using namespace mapbox::feature;

class Cluster {
public:
    const point<double> pos;
    const std::uint32_t num_points;
    std::uint32_t id;
    std::uint32_t parent_id = 0;
    bool visited = false;
    std::unique_ptr<property_map> properties{ nullptr };

    Cluster(const point<double> &pos_, const std::uint32_t num_points_, const std::uint32_t id_)
        : pos(pos_), num_points(num_points_), id(id_) {
    }

    Cluster(const point<double> &pos_,
            const std::uint32_t num_points_,
            const std::uint32_t id_,
            const property_map &properties_)
        : pos(pos_), num_points(num_points_), id(id_) {
        if (!properties_.empty()) {
            properties = std::make_unique<property_map>(properties_);
        }
    }

    mapbox::feature::feature<double> toGeoJSON() const {
        const double x = (pos.x - 0.5) * 360.0;
        const double y =
            360.0 * std::atan(std::exp((180.0 - pos.y * 360.0) * M_PI / 180)) / M_PI - 90.0;
        return { point<double>{ x, y }, getProperties(),
                 identifier(static_cast<std::uint64_t>(id)) };
    }

    property_map getProperties() const {
        property_map result{ { "cluster", true },
                             { "cluster_id", static_cast<std::uint64_t>(id) },
                             { "point_count", static_cast<std::uint64_t>(num_points) } };
        std::stringstream ss;
        if (num_points >= 1000) {
            ss << std::fixed;
            if (num_points < 10000) {
                ss << std::setprecision(1);
            }
            ss << double(num_points) / 1000 << "k";
        } else {
            ss << num_points;
        }
        result.emplace("point_count_abbreviated", ss.str());
        if (properties) {
            for (const auto &property : *properties) {
                result.emplace(property);
            }
        }
        return result;
    }
};

} // namespace supercluster
} // namespace mapbox

namespace kdbush {

using Cluster = mapbox::supercluster::Cluster;

template <>
struct nth<0, Cluster> {
    inline static double get(const Cluster &c) {
        return c.pos.x;
    };
};
template <>
struct nth<1, Cluster> {
    inline static double get(const Cluster &c) {
        return c.pos.y;
    };
};

} // namespace kdbush

namespace mapbox {
namespace supercluster {

#ifdef DEBUG_TIMER
class Timer {
public:
    std::chrono::high_resolution_clock::time_point started;
    Timer() {
        started = std::chrono::high_resolution_clock::now();
    }
    void operator()(std::string msg) {
        const auto now = std::chrono::high_resolution_clock::now();
        const auto ms = std::chrono::duration_cast<std::chrono::microseconds>(now - started);
        std::cerr << msg << ": " << double(ms.count()) / 1000 << "ms\n";
        started = now;
    }
};
#endif

struct Options {
    std::uint8_t minZoom = 0;   // min zoom to generate clusters on
    std::uint8_t maxZoom = 16;  // max zoom level to cluster the points on
    std::uint16_t radius = 40;  // cluster radius in pixels
    std::uint16_t extent = 512; // tile extent (radius is calculated relative to it)
    std::size_t minPoints = 2;  // minimum points to form a cluster
    bool generateId = false;    // whether to generate numeric ids for input features (in vector tiles)

    std::function<property_map(const property_map &)> map =
        [](const property_map &p) -> property_map { return p; };
    std::function<void(property_map &, const property_map &)> reduce{ nullptr };
};

class Supercluster {
    using GeoJSONPoint = point<double>;
    using GeoJSONFeature = mapbox::feature::feature<double>;
    using GeoJSONFeatures = feature_collection<double>;

    using TilePoint = point<std::int16_t>;
    using TileFeature = mapbox::feature::feature<std::int16_t>;
    using TileFeatures = feature_collection<std::int16_t>;

public:
    const GeoJSONFeatures features;
    const Options options;

    Supercluster(const GeoJSONFeatures &features_, Options options_ = Options())
        : features(features_), options(std::move(options_)) {

#ifdef DEBUG_TIMER
        Timer timer;
#endif
        // convert and index initial points
        zooms.emplace(options.maxZoom + 1, Zoom(features, options));
#ifdef DEBUG_TIMER
        timer(std::to_string(features.size()) + " initial points");
#endif
        for (int z = options.maxZoom; z >= options.minZoom; z--) {
            // cluster points from the previous zoom level
            const double r = options.radius / (options.extent * std::pow(2, z));
            zooms.emplace(z, Zoom(zooms[z + 1], r, z, options));
#ifdef DEBUG_TIMER
            timer(std::to_string(zooms[z].clusters.size()) + " clusters");
#endif
        }
    }

    TileFeatures
    getTile(const std::uint8_t z, const std::uint32_t x_, const std::uint32_t y) const {
        TileFeatures result;

        const auto zoom_iter = zooms.find(limitZoom(z));
        assert(zoom_iter != zooms.end());
        const auto &zoom = zoom_iter->second;

        std::uint32_t z2 = (std::uint32_t)std::pow(2, z);
        const double r = static_cast<double>(options.radius) / options.extent;
        std::int32_t x = x_;

        const auto visitor = [&, this](const auto &id) {
            assert(id < zoom.clusters.size());
            const auto &c = zoom.clusters[id];

            const TilePoint point(::round(this->options.extent * (c.pos.x * z2 - x)),
                                  ::round(this->options.extent * (c.pos.y * z2 - y)));

            if (c.num_points == 1) {
                const auto &original_feature = this->features[c.id];
                // Generate feature id if options.generateId is set.
                auto featureId = options.generateId ? identifier{static_cast<std::uint64_t>(c.id)} : original_feature.id;
                result.emplace_back(point, original_feature.properties, std::move(featureId));
            } else {
                result.emplace_back(point, c.getProperties(),
                                    identifier(static_cast<std::uint64_t>(c.id)));
            }
        };

        const double top = (y - r) / z2;
        const double bottom = (y + 1 + r) / z2;

        zoom.tree.range((x - r) / z2, top, (x + 1 + r) / z2, bottom, visitor);

        if (x_ == 0) {
            x = z2;
            zoom.tree.range(1 - r / z2, top, 1, bottom, visitor);
        }
        if (x_ == z2 - 1) {
            x = -1;
            zoom.tree.range(0, top, r / z2, bottom, visitor);
        }

        return result;
    }

    GeoJSONFeatures getChildren(const std::uint32_t cluster_id) const {
        GeoJSONFeatures children;
        eachChild(cluster_id,
                  [&, this](const auto &c) { children.push_back(this->clusterToGeoJSON(c)); });
        return children;
    }

    GeoJSONFeatures getLeaves(const std::uint32_t cluster_id,
                              const std::uint32_t limit = 10,
                              const std::uint32_t offset = 0) const {
        GeoJSONFeatures leaves;
        std::uint32_t skipped = 0;
        std::uint32_t limit_ = limit;
        eachLeaf(cluster_id, limit_, offset, skipped,
                 [&, this](const auto &c) { leaves.push_back(this->clusterToGeoJSON(c)); });
        return leaves;
    }

    std::uint8_t getClusterExpansionZoom(std::uint32_t cluster_id) const {
        auto cluster_zoom = (cluster_id % 32) - 1;
        while (cluster_zoom <= options.maxZoom) {
            std::uint32_t num_children = 0;

            eachChild(cluster_id, [&](const auto &c) {
                num_children++;
                cluster_id = c.id;
            });

            cluster_zoom++;

            if (num_children != 1)
                break;
        }
        return cluster_zoom;
    }

private:
    struct Zoom {
        kdbush::KDBush<Cluster, std::uint32_t> tree;
        std::vector<Cluster> clusters;

        Zoom() = default;

        Zoom(const GeoJSONFeatures &features_, const Options &options_) {
            // generate a cluster object for each point
            std::uint32_t i = 0;
            clusters.reserve(features_.size());
            for (const auto &f : features_) {
                if (options_.reduce) {
                    const auto clusterProperties = options_.map(f.properties);
                    clusters.emplace_back(project(f.geometry.get<GeoJSONPoint>()), 1, i++,
                                          clusterProperties);
                } else {
                    clusters.emplace_back(project(f.geometry.get<GeoJSONPoint>()), 1, i++);
                }
            }
            tree.fill(clusters);
        }

        Zoom(Zoom &previous, const double r, const std::uint8_t zoom, const Options &options_) {

            // The zoom parameter is restricted to [minZoom, maxZoom] by caller
            assert(((zoom + 1) & 0b11111) == (zoom + 1));

            // Since point index is encoded in the upper 27 bits, clamp the count of clusters
            const auto previous_clusters_size = std::min(
                previous.clusters.size(), static_cast<std::vector<Cluster>::size_type>(0x7ffffff));

            for (std::size_t i = 0; i < previous_clusters_size; i++) {
                auto &p = previous.clusters[i];

                if (p.visited) {
                    continue;
                }

                p.visited = true;

                const auto num_points_origin = p.num_points;
                auto num_points = num_points_origin;
                auto cluster_size = previous.clusters.size();
                // count the number of points in a potential cluster
                previous.tree.within(p.pos.x, p.pos.y, r, [&](const auto &neighbor_id) {
                    assert(neighbor_id < cluster_size);
                    const auto &b = previous.clusters[neighbor_id];
                    // filter out neighbors that are already processed
                    if (!b.visited) {
                        num_points += b.num_points;
                    }
                });

                auto clusterProperties = p.properties ? *p.properties : property_map{};
                if (num_points >= options_.minPoints) { // enough points to form a cluster
                    point<double> weight = p.pos * double(num_points_origin);
                    std::uint32_t id = static_cast<std::uint32_t>((i << 5) + (zoom + 1));

                    // find all nearby points
                    previous.tree.within(p.pos.x, p.pos.y, r, [&](const auto &neighbor_id) {
                        assert(neighbor_id < cluster_size);
                        auto &b = previous.clusters[neighbor_id];

                        // filter out neighbors that are already processed
                        if (b.visited) {
                            return;
                        }

                        b.visited = true;
                        b.parent_id = id;

                        // accumulate coordinates for calculating weighted center
                        weight += b.pos * double(b.num_points);

                        if (options_.reduce && b.properties) {
                            // apply reduce function to update clusterProperites
                            options_.reduce(clusterProperties, *b.properties);
                        }
                    });
                    p.parent_id = id;
                    clusters.emplace_back(weight / double(num_points), num_points, id,
                                          clusterProperties);
                } else {
                    clusters.emplace_back(p.pos, 1, p.id, clusterProperties);
                    if (num_points > 1) {
                        previous.tree.within(p.pos.x, p.pos.y, r, [&](const auto &neighbor_id) {
                            assert(neighbor_id < cluster_size);
                            auto &b = previous.clusters[neighbor_id];
                            // filter out neighbors that are already processed
                            if (b.visited) {
                                return;
                            }
                            b.visited = true;
                            clusters.emplace_back(b.pos, 1, b.id,
                                                  b.properties ? *b.properties : property_map{});
                        });
                    }
                }
            }

            tree.fill(clusters);
        }
    };

    std::unordered_map<std::uint8_t, Zoom> zooms;

    std::uint8_t limitZoom(const std::uint8_t z) const {
        if (z < options.minZoom)
            return options.minZoom;
        if (z > options.maxZoom + 1)
            return options.maxZoom + 1;
        return z;
    }

    template <typename TVisitor>
    void eachChild(const std::uint32_t cluster_id, const TVisitor &visitor) const {
        const auto origin_id = cluster_id >> 5;
        const auto origin_zoom = cluster_id % 32;

        const auto zoom_iter = zooms.find(origin_zoom);
        if (zoom_iter == zooms.end()) {
            throw std::runtime_error("No cluster with the specified id.");
        }

        auto &zoom = zoom_iter->second;
        if (origin_id >= zoom.clusters.size()) {
            throw std::runtime_error("No cluster with the specified id.");
        }

        const double r = options.radius / (double(options.extent) * std::pow(2, origin_zoom - 1));
        const auto &origin = zoom.clusters[origin_id];

        bool hasChildren = false;

        zoom.tree.within(origin.pos.x, origin.pos.y, r, [&](const auto &id) {
            assert(id < zoom.clusters.size());
            const auto &cluster_child = zoom.clusters[id];
            if (cluster_child.parent_id == cluster_id) {
                visitor(cluster_child);
                hasChildren = true;
            }
        });

        if (!hasChildren) {
            throw std::runtime_error("No cluster with the specified id.");
        }
    }

    template <typename TVisitor>
    void eachLeaf(const std::uint32_t cluster_id,
                  std::uint32_t &limit,
                  const std::uint32_t offset,
                  std::uint32_t &skipped,
                  const TVisitor &visitor) const {

        eachChild(cluster_id, [&, this](const auto &cluster_leaf) {
            if (limit == 0)
                return;
            if (cluster_leaf.num_points > 1) {
                if (skipped + cluster_leaf.num_points <= offset) {
                    // skip the whole cluster
                    skipped += cluster_leaf.num_points;
                } else {
                    // enter the cluster
                    this->eachLeaf(cluster_leaf.id, limit, offset, skipped, visitor);
                    // exit the cluster
                }
            } else if (skipped < offset) {
                // skip a single point
                skipped++;
            } else {
                // visit a single point
                visitor(cluster_leaf);
                limit--;
            }
        });
    }

    GeoJSONFeature clusterToGeoJSON(const Cluster &c) const {
        return c.num_points == 1 ? features[c.id] : c.toGeoJSON();
    }

    static point<double> project(const GeoJSONPoint &p) {
        const auto lngX = p.x / 360 + 0.5;
        const double sine = std::sin(p.y * M_PI / 180);
        const double y = 0.5 - 0.25 * std::log((1 + sine) / (1 - sine)) / M_PI;
        const auto latY = std::min(std::max(y, 0.0), 1.0);
        return { lngX, latY };
    }
};

} // namespace supercluster
} // namespace mapbox
