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

#ifndef INTERSECTION_CLASSIFICATION_H
#define INTERSECTION_CLASSIFICATION_H

#include "triangle_soup.h"
#include "aux_structure.h"
#include <cinolib/ipair.h>
#include <cinolib/predicates.h>
#include <vector>

#pragma GCC diagnostic ignored "-Wfloat-equal"

inline void find_intersections(const std::vector<cinolib::vec3d> & verts, const std::vector<uint>  & tris,
                               std::vector<cinolib::ipair> & intersections);

inline void detectIntersections(const TriangleSoup &ts, phmap::flat_hash_set<std::pair<uint, uint> > &intersection_list);

inline void classifyIntersections(TriangleSoup &ts, point_arena& arena, AuxiliaryStructure &g);

inline void checkTriangleTriangleIntersections(TriangleSoup &ts, point_arena& arena, AuxiliaryStructure &g, uint tA_id, uint tB_id);

inline uint addEdgeCrossEdgeInters(TriangleSoup &ts, point_arena& arena, uint e0_id, uint e1_id, AuxiliaryStructure &g);

inline uint addEdgeCrossEdgeInters(TriangleSoup &ts, point_arena& arena, uint e0_id, uint e1_id, uint t_id, AuxiliaryStructure &g);

inline uint addEdgeCrossTriInters(TriangleSoup &ts, point_arena& arena, uint e_id, uint t_id, AuxiliaryStructure &g);

inline void addSymbolicSegment(const TriangleSoup &ts, uint v0_id, uint v1_id, uint tA_id, uint tB_id, AuxiliaryStructure &g);

inline uint noCoplanarJollyPointID(const TriangleSoup &ts, const double *v0, const double *v1, const double *v2);

inline void checkSingleCoplanarEdgeIntersections(TriangleSoup &ts, point_arena& arena, uint e_v0, uint e_v1,
                                                 uint e_t_id, uint o_t_id, AuxiliaryStructure &g, phmap::flat_hash_set<uint> &il);

inline void checkSingleNoCoplanarEdgeIntersection(TriangleSoup &ts, point_arena& arena, uint e_id, uint t_id,
                                                  phmap::flat_hash_set<uint> &v_tmp, AuxiliaryStructure &g, phmap::flat_hash_set<uint> &li);

inline void checkVtxInTriangleIntersection(TriangleSoup &ts, uint v_id, uint t_id, phmap::flat_hash_set<uint> &v_tmp, AuxiliaryStructure &g, phmap::flat_hash_set<uint> &li);

inline void propagateCoplanarTrianglesIntersections(TriangleSoup &ts, AuxiliaryStructure &g);

inline void normalizeOrientations(double o[]);

inline bool sameOrientation(const double &o1, const double &o2);

// 1 if all edges are coplanar to the triangle, -1 otherwise
inline bool allCoplanarEdges(const double o[]);

// if there is a coplanar edge return its id, -1 otherwise
inline int singleCoplanarEdge(const double o[]);

// if there is a vertex in the plane and the opposite edge doesn't intersect the plane return the vtx id, -1 otherwise
inline int vtxInPlaneAndOppositeEdgeOnSameSide(const double o[]);

// if there is a vertex in the plane and the opposite edge intersect the plane return the vtx id, -1 otherwise
inline int vtxInPlaneAndOppositeEdgeCrossPlane(const double o[]);

// if there is a vertex on one side and the opposite edge on the other return the relative informations, -1 otherwise
inline int vtxOnASideAndOppositeEdgeOnTheOther(const double o[], uint &opp_v0, uint &opp_v1);

inline bool genericPointInsideTriangle(const TriangleSoup &ts, uint p_id, uint t_id, const bool &strict);

//#include "intersection_classification.cpp"

#endif // INTERSECTION_CLASSIFICATION_H
