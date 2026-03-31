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

#include <cinolib/parallel_for.h>
#include <cinolib/geometry/segment.h>
#include <cinolib/geometry/triangle.h>
#include <stack>

#include <thread>
#include "fake_tbb.h"
#include "foctree.h"
//#include <tbb/tbb.h>

namespace cinolib
{

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

CINO_INLINE
FOctree::FOctree(const uint max_depth,
               const uint items_per_leaf)
: max_depth(max_depth)
, items_per_leaf(items_per_leaf)
{}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

CINO_INLINE
FOctree::~FOctree()
{
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

CINO_INLINE
std::vector<int> FOctree::get_leaves() const {
    auto leaves = std::vector<int>();
    leaves.reserve(nodes.size());
    for(auto node_id = 0; node_id < (int)nodes.size(); node_id++)
        if(!nodes[node_id].is_inner) leaves.push_back(node_id);
    return leaves;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

CINO_INLINE
void FOctree::build_recursive(uint max_depth, uint items_per_leaf, int node_id, int depth, std::mutex& mutex, ::TaskGroup& group)
{
    subdivide(node_id, mutex);

    auto node = &nodes[node_id]; 
    for(int j=0; j<8; ++j)
    {
        auto child_id = node->start + j;
        auto child = &nodes[node->start + j];
        if(depth<max_depth && child->item_indices.size()>items_per_leaf)
        {
            group.run([=,&mutex,&group]{ this->build_recursive(max_depth, items_per_leaf, child_id, depth+1, mutex, group); });
        }
    }
}

CINO_INLINE
void FOctree::build(uint max_depth, uint items_per_leaf, bool parallel)
{
    this->max_depth = max_depth;
    this->items_per_leaf = items_per_leaf;

    if(items.empty()) return;

    // HACK
    nodes.reserve(items.size());

    // initialize root with all items, also updating its AABB
    nodes.emplace_back(AABB());
    auto root = &nodes.back();
    root->item_indices.resize(items.size());
    std::iota(root->item_indices.begin(),root->item_indices.end(),0);
    for(auto& it : items) root->bbox.push(it.aabb);

    root->bbox.scale(1.5); // enlarge bbox to account for queries outside legal area.
                           // this should disappear eventually....

    if(parallel) {
        if(root->item_indices.size()<items_per_leaf || max_depth==1) return;
        if(max_depth == 2) {
            std::mutex mutex;
            subdivide(0, mutex);
        } else {
            std::mutex mutex;
            ::TaskGroup group;
            build_recursive(max_depth, items_per_leaf, 0, 1, mutex, group);
            group.wait();
        }
    } else {
        std::mutex mutex;
        if(root->item_indices.size()<items_per_leaf || max_depth==1)
        {
        }
        else
        {
            subdivide(0, mutex);

            if(max_depth==2)
            {
            }
            else
            {
                std::queue<std::pair<int,uint>> splitlist[8]; // (node, depth)
                for(int i=0; i<8; ++i)
                {
                    auto child_id = root->start + i;
                    auto child = &nodes[root->start + i];
                    if(child->item_indices.size()>items_per_leaf)
                    {
                        splitlist[i].push(std::make_pair(child_id,2));
                    }
                }

                ::parallel_for((uint)0, (uint)8, [&](uint i)
                {
                    while(!splitlist[i].empty())
                    {
                        auto pair  = splitlist[i].front();
                        auto node_id  = pair.first;
                        auto node  = &nodes[pair.first];
                        uint depth = pair.second + 1;
                        splitlist[i].pop();

                        subdivide(node_id, mutex);

                        for(int j=0; j<8; ++j)
                        {
                            auto child_id = node->start + j;
                            auto child = &nodes[node->start + j];
                            if(depth<max_depth && child->item_indices.size()>items_per_leaf)
                            {
                                splitlist[i].push(std::make_pair(child_id, depth));
                            }
                        }
                    }
                });
            }
        }
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

CINO_INLINE
void FOctree::subdivide(int node_id, std::mutex& mutex)
{
    // create children octants
    auto node = &nodes[node_id];
    vec3d min = node->bbox.min;
    vec3d max = node->bbox.max;
    vec3d avg = node->bbox.center();
    {
        std::lock_guard<std::mutex> lock(mutex);
        node->start = (int)nodes.size();
        nodes.emplace_back(AABB(vec3d(min[0], min[1], min[2]), vec3d(avg[0], avg[1], avg[2])));
        nodes.emplace_back(AABB(vec3d(avg[0], min[1], min[2]), vec3d(max[0], avg[1], avg[2])));
        nodes.emplace_back(AABB(vec3d(avg[0], avg[1], min[2]), vec3d(max[0], max[1], avg[2])));
        nodes.emplace_back(AABB(vec3d(min[0], avg[1], min[2]), vec3d(avg[0], max[1], avg[2])));
        nodes.emplace_back(AABB(vec3d(min[0], min[1], avg[2]), vec3d(avg[0], avg[1], max[2])));
        nodes.emplace_back(AABB(vec3d(avg[0], min[1], avg[2]), vec3d(max[0], avg[1], max[2])));
        nodes.emplace_back(AABB(vec3d(avg[0], avg[1], avg[2]), vec3d(max[0], max[1], max[2])));
        nodes.emplace_back(AABB(vec3d(min[0], avg[1], avg[2]), vec3d(avg[0], max[1], max[2])));
    }
    node->is_inner = true;

    for(uint it : node->item_indices)
    {
        bool orphan = true;
        for(int i=0; i<8; ++i)
        {
            auto child = &nodes[node->start + i];
            if(child->bbox.intersects_box(items[it].aabb))
            {
                child->item_indices.push_back(it);
                orphan = false;
            }
        }
        assert(!orphan);
    }

    node->item_indices.clear();
    node->is_inner = true;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

/*CINO_INLINE
bool FOctree::intersects_triangle(const vec3d t1[],
                                  const vec3d t2[],
                                  const bool ignore_if_valid_complex,
                                  double * min,
                                  double * perm,
                                  double * t_min,
                                  double * t_perm) const
 {
     auto res = triangle_triangle_intersect_3d(t1[0].ptr(), t1[1].ptr(), t1[2].ptr(), t2[0].ptr(), t2[1].ptr(), t2[2].ptr(),min, perm, t_min, t_perm);
     if(ignore_if_valid_complex) return (res > SIMPLICIAL_COMPLEX);
     return (res>=SIMPLICIAL_COMPLEX);
 }*/
}
