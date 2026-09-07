#include "binding.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <queue>
#include <limits>
#include <unordered_map>

namespace skintokens::detail {
namespace {

float squared_distance(vec3 left, vec3 right) {
    const float x = left.x - right.x;
    const float y = left.y - right.y;
    const float z = left.z - right.z;
    return x*x + y*y + z*z;
}

float distance(vec3 left, vec3 right) {
    return std::sqrt(squared_distance(left, right));
}

struct grid_cell {
    std::int64_t x, y, z;
    bool operator==(const grid_cell& c) const
    { return x == c.x && y == c.y && z == c.z; }
};

struct grid_cell_hash {
    std::size_t operator()(const grid_cell & value) const noexcept {
        // Copied from Open3D 0.19 utility::hash_eigen. VoxelGrid stores its
        // cells in an unordered_map and get_voxels() exposes that iteration
        // order; upstream then feeds the resulting order to SciPy cKDTree.
        std::size_t result = 0U;
        result ^= std::hash<std::int64_t>{}(value.x) + 0x9e3779b9U + (result << 6U) + (result >> 2U);
        result ^= std::hash<std::int64_t>{}(value.y) + 0x9e3779b9U + (result << 6U) + (result >> 2U);
        result ^= std::hash<std::int64_t>{}(value.z) + 0x9e3779b9U + (result << 6U) + (result >> 2U);
        return result;
    }
};

struct dvec3 { double x, y, z; };

// Minimal k=1 port of SciPy 1.15.3 cKDTree's default balanced/compact tree.
// SkinTokens uses cKDTree at the final voxel_skin stage, and its first-hit
// behaviour for coincident seam vertices affects which connected component a
// joint seeds. Keep this local implementation in lock-step with that pinned
// reference instead of silently choosing the lowest vertex index.
class scipy_point_tree {
    struct node {
        std::size_t begin = 0U;
        std::size_t end = 0U;
        std::int32_t split_dimension = -1;
        double split = 0.0;
        std::size_t less = 0U;
        std::size_t greater = 0U;
    };
    struct pending_item {
        double priority;
        std::size_t node_index;
        std::array<double, 3> side_distances;
    };

    std::vector<dvec3> points_;
    std::vector<std::size_t> order_;
    std::vector<node> nodes_;
    std::array<double, 3> minimum_{};
    std::array<double, 3> maximum_{};

    static double coordinate(const dvec3 & value, std::size_t dimension) {
        return dimension == 0U ? value.x : dimension == 1U ? value.y : value.z;
    }

    std::size_t build(std::size_t begin, std::size_t end) {
        const auto result = nodes_.size();
        nodes_.push_back({begin, end});
        if (end - begin <= 16U) return result;

        std::array<double, 3> low{}, high{};
        for (std::size_t dimension = 0U; dimension < 3U; ++dimension)
            low[dimension] = high[dimension] = coordinate(points_[order_[begin]], dimension);
        for (std::size_t item = begin + 1U; item < end; ++item)
            for (std::size_t dimension = 0U; dimension < 3U; ++dimension) {
                const auto value = coordinate(points_[order_[item]], dimension);
                low[dimension] = std::min(low[dimension], value);
                high[dimension] = std::max(high[dimension], value);
            }
        std::size_t dimension = 0U;
        double spread = 0.0;
        for (std::size_t candidate = 0U; candidate < 3U; ++candidate)
            if (high[candidate] - low[candidate] > spread) {
                dimension = candidate;
                spread = high[candidate] - low[candidate];
            }
        if (high[dimension] == low[dimension]) return result;

        const auto compare = [&](std::size_t left, std::size_t right) {
            return coordinate(points_[left], dimension) < coordinate(points_[right], dimension);
        };
        auto first = order_.begin() + static_cast<std::ptrdiff_t>(begin);
        auto middle = first + static_cast<std::ptrdiff_t>((end - begin) / 2U);
        auto last = order_.begin() + static_cast<std::ptrdiff_t>(end);
        std::nth_element(first, middle, last, compare);
        double split = coordinate(points_[*middle], dimension);
        auto partition = std::partition(first, middle, [&](std::size_t index) {
            return coordinate(points_[index], dimension) < split;
        });
        auto pivot = static_cast<std::size_t>(partition - order_.begin());
        if (pivot == begin) {
            const auto smallest = *std::min_element(first, last, compare);
            split = std::nextafter(coordinate(points_[smallest], dimension),
                                   std::numeric_limits<double>::infinity());
            partition = std::partition(first, last, [&](std::size_t index) {
                return coordinate(points_[index], dimension) < split;
            });
            pivot = static_cast<std::size_t>(partition - order_.begin());
        } else if (pivot == end) {
            const auto largest = *std::max_element(first, last, compare);
            split = coordinate(points_[largest], dimension);
            partition = std::partition(first, last, [&](std::size_t index) {
                return coordinate(points_[index], dimension) < split;
            });
            pivot = static_cast<std::size_t>(partition - order_.begin());
        }
        if (pivot == begin || pivot == end) return result;

        const auto less = build(begin, pivot);
        const auto greater = build(pivot, end);
        nodes_[result].split_dimension = static_cast<std::int32_t>(dimension);
        nodes_[result].split = split;
        nodes_[result].less = less;
        nodes_[result].greater = greater;
        return result;
    }

    // SciPy's heap compares priorities only and leaves equal-priority entries
    // in insertion order. Reproduce that rather than adding an index tie-break.
    static void heap_push(std::vector<pending_item> & heap, pending_item item) {
        heap.push_back(item);
        std::size_t index = heap.size() - 1U;
        while (index > 0U) {
            const auto parent = (index - 1U) / 2U;
            if (!(heap[index].priority < heap[parent].priority)) break;
            std::swap(heap[index], heap[parent]);
            index = parent;
        }
    }

    static pending_item heap_pop(std::vector<pending_item> & heap) {
        auto output = heap.front();
        heap.front() = heap.back();
        heap.pop_back();
        std::size_t index = 0U;
        while (true) {
            const auto left = index * 2U + 1U;
            const auto right = left + 1U;
            if (left >= heap.size()) break;
            auto selected = left;
            if (right < heap.size() && heap[left].priority > heap[right].priority)
                selected = right;
            if (!(heap[index].priority > heap[selected].priority)) break;
            std::swap(heap[index], heap[selected]);
            index = selected;
        }
        return output;
    }

public:
    explicit scipy_point_tree(std::vector<dvec3> points)
        : points_(std::move(points)), order_(points_.size()) {
        if (points_.empty()) return;
        std::iota(order_.begin(), order_.end(), 0U);
        for (std::size_t dimension = 0U; dimension < 3U; ++dimension)
            minimum_[dimension] = maximum_[dimension] = coordinate(points_.front(), dimension);
        for (const auto & point : points_)
            for (std::size_t dimension = 0U; dimension < 3U; ++dimension) {
                minimum_[dimension] = std::min(minimum_[dimension], coordinate(point, dimension));
                maximum_[dimension] = std::max(maximum_[dimension], coordinate(point, dimension));
            }
        nodes_.reserve(points_.size() / 8U);
        build(0U, points_.size());
    }

    std::uint32_t nearest(dvec3 query) const {
        if (points_.empty()) return 0U;
        std::array<double, 3> side{};
        double initial_distance = 0.0;
        for (std::size_t dimension = 0U; dimension < 3U; ++dimension) {
            const auto value = coordinate(query, dimension);
            const auto delta = value < minimum_[dimension] ? minimum_[dimension] - value :
                               value > maximum_[dimension] ? value - maximum_[dimension] : 0.0;
            side[dimension] = delta * delta;
            initial_distance += side[dimension];
        }
        pending_item current{initial_distance, 0U, side};
        std::vector<pending_item> pending;
        pending.reserve(64U);
        double best_distance = std::numeric_limits<double>::infinity();
        std::size_t best = points_.size();
        while (true) {
            const auto & current_node = nodes_[current.node_index];
            if (current_node.split_dimension < 0) {
                for (std::size_t item = current_node.begin; item < current_node.end; ++item) {
                    const auto index = order_[item];
                    const auto dx = points_[index].x - query.x;
                    const auto dy = points_[index].y - query.y;
                    const auto dz = points_[index].z - query.z;
                    const auto candidate = dx*dx + dy*dy + dz*dz;
                    if (candidate < best_distance) {
                        best_distance = candidate;
                        best = index;
                    }
                }
                if (pending.empty()) break;
                current = heap_pop(pending);
                continue;
            }
            if (current.priority > best_distance) break;
            const auto dimension = static_cast<std::size_t>(current_node.split_dimension);
            auto far = current;
            const auto query_coordinate = coordinate(query, dimension);
            const auto delta = query_coordinate < current_node.split ?
                current_node.split - query_coordinate : query_coordinate - current_node.split;
            if (query_coordinate < current_node.split) {
                current.node_index = current_node.less;
                far.node_index = current_node.greater;
            } else {
                current.node_index = current_node.greater;
                far.node_index = current_node.less;
            }
            const auto new_side = delta * delta;
            far.priority += new_side - far.side_distances[dimension];
            far.side_distances[dimension] = new_side;
            if (far.priority <= best_distance) heap_push(pending, far);
        }
        return static_cast<std::uint32_t>(best);
    }

    std::vector<std::pair<float, std::uint32_t>> nearest(dvec3 query, std::size_t count) const {
        if (points_.empty() || count == 0U) return {};
        count = std::min(count, points_.size());
        std::array<double, 3> side{};
        double initial_distance = 0.0;
        for (std::size_t dimension = 0U; dimension < 3U; ++dimension) {
            const auto value = coordinate(query, dimension);
            const auto delta = value < minimum_[dimension] ? minimum_[dimension] - value :
                               value > maximum_[dimension] ? value - maximum_[dimension] : 0.0;
            side[dimension] = delta * delta;
            initial_distance += side[dimension];
        }
        pending_item current{initial_distance, 0U, side};
        std::vector<pending_item> pending;
        pending.reserve(64U);
        using neighbor = std::pair<double, std::uint32_t>;
        std::priority_queue<neighbor> neighbors;
        double distance_upper_bound = std::numeric_limits<double>::infinity();
        while (true) {
            const auto & current_node = nodes_[current.node_index];
            if (current_node.split_dimension < 0) {
                for (std::size_t item = current_node.begin; item < current_node.end; ++item) {
                    const auto index = order_[item];
                    const auto dx = points_[index].x - query.x;
                    const auto dy = points_[index].y - query.y;
                    const auto dz = points_[index].z - query.z;
                    const auto candidate = dx*dx + dy*dy + dz*dz;
                    if (candidate < distance_upper_bound) {
                        if (neighbors.size() == count) neighbors.pop();
                        neighbors.emplace(candidate, static_cast<std::uint32_t>(index));
                        if (neighbors.size() == count) distance_upper_bound = neighbors.top().first;
                    }
                }
                if (pending.empty()) break;
                current = heap_pop(pending);
                continue;
            }
            if (current.priority > distance_upper_bound) break;
            const auto dimension = static_cast<std::size_t>(current_node.split_dimension);
            auto far = current;
            const auto query_coordinate = coordinate(query, dimension);
            const auto delta = query_coordinate < current_node.split ?
                current_node.split - query_coordinate : query_coordinate - current_node.split;
            if (query_coordinate < current_node.split) {
                current.node_index = current_node.less;
                far.node_index = current_node.greater;
            } else {
                current.node_index = current_node.greater;
                far.node_index = current_node.less;
            }
            const auto new_side = delta * delta;
            far.priority += new_side - far.side_distances[dimension];
            far.side_distances[dimension] = new_side;
            if (far.priority <= distance_upper_bound) heap_push(pending, far);
        }
        std::vector<std::pair<float, std::uint32_t>> output(neighbors.size());
        for (std::size_t index = output.size(); index > 0U; --index) {
            output[index - 1U] = {static_cast<float>(neighbors.top().first), neighbors.top().second};
            neighbors.pop();
        }
        return output;
    }
};

dvec3 subtract(dvec3 a, dvec3 b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
dvec3 cross(dvec3 a, dvec3 b) {
    return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}
double dot(dvec3 a, dvec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }

// Separating-axis triangle/AABB test used by Open3D's mesh voxelizer.
bool triangle_box(dvec3 center, dvec3 half, dvec3 a, dvec3 b, dvec3 c) {
    a = subtract(a, center); b = subtract(b, center); c = subtract(c, center);
    const dvec3 edges[]{subtract(b, a), subtract(c, b), subtract(a, c)};
    const dvec3 box_axes[]{{1,0,0}, {0,1,0}, {0,0,1}};
    const auto overlap = [&](dvec3 axis) {
        const double magnitude = dot(axis, axis);
        if (magnitude <= 1.0e-30) return true;
        const double pa=dot(a,axis), pb=dot(b,axis), pc=dot(c,axis);
        const double radius=half.x*std::abs(axis.x)+half.y*std::abs(axis.y)+half.z*std::abs(axis.z);
        return std::min({pa,pb,pc}) <= radius && std::max({pa,pb,pc}) >= -radius;
    };
    for (const auto axis : box_axes) if (!overlap(axis)) return false;
    if (!overlap(cross(edges[0], edges[1]))) return false;
    for (const auto edge : edges)
        for (const auto axis : box_axes)
            if (!overlap(cross(edge, axis))) return false;
    return true;
}

std::vector<grid_cell> voxelize_like_open3d(
    nonstd::span<const vec3> vertices, nonstd::span<const triangle> faces, double voxel_size) {
    dvec3 minimum{vertices.front().x, vertices.front().y, vertices.front().z};
    for (const auto value : vertices) {
        minimum.x=std::min(minimum.x,static_cast<double>(value.x));
        minimum.y=std::min(minimum.y,static_cast<double>(value.y));
        minimum.z=std::min(minimum.z,static_cast<double>(value.z));
    }
    minimum.x-=voxel_size*.5; minimum.y-=voxel_size*.5; minimum.z-=voxel_size*.5;
    const dvec3 half{voxel_size*.5,voxel_size*.5,voxel_size*.5};
    std::unordered_map<grid_cell, bool, grid_cell_hash> occupied;
    for (const auto face : faces) {
        if (face[0]>=vertices.size() || face[1]>=vertices.size() || face[2]>=vertices.size()) continue;
        const dvec3 value[]{
            {vertices[face[0]].x,vertices[face[0]].y,vertices[face[0]].z},
            {vertices[face[1]].x,vertices[face[1]].y,vertices[face[1]].z},
            {vertices[face[2]].x,vertices[face[2]].y,vertices[face[2]].z}};
        const dvec3 low{std::min({value[0].x,value[1].x,value[2].x}),
                        std::min({value[0].y,value[1].y,value[2].y}),
                        std::min({value[0].z,value[1].z,value[2].z})};
        const dvec3 high{std::max({value[0].x,value[1].x,value[2].x}),
                         std::max({value[0].y,value[1].y,value[2].y}),
                         std::max({value[0].z,value[1].z,value[2].z})};
        const std::int64_t ix=static_cast<std::int64_t>(std::floor((low.x-minimum.x)/voxel_size));
        const std::int64_t iy=static_cast<std::int64_t>(std::floor((low.y-minimum.y)/voxel_size));
        const std::int64_t iz=static_cast<std::int64_t>(std::floor((low.z-minimum.z)/voxel_size));
        const std::int64_t nx=static_cast<std::int64_t>(std::round((high.x-low.x)/voxel_size))+2;
        const std::int64_t ny=static_cast<std::int64_t>(std::round((high.y-low.y)/voxel_size))+2;
        const std::int64_t nz=static_cast<std::int64_t>(std::round((high.z-low.z)/voxel_size))+2;
        for (std::int64_t x=ix; x<ix+nx; ++x)
            for (std::int64_t y=iy; y<iy+ny; ++y)
                for (std::int64_t z=iz; z<iz+nz; ++z) {
                    const dvec3 center{minimum.x+(static_cast<double>(x)+.5)*voxel_size,
                                       minimum.y+(static_cast<double>(y)+.5)*voxel_size,
                                       minimum.z+(static_cast<double>(z)+.5)*voxel_size};
                    if (triangle_box(center, half, value[0], value[1], value[2]))
                        occupied[{x,y,z}]=true;
                }
    }
    std::vector<grid_cell> result;
    result.reserve(occupied.size());
    for (const auto & [value, unused] : occupied) result.push_back(value);
    return result;
}

// Reproduce the upstream demo's complete voxel_skin graph, including its use
// of Open3D integer grid indices as graph coordinates.
struct surface_result {
    std::vector<float> weights;
    std::vector<std::int32_t> voxel_coordinates;
    std::vector<std::uint32_t> seeds;
    std::vector<float> distances;
    std::size_t graph_edges = 0U;
    float maximum_distance = 0.0F;
    std::vector<std::uint32_t> graph_sources;
    std::vector<std::uint32_t> graph_targets;
    std::vector<float> graph_weights;
};

surface_result surface_weights(
    nonstd::span<const vec3> vertices,
    nonstd::span<const triangle> faces,
    nonstd::span<const vec3> joints,
    nonstd::span<const precise_vec3> precise_vertices = {},
    nonstd::span<const precise_vec3> precise_joints = {}) {
    // Match Asset.voxel() exactly. Upstream computes the largest coordinate
    // spread within any one vertex, rather than the mesh AABB's largest axis.
    double maximum_spread = 0.0;
    for (const auto value : vertices)
        maximum_spread=std::max(maximum_spread, static_cast<double>(
            std::max({value.x,value.y,value.z})-std::min({value.x,value.y,value.z})));
    const double voxel_size=maximum_spread/196.0;
    const auto grid=voxelize_like_open3d(vertices,faces,voxel_size);
    const std::size_t mesh_count=vertices.size(), total_count=mesh_count+grid.size();
    const auto position = [&](std::size_t index) {
        if (index<mesh_count) return vertices[index];
        const auto value=grid[index-mesh_count];
        return vec3{static_cast<float>(value.x),static_cast<float>(value.y),static_cast<float>(value.z)};
    };
    std::vector<dvec3> combined;
    combined.reserve(total_count);
    for (const auto value : vertices) combined.push_back({value.x, value.y, value.z});
    for (const auto value : grid)
        combined.push_back({static_cast<double>(value.x), static_cast<double>(value.y),
                            static_cast<double>(value.z)});
    const scipy_point_tree combined_tree{std::move(combined)};
    using edge = std::pair<std::uint32_t, float>;
    std::unordered_map<std::uint64_t, float> directed_edges;
    directed_edges.reserve(faces.size() * 3U);
    const auto connect_weighted = [&](std::uint32_t left, std::uint32_t right, float weight) {
        if (left >= total_count || right >= total_count || left == right) return;
        if (weight < 0.0F || !std::isfinite(weight)) return;
        const auto key = (static_cast<std::uint64_t>(left) << 32U) | right;
        // scipy.sparse.csr_matrix sums duplicate (row, column) entries before
        // shortest_path(..., directed=False) exposes each directed CSR entry
        // from both endpoints.
        directed_edges[key] += weight;
    };
    const auto connect = [&](std::uint32_t left, std::uint32_t right) {
        connect_weighted(left, right, distance(position(left), position(right)));
    };
    for (const auto & face : faces) {
        connect(face[0], face[1]);
        connect(face[1], face[2]);
        connect(face[2], face[0]);
    }

    // voxel_skin also links each vertex to its three nearest neighbours when
    // they are within 1e-5. This reconnects coincident vertices introduced at
    // GLB seams; omitting it splits an otherwise continuous character into
    // components and produces apparently random remote influences.
    constexpr float link_distance = 1.0e-5F;
    std::vector<dvec3> vertex_points;
    vertex_points.reserve(vertices.size());
    for (const auto value : vertices) vertex_points.push_back({value.x, value.y, value.z});
    const scipy_point_tree vertex_tree{std::move(vertex_points)};
    const auto precise_position = [&](std::size_t index) {
        if (precise_vertices.size() == vertices.size()) return precise_vertices[index];
        const auto value=vertices[index];
        return precise_vec3{value.x,value.y,value.z};
    };
    const auto close_cell = [](precise_vec3 value) {
        return grid_cell{static_cast<std::int64_t>(std::floor(value.x / link_distance)),
                         static_cast<std::int64_t>(std::floor(value.y / link_distance)),
                         static_cast<std::int64_t>(std::floor(value.z / link_distance))};
    };
    std::unordered_map<grid_cell, std::vector<std::uint32_t>, grid_cell_hash> close_buckets;
    close_buckets.reserve(vertices.size());
    for (std::size_t vertex = 0; vertex < vertices.size(); ++vertex)
        close_buckets[close_cell(precise_position(vertex))].push_back(static_cast<std::uint32_t>(vertex));
    for (std::size_t vertex = 0; vertex < vertices.size(); ++vertex) {
        std::vector<std::pair<double, std::uint32_t>> nearest;
        const auto point=precise_position(vertex);
        const auto home = close_cell(point);
        for (std::int64_t x=-1; x<=1; ++x)
            for (std::int64_t y=-1; y<=1; ++y)
                for (std::int64_t z=-1; z<=1; ++z) {
                    const auto found=close_buckets.find({home.x+x,home.y+y,home.z+z});
                    if (found==close_buckets.end()) continue;
                    for (const auto candidate : found->second) {
                        const auto other=precise_position(candidate);
                        const double dx=point.x-other.x,dy=point.y-other.y,dz=point.z-other.z;
                        const double squared=dx*dx+dy*dy+dz*dz;
                        if (squared < static_cast<double>(link_distance)*link_distance)
                            nearest.emplace_back(squared,candidate);
                    }
                }
        std::sort(nearest.begin(),nearest.end());
        if (nearest.size()>4U) nearest.resize(4U);
        // cKDTree.query(vertices, 4) discards the first returned neighbour,
        // even when coincident seam duplicates mean that it is not `vertex`.
        for (std::size_t index = 1U; index < nearest.size(); ++index) {
            const float candidate = static_cast<float>(std::sqrt(nearest[index].first));
            if (candidate > 0.0F && candidate < link_distance)
                connect_weighted(static_cast<std::uint32_t>(vertex), nearest[index].second, candidate);
        }
    }

    // With normalized inputs voxel_size*1.74 is below one, so distinct integer
    // grid coordinates never pass upstream's grid-to-grid distance mask.
    // Grid-to-mesh links can only exist for the handful of integer positions
    // lying inside the normalized mesh bounds; evaluate those exactly.
    const float grid_range=static_cast<float>(voxel_size*1.74);
    for (std::size_t grid_index=0; grid_index<grid.size(); ++grid_index) {
        const vec3 grid_position{static_cast<float>(grid[grid_index].x),
                                 static_cast<float>(grid[grid_index].y),
                                 static_cast<float>(grid[grid_index].z)};
        const auto nearest = vertex_tree.nearest(
            {grid_position.x, grid_position.y, grid_position.z}, 27U);
        for (const auto & [squared, vertex] : nearest) {
            const float candidate = std::sqrt(squared);
            if (candidate > 0.0F && candidate < grid_range)
                connect(static_cast<std::uint32_t>(mesh_count + grid_index), vertex);
        }
    }

    std::vector<std::vector<edge>> adjacency(total_count);
    for (const auto [key, weight] : directed_edges) {
        const auto left = static_cast<std::uint32_t>(key >> 32U);
        const auto right = static_cast<std::uint32_t>(key & 0xffffffffU);
        adjacency[left].emplace_back(right, weight);
        adjacency[right].emplace_back(left, weight);
    }

    const std::size_t joint_count = joints.size();
    const auto precise_joint_position = [&](std::size_t index) {
        if (precise_joints.size() == joints.size()) return precise_joints[index];
        const auto value=joints[index];
        return precise_vec3{value.x,value.y,value.z};
    };
    std::vector<float> distances(total_count * joint_count,
                                 std::numeric_limits<float>::infinity());
    using queued = std::pair<float, std::uint32_t>;
    std::vector<std::uint32_t> seeds(joint_count);
    for (std::size_t joint = 0; joint < joint_count; ++joint) {
        const auto seed = combined_tree.nearest(
            {joints[joint].x, joints[joint].y, joints[joint].z});
        auto * row = distances.data() + joint * total_count;
        seeds[joint]=seed;
        row[seed] = 0.0F;
        std::priority_queue<queued, std::vector<queued>, std::greater<>> pending;
        pending.emplace(0.0F, seed);
        while (!pending.empty()) {
            const auto [current_distance, vertex] = pending.top();
            pending.pop();
            if (current_distance != row[vertex]) continue;
            for (const auto [next, weight] : adjacency[vertex]) {
                const float candidate = current_distance + weight;
                if (candidate >= row[next]) continue;
                row[next] = candidate;
                pending.emplace(candidate, next);
            }
        }
    }

    // Upstream fills the three nearest joint distances for components that no
    // graph seed can reach before finding max_dis. The fallback distances must
    // therefore participate in the value used to replace all remaining infs.
    for (std::size_t vertex = 0; vertex < vertices.size(); ++vertex) {
        bool unreachable = true;
        for (std::size_t joint = 0; joint < joint_count; ++joint)
            unreachable = unreachable && !std::isfinite(distances[joint * total_count + vertex]);
        if (unreachable) {
            const auto point=precise_position(vertex);
            std::vector<std::pair<double,std::uint32_t>> nearest;
            nearest.reserve(joint_count);
            for (std::size_t joint=0; joint<joint_count; ++joint) {
                const auto other=precise_joint_position(joint);
                const double x=point.x-other.x,y=point.y-other.y,z=point.z-other.z;
                nearest.emplace_back(x*x+y*y+z*z,static_cast<std::uint32_t>(joint));
            }
            std::partial_sort(nearest.begin(),nearest.begin()+3,nearest.end(),
                [](const auto & left,const auto & right) {
                    return left.first!=right.first ? left.first<right.first : left.second>right.second;
                });
            for (std::size_t index=0; index<3U; ++index)
                distances[static_cast<std::size_t>(nearest[index].second)*total_count+vertex]=
                    static_cast<float>(std::sqrt(nearest[index].first));
        }
    }
    float maximum = 0.0F;
    for (std::size_t joint=0; joint<joint_count; ++joint)
        for (std::size_t vertex=0; vertex<mesh_count; ++vertex) {
            const float value=distances[joint*total_count+vertex];
            if (std::isfinite(value)) maximum=std::max(maximum,value);
        }
    maximum = std::max(maximum, 1.0e-6F);
    for (std::size_t joint=0; joint<joint_count; ++joint)
        for (std::size_t vertex=0; vertex<mesh_count; ++vertex) {
            auto & value=distances[joint*total_count+vertex];
            if (!std::isfinite(value)) value=maximum;
            value=std::max(value,1.0e-6F);
        }

    std::vector<float> output(vertices.size() * joint_count);
    for (std::size_t vertex = 0; vertex < vertices.size(); ++vertex) {
        float sum = 0.0F;
        for (std::size_t joint = 0; joint < joint_count; ++joint) {
            float value = distances[joint * total_count + vertex];
            const float denominator = 0.5F * value + 0.5F * value * value;
            const float weight = 1.0F / (denominator * denominator);
            output[vertex * joint_count + joint] = weight;
            sum += weight;
        }
        if (sum > 0.0F)
            for (std::size_t joint = 0; joint < joint_count; ++joint)
                output[vertex * joint_count + joint] /= sum;
    }
    surface_result result;
    result.weights=std::move(output);
    result.seeds=std::move(seeds);
    result.graph_edges=directed_edges.size();
    result.graph_sources.reserve(directed_edges.size());
    result.graph_targets.reserve(directed_edges.size());
    result.graph_weights.reserve(directed_edges.size());
    for (const auto [key, weight] : directed_edges) {
        result.graph_sources.push_back(static_cast<std::uint32_t>(key >> 32U));
        result.graph_targets.push_back(static_cast<std::uint32_t>(key & 0xffffffffU));
        result.graph_weights.push_back(weight);
    }
    result.maximum_distance=maximum;
    result.distances.resize(joint_count * mesh_count);
    for (std::size_t joint=0; joint<joint_count; ++joint)
        std::copy_n(distances.data()+joint*total_count, mesh_count,
                    result.distances.data()+joint*mesh_count);
    result.voxel_coordinates.reserve(grid.size()*3U);
    for (const auto value:grid) {
        result.voxel_coordinates.push_back(static_cast<std::int32_t>(value.x));
        result.voxel_coordinates.push_back(static_cast<std::int32_t>(value.y));
        result.voxel_coordinates.push_back(static_cast<std::int32_t>(value.z));
    }
    return result;
}

skin integrate(
    const skeleton & target,
    nonstd::span<const vec3> normalized_vertices,
    nonstd::span<const vec3> sampled_points,
    nonstd::span<const std::vector<float>> dense_joint_weights,
    nonstd::span<const float> locality,
    binding_trace * trace) {
    skin output;
    output.rig = target;
    output.learned = true;
    output.joints.resize(normalized_vertices.size());
    output.weights.resize(normalized_vertices.size());
    if (trace != nullptr) {
        trace->neighbor_indices.resize(normalized_vertices.size() * 8U);
        trace->interpolation_weights.resize(normalized_vertices.size() * 8U);
        trace->surface_weights.assign(locality.begin(), locality.end());
        trace->final_dense_weights.resize(normalized_vertices.size() * dense_joint_weights.size());
    }

    std::vector<dvec3> sampled_double;
    sampled_double.reserve(sampled_points.size());
    for (const auto value : sampled_points) sampled_double.push_back({value.x, value.y, value.z});
    const scipy_point_tree tree{std::move(sampled_double)};
    struct binding_candidate { float learned_weight; std::uint16_t joint; };
    for (std::size_t vertex = 0; vertex < normalized_vertices.size(); ++vertex) {
        const auto point = normalized_vertices[vertex];
        const auto neighbors = tree.nearest({point.x, point.y, point.z}, 8U);
        std::array<float, 8> interpolation{};
        float interpolation_sum = 0.0F;
        for (std::size_t index = 0; index < neighbors.size(); ++index) {
            interpolation[index] = 1.0F / (std::sqrt(neighbors[index].first) + 1e-8F);
            interpolation_sum += interpolation[index];
            if (trace != nullptr) {
                trace->neighbor_indices[vertex * 8U + index] = neighbors[index].second;
                trace->interpolation_weights[vertex * 8U + index] = interpolation[index];
            }
        }

        std::vector<binding_candidate> ranked;
        ranked.reserve(dense_joint_weights.size());
        float dense_sum = 0.0F;
        for (std::size_t joint = 0; joint < dense_joint_weights.size(); ++joint) {
            float value = 0.0F;
            for (std::size_t index = 0; index < neighbors.size(); ++index)
                value += interpolation[index] * dense_joint_weights[joint][neighbors[index].second];
            value /= interpolation_sum;
            if (!locality.empty()) value *= locality[vertex * dense_joint_weights.size() + joint];
            ranked.push_back({value, static_cast<std::uint16_t>(joint)});
            dense_sum += value;
        }
        // The demo calls Asset.normalize_skin() after multiplying by
        // voxel_skin and before Blender chooses four influences. This row
        // normalization does not change the ranking, but retaining it here
        // makes the final dense trace match the actual upstream endpoint.
        if (!locality.empty() && dense_sum > 1.0e-30F) {
            for (auto & value : ranked) value.learned_weight /= dense_sum;
        }
        if (trace != nullptr) {
            for (const auto value : ranked)
                trace->final_dense_weights[vertex * dense_joint_weights.size() + value.joint] =
                    value.learned_weight;
        }
        const auto keep = std::min<std::size_t>(4U, ranked.size());
        std::partial_sort(ranked.begin(), ranked.begin() + static_cast<std::ptrdiff_t>(keep), ranked.end(),
            [](const auto & left, const auto & right) {
                if (left.learned_weight != right.learned_weight)
                    return left.learned_weight > right.learned_weight;
                return left.joint < right.joint;
            });
        float sum = 0.0F;
        for (std::size_t slot = 0; slot < 4U; ++slot) {
            const auto value = slot < keep ? ranked[slot] : binding_candidate{0.0F, 0U};
            output.weights[vertex][slot] = value.learned_weight;
            output.joints[vertex][slot] = value.joint;
            sum += value.learned_weight;
        }
        if (sum <= 1e-12F) {
            output.weights[vertex][0] = 1.0F;
            sum = 1.0F;
        }
        for (auto & value : output.weights[vertex]) value /= sum;
    }
    return output;
}

} // namespace

skin integrate_learned_binding(
    const skeleton & target,
    nonstd::span<const vec3> normalized_vertices,
    nonstd::span<const vec3> sampled_points,
    nonstd::span<const std::vector<float>> dense_joint_weights,
    binding_trace * trace) {
    return integrate(target, normalized_vertices, sampled_points, dense_joint_weights, {}, trace);
}

skin integrate_postprocessed_binding(
    const skeleton & target,
    nonstd::span<const vec3> normalized_vertices,
    nonstd::span<const triangle> faces,
    nonstd::span<const vec3> normalized_joints,
    nonstd::span<const vec3> sampled_points,
    nonstd::span<const std::vector<float>> dense_joint_weights,
    binding_trace * trace) {
    const auto locality = surface_weights(normalized_vertices, faces, normalized_joints);
    auto output=integrate(target, normalized_vertices, sampled_points, dense_joint_weights, locality.weights, trace);
    if (trace != nullptr) {
        trace->voxel_coordinates=locality.voxel_coordinates;
        trace->surface_seeds=locality.seeds;
        trace->surface_distances=locality.distances;
        trace->surface_graph_edges=locality.graph_edges;
        trace->maximum_surface_distance=locality.maximum_distance;
        trace->surface_graph_sources=locality.graph_sources;
        trace->surface_graph_targets=locality.graph_targets;
        trace->surface_graph_weights=locality.graph_weights;
    }
    return output;
}

skin integrate_postprocessed_binding_precise(
    const skeleton & target,
    nonstd::span<const vec3> normalized_vertices,
    nonstd::span<const precise_vec3> precise_vertices,
    nonstd::span<const triangle> faces,
    nonstd::span<const vec3> normalized_joints,
    nonstd::span<const precise_vec3> precise_joints,
    nonstd::span<const vec3> sampled_points,
    nonstd::span<const std::vector<float>> dense_joint_weights,
    binding_trace * trace) {
    const auto locality = surface_weights(
        normalized_vertices, faces, normalized_joints, precise_vertices, precise_joints);
    auto output=integrate(target, normalized_vertices, sampled_points, dense_joint_weights, locality.weights, trace);
    if (trace != nullptr) {
        trace->voxel_coordinates=locality.voxel_coordinates;
        trace->surface_seeds=locality.seeds;
        trace->surface_distances=locality.distances;
        trace->surface_graph_edges=locality.graph_edges;
        trace->maximum_surface_distance=locality.maximum_distance;
        trace->surface_graph_sources=locality.graph_sources;
        trace->surface_graph_targets=locality.graph_targets;
        trace->surface_graph_weights=locality.graph_weights;
    }
    return output;
}

} // namespace skintokens::detail
