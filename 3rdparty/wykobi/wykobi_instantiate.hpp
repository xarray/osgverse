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


#ifndef INCLUDE_WYKOBI_INSTANTIATE
#define INCLUDE_WYKOBI_INSTANTIATE


#include "wykobi.hpp"
#include "wykobi_nd.hpp"
#include "wykobi_algorithm.hpp"
#include "wykobi_utilities.hpp"


namespace wykobi
{
   #define INSTANTIATE_WYKOBI(T,InputIterator2d,InputIterator3d,OutputIterator2d,OutputIterator3d)\
      template int orientation<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& px, const T& py);\
      template int orientation<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& px, const T& py, const T& pz);\
      template int robust_orientation<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& px, const T& py);\
      template int robust_orientation<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& px, const T& py, const T& pz);\
      template int orientation<T>(const point2d<T>& point1, const point2d<T>& point2, const T& px, const T& py);\
      template int orientation<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);\
      template int orientation<T>(const line<T,2>& line, const point2d<T>& point);\
      template int orientation<T>(const segment<T,2>& segment, const point2d<T>& point);\
      template int orientation<T>(const triangle<T,2>& triangle);\
      template int orientation<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const T& px, const T& py, const T& pz);\
      template int orientation<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const point3d<T>& point4);\
      template int orientation<T>(const triangle<T,3>& triangle, const point3d<T>& point);\
      template bool differing_orientation<T>(const T& x1,  const T& y1, const T& x2,  const T& y2, const T& p1x, const T& p1y, const T& p2x, const T& p2y);\
      template bool differing_orientation<T>(const point2d<T>& p1, const point2d<T>& p2, const point2d<T>& q1, const point2d<T>& q2);\
      template int in_circle<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& px, const T& py);\
      template int in_circle<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3, const point2d<T>& point4);\
      template int in_circle<T>(const triangle<T,2>& triangle, const point2d<T>& point);\
      template int in_sphere<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& x4, const T& y4, const T& z4, const T& px, const T& py, const T& pz);\
      template int in_sphere<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const point3d<T>& point4, const point3d<T>& point5);\
      template int in_sphere<T>(const quadix<T,3>& quadix, const point3d<T>& point);\
      template T signed_area<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& px, const T& py);\
      template T signed_area<T>(const point2d<T>& point1, const point2d<T>& point2, const T& px, const T& py);\
      template T signed_area<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);\
      template T signed_area<T>(const segment<T,2>& segment, const point2d<T>& point);\
      template T signed_volume<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& px, const T& py, const T& pz);\
      template T signed_volume<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const T& px, const T& py, const T& pz);\
      template T signed_volume<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const point3d<T>& point4);\
      template T signed_volume<T>(const triangle<T,3>& triangle, const point3d<T>& point);\
      template bool collinear<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& epsilon);\
      template bool collinear<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& epsilon);\
      template bool collinear<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);\
      template bool collinear<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3);\
      template bool robust_collinear<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& epsilon);\
      template bool robust_collinear<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3, const T& epsilon);\
      template bool robust_collinear<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& epsilon);\
      template bool robust_collinear<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const T& epsilon);\
      template bool robust_collinear<T>(const line<T,2>& line, const point2d<T>& point, const T& epsilon);\
      template bool robust_collinear<T>(const line<T,3>& line, const point3d<T>& point, const T& epsilon);\
      template bool is_point_collinear<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& px, const T& py, const bool robust);\
      template bool is_point_collinear<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3, const bool robust);\
      template bool is_point_collinear<T>(const point2d<T>& point1, const point2d<T>& point2, const T& px, const T& py, const bool robust);\
      template bool is_point_collinear<T>(const segment<T,2>& segment, const point2d<T>&point, const bool robust);\
      template bool is_point_collinear<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& px, const T& py, const T& pz, const bool robust);\
      template bool is_point_collinear<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const bool robust);\
      template bool is_point_collinear<T>(const segment<T,3>& segment, const point3d<T>&point, const bool robust);\
      template bool robust_coplanar<T>(const point3d<T> point1, const point3d<T> point2, const point3d<T> point3, const point3d<T> point4, const T& epsilon);\
      template bool coplanar<T>(const ray<T,3>& ray1, const ray<T,3>& ray2);\
      template bool coplanar<T>(const segment<T,3>& segment1, const segment<T,3>& segment2);\
      template bool coplanar<T>(const line<T,3>& line1, const line<T,3>& line2);\
      template bool coplanar<T>(const triangle<T,3>& triangle1, const triangle<T,3>& triangle2);\
      template bool coplanar<T>(const quadix<T,3>& quadix1, const quadix<T,3>& quadix2);\
      template bool cocircular<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4, const T& epsilon);\
      template bool cocircular<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3, const point2d<T>& point4, const T& epsilon);\
      template bool cocircular<T>(const triangle<T,2>& triangle, const point2d<T>& point, const T& epsilon);\
      template bool cocircular<T>(const circle<T>& circle, const point2d<T>& point, const T& epsilon);\
      template bool is_skinny_triangle<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3);\
      template bool is_skinny_triangle<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);\
      template bool is_skinny_triangle<T>(const triangle<T,2>& triangle);\
      template bool intersect<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4);\
      template bool intersect<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4, T& ix,T& iy);\
      template bool intersect<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3, const point2d<T>& point4);\
      template bool intersect<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3, const point2d<T>& point4, point2d<T>& int_point);\
      template bool intersect<T>(const segment<T,2>& segment1, const segment<T,2>& segment2);\
      template bool intersect<T>(const segment<T,2>& segment1, const segment<T,2>& segment2,T& ix, T& iy);\
      template bool intersect<T>(const segment<T,2>& segment1, const segment<T,2>& segment2,point2d<T>& i_point);\
      template bool intersect<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& x4, const T& y4, const T& z4, const T& fuzzy);\
      template bool intersect<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const point3d<T>& point4, const T& fuzzy);\
      template bool intersect<T>(const segment<T,3>& segment1, const segment<T,3>& segment2, const T& fuzzy);\
      template bool intersect<T>(const segment<T,2>& segment, const rectangle<T>& rectangle);\
      template bool intersect<T>(const segment<T,2>& segment, const triangle<T,2>& triangle);\
      template bool intersect<T>(const segment<T,2>& segment, const quadix<T,2>& quadix);\
      template bool intersect<T>(const segment<T,2>& segment, const line<T,2>& line);\
      template bool intersect<T>(const segment<T,2>& segment, const circle<T>& circle);\
      template bool intersect<T>(const segment<T,2>& segment, const quadratic_bezier<T,2>& bezier, const std::size_t& steps);\
      template bool intersect<T>(const segment<T,2>& segment, const cubic_bezier<T,2>& bezier, const std::size_t& steps);\
      template bool intersect<T>(const segment<T,3>& segment, const line<T,3>& line, const T& fuzzy);\
      template bool intersect<T>(const segment<T,3>& segment, const box<T,3>& box);\
      template bool intersect<T>(const segment<T,3>& segment, const sphere<T>& sphere);\
      template bool intersect<T>(const segment<T,3>& segment, const plane<T,3>& plane);\
      template bool intersect<T>(const segment<T,3>& segment, const quadratic_bezier<T,3>& bezier, const std::size_t& steps);\
      template bool intersect<T>(const segment<T,3>& segment, const cubic_bezier<T,3>& bezier, const std::size_t& steps);\
      template bool intersect<T>(const line<T,2>& line, const triangle<T,2>& triangle);\
      template bool intersect<T>(const line<T,2>& line, const quadix<T,2>& quadix);\
      template bool intersect<T>(const line<T,2>& line1, const line<T,2>& line2);\
      template bool intersect<T>(const line<T,2>& line, const circle<T>& circle);\
      template bool intersect<T>(const line<T,2>& line, const quadratic_bezier<T,2>& bezier, const std::size_t& steps);\
      template bool intersect<T>(const line<T,2>& line, const cubic_bezier<T,2>& bezier, const std::size_t& steps);\
      template bool intersect<T>(const line<T,3>& line, const triangle<T,3>& triangle);\
      template bool intersect<T>(const line<T,3>& line, const plane<T,3>& plane);\
      template bool intersect<T>(const line<T,3>& line, const sphere<T>& sphere);\
      template bool intersect<T>(const line<T,3>& line, const quadratic_bezier<T,3>& bezier, const std::size_t& steps);\
      template bool intersect<T>(const line<T,3>& line, const cubic_bezier<T,3>& bezier, const std::size_t& steps);\
      template bool intersect<T>(const triangle<T,2>& triangle1, const triangle<T,2>& triangle2);\
      template bool intersect<T>(const triangle<T,2>& triangle, const circle<T>& circle);\
      template bool intersect<T>(const triangle<T,2>& triangle, const rectangle<T>& rectangle);\
      template bool intersect<T>(const triangle<T,2>& triangle, const quadratic_bezier<T,2>& bezier, const std::size_t& steps);\
      template bool intersect<T>(const triangle<T,2>& triangle, const cubic_bezier<T,2>& bezier, const std::size_t& steps);\
      template bool intersect<T>(const rectangle<T>& rectangle1, const rectangle<T>& rectangle2);\
      template bool intersect<T>(const rectangle<T>& rectangle, const quadratic_bezier<T,2>& bezier, const std::size_t& steps);\
      template bool intersect<T>(const rectangle<T>& rectangle, const cubic_bezier<T,2>& bezier, const std::size_t& steps);\
      template bool intersect<T>(const rectangle<T>& rectangle, const circle<T>& circle);\
      template bool intersect<T>(const quadix<T,2>& quadix, const quadratic_bezier<T,2>& bezier, const std::size_t& steps);\
      template bool intersect<T>(const quadix<T,2>& quadix, const cubic_bezier<T,2>& bezier, const std::size_t& steps);\
      template bool intersect<T>(const circle<T>& circle1, const circle<T>& circle2);\
      template bool intersect<T>(const circle<T>& circle, const quadratic_bezier<T,2>& bezier, const std::size_t& steps);\
      template bool intersect<T>(const circle<T>& circle, const cubic_bezier<T,2>& bezier, const std::size_t& steps);\
      template bool intersect<T>(const box<T,3>& box, const sphere<T>& sphere);\
      template bool intersect<T>(const sphere<T>& sphere1, const sphere<T>& sphere2);\
      template bool intersect<T>(const sphere<T>& sphere, const quadratic_bezier<T,3>& bezier, const std::size_t& steps);\
      template bool intersect<T>(const sphere<T>& sphere, const cubic_bezier<T,3>& bezier, const std::size_t& steps);\
      template bool intersect<T>(const ray<T,2>& ray1, const ray<T,2>& ray2);\
      template bool intersect<T>(const ray<T,3>& ray1, const ray<T,3>& ray2);\
      template bool intersect<T>(const ray<T,2>& ray, const segment<T,2>& segment);\
      template bool intersect<T>(const ray<T,3>& ray, const segment<T,3>& segment);\
      template bool intersect<T>(const ray<T,2>& ray, const rectangle<T>& rectangle);\
      template bool intersect<T>(const ray<T,3>& ray, const box<T,3>& box);\
      template bool intersect<T>(const ray<T,2>& ray, const triangle<T,2>& triangle);\
      template bool intersect<T>(const ray<T,3>& ray, const triangle<T,3>& triangle);\
      template bool intersect<T>(const ray<T,2>& ray, const quadix<T,2>& quadix);\
      template bool intersect<T>(const ray<T,2>& ray, const circle<T>& circle);\
      template bool intersect<T>(const ray<T,3>& ray, const sphere<T>& sphere);\
      template bool intersect<T>(const ray<T,3>& ray, const plane<T,3>& plane);\
      template bool intersect<T>(const ray<T,2>& ray, const polygon<T,2>& polygon);\
      template bool intersect<T>(const plane<T,3>& plane1, const plane<T,3>& plane2);\
      template bool intersect<T>(const plane<T,3>& plane, const sphere<T>& sphere);\
      template bool intersect<T>(const plane<T,3>& plane, const line<T,3>& line);\
      template bool simple_intersect<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4);\
      template bool simple_intersect<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3, const point2d<T>& point4);\
      template bool simple_intersect<T>(const segment<T,2>& segment1, const segment<T,2>& segment2);\
      template bool intersect_vertical_horizontal<T>(const segment<T,2>& segment1, const segment<T,2>& segment2);\
      template bool intersect_vertical_vertical<T>(const segment<T,2>& segment1, const segment<T,2>& segment2);\
      template bool intersect_horizontal_horizontal<T>(const segment<T,2>& segment1, const segment<T,2>& segment2);\
      template void intersection_point<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4, T& ix, T& iy);\
      template void intersection_point<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3, const point2d<T>& point4, T& ix, T& iy);\
      template point2d<T> intersection_point<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3, const point2d<T>& point4);\
      template point2d<T> intersection_point<T>(const segment<T,2>& segment1, const segment<T,2>& segment2);\
      template void intersection_point<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& x4, const T& y4, const T& z4, T& ix, T& iy, T& iz, const T& fuzzy);\
      template void intersection_point<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const point3d<T>& point4, T& ix, T& iy, T& iz, const T& fuzzy);\
      template point3d<T> intersection_point<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const point3d<T>& point4, const T& fuzzy);\
      template point3d<T> intersection_point<T>(const segment<T,3>& segment1, const segment<T,3>& segment2, const T& fuzzy);\
      template point2d<T> intersection_point<T>(const segment<T,2>& segment, const line<T,2>& line);\
      template point3d<T> intersection_point<T>(const segment<T,3>& segment, const line<T,3>& line, const T& fuzzy);\
      template point3d<T> intersection_point<T>(const segment<T,3>& segment, const plane<T,3>& plane);\
      template void intersection_point<T,OutputIterator3d>(const segment<T,3>& segment, const sphere<T>& sphere, OutputIterator3d out);\
      template void intersection_point<T,OutputIterator2d>(const segment<T,2>& segment, const quadratic_bezier<T,2>& bezier, OutputIterator2d out, const std::size_t& steps);\
      template void intersection_point<T,OutputIterator2d>(const segment<T,2>& segment, const cubic_bezier<T,2>& bezier, OutputIterator2d out, const std::size_t& steps);\
      template point2d<T> intersection_point<T>(const line<T,2>& line1, const line<T,2>& line2);\
      template point3d<T> intersection_point<T>(const line<T,3>& line1, const line<T,3>& line2, const T& fuzzy);\
      template void intersection_point<T>(const circle<T>& circle1, const circle<T>& circle2, point2d<T>& point1, point2d<T>& point2);\
      template void intersection_point<T,OutputIterator2d>(const segment<T,2>& segment, const triangle<T,2>& triangle, OutputIterator2d out);\
      template void intersection_point<T>(const line<T,3>& line, const triangle<T,3>& triangle, point3d<T>& ipoint);\
      template point3d<T> intersection_point<T>(const line<T,3>& line, const plane<T,3>& plane);\
      template void intersection_point<T,OutputIterator2d>(const T& x1, const T& y1, const T& x2, const T& y2, const T& cx, const T& cy, const T& radius, OutputIterator2d out);\
      template void intersection_point<T,OutputIterator2d>(const segment<T,2>& segment, const circle<T>& circle, OutputIterator2d out);\
      template void intersection_point<T,OutputIterator2d>(const line<T,2>& line, const circle<T>& circle, OutputIterator2d out);\
      template void intersection_point<T,OutputIterator3d>(const line<T,3>& line, const sphere<T>& sphere, OutputIterator3d out);\
      template point2d<T> intersection_point<T>(const ray<T,2>& ray1, const ray<T,2>& ray2);\
      template point3d<T> intersection_point<T>(const ray<T,3>& ray, const triangle<T,3>& triangle);\
      template point3d<T> intersection_point<T>(const ray<T,3>& ray, const plane<T,3>& plane);\
      template void intersection_point<T,OutputIterator2d>(const ray<T,2>& ray, const circle<T>& circle, OutputIterator2d out);\
      template void intersection_point<T,OutputIterator3d>(const ray<T,3>& ray, const sphere<T>& sphere, OutputIterator3d out);\
      template void intersection_point_line_to_line<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& x4, const T& y4, const T& z4, T& ix, T& iy, T& iz, const T& fuzzy);\
      template T normalize_angle<T>(const T& angle);\
      template T vertical_mirror<T>(const T& angle);\
      template T horizontal_mirror<T>(const T& angle);\
      template unsigned int quadrant<T>(const T& angle);\
      template unsigned int quadrant<T>(const T& x, const T& y);\
      template unsigned int quadrant<T>(const point2d<T>& point);\
      template T vertex_angle<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3);\
      template T vertex_angle<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);\
      template T vertex_angle<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3);\
      template T vertex_angle<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3);\
      template T oriented_vertex_angle<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const int orient);\
      template T oriented_vertex_angle<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3, const int orient);\
      template T cartesian_angle<T>(const T& x, const T& y);\
      template T cartesian_angle<T>(const point2d<T>& point);\
      template T robust_cartesian_angle<T>(const T& x, const T& y);\
      template T robust_cartesian_angle<T>(const point2d<T>& point);\
      template T cartesian_angle<T>(const T& x, const T& y, const T& ox, const T& oy);\
      template T cartesian_angle<T>(const point2d<T>& point, const point2d<T>& origin);\
      template T robust_cartesian_angle<T>(const T& x, const T& y, const T& ox, const T& oy);\
      template T robust_cartesian_angle<T>(const point2d<T>& point, const point2d<T>& origin);\
      template bool parallel<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4, const T& epsilon);\
      template bool parallel<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3, const point2d<T>& point4, const T& epsilon);\
      template bool parallel<T>(const segment<T,2>& segment1, const segment<T,2>& segment2, const T& epsilon);\
      template bool parallel<T>(const line<T,2>& line1, const line<T,2>& line2, const T& epsilon);\
      template bool parallel<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& x4, const T& y4, const T& z4, const T& epsilon);\
      template bool parallel<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const point3d<T>& point4, const T& epsilon);\
      template bool parallel<T>(const segment<T,3>& segment1, const segment<T,3>& segment2, const T& epsilon);\
      template bool parallel<T>(const line<T,3>& line1, const line<T,3>& line2, const T& epsilon);\
      template bool robust_parallel<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4, const T& epsilon);\
      template bool robust_parallel<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3, const point2d<T>& point4, const T& epsilon);\
      template bool robust_parallel<T>(const segment<T,2>& segment1, const segment<T,2>& segment2, const T& epsilon);\
      template bool robust_parallel<T>(const line<T,2>& line1, const line<T,2>& line2, const T& epsilon);\
      template bool robust_parallel<T>(const line<T,2>& line, const segment<T,2>& segment, const T& epsilon);\
      template bool robust_parallel<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& x4, const T& y4, const T& z4, const T& epsilon);\
      template bool robust_parallel<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const point3d<T>& point4, const T& epsilon);\
      template bool robust_parallel<T>(const segment<T,3>& segment1, const segment<T,3>& segment2, const T& epsilon);\
      template bool robust_parallel<T>(const line<T,3>& line1, const line<T,3>& line2, const T& epsilon);\
      template bool robust_parallel<T>(const line<T,3>& line, const segment<T,3>& segment, const T& epsilon);\
      template bool perpendicular<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4, const T& epsilon);\
      template bool perpendicular<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3, const point2d<T>& point4, const T& epsilon);\
      template bool perpendicular<T>(const segment<T,2>& segment1, const segment<T,2>& segment2, const T& epsilon);\
      template bool perpendicular<T>(const line<T,2>& line1, const line<T,2>& line2, const T& epsilon);\
      template bool perpendicular<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& x4, const T& y4, const T& z4, const T& epsilon);\
      template bool perpendicular<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const point3d<T>& point4, const T& epsilon);\
      template bool perpendicular<T>(const segment<T,3>& segment1, const segment<T,3>& segment2, const T& epsilon);\
      template bool perpendicular<T>(const line<T,3>& line1, const line<T,3>& line2, const T& epsilon);\
      template bool perpendicular<T>(const line<T,2>& line, const segment<T,2>& segment, const T& epsilon);\
      template bool robust_perpendicular<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4, const T& epsilon);\
      template bool robust_perpendicular<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3, const point2d<T>& point4, const T& epsilon);\
      template bool robust_perpendicular<T>(const segment<T,2>& segment1, const segment<T,2>& segment2, const T& epsilon);\
      template bool robust_perpendicular<T>(const line<T,2>& line1, const line<T,2>& line2, const T& epsilon);\
      template bool robust_perpendicular<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& x4, const T& y4, const T& z4, const T& epsilon);\
      template bool robust_perpendicular<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const point3d<T>& point4, const T& epsilon);\
      template bool robust_perpendicular<T>(const segment<T,3>& segment1, const segment<T,3>& segment2, const T& epsilon);\
      template bool robust_perpendicular<T>(const line<T,3>& line1, const line<T,3>& line2, const T& epsilon);\
      template bool robust_perpendicular<T>(const line<T,2>& line, const segment<T,2>& segment, const T& epsilon);\
      template bool line_to_line_intersect<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4);\
      template bool line_to_line_intersect<T>(const line<T,2>& line1, const line<T,2>& line2);\
      template bool rectangle_to_rectangle_intersect<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4);\
      template bool rectangle_to_rectangle_intersect<T>(const rectangle<T>& rectangle1, const rectangle<T>& rectangle2);\
      template bool box_to_box_intersect<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& x4, const T& y4, const T& z4);\
      template bool box_to_box_intersect<T>(const box<T,3>& box1, const box<T,3>& box2);\
      template bool rectangle_within_rectangle<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4);\
      template bool rectangle_within_rectangle<T>(const rectangle<T>& rectangle1, const rectangle<T>& rectangle2);\
      template bool box_within_box<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& x4, const T& y4, const T& z4);\
      template bool box_within_box<T>(const box<T,3>& box1, const box<T,3>& box2);\
      template bool circle_within_rectangle<T>(const T& x, const T& y, const T& radius, const T& x1, const T& y1, const T& x2, const T& y2);\
      template bool circle_within_rectangle<T>(const circle<T>& circle, const rectangle<T>& rectangle);\
      template bool triangle_within_rectangle<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4, const T& x5, const T& y5);\
      template bool triangle_within_rectangle<T>(const triangle<T,2>& triangle, const rectangle<T>& rectangle);\
      template bool segment_within_rectangle<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4);\
      template bool segment_within_rectangle<T>(const segment<T,2>& segment, const rectangle<T>& rectangle);\
      template bool quadix_within_rectangle<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4, const T& x5, const T& y5, const T& x6, const T& y6);\
      template bool quadix_within_rectangle<T>(const quadix<T,2>& quadix, const rectangle<T>& rectangle);\
      template bool polygon_within_rectangle<T>(const polygon<T,2>& polygon, const rectangle<T>& rectangle);\
      template bool sphere_within_box<T>(const T& x, const T& y, const T& z, const T& radius, const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2);\
      template bool sphere_within_box<T>(const sphere<T>& sphere, const box<T,3>& box);\
      template bool triangle_within_box<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& x4, const T& y4, const T& z4, const T& x5, const T& y5, const T& z5);\
      template bool triangle_within_box<T>(const triangle<T,3>& triangle, const box<T,3>& box);\
      template bool segment_within_box<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& x4, const T& y4, const T& z4) ;\
      template bool segment_within_box<T>(const segment<T,3>& segment, const box<T,3>& box);\
      template bool quadix_within_box<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& x4, const T& y4, const T& z4, const T& x5, const T& y5, const T& z5, const T& x6, const T& y6, const T& z6);\
      template bool quadix_within_box<T>(const quadix<T,3>& quadix, const box<T,3>& box);\
      template bool polygon_within_box<T>(const polygon<T,3>& polygon, const box<T,3>& box);\
      template bool circle_in_circle<T>(const circle<T>& circle1, const circle<T>& circle2);\
      template bool is_tangent<T>(const segment<T,2>& segment, const circle<T>& circle);\
      template bool point_of_reflection<T>(const T& sx1, const T& sy1, const T& sx2, const T& sy2, const T& p1x, const T& p1y, const T& p2x, const T& p2y, T& rpx, T& rpy);\
      template bool point_of_reflection<T>(const segment<T,2>& segment, const point2d<T>&point1, const point2d<T>&point2, point2d<T>&reflection_point);\
      template segment<T,2> edge<T>(const triangle<T,2>& triangle, const std::size_t& edge_index);\
      template segment<T,3> edge<T>(const triangle<T,3>& triangle, const std::size_t& edge_index);\
      template segment<T,2> edge<T>(const quadix<T,2>& quadix, const std::size_t& edge_index);\
      template segment<T,3> edge<T>(const quadix<T,3>& quadix, const std::size_t& edge_index);\
      template segment<T,2> edge<T>(const rectangle<T>& rectangle, const std::size_t& edge);\
      template segment<T,2> edge<T>(const polygon<T,2>& polygon, const std::size_t& edge);\
      template segment<T,3> edge<T>(const polygon<T,3>& polygon, const std::size_t& edge);\
      template segment<T,2> opposing_edge<T>(const triangle<T,2>& triangle, const std::size_t& corner);\
      template segment<T,3> opposing_edge<T>(const triangle<T,3>& triangle, const std::size_t& corner);\
      template segment<T,2> reverse_segment<T>(const segment<T,2>& segment);\
      template segment<T,3> reverse_segment<T>(const segment<T,3>& segment);\
      template point2d<T> rectangle_corner<T>(const rectangle<T>& rectangle, const std::size_t& corner_index);\
      template point3d<T> box_corner<T>(const box<T,3>& box, const std::size_t& corner_index);\
      template line<T,2> triangle_bisector<T>(const triangle<T,2>& triangle, const std::size_t& bisector);\
      template line<T,3> triangle_bisector<T>(const triangle<T,3>& triangle, const std::size_t& bisector);\
      template line<T,2> triangle_external_bisector<T>(const triangle<T,2>& triangle, const std::size_t& corner, const std::size_t& opposing_corner);\
      template line<T,3> triangle_external_bisector<T>(const triangle<T,3>& triangle, const std::size_t& corner, const std::size_t& opposing_corner);\
      template line<T,2> triangle_median<T>(const triangle<T,2>& triangle, const std::size_t& median);\
      template line<T,3> triangle_median<T>(const triangle<T,3>& triangle, const std::size_t& median);\
      template line<T,2> triangle_symmedian<T>(const triangle<T,2>& triangle, const std::size_t& symmedian);\
      template line<T,3> triangle_symmedian<T>(const triangle<T,3>& triangle, const std::size_t& symmedian);\
      template line<T,2> euler_line<T>(const triangle<T,2>& triangle);\
      template line<T,3> euler_line<T>(const triangle<T,3>& triangle);\
      template point2d<T> exmedian_point<T>(const triangle<T,2>& triangle, const std::size_t& corner);\
      template point3d<T> exmedian_point<T>(const triangle<T,3>& triangle, const std::size_t& corner);\
      template point2d<T> feuerbach_point<T>(const triangle<T,2>& triangle);\
      template line<T,2> confined_triangle_median<T>(const triangle<T,2>& triangle,const point2d<T>& point, const std::size_t& median);\
      template line<T,3> confined_triangle_median<T>(const triangle<T,3>& triangle,const point3d<T>& point, const std::size_t& median);\
      template line<T,2> create_parallel_line_on_point<T>(const line<T,2>& line, const point2d<T>& point);\
      template line<T,3> create_parallel_line_on_point<T>(const line<T,3>& line, const point3d<T>& point);\
      template segment<T,2> create_parallel_segment_on_point<T>(const line<T,2>& line, const point2d<T>& point);\
      template segment<T,3> create_parallel_segment_on_point<T>(const line<T,3>& line, const point3d<T>& point);\
      template bool point_in_rectangle<T>(const T& px, const T& py, const T& x1, const T& y1, const T& x2, const T& y2);\
      template bool point_in_rectangle<T>(const point2d<T>& point, const T& x1, const T& y1, const T& x2, const T& y2);\
      template bool point_in_rectangle<T>(const T& px, const T& py, const rectangle<T>& rectangle);\
      template bool point_in_rectangle<T>(const point2d<T>& point, const rectangle<T>& rectangle);\
      template bool point_in_rectangle<T>(const point2d<T>& point, const point2d<T>& rect_point1, point2d<T>& rect_point2);\
      template bool point_in_rectangle<T>(const point2d<T>& point, const segment<T,2>& segment);\
      template bool point_in_box<T>(const T& px, const T& py, const T& pz, const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2);\
      template bool point_in_box<T>(const point3d<T>& point, const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2);\
      template bool point_in_box<T>(const T& px, const T& py, const T& pz, const box<T,3>& box);\
      template bool point_in_box<T>(const point3d<T>& point, const box<T,3>& box);\
      template bool point_in_box<T>(const point3d<T>& point, const point3d<T>& box_point1, const point3d<T>& box_point2);\
      template bool point_in_box<T>(const point3d<T>& point, const segment<T,3>& segment);\
      template bool point_in_triangle<T>(const T& px, const T& py, const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3);\
      template bool point_in_triangle<T>(const point2d<T>& point, const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);\
      template bool point_in_triangle<T>(const T& px, const T& py, const triangle<T,2>& triangle);\
      template bool point_in_triangle<T>(const point2d<T>& point, const triangle<T,2>& triangle);\
      template bool point_in_quadix<T>(const T& px, const T& py, const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4);\
      template bool point_in_quadix<T>(const point2d<T>& point, const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3, const point2d<T>& point4);\
      template bool point_in_quadix<T>(const T& px, const T& py, const quadix<T,2>& quadix);\
      template bool point_in_quadix<T>(const point2d<T>& point, const quadix<T,2>& quadix);\
      template bool point_in_circle<T>(const T& px, const T& py, const T& cx, const T& cy, const T& radius);\
      template bool point_in_circle<T>(const T& px, const T& py, const circle<T>& circle);\
      template bool point_in_circle<T>(const point2d<T>& point, const circle<T>& circle);\
      template bool point_in_sphere<T>(const T& px, const T& py, const T& pz, const T& cx, const T& cy, const T& cz, const T& radius);\
      template bool point_in_sphere<T>(const T& px, const T& py, const T& pz, const sphere<T>& sphere);\
      template bool point_in_sphere<T>(const point3d<T>& point, const sphere<T>& sphere);\
      template bool point_in_three_point_circle<T>(const T& px, const T& py, const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3);\
      template bool point_in_three_point_circle<T>(const point2d<T>& point, const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);\
      template bool point_in_three_point_circle<T>(const point2d<T>& point, const triangle<T,2> triangle);\
      template bool point_in_focus_area<T>(const T& px, const T& py, const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3);\
      template bool point_in_focus_area<T>(const point2d<T>& point, const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);\
      template bool point_on_segment<T>(const point2d<T>& point, const segment<T,2>& segment);\
      template bool point_on_segment<T>(const point3d<T>& point, const segment<T,3>& segment);\
      template bool point_on_ray<T>(const T& px, const T& py, const T& ox, const T& oy, const T& dx, const T& dy);\
      template bool point_on_ray<T>(const T& px, const T& py, const T& pz, const T& ox, const T& oy, const T& oz, const T& dx, const T& dy, const T& dz);\
      template bool point_on_ray<T>(const point2d<T>& point, const ray<T,2>& ray);\
      template bool point_on_ray<T>(const point3d<T>& point, const ray<T,3>& ray);\
      template bool point_on_rectangle<T>(const T& px, const T& py, const T& x1, const T& y1, const T& x2, const T& y2);\
      template bool point_on_rectangle<T>(const point2d<T>& point, const T& x1, const T& y1, const T& x2, const T& y2);\
      template bool point_on_rectangle<T>(const T& px, const T& py, const rectangle<T>& rectangle);\
      template bool point_on_rectangle<T>(const point2d<T>& point, const rectangle<T>& rectangle);\
      template bool point_on_triangle<T>(const T& px, const T& py, const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3);\
      template bool point_on_triangle<T>(const point2d<T>& point, const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);\
      template bool point_on_triangle<T>(const T& px, const T& py, const triangle<T,2>& triangle);\
      template bool point_on_triangle<T>(const point2d<T>& point, const triangle<T,2>& triangle);\
      template bool point_on_quadix<T>(const T& px, const T& py, const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4);\
      template bool point_on_quadix<T>(const point2d<T>& point, const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3, const point2d<T>& point4);\
      template bool point_on_quadix<T>(const T& px, const T& py, const quadix<T,2>& quadix);\
      template bool point_on_quadix<T>(const point2d<T>& point, const quadix<T,2>& quadix);\
      template bool point_on_circle<T>(const T& px, const T& py, const T& cx, const T& cy, const T& radius);\
      template bool point_on_circle<T>(const T& px, const T& py, const circle<T>& circle);\
      template bool point_on_circle<T>(const point2d<T>& point, const circle<T>& circle);\
      template bool point_on_bezier<T>(const point2d<T>& point, const quadratic_bezier<T,2>& bezier, const std::size_t& steps, const T& fuzzy);\
      template bool point_on_bezier<T>(const point2d<T>& point, const cubic_bezier<T,2>& bezier, const std::size_t& steps, const T& fuzzy);\
      template bool point_on_bezier<T>(const point3d<T>& point, const quadratic_bezier<T,3>& bezier, const std::size_t& steps, const T& fuzzy);\
      template bool point_on_bezier<T>(const point3d<T>& point, const cubic_bezier<T,3>& bezier, const std::size_t& steps, const T& fuzzy);\
      template point2d<T> isogonal_conjugate<T>(const point2d<T>& point, const triangle<T,2>& triangle);\
      template point3d<T> isogonal_conjugate<T>(const point3d<T>& point, const triangle<T,3>& triangle);\
      template point2d<T> cyclocevian_conjugate<T>(const point2d<T>& point, const triangle<T,2>& triangle);\
      template point2d<T> symmedian_point<T>(const triangle<T,2>& triangle);\
      template point3d<T> symmedian_point<T>(const triangle<T,3>& triangle);\
      template void create_equilateral_triangle<T>(const T& x1, const T& y1, const T& x2, const T& y2, T& x3, T& y3);\
      template void create_equilateral_triangle<T>(const point2d<T>& point1, const point2d<T>& point2, point2d<T>& point3);\
      template triangle<T,2> create_equilateral_triangle<T>(const T& x1, const T& y1, const T& x2, const T& y2);\
      template triangle<T,2> create_equilateral_triangle<T>(const point2d<T>& point1, const point2d<T>& point2);\
      template triangle<T,2> create_equilateral_triangle<T>(const T& cx, const T& cy, const T& side_length);\
      template triangle<T,2> create_equilateral_triangle<T>(const point2d<T>& center_point, const T& side_length);\
      template triangle<T,2> create_isosceles_triangle<T>(const point2d<T>& point1, const point2d<T>& point2, const T& angle);\
      template triangle<T,2> create_isosceles_triangle<T>(const segment<T,2>& segment, const T& angle);\
      template triangle<T,2> create_triangle<T>(const point2d<T>& point1, const point2d<T>& point2, const T& angle1, const T& angle2);\
      template triangle<T,2> create_triangle<T>(const segment<T,2>& segment, const T& angle1, const T& angle2);\
      template triangle<T,2> create_morley_triangle<T>(const triangle<T,2>& triangle);\
      template triangle<T,2> create_cevian_triangle<T>(const triangle<T,2>& triangle, const point2d<T>& point);\
      template triangle<T,3> create_cevian_triangle<T>(const triangle<T,3>& triangle, const point3d<T>& point);\
      template triangle<T,2> create_anticevian_triangle<T>(const triangle<T,2>& triangle, const point2d<T>& point);\
      template triangle<T,3> create_anticevian_triangle<T>(const triangle<T,3>& triangle, const point3d<T>& point);\
      template triangle<T,2> create_anticomplementary_triangle<T>(const triangle<T,2>& triangle);\
      template triangle<T,3> create_anticomplementary_triangle<T>(const triangle<T,3>& triangle);\
      template triangle<T,2> create_inner_napoleon_triangle<T>(const triangle<T,2>& triangle);\
      template triangle<T,2> create_outer_napoleon_triangle<T>(const triangle<T,2>& triangle);\
      template triangle<T,2> create_inner_vecten_triangle<T>(const triangle<T,2>& triangle);\
      template triangle<T,2> create_outer_vecten_triangle<T>(const triangle<T,2>& triangle);\
      template triangle<T,2> create_medial_triangle<T>(const triangle<T,2>& triangle);\
      template triangle<T,3> create_medial_triangle<T>(const triangle<T,3>& triangle);\
      template triangle<T,2> create_contact_triangle<T>(const triangle<T,2>& triangle);\
      template triangle<T,3> create_contact_triangle<T>(const triangle<T,3>& triangle);\
      template triangle<T,2> create_symmedial_triangle<T>(const triangle<T,2>& triangle, const point2d<T>& point);\
      template triangle<T,2> create_orthic_triangle<T>(const triangle<T,2>& triangle);\
      template triangle<T,3> create_orthic_triangle<T>(const triangle<T,3>& triangle);\
      template triangle<T,2> create_pedal_triangle<T>(const point2d<T>& point, const triangle<T,2>& triangle);\
      template triangle<T,3> create_pedal_triangle<T>(const point3d<T>& point, const triangle<T,3>& triangle);\
      template triangle<T,2> create_antipedal_triangle<T>(const point2d<T>& point, const triangle<T,2>& triangle);\
      template triangle<T,2> create_excentral_triangle<T>(const triangle<T,2>& triangle);\
      template triangle<T,3> create_excentral_triangle<T>(const triangle<T,3>& triangle);\
      template triangle<T,2> create_incentral_triangle<T>(const triangle<T,2>& triangle);\
      template triangle<T,3> create_incentral_triangle<T>(const triangle<T,3>& triangle);\
      template triangle<T,2> create_intouch_triangle<T>(const triangle<T,2>& triangle);\
      template triangle<T,2> create_extouch_triangle<T>(const triangle<T,2>& triangle);\
      template triangle<T,3> create_extouch_triangle<T>(const triangle<T,3>& triangle);\
      template triangle<T,2> create_feuerbach_triangle<T>(const triangle<T,2>& triangle);\
      template triangle<T,2> create_circumcevian_triangle<T>(const triangle<T,2>& triangle, const point2d<T>& point);\
      template triangle<T,2> create_circummedial_triangle<T>(const triangle<T,2>& triangle);\
      template triangle<T,2> create_first_brocard_triangle<T>(const triangle<T,2>& triangle);\
      template void create_right_triangle<T>(const wykobi::point2d<T>& p1, const wykobi::point2d<T>& p2, wykobi::point2d<T>& c1, wykobi::point2d<T>& c2);\
      template void create_equilateral_quadix<T>(const T& x1, const T& y1, const T& x2, const T& y2, T& x3, T& y3, T& x4, T& y4);\
      template void create_equilateral_quadix<T>(const point2d<T>& point1, const point2d<T>& point2, point2d<T>& point3, point2d<T>& point4);\
      template quadix<T,2> create_equilateral_quadix<T>(const T& x1, const T& y1, const T& x2, const T& y2);\
      template quadix<T,2> create_equilateral_quadix<T>(const point2d<T>& point1, const point2d<T>& point2);\
      template quadix<T,2> create_equilateral_quadix<T>(const segment<T,2>& segment);\
      template quadix<T,2> create_equilateral_quadix<T>(const T& cx, const T& cy, const T& side_length);\
      template quadix<T,2> create_equilateral_quadix<T>(const point2d<T>& center_point, const T& side_length);\
      template void torricelli_point<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, T& px, T& py);\
      template point2d<T> torricelli_point<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);\
      template point2d<T> torricelli_point<T>(const triangle<T,2>& triangle);\
      template bool trilateration<T>(const T& c0x, const T& c0y, const T& c0r, const T& c1x, const T& c1y, const T& c1r, const T& c2x, const T& c2y, const T& c2r, T& px, T& py);\
      template point2d<T> trilateration<T>(const circle<T>& c0, const circle<T>& c1, const circle<T>& c2);\
      template void incenter<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, T& px, T& py);\
      template void incenter<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, T& px, T& py, T& pz);\
      template point2d<T> incenter<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);\
      template point3d<T> incenter<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3);\
      template point2d<T> incenter<T>(const triangle<T,2>& triangle);\
      template point3d<T> incenter<T>(const triangle<T,3>& triangle);\
      template void circumcenter<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, T& px, T& py);\
      template void circumcenter<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, T& px, T& py, T& pz);\
      template point2d<T> circumcenter<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);\
      template point3d<T> circumcenter<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3);\
      template point2d<T> circumcenter<T>(const triangle<T,2>& triangle);\
      template point3d<T> circumcenter<T>(const triangle<T,3>& triangle);\
      template circle<T> circumcircle<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3);\
      template circle<T> circumcircle<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);\
      template circle<T> circumcircle<T>(const triangle<T,2>& triangle);\
      template sphere<T> circumsphere<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3);\
      template sphere<T> circumsphere<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3);\
      template sphere<T> circumsphere<T>(const triangle<T,3>& triangle);\
      template circle<T> inscribed_circle<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3);\
      template circle<T> inscribed_circle<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);\
      template circle<T> inscribed_circle<T>(const triangle<T,2>& triangle);\
      template sphere<T> inscribed_sphere<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3);\
      template sphere<T> inscribed_sphere<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3);\
      template sphere<T> inscribed_sphere<T>(const triangle<T,3>& triangle);\
      template circle<T> nine_point_circle<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3);\
      template circle<T> nine_point_circle<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);\
      template circle<T> nine_point_circle<T>(const triangle<T,2>& triangle);\
      template point2d<T> orthocenter<T>(const triangle<T,2>& triangle);\
      template point3d<T> orthocenter<T>(const triangle<T,3>& triangle);\
      template point2d<T> excenter<T>(const triangle<T,2>& triangle, const std::size_t& corner);\
      template point3d<T> excenter<T>(const triangle<T,3>& triangle, const std::size_t& corner);\
      template circle<T> excircle<T>(const triangle<T,2>& triangle, const std::size_t& corner);\
      template circle<T> mandart_circle<T>(const triangle<T,2>& triangle);\
      template circle<T> brocard_circle<T>(const triangle<T,2>& triangle);\
      template circle<T> invert_circle_across_circle<T>(const circle<T>& circle1, const circle<T>& circle2);\
      template sphere<T> invert_sphere_across_sphere<T>(const sphere<T>& sphere1, const sphere<T>& sphere2);\
      template void circle_tangent_points<T>(const circle<T>& circle, const point2d<T>& point, point2d<T>& point1, point2d<T>& point2);\
      template line<T,2> tangent_line<T>(const circle<T>& circle, const point2d<T>& point);\
      template line<T,2> create_line_from_bisector<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3);\
      template segment<T,2> create_segment_from_bisector<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3);\
      template line<T,3> create_line_from_bisector<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3);\
      template segment<T,3> create_segment_from_bisector<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3);\
      template line<T,2> create_line_from_bisector<T>(const point2d<T>& point1,const point2d<T>& point2,const point2d<T>& point3);\
      template segment<T,2> create_segment_from_bisector<T>(const point2d<T>& point1,const point2d<T>& point2,const point2d<T>& point3);\
      template ray<T,2> create_ray_from_bisector<T>(const point2d<T>& point1,const point2d<T>& point2,const point2d<T>& point3);\
      template line<T,3> create_line_from_bisector<T>(const point3d<T>& point1,const point3d<T>& point2,const point3d<T>& point3);\
      template segment<T,3> create_segment_from_bisector<T>(const point3d<T>& point1,const point3d<T>& point2,const point3d<T>& point3);\
      template ray<T,3> create_ray_from_bisector<T>(const point3d<T>& point1,const point3d<T>& point2,const point3d<T>& point3);\
      template line<T,2> create_perpendicular_bisector<T>(const T& x1, const T& y1,const T& x2, const T& y2);\
      template line<T,2> create_perpendicular_bisector<T>(const point2d<T>& point1, const point2d<T>& point2);\
      template line<T,2> create_perpendicular_bisector<T>(const segment<T,2>& segment);\
      template line<T,2> create_perpendicular_line_at_end_point<T>(const line<T,2>& line);\
      template void closest_point_on_segment_from_point<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& px, const T& py, T& nx, T& ny);\
      template void closest_point_on_segment_from_point<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& px, const T& py, const T& pz, T& nx, T& ny, T& nz);\
      template void closest_point_on_line_from_point<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& px, const T& py, T& nx, T& ny);\
      template void closest_point_on_line_from_point<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& px, const T& py, const T& pz, T& nx, T& ny, T& nz);\
      template void order_sensitive_closest_point_on_segment_from_point<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& px, const T& py, T& nx, T& ny);\
      template void order_sensitive_closest_point_on_segment_from_point<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& px, const T& py, const T& pz, T& nx, T& ny, T& nz);\
      template void order_sensitive_closest_point_on_line_from_point<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& px, const T& py, T& nx, T& ny);\
      template void order_sensitive_closest_point_on_line_from_point<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& px, const T& py, const T& pz, T& nx, T& ny, T& nz);\
      template void closest_point_on_ray_from_point<T>(const T& ox, const T& oy, const T& dx, const T& dy, const T& px, const T& py, T& nx, T& ny);\
      template void closest_point_on_ray_from_point<T>(const T& ox, const T& oy, const T& oz, const T& dx, const T& dy, const T& dz, const T& px, const T& py, const T& pz, T& nx, T& ny, T& nz);\
      template point2d<T> closest_point_on_segment_from_point<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& px, const T& py);\
      template point3d<T> closest_point_on_segment_from_point<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& px, const T& py, const T& pz);\
      template point2d<T> closest_point_on_segment_from_point<T>(const segment<T,2>& segment, const point2d<T>& point);\
      template point3d<T> closest_point_on_segment_from_point<T>(const segment<T,3>& segment, const point3d<T>& point);\
      template point2d<T> closest_point_on_line_from_point<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& px, const T& py);\
      template point3d<T> closest_point_on_line_from_point<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& px, const T& py, const T& pz);\
      template point2d<T> closest_point_on_line_from_point<T>(const line<T,2>& line, const point2d<T>& point);\
      template point3d<T> closest_point_on_line_from_point<T>(const line<T,3>& line, const point3d<T>& point);\
      template point2d<T> closest_point_on_ray_from_point<T>(const T& ox, const T& oy, const T& dx, const T& dy, const T& px, const T& py);\
      template point3d<T> closest_point_on_ray_from_point<T>(const T& ox, const T& oy, const T& oz, const T& dx, const T& dy, const T& dz, const T& px, const T& py, const T& pz);\
      template point2d<T> closest_point_on_ray_from_point<T>(const ray<T,2>& ray, const point2d<T>& point);\
      template point3d<T> closest_point_on_ray_from_point<T>(const ray<T,3>& ray, const point3d<T>& point);\
      template void closest_point_on_triangle_from_point<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& px, const T& py, T& nx, T& ny);\
      template point2d<T> closest_point_on_triangle_from_point<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& px, const T& py);\
      template point2d<T> closest_point_on_triangle_from_point<T>(const triangle<T,2>& triangle, const T& px, const T& py);\
      template point2d<T> closest_point_on_triangle_from_point<T>(const triangle<T,2>& triangle, const point2d<T>& point);\
      template void closest_point_on_triangle_from_point<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& px, const T& py, const T& pz, T& nx, T& ny, T& nz);\
      template point3d<T> closest_point_on_triangle_from_point<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& px, const T& py, const T& pz);\
      template point3d<T> closest_point_on_triangle_from_point<T>(const triangle<T,3>& triangle, const T& px, const T& py, const T& pz);\
      template point3d<T> closest_point_on_triangle_from_point<T>(const triangle<T,3>& triangle, const point3d<T>& point);\
      template void closest_point_on_rectangle_from_point<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& px, const T& py, T& nx, T& ny);\
      template point2d<T> closest_point_on_rectangle_from_point<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& px, const T& py);\
      template point2d<T> closest_point_on_rectangle_from_point<T>(const rectangle<T>& rectangle, const T& px, const T& py);\
      template point2d<T> closest_point_on_rectangle_from_point<T>(const rectangle<T>& rectangle, const point2d<T>& point);\
      template void closest_point_on_box_from_point<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& px, const T& py, const T& pz, T& nx, T& ny, T& nz);\
      template point3d<T> closest_point_on_box_from_point<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& px, const T& py, const T& pz);\
      template point3d<T> closest_point_on_box_from_point<T>(const box<T,3>& box, const T& px, const T& py, const T& pz);\
      template point3d<T> closest_point_on_box_from_point<T>(const box<T,3>& box, const point3d<T>& point);\
      template void closest_point_on_quadix_from_point<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4, const T& px, const T& py, T& nx, T& ny);\
      template point2d<T> closest_point_on_quadix_from_point<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4, const T& px, const T& py);\
      template point2d<T> closest_point_on_quadix_from_point<T>(const quadix<T,2>& quadix, const point2d<T>& point);\
      template point2d<T> closest_point_on_circle_from_point<T>(const circle<T>& circle, const point2d<T>& point);\
      template point3d<T> closest_point_on_sphere_from_point<T>(const sphere<T>& sphere, const point3d<T>& point);\
      template point2d<T> closest_point_on_aabbb_from_point<T>(const rectangle<T>& rectangle, const point2d<T>&point);\
      template point2d<T> closest_point_on_circle_from_segment<T>(const circle<T>& circle, const segment<T,2>& segment);\
      template point3d<T> closest_point_on_sphere_from_segment<T>(const sphere<T>& sphere, const segment<T,3>& segment);\
      template point3d<T> closest_point_on_plane_from_point<T>(const plane<T,3>& plane, const point3d<T>& point);\
      template point2d<T> closest_point_on_bezier_from_point<T>(const quadratic_bezier<T,2>& bezier, const point2d<T>& point, const std::size_t& steps);\
      template point2d<T> closest_point_on_bezier_from_point<T>(const cubic_bezier<T,2>& bezier, const point2d<T>& point, const std::size_t& steps);\
      template point3d<T> closest_point_on_bezier_from_point<T>(const quadratic_bezier<T,3>& bezier, const point3d<T>& point, const std::size_t& steps);\
      template point3d<T> closest_point_on_bezier_from_point<T>(const cubic_bezier<T,3>& bezier, const point3d<T>& point, const std::size_t& steps);\
      template point2d<T> closest_point_on_circle_from_circle<T>(const circle<T>& circle1, const circle<T>& circle2);\
      template point3d<T> closest_point_on_sphere_from_sphere<T>(const sphere<T>& sphere1, const sphere<T>& sphere2);\
      template point2d<T> closest_point_on_polygon_from_point<T>(const polygon<T,2>& polygon, const point2d<T>& point);\
      template T minimum_distance_from_point_to_segment<T>(const T& px, const T& py, const T& x1, const T& y1, const T& x2, const T& y2);\
      template T minimum_distance_from_point_to_segment<T>(const T& px, const T& py, const T& pz, const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2);\
      template T minimum_distance_from_point_to_segment<T>(const point2d<T>& point, const segment<T,2>& segment);\
      template T minimum_distance_from_point_to_segment<T>(const point3d<T>& point, const segment<T,3>& segment);\
      template T minimum_distance_from_point_to_line<T>(const T& px, const T& py, const T& x1, const T& y1, const T& x2, const T& y2);\
      template T minimum_distance_from_point_to_line<T>(const T& px, const T& py, const T& pz, const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2);\
      template T minimum_distance_from_point_to_line<T>(const point2d<T>& point, const line<T,2>& line);\
      template T minimum_distance_from_point_to_line<T>(const point3d<T>& point, const line<T,3>& line);\
      template T minimum_distance_from_point_to_triangle<T>(const T& px, const T& py, const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3);\
      template T minimum_distance_from_point_to_triangle<T>(const point2d<T>& point, const triangle<T,2>& triangle);\
      template T minimum_distance_from_point_to_rectangle<T>(const T& px, const T& py, const T& x1, const T& y1, const T& x2, const T& y2);\
      template T minimum_distance_from_point_to_rectangle<T>(const point2d<T>& point, const rectangle<T>& rectangle);\
      template void segment_mid_point<T>(const T&x1, const T&y1, const T&x2, const T&y2, T& midx, T& midy);\
      template void segment_mid_point<T>(const segment<T,2>& segment, T& midx, T& midy);\
      template point2d<T> segment_mid_point<T>(const point2d<T>& point1, const point2d<T>& point2);\
      template point2d<T> segment_mid_point<T>(const segment<T,2>& segment);\
      template void segment_mid_point<T>(const T&x1, const T&y1, const T&z1, const T&x2, const T&y2, const T&z2, T& midx, T& midy, T& midz);\
      template void segment_mid_point<T>(const segment<T,3>& segment, T& midx, T& midy, T& midz);\
      template point3d<T> segment_mid_point<T>(const point3d<T>& point1, const point3d<T>& point2);\
      template point3d<T> segment_mid_point<T>(const segment<T,3>& segment);\
      template void centroid<T>(const T& x1, const T& y1, const T& x2, const T& y2, T& x, T& y);\
      template void centroid<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, T& x, T& y, T& z);\
      template point2d<T> centroid<T>(const point2d<T>& point1, const point2d<T>& point2);\
      template point2d<T> centroid<T>(const segment<T,2>& segment);\
      template void centroid<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, T& x, T& y);\
      template void centroid<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4, T& x, T& y);\
      template void centroid<T>(const triangle<T,2>& triangle, T& x, T& y);\
      template void centroid<T>(const triangle<T,3>& triangle, T& x, T& y,T& z);\
      template void centroid<T>(const quadix<T,2>& quadix, T& x, T& y);\
      template void centroid<T>(const rectangle<T>& rectangle, T& x, T& y);\
      template void centroid<T>(const box<T,3>& box, T& x, T& y, T& z);\
      template void centroid<T>(const polygon<T,2>& polygon, T& x, T& y);\
      template point2d<T> centroid<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);\
      template point2d<T> centroid<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3, const point2d<T>& point4);\
      template point2d<T> centroid<T>(const triangle<T,2>& triangle);\
      template point3d<T> centroid<T>(const triangle<T,3>& triangle);\
      template point2d<T> centroid<T>(const quadix<T,2>& quadix);\
      template point2d<T> centroid<T>(const rectangle<T>& rectangle);\
      template point3d<T> centroid<T>(const box<T,3>& box);\
      template point2d<T> centroid<T>(const polygon<T,2>& polygon);\
      template bool point_in_convex_polygon<T>(const T& px, const T& py, const polygon<T,2>& polygon);\
      template bool point_in_convex_polygon<T>(const point2d<T>& point, const polygon<T,2>& polygon);\
      template bool point_on_polygon_edge<T>(const T& px, const T& py, const polygon<T,2>& polygon);\
      template bool point_on_polygon_edge<T>(const point2d<T>& point, const polygon<T,2>& polygon);\
      template bool point_in_polygon<T>(const T& px, const T& py, const polygon<T,2>& polygon);\
      template bool point_in_polygon<T>(const point2d<T>& point, const polygon<T,2>& polygon);\
      template bool point_in_polygon_winding_number<T>(const T& px, const T& py, const polygon<T,2>& polygon);\
      template bool point_in_polygon_winding_number<T>(const point2d<T>& point, const polygon<T,2>& polygon);\
      template bool convex_quadix<T>(const quadix<T,2>& quadix);\
      template bool convex_quadix<T>(const quadix<T,3>& quadix);\
      template bool is_convex_polygon<T>(const polygon<T,2>& polygon);\
      template polygon<T,2> remove_consecutive_collinear_points<T>(const polygon<T,2>& polygon);\
      template void remove_consecutive_collinear_points<T,InputIterator2d,OutputIterator2d>(const InputIterator2d begin, const InputIterator2d end, OutputIterator2d out);\
      template bool convex_vertex<T>(const std::size_t& index, const polygon<T,2>& polygon, const int& polygon_orientation);\
      template bool collinear_vertex<T>(const std::size_t& index, const polygon<T,2>& polygon);\
      template bool vertex_is_ear<T>(const std::size_t& index, const polygon<T,2>& polygon);\
      template triangle<T,2> vertex_triangle<T>(const std::size_t& index, const polygon<T,2> polygon);\
      template int polygon_orientation<T>(const polygon<T,2>& polygon);\
      template bool is_equilateral_triangle<T>(const triangle<T,2>& triangle);\
      template bool is_equilateral_triangle<T>(const triangle<T,3>& triangle);\
      template bool is_isosceles_triangle<T>(const triangle<T,2>& triangle);\
      template bool is_isosceles_triangle<T>(const triangle<T,3>& triangle);\
      template bool is_right_triangle<T>(const wykobi::triangle<T,2>& triangle);\
      template bool is_right_triangle<T>(const wykobi::triangle<T,3>& triangle);\
      template bool are_perspective_triangles<T>(const triangle<T,2>& triangle1, const triangle<T,2>& triangle2);\
      template bool are_perspective_triangles<T>(const triangle<T,3>& triangle1, const triangle<T,3>& triangle2);\
      template line<T,2> perspectrix<T>(const triangle<T,2>& triangle1, const triangle<T,2>& triangle2);\
      template line<T,3> perspectrix<T>(const triangle<T,3>& triangle1, const triangle<T,3>& triangle2);\
      template void mirror<T>(const T& px, const T& py, const T& x1, const T& y1, const T& x2, const T& y2, T& nx, T& ny);\
      template void mirror<T>(const T& px, const T& py, const T& pz, const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, T& nx, T& ny, T& nz);\
      template point2d<T> mirror<T>(const point2d<T>& point, const line<T,2>& mirror_axis);\
      template segment<T,2> mirror<T>(const segment<T,2>& segment, const line<T,2>& mirror_axis);\
      template line<T,2> mirror<T>(const line<T,2>& line, const wykobi::line<T,2>& mirror_axis);\
      template rectangle<T> mirror<T>(const rectangle<T>& rectangle, const line<T,2>& mirror_axis);\
      template triangle<T,2> mirror<T>(const triangle<T,2>& triangle, const line<T,2>& mirror_axis);\
      template quadix<T,2> mirror<T>(const quadix<T,2>& quadix, const line<T,2>& mirror_axis);\
      template circle<T> mirror<T>(const circle<T>& circle, const line<T,2>& mirror_axis);\
      template polygon<T,2> mirror<T>(const polygon<T,2>& polygon, const line<T,2>& mirror_axis);\
      template point3d<T> mirror<T>(const point3d<T>& point, const line<T,3>& mirror_line);\
      template segment<T,3> mirror<T>(const segment<T,3>& segment, const line<T,3>& mirror_axis);\
      template line<T,3> mirror<T>(const line<T,3>& line, const wykobi::line<T,3>& mirror_axis);\
      template box<T,3> mirror<T>(const box<T,3>& box, const line<T,3>& mirror_axis);\
      template triangle<T,3> mirror<T>(const triangle<T,3>& triangle, const line<T,3>& mirror_axis);\
      template quadix<T,3> mirror<T>(const quadix<T,3>& quadix, const line<T,3>& mirror_axis);\
      template sphere<T> mirror<T>(const sphere<T>& sphere, const line<T,3>& mirror_axis);\
      template polygon<T,3> mirror<T>(const polygon<T,3>& polygon, const line<T,3>& mirror_axis);\
      template point3d<T> mirror<T>(const point3d<T>& point, const plane<T,3>& plane);\
      template segment<T,3> mirror<T>(const segment<T,3>& segment, const plane<T,3>& mirror_plane);\
      template line<T,3> mirror<T>(const line<T,3>& line, const plane<T,3>& mirror_plane);\
      template box<T,3> mirror<T>(const box<T,3>& box, const plane<T,3>& mirror_plane);\
      template triangle<T,3> mirror<T>(const triangle<T,3>& triangle, const plane<T,3>& mirror_plane);\
      template quadix<T,3> mirror<T>(const quadix<T,3>& quadix, const plane<T,3>& mirror_plane);\
      template sphere<T> mirror<T>(const sphere<T>& sphere, const plane<T,3>& mirror_plane);\
      template polygon<T,3> mirror<T>(const polygon<T,3>& polygon, const plane<T,3>& mirror_plane);\
      template void nonsymmetric_mirror<T>(const T& px, const T& py, const T& x1, const T& y1, const T& x2, const T& y2, const T& ratio, T& nx, T& ny);\
      template point2d<T> nonsymmetric_mirror<T>(const point2d<T>& point, const T& ratio, const line<T,2>& line);\
      template segment<T,2> nonsymmetric_mirror<T>(const segment<T,2>& segment, const T& ratio, const line<T,2>& line);\
      template rectangle<T> nonsymmetric_mirror<T>(const rectangle<T>& rectangle, const T& ratio, const line<T,2>& line);\
      template triangle<T,2> nonsymmetric_mirror<T>(const triangle<T,2>& triangle, const T& ratio, const line<T,2>& line);\
      template quadix<T,2> nonsymmetric_mirror<T>(const quadix<T,2>& quadix, const T& ratio, const line<T,2>& line);\
      template circle<T> nonsymmetric_mirror<T>(const circle<T>& circle, const T& ratio, const line<T,2>& line);\
      template polygon<T,2> nonsymmetric_mirror<T>(const polygon<T,2>& polygon, const T& ratio, const line<T,2>& line);\
      template point3d<T> nonsymmetric_mirror<T>(const point3d<T>& point, const T& ratio, const plane<T,3>& plane);\
      template segment<T,3> nonsymmetric_mirror<T>(const segment<T,3>& segment, const T& ratio, const plane<T,3>& plane);\
      template box<T,3> nonsymmetric_mirror<T>(const box<T,3>& box, const T& ratio, const plane<T,3>& plane);\
      template triangle<T,3> nonsymmetric_mirror<T>(const triangle<T,3>& triangle, const T& ratio, const plane<T,3>& plane);\
      template quadix<T,3> nonsymmetric_mirror<T>(const quadix<T,3>& quadix, const T& ratio, const plane<T,3>& plane);\
      template sphere<T> nonsymmetric_mirror<T>(const sphere<T>& sphere, const T& ratio, const plane<T,3>& plane);\
      template polygon<T,3> nonsymmetric_mirror<T>(const polygon<T,3>& polygon, const T& ratio, const plane<T,3>& plane);\
      template point2d<T> invert_point<T>(const point2d<T>& point, const circle<T>& circle);\
      template point3d<T> invert_point<T>(const point3d<T>& point, const sphere<T>& sphere);\
      template point2d<T> antipodal_point<T>(const point2d<T>& point, const circle<T>& circle);\
      template point3d<T> antipodal_point<T>(const point3d<T>& point, const sphere<T>& sphere);\
      template T distance<T>(const T& x1, const T& y1, const T& x2, const T& y2);\
      template T distance<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2);\
      template T distance<T>(const point2d<T>& point1, const point2d<T>& point2);\
      template T distance<T>(const point3d<T>& point1, const point3d<T>& point2);\
      template T distance<T>(const curve_point<T,2>& point1, const curve_point<T,2>& point2);\
      template T distance<T>(const curve_point<T,3>& point1, const curve_point<T,3>& point2);\
      template T distance<T>(const point2d<T>& point, const segment<T,2>& segment);\
      template T distance<T>(const point3d<T>& point, const segment<T,3>& segment);\
      template T distance<T>(const point2d<T>& point, const rectangle<T>& rectangle);\
      template T distance<T>(const point2d<T>& point, const triangle<T,2>& triangle);\
      template T distance<T>(const point2d<T>& point, const quadix<T,2>& quadix);\
      template T distance<T>(const point2d<T>& point, const ray<T,2>& ray);\
      template T distance<T>(const point3d<T>& point, const ray<T,3>& ray);\
      template T distance<T>(const point3d<T>& point, const plane<T,3>& plane);\
      template T distance<T>(const line<T,2>& line1, const line<T,2>& line2);\
      template T distance<T>(const line<T,3>& line1, const line<T,3>& line2);\
      template T distance<T>(const segment<T,2>& segment1, const segment<T,2>& segment2);\
      template T distance<T>(const segment<T,3>& segment1, const segment<T,3>& segment2);\
      template T distance<T>(const segment<T,2>& segment);\
      template T distance<T>(const segment<T,3>& segment);\
      template T distance<T>(const segment<T,2>& segment, const triangle<T,2>& triangle);\
      template T distance<T>(const segment<T,3>& segment, const triangle<T,3>& triangle);\
      template T distance<T>(const segment<T,2>& segment, const rectangle<T>& rectangle);\
      template T distance<T>(const segment<T,2>& segment, const circle<T>& circle);\
      template T distance<T>(const triangle<T,2>& triangle1, const triangle<T,2>& triangle2);\
      template T distance<T>(const triangle<T,2>& triangle, const rectangle<T>& rectangle);\
      template T distance<T>(const rectangle<T>& rectangle1, const rectangle<T>& rectangle2);\
      template T distance<T>(const triangle<T,2>& triangle, const circle<T>& circle);\
      template T distance<T>(const rectangle<T>& rectangle, const circle<T>& circle);\
      template T distance<T>(const point2d<T>& point, const circle<T>& circle);\
      template T distance<T>(const circle<T>& circle1, const circle<T>& circle2);\
      template T distance<T>(const sphere<T>& sphere1, const sphere<T>& sphere2);\
      template T lay_distance<T>(const T& x1, const T& y1, const T& x2, const T& y2);\
      template T lay_distance<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2);\
      template T lay_distance<T>(const point2d<T>& point1, const point2d<T>& point2);\
      template T lay_distance<T>(const point3d<T>& point1, const point3d<T>& point2);\
      template T lay_distance<T>(const point2d<T>& point, const triangle<T,2>& triangle);\
      template T lay_distance<T>(const point2d<T>& point, const quadix<T,2>& triangle);\
      template T lay_distance<T>(const point2d<T>& point, const ray<T,2>& ray);\
      template T lay_distance<T>(const point3d<T>& point, const ray<T,3>& ray);\
      template T lay_distance<T>(const point3d<T>& point, const plane<T,3>& plane);\
      template T lay_distance<T>(const segment<T,2>& segment1, const segment<T,2>& segment2);\
      template T lay_distance<T>(const segment<T,3>& segment1, const segment<T,3>& segment2);\
      template T lay_distance<T>(const line<T,3>& line1, const line<T,3>& line2);\
      template T lay_distance<T>(const segment<T,2>& segment);\
      template T lay_distance<T>(const segment<T,3>& segment);\
      template T lay_distance<T>(const segment<T,2>& segment, const triangle<T,2>& triangle);\
      template T lay_distance<T>(const segment<T,3>& segment, const triangle<T,3>& triangle);\
      template T manhattan_distance<T>(const T& x1, const T& y1, const T& x2, const T& y2);\
      template T manhattan_distance<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2);\
      template T manhattan_distance<T>(const point2d<T>& point1, const point2d<T>& point2);\
      template T manhattan_distance<T>(const point3d<T>& point1, const point3d<T>& point2);\
      template T manhattan_distance<T>(const point2d<T>& point, const ray<T,2>& ray);\
      template T manhattan_distance<T>(const point3d<T>& point, const ray<T,3>& ray);\
      template T manhattan_distance<T>(const segment<T,2>& segment);\
      template T manhattan_distance<T>(const segment<T,3>& segment);\
      template T manhattan_distance<T>(const circle<T>& circle1, const circle<T>& circle2);\
      template T chebyshev_distance<T>(const T& x1, const T& y1, const T& x2, const T& y2);\
      template T chebyshev_distance<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2);\
      template T chebyshev_distance<T>(const point2d<T>& point1, const point2d<T>& point2);\
      template T chebyshev_distance<T>(const point3d<T>& point1, const point3d<T>& point2);\
      template T chebyshev_distance<T>(const segment<T,2>& segment);\
      template T chebyshev_distance<T>(const segment<T,3>& segment);\
      template T chebyshev_distance<T>(const circle<T>& circle1, const circle<T>& circle2);\
      template T inverse_chebyshev_distance<T>(const T& x1, const T& y1, const T& x2, const T& y2);\
      template T inverse_chebyshev_distance<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2);\
      template T inverse_chebyshev_distance<T>(const point2d<T>& point1, const point2d<T>& point2);\
      template T inverse_chebyshev_distance<T>(const point3d<T>& point1, const point3d<T>& point2);\
      template T inverse_chebyshev_distance<T>(const segment<T,2>& segment);\
      template T inverse_chebyshev_distance<T>(const segment<T,3>& segment);\
      template T inverse_chebyshev_distance<T>(const circle<T>& circle1, const circle<T>& circle2);\
      template point2d<T> minkowski_sum<T>(const point2d<T>& point1, const point2d<T>& point2);\
      template polygon<T,2> minkowski_sum<T>(const rectangle<T>& rectangle1, const rectangle<T>& rectangle2);\
      template polygon<T,2> minkowski_sum<T>(const triangle<T,2>& triangle1, const triangle<T,2>& triangle2);\
      template polygon<T,2> minkowski_sum<T>(const quadix<T,2>& quadix1, const quadix<T,2>& quadix2);\
      template polygon<T,2> minkowski_sum<T>(const circle<T>& triangle, const circle<T>& circle);\
      template polygon<T,2> minkowski_sum<T>(const triangle<T,2>& triangle, const rectangle<T>& rectangle);\
      template polygon<T,2> minkowski_sum<T>(const triangle<T,2>& triangle, const quadix<T,2>& quadix);\
      template polygon<T,2> minkowski_sum<T>(const triangle<T,2>& triangle, const circle<T>& circle);\
      template polygon<T,2> minkowski_sum<T>(const quadix<T,2>& quadix, const circle<T>& circle);\
      template polygon<T,2> minkowski_sum<T>(const quadix<T,2>& quadix, const rectangle<T>& rectangle);\
      template polygon<T,2> minkowski_sum<T>(const rectangle<T>& rectangle, const circle<T>& circle);\
      template polygon<T,2> minkowski_sum<T>(const polygon<T,2>& polygon1, const polygon<T,2>& polygon2);\
      template point2d<T> minkowski_difference<T>(const point2d<T>& point1, const point2d<T>& point2);\
      template polygon<T,2> minkowski_difference<T>(const rectangle<T>& rectangle1, const rectangle<T>& rectangle2);\
      template polygon<T,2> minkowski_difference<T>(const triangle<T,2>& triangle1, const triangle<T,2>& triangle2);\
      template polygon<T,2> minkowski_difference<T>(const quadix<T,2>& quadix1, const quadix<T,2>& quadix2);\
      template polygon<T,2> minkowski_difference<T>(const circle<T>& triangle, const circle<T>& circle);\
      template polygon<T,2> minkowski_difference<T>(const triangle<T,2>& triangle, const rectangle<T>& rectangle);\
      template polygon<T,2> minkowski_difference<T>(const triangle<T,2>& triangle, const quadix<T,2>& quadix);\
      template polygon<T,2> minkowski_difference<T>(const triangle<T,2>& triangle, const circle<T>& circle);\
      template polygon<T,2> minkowski_difference<T>(const quadix<T,2>& quadix, const circle<T>& circle);\
      template polygon<T,2> minkowski_difference<T>(const quadix<T,2>& quadix, const rectangle<T>& rectangle);\
      template polygon<T,2> minkowski_difference<T>(const rectangle<T>& rectangle, const circle<T>& circle);\
      template polygon<T,2> minkowski_difference<T>(const polygon<T,2>& polygon1, const polygon<T,2>& polygon2);\
      template T distance_segment_to_segment<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4);\
      template T distance_segment_to_segment<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& x4, const T& y4, const T& z4);\
      template T lay_distance_segment_to_segment<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4);\
      template T lay_distance_segment_to_segment<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& x4, const T& y4, const T& z4);\
      template T distance_line_to_line<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4);\
      template T distance_line_to_line<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& x4, const T& y4, const T& z4);\
      template T lay_distance_line_to_line<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4);\
      template T lay_distance_line_to_line<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& x4, const T& y4, const T& z4);\
      template T lay_distance_from_point_to_circle_center<T>(const point2d<T>& point, const circle<T>& circle);\
      template T lay_distance_from_point_to_sphere_center<T>(const point3d<T>& point, const sphere<T>& sphere);\
      template T distance_from_point_to_circle_center<T>(const point2d<T>& point, const circle<T>& circle);\
      template T distance_from_point_to_sphere_center<T>(const point3d<T>& point, const sphere<T>& sphere);\
      template T span_length(const rectangle<T>& rect);\
      template T span_length(const box<T,3>& box);\
      template void project_point_t<T>(const T& srcx, const T& srcy, const T& destx, const T& desty, const T& t, T& nx, T& ny);\
      template void project_point_t<T>(const T& srcx, const T& srcy, const T& srcz, const T& destx, const T& desty, const T& destz, const T& t, T& nx, T& ny, T& nz);\
      template void project_point<T>(const T& srcx, const T& srcy, const T& destx, const T& desty, const T& dist, T& nx, T& ny);\
      template void project_point<T>(const T& srcx, const T& srcy, const T& srcz, const T& destx, const T& desty, const T& destz, const T& dist, T& nx, T& ny, T& nz);\
      template void project_point<T>(const T& px, const T& py, const T& angle, const T& distance, T& nx, T& ny);\
      template void project_point0 <T>(const T& px, const T& py, const T& distance, T& nx, T& ny);\
      template void project_point45 <T>(const T& px, const T& py, const T& distance, T& nx, T& ny);\
      template void project_point90 <T>(const T& px, const T& py, const T& distance, T& nx, T& ny);\
      template void project_point135<T>(const T& px, const T& py, const T& distance, T& nx, T& ny);\
      template void project_point180<T>(const T& px, const T& py, const T& distance, T& nx, T& ny);\
      template void project_point225<T>(const T& px, const T& py, const T& distance, T& nx, T& ny);\
      template void project_point270<T>(const T& px, const T& py, const T& distance, T& nx, T& ny);\
      template void project_point315<T>(const T& px, const T& py, const T& distance, T& nx, T& ny);\
      template point2d<T> project_point_t<T>(const point2d<T>& source_point, const point2d<T>& destination_point, const T& t);\
      template point3d<T> project_point_t<T>(const point3d<T>& source_point, const point3d<T>& destination_point, const T& t);\
      template point2d<T> project_point<T>(const point2d<T>& source_point, const point2d<T>& destination_point, const T& distance);\
      template point3d<T> project_point<T>(const point3d<T>& source_point, const point3d<T>& destination_point, const T& distance);\
      template point2d<T> project_point<T>(const point2d<T>& point, const T& angle, const T& distance);\
      template point2d<T> project_point0 <T>(const point2d<T>& point, const T& distance);\
      template point2d<T> project_point45 <T>(const point2d<T>& point, const T& distance);\
      template point2d<T> project_point90 <T>(const point2d<T>& point, const T& distance);\
      template point2d<T> project_point135<T>(const point2d<T>& point, const T& distance);\
      template point2d<T> project_point180<T>(const point2d<T>& point, const T& distance);\
      template point2d<T> project_point225<T>(const point2d<T>& point, const T& distance);\
      template point2d<T> project_point270<T>(const point2d<T>& point, const T& distance);\
      template point2d<T> project_point315<T>(const point2d<T>& point, const T& distance);\
      template point2d<T> project_object<T>(const point2d<T>& point, const T& angle, const T& distance);\
      template segment<T,2> project_object<T>(const segment<T,2>& segment, const T& angle, const T& distance);\
      template triangle<T,2> project_object<T>(const triangle<T,2>& triangle, const T& angle, const T& distance);\
      template quadix<T,2> project_object<T>(const quadix<T,2>& quadix, const T& angle, const T& distance);\
      template circle<T> project_object<T>(const circle<T>& circle, const T& angle, const T& distance);\
      template polygon<T,2> project_object<T>(const polygon<T,2>& polygon, const T& angle, const T& distance);\
      template segment<T,2> project_onto_axis<T>(const point2d<T>& point, const line<T,2>& axis);\
      template segment<T,2> project_onto_axis<T>(const triangle<T,2>& triangle, const line<T,2>& axis);\
      template segment<T,2> project_onto_axis<T>(const rectangle<T>& rectangle, const line<T,2>& axis);\
      template segment<T,2> project_onto_axis<T>(const quadix<T,2>& quadix, const line<T,2>& axis);\
      template segment<T,2> project_onto_axis<T>(const circle<T>& circle, const line<T,2>& axis);\
      template segment<T,2> project_onto_axis<T>(const polygon<T,2>& polygon, const line<T,2>& axis);\
      template segment<T,3> project_onto_axis<T>(const point3d<T>& point, const line<T,3>& axis);\
      template segment<T,3> project_onto_axis<T>(const triangle<T,3>& triangle, const line<T,3>& axis);\
      template segment<T,3> project_onto_axis<T>(const box<T,3>& box, const line<T,3>& axis);\
      template segment<T,3> project_onto_axis<T>(const quadix<T,3>& quadix, const line<T,3>& axis);\
      template segment<T,3> project_onto_axis<T>(const sphere<T>& sphere, const line<T,3>& axis);\
      template segment<T,3> project_onto_axis<T>(const polygon<T,3>& polygon, const line<T,3>& axis);\
      template void calculate_bezier_coefficients<T>(const quadratic_bezier<T,2>& bezier, T& ax, T& bx, T& ay, T& by);\
      template void calculate_bezier_coefficients<T>(const quadratic_bezier<T,3>& bezier, T& ax, T& bx, T& ay, T& by, T& az, T& bz);\
      template void calculate_bezier_coefficients<T>(const cubic_bezier<T,2>& bezier, T& ax, T& bx, T& cx, T& ay, T& by, T& cy);\
      template void calculate_bezier_coefficients<T>(const cubic_bezier<T,3>& bezier, T& ax, T& bx, T& cx, T& ay, T& by, T& cy, T& az, T& bz, T& cz);\
      template void calculate_bezier_coefficients<T>(const quadratic_bezier<T,2>& bezier, bezier_coefficients<T,2,eQuadraticBezier>& coeffs);\
      template void calculate_bezier_coefficients<T>(const quadratic_bezier<T,3>& bezier, bezier_coefficients<T,3,eQuadraticBezier>& coeffs);\
      template void calculate_bezier_coefficients<T>(const cubic_bezier<T,2>& bezier, bezier_coefficients<T,2,eCubicBezier>& coeffs);\
      template void calculate_bezier_coefficients<T>(const cubic_bezier<T,3>& bezier, bezier_coefficients<T,3,eCubicBezier>& coeffs);\
      template point2d<T> create_point_on_bezier<T>(const point2d<T>& start_point, const T& ax, const T& bx, const T& ay, const T& by, const T& t);\
      template point3d<T> create_point_on_bezier<T>(const point3d<T>& start_point, const T& ax, const T& bx, const T& ay, const T& by, const T& az, const T& bz, const T& t);\
      template point2d<T> create_point_on_bezier<T>(const point2d<T>& start_point, const T& ax, const T& bx, const T& cx, const T& ay, const T& by, const T& cy, const T& t);\
      template point3d<T> create_point_on_bezier<T>(const point3d<T>& start_point, const T& ax, const T& bx, const T& cx, const T& ay, const T& by, const T& cy, const T& az, const T& bz, const T& cz, const T& t);\
      template point2d<T> create_point_on_bezier<T>(const point2d<T>& start_point, const bezier_coefficients<T,2,eQuadraticBezier>& coeffs, const T& t);\
      template point3d<T> create_point_on_bezier<T>(const point3d<T>& start_point, const bezier_coefficients<T,3,eQuadraticBezier>& coeffs, const T& t);\
      template point2d<T> create_point_on_bezier<T>(const point2d<T>& start_point, const bezier_coefficients<T,2,eCubicBezier>& coeffs, const T& t);\
      template point3d<T> create_point_on_bezier<T>(const point3d<T>& start_point, const bezier_coefficients<T,3,eCubicBezier>& coeffs, const T& t);\
      template void generate_bezier<T,OutputIterator2d>(const quadratic_bezier<T,2>& bezier, OutputIterator2d out, const std::size_t& point_count);\
      template void generate_bezier<T,OutputIterator3d>(const quadratic_bezier<T,3>& bezier, OutputIterator3d out, const std::size_t& point_count);\
      template void generate_bezier<T,OutputIterator2d>(const cubic_bezier<T,2>& bezier, OutputIterator2d out, const std::size_t& point_count);\
      template void generate_bezier<T,OutputIterator3d>(const cubic_bezier<T,3>& bezier, OutputIterator3d out, const std::size_t& point_count);\
      template T bezier_curve_length<T>(const quadratic_bezier<T,2>& bezier, const std::size_t& point_count);\
      template T bezier_curve_length<T>(const quadratic_bezier<T,3>& bezier, const std::size_t& point_count);\
      template T bezier_curve_length<T>(const cubic_bezier<T,2>& bezier, const std::size_t& point_count);\
      template T bezier_curve_length<T>(const cubic_bezier<T,3>& bezier, const std::size_t& point_count);\
      template triangle<T,2> bezier_convex_hull<T>(const quadratic_bezier<T,2>& bezier);\
      template quadix<T,2> bezier_convex_hull<T>(const cubic_bezier<T,2>& bezier);\
      template segment<T,2> center_at_location<T>(const segment<T,2>& segment, const T& x, const T& y);\
      template segment<T,3> center_at_location<T>(const segment<T,3>& segment, const T& x, const T& y, const T& z);\
      template triangle<T,2> center_at_location<T>(const triangle<T,2>& triangle, const T& x, const T& y);\
      template rectangle<T> center_at_location<T>(const rectangle<T>& rectangle, const T& x, const T& y);\
      template box<T,3> center_at_location<T>(const box<T,3>& box, const T& x, const T& y, const T& z);\
      template quadix<T,2> center_at_location<T>(const quadix<T,2>& quadix, const T& x, const T& y);\
      template circle<T> center_at_location<T>(const circle<T>& circle, const T& x, const T& y);\
      template polygon<T,2> center_at_location<T>(const polygon<T,2>& polygon, const T& x, const T& y);\
      template segment<T,2> center_at_location<T>(const segment<T,2>& segment, const point2d<T>& center_point);\
      template segment<T,3> center_at_location<T>(const segment<T,3>& segment, const point3d<T>& center_point);\
      template triangle<T,2> center_at_location<T>(const triangle<T,2>& triangle, const point2d<T>& center_point);\
      template rectangle<T> center_at_location<T>(const rectangle<T>& rectangle, const point2d<T>& center_point);\
      template box<T,3> center_at_location<T>(const box<T,3>& box, const point3d<T>& center_point);\
      template quadix<T,2> center_at_location<T>(const quadix<T,2>& quadix, const point2d<T>& center_point);\
      template circle<T> center_at_location<T>(const circle<T>& circle, const point2d<T>& center_point);\
      template polygon<T,2> center_at_location<T>(const polygon<T,2>& polygon, const point2d<T>& center_point);\
      template void shorten_segment<T>(T& x1, T& y1, T& x2, T& y2, const T& amount);\
      template void shorten_segment<T>(T& x1, T& y1, T& z1, T& x2, T& y2, T& z2, const T& amount);\
      template segment<T,2> shorten_segment<T>(const segment<T,2>& segment, const T& amount);\
      template segment<T,3> shorten_segment<T>(const segment<T,3>& segment, const T& amount);\
      template void lengthen_segment<T>(T& x1, T& y1, T& x2, T& y2, const T& amount);\
      template void lengthen_segment<T>(T& x1, T& y1, T& z1, T& x2, T& y2, T& z2, const T& amount);\
      template segment<T,2> lengthen_segment<T>(const segment<T,2>& segment, const T& amount);\
      template segment<T,3> lengthen_segment<T>(const segment<T,3>& segment, const T& amount);\
      template int out_code<T>(const point2d<T>& point, const rectangle<T>& rectangle);\
      template bool clip<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4, T& cx1,T& cy1, T& cx2,T& cy2);\
      template bool clip<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& x4, const T& y4, const T& z4, T& cx1,T& cy1,T& cz1, T& cx2,T& cy2,T& cz2);\
      template bool clip<T>(const segment<T,2>& src_segment, const rectangle<T>& rectangle, segment<T,2>& csegment);\
      template bool clip<T>(const segment<T,2>& src_segment, const triangle<T,2>& triangle,segment<T,2>& csegment);\
      template bool clip<T>(const segment<T,2>& src_segment, const quadix<T,2>&quadix, segment<T,2>& csegment);\
      template bool clip<T>(const segment<T,2>& src_segment, const circle<T>& circle, segment<T,2>& csegment);\
      template bool clip<T>(const rectangle<T>& rectangle1, const rectangle<T>& rectangle2, rectangle<T>& crectangle);\
      template bool clip<T>(const box<T,3>& box1, const box<T,3>& box2, box<T,3>& cbox);\
      template T area<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);\
      template T area<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3);\
      template T area<T>(const triangle<T,2>& triangle);\
      template T area<T>(const triangle<T,3>& triangle);\
      template T area<T>(const quadix<T,2>& quadix);\
      template T area<T>(const quadix<T,3>& quadix);\
      template T area<T>(const rectangle<T>& rectangle);\
      template T area<T>(const circle<T>& circle);\
      template T area<T>(const polygon<T,2>& polygon);\
      template T perimeter<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);\
      template T perimeter<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3);\
      template T perimeter<T>(const triangle<T,2>& triangle);\
      template T perimeter<T>(const triangle<T,3>& triangle);\
      template T perimeter<T>(const quadix<T,2>& quadix);\
      template T perimeter<T>(const quadix<T,3>& quadix);\
      template T perimeter<T>(const rectangle<T>& rectangle);\
      template T perimeter<T>(const circle<T>& circle);\
      template T perimeter<T>(const polygon<T,2>& polygon);\
      template void rotate<T>(const T& rotation_angle, const T& x, const T& y, T& nx, T& ny);\
      template void rotate<T>(const T& rotation_angle, const T& x, const T& y, const T& ox, const T& oy, T& nx, T& ny);\
      template point2d<T> rotate<T>(const T& rotation_angle, const point2d<T>& point);\
      template point2d<T> rotate<T>(const T& rotation_angle, const point2d<T>& point, const point2d<T>& opoint);\
      template segment<T,2> rotate<T>(const T& rotation_angle, const segment<T,2>& segment);\
      template segment<T,2> rotate<T>(const T& rotation_angle, const segment<T,2>& segment, const point2d<T>& opoint);\
      template triangle<T,2> rotate<T>(const T& rotation_angle, const triangle<T,2>& triangle);\
      template triangle<T,2> rotate<T>(const T& rotation_angle, const triangle<T,2>& triangle, const point2d<T>& opoint);\
      template quadix<T,2> rotate<T>(const T& rotation_angle, const quadix<T,2>& quadix);\
      template quadix<T,2> rotate<T>(const T& rotation_angle, const quadix<T,2>& quadix, const point2d<T>& opoint);\
      template polygon<T,2> rotate<T>(const T& rotation_angle, const polygon<T,2>& polygon);\
      template polygon<T,2> rotate<T>(const T& rotation_angle, const polygon<T,2>& polygon, const point2d<T>& opoint);\
      template void rotate<T>(const T& rx, const T& ry, const T& rz, const T& x, const T& y, const T& z, T& nx, T& ny, T& nz);\
      template void rotate<T>(const T& rx, const T& ry, const T& rz, const T& x, const T& y, const T& z, const T& ox, const T& oy, const T& oz, T& nx, T& ny, T& nz);\
      template point3d<T> rotate<T>(const T& rx, const T& ry, const T& rz, const point3d<T>& point);\
      template point3d<T> rotate<T>(const T& rx, const T& ry, const T& rz, const point3d<T>& point, const point3d<T>& opoint);\
      template segment<T,3> rotate<T>(const T& rx, const T& ry, const T& rz, const segment<T,3>& segment);\
      template segment<T,3> rotate<T>(const T& rx, const T& ry, const T& rz, const segment<T,3>& segment, const point3d<T>& opoint);\
      template triangle<T,3> rotate<T>(const T& rx, const T& ry, const T& rz, const triangle<T,3>& triangle);\
      template triangle<T,3> rotate<T>(const T& rx, const T& ry, const T& rz, const triangle<T,3>& triangle, const point3d<T>& opoint);\
      template quadix<T,3> rotate<T>(const T& rx, const T& ry, const T& rz, const quadix<T,3>& quadix);\
      template quadix<T,3> rotate<T>(const T& rx, const T& ry, const T& rz, const quadix<T,3>& quadix, const point3d<T>& opoint);\
      template polygon<T,3> rotate<T>(const T& rx, const T& ry, const T& rz, const polygon<T,3>& polygon);\
      template polygon<T,3> rotate<T>(const T& rx, const T& ry, const T& rz, const polygon<T,3>& polygon, const point3d<T>& opoint);\
      template void fast_rotate<T>(const trig_luts<T>& lut, const int rotation_angle, const T& x, const T& y, T& nx, T& ny);\
      template void fast_rotate<T>(const trig_luts<T>& lut, const int rotation_angle, const T& x, const T& y, const T& ox, const T& oy, T& nx, T& ny);\
      template point2d<T> fast_rotate<T>(const trig_luts<T>& lut, const int rotation_angle, const point2d<T>& point);\
      template point2d<T> fast_rotate<T>(const trig_luts<T>& lut, const int rotation_angle, const point2d<T>& point, const point2d<T>& opoint);\
      template segment<T,2> fast_rotate<T>(const trig_luts<T>& lut, const int rotation_angle, const segment<T,2>& segment);\
      template segment<T,2> fast_rotate<T>(const trig_luts<T>& lut, const int rotation_angle, const segment<T,2>& segment, const point2d<T>& opoint);\
      template triangle<T,2> fast_rotate<T>(const trig_luts<T>& lut, const int rotation_angle, const triangle<T,2>& triangle);\
      template triangle<T,2> fast_rotate<T>(const trig_luts<T>& lut, const int rotation_angle, const triangle<T,2>& triangle, const point2d<T>& opoint);\
      template quadix<T,2> fast_rotate<T>(const trig_luts<T>& lut, const int rotation_angle, const quadix<T,2>& quadix);\
      template quadix<T,2> fast_rotate<T>(const trig_luts<T>& lut, const int rotation_angle, const quadix<T,2>& quadix, const point2d<T>& opoint);\
      template polygon<T,2> fast_rotate<T>(const trig_luts<T>& lut, const int rotation_angle, const polygon<T,2>& polygon);\
      template polygon<T,2> fast_rotate<T>(const trig_luts<T>& lut, const int rotation_angle, const polygon<T,2>& polygon, const point2d<T>& opoint);\
      template void fast_rotate<T>(const trig_luts<T>& lut, const int rx, const int ry, const int rz, const T&x, const T&y, const T&z, T& nx, T& ny, T& nz);\
      template void fast_rotate<T>(const trig_luts<T>& lut, const int rx, const int ry, const int rz, const T&x, const T&y, const T&z, const T& ox, const T& oy, const T& oz, T& nx, T& ny, T& nz);\
      template point3d<T> fast_rotate<T>(const trig_luts<T>& lut, const int rx, const int ry, const int rz, const point3d<T>& point);\
      template point3d<T> fast_rotate<T>(const trig_luts<T>& lut, const int rx, const int ry, const int rz, const point3d<T>& point, const point3d<T>& opoint);\
      template segment<T,3> fast_rotate<T>(const trig_luts<T>& lut, const int rx, const int ry, const int rz, const segment<T,3>& segment);\
      template segment<T,3> fast_rotate<T>(const trig_luts<T>& lut, const int rx, const int ry, const int rz, const segment<T,3>& segment, const point3d<T>& opoint);\
      template triangle<T,3> fast_rotate<T>(const trig_luts<T>& lut, const int rx, const int ry, const int rz, const triangle<T,3>& triangle);\
      template triangle<T,3> fast_rotate<T>(const trig_luts<T>& lut, const int rx, const int ry, const int rz, const triangle<T,3>& triangle, const point3d<T>& opoint);\
      template quadix<T,3> fast_rotate<T>(const trig_luts<T>& lut, const int rx, const int ry, const int rz, const quadix<T,3>& quadix);\
      template quadix<T,3> fast_rotate<T>(const trig_luts<T>& lut, const int rx, const int ry, const int rz, const quadix<T,3>& quadix, const point3d<T>& opoint);\
      template polygon<T,3> fast_rotate<T>(const trig_luts<T>& lut, const int rx, const int ry, const int rz, const polygon<T,3>& polygon);\
      template polygon<T,3> fast_rotate<T>(const trig_luts<T>& lut, const int rx, const int ry, const int rz, const polygon<T,3>& polygon, const point3d<T>& opoint);\
      template point2d<T> translate<T>(const T& dx, const T& dy, const point2d<T>& point);\
      template line<T,2> translate<T>(const T& dx, const T& dy, const line<T,2>& line);\
      template segment<T,2> translate<T>(const T& dx, const T& dy, const segment<T,2>& segment);\
      template triangle<T,2> translate<T>(const T& dx, const T& dy, const triangle<T,2>& triangle);\
      template quadix<T,2> translate<T>(const T& dx, const T& dy, const quadix<T,2>& quadix);\
      template rectangle<T> translate<T>(const T& dx, const T& dy, const rectangle<T>& rectangle);\
      template circle<T> translate<T>(const T& dx, const T& dy, const circle<T>& circle);\
      template polygon<T,2> translate<T>(const T& dx, const T& dy, const polygon<T,2>& polygon);\
      template point2d<T> translate<T>(const T& delta, const point2d<T>& point);\
      template line<T,2> translate<T>(const T& delta, const line<T,2>& line);\
      template segment<T,2> translate<T>(const T& delta, const segment<T,2>& segment);\
      template triangle<T,2> translate<T>(const T& delta, const triangle<T,2>& triangle);\
      template quadix<T,2> translate<T>(const T& delta, const quadix<T,2>& quadix);\
      template rectangle<T> translate<T>(const T& delta, const rectangle<T>& rectangle);\
      template circle<T> translate<T>(const T& delta, const circle<T>& circle);\
      template polygon<T,2> translate<T>(const T& delta, const polygon<T,2>& polygon);\
      template point2d<T> translate<T>(const vector2d<T>& v, const point2d<T>& point);\
      template line<T,2> translate<T>(const vector2d<T>& v, const line<T,2>& line);\
      template segment<T,2> translate<T>(const vector2d<T>& v, const segment<T,2>& segment);\
      template triangle<T,2> translate<T>(const vector2d<T>& v, const triangle<T,2>& triangle);\
      template quadix<T,2> translate<T>(const vector2d<T>& v, const quadix<T,2>& quadix);\
      template rectangle<T> translate<T>(const vector2d<T>& v, const rectangle<T>& rectangle);\
      template circle<T> translate<T>(const vector2d<T>& v, const circle<T>& circle);\
      template polygon<T,2> translate<T>(const vector2d<T>& v, const polygon<T,2>& polygon);\
      template point3d<T> translate<T>(const T& dx, const T& dy, const T& dz, const point3d<T>& point);\
      template line<T,3> translate<T>(const T& dx, const T& dy, const T& dz, const line<T,3>& line);\
      template segment<T,3> translate<T>(const T& dx, const T& dy, const T& dz, const segment<T,3>& segment);\
      template triangle<T,3> translate<T>(const T& dx, const T& dy, const T& dz, const triangle<T,3>& triangle);\
      template quadix<T,3> translate<T>(const T& dx, const T& dy, const T& dz, const quadix<T,3>& quadix);\
      template box<T,3> translate<T>(const T& dx, const T& dy, const T& dz, const box<T,3>& box);\
      template sphere<T> translate<T>(const T& dx, const T& dy, const T& dz, const sphere<T>& sphere);\
      template polygon<T,3> translate<T>(const T& dx, const T& dy, const T& dz, const polygon<T,3>& polygon);\
      template point3d<T> translate<T>(const T& delta, const point3d<T>& point);\
      template line<T,3> translate<T>(const T& delta, const line<T,3>& line);\
      template segment<T,3> translate<T>(const T& delta, const segment<T,3>& segment);\
      template triangle<T,3> translate<T>(const T& delta, const triangle<T,3>& triangle);\
      template quadix<T,3> translate<T>(const T& delta, const quadix<T,3>& quadix);\
      template box<T,3> translate<T>(const T& delta, const box<T,3>& box);\
      template sphere<T> translate<T>(const T& delta, const sphere<T>& sphere);\
      template polygon<T,3> translate<T>(const T& delta, const polygon<T,3>& polygon);\
      template point3d<T> translate<T>(const vector3d<T>& v, const point3d<T>& point);\
      template line<T,3> translate<T>(const vector3d<T>& v, const line<T,3>& line);\
      template segment<T,3> translate<T>(const vector3d<T>& v, const segment<T,3>& segment);\
      template triangle<T,3> translate<T>(const vector3d<T>& v, const triangle<T,3>& triangle);\
      template quadix<T,3> translate<T>(const vector3d<T>& v, const quadix<T,3>& quadix);\
      template box<T,3> translate<T>(const vector3d<T>& v, const box<T,3>& box);\
      template sphere<T> translate<T>(const vector3d<T>& v, const sphere<T>& sphere);\
      template polygon<T,3> translate<T>(const vector3d<T>& v, const polygon<T,3>& polygon);\
      template point2d<T> scale<T>(const T& dx, const T& dy, const point2d<T>& point);\
      template line<T,2> scale<T>(const T& dx, const T& dy, const line<T,2>& line);\
      template segment<T,2> scale<T>(const T& dx, const T& dy, const segment<T,2>& segment);\
      template triangle<T,2> scale<T>(const T& dx, const T& dy, const triangle<T,2>& triangle);\
      template quadix<T,2> scale<T>(const T& dx, const T& dy, const quadix<T,2>& quadix);\
      template rectangle<T> scale<T>(const T& dx, const T& dy, const rectangle<T>& rectangle);\
      template circle<T> scale<T>(const T& dr, const circle<T>& circle);\
      template polygon<T,2> scale<T>(const T& dx, const T& dy, const polygon<T,2>& polygon);\
      template point3d<T> scale<T>(const T& dx, const T& dy, const T& dz, const point3d<T>& point);\
      template line<T,3> scale<T>(const T& dx, const T& dy, const T& dz, const line<T,3>& line);\
      template segment<T,3> scale<T>(const T& dx, const T& dy, const T& dz, const segment<T,3>& segment);\
      template triangle<T,3> scale<T>(const T& dx, const T& dy, const T& dz, const triangle<T,3>& triangle);\
      template quadix<T,3> scale<T>(const T& dx, const T& dy, const T& dz, const quadix<T,3>& quadix);\
      template box<T,3> scale<T>(const T& dx, const T& dy, const T& dz, const box<T,3>& box);\
      template sphere<T> scale<T>(const T& dr, const sphere<T>& sphere);\
      template polygon<T,3> scale<T>(const T& dx, const T& dy, const T& dz, const polygon<T,3>& polygon);\
      template rectangle<T> aabb<T>(const segment<T,2>& segment);\
      template rectangle<T> aabb<T>(const triangle<T,2>& triangle);\
      template rectangle<T> aabb<T>(const rectangle<T>& rectangle);\
      template rectangle<T> aabb<T>(const quadix<T,2>& quadix);\
      template rectangle<T> aabb<T>(const circle<T>& circle);\
      template rectangle<T> aabb<T>(const polygon<T,2>& polygon);\
      template void aabb<T>(const segment<T,2>& segment,T& x1, T& y1, T& x2, T& y2);\
      template void aabb<T>(const triangle<T,2>& triangle, T& x1, T& y1, T& x2, T& y2);\
      template void aabb<T>(const rectangle<T>& rectangle, T& x1, T& y1, T& x2, T& y2);\
      template void aabb<T>(const quadix<T,2>& quadix, T& x1, T& y1, T& x2, T& y2);\
      template void aabb<T>(const circle<T>& circle, T& x1, T& y1, T& x2, T& y2);\
      template void aabb<T>(const polygon<T,2>& polygon,T& x1, T& y1, T& x2, T& y2);\
      template box<T,3> aabb<T>(const segment<T,3>& segment);\
      template box<T,3> aabb<T>(const triangle<T,3>& triangle);\
      template box<T,3> aabb<T>(const box<T,3>& rectangle);\
      template box<T,3> aabb<T>(const quadix<T,3>& quadix);\
      template box<T,3> aabb<T>(const sphere<T>& sphere);\
      template box<T,3> aabb<T>(const polygon<T,3>& polygon);\
      template void aabb<T>(const segment<T,3>& segment,T& x1, T& y1, T& z1, T& x2, T& y2, T& z2);\
      template void aabb<T>(const triangle<T,3>& triangle, T& x1, T& y1, T& z1, T& x2, T& y2, T& z2);\
      template void aabb<T>(const box<T,3>& box, T& x1, T& y1, T& z1, T& x2, T& y2, T& z2);\
      template void aabb<T>(const quadix<T,3>& quadix, T& x1, T& y1, T& z1, T& x2, T& y2, T& z2);\
      template void aabb<T>(const sphere<T>& sphere, T& x1, T& y1, T& z1, T& x2, T& y2, T& z2);\
      template void aabb<T>(const polygon<T,3>& polygon,T& x1, T& y1, T& z1, T& x2, T& y2, T& z2);\
      template rectangle<T> update_rectangle<T>(const rectangle<T>& rectangle, point2d<T>& point);\
      template box<T,3> update_box<T>(const box<T,3>& box, point3d<T>& point);\
      template circle<T> update_circle<T>(const circle<T>& circle, point2d<T>& point);\
      template sphere<T> update_sphere<T>(const sphere<T>& sphere, point3d<T>& point);\
      template point2d<T> generate_point_on_segment<T>(const segment<T,2>& segment, const T& t);\
      template point3d<T> generate_point_on_segment<T>(const segment<T,3>& segment, const T& t);\
      template point2d<T> generate_point_on_ray<T>(const ray<T,2>& ray, const T& t);\
      template point3d<T> generate_point_on_ray<T>(const ray<T,3>& ray, const T& t);\
      template T generate_random_value<T>(const T& range);\
      template point2d<T> generate_random_point<T>(const T& dx, const T& dy);\
      template point3d<T> generate_random_point<T>(const T& dx, const T& dy, const T& dz);\
      template point2d<T> generate_random_point<T>(const segment<T,2>& segment);\
      template point3d<T> generate_random_point<T>(const segment<T,3>& segment);\
      template point2d<T> generate_random_point<T>(const triangle<T,2>& triangle);\
      template point3d<T> generate_random_point<T>(const triangle<T,3>& triangle);\
      template point2d<T> generate_random_point<T>(const quadix<T,2>& quadix);\
      template point3d<T> generate_random_point<T>(const quadix<T,3>& quadix);\
      template point2d<T> generate_random_point<T>(const rectangle<T>& rectangle);\
      template point3d<T> generate_random_point<T>(const box<T,3>& box);\
      template void generate_random_points<T>(const T& x1, const T& y1, const T& x2, const T& y2, const std::size_t& point_count, OutputIterator2d out);\
      template void generate_random_points<T>(const rectangle<T>& rectangle, const std::size_t& point_count, OutputIterator2d out);\
      template void generate_random_points<T>(const segment<T,2>& segment, const std::size_t& point_count, OutputIterator2d out);\
      template void generate_random_points<T>(const segment<T,3>& segment, const std::size_t& point_count, OutputIterator2d out);\
      template void generate_random_points<T>(const triangle<T,2>& triangle, const std::size_t& point_count, OutputIterator2d out);\
      template void generate_random_points<T>(const triangle<T,3>& triangle, const std::size_t& point_count, OutputIterator2d out);\
      template void generate_random_points<T>(const quadix<T,2>& quadix, const std::size_t& point_count, OutputIterator2d out);\
      template void generate_random_points<T>(const quadix<T,3>& quadix, const std::size_t& point_count, OutputIterator2d out);\
      template void generate_random_points<T>(const circle<T>& circle, const std::size_t& point_count, OutputIterator2d out);\
      template void generate_random_object<T>(const T& x1, const  T& y1, const  T& x2, const  T& y2, segment<T,2>& segment);\
      template void generate_random_object<T>(const T& x1, const T& y1, const T& x2, const T& y2, rectangle<T>& rectangle);\
      template void generate_random_object<T>(const T& x1, const T& y1, const T& x2, const T& y2, triangle<T,2>& triangle);\
      template void generate_random_object<T>(const T& x1, const T& y1, const T& x2, const T& y2, quadix<T,2>& quadix);\
      template void generate_random_object<T>(const T& x1, const T& y1, const T& x2, const T& y2, circle<T>& circle);\
      template void generate_random_object<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, box<T,3>& box);\
      template triangle<T,2> right_shift<T>(const triangle<T,2>& triangle, const std::size_t& shift);\
      template triangle<T,3> right_shift<T>(const triangle<T,3>& triangle, const std::size_t& shift);\
      template quadix<T,2> right_shift<T>(const quadix<T,2>& quadix, const std::size_t& shift);\
      template quadix<T,3> right_shift<T>(const quadix<T,3>& quadix, const std::size_t& shift);\
      template T vector_norm<T>(const vector2d<T>& v);\
      template T vector_norm<T>(const vector3d<T>& v);\
      template vector2d<T> normalize<T>(const vector2d<T>& v);\
      template vector3d<T> normalize<T>(const vector3d<T>& v);\
      template vector2d<T> perpendicular<T>(const vector2d<T>& v);\
      template vector3d<T> perpendicular<T>(const vector3d<T>& v);\
      template vector2d<T> operator+<T>(const vector2d<T>& v1, const vector2d<T>& v2);\
      template vector3d<T> operator+<T>(const vector3d<T>& v1, const vector3d<T>& v2);\
      template vector2d<T> operator-<T>(const vector2d<T>& v1, const vector2d<T>& v2);\
      template vector3d<T> operator-<T>(const vector3d<T>& v1, const vector3d<T>& v2);\
      template T operator*<T>(const vector2d<T>& v1, const vector2d<T>& v2);\
      template vector3d<T> operator*<T>(const vector3d<T>& v1, const vector3d<T>& v2);\
      template T dot_product<T>(const vector2d<T>& v1, const vector2d<T>& v2);\
      template T dot_product<T>(const vector3d<T>& v1, const vector3d<T>& v2);\
      template T perpendicular_product<T>(const vector2d<T>& v1, const vector2d<T>& v2);\
      template T triple_product<T>(const vector3d<T>& v1, const vector3d<T>& v2, const vector3d<T>& v3);\
      template vector2d<T> operator*<T>(const vector2d<T>& v1, const T& scale);\
      template vector3d<T> operator*<T>(const vector3d<T>& v1, const T& scale);\
      template vector2d<T> operator*<T>(const T& scale, const vector2d<T>& v1);\
      template vector3d<T> operator*<T>(const T& scale, const vector3d<T>& v1);\
      template point2d<T> operator*<T>(const point2d<T>& point, const T& scale);\
      template point3d<T> operator*<T>(const point3d<T>& point, const T& scale);\
      template point2d<T> operator*<T>(const T& scale, const point2d<T>& point);\
      template point3d<T> operator*<T>(const T& scale, const point3d<T>& point);\
      template point2d<T> operator+<T>(const point2d<T>& point, const vector2d<T>& v);\
      template point2d<T> operator+<T>(const vector2d<T>& v, const point2d<T>& point);\
      template point3d<T> operator+<T>(const point3d<T>& point, const vector3d<T>& v);\
      template point3d<T> operator+<T>(const vector3d<T>& v, const point3d<T>& point);\
      template vector2d<T> operator-<T>(const point2d<T>& p1, const point2d<T>& p2);\
      template vector3d<T> operator-<T>(const point3d<T>& p1, const point3d<T>& p2);\
      template point2d<T> operator+<T>(const point2d<T>& p1, const point2d<T>& p2);\
      template point3d<T> operator+<T>(const point3d<T>& p1, const point3d<T>& p2);\
      template bool is_equal<T>(const T& val1, const T& val2, const T& epsilon);\
      template bool is_equal<T>(const point2d<T>& point1, const point2d<T>& point2, const T& epsilon);\
      template bool is_equal<T>(const point3d<T>& point1, const point3d<T>& point2, const T& epsilon);\
      template bool is_equal<T>(const T& val1, const T& val2);\
      template bool is_equal<T>(const point2d<T>& point1, const point2d<T>& point2);\
      template bool is_equal<T>(const point3d<T>& point1, const point3d<T>& point2);\
      template bool is_equal<T>(const rectangle<T>& rectangle1, const rectangle<T>& rectangle2);\
      template bool is_equal<T>(const circle<T>& circle1, const circle<T>& circle2);\
      template bool is_equal<T>(const box<T,3>& box1, const box<T,3>& box2);\
      template bool is_equal<T>(const sphere<T>& sphere1, const sphere<T>& sphere2);\
      template bool not_equal<T>(const T& val1, const T& val2, const T& epsilon);\
      template bool not_equal<T>(const point2d<T>& point1, const point2d<T>& point2, const T& epsilon);\
      template bool not_equal<T>(const point3d<T>& point1, const point3d<T>& point2, const T& epsilon);\
      template bool not_equal<T>(const T& val1, const T& val2);\
      template bool not_equal<T>(const point2d<T>& point1, const point2d<T>& point2);\
      template bool not_equal<T>(const point3d<T>& point1, const point3d<T>& point2);\
      template bool less_than_or_equal<T>(const T& val1, const T& val2, const T& epsilon);\
      template bool less_than_or_equal<T>(const T& val1, const T& val2);\
      template bool greater_than_or_equal<T>(const T& val1, const T& val2, const T& epsilon);\
      template bool greater_than_or_equal<T>(const T& val1, const T& val2);\
      template bool operator< <T>(const point2d<T>& point1, const point2d<T>& point2);\
      template bool operator< <T>(const point3d<T>& point1, const point3d<T>& point2);\
      template bool operator><T>(const point2d<T>& point1, const point2d<T>& point2);\
      template bool operator><T>(const point3d<T>& point1, const point3d<T>& point2);\
      template bool operator==<T>(const point2d<T>& point1, const point2d<T>& point2);\
      template bool operator==<T>(const point3d<T>& point1, const point3d<T>& point2);\
      template bool is_degenerate<T>(const T& x1, const T& y1, const T& x2, const T& y2);\
      template bool is_degenerate<T>(const segment<T,2>& segment);\
      template bool is_degenerate<T>(const line<T,2>& line);\
      template bool is_degenerate<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2);\
      template bool is_degenerate<T>(const segment<T,3>& segment);\
      template bool is_degenerate<T>(const line<T,3>& line);\
      template bool is_degenerate<T>(const triangle<T,2>& triangle);\
      template bool is_degenerate<T>(const triangle<T,3>& triangle);\
      template bool is_degenerate<T>(const quadix<T,2>& quadix);\
      template bool is_degenerate<T>(const quadix<T,3>& quadix);\
      template bool is_degenerate<T>(const rectangle<T>& rectangle);\
      template bool is_degenerate<T>(const circle<T>& circle);\
      template bool is_degenerate<T>(const sphere<T>& sphere);\
      template bool is_degenerate<T>(const circular_arc<T>& arc);\
      template point2d<T> degenerate_point2d<T>();\
      template point3d<T> degenerate_point3d<T>();\
      template vector2d<T> degenerate_vector2d<T>();\
      template vector3d<T> degenerate_vector3d<T>();\
      template ray<T,2> degenerate_ray2d<T>();\
      template ray<T,3> degenerate_ray3d<T>();\
      template line<T,2> degenerate_line2d<T>();\
      template line<T,3> degenerate_line3d<T>();\
      template segment<T,2> degenerate_segment2d<T>();\
      template segment<T,3> degenerate_segment3d<T>();\
      template triangle<T,2> degenerate_triangle2d<T>();\
      template triangle<T,3> degenerate_triangle3d<T>();\
      template quadix<T,2> degenerate_quadix2d<T>();\
      template quadix<T,3> degenerate_quadix3d<T>();\
      template rectangle<T> degenerate_rectangle<T>();\
      template circle<T> degenerate_circle<T>();\
      template sphere<T> degenerate_sphere<T>();\
      template point2d<T> positive_infinite_point2d<T>();\
      template point2d<T> negative_infinite_point2d<T>();\
      template point3d<T> positive_infinite_point3d<T>();\
      template point3d<T> negative_infinite_point3d<T>();\
      template point2d<T> make_point<T>(const T& x, const T& y);\
      template point3d<T> make_point<T>(const T& x, const T& y, const T& z);\
      template point2d<T> make_point<T>(const point3d<T> point);\
      template point3d<T> make_point<T>(const point2d<T> point, const T& z);\
      template point2d<T> make_point<T>(const circle<T>& circle);\
      template point3d<T> make_point<T>(const sphere<T>& sphere);\
      template vector2d<T> make_vector<T>(const T& x, const T& y);\
      template vector3d<T> make_vector<T>(const T& x, const T& y, const T& z);\
      template vector2d<T> make_vector<T>(const vector3d<T> v);\
      template vector3d<T> make_vector<T>(const vector2d<T> v, const T& z);\
      template vector2d<T> make_vector<T>(const point2d<T> point);\
      template vector3d<T> make_vector<T>(const point3d<T> point);\
      template ray<T,2> make_ray<T>(const T& ox, const T& oy, const T& dir_x, const T& dir_y);\
      template ray<T,3> make_ray<T>(const T& ox, const T& oy, const T& oz, const T& dir_x, const T& dir_y, const T& dir_z);\
      template ray<T,2> make_ray<T>(const point2d<T>& origin, const vector2d<T>& direction);\
      template ray<T,3> make_ray<T>(const point3d<T>& origin, const vector3d<T>& direction);\
      template ray<T,2> make_ray<T>(const point2d<T>& origin, const T& bearing);\
      template curve_point<T,2> make_curve_point<T>(const T& x, const T& y, const T& t);\
      template curve_point<T,3> make_curve_point<T>(const T& x, const T& y, const T& z, const T& t);\
      template curve_point<T,2> make_curve_point<T>(const point2d<T>& point, const T& t);\
      template curve_point<T,3> make_curve_point<T>(const point3d<T>& point, const T& t);\
      template segment<T,2> make_segment<T>(const T& x1, const T& y1, const T& x2, const T& y2);\
      template segment<T,3> make_segment<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2);\
      template segment<T,2> make_segment<T>(const point2d<T>& point1, const point2d<T>& point2);\
      template segment<T,3> make_segment<T>(const point3d<T>& point1, const point3d<T>& point2);\
      template segment<T,2> make_segment<T>(const line<T,2>& line);\
      template segment<T,3> make_segment<T>(const line<T,3>& line);\
      template line<T,2> make_line<T>(const T& x1, const T& y1, const T& x2, const T& y2);\
      template line<T,3> make_line<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2);\
      template line<T,2> make_line<T>(const point2d<T>& point1, const point2d<T>& point2);\
      template line<T,3> make_line<T>(const point3d<T>& point1, const point3d<T>& point2);\
      template line<T,2> make_line<T>(const segment<T,2>& segment);\
      template line<T,3> make_line<T>(const segment<T,3>& segment);\
      template line<T,2> make_line<T>(const ray<T,2>& ray);\
      template line<T,3> make_line<T>(const ray<T,3>& ray);\
      template rectangle<T> make_rectangle<T>(const T& x1, const T& y1, const T& x2, const T& y2);\
      template rectangle<T> make_rectangle<T>(const point2d<T>& point1, const point2d<T>& point2);\
      template box<T,3> make_box<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2);\
      template box<T,3> make_box<T>(const point3d<T>& point1, const point3d<T>& point2);\
      template triangle<T,2> make_triangle<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3);\
      template triangle<T,3> make_triangle<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3);\
      template triangle<T,2> make_triangle<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);\
      template triangle<T,3> make_triangle<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3);\
      template quadix<T,2> make_quadix<T>(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3, const T& x4, const T& y4);\
      template quadix<T,3> make_quadix<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3, const T& x4, const T& y4, const T& z4);\
      template quadix<T,2> make_quadix<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3, const point2d<T>& point4);\
      template quadix<T,3> make_quadix<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const point3d<T>& point4);\
      template quadix<T,2> make_quadix<T>(const T& x1, const T& y1, const T& x2, const T& y2);\
      template quadix<T,2> make_quadix<T>(const rectangle<T>& rectangle);\
      template circle<T> make_circle<T>(const T& x, const T& y, const T& radius);\
      template circle<T> make_circle<T>(const point2d<T>& point, const T& radius);\
      template circle<T> make_circle<T>(const point2d<T>& point1, const point2d<T>& point2);\
      template circle<T> make_circle<T>(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);\
      template circle<T> make_circle<T>(const triangle<T,2>& triangle);\
      template sphere<T> make_sphere<T>(const T& x, const T& y, const T& z, const T& radius);\
      template sphere<T> make_sphere<T>(const point3d<T>& point, const T& radius);\
      template sphere<T> make_sphere<T>(const point3d<T>& point1, const point3d<T>& point2);\
      template plane<T,3> make_plane<T>(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, const T& x3, const T& y3, const T& z3);\
      template plane<T,3> make_plane<T>(const T& px, const T& py, const T& pz, const T& nx, const T& ny, const T& nz);\
      template plane<T,3> make_plane<T>(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3);\
      template plane<T,3> make_plane<T>(const point3d<T>& point, const vector3d<T>& normal);\
      template plane<T,3> make_plane<T>(const triangle<T,3>& triangle);\
      template polygon<T,2> make_polygon<T>(const std::vector< point2d<T> >& point_list);\
      template polygon<T,3> make_polygon<T>(const std::vector< point3d<T> >& point_list);\
      template polygon<T,2> make_polygon<T>(const triangle<T,2>& triangle);\
      template polygon<T,2> make_polygon<T>(const quadix<T,2>& quadix);\
      template polygon<T,2> make_polygon<T>(const rectangle<T>& rectangle);\
      template polygon<T,2> make_polygon<T>(const circle<T>& circle, const unsigned int point_count);\

   #define INSTANTIATE_WYKOBI_ND(T, D, OutputIterator)\
      template bool parallel<T,D>(const line<T,D>& line1, const line<T,D>& line2);\
      template bool parallel<T,D>(const segment<T,D>& segment1, const segment<T,D>& segment2);\
      template bool collinear<T,D>(const pointnd<T,D>& point1, const pointnd<T,D>& point2, const pointnd<T,D>& point3);\
      template bool robust_collinear<T,D>(const pointnd<T,D>& point1, const pointnd<T,D>& point2, const pointnd<T,D>& point3);\
      template bool is_point_collinear<T,D>(const segment<T,D>& segment, const pointnd<T,D>& point, const bool robust);\
      template bool perpendicular<T,D>(const line<T,D>& line1, const line<T,D>& line2);\
      template bool perpendicular<T,D>(const segment<T,D>& segment1, const segment<T,D>& segment2);\
      template bool intersect<T,D>(const segment<T,D>& segment1, const segment<T,D>& segment2, const T& fuzzy);\
      template bool intersect<T,D>(const line<T,D>& line1, const line<T,D>& line2, const T& fuzzy);\
      template pointnd<T,D> intersection_point<T,D>(const segment<T,D>& segment1, const segment<T,D>& segment2, const T& fuzzy);\
      template pointnd<T,D> intersection_point<T,D>(const line<T,D>& line1, const line<T,D>& line2, const T& fuzzy);\
      template T distance<T,D>(const pointnd<T,D>& point1, const pointnd<T,D>& point2);\
      template T distance<T,D>(const pointnd<T,D>& point, const segment<T,D>& segment);\
      template T distance<T,D>(const pointnd<T,D>& point, const line<T,D>& line);\
      template T distance<T,D>(const segment<T,D>& segment1, const segment<T,D>& segment2);\
      template T distance<T,D>(const line<T,D>& line1, const line<T,D>& line2);\
      template T lay_distance<T,D>(const pointnd<T,D>& point1, const pointnd<T,D>& point2);\
      template T lay_distance<T,D>(const pointnd<T,D>& point, const segment<T,D>& segment);\
      template T lay_distance<T,D>(const pointnd<T,D>& point, const line<T,D>& line);\
      template T lay_distance<T,D>(const segment<T,D>& segment1, const segment<T,D>& segment2);\
      template T lay_distance<T,D>(const line<T,D>& line1, const line<T,D>& line2);\
      template T manhattan_distance<T,D>(const pointnd<T,D>& point1, const pointnd<T,D>& point2);\
      template T chebyshev_distance<T,D>(const pointnd<T,D>& point1, const pointnd<T,D>& point2);\
      template T inverse_chebyshev_distance<T,D>(const pointnd<T,D>& point1, const pointnd<T,D>& point2);\
      template bool point_in_box<T,D>(const pointnd<T,D>& point, const box<T,D>& box);\
      template bool point_in_sphere<T,D>(const pointnd<T,D>& point, const hypersphere<T,D>& sphere);\
      template pointnd<T,D> closest_point_on_segment_from_point<T,D>(const segment<T,D>& segment, const wykobi::segment<T,D>::PointType& point);\
      template pointnd<T,D> closest_point_on_line_from_point<T,D>(const line<T,D>& segment, const wykobi::line<T,D>::PointType& point);\
      template pointnd<T,D> closest_point_on_sphere_from_point<T,D>(const hypersphere<T,D>& sphere, const wykobi::hypersphere<T,D>::PointType& point);\
      template pointnd<T,D> closest_point_on_plane_from_point<T,D>(const plane<T,D>& plane, const wykobi::plane<T,D>::PointType& point);\
      template pointnd<T,D> closest_point_on_box_from_point<T,D>(const box<T,D>& box, const wykobi::box<T,D>::PointType& point);\
      template pointnd<T,D> project_point_t<T,D>(const pointnd<T,D>& source_point, const pointnd<T,D>& destination_point, const T& t);\
      template pointnd<T,D> project_point<T,D>(const pointnd<T,D>& source_point, const pointnd<T,D>& destination_point, const T& distance);\
      template pointnd<T,D> mirror<T,D>(const pointnd<T,D>& point, const line<T,D>& mirror_axis);\
      template segment<T,D> mirror<T,D>(const segment<T,D>& segment, const line<T,D>& mirror_axis);\
      template line<T,D> mirror<T,D>(const line<T,D>& line, const wykobi::line<T,D>& mirror_axis);\
      template box<T,D> mirror<T,D>(const box<T,D>& box, const line<T,D>& mirror_axis);\
      template triangle<T,D> mirror<T,D>(const triangle<T,D>& triangle, const line<T,D>& mirror_axis);\
      template quadix<T,D> mirror<T,D>(const quadix<T,D>& quadix, const line<T,D>& mirror_axis);\
      template hypersphere<T,D> mirror<T,D>(const hypersphere<T,D>& sphere, const line<T,D>& mirror_axis);\
      template polygon<T,D> mirror<T,D>(const polygon<T,D>& polygon, const line<T,D>& mirror_axis);\
      template segment<T,D> project_onto_axis<T,D>(const pointnd<T,D>& point, const line<T,D>& axis);\
      template segment<T,D> project_onto_axis<T,D>(const triangle<T,D>& triangle, const line<T,D>& axis);\
      template segment<T,D> project_onto_axis<T,D>(const box<T,D>& rectangle, const line<T,D>& axis);\
      template segment<T,D> project_onto_axis<T,D>(const quadix<T,D>& quadix, const line<T,D>& axis);\
      template segment<T,D> project_onto_axis<T,D>(const hypersphere<T,D>& sphere, const line<T,D>& axis);\
      template segment<T,D> project_onto_axis<T,D>(const polygon<T,D>& polygon, const line<T,D>& axis);\
      template T perimeter<T,D>(const triangle<T,D>& triangle);\
      template T perimeter<T,D>(const quadix<T,D>& quadix);\
      template T perimeter<T,D>(const polygon<T,D>& polygon);\
      template pointnd<T,D> generate_random_point<T,D>(const segment<T,D>& segment);\
      template pointnd<T,D> generate_random_point<T,D>(const triangle<T,D>& triangle);\
      template pointnd<T,D> generate_random_point<T,D>(const quadix<T,D>& quadix);\
      template pointnd<T,D> generate_random_point<T,D>(const box<T,D>& box);\
      template void generate_random_points<T,D,OutputIterator>(const box<T,D>& box, const std::size_t& point_count, OutputIterator out);\
      template void generate_random_points<T,D,OutputIterator>(const segment<T,D>& segment, const std::size_t& point_count, OutputIterator out);\
      template void generate_random_points<T,D,OutputIterator>(const triangle<T,D>& triangle, const std::size_t& point_count, OutputIterator out);\
      template void generate_random_points<T,D,OutputIterator>(const quadix<T,D>& quadix, const std::size_t& point_count, OutputIterator out);\
      template T vector_norm<T,D>(const vectornd<T,D>& v);\
      template vectornd<T,D> normalize<T,D>(const vectornd<T,D>& v);\
      template vectornd<T,D> operator+<T,D>(const vectornd<T,D>& v1, const vectornd<T,D>& v2);\
      template vectornd<T,D> operator-<T,D>(const vectornd<T,D>& v1, const vectornd<T,D>& v2);\
      template T dot_product<T,D>(const vectornd<T,D>& v1, const vectornd<T,D>& v2);\
      template vectornd<T,D> operator*<T,D>(const vectornd<T,D>& v1, const T& scale);\
      template vectornd<T,D> operator*<T,D>(const T& scale, const vectornd<T,D>& v1);\
      template pointnd<T,D> operator*<T,D>(const pointnd<T,D>& point, const T& scale);\
      template pointnd<T,D> operator*<T,D>(const T& scale, const pointnd<T,D>& point);\
      template pointnd<T,D> operator+<T,D>(const pointnd<T,D>& point, const vectornd<T,D>& v);\
      template pointnd<T,D> operator+<T,D>(const vectornd<T,D>& v, const pointnd<T,D>& point);\
      template vectornd<T,D> operator-<T,D>(const pointnd<T,D>& p1, const pointnd<T,D>& p2);\
      template pointnd<T,D> operator+<T,D>(const pointnd<T,D>& p1, const pointnd<T,D>& p2);\
      template bool operator < <T,D>(const pointnd<T,D>& point1, const pointnd<T,D>& point2);\
      template bool operator > <T,D>(const pointnd<T,D>& point1, const pointnd<T,D>& point2);\
      template bool operator==<T,D>(const pointnd<T,D>& point1, const pointnd<T,D>& point2);\
      template bool is_equal<T,D>(const pointnd<T,D>& point1, const pointnd<T,D>& point2, const T& epsilon);\
      template bool is_equal<T,D>(const pointnd<T,D>& point1, const pointnd<T,D>& point2);\
      template bool not_equal<T,D>(const pointnd<T,D>& point1, const pointnd<T,D>& point2, const T& epsilon);\
      template bool not_equal<T,D>(const pointnd<T,D>& point1, const pointnd<T,D>& point2);\
      template pointnd<T,D> degenerate_pointnd<T,D>();\
      template vectornd<T,D> degenerate_vectornd<T,D>();\
      template ray<T,D> degenerate_raynd<T,D>();\
      template line<T,D> degenerate_linend<T,D>();\
      template segment<T,D> degenerate_segmentnd<T,D>();\
      template triangle<T,D> degenerate_trianglend<T,D>();\
      template quadix<T,D> degenerate_quadixnd<T,D>();\
      template box<T,D> degenerate_box<T,D>();\
      template hypersphere<T,D> degenerate_hypersphere<T,D>();\
      template pointnd<T,D> positive_infinite_pointnd<T,D>();\
      template pointnd<T,D> negative_infinite_pointnd<T,D>();\
      template void swap<T,D>(pointnd<T,D>& point1, pointnd<T,D>& point2);\
      template vectornd<T,D> make_vector<T,D>(const pointnd<T,D>& point);\
      template ray<T,D> make_ray<T,D>(const pointnd<T,D>& origin, const vectornd<T,D>& direction);\
      template segment<T,D> make_segment<T,D>(const pointnd<T,D>& point1, const pointnd<T,D>& point2);\
      template line<T,D> make_line<T,D>(const pointnd<T,D>& point1, const pointnd<T,D>& point2);\
      template box<T,D> make_box<T,D>(const pointnd<T,D>& point1, const pointnd<T,D>& point2);\
      template triangle<T,D> make_triangle<T,D>(const pointnd<T,D>& point1, const pointnd<T,D>& point2, const pointnd<T,D>& point3);\
      template quadix<T,D> make_quadix<T,D>(const pointnd<T,D>& point1, const pointnd<T,D>& point2, const pointnd<T,D>& point3, const pointnd<T,D>& point4);\
      template hypersphere<T,D> make_sphere<T,D>(const pointnd<T,D>& point, const T& radius);\
      template hypersphere<T,D> make_sphere<T,D>(const pointnd<T,D>& point1, const pointnd<T,D>& point2);\
      template polygon<T,D> make_polygon<T,D>(const std::vector< pointnd<T,D> >& point_list);\
      template polygon<T,D> make_polygon<T,D>(const triangle<T,D>& triangle);\
      template polygon<T,D> make_polygon<T,D>(const quadix<T,D>& quadix);

   #define INSTANTIATE_WYKOBI_MATH(T)                                \
      template T sqr<T> (const T& val);                              \
      template T abs<T> (const T& value);                            \
      template T max<T> (const T& value1, const T& value2);          \
      template T min<T> (const T& value1, const T& value2);          \
      template T infinity<T>();                                      \
      template T sin<T> (const T& value);                            \
      template T cos<T> (const T& value);                            \
      template T tan<T> (const T& value);                            \
      template T atan<T>(const T& value);                            \
      template T approx_sin<T>(T angle);                             \
      template T approx_cos<T>(T angle);                             \
      template T approx_tan<T>(T angle);                             \
      template T clamp(const T& value, const T& low, const T& high); \


   #define INSTANTIATE_WYKOBI_UTILITIES_1(T)                                                 \
      template std::ostream& operator<< <T>(std::ostream& os, const point2d<T>& point);      \
      template std::ostream& operator<< <T>(std::ostream& os, const point3d<T>& point);      \
      template std::ostream& operator<< <T>(std::ostream& os, const ray<T,2>& ray);          \
      template std::ostream& operator<< <T>(std::ostream& os, const ray<T,3>& ray);          \
      template std::ostream& operator<< <T>(std::ostream& os, const vector2d<T>& v);         \
      template std::ostream& operator<< <T>(std::ostream& os, const vector3d<T>& v);         \
      template std::ostream& operator<< <T>(std::ostream& os, const circle<T>& circle);      \
      template std::ostream& operator<< <T>(std::ostream& os, const sphere<T>& sphere);      \
      template std::ostream& operator<< <T>(std::ostream& os, const rectangle<T>& rectangle);\
      template std::ostream& operator<< <T>(std::ostream& os, const box<T,3>& box);          \


   #define INSTANTIATE_WYKOBI_UTILITIES_2(T,D)                                                 \
      template std::ostream& operator<< <T,D>(std::ostream& os, const segment<T,D>& segment);  \
      template std::ostream& operator<< <T,D>(std::ostream& os, const line<T,D>& line);        \
      template std::ostream& operator<< <T,D>(std::ostream& os, const triangle<T,D>& triangle);\
      template std::ostream& operator<< <T,D>(std::ostream& os, const quadix<T,D>& quadix);    \


   #define INSTANTIATE_WYKOBI_ALGORITHMS(T,K)                                                                                                \
   std::vector<point2d<T>    > K##vec2d;                                                                                                     \
   std::vector<point3d<T>    > K##vec3d;                                                                                                     \
   std::vector<circle<T>     > K##clist;                                                                                                     \
   std::vector<segment<T,2>  > K##s2dlist;                                                                                                   \
   std::vector<segment<T,3>  > K##s3dlist;                                                                                                   \
   std::vector<triangle<T,2> > K##t2dlist;                                                                                                   \
   rectangle<T>                K##rect2d;                                                                                                    \
   polygon<T,2>                K##poly2d;                                                                                                    \
   circle<T>                   K##circle2d;                                                                                                  \
   sphere<T>                   K##sphere3d;                                                                                                  \
                                                                                                                                             \
   algorithm::isotropic_normalization< point2d<T> >                          K##obj00(K##vec2d.begin(),K##vec2d.end());                      \
   algorithm::isotropic_normalization< point3d<T> >                          K##obj01(K##vec3d.begin(),K##vec3d.end());                      \
   algorithm::convex_hull_graham_scan< point2d<T> >                          K##obj02(K##vec2d.begin(),K##vec2d.end(),K##vec2d.begin());     \
   algorithm::convex_hull_jarvis_march< point2d<T> >                         K##obj03(K##vec2d.begin(),K##vec2d.end(),K##vec2d.begin());     \
   algorithm::convex_hull_melkman< point2d<T> >                              K##obj04(K##vec2d.begin(),K##vec2d.end(),K##vec2d.begin());     \
   algorithm::covariance_matrix< point2d<T> >                                K##obj05;                                                       \
   algorithm::covariance_matrix< point3d<T> >                                K##obj06;                                                       \
   algorithm::ordered_polygon< point2d<T> >                                  K##obj07(K##vec2d  .begin(),K##vec2d  .end(),K##vec2d.begin()); \
   algorithm::naive_group_intersections< segment<T,2> >                      K##obj08(K##s2dlist.begin(),K##s2dlist.end(),K##vec2d.begin()); \
   algorithm::naive_group_intersections< segment<T,3> >                      K##obj09(K##s3dlist.begin(),K##s3dlist.end(),K##vec3d.begin()); \
   algorithm::naive_group_intersections< circle<T> >                         K##obj10(K##clist  .begin(),K##clist  .end(),K##vec2d.begin()); \
   algorithm::naive_minimum_bounding_ball< point2d<T> >                      K##obj11(K##vec2d  .begin(),K##vec2d  .end(),K##circle2d);      \
   algorithm::naive_minimum_bounding_ball_with_ch_filter< point2d<T> >       K##obj12(K##vec2d  .begin(),K##vec2d  .end(),K##circle2d);      \
   algorithm::randomized_minimum_bounding_ball< point2d<T> >                 K##obj13(K##vec2d  .begin(),K##vec2d  .end(),K##circle2d);      \
   algorithm::randomized_minimum_bounding_ball_with_ch_filter < point2d<T> > K##obj14(K##vec2d  .begin(),K##vec2d  .end(),K##circle2d);      \
   algorithm::ritter_minimum_bounding_ball< point2d<T> >                     K##obj15(K##vec2d  .begin(),K##vec2d  .end(),K##circle2d);      \
   algorithm::ritter_minimum_bounding_ball< point3d<T> >                     K##obj16(K##vec3d  .begin(),K##vec3d  .end(),K##sphere3d);      \
   algorithm::ritter_minimum_bounding_ball_with_ch_filter< point2d<T> >      K##obj19(K##vec2d  .begin(),K##vec2d  .end(),K##circle2d);      \
   algorithm::generate_axis_projection_descriptor<T>                         K##obj20(K##poly2d,K##vec2d.begin());                           \
   algorithm::sutherland_hodgman_polygon_clipper< point2d<T> >               K##obj21(K##rect2d,K##poly2d,K##poly2d);                        \
   algorithm::polygon_triangulate< point2d<T> >                              K##obj23(K##poly2d,K##t2dlist.begin());                         \

   typedef wykobi::point2d<float>*  flt_pnt_2d;
   typedef wykobi::point2d<double>* dbl_pnt_2d;

   typedef wykobi::point3d<float>*  flt_pnt_3d;
   typedef wykobi::point3d<double>* dbl_pnt_3d;

   INSTANTIATE_WYKOBI( float, flt_pnt_2d, flt_pnt_3d, flt_pnt_2d, flt_pnt_3d)
   INSTANTIATE_WYKOBI(double, dbl_pnt_2d, dbl_pnt_3d, dbl_pnt_2d, dbl_pnt_3d)


   /*
   typedef pointnd<float, 4> pointnd_flt_4;
   typedef pointnd<float, 5> pointnd_flt_5;
   typedef pointnd<float, 6> pointnd_flt_6;
   typedef pointnd<float, 7> pointnd_flt_7;
   typedef pointnd<float, 8> pointnd_flt_8;
   typedef pointnd<float, 9> pointnd_flt_9;
   typedef pointnd<float,10> pointnd_flt_10;

   typedef pointnd<double, 4> pointnd_dbl_4;
   typedef pointnd<double, 5> pointnd_dbl_5;
   typedef pointnd<double, 6> pointnd_dbl_6;
   typedef pointnd<double, 7> pointnd_dbl_7;
   typedef pointnd<double, 8> pointnd_dbl_8;
   typedef pointnd<double, 9> pointnd_dbl_9;
   typedef pointnd<double,10> pointnd_dbl_10;

   INSTANTIATE_WYKOBI_ND(float,  4,  pointnd_flt_4)
   INSTANTIATE_WYKOBI_ND(float,  5,  pointnd_flt_5)
   INSTANTIATE_WYKOBI_ND(float,  6,  pointnd_flt_6)
   INSTANTIATE_WYKOBI_ND(float,  7,  pointnd_flt_7)
   INSTANTIATE_WYKOBI_ND(float,  8,  pointnd_flt_8)
   INSTANTIATE_WYKOBI_ND(float,  9,  pointnd_flt_9)
   INSTANTIATE_WYKOBI_ND(float, 10, pointnd_flt_10)

   INSTANTIATE_WYKOBI_ND(double,  4,  pointnd_dbl_4)
   INSTANTIATE_WYKOBI_ND(double,  5,  pointnd_dbl_5)
   INSTANTIATE_WYKOBI_ND(double,  6,  pointnd_dbl_6)
   INSTANTIATE_WYKOBI_ND(double,  7,  pointnd_dbl_7)
   INSTANTIATE_WYKOBI_ND(double,  8,  pointnd_dbl_8)
   INSTANTIATE_WYKOBI_ND(double,  9,  pointnd_dbl_9)
   INSTANTIATE_WYKOBI_ND(double, 10, pointnd_dbl_10)
   */

   INSTANTIATE_WYKOBI_MATH(float)
   INSTANTIATE_WYKOBI_MATH(double)

   INSTANTIATE_WYKOBI_UTILITIES_1(float)
   INSTANTIATE_WYKOBI_UTILITIES_1(double)

   INSTANTIATE_WYKOBI_UTILITIES_2(float,2)
   INSTANTIATE_WYKOBI_UTILITIES_2(double,2)

   INSTANTIATE_WYKOBI_UTILITIES_2(float,3)
   INSTANTIATE_WYKOBI_UTILITIES_2(double,3)

   INSTANTIATE_WYKOBI_ALGORITHMS(float, f)
   INSTANTIATE_WYKOBI_ALGORITHMS(double,d)

} // wykobi namespace


#endif
