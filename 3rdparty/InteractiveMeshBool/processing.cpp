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

#include "processing.h"

#include "utils.h"

inline double computeMultiplier(const std::vector<double> &coords)
{
    const double R = 11259470696.0; //avg_max_coord (167.78) * old_multiplier (67108864.0)

    double max_coord = *std::max_element(coords.begin(), coords.end());
    double min_coord = *std::min_element(coords.begin(), coords.end());

    double abs_max_coord = std::max(std::abs(min_coord), std::abs(max_coord));

    double div = R / abs_max_coord;

    //closest power of 2
    int e = static_cast<int>(std::round(std::log2(div)));
    double multiplier = (e >= 0) ? static_cast<double>(1 << e) : (1.0 / (1 << (-1 * e)));

    if(multiplier < 0) multiplier = 1; // temporary fix

    return multiplier;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void mergeDuplicatedVertices(const std::vector<double> &in_coords, const std::vector<uint> &in_tris,
                                    point_arena& arena, std::vector<genericPoint*> &verts, std::vector<uint> &tris,
                                    bool parallel)
{
    verts.reserve(in_coords.size() / 3);
    tris.reserve(in_tris.size());
    arena.init.reserve(in_tris.size());

    if(parallel)
    {
        using vec3 = std::array<double, 3>;
        auto in_vecs = (vec3*)in_coords.data();
        std::vector<uint> sorted = std::vector<uint>(in_coords.size() / 3);

        for(uint idx = 0; idx < (uint)sorted.size(); idx ++) sorted[idx] = idx;

#if false
        tbb::parallel_sort(sorted.begin(), sorted.end(), [in_vecs](auto a, auto b)
#else
        std::sort(sorted.begin(), sorted.end(), [in_vecs](auto a, auto b)
#endif
        {
            return in_vecs[a] < in_vecs[b];
        });

        std::vector<uint> lookup = std::vector<uint>(sorted.size());
        for(uint idx = 0; idx < (uint)sorted.size(); idx ++)
        {
            if (idx == 0 || in_vecs[sorted[idx]] != in_vecs[sorted[idx-1]])
            {
                auto v = in_vecs[sorted[idx]];
                arena.init.emplace_back(v[0], v[1], v[2]);
                auto& v2 = arena.init.back(); verts.push_back(&v2);
            }
            lookup[sorted[idx]] = (uint)verts.size() - 1;
        }
        tris.resize(in_tris.size());
        ::parallel_for((uint)0, (uint)in_tris.size(), [&](uint idx)
        {
            tris[idx] = lookup[in_tris[idx]];
        });
    }
    else
    {
        phmap::flat_hash_map <std::array<double, 3>, uint> v_map;
        v_map.reserve(in_tris.size() * 3);

        for(const uint &v_id : in_tris)
        {
            std::array<double, 3> v = {in_coords[(3 * v_id)], in_coords[(3 * v_id) +1], in_coords[(3 * v_id) +2]};

            auto ins = v_map.insert({v, v_map.size()});
            if (ins.second)
            {
                arena.init.emplace_back(v[0], v[1], v[2]);
                auto& v2 = arena.init.back(); verts.push_back(&v2); // new_vtx added
            }
            tris.push_back(ins.first->second);
        }
    }
}


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void removeDegenerateAndDuplicatedTriangles(const std::vector<genericPoint*> &verts, const std::vector<std::bitset<NBIT> > &in_labels,
                                                   std::vector<uint> &tris, std::vector< std::bitset<NBIT> > &labels)
{
    labels = in_labels;

    uint num_orig_tris = static_cast<uint>(tris.size() / 3);
    uint t_off = 0;
    uint l_off = 0;

    phmap::flat_hash_map < std::array<uint, 3>, uint> tris_map;
    tris_map.reserve(num_orig_tris);

    for(uint t_id = 0; t_id < num_orig_tris; t_id++)
    {
        uint v0_id = tris[(3 * t_id)];
        uint v1_id = tris[(3 * t_id) +1];
        uint v2_id = tris[(3 * t_id) +2];
        std::bitset<NBIT> l = labels[t_id];

        if(!cinolib::points_are_colinear_3d(verts[v0_id]->toExplicit3D().ptr(),
                                            verts[v1_id]->toExplicit3D().ptr(),
                                            verts[v2_id]->toExplicit3D().ptr())) // good triangle
        {
            std::array<uint, 3> tri = {v0_id, v1_id, v2_id};
            std::sort(tri.begin(), tri.end());

            auto ins = tris_map.insert({tri, l_off});

            if(ins.second) // first time for tri v0, v1, v2
            {
                labels[l_off] = l;
                l_off++;

                tris[t_off]    = v0_id;
                tris[t_off +1] = v1_id;
                tris[t_off +2] = v2_id;
                t_off += 3;
            }
            else
            {
                uint pos = ins.first->second;
                labels[pos] |= l; // label for duplicates
            }
        }
    }

    tris.resize(t_off);
    labels.resize(l_off);
}


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void freePointsMemory(std::vector<genericPoint *> &points)
{
    for(uint p = 0; p < points.size(); p++)
        delete points[p];
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void computeApproximateCoordinates(const std::vector<genericPoint *> &vertices, std::vector<double> &coords)
{
    coords.reserve(3 * (vertices.size() -5));
    double multiplier = vertices.back()->toExplicit3D().X();

    for(uint i = 0; i < (vertices.size() - 5); i++)
    {
        auto &v = vertices[i];

        if(v->isExplicit3D())
        {
            coords.push_back(v->toExplicit3D().X() / multiplier);
            coords.push_back(v->toExplicit3D().Y() / multiplier);
            coords.push_back(v->toExplicit3D().Z() / multiplier);
        }
        else //implicit point
        {
            double x, y, z;
            v->getApproxXYZCoordinates(x, y, z);
            coords.push_back(x / multiplier);
            coords.push_back(y / multiplier);
            coords.push_back(z / multiplier);
        }
    }
}
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void computeApproximateCoordinates(const std::vector<genericPoint *> &vertices, std::vector<cinolib::vec3d> &out_vertices)
{
    out_vertices.reserve((vertices.size() -5));
    double multiplier = vertices.back()->toExplicit3D().X();

    for(uint i = 0; i < (vertices.size() - 5); i++)
    {
        auto &v = vertices[i];

        if(v->isExplicit3D())
        {
            out_vertices.emplace_back(v->toExplicit3D().X() / multiplier,
                                      v->toExplicit3D().Y() / multiplier,
                                      v->toExplicit3D().Z() / multiplier);
        }
        else //implicit point
        {
            double x, y, z;
            v->getApproxXYZCoordinates(x, y, z);
            out_vertices.emplace_back(x / multiplier,
                                      y / multiplier,
                                      z / multiplier);
        }
    }
}


