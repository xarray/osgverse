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

#ifndef TRIANGLESOUP_H
#define TRIANGLESOUP_H

#include <cinolib/geometry/vec_mat.h>
#include "common.h"
#include "implicit_point.h"

#include "../parallel_hashmap/phmap.h"
//#include <tbb/tbb.h>

#include "utils.h"

#include <vector>
#include <map>
#include <bitset>

typedef std::pair<uint, uint> Edge;

template<typename K, typename V>
using EdgeMap = phmap::flat_hash_map<K, V>;

class TriangleSoup
{
    public:

        inline TriangleSoup(point_arena& arena, std::vector<genericPoint*> &in_vertices, std::vector<uint> &in_tris, std::vector< std::bitset<NBIT> > &labels, double multiplier, bool parallel)
            : vertices(in_vertices), triangles(in_tris), tri_labels(labels)
        {
            init(arena, multiplier, parallel);
        }

        inline ~TriangleSoup()
        {
        }

        inline void init(point_arena& arena, double multiplier, bool parallel);

        inline uint numVerts() const;
        inline uint numTris() const;
        inline uint numEdges() const;

        //inline uint numOrigVertices() const;
        inline uint numOrigTriangles() const;

        // VERTICES
        inline const genericPoint* vert(uint v_id) const;

        inline const double* vertPtr(uint v_id) const;

        inline double vertX(uint v_id) const;
        inline double vertY(uint v_id) const;
        inline double vertZ(uint v_id) const;

        inline uint addImplVert(genericPoint* gp);

        // EDGES
        inline int edgeID(uint v0_id, uint v1_id) const;

        inline const genericPoint* edgeVert(uint e_id, uint off) const;

        inline const double* edgeVertPtr(uint e_id, uint off) const;

        inline uint edgeOppositeToVert(uint t_id, uint v_id) const;

        inline void addEdge(uint v0_id, uint v1_id);

        // TRIANGLES
        inline const std::vector<uint>& trisVector() const;

        inline const uint* tri(uint t_id) const;

        inline uint triVertID(uint t_id, uint off) const;

        inline const genericPoint* triVert(uint t_id, uint off) const;

        inline const double* triVertPtr(uint t_id, uint off) const;

        inline uint triEdgeID(uint t_id, uint off) const;

        inline Plane triPlane(uint t_id) const;

        inline bool triContainsVert(uint t_id, uint v_id) const;

        inline bool triContainsEdge(const uint t_id, uint ev0_id, uint ev1_id) const;

        inline std::bitset<NBIT> triLabel(uint t_id) const;

        // JOLLY POINTS
        inline const genericPoint* jollyPoint(uint off) const;

        inline void appendJollyPoints();

    private:

        std::vector<genericPoint*>      &vertices;

        std::vector<Edge>               edges;

        std::vector<uint>               &triangles;
        std::vector<std::bitset<NBIT>>  &tri_labels;
        std::vector<Plane>              tri_planes;

        std::vector<genericPoint*>      jolly_points;

        EdgeMap <Edge, uint> edge_map;

        uint num_orig_vtxs;
        uint num_orig_tris;

        // PRIVATE METHODS
        inline void initJollyPoints(point_arena& arena, double multiplier);

        inline Edge uniqueEdge(uint v0_id, uint v1_id) const;
};

//#include "triangle_soup.cpp"

#endif // TRIANGLESOUP_H
