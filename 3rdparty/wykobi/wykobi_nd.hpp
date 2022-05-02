/*
(***********************************************************************)
(*                                                                     *)
(* Wykobi Computational Geometry Library                               *)
(* Release Version 0.0.5                                               *)
(* http://www.wykobi.com                                               *)
(* Copyright (c) 2005-2019 Arash Partow, All Rights Reserved.          *)
(*                                                                     *)
(* The Wykobi computational geometry library and its components are    *)
(* supplied under the terms of the open source MIT License.            *)
(* The contents of the Wykobi computational geometry library and its   *)
(* components may not be copied or disclosed except in accordance with *)
(* the terms of the MIT License.                                       *)
(*                                                                     *)
(* URL: https://opensource.org/licenses/MIT                            *)
(*                                                                     *)
(***********************************************************************)
*/


#ifndef INCLUDE_WYKOBI_ND
#define INCLUDE_WYKOBI_ND


#include <vector>
#include <limits>
#include <algorithm>
#include <cassert>

#include "wykobi.hpp"
#include "wykobi_math.hpp"


namespace wykobi
{
   template <typename T, std::size_t D>
   inline bool parallel(const line<T,D>& line1, const line<T,D>& line2);

   template <typename T, std::size_t D>
   inline bool parallel(const segment<T,D>& segment1, const segment<T,D>& segment2);

   template <typename T, std::size_t D>
   inline bool perpendicular(const line<T,D>& line1, const line<T,D>& line2);

   template <typename T, std::size_t D>
   inline bool perpendicular(const segment<T,D>& segment1, const segment<T,D>& segment2);

   template <typename T, std::size_t D>
   inline bool collinear(const pointnd<T,D>& point1, const pointnd<T,D>& point2, const pointnd<T,D>& point3);

   template <typename T, std::size_t D>
   inline bool robust_collinear(const pointnd<T,D>& point1, const pointnd<T,D>& point2, const pointnd<T,D>& point3);

   template <typename T, std::size_t D>
   inline bool is_point_collinear(const segment<T,D>& segment, const pointnd<T,D>& point, const bool robust = false);

   template <typename T, std::size_t D>
   inline bool intersect(const segment<T,D>& segment1, const segment<T,D>& segment2, const T& fuzzy = T(0.0));

   template <typename T, std::size_t D>
   inline bool intersect(const line<T,D>& line1, const line<T,D>& line2, const T& fuzzy = T(0.0));

   template <typename T, std::size_t D>
   inline pointnd<T,D> intersection_point(const segment<T,D>& segment1, const segment<T,D>& segment2, const T& fuzzy = T(0.0));

   template <typename T, std::size_t D>
   inline pointnd<T,D> intersection_point(const line<T,D>& line1, const line<T,D>& line2, const T& fuzzy = T(0.0));

   template <typename T, std::size_t D>
   inline T distance(const pointnd<T,D>& point1, const pointnd<T,D>& point2);

   template <typename T, std::size_t D>
   inline T distance(const pointnd<T,D>& point, const segment<T,D>& segment);

   template <typename T, std::size_t D>
   inline T distance(const pointnd<T,D>& point, const line<T,D>& line);

   template <typename T, std::size_t D>
   inline T distance(const segment<T,D>& segment1, const segment<T,D>& segment2);

   template <typename T, std::size_t D>
   inline T distance(const line<T,D>& line1, const line<T,D>& line2);

   template <typename T, std::size_t D>
   inline T lay_distance(const pointnd<T,D>& point1, const pointnd<T,D>& point2);

   template <typename T, std::size_t D>
   inline T lay_distance(const pointnd<T,D>& point, const segment<T,D>& segment);

   template <typename T, std::size_t D>
   inline T lay_distance(const pointnd<T,D>& point, const line<T,D>& line);

   template <typename T, std::size_t D>
   inline T lay_distance(const segment<T,D>& segment1, const segment<T,D>& segment2);

   template <typename T, std::size_t D>
   inline T lay_distance(const line<T,D>& line1, const line<T,D>& line2);

   template <typename T, std::size_t D>
   inline T manhattan_distance(const pointnd<T,D>& point1, const pointnd<T,D>& point2);

   template <typename T, std::size_t D>
   inline T chebyshev_distance(const pointnd<T,D>& point1, const pointnd<T,D>& point2);

   template <typename T, std::size_t D>
   inline T manhattan_distance(const pointnd<T,D>& point1, const pointnd<T,D>& point2);

   template <typename T, std::size_t D>
   inline T inverse_chebyshev_distance(const pointnd<T,D>& point1, const pointnd<T,D>& point2);

   template <typename T, std::size_t D>
   inline bool point_in_box(const pointnd<T,D>& point, const box<T,D>& box);

   template <typename T, std::size_t D>
   inline bool point_in_sphere(const pointnd<T,D>& point, const hypersphere<T,D>& hypersphere);

   template <typename T, std::size_t D>
   inline pointnd<T,D> closest_point_on_segment_from_point(const segment<T,D>& segment, const pointnd<T,D>& point);

   template <typename T, std::size_t D>
   inline pointnd<T,D> closest_point_on_line_from_point(const line<T,D>& segment, const pointnd<T,D>& point);

   template <typename T, std::size_t D>
   inline pointnd<T,D> closest_point_on_sphere_from_point(const hypersphere<T,D>& sphere, const pointnd<T,D>& point);

   template <typename T, std::size_t D>
   inline pointnd<T,D> closest_point_on_plane_from_point(const plane<T,D>& plane, const pointnd<T,D>& point);

   template <typename T, std::size_t D>
   inline pointnd<T,D> closest_point_on_box_from_point(const box<T,D>& box, const pointnd<T,D>& point);

   template <typename T, std::size_t D>
   inline pointnd<T,D> project_point_t(const pointnd<T,D>& source_point,
                                       const pointnd<T,D>& destination_point,
                                       const T& t);

   template <typename T, std::size_t D>
   inline pointnd<T,D> project_point(const pointnd<T,D>& source_point,
                                     const pointnd<T,D>& destination_point,
                                     const T& distance);

   template <typename T, std::size_t D>
   inline pointnd<T,D> mirror(const pointnd<T,D>& point, const line<T,D>& mirror_axis);

   template <typename T, std::size_t D>
   inline segment<T,D> mirror(const segment<T,D>& segment, const line<T,D>& mirror_axis);

   template <typename T, std::size_t D>
   inline line<T,D> mirror(const line<T,D>& line, const wykobi::line<T,D>& mirror_axis);

   template <typename T, std::size_t D>
   inline box<T,D> mirror(const box<T,D>& box, const line<T,D>& mirror_axis);

   template <typename T, std::size_t D>
   inline triangle<T,D> mirror(const triangle<T,D>& triangle, const line<T,D>& mirror_axis);

   template <typename T, std::size_t D>
   inline quadix<T,D> mirror(const quadix<T,D>& quadix, const line<T,D>& mirror_axis);

   template <typename T, std::size_t D>
   inline hypersphere<T,D> mirror(const hypersphere<T,D>& sphere, const line<T,D>& mirror_axis);

   template <typename T, std::size_t D>
   inline polygon<T,D> mirror(const polygon<T,D>& polygon, const line<T,D>& mirror_axis);

   template <typename T, std::size_t D>
   inline segment<T,D> project_onto_axis(const pointnd<T,D>& point, const line<T,D>& axis);

   template <typename T, std::size_t D>
   inline segment<T,D> project_onto_axis(const triangle<T,D>& triangle, const line<T,D>& axis);

   template <typename T, std::size_t D>
   inline segment<T,D> project_onto_axis(const box<T,D>& box, const line<T,D>& axis);

   template <typename T, std::size_t D>
   inline segment<T,D> project_onto_axis(const quadix<T,D>& quadix, const line<T,D>& axis);

   template <typename T, std::size_t D>
   inline segment<T,D> project_onto_axis(const hypersphere<T,D>& sphere, const line<T,D>& axis);

   template <typename T, std::size_t D>
   inline segment<T,D> project_onto_axis(const polygon<T,D>& polygon, const line<T,D>& axis);

   template <typename T, std::size_t D>
   inline T perimeter(const triangle<T,D>& triangle);

   template <typename T, std::size_t D>
   inline T perimeter(const quadix<T,D>& quadix);

   template <typename T, std::size_t D>
   inline T perimeter(const polygon<T,D>& polygon);

   template <typename T, std::size_t D>
   inline pointnd<T,D> generate_random_point(const segment<T,D>& segment);

   template <typename T, std::size_t D>
   inline pointnd<T,D> generate_random_point(const triangle<T,D>& triangle);

   template <typename T, std::size_t D>
   inline pointnd<T,D> generate_random_point(const quadix<T,D>& quadix);

   template <typename T, std::size_t D>
   inline pointnd<T,D> generate_random_point(const box<T,D>& box);

   template <typename T, std::size_t D, typename OutputIterator>
   inline void generate_random_points(const box<T,D>& box, const std::size_t& point_count, OutputIterator out);

   template <typename T, std::size_t D, typename OutputIterator>
   inline void generate_random_points(const segment<T,D>& segment, const std::size_t& point_count, OutputIterator out);

   template <typename T, std::size_t D, typename OutputIterator>
   inline void generate_random_points(const triangle<T,D>& triangle, const std::size_t& point_count, OutputIterator out);

   template <typename T, std::size_t D, typename OutputIterator>
   inline void generate_random_points(const quadix<T,D>& quadix, const std::size_t& point_count, OutputIterator out);

   template <typename T, std::size_t D>
   inline T vector_norm(const vectornd<T,D>& v);

   template <typename T, std::size_t D>
   inline vectornd<T,D> normalize(const vectornd<T,D>& v);

   template <typename T, std::size_t D>
   inline vectornd<T,D> operator+(const vectornd<T,D>& v1, const vectornd<T,D>& v2);

   template <typename T, std::size_t D>
   inline vectornd<T,D> operator-(const vectornd<T,D>& v1, const vectornd<T,D>& v2);

   template <typename T, std::size_t D>
   inline T dot_product(const vectornd<T,D>& v1, const vectornd<T,D>& v2);

   template <typename T, std::size_t D>
   inline vectornd<T,D> operator*(const vectornd<T,D>& v1, const T& scale);

   template <typename T, std::size_t D>
   inline vectornd<T,D> operator*(const T& scale, const vectornd<T,D>& v1);

   template <typename T, std::size_t D>
   inline pointnd<T,D> operator*(const pointnd<T,D>& point, const T& scale);

   template <typename T, std::size_t D>
   inline pointnd<T,D> operator*(const T& scale, const pointnd<T,D>& point);

   template <typename T, std::size_t D>
   inline vectornd<T,D> operator/(const vectornd<T,D>& v1, const T& scale);

   template <typename T, std::size_t D>
   inline pointnd<T,D> operator/(const pointnd<T,D>& point, const T& scale);

   template <typename T, std::size_t D>
   inline pointnd<T,D> operator+(const pointnd<T,D>& point, const vectornd<T,D>& v);

   template <typename T, std::size_t D>
   inline pointnd<T,D> operator+(const vectornd<T,D>& v, const pointnd<T,D>& point);

   template <typename T, std::size_t D>
   inline vectornd<T,D> operator-(const pointnd<T,D>& p1, const pointnd<T,D>& p2);

   template <typename T, std::size_t D>
   inline pointnd<T,D> operator+(const pointnd<T,D>& p1, const pointnd<T,D>& p2);

   template <typename T>
   inline T operator*(const vectornd<T,2>& v1, const vectornd<T,2>& v2);

   template <typename T>
   inline vectornd<T,3> operator*(const vectornd<T,3>& v1, const vectornd<T,3>& v2);

   template <typename T, std::size_t D>
   inline bool operator < (const pointnd<T,D>& point1, const pointnd<T,D>& point2);

   template <typename T, std::size_t D>
   inline bool operator > (const pointnd<T,D>& point1, const pointnd<T,D>& point2);

   template <typename T, std::size_t D>
   inline bool operator == (const pointnd<T,D>& point1, const pointnd<T,D>& point2);

   template <typename T, std::size_t D>
   inline bool is_equal(const pointnd<T,D>& point1, const pointnd<T,D>& point2, const T& epsilon);

   template <typename T, std::size_t D>
   inline bool is_equal(const pointnd<T,D>& point1, const pointnd<T,D>& point2);

   template <typename T, std::size_t D>
   inline bool not_equal(const pointnd<T,D>& point1, const pointnd<T,D>& point2, const T& epsilon);

   template <typename T, std::size_t D>
   inline bool not_equal(const pointnd<T,D>& point1, const pointnd<T,D>& point2);

   template <typename T, std::size_t D>
   inline pointnd<T,D> degenerate_pointnd();

   template <typename T, std::size_t D>
   inline vectornd<T,D> degenerate_vectornd();

   template <typename T, std::size_t D>
   inline ray<T,D> degenerate_raynd();

   template <typename T, std::size_t D>
   inline line<T,D> degenerate_linend();

   template <typename T, std::size_t D>
   inline segment<T,D> degenerate_segmentnd();

   template <typename T, std::size_t D>
   inline triangle<T,D> degenerate_trianglend();

   template <typename T, std::size_t D>
   inline quadix<T,D> degenerate_quadixnd();

   template <typename T, std::size_t D>
   inline box<T,D> degenerate_box();

   template <typename T, std::size_t D>
   inline hypersphere<T,D> degenerate_hypersphere();

   template <typename T, std::size_t D>
   inline pointnd<T,D> positive_infinite_pointnd();

   template <typename T, std::size_t D>
   inline pointnd<T,D> negative_infinite_pointnd();

   template <typename T, std::size_t D>
   inline void swap(pointnd<T,D>& point1, pointnd<T,D>& point2);

   template <typename T, std::size_t D>
   inline vectornd<T,D> make_vector(const pointnd<T,D>& point);

   template <typename T, std::size_t D>
   inline ray<T,D> make_ray(const pointnd<T,D>& origin, const vectornd<T,D>& direction);

   template <typename T, std::size_t D>
   inline segment<T,D> make_segment(const pointnd<T,D>& point1, const pointnd<T,D>& point2);

   template <typename T, std::size_t D>
   inline line<T,D> make_line(const pointnd<T,D>& point1, const pointnd<T,D>& point2);

   template <typename T, std::size_t D>
   inline box<T,D> make_box(const pointnd<T,D>& point1, const pointnd<T,D>& point2);

   template <typename T, std::size_t D>
   inline triangle<T,D> make_triangle(const pointnd<T,D>& point1, const pointnd<T,D>& point2, const pointnd<T,D>& point3);

   template <typename T, std::size_t D>
   inline quadix<T,D> make_quadix(const pointnd<T,D>& point1, const pointnd<T,D>& point2, const pointnd<T,D>& point3, const pointnd<T,D>& point4);

   template <typename T, std::size_t D>
   inline hypersphere<T,D> make_sphere(const pointnd<T,D>& point, const T& radius);

   template <typename T, std::size_t D>
   inline hypersphere<T,D> make_sphere(const pointnd<T,D>& point1, const pointnd<T,D>& point2);

   template <typename T, std::size_t D>
   inline polygon<T,D> make_polygon(const std::vector< pointnd<T,D> >& point_list);

   template <typename T, std::size_t D>
   inline polygon<T,D> make_polygon(const triangle<T,D>& triangle);

   template <typename T, std::size_t D>
   inline polygon<T,D> make_polygon(const quadix<T,D>& quadix);

} // wykobi namespace

#include "wykobi_nd.inl"

#endif
