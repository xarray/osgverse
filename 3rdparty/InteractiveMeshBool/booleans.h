/*****************************************************************************************
 *              MIT License                                                              *
 *                                                                                       *
 * Copyright (c) 2022 G. Cherchi, F. Pellacini, M. Attene and M. Livesu                  *
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
 *      Fabio Pellacini (fabio.pellacini@uniroma1.it)                                    *
 *      https://pellacini.di.uniroma1.it                                                 *
 *                                                                                       *
 *      Marco Attene (marco.attene@ge.imati.cnr.it)                                      *
 *      https://www.cnr.it/en/people/marco.attene/                                       *
 *                                                                                       *
 *      Marco Livesu (marco.livesu@ge.imati.cnr.it)                                      *
 *      http://pers.ge.imati.cnr.it/livesu/                                              *
 *                                                                                       *
 * ***************************************************************************************/

#ifndef EXACT_BOOLEANS_BOOLEANS_H
#define EXACT_BOOLEANS_BOOLEANS_H

#include "processing.h"
#include "aux_structure.h"
#include "triangle_soup.h"
#include "intersection_classification.h"
#include "triangulation.h"
#include <cinolib/octree.h>
#include <bitset>

struct Labels
{
    std::vector< std::bitset<NBIT> > surface;
    std::vector< std::bitset<NBIT> > inside;
    uint num;
};

struct Ray
{
    explicitPoint3D v0;
    explicitPoint3D v1;
    char dir = 'X';
    int tv[3] = {-1, -1, -1};
};

struct DuplTriInfo
{
    uint t_id;
    uint l_id;
    bool w;
};

enum BoolOp {UNION, INTERSECTION, SUBTRACTION, XOR, NONE};

enum IntersInfo {DISCARD, NO_INT, INT_IN_V0, INT_IN_V1, INT_IN_V2, INT_IN_EDGE01, INT_IN_EDGE12, INT_IN_EDGE20, INT_IN_TRI};

struct less_than_GP_on_X // lessThan GenericPoint along X
{
    bool operator() (const std::pair<genericPoint*, uint> &p0, const std::pair<genericPoint*, uint> &p1) const
    {
        return (genericPoint::lessThanOnX(*p0.first, *p1.first) <= 0);
    }
};

struct less_than_GP_on_Y // lessThan GenericPoint along Y
{
    bool operator() (const std::pair<genericPoint*, uint> &p0, const std::pair<genericPoint*, uint> &p1) const
    {
        return (genericPoint::lessThanOnY(*p0.first, *p1.first) <= 0);
    }
};

struct less_than_GP_on_Z // lessThan GenericPoint along Z
{
    bool operator() (const std::pair<genericPoint*, uint> &p0, const std::pair<genericPoint*, uint> &p1) const
    {
        return (genericPoint::lessThanOnZ(*p0.first, *p1.first) <= 0);
    }
};

inline void customBooleanPipeline(std::vector<genericPoint*>& arr_verts, std::vector<uint>& arr_in_tris,
                                  std::vector<uint>& arr_out_tris, std::vector<std::bitset<NBIT>>& arr_in_labels,
                                  std::vector<DuplTriInfo>& dupl_triangles, Labels& labels,
                                  std::vector<phmap::flat_hash_set<uint>>& patches, cinolib::Octree& octree,
                                  const BoolOp &op, std::vector<double> &bool_coords, std::vector<uint> &bool_tris,
                                  std::vector< std::bitset<NBIT>> &bool_labels);

inline void booleanPipeline(const std::vector<double> &in_coords, const std::vector<uint> &in_tris,
                            const std::vector<uint> &in_labels, const BoolOp &op, std::vector<double> &bool_coords,
                            std::vector<uint> &bool_tris, std::vector< std::bitset<NBIT> > &bool_labels);


inline void customArrangementPipeline(const std::vector<double> &in_coords, const std::vector<uint> &in_tris, const std::vector<uint> &in_labels,
                                      std::vector<uint> &arr_in_tris, std::vector< std::bitset<NBIT>> &arr_in_labels,
                                      point_arena& arena, std::vector<genericPoint *> &vertices, std::vector<uint> &arr_out_tris, Labels &labels,
                                      cinolib::Octree &octree, std::vector<DuplTriInfo> &dupl_triangles, bool parallel);

inline void customRemoveDegenerateAndDuplicatedTriangles(const std::vector<genericPoint*> &verts, std::vector<uint> &tris,
                                                         std::vector< std::bitset<NBIT> > &labels, std::vector<DuplTriInfo> &dupl_triangles,
                                                         bool parallel);

inline void customDetectIntersections(const TriangleSoup &ts, std::vector<std::pair<uint, uint> > &intersection_list, cinolib::Octree &o);

inline void addDuplicateTrisInfoInStructures(const std::vector<DuplTriInfo> &dupl_tris, std::vector<uint> &in_tris,
                                             std::vector<std::bitset<NBIT>> &in_labels, cinolib::Octree &octree);

inline void computeAllPatches(FastTrimesh &tm, const Labels &labels, std::vector<phmap::flat_hash_set<uint>> &patches, bool parallel);

inline void computeSinglePatch(FastTrimesh &tm, uint seed_t, const Labels &labels, phmap::flat_hash_set<uint> &patch);
inline void computeSinglePatch(FastTrimesh &tm, uint seed_t, const Labels &labels, phmap::flat_hash_set<uint> &patch, const std::vector<std::array<uint, 3>>& adjT2E);

inline void findRayEndpoints(const FastTrimesh &tm, const phmap::flat_hash_set<uint> &patch, const cinolib::vec3d &max_coords, Ray &ray);

inline bool intersects_box(const cinolib::Octree& tree, const cinolib::AABB & b, phmap::flat_hash_set<uint> & ids);

inline void computeInsideOut(const FastTrimesh &tm, const std::vector<phmap::flat_hash_set<uint>> &patches, const cinolib::Octree &octree,
                             const std::vector<genericPoint *> &in_verts, const std::vector<uint> &in_tris,
                             const std::vector<std::bitset<NBIT>> &in_labels, const cinolib::vec3d &max_coords, Labels &labels);

inline void pruneIntersectionsAndSortAlongRay(const Ray &ray, const std::vector<genericPoint*> &in_verts,
                                              const std::vector<uint> &in_tris, const std::vector<std::bitset<NBIT>> &in_labels,
                                              const phmap::flat_hash_set<uint> &tmp_inters, const std::bitset<NBIT> &patch_surface_label,
                                              std::vector<uint> &inters_tris);

inline void analyzeSortedIntersections(const Ray &ray, const std::vector<genericPoint*> &in_verts, const std::vector<uint> &in_tris,
                                       const std::vector<std::bitset<NBIT>> &in_labels, const std::vector<uint> &sorted_inters,
                                       std::bitset<NBIT> &patch_inner_label);

inline bool triContainsVert(uint t_id, uint v_id, const std::vector<uint> &in_tris);

inline void findVertRingTris(uint v_id, const std::bitset<NBIT> &ref_label, const phmap::flat_hash_set<uint> &inters_tris,
                             const std::vector<uint> &in_tris, const std::vector<std::bitset<NBIT>> &in_labels,
                             std::vector<uint> &one_ring);

inline void findEdgeTris(uint ev0_id, uint ev1_id, const std::bitset<NBIT> &ref_label, const phmap::flat_hash_set<uint> &inters_tris,
                             const std::vector<uint> &in_tris, const std::vector<std::bitset<NBIT>> &in_labels,
                             std::vector<uint> &edge_tris);

inline Ray perturbXRay(const Ray &ray, uint offset);

inline Ray perturbYRay(const Ray &ray, uint offset);

inline Ray perturbZRay(const Ray &ray, uint offset);

inline int perturbRayAndFindIntersTri(const Ray &ray, const std::vector<genericPoint*> &in_verts, const std::vector<uint> &in_tris,
                                       const std::vector<uint> &tris_to_test);

inline IntersInfo fast2DCheckIntersectionOnRay(const Ray &ray, const explicitPoint3D &tv0, const explicitPoint3D &tv1, const explicitPoint3D &tv2);

inline bool checkIntersectionInsideTriangle3D(const Ray &ray, const explicitPoint3D &tv0, const explicitPoint3D &tv1, const explicitPoint3D &tv2);

inline bool checkIntersectionInsideTriangle3DImplPoints(const Ray &ray, const genericPoint *tv0, const genericPoint *tv1, const genericPoint *tv2);

inline void sortIntersectedTrisAlongX(const Ray &ray, const std::vector<genericPoint*> &in_verts,
                                      const std::vector<uint> &in_tris, std::vector<uint> &inters_tris);

inline void sortIntersectedTrisAlongY(const Ray &ray, const std::vector<genericPoint*> &in_verts,
                                      const std::vector<uint> &in_tris, std::vector<uint> &inters_tris);

inline void sortIntersectedTrisAlongZ(const Ray &ray, const std::vector<genericPoint*> &in_verts,
                                      const std::vector<uint> &in_tris, std::vector<uint> &inters_tris);

inline uint checkTriangleOrientation(const Ray &ray, const explicitPoint3D &tv0, const explicitPoint3D &tv1, const explicitPoint3D &tv2);

inline void propagateInnerLabelsOnPatch(const phmap::flat_hash_set<uint> &patch_tris, const std::bitset<NBIT> &patch_inner_label, Labels &labels);

inline void computeFinalExplicitResult(const FastTrimesh &tm, const Labels &labels, uint num_tris_in_final_res,
                                       std::vector<double> &out_coords, std::vector<uint> &out_tris, std::vector<std::bitset<NBIT>> &out_label, bool flat_array);

inline uint boolIntersection(FastTrimesh &tm, const Labels &labels);

inline uint boolUnion(FastTrimesh &tm, const Labels &labels);

inline uint boolSubtraction(FastTrimesh &tm, const Labels &labels);

inline uint boolXOR(FastTrimesh &tm, const Labels &labels);

inline uint bitsetToUint(const std::bitset<NBIT> &b);

inline bool consistentWinding(const uint *t0, const uint *t1);

/// :::::::::::::: DEBUG :::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline std::string printBitset(const std::bitset<NBIT> &b, uint num_label); // just for debug

inline void saveOutputWithLabels(const std::string &filename, cinolib::Trimesh<> &m, const std::vector<std::bitset<NBIT>> &labels);

inline void loadInputWithLabels(const std::string &filename, std::vector<double> &coords, std::vector<uint> &tris, std::vector<std::bitset<NBIT> > &labels);

inline void loadInputWithLabels(const std::string &filename, std::vector<double> &coords, std::vector<uint> &tris, std::vector<uint> &labels);

//#include "booleans.cpp"

#endif //EXACT_BOOLEANS_BOOLEANS_H
