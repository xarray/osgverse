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


#ifndef INCLUDE_WYKOBI_ALGORITHM
#define INCLUDE_WYKOBI_ALGORITHM


#include <algorithm>

#include "wykobi.hpp"


namespace wykobi
{
   namespace algorithm
   {
      template <typename T> struct isotropic_normalization;
      template <typename T> struct isotropic_normalization< point2d<T> >;
      template <typename T> struct isotropic_normalization< point3d<T> >;

      template <typename T> struct convex_hull_graham_scan;
      template <typename T> struct convex_hull_graham_scan< point2d<T> >;

      template <typename T> struct convex_hull_jarvis_march;
      template <typename T> struct convex_hull_jarvis_march< point2d<T> >;

      template <typename T> struct convex_hull_melkman;
      template <typename T> struct convex_hull_melkman< point2d<T> >;

      template <typename T> struct covariance_matrix;
      template <typename T> struct covariance_matrix< point2d<T> >;
      template <typename T> struct covariance_matrix< point3d<T> >;

      template <typename T> struct ordered_polygon;
      template <typename T> struct ordered_polygon< point2d<T> >;

      template <typename T> struct remove_duplicates;

      template <typename T> struct naive_group_intersections;
      template <typename T> struct naive_group_intersections< segment<T,2> >;
      template <typename T> struct naive_group_intersections< segment<T,3> >;
      template <typename T> struct naive_group_intersections< circle<T> >;

      template <typename T> struct naive_minimum_bounding_ball;
      template <typename T> struct naive_minimum_bounding_ball< point2d<T> >;

      template <typename T> struct naive_minimum_bounding_ball_with_ch_filter;
      template <typename T> struct naive_minimum_bounding_ball_with_ch_filter< point2d<T> >;

      template <typename T> struct randomized_minimum_bounding_ball;
      template <typename T> struct randomized_minimum_bounding_ball< point2d<T> >;

      template <typename T> struct randomized_minimum_bounding_ball_with_ch_filter;
      template <typename T> struct randomized_minimum_bounding_ball_with_ch_filter < point2d<T> >;

      template <typename T> struct ritter_minimum_bounding_ball;
      template <typename T> struct ritter_minimum_bounding_ball< point2d<T> >;
      template <typename T> struct ritter_minimum_bounding_ball< point3d<T> >;
      //template <typename T, std::size_t D> struct ritter_minimum_bounding_ball< pointnd<T,D> >;

      template <typename T> struct ritter_minimum_bounding_ball_with_ch_filter;
      template <typename T> struct ritter_minimum_bounding_ball_with_ch_filter< point2d<T> >;

      template <typename T> struct generate_axis_projection_descriptor;

      template <typename T> struct sutherland_hodgman_polygon_clipper;
      template <typename T> struct sutherland_hodgman_polygon_clipper< point2d<T> >;

      template <typename T> struct sutherland_hodgman_polygon_clipper_engine;
      template <typename T> struct sutherland_hodgman_polygon_clipper_engine< point2d<T> >;

      template <typename T> struct polygon_triangulate;
      template <typename T> struct polygon_triangulate< point2d<T> >;


   } // namespace wykobi::algorithm

} // namespace wykobi

#include "wykobi_normalization.inl"
#include "wykobi_hull.inl"
#include "wykobi_ordered_polygon.inl"
#include "wykobi_duplicates.inl"
#include "wykobi_naive_group_intersections.inl"
#include "wykobi_minimum_bounding_ball.inl"
#include "wykobi_axis_projection_descriptor.inl"
#include "wykobi_clipping.inl"
#include "wykobi_earclipping.inl"

#endif
