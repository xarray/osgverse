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

#ifndef SOLVE_INTERSECTIONS_H
#define SOLVE_INTERSECTIONS_H

#include "aux_structure.h"

//#include <bitset>

/**
* This function performs the mesh arrangement of an input triangle set.
* Use one of the solveInterctions functions to interface whit it.
*/
inline void meshArrangementPipeline(const std::vector<double> &in_coords, const std::vector<uint> &in_tris, const std::vector< std::bitset<NBIT> > &in_labels, point_arena &arena,
                                    std::vector<genericPoint*> &out_vertices, std::vector<uint> &out_tris, std::vector< std::bitset<NBIT> > &out_labels);


/**
 * This function performs the mesh arrangement of an input set of triangles returning an approximation
 * of the intersection points' coordinates.
 *
 * @param in_coords: the coordinates of the input vertices
 * @param in_tris: the indices of the vertices of the input triangles
 * @param arena: a temporary structure of type "point_arena" to efficiently manage the memory
 * @param out_coords: the coordinates of the points after the arrangement (the coordinates of the intersection points are approximate)
 * @param out_tris: the indices of the vertices of the output triangles
 */
inline void solveIntersections(const std::vector<double> &in_coords, const std::vector<uint> &in_tris, point_arena &arena,
                               std::vector<double> &out_coords, std::vector<uint> &out_tris);


/**
 * This function performs the mesh arrangement of an input set of triangles returning the intersection points
 * in implicit form (exact).
 *
 * @param in_coords: the coordinates of the input vertices
 * @param in_tris: the indices of the vertices of the input triangles
 * @param arena: a temporary structure of type "point_arena" to efficiently manage the memory
 * @param out_vertices: the set of vertices after the arrangement in implicit form (type: genericPoint*)
 * @param out_tris: the indices of the vertices of the output triangles
 *
 * IMPORTANT: if you use this function
 * - if, at some point, you need an approximation of your vertices you need to call the computeApproximateCoordinates(...) function contained in processing.h
 * - remember to free the dynamic allocated memory of the implicit points by calling the freePointsMemory(...) function contained in processing.h
 */
inline void solveIntersections(const std::vector<double> &in_coords, const std::vector<uint> &in_tris, point_arena &arena,
                               std::vector<genericPoint*> &out_vertices, std::vector<uint> &out_tris);



/**
 * This function performs the mesh arrangement of an input set of triangles returning an approximation
 * of the intersection points' coordinates and a label for each triangle.
 *
 * @param in_coords: the coordinates of the input vertices
 * @param in_tris: the indices of the vertices of the input triangles
 * @param in_labels: the labels of the input triangles (i.e. the id of the mesh containing them)
 * @param arena: a temporary structure of type "point_arena" to efficiently manage the memory
 * @param out_coords: the coordinates of the points after the arrangement (the coordinates of the intersection points are approximate)
 * @param out_tris: the indices of the vertices of the output triangles
 * @param out_labels: a vector of bitset containing, for each output triangle, the set of labels of the generating input triangles
 */
inline void solveIntersections(const std::vector<double> &in_coords, const std::vector<uint> &in_tris, const std::vector<uint> &in_labels, point_arena &arena,
                               std::vector<double> &out_coords, std::vector<uint> &out_tris, std::vector< std::bitset<NBIT> > &out_labels);


/**
 * This function performs the mesh arrangement of an input set of triangles returning the intersection points
 * in implicit form (exact).
 *
 * @param in_coords: the coordinates of the input vertices
 * @param in_tris: the indices of the vertices of the input triangles
 * @param in_labels: the labels of the input triangles (i.e. the id of the mesh containing them)
 * @param arena: a temporary structure of type "point_arena" to efficiently manage the memory
 * @param out_vertices: the set of vertices after the arrangement in implicit form (type: genericPoint*)
 * @param out_tris: the indices of the vertices of the output triangles
 * @param out_labels: a vector of bitset containing, for each output triangle, the set of labels of the generating input triangles
 *
 * IMPORTANT: if you use this function
 * - if, at some point, you need an approximation of your vertices you need to call the computeApproximateCoordinates(...) function contained in processing.h
 * - remember to free the dynamic allocated memory of the implicit points by calling the freePointsMemory(...) function contained in processing.h
 */
inline void solveIntersections(const std::vector<double> &in_coords, const std::vector<uint> &in_tris, const std::vector<uint> &in_labels, point_arena &arena,
                               std::vector<genericPoint*> &vertices, std::vector<uint> &out_tris, std::vector< std::bitset<NBIT> > &out_labels);


//#include "solve_intersections.cpp"


#endif // SOLVE_INTERSECTIONS_H
