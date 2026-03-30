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
 * ***************************************************************************************/

#include "processing.h"
#include "triangle_soup.h"
#include "intersection_classification.h"
#include "triangulation.h"
#include "solve_intersections.h"

inline void meshArrangementPipeline(const std::vector<double> &in_coords, const std::vector<uint> &in_tris, const std::vector< std::bitset<NBIT> > &in_labels, point_arena &arena,
                                    std::vector<genericPoint*> &vertices, std::vector<uint> &out_tris, std::vector< std::bitset<NBIT> > &out_labels)
{
    initFPU();

    AuxiliaryStructure g;

    double multiplier = computeMultiplier(in_coords);

    std::vector<uint> tmp_tris;
    std::vector< std::bitset<NBIT> > tmp_labels;

    mergeDuplicatedVertices(in_coords, in_tris, arena, vertices, tmp_tris, true);

    removeDegenerateAndDuplicatedTriangles(vertices, in_labels, tmp_tris, tmp_labels);

    TriangleSoup ts(arena, vertices, tmp_tris, tmp_labels, multiplier, true);

    phmap::flat_hash_set<std::pair<uint, uint>> intersectionList;
    intersectionList.insert(g.intersectionList().begin(), g.intersectionList().end());
    detectIntersections(ts, intersectionList);

    g.initFromTriangleSoup(ts);

    classifyIntersections(ts, arena, g);

    triangulation(ts, arena, g, out_tris, out_labels, true);

    ts.appendJollyPoints();
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void solveIntersections(const std::vector<double> &in_coords, const std::vector<uint> &in_tris, point_arena &arena,
                               std::vector<double> &out_coords, std::vector<uint> &out_tris)
{
    std::vector<genericPoint*> vertices;
    std::vector< std::bitset<NBIT>> tmp_in_labels(in_tris.size() / 3), out_labels;

    meshArrangementPipeline(in_coords, in_tris, tmp_in_labels, arena, vertices, out_tris, out_labels);

    computeApproximateCoordinates(vertices, out_coords);
    freePointsMemory(vertices);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void solveIntersections(const std::vector<double> &in_coords, const std::vector<uint> &in_tris, point_arena &arena,
                               std::vector<genericPoint *> &out_vertices, std::vector<uint> &out_tris)
{
    std::vector< std::bitset<NBIT>> tmp_in_labels(in_tris.size() / 3), out_labels;

    meshArrangementPipeline(in_coords, in_tris, tmp_in_labels, arena, out_vertices, out_tris, out_labels);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void solveIntersections(const std::vector<double> &in_coords, const std::vector<uint> &in_tris, const std::vector<uint> &in_labels, point_arena &arena,
                                  std::vector<double> &out_coords, std::vector<uint> &out_tris, std::vector< std::bitset<NBIT> > &out_labels)
{
    std::vector<genericPoint*> vertices;
    std::vector< std::bitset<NBIT>> tmp_in_labels(in_labels.size());

    for(uint i = 0; i < in_labels.size(); i++)
        tmp_in_labels[i][in_labels[i]] = 1;

    meshArrangementPipeline(in_coords, in_tris, tmp_in_labels, arena, vertices, out_tris, out_labels);

    computeApproximateCoordinates(vertices, out_coords);
    freePointsMemory(vertices);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void solveIntersections(const std::vector<double> &in_coords, const std::vector<uint> &in_tris, const std::vector<uint> &in_labels, point_arena &arena,
                               std::vector<genericPoint *> &vertices, std::vector<uint> &out_tris, std::vector<std::bitset<NBIT> > &out_labels)
{
    std::vector< std::bitset<NBIT>> tmp_in_labels(in_labels.size());

    for(uint i = 0; i < in_labels.size(); i++)
        tmp_in_labels[i][in_labels[i]] = 1;

    meshArrangementPipeline(in_coords, in_tris, tmp_in_labels, arena, vertices, out_tris, out_labels);
}
