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
 *      https://www.gianmarcocherchi.com                                                 *
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

#ifndef TRIANGULATION_H
#define TRIANGULATION_H

#include "aux_structure.h"
#include "triangle_soup.h"
#include "fast_trimesh.h"
#include "tree.h"
#include "custom_stack.h"

#pragma GCC diagnostic ignored "-Wfloat-equal"

typedef unsigned int uint;

// lessThan(a,b) =
// -1 - if a < b
// 0  - if a == b
// 1  - if a > b
inline bool lessThanP(std::pair<const genericPoint*, uint> &a, std::pair<const genericPoint*, uint> &b)
{
    return (genericPoint::lessThan(*a.first, *b.first) < 0);
}


inline void triangulation(TriangleSoup &ts, point_arena& arena, AuxiliaryStructure &g, std::vector<uint> &new_tris, std::vector<std::bitset<NBIT> > &new_labels, bool parallel);

inline void triangulateSingleTriangle(TriangleSoup &ts, FastTrimesh &subm, uint t_id, AuxiliaryStructure &g, std::vector<uint> &new_tris, std::vector<std::bitset<NBIT> > &new_labels);

inline void splitSingleTriangle(const TriangleSoup &ts, FastTrimesh &subm, const std::vector<uint> &points);
inline void splitSingleTriangle(const TriangleSoup &ts, FastTrimesh &subm, const auxvector<uint> &points);

inline void splitSingleTriangleWithTree(const TriangleSoup &ts, FastTrimesh &subm, const std::vector<uint> &points);
inline void splitSingleTriangleWithTree(const TriangleSoup &ts, FastTrimesh &subm, const auxvector<uint> &points);

inline void splitSingleTriangleWithStack(const TriangleSoup &ts, FastTrimesh &subm, const auxvector<uint> &points,  const auxvector<uint> &e0_points, const auxvector<uint> &e1_points, const auxvector<uint> &e2_points);
inline void repositionPointsInStack(FastTrimesh &subm, CustomStack &stack_sub_tri, std::vector<auxvector<uint>> &curr_subdv, auxvector<uint> &curr_tri);

inline int findContainingTriangle(const FastTrimesh &subm, uint p_id);

inline const Node &findContainingTriangleInTree(const FastTrimesh &subm, uint p_id, const Tree &tree);
inline const Node &innerFindContainingTriangleInTree(const Tree &tree, uint root_id, const FastTrimesh &subm, const genericPoint *p);

inline void splitSingleEdge(const TriangleSoup &ts, FastTrimesh &subm, uint v0_id, uint v1_id, auxvector<uint> &points);

inline void addConstraintSegmentsInSingleTriangle(TriangleSoup &ts, point_arena& arena, FastTrimesh &subm, AuxiliaryStructure &g, auxvector<UIPair> &segment_list, std::mutex& mutex);

inline void addConstraintSegment(TriangleSoup &ts, point_arena& arena, FastTrimesh &subm, uint v0_id, uint v1_id, const int orientation,
                                 AuxiliaryStructure &g, auxvector<UIPair> &segment_list, phmap::flat_hash_map< UIPair, UIPair > &sub_segs_map, std::mutex& mutex);

inline void findIntersectingElements(TriangleSoup &ts, point_arena& arena, FastTrimesh &subm, uint v_start, uint v_stop, auxvector<uint> &intersected_edges, auxvector<uint> &intersected_tris,
                                     AuxiliaryStructure &g, auxvector<UIPair> &segment_list, phmap::flat_hash_map< UIPair, UIPair > &sub_seg_map, std::mutex& mutex);

template<typename iterator>
inline void boundaryWalker(const FastTrimesh &subm, uint v_start, uint v_stop, iterator curr_p, iterator curr_e, std::vector<uint> &h);

inline void earcut(const FastTrimesh &subm, std::vector<uint> &poly, std::vector<uint> &tris, const Plane &ref_p, const int &orientation);

inline void earcutLinear(const FastTrimesh &subm, const std::vector<uint> &poly, std::vector<uint> &tris, const int &orientation);

inline uint createTPI(TriangleSoup &ts, point_arena& arena, FastTrimesh &subm, const UIPair &e0, const UIPair &e1, AuxiliaryStructure &g, const phmap::flat_hash_map< UIPair, UIPair > &sub_segs_map);

inline std::vector<const genericPoint *> computeTriangleOfSegment(const TriangleSoup &ts, const UIPair &seg, std::vector<uint> &ref_t,
                                                                  const AuxiliaryStructure &g, const phmap::flat_hash_map< UIPair, UIPair > &sub_segs_map);

inline std::vector<const genericPoint *> computeTriangleOfSegmentInCoplanarCase(const TriangleSoup &ts, const UIPair &seg, const auxvector<uint> &tris, const std::vector<uint> &ref_t);

inline bool vectorsAreEqual(std::vector<uint> &v0, std::vector<uint> &v1);

inline bool fastPointOnLine(const FastTrimesh &subm, uint e_id, uint p_id);

inline bool segmentsIntersectInside(const FastTrimesh &subm, uint e00_id, uint e01_id, uint e10_id, uint e11_id);

inline bool pointInsideSegment(const FastTrimesh &subm, uint ev0_id, uint ev1_id, uint p_id);

inline void splitSegmentInSubSegments(uint v_start, uint v_stop, uint mid_point, phmap::flat_hash_map< UIPair, UIPair > &sub_segments_map);

inline const auxvector<uint> &segmentTrianglesList(const UIPair &seg, const phmap::flat_hash_map< UIPair, UIPair > &sub_segments_map, const AuxiliaryStructure &g);

inline void solvePocketsInCoplanarTriangle(const FastTrimesh &subm, AuxiliaryStructure &g, std::vector<uint> &new_tris,
                                           std::vector<std::bitset<NBIT> > &new_labels, const std::bitset<NBIT> &label);

inline void findPocketsInTriangle(const FastTrimesh &subm, std::vector<std::vector<uint> > &tri_pockets, std::vector<std::set<uint> > &polygons);

inline int customOrient2D(const genericPoint *p0, const genericPoint *p1, const genericPoint *p2, const Plane &ref_p);

inline void sortedVertexListAlongSegment(const TriangleSoup &ts, const std::vector<uint> &point_list, uint v0_id, uint v1_id, std::vector<uint> &res);
inline void sortedVertexListAlongSegment(const TriangleSoup &ts, const auxvector<uint> &point_list, uint v0_id, uint v1_id, auxvector<uint> &res);


//#include "triangulation.cpp"

#endif // TRIANGULATION_H
