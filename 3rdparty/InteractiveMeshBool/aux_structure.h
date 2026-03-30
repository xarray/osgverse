/*****************************************************************************************
 *              MIT License                                                              *
 *                                                                                       *
 * Copyright (c) 2022 G. Cherchi, M. Livesu, R. Scateni, M. Attene and F. Pellacini      *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION     *
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE        *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                *
 *                                                                                       *
 * Authors:                                                                              *
 *      Gianmarco Cherchi (g.cherchi@unica.it)                                           *
 *      https://people.unica.it/gianmarcocherchi/                                        *
 *                                                                                       *
 *      Marco Livesu (marco.livesu@ge.imati.cnr.it)                                      *
 *      http://pers.ge.imati.cnr.it/livesu/                                              *
 *                                                                                       *
 *      Riccardo Scateni (riccardo@unica.it)                                             *
 *      https://people.unica.it/riccardoscateni/                                         *
 *                                                                                       *
 *      Marco Attene (marco.attene@ge.imati.cnr.it)                                      *
 *      https://www.cnr.it/en/people/marco.attene/                                       *
 *                                                                                       *
 *      Fabio Pellacini (fabio.pellacini@uniroma1.it)                                    *
 *      https://pellacini.di.uniroma1.it                                                 *
 *                                                                                       *
 * ***************************************************************************************/

#ifndef INTERSECTIONS_GRAPH_H
#define INTERSECTIONS_GRAPH_H

#include "triangle_soup.h"

#include <set>

#include <mutex>

#include "utils.h"

typedef std::pair<uint, uint> UIPair;

//#include <absl/container/inlined_vector.h>
template<typename T>
//using auxvector = absl::InlinedVector<T, 16>;
using auxvector = std::vector<T>;

#include "../parallel_hashmap/btree.h"

#define PREDICATES_NO 0

#if PREDICATES_NO
constexpr double aux_point_epsilon = 0.00001;
struct aux_point {
    const genericPoint* pt;
    std::array<double, 3> v;

    aux_point(const genericPoint* pt_) : pt{pt_} {
    	pt->getApproxXYZCoordinates(v[0], v[1], v[2], false);
    }
};

inline bool operator<(const aux_point& a, const aux_point& b) {
    if (a.pt->isExplicit3D() && b.pt->isExplicit3D()) return (genericPoint::lessThan(*a.pt, *b.pt) < 0);
    if (fabs(a.v[0] - b.v[0]) > aux_point_epsilon &&
        fabs(a.v[1] - b.v[1]) > aux_point_epsilon &&
        fabs(a.v[2] - b.v[2]) > aux_point_epsilon) {
        return a.v < b.v;
    } else {
        return (genericPoint::lessThan(*a.pt, *b.pt) < 0);
    }
}
#else
struct aux_point {
    const genericPoint* pt;

    aux_point(const genericPoint* pt_) : pt{pt_} { }
};

inline bool operator<(const aux_point& a, const aux_point& b) {
    return (genericPoint::lessThan(*a.pt, *b.pt) < 0);
}
#endif

#define PREDICATES_MAP 0

template<typename T>
struct aux_point_map {
    phmap::btree_map<aux_point, T> map;
    phmap::node_hash_map<std::array<double, 3>, phmap::btree_map<aux_point, T>> grid;
    phmap::parallel_node_hash_map<std::array<double, 3>, phmap::btree_map<aux_point, T>, phmap::priv::hash_default_hash<std::array<double, 3>>, phmap::priv::hash_default_eq<std::array<double, 3>>, std::allocator<std::pair<const std::array<double, 3>, phmap::btree_map<aux_point, T>>>, 4, std::mutex> pgrid;
    phmap::flat_hash_map<std::array<double, 3>, T> map_approx;
    size_t start_size = 0;
    size_t insert_tries = 0;

    auto insert(const std::pair<aux_point, T>& item) {
        insert_tries += 1;
#if   PREDICATES_MAP == 0
        return map.insert(item);
#elif PREDICATES_MAP == 1
        std::array<double, 3> pt;
        item.first.pt->getApproxXYZCoordinates(pt[0], pt[1], pt[2], true);
        return grid[pt].insert(item);
#elif PREDICATES_MAP == 2
        std::array<double, 3> pt;
        item.first.pt->getApproxXYZCoordinates(pt[0], pt[1], pt[2], true);
        return pgrid[pt].insert(item);
#elif PREDICATES_MAP == 3
        std::array<double, 3> pt;
        item.first.pt->getApproxXYZCoordinates(pt[0], pt[1], pt[2], true);
        return map_approx.insert({pt, item.second});
#elif PREDICATES_MAP == 4
        std::array<double, 3> pt;
        item.first.pt->getApproxXYZCoordinates(pt[0], pt[1], pt[2], true);
        auto rret = grid[pt].insert(item); 
        auto ret = map.insert(item);
        if(ret.second != ret.second) throw std::runtime_error{"shit"};
        return ret;
#else
#endif
    }
};

class AuxiliaryStructure
{
    public:

        inline AuxiliaryStructure() {}

        inline void initFromTriangleSoup(TriangleSoup &ts);

        inline std::vector< std::pair<uint, uint> > &intersectionList();

        inline const std::vector<std::pair<uint, uint> > &intersectionList() const;

        inline bool addVertexInTriangle(uint t_id, uint v_id);

        inline bool addVertexInEdge(uint e_id, uint v_id);

        inline bool addSegmentInTriangle(uint t_id, const UIPair &seg);

        inline void addTrianglesInSegment(const UIPair &seg, uint tA_id, uint tB_id);

        inline void splitSegmentInSubSegments(uint orig_v0, uint orig_v1, uint midpoint);

        inline void addCoplanarTriangles(uint ta, uint tb);

        inline const auxvector<uint> &coplanarTriangles(uint t_id) const;

        inline bool triangleHasCoplanars(uint t_id) const;

        inline void setTriangleHasIntersections(uint t_id);

        inline bool triangleHasIntersections(uint t_id) const;

        inline const auxvector<uint> &trianglePointsList(uint t_id) const;

        inline const auxvector<uint> &edgePointsList(uint e_id) const;

        inline const auxvector<UIPair> &triangleSegmentsList(uint t_id) const;

        inline const auxvector<uint> &segmentTrianglesList(const UIPair &seg) const;

        inline std::pair<uint, bool> addVertexInSortedList(const genericPoint *v, uint pos);

        inline int addVisitedPolygonPocket(const std::vector<uint> &polygon, uint pos);

        inline const auto& get_vmap() const { return v_map; }
        inline auto& get_vmap() { return v_map; }

    private:
        uint    num_original_vtx;
        uint    num_original_tris;
        int     num_intersections;
        uint    num_tpi;

        std::vector< std::pair<uint, uint> > intersection_list;
        std::vector< auxvector<uint> > coplanar_tris;
        std::vector< auxvector<uint> > tri2pts;
        std::vector< auxvector<uint> > edge2pts;
        std::vector< auxvector<UIPair> > tri2segs;
        phmap::flat_hash_map< UIPair, auxvector<uint>  > seg2tris;
        std::vector<bool> tri_has_intersections;
        aux_point_map<uint> v_map;
        phmap::flat_hash_set< std::vector<uint> > visited_pockets;
        phmap::flat_hash_map< std::vector<uint>, uint> pockets_map;

        inline UIPair uniquePair(const UIPair &uip) const;
};


//#include "aux_structure.cpp"

#endif // INTERSECTIONS_GRAPH_H




