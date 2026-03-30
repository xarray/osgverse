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

#ifndef FASTTRIMESH_H
#define FASTTRIMESH_H

#include "implicit_point.h"
#include "tree.h"
#include "common.h"

#include "../parallel_hashmap/phmap.h"

//#include <absl/container/inlined_vector.h>
template<typename T>
//using fmvector = absl::InlinedVector<T, 16>;
using fmvector = std::vector<T>;

struct iVtx
{
    iVtx() : p(nullptr), info(0){}
    iVtx(const genericPoint *_p, uint _info) : p(_p), info(_info){}

    const genericPoint* p;
    uint info;
};

struct iEdge
{
    iEdge() : v(0,0), constr(false) { }
    iEdge(uint _v0, uint _v1, const bool _b) : v(_v0, _v1), constr(_b){}

    std::pair<uint, uint> v;
    bool constr = false;
};

struct iTri
{
    iTri() : v{0,0,0}, info(0) { }
    iTri(uint _v0, uint _v1, uint _v2, uint _n) : info(_n)
    {
        v[0] = _v0;
        v[1] = _v1;
        v[2] = _v2;
    }

    uint v[3];
    uint info = 0;
};

class FastTrimesh
{
    public:

        inline FastTrimesh(){}

        inline FastTrimesh(const genericPoint* tv0, const genericPoint* tv1, const genericPoint *tv2, const uint *tv_id, const Plane &ref_p);

        inline FastTrimesh(const std::vector<genericPoint*> &in_verts, const std::vector<uint> &in_tris, bool parallel);


        inline void preAllocateSpace(uint estimated_num_verts);

        inline void resetTrianglesInfo();

        inline uint numVerts() const;
        inline uint numEdges() const;
        inline uint numTris() const;
        inline Plane refPlane() const;

        // VERTICES
        inline const genericPoint* vert(uint v_id) const;

        inline uint vertOrigID(uint new_v_id) const; // from new_id to original mesh id

        inline uint vertNewID(uint orig_v_id) const; // from original mesh id to new_id

        inline uint vertValence(uint v_id) const;

        inline const fmvector<uint> &adjV2E(uint v_id) const;

        inline fmvector<uint> adjV2T(uint v_id) const;

        inline void resetVerticesInfo();

        inline void setVertInfo(const uint v_id, const uint info);

        inline uint vertInfo(const uint v_id) const;

        // EDGES
        inline const std::pair<uint, uint> &edge(uint e_id) const;

        inline uint edgeVertID(uint e_id, uint off) const;

        inline int edgeID(uint ev0_id, uint ev1_id) const;

        inline bool edgeIsConstr(uint e_id) const;

        inline void setEdgeConstr(uint e_id);

        inline uint edgeOppToVert(uint t_id, uint v_id) const;

        inline bool edgeIsBoundary(uint e_id) const;

        inline bool edgeIsManifold(uint e_id) const;

        inline const fmvector<uint> &adjE2T(uint e_id) const;

        inline void edgeSetVisited(uint e_id, const bool &vis);

        inline bool edgeIsVisited(uint e_id) const;


        // TRIANGLES
        inline const uint *tri(uint t_id) const;

        inline int triID(uint tv0_id, uint tv1_id, uint tv2_id) const;

        inline uint triVertID(uint t_id, uint off) const;

        inline const genericPoint *triVert(uint t_id, uint off) const;

        inline int triEdgeID(uint t_id, uint off) const;

        inline uint triNodeID(uint t_id) const;

        inline void setTriNodeID(uint t_id, uint n_id);

        inline uint triVertOppositeTo(uint t_id, uint v0_id, uint v1_id) const;

        inline int triOppToEdge(uint e_id, uint t_id) const;

        inline fmvector<uint> adjT2E(uint t_id) const;
        inline std::vector<std::array<uint, 3>> adjT2EAll(bool parallel) const;

        inline fmvector<uint> adjT2T(uint t_id) const;

        inline bool triVertsAreCCW(uint t_id, uint curr_v_id, uint prev_v_id) const;

        inline int triOrientation(uint t_id) const;

        inline bool triContainsVert(uint t_id, uint v_id) const;

        inline uint triVertOffset(uint t_id, uint v_id) const;

        inline uint triInfo(uint t_id) const;

        inline void setTriInfo(uint t_id, uint val);

        // MESH MANIPULATION
        inline uint addVert(const genericPoint *v, uint orig_v_id);

        inline void addVert(const genericPoint *v);

        inline uint addTri(uint tv0_id, uint tv1_id, uint tv2_id);

        inline void removeEdge(uint e_id);

        inline void removeTri(uint t_id);

        inline void removeTris(const std::vector<uint> &t_ids);
        //inline void removeTris(const fmvector<uint> &t_ids);

        inline void splitEdge(const uint  &e_id, uint v_id);

        inline void splitEdge(const uint  &e_id, uint v_id, Tree &tree);

        inline void splitTri(uint t_id, uint v_id);

        inline void splitTri(uint t_id, uint v_id, Tree &tree);

        inline void flipTri(uint t_id);

    private:
        std::vector<iVtx>    vertices;
        std::vector<iEdge>   edges;
        std::vector<iTri>    triangles;

        std::vector< fmvector<uint> >    v2e;
        std::vector< fmvector<uint> >    e2t;

        phmap::flat_hash_map <uint, uint> rev_vtx_map;

        Plane triangle_plane;

        // PRIVATE METHODS
        inline int addEdge(uint ev0_id, uint ev1_id);

        inline bool edgeContainsVert(uint e_id, uint v_id) const;

        inline void removeFromVec(fmvector<uint> &vec, uint elem);

        inline void triSwitch(uint t0_id, uint t1_id);

        inline void edgeSwitch(uint e0_id, const uint e1_id);

        inline void removeEdgeUnref(uint e_id);

        inline void removeTriUnref(uint t_id);
};

//#include "fast_trimesh.cpp"

#endif // FASTTRIMESH_H


