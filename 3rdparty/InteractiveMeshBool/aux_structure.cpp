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


#include "aux_structure.h"
#include "utils.h"

inline void AuxiliaryStructure::initFromTriangleSoup(TriangleSoup &ts)
{
    num_original_vtx = ts.numVerts();
    num_original_tris = ts.numTris();

    coplanar_tris.resize(ts.numTris());

    tri2pts.resize(ts.numTris());
    edge2pts.resize(ts.numEdges());
    tri2segs.resize(ts.numTris());
    tri_has_intersections.resize(ts.numTris(), false);

    num_intersections = 0;
    num_tpi = 0;

    for(uint v_id = 0; v_id < ts.numVerts(); v_id++)
    {
        v_map.insert({ts.vert(v_id), v_id});
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline std::vector<std::pair<uint, uint> > &AuxiliaryStructure::intersectionList()
{
    return intersection_list;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline const std::vector<std::pair<uint, uint> > &AuxiliaryStructure::intersectionList() const
{
    return intersection_list;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline bool AuxiliaryStructure::addVertexInTriangle(uint t_id, uint v_id)
{
    assert(t_id < tri2pts.size());
    auto& points = tri2pts[t_id];
    if(contains(points, v_id)) return false;
    if(points.empty()) points.reserve(8);
    points.push_back(v_id);
    return true;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline bool AuxiliaryStructure::addVertexInEdge(uint e_id, uint v_id)
{
    assert(e_id < edge2pts.size());
    auto& points = edge2pts[e_id];
    if(contains(points, v_id)) return false;
    if(points.empty()) points.reserve(8);
    points.push_back(v_id);
    return true;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline bool AuxiliaryStructure::addSegmentInTriangle(uint t_id, const UIPair &seg)
{
    assert(t_id < tri2segs.size());
    UIPair key_seg = uniquePair(seg);
    auto& segments = tri2segs[t_id];
    if(contains(segments, key_seg)) return false;
    if(segments.empty()) segments.reserve(8);
    segments.push_back(key_seg);
    return true;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void AuxiliaryStructure::addTrianglesInSegment(const UIPair &seg, uint tA_id, uint tB_id)
{
    UIPair key_seg = uniquePair(seg);
    auto& tris = seg2tris[key_seg];
    if(tris.empty()) tris.reserve(4);
    if(tA_id == tB_id)
    {
        if(!contains(tris, tA_id)) tris.push_back(tA_id);
    }
    else
    {
        if(!contains(tris, tA_id)) tris.push_back(tA_id);
        if(!contains(tris, tB_id)) tris.push_back(tB_id);
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void AuxiliaryStructure::splitSegmentInSubSegments(uint orig_v0, uint orig_v1, uint midpoint)
{
    auto& tris = segmentTrianglesList(std::make_pair(orig_v0, orig_v1));
    UIPair sub_seg0 = uniquePair(std::make_pair(orig_v0, midpoint));
    UIPair sub_seg1 = uniquePair(std::make_pair(midpoint, orig_v1));
    seg2tris[sub_seg0] = tris;
    seg2tris[sub_seg1] = tris;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void AuxiliaryStructure::addCoplanarTriangles(uint ta, uint tb)
{
    assert(ta != tb);
    assert(ta < coplanar_tris.size() && tb < coplanar_tris.size());

    if(coplanar_tris[ta].empty()) coplanar_tris[ta].reserve(8);
    if(coplanar_tris[tb].empty()) coplanar_tris[tb].reserve(8);
    coplanar_tris[ta].push_back(tb);
    coplanar_tris[tb].push_back(ta);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline const auxvector<uint> &AuxiliaryStructure::coplanarTriangles(uint t_id) const
{
    assert(t_id < coplanar_tris.size());
    return coplanar_tris[t_id];
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline bool AuxiliaryStructure::triangleHasCoplanars(uint t_id) const
{
    assert(t_id < coplanar_tris.size());
    return (coplanar_tris[t_id].size() > 0);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void AuxiliaryStructure::setTriangleHasIntersections(uint t_id)
{
    assert(t_id < tri_has_intersections.size());
    tri_has_intersections[t_id] = true;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline bool AuxiliaryStructure::triangleHasIntersections(uint t_id) const
{
    assert(t_id < tri_has_intersections.size());
    return tri_has_intersections[t_id];
}


//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline const auxvector<uint> &AuxiliaryStructure::trianglePointsList(uint t_id) const
{
    assert(t_id < tri2pts.size());
    return tri2pts[t_id];
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline const auxvector<uint> &AuxiliaryStructure::edgePointsList(uint e_id) const
{
    assert(e_id < edge2pts.size());
    return edge2pts[e_id];
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline const auxvector<UIPair> &AuxiliaryStructure::triangleSegmentsList(uint t_id) const
{
    assert(t_id < tri2segs.size());
    return tri2segs[t_id];
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline const auxvector<uint> &AuxiliaryStructure::segmentTrianglesList(const UIPair &seg) const
{
    UIPair key_seg = uniquePair(seg);

    auto res = seg2tris.find(key_seg);
    assert(res != seg2tris.end());

    return res->second;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline std::pair<uint, bool> AuxiliaryStructure::addVertexInSortedList(const genericPoint *v, uint pos)
{
    auto ins = v_map.insert({v, pos});

    return std::make_pair(ins.first->second, // the position of v (pos if first time, or the previous saved position otherwise)
                          ins.second);       // the result of the insert operation /true or false)
}

//it returns -1 if the pocket is not already present,
// the i-index of the corresponding triangles in the new_label array otherwise
inline int AuxiliaryStructure::addVisitedPolygonPocket(const std::vector<uint> &polygon, uint pos)
{
    auto poly_it = pockets_map.insert(std::make_pair(polygon, pos));

    if(poly_it.second) return -1; // polygon not present yet

    return static_cast<int>(poly_it.first->second);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline UIPair AuxiliaryStructure::uniquePair(const UIPair &uip) const
{
    if(uip.first < uip.second) return  uip;
    return std::make_pair(uip.second, uip.first);
}