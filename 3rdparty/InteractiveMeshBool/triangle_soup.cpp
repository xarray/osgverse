/*****************************************************************************************
 *              MIT License                                                              *
 *                                                                                       *
 * Copyright (c) 2020 Gianmarco Cherchi, Marco Livesu, Riccardo Scateni e Marco Attene   *
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
 * ***************************************************************************************/

#include "triangle_soup.h"

//#include <tbb/tbb.h>

inline void TriangleSoup::init(point_arena& arena, double multiplier, bool parallel)
{
    if (parallel) {
        num_orig_vtxs = static_cast<uint>(vertices.size());
        num_orig_tris = static_cast<uint>(triangles.size() / 3);

        edges.reserve(numVerts() + numTris());
        edge_map.reserve(numVerts() + numTris());
        tri_planes.resize(numTris());

        // vertices
        for(uint v_id = 0; v_id < num_orig_vtxs; v_id++)
        {
            const explicitPoint3D &e = vertices[v_id]->toExplicit3D();
            vertices[v_id]->toExplicit3D().set(e.X() * multiplier, e.Y() * multiplier, e.Z() * multiplier);
        }

        // this is done separately since it is expensive
        ::parallel_for((uint)0, num_orig_tris, [this](uint t_id)
        {
            uint v0_id = triVertID(t_id, 0), v1_id = triVertID(t_id, 1), v2_id = triVertID(t_id, 2);

            tri_planes[t_id] = intToPlane(genericPoint::maxComponentInTriangleNormal(vertX(v0_id), vertY(v0_id), vertZ(v0_id),
                                                                                    vertX(v1_id), vertY(v1_id), vertZ(v1_id),
                                                                                    vertX(v2_id), vertY(v2_id), vertZ(v2_id)));
        });
        
        // triangles
        auto edges = std::vector<Edge>(num_orig_tris * 3);
        for(uint t_id = 0; t_id < num_orig_tris; t_id++)
        {
            uint v0_id = triVertID(t_id, 0), v1_id = triVertID(t_id, 1), v2_id = triVertID(t_id, 2);
            edges[t_id * 3 + 0] = uniqueEdge(v0_id, v1_id);
            edges[t_id * 3 + 1] = uniqueEdge(v1_id, v2_id);
            edges[t_id * 3 + 2] = uniqueEdge(v2_id, v0_id);
        }
#if false
        tbb::parallel_sort(edges.begin(), edges.end());
#else
        std::sort(edges.begin(), edges.end());
#endif

        // I think since it keeps order that we do not need
        for(auto e_id = (uint)0; e_id < (uint)edges.size(); e_id++)
        {
            if(e_id == 0 || edges[e_id] != edges[e_id - 1]) 
                addEdge(edges[e_id].first, edges[e_id].second);
        }

        initJollyPoints(arena, multiplier);
    }
    else
    {
        num_orig_vtxs = static_cast<uint>(vertices.size());
        num_orig_tris = static_cast<uint>(triangles.size() / 3);

        edges.reserve(numVerts() + numTris());
        edge_map.reserve(numVerts() + numTris());
        tri_planes.resize(numTris());

        // vertices
        for(uint v_id = 0; v_id < num_orig_vtxs; v_id++)
        {
            const explicitPoint3D &e = vertices[v_id]->toExplicit3D();
            vertices[v_id]->toExplicit3D().set(e.X() * multiplier, e.Y() * multiplier, e.Z() * multiplier);
        }

        // this is done separately since it is expensive
        for(uint t_id = 0; t_id < num_orig_tris; t_id++)
        {
            uint v0_id = triVertID(t_id, 0), v1_id = triVertID(t_id, 1), v2_id = triVertID(t_id, 2);

            tri_planes[t_id] = intToPlane(genericPoint::maxComponentInTriangleNormal(vertX(v0_id), vertY(v0_id), vertZ(v0_id),
                                                                                    vertX(v1_id), vertY(v1_id), vertZ(v1_id),
                                                                                    vertX(v2_id), vertY(v2_id), vertZ(v2_id)));
        }
        
        // triangles
        for(uint t_id = 0; t_id < num_orig_tris; t_id++)
        {
            uint v0_id = triVertID(t_id, 0), v1_id = triVertID(t_id, 1), v2_id = triVertID(t_id, 2);

            // edges
            addEdge(v0_id, v1_id);
            addEdge(v1_id, v2_id);
            addEdge(v2_id, v0_id);
        }

        initJollyPoints(arena, multiplier);
    }
}

/*******************************************************************************************************
 *      VERTICES
 * ****************************************************************************************************/

inline uint TriangleSoup::numVerts() const
{
    return static_cast<uint>(vertices.size());
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline uint TriangleSoup::numTris() const
{
    return static_cast<uint>(triangles.size() / 3);
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline uint TriangleSoup::numEdges() const
{
    return static_cast<uint>(edges.size());
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline uint TriangleSoup::numOrigTriangles() const
{
    return num_orig_tris;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline const genericPoint* TriangleSoup::vert(uint v_id) const
{
    assert(v_id < numVerts() && "vtx id out of range");
    return vertices[v_id];
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline const double* TriangleSoup::vertPtr(uint v_id) const
{
    assert(v_id < num_orig_vtxs && "vtx id out of range of original points");
    return vertices[v_id]->toExplicit3D().ptr();
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline double TriangleSoup::vertX(uint v_id) const
{
    assert(v_id < num_orig_vtxs && "vtx id out of range");
    return vertices[v_id]->toExplicit3D().X();
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline double TriangleSoup::vertY(uint v_id) const
{
    assert(v_id < num_orig_vtxs && "vtx id out of range");
    return vertices[v_id]->toExplicit3D().Y();
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline double TriangleSoup::vertZ(uint v_id) const
{
    assert(v_id < num_orig_vtxs && "vtx id out of range");
    return vertices[v_id]->toExplicit3D().Z();
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline uint TriangleSoup::addImplVert(genericPoint* gp)
{
    vertices.push_back(gp);
    return static_cast<uint>(vertices.size() -1);
}

/*******************************************************************************************************
 *      EDGES
 * ****************************************************************************************************/

inline int TriangleSoup::edgeID(uint v0_id, uint v1_id) const
{
    auto it = edge_map.find(uniqueEdge(v0_id, v1_id));

    if(it == edge_map.end()) return -1;
    return static_cast<int>(it->second); // edge id
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline const genericPoint* TriangleSoup::edgeVert(uint e_id, uint off) const
{
    assert(e_id < edges.size() && "e_id out of range");
    if(off == 0) return vert(edges[e_id].first);
    else         return vert(edges[e_id].second);
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline const double* TriangleSoup::edgeVertPtr(uint e_id, uint off) const
{
    assert(e_id < edges.size() && "e_id out of range");
    if(off == 0) return vertPtr(edges[e_id].first);
    else         return vertPtr(edges[e_id].second);
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline uint TriangleSoup::edgeOppositeToVert(uint t_id, uint v_id) const
{
    assert(t_id < numTris() && "t_id out of range");
    assert(v_id < numVerts() && "vtx id out of range");

    int e_id = -1;

    if     (triVertID(t_id, 0) == v_id) e_id = edgeID(triVertID(t_id, 1), triVertID(t_id, 2));
    else if(triVertID(t_id, 1) == v_id) e_id = edgeID(triVertID(t_id, 0), triVertID(t_id, 2));
    else if(triVertID(t_id, 2) == v_id) e_id = edgeID(triVertID(t_id, 0), triVertID(t_id, 1));

    assert(e_id >= 0 && "Opposite edge not found");
    return static_cast<uint>(e_id);
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void TriangleSoup::addEdge(uint v0_id, uint v1_id)
{
    uint tmp_id = static_cast<uint>(edges.size());
    Edge e = uniqueEdge(v0_id, v1_id);

    auto it = edge_map.insert({e, tmp_id});

    if(it.second) edges.push_back(e);
}

/*******************************************************************************************************
 *      TRIANGLES
 * ****************************************************************************************************/

const std::vector<uint>& TriangleSoup::trisVector() const
{
    return triangles;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline const uint* TriangleSoup::tri(uint t_id) const
{
    assert(t_id < numTris() && "t_id out of range");
    return &triangles[ 3 * t_id];
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline uint TriangleSoup::triVertID(uint t_id, uint off) const
{
    assert(t_id < numTris() && "t_id out of range");
    return triangles[3 * t_id + off];
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline const genericPoint* TriangleSoup::triVert(uint t_id, uint off) const
{
    assert(t_id < numTris() && "t_id out of range");
    return vert(triangles[3 * t_id + off]);
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline const double* TriangleSoup::triVertPtr(uint t_id, uint off) const
{
    assert(t_id < numTris() && "t_id out of range");
    return vertPtr(triangles[3 * t_id + off]);
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline uint TriangleSoup::triEdgeID(uint t_id, uint off) const
{
    assert(t_id < numTris() && "t_id out of range");
    int e_id = edgeID(triangles[3 * t_id + off],
                      triangles[3 * t_id + ((off +1) %3)]);

    assert(e_id >= 0 && "no triangle edge found");
    return static_cast<uint>(e_id);
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline Plane TriangleSoup::triPlane(uint t_id) const
{
    assert(t_id < numTris() && "t_id out of range");
    return tri_planes[t_id];
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline bool TriangleSoup::triContainsVert(uint t_id, uint v_id) const
{
    assert(t_id < numTris() && "t_id out of range");
    assert(v_id < numVerts() && "v_id out of range");

    if(triangles[3 * t_id]     == v_id) return true;
    if(triangles[3 * t_id + 1] == v_id) return true;
    if(triangles[3 * t_id + 2] == v_id) return true;
    return false;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline bool TriangleSoup::triContainsEdge(const uint t_id, uint ev0_id, uint ev1_id) const
{
    return (triContainsVert(t_id, ev0_id) && triContainsVert(t_id, ev1_id));
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline std::bitset<NBIT> TriangleSoup::triLabel(uint t_id) const
{
    assert(t_id < numTris() && "t_id out of range");
    return tri_labels[t_id];
}

/*******************************************************************************************************
 *      JOLLY POINTS
 * ****************************************************************************************************/

inline const genericPoint* TriangleSoup::jollyPoint(uint off) const
{
    assert(off < 4 && "jolly point id out of range");
    return jolly_points[off];
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void TriangleSoup::appendJollyPoints()
{
    vertices.push_back(jolly_points[0]);
    vertices.push_back(jolly_points[1]);
    vertices.push_back(jolly_points[2]);
    vertices.push_back(jolly_points[3]);
    vertices.push_back(jolly_points[4]);
}

/********************************************************************************************************
 *              PRIVATE METHODS
 * ****************************************************************************************************/

inline void TriangleSoup::initJollyPoints(point_arena& arena, double multiplier)
{
    jolly_points.push_back(&arena.jolly.emplace_back(0.94280904158 * multiplier, 0.0 * multiplier, -0.333333333 * multiplier));
    jolly_points.push_back(&arena.jolly.emplace_back(-0.47140452079 * multiplier, 0.81649658092 * multiplier, -0.333333333 * multiplier));
    jolly_points.push_back(&arena.jolly.emplace_back(-0.47140452079 * multiplier, -0.81649658092 * multiplier, -0.333333333 * multiplier));
    jolly_points.push_back(&arena.jolly.emplace_back(0.0 * multiplier, 0.0 * multiplier, 1.0 * multiplier));
    jolly_points.push_back(&arena.jolly.emplace_back(multiplier, 0.0, 0.0));
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline Edge TriangleSoup::uniqueEdge(uint v0_id, uint v1_id) const
{
    if(v0_id < v1_id) return {v0_id, v1_id};
    return {v1_id, v0_id};
}

