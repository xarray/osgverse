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

#ifndef FOCTREE_H
#define FOCTREE_H

#include <cinolib/geometry/spatial_data_structure_item.h>
#include <cinolib/meshes/meshes.h>
#include <queue>
#include <vector>
#include <mutex>
//#include <absl/container/inlined_vector.h>

template<typename T>
//using onvector = absl::InlinedVector<T, 16>;
using onvector = std::vector<T>;

namespace cinolib
{

class FOctreeNode
{
    public:
        FOctreeNode(const AABB & bbox) : bbox(bbox) {}
        AABB              bbox;
        int               start = 0;
        onvector<uint>    item_indices; // index FOctree::items, avoiding to store a copy of the same object multiple times in each node it appears
        bool              is_inner = false;
};
// due to the bool flag this struct requires 7 bytes padding. Also the
// pointer to the father is not necessary and could be removed, shrinking it.
// It could be designed better....
// https://stackoverflow.com/questions/4306186/structure-padding-and-packing
// http://www.catb.org/esr/structure-packing/

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

/* Usage:
 *
 *  i)   Create an empty octree
 *  ii)  Use the push_segment/triangle/tetrahedron facilities to populate it
 *  iii) Call build to make the tree
*/

class FOctree
{
    public:

        explicit FOctree(const uint max_depth      = 7,
                        const uint items_per_leaf = 50);

        virtual ~FOctree();

        //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

        void build(uint max_depth, uint items_per_leaf, bool parallel);
        void build_recursive(uint max_depth, uint items_per_leaf, int node_id, int depth, std::mutex& mutex, ::TaskGroup& group);

        //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

        void subdivide(int node_id, std::mutex& mutex);

        //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

        void build_from_vectors(const std::vector<vec3d> & verts,
                                const std::vector<uint>  & tris,
                                uint max_depth, uint items_per_leaf,
                                bool parallel)
        {
            assert(items.empty());
            items.reserve(tris.size()/3);
            for(uint i=0; i<tris.size(); i+=3)
            {
                items.emplace_back(i/3, verts.at(tris[i  ]),
                                    verts.at(tris[i+1]),
                                    verts.at(tris[i+2]));
            }
            build(max_depth, items_per_leaf, parallel);
        }

        std::vector<int> get_leaves() const;

        // all items live here, and leaf nodes only store indices to items
        std::vector<Triangle> items;
        std::vector<FOctreeNode> nodes;

        /*bool intersects_triangle(const vec3d   t1[],
                                 const vec3d   t2[],
                                 const bool ignore_if_valid_complex,
                                 double * min = NULL,
                                 double * perm = NULL,
                                 double * t_min = NULL,
                                 double * t_perm = NULL) const;*/

        //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

        protected:

        uint max_depth;      // maximum allowed depth of the tree
        uint items_per_leaf; // prescribed number of items per leaf (can't go deeper than max_depth anyways)
};

}

//#include "foctree.cpp"

#endif
