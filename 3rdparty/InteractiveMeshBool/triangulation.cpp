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

#include <numeric>
#include <stack>

#include "yocto_parallel.h"
#include "triangulation.h"
#include "utils.h"

//#include <tbb/tbb.h>
#include "custom_stack.h"

inline void triangulateSingleTriangle(TriangleSoup &ts, point_arena& arena, FastTrimesh &subm, uint t_id, AuxiliaryStructure &g, std::vector<uint> &new_tris, std::vector< std::bitset<NBIT> > &new_labels, std::mutex& mutex)
{
    /*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
     *                                  POINTS AND SEGMENTS RECOVERY
     * :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

    const auto& t_points = g.trianglePointsList(t_id);

    int e0_id = ts.edgeID(subm.vertOrigID(0), subm.vertOrigID(1));      assert(e0_id != -1);
    int e1_id = ts.edgeID(subm.vertOrigID(1), subm.vertOrigID(2));      assert(e1_id != -1);
    int e2_id = ts.edgeID(subm.vertOrigID(2), subm.vertOrigID(0));      assert(e2_id != -1);

    //auxvector<uint> e0_points, e1_points, e2_points;
    //sortedVertexListAlongSegment(ts, g.edgePointsList(static_cast<uint>(e0_id)), subm.vertOrigID(0), subm.vertOrigID(1), e0_points);
    //sortedVertexListAlongSegment(ts, g.edgePointsList(static_cast<uint>(e1_id)), subm.vertOrigID(1), subm.vertOrigID(2), e1_points);
    //sortedVertexListAlongSegment(ts, g.edgePointsList(static_cast<uint>(e2_id)), subm.vertOrigID(2), subm.vertOrigID(0), e2_points);

    const auxvector<uint> &e0_points = g.edgePointsList(static_cast<uint>(e0_id));
    const auxvector<uint> &e1_points = g.edgePointsList(static_cast<uint>(e1_id));
    const auxvector<uint> &e2_points = g.edgePointsList(static_cast<uint>(e2_id));

    auxvector<UIPair> t_segments(g.triangleSegmentsList(t_id).begin(), g.triangleSegmentsList(t_id).end());

    //uint estimated_vert_num = static_cast<uint>(t_points.size() + e0_points.size() + e1_points.size() + e2_points.size());
    uint estimated_vert_num = static_cast<uint>(3 + t_points.size() + e0_points.size() + e1_points.size() + e2_points.size());
    subm.preAllocateSpace(estimated_vert_num);

    /*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
     *                                  TRIANGLE SPLIT
     * :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

    /*if(t_points.size() < 50)
        splitSingleTriangle(ts, subm, t_points);
    else
        splitSingleTriangleWithTree(ts, subm, t_points);
    */
    splitSingleTriangleWithStack(ts, subm, t_points, e0_points, e1_points, e2_points);

    /*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
     *                                  EDGE SPLIT
     * :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

    //splitSingleEdge(ts, subm, 0, 1, e0_points);
    //splitSingleEdge(ts, subm, 1, 2, e1_points);
    //splitSingleEdge(ts, subm, 2, 0, e2_points);

    /*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
     *                           CONSTRAINT SEGMENT INSERTION
     * :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

    addConstraintSegmentsInSingleTriangle(ts, arena, subm, g, t_segments, mutex);

    /*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
     *                      POCKETS IN COPLANAR TRIANGLES SOLVING
     * :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

    if(g.triangleHasCoplanars(t_id))
    {
        { // start critical section...
            std::lock_guard<std::mutex> lock(mutex);
            solvePocketsInCoplanarTriangle(subm, g, new_tris, new_labels, ts.triLabel(t_id));
        } // end critical section
    }
    else
    {
        /*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
         *                     NEW TRIANGLE CREATION (for final mesh)
         * :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

        { // start critical section...
            std::lock_guard<std::mutex> lock(mutex);
            for(uint ti = 0; ti < subm.numTris(); ti++)
            {
                const uint *tri = subm.tri(ti);
                new_tris.push_back(subm.vertOrigID(tri[0]));
                new_tris.push_back(subm.vertOrigID(tri[1]));
                new_tris.push_back(subm.vertOrigID(tri[2]));
                new_labels.push_back(ts.triLabel(t_id));
            } // endl critical section
        }
    }
}

inline void triangulation(TriangleSoup &ts, point_arena& arena, AuxiliaryStructure &g, std::vector<uint> &new_tris, std::vector< std::bitset<NBIT> > &new_labels, bool parallel)
{
    new_labels.clear();
    new_tris.clear();
    new_tris.reserve(2 * 3 * ts.numTris());
    new_labels.reserve(2 * ts.numTris());

    std::vector<uint> tris_to_split;
    tris_to_split.reserve(ts.numTris());

    for(uint t_id = 0; t_id < ts.numTris(); t_id++)
    {
        if(g.triangleHasIntersections(t_id) || g.triangleHasCoplanars(t_id))
            tris_to_split.push_back(t_id);
        else
        {
            // triangle without intersections directly goes to the output list
            new_tris.push_back(ts.triVertID(t_id, 0));
            new_tris.push_back(ts.triVertID(t_id, 1));
            new_tris.push_back(ts.triVertID(t_id, 2));
            new_labels.push_back(ts.triLabel(t_id));
        }
    }

    // processing the triangles to split
    std::mutex mutex;
    if(parallel){
        ::parallel_for((uint)0, (uint)tris_to_split.size(), [&](uint t) {
            //for (uint t=0; t < (uint)tris_to_split.size(); t++) {
            uint t_id = tris_to_split[t];
            FastTrimesh subm(ts.triVert(t_id, 0),
                             ts.triVert(t_id, 1),
                             ts.triVert(t_id, 2),
                             ts.tri(t_id),
                             ts.triPlane(t_id));

            triangulateSingleTriangle(ts, arena, subm, t_id, g, new_tris, new_labels, mutex);
        });
    }else{
        for (uint t=0; t < (uint)tris_to_split.size(); t++) {
            uint t_id = tris_to_split[t];
            FastTrimesh subm(ts.triVert(t_id, 0),
                             ts.triVert(t_id, 1),
                             ts.triVert(t_id, 2),
                             ts.tri(t_id),
                             ts.triPlane(t_id));

            triangulateSingleTriangle(ts, arena, subm, t_id, g, new_tris, new_labels, mutex);
        }
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void splitSingleTriangle(const TriangleSoup &ts, FastTrimesh &subm, const auxvector<uint> &points)
{
    if(points.empty()) return;

    // add the first point
    auto curr = points.begin();
    uint v_pos = subm.addVert(ts.vert(*curr), *curr);
    subm.splitTri(0, v_pos);

    // progressively add the other points, looking for the triangle
    // that contains them only among the newly generated triangles
    while(++curr != points.end())
    {
        v_pos = subm.addVert(ts.vert(*curr), *curr);

        int cont_t_id = findContainingTriangle(subm, v_pos);
        assert(cont_t_id >= 0 && "No containing triangle found!");

        uint e0_id = static_cast<uint>(subm.triEdgeID(static_cast<uint>(cont_t_id), 0));
        uint e1_id = static_cast<uint>(subm.triEdgeID(static_cast<uint>(cont_t_id), 1));
        uint e2_id = static_cast<uint>(subm.triEdgeID(static_cast<uint>(cont_t_id), 2));

        if(fastPointOnLine(subm, e0_id, v_pos))
            subm.splitEdge(e0_id, v_pos);

        else if(fastPointOnLine(subm, e1_id, v_pos))
            subm.splitEdge(e1_id, v_pos);

        else if(fastPointOnLine(subm, e2_id, v_pos))
            subm.splitEdge(e2_id, v_pos);

        else subm.splitTri(static_cast<uint>(cont_t_id), v_pos);
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
inline void splitSingleTriangleWithStack(const TriangleSoup &ts, FastTrimesh &subm, const auxvector<uint> &points,  const auxvector<uint> &e0_points, const auxvector<uint> &e1_points, const auxvector<uint> &e2_points)
{
    if(points.empty() && e0_points.empty() && e1_points.empty() && e2_points.empty()) return;

    int size_p2ins = 3 + points.size() + e0_points.size() + e1_points.size() + e2_points.size();
    CustomStack stack_sub_tri(size_p2ins * 3);
    std::vector<auxvector<uint>>curr_subdv(4);

    auxvector<uint> all_points;
    all_points.reserve(size_p2ins);

    all_points.push_back(subm.triVertID(0,0));
    all_points.push_back(subm.triVertID(0,1));
    all_points.push_back(subm.triVertID(0,2));

    for (uint p : e0_points){
        uint v_pos = subm.addVert(ts.vert(p), p);
        all_points.push_back(v_pos);
    }

    for (uint p : e1_points){
        uint v_pos = subm.addVert(ts.vert(p), p);
        all_points.push_back(v_pos);
    }

    for (uint p : e2_points){
        uint v_pos = subm.addVert(ts.vert(p), p);
        all_points.push_back(v_pos);
    }

    for (uint p : points){
        uint v_pos = subm.addVert(ts.vert(p), p);
        all_points.push_back(v_pos);
    }

    stack_sub_tri.push(all_points);

    while(!stack_sub_tri.empty()){

        auxvector<uint> &curr_tri = stack_sub_tri.pop();
        bool onEdge = false;

        if(curr_tri.empty()) continue;

        curr_subdv[0].clear();
        curr_subdv[1].clear();
        if (!curr_subdv[2].empty()) curr_subdv[2].clear();
        if (!curr_subdv[3].empty()) curr_subdv[3].clear();

        int t_id = subm.triID(curr_tri[0], curr_tri[1], curr_tri[2]);

        if(t_id == -1) continue;

        uint v_pos = static_cast<uint>(curr_tri[3]);

        for (int i = 0 ; i < 3; ++i){
            int e_id = subm.triEdgeID(static_cast<uint>(t_id), i); assert(e_id != -1 && "No edge found");

            if(fastPointOnLine(subm, static_cast<uint>(e_id), v_pos)) {
                onEdge = true;

                uint v0 = subm.edgeVertID(static_cast<uint>(e_id),0);
                uint v1 = subm.edgeVertID(static_cast<uint>(e_id),1);

                uint v_opp = subm.triVertOppositeTo(static_cast<uint>(t_id),v0,v1);

                curr_subdv[0].reserve(curr_tri.size());
                curr_subdv[1].reserve(curr_tri.size());

                curr_subdv[0].push_back(v0);
                curr_subdv[0].push_back(v_pos);
                curr_subdv[0].push_back(v_opp);

                curr_subdv[1].push_back(v_opp);
                curr_subdv[1].push_back(v_pos);
                curr_subdv[1].push_back(v1);

                fmvector<uint> e2t = subm.adjE2T(static_cast<uint>(e_id));

                if(e2t.size() > 1) {

                    curr_subdv[2].reserve(curr_tri.size());
                    curr_subdv[3].reserve(curr_tri.size());

                    uint t_adj_id = (e2t[0] == static_cast<uint>(t_id)) ? e2t[1] : e2t[0];

                    v_opp = subm.triVertOppositeTo(t_adj_id,v1,v0);

                    const auxvector<uint> &adj_tri = stack_sub_tri.getTriangleFromStack(v1,v_opp,v0);

                    curr_tri.reserve(curr_tri.size() + adj_tri.size());

                    for (int j = 3; j < adj_tri.size(); ++j) {
                        uint p = adj_tri[j];
                        if (p != v_pos && std::find(curr_tri.begin(), curr_tri.end(), p) == curr_tri.end())
                            curr_tri.push_back(p);
                    }

                    curr_subdv[2].push_back(v_opp);
                    curr_subdv[2].push_back(v_pos);
                    curr_subdv[2].push_back(v0);

                    curr_subdv[3].push_back(v1);
                    curr_subdv[3].push_back(v_pos);
                    curr_subdv[3].push_back(v_opp);
                }

                subm.splitEdge(static_cast<uint>(e_id), v_pos);
                break;
            }
        }

        if (!onEdge){
            curr_subdv[0].reserve(curr_tri.size());
            curr_subdv[1].reserve(curr_tri.size());
            curr_subdv[2].reserve(curr_tri.size());

            curr_subdv[0].push_back(curr_tri[1]);
            curr_subdv[0].push_back(v_pos);
            curr_subdv[0].push_back(curr_tri[0]);

            curr_subdv[1].push_back(curr_tri[2]);
            curr_subdv[1].push_back(v_pos);
            curr_subdv[1].push_back(curr_tri[1]);

            curr_subdv[2].push_back(curr_tri[0]);
            curr_subdv[2].push_back(v_pos);
            curr_subdv[2].push_back(curr_tri[2]);

            subm.splitTri(static_cast<uint>(t_id), v_pos);
        }

        if (curr_tri.size() > 4)
            repositionPointsInStack(subm, stack_sub_tri, curr_subdv, curr_tri);
    }
}


inline void repositionPointsInStack(FastTrimesh &subm, CustomStack &stack_sub_tri, std::vector<auxvector<uint>> &curr_subdv, auxvector<uint> &curr_tri)
{
    int n_insertions;

    for (int i = 4; i < curr_tri.size() ; ++i){

        n_insertions = 0;
        const genericPoint &p = *subm.vert(curr_tri[i]);
        const genericPoint &v_pos = *subm.vert(curr_subdv[0][1]);



        if(genericPoint::pointInTriangle(p,*subm.vert(curr_subdv[0][0]), v_pos, *subm.vert(curr_subdv[0][2]))){
            n_insertions++;
            curr_subdv[0].push_back(curr_tri[i]);}


        if(genericPoint::pointInTriangle(p, *subm.vert(curr_subdv[1][0]), v_pos, *subm.vert(curr_subdv[1][2]))){
            n_insertions++;
            curr_subdv[1].push_back(curr_tri[i]);
        }

        if(n_insertions == 2) continue;


        if(!curr_subdv[2].empty() && genericPoint::pointInTriangle(p, *subm.vert(curr_subdv[2][0]), v_pos,
                                                                   *subm.vert(curr_subdv[2][2]))){
            n_insertions++;
            curr_subdv[2].push_back(curr_tri[i]);
        }

        if(n_insertions == 2) continue;


        if(!curr_subdv[3].empty() && genericPoint::pointInTriangle(p,*subm.vert(curr_subdv[3][0]), v_pos,
                                                                   *subm.vert(curr_subdv[3][2]))) {
            curr_subdv[3].push_back(curr_tri[i]);
        }
    }

    for(int i = 0 ; i < curr_subdv.size(); ++i){
        if((i > 1 && curr_subdv[i].empty()) || curr_subdv[i].size() == 3)
            continue;

        stack_sub_tri.push(curr_subdv[i]);

    }
}
//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void splitSingleTriangleWithTree(const TriangleSoup &ts, FastTrimesh &subm, const auxvector<uint> &points)
{
    if(points.empty()) return;

    Tree tree(static_cast<uint>(points.size() * points.size()));
    uint n_id = tree.addNode(0, 1, 2);
    subm.setTriNodeID(0, n_id);

    // add the first point
    auto curr = points.begin();
    uint v_pos = subm.addVert(ts.vert(*curr), *curr);
    subm.splitTri(0, v_pos, tree);

    // progressively add the other points, looking for the triangle
    // that contains them only among the newly generated triangles
    while(++curr != points.end())
    {
        v_pos = subm.addVert(ts.vert(*curr), *curr);

        const Node n_cont = findContainingTriangleInTree(subm, v_pos, tree);
        int cont_t_id = subm.triID(n_cont.v0, n_cont.v1, n_cont.v2);
        assert(cont_t_id >= 0 && "No containing triangle found!");

        uint e0_id = static_cast<uint>(subm.triEdgeID(static_cast<uint>(cont_t_id), 0));
        uint e1_id = static_cast<uint>(subm.triEdgeID(static_cast<uint>(cont_t_id), 1));
        uint e2_id = static_cast<uint>(subm.triEdgeID(static_cast<uint>(cont_t_id), 2));

        if(fastPointOnLine(subm, e0_id, v_pos))
            subm.splitEdge(e0_id, v_pos, tree);

        else if(fastPointOnLine(subm, e1_id, v_pos))
            subm.splitEdge(e1_id, v_pos, tree);

        else if(fastPointOnLine(subm, e2_id, v_pos))
            subm.splitEdge(e2_id, v_pos, tree);

        else subm.splitTri(static_cast<uint>(cont_t_id), v_pos, tree);
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline int findContainingTriangle(const FastTrimesh &subm, uint p_id)
{
    for(uint t_id = 0; t_id < subm.numTris(); t_id++)
    {
        if(genericPoint::pointInTriangle(*subm.vert(p_id), *subm.vert(subm.triVertID(t_id, 0)),
                                         *subm.vert(subm.triVertID(t_id, 1)), *subm.vert(subm.triVertID(t_id, 2))))
            return static_cast<int>(t_id);
    }

    return -1; // this should not appen
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline const Node &findContainingTriangleInTree(const FastTrimesh &subm, uint p_id, const Tree &tree)
{
    return innerFindContainingTriangleInTree(tree, 0, subm, subm.vert(p_id));
}

inline const Node &innerFindContainingTriangleInTree(const Tree &tree, uint root_id, const FastTrimesh &subm, const genericPoint *p)
{
    const Node &root = tree.getNode(root_id);
    assert(genericPoint::pointInTriangle(*p, *subm.vert(root.v0), *subm.vert(root.v1), *subm.vert(root.v2)) && "the leaf does not contain the point");

    if(root.children[0] == -1) return root;

    // check its children
    for(uint i = 0; i < 3; i++) // max 3 children
    {
        int c = root.children[i];

        if(c != -1)
        {
            const Node &n = tree.getNode(static_cast<uint>(c));

            if(genericPoint::pointInTriangle(*p, *subm.vert(n.v0), *subm.vert(n.v1), *subm.vert(n.v2)))
                return innerFindContainingTriangleInTree(tree, static_cast<uint>(c), subm, p);
        }
    }

    assert(false && "no containing triangle found");
    return root; // warning killer
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void splitSingleEdge(const TriangleSoup &ts, FastTrimesh &subm, uint v0_id, uint v1_id, std::vector<uint> &points)
{
    if(points.empty()) return;

    int e_id = subm.edgeID(v0_id, v1_id);
    assert(e_id >= 0);

    // vertices in mesh
    for(uint p_pos = 1; p_pos < points.size()-1; p_pos++)
    {
        uint p_id = points[p_pos];
        uint v_pos = subm.addVert(ts.vert(p_id), p_id);
        points[p_pos] = v_pos;
    }

    points[0] = v0_id;
    points[points.size() - 1] = v1_id;

    // make new_triangles
    for(auto i = points.begin(), j = i+1; j < points.end(); ++i, ++j)
    {
        for(uint t_id : subm.adjE2T(static_cast<uint>(e_id)))
        {
            uint opp = subm.triVertOppositeTo(t_id, v0_id, v1_id);

            if(subm.triVertsAreCCW(t_id, v0_id, v1_id))
                subm.addTri(*j, *i, opp);
            else
                subm.addTri(*i, *j, opp);
        }
    }

    // remove the original edge and the tris attached to it
    subm.removeEdge(static_cast<uint>(e_id));
}

/*inline void splitSingleEdge(const TriangleSoup& ts, FastTrimesh& subm, uint v0_id, uint v1_id, auxvector<uint>& points)
{
    if(points.empty()) return;

    int e_id = subm.edgeID(v0_id, v1_id);
    assert(e_id >= 0);

    // new_vertices in mesh
    for(uint p_pos = 1; p_pos < points.size()-1; p_pos++)
    {
        uint p_id = points[p_pos];
        uint v_pos = subm.addVert(ts.vert(p_id), p_id);
        points[p_pos] = v_pos;
    }

    points[0] = v0_id;
    points[points.size() - 1] = v1_id;

    // make new_triangles
    for(auto i = points.begin(), j = i+1; j < points.end(); ++i, ++j)
    {
        for(uint t_id : subm.adjE2T(static_cast<uint>(e_id)))
        {
            uint opp = subm.triVertOppositeTo(t_id, v0_id, v1_id);

            if(subm.triVertsAreCCW(t_id, v0_id, v1_id))
                subm.addTri(*j, *i, opp);
            else
                subm.addTri(*i, *j, opp);
        }
    }

    // remove the original edge and the tris attached to it
    subm.removeEdge(static_cast<uint>(e_id));
}*/


//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void addConstraintSegmentsInSingleTriangle(TriangleSoup &ts, point_arena& arena, FastTrimesh &subm, AuxiliaryStructure &g, auxvector<UIPair> &segment_list, std::mutex& mutex)
{
    int orientation = subm.triOrientation(0);

    phmap::flat_hash_map< UIPair, UIPair > sub_segs_map;
    sub_segs_map.reserve(segment_list.size());

    while(segment_list.size() > 0)
    {
        UIPair seg = segment_list.back();
        segment_list.pop_back();

        uint v0_id = subm.vertNewID(seg.first);
        uint v1_id = subm.vertNewID(seg.second);

        addConstraintSegment(ts, arena, subm, v0_id, v1_id, orientation, g, segment_list, sub_segs_map, mutex);
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void addConstraintSegment(TriangleSoup &ts, point_arena& arena, FastTrimesh &subm, uint v0_id, uint v1_id, const int orientation,
                                 AuxiliaryStructure &g, auxvector<UIPair> &segment_list, phmap::flat_hash_map< UIPair, UIPair > &sub_segs_map, std::mutex& mutex)
{
    int e_id = subm.edgeID(v0_id, v1_id);

    if(e_id != -1) // edge already present in the mesh, just flag it as constraint
    {
        subm.setEdgeConstr(static_cast<uint>(e_id));
        return;
    }

    // for efficiency, it's better to start from the vert with lowest valence
    uint v_start = (subm.vertValence(v0_id) < subm.vertValence(v1_id)) ? v0_id : v1_id;
    uint v_stop  = (v_start == v0_id) ? v1_id : v0_id;

    auxvector<uint> intersected_edges;
    auxvector<uint> intersected_tris;

    findIntersectingElements(ts, arena, subm, v_start, v_stop, intersected_edges, intersected_tris, g, segment_list, sub_segs_map, mutex);

    if(intersected_edges.size() == 0) return;

    // walk along the border
    std::vector<uint> h0, h1;
    boundaryWalker(subm, v_start, v_stop,  intersected_tris.begin(),  intersected_edges.begin(),  h0);
    boundaryWalker(subm, v_stop,  v_start, intersected_tris.rbegin(), intersected_edges.rbegin(), h1);

    assert(h0.size() >= 3);
    assert(h1.size() >= 3);

    std::vector<uint> new_tris;
    earcutLinear(subm, h0, new_tris, orientation);
    earcutLinear(subm, h1, new_tris, orientation);

    for(std::vector<uint>::iterator i = new_tris.begin(); i < new_tris.end(); i+=3)
    {
        subm.addTri(*i, *(i+1), *(i+2));
    }

    subm.removeTris(intersected_tris);
    e_id = subm.edgeID(v_start, v_stop);
    assert(e_id != -1);
    subm.setEdgeConstr(static_cast<uint>(e_id)); // edge marked as constr
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void findIntersectingElements(TriangleSoup &ts, point_arena& arena, FastTrimesh &subm, uint v_start, uint v_stop, auxvector<uint> &intersected_edges, auxvector<uint> &intersected_tris,
                                     AuxiliaryStructure &g, auxvector<UIPair> &segment_list, phmap::flat_hash_map< UIPair, UIPair > &sub_seg_map, std::mutex& mutex)
{
    uint orig_vstart = subm.vertOrigID(v_start);
    uint orig_vstop  = subm.vertOrigID(v_stop);

    // find the edge in link(seed) that intersect {A,B}
    for(uint t_id : subm.adjV2T(v_start))
    {
        uint e_id = subm.edgeOppToVert(t_id, v_start);
        uint ev0_id = subm.edgeVertID(e_id, 0);
        uint ev1_id = subm.edgeVertID(e_id, 1);
        assert((ev0_id != v_stop && ev1_id != v_stop) && "v0_id or v1_id == v_stop");

        if(segmentsIntersectInside(subm, v_start, v_stop, ev0_id, ev1_id))
        {
            intersected_edges.push_back(e_id);
            intersected_tris.push_back(t_id);
            break;
        }

        else if(pointInsideSegment(subm, v_start, v_stop, ev0_id))
        {
            // the original edge (v_start, v_stop) is split in (v_start-v0) - (v0-v_stop) and put in the segment_list to check later
            uint orig_v0 = subm.vertOrigID(ev0_id);
            int edge_id  = subm.edgeID(v_start, ev0_id);
            assert(edge_id != -1);

            subm.setEdgeConstr(static_cast<uint>(edge_id)); // edge marked as constr

            segment_list.push_back(std::make_pair(orig_v0, orig_vstop));
            intersected_edges.clear();

            splitSegmentInSubSegments(orig_vstart, orig_vstop, orig_v0, sub_seg_map);

            return;
        }

        else if(pointInsideSegment(subm, v_start, v_stop, ev1_id))
        {
            // the original edge (v_start, v_stop) is split in (v_start-v1) - (v1-v_stop) and put in the segment_list to check later
            uint orig_v1 = subm.vertOrigID(ev1_id);
            int edge_id  = subm.edgeID(v_start, ev1_id);
            assert(edge_id != -1);

            subm.setEdgeConstr(static_cast<uint>(edge_id)); // edge marked as constr

            segment_list.push_back(std::make_pair(orig_v1, orig_vstop));
            intersected_edges.clear();

            splitSegmentInSubSegments(orig_vstart, orig_vstop, orig_v1, sub_seg_map);

            return;
        }
    }

    assert(intersected_edges.size() > 0);

    // walk along the topology to find the sorted list of edges and tris that intersect {v_start, v_stop}
    while(true)
    {
        uint e_id   = intersected_edges.back();
        uint ev0_id = subm.edgeVertID(e_id, 0);
        uint ev1_id = subm.edgeVertID(e_id, 1);

        if(!subm.edgeIsConstr(e_id)) // no-constraint edge
        {
            int t_id = subm.triOppToEdge(e_id, intersected_tris.back());
            assert(t_id >= 0);
            uint v2  = subm.triVertOppositeTo(static_cast<uint>(t_id), ev0_id, ev1_id);

            if(segmentsIntersectInside(subm, v_start, v_stop, ev0_id, v2))
            {
                int int_edge = subm.edgeID(ev0_id, v2);
                assert(int_edge >= 0);

                intersected_edges.push_back(static_cast<uint>(int_edge));
                intersected_tris.push_back(static_cast<uint>(t_id));
            }
            else if(segmentsIntersectInside(subm, v_start, v_stop, ev1_id, v2))
            {
                int int_edge = subm.edgeID(ev1_id, v2);
                assert(int_edge >= 0);

                intersected_edges.push_back(static_cast<uint>(int_edge));
                intersected_tris.push_back(static_cast<uint>(t_id));
            }
            else if(v2 != v_stop)
            {
                assert(pointInsideSegment(subm, v_start, v_stop, v2));

                // the original edge (v_start, v_stop) is split in (v_start-v2) - (v2-v_stop) and put in the segment_list to check later
                uint orig_v2 = subm.vertOrigID(v2);

                segment_list.push_back(std::make_pair(orig_vstart, orig_v2));
                segment_list.push_back(std::make_pair(orig_v2, orig_vstop));
                intersected_edges.clear();

                splitSegmentInSubSegments(orig_vstart, orig_vstop, orig_v2, sub_seg_map);

                return;
            }

            else
            {
                break; // converged (v2 == v_stop)
            }

        }

            // e_id is a constraint edge already present in the triangulation
        else
        {
            // TPI creation (if it doesn't exist)
            uint orig_v0 = subm.vertOrigID(ev0_id);
            uint orig_v1 = subm.vertOrigID(ev1_id);
            uint orig_tpi_id;

#pragma omp critical
            { // start critical section...
                std::lock_guard<std::mutex> lock(mutex);
                orig_tpi_id = createTPI(ts, arena, subm, std::make_pair(orig_vstart, orig_vstop), std::make_pair(orig_v0, orig_v1), g, sub_seg_map);
            } // end critical section

            //adding the TPI in the new_mesh
            uint new_tpi_id = subm.addVert(ts.vert(orig_tpi_id), orig_tpi_id);
            subm.splitEdge(e_id, new_tpi_id);

            int edge0_id = subm.edgeID(ev0_id, new_tpi_id);       assert(edge0_id != -1);
            int edge1_id = subm.edgeID(new_tpi_id, ev1_id);       assert(edge1_id != -1);

            subm.setEdgeConstr(static_cast<uint>(edge0_id));
            subm.setEdgeConstr(static_cast<uint>(edge1_id));

            // the original edge (v_start, v_stop) is split in (v_start-tpi) - (tpi-v_stop) and put in the segment_list to check later
            segment_list.push_back(std::make_pair(orig_vstart, orig_tpi_id));
            segment_list.push_back(std::make_pair(orig_tpi_id, orig_vstop));
            intersected_edges.clear();

            splitSegmentInSubSegments(orig_vstart, orig_vstop, orig_tpi_id, sub_seg_map);
            splitSegmentInSubSegments(orig_v0, orig_v1, orig_tpi_id, sub_seg_map);

            return;
        }

    }

    // append the last triangle
    uint e_id = intersected_edges.back();
    int t_id = subm.triOppToEdge(e_id, intersected_tris.back());
    assert(t_id != -1 && "tri opposite to edge not found");

    intersected_tris.push_back(static_cast<uint>(t_id));

    assert(subm.triContainsVert(static_cast<uint>(t_id), v_start) ||
           subm.triContainsVert(static_cast<uint>(t_id), v_stop));

}


//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<typename iterator>
inline void boundaryWalker(const FastTrimesh &subm, uint v_start, uint v_stop, iterator curr_p, iterator curr_e, std::vector<uint> &h)
{
    h.clear();
    h.push_back(v_start);

    do
    {
        uint curr_v = h.back();
        uint off    = subm.triVertOffset(*curr_p, curr_v);
        uint next_v = subm.triVertID(*curr_p, (off +1) % 3);

        while(subm.edgeID(curr_v, next_v) == static_cast<int>(*curr_e))
        {
            ++curr_p;
            if(subm.triContainsVert(*curr_p, v_stop))
            {
                h.push_back(v_stop);
                return;
            }

            ++curr_e;

            assert(*curr_p < subm.numTris());
            assert(*curr_e < subm.numEdges());

            off    = subm.triVertOffset(*curr_p, curr_v);
            next_v = subm.triVertID(*curr_p, (off +1) %3);
            assert(subm.edgeID(curr_v, next_v) != -1);
        }

        h.push_back(next_v);
        ++curr_p;

        if(subm.triContainsVert(*curr_p, v_stop))
        {
            h.push_back(v_stop);
            return;
        }

        ++curr_e;
    }
    while(h.back()!=v_stop);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void earcut(const FastTrimesh &subm, std::vector<uint> &poly, std::vector<uint> &tris, const Plane &ref_p, const int &orientation)
{
    if(poly.size() < 3) return;

    if(poly.size() == 3)
    {
        for(uint v_id : poly) tris.push_back(v_id);
        return;
    }

    while(true)
    {
        if(poly.size() == 3)
        {
            for(uint v_id : poly)
                tris.push_back(v_id);
            return;
        }

        for(uint i = 1; i < poly.size()-1; ++i)
        {
            uint curr = poly.at(i);
            uint next = poly.at(i + 1);
            uint prev = poly.at(i - 1);

            if(prev == next) continue; // dangling edge: not even do the ear test...

            const genericPoint *prev_p = subm.vert(prev);
            const genericPoint *curr_p = subm.vert(curr);
            const genericPoint *next_p = subm.vert(next);

            //auto t2 = tic();
            int check = customOrient2D(prev_p, curr_p, next_p, ref_p);
            //time_ORIENT += toc(t2);

            if( (check > 0 && orientation > 0) || (check < 0 && orientation < 0)) // found a candidate ear
            {
                tris.push_back(prev);
                tris.push_back(curr);
                tris.push_back(next);

                // remove curr vert
                poly.erase(poly.begin() + i);

                if(poly.size() == 3)
                {
                    tris.push_back(poly.at(0));
                    tris.push_back(poly.at(1));
                    tris.push_back(poly.at(2));
                    return;
                }
                i--;
            }
        }
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void earcutLinear(const FastTrimesh &subm, const std::vector<uint> &poly, std::vector<uint> &tris, const int &orientation)
{
    assert(poly.size() >= 3 && "no valid poly dimension");

    // doubly linked list for fat polygon inspection
    uint size = static_cast<uint>(poly.size());
    std::vector<uint> prev(size);
    std::vector<uint> next(size);
    std::iota(prev.begin(), prev.end(),-1);
    std::iota(next.begin(), next.end(), 1);
    prev.front() = size-1;
    next.back()  = 0;

    // keep a list of the ears to be cut
    std::vector<uint> ears;
    ears.reserve(size);

    // this always has size |poly|, and keeps track of ears
    // (corners that were not ears at the beginning may become so later on)
    std::vector<bool> is_ear(size, false);

    // detect all safe ears in O(n).
    // This amounts to finding all convex vertices but the endpoints of the constrained edge
    for(uint curr = 1; curr < size-1; ++curr)
    {
        // NOTE: the polygon may contain danging edges, prev!=next
        // avoids to even do the ear test for them

        const genericPoint *p0 = subm.vert(poly[prev[curr]]);
        const genericPoint *p1 = subm.vert(poly[curr]);
        const genericPoint *p2 = subm.vert(poly[next[curr]]);

        int check = customOrient2D(p0, p1, p2, subm.refPlane());

        if( (prev != next) && ((check > 0 && orientation > 0) || (check < 0 && orientation < 0)) )
        {
            ears.emplace_back(curr);
            is_ear.at(curr) = true;
        }
    }

    // progressively delete all ears, also updating the data structure
    uint length = size;
    while(true)
    {
        uint curr = ears.back();
        ears.pop_back();

        // make new_tri
        tris.push_back(poly[prev[curr]]);
        tris.push_back(poly[curr]);
        tris.push_back(poly[next[curr]]);

        // exclude curr from the polygon, connecting prev and next
        next[prev[curr]] = next[curr];
        prev[next[curr]] = prev[curr];

        // last triangle?
        if(--length < 3) return;

        // check if prev and next have become new_ears
        if(!is_ear[prev[curr]] && prev[curr] != 0)
        {
            const genericPoint *p0 = subm.vert(poly[prev[prev[curr]]]);
            const genericPoint *p1 = subm.vert(poly[prev[curr]]);
            const genericPoint *p2 = subm.vert(poly[next[curr]]);

            int check = customOrient2D(p0, p1, p2, subm.refPlane());

            if( (prev[prev[curr]] != next[curr]) && ((check > 0 && orientation > 0) || (check < 0 && orientation < 0)))
            {
                ears.emplace_back(prev[curr]);
                is_ear.at(prev[curr]) = true;
            }
        }

        if(!is_ear[next[curr]] && next[curr] < size-1)
        {
            const genericPoint *p0 = subm.vert(poly[prev[curr]]);
            const genericPoint *p1 = subm.vert(poly[next[curr]]);
            const genericPoint *p2 = subm.vert(poly[next[next[curr]]]);

            int check = customOrient2D(p0, p1, p2, subm.refPlane());

            if( (next[next[curr]] != prev[curr]) && ((check > 0 && orientation > 0) || (check < 0 && orientation < 0)))
            {
                ears.emplace_back(next[curr]);
                is_ear.at(next[curr]) = true;
            }
        }
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline uint createTPI(TriangleSoup &ts, point_arena& arena, FastTrimesh &subm, const UIPair &e0, const UIPair &e1, AuxiliaryStructure &g, const phmap::flat_hash_map< UIPair, UIPair > &sub_segs_map)
{
    std::vector<uint> t0_ids = {subm.vertOrigID(0), subm.vertOrigID(1), subm.vertOrigID(2)};

    std::vector<const genericPoint*> tv = {ts.vert(t0_ids[0]), ts.vert(t0_ids[1]), ts.vert(t0_ids[2])};
    std::vector<const genericPoint*> tv0 = computeTriangleOfSegment(ts, e0, t0_ids, g, sub_segs_map);
    std::vector<const genericPoint*> tv1 = computeTriangleOfSegment(ts, e1, t0_ids, g, sub_segs_map);

    implicitPoint3D_TPI *new_v = &arena.tpi.emplace_back(tv[0]->toExplicit3D(), tv[1]->toExplicit3D(), tv[2]->toExplicit3D(),
                                                         tv0[0]->toExplicit3D(), tv0[1]->toExplicit3D(), tv0[2]->toExplicit3D(),
                                                         tv1[0]->toExplicit3D(), tv1[1]->toExplicit3D(), tv1[2]->toExplicit3D());


    // we check if the new_tpi as already been inserted
    //std::pair<uint, bool> ins = g.addVertexInSortedList(std::make_pair(new_v, ts.numVerts()));
    std::pair<uint, bool> ins = g.addVertexInSortedList(new_v, ts.numVerts());

    if(ins.second == false) //vtx already present
    {
        arena.tpi.pop_back();
        return ins.first;
    }

    // vertex not present, we add the new_TPI in the original mesh
    double x = 0., y = 0., z = 0.;
    assert(new_v->getApproxXYZCoordinates(x, y, z) && "TPI point badly formed");

    uint v_id = ts.addImplVert(new_v);

    return v_id;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline std::vector<const genericPoint *> computeTriangleOfSegment(const TriangleSoup &ts, const UIPair &seg, std::vector<uint> &ref_t,
                                                                  const AuxiliaryStructure &g, const phmap::flat_hash_map< UIPair, UIPair > &sub_segs_map)
{
    const auxvector<uint> &e_tris = segmentTrianglesList(seg, sub_segs_map, g);

    // Looking for a no-coplanar triangle
    for(auto &t1 : e_tris)
    {
        std::vector<uint> tv1 = {ts.triVertID(t1, 0), ts.triVertID(t1, 1), ts.triVertID(t1, 2)};
        if(vectorsAreEqual(tv1, ref_t)) continue;

        bool copl = true;

        if(genericPoint::orient3D(*ts.vert(ref_t[0]), *ts.vert(ref_t[1]), *ts.vert(ref_t[2]), *ts.vert(tv1[0])) != 0.0)
            copl = false;
        else if(genericPoint::orient3D(*ts.vert(ref_t[0]), *ts.vert(ref_t[1]), *ts.vert(ref_t[2]), *ts.vert(tv1[1])) != 0.0)
            copl = false;
        else if(genericPoint::orient3D(*ts.vert(ref_t[0]), *ts.vert(ref_t[1]), *ts.vert(ref_t[2]), *ts.vert(tv1[2])) != 0.0)
            copl = false;

        // no-coplanar triangle found
        if(copl == false)
            return {ts.vert(tv1[0]), ts.vert(tv1[1]), ts.vert(tv1[2])};
    }

    // no-coplanar triangle NOT found (jolly point required)
    return computeTriangleOfSegmentInCoplanarCase(ts, seg, e_tris, ref_t);

    assert(false && "no triangle found for TPI creation");
    return {nullptr, nullptr, nullptr}; // warning killer
}


//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline std::vector<const genericPoint *> computeTriangleOfSegmentInCoplanarCase(const TriangleSoup &ts, const UIPair &seg, const auxvector<uint> &tris, const std::vector<uint> &ref_t)
{
    std::vector<const genericPoint *> res;
    uint e0 = seg.first, e1 = seg.second;

    if(ts.vert(e0)->isExplicit3D() && ts.vert(e1)->isExplicit3D())
    {
        res.push_back(ts.vert(e0));
        res.push_back(ts.vert(e1));
    }
    else
    {
        for(auto &t : tris)
        {
            const std::vector<uint> &tv = {ts.triVertID(t, 0), ts.triVertID(t, 1), ts.triVertID(t, 2)};

            //edge 0 test of t
            if(genericPoint::pointInSegment(*ts.vert(e0), *ts.vert(tv[0]), *ts.vert(tv[1])) &&
               genericPoint::pointInSegment(*ts.vert(e1), *ts.vert(tv[0]), *ts.vert(tv[1])))
            {
                res.push_back(ts.vert(tv[0]));
                res.push_back(ts.vert(tv[1]));
                break;
            }
            //edge 1 of t
            if(genericPoint::pointInSegment(*ts.vert(e0), *ts.vert(tv[1]), *ts.vert(tv[2])) &&
               genericPoint::pointInSegment(*ts.vert(e1), *ts.vert(tv[1]), *ts.vert(tv[2])))
            {
                res.push_back(ts.vert(tv[1]));
                res.push_back(ts.vert(tv[2]));
                break;
            }
            //edge 2 of t
            if(genericPoint::pointInSegment(*ts.vert(e0), *ts.vert(tv[2]), *ts.vert(tv[0])) &&
               genericPoint::pointInSegment(*ts.vert(e1), *ts.vert(tv[2]), *ts.vert(tv[0])))
            {
                res.push_back(ts.vert(tv[2]));
                res.push_back(ts.vert(tv[0]));
                break;
            }
        }
    }

    assert(res.size() == 2 && "No edge found containing the given endpoints");
    assert((res[0]->isExplicit3D() && res[1]->isExplicit3D()) && "Impossible triangle");

    // check for the 3rd point of the triangle
    for(uint jp_id = 0; jp_id < 4; jp_id++)
    {
        if(genericPoint::orient3D(*ts.vert(ref_t[0]), *ts.vert(ref_t[1]), *ts.vert(ref_t[2]), *ts.jollyPoint(jp_id)) != 0.0)
        {
            res.push_back(ts.jollyPoint(jp_id));
            return res;
        }
    }

    assert(false && "No no-coplanar vtx found to create a triangle");
    return res;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline bool vectorsAreEqual(std::vector<uint> &v0, std::vector<uint> &v1)
{
    assert(v0.size() == v1.size());

    std::sort(v0.begin(), v0.end());
    std::sort(v1.begin(), v1.end());

    for(uint i = 0; i < v0.size(); i++)
        if(v0[i] != v1[i]) return false;

    return true;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline bool fastPointOnLine(const FastTrimesh &subm, uint e_id, uint p_id)
{
    uint ev0_id = subm.edgeVertID(e_id, 0);
    uint ev1_id = subm.edgeVertID(e_id, 1);

    switch (subm.refPlane())
    {
        case XY: return (genericPoint::orient2Dxy(*subm.vert(ev0_id), *subm.vert(ev1_id), *subm.vert(p_id)) == 0);
        case YZ: return (genericPoint::orient2Dyz(*subm.vert(ev0_id), *subm.vert(ev1_id), *subm.vert(p_id)) == 0);
        case ZX: return (genericPoint::orient2Dzx(*subm.vert(ev0_id), *subm.vert(ev1_id), *subm.vert(p_id)) == 0);
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

// check whether edges {e00,e01} and {e10,e11} intersect
// at a point that is inside both segments
inline bool segmentsIntersectInside(const FastTrimesh &subm, uint e00_id, uint e01_id, uint e10_id, uint e11_id)
{
    return genericPoint::innerSegmentsCross(*subm.vert(e00_id), *subm.vert(e01_id),
                                            *subm.vert(e10_id), *subm.vert(e11_id));
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline bool pointInsideSegment(const FastTrimesh &subm, uint ev0_id, uint ev1_id, uint p_id)
{
    return genericPoint::pointInInnerSegment(*subm.vert(p_id), *subm.vert(ev0_id), *subm.vert(ev1_id));
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void splitSegmentInSubSegments(uint v_start, uint v_stop, uint mid_point, phmap::flat_hash_map< UIPair, UIPair > &sub_segments_map)
{
    UIPair orig_seg, sub_seg0, sub_seg1;
    (v_start < v_stop) ? orig_seg = std::make_pair(v_start, v_stop) : orig_seg = std::make_pair(v_stop, v_start);
    (v_start < mid_point) ? sub_seg0 = std::make_pair(v_start, mid_point) : sub_seg0 = std::make_pair(mid_point, v_start);
    (v_stop < mid_point) ? sub_seg1 = std::make_pair(v_stop, mid_point) : sub_seg1 = std::make_pair(mid_point, v_stop);

    auto it = sub_segments_map.find(orig_seg);

    if(it == sub_segments_map.end()) // orig_seg is a segment in the original map
    {
        sub_segments_map[sub_seg0] = orig_seg;
        sub_segments_map[sub_seg1] = orig_seg;
    }
    else // orig_seg is an already split segment (it must be already present in the sub_split_map)
    {
        UIPair ref_seg = it->second;
        sub_segments_map[sub_seg0] = ref_seg;
        sub_segments_map[sub_seg1] = ref_seg;
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline const auxvector<uint> &segmentTrianglesList(const UIPair &seg, const phmap::flat_hash_map< UIPair, UIPair > &sub_segments_map, const AuxiliaryStructure &g)
{
    UIPair key_seg;
    (seg.first < seg.second) ? key_seg = seg : key_seg = std::make_pair(seg.second, seg.first);

    auto it = sub_segments_map.find(key_seg);

    if(it != sub_segments_map.end()) // the segment is a sub segment
        return g.segmentTrianglesList(it->second);

    else // the segment is an original segment
        return g.segmentTrianglesList(key_seg);

}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void solvePocketsInCoplanarTriangle(const FastTrimesh &subm, AuxiliaryStructure &g, std::vector<uint> &new_tris,
                                           std::vector< std::bitset<NBIT> > &new_labels, const std::bitset<NBIT> &label)
{
    std::vector< std::vector<uint> > tri_pockets;
    std::vector< std::set<uint> > polygons;

    findPocketsInTriangle(subm, tri_pockets, polygons);
    assert(tri_pockets.size() == polygons.size());

    std::vector<uint> curr_p; // TODO: can be moved out
    for(uint p_id = 0; p_id < polygons.size(); p_id++)
    {
        curr_p.clear();
        for(auto &p : polygons[p_id]) // conversion from new_to original vertices ids
            curr_p.push_back(subm.vertOrigID(p));
        remove_duplicates(curr_p);

        int pos = g.addVisitedPolygonPocket(curr_p, static_cast<uint>(new_labels.size()));

        if(pos == -1) //pocket not added yet
        {
            const std::vector<uint> &tri_list = tri_pockets[p_id];
            for(auto &t : tri_list)
            {
                const uint *tri = subm.tri(t);
                new_tris.push_back(subm.vertOrigID(tri[0]));
                new_tris.push_back(subm.vertOrigID(tri[1]));
                new_tris.push_back(subm.vertOrigID(tri[2]));
                new_labels.push_back(label);
            }
        }
        else //pocket already present
        {
            uint num_tris = static_cast<uint>(curr_p.size() - 2);

            for(uint i = 0; i < num_tris; i++)
                new_labels[static_cast<uint>(pos) + i] |= label;
        }
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void findPocketsInTriangle(const FastTrimesh &subm, std::vector< std::vector<uint> > &tri_pockets, std::vector< std::set<uint> > &tri_polygons)
{
    std::vector<bool> visited(subm.numTris(), false);

    for(uint t_seed = 0; t_seed < subm.numTris(); t_seed++)
    {
        if(!visited[t_seed])
        {
            std::vector<uint> curr_tri_pocket;
            std::set<uint> curr_tri_poly;
            std::stack<uint> stack;
            stack.push(t_seed);

            while(!stack.empty())
            {
                uint curr_t = stack.top();
                stack.pop();

                if(!visited[curr_t])
                {
                    visited[curr_t] = true;
                    curr_tri_pocket.push_back(curr_t);

                    fmvector<uint> t2e = subm.adjT2E(curr_t);

                    for(uint e : t2e)
                    {
                        if(subm.edgeIsConstr(e) || subm.edgeIsBoundary(e))
                        {
                            curr_tri_poly.insert(subm.edgeVertID(e, 0));
                            curr_tri_poly.insert(subm.edgeVertID(e, 1));
                        }
                        else
                        {
                            for(uint t : subm.adjE2T(e))
                            {
                                if(t != curr_t) stack.push(t);
                            }
                        }
                    }
                }
            }

            tri_pockets.push_back(curr_tri_pocket);
            tri_polygons.push_back(curr_tri_poly);
        }
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline int customOrient2D(const genericPoint *p0, const genericPoint *p1, const genericPoint *p2, const Plane &ref_p)
{
    switch (ref_p)
    {
        case XY: return genericPoint::orient2Dxy(*p0, *p1, *p2);
        case YZ: return genericPoint::orient2Dyz(*p0, *p1, *p2);
        case ZX: return genericPoint::orient2Dzx(*p0, *p1, *p2);
    }

    return 0; // warning killer
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void sortedVertexListAlongSegment(const TriangleSoup &ts, const auxvector<uint> &point_list,
                                         uint v0_id, uint v1_id, auxvector<uint> &out_point_list)
{
    if(point_list.size() == 0) return;

    auxvector< std::pair<const genericPoint*, uint> > sorted;
    sorted.reserve(point_list.size() + 2);

    sorted.push_back(std::make_pair(ts.vert(v0_id), v0_id));
    sorted.push_back(std::make_pair(ts.vert(v1_id), v1_id));

    for(uint v_id : point_list)
    {
        sorted.push_back(std::make_pair(ts.vert(v_id), v_id));
    }

    std::sort(sorted.begin(), sorted.end(), lessThanP);

    out_point_list.reserve(sorted.size());

    if(sorted[0].second == v0_id) // first element must be v0
    {
        for(auto it = sorted.begin(); it != sorted.end(); ++it)
            out_point_list.push_back(it->second);
    }
    else
    {
        for(auto it = sorted.rbegin(); it != sorted.rend(); ++it)
            out_point_list.push_back(it->second);
    }

    assert(out_point_list[0] == v0_id && out_point_list.back() == v1_id && "Sorted list not correct");
}
