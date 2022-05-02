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


#include "wykobi.hpp"
#include "wykobi_nd.hpp"
#include "wykobi_math.hpp"

#include <algorithm>
#include <iterator>
#include <vector>


namespace wykobi
{

   template <typename T, std::size_t D>
   inline bool parallel(const line<T,D>& line1, const line<T,D>& line2)
   {
      const vectornd<T,D> v1 = line1[0] - line1[1];
      const vectornd<T,D> v2 = line2[0] - line2[1];

      return is_equal(sqr(dot_product(v1,v2)),dot_product(v1,v1) * dot_product(v2,v2));
   }

   template <typename T, std::size_t D>
   inline bool parallel(const segment<T,D>& segment1, const segment<T,D>& segment2)
   {
      const vectornd<T,D> v1 = segment1[0] - segment1[1];
      const vectornd<T,D> v2 = segment2[0] - segment2[1];

      return is_equal(sqr(dot_product(v1,v2)),dot_product(v1,v1) * dot_product(v2,v2));
   }

   template <typename T, std::size_t D>
   inline bool perpendicular(const line<T,D>& line1, const line<T,D>& line2)
   {
      return is_equal(dot_product(line1[0] - line1[1],line2[0] - line2[1]),T(0.0));
   }

   template <typename T, std::size_t D>
   inline bool perpendicular(const segment<T,D>& segment1, const segment<T,D>& segment2)
   {
      return is_equal(dot_product(segment1[0] - segment1[1],segment2[0] - segment2[1]),T(0.0));
   }

   template <typename T, std::size_t D>
   inline bool collinear(const pointnd<T,D>& point1, const pointnd<T,D>& point2, const pointnd<T,D>& point3)
   {
      const vectornd<T,D> v1 = point2 - point1;
      const vectornd<T,D> v2 = point3 - point1;

      return is_equal(sqr(dot_product(v1,v2)),dot_product(v1,v1) * dot_product(v2,v2));
   }

   template <typename T, std::size_t D>
   inline bool robust_collinear(const pointnd<T,D>& point1, const pointnd<T,D>& point2, const pointnd<T,D>& point3)
   {
      return (less_than_or_equal(lay_distance(closest_point_on_line_from_point(make_line(point1,point2),point3),point3),T(0.0)));
   }

   template <typename T, std::size_t D>
   inline bool is_point_collinear(const segment<T,D>& segment, const pointnd<T,D>& point, const bool robust)
   {

      for (std::size_t i = 0; i < D; ++i)
      {
         const T max_v = max(segment[0][i],segment[1][i]);
         const T min_v = min(segment[0][i],segment[1][i]);

         if ((point[i] < min_v) || (point[i] > max_v))
         {
            return false;
         }
      }

      if (robust)
         return robust_collinear(pointnd<T,D>(segment[0]),pointnd<T,D>(segment[1]),point);
      else
         return collinear(pointnd<T,D>(segment[0]),pointnd<T,D>(segment[1]),point);
   }

   template <typename T, std::size_t D>
   inline bool intersect(const segment<T,D>& segment1, const segment<T,D>& segment2, const T& fuzzy)
   {
      return is_equal(distance(segment1,segment2),fuzzy);
   }

   template <typename T, std::size_t D>
   inline bool intersect(const line<T,D>& line1, const line<T,D>& line2, const T& fuzzy)
   {
      return is_equal(distance(line1,line2),fuzzy);
   }

   template <typename T, std::size_t D>
   inline pointnd<T,D> intersection_point(const segment<T,D>& segment1, const segment<T,D>& segment2, const T& fuzzy)
   {
      const vectornd<T,D> u = segment1[1] - segment1[0];
      const vectornd<T,D> v = segment2[1] - segment2[0];
      const vectornd<T,D> w = segment1[0] - segment2[0];

      const T a = dot_product(u,u);
      const T b = dot_product(u,v);
      const T c = dot_product(v,v);
      const T d = dot_product(u,w);
      const T e = dot_product(v,w);

      const T dt = a * c - b * b;

      T sd = dt;
      T td = dt;
      T sn = T(0.0);
      T tn = T(0.0);

      if (is_equal(dt,T(0.0)))
      {
         sn = T(0.0);
         sd = T(1.00);
         tn = e;
         td = c;
      }
      else
      {
         sn = (b * e - c * d);
         tn = (a * e - b * d);
         if (sn < T(0.0))
         {
            sn = T(0.0);
            tn = e;
            td = c;
         }
         else if (sn > sd)
         {
            sn = sd;
            tn = e + b;
            td = c;
         }
      }

      if (tn < T(0.0))
      {
         tn = T(0.0);
         if (-d < T(0.0))
            sn = T(0.0);
         else if (-d > a)
            sn = sd;
         else
         {
            sn = -d;
            sd = a;
         }
      }
      else if (tn > td)
      {
         tn = td;
         if ((-d + b) < T(0.0))
            sn = T(0.0);
         else if ((-d + b) > a)
            sn = sd;
         else
         {
            sn = (-d + b);
            sd = a;
         }
      }

      T sc = T(0.0);
      T tc = T(0.0);

      if (is_equal(sn,T(0.0)))
         sc = T(0.0);
      else
         sc = sn / sd;

      if (is_equal(tn,T(0.0)))
         tc = T(0.0);
      else
         tc = tn / td;

      const vectornd<T,D> dv = w + (sc * u) - (tc * v);

      if (less_than_or_equal(dot_product(dv,dv),sqr(fuzzy)))
      {
         return ((segment1[0] + (sc * u)) + (segment2[0] + (tc * v))) * T(0.5);
      }
      else
      {
         return positive_infinite_pointnd<T,D>();
      }
   }

   template <typename T, std::size_t D>
   inline pointnd<T,D> intersection_point(const line<T,D>& line1, const line<T,D>& line2, const T& fuzzy)
   {
      const vectornd<T,D> u = line1[1] - line1[0];
      const vectornd<T,D> v = line2[1] - line2[0];
      const vectornd<T,D> w = line1[0] - line2[0];

      const T a = dot_product(u,u);
      const T b = dot_product(u,v);
      const T c = dot_product(v,v);
      const T d = dot_product(u,w);
      const T e = dot_product(v,w);

      const T dt = a * c - b * b;

      T sc = T(0.0);
      T tc = T(0.0);

      if (is_equal(dt,T(0.0)))
      {
         sc = T(0.0);

         if (b > c )
            tc = d / b;
         else
            tc = e / c;
      }
      else
      {
         sc = (b * e - c * d) / dt;
         tc = (a * e - b * d) / dt;
      }

      const vectornd<T,D> dv = w + (sc * u) - (tc * v);

      if (less_than_or_equal(dot_product(dv,dv),sqr(fuzzy)))
      {
         return ((line1[0] + (sc * u)) + (line2[0] + (tc * v))) * T(0.5);
      }
      else
      {
         return positive_infinite_pointnd<T,D>();
      }
   }

   template <typename T, std::size_t D>
   inline T distance(const pointnd<T,D>& point1, const pointnd<T,D>& point2)
   {
      return sqrt(lay_distance(point1,point2));
   }

   template <typename T, std::size_t D>
   inline T distance(const pointnd<T,D>& point, const segment<T,D>& segment)
   {
      return distance(closest_point_on_segment_from_point(segment,point),point);
   }

   template <typename T, std::size_t D>
   inline T distance(const pointnd<T,D>& point, const line<T,D>& line)
   {
      return distance(closest_point_on_line_from_point(line,point),point);
   }

   template <typename T, std::size_t D>
   inline T distance(const segment<T,D>& segment1, const segment<T,D>& segment2)
   {
      return sqrt(lay_distance(segment1,segment2));
   }

   template <typename T, std::size_t D>
   inline T distance(const line<T,D>& line1, const line<T,D>& line2)
   {
      return sqrt(lay_distance(line1,line2));
   }

   template <typename T, std::size_t D>
   inline T lay_distance(const pointnd<T,D>& point1, const pointnd<T,D>& point2)
   {
      T sum = T(0.0);

      for (std::size_t i = 0; i < D; ++i)
      {
         sum += sqr(point1[i] - point2[i]);
      }

      return sum;
   }

   template <typename T, std::size_t D>
   inline T lay_distance(const pointnd<T,D>& point, const segment<T,D>& segment)
   {
      return lay_distance(closest_point_on_segment_from_point(segment,point),point);
   }

   template <typename T, std::size_t D>
   inline T lay_distance(const pointnd<T,D>& point, const line<T,D>& line)
   {
      return lay_distance(closest_point_on_line_from_point(line,point),point);
   }

   template <typename T, std::size_t D>
   inline T lay_distance(const segment<T,D>& segment1, const segment<T,D>& segment2)
   {
      const vectornd<T,D> u = segment1[1] - segment1[0];
      const vectornd<T,D> v = segment2[1] - segment2[0];
      const vectornd<T,D> w = segment1[0] - segment2[0];

      const T a = dot_product(u,u);
      const T b = dot_product(u,v);
      const T c = dot_product(v,v);
      const T d = dot_product(u,w);
      const T e = dot_product(v,w);

      const T dt = a * c - b * b;

      T sd = dt;
      T td = dt;
      T sn = T(0.0);
      T tn = T(0.0);

      if (is_equal(dt,T(0.0)))
      {
         sn = T(0.0);
         sd = T(1.0);
         tn = e;
         td = c;
      }
      else
      {
         sn = (b * e - c * d);
         tn = (a * e - b * d);

         if (sn < T(0.0))
         {
            sn = T(0.0);
            tn = e;
            td = c;
         }
         else if (sn > sd)
         {
            sn = sd;
            tn = e + b;
            td = c;
         }
      }

      if (tn < T(0.0))
      {
         tn = T(0.0);

         if (-d < T(0.0))
            sn = T(0.0);
         else if (-d > a)
            sn = sd;
         else
         {
            sn = -d;
            sd = a;
         }
      }
      else if (tn > td)
      {
         tn = td;

         if ((-d + b) < T(0.0))
            sn = T(0.0);
         else if ((-d + b) > a)
            sn = sd;
         else
         {
            sn = (-d + b);
            sd = a;
         }
      }

      T sc = T(0.0);
      T tc = T(0.0);

      if (is_equal(sn,T(0.0)))
         sc = T(0.0);
      else
         sc = sn / sd;

      if (is_equal(tn,T(0.0)))
         tc = T(0.0);
      else
         tc = tn / td;

      const vectornd<T,D> dv = w + (sc * u) - (tc * v);

      return dot_product(dv,dv);
   }

   template <typename T, std::size_t D>
   inline T lay_distance(const line<T,D>& line1, const line<T,D>& line2)
   {
      const vectornd<T,D> u = line1[1] - line1[0];
      const vectornd<T,D> v = line2[1] - line2[0];
      const vectornd<T,D> w = line1[0] - line2[0];

      const T a = dot_product(u,u);
      const T b = dot_product(u,v);
      const T c = dot_product(v,v);
      const T d = dot_product(u,w);
      const T e = dot_product(v,w);

      const T dt = a * c - b * b;

      T sc = T(0.0);
      T tc = T(0.0);

      if (is_equal(dt,T(0.0)))
      {
         sc = T(0.0);

         if (b > c )
            tc = d / b;
         else
            tc = e / c;
      }
      else
      {
         sc = (b * e - c * d) / dt;
         tc = (a * e - b * d) / dt;
      }

      const vectornd<T,D> dv = w + (sc * u) - (tc * v);

      return dot_product(dv,dv);
   }

   template <typename T, std::size_t D>
   inline T manhattan_distance(const pointnd<T,D>& point1, const pointnd<T,D>& point2)
   {
      T sum = T(0.0);

      for (std::size_t i = 0; i < D; ++i)
      {
         sum += abs(point1[i] - point2[i]);
      }

      return sum;
   }

   template <typename T, std::size_t D>
   inline T chebyshev_distance(const pointnd<T,D>& point1, const pointnd<T,D>& point2)
   {
      T max_diff = abs(point1[0] - point2[0]);

      for (std::size_t i = 1; i < D; ++i)
      {
         max_diff = max(max_diff,abs(point1[i] - point2[i]));
      }

      return max_diff;
   }

   template <typename T, std::size_t D>
   inline T inverse_chebyshev_distance(const pointnd<T,D>& point1, const pointnd<T,D>& point2)
   {
      T min_diff = abs(point1[0] - point2[0]);

      for (std::size_t i = 1; i < D; ++i)
      {
         min_diff = min(min_diff,abs(point1[i] - point2[i]));
      }

      return min_diff;
   }

   template <typename T, std::size_t D>
   inline bool point_in_box(const pointnd<T,D>& point, const box<T,D>& box)
   {
      for (std::size_t i = 0; i < D; ++i)
      {
         if ((point[i] < (box[0])[i]) || (point[i] > (box[1])[i]))
         {
            return false;
         }
      }

      return true;
   }

   template <typename T, std::size_t D>
   inline bool point_in_sphere(const pointnd<T,D>& point, const hypersphere<T,D>& sphere)
   {
      return less_than_or_equal(sqr(sphere.radius),lay_distance(point,pointnd<T,D>(sphere.center)));
   }

   template <typename T, std::size_t D>
   inline pointnd<T,D> closest_point_on_segment_from_point(const segment<T,D>& segment, const pointnd<T,D>& point)
   {
      const vectornd<T,D> v1 = segment[1] - segment[0];
      const vectornd<T,D> v2 =      point - segment[0];

      const T c1 = dot_product(v1,v2);

      if (c1 <= T(0.0))
      {
         return segment[0];
      }

      const T c2 = dot_product(v1,v1);

      if (c2 <= c1)
      {
         return segment[1];
      }

      const T ratio = c1 / c2;

      pointnd<T,D> _point;

      for (std::size_t i = 0; i < D; ++i)
      {
         _point[i] = point[i] + ratio * v1[i];
      }

      return _point;
   }

   template <typename T, std::size_t D>
   inline pointnd<T,D> closest_point_on_line_from_point(const line<T,D>& line, const pointnd<T,D>& point)
   {
      const vectornd<T,D> v1 = line[1] - line[0];
      const vectornd<T,D> v2 =   point - line[0];

      const T c1 = dot_product(v1,v2);
      const T c2 = dot_product(v1,v1);

      const T ratio = c1 / c2;

      pointnd<T,D> _point;

      for (std::size_t i = 0; i < D; ++i)
      {
         _point[i] = point[i] + ratio * v1[i];
      }

      return _point;
   }

   template <typename T, std::size_t D>
   inline pointnd<T,D> closest_point_on_sphere_from_point(const hypersphere<T,D>& sphere, const pointnd<T,D>& point)
   {
      return sphere.center + sphere.radius / distance(sphere.center,point) * (point - sphere.center);
   }

   template <typename T, std::size_t D>
   inline pointnd<T,D> closest_point_on_plane_from_point(const plane<T,D>& plane, const pointnd<T,D>& point)
   {
      const T mu = dot_product(plane.normal,plane.normal) - plane.constant;

      if (is_equal(mu,T(0.0)))
         return point;
      else
      {
         pointnd<T,D> _point;

         for (std::size_t i = 0; i < D; ++i)
         {
            _point[i] = point[i] - mu * plane.normal[i];
         }

         return _point;
      }
   }

   template <typename T, std::size_t D>
   inline pointnd<T,D> closest_point_on_box_from_point(const box<T,D>& box, const pointnd<T,D>& point)
   {
      pointnd<T,D> _point = point;

      for (std::size_t i = 0; i < D; ++i)
      {
         const T max_v = max(box[0][i],box[1][i]);
         const T min_v = min(box[0][i],box[1][i]);

         if (_point[i] < min_v)
         {
            _point[i] = min_v;
         }
         else if (_point[i] > max_v)
         {
            _point[i] = max_v;
         }
      }

      return _point;
   }

   template <typename T, std::size_t D>
   inline pointnd<T,D> project_point_t(const pointnd<T,D>& source_point,
                                       const pointnd<T,D>& destination_point,
                                       const T& t)
   {
      pointnd<T,D> _point;

      for (std::size_t i = 0; i < D; ++i)
      {
         _point[i] = source_point[i] + t * (destination_point[i] - source_point[i]);
      }

      return _point;
   }

   template <typename T, std::size_t D>
   inline pointnd<T,D> project_point(const pointnd<T,D>& source_point,
                                     const pointnd<T,D>& destination_point,
                                     const T& distance)
   {
      return project_point_t
             (
               source_point,
               destination_point,
               distance / wykobi::distance(source_point, destination_point)
             );
   }

   template <typename T, std::size_t D>
   inline pointnd<T,D> mirror(const pointnd<T,D>& point, const line<T,D>& mirror_axis)
   {
      return project_point_t(point,closest_point_on_line_from_point(mirror_axis, point), T(2.0));
   }

   template <typename T, std::size_t D>
   inline segment<T,D> mirror(const segment<T,D>& segment, const line<T,D>& mirror_axis)
   {
      wykobi::segment<T,D> _segment;

      for (std::size_t i = 0; i < wykobi::segment<T,D>::PointCount; ++i)
      {
         _segment[i] = mirror(segment[i],mirror_axis);
      }

      return _segment;
   }

   template <typename T, std::size_t D>
   inline line<T,D> mirror(const line<T,D>& line, const wykobi::line<T,D>& mirror_axis)
   {
      wykobi::line<T,D> _line;

      for (std::size_t i = 0; i < wykobi::line<T,D>::PointCount; ++i)
      {
         _line[i] = mirror(line[i],mirror_axis);
      }

      return _line;
   }

   template <typename T, std::size_t D>
   inline box<T,D> mirror(const box<T,D>& box, const line<T,D>& mirror_axis)
   {
      wykobi::box<T,D> _box;

      for (std::size_t i = 0; i < wykobi::box<T,D>::PointCount; ++i)
      {
         _box[i] = mirror(box[i],mirror_axis);
      }

      return _box;
   }

   template <typename T, std::size_t D>
   inline triangle<T,D> mirror(const triangle<T,D>& triangle, const line<T,D>& mirror_axis)
   {
      wykobi::triangle<T,D> _triangle;

      for (std::size_t i = 0; i < wykobi::triangle<T,D>::PointCount; ++i)
      {
         _triangle[i] = mirror(triangle[i],mirror_axis);
      }

      return _triangle;
   }

   template <typename T, std::size_t D>
   inline quadix<T,D> mirror(const quadix<T,D>& quadix, const line<T,D>& mirror_axis)
   {
      wykobi::quadix<T,D> _quadix;

      for (std::size_t i = 0; i < wykobi::quadix<T,D>::PointCount; ++i)
      {
         _quadix[i] = mirror(quadix[i],mirror_axis);
      }

      return _quadix;
   }

   template <typename T, std::size_t D>
   inline hypersphere<T,D> mirror(const hypersphere<T,D>& sphere, const line<T,D>& mirror_axis)
   {
      wykobi::hypersphere<T,D> _sphere;

      _sphere.center = mirror(sphere.center,mirror_axis);
      _sphere.radius = sphere.radius;

      return _sphere;
   }

   template <typename T, std::size_t D>
   inline polygon<T,D> mirror(const polygon<T,D>& polygon, const line<T,D>& mirror_axis)
   {
      wykobi::polygon<T,D> _polygon;

      _polygon.reserve(polygon.size());

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         _polygon.push_back(mirror(polygon[i],mirror_axis));
      }

      return _polygon;
   }

   template <typename T, std::size_t D>
   inline segment<T,D> project_onto_axis(const pointnd<T,D>& point, const line<T,D>& axis)
   {
      wykobi::pointnd<T,D> _point = closest_point_on_line_from_point(axis,point);

      return make_segment(_point,_point);
   }

   template <typename T, std::size_t D>
   inline segment<T,D> project_onto_axis(const triangle<T,D>& triangle, const line<T,D>& axis)
   {
      std::vector< pointnd<T,D> > point_list;

      point_list.reserve(wykobi::triangle<T,D>::PointCount);

      for (std::size_t i = 0; i < wykobi::triangle<T,D>::PointCount; ++i)
      {
         point_list.push_back(closest_point_on_line_from_point(axis,triangle[i]));
      }

      std::sort(point_list.begin(),point_list.end());

      return make_segment(point_list.front(),point_list.back());
   }

   template <typename T, std::size_t D>
   inline segment<T,D> project_onto_axis(const box<T,D>& box, const line<T,D>& axis)
   {
      std::vector< pointnd<T,D> > point_list;

      point_list.reserve(wykobi::box<T,D>::PointCount);

      for (std::size_t i = 0; i < wykobi::box<T,D>::PointCount; ++i)
      {
         point_list.push_back(closest_point_on_line_from_point(axis,box[i]));
      }

      std::sort(point_list.begin(),point_list.end());

      return make_segment(point_list.front(),point_list.back());
   }

   template <typename T, std::size_t D>
   inline segment<T,D> project_onto_axis(const quadix<T,D>& quadix, const line<T,D>& axis)
   {
      std::vector< pointnd<T,D> > point_list;

      point_list.reserve(wykobi::quadix<T,D>::PointCount);

      for (std::size_t i = 0; i < wykobi::quadix<T,D>::PointCount; ++i)
      {
         point_list.push_back(closest_point_on_line_from_point(axis,quadix[i]));
      }

      std::sort(point_list.begin(),point_list.end());

      return make_segment(point_list.front(),point_list.back());
   }

   template <typename T, std::size_t D>
   inline segment<T,D> project_onto_axis(const hypersphere<T,D>& sphere, const line<T,D>& axis)
   {
      vectornd<T,D> v = normalize(axis[0] - axis[1]);

      std::vector< pointnd<T,D> > point_list;

      point_list.reserve(2);

      point_list.push_back(closest_point_on_line_from_point(axis,sphere.center));
      point_list.push_back(closest_point_on_line_from_point(axis,point_list.front() + (v * sphere.radius)));
      point_list.push_back(closest_point_on_line_from_point(axis,point_list.front() - (v * sphere.radius)));

      std::sort(point_list.begin(),point_list.end());

      return make_segment(point_list.front(),point_list.back());
   }

   template <typename T, std::size_t D>
   inline segment<T,D> project_onto_axis(const polygon<T,D>& polygon, const line<T,D>& axis)
   {
      if (polygon.size() == 0)
         return degenerate_segmentnd<T,D>();

      std::vector< pointnd<T,D> > point_list;

      point_list.reserve(polygon.size());

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         point_list.push_back(closest_point_on_line_from_point(axis,polygon[i]));
      }

      std::sort(point_list.begin(),point_list.end());

      return make_segment(point_list.front(),point_list.back());
   }

   template <typename T, std::size_t D>
   inline T perimeter(const triangle<T,D>& triangle)
   {
      return distance(triangle[0],triangle[1]) +
             distance(triangle[1],triangle[2]) +
             distance(triangle[2],triangle[0]) ;
   }

   template <typename T, std::size_t D>
   inline T perimeter(const quadix<T,D>& quadix)
   {
      return distance(quadix[0],quadix[1]) +
             distance(quadix[1],quadix[2]) +
             distance(quadix[2],quadix[3]) +
             distance(quadix[3],quadix[0]) ;
   }

   template <typename T, std::size_t D>
   inline T perimeter(const polygon<T,D>& polygon)
   {
      T total_length = distance(polygon.back(),polygon[0]);

      for (std::size_t i = 0; i < polygon.size() - 1; ++i)
      {
         total_length += distance(polygon[i],polygon[i + 1]);

      }

      return total_length;
   }

   template <typename T, std::size_t D>
   inline pointnd<T,D> generate_random_point(const segment<T,D>& segment)
   {
      const T t = generate_random_value(T(1.0));

      return ((1 - t) * segment[0]) + (t * segment[1]);
   }

   template <typename T, std::size_t D>
   inline pointnd<T,D> generate_random_point(const triangle<T,D>& triangle)
   {
      T a = generate_random_value(T(1.0));
      T b = generate_random_value(T(1.0));

      if ((a + b) > T(1.0))
      {
         a = 1 - a;
         b = 1 - b;
      }

      const T c = (1 - a - b);

      return (triangle[0] * a) + (triangle[1] * b) + (triangle[2] * c);
   }

   template <typename T, std::size_t D>
   inline pointnd<T,D> generate_random_point(const quadix<T,D>& quadix)
   {
      const T a = (2 * generate_random_value(T(1.0))) - 1;
      const T b = (2 * generate_random_value(T(1.0))) - 1;

      const T a1 = T(1.0) - a;
      const T a2 = T(1.0) + a;

      const T b1 = T(1.0) - b;
      const T b2 = T(1.0) + b;

      const T r1 = a1 * b1;
      const T r2 = a2 * b1;
      const T r3 = a2 * b2;
      const T r4 = a1 * b2;

      return ((r1 * quadix[0]) + (r2 * quadix[1]) + (r3 * quadix[2]) + (r4 * quadix[3])) * T(0.25);
   }

   template <typename T, std::size_t D>
   inline pointnd<T,D> generate_random_point(const box<T,D>& box)
   {
      pointnd<T,D> _point;

      for (std::size_t i = 0; i < D; ++i)
      {
         _point[i] = min(box[0][i],box[1][i]) + generate_random_value(abs(box[0][i] - box[1][i]));
      }

      return _point;
   }

   template <typename T, std::size_t D>
   inline void generate_random_points(const box<T,D>& box, std::vector< pointnd<T,D> >& point_list)
   {
      for (std::size_t i = 0; i < point_list.size(); ++i)
      {
         point_list[i] = generate_random_point(box);
      }
   }

   template <typename T, std::size_t D>
   inline void generate_random_points(const segment<T,D>& segment, std::vector< pointnd<T,D> >& point_list)
   {
      for (std::size_t i = 0; i < point_list.size(); ++i)
      {
         point_list[i] = generate_random_point(segment);
      }
   }

   template <typename T, std::size_t D>
   inline void generate_random_points(const triangle<T,D>& triangle, std::vector< pointnd<T,D> >& point_list)
   {
      for (std::size_t i = 0; i < point_list.size(); ++i)
      {
         point_list[i] = generate_random_point(triangle);
      }
   }

   template <typename T, std::size_t D>
   inline void generate_random_points(const quadix<T,D>& quadix, std::vector< pointnd<T,D> >& point_list)
   {
      for (std::size_t i = 0; i < point_list.size(); ++i)
      {
         point_list[i] = generate_random_point(quadix);
      }
   }

   template <typename T, std::size_t D>
   inline T vector_norm(const vectornd<T,D>& v)
   {
      return sqrt(dot_product(v,v));
   }

   template <typename T, std::size_t D>
   inline vectornd<T,D> normalize(const vectornd<T,D>& v)
   {
      return v * (T(1.0) / vector_norm(v));
   }

   template <typename T, std::size_t D>
   inline vectornd<T,D> operator+(const vectornd<T,D>& v1, const vectornd<T,D>& v2)
   {
      vectornd<T,D> v3;

      for (std::size_t i = 0; i < D; ++i)
      {
         v3[i] = v1[i] + v2[i];
      }

      return v3;
   }

   template <typename T, std::size_t D>
   inline vectornd<T,D> operator-(const vectornd<T,D>& v1, const vectornd<T,D>& v2)
   {
      vectornd<T,D> v3;

      for (std::size_t i = 0; i < D; ++i)
      {
         v3[i] = v1[i] - v2[i];
      }

      return v3;
   }

   template <typename T, std::size_t D>
   inline T dot_product(const vectornd<T,D>& v1, const vectornd<T,D>& v2)
   {
      T result = T(0.0);

      for (std::size_t i = 0; i < D; ++i)
      {
         result += (v1[i] * v2[i]);
      }

      return result;
   }

   template <typename T, std::size_t D>
   inline vectornd<T,D> operator*(const vectornd<T,D>& v1, const T& scale)
   {
      vectornd<T,D> v3;

      for (std::size_t i = 0; i < D; ++i)
      {
         v3[i] = v1[i] * scale;
      }

      return v3;
   }

   template <typename T, std::size_t D>
   inline vectornd<T,D> operator*(const T& scale, const vectornd<T,D>& v1)
   {
      vectornd<T,D> v3;

      for (std::size_t i = 0; i < D; ++i)
      {
         v3[i] = v1[i] * scale;
      }

      return v3;
   }

   template <typename T, std::size_t D>
   inline pointnd<T,D> operator*(const pointnd<T,D>& point, const T& scale)
   {
      pointnd<T,D> _point;

      for (std::size_t i = 0; i < D; ++i)
      {
         _point[i] = point[i] * scale;
      }

      return _point;
   }

   template <typename T, std::size_t D>
   inline pointnd<T,D> operator*(const T& scale, const pointnd<T,D>& point)
   {
      pointnd<T,D> _point;

      for (std::size_t i = 0; i < D; ++i)
      {
         _point[i] = point[i] * scale;
      }

      return _point;
   }

   template <typename T, std::size_t D>
   inline vectornd<T,D> operator/(const vectornd<T,D>& v1, const T& scale)
   {
      vectornd<T,D> v2;

      for (std::size_t i = 0; i < D; ++i)
      {
         v2[i] = v1[i] / scale;
      }

      return v2;
   }

   template <typename T, std::size_t D>
   inline pointnd<T,D> operator/(const pointnd<T,D>& point, const T& scale)
   {
      pointnd<T,D> _point;

      for (std::size_t i = 0; i < D; ++i)
      {
         _point[i] = point[i] / scale;
      }

      return _point;
   }

   template <typename T, std::size_t D>
   inline pointnd<T,D> operator+(const pointnd<T,D>& point, const vectornd<T,D>& v)
   {
      pointnd<T,D> _point;

      for (std::size_t i = 0; i < D; ++i)
      {
         _point[i] = point[i] + v[i];
      }

      return _point;
   }

   template <typename T, std::size_t D>
   inline pointnd<T,D> operator+(const vectornd<T,D>& v, const pointnd<T,D>& point)
   {
      pointnd<T,D> _point;

      for (std::size_t i = 0; i < D; ++i)
      {
         _point[i] = point[i] + v[i];
      }

      return _point;
   }

   template <typename T, std::size_t D>
   inline vectornd<T,D> operator-(const pointnd<T,D>& p1, const pointnd<T,D>& p2)
   {
      vectornd<T,D> _v;

      for (std::size_t i = 0; i < D; ++i)
      {
         _v[i] = p1[i] - p2[i];
      }

      return _v;
   }

   template <typename T>
   inline T operator*(const vectornd<T,2>& v1, const vectornd<T,2>& v2)
   {
      return T((v1[0] * v2[1]) - (v1[1] * v2[0]));
   }

   template <typename T>
   inline vectornd<T,3> operator*(const vectornd<T,3>& v1, const vectornd<T,3>& v2)
   {
      return vectornd<T,3>( v1[1] * v2[2] - v1[2] * v2[1],
                            v1[2] * v2[0] - v1[0] * v2[2],
                            v1[0] * v2[1] - v1[1] * v2[0]);
   }

   template <typename T, std::size_t D>
   inline pointnd<T,D> operator+(const pointnd<T,D>& p1, const pointnd<T,D>& p2)
   {
      pointnd<T,D> _point;

      for (std::size_t i = 0; i < D; ++i)
      {
         _point[i] = p1[i] + p2[i];
      }

      return _point;
   }

   template <typename T, std::size_t D>
   inline bool operator < (const pointnd<T,D>& point1, const pointnd<T,D>& point2)
   {
      for (std::size_t i = 0; i < D; ++i)
      {
         if (point1[i] < point2[i])
         {
            return true;
         }
         else if (point1[i] > point2[i])
         {
            return false;
         }
      }

      return true;
   }

   template <typename T, std::size_t D>
   inline bool operator > (const pointnd<T,D>& point1, const pointnd<T,D>& point2)
   {
      for (std::size_t i = 0; i < D; ++i)
      {
         if (point1[i] > point2[i])
         {
            return true;
         }
         else if (point1[i] < point2[i])
         {
            return false;
         }
      }

      return true;
   }

   template <typename T, std::size_t D>
   inline bool operator == (const pointnd<T,D>& point1, const pointnd<T,D>& point2)
   {
      return is_equal(point1,point2);
   }

   template <typename T, std::size_t D>
   inline bool is_equal(const pointnd<T,D>& point1, const pointnd<T,D>& point2, const T& epsilon)
   {
      for (std::size_t i = 0; i < D; ++i)
      {
         if (!is_equal(point1[i],point2[i],epsilon))
         {
            return false;
         }
      }

      return true;
   }

   template <typename T, std::size_t D>
   inline bool is_equal(const pointnd<T,D>& point1, const pointnd<T,D>& point2)
   {
      for (std::size_t i = 0; i < D; ++i)
      {
         if (!is_equal(point1[i],point2[i],T(Epsilon)))
         {
            return false;
         }
      }

      return true;
   }

   template <typename T, std::size_t D>
   inline bool not_equal(const pointnd<T,D>& point1, const pointnd<T,D>& point2, const T& epsilon)
   {
      return !is_equal(point1,point2,epsilon);
   }

   template <typename T, std::size_t D>
   inline bool not_equal(const pointnd<T,D>& point1, const pointnd<T,D>& point2)
   {
      return !is_equal(point1,point2);
   }

   template <typename T, std::size_t D>
   inline pointnd<T,D> degenerate_pointnd()
   {
      pointnd<T,D> _point;

      for (std::size_t i = 0; i < D; ++i)
      {
         _point[i] = infinity<T>();
      }

      return _point;
   }

   template <typename T, std::size_t D>
   inline vectornd<T,D> degenerate_vectornd()
   {
      vectornd<T,D> _vector;

      for (std::size_t i = 0; i < D; ++i)
      {
         _vector[i] = infinity<T>();
      }

      return _vector;
   }

   template <typename T, std::size_t D>
   inline ray<T,D> degenerate_raynd()
   {
      return make_ray(degenerate_pointnd<T,D>(),degenerate_vectornd<T,D>());
   }

   template <typename T, std::size_t D>
   inline line<T,D> degenerate_linend()
   {
      return make_line(degenerate_pointnd<T,D>(),degenerate_pointnd<T,D>());
   }

   template <typename T, std::size_t D>
   inline segment<T,D> degenerate_segmentnd()
   {
      return make_segment(degenerate_pointnd<T,D>(),degenerate_pointnd<T,D>());
   }

   template <typename T, std::size_t D>
   inline triangle<T,D> degenerate_trianglend()
   {
      return make_triangle(degenerate_pointnd<T,D>(),
                           degenerate_pointnd<T,D>(),
                           degenerate_pointnd<T,D>());
   }

   template <typename T, std::size_t D>
   inline quadix<T,D> degenerate_quadixnd()
   {
      return make_quadix(degenerate_pointnd<T,D>(),
                         degenerate_pointnd<T,D>(),
                         degenerate_pointnd<T,D>(),
                         degenerate_pointnd<T,D>());
   }

   template <typename T, std::size_t D>
   inline box<T,D> degenerate_box()
   {
      return make_box(degenerate_pointnd<T,D>(),degenerate_pointnd<T,D>());
   }

   template <typename T, std::size_t D>
   inline hypersphere<T,D> degenerate_hypersphere()
   {
      return make_sphere(degenerate_pointnd<T,D>(),+infinity<T>());
   }

   template <typename T, std::size_t D>
   inline pointnd<T,D> positive_infinite_pointnd()
   {
      pointnd<T,D> _point;

      for (std::size_t i = 0; i < D; ++i)
      {
         _point[i] = +infinity<T>();
      }

      return _point;
   }

   template <typename T, std::size_t D>
   inline pointnd<T,D> negative_infinite_pointnd()
   {
      pointnd<T,D> _point;

      for (std::size_t i = 0; i < D; ++i)
      {
         _point[i] = -infinity<T>();
      }

      return _point;
   }

   template <typename T, std::size_t D>
   inline void swap(pointnd<T,D>& point1, pointnd<T,D>& point2)
   {
      for (std::size_t i = 0; i < D; ++i)
      {
         T temp = point1[i];
         point1[i] = point2[i];
         point2[i] = temp;
      }
   }

   template <typename T, std::size_t D>
   inline vectornd<T,D> make_vector(const pointnd<T,D>& point)
   {
      vectornd<T,D> vec;

      for (std::size_t i = 0; i < D; ++i)  vec[i] = point[i];

      return vec;
   }

   template <typename T, std::size_t D>
   inline ray<T,D> make_ray(const pointnd<T,D>& origin, const vectornd<T,D>& direction)
   {
      ray<T,D> _ray;

      _ray.origin    = origin;
      _ray.direction = direction;

      return _ray;
   }

   template <typename T, std::size_t D>
   inline segment<T,D> make_segment(const pointnd<T,D>& point1, const pointnd<T,D>& point2)
   {
      segment<T,D> _segment;

      _segment[0] = point1;
      _segment[1] = point2;

      return _segment;
   }

   template <typename T, std::size_t D>
   inline line<T,D> make_line(const pointnd<T,D>& point1, const pointnd<T,D>& point2)
   {
      line<T,D> _line;

      _line[0] = point1;
      _line[1] = point2;

      return _line;
   }

   template <typename T, std::size_t D>
   inline box<T,D> make_box(const pointnd<T,D>& point1, const pointnd<T,D>& point2)
   {
      box<T,D> _box;

      _box[0] = point1;
      _box[1] = point2;

      return _box;
   }

   template <typename T, std::size_t D>
   inline triangle<T,D> make_triangle(const pointnd<T,D>& point1, const pointnd<T,D>& point2, const pointnd<T,D>& point3)
   {
      triangle<T,D> _triangle;

      _triangle[0] = point1;
      _triangle[1] = point2;
      _triangle[2] = point3;

      return _triangle;
   }

   template <typename T, std::size_t D>
   inline quadix<T,D> make_quadix(const pointnd<T,D>& point1, const pointnd<T,D>& point2, const pointnd<T,D>& point3, const pointnd<T,D>& point4)
   {
      quadix<T,D> _quadix;

      _quadix[0] = point1;
      _quadix[1] = point2;
      _quadix[2] = point3;
      _quadix[3] = point4;

      return _quadix;
   }

   template <typename T, std::size_t D>
   inline hypersphere<T,D> make_sphere(const pointnd<T,D>& point, const T& radius)
   {
      hypersphere<T,D> sphere;

      sphere.center = point;
      sphere.radius = radius;

      return sphere;
   }

   template <typename T, std::size_t D>
   inline hypersphere<T,D> make_sphere(const pointnd<T,D>& point1, const pointnd<T,D>& point2)
   {
      hypersphere<T,D> sphere;

      sphere.center = (point1 + point2) * T(0.5);
      sphere.radius = distance(point1,point2) * T(0.5);

      return sphere;
   }

   template <typename T, std::size_t D>
   inline polygon<T,D> make_polygon(const std::vector< pointnd<T,D> >& point_list)
   {
      polygon<T,D> _polygon;

      _polygon.reserve(point_list.size());

      for (std::size_t i = 0; i < point_list.size(); ++i)
      {
         _polygon.push_back(point_list[i]);
      }

      return _polygon;
   }

   template <typename T, std::size_t D>
   inline polygon<T,D> make_polygon(const triangle<T,D>& triangle)
   {
      polygon<T,D> polygon;

      polygon.reserve(wykobi::triangle<T,D>::PointCount);

      for (std::size_t i = 0; i < wykobi::triangle<T,D>::PointCount; ++i)
      {
         polygon.push_back(triangle[i]);
      }

      return polygon;
   }

   template <typename T, std::size_t D>
   inline polygon<T,D> make_polygon(const quadix<T,D>& quadix)
   {
      polygon<T,D> polygon;

      polygon.reserve(wykobi::quadix<T,D>::PointCount);

      for (std::size_t i = 0; i < wykobi::quadix<T,D>::PointCount; ++i)
      {
         polygon.push_back(quadix[i]);
      }

      return polygon;
   }

} // namespace wykobi
