#ifndef MANA_MODELING_OCTREE_HPP
#define MANA_MODELING_OCTREE_HPP

#include <osg/io_utils>
#include <osg/Math>
#include <osg/Vec2>
#include <osg/Vec3>
#include <osg/Vec4>
#include <cmath>
#include <cstring>
#include <limits>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>

namespace osgVerse
{
    namespace octree
    {
        template <typename T> struct PointComparer
        {
            T point; float distance;
            PointComparer() { distance = FLT_MAX; }
            PointComparer(T p, float d) { point = p; distance = d; }
            bool operator<(const PointComparer& r) const { return distance < r.distance; }
        };

        template <typename PT> class CustomHeap
        {
        public:
            CustomHeap(int max_capacity = 100)
            {
                heap_size = 0; cap = max_capacity;
                heap = new PointComparer<PT>[max_capacity];
            }
            ~CustomHeap() { delete[] heap; }

            void pop()
            {
                if (heap_size == 0) return;
                heap[0] = heap[heap_size - 1];
                heap_size--; moveDown(0); return;
            }

            void push(PointComparer<PT> point)
            {
                if (heap_size >= cap) { if (point < heap[0]) pop(); else return; }
                heap[heap_size] = point; floatUp(heap_size);
                heap_size++; return;
            }

            PointComparer<PT> top() { return heap[0]; }
            float top_value() { return heap[0].distance; }
            bool full() { return heap_size >= cap; }
            int size() { return heap_size; }
            void clear() { heap_size = 0; }

            std::vector<PointComparer<PT>> get_data()
            {
                std::vector<PointComparer<PT>> datas;
                for (int i = 0; i < heap_size; i++) { datas.push_back(heap[i]); }
                return datas;
            }

        private:
            void moveDown(int heap_index)
            {
                PointComparer<PT> tmp = heap[heap_index];
                int l = heap_index * 2 + 1;
                while (l < heap_size)
                {
                    if (l + 1 < heap_size && heap[l] < heap[l + 1]) l++;
                    if (tmp < heap[l])
                    {
                        heap[heap_index] = heap[l]; heap_index = l;
                        l = heap_index * 2 + 1;
                    }
                    else break;
                }
                heap[heap_index] = tmp; return;
            }

            void floatUp(int heap_index)
            {
                PointComparer<PT> tmp = heap[heap_index];
                int ancestor = (heap_index - 1) / 2;
                while (heap_index > 0)
                {
                    if (heap[ancestor] < tmp)
                    {
                        heap[heap_index] = heap[ancestor];
                        heap_index = ancestor; ancestor = (heap_index - 1) / 2;
                    }
                    else break;
                }
                heap[heap_index] = tmp; return;
            }

            PointComparer<PT>* heap;
            int heap_size, cap;
        };

        struct DistanceIndex
        {
            float distance; float* index;
            DistanceIndex() { distance = FLT_MAX; index = NULL; }
            DistanceIndex(float dist, float* idx) : distance(dist), index(idx) {}
            bool operator<(const DistanceIndex& r) const { return distance < r.distance; }
        };

        struct BoundingBoxType
        {
            float min[3];
            float max[3];
        };

        class KNNSimpleResultSet
        {
        public:
            KNNSimpleResultSet(size_t cap) : capacity(cap)
            { dist_index.resize(capacity, DistanceIndex()); clear(); }
            ~KNNSimpleResultSet() {}

            const std::vector<DistanceIndex>& get_data() { return dist_index; }
            float worstDistance() const { return worst_distance; }
            bool full() const { return count == capacity; }
            size_t size() const { return count; }

            void clear()
            {
                worst_distance = FLT_MAX; count = 0;
                dist_index[capacity - 1].distance = worst_distance;
            }

            void addPoint(float dist, float* index)
            {
                if (dist >= worst_distance) return;
                if (count < capacity) ++count; size_t i = 0;
                for (i = count - 1; i > 0; --i)
                {
                    if (dist_index[i - 1].distance <= dist) break;
                    else dist_index[i] = dist_index[i - 1];
                }
                dist_index[i].distance = dist;
                dist_index[i].index = index;
                worst_distance = dist_index[capacity - 1].distance;
            }

        private:
            size_t capacity, count;
            float worst_distance;
            std::vector<DistanceIndex> dist_index;
        };

        class Octant
        {
        public:
            std::vector<float*> points;
            float x, y, z, extent;
            bool isActive;
            Octant** child;

            Octant() : x(0.0f), y(0.0f), z(0.0f), extent(0.0f)
            { child = NULL; isActive = true; }

            ~Octant()
            {
                if (child != NULL)
                {
                    for (size_t i = 0; i < 8; ++i) { if (child[i] != 0) delete child[i]; }
                    delete[] child; child = NULL;
                }
                else
                {
                    delete[] points[0];
                    std::vector<float*>().swap(points);
                }
            }

            size_t size()
            {
                size_t pts_num = 0;
                get_octant_size(this, pts_num);
                return pts_num;
            }

            void get_octant_size(Octant* octant, size_t& s)
            {
                if (octant->child == NULL) { s += octant->points.size(); return; }
                for (size_t c = 0; c < 8; ++c)
                {
                    if (octant->child[c] == 0) continue;
                    get_octant_size(octant->child[c], s);
                }
            }

            void init_child()
            {
                child = new Octant * [8];
                memset(child, 0, 8 * sizeof(Octant*));
            }
        };
    }

    class Octree
    {
    public:
        size_t bucketSize;
        float minExtent; int dim;
        bool downSize, ordered;

        size_t ordered_indies[8][7] = {
            {1, 2, 4, 3, 5, 6, 7},
            {0, 3, 5, 2, 4, 7, 6},
            {0, 3, 6, 1, 4, 7, 5},
            {1, 2, 7, 0, 5, 6, 4},
            {0, 5, 6, 1, 2, 7, 3},
            {1, 4, 7, 0, 3, 6, 2},
            {2, 4, 7, 0, 3, 5, 1},
            {3, 5, 6, 1, 2, 4, 0}
        };

        Octree() : bucketSize(32), minExtent(0.01f), octRoot(0), downSize(false)
        {
            ordered = true; dim = 4;
            pts_num_deleted = last_pts_num = 0;
        }

        Octree(size_t s, float minE) : bucketSize(s), minExtent(minE), octRoot(0), downSize(false)
        {
            ordered = true; dim = 4;
            pts_num_deleted = last_pts_num = 0;
        }

        Octree(size_t s, float minE, int d) : bucketSize(s), minExtent(minE), octRoot(0), downSize(false)
        {
            ordered = true; dim = 4; if (d > 4) dim = d;
            pts_num_deleted = last_pts_num = 0;
        }

        ~Octree() { clear(); }
        void set_order(bool o = false) { ordered = o; }
        void set_min_extent(float e) { minExtent = e; }
        void set_bucket_size(size_t s) { bucketSize = s; }
        void set_down_size(bool s) { downSize = s; }

        template <typename CT>
        void initialize(CT& pts)
        {
            const size_t pts_num = pts.size();
            std::vector<float*> points; clear();
            points.resize(pts_num, 0);
            size_t cloud_index = 0;
            float min[3], max[3];
            for (size_t i = 0; i < pts_num; ++i)
            {
                const float& x = pts[i].x();
                const float& y = pts[i].y();
                const float& z = pts[i].z();
                if (std::isnan(x) || std::isnan(y) || std::isnan(z)) continue;

                float* cloud_ptr = new float[dim];
                cloud_ptr[0] = x; cloud_ptr[1] = y; cloud_ptr[2] = z;
                cloud_ptr[3] = float(cloud_index);
                points[cloud_index] = cloud_ptr;
                if (cloud_index == 0)
                    { min[0] = max[0] = x; min[1] = max[1] = y; min[2] = max[2] = z; }
                else
                {
                    min[0] = x < min[0] ? x : min[0]; max[0] = x > max[0] ? x : max[0];
                    min[1] = y < min[1] ? y : min[1]; max[1] = y > max[1] ? y : max[1];
                    min[2] = z < min[2] ? z : min[2]; max[2] = z > max[2] ? z : max[2];
                }
                cloud_index++;
            }

            last_pts_num = cloud_index; points.resize(cloud_index);
            float ctr[3] = { min[0], min[1], min[2] };
            float maxextent = 0.5f * (max[0] - min[0]);
            maxextent = std::max(maxextent, 0.01f); ctr[0] += maxextent;
            for (size_t i = 1; i < 3; ++i)
            {
                float extent = 0.5f * (max[i] - min[i]); ctr[i] += extent;
                if (extent > maxextent) maxextent = extent;
            }

            octRoot = createOctant(ctr[0], ctr[1], ctr[2], maxextent, points);
            for (size_t i = 0; i < points.size(); ++i) delete[] points[i];
        }

        template <typename CT>
        void update(CT& pts, bool down_size = false)
        {
            if (octRoot == 0) { initialize(pts); return; }
            size_t pts_num = pts.size(); downSize = down_size;
            std::vector<float*> points_tmp; points_tmp.resize(pts_num, 0);
            size_t cloud_index = 0; float min[3], max[3];
            const size_t N_old = last_pts_num;

            for (size_t i = 0; i < pts_num; ++i)
            {
                const float& x = pts_[i].x();
                const float& y = pts_[i].y();
                const float& z = pts_[i].z();
                if (std::isnan(x) || std::isnan(y) || std::isnan(z)) continue;

                float* cloud_ptr = new float[dim];
                cloud_ptr[0] = x; cloud_ptr[1] = y;
                cloud_ptr[2] = z; cloud_ptr[3] = N_old + cloud_index;
                points_tmp[cloud_index] = cloud_ptr;
                if (cloud_index == 0)
                    { min[0] = max[0] = x; min[1] = max[1] = y; min[2] = max[2] = z; }
                else
                {
                    min[0] = x < min[0] ? x : min[0]; max[0] = x > max[0] ? x : max[0];
                    min[1] = y < min[1] ? y : min[1]; max[1] = y > max[1] ? y : max[1];
                    min[2] = z < min[2] ? z : min[2]; max[2] = z > max[2] ? z : max[2];
                }
                cloud_index++;
            }

            if (cloud_index == 0) return;
            static const float factor[] = { -0.5f, 0.5f };
            points_tmp.resize(cloud_index);
            while (std::abs(max[0] - octRoot->x) > octRoot->extent ||
                   std::abs(max[1] - octRoot->y) > octRoot->extent ||
                   std::abs(max[2] - octRoot->z) > octRoot->extent)
            {
                float parentExtent = 2 * octRoot->extent;
                float parentX = octRoot->x + factor[max[0] > octRoot->x] * parentExtent;
                float parentY = octRoot->y + factor[max[1] > octRoot->y] * parentExtent;
                float parentZ = octRoot->z + factor[max[2] > octRoot->z] * parentExtent;

                Octant* octant = new Octant;
                octant->x = parentX; octant->y = parentY; octant->z = parentZ;
                octant->extent = parentExtent; octant->init_child();

                size_t mortonCode = 0;
                if (octRoot->x > parentX) mortonCode |= 1;
                if (octRoot->y > parentY) mortonCode |= 2;
                if (octRoot->z > parentZ) mortonCode |= 4;
                octant->child[mortonCode] = octRoot;
                octRoot = octant;
            }

            while (std::abs(min[0] - octRoot->x) > octRoot->extent ||
                   std::abs(min[1] - octRoot->y) > octRoot->extent ||
                   std::abs(min[2] - octRoot->z) > octRoot->extent)
            {
                float parentExtent = 2 * octRoot->extent;
                float parentX = octRoot->x + factor[min[0] > octRoot->x] * parentExtent;
                float parentY = octRoot->y + factor[min[1] > octRoot->y] * parentExtent;
                float parentZ = octRoot->z + factor[min[2] > octRoot->z] * parentExtent;

                Octant* octant = new Octant;
                octant->x = parentX; octant->y = parentY; octant->z = parentZ;
                octant->extent = parentExtent; octant->init_child();

                size_t mortonCode = 0;
                if (octRoot->x > parentX) mortonCode |= 1;
                if (octRoot->y > parentY) mortonCode |= 2;
                if (octRoot->z > parentZ) mortonCode |= 4;
                octant->child[mortonCode] = octRoot;
                octRoot = octant;
            }

            if (points_tmp.size() == 0) return;
            last_pts_num += points_tmp.size();
            updateOctant(octRoot, points_tmp);
            for (size_t i = 0; i < points_tmp.size(); ++i) delete[] points_tmp[i];
        }

        void clear()
        { delete octRoot; octRoot = 0; }

        template <typename PT>
        void radiusNeighbors(const PT& query, float radius, std::vector<size_t>& resultIndices)
        {
            resultIndices.clear(); if (octRoot == 0) return;
            float sqrRadius = radius * radius;
            float queryData[3] = { query.x(), query.y(), query.z() };

            std::vector<float*> points_ptr;
            radiusNeighbors(octRoot, queryData, radius, sqrRadius, points_ptr);
            resultIndices.resize(points_ptr.size());
            for (size_t i = 0; i < points_ptr.size(); i++)
                resultIndices[i] = size_t(points_ptr[i][3]);
        }

        template <typename PT>
        void radiusNeighbors(const PT& query, float radius, std::vector<size_t>& resultIndices,
                             std::vector<float>& distances)
        {
            resultIndices.clear(); distances.clear(); if (octRoot == 0) return;
            float sqrRadius = radius * radius;
            float queryData[3] = { query.x(), query.y(), query.z() };

            std::vector<float*> points_ptr;
            radiusNeighbors(octRoot, queryData, radius, sqrRadius, points_ptr, distances);
            resultIndices.resize(points_ptr.size());
            for (size_t i = 0; i < points_ptr.size(); i++)
                resultIndices[i] = size_t(points_ptr[i][3]);
        }

        template <typename PT>
        void radiusNeighbors(const PT& query, float radius, std::vector<PT>& resultIndices,
                             std::vector<float>& distances)
        {
            resultIndices.clear(); distances.clear(); if (octRoot == 0) return;
            float sqrRadius = radius * radius;
            float queryData[3] = { query.x(), query.y(), query.z()};

            std::vector<float*> points_ptr;
            radiusNeighbors(octRoot, queryData, radius, sqrRadius, points_ptr, distances);
            resultIndices.resize(points_ptr.size());
            for (size_t i = 0; i < resultIndices.size(); i++)
            {
                PT pt; pt.x() = points_ptr[i][0];
                pt.y() = points_ptr[i][1]; pt.z() = points_ptr[i][2];
                resultIndices[i] = pt;
            }
        }

        template <typename PT>
        int32_t knnNeighbors(const PT& query, int k, std::vector<PT>& resultIndices,
                             std::vector<float>& distances)
        {
            if (octRoot == 0) return 0;
            float queryData[3] = { query.x(), query.y(), query.z() };
            KNNSimpleResultSet heap(k);
            knnNeighbors(m_octRoot, queryData, heap);

            std::vector<DistanceIndex> data = heap.get_data();
            resultIndices.resize(heap.size());
            distances.resize(heap.size());
            for (size_t i = 0; i < heap.size(); i++)
            {
                PT pt; pt.x = data[i].index_[0];
                pt.y = data[i].index[1]; pt.z = data[i].index[2];
                resultIndices[i] = pt; distances[i] = data[i].distance;
            }
            return data.size();
        }

        template <typename PT>
        int32_t knnNeighbors(const PT& query, int k, std::vector<size_t>& resultIndices,
                             std::vector<float>& distances)
        {
            if (octRoot == 0) return 0;
            float queryData[3] = { query.x(), query.y(), query.z() };
            KNNSimpleResultSet heap(k);
            knnNeighbors(octRoot, queryData, heap);

            std::vector<DistanceIndex> data = heap.get_data();
            resultIndices.resize(heap.size());
            distances.resize(heap.size());
            for (int i = 0; i < heap.size(); i++)
            {
                resultIndices[i] = size_t(data[i].index[3]);
                distances[i] = data[i].distance;
            }
            return data.size();
        }

        template <typename PT>
        int32_t knnNeighbors(const PT& query, int k, std::vector<int>& resultIndices,
                             std::vector<float>& distances)
        {
            if (octRoot == 0) return 0;
            float queryData[3] = { query.x(), query.y(), query.z()};
            KNNSimpleResultSet heap(k);
            knnNeighbors(octRoot, queryData, heap);

            std::vector<DistanceIndex> data = heap.get_data();
            resultIndices.resize(heap.size());
            distances.resize(heap.size());
            for (int i = 0; i < heap.size(); i++)
            {
                resultIndices[i] = int(data[i].index[3]);
                distances[i] = data[i].distance;
            }
            return data.size();
        }

        void boxWiseDelete(const octree::BoundingBoxType& box_range, bool clear_data)
        {
            if (octRoot == 0) return; bool deleted = false;
            boxWiseDelete(octRoot, box_range, deleted, clear_data);
            if (deleted) octRoot = 0;
        }

        void boxWiseDelete(const float* min, const float* max, bool clear_data)
        {
            if (octRoot == 0) return; bool deleted = false;
            octree::BoundingBoxType box_range;
            box_range.min[0] = min[0]; box_range.max[0] = max[0];
            box_range.min[1] = min[1]; box_range.max[1] = max[1];
            box_range.min[2] = min[2]; box_range.max[2] = max[2];
            boxWiseDelete(octRoot, box_range, deleted, clear_data);
            if (deleted) octRoot = 0;
        }

        size_t size()
        { return last_pts_num - pts_num_deleted; }

        void get_nodes(octree::Octant* octant, std::vector<octree::Octant*>& nodes, float min_extent = 0)
        {
            if (octant == 0) return;
            if (min_extent > 0)
            {
                if (octant->extent <= min_extent)
                { nodes.push_back(octant); return; }
            }

            nodes.push_back(octant);
            if (octant->child == NULL)  return;
            for (int i = 0; i < 8; i++)
                get_nodes(octant->child[i], nodes, min_extent);
        }

        void get_leaf_nodes(const octree::Octant* octant, std::vector<const octree::Octant*>& nodes)
        {
            if (octant == 0) return;
            if (octant->child == NULL) { nodes.push_back(octant); return; }

            nodes.reserve(nodes.size() + 8);
            for (int i = 0; i < 8; i++) get_leaf_nodes(octant->child[i], nodes);
        }

        template <typename PT, typename CT>
        CT get_data()
        {
            std::vector<octree::Octant*> nodes; CT pts;
            get_nodes(octRoot, nodes);
            for (auto octant : nodes)
            {
                for (auto p : octant->points)
                {
                    PT pt; pt.x = p[0];
                    pt.y = p[1]; pt.z = p[2];
                    pts.push_back(pt);
                }
            }
            return pts;
        }

    protected:
        octree::Octant* octRoot;
        size_t last_pts_num, pts_num_deleted;

        octree::Octant* createOctant(float x, float y, float z, float extent, std::vector<float*>& points)
        {
            octree::Octant* octant = new octree::Octant;
            const size_t size = points.size();
            octant->x = x; octant->y = y; octant->z = z;
            octant->extent = extent;

            static const float factor[] = { -0.5f, 0.5f };
            if (size > bucketSize && extent > 2 * minExtent) // 32 0
            {
                std::vector<std::vector<float*>> child_points(8, std::vector<float*>());
                for (size_t i = 0; i < size; ++i)
                {
                    float* p = points[i]; size_t mortonCode = 0;
                    if (p[0] > x) mortonCode |= 1;
                    if (p[1] > y) mortonCode |= 2;
                    if (p[2] > z) mortonCode |= 4;
                    child_points[mortonCode].push_back(p);
                }

                float childExtent = 0.5f * extent; octant->init_child();
                for (size_t i = 0; i < 8; ++i)
                {
                    if (child_points[i].size() == 0) continue;
                    float childX = x + factor[(i & 1) > 0] * extent;
                    float childY = y + factor[(i & 2) > 0] * extent;
                    float childZ = z + factor[(i & 4) > 0] * extent;
                    octant->child[i] = createOctant(childX, childY, childZ, childExtent, child_points[i]);
                }
            }
            else
            {
                const size_t size = points.size(); octant->points.resize(size, 0);
                float* continue_points = new float[size * dim];
                for (size_t i = 0; i < size; ++i)
                {
                    std::copy(points[i], points[i] + dim, continue_points + dim * i);
                    octant->points[i] = continue_points + dim * i;
                }
            }
            return octant;
        }

        void updateOctant(octree::Octant* octant, const std::vector<float*>& points)
        {
            static const float factor[] = { -0.5f, 0.5f };
            const float x = octant->x, y = octant->y, z = octant->z, extent = octant->extent;
            octant->isActive = true;
            if (octant->child == NULL)
            {
                if (octant->points.size() + points.size() > bucketSize && extent > 2 * minExtent) // 32 0
                {
                    octant->points.insert(octant->points.end(), points.begin(), points.end());
                    const size_t size = octant->points.size();
                    std::vector<std::vector<float*>> child_points(8, std::vector<float*>());
                    for (size_t i = 0; i < size; ++i)
                    {
                        size_t mortonCode = 0;
                        if (octant->points[i][0] > x) mortonCode |= 1;
                        if (octant->points[i][1] > y) mortonCode |= 2;
                        if (octant->points[i][2] > z) mortonCode |= 4;
                        child_points[mortonCode].push_back(octant->points[i]);
                    }

                    float childExtent = 0.5f * extent; octant->init_child();
                    for (size_t i = 0; i < 8; ++i)
                    {
                        if (child_points[i].size() == 0) continue;
                        float childX = x + factor[(i & 1) > 0] * extent;
                        float childY = y + factor[(i & 2) > 0] * extent;
                        float childZ = z + factor[(i & 4) > 0] * extent;
                        octant->child[i] = createOctant(childX, childY, childZ, childExtent, child_points[i]);
                    }
                    delete[] octant->points[0];
                    std::vector<float*>().swap(octant->points);
                }
                else
                {
                    if (downSize && extent <= 2 * minExtent && octant->points.size() > bucketSize / 8) return;
                    octant->points.insert(octant->points.end(), points.begin(), points.end());
                    const size_t size = octant->points.size();
                    float* continue_points = new float[size * dim];
                    float* old_points = octant->points[0];
                    for (size_t i = 0; i < size; ++i)
                    {
                        std::copy(octant->points[i], octant->points[i] + dim, continue_points + dim * i);
                        octant->points[i] = continue_points + dim * i;
                    }
                    delete[] old_points;
                }
            }
            else
            {
                const size_t size = points.size();
                std::vector<std::vector<float*>> child_points(8, std::vector<float*>());
                for (size_t i = 0; i < size; ++i)
                {
                    size_t mortonCode = 0;
                    if (points[i][0] > x) mortonCode |= 1;
                    if (points[i][1] > y) mortonCode |= 2;
                    if (points[i][2] > z) mortonCode |= 4;
                    child_points[mortonCode].push_back(points[i]);
                }
                float childExtent = 0.5f * extent;
                for (size_t i = 0; i < 8; ++i)
                {
                    if (child_points[i].size() > 0)
                    {
                        if (octant->child[i] == 0)
                        {
                            float childX = x + factor[(i & 1) > 0] * extent;
                            float childY = y + factor[(i & 2) > 0] * extent;
                            float childZ = z + factor[(i & 4) > 0] * extent;
                            octant->child[i] = createOctant(childX, childY, childZ, childExtent, child_points[i]);
                        }
                        else
                            updateOctant(octant->child[i], child_points[i]);
                    }
                }
            }
        }

        void radiusNeighbors(const octree::Octant* octant, const float* query, float radius, float sqrRadius,
                             std::vector<float*>& resultIndices)
        {
            if (!octant->isActive) return;
            if (3 * octant->extent * octant->extent < sqrRadius && contains(query, sqrRadius, octant))
            {
                std::vector<const octree::Octant*> candidate_octants;
                candidate_octants.reserve(8);
                get_leaf_nodes(octant, candidate_octants);
                for (size_t k = 0; k < candidate_octants.size(); k++)
                {
                    const size_t size = candidate_octants[k]->points.size();
                    const size_t result_size = resultIndices.size();
                    resultIndices.resize(result_size + size);
                    for (size_t i = 0; i < size; ++i)
                        resultIndices[result_size + i] = candidate_octants[k]->points[i];
                }
                return;
            }

            if (octant->child == NULL)
            {
                const size_t size = octant->points.size();
                for (size_t i = 0; i < size; ++i)
                {
                    const float* p = octant->points[i];
                    float dist = 0, diff = 0;
                    for (size_t j = 0; j < 3; ++j) { diff = p[j] - query[j]; dist += diff * diff; }
                    if (dist < sqrRadius) resultIndices.push_back(octant->points[i]);
                }
                return;
            }

            for (size_t c = 0; c < 8; ++c)
            {
                if (octant->child[c] == 0) continue;
                if (!overlaps(query, sqrRadius, octant->child[c])) continue;
                radiusNeighbors(octant->child[c], query, radius, sqrRadius, resultIndices);
            }
        }

        void radiusNeighbors(const octree::Octant* octant, const float* query, float radius,
                             float sqrRadius, std::vector<float*>& resultIndices, std::vector<float>& distances)
        {
            if (!octant->isActive) return;
            if (3 * octant->extent * octant->extent < sqrRadius && contains(query, sqrRadius, octant))
            {
                std::vector<const octree::Octant*> candidate_octants;
                get_leaf_nodes(octant, candidate_octants);
                for (size_t k = 0; k < candidate_octants.size(); k++)
                {
                    const size_t size = candidate_octants[k]->points.size();
                    const size_t result_size = resultIndices.size();
                    resultIndices.resize(result_size + size);
                    for (size_t i = 0; i < size; ++i)
                    {
                        const float* p = candidate_octants[k]->points[i]; float dist = 0, diff = 0;
                        for (size_t j = 0; j < 3; ++j) { diff = p[j] - query[j]; dist += diff * diff; }
                        distances.push_back(dist);
                        resultIndices[result_size + i] = candidate_octants[k]->points[i];
                    }
                }
                return;
            }

            if (octant->child == NULL)
            {
                const size_t size = octant->points.size();
                for (size_t i = 0; i < size; ++i)
                {
                    const float* p = octant->points[i]; float dist = 0, diff = 0;
                    for (size_t j = 0; j < 3; ++j) { diff = p[j] - query[j]; dist += diff * diff; }
                    if (dist < sqrRadius)
                    {
                        resultIndices.push_back(octant->points[i]);
                        distances.push_back(dist);
                    }
                }
                return;
            }

            for (size_t c = 0; c < 8; ++c)
            {
                if (octant->child[c] == 0) continue;
                if (!overlaps(query, sqrRadius, octant->child[c])) continue;
                radiusNeighbors(octant->child[c], query, radius, sqrRadius, resultIndices, distances);
            }
        }

        bool radiusNeighbors2(const octree::Octant* octant, const float* query, float sqrRadius,
                              std::vector<size_t>& resultIndices, std::vector<float>& distances)
        {
            if (!octant->isActive) return false;
            if (octant->child == NULL)
            {
                const size_t size = octant->points.size();
                for (int i = 0; i < size; ++i)
                {
                    const float* p = octant->points[i]; float dist = 0, diff = 0;
                    for (int j = 0; j < 3; ++j) { diff = p[j] - query[j]; dist += diff * diff; }
                    if (dist < sqrRadius)
                    {
                        resultIndices.push_back(size_t(p[3]));
                        distances.push_back(dist);
                    }
                }
                return inside(query, sqrRadius, octant);
            }

            size_t mortonCode = 0;
            if (query[0] > octant->x) mortonCode |= 1;
            if (query[1] > octant->y) mortonCode |= 2;
            if (query[2] > octant->z) mortonCode |= 4;
            if (octant->child[mortonCode] != 0)
                if (radiusNeighbors2(octant->child[mortonCode], query, sqrRadius, resultIndices, distances))
                    return true;

            for (int i = 0; i < 7; ++i)
            {
                int c = ordered_indies[mortonCode][i];
                if (octant->child[c] == 0) continue;
                if (!overlaps(query, sqrRadius, octant->child[c])) continue;
                if (radiusNeighbors2(octant->child[c], query, sqrRadius, resultIndices, distances))
                    return true;
            }
            return inside(query, sqrRadius, octant);
        }

        bool knnNeighbors(const octree::Octant* octant, const float* query, octree::KNNSimpleResultSet& heap)
        {
            if (!octant->isActive) return false;
            if (octant->child == NULL)
            {
                const size_t size = octant->points.size();
                for (int i = 0; i < size; ++i)
                {
                    const float* p = octant->points[i]; float dist = 0, diff = 0;
                    for (int j = 0; j < 3; ++j) { diff = p[j] - query[j]; dist += diff * diff; }
                    if (dist < heap.worstDistance())
                        heap.addPoint(dist, octant->points[i]);
                }
                return heap.full() && inside(query, heap.worstDistance(), octant);
            }

            size_t mortonCode = 0;
            if (query[0] > octant->x) mortonCode |= 1;
            if (query[1] > octant->y) mortonCode |= 2;
            if (query[2] > octant->z) mortonCode |= 4;
            if (octant->child[mortonCode] != 0)
                if (knnNeighbors(octant->child[mortonCode], query, heap))
                    return true;

            for (int i = 0; i < 7; ++i)
            {
                int c = ordered_indies[mortonCode][i];
                if (octant->child[c] == 0) continue;
                if (heap.full() && !overlaps(query, heap.worstDistance(), octant->child[c])) continue;
                if (knnNeighbors(octant->child[c], query, heap)) return true;
            }
            return heap.full() && inside(query, heap.worstDistance(), octant);
        }

        bool knnNeighbors(const octree::Octant* octant, const float* query, octree::CustomHeap<size_t>& heap)
        {
            if (!octant->isActive) return false;
            if (octant->child == NULL)
            {
                const size_t size = octant->points.size();
                for (size_t i = 0; i < size; ++i)
                {
                    const float* p = octant->points[i]; float dist = 0, diff = 0;
                    for (size_t j = 0; j < 3; ++j) { diff = *p++ - *query++; dist += diff * diff; }
                    octree::PointComparer<size_t> pt(size_t(p[3]), dist); heap.push(pt);
                }
                return heap.full() && inside(query, heap.top_value(), octant);
            }

            size_t mortonCode = 0;
            if (query[0] > octant->x) mortonCode |= 1;
            if (query[1] > octant->y) mortonCode |= 2;
            if (query[2] > octant->z) mortonCode |= 4;
            if (octant->child != NULL && octant->child[mortonCode] != 0)
                if (knnNeighbors(octant->child[mortonCode], query, heap))
                    return true;

            for (size_t c = 0; c < 8 && octant->child != NULL; ++c)
            {
                if (c == mortonCode) continue;
                if (octant->child[c] == 0) continue;
                if (heap.full() && !overlaps(query, heap.top_value(), octant->child[c])) continue;
                if (knnNeighbors(octant->child[c], query, heap)) return true;
            }
            return heap.full() && inside(query, heap.top_value(), octant);
        }

        void boxWiseDelete(octree::Octant* octant, const octree::BoundingBoxType& box_range,
                           bool& deleted, bool clear_data)
        {
            float cur_min[3], cur_max[3];
            cur_min[0] = octant->x - octant->extent;
            cur_min[1] = octant->y - octant->extent;
            cur_min[2] = octant->z - octant->extent;
            cur_max[0] = octant->x + octant->extent;
            cur_max[1] = octant->y + octant->extent;
            cur_max[2] = octant->z + octant->extent;
            if (cur_min[0] > box_range.max[0] || box_range.min[0] > cur_max[0]) return;
            if (cur_min[1] > box_range.max[1] || box_range.min[1] > cur_max[1]) return;
            if (cur_min[2] > box_range.max[2] || box_range.min[2] > cur_max[2]) return;

            if (cur_min[0] >= box_range.min[0] && cur_min[1] >= box_range.min[1] && cur_min[2] >= box_range.min[2] &&
                cur_max[0] <= box_range.max[0] && cur_max[1] <= box_range.max[1] && cur_max[2] <= box_range.max[2])
            {
                if (!clear_data)
                    { octant->isActive = false; return; }
                else
                {
                    pts_num_deleted += octant->size();
                    delete octant; deleted = true; return;
                }
            }
            if (octant->child == NULL)
            {
                if (!clear_data)
                    { octant->isActive = false; return; }
                else
                {
                    const size_t size = octant->points.size();
                    std::vector<float*> remainder_points;
                    remainder_points.resize(size, 0);

                    size_t valid_num = 0;
                    for (int i = 0; i < size; ++i)
                    {
                        const float* p = octant->points[i];
                        if (p[0] > box_range.max[0] || box_range.min[0] > p[0])
                        {
                            remainder_points[valid_num] = octant->points[i];
                            valid_num++; continue;
                        }
                        if (p[1] > box_range.max[1] || box_range.min[1] > p[1])
                        {
                            remainder_points[valid_num] = octant->points[i];
                            valid_num++; continue;
                        }
                        if (p[2] > box_range.max[2] || box_range.min[2] > p[2])
                        {
                            remainder_points[valid_num] = octant->points[i];
                            valid_num++; continue;
                        }
                    }

                    pts_num_deleted += size - valid_num;
                    if (valid_num == 0) { delete octant; deleted = true; return; }
                    float* continue_points = new float[valid_num * dim];
                    float* old_points = octant->points[0];
                    for (size_t i = 0; i < valid_num; ++i)
                    {
                        std::copy(remainder_points[i], remainder_points[i] + dim, continue_points + dim * i);
                        octant->points[i] = continue_points + dim * i;
                    }
                    octant->points.resize(valid_num);
                    delete[] old_points; return;
                }
            }

            // check whether child nodes are in range.
            for (size_t c = 0; c < 8; ++c)
            {
                bool deleted1 = false; if (octant->child[c] == 0) continue;
                boxWiseDelete(octant->child[c], box_range, deleted1, clear_data);
                if (deleted1) octant->child[c] = 0;
            }

            int valid_child = 0;
            for (size_t c = 0; c < 8; ++c) { if (octant->child[c] == 0) continue; valid_child++; }
            if (valid_child == 0) { delete octant; deleted = true; return; }
        }

        bool overlaps(const float* query, float sqRadius, const octree::Octant* o)
        {
            float x = std::abs(query[0] - o->x) - o->extent;
            float y = std::abs(query[1] - o->y) - o->extent;
            float z = std::abs(query[2] - o->z) - o->extent;
            if ((x > 0 && x * x > sqRadius) || (y > 0 && y * y > sqRadius) ||
                (z > 0 && z * z > sqRadius)) return false;

            int32_t num_less_extent = (x < 0) + (y < 0) + (z < 0);
            if (num_less_extent > 1) return true;
            x = std::max(x, 0.0f); y = std::max(y, 0.0f); z = std::max(z, 0.0f);
            return (x * x + y * y + z * z < sqRadius);
        }

        float sqrDist_point2octant(const float* query, const octree::Octant* o)
        {
            float x = std::max(std::abs(query[0] - o->x) - o->extent, 0.0f);
            float y = std::max(std::abs(query[1] - o->y) - o->extent, 0.0f);
            float z = std::max(std::abs(query[2] - o->z) - o->extent, 0.0f);
            return (x * x + y * y + z * z);
        }

        bool contains(const float* query, float sqRadius, const octree::Octant* o)
        {
            float x = std::abs(query[0] - o->x) + o->extent;
            float y = std::abs(query[1] - o->y) + o->extent;
            float z = std::abs(query[2] - o->z) + o->extent;
            return (x * x + y * y + z * z < sqRadius);
        }

        bool inside(const float* query, float radius2, const octree::Octant* octant)
        {
            float x = octant->extent - std::abs(query[0] - octant->x);
            float y = octant->extent - std::abs(query[1] - octant->y);
            float z = octant->extent - std::abs(query[2] - octant->z);
            if (x < 0 || x * x < radius2) return false;
            if (y < 0 || y * y < radius2) return false;
            if (z < 0 || z * z < radius2) return false;
            return true;
        }
    };
}

#endif
