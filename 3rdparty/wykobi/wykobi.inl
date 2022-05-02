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


namespace wykobi
{
   template <typename T>
   inline int orientation(const T& x1, const T& y1,
                          const T& x2, const T& y2,
                          const T& px, const T& py)
   {
      const T orin = (x2 - x1) * (py - y1) - (px - x1) * (y2 - y1);

      if (orin > T(0.0))      return LeftHandSide;         /* Orientaion is to the left-hand side  */
      else if (orin < T(0.0)) return RightHandSide;        /* Orientaion is to the right-hand side */
      else                    return CollinearOrientation; /* Orientaion is neutral aka collinear  */
   }

   template <typename T>
   inline int orientation(const T& x1, const T& y1, const T& z1,
                          const T& x2, const T& y2, const T& z2,
                          const T& x3, const T& y3, const T& z3,
                          const T& px, const T& py, const T& pz)
   {
      const T px1 = x1 - px;
      const T px2 = x2 - px;
      const T px3 = x3 - px;

      const T py1 = y1 - py;
      const T py2 = y2 - py;
      const T py3 = y3 - py;

      const T pz1 = z1 - pz;
      const T pz2 = z2 - pz;
      const T pz3 = z3 - pz;

      const T orin = px1 * (py2 * pz3 - pz2 * py3) +
                     px2 * (py3 * pz1 - pz3 * py1) +
                     px3 * (py1 * pz2 - pz1 * py2) ;

      if (orin < T(0.0))      return BelowOrientation;    /* Orientaion is below plane                      */
      else if (orin > T(0.0)) return AboveOrientation;    /* Orientaion is above plane                      */
      else                    return CoplanarOrientation; /* Orientaion is coplanar to plane if Result is 0 */
   }

   template <typename T>
   inline int robust_orientation(const T& x1, const T& y1,
                                 const T& x2, const T& y2,
                                 const T& px, const T& py)
   {
      const T orin = (x2 - x1) * (py - y1) - (px - x1) * (y2 - y1);

      /*
         Calculation Policy:
         if |Orin - Orin`| < Epsilon then Orin` is assumed to be equal to zero.
         Where:
           Orin : is the "real" mathematically precise orientation value, using infinite
                  precision arithmetic (hypothetical)
           Orin`: is the calculated imprecise orientation value, using finite precision
                  arithmetic
      */
      if (is_equal(orin,T(0.0))) return CollinearOrientation; /* orientaion is neutral aka collinear  */
      else if (orin < T(0.0))    return RightHandSide;        /* orientaion is to the right-hand side */
      else                       return LeftHandSide;         /* orientaion is to the left-hand side  */
   }

   template <typename T>
   inline int robust_orientation(const T& x1, const T& y1, const T& z1,
                                 const T& x2, const T& y2, const T& z2,
                                 const T& x3, const T& y3, const T& z3,
                                 const T& px, const T& py, const T& pz)
   {
      const T px1 = x1 - px;
      const T px2 = x2 - px;
      const T px3 = x3 - px;

      const T py1 = y1 - py;
      const T py2 = y2 - py;
      const T py3 = y3 - py;

      const T pz1 = z1 - pz;
      const T pz2 = z2 - pz;
      const T pz3 = z3 - pz;

      const T orin = px1 * (py2 * pz3 - pz2 * py3) +
                     px2 * (py3 * pz1 - pz3 * py1) +
                     px3 * (py1 * pz2 - pz1 * py2) ;

      if (is_equal(orin,T(0.0))) return CoplanarOrientation; /* Orientaion is coplanar to plane if Result is 0 */
      else if (orin < T(0.0))    return BelowOrientation;    /* Orientaion is below plane                      */
      else                       return AboveOrientation;    /* Orientaion is above plane                      */
   }

   template <typename T>
   inline int orientation(const point2d<T>& point1,
                          const point2d<T>& point2,
                          const T&           px,
                          const T&           py)
   {
      return orientation(point1.x,point1.y,point2.x,point2.y,px,py);
   }

   template <typename T>
   inline int orientation(const point2d<T>& point1,
                          const point2d<T>& point2,
                          const point2d<T>& point3)
   {
      return orientation(point1.x,point1.y,point2.x,point2.y,point3.x,point3.y);
   }

   template <typename T>
   inline int orientation(const line<T,2>&  line,
                          const point2d<T>& point)
   {
      return orientation(line[0],line[1],point);
   }

   template <typename T>
   inline int orientation(const segment<T,2>& segment,
                          const point2d<T>&   point)
   {
      return orientation(segment[0],segment[1],point);
   }

   template <typename T>
   inline int orientation(const triangle<T,2>& triangle)
   {
      return orientation(triangle[0],triangle[1],triangle[2]);
   }

   template <typename T>
   inline int orientation(const point3d<T>& point1,
                          const point3d<T>& point2,
                          const point3d<T>& point3,
                          const T&           px,
                          const T&           py,
                          const T&           pz)
   {
      return orientation(point1.x, point1.y, point1.z,
                         point2.x, point2.y, point2.z,
                         point3.x, point3.y, point3.z,
                               px,       py,       pz);
   }

   template <typename T>
   inline int orientation(const point3d<T>& point1,
                          const point3d<T>& point2,
                          const point3d<T>& point3,
                          const point3d<T>& point4)
   {
      return orientation(point1.x, point1.y, point1.z,
                         point2.x, point2.y, point2.z,
                         point3.x, point3.y, point3.z,
                         point4.x, point4.y, point4.z);
   }

   template <typename T>
   inline int orientation(const triangle<T,3>& triangle,
                          const point3d<T>&    point)
   {
      return orientation(triangle[0],triangle[1],triangle[2],point);
   }

   template <typename T>
   inline bool differing_orientation(const T& x1,  const T& y1,
                                     const T& x2,  const T& y2,
                                     const T& p1x, const T& p1y,
                                     const T& p2x, const T& p2y)
   {
      /* Collinear orientation is not considered */
      return ((orientation(x1,y1,x2,y2,p1x,p1y) * orientation(x1,y1,x2,y2,p2x,p2y)) == -1);
   }

   template <typename T>
   inline bool differing_orientation(const point2d<T>& p1, const point2d<T>& p2,
                                     const point2d<T>& q1, const point2d<T>& q2)
   {
      return differing_orientation(p1.x,p1.y,p2.x,p2.y,q1.x,q1.y,q2.x,q2.y);
   }

   template <typename T>
   inline int in_circle(const T& x1, const T& y1,
                        const T& x2, const T& y2,
                        const T& x3, const T& y3,
                        const T& px, const T& py)
   {
      const T dx1 = x1 - px;
      const T dy1 = y1 - py;
      const T dx2 = x2 - px;
      const T dy2 = y2 - py;
      const T dx3 = x3 - px;
      const T dy3 = y3 - py;

      const T det1  = dx1 * dy2 - dx2 * dy1;
      const T det2  = dx2 * dy3 - dx3 * dy2;
      const T det3  = dx3 * dy1 - dx1 * dy3;
      const T lift1 = dx1 * dx1 + dy1 * dy1;
      const T lift2 = dx2 * dx2 + dy2 * dy2;
      const T lift3 = dx3 * dx3 + dy3 * dy3;

      const T result = lift1 * det2 + lift2 * det3 + lift3 * det1;

      if (is_equal(result,T(0.0))) return Cocircular;
      else if (result > T(0.0))    return PointInside;
      else                         return PointOutside;
   }

   template <typename T>
   inline int in_circle(const point2d<T>& point1,
                        const point2d<T>& point2,
                        const point2d<T>& point3,
                        const point2d<T>& point4)
   {
      return in_circle(point1.x, point1.y,
                       point2.x, point2.y,
                       point3.x, point3.y,
                       point4.x, point4.y);
   }

   template <typename T>
   inline int in_circle(const triangle<T,2>& triangle, const point2d<T>& point)
   {
      return in_circle(triangle[0],triangle[1],triangle[2],point);
   }

   template <typename T>
   inline int in_sphere(const T& x1, const T& y1, const T& z1,
                        const T& x2, const T& y2, const T& z2,
                        const T& x3, const T& y3, const T& z3,
                        const T& x4, const T& y4, const T& z4,
                        const T& px, const T& py, const T& pz)
   {
      const T dx1 = x1 - px;
      const T dx2 = x2 - px;
      const T dx3 = x3 - px;
      const T dx4 = x4 - px;
      const T dy1 = y1 - py;
      const T dy2 = y2 - py;
      const T dy3 = y3 - py;
      const T dy4 = y4 - py;
      const T dz1 = z1 - pz;
      const T dz2 = z2 - pz;
      const T dz3 = z3 - pz;
      const T dz4 = z4 - pz;

      const T ab = dx1 * dy2 - dx2 * dy1;
      const T bc = dx2 * dy3 - dx3 * dy2;
      const T cd = dx3 * dy4 - dx4 * dy3;
      const T da = dx4 * dy1 - dx1 * dy4;
      const T ac = dx1 * dy3 - dx3 * dy1;
      const T bd = dx2 * dy4 - dx4 * dy2;

      const T abc = dz1 * bc - dz2 * ac + dz3 * ab;
      const T bcd = dz2 * cd - dz3 * bd + dz4 * bc;
      const T cda = dz3 * da + dz4 * ac + dz1 * cd;
      const T dab = dz4 * ab + dz1 * bd + dz2 * da;

      const T alift = dx1 * dx1 + dy1 * dy1 + dz1 * dz1;
      const T blift = dx2 * dx2 + dy2 * dy2 + dz2 * dz2;
      const T clift = dx3 * dx3 + dy3 * dy3 + dz3 * dz3;
      const T dlift = dx4 * dx4 + dy4 * dy4 + dz4 * dz4;

      const T result = (dlift * abc - clift * dab) + (blift * cda - alift * bcd);

      if (is_equal(result,T(0.0))) return Cospherical;
      else if (result > T(0.0))    return PointInside;
      else                         return PointOutside;
   }

   template <typename T>
   inline int in_sphere(const point3d<T>& point1,
                        const point3d<T>& point2,
                        const point3d<T>& point3,
                        const point3d<T>& point4,
                        const point3d<T>& point5)
   {
      return in_sphere(point1.x,point1.y,point1.z,
                       point2.x,point2.y,point2.z,
                       point3.x,point3.y,point3.z,
                       point4.x,point4.y,point4.z,
                       point5.x,point5.y,point5.z);
   }

   template <typename T>
   inline int in_sphere(const quadix<T,3>& quadix, const point3d<T>& point)
   {
      return in_sphere(quadix[0],quadix[1],quadix[2],quadix[3],point);
   }

   template <typename T>
   inline T signed_area(const T& x1, const T& y1,
                        const T& x2, const T& y2,
                        const T& px, const T& py)
   {
      return (x2 - x1) * (py - y1) - (px - x1) * (y2 - y1);
   }

   template <typename T>
   inline T signed_area(const point2d<T>& point1, const point2d<T>& point2, const T& px, const T& py)
   {
      return signed_area(point1.x,point1.y,point2.x,point2.y,px,py);
   }

   template <typename T>
   inline T signed_area(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3)
   {
      return signed_area(point1.x,point1.y,point2.x,point2.y,point3.x,point3.y);
   }

   template <typename T>
   inline T signed_area(const segment<T,2>& segment, const point2d<T>& point)
   {
      return signed_area(segment[0],segment[1],point);
   }

   template <typename T>
   inline T signed_volume(const T& x1, const T& y1, const T& z1,
                          const T& x2, const T& y2, const T& z2,
                          const T& x3, const T& y3, const T& z3,
                          const T& px, const T& py, const T& pz)
   {
      const T px1 = x1 - px;
      const T px2 = x2 - px;
      const T px3 = x3 - px;

      const T py1 = y1 - py;
      const T py2 = y2 - py;
      const T py3 = y3 - py;

      const T pz1 = z1 - pz;
      const T pz2 = z2 - pz;
      const T pz3 = z3 - pz;

      return px1 * (py2 * pz3 - pz2 * py3) +
             px2 * (py3 * pz1 - pz3 * py1) +
             px3 * (py1 * pz2 - pz1 * py2);
   }

   template <typename T>
   inline T signed_volume(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const T& px, const T& py, const T& pz)
   {
      return signed_volume(point1.x, point1.y, point1.z,
                           point2.x, point2.y, point2.z,
                           point3.x, point3.y, point3.z,
                           px,       py,       pz      );
   }

   template <typename T>
   inline T signed_volume(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const point3d<T>& point4)
   {
      return signed_volume(point1.x, point1.y, point1.z,
                           point2.x, point2.y, point2.z,
                           point3.x, point3.y, point3.z,
                           point4.x, point4.y, point4.z);
   }

   template <typename T>
   inline T signed_volume(const triangle<T,3>& triangle, const point3d<T>& point)
   {
      return signed_volume(triangle[0],triangle[1],triangle[2],point);
   }

   template <typename T>
   inline bool collinear(const T& x1, const T& y1,
                         const T& x2, const T& y2,
                         const T& x3, const T& y3, const T& epsilon)
   {
      return is_equal((x2 - x1) * (y3 - y1) ,(x3 - x1) * (y2 - y1),epsilon);
   }

   template <typename T>
   inline bool collinear(const T& x1, const T& y1, const T& z1,
                         const T& x2, const T& y2, const T& z2,
                         const T& x3, const T& y3, const T& z3,
                         const T& epsilon)
   {
      const T dx1 = x2 - x1;
      const T dy1 = y2 - y1;
      const T dz1 = z2 - z1;
      const T dx2 = x3 - x1;
      const T dy2 = y3 - y1;
      const T dz2 = z3 - z1;
      const T cx = (dy1 * dz2) - (dy2 * dz1);
      const T cy = (dx2 * dz1) - (dx1 * dz2);
      const T cz = (dx1 * dy2) - (dx2 * dy1);

      return is_equal(cx * cx + cy * cy + cz * cz,epsilon);
   }

   template <typename T>
   inline bool collinear(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3)
   {
      return collinear(point1.x,point1.y,point2.x,point2.y,point3.x,point3.y);
   }

   template <typename T>
   inline bool collinear(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3)
   {
      return collinear(point1.x,point1.y,point1.z,
                       point2.x,point2.y,point2.z,
                       point3.x,point3.y,point3.z);
   }

   template <typename T>
   inline bool robust_collinear(const T& x1, const T& y1,
                                const T& x2, const T& y2,
                                const T& x3, const T& y3, const T& epsilon)
   {
      const T leydist1 = lay_distance(x1,y1,x2,y2);
      const T leydist2 = lay_distance(x2,y2,x3,y3);
      const T leydist3 = lay_distance(x3,y3,x1,y1);

      if (leydist1 >= leydist2)
         if (leydist1 >= leydist3)
            return is_equal(minimum_distance_from_point_to_line(x3,y3,x1,y1,x2,y2),T(0.0),epsilon);
         else
            return is_equal(minimum_distance_from_point_to_line(x2,y2,x3,y3,x1,y1),T(0.0),epsilon);
      else if (leydist2 >= leydist3)
         return is_equal(minimum_distance_from_point_to_line(x1,y1,x2,y2,x3,y3),T(0.0),epsilon);
      else
         return is_equal(minimum_distance_from_point_to_line(x2,y2,x3,y3,x1,y1),T(0.0),epsilon);
   }

   template <typename T>
   inline bool robust_collinear(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3, const T& epsilon)
   {
      return robust_collinear(point1.x,point1.y,point2.x,point2.y,point3.x,point3.y,epsilon);
   }

   template <typename T>
   inline bool robust_collinear(const T& x1, const T& y1, const T& z1,
                                const T& x2, const T& y2, const T& z2,
                                const T& x3, const T& y3, const T& z3, const T& epsilon)
   {
      const T leydist1 = lay_distance(x1,y1,z1,x2,y2,z2);
      const T leydist2 = lay_distance(x2,y2,z2,x3,y3,z3);
      const T leydist3 = lay_distance(x3,y3,z3,x1,y1,z1);

      if (leydist1 >= leydist2)
         if (leydist1 >= leydist3)
            return is_equal(minimum_distance_from_point_to_line(x3,y3,z3,x1,y1,z1,x2,y2,z2),T(0.0),epsilon);
         else
            return is_equal(minimum_distance_from_point_to_line(x2,y2,z2,x3,y3,z3,x1,y1,z1),T(0.0),epsilon);
      else if (leydist2 >= leydist3)
         return is_equal(minimum_distance_from_point_to_line(x1,y1,z1,x2,y2,z2,x3,y3,z3),T(0.0),epsilon);
      else
         return is_equal(minimum_distance_from_point_to_line(x2,y2,z2,x3,y3,z3,x1,y1,z1),T(0.0),epsilon);
   }

   template <typename T>
   inline bool robust_collinear(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const T& epsilon)
   {
      return robust_collinear(point1.x, point1.y, point1.z,
                              point2.x, point2.y, point2.z,
                              point3.x, point3.y, point3.z, epsilon);
   }

   template <typename T>
   inline bool robust_collinear(const line<T,2>& line, const point2d<T>& point, const T& epsilon)
   {
      return robust_collinear(line[0],line[1],point,epsilon);
   }

   template <typename T>
   inline bool robust_collinear(const line<T,3>& line, const point3d<T>& point, const T& epsilon)
   {
      return robust_collinear(line[0],line[1],point,epsilon);
   }

   template <typename T>
   inline bool is_point_collinear(const T& x1, const T& y1,
                                  const T& x2, const T& y2,
                                  const T& px, const T& py,
                                  const bool robust)
   {
      /*
         This method will return true iff the point (px,py) is collinear
         to points (x1,y1) and (x2,y2) and exists on the line segment A(x1,y1)->B(x2,y2)
      */
      if (
           ((less_than_or_equal(x1,px) && less_than_or_equal(px,x2))  ||
            (less_than_or_equal(x2,px) && less_than_or_equal(px,x1))) &&
           ((less_than_or_equal(y1,py) && less_than_or_equal(py,y2))  ||
            (less_than_or_equal(y2,py) && less_than_or_equal(py,y1)))
         )
      {
         if (robust)
           return robust_collinear(x1,y1,x2,y2,px,py);
         else
           return collinear(x1,y1,x2,y2,px,py);
      }

      return false;
   }

   template <typename T>
   inline bool is_point_collinear(const point2d<T>& point1,
                                  const point2d<T>& point2,
                                  const point2d<T>& point3,
                                  const bool robust)
   {
      return is_point_collinear(point1.x,point1.y,
                                point2.x,point2.y,
                                point3.x,point3.y,
                                robust);
   }

   template <typename T>
   inline bool is_point_collinear(const point2d<T>& point1,
                                  const point2d<T>& point2,
                                  const T& px, const T& py,
                                  const bool robust)
   {
      return is_point_collinear(point1.x,point1.y,
                                point2.x,point2.y,
                                px      ,py      ,
                                robust);
   }

   template <typename T>
   inline bool is_point_collinear(const segment<T,2>& segment,
                                  const point2d<T>&   point,
                                  const bool robust)
   {
      return is_point_collinear(segment[0],segment[1],point,robust);
   }

   template <typename T>
   inline bool is_point_collinear(const T& x1, const T& y1, const T& z1,
                                  const T& x2, const T& y2, const T& z2,
                                  const T& px, const T& py, const T& pz,
                                  const bool robust)
   {
      /*
        This method will return true iff the point (px,py,pz) is collinear
        to points (x1,y1,z1) and (x2,y2,z2) and exists on the line segment A(x1,y1,z1)->B(x2,y2,z2)
      */
      if (
           ((less_than_or_equal(x1,px) && less_than_or_equal(px,x2))  ||
            (less_than_or_equal(x2,px) && less_than_or_equal(px,x1))) &&
           ((less_than_or_equal(y1,py) && less_than_or_equal(py,y2))  ||
            (less_than_or_equal(y2,py) && less_than_or_equal(py,y1))) &&
           ((less_than_or_equal(z1,pz) && less_than_or_equal(pz,z2))  ||
            (less_than_or_equal(z2,pz) && less_than_or_equal(pz,z1)))
         )
      {
         if (robust)
            return robust_collinear(x1,y1,z1,x2,y2,z2,px,py,pz);
         else
            return collinear(x1,y1,z1,x2,y2,z2,px,py,pz);
      }

      return false;
   }

   template <typename T>
   inline bool is_point_collinear(const point3d<T>& point1,
                                  const point3d<T>& point2,
                                  const point3d<T>& point3,
                                  const bool robust)
   {
      return is_point_collinear(point1.x, point1.y, point1.z,
                                point2.x, point2.y, point2.z,
                                point3.x, point3.y, point3.z, robust);
   }

   template <typename T>
   inline bool is_point_collinear(const segment<T,3>& segment,
                                  const point3d<T>&   point,
                                  const bool robust)
   {
      return is_point_collinear(segment[0],segment[1],point,robust);
   }

   template <typename T>
   inline bool robust_coplanar(const point3d<T> point1,
                               const point3d<T> point2,
                               const point3d<T> point3,
                               const point3d<T> point4,
                               const T& epsilon)
   {
      return less_than_or_equal(lay_distance(point4,make_plane(point1,point2,point3)),sqr(epsilon));
   }

   template <typename T>
   inline bool coplanar(const ray<T,3>& ray1, const ray<T,3>& ray2)
   {
      const point3d<T> pnt1 = generate_point_on_ray(ray1,T(1.0));
      const point3d<T> pnt2 = generate_point_on_ray(ray2,T(1.0));

      if (robust_collinear(ray1.origin,pnt1,pnt2) && robust_collinear(ray2.origin,pnt2,pnt1))
         return true;
      else
         return robust_coplanar(ray1.origin,ray2.origin,pnt1,pnt2);
   }

   template <typename T>
   inline bool coplanar(const segment<T,3>& segment1, const segment<T,3>& segment2)
   {
      if (
           robust_collinear(segment1[0],segment1[1],segment2[0]) &&
           robust_collinear(segment1[0],segment1[1],segment2[1])
         )
         return true;
      else
         return robust_coplanar(segment1[0],segment1[1],segment2[0],segment2[1]);
   }

   template <typename T>
   inline bool coplanar(const line<T,3>& line1, const line<T,3>& line2)
   {
      if (
           robust_collinear(line1[0],line1[1],line2[0]) &&
           robust_collinear(line1[0],line1[1],line2[1])
         )
         return true;
      else
         return robust_coplanar(line1[0],line1[1],line2[0],line2[1]);
   }

   template <typename T>
   inline bool coplanar(const triangle<T,3>& triangle1, const triangle<T,3>& triangle2)
   {
      return robust_coplanar(triangle1[0], triangle1[1], triangle1[2], triangle2[0]) &&
             robust_coplanar(triangle1[0], triangle1[1], triangle1[2], triangle2[1]) &&
             robust_coplanar(triangle1[0], triangle1[1], triangle1[2], triangle2[2]) ;
   }

   template <typename T>
   inline bool coplanar(const quadix<T,3>& quadix1, const quadix<T,3>& quadix2)
   {
      return robust_coplanar(quadix1[0], quadix1[1], quadix1[2], quadix2[0]) &&
             robust_coplanar(quadix1[0], quadix1[1], quadix1[2], quadix2[1]) &&
             robust_coplanar(quadix1[0], quadix1[1], quadix1[2], quadix2[2]) &&
             robust_coplanar(quadix1[0], quadix1[1], quadix1[2], quadix2[3]) ;
   }

   template <typename T>
   inline bool cocircular(const T& x1, const T& y1,
                          const T& x2, const T& y2,
                          const T& x3, const T& y3,
                          const T& x4, const T& y4,
                          const T& epsilon)
   {
      const circle<T> circle = circumcircle(x1,y1,x2,y2,x3,y3);

      return is_equal(distance(x4,y4,circle.x,circle.y),circle.radius,epsilon);
   }

   template <typename T>
   inline bool cocircular(const point2d<T>& point1,
                          const point2d<T>& point2,
                          const point2d<T>& point3,
                          const point2d<T>& point4,
                          const T& epsilon)
   {
      return cocircular(point1.x, point1.y,
                        point2.x, point2.y,
                        point3.x, point3.y,
                        point4.x, point4.y,epsilon);
   }

   template <typename T>
   inline bool cocircular(const triangle<T,2>& triangle, const point2d<T>& point, const T& epsilon)
   {
      return cocircular(triangle[0],triangle[1],triangle[2],point,epsilon);
   }

   template <typename T>
   inline bool cocircular(const circle<T>& circle, const point2d<T>& point, const T& epsilon)
   {
      return is_equal(distance(point.x,point.y,circle.x,circle.y),circle.radius,epsilon);
   }

   template <typename T>
   inline bool is_skinny_triangle(const T& x1, const T& y1,
                                  const T& x2, const T& y2,
                                  const T& x3, const T& y3)
   {
      /*
         L. Paul Chew, Guaranteed-Quality Triangular Meshes,
         Technical Report TR-89-983,
         Department of Computer Science, Cornell University, 1989.
         Skinny only if circumradius-to-shortest edge ratio > 1.
      */
      const circle<T> cir_circle = circumcircle(x1,y1,x2,y2,x3,y3);

      const T shortest_length = sqrt(min(lay_distance(x1, y1, x2, y2),
                                         lay_distance(x1, y1, x3, y3),
                                         lay_distance(x2, y2, x3, y3)));

      return ((cir_circle.radius / shortest_length) > T(1.0));
   }

   template <typename T>
   inline bool is_skinny_triangle(const point2d<T>& point1,
                                  const point2d<T>& point2,
                                  const point2d<T>& point3)
   {
      return is_skinny_triangle(point1.x,point1.y,
                                point2.x,point2.y,
                                point3.x,point3.y);
   }

   template <typename T>
   inline bool is_skinny_triangle(const triangle<T,2>& triangle)
   {
     return is_skinny_triangle(triangle[0],triangle[1],triangle[2]);
   }

   template <typename T>
   inline bool intersect(const T& x1, const T& y1,
                         const T& x2, const T& y2,
                         const T& x3, const T& y3,
                         const T& x4, const T& y4)
   {
      const T ax = x2 - x1;
      const T bx = x3 - x4;

      T lowerx;
      T upperx;
      T uppery;
      T lowery;

      if (ax < T(0.0))
      {
         lowerx = x2;
         upperx = x1;
      }
      else
      {
         upperx = x2;
         lowerx = x1;
      }

      if (bx > T(0.0))
      {
         if ((upperx < x4) || (x3 < lowerx))
         return false;
      }
      else if ((upperx < x3) || (x4 < lowerx))
         return false;

      const T ay = y2 - y1;
      const T by = y3 - y4;

      if (ay < T(0.0))
      {
         lowery = y2;
         uppery = y1;
      }
      else
      {
         uppery = y2;
         lowery = y1;
      }

      if (by > T(0.0))
      {
         if ((uppery < y4) || (y3 < lowery))
            return false;
      }
      else if ((uppery < y3) || (y4 < lowery))
         return false;

      const T cx = x1 - x3;
      const T cy = y1 - y3;
      const T  d = (by * cx) - (bx * cy);
      const T  f = (ay * bx) - (ax * by);

      if (f > T(0.0))
      {
         if ((d < T(0.0)) || (d > f))
            return false;
      }
      else if ((d > T(0.0)) || (d < f))
         return false;

      const T e = (ax * cy) - (ay * cx);

      if (f > T(0.0))
      {
         if ((e < T(0.0)) || (e > f))
            return false;
      }
      else if ((e > T(0.0)) || (e < f))
         return false;

      return true;
   }

   template <typename T>
   inline bool intersect(const T& x1, const T& y1,
                         const T& x2, const T& y2,
                         const T& x3, const T& y3,
                         const T& x4, const T& y4,
                               T& ix,       T& iy)
   {
      const T ax = x2 - x1;
      const T bx = x3 - x4;

      T lowerx;
      T upperx;
      T uppery;
      T lowery;

      if (ax < T(0.0))
      {
         lowerx = x2;
         upperx = x1;
      }
      else
      {
         upperx = x2;
         lowerx = x1;
      }

      if (bx > T(0.0))
      {
         if ((upperx < x4) || (x3 < lowerx))
            return false;
      }
      else if ((upperx < x3) || (x4 < lowerx))
         return false;

      const T ay = y2 - y1;
      const T by = y3 - y4;

      if (ay < T(0.0))
      {
         lowery = y2;
         uppery = y1;
      }
      else
      {
         uppery = y2;
         lowery = y1;
      }

      if (by > T(0.0))
      {
         if ((uppery < y4) || (y3 < lowery))
            return false;
      }
      else if ((uppery < y3) || (y4 < lowery))
         return false;

      const T cx = x1 - x3;
      const T cy = y1 - y3;
      const T d  = (by * cx) - (bx * cy);
      const T f  = (ay * bx) - (ax * by);

      if (f > T(0.0))
      {
         if ((d < T(0.0)) || (d > f))
            return false;
      }
      else if ((d > T(0.0)) || (d < f))
         return false;

      const T e = (ax * cy) - (ay * cx);

      if (f > T(0.0))
      {
         if ((e < T(0.0)) || (e > f))
            return false;
      }
      else if ((e > T(0.0)) || (e < f))
         return false;

      T ratio = (ax * -by) - (ay * -bx);

      if (not_equal(ratio,T(0.0)))
      {
         ratio = ((cy * -bx) - (cx * -by)) / ratio;
         ix    = x1 + (ratio * ax);
         iy    = y1 + (ratio * ay);
      }
      else
      {
         if (is_equal((ax * -cy),(-cx * ay)))
         {
            ix = x3;
            iy = y3;
         }
         else
         {
            ix = x4;
            iy = y4;
         }
      }

      return true;
   }

   template <typename T>
   inline bool intersect(const point2d<T>& point1,const point2d<T>& point2,
                         const point2d<T>& point3,const point2d<T>& point4)
   {
      return intersect(point1.x,point1.y,
                       point2.x,point2.y,
                       point3.x,point3.y,
                       point4.x,point4.y);
   }

   template <typename T>
   inline bool intersect(const point2d<T>& point1, const point2d<T>& point2,
                         const point2d<T>& point3, const point2d<T>& point4, point2d<T>& int_point)
   {
      return intersect(point1   .x, point1   .y,
                       point2   .x, point2   .y,
                       point3   .x, point3   .y,
                       point4   .x, point4   .y,
                       int_point.x, int_point.y);
   }

   template <typename T>
   inline bool intersect(const segment<T,2>& segment1, const segment<T,2>& segment2)
   {
      return intersect(segment1[0],segment1[1],segment2[0],segment2[1]);
   }

   template <typename T>
   inline bool intersect_vertical_horizontal(const segment<T,2>& segment1, const segment<T,2>& segment2)
   {
      return (((segment1[0].y <= segment2[0].y) && (segment2[0].y <= segment1[1].y)) ||
              ((segment1[1].y <= segment2[0].y) && (segment2[0].y <= segment1[0].y))) &&
              (
                ((segment2[0].x <= segment1[0].x) && (segment1[0].x <= segment2[1].x)) ||
                ((segment2[1].x <= segment1[0].x) && (segment1[0].x <= segment2[0].x))
              );
   }

   template <typename T>
   inline bool intersect_vertical_vertical(const segment<T,2>& segment1, const segment<T,2>& segment2)
   {
      return (segment1[0].x == segment2[0].x) &&
             (
               ((segment1[0].y <= segment2[0].y) && (segment2[0].y <= segment1[1].y)) ||
               ((segment1[0].y <= segment2[1].y) && (segment2[1].y <= segment1[1].y))
             );
   }

   template <typename T>
   inline bool intersect_horizontal_horizontal(const segment<T,2>& segment1, const segment<T,2>& segment2)
   {
      return (segment1[0].y == segment2[0].y) &&
             (
               ((segment1[0].x <= segment2[0].x) && (segment2[0].x <= segment1[1].x)) ||
               ((segment1[0].x <= segment2[1].x) && (segment2[1].x <= segment1[1].x))
             );
   }

   template <typename T>
   inline bool intersect(const segment<T,2>& segment1, const segment<T,2>& segment2, T& ix,T& iy)
   {
      return intersect(segment1[0].x,segment1[0].y,segment1[1].x,segment1[1].y,
                       segment2[0].x,segment2[0].y,segment2[1].x,segment2[1].y,
                       ix,iy);
   }

   template <typename T>
   inline bool intersect(const segment<T,2>& segment1, const segment<T,2>& segment2,point2d<T>& i_point)
   {
      return intersect(segment1[0].x,segment1[0].y,segment1[1].x,segment1[1].y,
                       segment2[0].x,segment2[0].y,segment2[1].x,segment2[1].y,
                       i_point.x,i_point.y);
   }

   template <typename T>
   inline bool intersect(const T& x1, const T& y1, const T& z1,
                         const T& x2, const T& y2, const T& z2,
                         const T& x3, const T& y3, const T& z3,
                         const T& x4, const T& y4, const T& z4,
                         const T& fuzzy)
   {
      return (less_than_or_equal(lay_distance_segment_to_segment(x1,y1,z1,x2,y2,z2,x3,y3,z3,x4,y4,z4),fuzzy));
   }

   template <typename T>
   inline bool intersect(const point3d<T>& point1,
                         const point3d<T>& point2,
                         const point3d<T>& point3,
                         const point3d<T>& point4,
                         const T& fuzzy)
   {
      return intersect
             (
               point1.x, point1.y, point1.z,
               point2.x, point2.y, point2.z,
               point3.x, point3.y, point3.z,
               point4.x, point4.y, point4.z,
               fuzzy
             );
   }

   template <typename T>
   inline bool intersect(const segment<T,3>& segment1, const segment<T,3>& segment2, const T& fuzzy)
   {
      return intersect(segment1[0],segment1[1],segment2[0],segment2[1],fuzzy);
   }

   template <typename T>
   inline bool intersect(const segment<T,2>& segment, const rectangle<T>& rectangle)
   {
      if (intersect(rectangle,make_rectangle(segment[0],segment[1])))
      {

         int orin = orientation(segment[0],segment[1],rectangle[0].x,rectangle[0].y);

         if (
              (orientation(segment[0],segment[1],rectangle[0].x,rectangle[1].y) == orin) &&
              (orientation(segment[0],segment[1],rectangle[1].x,rectangle[1].y) == orin) &&
              (orientation(segment[0],segment[1],rectangle[1].x,rectangle[0].y) == orin)
            )
            return false;
         else
            return true;
      }
      else
         return false;
   }

   template <typename T>
   inline bool intersect(const segment<T,2>& segment, const triangle<T,2>& triangle)
   {
      return intersect(segment,edge(triangle,0))    ||
             intersect(segment,edge(triangle,1))    ||
             intersect(segment,edge(triangle,2))    ||
             point_in_triangle(segment[0],triangle) ||
             point_in_triangle(segment[1],triangle) ;
   }

   template <typename T>
   inline bool intersect(const segment<T,2>& segment, const quadix<T,2>& quadix)
   {
      return intersect(segment, make_triangle(quadix[0], quadix[1], quadix[2])) ||
             intersect(segment, make_triangle(quadix[0], quadix[2], quadix[3])) ;
   }

   template <typename T>
   inline bool intersect(const segment<T,2>& segment, const line<T,2>& line)
   {
      return (orientation(line,segment[0]) * orientation(line,segment[1]) <= 0);
   }

   template <typename T>
   inline bool intersect(const segment<T,3>& segment, const line<T,3>& line, const T& fuzzy)
   {
      return intersect
             (
               segment,
               make_segment
               (
                 closest_point_on_line_from_point(line, segment[0]),
                 closest_point_on_line_from_point(line, segment[1])
               ),
               fuzzy
             );
   }

   template <typename T>
   inline bool intersect(const segment<T,3>& segment, const box<T,3>& box)
   {
      const T cx = (box[0].x + box[1].x) * T(0.5);
      const T cy = (box[0].y + box[1].y) * T(0.5);
      const T cz = (box[0].z + box[1].z) * T(0.5);

      const T ex = box[1].x - cx;
      const T ey = box[1].y - cy;
      const T ez = box[1].z - cz;

      const T mx = (segment[0].x + segment[1].x) * T(0.5) - cx;
      const T my = (segment[0].y + segment[1].y) * T(0.5) - cy;
      const T mz = (segment[0].z + segment[1].z) * T(0.5) - cz;

      const T dx = segment[1].x - mx;
      const T dy = segment[1].y - my;
      const T dz = segment[1].z - mz;

      T adx = abs(dx);
      if (abs(mx) > ex + adx) return false;

      T ady = abs(dy);
      if (abs(my) > ey + ady) return false;

      T adz = abs(dz);
      if (abs(mz) > ez + adz) return false;

      adx += T(Epsilon);
      ady += T(Epsilon);
      adz += T(Epsilon);

      if (abs(my * dz - mz * dy) > (ey * adz + ez * ady)) return false;
      if (abs(mz * dx - mx * dz) > (ex * adz + ez * adx)) return false;
      if (abs(mx * dy - my * dx) > (ex * ady + ey * adx)) return false;

      return true;
   }

   template <typename T>
   inline bool intersect(const segment<T,2>& segment, const circle<T>& circle)
   {
      T px;
      T py;

      closest_point_on_segment_from_point
      (
        segment[0].x, segment[0].y, segment[1].x, segment[1].y,
        circle.x, circle.y,
        px, py
      );

      return (lay_distance(px,py,circle.x,circle.y) <= (circle.radius * circle.radius));
   }

   template <typename T>
   inline bool intersect(const segment<T,2>& segment, const quadratic_bezier<T,2>& bezier, const std::size_t& steps)
   {
      return simplex_to_bezier_intersect
             <T,2,wykobi::segment<T,2>,quadratic_bezier<T,2> >
             (segment, bezier, steps);
   }

   template <typename T>
   inline bool intersect(const segment<T,2>& segment, const cubic_bezier<T,2>& bezier, const std::size_t& steps)
   {
      return simplex_to_bezier_intersect
             <T,2,wykobi::segment<T,2>,cubic_bezier<T,2> >
             (segment, bezier, steps);
   }

   template <typename T>
   inline bool intersect(const segment<T,3>& segment, const sphere<T>& sphere)
   {
      const T a = lay_distance(segment);
      const T b = 2 * ((segment[1].x - segment[0].x) * (segment[0].x - sphere.x) + (segment[1].y - segment[0].y) * (segment[0].y - sphere.y) + (segment[1].z - segment[0].z) * (segment[0].z  - sphere.z));
      const T c = sqr(sphere.x) + sqr(sphere.y) + sqr(sphere.z) + sqr(segment[1].x) + sqr(segment[1].y) + sqr(segment[1].z) - 2 * (sphere.x * segment[1].x + sphere.y * segment[1].y + sphere.z * segment[1].z) - sqr(sphere.radius);

      //((b * b - 4 * a * c) >= 0)
      return greater_than_or_equal((b * b - 4 * a * c),T(0.0));
   }

   template <typename T>
   inline bool intersect(const segment<T,3>& segment, const plane<T,3>& plane)
   {
      T signed_dist1 = dot_product(plane.normal,make_vector(segment[0])) - plane.constant;
      T signed_dist2 = dot_product(plane.normal,make_vector(segment[1])) - plane.constant;

      signed_dist1 = (is_equal(abs(signed_dist1),T(0.0)))? T(0.0) : signed_dist1;
      signed_dist2 = (is_equal(abs(signed_dist2),T(0.0)))? T(0.0) : signed_dist2;

      return less_than_or_equal(signed_dist1 * signed_dist2,T(0.0));
   }

   template <typename T>
   inline bool intersect(const segment<T,3>& segment, const quadratic_bezier<T,3>& bezier, const std::size_t& steps)
   {
      return simplex_to_bezier_intersect<T,3,wykobi::segment<T,3>,quadratic_bezier<T,3> >
             (segment, bezier, steps);
   }

   template <typename T>
   inline bool intersect(const segment<T,3>& segment, const cubic_bezier<T,3>& bezier, const std::size_t& steps)
   {
      return simplex_to_bezier_intersect
             <T,3,wykobi::segment<T,3>,cubic_bezier<T,3> >
             (segment, bezier, steps);
   }

   template <typename T>
   inline bool intersect(const line<T,2>& line, const triangle<T,2>& triangle)
   {
      const int or1 = orientation (line[0], line[1], triangle[0]);

      if (0 == or1) return true;
      int or2 = orientation (line[0], line[1], triangle[1]);

      if (or2 != or1) return true;
      or2 = orientation (line[0], line[1], triangle[2]);

      return (or2 != or1);
   }

   template <typename T>
   inline bool intersect(const line<T,2>& line, const quadix<T,2>& quadix)
   {
      const int or1 = orientation (line[0], line[1], quadix[0]);

      if (0 == or1) return true;
      int or2 = orientation (line[0], line[1], quadix[1]);

      if (or2 != or1) return true;
      or2 = orientation (line[0], line[1], quadix[2]);

      if (or2 != or1) return true;
      or2 = orientation (line[0], line[1], quadix[3]);

      return (or2 != or1);
   }

   template <typename T>
   inline bool intersect(const line<T,2>& line1, const line<T,2>& line2)
   {
      return line_to_line_intersect(line1,line2);
   }

   template <typename T>
   inline bool intersect(const line<T,2>& line, const circle<T>& circle)
   {
      /*
         It is assumed that an intersection of a circle by a line
         is either a full intersection (2 points), partial intersection
         (1 point), or tangential.
         anything else will result in a false output.
      */
      const T x1 = line[0].x - circle.x;
      const T y1 = line[0].y - circle.y;
      const T x2 = line[1].x - circle.x;
      const T y2 = line[1].y - circle.y;

      return greater_than_or_equal(((circle.radius * circle.radius) * lay_distance(x1,y1,x2,y2) - sqr(x1 * y2 - x2 * y1)),T(0.0));
   }

   template <typename T>
   inline bool intersect(const line<T,2>& line, const quadratic_bezier<T,2>& bezier, const std::size_t& steps)
   {
      return simplex_to_bezier_intersect
             <T,2,wykobi::line<T,2>,quadratic_bezier<T,2> >
             (line, bezier, steps);
   }

   template <typename T>
   inline bool intersect(const line<T,2>& line, const cubic_bezier<T,2>& bezier, const std::size_t& steps)
   {
      return simplex_to_bezier_intersect
             <T,2,wykobi::line<T,2>,cubic_bezier<T,2> >
             (line, bezier, steps);
   }

   template <typename T>
   inline bool intersect(const line<T,3>& line, const triangle<T,3>& triangle)
   {
      vector3d<T> diff     = line[0] - triangle[0];
      vector3d<T> line_dir = line[1] - line[0];
      vector3d<T> edge1    = triangle[1] - triangle[0];
      vector3d<T> edge2    = triangle[2] - triangle[0];
      vector3d<T> normal   = edge1 * edge2;

      T denom =  dot_product(line_dir,normal);
      T sign  = 0.0;

      if (denom > T(0.0))
      {
         sign = T(1.0);
      }
      else if (denom < T(0.0))
      {
         sign  = T(-1.0);
         denom = -denom;
      }
      else
         return false;

      T val1 = sign * dot_product(line_dir,(diff * edge2));

      if (greater_than_or_equal(val1,T(0.0)))
      {
         T val2 = sign * dot_product(line_dir,edge1 * diff);

         if (greater_than_or_equal(val2,T(0.0)))
         {
            if (val1 + val2 <= denom)
            {
               return true;
            }
         }
      }

      return false;
   }

   template <typename T> inline bool intersect(const line<T,3>& line, const plane<T,3>& plane)
   {
      return not_equal(dot_product((line[1] - line[0]),plane.normal),T(0.0));
   }

   template <typename T>
   inline bool intersect(const line<T,3>& line, const sphere<T>& sphere)
   {
      const T a =  sqr(line[1].x - line[0].x) + sqr(line[1].y - line[0].y) + sqr(line[1].z - line[0].z);

      const T b =  T(2.0) * ((line[1].x - line[0].x) * (line[0].x - sphere.x) +
                             (line[1].y - line[0].y) * (line[0].y - sphere.y) +
                             (line[1].z - line[0].z) * (line[0].z - sphere.z));

      const T c =  sqr(sphere.x)  + sqr(sphere.y)  + sqr(sphere.z) +
                   sqr(line[0].x) + sqr(line[0].y) + sqr(line[0].z) -
                   T(2.0) * (sphere.x * line[0].x + sphere.y * line[0].y + sphere.z * line[0].z) - sqr(sphere.radius);

      return greater_than_or_equal(b * b - 4 * a * c, T(0.0));
   }

   template <typename T>
   inline bool intersect(const line<T,3>& line, const quadratic_bezier<T,3>& bezier, const std::size_t& steps)
   {
      return simplex_to_bezier_intersect
             <T,3,wykobi::line<T,3>,quadratic_bezier<T,3> >
             (line, bezier, steps);
   }

   template <typename T>
   inline bool intersect(const line<T,3>& line, const cubic_bezier<T,3>& bezier, const std::size_t& steps)
   {
      return simplex_to_bezier_intersect
             <T,3,wykobi::line<T,3>,cubic_bezier<T,3> >
             (line, bezier, steps);
   }

   template <typename T>
   inline bool intersect(const triangle<T,2>& triangle, const circle<T>& circle)
   {
      return point_in_circle
             (
               closest_point_on_triangle_from_point(triangle, circle.x, circle.y),
               circle
             );
   }

   template <typename T>
   inline bool intersect(const triangle<T,2>& triangle, const rectangle<T>& rectangle)
   {
      return intersect(make_segment(triangle[0],triangle[1]),rectangle) ||
             intersect(make_segment(triangle[1],triangle[2]),rectangle) ||
             intersect(make_segment(triangle[2],triangle[0]),rectangle) ;
   }

   template <typename T>
   inline bool intersect(const triangle<T,2>& triangle, const quadratic_bezier<T,2>& bezier, const std::size_t& steps)
   {
      return simplex_to_bezier_intersect
             <T,2,wykobi::triangle<T,2>,quadratic_bezier<T,2> >
             (triangle, bezier, steps);
   }

   template <typename T>
   inline bool intersect(const triangle<T,2>& triangle, const cubic_bezier<T,2>& bezier, const std::size_t& steps)
   {
      return simplex_to_bezier_intersect<T,2,wykobi::triangle<T,2>,cubic_bezier<T,2> >(triangle,bezier,steps);
   }

   template <typename T>
   inline bool intersect(const triangle<T,2>& triangle1, const triangle<T,2>& triangle2)
   {
      for (std::size_t i = 0; i < triangle<T,2>::PointCount; ++i)
      {
         if (
               is_equal(minimum_distance_from_point_to_triangle(triangle1[i],triangle2),T(0.0)) ||
               is_equal(minimum_distance_from_point_to_triangle(triangle2[i],triangle1),T(0.0))
            )
         {
            return true;
         }
      }
      return false;
   }

   template <typename T>
   inline bool intersect(const rectangle<T>& rectangle1, const rectangle<T>& rectangle2)
   {
      return rectangle_to_rectangle_intersect
             (
               rectangle1[0].x, rectangle1[0].y,
               rectangle1[1].x, rectangle1[1].y,
               rectangle2[0].x, rectangle2[0].y,
               rectangle2[1].x, rectangle2[1].y
             );
   }

   template <typename T>
   inline bool intersect(const rectangle<T>& rectangle, const quadratic_bezier<T,2>& bezier, const std::size_t& steps)
   {
      return simplex_to_bezier_intersect
             <T,2,wykobi::rectangle<T>,quadratic_bezier<T,2> >
             (rectangle, bezier, steps);
   }

   template <typename T>
   inline bool intersect(const rectangle<T>& rectangle, const cubic_bezier<T,2>& bezier, const std::size_t& steps)
   {
      return simplex_to_bezier_intersect
             <T,2,wykobi::rectangle<T>,cubic_bezier<T,2> >
             (rectangle, bezier, steps);
   }

   template <typename T>
   inline bool intersect(const rectangle<T>& rectangle, const circle<T>& circle)
   {
      return  point_in_circle
              (
                closest_point_on_rectangle_from_point(rectangle, circle.x, circle.y),
                circle
              );
   }

   template <typename T>
   inline bool intersect(const quadix<T,2>& quadix, const quadratic_bezier<T,2>& bezier, const std::size_t& steps)
   {
      return simplex_to_bezier_intersect
             <T,2,wykobi::quadix<T,2>,quadratic_bezier<T,2> >
             (quadix, bezier, steps);
   }

   template <typename T>
   inline bool intersect(const quadix<T,2>& quadix, const cubic_bezier<T,2>& bezier, const std::size_t& steps)
   {
      return simplex_to_bezier_intersect
             <T,2,wykobi::quadix<T,2>,cubic_bezier<T,2> >
             (quadix, bezier, steps);
   }

   template <typename T>
   inline bool intersect(const circle<T>& circle1, const circle<T>& circle2)
   {
      return (lay_distance(circle1.x,circle1.y,circle2.x,circle2.y) <= ((circle1.radius + circle2.radius) * (circle1.radius + circle2.radius)));
   }

   template <typename T>
   inline bool intersect(const circle<T>& circle, const quadratic_bezier<T,2>& bezier, const std::size_t& steps)
   {
      return simplex_to_bezier_intersect<T,2,wykobi::circle<T>,quadratic_bezier<T,2> >(circle,bezier,steps);
   }

   template <typename T>
   inline bool intersect(const circle<T>& circle, const cubic_bezier<T,2>& bezier, const std::size_t& steps)
   {
      return simplex_to_bezier_intersect<T,2,wykobi::circle<T>,cubic_bezier<T,2> >(circle,bezier,steps);
   }

   template <typename T>
   inline bool intersect(const box<T,3>& box, const sphere<T>& sphere)
   {
      return point_in_sphere(closest_point_on_box_from_point(box,sphere.x,sphere.y,sphere.z),sphere);
   }

   template <typename T>
   inline bool intersect(const sphere<T>& sphere1, const sphere<T>& sphere2)
   {
      return (lay_distance(sphere1.x,sphere1.y,sphere1.z,sphere2.x,sphere2.y,sphere2.z) <= ((sphere1.radius + sphere2.radius) * (sphere1.radius + sphere2.radius)));
   }

   template <typename T>
   inline bool intersect(const sphere<T>& sphere, const quadratic_bezier<T,3>& bezier, const std::size_t& steps)
   {
      return simplex_to_bezier_intersect<T,3,wykobi::sphere<T>,quadratic_bezier<T,3> >(sphere,bezier,steps);
   }

   template <typename T>
   inline bool intersect(const sphere<T>& sphere, const cubic_bezier<T,3>& bezier, const std::size_t& steps)
   {
      return simplex_to_bezier_intersect<T,3,wykobi::sphere<T>,cubic_bezier<T,3> >(sphere,bezier,steps);
   }

   template <typename T>
   inline bool intersect(const ray<T,2>& ray1, const ray<T,2>& ray2)
   {
      const T denom = dot_product(perpendicular(ray1.direction),ray2.direction);

      if (denom != T(0.0))
      {
         vector2d<T> diff = ray1.origin - ray2.origin;

         const T s = dot_product(perpendicular(ray1.direction),diff) / denom;
         const T t = dot_product(perpendicular(ray2.direction),diff) / denom;

         return (greater_than_or_equal(t,T(0.0)) &&  greater_than_or_equal(s,T(0.0)));
      }
      else // parallel
        return (point_on_ray(ray2.origin,ray1) || point_on_ray(ray1.origin,ray2));
   }

   template <typename T>
   inline bool intersect(const ray<T,3>& ray1, const ray<T,3>& ray2)
   {
      if (!coplanar(ray1,ray2)) return false;

      const T denom = dot_product(perpendicular(ray1.direction),ray2.direction);

      if (denom != T(0.0))
      {
         vector3d<T> diff = ray1.origin - ray2.origin;

         const T s = dot_product(perpendicular(ray1.direction),diff) / denom;
         const T t = dot_product(perpendicular(ray2.direction),diff) / denom;

         return (greater_than_or_equal(t,T(0.0)) &&  greater_than_or_equal(s,T(0.0)));
      }
      else // parallel
        return (point_on_ray(ray2.origin,ray1) || point_on_ray(ray1.origin,ray2));
   }

   template <typename T>
   inline bool intersect(const ray<T,2>& ray, const segment<T,2>& segment)
   {
      vector2d<T> delta = perpendicular(segment[1] - segment[0]);

      const T denom = dot_product(delta,ray.direction);

      if (denom != T(0.0))
      {
         vector2d<T> diff = ray.origin - segment[0];

         const T s = dot_product(delta, diff) / denom;
         const T t = dot_product(perpendicular(ray.direction), diff) / denom;

         return greater_than_or_equal(t, T(0.0)) &&
                less_than_or_equal   (t, T(1.0)) &&
                greater_than_or_equal(s, T(0.0));
      }
      else
        return point_on_ray(segment[0],ray);
   }

   template <typename T>
   inline bool intersect(const ray<T,3>& ray, const segment<T,3>& segment)
   {
      if (robust_coplanar(segment[0],segment[1],ray.origin,generate_point_on_ray(ray,T(1.0))))
         return point_on_segment(closest_point_on_ray_from_point(ray,segment[0]),segment);
      else
         return false;
   }

   template <typename T>
   inline bool intersect(const ray<T,2>& ray, const rectangle<T>& rectangle)
   {
      T tmin = 0.0;
      T tmax = 1.0;
      T t;

      if (not_equal(ray.direction.x,T(0.0)))
      {
         const T recip_dirx = T(1.0) / ray.direction.x;

         if (ray.direction.x > T(0.0))
         {
            if ((t = (rectangle[1].x - ray.origin.x) * recip_dirx) < tmin) { return false; } tmax = min(t,tmax);
            if ((t = (rectangle[0].x - ray.origin.x) * recip_dirx) > tmax) { return false; } tmin = max(t,tmin);
         }
         else
         {
            if ((t = (rectangle[0].x - ray.origin.x) * recip_dirx) < tmin) { return false; } tmax = min(t,tmax);
            if ((t = (rectangle[1].x - ray.origin.x) * recip_dirx) > tmax) { return false; } tmin = max(t,tmin);
         }
      }
      else if ((ray.origin.x < rectangle[0].x) || (ray.origin.x > rectangle[1].x))
         return false;

      if (not_equal(ray.direction.y,T(0.0)))
      {
         const T recip_diry = T(1.0) / ray.direction.y;

         if (ray.direction.y > T(0.0))
         {
            if ((t = (rectangle[1].y - ray.origin.y) * recip_diry) < tmin) { return false; } tmax = min(t,tmax);
            if ((t = (rectangle[0].y - ray.origin.y) * recip_diry) > tmax) { return false; } tmin = max(t,tmin);
         }
         else
         {
            if ((t = (rectangle[0].y - ray.origin.y) * recip_diry) < tmin) { return false; } tmax = min(t,tmax);
            if ((t = (rectangle[1].y - ray.origin.y) * recip_diry) > tmax) { return false; } tmin = max(t,tmin);
         }
      }
      else if ((ray.origin.y < rectangle[0].y) || (ray.origin.y > rectangle[1].y))
         return false;

      return (tmin < tmax);
   }

   template <typename T>
   inline bool intersect(const ray<T,3>& ray, const box<T,3>& box)
   {
      T tmin = 0.0;
      T tmax = 1.0;
      T t;

      if (not_equal(ray.direction.x,T(0.0)))
      {
         const T recip_dirx = T(1.0) / ray.direction.x;

         if (ray.direction.x > T(0.0))
         {
            if ((t = (box[1].x - ray.origin.x) * recip_dirx) < tmin) { return false; } tmax = min(t,tmax);
            if ((t = (box[0].x - ray.origin.x) * recip_dirx) > tmax) { return false; } tmin = max(t,tmin);
         }
         else
         {
            if ((t = (box[0].x - ray.origin.x) * recip_dirx) < tmin) { return false; } tmax = min(t,tmax);
            if ((t = (box[1].x - ray.origin.x) * recip_dirx) > tmax) { return false; } tmin = max(t,tmin);
         }
      }
      else if ((ray.origin.x < box[0].x) || (ray.origin.x > box[1].x))
         return false;

      if (not_equal(ray.direction.y,T(0.0)))
      {
         const T recip_diry = T(1.0) / ray.direction.y;

         if (ray.direction.y > T(0.0))
         {
            if ((t = (box[1].y - ray.origin.y) * recip_diry) < tmin) { return false; } tmax = min(t,tmax);
            if ((t = (box[0].y - ray.origin.y) * recip_diry) > tmax) { return false; } tmin = max(t,tmin);
         }
         else
         {
            if ((t = (box[0].y - ray.origin.y) * recip_diry) < tmin) { return false; } tmax = min(t,tmax);
            if ((t = (box[1].y - ray.origin.y) * recip_diry) > tmax) { return false; } tmin = max(t,tmin);
         }
      }
      else if ((ray.origin.y < box[0].y) || (ray.origin.y > box[1].y))
         return false;

      if (not_equal(ray.direction.z,T(0.0)))
      {
         const T recip_dirz = T(1.0) / ray.direction.z;

         if (ray.direction.z > T(0.0))
         {
            if ((t = (box[1].z - ray.origin.z) * recip_dirz) < tmin) { return false; } tmax = min(t,tmax);
            if ((t = (box[0].z - ray.origin.z) * recip_dirz) > tmax) { return false; } tmin = max(t,tmin);
         }
         else
         {
            if ((t = (box[0].z - ray.origin.z) * recip_dirz) < tmin) { return false; } tmax = min(t,tmax);
            if ((t = (box[1].z - ray.origin.z) * recip_dirz) > tmax) { return false; } tmin = max(t,tmin);
         }
      }
      else if ((ray.origin.z < box[0].z) || (ray.origin.z > box[1].z))
         return false;

      return (tmin < tmax);
   }

   template <typename T>
   inline bool intersect(const ray<T,2>& ray, const triangle<T,2>& triangle)
   {
      if (point_in_triangle(ray.origin, triangle))
         return true;
      else
         return intersect(ray,edge(triangle,0)) ||
                intersect(ray,edge(triangle,1)) ||
                intersect(ray,edge(triangle,2)) ;
   }

   template <typename T>
   inline bool intersect(const ray<T,3>& ray, const triangle<T,3>& triangle)
   {
      const T edge1_x = triangle[1].x - triangle[0].x;
      const T edge1_y = triangle[1].y - triangle[0].y;
      const T edge1_z = triangle[1].z - triangle[0].z;
      const T edge2_x = triangle[2].x - triangle[0].x;
      const T edge2_y = triangle[2].y - triangle[0].y;
      const T edge2_z = triangle[2].z - triangle[0].z;

      const T pvec_x = (ray.direction.y * edge2_z) - (ray.direction.z * edge2_y);
      const T pvec_y = (ray.direction.z * edge2_x) - (ray.direction.x * edge2_z);
      const T pvec_z = (ray.direction.x * edge2_y) - (ray.direction.y * edge2_x);

      const T det = edge1_x * pvec_x + edge1_y * pvec_y + edge1_z * pvec_z;

      if (is_equal(det,T(0.0))) return false;

      const T inv_det = T(1.0) / det;

      const T tvec_x = ray.origin.x - triangle[0].x;
      const T tvec_y = ray.origin.y - triangle[0].y;
      const T tvec_z = ray.origin.z - triangle[0].z;

      const T u = (tvec_x * pvec_x + tvec_y * pvec_y + tvec_z * pvec_z) * inv_det;

      if (u < 0.0 || u > 1.0) return false;

      const T qvec_x = (tvec_y * edge1_z) - (tvec_z * edge1_y);
      const T qvec_y = (tvec_z * edge1_x) - (tvec_x * edge1_z);
      const T qvec_z = (tvec_x * edge1_y) - (tvec_y * edge1_x);

      const T v = (ray.direction.x * qvec_x + ray.direction.y * qvec_y + ray.direction.z * qvec_z) * inv_det;

      if ((v < 0.0) || ((u + v) > 1.0)) return false;

      const T t = (edge2_x * qvec_x + edge2_y * qvec_y + edge2_z * qvec_z) * inv_det;

      return (t > T(0.0));
   }

   template <typename T>
   inline bool intersect(const ray<T,2>& ray, const quadix<T,2>& quadix)
   {
      if (point_in_quadix(ray.origin, quadix))
         return true;
      else
         return intersect(ray, edge(quadix, 0)) ||
                intersect(ray, edge(quadix, 1)) ||
                intersect(ray, edge(quadix, 2)) ||
                intersect(ray, edge(quadix, 3)) ;
   }

   template <typename T>
   inline bool intersect(const ray<T,2>& ray, const circle<T>& circle)
   {
      const T dx = ray.origin.x - circle.x;
      const T dy = ray.origin.y - circle.y;
      const T c  = (dx * dx) + (dy * dy) - (circle.radius * circle.radius);

      if (less_than_or_equal(c,T(0.0)))
      {
         return true;
      }

      const T b = dx * ray.direction.x + dy * ray.direction.y;

      if (greater_than_or_equal(b,T(0.0)))
      {
         return false;
      }

      return greater_than_or_equal(b * b,c);
   }

   template <typename T>
   inline bool intersect(const ray<T,3>& ray, const sphere<T>& sphere)
   {
      const T dx = ray.origin.x - sphere.x;
      const T dy = ray.origin.y - sphere.y;
      const T dz = ray.origin.z - sphere.z;
      const T c  = (dx * dx) + (dy * dy) + (dz * dz) - (sphere.radius * sphere.radius);

      if (less_than_or_equal(c,T(0.0)))
      {
         return true;
      }

      const T b = dx * ray.direction.x + dy * ray.direction.y + dz * ray.direction.z;

      if (greater_than_or_equal(b,T(0.0)))
      {
         return false;
      }

      return greater_than_or_equal(b * b,c);
   }

   template <typename T>
   inline bool intersect(const ray<T,3>& ray, const plane<T,3>& plane)
   {
      const T denom = dot_product(ray.direction,plane.normal);

      if (not_equal(denom,T(0.0)))
      {
         return ((-distance(ray.origin,plane) / denom) >= T(0.0));
      }
      else
         return false;
   }

   template <typename T>
   inline bool intersect(const ray<T,2>& ray, const polygon<T,2>& polygon)
   {
      if (polygon.size() < 3) return false;

      std::size_t j = polygon.size() - 1;

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         if (intersect(ray,make_segment(polygon[i],polygon[j])))
         {
            return true;
         }

         j = i;
      }

      return false;
   }

   template <typename T>
   inline bool intersect(const plane<T,3>& plane1, const plane<T,3>& plane2)
   {
      return is_equal(dot_product(plane1.normal,plane2.normal),T(1.0));
   }

   template <typename T>
   inline bool intersect(const plane<T,3>& plane, const sphere<T>& sphere)
   {
      return less_than_or_equal(distance(make_point(sphere.x,sphere.y,sphere.z),plane),sphere.radius);
   }

   template <typename T>
   inline bool intersect(const plane<T,3>& plane, const line<T,3>& line)
   {
      return not_equal(dot_product(plane.normal,(line[1] - line[0])),T(0.0));
   }

   template <typename T>
   inline bool simple_intersect(const T& x1, const T& y1,
                                const T& x2, const T& y2,
                                const T& x3, const T& y3,
                                const T& x4, const T& y4)
   {
      return (
              ((orientation(x1,y1,x2,y2,x3,y3) * orientation(x1,y1,x2,y2,x4,y4)) <= 0) &&
              ((orientation(x3,y3,x4,y4,x1,y1) * orientation(x3,y3,x4,y4,x2,y2)) <= 0)
             );
   }

   template <typename T>
   inline bool simple_intersect(const point2d<T>& point1, const point2d<T>& point2,
                                const point2d<T>& point3, const point2d<T>& point4)
   {
      return simple_intersect
             (
               point1.x, point1.y,
               point2.x, point2.y,
               point3.x, point3.y,
               point4.x, point4.y
             );
   }

   template <typename T>
   inline bool simple_intersect(const segment<T,2>& segment1, const segment<T,2>& segment2)
   {
      return simple_intersect(segment1[0],segment1[1],segment2[0],segment2[1]);
   }

   template <typename T>
   inline void intersection_point(const T& x1, const T& y1,
                                  const T& x2, const T& y2,
                                  const T& x3, const T& y3,
                                  const T& x4, const T& y4,
                                        T& ix,       T& iy)
   {
      const T dx1 = x2 - x1;
      const T dx2 = x4 - x3;
      const T dx3 = x1 - x3;

      const T dy1 = y2 - y1;
      const T dy2 = y1 - y3;
      const T dy3 = y4 - y3;

      T ratio = dx1 * dy3 - dy1 * dx2;

      if (not_equal(ratio,T(0.0)))
      {
         ratio = (dy2 * dx2 - dx3 * dy3) / ratio;
         ix    = x1 + ratio * dx1;
         iy    = y1 + ratio * dy1;
      }
      else
      {
         if (is_equal((dx1 * -dy2),(-dx3 * dy1)))
         {
            ix = x3;
            iy = y3;
         }
         else
         {
            ix = x4;
            iy = y4;
         }
      }
   }

   template <typename T>
   inline void intersection_point(const point2d<T>& point1,
                                  const point2d<T>& point2,
                                  const point2d<T>& point3,
                                  const point2d<T>& point4,
                                        T& ix,       T& iy)
   {
      intersection_point(point1.x, point1.y,
                         point2.x, point2.y,
                         point3.x, point3.y,
                         point4.x, point4.y,
                               ix,       iy);
   }

   template <typename T>
   inline point2d<T> intersection_point(const point2d<T>& point1,
                                        const point2d<T>& point2,
                                        const point2d<T>& point3,
                                        const point2d<T>& point4)
   {
      point2d<T> point_;
      intersection_point(point1.x, point1.y,
                         point2.x, point2.y,
                         point3.x, point3.y,
                         point4.x, point4.y,
                         point_.x, point_.y);
      return point_;
   }

   template <typename T>
   inline point2d<T> intersection_point(const segment<T,2>& segment1,
                                        const segment<T,2>& segment2)
   {
      return intersection_point(segment1[0],segment1[1],segment2[0],segment2[1]);
   }

   template <typename T>
   inline void  intersection_point(const T& x1, const T& y1, const T& z1,
                                   const T& x2, const T& y2, const T& z2,
                                   const T& x3, const T& y3, const T& z3,
                                   const T& x4, const T& y4, const T& z4,
                                         T& ix,       T& iy,       T& iz, const T& fuzzy)
   {
      const T ux = x2 - x1;
      const T uy = y2 - y1;
      const T uz = z2 - z1;

      const T vx = x4 - x3;
      const T vy = y4 - y3;
      const T vz = z4 - z3;

      const T wx = x1 - x3;
      const T wy = y1 - y3;
      const T wz = z1 - z3;

      const T a  = (ux * ux + uy * uy + uz * uz);
      const T b  = (ux * vx + uy * vy + uz * vz);
      const T c  = (vx * vx + vy * vy + vz * vz);
      const T d  = (ux * wx + uy * wy + uz * wz);
      const T e  = (vx * wx + vy * wy + vz * wz);
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

      const T dx = wx + (sc * ux) - (tc * vx);
      const T dy = wy + (sc * uy) - (tc * vy);
      const T dz = wz + (sc * uz) - (tc * vz);

      if ((dx * dx + dy * dy + dz * dz) <= sqr(fuzzy))
      {
         ix = ((x1 + (sc * ux)) + (x3 + (tc * vx))) * T(0.5);
         iy = ((y1 + (sc * uy)) + (y3 + (tc * vy))) * T(0.5);
         iz = ((z1 + (sc * uz)) + (z3 + (tc * vz))) * T(0.5);
      }
      else
      {
         ix = +infinity<T>();
         iy = +infinity<T>();
         iz = +infinity<T>();
      }
   }

   template <typename T>
   inline void intersection_point(const point3d<T>& point1,
                                  const point3d<T>& point2,
                                  const point3d<T>& point3,
                                  const point3d<T>& point4,
                                        T& ix, T& iy, T& iz, const T& fuzzy)
   {
      intersection_point(point1.x, point1.y, point1.z,
                         point2.x, point2.y, point2.z,
                         point3.x, point3.y, point3.z,
                         point4.x, point4.y, point4.z,
                               ix,       iy,       iz,fuzzy);
   }

   template <typename T>
   inline point3d<T> intersection_point(const point3d<T>& point1,
                                        const point3d<T>& point2,
                                        const point3d<T>& point3,
                                        const point3d<T>& point4, const T& fuzzy)
   {
      point3d<T> point_;

      intersection_point
      (
        point1.x, point1.y, point1.z,
        point2.x, point2.y, point2.z,
        point3.x, point3.y, point3.z,
        point4.x, point4.y, point4.z,
        point_.x, point_.y, point_.z,
        fuzzy
      );

      return point_;
   }

   template <typename T>
   inline point3d<T> intersection_point(const segment<T,3>& segment1,
                                        const segment<T,3>& segment2, const T& fuzzy)
   {
      return intersection_point
             (
               segment1[0], segment1[1],
               segment2[0], segment2[1],
               fuzzy
             );
   }

   template <typename T>
   inline point2d<T> intersection_point(const segment<T,2>& segment, const line<T,2>& line)
   {
      return intersection_point
             (
               segment,
               make_segment
               (
                 closest_point_on_line_from_point(line, segment[0]),
                 closest_point_on_line_from_point(line, segment[1])
               )
             );
   }

   template <typename T>
   inline point3d<T> intersection_point(const segment<T,3>& segment,
                                        const line<T,3>& line, const T& fuzzy)
   {
      return intersection_point
             (
               segment,
               make_segment
               (
                 closest_point_on_line_from_point(line, segment[0]),
                 closest_point_on_line_from_point(line, segment[1])
               ),
               fuzzy
             );
   }

   template <typename T>
   inline point3d<T> intersection_point(const segment<T,3>& segment,
                                        const plane<T,3>& plane)
   {
      vector3d<T> seg_vec = segment[1] - segment[0];

      const T denom = dot_product(seg_vec,plane.normal);

      point3d<T> ipoint = degenerate_point3d<T>();

      if (not_equal(denom,T(0.0)))
      {
         const T t = -distance(segment[0],plane) / denom;

         if ((t > T(0.0)) && (t < T(1.0)))
         {
            ipoint = segment[0] + t * (segment[1] - segment[0]);
         }
      }

      return ipoint;
   }

   template <typename T, unsigned int Dimension, typename Simplex, typename Bezier, typename OutputIterator>
   inline void simplex_to_bezier_intersection_point(const Simplex& simplex,
                                                    const Bezier& bezier,
                                                    OutputIterator out,
                                                    const std::size_t& steps)
   {
      if (0 == steps) return;

      typedef typename define_point_type<T,Dimension>::PointType PointType;

      T t  = T(0.0);
      T dt = T(1.0) / (T(1.0) * steps - T(1.0));

      bezier_coefficients<T,Dimension,Bezier::Type> coeffs;

      calculate_bezier_coefficients(bezier,coeffs);

      PointType previous_point = create_point_on_bezier(bezier[0],coeffs,t);

      t += dt;

      PointType ipoint;

      for (std::size_t i = 1; i < steps; t += dt, ++i)
      {
         PointType current_point = create_point_on_bezier(bezier[0],coeffs,t);

         if (intersect(make_segment(previous_point,current_point),simplex, ipoint))
         {
            (*out++) = ipoint;
         }

         previous_point = current_point;
      }
   }

   template <typename T, typename OutputIterator>
   inline void intersection_point(const segment<T,2>& segment,
                                  const quadratic_bezier<T,2>& bezier,
                                  OutputIterator out,
                                  const std::size_t& steps)
   {
      simplex_to_bezier_intersection_point
      <T, 2, wykobi::segment<T,2>, quadratic_bezier<T,2>, OutputIterator>
      (
        segment,
        bezier,
        out,
        steps
      );
   }

   template <typename T, typename OutputIterator>
   inline void intersection_point(const segment<T,2>& segment,
                                  const cubic_bezier<T,2>& bezier,
                                  OutputIterator out,
                                  const std::size_t& steps)
   {
      simplex_to_bezier_intersection_point
      <T,2,wykobi::segment<T,2>,cubic_bezier<T,2>,OutputIterator>
      (
        segment,
        bezier,
        out,
        steps
      );
   }

   template <typename T, typename OutputIterator>
   inline void intersection_point(const segment<T,3>& segment,
                                  const quadratic_bezier<T,3>& bezier,
                                  OutputIterator out,
                                  const std::size_t& steps)
   {
      simplex_to_bezier_intersection_point
      <T,3,wykobi::segment<T,3>,quadratic_bezier<T,3>,OutputIterator>
      (
        segment,
        bezier,
        out,
        steps
      );
   }

   template <typename T, typename OutputIterator>
   inline void intersection_point(const segment<T,3>& segment,
                                  const cubic_bezier<T,3>& bezier,
                                  OutputIterator out,
                                  const std::size_t& steps)
   {
      simplex_to_bezier_intersection_point
      <T,3,wykobi::segment<T,3>,cubic_bezier<T,3>,OutputIterator>
      (
        segment,
        bezier,
        out,
        steps
      );
   }

   template <typename T>
   inline point2d<T> intersection_point(const line<T,2>& line1,
                                        const line<T,2>& line2)
   {
      const T dx1 = line1[0].x - line1[1].x;
      const T dx2 = line2[0].x - line2[1].x;
      const T dx3 = line2[1].x - line1[1].x;
      const T dy1 = line1[0].y - line1[1].y;
      const T dy2 = line2[0].y - line2[1].y;
      const T dy3 = line2[1].y - line1[1].y;

      const T det = (dx2 * dy1) - (dy2 * dx1);

      point2d<T> point_ = make_point<T>(T(0.0),T(0.0));

      if (is_equal(det,T(0.0)))
      {
         if (is_equal((dx2 * dy3),(dy2 * dx3)))
         {
            point_.x = line2[1].x;
            point_.y = line2[1].y;
         }

         return point_;
      }

      const T ratio  = ((dx1 * dy3) - (dy1 * dx3)) / det;

      point_.x = (ratio * dx2) + line2[1].x;
      point_.y = (ratio * dy2) + line2[1].y;

      return point_;
   }

   template <typename T>
   inline point3d<T> intersection_point(const line<T,3>& line1,
                                        const line<T,3>& line2,
                                        const T& fuzzy)
   {
      point3d<T> point_;

      intersection_point_line_to_line
      (
        line1[0].x, line1[0].y, line1[0].z,
        line1[1].x, line1[1].y, line1[1].z,
        line2[0].x, line2[0].y, line2[0].z,
        line2[1].x, line2[1].y, line2[1].z,
          point_.x,   point_.y,   point_.z,
        fuzzy
      );

      return point_;
   }

   template <typename T>
   inline void intersection_point(const circle<T>&  circle1,
                                  const circle<T>&  circle2,
                                        point2d<T>& point1,
                                        point2d<T>& point2)
   {
      const T dist    = distance(circle1.x, circle1.y, circle2.x, circle2.y);
      const T dstsqr  = dist * dist;
      const T r1sqr   = circle1.radius * circle1.radius;
      const T r2sqr   = circle2.radius * circle2.radius;

      const T a       = (dstsqr - r2sqr + r1sqr) / (2 * dist);
      const T h       = sqrt(r1sqr - (a*a));

      const T ratio_a = a / dist;
      const T ratio_h = h / dist;

            T dx      = circle2.x - circle1.x;
            T dy      = circle2.y - circle1.y;

      const T phix    = circle1.x + (ratio_a * dx);
      const T phiy    = circle1.y + (ratio_a * dy);

      dx       = dx * ratio_h;
      dy       = dy * ratio_h;

      point1.x = phix + dy;
      point1.y = phiy - dx;

      point2.x = phix - dy;
      point2.y = phiy + dx;
   }

   template <typename T, typename OutputIterator>
   inline void intersection_point(const segment<T,2>&  segment,
                                  const triangle<T,2>& triangle,
                                  OutputIterator       out)
   {
      T ix = T(0.0);
      T iy = T(0.0);

      std::size_t int_count = 0;

      if (intersect(segment,edge(triangle,1),ix,iy))
      {
         (*out++) = make_point(ix,iy);

         ++int_count;
      }

      if (intersect(segment,edge(triangle,2),ix,iy))
      {
         if (int_count)
         {
            (*out++) = make_point(ix,iy);
            return;
         }

         (*out++) = make_point(ix,iy);

         ++int_count;
      }

      if (intersect(segment,edge(triangle,3),ix,iy))
      {
         if (int_count)
         {
           (*out++) = make_point(ix,iy);
            return;
         }

         (*out++) = make_point(ix,iy);
      }
   }

   template <typename T>
   inline void intersection_point(const line<T,3>&     line,
                                  const triangle<T,3>& triangle,
                                  point3d<T>&          ipoint)
   {
      const vector3d<T> dir = line[1] - line[0];
      const vector3d<T> u   = triangle[1] - triangle[0];
      const vector3d<T> v   = triangle[2] - triangle[0];
      const vector3d<T> n   = u * v;
      const vector3d<T> w   = line[0] - triangle[0];

      const T a = dot_product(n,w) * T(-1.0);
      const T b = dot_product(n,dir);
      const T r = a / b;

      ipoint = line[0] + r * dir;
   }

   template <typename T>
   inline point3d<T> intersection_point(const line<T,3>&  line,
                                        const plane<T,3>& plane)
   {
      const vector3d<T> line_vec = line[1] - line[0];

      const T denom = dot_product(line_vec,plane.normal);

      point3d<T> ipoint = degenerate_point3d<T>();

      if (not_equal(denom,T(0.0)))
      {
         const T t = -distance(line[0],plane) / denom;

         ipoint = line[0] + t * (line[1] - line[0]);
      }

      return ipoint;
   }

   template <typename T, typename OutputIterator>
   inline void intersection_point(const T& x1, const T& y1,
                                  const T& x2, const T& y2,
                                  const T& cx, const T& cy,
                                  const T& radius,
                                  OutputIterator out)
   {
      const bool p1_in_circle = point_in_circle(x1, y1, cx, cy, radius);
      const bool p2_in_circle = point_in_circle(x2, y2, cx, cy, radius);

      T ix = T(0.0);
      T iy = T(0.0);

      if (p1_in_circle && p2_in_circle)
      {
         (*out++) = make_point(x1,y1);
         (*out++) = make_point(x2,y2);

         return;
      }

      T px = T(0.0);
      T py = T(0.0);

      if (p1_in_circle || p2_in_circle)
      {
         closest_point_on_line_from_point(x1,y1,x2,y2,cx,cy,px,py);

         if (p1_in_circle)
         {
            const T h = distance(px,py,cx,cy);
            const T a = sqrt((radius * radius) - (h * h));

            (*out++) = make_point(x1,y1);

            project_point(px,py,x2,y2,a,ix,iy);

            (*out++) = make_point(ix,iy);
         }
         else if (p2_in_circle)
         {
            const T h = distance(px,py,cx,cy);
            const T a = sqrt((radius * radius) - (h * h));

            (*out++) = make_point(x2,y2);

            project_point(px,py,x1,y1,a,ix,iy);

            (*out++) = make_point(ix,iy);
         }

         return;
      }

      closest_point_on_segment_from_point(x1,y1,x2,y2,cx,cy,px,py);

      if (
           (is_equal(x1,px) && is_equal(y1,py)) ||
           (is_equal(x2,px) && is_equal(y2,py))
         )
         return;
      else
      {
         T h = distance(px,py,cx,cy);

         if (h > radius)
            return;
         else if (is_equal(h,radius))
         {
            (*out++) = make_point(x1,y1);
            return;
         }
         else if (is_equal(h,T(0.0)))
         {
            project_point(cx,cy,x1,y1,radius,ix,iy);

            (*out++) = make_point(ix,iy);

            project_point(cx,cy,x2,y2,radius,ix,iy);

            (*out++) = make_point(ix,iy);

            return;
         }
         else
         {
            const T a = sqrt((radius * radius) - (h * h));

            project_point(px,py,x1,y1,a,ix,iy);

            (*out++) = make_point(ix,iy);

            project_point(px,py,x2,y2,a,ix,iy);

            (*out++) = make_point(ix,iy);
         }
      }
   }

   template <typename T, typename OutputIterator>
   inline void intersection_point(const segment<T,2>& segment,
                                  const circle<T>&    circle,
                                  OutputIterator      out)
   {
      intersection_point
      (
        segment[0].x, segment[0].y,
        segment[1].x, segment[1].y,
        circle.x    , circle.y    , circle.radius,
        out
      );
   }

   template <typename T, typename OutputIterator>
   inline void intersection_point(const line<T,2>& line,
                                  const circle<T>& circle,
                                  OutputIterator   out)
   {
      const T a =  sqr(line[1].x - line[0].x) + sqr(line[1].y - line[0].y);

      const T b =  T(2.0) * ((line[1].x - line[0].x)*(line[0].x - circle.x) +
                             (line[1].y - line[0].y)*(line[0].y - circle.y));

      const T c = sqr(circle.x)  + sqr(circle.y)  +
                  sqr(line[0].x) + sqr(line[0].y) -
                  T(2.0) * (circle.x * line[0].x + circle.y * line[0].y) - sqr(circle.radius);

      const T det =  b * b - T(4.0) * a * c;

      if (det < T(0.0))
      {
         return;
      }
      else if (is_equal(det,T(0.0)))
      {
         const T delta = -b / (T(2.0) * a);

         (*out++) = make_point
                    (
                      line[0].x + delta * (line[1].x - line[0].x),
                      line[0].y + delta * (line[1].y - line[0].y)
                    );

         return;
      }
      else if (det > T(0.0))
      {
         const T sqrt_det = sqrt(det);

         T delta = (-b + sqrt_det) / (T(2.0) * a);

         (*out++) = make_point
                    (
                      line[0].x + delta * (line[1].x - line[0].x),
                      line[0].y + delta * (line[1].y - line[0].y)
                    );

         delta = (-b - sqrt_det) / (T(2.0) * a);

         (*out++) = make_point
                    (
                      line[0].x + delta * (line[1].x - line[0].x),
                      line[0].y + delta * (line[1].y - line[0].y)
                    );

         return;
      }
   }

   template <typename T, typename OutputIterator>
   inline void intersection_point(const segment<T,3>& segment,
                                  const sphere<T>&    sphere,
                                  OutputIterator      out)
   {
      std::vector< point3d<T> > ipoint_;

      intersection_point(make_line(segment),sphere,std::back_inserter(ipoint_));

      for (std::size_t i = 0; i < ipoint_.size(); ++i)
      {
         if (point_in_box(ipoint_[i],segment))
         {
            (*out++) = ipoint_[i];
         }
      }
   }

   template <typename T, typename OutputIterator>
   inline void intersection_point(const line<T,3>& line,
                                  const sphere<T>& sphere,
                                  OutputIterator   out)
   {
      const T a =  sqr(line[1].x - line[0].x) + sqr(line[1].y - line[0].y) + sqr(line[1].z - line[0].z);

      const T b =  T(2.0) * ((line[1].x - line[0].x)*(line[0].x - sphere.x) +
                             (line[1].y - line[0].y)*(line[0].y - sphere.y) +
                             (line[1].z - line[0].z)*(line[0].z - sphere.z));

      const T c = sqr(sphere.x)  + sqr(sphere.y)  +
                  sqr(sphere.z)  + sqr(line[0].x) +
                  sqr(line[0].y) + sqr(line[0].z) -
                  T(2.0) * (sphere.x * line[0].x + sphere.y * line[0].y + sphere.z * line[0].z) - sqr(sphere.radius);

      const T det =  b * b - T(4.0) * a * c ;

      if (det < T(0.0))
      {
         return;
      }
      else if (is_equal(det,T(0.0)))
      {
         const T delta = -b / (T(2.0) * a);

         (*out++) = make_point
                    (
                      line[0].x + delta * (line[1].x - line[0].x),
                      line[0].y + delta * (line[1].y - line[0].y),
                      line[0].z + delta * (line[1].z - line[0].z)
                    );
         return;
      }
      else if (det > T(0.0))
      {
         T delta = (-b + sqrt(det)) / (T(2.0) * a);

         (*out++) = make_point
                    (
                      line[0].x + delta * (line[1].x - line[0].x),
                      line[0].y + delta * (line[1].y - line[0].y),
                      line[0].z + delta * (line[1].z - line[0].z)
                    );

         delta = (-b - sqrt(det)) / (T(2.0) * a);

         (*out++) = make_point
                    (
                      line[0].x + delta * (line[1].x - line[0].x),
                      line[0].y + delta * (line[1].y - line[0].y),
                      line[0].z + delta * (line[1].z - line[0].z)
                    );
         return;
      }
   }

   template <typename T>
   inline point2d<T> intersection_point(const ray<T,2>& ray1, const ray<T,2>& ray2)
   {
      const T denom = (ray2.direction.y * ray1.direction.x  - ray2.direction.x * ray1.direction.y);

      if (denom != T(0.0))
      {
         const T ta = (ray2.direction.x * (ray1.origin.y - ray2.origin.y) - ray2.direction.y * (ray1.origin.x - ray2.origin.x)) / denom;
         const T tb = (ray1.direction.y * (ray2.origin.x - ray1.origin.x) - ray1.direction.x * (ray2.origin.y - ray1.origin.y)) / denom;

         if (
              greater_than_or_equal(ta,T(0.0)) &&
              greater_than_or_equal(tb,T(0.0))
            )
            return make_point
                   (
                     ray1.origin.x + (ray1.direction.x * ta),
                     ray1.origin.y + (ray1.direction.y * ta)
                   );
         else
            return degenerate_point2d<T>();

      }
      else if (point_on_ray(ray2.origin,ray1))
         return ray2.origin;
      else if (point_on_ray(ray1.origin,ray2))
         return ray1.origin;
      else
         return degenerate_point2d<T>();
   }

   template <typename T>
   inline point3d<T> intersection_point(const ray<T,3>& ray, const triangle<T,3>& triangle)
   {
      const T edge1_x = triangle[1].x - triangle[0].x;
      const T edge1_y = triangle[1].y - triangle[0].y;
      const T edge1_z = triangle[1].z - triangle[0].z;

      const T edge2_x = triangle[2].x - triangle[0].x;
      const T edge2_y = triangle[2].y - triangle[0].y;
      const T edge2_z = triangle[2].z - triangle[0].z;

      const T pvec_x = (ray.direction.y * edge2_z) - (ray.direction.z * edge2_y);
      const T pvec_y = (ray.direction.z * edge2_x) - (ray.direction.x * edge2_z);
      const T pvec_z = (ray.direction.x * edge2_y) - (ray.direction.y * edge2_x);

      const T det = edge1_x * pvec_x + edge1_y * pvec_y + edge1_z * pvec_z;

      if (is_equal(det,T(0.0)))
      {
         return degenerate_point3d<T>();
      }

      const T inv_det = T(1.0) / det;

      const T tvec_x = ray.origin.x - triangle[0].x;
      const T tvec_y = ray.origin.y - triangle[0].y;
      const T tvec_z = ray.origin.z - triangle[0].z;

      const T u = (tvec_x * pvec_x + tvec_y * pvec_y + tvec_z * pvec_z) * inv_det;

      if ((u < T(0.0)) || (u > T(1.0)))
      {
         return degenerate_point3d<T>();
      }

      const T qvec_x = (tvec_y * edge1_z) - (tvec_z * edge1_y);
      const T qvec_y = (tvec_z * edge1_x) - (tvec_x * edge1_z);
      const T qvec_z = (tvec_x * edge1_y) - (tvec_y * edge1_x);

      const T v = (ray.direction.x * qvec_x + ray.direction.y * qvec_y + ray.direction.z * qvec_z) * inv_det;

      if ((v < T(0.0)) || ((u + v) > T(1.0)))
      {
         return degenerate_point3d<T>();
      }

      const T t = (edge2_x * qvec_x + edge2_y * qvec_y + edge2_z * qvec_z) * inv_det;

      if (t > T(0.0))
         return make_point
                (
                  ray.origin.x + (ray.direction.x * t),
                  ray.origin.y + (ray.direction.y * t),
                  ray.origin.z + (ray.direction.z * t)
                );
      else
         return degenerate_point3d<T>();
   }

   template <typename T>
   inline point3d<T> intersection_point(const ray<T,3>& ray, const plane<T,3>& plane)
   {
      const T denom = dot_product(ray.direction,plane.normal);

      point3d<T> ipoint = degenerate_point3d<T>();

      if (not_equal(denom,T(0.0)))
      {
         const T t = -distance(ray.origin,plane) / denom;

         if (t >= T(0.0))
         {
            ipoint = ray.origin + t * ray.direction;
         }
      }

      return ipoint;
   }

   template <typename T, typename OutputIterator>
   inline void intersection_point(const ray<T,2>& ray,
                                  const circle<T>& circle,
                                  OutputIterator out)

   {
      std::vector< point2d<T> > ipoint_;

      intersection_point(make_line(ray),circle,std::back_inserter(ipoint_));

      if (!ipoint_.empty())
      {
         for (std::size_t i = 0; i < ipoint_.size(); ++i)
         {
             if (point_on_ray(ipoint_[i],ray))
             {
                (*out++) = ipoint_[i];
             }
         }
      }
   }

   template <typename T, typename OutputIterator>
   inline void intersection_point(const ray<T,3>& ray,
                                  const sphere<T>& sphere,
                                  OutputIterator out)
   {
      const vector3d<T> v = ray.origin - make_point(sphere);

      const T diff = dot_product(v,v) - sqr(sphere.radius);

      if (less_than_or_equal(diff,T(0.0)))
      {
         const T b = dot_product(ray.direction,v);
         const T t = -b + sqrt(sqr(b) - diff);

         (*out++) = (ray.origin + t * ray.direction);

         return;
      }

      const T a = dot_product(ray.direction,v);

      if (greater_than_or_equal(a,T(0.0)))
      {
         return;
      }

      const T det = sqr(a) - diff;

      if (det < T(0.0))
      {
          return;
      }
      else if (greater_than_or_equal(det,T(0.0)))
      {
         const T root = sqrt(det);
         const T t1 = -a - root;
         const T t2 = -a + root;

         (*out++) = (ray.origin + t1 * ray.direction);
         (*out++) = (ray.origin + t2 * ray.direction);
      }
      else
      {
         (*out++) = (ray.origin - a * ray.direction);
      }
   }

   template <typename T>
   inline void intersection_point_line_to_line(const T& x1, const T& y1, const T& z1,
                                               const T& x2, const T& y2, const T& z2,
                                               const T& x3, const T& y3, const T& z3,
                                               const T& x4, const T& y4, const T& z4,
                                                     T& ix,       T& iy,       T& iz,
                                               const T& fuzzy)
   {
      const T ux = x2 - x1;
      const T uy = y2 - y1;
      const T uz = z2 - z1;

      const T vx = x4 - x3;
      const T vy = y4 - y3;
      const T vz = z4 - z3;

      const T wx = x1 - x3;
      const T wy = y1 - y3;
      const T wz = z1 - z3;

      const T a  = (ux * ux + uy * uy + uz * uz);
      const T b  = (ux * vx + uy * vy + uz * vz);
      const T c  = (vx * vx + vy * vy + vz * vz);
      const T d  = (ux * wx + uy * wy + uz * wz);
      const T e  = (vx * wx + vy * wy + vz * wz);
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

      const T dx = wx + (sc * ux) - (tc * vx);
      const T dy = wy + (sc * uy) - (tc * vy);
      const T dz = wz + (sc * uz) - (tc * vz);

      if ((dx * dx + dy * dy + dz * dz) <= sqr(fuzzy))
      {
         ix = ((x1 + (sc * ux)) + (x3 + (tc * vx))) * T(0.5);
         iy = ((y1 + (sc * uy)) + (y3 + (tc * vy))) * T(0.5);
         iz = ((z1 + (sc * uz)) + (z3 + (tc * vz))) * T(0.5);
      }
      else
      {
         ix = +infinity<T>();
         iy = +infinity<T>();
         iz = +infinity<T>();
      }
   }

   template <typename T>
   inline T normalize_angle(const T& angle)
   {
      T n_angle = angle;

      if (n_angle > T(360.0))
      {
         n_angle = n_angle - (int(n_angle / T(360.0)) * T(360.0));
      }
      else if (n_angle < T(0.0))
      {
         while (n_angle < T(0.0))
         {
            n_angle = n_angle + T(360.0);
         }
      }

      return n_angle;
   }

   template <typename T>
   inline T vertical_mirror(const T& angle)
   {
      if (
           is_equal(angle,   T(0.0)) ||
           is_equal(angle, T(180.0)) ||
           is_equal(angle, T(360.0))
         )
         return angle;

      return (T(360.0) - angle);
   }

   template <typename T>
   inline T horizontal_mirror(const T& angle)
   {
      if (angle <= T(180.0))
         return (T(180.0) - angle);
      else
         return (T(540.0) - angle);
   }

   template <typename T>
   inline unsigned int quadrant(const T& angle)
   {
           if ((angle >=   T(0.0)) && (angle <  T(90.0))) return 1;
      else if ((angle >=  T(90.0)) && (angle < T(180.0))) return 2;
      else if ((angle >= T(180.0)) && (angle < T(270.0))) return 3;
      else if ((angle >= T(270.0)) && (angle < T(360.0))) return 4;
      else if (angle  == T(360.0))                        return 1;
      else                                                return 0;
   }

   template <typename T>
   inline unsigned int quadrant(const T& x, const T& y)
   {
      if      ((x >  T(0.0)) && (y >= T(0.0))) return 1;
      else if ((x <= T(0.0)) && (y >  T(0.0))) return 2;
      else if ((x <  T(0.0)) && (y <= T(0.0))) return 3;
      else if ((x >= T(0.0)) && (y <  T(0.0))) return 4;
      else                                     return 0;

   }

   template <typename T>
   inline unsigned int quadrant(const point2d<T>& point)
   {
      return quadrant(point.x,point.y);
   }

   template <typename T>
   inline T vertex_angle(const T& x1, const T& y1,
                         const T& x2, const T& y2,
                         const T& x3, const T& y3)
   {
      /*
         using the cosine identity:
         cos(a) = (b^2 + c^2 - a^2) / (2*b*c)
         a      = cos'((b^2 + c^2 - a^2) / (2*b*c))

         where:
         a,b and c : are edges in the triangle
         a         : is the angle at the vertex opposite edge 'a'
         aka the edge defined by the vertex <x1y1-x2y2-x3y3>
      */

      /* quantify coordinates */
      const T x1_ = x1 - x2;
      const T x3_ = x3 - x2;
      const T y1_ = y1 - y2;
      const T y3_ = y3 - y2;

      /* calculate lay distance */
      const T dist = (x1_ * x1_ + y1_ * y1_) * (x3_ * x3_ + y3_ * y3_);

      if (is_equal(dist,T(0.0)))
         return T(0.0);
      else
      {
         const T input_term = (x1_ * x3_ + y1_ * y3_) / sqrt(dist);

         if (is_equal(input_term,T(1.0)))
            return T(0.0);
         else if (is_equal(input_term,T(-1.0)))
            return T(180.0);
         else
            return T(acos(input_term) * _180DivPI);
      }
   }

   template <typename T>
   inline T vertex_angle(const point2d<T>& point1,
                         const point2d<T>& point2,
                         const point2d<T>& point3)
   {
      return vertex_angle
             (
               point1.x,point1.y,
               point2.x,point2.y,
               point3.x,point3.y
             );
   }

   template <typename T>
   inline T vertex_angle(const T& x1, const T& y1, const T& z1,
                         const T& x2, const T& y2, const T& z2,
                         const T& x3, const T& y3, const T& z3)
   {
      /*
        Method is the same as the one described in
        the above routine.
      */

      /* Quantify coordinates */
      const T x1_ = x1 - x2;
      const T x3_ = x3 - x2;
      const T y1_ = y1 - y2;
      const T y3_ = y3 - y2;
      const T z1_ = z1 - z2;
      const T z3_ = z3 - z2;

      /* Calculate Ley Distance */
      const T dist = (x1_ * x1_ + y1_ * y1_ + z1_ * z1_) * (x3_ * x3_ + y3_ * y3_ + z3_ * z3_);

      if (is_equal(dist,T(0.0)))
         return T(0.0);
      else
         return T(acos((x1_ * x3_ + y1_ * y3_ + z1_ * z3_) / sqrt(dist)) * _180DivPI);
   }

   template <typename T>
   inline T vertex_angle(const point3d<T>& point1,
                         const point3d<T>& point2,
                         const point3d<T>& point3)
   {
      return vertex_angle
             (
               point1.x,point1.y,point1.z,
               point2.x,point2.y,point2.z,
               point3.x,point3.y,point3.z
             );
   }

   template <typename T>
   inline T oriented_vertex_angle(const T& x1, const T& y1,
                                  const T& x2, const T& y2,
                                  const T& x3, const T& y3,
                                  const int orient)
   {
      if (orientation(x1,y1,x2,y2,x3,y3) != orient)
         return T(360.0) - vertex_angle(x1,y1,x2,y2,x3,y3);
      else
         return vertex_angle(x1,y1,x2,y2,x3,y3);
   }

   template <typename T>
   inline T oriented_vertex_angle(const point2d<T>& point1,
                                  const point2d<T>& point2,
                                  const point2d<T>& point3,
                                  const int orient)
   {
      return oriented_vertex_angle
             (
               point1.x,point1.y,
               point2.x,point2.y,
               point3.x,point3.y,
               orient
             );
   }

   template <typename T>
   inline T cartesian_angle(const T& x, const T& y)
   {
      if      ((x >  T(0.0)) && (y >  T(0.0))) return T(atan( y / x) * _180DivPI);
      else if ((x <  T(0.0)) && (y >  T(0.0))) return T(atan(-x / y) * _180DivPI) +  T(90.0);
      else if ((x <  T(0.0)) && (y <  T(0.0))) return T(atan( y / x) * _180DivPI) + T(180.0);
      else if ((x >  T(0.0)) && (y <  T(0.0))) return T(atan(-x / y) * _180DivPI) + T(270.0);
      else if ((x == T(0.0)) && (y >  T(0.0))) return T( 90.0);
      else if ((x <  T(0.0)) && (y == T(0.0))) return T(180.0);
      else if ((x == T(0.0)) && (y <  T(0.0))) return T(270.0);
      else                                     return T(  0.0);
   }

   template <typename T>
   inline T cartesian_angle(const point2d<T>& point)
   {
      return cartesian_angle(point.x,point.y);
   }

   template <typename T>
   inline T robust_cartesian_angle(const T& x, const T& y)
   {
      if      (        (x >  T(0.0)) &&         (y >  T(0.0))) return T(atan( y / x) * _180DivPI);
      else if (        (x <  T(0.0)) &&         (y >  T(0.0))) return T(atan(-x / y) * _180DivPI) +  T(90.0);
      else if (        (x <  T(0.0)) &&         (y <  T(0.0))) return T(atan( y / x) * _180DivPI) + T(180.0);
      else if (        (x >  T(0.0)) &&         (y <  T(0.0))) return T(atan(-x / y) * _180DivPI) + T(270.0);
      else if (is_equal(x,   T(0.0)) &&         (y >  T(0.0))) return T( 90.0);
      else if (        (x <  T(0.0)) && is_equal(y,   T(0.0))) return T(180.0);
      else if (is_equal(x,   T(0.0)) &&         (y <  T(0.0))) return T(270.0);
      else                                                     return T(  0.0);

   }

   template <typename T>
   inline T robust_cartesian_angle(const point2d<T>& point)
   {
      return robust_cartesian_angle(point.x,point.y);
   }

   template <typename T>
   inline T cartesian_angle(const T& x, const T& y, const T& ox, const T& oy)
   {
      return cartesian_angle(x - ox, y - oy);
   }

   template <typename T>
   inline T cartesian_angle(const point2d<T>& point, const point2d<T>& origin)
   {
      return cartesian_angle(point.x - origin.x, point.y - origin.y);
   }

   template <typename T>
   inline T robust_cartesian_angle(const T& x, const T& y, const T& ox, const T& oy)
   {
      return robust_cartesian_angle(x - ox, y - oy);
   }

   template <typename T>
   inline T robust_cartesian_angle(const point2d<T>& point, const point2d<T>& origin)
   {
      return robust_cartesian_angle(point.x - origin.x, point.y - origin.y);
   }

   template <typename T>
   inline bool parallel(const T& x1, const T& y1,
                        const T& x2, const T& y2,
                        const T& x3, const T& y3,
                        const T& x4, const T& y4,
                        const T& epsilon)
   {
      return is_equal(((y1 - y2) * (x3 - x4)),((y3 - y4) * (x1 - x2)),epsilon);
   }

   template <typename T>
   inline bool parallel(const point2d<T>& point1,
                        const point2d<T>& point2,
                        const point2d<T>& point3,
                        const point2d<T>& point4,
                        const T& epsilon)
   {
      return parallel
             (
               point1.x, point1.y,
               point2.x, point2.y,
               point3.x, point3.y,
               point4.x, point4.y,
               epsilon
             );
   }

   template <typename T>
   inline bool parallel(const segment<T,2>& segment1,
                        const segment<T,2>& segment2,
                        const T& epsilon)
   {
      return parallel
             (
               segment1[0], segment1[1],
               segment2[0], segment2[1],
               epsilon
             );
   }

   template <typename T>
   inline bool parallel(const line<T,2>& line1,
                        const line<T,2>& line2,
                        const T& epsilon)
   {
      return parallel
             (
               line1[0], line1[1],
               line2[0], line2[1],
               epsilon
             );
   }

   template <typename T>
   inline bool parallel(const T& x1, const T& y1, const T& z1,
                        const T& x2, const T& y2, const T& z2,
                        const T& x3, const T& y3, const T& z3,
                        const T& x4, const T& y4, const T& z4,
                        const T& epsilon)
   {
      /*
         Theory:
         If the gradients in the following planes x-y, y-z, z-x are equal
         then it can be said that the segments are parallel in 3d.
         Worst case scenario: 6 multiplications and 9 subtractions
      */

      const T dx1 = x1 - x2;
      const T dx2 = x3 - x4;

      const T dy1 = y1 - y2;
      const T dy2 = y3 - y4;

      const T dz1 = z1 - z2;
      const T dz2 = z3 - z4;

      if (not_equal((dy1 * dx2),(dy2 * dx1),epsilon)) return false;
      if (not_equal((dz1 * dy2),(dz2 * dy1),epsilon)) return false;
      if (not_equal((dx1 * dz2),(dx2 * dz1),epsilon)) return false;

      return true;
   }

   template <typename T>
   inline bool parallel(const point3d<T>& point1,
                        const point3d<T>& point2,
                        const point3d<T>& point3,
                        const point3d<T>& point4,
                        const T& epsilon)
   {
      return parallel
             (
               point1.x, point1.y, point1.z,
               point2.x, point2.y, point2.z,
               point3.x, point3.y, point3.z,
               point4.x, point4.y, point4.z, epsilon
             );
   }

   template <typename T>
   inline bool parallel(const segment<T,3>& segment1,
                        const segment<T,3>& segment2,
                        const T& epsilon)
   {
      return parallel
             (
               segment1[0],segment1[1],
               segment2[0],segment2[1],epsilon
             );
   }

   template <typename T>
   inline bool parallel(const line<T,3>& line1,
                        const line<T,3>& line2,
                        const T& epsilon)
   {
      return parallel
             (
               line1[0],line1[1],
               line2[0],line2[1],epsilon
             );
   }

   template <typename T>
   inline bool robust_parallel(const T& x1, const T& y1,
                               const T& x2, const T& y2,
                               const T& x3, const T& y3,
                               const T& x4, const T& y4,
                               const T& epsilon)
   {
      T px1 = T(0.0);
      T py1 = T(0.0);
      T px2 = T(0.0);
      T py2 = T(0.0);

      closest_point_on_line_from_point(x1,y1,x2,y2,x3,y3,px1,py1);
      closest_point_on_line_from_point(x1,y1,x2,y2,x4,y4,px2,py2);

      return is_equal
             (
               distance(x3, y3, px1, py1),
               distance(x4, y4, px2, py2),
               epsilon
             );
   }

   template <typename T>
   inline bool robust_parallel(const point2d<T>& point1,
                               const point2d<T>& point2,
                               const point2d<T>& point3,
                               const point2d<T>& point4,
                               const T& epsilon)
   {
      return robust_parallel
             (
               point1.x, point1.y,
               point2.x, point2.y,
               point3.x, point3.y,
               point4.x, point4.y,
               epsilon
             );
   }

   template <typename T>
   inline bool robust_parallel(const segment<T,2>& segment1,
                               const segment<T,2>& segment2,
                               const T& epsilon)
   {
      return robust_parallel
             (
               segment1[0], segment1[1],
               segment2[0], segment2[1],
               epsilon
             );
   }

   template <typename T>
   inline bool robust_parallel(const line<T,2>& line1,
                               const line<T,2>& line2,
                               const T& epsilon)
   {
      return robust_parallel
             (
               line1[0], line1[1],
               line2[0], line2[1],
               epsilon
             );
   }

   template <typename T>
   inline bool robust_parallel(const line<T,2>& line,
                               const segment<T,2>& segment,
                               const T& epsilon)
   {
      return robust_parallel
             (
               line[0],   line[1],
               segment[0],segment[1],
               epsilon
             );
   }

   template <typename T>
   inline bool robust_parallel(const T& x1, const T& y1, const T& z1,
                               const T& x2, const T& y2, const T& z2,
                               const T& x3, const T& y3, const T& z3,
                               const T& x4, const T& y4, const T& z4,
                               const T& epsilon)
   {
      T px1 = T(0.0);
      T py1 = T(0.0);
      T pz1 = T(0.0);
      T px2 = T(0.0);
      T py2 = T(0.0);
      T pz2 = T(0.0);

      closest_point_on_line_from_point(x1,y1,z1,x2,y2,z2,x3,y3,z3,px1,py1,pz1);
      closest_point_on_line_from_point(x1,y1,z1,x2,y2,z2,x4,y4,z4,px2,py2,pz2);

      return is_equal
             (
               distance(x3, y3, z3, px1, py1, pz1),
               distance(x4, y4, z4, px2, py2, pz2),
               epsilon
             );
   }

   template <typename T>
   inline bool robust_parallel(const point3d<T>& point1,
                               const point3d<T>& point2,
                               const point3d<T>& point3,
                               const point3d<T>& point4,
                               const T& epsilon)
   {
      return robust_parallel
             (
               point1.x, point1.y, point1.z,
               point2.x, point2.y, point2.z,
               point3.x, point3.y, point3.z,
               point4.x, point4.y, point4.z,
               epsilon
             );
   }

   template <typename T>
   inline bool robust_parallel(const segment<T,3>& segment1,
                               const segment<T,3>& segment2,
                               const T& epsilon)
   {
      return robust_parallel
             (
               segment1[0], segment1[1],
               segment2[0], segment2[1],
               epsilon
             );
   }

   template <typename T>
   inline bool robust_parallel(const line<T,3>& line1,
                               const line<T,3>& line2,
                               const T& epsilon)
   {
      return robust_parallel
             (
               line1[0], line1[1],
               line2[0], line2[1],
               epsilon
             );
   }

   template <typename T>
   inline bool robust_parallel(const line<T,3>& line,
                               const segment<T,3>& segment,
                               const T& epsilon)
   {
      return robust_parallel
             (
               line   [0], line   [1],
               segment[0], segment[1],
               epsilon
             );
   }

   template <typename T>
   inline bool perpendicular(const T& x1, const T& y1,
                             const T& x2, const T& y2,
                             const T& x3, const T& y3,
                             const T& x4, const T& y4,
                             const T& epsilon)
   {
      return is_equal(T(-1.0) * (y2 - y1) * (y4 - y3),(x4 - x3) * (x2 - x1), epsilon);
   }

   template <typename T>
   inline bool perpendicular(const point2d<T>& point1,
                             const point2d<T>& point2,
                             const point2d<T>& point3,
                             const point2d<T>& point4,
                             const T& epsilon)
   {
      return perpendicular
             (
               point1.x, point1.y,
               point2.x, point2.y,
               point3.x, point3.y,
               point4.x, point4.y,
               epsilon
             );
   }

   template <typename T>
   inline bool perpendicular(const segment<T,2>& segment1,
                             const segment<T,2>& segment2,
                             const T& epsilon)
   {
      return perpendicular
             (
               segment1[0], segment1[1],
               segment2[0], segment2[1],
               epsilon
             );
   }

   template <typename T>
   inline bool perpendicular(const line<T,2>& line1,
                             const line<T,2>& line2,
                             const T& epsilon)
   {
      return perpendicular
             (
               line1[0], line1[1],
               line2[0], line2[1],
               epsilon
             );
   }

   template <typename T>
   inline bool perpendicular(const line<T,2>& line,
                             const segment<T,2>& segment,
                             const T& epsilon)
   {
      return perpendicular
             (
               line   [0], line   [1],
               segment[0], segment[1],
               epsilon
              );
   }

   template <typename T>
   inline bool perpendicular(const T& x1, const T& y1, const T& z1,
                             const T& x2, const T& y2, const T& z2,
                             const T& x3, const T& y3, const T& z3,
                             const T& x4, const T& y4, const T& z4,
                             const T& epsilon)
   {
      /*
         The dot product of the vector forms of the segments will
         be 0 if the segments are perpendicular
      */

      const T dx1 = x1 - x2;
      const T dx2 = x3 - x4;

      const T dy1 = y1 - y2;
      const T dy2 = y3 - y4;

      const T dz1 = z1 - z2;
      const T dz2 = z3 - z4;

      return is_equal((dx1 * dx2) + (dy1 * dy2) + (dz1 * dz2), T(0.0), epsilon);
   }

   template <typename T>
   inline bool perpendicular(const point3d<T>& point1,
                             const point3d<T>& point2,
                             const point3d<T>& point3,
                             const point3d<T>& point4,
                             const T& epsilon)
   {
      return perpendicular
             (
               point1.x, point1.y, point1.z,
               point2.x, point2.y, point2.z,
               point3.x, point3.y, point3.z,
               point4.x, point4.y, point4.z,
               epsilon
             );
   }

   template <typename T>
   inline bool perpendicular(const segment<T,3>& segment1,
                             const segment<T,3>& segment2,
                             const T& epsilon)
   {
      return perpendicular
             (
               segment1[0], segment1[1],
               segment2[0], segment2[1],
               epsilon
             );
   }

   template <typename T>
   inline bool perpendicular(const line<T,3>& line1,
                             const line<T,3>& line2,
                             const T& epsilon)
   {
      return perpendicular
             (
               line1[0], line1[1],
               line2[0], line2[1],
               epsilon
             );
   }

   template <typename T>
   inline bool robust_perpendicular(const T& x1, const T& y1,
                                    const T& x2, const T& y2,
                                    const T& x3, const T& y3,
                                    const T& x4, const T& y4,
                                    const T& epsilon)
   {
      T p1x = T(0.0);
      T p1y = T(0.0);
      T p2x = T(0.0);
      T p2y = T(0.0);

      closest_point_on_line_from_point(x1, y1, x2, y2, x3, y3, p1x, p1y);
      closest_point_on_line_from_point(x1, y1, x2, y2, x4, y4, p2x, p2y);

      return is_equal(distance(p1x,p1y,p2x,p2y),T(0.0),epsilon);
   }

   template <typename T>
   inline bool robust_perpendicular(const point2d<T>& point1,
                                    const point2d<T>& point2,
                                    const point2d<T>& point3,
                                    const point2d<T>& point4,
                                    const T& epsilon)
   {
      return robust_perpendicular
             (
               point1.x, point1.y,
               point2.x, point2.y,
               point3.x, point3.y,
               point4.x, point4.y,
               epsilon
             );
   }

   template <typename T>
   inline bool robust_perpendicular(const segment<T,2>& segment1,
                                    const segment<T,2>& segment2,
                                    const T& epsilon)
   {
      return robust_perpendicular
             (
               segment1[0], segment1[1],
               segment2[0], segment2[1],
               epsilon
             );
   }

   template <typename T>
   inline bool robust_perpendicular(const line<T,2>& line1,
                                    const line<T,2>& line2,
                                    const T& epsilon)
   {
      return robust_perpendicular
             (
               line1[0], line1[1],
               line2[0], line2[1],
               epsilon
             );
   }

   template <typename T>
   inline bool robust_perpendicular(const line<T,2>& line,
                                    const segment<T,2>& segment,
                                    const T& epsilon)
   {
      return robust_perpendicular
             (
               line   [0], line  [1],
               segment[0],segment[1],
               epsilon
             );
   }

   template <typename T>
   inline bool robust_perpendicular(const T& x1, const T& y1, const T& z1,
                                    const T& x2, const T& y2, const T& z2,
                                    const T& x3, const T& y3, const T& z3,
                                    const T& x4, const T& y4, const T& z4,
                                    const T& epsilon)
   {
      T p1x = T(0.0);
      T p1y = T(0.0);
      T p1z = T(0.0);
      T p2x = T(0.0);
      T p2y = T(0.0);
      T p2z = T(0.0);

      closest_point_on_line_from_point(x1,y1,z1,x2,y2,z2,x3,y3,z3,p1x,p1y,p1z);
      closest_point_on_line_from_point(x1,y1,z1,x2,y2,z2,x4,y4,z4,p2x,p2y,p2z);

      return is_equal(distance(p1x,p1y,p1z,p2x,p2y,p2z),T(0.0),epsilon);
   }

   template <typename T>
   inline bool robust_perpendicular(const point3d<T>& point1,
                                    const point3d<T>& point2,
                                    const point3d<T>& point3,
                                    const point3d<T>& point4,
                                    const T& epsilon)
   {
      return robust_perpendicular
             (
               point1.x, point1.y, point1.z,
               point2.x, point2.y, point2.z,
               point3.x, point3.y, point3.z,
               point4.x, point4.y, point4.z,
               epsilon
             );
   }

   template <typename T>
   inline bool robust_perpendicular(const segment<T,3>& segment1,
                                    const segment<T,3>& segment2,
                                    const T& epsilon)
   {
      return robust_perpendicular
             (
               segment1[0], segment1[1],
               segment2[0], segment2[1],
               epsilon
             );
   }

   template <typename T>
   inline bool robust_perpendicular(const line<T,3>& line1,
                                    const line<T,3>& line2,
                                    const T& epsilon)
   {
      return robust_perpendicular
             (
               line1[0], line1[1],
               line2[0], line2[1],
               epsilon
             );
   }

   template <typename T>
   inline bool line_to_line_intersect(const T& x1, const T& y1,
                                      const T& x2, const T& y2,
                                      const T& x3, const T& y3,
                                      const T& x4, const T& y4)
   {
       if (not_equal((x1 - x2) * (y3 - y4),(y1 - y2) * (x3 - x4)))
         return true;
      else if (collinear(x1,y1,x2,y2,x3,y3))
         return true;
      else
         return false;
   }

   template <typename T>
   inline bool line_to_line_intersect(const line<T,2>& line1, const line<T,2>& line2)
   {
      return line_to_line_intersect(line1[0].x,line1[0].y,line1[1].x,line1[1].y,
                                    line2[0].x,line2[0].y,line2[1].x,line2[1].y);
   }

   template <typename T>
   inline bool rectangle_to_rectangle_intersect(const T& x1, const T& y1,
                                                const T& x2, const T& y2,
                                                const T& x3, const T& y3,
                                                const T& x4, const T& y4)
   {
      return (
               (x1 <= x4) && (x2 >= x3) &&
               (y1 <= y4) && (y2 >= y3)
             );
   }

   template <typename T>
   inline bool rectangle_to_rectangle_intersect(const rectangle<T>& rectangle1,
                                                const rectangle<T>& rectangle2)
   {
     return rectangle_to_rectangle_intersect(rectangle1[0].x,rectangle1[0].y,
                                             rectangle1[1].x,rectangle1[1].y,
                                             rectangle2[0].x,rectangle2[0].y,
                                             rectangle2[1].x,rectangle2[1].y);
   }

   template <typename T>
   inline bool box_to_box_intersect(const T& x1, const T& y1, const T& z1,
                                    const T& x2, const T& y2, const T& z2,
                                    const T& x3, const T& y3, const T& z3,
                                    const T& x4, const T& y4, const T& z4)
   {
      return (
               (x1 <= x4) && (x2 >= x3) &&
               (y1 <= y4) && (y2 >= y3) &&
               (z1 <= z4) && (z2 >= z3)
             );
   }

   template <typename T>
   inline bool box_to_box_intersect(const box<T,3>& box1, const box<T,3>& box2)
   {
      return box_to_box_intersect(box1[0].x, box1[0].y, box1[0].z,
                                  box1[1].x, box1[1].y, box1[1].z,
                                  box2[0].x, box2[0].y, box2[0].z,
                                  box2[1].x, box2[1].y, box2[1].z);
   }

   template <typename T, unsigned int Dimension, typename Simplex, typename Bezier>
   inline bool simplex_to_bezier_intersect(const Simplex& simplex,
                                           const Bezier& bezier,
                                           const std::size_t& steps)
   {
      if (0 == steps) return false;

      typedef typename define_point_type<T,Dimension>::PointType PointType;

      const T dt = T(1.0) / (T(1.0) * steps - T(1.0));
      T t  = T(0.0);

      bezier_coefficients<T,Dimension,Bezier::Type> coeffs;

      calculate_bezier_coefficients(bezier,coeffs);

      PointType previous_point = create_point_on_bezier(bezier[0],coeffs,t);

      t += dt;

      for (std::size_t i = 1; i < steps; ++i)
      {
         PointType current_point = create_point_on_bezier(bezier[0],coeffs,t);

         if (intersect(make_segment(previous_point,current_point),simplex))
         {
            return true;
         }

         previous_point = current_point;

         t += dt;
      }

      return false;
   }

   template <typename T, unsigned int Dimension, typename Bezier, typename Iterator>
   inline bool simplex_to_bezier_intersect(const Iterator& begin,
                                           const Iterator& end,
                                           const Bezier& bezier,
                                           const std::size_t& steps)
   {
      if (0 == steps) return false;

      typedef typename define_point_type<T,Dimension>::PointType PointType;

      T t  = T(0.0);
      T dt = T(1.0) / (T(1.0) * steps - T(1.0));
      bezier_coefficients<T,Dimension,Bezier::Type> coeffs;

      calculate_bezier_coefficients(bezier,coeffs);

      PointType previous_point = create_point_on_bezier(bezier[0],coeffs,t);
      t += dt;

      for (std::size_t i = 1; i < steps; ++i)
      {
         PointType current_point = create_point_on_bezier(bezier[0],coeffs,t);

         wykobi::segment<T,Dimension> segment = make_segment(previous_point,current_point);

         for (Iterator it = begin; it != end; ++it)
         {
            if (intersect(segment,(*it)))
            {
               return true;
            }
         }

         previous_point = current_point;
         t += dt;
      }

      return false;
   }

   template <typename T>
   inline bool rectangle_within_rectangle(const T& x1, const T& y1,
                                          const T& x2, const T& y2,
                                          const T& x3, const T& y3,
                                          const T& x4, const T& y4)
   {
      return point_in_rectangle(x1, y1, x3, y3, x4, y4) &&
             point_in_rectangle(x2, y2, x3, y3, x4, y4) ;
   }

   template <typename T>
   inline bool rectangle_within_rectangle(const rectangle<T>& rectangle1,
                                          const rectangle<T>& rectangle2)
   {
      return rectangle_within_rectangle
             (
               rectangle1[0].x, rectangle1[0].y,
               rectangle1[1].x, rectangle1[1].y,
               rectangle2[0].x, rectangle2[0].y,
               rectangle2[1].x, rectangle2[1].y
             );
   }

   template <typename T>
   inline bool box_within_box(const T& x1, const T& y1, const T& z1,
                              const T& x2, const T& y2, const T& z2,
                              const T& x3, const T& y3, const T& z3,
                              const T& x4, const T& y4, const T& z4)
   {
      return point_in_box(x1, y1, z1, x3, y3, z3, x4, y4, z4) &&
             point_in_box(x2, y2, z2, x3, y3, z3, x4, y4, z4) ;
   }

   template <typename T>
   inline bool box_within_box(const box<T,3>& box1, const box<T,3>& box2)
   {
      return box_within_box
             (
               box1[0].x, box1[0].y, box1[0].z,
               box1[1].x, box1[1].y, box1[1].z,
               box2[0].x, box2[0].y, box2[0].z,
               box2[1].x, box2[1].y, box2[1].z
             );
   }

   template <typename T>
   inline bool circle_within_rectangle(const T&  x, const T&  y, const T& radius,
                                       const T& x1, const T& y1,
                                       const T& x2, const T& y2)
   {
      return rectangle_within_rectangle
             (
               aabb(make_circle(x, y, radius)),
               make_rectangle(x1, y1, x2, y2)
             );
   }

   template <typename T>
   inline bool circle_within_rectangle(const circle<T>& circle, const rectangle<T>& rectangle)
   {
      return circle_within_rectangle
             (
               circle.x, circle.y, circle.radius,
               rectangle[0].x, rectangle[0].y,
               rectangle[1].x, rectangle[1].y
             );
   }

   template <typename T>
   inline bool triangle_within_rectangle(const T& x1, const T& y1,
                                         const T& x2, const T& y2,
                                         const T& x3, const T& y3,
                                         const T& x4, const T& y4,
                                         const T& x5, const T& y5)
   {
      return point_in_rectangle(x1, y1, x4, y4, x5, y5) &&
             point_in_rectangle(x2, y2, x4, y4, x5, y5) &&
             point_in_rectangle(x3, y3, x4, y4, x5, y5) ;
   }

   template <typename T>
   inline bool triangle_within_rectangle(const triangle<T,2>& triangle, const rectangle<T>& rectangle)
   {
      return triangle_within_rectangle
             (
               triangle [0].x,  triangle[0].y,
               triangle [1].x,  triangle[1].y,
               triangle [2].x,  triangle[2].y,
               rectangle[0].x, rectangle[0].y,
               rectangle[1].x, rectangle[1].y
             );
   }

   template <typename T>
   inline bool segment_within_rectangle(const T& x1, const T& y1,
                                        const T& x2, const T& y2,
                                        const T& x3, const T& y3,
                                        const T& x4, const T& y4)
   {
      return point_in_rectangle(x1,y1,x3,y3,x4,y4) &&
             point_in_rectangle(x2,y2,x3,y3,x4,y4) ;
   }

   template <typename T>
   inline bool segment_within_rectangle(const segment<T,2>& segment, const rectangle<T>& rectangle)
   {
      return segment_within_rectangle(  segment[0].x,   segment[0].y,
                                        segment[1].x,   segment[1].y,
                                      rectangle[0].x, rectangle[0].y,
                                      rectangle[1].x, rectangle[1].y);
   }

   template <typename T>
   inline bool quadix_within_rectangle(const T& x1, const T& y1,
                                       const T& x2, const T& y2,
                                       const T& x3, const T& y3,
                                       const T& x4, const T& y4,
                                       const T& x5, const T& y5,
                                       const T& x6, const T& y6)
   {
      return point_in_rectangle(x1, y1, x5, y5, x6, y6) &&
             point_in_rectangle(x2, y2, x5, y5, x6, y6) &&
             point_in_rectangle(x3, y3, x5, y5, x6, y6) &&
             point_in_rectangle(x4, y4, x5, y5, x6, y6);
   }

   template <typename T>
   inline bool quadix_within_rectangle(const quadix<T,2>& quadix, const rectangle<T>& rectangle)
   {
      return quadix_within_rectangle(   quadix[0].x,    quadix[0].y,
                                        quadix[1].x,    quadix[1].y,
                                        quadix[2].x,    quadix[2].y,
                                        quadix[3].x,    quadix[3].y,
                                     rectangle[0].x, rectangle[0].y,
                                     rectangle[1].x, rectangle[1].y);
   }

   template <typename T>
   inline bool polygon_within_rectangle(const polygon<T,2>& polygon, const rectangle<T>& rectangle)
   {
      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         if (!point_in_rectangle(polygon[i],rectangle))
         {
            return false;
         }
      }

      return true;
   }

   template <typename T>
   inline bool sphere_within_box(const T&  x, const T&  y, const T&  z, const T& radius,
                                 const T& x1, const T& y1, const T& z1,
                                 const T& x2, const T& y2, const T& z2)
   {
      return box_within_box
             (
               aabb(make_sphere(x, y, z, radius)),
               make_box(x1, y1, z1, x2, y2, z2)
             );
   }

   template <typename T>
   inline bool sphere_within_box(const sphere<T>& sphere, const box<T,3>& box)
   {
      return sphere_within_box
             (
               sphere.x, sphere.y, sphere.z, sphere.radius,
               box[0].x, box[0].y, box[0].z,
               box[1].x, box[1].y, box[1].z
             );
   }

   template <typename T>
   inline bool triangle_within_box(const T& x1, const T& y1, const T& z1,
                                   const T& x2, const T& y2, const T& z2,
                                   const T& x3, const T& y3, const T& z3,
                                   const T& x4, const T& y4, const T& z4,
                                   const T& x5, const T& y5, const T& z5)
   {
      return point_in_box(x1, y1, z1, x4, y4, z4, x5, y5, z5) &&
             point_in_box(x2, y2, z2, x4, y4, z4, x5, y5, z5) &&
             point_in_box(x3, y3, z3, x4, y4, z4, x5, y5, z5) ;

   }

   template <typename T>
   inline bool triangle_within_box(const triangle<T,3>& triangle, const box<T,3>& box)
   {
      return triangle_within_box(triangle[0].x, triangle[0].y, triangle[0].z,
                                 triangle[1].x, triangle[1].y, triangle[1].z,
                                 triangle[2].x, triangle[2].y, triangle[2].z,
                                      box[0].x,      box[0].y, box[0].z,
                                      box[1].x,      box[1].y, box[1].z);
   }

   template <typename T>
   inline bool segment_within_box(const T& x1, const T& y1, const T& z1,
                                  const T& x2, const T& y2, const T& z2,
                                  const T& x3, const T& y3, const T& z3,
                                  const T& x4, const T& y4, const T& z4)
   {
      return point_in_box(x1, y1, z1, x3, y3, z3, x4, y4, z4) &&
             point_in_box(x2, y2, z2, x3, y3, z3, x4, y4, z4);
   }

   template <typename T>
   inline bool segment_within_box(const segment<T,3>& segment, const box<T,3>& box)
   {
      return segment_within_box(segment[0].x, segment[0].y, segment[0].z,
                                segment[1].x, segment[1].y, segment[1].z,
                                    box[0].x,     box[0].y,     box[0].z,
                                    box[1].x,     box[1].y,     box[1].z);
   }

   template <typename T>
   inline bool quadix_within_box(const T& x1, const T& y1, const T& z1,
                                 const T& x2, const T& y2, const T& z2,
                                 const T& x3, const T& y3, const T& z3,
                                 const T& x4, const T& y4, const T& z4,
                                 const T& x5, const T& y5, const T& z5,
                                 const T& x6, const T& y6, const T& z6)
   {
      return point_in_box(x1, y1, z1, x5, y5, z5, x6, y6, z6) &&
             point_in_box(x2, y2, z2, x5, y5, z5, x6, y6, z6) &&
             point_in_box(x3, y3, z3, x5, y5, z5, x6, y6, z6) &&
             point_in_box(x4, y4, z4, x5, y5, z5, x6, y6, z6) ;
   }

   template <typename T>
   inline bool quadix_within_box(const quadix<T,3>& quadix, const box<T,3>& box)
   {
      return quadix_within_box
             (
               quadix[0].x, quadix[0].y, quadix[0].z,
               quadix[1].x, quadix[1].y, quadix[1].z,
               quadix[2].x, quadix[2].y, quadix[2].z,
               quadix[3].x, quadix[3].y, quadix[3].z,
               box   [0].x, box   [0].y, box   [0].z,
               box   [1].x, box   [1].y, box   [1].z
             );
   }

   template <typename T>
   inline bool polygon_within_box(const polygon<T,3>& polygon, const box<T,3>& box)
   {
      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         if (!point_in_box(polygon[i],box))
         {
            return false;
         }
      }

      return true;
   }

   template <typename T>
   inline bool circle_in_circle(const circle<T>& circle1, const circle<T>& circle2)
   {
      return greater_than_or_equal(circle2.radius - (distance(circle1.x,circle1.y,circle2.x,circle2.y) + circle1.radius),T(0.0));
   }

   template <typename T>
   inline bool is_tangent(const segment<T,2>& segment, const circle<T>& circle)
   {
      wykobi::segment<T,2> tmp_segment = translate(-circle.x, -circle.y, segment);

      const T rsqr  = wykobi::sqr(circle.radius);
      const T drsqr = lay_distance(tmp_segment);
      const T dsqr  = wykobi::sqr(tmp_segment[0].x * tmp_segment[1].y - tmp_segment[1].x * tmp_segment[0].y);

      return is_equal((rsqr * drsqr - dsqr),T(0.0));
   }

   template <typename T>
   inline bool point_of_reflection(const T& sx1, const T& sy1,
                                   const T& sx2, const T& sy2,
                                   const T& p1x, const T& p1y,
                                   const T& p2x, const T& p2y,
                                         T& rpx,       T& rpy)
   {
      if (
           (!collinear (sx1, sy1, sx2, sy2, p1x, p1y)) &&
           (!collinear (sx1, sy1, sx2, sy2, p2x, p2y)) &&
           (
             orientation(sx1, sy1, sx2, sy2, p1x, p1y) ==
             orientation(sx1, sy1, sx2, sy2, p2x, p2y)
           )
         )
      {
         T   ix = T(0.0);
         T   iy = T(0.0);
         T p1px = T(0.0);
         T p1py = T(0.0);
         T p2px = T(0.0);
         T p2py = T(0.0);

         closest_point_on_line_from_point(sx1, sy1, sx2, sy2, p1x, p1y, p1px, p1py);
         closest_point_on_line_from_point(sx1, sy1, sx2, sy2, p2x, p2y, p2px, p2py);

         intersect(p1x,p1y,p2px,p2py,p2x,p2y,p1px,p1py,ix,iy);

         closest_point_on_line_from_point(sx1,sy1,sx2,sy2,ix,iy,rpx,rpy);

         if (is_point_collinear(sx1,sy1,sx2,sy2,rpx,rpy))
         {
            return true;
         }
      }

      return false;
   }

   template <typename T>
   inline bool point_of_reflection(const segment<T,2>& segment,
                                   const point2d<T>&   point1,
                                   const point2d<T>&   point2,
                                         point2d<T>&   reflection_point)
   {
      return point_of_reflection(      segment[0].x,       segment[0].y,
                                       segment[1].x,       segment[1].y,
                                           point1.x,           point1.y,
                                           point2.x,           point2.y,
                                 reflection_point.x, reflection_point.y);
   }

   template <typename T>
   inline segment<T,2> edge(const triangle<T,2>& triangle, const std::size_t& edge_index)
   {
      switch(edge_index)
      {
          case 0 : return make_segment(triangle[0],triangle[1]);
          case 1 : return make_segment(triangle[1],triangle[2]);
          case 2 : return make_segment(triangle[2],triangle[0]);
         default : return degenerate_segment2d<T>();
      }
   }

   template <typename T>
   inline segment<T,3> edge(const triangle<T,3>& triangle, const std::size_t& edge_index)
   {
      switch(edge_index)
      {
          case 0 : return make_segment(triangle[0],triangle[1]);
          case 1 : return make_segment(triangle[1],triangle[2]);
          case 2 : return make_segment(triangle[2],triangle[0]);
         default : return degenerate_segment3d<T>();
      }
   }

   template <typename T>
   inline segment<T,2> edge(const quadix<T,2>& quadix, const std::size_t& edge_index)
   {
      switch(edge_index)
      {
          case 0 : return make_segment(quadix[0],quadix[1]);
          case 1 : return make_segment(quadix[1],quadix[2]);
          case 2 : return make_segment(quadix[2],quadix[3]);
          case 3 : return make_segment(quadix[3],quadix[0]);
         default : return degenerate_segment2d<T>();
      }
   }

   template <typename T>
   inline segment<T,3> edge(const quadix<T,3>& quadix, const std::size_t& edge_index)
   {
      switch(edge_index)
      {
          case 0 : return make_segment(quadix[0],quadix[1]);
          case 1 : return make_segment(quadix[1],quadix[2]);
          case 2 : return make_segment(quadix[2],quadix[3]);
          case 3 : return make_segment(quadix[3],quadix[0]);
         default : return degenerate_segment3d<T>();
      }
   }

   template <typename T>
   inline segment<T,2> edge(const rectangle<T>& rectangle, const std::size_t& edge_index)
   {
      switch(edge_index)
      {
          case 0 : return make_segment(rectangle[0].x,rectangle[0].y,rectangle[1].x,rectangle[0].y);
          case 1 : return make_segment(rectangle[1].x,rectangle[0].y,rectangle[1].x,rectangle[1].y);
          case 2 : return make_segment(rectangle[1].x,rectangle[1].y,rectangle[0].x,rectangle[1].y);
          case 3 : return make_segment(rectangle[0].x,rectangle[1].y,rectangle[0].x,rectangle[0].y);
         default : return degenerate_segment2d<T>();
      }
   }

   template <typename T>
   inline segment<T,2> edge(const polygon<T,2>& polygon, const std::size_t& edge_index)
   {
      if (edge_index >= polygon.size()) return make_segment(T(0.0),T(0.0),T(0.0),T(0.0));
      if (edge_index == (polygon.size() - 1))
         return make_segment(polygon[edge_index],polygon[0]);
      else
         return make_segment(polygon[edge_index],polygon[edge_index + 1]);
   }

   template <typename T>
   inline segment<T,3> edge(const polygon<T,3>& polygon, const std::size_t& edge_index)
   {
      if (edge_index >= polygon.size())
         return make_segment(T(0.0),T(0.0),T(0.0),T(0.0),T(0.0),T(0.0));
      else if (edge_index == (polygon.size() - 1))
         return make_segment(polygon[edge_index],polygon[0]);
      else
         return make_segment(polygon[edge_index],polygon[edge_index + 1]);
   }

   template <typename T>
   inline segment<T,2> opposing_edge(const triangle<T,2>& triangle, const std::size_t& corner)
   {
      switch(corner)
      {
          case 0 : return edge(triangle,1);
          case 1 : return edge(triangle,2);
          case 2 : return edge(triangle,0);
         default : return degenerate_segment2d<T>();
      }
   }

   template <typename T>
   inline segment<T,3> opposing_edge(const triangle<T,3>& triangle, const std::size_t& corner)
   {
      switch(corner)
      {
          case 0 : return edge(triangle,1);
          case 1 : return edge(triangle,2);
          case 2 : return edge(triangle,0);
         default : return degenerate_segment3d<T>();
      }
   }

   template <typename T>
   inline segment<T,2> reverse_segment(const segment<T,2>& segment)
   {
      return make_segment(segment[1],segment[0]);
   }

   template <typename T>
   inline segment<T,3> reverse_segment(const segment<T,3>& segment)
   {
      return make_segment(segment[1],segment[0]);
   }

   template <typename T>
   inline point2d<T> rectangle_corner(const rectangle<T>& rectangle, const std::size_t& corner_index)
   {
      switch(corner_index)
      {
          case 0 : return rectangle[0];
          case 1 : return make_point(rectangle[1].x,rectangle[0].y);
          case 2 : return make_point(rectangle[1].x,rectangle[1].y);
          case 3 : return make_point(rectangle[0].x,rectangle[1].y);
         default : return degenerate_point2d<T>();
      }
   }

   template <typename T>
   inline point3d<T> box_corner(const box<T,3>& box, const std::size_t& corner_index)
   {
      switch(corner_index)
      {
          case 0 : return box[0];
          case 1 : return make_point(box[1].x,box[0].y,box[0].z);
          case 2 : return make_point(box[1].x,box[1].y,box[0].z);
          case 3 : return make_point(box[0].x,box[1].y,box[0].z);
          case 4 : return box[1];
          case 5 : return make_point(box[1].x,box[0].y,box[1].z);
          case 6 : return make_point(box[1].x,box[1].y,box[1].z);
          case 7 : return make_point(box[0].x,box[1].y,box[1].z);
         default : return degenerate_point3d<T>();
      }
   }

   template <typename T>
   inline line<T,2> triangle_bisector(const triangle<T,2>& triangle, const std::size_t& bisector)
   {
      switch(bisector)
      {
          case 0 : return create_line_from_bisector(triangle[2],triangle[0],triangle[1]);
          case 1 : return create_line_from_bisector(triangle[0],triangle[1],triangle[2]);
          case 2 : return create_line_from_bisector(triangle[0],triangle[2],triangle[1]);
         default : return degenerate_line2d<T>();
      }
   }

   template <typename T>
   inline line<T,3> triangle_bisector(const triangle<T,3>& triangle, const std::size_t& bisector)
   {
      switch(bisector)
      {
          case 0 : return create_line_from_bisector(triangle[2],triangle[0],triangle[1]);
          case 1 : return create_line_from_bisector(triangle[0],triangle[1],triangle[2]);
          case 2 : return create_line_from_bisector(triangle[0],triangle[2],triangle[1]);
         default : return degenerate_line3d<T>();
      }
   }

   template <typename T>
   inline line<T,2> triangle_external_bisector(const triangle<T,2>& triangle,
                                               const std::size_t& corner,
                                               const std::size_t& opposing_corner)
   {
      return create_line_from_bisector(project_point_t(triangle[corner],triangle[opposing_corner],T(2.0)),
                                       triangle[opposing_corner],
                                       triangle[(opposing_corner + 1) % 3]);
   }

   template <typename T>
   inline line<T,3> triangle_external_bisector(const triangle<T,3>& triangle,
                                               const std::size_t& corner,
                                               const std::size_t& opposing_corner)
   {
      return create_line_from_bisector(project_point_t(triangle[corner],triangle[opposing_corner],T(2.0)),
                                       triangle[opposing_corner],
                                       triangle[(opposing_corner + 1) % 3]);
   }

   template <typename T>
   inline line<T,2> triangle_median(const triangle<T,2>& triangle, const std::size_t& median)
   {
      switch(median)
      {
          case 0 : return make_line(triangle[0],segment_mid_point(triangle[1],triangle[2]));
          case 1 : return make_line(triangle[1],segment_mid_point(triangle[0],triangle[2]));
          case 2 : return make_line(triangle[2],segment_mid_point(triangle[0],triangle[1]));
         default : return degenerate_line2d<T>();
      }
   }

   template <typename T>
   inline line<T,3> triangle_median(const triangle<T,3>& triangle, const std::size_t& median)
   {
      switch(median)
      {
          case 0 : return make_line(triangle[0],segment_mid_point(triangle[1],triangle[2]));
          case 1 : return make_line(triangle[1],segment_mid_point(triangle[0],triangle[2]));
          case 2 : return make_line(triangle[2],segment_mid_point(triangle[0],triangle[1]));
         default : return degenerate_line3d<T>();
      }
   }

   template <typename T>
   inline line<T,2> triangle_symmedian(const triangle<T,2>& triangle, const std::size_t& symmedian)
   {
      if (symmedian < 3)
         return mirror(triangle_median(triangle,symmedian),triangle_bisector(triangle,symmedian));
      else
         return degenerate_line2d<T>();
   }

   template <typename T>
   inline line<T,3> triangle_symmedian(const triangle<T,3>& triangle, const std::size_t& symmedian)
   {
      if (symmedian < 3)
         return mirror(triangle_median(triangle,symmedian),triangle_bisector(triangle,symmedian));
      else
         return degenerate_line3d<T>();
   }

   template <typename T>
   inline line<T,2> euler_line(const triangle<T,2>& triangle)
   {
      return make_line(centroid(triangle),orthocenter(triangle));
   }

   template <typename T>
   inline line<T,3> euler_line(const triangle<T,3>& triangle)
   {
      return make_line(centroid(triangle),orthocenter(triangle));
   }

   template <typename T>
   inline point2d<T> exmedian_point(const triangle<T,2>& triangle, const std::size_t& corner)
   {
      line<T,2> line1 = create_parallel_line_on_point(make_line(triangle[corner],triangle[(corner + 1) % 3]),triangle[(corner + 2) % 3]);
      line<T,2> line2 = create_parallel_line_on_point(make_line(triangle[corner],triangle[(corner + 2) % 3]),triangle[(corner + 1) % 3]);

      return intersection_point(line1,line2);
   }

   template <typename T>
   inline point3d<T> exmedian_point(const triangle<T,3>& triangle, const std::size_t& corner)
   {
      line<T,3> line1 = create_parallel_line_on_point(make_line(triangle[corner],triangle[(corner + 1) % 3]),triangle[(corner + 2) % 3]);
      line<T,3> line2 = create_parallel_line_on_point(make_line(triangle[corner],triangle[(corner + 2) % 3]),triangle[(corner + 1) % 3]);

      return intersection_point(line1,line2);
   }

   template <typename T>
   inline point2d<T> feuerbach_point(const triangle<T,2>& triangle)
   {
     point2d<T> ipoint1 = degenerate_point2d<T>();
     point2d<T> ipoint2 = degenerate_point2d<T>();

     intersection_point(inscribed_circle(triangle),nine_point_circle(triangle),ipoint1,ipoint2);

     return ipoint1;
   }

   template <typename T>
   inline line<T,2> confined_triangle_median(const triangle<T,2>& triangle,const point2d<T>& point, const std::size_t& median)
   {
      switch(median)
      {
         case 0 : return make_line(triangle[0],intersection_point(edge(triangle,1),make_line(triangle[0],point)));
         case 1 : return make_line(triangle[1],intersection_point(edge(triangle,2),make_line(triangle[1],point)));
         case 2 : return make_line(triangle[2],intersection_point(edge(triangle,0),make_line(triangle[2],point)));
         default: return degenerate_line2d<T>();
      }
   }

   template <typename T>
   inline line<T,3> confined_triangle_median(const triangle<T,3>& triangle,const point3d<T>& point, const std::size_t& median)
   {
      switch(median)
      {
         case 0 : return make_line(triangle[0],intersection_point(edge(triangle,1),make_line(triangle[0],point)));
         case 1 : return make_line(triangle[1],intersection_point(edge(triangle,2),make_line(triangle[1],point)));
         case 2 : return make_line(triangle[2],intersection_point(edge(triangle,0),make_line(triangle[2],point)));
         default: return degenerate_line3d<T>();
      }
   }

   template <typename T>
   inline line<T,2> create_parallel_line_on_point(const line<T,2>& line, const point2d<T>& point)
   {
      return make_line(point, point + (line[1] - line[0]));
   }

   template <typename T>
   inline line<T,3> create_parallel_line_on_point(const line<T,3>& line, const point3d<T>& point)
   {
      return make_line(point, point + (line[1] - line[0]));
   }

   template <typename T>
   inline segment<T,2> create_parallel_segment_on_point(const line<T,2>& line, const point2d<T>& point)
   {
      return make_segment(point, point + (line[1] - line[0]));
   }

   template <typename T>
   inline segment<T,3> create_parallel_segment_on_point(const line<T,3>& line, const point3d<T>& point)
   {
      return make_segment(point, point + (line[1] - line[0]));
   }

   template <typename T>
   inline bool point_in_rectangle(const T& px, const T& py,
                                  const T& x1, const T& y1,
                                  const T& x2, const T& y2)
   {
      return ((x1 <= px) && (px <= x2) && (y1 <= py) && (py <= y2)) ||
             ((x2 <= px) && (px <= x1) && (y2 <= py) && (py <= y1)) ;
   }

   template <typename T>
   inline bool point_in_rectangle(const point2d<T>& point,
                                  const T& x1, const T& y1,
                                  const T& x2, const T& y2)
   {
      return point_in_rectangle(point.x,point.y,x1,y1,x2,y2);
   }

   template <typename T>
   inline bool point_in_rectangle(const T& px, const T& py, const rectangle<T>& rectangle)
   {
      return point_in_rectangle(px,py,rectangle[0].x,rectangle[0].y,rectangle[1].x,rectangle[1].y);
   }

   template <typename T>
   inline bool point_in_rectangle(const point2d<T>& point, const rectangle<T>& rectangle)
   {
      return point_in_rectangle(point.x,point.y,rectangle);
   }

   template <typename T>
   inline bool point_in_rectangle(const point2d<T>& point, const point2d<T>& rect_point1, point2d<T>& rect_point2)
   {
      return point_in_rectangle
             (
               point.x, point.y,
               rect_point1.x, rect_point1.y, rect_point2.x, rect_point2.y
             );
   }

   template <typename T>
   inline bool point_in_rectangle(const point2d<T>& point, const segment<T,2>& segment)
   {
      return point_in_rectangle(point.x,point.y,segment[0].x,segment[0].y,segment[1].x,segment[1].y);
   }

   template <typename T>
   inline bool point_in_box(const T& px, const T& py, const T& pz,
                            const T& x1, const T& y1, const T& z1,
                            const T& x2, const T& y2, const T& z2)
   {
      return ((x1 <= px) && (px <= x2) && (y1 <= py) && (py <= y2) && (z1 <= pz) && (pz <= z2)) ||
             ((x2 <= px) && (px <= x1) && (y2 <= py) && (py <= y1) && (z2 <= pz) && (pz <= z1)) ;
   }

   template <typename T>
   inline bool point_in_box(const point3d<T>& point,
                            const T& x1, const T& y1, const T& z1,
                            const T& x2, const T& y2, const T& z2)
   {
      return point_in_box(point.x,point.y,point.z,x1,y1,z1,x2,y2,z2);
   }

   template <typename T>
   inline bool point_in_box(const T& px, const T& py, const T& pz, const box<T,3>& box)
   {
      return point_in_box(px,py,pz,box[0].x,box[0].y,box[0].z,box[1].x,box[1].y,box[1].z);
   }

   template <typename T>
   inline bool point_in_box(const point3d<T>& point, const box<T,3>& box)
   {
      return point_in_box(point.x,point.y,point.z,box);
   }

   template <typename T>
   inline bool point_in_box(const point3d<T>& point, const point3d<T>& box_point1, const point3d<T>& box_point2)
   {
      return point_in_box
             (
               point.x,      point.y,      point.z,
               box_point1.x, box_point1.y, box_point1.z,
               box_point2.x, box_point2.y, box_point2.z
             );
   }

   template <typename T>
   inline bool point_in_box(const point3d<T>& point, const segment<T,3>& segment)
   {
      return point_in_box(point,segment[0],segment[1]);
   }

   template <typename T>
   inline bool point_in_triangle(const T& px, const T& py,
                                 const T& x1, const T& y1,
                                 const T& x2, const T& y2,
                                 const T& x3, const T& y3)
   {
      const int or1 = orientation(x1, y1, x2, y2, px, py);
      const int or2 = orientation(x2, y2, x3, y3, px, py);

      if ((or1 * or2) == -1)
         return false;
      else
      {
         int or3 = orientation(x3, y3, x1, y1, px, py);
         if ((or1 == or3) || (0 == or3))
            return true;
         else if (0 == or1)
            return ((or2 * or3) >= 0);
         else if (0 == or2)
            return ((or1 * or3) >= 0);
         else
            return false;
      }
   }

   template <typename T>
   inline bool point_in_triangle(const T& px, const T& py, const triangle<T,2>& triangle)
   {
      return point_in_triangle
             (
               px,            py,
               triangle[0].x, triangle[0].y,
               triangle[1].x, triangle[1].y,
               triangle[2].x, triangle[2].y
             );
   }

   template <typename T>
   inline bool point_in_triangle(const point2d<T>& point,
                                 const point2d<T>& point1,
                                 const point2d<T>& point2,
                                 const point2d<T>& point3)
   {
      return point_in_triangle( point.x,  point.y,
                               point1.x, point1.y,
                               point2.x, point2.y,
                               point3.x, point3.y);
   }

   template <typename T>
   inline bool point_in_triangle(const point2d<T>& point, const triangle<T,2>& triangle)
   {
      return point_in_triangle
             (
               point.x,       point.y,
               triangle[0].x, triangle[0].y,
               triangle[1].x, triangle[1].y,
               triangle[2].x, triangle[2].y
             );
   }

   template <typename T>
   inline bool point_in_quadix(const T& px, const T& py,
                               const T& x1, const T& y1,
                               const T& x2, const T& y2,
                               const T& x3, const T& y3,
                               const T& x4, const T& y4)
   {
      const int or1 = orientation(x1, y1, x2, y2, px, py);
      const int or2 = orientation(x2, y2, x3, y3, px, py);
      const int or3 = orientation(x3, y3, x4, y4, px, py);
      const int or4 = orientation(x4, y4, x1, y1, px, py);

      if ((or1 == or2) && (or2 == or3) && (or3 == or4))
         return true;
      else if (0 == or1)
         return (0 == (or2 * or4));
      else if (0 == or2)
         return (0 == (or1 * or3));
      else if (0 == or3)
         return (0 == (or2 * or4));
      else if (0 == or4)
         return (0 == (or1 * or3));
      else
         return false;
   }

   template <typename T>
   inline bool point_in_quadix(const point2d<T>& point,
                               const point2d<T>& point1,
                               const point2d<T>& point2,
                               const point2d<T>& point3,
                               const point2d<T>& point4)
   {
      return point_in_quadix
             (
                point.x,  point.y,
               point1.x, point1.y,
               point2.x, point2.y,
               point3.x, point3.y,
               point4.x, point4.y
             );
   }

   template <typename T>
   inline bool point_in_quadix(const T& px, const T& py,
                               const quadix<T,2>& quadix)
   {
      return point_in_quadix
             (
               px,          py,
               quadix[0].x, quadix[0].y,
               quadix[1].x, quadix[1].y,
               quadix[2].x, quadix[2].y,
               quadix[3].x, quadix[3].y
             );
   }

   template <typename T>
   inline bool point_in_quadix(const point2d<T>&  point,
                               const quadix<T,2>& quadix)
   {
      return point_in_quadix(point.x, point.y, quadix);
   }

   template <typename T>
   inline bool point_in_circle(const T& px, const T& py, const T& cx, const T& cy, const T& radius)
   {
      return (lay_distance(px, py, cx, cy) <= (radius * radius));
   }

   template <typename T>
   inline bool point_in_circle(const T& px, const T& py, const circle<T>& circle)
   {
      return point_in_circle(px, py, circle.x, circle.y, circle.radius);
   }

   template <typename T>
   inline bool point_in_circle(const point2d<T>& point, const circle<T>& circle)
   {
      return point_in_circle(point.x, point.y, circle);
   }

   template <typename T>
   inline bool point_in_sphere(const T& px, const T& py, const T& pz, const T& cx, const T& cy, const T& cz, const T& radius)
   {
      return less_than_or_equal(lay_distance(px, py, pz, cx, cy, cz), radius * radius);
   }

   template <typename T>
   inline bool point_in_sphere(const T& px, const T& py, const T& pz, const sphere<T>& sphere)
   {
      return point_in_sphere(px,py,pz,sphere.x,sphere.y,sphere.z,sphere.radius);
   }

   template <typename T>
   inline bool point_in_sphere(const point3d<T>& point, const sphere<T>& sphere)
   {
      return point_in_sphere(point.x,point.y,point.z,sphere);
   }

   template <typename T>
   inline bool point_in_three_point_circle(const T& px, const T& py,
                                           const T& x1, const T& y1,
                                           const T& x2, const T& y2,
                                           const T& x3, const T& y3)
   {
      const T dx1 = x1 - px;
      const T dx2 = x2 - px;
      const T dx3 = x3 - px;
      const T dy1 = y2 - py;
      const T dy2 = y3 - py;
      const T dy3 = y1 - py;
      const T a11 = dx3 * dy1 - dx2 * dy2;
      const T a12 = dx3 * dy3 - dx1 * dy2;
      const T a21 = dx2 * (x2 - x3) + dy1 * (y2 - y3);
      const T a22 = dx1 * (x1 - x3) + dy3 * (y1 - y3);

      return less_than_or_equal(a11 * a22 - a21 * a12,T(0.0));
   }

   template <typename T>
   inline bool point_in_three_point_circle(const point2d<T>& point,
                                           const point2d<T>& point1,
                                           const point2d<T>& point2,
                                           const point2d<T>& point3)
   {
      return point_in_three_point_circle
             (
               point .x, point .y,
               point1.x, point1.y,
               point2.x, point2.y,
               point3.x, point3.y
             );
   }

   template <typename T>
   inline bool point_in_three_point_circle(const point2d<T>& point, const triangle<T,2> triangle)
   {
      return point_in_three_point_circle(point, triangle[0], triangle[1], triangle[2]);
   }

   template <typename T>
   inline bool point_in_focus_area(const T& px, const T& py,
                                   const T& x1, const T& y1,
                                   const T& x2, const T& y2,
                                   const T& x3, const T& y3)
   {
      return (-1 == (orientation((x2 + x3) * T(0.5),(y2 + y3) * T(0.5),x1,y1,px,py) *
                     orientation((x1 + x3) * T(0.5),(y1 + y3) * T(0.5),x2,y2,px,py)));
   }

   template <typename T>
   inline bool point_in_focus_area(const point2d<T>& point,
                                   const point2d<T>& point1,
                                   const point2d<T>& point2,
                                   const point2d<T>& point3)
   {
      return point_in_focus_area
             (
                point.x, point.y,
               point1.x,point1.y,
               point2.x,point2.y,
               point3.x,point3.y
             );
   }

   template <typename T>
   inline bool point_on_segment(const point2d<T>& point, const segment<T,2>& segment)
   {
      return is_point_collinear(segment,point,true);
   }

   template <typename T>
   inline bool point_on_segment(const point3d<T>& point, const segment<T,3>& segment)
   {
      return is_point_collinear(segment,point,true);
   }

   template <typename T>
   inline bool point_on_ray(const T& px, const T& py,
                            const T& ox, const T& oy,
                            const T& dx, const T& dy)
   {
      return point_on_ray(make_point(px,py),make_ray(ox,oy,dx,dy));
   }

   template <typename T>
   inline bool point_on_ray(const T& px, const T& py, const T& pz,
                            const T& ox, const T& oy, const T& oz,
                            const T& dx, const T& dy, const T& dz)
   {
      return point_on_ray
             (
               make_point(px, py, pz),
               make_ray(ox, oy, oz, dx, dy, dz)
             );
   }

   template <typename T>
   inline bool point_on_ray(const point2d<T>& point, const ray<T,2>& ray)
   {
      const T t = dot_product(ray.direction,point - ray.origin);

      if (greater_than_or_equal(t,T(0.0)))
      {
         return is_equal(point,generate_point_on_ray(ray,t));
      }
      else
         return false;
   }

   template <typename T>
   inline bool point_on_ray(const point3d<T>& point, const ray<T,3>& ray)
   {
      const T t = dot_product(ray.direction,point - ray.origin);

      if (greater_than_or_equal(t,T(0.0)))
      {
         return is_equal(point,generate_point_on_ray(ray,t),T(0.0));
      }
      else
         return false;
   }

   template <typename T>
   inline bool point_on_rectangle(const T& px, const T& py,
                                  const T& x1, const T& y1,
                                  const T& x2, const T& y2)
   {
      return (((x1 <= px) && (px <= x2)) && ((py == y1) || (py == y2))) ||
             (((y1 <= py) && (py <= y2)) && ((px == x1) || (px == x2))) ;
   }

   template <typename T>
   inline bool point_on_rectangle(const point2d<T>& point,
                                  const T& x1, const T& y1,
                                  const T& x2, const T& y2)
   {
      return point_on_rectangle(point.x, point.y, x1, y1, x2, y2);
   }

   template <typename T>
   inline bool point_on_rectangle(const T& px, const T& py, const rectangle<T>& rectangle)
   {
      return point_on_rectangle(px,py,rectangle[0].x,rectangle[0].y,rectangle[1].x,rectangle[1].y);
   }

   template <typename T>
   inline bool point_on_rectangle(const point2d<T>& point, const rectangle<T>& rectangle)
   {
      return point_on_rectangle(point.x,point.y,rectangle);
   }

   template <typename T>
   inline bool point_on_triangle(const T& px, const T& py,
                                 const T& x1, const T& y1,
                                 const T& x2, const T& y2,
                                 const T& x3, const T& y3)
   {
      return is_point_collinear(x1, y1, x2, y2, px, py, true) ||
             is_point_collinear(x2, y2, x3, y3, px, py, true) ||
             is_point_collinear(x3, y3, x1, y1, px, py, true) ;
   }

   template <typename T>
   inline bool point_on_triangle(const T& px, const T& py, const triangle<T,2>& triangle)
   {
      return point_on_triangle
             (
               px,            py,
               triangle[0].x, triangle[0].y,
               triangle[1].x, triangle[1].y,
               triangle[2].x, triangle[2].y
             );
   }

   template <typename T>
   inline bool point_on_triangle(const point2d<T>& point,
                                 const point2d<T>& point1,
                                 const point2d<T>& point2,
                                 const point2d<T>& point3)
   {
      return point_on_triangle
             (
                point.x,  point.y,
               point1.x, point1.y,
               point2.x, point2.y,
               point3.x, point3.y
             );
   }

   template <typename T>
   inline bool point_on_triangle(const point2d<T>& point, const triangle<T,2>& triangle)
   {
      return point_on_triangle(      point.x,       point.y,
                               triangle[0].x, triangle[0].y,
                               triangle[1].x, triangle[1].y,
                               triangle[2].x, triangle[2].y);
   }

   template <typename T>
   inline bool point_on_quadix(const T& px, const T& py,
                               const T& x1, const T& y1,
                               const T& x2, const T& y2,
                               const T& x3, const T& y3,
                               const T& x4, const T& y4)
   {
      return is_point_collinear(x1, y1, x2, y2, px, py, true) ||
             is_point_collinear(x2, y2, x3, y3, px, py, true) ||
             is_point_collinear(x3, y3, x4, y4, px, py, true) ||
             is_point_collinear(x4, y4, x1, y1, px, py, true) ;
   }

   template <typename T>
   inline bool point_on_quadix(const point2d<T>& point,
                               const point2d<T>& point1,
                               const point2d<T>& point2,
                               const point2d<T>& point3,
                               const point2d<T>& point4)
   {
      return point_on_quadix( point.x,  point.y,
                             point1.x, point1.y,
                             point2.x, point2.y,
                             point3.x, point3.y,
                             point4.x, point4.y);
   }

   template <typename T>
   inline bool point_on_quadix(const T& px, const T& py,
                               const quadix<T,2>& quadix)
   {
      return point_on_quadix(         px,          py,
                             quadix[0].x, quadix[0].y,
                             quadix[1].x, quadix[1].y,
                             quadix[2].x, quadix[2].y,
                             quadix[3].x, quadix[3].y);
   }

   template <typename T>
   inline bool point_on_quadix(const point2d<T>&  point,
                               const quadix<T,2>& quadix)
   {
      return point_on_quadix(point.x,point.y,quadix);
   }

   template <typename T>
   inline bool point_on_circle(const T& px, const T& py, const T& cx, const T& cy, const T& radius)
   {
      return (lay_distance(px,py,cx,cy) == (radius * radius));
   }

   template <typename T>
   inline bool point_on_circle(const T& px, const T& py, const circle<T>& circle)
   {
      return point_on_circle(px,py,circle.x,circle.y,circle.radius);
   }

   template <typename T>
   inline bool point_on_circle(const point2d<T>& point, const circle<T>& circle)
   {
      return point_on_circle(point.x,point.y,circle);
   }

   template <typename T>
   inline bool point_on_bezier(const point2d<T>& point, const quadratic_bezier<T,2>& bezier, const std::size_t& steps, const T& fuzzy)
   {
      return (is_equal(distance(closest_point_on_bezier_from_point(bezier,point,steps),point),T(0.0),fuzzy));
   }

   template <typename T>
   inline bool point_on_bezier(const point2d<T>& point, const cubic_bezier<T,2>& bezier, const std::size_t& steps, const T& fuzzy)
   {
      return (is_equal(distance(closest_point_on_bezier_from_point(bezier,point,steps),point),T(0.0),fuzzy));
   }

   template <typename T>
   inline bool point_on_bezier(const point3d<T>& point, const quadratic_bezier<T,3>& bezier, const std::size_t& steps, const T& fuzzy)
   {
      return (is_equal(distance(closest_point_on_bezier_from_point(bezier,point,steps),point),T(0.0),fuzzy));
   }

   template <typename T>
   inline bool point_on_bezier(const point3d<T>& point, const cubic_bezier<T,3>& bezier, const std::size_t& steps, const T& fuzzy)
   {
      return (is_equal(distance(closest_point_on_bezier_from_point(bezier,point,steps),point),T(0.0),fuzzy));
   }

   template <typename T>
   inline point2d<T> isogonal_conjugate(const point2d<T>& point, const triangle<T,2>& triangle)
   {
      return intersection_point
             (
               mirror(make_line(triangle[0], point),triangle_median(triangle, 0)),
               mirror(make_line(triangle[1], point),triangle_median(triangle, 1))
             );
   }

   template <typename T>
   inline point3d<T> isogonal_conjugate(const point3d<T>& point, const triangle<T,3>& triangle)
   {
      return intersection_point
             (
               mirror(make_line(triangle[0], point), triangle_median(triangle, 0)),
               mirror(make_line(triangle[1], point), triangle_median(triangle, 1))
             );
   }

   template <typename T>
   inline point2d<T> cyclocevian_conjugate(const point2d<T>& point, const triangle<T,2>& triangle)
   {
      point2d<T> a_prime = intersection_point(make_line(triangle[0],point),make_line(edge(triangle,1)));
      point2d<T> b_prime = intersection_point(make_line(triangle[1],point),make_line(edge(triangle,2)));
      point2d<T> c_prime = intersection_point(make_line(triangle[2],point),make_line(edge(triangle,0)));

      circle<T> circle = circumcircle(a_prime,b_prime,c_prime);

      std::vector< point2d<T> > point_set1;
      std::vector< point2d<T> > point_set2;

      intersection_point(edge(triangle,1),circle,std::back_inserter(point_set1));
      intersection_point(edge(triangle,2),circle,std::back_inserter(point_set2));

      line<T,2> line_prime_a = degenerate_line2d<T>();
      line<T,2> line_prime_b = degenerate_line2d<T>();

      if (point_set1.size() == 2)
      {
         if (is_equal(point_set1[0],a_prime))
            line_prime_a = make_line(triangle[0],point_set1[1]);
         else
            line_prime_a = make_line(triangle[0],point_set1[0]);
      }
      else if (point_set1.size() == 1)
         line_prime_a = make_line(triangle[0],point_set1[0]);

      if (point_set2.size() == 2)
      {
         if (is_equal(point_set2[0],b_prime))
            line_prime_b = make_line(triangle[0],point_set2[1]);
         else
            line_prime_b = make_line(triangle[0],point_set2[0]);
      }
      else if (point_set2.size() == 1)
         line_prime_b = make_line(triangle[0],point_set2[0]);

      return intersection_point(line_prime_a,line_prime_b);
   }

   template <typename T>
   inline point2d<T> symmedian_point(const triangle<T,2>& triangle)
   {
      return isogonal_conjugate(centroid(triangle),triangle);
   }

   template <typename T>
   inline point3d<T> symmedian_point(const triangle<T,3>& triangle)
   {
      return isogonal_conjugate(centroid(triangle),triangle);
   }

   template <typename T>
   inline void create_equilateral_triangle(const T& x1, const T& y1,
                                           const T& x2, const T& y2,
                                                 T& x3,       T& y3)
   {
      const T sin60 = T(0.86602540378443864676372317075294);
      const T cos60 = T(0.50000000000000000000000000000000);

      /* translate for x1,y1 to be origin */
      const T tx = x2 - x1;
      const T ty = y2 - y1;

      /* rotate 60 degrees and translate back */
      x3 = ((tx * cos60) - (ty * sin60)) + x1;
      y3 = ((ty * cos60) + (tx * sin60)) + y1;
   }

   template <typename T>
   inline void create_equilateral_triangle(const point2d<T>& point1,
                                           const point2d<T>& point2,
                                                 point2d<T>& point3)
   {
      return create_equilateral_triangle
             (
               point1.x, point1.y,
               point2.x, point2.y,
               point3.x, point3.y
             );
   }

   template <typename T>
   inline triangle<T,2> create_equilateral_triangle(const T& x1, const T& y1,
                                                    const T& x2, const T& y2)
   {
      triangle<T,2> triangle_;

      triangle_[0].x = x1;
      triangle_[0].y = y1;
      triangle_[1].x = x2;
      triangle_[1].y = y2;

      create_equilateral_triangle(x1, y1, x2, y2, triangle_[2].x, triangle_[2].y);

      return triangle_;
   }

   template <typename T>
   inline triangle<T,2> create_equilateral_triangle(const point2d<T>& point1,
                                                    const point2d<T>& point2)
   {
      return create_equilateral_triangle(point1.x,point1.y,point2.x,point2.y);
   }

   template <typename T>
   inline triangle<T,2> create_equilateral_triangle(const T& cx, const T& cy, const T& side_length)
   {
      return center_at_location
             (
               create_equilateral_triangle(-side_length * T(0.5), T(0.0), side_length * T(0.5), T(0.0)),
               cx, cy
             );
   }

   template <typename T>
   inline triangle<T,2> create_equilateral_triangle(const point2d<T>& center_point, const T& side_length)
   {
      return create_equilateral_triangle(center_point.x,center_point.y,side_length);
   }

   template <typename T>
   inline triangle<T,2> create_isosceles_triangle(const point2d<T>& point1, const point2d<T>& point2, const T& angle)
   {
      return create_triangle(point1,point2,angle,angle);
   }

   template <typename T>
   inline triangle<T,2> create_isosceles_triangle(const segment<T,2>& segment, const T& angle)
   {
      return create_isosceles_triangle(segment[0],segment[1],angle);
   }

   template <typename T>
   inline triangle<T,2> create_triangle(const point2d<T>& point1, const point2d<T>& point2, const T& angle1, const T& angle2)
   {
      if (greater_than_or_equal(angle1 + angle2, T(180.0)))
      {
         return degenerate_triangle2d<T>();
      }

      const T bearing = cartesian_angle(point2.x - point1.x,point2.y - point1.y);

      T theta_a = T(0.0);
      T theta_b = T(0.0);

      switch(quadrant(bearing))
      {
         case 1 : {
                     theta_a = normalize_angle(bearing - angle1);
                     theta_b = normalize_angle(T(180.0) + bearing + angle2);
                  }
                  break;

         case 2 : {
                     theta_a = normalize_angle(bearing - angle1);
                     theta_b = normalize_angle(T(180.0) + bearing + angle2);
                  }
                  break;

         case 3 : {
                     theta_a = normalize_angle(bearing - angle1);
                     theta_b = normalize_angle(bearing - T(180.0) + angle2);
                  }
                  break;

         case 4 : {
                     theta_a = normalize_angle(bearing - angle1);
                     theta_b = normalize_angle(T(180.0) - (T(360.0) - bearing) + angle2);
                  }
                  break;
      }

      const ray<T,2> ray1 = make_ray(point1,theta_a);
      const ray<T,2> ray2 = make_ray(point2,theta_b);

      return make_triangle
             (
               intersection_point(ray1,ray2),
               point1,
               point2
             );
   }

   template <typename T>
   inline triangle<T,2> create_triangle(const segment<T,2>& segment, const T& angle1, const T& angle2)
   {
      return create_triangle(segment[0],segment[1],angle1,angle2);
   }

   template <typename T>
   inline triangle<T,2> create_morley_triangle(const triangle<T,2>& triangle)
   {
      wykobi::triangle<T,2> tri = triangle;

      if (orientation(tri[0],tri[1],tri[2]) == LeftHandSide)
      {
        swap(tri[0],tri[1]);
      }

      const T angle1 = vertex_angle<T>(tri[2], tri[0], tri[1]) * T(1.0 / 3.0);
      const T angle2 = vertex_angle<T>(tri[0], tri[1], tri[2]) * T(1.0 / 3.0);
      const T angle3 = vertex_angle<T>(tri[1], tri[2], tri[0]) * T(1.0 / 3.0);

      const wykobi::triangle<T,2> triangle1 = create_triangle(edge(tri, 0), angle1, angle2);
      const wykobi::triangle<T,2> triangle2 = create_triangle(edge(tri, 1), angle2, angle3);
      const wykobi::triangle<T,2> triangle3 = create_triangle(edge(tri, 2), angle3, angle1);

      return make_triangle(triangle1[0], triangle2[0], triangle3[0]);
   }

   template <typename T>
   inline triangle<T,2> create_cevian_triangle(const triangle<T,2>& triangle, const point2d<T>& point)
   {
      return make_triangle
            (
              intersection_point(make_segment(triangle[0], point), edge(triangle, 1)),
              intersection_point(make_segment(triangle[1], point), edge(triangle, 2)),
              intersection_point(make_segment(triangle[2], point), edge(triangle, 0))
            );
   }

   template <typename T>
   inline triangle<T,3> create_cevian_triangle(const triangle<T,3>& triangle, const point3d<T>& point)
   {
      return make_triangle
             (
               intersection_point(make_segment(triangle[0], point), edge(triangle, 1)),
               intersection_point(make_segment(triangle[1], point), edge(triangle, 2)),
               intersection_point(make_segment(triangle[2], point), edge(triangle, 0))
             );
   }

   template <typename T>
   inline triangle<T,2> create_anticevian_triangle(const triangle<T,2>& triangle, const point2d<T>& point)
   {
      std::vector< point2d<T> > point_list(3,degenerate_point2d<T>());

      for (std::size_t i = 0; i < 3; ++i)
      {
         typedef const wykobi::line<T,2> line2d_t;

         line2d_t opp_edge    = wykobi::make_line(opposing_edge(triangle,i));
         line2d_t cevian_edge = wykobi::make_line(triangle[i],point);
         line2d_t orthic_edge = wykobi::make_line
                                        (
                                          triangle[i],
                                          wykobi::closest_point_on_line_from_point(opp_edge,triangle[i])
                                        );

         //if (!robust_collinear(orthic_edge,point))
         {
            point_list[i] = wykobi::intersection_point(cevian_edge, make_line(orthic_edge[i], wykobi::mirror(point,opp_edge)));
         }
         //else
         //   point_list[i] = triangle[i];
      }

      /* Note: Buggy - DO NOT USE !*/
      return make_triangle(point_list[0], point_list[1], point_list[2]);
   }

   template <typename T>
   inline triangle<T,3> create_anticevian_triangle(const triangle<T,3>& triangle, const point3d<T>& point)
   {
      return make_triangle
             (
               intersection_point
               (
                 make_line(triangle[0],point),
                 make_line
                 (
                   closest_point_on_line_from_point(make_line(opposing_edge(triangle,0)),triangle[0]),
                   mirror(point,make_line(opposing_edge(triangle,0)))
                 )
               ),
               intersection_point
               (
                 make_line(triangle[1],point),
                 make_line
                 (
                   closest_point_on_line_from_point(make_line(opposing_edge(triangle,1)),triangle[1]),
                   mirror(point,make_line(opposing_edge(triangle,1)))
                 )
               ),
               intersection_point
               (
                 make_line(triangle[2],point),
                 make_line
                 (
                   closest_point_on_line_from_point(make_line(opposing_edge(triangle,2)),triangle[2]),
                   mirror(point,make_line(opposing_edge(triangle,2)))
                 )
               )
             );
   }

   template <typename T>
   inline triangle<T,2> create_anticomplementary_triangle(const triangle<T,2>& triangle)
   {
      return make_triangle
             (
               exmedian_point(triangle,0),
               exmedian_point(triangle,1),
               exmedian_point(triangle,2)
             );
   }

   template <typename T>
   inline triangle<T,3> create_anticomplementary_triangle(const triangle<T,3>& triangle)
   {
      return make_triangle
             (
               exmedian_point(triangle,0),
               exmedian_point(triangle,1),
               exmedian_point(triangle,2)
             );
   }

   template <typename T>
   inline triangle<T,2> create_inner_napoleon_triangle(const triangle<T,2>& triangle)
   {
      if (orientation(triangle) == RightHandSide)
      {
         return make_triangle
                (
                  centroid(create_equilateral_triangle(triangle[1], triangle[0])),
                  centroid(create_equilateral_triangle(triangle[2], triangle[1])),
                  centroid(create_equilateral_triangle(triangle[0], triangle[2]))
                );
      }
      else
      {
         return make_triangle
                (
                  centroid(create_equilateral_triangle(triangle[0], triangle[1])),
                  centroid(create_equilateral_triangle(triangle[1], triangle[2])),
                  centroid(create_equilateral_triangle(triangle[2], triangle[0]))
                );
      }
   }

   template <typename T>
   inline triangle<T,2> create_outer_napoleon_triangle(const triangle<T,2>& triangle)
   {
      if (orientation(triangle) == RightHandSide)
      {
         return make_triangle
                (
                  centroid(create_equilateral_triangle(triangle[0], triangle[1])),
                  centroid(create_equilateral_triangle(triangle[1], triangle[2])),
                  centroid(create_equilateral_triangle(triangle[2], triangle[0]))
                );
      }
      else
      {
         return make_triangle
                (
                  centroid(create_equilateral_triangle(triangle[1], triangle[0])),
                  centroid(create_equilateral_triangle(triangle[2], triangle[1])),
                  centroid(create_equilateral_triangle(triangle[0], triangle[2]))
                );
      }
   }

   template <typename T>
   inline triangle<T,2> create_inner_vecten_triangle(const triangle<T,2>& triangle)
   {
      if (orientation(triangle) == RightHandSide)
      {
         return make_triangle
                (
                  centroid(create_equilateral_quadix(reverse_segment(edge(triangle, 0)))),
                  centroid(create_equilateral_quadix(reverse_segment(edge(triangle, 1)))),
                  centroid(create_equilateral_quadix(reverse_segment(edge(triangle, 2))))
                );
      }
      else
      {
         return make_triangle
                (
                  centroid(create_equilateral_quadix(edge(triangle, 0))),
                  centroid(create_equilateral_quadix(edge(triangle, 1))),
                  centroid(create_equilateral_quadix(edge(triangle, 2)))
                );
      }
   }

   template <typename T>
   inline triangle<T,2> create_outer_vecten_triangle(const triangle<T,2>& triangle)
   {
      if (orientation(triangle) == RightHandSide)
      {
         return make_triangle
                (
                  centroid(create_equilateral_quadix(edge(triangle, 0))),
                  centroid(create_equilateral_quadix(edge(triangle, 1))),
                  centroid(create_equilateral_quadix(edge(triangle, 2)))
                );
      }
      else
      {
         return make_triangle
                (
                  centroid(create_equilateral_quadix(reverse_segment(edge(triangle, 0)))),
                  centroid(create_equilateral_quadix(reverse_segment(edge(triangle, 1)))),
                  centroid(create_equilateral_quadix(reverse_segment(edge(triangle, 2))))
                );
      }
   }

   template <typename T>
   inline triangle<T,2> create_medial_triangle(const triangle<T,2>& triangle)
   {
      return make_triangle
             (
               segment_mid_point(triangle[0], triangle[1]),
               segment_mid_point(triangle[1], triangle[2]),
               segment_mid_point(triangle[2], triangle[0])
             );
   }

   template <typename T>
   inline triangle<T,3> create_medial_triangle(const triangle<T,3>& triangle)
   {
      return make_triangle
             (
               segment_mid_point(triangle[0], triangle[1]),
               segment_mid_point(triangle[1], triangle[2]),
               segment_mid_point(triangle[2], triangle[0])
             );
   }

   template <typename T>
   inline triangle<T,2> create_contact_triangle(const triangle<T,2>& triangle)
   {
      point2d<T> center = incenter(triangle);

      return make_triangle
             (
               closest_point_on_line_from_point(make_line(edge(triangle, 0)), center),
               closest_point_on_line_from_point(make_line(edge(triangle, 1)), center),
               closest_point_on_line_from_point(make_line(edge(triangle, 2)), center)
             );
   }

   template <typename T>
   inline triangle<T,3> create_contact_triangle(const triangle<T,3>& triangle)
   {
      point3d<T> center = incenter(triangle);

      return make_triangle
             (
               closest_point_on_line_from_point(make_line(edge(triangle, 0)),center),
               closest_point_on_line_from_point(make_line(edge(triangle, 1)),center),
               closest_point_on_line_from_point(make_line(edge(triangle, 2)),center)
             );
   }

   template <typename T>
   inline triangle<T,2> create_symmedial_triangle(const triangle<T,2>& triangle, const point2d<T>& point)
   {
      return make_triangle
             (
               intersection_point(make_line(triangle[0], point),make_line(edge(triangle, 1))),
               intersection_point(make_line(triangle[1], point),make_line(edge(triangle, 2))),
               intersection_point(make_line(triangle[2], point),make_line(edge(triangle, 0)))
             );
   }

   template <typename T>
   inline triangle<T,2> create_orthic_triangle(const triangle<T,2>& triangle)
   {
      return make_triangle
            (
              closest_point_on_line_from_point(make_line(edge(triangle, 0)),triangle[2]),
              closest_point_on_line_from_point(make_line(edge(triangle, 1)),triangle[0]),
              closest_point_on_line_from_point(make_line(edge(triangle, 2)),triangle[1])
            );
   }

   template <typename T>
   inline triangle<T,3> create_orthic_triangle(const triangle<T,3>& triangle)
   {
      return make_triangle
             (
               closest_point_on_line_from_point(make_line(edge(triangle, 0)), triangle[2]),
               closest_point_on_line_from_point(make_line(edge(triangle, 1)), triangle[0]),
               closest_point_on_line_from_point(make_line(edge(triangle, 2)), triangle[1])
             );
   }

   template <typename T>
   inline triangle<T,2> create_pedal_triangle(const point2d<T>& point, const triangle<T,2>& triangle)
   {
      return make_triangle
             (
               closest_point_on_line_from_point(make_line(edge(triangle, 0)), point),
               closest_point_on_line_from_point(make_line(edge(triangle, 1)), point),
               closest_point_on_line_from_point(make_line(edge(triangle, 2)), point)
             );
   }

   template <typename T>
   inline triangle<T,3> create_pedal_triangle(const point3d<T>& point, const triangle<T,3>& triangle)
   {
      return make_triangle
             (
               closest_point_on_line_from_point(make_line(edge(triangle, 0)), point),
               closest_point_on_line_from_point(make_line(edge(triangle, 1)), point),
               closest_point_on_line_from_point(make_line(edge(triangle, 2)), point)
             );
   }

   template <typename T>
   inline triangle<T,2> create_antipedal_triangle(const point2d<T>& point, const triangle<T,2>& triangle)
   {
      typedef const line<T,2> line_t;

      line_t line1 = create_perpendicular_line_at_end_point(make_line(point,closest_point_on_line_from_point(make_line(edge(triangle,0)),point)));
      line_t line2 = create_perpendicular_line_at_end_point(make_line(point,closest_point_on_line_from_point(make_line(edge(triangle,1)),point)));
      line_t line3 = create_perpendicular_line_at_end_point(make_line(point,closest_point_on_line_from_point(make_line(edge(triangle,2)),point)));

      return make_triangle
             (
               intersection_point(line1, line2),
               intersection_point(line1, line3),
               intersection_point(line2, line3)
             );
   }

   template <typename T>
   inline triangle<T,2> create_excentral_triangle(const triangle<T,2>& triangle)
   {
      return make_triangle(excenter(triangle,0),excenter(triangle,1),excenter(triangle,2));
   }

   template <typename T>
   inline triangle<T,3> create_excentral_triangle(const triangle<T,3>& triangle)
   {
      return make_triangle(excenter(triangle,0),excenter(triangle,1),excenter(triangle,2));
   }

   template <typename T>
   inline triangle<T,2> create_incentral_triangle(const triangle<T,2>& triangle)
   {
      return create_cevian_triangle(triangle,incenter(triangle));
   }

   template <typename T>
   inline triangle<T,3> create_incentral_triangle(const triangle<T,3>& triangle)
   {
      return create_cevian_triangle(triangle,incenter(triangle));
   }

   template <typename T>
   inline triangle<T,2> create_intouch_triangle(const triangle<T,2>& triangle)
   {
      const point2d<T> p = make_point(wykobi::inscribed_circle(triangle));

      return make_triangle
             (
               closest_point_on_segment_from_point(opposing_edge(triangle,0), p),
               closest_point_on_segment_from_point(opposing_edge(triangle,1), p),
               closest_point_on_segment_from_point(opposing_edge(triangle,2), p)
             );
   }

   template <typename T>
   inline triangle<T,2> create_extouch_triangle(const triangle<T,2>& triangle)
   {
      wykobi::triangle<T,2> triangle_ = create_excentral_triangle(triangle);

      triangle_[0] = closest_point_on_segment_from_point(opposing_edge(triangle,0), triangle_[0]);
      triangle_[1] = closest_point_on_segment_from_point(opposing_edge(triangle,1), triangle_[1]);
      triangle_[2] = closest_point_on_segment_from_point(opposing_edge(triangle,2), triangle_[2]);

      return triangle_;
   }

   template <typename T>
   inline triangle<T,3> create_extouch_triangle(const triangle<T,3>& triangle)
   {
      wykobi::triangle<T,3> triangle_ = create_excentral_triangle(triangle);

      triangle_[0] = closest_point_on_segment_from_point(opposing_edge(triangle,0), triangle_[0]);
      triangle_[1] = closest_point_on_segment_from_point(opposing_edge(triangle,1), triangle_[1]);
      triangle_[2] = closest_point_on_segment_from_point(opposing_edge(triangle,2), triangle_[2]);

      return triangle_;
   }

   template <typename T>
   inline triangle<T,2> create_feuerbach_triangle(const triangle<T,2>& triangle)
   {
      circle<T> circle = nine_point_circle(triangle);

      return make_triangle
             (
               project_point(make_point(circle), make_point(excircle(triangle, 0)), circle.radius),
               project_point(make_point(circle), make_point(excircle(triangle, 1)), circle.radius),
               project_point(make_point(circle), make_point(excircle(triangle, 2)), circle.radius)
             );
   }

   template <typename T>
   inline triangle<T,2> create_circumcevian_triangle(const triangle<T,2>& triangle, const point2d<T>& point)
   {
      circle<T> circum_circle = make_circle(triangle);

      wykobi::triangle<T,2> triangle_ = degenerate_triangle2d<T>();

      std::vector< point2d<T> > ipoint;

      intersection_point
      (
        make_line(triangle[0], point),
        circum_circle,
        std::back_inserter(ipoint)
      );

      if (ipoint.empty() || (2 != ipoint.size())) return triangle_;

      triangle_[0] = (is_equal(ipoint[0],triangle[0])) ? ipoint[1] : ipoint[0];

      ipoint.clear();

      intersection_point
      (
        make_line(triangle[1], point),
        circum_circle,
        std::back_inserter(ipoint)
      );

      if (ipoint.empty() || (2 != ipoint.size())) return triangle_;

      triangle_[1] = (is_equal(ipoint[0],triangle[1])) ? ipoint[1] : ipoint[0];

      ipoint.clear();

      intersection_point
      (
        make_line(triangle[2], point),
        circum_circle,
        std::back_inserter(ipoint)
      );

      if (ipoint.empty() || (2 != ipoint.size())) return triangle_;

      triangle_[2] = (is_equal(ipoint[0],triangle[2])) ? ipoint[1] : ipoint[0];

      return triangle_;
   }

   template <typename T>
   inline triangle<T,2> create_circummedial_triangle(const triangle<T,2>& triangle)
   {
      circle<T> circum_circle = make_circle(triangle);

      wykobi::triangle<T,2> triangle_ = degenerate_triangle2d<T>();

      point2d<T> point_ = centroid(triangle);

      std::vector< point2d<T> > ipoint;

      intersection_point
      (
        make_line(triangle[0], point_),
        circum_circle,
        std::back_inserter(ipoint)
      );

      if (ipoint.empty() || (2 != ipoint.size()))
         return triangle_;

      triangle_[0] = (is_equal(ipoint[0],triangle[0])) ? ipoint[1] : ipoint[0];

      ipoint.clear();

      intersection_point
      (
        make_line(triangle[1], point_),
        circum_circle,
        std::back_inserter(ipoint)
      );

      if (ipoint.empty() || (2 != ipoint.size()))
         return triangle_;

      triangle_[1] = (is_equal(ipoint[0], triangle[1])) ? ipoint[1] : ipoint[0];

      ipoint.clear();

      intersection_point
      (
        make_line(triangle[2], point_),
        circum_circle,
        std::back_inserter(ipoint)
      );

      if (ipoint.empty() || (2 != ipoint.size()))
         return triangle_;

      triangle_[2] = (is_equal(ipoint[0],triangle[2])) ? ipoint[1] : ipoint[0];

      return triangle_;
   }

   template <typename T>
   inline triangle<T,2> create_first_brocard_triangle(const triangle<T,2>& triangle)
   {
      point2d<T> o = circumcenter(triangle);
      point2d<T> k = symmedian_point(triangle);
      circle<T> circle = make_circle(o,k);

      point2d<T> closest_point;

      wykobi::triangle<T,2> result;

      std::vector< point2d<T> > int_pts;

      for (std::size_t i = 0; i < 3; ++i)
      {
         closest_point = closest_point_on_segment_from_point(edge(triangle, i), o);

         intersection_point
         (
           make_segment(o, closest_point),
           circle,
           std::back_inserter(int_pts)
         );

         result[i] = (is_equal(o,int_pts[0]) ? int_pts[1] : int_pts[2]);

         int_pts.resize(0);
      }

      return result;
   }

   template <typename T> inline void create_right_triangle(const wykobi::point2d<T>& p1, const wykobi::point2d<T>& p2,
                                                                 wykobi::point2d<T>& c1,       wykobi::point2d<T>& c2)
   {
      const T distance = wykobi::distance(p1,p2);

      const wykobi::point2d <T> mid = wykobi::segment_mid_point(p1,p2);
      const wykobi::vector2d<T>   v = wykobi::normalize(wykobi::perpendicular(p1 - p2)) * (distance / T(2.0));

      c1 = mid + v;
      c2 = mid - v;
   }

   template <typename T>
   inline void create_equilateral_quadix(const T& x1, const T& y1,
                                         const T& x2, const T& y2,
                                               T& x3,       T& y3,
                                               T& x4,       T& y4)
   {
      const T tx = x2 - x1;
      const T ty = y2 - y1;

      x4 = x1 - ty;
      y4 = y1 + tx;
      x3 = x2 - ty;
      y3 = y2 + tx;
   }

   template <typename T>
   inline void create_equilateral_quadix(const point2d<T>& point1,
                                         const point2d<T>& point2,
                                               point2d<T>& point3,
                                               point2d<T>& point4)
   {
      return create_equilateral_quadix
             (
               point1.x,point1.y,
               point2.x,point2.y,
               point3.x,point3.y,
               point4.x,point4.y
             );
   }

   template <typename T>
   inline quadix<T,2> create_equilateral_quadix(const T& x1, const T& y1,
                                                const T& x2, const T& y2)
   {
      quadix<T,2> quadix_;

      quadix_[0].x = x1;
      quadix_[0].y = y1;
      quadix_[1].x = x2;
      quadix_[1].y = y2;

      create_equilateral_quadix(x1, y1, x2, y2, quadix_[2].x, quadix_[2].y, quadix_[3].x, quadix_[3].y);

      return quadix_;
   }

   template <typename T>
   inline quadix<T,2> create_equilateral_quadix(const point2d<T>& point1,
                                                const point2d<T>& point2)
   {
      return create_equilateral_quadix(point1.x, point1.y, point2.x, point2.y);
   }

   template <typename T>
   inline quadix<T,2> create_equilateral_quadix(const segment<T,2>& segment)
   {
      return create_equilateral_quadix(segment[0],segment[1]);
   }

   template <typename T>
   inline quadix<T,2> create_equilateral_quadix(const T& cx, const T& cy, const T& side_length)
   {
      return center_at_location
             (
               create_equilateral_quadix(-side_length * T(0.5), T(0.0), side_length * T(0.5), T(0.0)),
               cx, cy
             );
   }

   template <typename T>
   inline quadix<T,2> create_equilateral_quadix(const point2d<T>& center_point, const T& side_length)
   {
      return create_equilateral_quadix(center_point.x, center_point.y, side_length);
   }

   template <typename T>
   inline void torricelli_point(const T& x1, const T& y1,
                                const T& x2, const T& y2,
                                const T& x3, const T& y3,
                                      T& px,       T& py)
   {
      /*
         Proven by Bonaventura Francesco Cavalieri in the book
         "Exercitationes geometricae sex" 1647. The theory goes,
         if the triangle has an angle of 120 degrees or more the
         toricelli point lies at the vertex of the large angle.
         Otherwise the point at which the simpson lines intersect
         is said to be the optimal solution.
         To find an intersection in 2D, all that is needed is 2
         lines (segments), hence not all three of the simpson
         lines are calculated.
      */
      if (greater_than_or_equal(vertex_angle(x1,y1,x2,y2,x3,y3),T(120.0)))
      {
         px = x2;
         py = y2;

         return;
      }
      else if (greater_than_or_equal(vertex_angle(x3,y3,x1,y1,x2,y2),T(120.0)))
      {
         px = x1;
         py = y1;

         return;
      }
      else if (greater_than_or_equal(vertex_angle(x2,y2,x3,y3,x1,y1),T(120.0)))
      {
         px = x3;
         py = y3;

         return;
      }
      else
      {
         T oetx1 = T(0.0);
         T oety1 = T(0.0);
         T oetx2 = T(0.0);
         T oety2 = T(0.0);

         if (orientation(x1,y1,x2,y2,x3,y3) == RightHandSide)
         {
            create_equilateral_triangle(x1, y1, x2, y2, oetx1, oety1);
            create_equilateral_triangle(x2, y2, x3, y3, oetx2, oety2);
         }
         else
         {
            create_equilateral_triangle(x2, y2, x1, y1, oetx1, oety1);
            create_equilateral_triangle(x3, y3, x2, y2, oetx2, oety2);
         }

         intersection_point(oetx1,oety1,x3,y3,oetx2,oety2,x1,y1,px,py);
      }
   }

   template <typename T>
   inline point2d<T> torricelli_point(const point2d<T>& point1,
                                      const point2d<T>& point2,
                                      const point2d<T>& point3)
   {
      point2d<T> point_;

      torricelli_point
      (
        point1.x,point1.y,
        point2.x,point2.y,
        point3.x,point3.y,
        point_.x,point_.y
      );

      return point_;
   }

   template <typename T>
   inline point2d<T> torricelli_point(const triangle<T,2>& triangle)
   {
      return torricelli_point(triangle[0],triangle[1],triangle[2]);
   }

   template <typename T>
   inline bool trilateration(const T& c0x, const T& c0y, const T& c0r,
                             const T& c1x, const T& c1y, const T& c1r,
                             const T& c2x, const T& c2y, const T& c2r,
                                   T&  px,       T&  py)
   {

      if (
           (lay_distance(c0x, c0y, c1x, c1y) > sqr(c0r + c1r)) ||
           (lay_distance(c0x, c0y, c2x, c2y) > sqr(c0r + c2r)) ||
           (lay_distance(c1x, c1y, c2x, c2y) > sqr(c1r + c2r))
         )
         return false;

      const T eqn0 = T(2.0) * (c1x - c0x);
      const T eqn1 = T(2.0) * (c1y - c0y);
      const T eqn2 = sqr(c0r) - sqr(c1r) - sqr(c0x) + sqr(c1x) - sqr(c0y) + sqr(c1y);
      const T eqn3 = T(2.0) * (c2x - c1x);
      const T eqn4 = T(2.0) * (c2y - c1y);
      const T eqn5 = sqr(c1r) - sqr(c2r) - sqr(c1x) + sqr(c2x) - sqr(c1y) + sqr(c2y);

      const T det = (eqn0 * eqn4) - (eqn1 * eqn3);

      if (is_equal(det,T(0.0)))
      {
         return false;
      }

      px = ((eqn2 * eqn4) - (eqn5 * eqn1)) / det;
      py = ((eqn0 * eqn5) - (eqn2 * eqn3)) / det;

      return true;
   }

   template <typename T>
   inline point2d<T> trilateration(const circle<T>& c0, const circle<T>& c1, const circle<T>& c2)
   {
      point2d<T> p;

      const bool result = trilateration
                          (
                            c0.x, c0.y, c0.radius,
                            c1.x, c1.y, c1.radius,
                            c2.x, c2.y, c2.radius,
                            p.x,   p.y
                          );

      return result ? p : degenerate_point2d<T>();
   }

   template <typename T>
   inline void incenter(const T& x1, const T& y1,
                        const T& x2, const T& y2,
                        const T& x3, const T& y3,
                              T& px,       T& py)
   {
      /* Using Heron's s=ur */
      const T side12 = distance(x1,y1,x2,y2);
      const T side23 = distance(x2,y2,x3,y3);
      const T side31 = distance(x3,y3,x1,y1);
      const T perim  = T(1.0) / (side12 + side23 + side31);

      px = (side23 * x1 + side31 * x2 + side12 * x3) * perim;
      py = (side23 * y1 + side31 * y2 + side12 * y3) * perim;
   }

   template <typename T>
   inline void incenter(const T& x1, const T& y1, const T& z1,
                        const T& x2, const T& y2, const T& z2,
                        const T& x3, const T& y3, const T& z3,
                              T& px,       T& py,       T& pz)
   {
      /* Using Heron's s=ur */
      const T side12 = distance(x1,y1,z1,x2,y2,z2);
      const T side23 = distance(x2,y2,z2,x3,y3,z3);
      const T side31 = distance(x3,y3,z3,x1,y1,z1);
      const T perim  = T(1.0) / (side12 + side23 + side31);

      px = (side23 * x1 + side31 * x2 + side12 * x3) * perim;
      py = (side23 * y1 + side31 * y2 + side12 * y3) * perim;
      pz = (side23 * z1 + side31 * z2 + side12 * z3) * perim;
   }

   template <typename T>
   inline point2d<T> incenter(const point2d<T>& point1,
                              const point2d<T>& point2,
                              const point2d<T>& point3)
   {
      point2d<T> point_;

      incenter
      (
        point1.x, point1.y,
        point2.x, point2.y,
        point3.x, point3.y,
        point_.x, point_.y
      );

      return point_;
   }

   template <typename T>
   inline point3d<T> incenter(const point3d<T>& point1,
                              const point3d<T>& point2,
                              const point3d<T>& point3)
   {
      point3d<T> point_;

      incenter
      (
        point1.x, point1.y, point1.z,
        point2.x, point2.y, point2.z,
        point3.x, point3.y, point3.z,
        point_.x, point_.y, point_.z
      );

      return point_;
   }

   template <typename T>
   inline point2d<T> incenter(const triangle<T,2>& triangle)
   {
      return incenter(triangle[0],triangle[1],triangle[2]);
   }

   template <typename T>
   inline point3d<T> incenter(const triangle<T,3>& triangle)
   {
      return incenter(triangle[0],triangle[1],triangle[2]);
   }

   template <typename T>
   inline void circumcenter(const T& x1, const T& y1,
                            const T& x2, const T& y2,
                            const T& x3, const T& y3,
                                  T& px,       T& py)
   {
      const T a = x2 - x1;
      const T b = y2 - y1;
      const T c = x3 - x1;
      const T d = y3 - y1;
      const T e = a * (x1 + x2) + b * (y1 + y2);
      const T f = c * (x1 + x3) + d * (y1 + y3);
      const T g = T(2.0) * (a * (y3 - y2) - b * (x3 - x2));

      if (is_equal(g,T(0.0)))
      {
         px = infinity<T>();
         py = infinity<T>();
      }
      else
      {
         px = (d * e - b * f) / g;
         py = (a * f - c * e) / g;
      }
   }

   template <typename T>
   inline void circumcenter(const T& x1, const T& y1, const T& z1,
                            const T& x2, const T& y2, const T& z2,
                            const T& x3, const T& y3, const T& z3,
                                  T& px,       T& py,       T& pz)
   {
      const plane<T,3> tri_plane = make_plane(x1,y1,z1,x2,y2,z2,x3,y3,z3);

      const vector3d<T> vec1 = make_vector(x1 - x2,y1 - y2,z1 - z2);
      const vector3d<T> vec2 = make_vector(x3 - x2,y3 - y2,z3 - z2);

      const point3d<T> p1 = make_point((x1 + x2) * T (0.5),(y1 + y2) * T (0.5),(z1 + z2) * T (0.5));
      const point3d<T> p2 = make_point((x3 + x2) * T (0.5),(y3 + y2) * T (0.5),(z3 + z2) * T (0.5));

      const point3d<T> p1_ = p1 + (vec1 * tri_plane.normal);
      const point3d<T> p2_ = p2 + (vec2 * tri_plane.normal);

      intersection_point_line_to_line
      (
        p1.x , p1.y , p1.z ,
        p1_.x, p1_.y, p1_.z,
        p2.x , p2.y , p2.z ,
        p2_.x, p2_.y, p2_.z,
        px   , py   , pz   , T(Epsilon)
      );
   }

   template <typename T>
   inline point2d<T> circumcenter(const point2d<T>& point1,
                                  const point2d<T>& point2,
                                  const point2d<T>& point3)
   {
      point2d<T> point_;

      circumcenter
      (
        point1.x, point1.y,
        point2.x, point2.y,
        point3.x, point3.y,
        point_.x, point_.y
      );

      return point_;
   }

   template <typename T>
   inline point3d<T> circumcenter(const point3d<T>& point1,
                                  const point3d<T>& point2,
                                  const point3d<T>& point3)
   {
      point3d<T> point_;

      circumcenter
      (
        point1.x, point1.y, point1.z,
        point2.x, point2.y, point2.z,
        point3.x, point3.y, point3.z,
        point_.x, point_.y, point_.z
      );

      return point_;
   }

   template <typename T>
   inline point2d<T> circumcenter(const triangle<T,2>& triangle)
   {
      return circumcenter(triangle[0],triangle[1],triangle[2]);
   }

   template <typename T>
   inline point3d<T> circumcenter(const triangle<T,3>& triangle)
   {
      return circumcenter(triangle[0],triangle[1],triangle[2]);
   }

   template <typename T>
   inline circle<T> circumcircle(const T& x1, const T& y1,
                                 const T& x2, const T& y2,
                                 const T& x3, const T& y3)
   {
      T cx = T(0.0);
      T cy = T(0.0);

      circumcenter(x1,y1,x2,y2,x3,y3,cx,cy);

      return make_circle(cx,cy,distance(cx,cy,x1,y1));
   }

   template <typename T>
   inline circle<T> circumcircle(const point2d<T>& point1,
                                 const point2d<T>& point2,
                                 const point2d<T>& point3)
   {
      return circumcircle
             (
               point1.x, point1.y,
               point2.x, point2.y,
               point3.x, point3.y
             );
   }

   template <typename T>
   inline circle<T> circumcircle(const triangle<T,2>& triangle)
   {
      return circumcircle(triangle[0],triangle[1],triangle[2]);
   }

   template <typename T>
   inline sphere<T> circumsphere(const T& x1, const T& y1, const T& z1,
                                 const T& x2, const T& y2, const T& z2,
                                 const T& x3, const T& y3, const T& z3)
   {
      T cx = T(0.0);
      T cy = T(0.0);
      T cz = T(0.0);

      circumcenter(x1, y1, z1, x2, y2, z2, x3, y3, z3, cx, cy, cz);

      return make_sphere
             (
               cx, cy, cz,
               distance(cx, cy, cz, x1, y1, z2)
             );
   }

   template <typename T>
   inline sphere<T> circumsphere(const point3d<T>& point1,
                                 const point3d<T>& point2,
                                 const point3d<T>& point3)
   {
      return circumsphere
             (
               point1.x, point1.y, point1.z,
               point2.x, point2.y, point2.z,
               point3.x, point3.y, point3.z
             );
   }

   template <typename T>
   inline sphere<T> circumsphere(const triangle<T,3>& triangle)
   {
      return circumsphere(triangle[0], triangle[1], triangle[2]);
   }

   template <typename T>
   inline circle<T> inscribed_circle(const T& x1, const T& y1,
                                     const T& x2, const T& y2,
                                     const T& x3, const T& y3)
   {
      /* using heron's s = ur */
      const T side12    = distance(x1,y1,x2,y2);
      const T side23    = distance(x2,y2,x3,y3);
      const T side31    = distance(x3,y3,x1,y1);
      const T perimeter = T(1.0) / (side12 + side23 + side31);

      return make_circle(
                          (side23 * x1 + side31 * x2 + side12 * x3) * perimeter,
                          (side23 * y1 + side31 * y2 + side12 * y3) * perimeter,
                          T(0.5) * sqrt((-side12 + side23 + side31) * (side12 - side23 + side31) * (side12 + side23 - side31) * perimeter)
                        );
   }

   template <typename T>
   inline circle<T> inscribed_circle(const point2d<T>& point1,
                                     const point2d<T>& point2,
                                     const point2d<T>& point3)
   {
      return inscribed_circle
             (
               point1.x, point1.y,
               point2.x, point2.y,
               point3.x, point3.y
             );
   }

   template <typename T>
   inline circle<T> inscribed_circle(const triangle<T,2>& triangle)
   {
      return inscribed_circle(triangle[0],triangle[1],triangle[2]);
   }

   template <typename T>
   inline sphere<T> inscribed_sphere(const T& x1, const T& y1, const T& z1,
                                     const T& x2, const T& y2, const T& z2,
                                     const T& x3, const T& y3, const T& z3)
   {
      point3d<T> int_point = intersection_point
                             (
                               create_line_from_bisector(x1, y1, z1, x2, y2, z2, x3, y3, z3),
                               create_line_from_bisector(x2, y2, z2, x3, y3, z3, x1, y1, z1),
                               T(Epsilon)
                             );

      return make_sphere
             (
               int_point,
               distance
               (
                 int_point,
                 closest_point_on_line_from_point
                 (
                   make_line(x1, y1, z1, x2, y2, z2),
                   int_point
                 )
               )
             );
   }

   template <typename T>
   inline sphere<T> inscribed_sphere(const point3d<T>& point1,
                                     const point3d<T>& point2,
                                     const point3d<T>& point3)
   {
      return inscribed_sphere
             (
               point1.x, point1.y, point1.z,
               point2.x, point2.y, point2.z,
               point3.x, point3.y, point3.z
             );
   }

   template <typename T>
   inline sphere<T> inscribed_sphere(const triangle<T,3>& triangle)
   {
      return inscribed_sphere(triangle[0],triangle[1],triangle[2]);
   }

   template <typename T>
   inline circle<T> nine_point_circle(const T& x1, const T& y1,
                                      const T& x2, const T& y2,
                                      const T& x3, const T& y3)
   {
      T hx1 = T(0.0);
      T hy1 = T(0.0);
      T hx2 = T(0.0);
      T hy2 = T(0.0);
      T hx3 = T(0.0);
      T hy3 = T(0.0);

      closest_point_on_line_from_point(x2, y2, x3, y3, x1, y1, hx1, hy1);
      closest_point_on_line_from_point(x1, y1, x3, y3, x2, y2, hx2, hy2);
      closest_point_on_line_from_point(x1, y1, x2, y2, x3, y3, hx3, hy3);

      return circumcircle(hx1, hy1, hx2, hy2, hx3, hy3);
   }

   template <typename T>
   inline circle<T> nine_point_circle(const point2d<T>& point1,
                                      const point2d<T>& point2,
                                      const point2d<T>& point3)
   {
      return nine_point_circle
             (
               point1.x, point1.y,
               point2.x, point2.y,
               point3.x, point3.y
             );
   }

   template <typename T>
   inline circle<T> nine_point_circle(const triangle<T,2>& triangle)
   {
      return nine_point_circle(triangle[0],triangle[1],triangle[2]);
   }

   template <typename T>
   inline point2d<T> orthocenter(const triangle<T,2>& triangle)
   {
      return intersection_point
             (
               make_line(triangle[0], closest_point_on_line_from_point(make_line(opposing_edge(triangle, 0)), triangle[0])),
               make_line(triangle[1], closest_point_on_line_from_point(make_line(opposing_edge(triangle, 1)), triangle[0]))
             );
   }

   template <typename T>
   inline point3d<T> orthocenter(const triangle<T,3>& triangle)
   {
      return intersection_point
             (
               make_line(triangle[0], closest_point_on_line_from_point(make_line(opposing_edge(triangle, 0)), triangle[0])),
               make_line(triangle[1], closest_point_on_line_from_point(make_line(opposing_edge(triangle, 1)), triangle[0]))
             );
   }

   template <typename T>
   inline point2d<T> excenter(const triangle<T,2>& triangle, const std::size_t& corner)
   {
      return intersection_point
             (
               triangle_bisector(triangle, corner),
               triangle_external_bisector(triangle, corner,(corner + 1) % 3)
             );
   }

   template <typename T>
   inline point3d<T> excenter(const triangle<T,3>& triangle, const std::size_t& corner)
   {
      return intersection_point
             (
               triangle_bisector(triangle, corner),
               triangle_external_bisector(triangle, corner,(corner + 1) % 3)
             );
   }

   template <typename T>
   inline circle<T> excircle(const triangle<T,2>& triangle, const std::size_t& i)
   {
      if (i < 3)
      {
         const point2d<T> center = excenter(triangle,i);
         const T          radius = minimum_distance_from_point_to_segment
                                   (
                                     center,
                                     opposing_edge(triangle,i)
                                   );

         return make_circle(center,radius);
      }
      else
         return degenerate_circle<T>();
   }

   template <typename T>
   inline circle<T> mandart_circle(const triangle<T,2>& triangle)
   {
      return circumcircle(create_extouch_triangle(triangle));
   }

   template <typename T>
   inline circle<T> brocard_circle(const triangle<T,2>& triangle)
   {
      return make_circle(circumcenter(triangle),symmedian_point(triangle));
   }

   template <typename T>
   inline circle<T> invert_circle_across_circle(const circle<T>& circle1, const circle<T>& circle2)
   {
      const T t = sqr(circle2.radius) /
                     (
                       sqr(circle2.x - circle1.x) +
                       sqr(circle2.y - circle1.y) -
                       sqr(circle1.radius)
                     );

      return make_circle(project_point_t(make_point(circle2),make_point(circle1),t),t * circle1.radius);
   }

   template <typename T>
   inline sphere<T> invert_sphere_across_sphere(const sphere<T>& sphere1, const sphere<T>& sphere2)
   {
      const T t = sqr(sphere2.radius) /
                  (
                    sqr(sphere2.x - sphere1.x) +
                    sqr(sphere2.y - sphere1.y) +
                    sqr(sphere2.z - sphere1.z) -
                    sqr(sphere1.radius)
                  );

      return make_sphere(project_point_t(make_point(sphere2),make_point(sphere1),t),t * sphere1.radius);
   }

   template <typename T>
   inline void circle_tangent_points(const circle<T>& circle, const point2d<T>& point, point2d<T>& point1, point2d<T>& point2)
   {
      const vector2d<T> v = point - make_point(circle);
      const T sqr_length  = sqr(v.x) + sqr(v.y);
      const T radius_sqr  = sqr(circle.radius);

      if (greater_than_or_equal(sqr_length,radius_sqr))
      {
         const T ratio = T(1.0) / sqr_length;
         const T delta_dist  = sqrt(abs(sqr_length - radius_sqr));

         point1.x = circle.x + circle.radius * (circle.radius * v.x - v.y * delta_dist) * ratio;
         point1.y = circle.y + circle.radius * (circle.radius * v.y + v.x * delta_dist) * ratio;
         point2.x = circle.x + circle.radius * (circle.radius * v.x + v.y * delta_dist) * ratio;
         point2.y = circle.y + circle.radius * (circle.radius * v.y - v.x * delta_dist) * ratio;
      }
      else
      {
         point1 = degenerate_point2d<T>();
         point2 = degenerate_point2d<T>();
      }
   }

   template <typename T>
   inline void circle_internal_tangent_lines(const circle<T>& circle0,
                                             const circle<T>& circle1,
                                             std::vector<line<T,2> >& lines)
   {
      const point2d<T> c0 = make_point(circle0.x, circle0.y);
      const point2d<T> c1 = make_point(circle1.x, circle1.y);

      const T dist = distance(c0,c1);

      if ((dist - (circle0.radius + circle1.radius)) < T(0))
         return;
      else if (is_equal(std::abs(dist - (circle0.radius + circle1.radius)),T(0.0)))
         lines.push_back(create_perpendicular_bisector(make_segment(c0, c1)));
      else
      {
         const T m  = circle0.radius / circle1.radius;
         const T h0 = (m * dist) / (m + T(1));
         const T h1 =      dist  / (m + T(1));

         const point2d<T> i = make_point<T>
                              (
                                (h1 * circle0.x + h0 * circle1.x) / dist,
                                (h1 * circle0.y + h0 * circle1.y) / dist
                              );

         wykobi::point2d<T> c0pnt0;
         wykobi::point2d<T> c0pnt1;
         wykobi::point2d<T> c1pnt0;
         wykobi::point2d<T> c1pnt1;

         wykobi::circle_tangent_points(circle0, i, c0pnt0, c0pnt1);
         wykobi::circle_tangent_points(circle1, i, c1pnt0, c1pnt1);

         lines.push_back(make_line(c0pnt0, c1pnt0));
         lines.push_back(make_line(c0pnt1, c1pnt1));
      }
   }

   template <typename T>
   inline void circle_internal_tangent_segments(const circle<T>& circle0,
                                                const circle<T>& circle1,
                                                std::vector< segment<T,2> >& segments)
   {
      std::vector< line<T,2> > lines;

      circle_internal_tangent_lines(circle0, circle1, lines);

      switch (lines.size())
      {
         case 0 : return;

         case 1 : segments.push_back(make_segment(lines[0]));
                  return;

         case 2 : segments.push_back(make_segment(lines[0]));
                  segments.push_back(make_segment(lines[1]));
                  return;
      }
   }

   template <typename T>
   inline void circle_outer_tangent_lines(const circle<T>& circle0,
                                          const circle<T>& circle1,
                                          std::vector< line<T,2> >& lines)
   {
      const point2d<T> c0 = make_point(circle0.x, circle0.y);
      const point2d<T> c1 = make_point(circle1.x, circle1.y);

      const T dist = distance(c0,c1);

      if (dist < std::abs(circle0.radius - circle1.radius))
         return;
      else if (is_equal(std::abs(dist - std::abs(circle0.radius - circle1.radius)),T(0.0)))
         lines.push_back(create_perpendicular_bisector(make_segment(c0, c1)));
      else if (is_equal(circle0.radius - circle1.radius,T(0.0)))
      {
         const point2d<T> c0pnt0 = c0 + (+circle0.radius * perpendicular(normalize(c1 - c0)));
         const point2d<T> c0pnt1 = c0 + (-circle0.radius * perpendicular(normalize(c1 - c0)));
         const point2d<T> c1pnt0 = c1 + (+circle1.radius * perpendicular(normalize(c0 - c1)));
         const point2d<T> c1pnt1 = c1 + (-circle1.radius * perpendicular(normalize(c0 - c1)));

         lines.push_back(make_line(c0pnt0, c1pnt1));
         lines.push_back(make_line(c0pnt1, c1pnt0));
      }
      else
      {
         point2d<T> p;

         if (circle0.radius > circle1.radius)
            p = make_point
                (
                  c1.x * circle0.radius -  c0.x * circle1.radius,
                  c1.y * circle0.radius -  c0.y * circle1.radius
                );
         else
            p = make_point
                (
                  c0.x * circle1.radius -  c1.x * circle0.radius,
                  c0.y * circle1.radius -  c1.y * circle0.radius
                );

         const T diff = std::abs(circle1.radius - circle0.radius);

         p.x /= diff;
         p.y /= diff;

         wykobi::point2d<T> c0pnt0;
         wykobi::point2d<T> c0pnt1;
         wykobi::point2d<T> c1pnt0;
         wykobi::point2d<T> c1pnt1;

         wykobi::circle_tangent_points(circle0, p, c0pnt0, c0pnt1);
         wykobi::circle_tangent_points(circle1, p, c1pnt0, c1pnt1);

         lines.push_back(make_line(c0pnt0, c1pnt0));
         lines.push_back(make_line(c0pnt1, c1pnt1));
      }
   }

   template <typename T>
   inline void circle_outer_tangent_segments(const circle<T>& circle0,
                                             const circle<T>& circle1,
                                             std::vector< segment<T,2> >& segments)
   {
      std::vector< line<T,2> > lines;

      circle_outer_tangent_lines(circle0, circle1, lines);

      switch (lines.size())
      {
         case 0 : return;

         case 1 : segments.push_back(make_segment(lines[0]));
                  return;

         case 2 : segments.push_back(make_segment(lines[0]));
                  segments.push_back(make_segment(lines[1]));
                  return;
      }
   }

   template <typename T>
   inline line<T,2> tangent_line(const circle<T>& circle, const point2d<T>& point)
   {
      return make_line(point,point + perpendicular(point - make_point(circle)));
   }

   template <typename T>
   inline line<T,2> create_line_from_bisector(const T& x1, const T& y1,
                                              const T& x2, const T& y2,
                                              const T& x3, const T& y3)
   {
      /*
         Bisector angle theorem:
         D: bisector intersect point along (x1y1)(x3,y3)
         |(x1y1)(x2-y2)| : |(x2y2)(x3y3)| == |(x1y1)D| : |D(x3y3)|
      */
      const T dist1 = distance(x1,y1,x2,y2);
      const T dist2 = distance(x2,y2,x3,y3);
      const T ratio = dist2 / (dist1 + dist2);

      return make_line(x2,y2,x3 + ratio * (x1 - x3), y3 + ratio * (y1 - y3));
   }

   template <typename T>
   inline segment<T,2> create_segment_from_bisector(const T& x1, const T& y1,
                                                    const T& x2, const T& y2,
                                                    const T& x3, const T& y3)
   {
      const T dist1 = distance(x1,y1,x2,y2);
      const T dist2 = distance(x2,y2,x3,y3);
      const T ratio = dist2 / (dist1 + dist2);

      return make_segment(x2,y2,x3 + ratio * (x1 - x3), y3 + ratio * (y1 - y3));
   }

   template <typename T>
   inline ray<T,2> create_ray_from_bisector(const T& x1, const T& y1,
                                            const T& x2, const T& y2,
                                            const T& x3, const T& y3)
   {
      segment<T,2> segment = create_segment_from_bisector(x1,y1,x2,y2,x3,y3);

      return make_ray(segment[0],segment[1] - segment[0]);
   }

   template <typename T>
   inline line<T,3> create_line_from_bisector(const T& x1, const T& y1, const T& z1,
                                              const T& x2, const T& y2, const T& z2,
                                              const T& x3, const T& y3, const T& z3)
   {
      const T dist1 = distance(x1,y1,z1,x2,y2,z2);
      const T dist2 = distance(x2,y2,z2,x3,y3,z3);
      const T ratio = dist2 / (dist1 + dist2);

      return make_line(x2,y2,z2,x3 + ratio * (x1 - x3), y3 + ratio * (y1 - y3), z3 + ratio * (z1 - z3));
   }

   template <typename T>
   inline segment<T,3> create_segment_from_bisector(const T& x1, const T& y1, const T& z1,
                                                    const T& x2, const T& y2, const T& z2,
                                                    const T& x3, const T& y3, const T& z3)
   {
      const T dist1 = distance(x1,y1,z1,x2,y2,z2);
      const T dist2 = distance(x2,y2,z2,x3,y3,z3);
      const T ratio = dist2 / (dist1 + dist2);

      return make_segment
             (
               x2, y2, z2,
               x3 + ratio * (x1 - x3), y3 + ratio * (y1 - y3), z3 + ratio * (z1 - z3)
             );
   }

   template <typename T>
   inline ray<T,3> create_ray_from_bisector(const T& x1, const T& y1, const T& z1,
                                            const T& x2, const T& y2, const T& z2,
                                            const T& x3, const T& y3, const T& z3)
   {
      segment<T,3> segment = create_segment_from_bisector(x1,y1,z1,x2,y2,z2,x3,y3,z3);

      return make_ray(segment[0],segment[1] - segment[0]);
   }

   template <typename T>
   inline line<T,2> create_line_from_bisector(const point2d<T>& point1,const point2d<T>& point2,const point2d<T>& point3)
   {
      return create_line_from_bisector(point1.x,point1.y,point2.x,point2.y,point3.x,point3.y);
   }

   template <typename T>
   inline segment<T,2> create_segment_from_bisector(const point2d<T>& point1,const point2d<T>& point2,const point2d<T>& point3)
   {
      return create_segment_from_bisector(point1.x,point1.y,point2.x,point2.y,point3.x,point3.y);
   }

   template <typename T>
   inline ray<T,2> create_ray_from_bisector(const point2d<T>& point1,const point2d<T>& point2,const point2d<T>& point3)
   {
      return create_ray_from_bisector(point1.x,point1.y,point2.x,point2.y,point3.x,point3.y);
   }

   template <typename T>
   inline line<T,3> create_line_from_bisector(const point3d<T>& point1,const point3d<T>& point2,const point3d<T>& point3)
   {
      return create_line_from_bisector(point1.x,point1.y,point1.z,point2.x,point2.y,point2.z,point3.x,point3.y,point3.z);
   }

   template <typename T>
   inline segment<T,3> create_segment_from_bisector(const point3d<T>& point1,const point3d<T>& point2,const point3d<T>& point3)
   {
      return create_segment_from_bisector(point1.x,point1.y,point1.z,point2.x,point2.y,point2.z,point3.x,point3.y,point3.z);
   }

   template <typename T>
   inline ray<T,3> create_ray_from_bisector(const point3d<T>& point1,const point3d<T>& point2,const point3d<T>& point3)
   {
      return create_ray_from_bisector(point1.x,point1.y,point1.z,point2.x,point2.y,point2.z,point3.x,point3.y,point3.z);
   }

   template <typename T>
   inline line<T,2> create_perpendicular_bisector(const T& x1, const T& y1,const T& x2, const T& y2)
   {
      const T mx = (x1 + x2) * T(0.5);
      const T my = (y1 + y2) * T(0.5);

      return make_line(mx, my, mx + (y1 - y2), my + (x2 - x1));
   }

   template <typename T>
   inline line<T,2> create_perpendicular_bisector(const point2d<T>& point1, const point2d<T>& point2)
   {
      return create_perpendicular_bisector(point1.x,point1.y,point2.x,point2.y);
   }

   template <typename T>
   inline line<T,2> create_perpendicular_bisector(const segment<T,2>& segment)
   {
      return create_perpendicular_bisector(segment[0],segment[1]);
   }

   template <typename T>
   inline line<T,2> create_perpendicular_line_at_end_point(const line<T,2>& line)
   {
      return make_line(line[1],perpendicular(line[1] - line[0]) + line[1]);
   }

   template <typename T>
   inline void closest_point_on_segment_from_point(const T& x1, const T& y1,
                                                   const T& x2, const T& y2,
                                                   const T& px, const T& py,
                                                         T& nx,       T& ny)
   {
      const T vx = x2 - x1;
      const T vy = y2 - y1;
      const T wx = px - x1;
      const T wy = py - y1;

      const T c1 = vx * wx + vy * wy;

      if (c1 <= T(0.0))
      {
         nx = x1;
         ny = y1;

         return;
      }

      const T c2 = vx * vx + vy * vy;

      if (c2 <= c1)
      {
         nx = x2;
         ny = y2;

         return;
      }

      const T ratio = c1 / c2;

      nx = x1 + ratio * vx;
      ny = y1 + ratio * vy;
   }

   template <typename T>
   inline void closest_point_on_segment_from_point(const T& x1, const T& y1, const T& z1,
                                                   const T& x2, const T& y2, const T& z2,
                                                   const T& px, const T& py, const T& pz,
                                                         T& nx,       T& ny,       T& nz)
   {
      const T vx = x2 - x1;
      const T vy = y2 - y1;
      const T vz = z2 - z1;
      const T wx = px - x1;
      const T wy = py - y1;
      const T wz = pz - z1;

      const T c1 = vx * wx + vy * wy + vz * wz;

      if (c1 <= T(0.0))
      {
         nx = x1;
         ny = y1;
         nz = z1;

         return;
      }

      const T c2 = vx * vx + vy * vy + vz * vz;

      if (c2 <= c1)
      {
         nx = x2;
         ny = y2;
         nz = z2;

         return;
      }

      const T ratio = c1 / c2;

      nx = x1 + ratio * vx;
      ny = y1 + ratio * vy;
      nz = z1 + ratio * vz;
   }

   template <typename T>
   inline void closest_point_on_line_from_point(const T& x1, const T& y1,
                                                const T& x2, const T& y2,
                                                const T& px, const T& py,
                                                      T& nx,       T& ny)
   {
      const T vx = x2 - x1;
      const T vy = y2 - y1;
      const T wx = px - x1;
      const T wy = py - y1;

      const T c1 = vx * wx + vy * wy;
      const T c2 = vx * vx + vy * vy;

      const T ratio = c1 / c2;

      nx = x1 + ratio * vx;
      ny = y1 + ratio * vy;
   }

   template <typename T>
   inline void closest_point_on_line_from_point(const T& x1, const T& y1, const T& z1,
                                                const T& x2, const T& y2, const T& z2,
                                                const T& px, const T& py, const T& pz,
                                                      T& nx,       T& ny,       T& nz)
   {
      const T vx = x2 - x1;
      const T vy = y2 - y1;
      const T vz = z2 - z1;
      const T wx = px - x1;
      const T wy = py - y1;
      const T wz = pz - z1;

      const T c1 = vx * wx + vy * wy + vz * wz;
      const T c2 = vx * vx + vy * vy + vz * vz;

      const T ratio = c1 / c2;

      nx = x1 + ratio * vx;
      ny = y1 + ratio * vy;
      nz = z1 + ratio * vz;
   }

   template <typename T>
   inline void order_sensitive_closest_point_on_segment_from_point(const T& x1, const T& y1,
                                                                   const T& x2, const T& y2,
                                                                   const T& px, const T& py,
                                                                         T& nx,       T& ny)
   {
      if (x1 < x2)
      {
         closest_point_on_segment_from_point(x1,y1,x2,y2,px,py,nx,ny);
         return;
      }
      else if (is_equal(x1,x2) && (y1 < y2))
      {
         closest_point_on_segment_from_point(x1,y1,x2,y2,px,py,nx,ny);
         return;
      }
      else
         closest_point_on_segment_from_point(x2,y2,x1,y1,px,py,nx,ny);
   }

   template <typename T>
   inline void order_sensitive_closest_point_on_segment_from_point(const T& x1, const T& y1, const T& z1,
                                                                   const T& x2, const T& y2, const T& z2,
                                                                   const T& px, const T& py, const T& pz,
                                                                         T& nx,       T& ny,       T& nz)
   {
      if (x1 < x2)
      {
         closest_point_on_segment_from_point(x1,y1,z1,x2,y2,z2,px,py,pz,nx,ny,nz);
         return;
      }
      else if (is_equal(x1,x2) && (y1 < y2))
      {
         closest_point_on_segment_from_point(x1,y1,z1,x2,y2,z2,px,py,pz,nx,ny,nz);
         return;
      }
      else if (is_equal(y1,y2) && (z1 < z2))
      {
         closest_point_on_segment_from_point(x1,y1,z1,x2,y2,z2,px,py,pz,nx,ny,nz);
         return;
      }
      else
         closest_point_on_segment_from_point(x2,y2,z2,x1,y1,z1,px,py,pz,nx,ny,nz);
   }

   template <typename T>
   inline void order_sensitive_closest_point_on_line_from_point(const T& x1, const T& y1,
                                                                const T& x2, const T& y2,
                                                                const T& px, const T& py,
                                                                      T& nx,       T& ny)
   {
      if (x1 < x2)
      {
         closest_point_on_line_from_point(x1,y1,x2,y2,px,py,nx,ny);
         return;
      }
      else if (is_equal(x1,x2) && (y1 < y2))
      {
         closest_point_on_line_from_point(x1,y1,x2,y2,px,py,nx,ny);
         return;
      }
      else
         closest_point_on_line_from_point(x2,y2,x1,y1,px,py,nx,ny);
   }

   template <typename T>
   inline void order_sensitive_closest_point_on_line_from_point(const T& x1, const T& y1, const T& z1,
                                                                const T& x2, const T& y2, const T& z2,
                                                                const T& px, const T& py, const T& pz,
                                                                      T& nx,       T& ny,       T& nz)
   {
      if (x1 < x2)
      {
         closest_point_on_line_from_point(x1,y1,z1,x2,y2,z2,px,py,pz,nx,ny,nz);
         return;
      }
      else if (is_equal(x1,x2) && (y1 < y2))
      {
         closest_point_on_line_from_point(x1,y1,z1,x2,y2,z2,px,py,pz,nx,ny,nz);
         return;
      }
      else if (is_equal(y1,y2) && (z1 < z2))
      {
         closest_point_on_line_from_point(x1,y1,z1,x2,y2,z2,px,py,pz,nx,ny,nz);
         return;
      }
      else
         closest_point_on_segment_from_point(x2,y2,z2,x1,y1,z1,px,py,pz,nx,ny,nz);
   }

   template <typename T>
   inline void closest_point_on_ray_from_point(const T& ox, const T& oy,
                                               const T& dx, const T& dy,
                                               const T& px, const T& py,
                                                     T& nx,       T& ny)
   {
      const T t  = dx * (px - ox) + dy * (py - oy);

      if (t < T(0.0))
      {
         nx = ox;
         ny = oy;
      }
      else
      {
         nx = ox + dx * t;
         ny = oy + dy * t;
      }
   }

   template <typename T>
   inline void closest_point_on_ray_from_point(const T& ox, const T& oy, const T& oz,
                                               const T& dx, const T& dy, const T& dz,
                                               const T& px, const T& py, const T& pz,
                                                     T& nx,       T& ny,       T& nz)
   {
      const T t  = dx * (px - ox) + dy * (py - oy) + dz * (pz - oz);

      if (t < T(0.0))
      {
         nx = ox;
         ny = oy;
         nz = oz;
      }
      else
      {
         nx = ox + dx * t;
         ny = oy + dy * t;
         nz = oz + dz * t;
      }
   }

   template <typename T>
   inline point2d<T> closest_point_on_segment_from_point(const T& x1, const T& y1,
                                                         const T& x2, const T& y2,
                                                         const T& px, const T& py)
   {
      point2d<T> point;
      closest_point_on_segment_from_point(x1,y1,x2,y2,px,py,point.x,point.y);

      return point;
   }

   template <typename T>
   inline point3d<T> closest_point_on_segment_from_point(const T& x1, const T& y1, const T& z1,
                                                         const T& x2, const T& y2, const T& z2,
                                                         const T& px, const T& py, const T& pz)
   {
      point3d<T> point;
      closest_point_on_segment_from_point(x1,y1,z1,x2,y2,z2,px,py,pz,point.x,point.y,point.z);

      return point;
   }

   template <typename T>
   inline point2d<T> closest_point_on_segment_from_point(const segment<T,2>& segment, const point2d<T>& point)
   {
      point2d<T> point_;
      closest_point_on_segment_from_point(segment[0].x, segment[0].y,
                                          segment[1].x, segment[1].y,
                                               point.x,      point.y,
                                              point_.x,     point_.y);
      return point_;
   }

   template <typename T>
   inline point3d<T> closest_point_on_segment_from_point(const segment<T,3>& segment, const point3d<T>& point)
   {
      point3d<T> point_;
      closest_point_on_segment_from_point(segment[0].x, segment[0].y, segment[0].z,
                                          segment[1].x, segment[1].y, segment[1].z,
                                               point.x,      point.y,      point.z,
                                              point_.x,     point_.y,     point_.z);
      return point_;
   }

   template <typename T>
   inline point2d<T> closest_point_on_line_from_point(const T& x1, const T& y1,
                                                      const T& x2, const T& y2,
                                                      const T& px, const T& py)
   {
      point2d<T> point;
      closest_point_on_line_from_point(x1,y1,x2,y2,px,py,point.x,point.y);
      return point;

   }

   template <typename T>
   inline point3d<T> closest_point_on_line_from_point(const T& x1, const T& y1, const T& z1,
                                                      const T& x2, const T& y2, const T& z2,
                                                      const T& px, const T& py, const T& pz)
   {
      point3d<T> point;
      closest_point_on_line_from_point(x1,y1,z1,x2,y2,z2,px,py,pz,point.x,point.y,point.z);
      return point;

   }

   template <typename T>
   inline point2d<T> closest_point_on_line_from_point(const line<T,2>& line, const point2d<T>& point)
   {
      point2d<T> point_;

      closest_point_on_line_from_point
      (
        line[0].x, line[0].y,
        line[1].x, line[1].y,
          point.x,   point.y,
         point_.x,  point_.y
      );

      return point_;
   }

   template <typename T>
   inline point3d<T> closest_point_on_line_from_point(const line<T,3>& line, const point3d<T>& point)
   {
      point3d<T> point_;

      closest_point_on_line_from_point
      (
        line[0].x, line[0].y, line[0].z,
        line[1].x, line[1].y, line[1].z,
          point.x,   point.y,   point.z,
         point_.x,  point_.y,  point_.z
      );

      return point_;
   }

   template <typename T>
   inline point2d<T> closest_point_on_ray_from_point(const T& ox, const T& oy,
                                                     const T& dx, const T& dy,
                                                     const T& px, const T& py)
   {
      point2d<T> point_;
      closest_point_on_ray_from_point(ox,oy,dx,dy,px,py,point_.x,point_.y);
      return point_;
   }

   template <typename T>
   inline point3d<T> closest_point_on_ray_from_point(const T& ox, const T& oy, const T& oz,
                                                     const T& dx, const T& dy, const T& dz,
                                                     const T& px, const T& py, const T& pz)
   {
      point3d<T> point_;
      closest_point_on_ray_from_point(ox,oy,oz,dx,dy,dz,px,py,pz,point_.x,point_.y,point_.z);
      return point_;
   }

   template <typename T>
   inline point2d<T> closest_point_on_ray_from_point(const ray<T,2>& ray, const point2d<T>& point)
   {
      return closest_point_on_ray_from_point
             (
               ray.origin   .x, ray.origin   .y,
               ray.direction.x, ray.direction.y,
               point        .x, point        .y
             );
   }

   template <typename T>
   inline point3d<T> closest_point_on_ray_from_point(const ray<T,3>& ray, const point3d<T>& point)
   {
      return closest_point_on_ray_from_point
             (
               ray.origin   .x, ray.origin   .y, ray.origin   .z,
               ray.direction.x, ray.direction.y, ray.direction.z,
               point        .x, point        .y, point        .z
             );
   }

   template <typename T>
   inline void closest_point_on_triangle_from_point(const T& x1, const T& y1,
                                                    const T& x2, const T& y2,
                                                    const T& x3, const T& y3,
                                                    const T& px, const T& py,
                                                          T& nx,       T& ny)
   {
      const int or1 = orientation((x2 + x3) * T(0.5), (y2 + y3) * T(0.5), x1, y1, px, py);
      const int or2 = orientation((x1 + x3) * T(0.5), (y1 + y3) * T(0.5), x2, y2, px, py);

      if (differing_orientation(x1,y1,x2,y2,px,py,x3,y3) && ((or1 * or2) == -1))
      {
         closest_point_on_segment_from_point(x1,y1,x2,y2,px,py,nx,ny);
         return;
      }

      const int or3 = orientation((x1 + x2) * T(0.5),(y1 + y2) * T(0.5),x3,y3,px,py);

      if (differing_orientation(x2,y2,x3,y3,px,py,x1,y1) && ((or2 * or3) == -1))
      {
         closest_point_on_segment_from_point(x2,y2,x3,y3,px,py,nx,ny);
         return;
      }

      if (differing_orientation(x3,y3,x1,y1,px,py,x2,y2) && ((or3 * or1) == -1))
      {
         closest_point_on_segment_from_point(x3,y3,x1,y1,px,py,nx,ny);
         return;
      }

      nx = px;
      ny = py;
   }

   template <typename T>
   inline point2d<T> closest_point_on_triangle_from_point(const T& x1, const T& y1,
                                                          const T& x2, const T& y2,
                                                          const T& x3, const T& y3,
                                                          const T& px, const T& py)
   {
      point2d<T> point;
      closest_point_on_triangle_from_point(x1,y1,x2,y2,x3,y3,px,py,point.x,point.y);
      return point;
   }

   template <typename T>
   inline point2d<T> closest_point_on_triangle_from_point(const triangle<T,2>& triangle, const T& px, const T& py)
   {
      return closest_point_on_triangle_from_point(triangle[0].x,triangle[0].y,
                                                  triangle[1].x,triangle[1].y,
                                                  triangle[2].x,triangle[2].y,
                                                             px,           py);

   }

   template <typename T>
   inline point2d<T> closest_point_on_triangle_from_point(const triangle<T,2>& triangle, const point2d<T>& point)
   {
      return closest_point_on_triangle_from_point(triangle[0].x,triangle[0].y,
                                                  triangle[1].x,triangle[1].y,
                                                  triangle[2].x,triangle[2].y,
                                                        point.x,      point.y);
   }

   template <typename T>
   inline void closest_point_on_triangle_from_point(const T& x1, const T& y1, const T& z1,
                                                    const T& x2, const T& y2, const T& z2,
                                                    const T& x3, const T& y3, const T& z3,
                                                    const T& px, const T& py, const T& pz,
                                                          T& nx,       T& ny,       T& nz)
   {
      point3d<T> point_;

      point_ = closest_point_on_triangle_from_point(x1,y1,z1,x2,y2,z2,x3,y3,z3,px,py,pz);

      nx = point_.x;
      ny = point_.y;
      nz = point_.z;
   }

   template <typename T>
   inline point3d<T> closest_point_on_triangle_from_point(const T& x1, const T& y1, const T& z1,
                                                          const T& x2, const T& y2, const T& z2,
                                                          const T& x3, const T& y3, const T& z3,
                                                          const T& px, const T& py, const T& pz)
   {
      return closest_point_on_triangle_from_point(make_triangle(x1,y1,z1,x2,y2,z2,x3,y3,z3),px,py,pz);
   }

   template <typename T>
   inline point3d<T> closest_point_on_triangle_from_point(const triangle<T,3>& triangle, const T& px, const T& py, const T& pz)
   {
      return closest_point_on_triangle_from_point(triangle, make_point(px,py,pz));
   }

   template <typename T>
   inline point3d<T> closest_point_on_triangle_from_point(const triangle<T,3>& triangle, const point3d<T>& point)
   {
      const vector3d<T> ab = triangle[1] - triangle[0];
      const vector3d<T> ac = triangle[2] - triangle[0];
      const vector3d<T> bc = triangle[2] - triangle[1];

      const T snom   = dot_product(point - triangle[0], ab);
      const T sdenom = dot_product(point - triangle[1], triangle[0] - triangle[1]);

      const T tnom   = dot_product(point - triangle[0], ac);
      const T tdenom = dot_product(point - triangle[2], triangle[0] - triangle[2]);

      if (snom <= T(0.0) && tnom <= T(0.0))
      {
         return triangle[0];
      }

      const T unom   = dot_product(point - triangle[1], bc);
      const T udenom = dot_product(point - triangle[2], triangle[1] - triangle[2]);

      if ((sdenom <= T(0.0)) && (unom <= T(0.0)))
      {
         return triangle[1];
      }

      if ((tdenom <= T(0.0)) && (udenom <= T(0.0)))
      {
         return triangle[2];
      }

      const vector3d<T> n = (triangle[1] - triangle[0]) * (triangle[2] - triangle[0]);

      const T vc = dot_product(n, (triangle[0] - point) * (triangle[1] - point));

      if ((vc <= T(0.0)) && (snom >= T(0.0)) && (sdenom >= T(0.0)))
      {
         return triangle[0] + snom / (snom + sdenom) * ab;
      }

      const T va = dot_product(n,(triangle[1] - point) * (triangle[2] - point));

      if (va <= T(0.0) && unom >= T(0.0) && udenom >= T(0.0))
      {
         return triangle[1] + unom / (unom + udenom) * bc;
      }

      const T vb = dot_product(n, (triangle[2] - point) * (triangle[0] - point));

      if ((vb <= T(0.0)) && (tnom >= T(0.0)) && (tdenom >= T(0.0)))
      {
         return triangle[0] + tnom / (tnom + tdenom) * ac;
      }

      const T u = va / (va + vb + vc);
      const T v = vb / (va + vb + vc);
      const T w = T(1.0) - u - v;

      return (u * triangle[0]) + (v * triangle[1]) + (w * triangle[2]);
   }

   template <typename T>
   inline void closest_point_on_rectangle_from_point(const T& x1, const T& y1,
                                                     const T& x2, const T& y2,
                                                     const T& px, const T& py,
                                                           T& nx,       T& ny)
   {
      if (px <min(x1,x2))
         nx = min(x1,x2);
      else if (px > max(x1,x2))
         nx = max(x1,x2);
      else
         nx = px;

      if (py <min(y1,y2))
         ny = min(y1,y2);
      else if (py > max(y1,y2))
         ny = max(y1,y2);
      else
         ny = py;
   }

   template <typename T>
   inline point2d<T> closest_point_on_rectangle_from_point(const T& x1, const T& y1,
                                                           const T& x2, const T& y2,
                                                           const T& px, const T& py)
   {
      point2d<T> point;
      closest_point_on_rectangle_from_point(x1,y1,x2,y2,px,py,point.x,point.y);
      return point;
   }

   template <typename T>
   inline point2d<T> closest_point_on_rectangle_from_point(const rectangle<T>& rectangle, const T& px, const T& py)
   {
      return closest_point_on_rectangle_from_point(rectangle[0].x,rectangle[0].y,
                                                   rectangle[1].x,rectangle[1].y,
                                                               px,            py);
   }

   template <typename T>
   inline point2d<T> closest_point_on_rectangle_from_point(const rectangle<T>& rectangle, const point2d<T>& point)
   {
      return closest_point_on_rectangle_from_point(rectangle[0].x,rectangle[0].y,
                                                   rectangle[1].x,rectangle[1].y,
                                                   point.x,              point.y);
   }

   template <typename T>
   inline void closest_point_on_box_from_point(const T& x1, const T& y1, const T& z1,
                                               const T& x2, const T& y2, const T& z2,
                                               const T& px, const T& py, const T& pz,
                                                     T& nx,       T& ny,       T& nz)
   {
      if (px <min(x1,x2))
         nx = min(x1,x2);
      else if (px > max(x1,x2))
         nx = max(x1,x2);
      else
         nx = px;

      if (py <min(y1,y2))
         ny = min(y1,y2);
      else if (py > max(y1,y2))
         ny = max(y1,y2);
      else
         ny = py;

      if (pz <min(z1,z2))
         nz = min(z1,z2);
      else if (pz > max(z1,z2))
         nz = max(z1,z2);
      else
         nz = pz;
   }

   template <typename T>
   inline point3d<T> closest_point_on_box_from_point(const T& x1, const T& y1, const T& z1,
                                                     const T& x2, const T& y2, const T& z2,
                                                     const T& px, const T& py, const T& pz)
   {
      point3d<T> point;
      closest_point_on_box_from_point(x1,y1,z1,x2,y2,z2,px,py,pz,point.x,point.y,point.z);
      return point;
   }

   template <typename T>
   inline point3d<T> closest_point_on_box_from_point(const box<T,3>& box, const T& px, const T& py, const T& pz)
   {
      return closest_point_on_box_from_point(box[0].x,box[0].y,box[0].z,
                                                   box[1].x,box[1].y,box[1].z,
                                                         px,      py,      pz);
   }

   template <typename T>
   inline point3d<T> closest_point_on_box_from_point(const box<T,3>& box, const point3d<T>& point)
   {
      return closest_point_on_box_from_point(box[0].x,box[0].y,box[0].z,
                                             box[1].x,box[1].y,box[1].z,
                                              point.x, point.y, point.z);
   }

   template <typename T>
   inline void closest_point_on_quadix_from_point(const T& x1, const T& y1,
                                                  const T& x2, const T& y2,
                                                  const T& x3, const T& y3,
                                                  const T& x4, const T& y4,
                                                  const T& px, const T& py,
                                                        T& nx,       T& ny)
   {
      nx = px;
      ny = py;

      if (point_in_quadix(px,py,x1,y1,x2,y2,x3,y3,x4,y4))
      {
         return;
      }

      T tx;
      T ty;
      T temp_dist;

      closest_point_on_segment_from_point(x1,y1,x2,y2,px,py,nx,ny);

      T min_dist = distance(nx,ny,px,py);

      closest_point_on_segment_from_point(x2,y2,x3,y3,px,py,tx,ty);
      temp_dist = distance(tx,ty,px,py);

      if (min_dist > temp_dist)
      {
         min_dist = temp_dist;
         nx = tx;
         ny = ty;
      }

      closest_point_on_segment_from_point(x3,y3,x4,y4,px,py,tx,ty);

      temp_dist = distance(tx,ty,px,py);

      if (min_dist > temp_dist)
      {
         min_dist = temp_dist;
         nx = tx;
         ny = ty;
      }

      closest_point_on_segment_from_point(x4,y4,x1,y1,px,py,tx,ty);

      temp_dist = distance(tx,ty,px,py);

      if (min_dist > temp_dist)
      {
         nx = tx;
         ny = ty;
      }
   }

   template <typename T>
   inline point2d<T> closest_point_on_quadix_from_point(const T& x1, const T& y1,
                                                        const T& x2, const T& y2,
                                                        const T& x3, const T& y3,
                                                        const T& x4, const T& y4,
                                                        const T& px, const T& py)
   {
      point2d<T> point;

      closest_point_on_quadix_from_point(x1,y1,x2,y2,x3,y3,x4,y4,px,py,point.x,point.y);

      return point;
   }

   template <typename T>
   inline point2d<T> closest_point_on_quadix_from_point(const quadix<T,2>& quadix,
                                                        const point2d<T>&  point)
   {
      return closest_point_on_quadix_from_point(quadix[0].x, quadix[0].y,
                                                quadix[1].x, quadix[1].y,
                                                quadix[2].x, quadix[2].y,
                                                quadix[3].x, quadix[3].y,
                                                    point.x,     point.y);
   }

   template <typename T>
   inline point2d<T> closest_point_on_circle_from_point(const circle<T>&  circle,
                                                        const point2d<T>& point)
   {
      const T dx = point.x - circle.x;
      const T dy = point.y - circle.y;

      if ((sqr(dx) + sqr(dy)) <= sqr(circle.radius))
      {
         return point;
      }

      point2d<T> point_;

      const T ratio  = circle.radius / sqrt(dx * dx + dy * dy);

      point_.x = circle.x + ratio * dx;
      point_.y = circle.y + ratio * dy;

      return point_;
   }

   template <typename T>
   inline point3d<T> closest_point_on_sphere_from_point(const sphere<T>&  sphere,
                                                        const point3d<T>& point)
   {
      point3d<T> point_;

      const T dx    = point.x - sphere.x;
      const T dy    = point.y - sphere.y;
      const T dz    = point.z - sphere.z;
      const T ratio = sphere.radius / sqrt(dx * dx + dy * dy);

      point_.x = sphere.x + ratio * dx;
      point_.y = sphere.y + ratio * dy;
      point_.z = sphere.z + ratio * dz;

      return point_;
   }

   template <typename T>
   inline point2d<T> closest_point_on_aabbb_from_point(const rectangle<T>& rectangle,
                                                       const point2d<T>&   point)
   {
      return closest_point_on_rectangle_from_point(rectangle,point);
   }

   template <typename T>
   inline point2d<T> closest_point_on_circle_from_segment(const circle<T>&    circle,
                                                          const segment<T,2>& segment)
   {
      T nx = T(0.0);
      T ny = T(0.0);

      point2d<T> point;

      closest_point_on_segment_from_point
      (
        segment[0].x, segment[0].y,
        segment[1].x, segment[1].y,
        circle.x, circle.y,
        nx, ny
      );

      const T ratio = circle.radius / distance(circle.x,circle.y,nx,ny);

      point.x = circle.x + ratio * (nx - circle.x);
      point.y = circle.y + ratio * (ny - circle.y);

      return point;
   }

   template <typename T>
   inline point3d<T> closest_point_on_sphere_from_segment(const sphere<T>&    sphere,
                                                          const segment<T,3>& segment)
   {
      T nx = T(0.0);
      T ny = T(0.0);
      T nz = T(0.0);

      point3d<T> point;

      closest_point_on_segment_from_point(segment[0].x, segment[0].y, segment[0].z,
                                          segment[1].x, segment[1].y, segment[1].z,
                                              sphere.x,     sphere.y,     sphere.z,
                                                    nx,           ny,           nz);

      const T ratio = sphere.radius / distance(sphere.x,sphere.y,nx,ny);

      point.x = sphere.x + ratio * (nx - sphere.x);
      point.y = sphere.y + ratio * (ny - sphere.y);
      point.z = sphere.z + ratio * (nz - sphere.z);

      return point;
   }

   template <typename T>
   inline point3d<T> closest_point_on_plane_from_point(const plane<T,3>& plane,
                                                       const point3d<T>& point)
   {
      const T mu = plane.normal.x * point.x +
                   plane.normal.y * point.y +
                   plane.normal.z * point.z  - plane.constant;

      if (is_equal(mu,T(0.0)))
         return point;
      else
         return make_point(point.x - mu * plane.normal.x,
                           point.y - mu * plane.normal.y,
                           point.z - mu * plane.normal.z);
   }

   template <typename T>
   inline point2d<T> closest_point_on_bezier_from_point(const quadratic_bezier<T,2>& bezier,
                                                        const point2d<T>& point,
                                                        const std::size_t& steps)
   {
      typedef point2d<T> point_type;

      T smallest_distance = +infinity<T>();

      point_type closest_point = degenerate_point2d<T>();

      std::vector<point_type> point_list;
      point_list.reserve(steps);

      wykobi::generate_bezier(bezier,std::back_inserter(point_list),steps);

      for (std::size_t i = 0; i < (point_list.size() - 1); ++i)
      {
         point_type current_point = closest_point_on_segment_from_point(make_segment(point_list[i],point_list[i+1]),point);

         const T current_distance = distance(current_point,point);

         if (current_distance < smallest_distance)
         {
            closest_point = current_point;
            smallest_distance = current_distance;
         }
      }

      return closest_point;
   }

   template <typename T>
   inline point2d<T> closest_point_on_bezier_from_point(const cubic_bezier<T,2>& bezier,
                                                        const point2d<T>& point,
                                                        const std::size_t& steps)
   {
      typedef point2d<T> point_type;

      T smallest_distance = +infinity<T>();

      point_type closest_point = degenerate_point2d<T>();

      std::vector<point_type> point_list;
      point_list.reserve(steps);

      wykobi::generate_bezier(bezier,std::back_inserter(point_list),steps);

      for (std::size_t i = 0; i < (point_list.size() - 1); ++i)
      {
         point_type current_point = closest_point_on_segment_from_point(make_segment(point_list[i],point_list[i+1]),point);

         const T current_distance = distance(current_point,point);

         if (current_distance < smallest_distance)
         {
            closest_point = current_point;
            smallest_distance = current_distance;
         }
      }

      return closest_point;
   }

   template <typename T>
   inline point3d<T> closest_point_on_bezier_from_point(const quadratic_bezier<T,3>& bezier,
                                                        const point3d<T>& point,
                                                        const std::size_t& steps)
   {
      typedef point3d<T> point_type;

      T smallest_distance = +infinity<T>();

      point_type closest_point = degenerate_point3d<T>();

      std::vector<point_type> point_list;
      point_list.reserve(steps);

      wykobi::generate_bezier(bezier,std::back_inserter(point_list),steps);

      for (std::size_t i = 0; i < (point_list.size() - 1); ++i)
      {
         point_type current_point = closest_point_on_segment_from_point
                                    (
                                      make_segment(point_list[i], point_list[i + 1]),
                                      point
                                    );

         const T current_distance = distance(current_point,point);

         if (current_distance < smallest_distance)
         {
            closest_point = current_point;
            smallest_distance = current_distance;
         }
      }

      return closest_point;
   }

   template <typename T>
   inline point3d<T> closest_point_on_bezier_from_point(const cubic_bezier<T,3>& bezier,
                                                        const point3d<T>& point,
                                                        const std::size_t& steps)
   {
      typedef point3d<T> point_type;

      T smallest_distance = +infinity<T>();

      point_type closest_point = degenerate_point3d<T>();

      std::vector<point_type> point_list;

      point_list.reserve(steps);

      wykobi::generate_bezier(bezier,std::back_inserter(point_list),steps);

      for (std::size_t i = 0; i < (point_list.size() - 1); ++i)
      {
         point_type current_point = closest_point_on_segment_from_point
                                    (
                                      make_segment(point_list[i], point_list[i + 1]),
                                      point
                                    );

         const T current_distance = distance(current_point, point);

         if (current_distance < smallest_distance)
         {
            closest_point = current_point;
            smallest_distance = current_distance;
         }
      }

      return closest_point;
   }

   template <typename T>
   inline point2d<T> closest_point_on_circle_from_circle(const circle<T>& circle1, const circle<T>& circle2)
   {
      return closest_point_on_circle_from_point
             (
               circle1,
               closest_point_on_circle_from_point
               (
                 circle2, make_point(circle1)
               )
             );
   }

   template <typename T>
   inline point3d<T> closest_point_on_sphere_from_sphere(const sphere<T>& sphere1, const sphere<T>& sphere2)
   {
      return closest_point_on_sphere_from_point
             (
               sphere1,
               closest_point_on_sphere_from_point
               (
                 sphere2,
                 make_point(sphere1)
               )
             );
   }

   template <typename T>
   inline point2d<T> closest_point_on_polygon_from_point(const polygon<T,2>& polygon, const point2d<T>& point)
   {
      if (polygon.size() < 3) return degenerate_point2d<T>();

      if (point_in_polygon(point,polygon)) return point;

      std::size_t j = polygon.size() - 1;
      T min_dist = std::numeric_limits<T>::max();

      point2d<T> closest_point = degenerate_point2d<T>();

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         point2d<T> curr_point = closest_point_on_segment_from_point
                                 (
                                   make_segment(polygon[i], polygon[j]),
                                   point
                                 );

         const T curr_dist = distance(point,curr_point);

         if (curr_dist < min_dist)
         {
            closest_point = curr_point;
            min_dist = curr_dist;
         }

         j = i;
      }

      return closest_point;
   }

   template <typename T>
   inline T minimum_distance_from_point_to_segment(const T& px, const T& py,
                                                   const T& x1, const T& y1,
                                                   const T& x2, const T& y2)
   {
      T nx = T(0.0);
      T ny = T(0.0);

      closest_point_on_segment_from_point(x1,y1,x2,y2,px,py,nx,ny);

      return distance(px,py,nx,ny);
   }

   template <typename T>
   inline T minimum_distance_from_point_to_segment(const T& px, const T& py, const T& pz,
                                                   const T& x1, const T& y1, const T& z1,
                                                   const T& x2, const T& y2, const T& z2)
   {
      T nx = T(0.0);
      T ny = T(0.0);
      T nz = T(0.0);

      closest_point_on_segment_from_point(x1,y1,z1,x2,y2,z2,px,py,pz,nx,ny,nz);

      return distance(px,py,pz,nx,ny,nz);
   }

   template <typename T>
   inline T minimum_distance_from_point_to_segment(const point2d<T>& point, const segment<T,2>& segment)
   {
      return minimum_distance_from_point_to_segment
             (
                    point.x,      point.y,
               segment[0].x, segment[0].y,
               segment[1].x, segment[1].y
             );
   }

   template <typename T>
   inline T minimum_distance_from_point_to_segment(const point3d<T>& point, const segment<T,3>& segment)
   {
      return minimum_distance_from_point_to_segment
             (
                    point.x,      point.y,      point.z,
               segment[0].x, segment[0].y, segment[0].z,
               segment[1].x, segment[1].y, segment[1].z
             );
   }

   template <typename T>
   inline T minimum_distance_from_point_to_line(const T& px, const T& py,
                                                const T& x1, const T& y1,
                                                const T& x2, const T& y2)
   {
      T nx = T(0.0);
      T ny = T(0.0);

      closest_point_on_line_from_point(x1,y1,x2,y2,px,py,nx,ny);

      return distance(px, py, nx, ny);
   }

   template <typename T>
   inline T minimum_distance_from_point_to_line(const T& px, const T& py, const T& pz,
                                                const T& x1, const T& y1, const T& z1,
                                                const T& x2, const T& y2, const T& z2)
   {
      T nx = T(0.0);
      T ny = T(0.0);
      T nz = T(0.0);

      closest_point_on_line_from_point(x1,y1,z1,x2,y2,z2,px,py,pz,nx,ny,nz);

      return distance(px, py, pz, nx, ny, nz);
   }

   template <typename T>
   inline T minimum_distance_from_point_to_line(const point2d<T>& point, const line<T,2>& line)
   {
      return minimum_distance_from_point_to_line
             (
               line[0].x, line[0].y,
               line[1].x, line[1].y,
                 point.x,   point.y
             );
   }

   template <typename T>
   inline T minimum_distance_from_point_to_line(const point3d<T>& point, const line<T,3>& line)
   {
      return minimum_distance_from_point_to_line
             (
               line[0].x, line[0].y, line[0].z,
               line[1].x, line[1].y, line[1].z,
                 point.x,   point.y,   point.z
             );
   }

   template <typename T>
   inline T minimum_distance_from_point_to_triangle(const T& px, const T& py,
                                                    const T& x1, const T& y1,
                                                    const T& x2, const T& y2,
                                                    const T& x3, const T& y3)
   {
      T nx = T(0.0);
      T ny = T(0.0);

      closest_point_on_triangle_from_point(x1,y1,x2,y2,x3,y3,px,py,nx,ny);

      return distance(px,py,nx,ny);
   }

   template <typename T>
   inline T minimum_distance_from_point_to_triangle(const point2d<T>& point, const triangle<T,2>& triangle)
   {
      return minimum_distance_from_point_to_triangle
             (
                     point.x,       point.y,
               triangle[0].x, triangle[0].y,
               triangle[1].x, triangle[1].y,
               triangle[2].x, triangle[2].y
             );
   }

   template <typename T>
   inline T minimum_distance_from_point_to_rectangle(const T& px, const T& py,
                                                     const T& x1, const T& y1,
                                                     const T& x2, const T& y2)
   {
      T nx = T(0.0);
      T ny = T(0.0);

      closest_point_on_rectangle_from_point(x1,y1,x2,y2,px,py,nx,ny);

      return distance(px,py,nx,ny);
   }

   template <typename T>
   inline T minimum_distance_from_point_to_rectangle(const point2d<T>& point, const rectangle<T>& rectangle)
   {
      return minimum_distance_from_point_to_rectangle(rectangle[0].x, rectangle[0].y,
                                                      rectangle[1].x, rectangle[1].y,
                                                             point.x,        point.y);
   }

   template <typename T>
   inline void segment_mid_point(const T&   x1, const T&   y1,
                                 const T&   x2, const T&   y2,
                                       T& midx,       T& midy)
   {
      midx = (x1 + x2) * T(0.5);
      midy = (y1 + y2) * T(0.5);
   }

   template <typename T>
   inline void segment_mid_point(const segment<T,2>& segment, T& midx, T& midy)
   {
      segment_mid_point(segment[0].x,segment[0].y,
                        segment[1].x,segment[1].y,
                                midx,        midy);
   }

   template <typename T>
   inline point2d<T> segment_mid_point(const point2d<T>& point1, const point2d<T>& point2)
   {
      point2d<T> point_;
      segment_mid_point(point1.x,point1.y,point2.x,point2.y,point_.x,point_.y);
      return point_;
   }

   template <typename T>
   inline point2d<T> segment_mid_point(const segment<T,2>& segment)
   {
      return segment_mid_point(segment[0],segment[1]);
   }

   template <typename T>
   inline void segment_mid_point(const T&   x1, const T&   y1, const T&   z1,
                                 const T&   x2, const T&   y2, const T&   z2,
                                       T& midx,       T& midy,       T& midz)
   {
      midx = (x1 + x2) * T(0.5);
      midy = (y1 + y2) * T(0.5);
      midz = (z1 + z2) * T(0.5);
   }

   template <typename T>
   inline void segment_mid_point(const segment<T,3>& segment, T& midx, T& midy, T& midz)
   {
      segment_mid_point(segment[0].x, segment[0].y, segment[0].z,
                        segment[1].x, segment[1].y, segment[1].z,
                                midx,         midy,         midz);
   }

   template <typename T>
   inline point3d<T> segment_mid_point(const point3d<T>& point1, const point3d<T>& point2)
   {
      point3d<T> point_;

      segment_mid_point(point1.x, point1.y, point1.z,
                        point2.x, point2.y, point2.z,
                        point_.x, point_.y, point_.z);

      return point_;
   }

   template <typename T>
   inline point3d<T> segment_mid_point(const segment<T,3>& segment)
   {
      return segment_mid_point(segment[0],segment[1]);
   }

   template <typename T>
   inline void centroid(const T& x1, const T& y1,
                        const T& x2, const T& y2,
                              T&  x,       T&  y)
   {
      x = (x1 + x2) * T(0.5);
      y = (y1 + y2) * T(0.5);
   }

   template <typename T>
   inline point2d<T> centroid(const point2d<T>& point1, const point2d<T>& point2)
   {
      point2d<T> point_;
      centroid(point1.x,point1.y,point2.x,point2.y,point_.x,point_.y);
      return point_;
   }

   template <typename T>
   inline point2d<T> centroid(const segment<T,2>& segment)
   {
      return segment_mid_point(segment[0],segment[1]);
   }

   template <typename T>
   inline void centroid(const T& x1, const T& y1,
                        const T& x2, const T& y2,
                        const T& x3, const T& y3,
                              T&  x,       T&  y)
   {
      T midx1 = T(0.0);
      T midy1 = T(0.0);
      T midx2 = T(0.0);
      T midy2 = T(0.0);

      segment_mid_point(x2,y2,x3,y3,midx1,midy1);
      segment_mid_point(x1,y1,x3,y3,midx2,midy2);

      intersect(x1,y1,midx1,midy1,x2,y2,midx2,midy2,x,y);
   }

   template <typename T>
   inline void centroid(const T& x1, const T& y1, const T& z1,
                        const T& x2, const T& y2, const T& z2,
                        const T& x3, const T& y3, const T& z3,
                              T&  x,       T&  y,       T& z)
   {
      T midx1 = T(0.0);
      T midy1 = T(0.0);
      T midz1 = T(0.0);
      T midx2 = T(0.0);
      T midy2 = T(0.0);
      T midz2 = T(0.0);

      segment_mid_point(x2,y2,z2,x3,y3,z3,midx1,midy1,midz1);
      segment_mid_point(x1,y1,z1,x3,y3,z3,midx2,midy2,midz2);

      intersection_point(x1,y1,z1,midx1,midy1,midz1,x2,y2,z2,midx2,midy2,midz2,x,y,z);
   }

   template <typename T>
   inline void centroid(const T& x1, const T& y1,
                        const T& x2, const T& y2,
                        const T& x3, const T& y3,
                        const T& x4, const T& y4,
                              T&  x,       T&  y)
   {
      x = 0.0;
      y = 0.0;

      T asum = 0.0;
      T term = 0.0;

      term = ((x4 * y1) - (y4 * x1));
      asum = asum + term;
      x = x + (x4 + x1) * term;
      y = y + (y4 + y1) * term;

      term = ((x1 * y2) - (y1 * x2));
      asum = asum + term;
      x = x + (x1 + x2) * term;
      y = y + (y1 + y2) * term;

      term = ((x2 * y3) - (y2 * x3));
      asum = asum + term;
      x = x + (x2 + x3) * term;
      y = y + (y2 + y3) * term;

      term = ((x3 * y4) - (y3 * x4));
      asum = asum + term;
      x = x + (x3 + x4) * term;
      y = y + (y3 + y4) * term;

      if (asum != T(0.0))
      {
         x = x / (T(3.0) * asum);
         y = y / (T(3.0) * asum);
      }
   }

   template <typename T>
   inline void centroid(const triangle<T,2>& triangle, T& x, T& y)
   {
      centroid(triangle[0].x,triangle[0].y,
               triangle[1].x,triangle[1].y,
               triangle[2].x,triangle[2].y,
                           x,            y);
   }

   template <typename T>
   inline void centroid(const triangle<T,3>& triangle, T& x, T& y, T& z)
   {
      centroid(triangle[0].x,triangle[0].y,triangle[0].z,
               triangle[1].x,triangle[1].y,triangle[1].z,
               triangle[2].x,triangle[2].y,triangle[2].z,
                           x,            y,            z);
   }

   template <typename T>
   inline void centroid(const quadix<T,2>& quadix, T& x, T& y)
   {
      centroid(quadix[0].x,quadix[0].y,
               quadix[1].x,quadix[1].y,
               quadix[2].x,quadix[2].y,
               quadix[3].x,quadix[3].y,
                         x,          y);
   }

   template <typename T>
   inline void centroid(const rectangle<T>& rectangle, T& x, T& y)
   {
      x = (rectangle[0].x + rectangle[1].x) * T(0.5);
      y = (rectangle[0].y + rectangle[1].y) * T(0.5);
   }

   template <typename T>
   inline void centroid(const box<T,3>& box, T& x, T& y, T& z)
   {
      x = (box[0].x + box[1].x) * T(0.5);
      y = (box[0].y + box[1].y) * T(0.5);
      z = (box[0].z + box[1].z) * T(0.5);
   }

   template <typename T>
   inline void centroid(const polygon<T,2>& polygon, T& x, T& y)
   {
      x = T(0.0);
      y = T(0.0);

      if (polygon.size() < 3) return;

      T asum = T(0.0);
      std::size_t j = polygon.size() - 1;

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         const T term  = ((polygon[j].x * polygon[i].y) - (polygon[j].y * polygon[i].x));

         asum += term;
         x    += ((polygon[j].x + polygon[i].x) * term);
         y    += ((polygon[j].y + polygon[i].y) * term);
         j     = i;
      }

      if (not_equal(asum,T(0.0)))
      {
         x /= (T(3.0) * asum);
         y /= (T(3.0) * asum);
      }
   }

   template <typename T>
   inline point2d<T> centroid(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3)
   {
      point2d<T> point_;

      centroid(point1.x,point1.y,
               point2.x,point2.y,
               point3.x,point3.y,
               point_.x,point_.y);

      return point_;
   }

   template <typename T>
   inline point2d<T> centroid(const triangle<T,2>& triangle)
   {
      point2d<T> point_;
      centroid(triangle,point_.x,point_.y);
      return point_;
   }

   template <typename T>
   inline point3d<T> centroid(const triangle<T,3>& triangle)
   {
      point3d<T> point_;
      centroid(triangle,point_.x,point_.y,point_.z);
      return point_;
   }

   template <typename T>
   inline point2d<T> centroid(const point2d<T>& point1,
                              const point2d<T>& point2,
                              const point2d<T>& point3,
                              const point2d<T>& point4)
   {
      point2d<T> point_;

      centroid
      (
        point1.x, point1.y,
        point2.x, point2.y,
        point3.x, point3.y,
        point4.x, point4.y,
        point_.x, point_.y
      );

      return point_;
   }

   template <typename T>
   inline point2d<T> centroid(const quadix<T,2>& quadix)
   {
      point2d<T> point_;
      centroid(quadix,point_.x,point_.y);
      return point_;
   }

   template <typename T>
   inline point2d<T> centroid(const polygon<T,2>& polygon)
   {
      point2d<T> point_;
      centroid(polygon,point_.x,point_.y);
      return point_;
   }

   template <typename T>
   inline bool point_in_convex_polygon(const T& px, const T& py, const polygon<T,2>& polygon)
   {
      if (polygon.size() < 3) return false;

      int initial_orientation = orientation
                                (
                                  polygon[0],
                                  polygon[static_cast<int>(polygon.size()) - 1],
                                  px, py
                                );
      std::size_t j = 0;

      for (std::size_t i = 1; i < polygon.size(); ++i)
      {
         if (initial_orientation != orientation(polygon[i],polygon[j],px,py))
         {
            return false;
         }

         j = i;
      }

      return true;
   }

   template <typename T>
   inline bool point_in_convex_polygon(const point2d<T>& point, const polygon<T,2>& polygon)
   {
      return point_in_convex_polygon(point.x,point.y,polygon);
   }

   template <typename T>
   inline bool point_on_polygon_edge(const T& px, const T& py, const polygon<T,2>& polygon)
   {
      if (polygon.size() < 3) return false;

      std::size_t j = polygon.size() - 1;

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         if (is_point_collinear(polygon[i],polygon[j],px,py,true))
         {
            return true;
         }

         j = i;
      }

      return false;
   }

   template <typename T>
   inline bool point_on_polygon_edge(const point2d<T>& point, const polygon<T,2>& polygon)
   {
      return point_on_polygon_edge(point.x,point.y,polygon);
   }

   template <typename T>
   inline bool point_in_polygon(const T& px, const T& py, const polygon<T,2>& polygon)
   {
      bool result = false;
      if (polygon.size() < 3) return false;

      std::size_t j = polygon.size() - 1;

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         if (
              ((polygon[i].y <= py) && (py < polygon[j].y)) || // an upward crossing
              ((polygon[j].y <= py) && (py < polygon[i].y))    // a downward crossing
            )
         {
            /* compute the edge-ray intersect @ the x-coordinate */
            if (px - polygon[i].x < ((polygon[j].x - polygon[i].x) * (py - polygon[i].y) / (polygon[j].y - polygon[i].y)))
            {
               result = !result;
            }
         }

         j = i;
      }

      return result;
   }

   template <typename T>
   inline bool point_in_polygon(const point2d<T>& point, const polygon<T,2>& polygon)
   {
      return point_in_polygon(point.x,point.y,polygon);
   }

   template <typename T>
   inline bool point_in_polygon_winding_number(const T& px, const T& py, const polygon<T,2>& polygon)
   {
      int winding_number = 0;
      std::size_t j = polygon.size() - 1;

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         if (polygon[j].y <= py)
         {
            if (
                 (polygon[i].y > py) &&
                 (orientation(polygon[j],polygon[i],px,py) == LeftHandSide)
               )
            {
               winding_number++;
            }
         }
         else
         {
            if (
                 (polygon[i].y <= py) &&
                 (orientation(polygon[j],polygon[i],px,py) == RightHandSide)
               )
            {
               winding_number--;
            }
         }

         j = i;
      }

      return (winding_number != 0);
   }

   template <typename T>
   inline bool point_in_polygon_winding_number(const point2d<T>& point, const polygon<T,2>& polygon)
   {
      return point_in_polygon_winding_number(point.x,point.y,polygon);
   }

   template <typename T>
   inline bool convex_quadix(const quadix<T,2>& quadix)
   {
      const int orin = orientation(quadix[0],quadix[2],quadix[1]);

      if (orin != orientation(quadix[1], quadix[3], quadix[2])) return false;
      if (orin != orientation(quadix[2], quadix[0], quadix[3])) return false;
      if (orin != orientation(quadix[3], quadix[1], quadix[0])) return false;

      return true;
   }

   template <typename T>
   inline bool convex_quadix(const quadix<T,3>& quadix)
   {
      vector3d<T> bda = (quadix[3] - quadix[1]) * (quadix[0] - quadix[1]);
      vector3d<T> bdc = (quadix[3] - quadix[1]) * (quadix[2] - quadix[1]);

      if (greater_than_or_equal(dot_product(bda,bdc),T(0.0)))
      {
         return false;
      }

      vector3d<T> acd = (quadix[2] - quadix[0]) * (quadix[3] - quadix[0]);
      vector3d<T> acb = (quadix[2] - quadix[0]) * (quadix[1] - quadix[0]);

      return dot_product(acd,acb) < T(0.0);
   }

   template <typename T>
   inline bool is_convex_polygon(const polygon<T,2>& polygon)
   {
      if (polygon.size() < 3)
      {
         return false;
      }

      std::size_t i = 0;
      std::size_t j = polygon.size() - 1;
      std::size_t k = polygon.size() - 2;
      int initial_orientation = 0;

      while ((initial_orientation = orientation(polygon[k],polygon[j],polygon[i])) == CollinearOrientation)
      {
         k = j;
         j = i++;

         if (i >= polygon.size())
         {
            return false;
         }
      }

      while (i < polygon.size())
      {
         if (orientation(polygon[k],polygon[j],polygon[i]) != initial_orientation)
         {
            return false;
         }

         k = j;
         j = i++;

         if (i >= polygon.size()) break;
      }

      return true;
   }

   template <typename T>
   inline polygon<T,2> remove_consecutive_collinear_points(const polygon<T,2>& polygon)
   {
      wykobi::polygon<T,2> polygon_;

      point2d<T> previous_point = polygon[polygon.size() - 1];

      for (std::size_t i = 0; i < polygon.size() - 1; ++i)
      {
         if (orientation(previous_point,polygon[i],polygon[i + 1]) != CollinearOrientation)
         {
            polygon_.push_back(polygon[i]);
            previous_point = polygon[i];
         }
      }

      if (orientation(previous_point,polygon.back(),polygon.front()) != CollinearOrientation)
      {
         polygon_.push_back(polygon.back());
      }

      return polygon_;
   }

   template <typename T, typename InputIterator, typename OutputIterator>
   inline void remove_consecutive_collinear_points(const InputIterator begin, const InputIterator end,
                                                   OutputIterator out)
   {
      if (std::distance(begin,end) < 3)
      {
         std::copy(begin,end,out);
         return;
      }

      InputIterator prev = begin;
      InputIterator it   = begin + 1;
      InputIterator next = begin + 2;

      while (next != end)
      {
         if (orientation(*prev,*it,*next) != CollinearOrientation)
         {
            (*out++) = *it;
            prev = it;
         }

         it = next;
         ++next;
      }
   }

   template <typename T>
   inline bool convex_vertex(const std::size_t& index, const polygon<T,2>& polygon, const int& polygon_orientation)
   {
      if (0 == index)
      {
         return (orientation(polygon.back(),polygon.front(),polygon[1]) == polygon_orientation);
      }
      else if (index == (polygon.size() - 1))
      {
         return (orientation(polygon[polygon.size() - 2],polygon.back(),polygon.front()) == polygon_orientation);
      }
      else
      {
         return (orientation(polygon[index - 1],polygon[index],polygon[index + 1]) == polygon_orientation);
      }
   }

   template <typename T>
   inline bool collinear_vertex(const std::size_t& index, const polygon<T,2>& polygon)
   {
      if (0 == index)
      {
         return robust_collinear(polygon.back(),polygon.front(),polygon[1]);
      }
      else if (index == (polygon.size() - 1))
      {
         return robust_collinear(polygon[polygon.size() - 2],polygon.back(),polygon.front());
      }
      else
      {
         return robust_collinear(polygon[index - 1],polygon[index],polygon[index + 1]);
      }
   }

   template <typename T>
   inline bool vertex_is_ear(const std::size_t& index, const polygon<T,2>& polygon)
   {
      std::size_t pred_index;
      std::size_t succ_index;

      if (0 == index)
      {
         pred_index = polygon.size() - 1;
         succ_index = 1;
      }
      else if (index == polygon.size() - 1)
      {
         pred_index = polygon.size() - 2;
         succ_index = 0;
      }
      else
      {
         pred_index = index - 1;
         succ_index = index + 1;
      }

      triangle<T,2> triangle = make_triangle(polygon[pred_index],polygon[index],polygon[succ_index]);

      if (robust_collinear(triangle[0],triangle[1],triangle[2]))
      {
         return false;
      }

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         if ((i != pred_index) && (i != succ_index) && (i != index))
         {
            if (point_in_triangle(polygon[i],triangle))
            {
               return false;
            }
         }
      }

      return true;
   }

   template <typename T>
   inline triangle<T,2> vertex_triangle(const std::size_t& index, const polygon<T,2> polygon)
   {
      if (0 == index)
      {
         return make_triangle(polygon.back(),polygon.front(),polygon[1]);
      }
      else if (index == (polygon.size() - 1))
      {
         return make_triangle(polygon[polygon.size() - 2],polygon.back(),polygon.front());
      }
      else
      {
         return make_triangle(polygon[index - 1],polygon[index],polygon[index + 1]);
      }
   }

   template <typename T>
   inline int polygon_orientation(const polygon<T,2>& polygon)
   {
      if (polygon.size() < 3)
      {
         return 0;
      }

      T area = T(0.0);

      std::size_t prev_index = polygon.size() - 1;

      for (std::size_t index = 0; index < polygon.size(); ++index)
      {
         area += (polygon[prev_index].x * polygon[index].y - polygon[index].x * polygon[prev_index].y);
         prev_index = index;
      }

      return ((greater_than_or_equal(area,T(0.0))) ? CounterClockwise : Clockwise);

      /*
      std::size_t anchor = 0;

      for (std::size_t i = 1; i < polygon.size(); ++i)
      {
         if (polygon[i].x > polygon[anchor].x)
            anchor = i;
         else if ((polygon[i].x == polygon[anchor].x) && (polygon[i].y  < polygon[anchor].y))
            anchor = i;
      }

      if (0 == anchor)
      {
         return orientation(*(polygon.end() - 1),*polygon.end(),*polygon.begin());
      }
      else if (anchor == (polygon.size() - 1))
      {
         return orientation(*(polygon.end() - 2),*(polygon.end() - 1),*polygon.end());
      }
      else
      {
         return orientation(polygon[anchor - 2],polygon[anchor - 1],polygon[anchor]);
      }
      */
   }

   template <typename T>
   inline bool is_equilateral_triangle(const triangle<T,2>& triangle)
   {
      const T dist = distance(triangle[0],triangle[1]);

      return is_equal(distance(triangle[1], triangle[2]), dist) &&
             is_equal(distance(triangle[0], triangle[2]), dist);
   }

   template <typename T>
   inline bool is_equilateral_triangle(const triangle<T,3>& triangle)
   {
      const T dist = distance(triangle[0],triangle[1]);

      return is_equal(distance(triangle[1], triangle[2]), dist) &&
             is_equal(distance(triangle[0], triangle[2]), dist);
   }

   template <typename T>
   inline bool is_isosceles_triangle(const triangle<T,2>& triangle)
   {
      const T dist1 = lay_distance(triangle[0],triangle[1]);
      const T dist2 = lay_distance(triangle[1],triangle[2]);
      const T dist3 = lay_distance(triangle[2],triangle[0]);

      return is_equal(dist1,dist2) || is_equal(dist1,dist3) || is_equal(dist2,dist3);
   }

   template <typename T>
   inline bool is_isosceles_triangle(const triangle<T,3>& triangle)
   {
      const T dist1 = lay_distance(triangle[0],triangle[1]);
      const T dist2 = lay_distance(triangle[1],triangle[2]);
      const T dist3 = lay_distance(triangle[2],triangle[0]);

      return is_equal(dist1,dist2) || is_equal(dist1,dist3) || is_equal(dist2,dist3);
   }

   template <typename T>
   inline bool is_right_triangle(const wykobi::triangle<T,2>& triangle)
   {
      const T a = wykobi::lay_distance(wykobi::edge(triangle,0));
      const T b = wykobi::lay_distance(wykobi::edge(triangle,1));
      const T c = wykobi::lay_distance(wykobi::edge(triangle,2));

      return wykobi::is_equal(a + b,c) || wykobi::is_equal(a + c,b) || wykobi::is_equal(b + c,a);
   }

   template <typename T>
   inline bool is_right_triangle(const wykobi::triangle<T,3>& triangle)
   {
      const T a = wykobi::lay_distance(wykobi::edge(triangle,0));
      const T b = wykobi::lay_distance(wykobi::edge(triangle,1));
      const T c = wykobi::lay_distance(wykobi::edge(triangle,2));

      return wykobi::is_equal(a + b,c) || wykobi::is_equal(a + c,b) || wykobi::is_equal(b + c,a);
   }

   template <typename T>
   inline bool are_perspective_triangles(const triangle<T,2>& triangle1, const triangle<T,2>& triangle2)
   {
      for (std::size_t i = 0; i < 3; ++i)
      {
         if (robust_parallel(edge(triangle1,i),edge(triangle2,i)))
         {
            return false;
         }
      }

      return robust_collinear
             (
               intersection_point(make_line(edge(triangle1,0)),make_line(edge(triangle2,0))),
               intersection_point(make_line(edge(triangle1,1)),make_line(edge(triangle2,1))),
               intersection_point(make_line(edge(triangle1,2)),make_line(edge(triangle2,2)))
             );
   }

   template <typename T>
   inline bool are_perspective_triangles(const triangle<T,3>& triangle1, const triangle<T,3>& triangle2)
   {
      for (std::size_t i = 0; i < 3; ++i)
      {
         if (robust_parallel(edge(triangle1,i),edge(triangle2,i)))
         {
            return false;
         }
      }

      return robust_collinear
             (
               intersection_point(make_line(edge(triangle1,0)),make_line(edge(triangle2,0))),
               intersection_point(make_line(edge(triangle1,1)),make_line(edge(triangle2,1))),
               intersection_point(make_line(edge(triangle1,2)),make_line(edge(triangle2,2)))
             );
   }

   template <typename T>
   inline line<T,2> perspectrix(const triangle<T,2>& triangle1, const triangle<T,2>& triangle2)
   {
      for (std::size_t i = 0; i < 3; ++i)
      {
         if (robust_parallel(edge(triangle1,i),edge(triangle2,i)))
         {
            return degenerate_line2d<T>();
         }
      }

      point2d<T> ipoint0 = intersection_point(make_line(edge(triangle1,0)),make_line(edge(triangle2,0)));
      point2d<T> ipoint1 = intersection_point(make_line(edge(triangle1,1)),make_line(edge(triangle2,1)));
      point2d<T> ipoint2 = intersection_point(make_line(edge(triangle1,2)),make_line(edge(triangle2,2)));

      return (robust_collinear(ipoint0,ipoint1,ipoint2)) ? make_line(ipoint0,ipoint1) : degenerate_line2d<T>();
   }

   template <typename T>
   inline line<T,3> perspectrix(const triangle<T,3>& triangle1, const triangle<T,3>& triangle2)
   {
      for (std::size_t i = 0; i < 3; ++i)
      {
         if (robust_parallel(edge(triangle1,i),edge(triangle2,i)))
         {
            return degenerate_line3d<T>();
         }
      }

      point3d<T> ipoint0 = intersection_point(make_line(edge(triangle1,0)),make_line(edge(triangle2,0)));
      point3d<T> ipoint1 = intersection_point(make_line(edge(triangle1,1)),make_line(edge(triangle2,1)));
      point3d<T> ipoint2 = intersection_point(make_line(edge(triangle1,2)),make_line(edge(triangle2,2)));

      if (robust_collinear(ipoint0, ipoint1, ipoint2))
         return make_line(ipoint0, ipoint1);
      else
         return degenerate_line3d<T>();
   }

   template <typename T>
   inline point2d<T> centroid(const rectangle<T>& rectangle)
   {
      point2d<T> point_;
      centroid(rectangle,point_.x,point_.y);
      return point_;
   }

   template <typename T>
   inline point3d<T> centroid(const box<T,3>& box)
   {
      point3d<T> point_;
      centroid(box,point_.x,point_.y,point_.z);
      return point_;
   }

   template <typename T>
   inline void mirror(const T& px, const T& py,
                      const T& x1, const T& y1,
                      const T& x2, const T& y2,
                            T& nx,       T& ny)
   {
      closest_point_on_line_from_point(x1,y1,x2,y2,px,py,nx,ny);

      nx = px + T(2.0) * (nx - px);
      ny = py + T(2.0) * (ny - py);
   }

   template <typename T>
   inline void mirror(const T& px, const T& py, const T& pz,
                      const T& x1, const T& y1, const T& z1,
                      const T& x2, const T& y2, const T& z2,
                            T& nx,       T& ny,       T& nz)
   {
      closest_point_on_line_from_point(x1,y1,z1,x2,y2,z2,px,py,pz,nx,ny,nz);

      nx = px + T(2.0) * (nx - px);
      ny = py + T(2.0) * (ny - py);
      nz = pz + T(2.0) * (nz - pz);
   }

   template <typename T>
   inline point2d<T> mirror(const point2d<T>& point, const line<T,2>& mirror_axis)
   {
      wykobi::point2d<T> point_;

      mirror(point.x,          point.y,
             mirror_axis[0].x, mirror_axis[0].y,
             mirror_axis[1].x, mirror_axis[1].y,
             point_.x,         point_.y);

      return point_;
   }

   template <typename T>
   inline segment<T,2> mirror(const segment<T,2>& segment, const line<T,2>& mirror_axis)
   {
      wykobi::segment<T,2> segment_;

      for (std::size_t i = 0; i < wykobi::segment<T,2>::PointCount; ++i)
      {
         segment_[i] = mirror(segment[i],mirror_axis);
      }

      return segment_;
   }

   template <typename T>
   inline line<T,2> mirror(const line<T,2>& line, const wykobi::line<T,2>& mirror_axis)
   {
      wykobi::line<T,2> line_;

      for (std::size_t i = 0; i < wykobi::line<T,2>::PointCount; ++i)
      {
         line_[i] = mirror(line[i],mirror_axis);
      }

      return line_;
   }

   template <typename T>
   inline rectangle<T> mirror(const rectangle<T>& rectangle, const line<T,2>& mirror_axis)
   {
      wykobi::rectangle<T> rectangle_;

      for (std::size_t i = 0; i < wykobi::rectangle<T>::PointCount; ++i)
      {
         rectangle_[i] = mirror(rectangle[i],mirror_axis);
      }

      return rectangle_;
   }

   template <typename T>
   inline triangle<T,2> mirror(const triangle<T,2>& triangle, const line<T,2>& mirror_axis)
   {
      wykobi::triangle<T,2> triangle_;

      for (std::size_t i = 0; i < wykobi::triangle<T,2>::PointCount; ++i)
      {
         triangle_[i] = mirror(triangle[i],mirror_axis);
      }

      return triangle_;
   }

   template <typename T>
   inline quadix<T,2> mirror(const quadix<T,2>& quadix, const line<T,2>& mirror_axis)
   {
      wykobi::quadix<T,2> quadix_;

      for (std::size_t i = 0; i < wykobi::quadix<T,2>::PointCount; ++i)
      {
         quadix_[i] = mirror(quadix[i],mirror_axis);
      }

      return quadix_;
   }

   template <typename T>
   inline circle<T> mirror(const circle<T>& circle, const line<T,2>& mirror_axis)
   {
      wykobi::circle<T> circle_ = circle;

      mirror(circle.x,         circle.y,
             mirror_axis[0].x, mirror_axis[0].y,
             mirror_axis[1].x, mirror_axis[1].y,
             circle_.x,        circle_.y);

      return circle_;
   }

   template <typename T>
   inline polygon<T,2> mirror(const polygon<T,2>& polygon, const line<T,2>& mirror_axis)
   {
      wykobi::polygon<T,2> polygon_;

      polygon_.reserve(polygon.size());

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         polygon_.push_back(mirror(polygon[i],mirror_axis));
      }

      return polygon_;
   }

   template <typename T>
   inline point3d<T> mirror(const point3d<T>& point, const line<T,3>& mirror_axis)
   {
      wykobi::point3d<T> point_;

      mirror(point.x,          point.y,
             mirror_axis[0].x, mirror_axis[0].y,
             mirror_axis[1].x, mirror_axis[1].y,
             point_.x,         point_.y);

      return point_;
   }

   template <typename T>
   inline segment<T,3> mirror(const segment<T,3>& segment, const line<T,3>& mirror_axis)
   {
      wykobi::segment<T,3> segment_;

      for (std::size_t i = 0; i < wykobi::segment<T,3>::PointCount; ++i)
      {
         segment_[i] = mirror(segment[i],mirror_axis);
      }

      return segment_;
   }

   template <typename T>
   inline line<T,3> mirror(const line<T,3>& line, const wykobi::line<T,3>& mirror_axis)
   {
      wykobi::line<T,3> line_;

      for (std::size_t i = 0; i < wykobi::line<T,3>::PointCount; ++i)
      {
         line_[i] = mirror(line[i],mirror_axis);
      }

      return line_;
   }

   template <typename T>
   inline box<T,3> mirror(const box<T,3>& box, const line<T,3>& mirror_axis)
   {
      wykobi::box<T,3> box_;

      for (std::size_t i = 0; i < wykobi::box<T,3>::PointCount; ++i)
      {
         box_[i] = mirror(box[i],mirror_axis);
      }

      return box_;
   }

   template <typename T>
   inline triangle<T,3> mirror(const triangle<T,3>& triangle, const line<T,3>& mirror_axis)
   {
      wykobi::triangle<T,3> triangle_;

      for (std::size_t i = 0; i < wykobi::triangle<T,3>::PointCount; ++i)
      {
         triangle_[i] = mirror(triangle[i],mirror_axis);
      }

      return triangle_;
   }

   template <typename T>
   inline quadix<T,3> mirror(const quadix<T,3>& quadix, const line<T,3>& mirror_axis)
   {
      wykobi::quadix<T,3> quadix_;

      for (std::size_t i = 0; i < wykobi::quadix<T,3>::PointCount; ++i)
      {
         quadix_[i] = mirror(quadix[i],mirror_axis);
      }

      return quadix_;
   }

   template <typename T>
   inline sphere<T> mirror(const sphere<T>& sphere, const line<T,3>& mirror_axis)
   {
      wykobi::sphere<T> sphere_ = sphere;

      mirror(sphere.x,         sphere.y,         sphere.z,
             mirror_axis[0].x, mirror_axis[0].y, mirror_axis[0].z,
             mirror_axis[1].x, mirror_axis[1].y, mirror_axis[1].z,
             sphere_.x,        sphere_.y,        sphere_.z);

      return sphere_;
   }

   template <typename T>
   inline polygon<T,3> mirror(const polygon<T,3>& polygon, const line<T,3>& mirror_axis)
   {
      wykobi::polygon<T,3> polygon_;
      polygon_.reserve(polygon.size());

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         polygon_.push_back(mirror(polygon[i],mirror_axis));
      }

      return polygon_;
   }

   template <typename T>
   inline point3d<T> mirror(const point3d<T>& point, const plane<T,3>& plane)
   {
      return point + T(2.0) * closest_point_on_plane_from_point(plane,point) - point;
   }

   template <typename T>
   inline segment<T,3> mirror(const segment<T,3>& segment, const plane<T,3>& plane)
   {
      wykobi::segment<T,3> segment_;

      for (std::size_t i = 0; i < wykobi::segment<T,3>::PointCount; ++i)
      {
         segment_[i] = mirror(segment[i],plane);
      }

      return segment_;
   }

   template <typename T>
   inline line<T,3> mirror(const line<T,3>& line, const plane<T,3>& plane)
   {
      wykobi::line<T,3> line_;

      for (std::size_t i = 0; i < wykobi::line<T,3>::PointCount; ++i)
      {
         line_[i] = mirror(line[i],plane);
      }

      return line_;
   }

   template <typename T>
   inline box<T,3> mirror(const box<T,3>& box, const plane<T,3>& plane)
   {
      wykobi::box<T,3> box_;

      for (std::size_t i = 0; i < wykobi::box<T,3>::PointCount; ++i)
      {
         box_[i] = mirror(box[i],plane);
      }

      return box_;
   }

   template <typename T>
   inline triangle<T,3> mirror(const triangle<T,3>& triangle, const plane<T,3>& plane)
   {
      wykobi::triangle<T,3> triangle_;

      for (std::size_t i = 0; i < wykobi::triangle<T,3>::PointCount; ++i)
      {
         triangle_[i] = mirror(triangle[i],plane);
      }

      return triangle_;
   }

   template <typename T>
   inline quadix<T,3> mirror(const quadix<T,3>& quadix, const plane<T,3>& plane)
   {
      wykobi::quadix<T,3> quadix_;

      for (std::size_t i = 0; i < wykobi::quadix<T,3>::PointCount; ++i)
      {
         quadix_[i] = mirror(quadix[i],plane);
      }

      return quadix_;
   }

   template <typename T>
   inline sphere<T> mirror(const sphere<T>& sphere, const plane<T,3>& plane)
   {
      point3d<T> center = make_point(sphere);
      return make_sphere(mirror(center,plane),sphere.radius);
   }

   template <typename T>
   inline polygon<T,3> mirror(const polygon<T,3>& polygon, const plane<T,3>& plane)
   {
      wykobi::polygon<T,3> polygon_;

      polygon_.reserve(polygon.size());

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         polygon_.push_back(mirror(polygon[i],plane));
      }

      return polygon_;
   }

   template <typename T>
   inline void nonsymmetric_mirror(const T& px, const T& py,
                                   const T& x1, const T& y1,
                                   const T& x2, const T& y2,
                                   const T& ratio,
                                         T& nx,       T& ny)
   {
      closest_point_on_line_from_point(x1, y1, x2, y2, px, py, nx, ny);

      const T general_ratio = T(2.0) * ratio;

      nx = px + general_ratio * (nx - px);
      ny = py + general_ratio * (ny - py);
   }

   template <typename T>
   inline point2d<T> nonsymmetric_mirror(const point2d<T>& point, const T& ratio, const line<T,2>& line)
   {
      wykobi::point2d<T> point_;

      nonsymmetric_mirror(point.x,point.y,line[0].x,line[0].y,ratio,line[1].x,line[1].y,point_.x,point_.y);

      return point_;
   }

   template <typename T>
   inline segment<T,2> nonsymmetric_mirror(const segment<T,2>& segment, const T& ratio, const line<T,2>& line)
   {
      wykobi::segment<T,2> segment_;

      for (std::size_t i = 0; i < wykobi::segment<T,2>::PointCount; ++i)
      {
         segment_[i] = nonsymmetric_mirror(segment[i],ratio,line);
      }

      return segment_;
   }

   template <typename T>
   inline rectangle<T> nonsymmetric_mirror(const rectangle<T>& rectangle, const T& ratio, const line<T,2>& line)
   {
      wykobi::rectangle<T> rectangle_;

      for (std::size_t i = 0; i < wykobi::rectangle<T>::PointCount; ++i)
      {
         rectangle_[i] = nonsymmetric_mirror(rectangle[i],ratio,line);
      }

      return rectangle_;
   }

   template <typename T>
   inline triangle<T,2> nonsymmetric_mirror(const triangle<T,2>& triangle, const T& ratio, const line<T,2>& line)
   {
      wykobi::triangle<T,2> triangle_;

      for (std::size_t i = 0; i < wykobi::triangle<T,2>::PointCount; ++i)
      {
         triangle_[i] = nonsymmetric_mirror(triangle[i],ratio,line);
      }

      return triangle_;
   }

   template <typename T>
   inline quadix<T,2> nonsymmetric_mirror(const quadix<T,2>& quadix, const T& ratio, const line<T,2>& line)
   {
      wykobi::quadix<T,2> quadix_;

      for (std::size_t i = 0; i < wykobi::quadix<T,2>::PointCount; ++i)
      {
         quadix_[i] = nonsymmetric_mirror(quadix[i],ratio,line);
      }

      return quadix_;
   }

   template <typename T>
   inline circle<T> nonsymmetric_mirror(const circle<T>& circle, const T& ratio, const line<T,2>& line)
   {
      wykobi::circle<T> circle_ = circle;
      nonsymmetric_mirror(circle.x,circle.y,line[0].x,line[0].y,line[1].x,line[1].y,ratio,circle_.x,circle_.y);
      return circle_;
   }

   template <typename T>
   inline polygon<T,2> nonsymmetric_mirror(const polygon<T,2>& polygon, const T& ratio, const line<T,2>& line)
   {
      wykobi::polygon<T,2> polygon_;
      polygon_.reserve(polygon.size());

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         polygon_.push_back(nonsymmetric_mirror(polygon[i],ratio,line));
      }

      return polygon_;
   }

   template <typename T>
   inline point3d<T> nonsymmetric_mirror(const point3d<T>& point, const T& ratio, const plane<T,3>& plane)
   {
      return point + T(2.0) * ratio * closest_point_on_plane_from_point(plane,point) - point;
   }

   template <typename T>
   inline segment<T,3> nonsymmetric_mirror(const segment<T,3>& segment, const T& ratio, const plane<T,3>& plane)
   {
      wykobi::segment<T,3> segment_;

      for (std::size_t i = 0; i < wykobi::segment<T,3>::PointCount; ++i)
      {
         segment_[i] = nonsymmetric_mirror(segment[i],ratio,plane);
      }

      return segment_;
   }

   template <typename T>
   inline box<T,3> nonsymmetric_mirror(const box<T,3>& box, const T& ratio, const plane<T,3>& plane)
   {
      wykobi::box<T,3> box_;

      for (std::size_t i = 0; i < wykobi::box<T,3>::PointCount; ++i)
      {
         box_[i] = nonsymmetric_mirror(box[i],ratio,plane);
      }

      return box_;
   }

   template <typename T>
   inline triangle<T,3> nonsymmetric_mirror(const triangle<T,3>& triangle, const T& ratio, const plane<T,3>& plane)
   {
      wykobi::triangle<T,3> triangle_;

      for (std::size_t i = 0; i < wykobi::triangle<T,3>::PointCount; ++i)
      {
         triangle_[i] = nonsymmetric_mirror(triangle[i],ratio,plane);
      }

      return triangle_;
   }

   template <typename T>
   inline quadix<T,3> nonsymmetric_mirror(const quadix<T,3>& quadix, const T& ratio, const plane<T,3>& plane)
   {
      wykobi::quadix<T,3> quadix_;

      for (std::size_t i = 0; i < wykobi::quadix<T,3>::PointCount; ++i)
      {
         quadix_[i] = nonsymmetric_mirror(quadix[i],ratio,plane);
      }

      return quadix_;
   }

   template <typename T>
   inline sphere<T> nonsymmetric_mirror(const sphere<T>& sphere, const T& ratio, const plane<T,3>& plane)
   {
      point3d<T> center = make_point(sphere);
      return make_sphere(nonsymmetric_mirror(center,ratio,plane),sphere.radius);
   }

   template <typename T>
   inline polygon<T,3> nonsymmetric_mirror(const polygon<T,3>& polygon, const T& ratio, const plane<T,3>& plane)
   {
      wykobi::polygon<T,3> polygon_;
      polygon_.reserve(polygon.size());

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         polygon_.push_back(nonsymmetric_mirror(polygon[i],ratio,plane));
      }

      return polygon_;
   }

   template <typename T>
   inline point2d<T> invert_point(const point2d<T>& point, const circle<T>& circle)
   {
      return project_point_t(make_point(circle),point,sqr(circle.radius) / (sqr(point.x - circle.x) + sqr(point.y - circle.y)));
   }

   template <typename T>
   inline point3d<T> invert_point(const point3d<T>& point, const sphere<T>& sphere)
   {
      return project_point_t
             (
               make_point(sphere),
               point,
               sqr(sphere.radius) / (sqr(point.x - sphere.x) + sqr(point.y - sphere.y) + sqr(point.z - sphere.z))
             );
   }

   template <typename T>
   inline point2d<T> antipodal_point(const point2d<T>& point, const circle<T>& circle)
   {
      return project_point_t(point,make_point(circle),T(2.0));
   }

   template <typename T>
   inline point3d<T> antipodal_point(const point3d<T>& point, const sphere<T>& sphere)
   {
      return project_point_t(point,make_point(sphere),T(2.0));
   }

   template <typename T>
   inline T distance(const T& x1, const T& y1, const T& x2, const T& y2)
   {
      const T dx = (x1 - x2);
      const T dy = (y1 - y2);

      return sqrt(dx * dx + dy * dy);
   }

   template <typename T>
   inline T distance(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2)
   {
      const T dx = (x1 - x2);
      const T dy = (y1 - y2);
      const T dz = (z1 - z2);

      return sqrt(dx * dx + dy * dy + dz * dz);
   }

   template <typename T>
   inline T distance(const point2d<T>& point1, const point2d<T>& point2)
   {
      return distance(point1.x,point1.y,point2.x,point2.y);
   }

   template <typename T>
   inline T distance(const point3d<T>& point1, const point3d<T>& point2)
   {
      return distance(point1.x,point1.y,point1.z,point2.x,point2.y,point2.z);
   }

   template <typename T>
   inline T distance(const point2d<T>& point, const segment<T,2>& segment)
   {
      return distance(closest_point_on_segment_from_point(segment,point),point);
   }

   template <typename T>
   inline T distance(const curve_point<T,2>& point1, const curve_point<T,2>& point2)
   {
      return distance(point1().x,point1().y,point2().x,point2().y);
   }

   template <typename T>
   inline T distance(const curve_point<T,3>& point1, const curve_point<T,3>& point2)
   {
      return distance(point1().x,point1().y,point1().z,point2().x,point2().y,point2().z);
   }

   template <typename T>
   inline T distance(const point3d<T>& point, const segment<T,3>& segment)
   {
      return distance(closest_point_on_segment_from_point(segment,point),point);
   }

   template <typename T>
   inline T distance(const point2d<T>& point, const rectangle<T>& rectangle)
   {
      return distance(closest_point_on_rectangle_from_point(rectangle,point),point);
   }

   template <typename T>
   inline T distance(const point2d<T>& point, const triangle<T,2>& triangle)
   {
      return distance(closest_point_on_triangle_from_point(triangle,point),point);
   }

   template <typename T>
   inline T distance(const point2d<T>& point, const quadix<T,2>& quadix)
   {
      return sqrt(lay_distance(point,quadix));
   }

   template <typename T>
   inline T distance(const point2d<T>& point, const ray<T,2>& ray)
   {
      return distance(point,closest_point_on_ray_from_point(ray,point));
   }

   template <typename T>
   inline T distance(const point3d<T>& point, const ray<T,3>& ray)
   {
      return distance(point,closest_point_on_ray_from_point(ray,point));
   }

   template <typename T>
   inline T distance(const point3d<T>& point, const plane<T,3>& plane)
   {
      return (plane.normal.x * point.x + plane.normal.y * point.y + plane.normal.z * point.z ) - plane.constant;
   }

   template <typename T>
   inline T distance(const line<T,2>& line1, const line<T,2>& line2)
   {
      return distance_line_to_line(line1[0].x, line1[0].y,
                                   line1[1].x, line1[1].y,
                                   line2[0].x, line2[0].y,
                                   line2[1].x, line2[1].y);
   }

   template <typename T>
   inline T distance(const line<T,3>& line1, const line<T,3>& line2)
   {
      return distance_line_to_line(line1[0].x, line1[0].y, line1[0].z,
                                   line1[1].x, line1[1].y, line1[1].z,
                                   line2[0].x, line2[0].y, line2[0].z,
                                   line2[1].x, line2[1].y, line2[1].z);
   }

   template <typename T>
   inline T distance(const segment<T,2>& segment1, const segment<T,2>& segment2)
   {
      return distance_segment_to_segment
             (
               segment1[0].x, segment1[0].y,
               segment1[1].x, segment1[1].y,
               segment2[0].x, segment2[0].y,
               segment2[1].x, segment2[1].y
             );
   }

   template <typename T>
   inline T distance(const segment<T,3>& segment1, const segment<T,3>& segment2)
   {
      return distance_segment_to_segment
             (
               segment1[0].x, segment1[0].y, segment1[0].z,
               segment1[1].x, segment1[1].y, segment1[1].z,
               segment2[0].x, segment2[0].y, segment2[0].z,
               segment2[1].x, segment2[1].y, segment2[1].z
             );
   }

   template <typename T>
   inline T distance(const segment<T,2>& segment)
   {
      return distance(segment[0],segment[1]);
   }

   template <typename T>
   inline T distance(const segment<T,3>& segment)
   {
      return distance(segment[0],segment[1]);
   }

   template <typename T>
   inline T distance(const segment<T,2>& segment, const triangle<T,2>& triangle)
   {
      return sqrt(lay_distance(segment,triangle));
   }

   template <typename T>
   inline T distance(const segment<T,3>& segment, const triangle<T,3>& triangle)
   {
      return sqrt(lay_distance(segment,triangle));
   }

   template <typename T>
   inline T distance(const segment<T,2>& segment, const rectangle<T>& rectangle)
   {
      return min(min(distance(segment, edge(rectangle,0)), distance(segment, edge(rectangle,1))),
                 min(distance(segment, edge(rectangle,2)), distance(segment, edge(rectangle,3))));
   }

   template <typename T>
   inline T distance(const segment<T,2>& segment, const circle<T>& circle)
   {
      return distance(closest_point_on_circle_from_segment(circle,segment),segment);
   }

   template <typename T>
   inline T distance(const triangle<T,2>& triangle1, const triangle<T,2>& triangle2)
   {
      T min_dist = min(minimum_distance_from_point_to_triangle(triangle1[0],triangle2),
                       minimum_distance_from_point_to_triangle(triangle2[0],triangle1));

      for (std::size_t i = 1; i < triangle<T,2>::PointCount; ++i)
      {
         if (is_equal(min_dist,T(0.0)))
            return min_dist;

         min_dist = min(min_dist,
                       min(minimum_distance_from_point_to_triangle(triangle1[i],triangle2),
                           minimum_distance_from_point_to_triangle(triangle2[i],triangle1)));
      }

      return min_dist;
   }

   template <typename T>
   inline T distance(const triangle<T,2>& triangle, const rectangle<T>& rectangle)
   {
      if (intersect(triangle,rectangle))
         return T(0.0);
      else
         return min(
                     min(
                          distance(edge(rectangle, 0), triangle),
                          distance(edge(rectangle, 1), triangle)
                        ),
                     min(
                          distance(edge(rectangle, 2), triangle),
                          distance(edge(rectangle, 3), triangle)
                        )
                   );
   }

   template <typename T>
   inline T distance(const rectangle<T>& rectangle1, const rectangle<T>& rectangle2)
   {
      if (intersect(rectangle1,rectangle2))
         return T(0.0);
      else
      {
         const rectangle<T> rec1 = aabb(rectangle1);
         const rectangle<T> rec2 = aabb(rectangle2);

         if (rec1[1].y < rec2[0].y)
            return distance
                   (
                     make_segment(rec1[0].x,rec1[1].y,rec1[1].x,rec1[1].y),
                     make_segment(rec2[0].x,rec2[0].y,rec2[1].x,rec2[0].y)
                   );
         else if (rec1[0].y > rec2[1].y)
            return distance
                   (
                     make_segment(rec1[0].x,rec1[0].y,rec1[1].x,rec1[0].y),
                     make_segment(rec2[0].x,rec2[1].y,rec2[1].x,rec2[1].y)
                   );
         else if (rec1[1].x < rec2[0].x)
            return distance
                   (
                     make_segment(rec1[1].x,rec1[0].y,rec1[1].x,rec1[1].y),
                     make_segment(rec2[0].x,rec2[0].y,rec2[0].x,rec2[1].y)
                   );
         else if (rec1[0].x > rec2[1].x)
            return distance
                   (
                     make_segment(rec1[0].x,rec1[0].y,rec1[0].x,rec1[1].y),
                     make_segment(rec2[1].x,rec2[0].y,rec2[1].x,rec2[1].y)
                   );
         else
            return T(0.0);
      }
   }

   template <typename T>
   inline T distance(const triangle<T,2>& triangle, const circle<T>& circle)
   {
      if (intersect(triangle,circle))
         return T(0.0);
      else
      {
         const point2d<T> point1 = closest_point_on_triangle_from_point(triangle, circle.x, circle.y);
         const point2d<T> point2 = closest_point_on_circle_from_point(circle, point1);

         return distance(point1,point2);
      }
   }

   template <typename T>
   inline T distance(const rectangle<T>& rectangle, const circle<T>& circle)
   {
      if (intersect(rectangle,circle))
         return T(0.0);
      else
      {
         const point2d<T> point1 = closest_point_on_rectangle_from_point(rectangle, circle.x, circle.y);
         const point2d<T> point2 = closest_point_on_circle_from_point(circle, point1);

         return distance(point1,point2);
      }
   }

   template <typename T>
   inline T distance(const point2d<T>& point, const circle<T>& circle)
   {
      if (point_in_circle(point,circle))
         return T(0.0);
      else
         return distance(point,closest_point_on_circle_from_point(circle,point));
   }

   template <typename T>
   inline T distance(const circle<T>& circle1, const circle<T>& circle2)
   {
      const T dist = distance(circle1.x, circle1.y, circle2.x, circle2.y);

      if (dist > (circle1.radius + circle2.radius))
         return (dist - (circle1.radius + circle2.radius));
      else
         return T(0.0);
   }

   template <typename T>
   inline T distance(const sphere<T>& sphere1, const sphere<T>& sphere2)
   {
      const T dist = distance
                     (
                       sphere1.x, sphere1.y, sphere1.z,
                       sphere2.x, sphere2.y, sphere2.z
                     );

      if (dist > (sphere1.radius + sphere2.radius))
         return (dist - (sphere1.radius + sphere2.radius));
      else
         return T(0.0);
   }

   template <typename T>
   inline T lay_distance(const T& x1, const T& y1, const T& x2, const T& y2)
   {
      const T dx = (x2 - x1);
      const T dy = (y2 - y1);

      return dx * dx + dy * dy;
   }

   template <typename T>
   inline T lay_distance(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2)
   {
      const T dx = (x2 - x1);
      const T dy = (y2 - y1);
      const T dz = (z2 - z1);

      return dx * dx + dy * dy + dz * dz;
   }

   template <typename T>
   inline T lay_distance(const point2d<T>& point1, const point2d<T>& point2)
   {
      return lay_distance(point1.x,point1.y,point2.x,point2.y);
   }

   template <typename T>
   inline T lay_distance(const point3d<T>& point1, const point3d<T>& point2)
   {
      return lay_distance(point1.x,point1.y,point1.z,point2.x,point2.y,point2.z);
   }

   template <typename T>
   inline T lay_distance(const point2d<T>& point, const triangle<T,2>& triangle)
   {
      return lay_distance(point,closest_point_on_triangle_from_point(triangle,point));
   }

   template <typename T>
   inline T lay_distance(const point2d<T>& point, const quadix<T,2>& quadix)
   {
      return lay_distance(point,closest_point_on_quadix_from_point(quadix,point));
   }

   template <typename T>
   inline T lay_distance(const point2d<T>& point, const ray<T,2>& ray)
   {
      return lay_distance(point,closest_point_on_ray_from_point(ray,point));
   }

   template <typename T>
   inline T lay_distance(const point3d<T>& point, const ray<T,3>& ray)
   {
      return lay_distance(point,closest_point_on_ray_from_point(ray,point));
   }

   template <typename T>
   inline T lay_distance(const point3d<T>& point, const plane<T,3>& plane)
   {
      return sqr((plane.normal.x * point.x + plane.normal.y * point.y + plane.normal.z * point.z ) - plane.constant);
   }

   template <typename T>
   inline T lay_distance(const segment<T,2>& segment1, const segment<T,2>& segment2)
   {
      return lay_distance_segment_to_segment
             (
               segment1[0].x, segment1[0].y,
               segment1[1].x, segment1[1].y,
               segment2[0].x, segment2[0].y,
               segment2[1].x, segment2[1].y
             );
   }

   template <typename T>
   inline T lay_distance(const segment<T,3>& segment1, const segment<T,3>& segment2)
   {
      return lay_distance_segment_to_segment
             (
               segment1[0].x, segment1[0].y, segment1[0].z,
               segment1[1].x, segment1[1].y, segment1[1].z,
               segment2[0].x, segment2[0].y, segment2[0].z,
               segment2[1].x, segment2[1].y, segment2[1].z
             );
   }

   template <typename T>
   inline T lay_distance(const line<T,3>& line1, const line<T,3>& line2)
   {
      return lay_distance_line_to_line
             (
               line1[0].x, line1[0].y, line1[0].z,
               line1[1].x, line1[1].y, line1[1].z,
               line2[0].x, line2[0].y, line2[0].z,
               line2[1].x, line2[1].y, line2[1].z
             );
   }

   template <typename T>
   inline T lay_distance(const segment<T,2>& segment)
   {
      return lay_distance(segment[0],segment[1]);
   }

   template <typename T>
   inline T lay_distance(const segment<T,3>& segment)
   {
      return lay_distance(segment[0],segment[1]);
   }

   template <typename T>
   inline T lay_distance(const segment<T,2>& segment, const triangle<T,2>& triangle)
   {
      return min
             (
               min
               (
                 lay_distance_segment_to_segment
                 (
                    segment[0].x,  segment[0].y,
                    segment[1].x,  segment[1].y,
                   triangle[0].x, triangle[0].y,
                   triangle[1].x, triangle[1].y
                 ),
                 lay_distance_segment_to_segment
                 (
                    segment[0].x, segment[0].y,
                    segment[1].x, segment[1].y,
                   triangle[1].x,triangle[1].y,
                   triangle[2].x,triangle[2].y
                 )
               ),
               lay_distance_segment_to_segment
               (
                  segment[0].x, segment[0].y,
                  segment[1].x, segment[1].y,
                 triangle[2].x,triangle[2].y,
                 triangle[0].x,triangle[0].y
               )
             );
   }

   template <typename T>
   inline T lay_distance(const segment<T,3>& segment, const triangle<T,3>& triangle)
   {
      return min
             (
               min
               (
                 lay_distance_segment_to_segment( segment[0].x, segment[0].y, segment[0].z,
                                                  segment[1].x, segment[1].y, segment[1].z,
                                                 triangle[0].x,triangle[0].y,triangle[0].z,
                                                 triangle[1].x,triangle[1].y,triangle[1].z),
                 lay_distance_segment_to_segment( segment[0].x, segment[0].y, segment[0].z,
                                                  segment[1].x, segment[1].y, segment[1].z,
                                                 triangle[1].x,triangle[1].y,triangle[1].z,
                                                 triangle[2].x,triangle[2].y,triangle[2].z)
               ),
               lay_distance_segment_to_segment( segment[0].x, segment[0].y,  segment[0].z,
                                                segment[1].x, segment[1].y,  segment[1].z,
                                               triangle[2].x,triangle[2].y, triangle[2].z,
                                               triangle[0].x,triangle[0].y, triangle[0].z)
             );
   }

   template <typename T>
   inline T manhattan_distance(const T& x1, const T& y1, const T& x2, const T& y2)
   {
      return abs(x2 - x1) + abs(y2 - y1);
   }

   template <typename T>
   inline T manhattan_distance(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2)
   {
      return abs(x2 - x1) + abs(y2 - y1) + abs(z2 - z1);
   }

   template <typename T>
   inline T manhattan_distance(const point2d<T>& point1, const point2d<T>& point2)
   {
      return manhattan_distance(point1.x,point1.y,point2.x,point2.y);
   }

   template <typename T>
   inline T manhattan_distance(const point3d<T>& point1, const point3d<T>& point2)
   {
      return manhattan_distance(point1.x,point1.y,point1.z,point2.x,point2.y,point2.z);
   }

   template <typename T>
   inline T manhattan_distance(const point2d<T>& point, const ray<T,2>& ray)
   {
      return manhattan_distance(point,closest_point_on_ray_from_point(ray,point));
   }

   template <typename T>
   inline T manhattan_distance(const point3d<T>& point, const ray<T,3>& ray)
   {
      return manhattan_distance(point,closest_point_on_ray_from_point(ray,point));
   }

   template <typename T>
   inline T manhattan_distance(const segment<T,2>& segment)
   {
      return manhattan_distance(segment[0],segment[1]);
   }

   template <typename T>
   inline T manhattan_distance(const segment<T,3>& segment)
   {
      return manhattan_distance(segment[0],segment[1]);
   }

   template <typename T>
   inline T manhattan_distance(const circle<T>& circle1, const circle<T>& circle2)
   {
      return manhattan_distance(circle1.x,circle1.y,circle2.x,circle2.y);
   }

   template <typename T>
   inline T chebyshev_distance(const T& x1, const T& y1, const T& x2, const T& y2)
   {
      return max(abs(x2 - x1), abs(y2 - y1));
   }

   template <typename T>
   inline T chebyshev_distance(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2)
   {
      return max(max(abs(x2 - x1), abs(y2 - y1)), abs(z2 - z1));
   }

   template <typename T>
   inline T chebyshev_distance(const point2d<T>& point1, const point2d<T>& point2)
   {
      return chebyshev_distance(point1.x,point1.y,point2.x,point2.y);
   }

   template <typename T>
   inline T chebyshev_distance(const point3d<T>& point1, const point3d<T>& point2)
   {
      return chebyshev_distance(point1.x,point1.y,point1.z,point2.x,point2.y,point2.z);
   }

   template <typename T>
   inline T chebyshev_distance(const segment<T,2>& segment)
   {
      return chebyshev_distance(segment[0],segment[1]);
   }

   template <typename T>
   inline T chebyshev_distance(const segment<T,3>& segment)
   {
      return chebyshev_distance(segment[0],segment[1]);
   }

   template <typename T>
   inline T chebyshev_distance(const circle<T>& circle1, const circle<T>& circle2)
   {
      return chebyshev_distance(circle1.x,circle1.y,circle2.x,circle2.y);
   }

   template <typename T>
   inline T inverse_chebyshev_distance(const T& x1, const T& y1, const T& x2, const T& y2)
   {
      return min(abs(x2 - x1),abs(y2 - y1));
   }

   template <typename T>
   inline T inverse_chebyshev_distance(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2)
   {
      return min(min(abs(x2 - x1), abs(y2 - y1)), abs(z2 - z1));
   }

   template <typename T>
   inline T inverse_chebyshev_distance(const point2d<T>& point1, const point2d<T>& point2)
   {
      return inverse_chebyshev_distance(point1.x,point1.y,point2.x,point2.y);
   }

   template <typename T>
   inline T inverse_chebyshev_distance(const point3d<T>& point1, const point3d<T>& point2)
   {
      return inverse_chebyshev_distance(point1.x,point1.y,point1.z,point2.x,point2.y,point2.z);
   }

   template <typename T>
   inline T inverse_chebyshev_distance(const segment<T,2>& segment)
   {
      return inverse_chebyshev_distance(segment[0],segment[1]);
   }

   template <typename T>
   inline T inverse_chebyshev_distance(const segment<T,3>& segment)
   {
      return inverse_chebyshev_distance(segment[0],segment[1]);
   }

   template <typename T>
   inline T inverse_chebyshev_distance(const circle<T>& circle1, const circle<T>& circle2)
   {
      return inverse_chebyshev_distance(circle1.x,circle1.y,circle2.x,circle2.y);
   }

   template <typename T>
   inline point2d<T> minkowski_sum(const point2d<T>& point1, const point2d<T>& point2)
   {
      return point1 + point2;
   }

   template <typename T>
   inline polygon<T,2> minkowski_sum(const rectangle<T>& rectangle1, const rectangle<T>& rectangle2)
   {
      polygon<T,2> polygon;
      polygon.reserve(16);

      for (std::size_t i = 0; i < 4; ++i)
      {
         point2d<T> point = rectangle_corner(rectangle1,i);

         for (std::size_t j = 0; j < 4; ++j)
         {
            polygon.push_back(minkowski_sum(point,rectangle_corner(rectangle2,j)));
         }
      }

      return polygon;
   }

   template <typename T>
   inline polygon<T,2> minkowski_sum(const triangle<T,2>& triangle1, const triangle<T,2>& triangle2)
   {
      polygon<T,2> polygon;
      polygon.reserve(9);

      for (std::size_t i = 0; i < wykobi::triangle<T,2>::PointCount; ++i)
      {
         for (std::size_t j = 0; j < wykobi::triangle<T,2>::PointCount; ++j)
         {
            polygon.push_back(minkowski_sum(triangle1[i],triangle2[j]));
         }
      }

      return polygon;
   }

   template <typename T>
   inline polygon<T,2> minkowski_sum(const quadix<T,2>& quadix1, const quadix<T,2>& quadix2)
   {
      polygon<T,2> polygon;
      polygon.reserve(16);

      for (std::size_t i = 0; i < wykobi::quadix<T,2>::PointCount; ++i)
      {
         for (std::size_t j = 0; j < wykobi::quadix<T,2>::PointCount; ++j)
         {
            polygon.push_back(minkowski_sum(quadix1[i],quadix2[j]));
         }
      }

      return polygon;
   }

   template <typename T>
   inline polygon<T,2> minkowski_sum(const circle<T>& circle1, const circle<T>& circle2)
   {
      return minkowski_sum(make_polygon(circle1),make_polygon(circle2));
   }

   template <typename T>
   inline polygon<T,2> minkowski_sum(const triangle<T,2>& triangle, const rectangle<T>& rectangle)
   {
      polygon<T,2> polygon;
      polygon.reserve(12);

      for (std::size_t i = 0; i < wykobi::triangle<T,2>::PointCount; ++i)
      {
         for (std::size_t j = 0; j < 4; ++j)
         {
            polygon.push_back(minkowski_sum(triangle[i],rectangle_corner(rectangle,j)));
         }
      }

      return polygon;
   }

   template <typename T>
   inline polygon<T,2> minkowski_sum(const triangle<T,2>& triangle, const quadix<T,2>& quadix)
   {
      polygon<T,2> polygon;
      polygon.reserve(12);

      for (std::size_t i = 0; i < wykobi::triangle<T,2>::PointCount; ++i)
      {
         for (std::size_t j = 0; j < wykobi::quadix<T,2>::PointCount; ++j)
         {
            polygon.push_back(minkowski_sum(triangle[i],quadix[j]));
         }
      }

      return polygon;
   }

   template <typename T>
   inline polygon<T,2> minkowski_sum(const triangle<T,2>& triangle, const circle<T>& circle)
   {
      wykobi::polygon<T,2> polygon;
      wykobi::polygon<T,2> circle_polygon = make_polygon(circle);
      polygon.reserve(1080);

      for (std::size_t i = 0; i < wykobi::triangle<T,2>::PointCount; ++i)
      {
         for (std::size_t j = 0; j < circle_polygon.size(); ++j)
         {
            polygon.push_back(minkowski_sum(triangle[i],circle_polygon[j]));
         }
      }

      return polygon;
   }

   template <typename T>
   inline polygon<T,2> minkowski_sum(const quadix<T,2>& quadix, const circle<T>& circle)
   {
      wykobi::polygon<T,2> polygon;
      wykobi::polygon<T,2> circle_polygon = make_polygon(circle);
      polygon.reserve(1440);

      for (std::size_t i = 0; i < wykobi::quadix<T,2>::PointCount; ++i)
      {
         for (std::size_t j = 0; j < circle_polygon.size(); ++j)
         {
            polygon.push_back(minkowski_sum(quadix[i],circle_polygon[j]));
         }
      }

      return polygon;
   }

   template <typename T>
   inline polygon<T,2> minkowski_sum(const quadix<T,2>& quadix, const rectangle<T>& rectangle)
   {
      polygon<T,2> polygon;
      polygon.reserve(16);

      for (std::size_t i = 0; i < wykobi::quadix<T,2>::PointCount; ++i)
      {
         for (std::size_t j = 0; j < 4; ++j)
         {
            polygon.push_back(minkowski_sum(quadix[i],rectangle_corner(rectangle,j)));
         }
      }

      return polygon;
   }

   template <typename T>
   inline polygon<T,2> minkowski_sum(const rectangle<T>& rectangle, const circle<T>& circle)
   {
      wykobi::polygon<T,2> polygon;
      wykobi::polygon<T,2> circle_polygon = make_polygon(circle);
      polygon.reserve(1440);

      for (std::size_t i = 0; i < 4; ++i)
      {
         for (std::size_t j = 0; j < circle_polygon.size(); ++j)
         {
            polygon.push_back(minkowski_sum(rectangle_corner(rectangle,i),circle_polygon[j]));
         }
      }

      return polygon;
   }

   template <typename T>
   inline polygon<T,2> minkowski_sum(const polygon<T,2>& polygon1, const polygon<T,2>& polygon2)
   {
      polygon<T,2> polygon;
      polygon.reserve(polygon1.size() * polygon2.size());

      for (std::size_t i = 0; i < polygon1.size(); ++i)
      {
         for (std::size_t j = 0; j < polygon2.size(); ++j)
         {
            polygon.push_back(minkowski_sum(polygon1[i],polygon2[j]));
         }
      }

      return polygon;
   }

   template <typename T>
   inline point2d<T> minkowski_difference(const point2d<T>& point1, const point2d<T>& point2)
   {
      return make_point(point1.x - point2.x, point1.y - point2.y);
   }

   template <typename T>
   inline polygon<T,2> minkowski_difference(const rectangle<T>& rectangle1, const rectangle<T>& rectangle2)
   {
      polygon<T,2> polygon;
      polygon.reserve(16);

      for (std::size_t i = 0; i < 4; ++i)
      {
         point2d<T> point = rectangle_corner(rectangle1,i);

         for (std::size_t j = 0; j < 4; ++j)
         {
            polygon.push_back(minkowski_difference(point,rectangle_corner(rectangle2,j)));
         }
      }

      return polygon;
   }

   template <typename T>
   inline polygon<T,2> minkowski_difference(const triangle<T,2>& triangle1, const triangle<T,2>& triangle2)
   {
      polygon<T,2> polygon;
      polygon.reserve(9);

      for (std::size_t i = 0; i < wykobi::triangle<T,2>::PointCount; ++i)
      {
         for (std::size_t j = 0; j < wykobi::triangle<T,2>::PointCount; ++j)
         {
            polygon.push_back(minkowski_difference(triangle1[i],triangle2[j]));
         }
      }

      return polygon;
   }

   template <typename T>
   inline polygon<T,2> minkowski_difference(const quadix<T,2>& quadix1, const quadix<T,2>& quadix2)
   {
      polygon<T,2> polygon;
      polygon.reserve(16);

      for (std::size_t i = 0; i < wykobi::quadix<T,2>::PointCount; ++i)
      {
         for (std::size_t j = 0; j < wykobi::quadix<T,2>::PointCount; ++j)
         {
            polygon.push_back(minkowski_difference(quadix1[i],quadix2[j]));
         }
      }

      return polygon;
   }

   template <typename T>
   inline polygon<T,2> minkowski_difference(const circle<T>& circle1, const circle<T>& circle2)
   {
      return minkowski_difference(make_polygon(circle1),make_polygon(circle2));
   }

   template <typename T>
   inline polygon<T,2> minkowski_difference(const triangle<T,2>& triangle, const rectangle<T>& rectangle)
   {
      polygon<T,2> polygon;
      polygon.reserve(12);

      for (std::size_t i = 0; i < wykobi::triangle<T,2>::PointCount; ++i)
      {
         for (std::size_t j = 0; j < 4; ++j)
         {
            polygon.push_back(minkowski_difference(triangle[i],rectangle_corner(rectangle,j)));
         }
      }

      return polygon;
   }

   template <typename T>
   inline polygon<T,2> minkowski_difference(const triangle<T,2>& triangle, const quadix<T,2>& quadix)
   {
      polygon<T,2> polygon;
      polygon.reserve(12);

      for (std::size_t i = 0; i < wykobi::triangle<T,2>::PointCount; ++i)
      {
         for (std::size_t j = 0; j < wykobi::quadix<T,2>::PointCount; ++j)
         {
            polygon.push_back(minkowski_difference(triangle[i],quadix[j]));
         }
      }

      return polygon;
   }

   template <typename T>
   inline polygon<T,2> minkowski_difference(const triangle<T,2>& triangle, const circle<T>& circle)
   {
      wykobi::polygon<T,2> polygon;
      wykobi::polygon<T,2> circle_polygon = make_polygon(circle);

      polygon.reserve(1080);

      for (std::size_t i = 0; i < wykobi::triangle<T,2>::PointCount; ++i)
      {
         for (std::size_t j = 0; j < circle_polygon.size(); ++j)
         {
            polygon.push_back(minkowski_difference(triangle[i],circle_polygon[j]));
         }
      }

      return polygon;
   }

   template <typename T>
   inline polygon<T,2> minkowski_difference(const quadix<T,2>& quadix, const circle<T>& circle)
   {
      wykobi::polygon<T,2> polygon;
      wykobi::polygon<T,2> circle_polygon = make_polygon(circle);

      polygon.reserve(1440);

      for (std::size_t i = 0; i < wykobi::quadix<T,2>::PointCount; ++i)
      {
         for (std::size_t j = 0; j < circle_polygon.size(); ++j)
         {
            polygon.push_back(minkowski_difference(quadix[i],circle_polygon[j]));
         }
      }

      return polygon;
   }

   template <typename T>
   inline polygon<T,2> minkowski_difference(const quadix<T,2>& quadix, const rectangle<T>& rectangle)
   {
      polygon<T,2> polygon;

      polygon.reserve(16);

      for (std::size_t i = 0; i < wykobi::quadix<T,2>::PointCount; ++i)
      {
         for (std::size_t j = 0; j < 4; ++j)
         {
            polygon.push_back(minkowski_difference(quadix[i],rectangle_corner(rectangle,j)));
         }
      }

      return polygon;
   }

   template <typename T>
   inline polygon<T,2> minkowski_difference(const rectangle<T>& rectangle, const circle<T>& circle)
   {
      wykobi::polygon<T,2> polygon;
      wykobi::polygon<T,2> circle_polygon = make_polygon(circle);

      polygon.reserve(1440);

      for (std::size_t i = 0; i < 4; ++i)
      {
         for (std::size_t j = 0; j < circle_polygon.size(); ++j)
         {
            polygon.push_back(minkowski_difference(rectangle_corner(rectangle,i),circle_polygon[j]));
         }
      }

      return polygon;
   }

   template <typename T>
   inline polygon<T,2> minkowski_difference(const polygon<T,2>& polygon1, const polygon<T,2>& polygon2)
   {
      polygon<T,2> polygon;

      polygon.reserve(polygon1.size() * polygon2.size());

      for (std::size_t i = 0; i < polygon1.size(); ++i)
      {
         for (std::size_t j = 0; j < polygon2.size(); ++j)
         {
            polygon.push_back(minkowski_difference(polygon1[i],polygon2[j]));
         }
      }

      return polygon;
   }

   template <typename T>
   inline T distance_segment_to_segment(const T& x1, const T& y1,
                                        const T& x2, const T& y2,
                                        const T& x3, const T& y3,
                                        const T& x4, const T& y4)
   {
      return sqrt(lay_distance_segment_to_segment(x1,y1,x2,y2,x3,y3,x4,y4));
   }

   template <typename T>
   inline T distance_segment_to_segment(const T& x1, const T& y1, const T& z1,
                                        const T& x2, const T& y2, const T& z2,
                                        const T& x3, const T& y3, const T& z3,
                                        const T& x4, const T& y4, const T& z4)
   {
      return sqrt(lay_distance_segment_to_segment(x1,y1,z1,x2,y2,z2,x3,y3,z3,x4,y4,z4));
   }

   template <typename T>
   inline T lay_distance_segment_to_segment(const T& x1, const T& y1,
                                            const T& x2, const T& y2,
                                            const T& x3, const T& y3,
                                            const T& x4, const T& y4)
   {
      const T ux = x2 - x1;
      const T uy = y2 - y1;

      const T vx = x4 - x3;
      const T vy = y4 - y3;

      const T wx = x1 - x3;
      const T wy = y1 - y3;

      const T a  = (ux * ux + uy * uy);
      const T b  = (ux * vx + uy * vy);
      const T c  = (vx * vx + vy * vy);
      const T d  = (ux * wx + uy * wy);
      const T e  = (vx * wx + vy * wy);
      const T dt = a * c - b * b;

      T sc = T(0.0);
      T sn = T(0.0);
      T tc = T(0.0);
      T tn = T(0.0);
      T sd = dt;
      T td = dt;

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

      if (is_equal(sn,T(0.0)))
         sc = T(0.0);
      else
         sc = sn / sd;

      if (is_equal(tn,T(0.0)))
         tc = T(0.0);
      else
         tc = tn / td;

      const T dx = wx + (sc * ux) - (tc * vx);
      const T dy = wy + (sc * uy) - (tc * vy);

      return dx * dx + dy * dy;
   }

   template <typename T>
   inline T lay_distance_segment_to_segment(const T& x1, const T& y1, const T& z1,
                                            const T& x2, const T& y2, const T& z2,
                                            const T& x3, const T& y3, const T& z3,
                                            const T& x4, const T& y4, const T& z4)
   {
      const T ux = x2 - x1;
      const T uy = y2 - y1;
      const T uz = z2 - z1;

      const T vx = x4 - x3;
      const T vy = y4 - y3;
      const T vz = z4 - z3;

      const T wx = x1 - x3;
      const T wy = y1 - y3;
      const T wz = z1 - z3;

      const T a  = (ux * ux + uy * uy + uz * uz);
      const T b  = (ux * vx + uy * vy + uz * vz);
      const T c  = (vx * vx + vy * vy + vz * vz);
      const T d  = (ux * wx + uy * wy + uz * wz);
      const T e  = (vx * wx + vy * wy + vz * wz);
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

      const T dx = wx + (sc * ux) - (tc * vx);
      const T dy = wy + (sc * uy) - (tc * vy);
      const T dz = wz + (sc * uz) - (tc * vz);

      return (dx * dx) + (dy * dy) + (dz * dz);
   }

   template <typename T>
   inline T distance_line_to_line(const T& x1, const T& y1,
                                  const T& x2, const T& y2,
                                  const T& x3, const T& y3,
                                  const T& x4, const T& y4)
   {
      return sqrt(lay_distance_line_to_line(x1, y1, x2, y2, x3, y3, x4, y4));
   }

   template <typename T>
   inline T distance_line_to_line(const T& x1, const T& y1, const T& z1,
                                  const T& x2, const T& y2, const T& z2,
                                  const T& x3, const T& y3, const T& z3,
                                  const T& x4, const T& y4, const T& z4)
   {
      return sqrt(lay_distance_line_to_line(x1, y1, z1, x2, y2, z2,
                                            x3, y3, z3, x4, y4, z4));
   }

   template <typename T>
   inline T lay_distance_line_to_line(const T& x1, const T& y1,
                                      const T& x2, const T& y2,
                                      const T& x3, const T& y3,
                                      const T& x4, const T& y4)
   {
      const T ux = x2 - x1;
      const T uy = y2 - y1;

      const T vx = x4 - x3;
      const T vy = y4 - y3;

      if (not_equal(ux * vy, uy * vx))
      {
         return T(0.0);
      }

      const T wx = x1 - x3;
      const T wy = y1 - y3;

      const T a  = (ux * ux + uy * uy);
      const T b  = (ux * vx + uy * vy);
      const T c  = (vx * vx + vy * vy);
      const T d  = (ux * wx + uy * wy);
      const T e  = (vx * wx + vy * wy);
      const T dt = a * c - b * b;

      T sc = T(0.0);
      T tc = T(0.0);

      if (is_equal(dt,T(0.0)))
      {
         sc = T(0.0);

         if (b > c)
            tc = d / b;
         else
            tc = e / c;
      }
      else
      {
         sc = (b * e - c * d) / dt;
         tc = (a * e - b * d) / dt;
      }

      const T dx = wx + (sc * ux) - (tc * vx);
      const T dy = wy + (sc * uy) - (tc * vy);

      return dx * dx + dy * dy;
   }

   template <typename T>
   inline T lay_distance_line_to_line(const T& x1, const T& y1, const T& z1,
                                      const T& x2, const T& y2, const T& z2,
                                      const T& x3, const T& y3, const T& z3,
                                      const T& x4, const T& y4, const T& z4)
   {
      const T ux = x2 - x1;
      const T uy = y2 - y1;
      const T uz = z2 - z1;

      const T vx = x4 - x3;
      const T vy = y4 - y3;
      const T vz = z4 - z3;

      const T wx = x1 - x3;
      const T wy = y1 - y3;
      const T wz = z1 - z3;

      const T a  = (ux * ux + uy * uy + uz * uz);
      const T b  = (ux * vx + uy * vy + uz * vz);
      const T c  = (vx * vx + vy * vy + vz * vz);
      const T d  = (ux * wx + uy * wy + uz * wz);
      const T e  = (vx * wx + vy * wy + vz * wz);
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

      const T dx = wx + (sc * ux) - (tc * vx);
      const T dy = wy + (sc * uy) - (tc * vy);
      const T dz = wz + (sc * uz) - (tc * vz);

      return (dx * dx) + (dy * dy) + (dz * dz);
   }

   template <typename T>
   inline T lay_distance_from_point_to_circle_center(const point2d<T>& point, const circle<T>& circle)
   {
      return lay_distance(point.x,point.y,circle.x,circle.y);
   }

   template <typename T>
   inline T lay_distance_from_point_to_sphere_center(const point3d<T>& point, const sphere<T>& sphere)
   {
      return lay_distance(point.x,point.y,point.z,sphere.x,sphere.y,sphere.z);
   }

   template <typename T>
   inline T distance_from_point_to_circle_center(const point2d<T>& point, const circle<T>& circle)
   {
      return sqrt(lay_distance_from_point_to_circle_center(point,circle));
   }

   template <typename T>
   inline T distance_from_point_to_sphere_center(const point3d<T>& point, const sphere<T>& sphere)
   {
      return sqrt(lay_distance_from_point_to_sphere_center(point,sphere));
   }

   template <typename T>
   inline T span_length(const rectangle<T>& rect)
   {
      return distance(rect[0],rect[1]);
   }

   template <typename T>
   inline T span_length(const box<T,3>& box)
   {
      return distance(box[0],box[1]);
   }

   template <typename T>
   inline void project_point_t(const T&  srcx, const T&  srcy,
                               const T& destx, const T& desty,
                               const T& t,
                               T& nx, T& ny)
   {
      nx = srcx + t * (destx - srcx);
      ny = srcy + t * (desty - srcy);
   }

   template <typename T>
   inline void project_point_t(const T&  srcx, const T&  srcy, const T&  srcz,
                               const T& destx, const T& desty, const T& destz,
                               const T& t,
                               T& nx, T& ny, T& nz)
   {
      nx = srcx + t * (destx - srcx);
      ny = srcy + t * (desty - srcy);
      nz = srcz + t * (destz - srcz);
   }

   template <typename T>
   inline void project_point(const T&  srcx, const T&  srcy,
                             const T& destx, const T& desty,
                             const T& dist,
                             T& nx, T& ny)
   {
      project_point_t
      (
        srcx,  srcy,
        destx, desty,
        dist / distance(srcx, srcy, destx, desty),
        nx, ny
      );
   }

   template <typename T>
   inline void project_point(const T&  srcx, const T&  srcy, const T&  srcz,
                             const T& destx, const T& desty, const T& destz,
                             const T& dist,
                             T& nx, T& ny, T& nz)
   {
      project_point_t
      (
        srcx,  srcy,  srcz,
        destx, desty, destz,
        dist / distance(srcx, srcy, srcz, destx, desty, destz),
        nx, ny, nz
      );
   }

   template <typename T>
   inline void project_point(const T& px, const T& py, const T& angle, const T& distance, T& nx, T& ny)
   {
      T dx = T(0.0);
      T dy = T(0.0);

      switch (quadrant(angle))
      {
         case 1 : {
                     dx = T(cos(angle * T(PIDiv180))) * distance;
                     dy = T(sin(angle * T(PIDiv180))) * distance;
                  }
                  break;

         case 2 : {
                     dx = T(sin((angle - T(90.0)) * T(PIDiv180))) * distance * T(-1.0);
                     dy = T(cos((angle - T(90.0)) * T(PIDiv180))) * distance;
                  }
                  break;

         case 3 : {
                     dx = T(cos((angle - T(180.0)) * T(PIDiv180))) * distance * T(-1.0);
                     dy = T(sin((angle - T(180.0)) * T(PIDiv180))) * distance * T(-1.0);
                  }
                  break;

         case 4 : {
                     dx = T(sin((angle - T(270.0)) * T(PIDiv180))) * distance;
                     dy = T(cos((angle - T(270.0)) * T(PIDiv180))) * distance * T(-1.0);
                  }
                  break;
      }

      nx = px + dx;
      ny = py + dy;
   }

   template <typename T>
   inline void project_point0(const T& px, const T& py, const T& distance, T& nx, T& ny)
   {
      nx = px + distance;
      ny = py;
   }

   template <typename T>
   inline void project_point45(const T& px, const T& py, const T& distance, T& nx, T& ny)
   {
      nx = px + T(0.70710678118654752440084436210485) * distance;
      ny = py + T(0.70710678118654752440084436210485) * distance;
   }

   template <typename T>
   inline void project_point90(const T& px, const T& py, const T& distance, T& nx, T& ny)
   {
      nx = px;
      ny = py + distance;
   }

   template <typename T>
   inline void project_point135(const T& px, const T& py, const T& distance, T& nx, T& ny)
   {
      nx = px - T(0.70710678118654752440084436210485) * distance;
      ny = py + T(0.70710678118654752440084436210485) * distance;
   }

   template <typename T>
   inline void project_point180(const T& px, const T& py, const T& distance, T& nx, T& ny)
   {
      nx = px - distance;
      ny = py;
   }

   template <typename T>
   inline void project_point225(const T& px, const T& py, const T& distance, T& nx, T& ny)
   {
      nx = px - T(0.70710678118654752440084436210485) * distance;
      ny = py - T(0.70710678118654752440084436210485) * distance;
   }

   template <typename T>
   inline void project_point270(const T& px, const T& py, const T& distance, T& nx, T& ny)
   {
      nx = px;
      ny = py - distance;
   }

   template <typename T>
   inline void project_point315(const T& px, const T& py, const T& distance, T& nx, T& ny)
   {
      nx = px + T(0.70710678118654752440084436210485) * distance;
      ny = py - T(0.70710678118654752440084436210485) * distance;
   }

   template <typename T>
   inline point2d<T> project_point_t(const point2d<T>& source_point,
                                     const point2d<T>& destination_point,
                                     const T& t)
   {
      point2d<T> point_;

      project_point_t
      (
        source_point     .x, source_point     .y,
        destination_point.x, destination_point.y,
        t,
        point_           .x,point_            .y
      );

      return point_;
   }

   template <typename T>
   inline point3d<T> project_point_t(const point3d<T>& source_point,
                                     const point3d<T>& destination_point,
                                     const T& t)
   {
      point3d<T> point_;

      project_point_t
      (
        source_point     .x, source_point     .y, source_point     .z,
        destination_point.x, destination_point.y, destination_point.z,
        t,
        point_           .x, point_           .y, point_           .z
      );

      return point_;
   }

   template <typename T>
   inline point2d<T> project_point(const point2d<T>& source_point,
                                   const point2d<T>& destination_point,
                                   const T& distance)
   {
      point2d<T> point_;

      project_point
      (
        source_point     .x, source_point    .y,
        destination_point.x,destination_point.y,
        distance,
        point_           .x,point_           .y
      );

      return point_;
   }

   template <typename T>
   inline point3d<T> project_point(const point3d<T>& source_point,
                                   const point3d<T>& destination_point,
                                   const T& distance)
   {
      point3d<T> point_;

      project_point
      (
        source_point     .x, source_point     .y, source_point     .z,
        destination_point.x, destination_point.y, destination_point.z,
        distance,
        point_           .x, point_           .y, point_           .z
      );

      return point_;
   }

   template <typename T>
   inline point2d<T> project_point(const point2d<T>& point,
                                   const T& angle,
                                   const T& distance)
   {
      point2d<T> point_;
      project_point(point.x, point.y, angle, distance, point_.x, point_.y);
      return point_;
   }

   template <typename T>
   inline point2d<T> project_point0(const point2d<T>& point, const T& distance)
   {
      point2d<T> point_;
      project_point0(point.x,point.y,distance,point_.x,point_.y);
      return point_;
   }

   template <typename T>
   inline point2d<T> project_point45(const point2d<T>& point, const T& distance)
   {
      point2d<T> point_;
      project_point45(point.x,point.y,distance,point_.x,point_.y);
      return point_;
   }

   template <typename T>
   inline point2d<T> project_point90(const point2d<T>& point, const T& distance)
   {
      point2d<T> point_;
      project_point90(point.x,point.y,distance,point_.x,point_.y);
      return point_;
   }

   template <typename T>
   inline point2d<T> project_point135(const point2d<T>& point, const T& distance)
   {
      point2d<T> point_;
      project_point135(point.x,point.y,distance,point_.x,point_.y);
      return point_;
   }

   template <typename T>
   inline point2d<T> project_point180(const point2d<T>& point, const T& distance)
   {
      point2d<T> point_;
      project_point180(point.x,point.y,distance,point_.x,point_.y);
      return point_;
   }

   template <typename T>
   inline point2d<T> project_point225(const point2d<T>& point, const T& distance)
   {
      point2d<T> point_;
      project_point225(point.x,point.y,distance,point_.x,point_.y);
      return point_;
   }

   template <typename T>
   inline point2d<T> project_point270(const point2d<T>& point, const T& distance)
   {
      point2d<T> point_;
      project_point270(point.x,point.y,distance,point_.x,point_.y);
      return point_;
   }

   template <typename T>
   inline point2d<T> project_point315(const point2d<T>& point, const T& distance)
   {
      point2d<T> point_;
      project_point315(point.x,point.y,distance,point_.x,point_.y);
      return point_;
   }

   template <typename T>
   inline point2d<T> project_object(const point2d<T>& point, const T& angle, const T& distance)
   {
      return project_point(point,angle,distance);
   }

   template <typename T>
   inline segment<T,2> project_object(const segment<T,2>& segment, const T& angle, const T& distance)
   {
      wykobi::segment<T,2> segment_;

      for (std::size_t i = 0; i < wykobi::segment<T,2>::PointCount; ++i)
      {
         segment_[i] = project_point(segment[i],angle,distance);
      }

      return segment_;
   }

   template <typename T>
   inline triangle<T,2> project_object(const triangle<T,2>& triangle, const T& angle, const T& distance)
   {
      wykobi::triangle<T,2> triangle_;

      for (std::size_t i = 0; i < wykobi::triangle<T,2>::PointCount; ++i)
      {
         triangle_[i] = project_point(triangle[i],angle,distance);
      }

      return triangle_;
   }

   template <typename T>
   inline quadix<T,2> project_object(const quadix<T,2>& quadix, const T& angle, const T& distance)
   {
      wykobi::quadix<T,2> quadix_;

      for (std::size_t i = 0; i < wykobi::quadix<T,2>::PointCount; ++i)
      {
         quadix_[i] = project_point(quadix[i],angle,distance);
      }

      return quadix_;
   }

   template <typename T>
   inline circle<T> project_object(const circle<T>& circle, const T& angle, const T& distance)
   {
      wykobi::circle<T> circle_ = circle;
      project_point(circle.x,circle.y,angle,distance,circle_.x,circle_.y);
      return circle_;
   }

   template <typename T>
   inline polygon<T,2> project_object(const polygon<T,2>& polygon, const T& angle, const T& distance)
   {
      wykobi::polygon<T,2> polygon_;

      polygon_.reserve(polygon.size());

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         polygon_.push_back(project_point(polygon[i],angle,distance));
      }

      return polygon_;
   }

   template <typename T>
   inline segment<T,2> project_onto_axis(const point2d<T>& point, const line<T,2>& axis)
   {
      wykobi::point2d<T> point_ = closest_point_on_line_from_point(axis,point);
      return make_segment(point_,point_);
   }

   template <typename T>
   inline segment<T,2> project_onto_axis(const triangle<T,2>& triangle, const line<T,2>& axis)
   {
      std::vector< point2d<T> > point_list;

      point_list.reserve(3);

      for (std::size_t i = 0; i < wykobi::triangle<T,2>::PointCount; ++i)
      {
         point_list.push_back(closest_point_on_line_from_point(axis,triangle[i]));
      }

      std::sort(point_list.begin(),point_list.end());

      return make_segment(point_list.front(),point_list.back());
   }

   template <typename T>
   inline segment<T,2> project_onto_axis(const rectangle<T>& rectangle, const line<T,2>& axis)
   {
      std::vector< point2d<T> > point_list;

      point_list.reserve(4);

      point_list.push_back(closest_point_on_line_from_point(axis,make_point(rectangle[0].x,rectangle[0].y)));
      point_list.push_back(closest_point_on_line_from_point(axis,make_point(rectangle[1].x,rectangle[0].y)));
      point_list.push_back(closest_point_on_line_from_point(axis,make_point(rectangle[1].x,rectangle[1].y)));
      point_list.push_back(closest_point_on_line_from_point(axis,make_point(rectangle[0].x,rectangle[1].y)));

      std::sort(point_list.begin(),point_list.end());

      return make_segment(point_list.front(),point_list.back());
   }

   template <typename T>
   inline segment<T,2> project_onto_axis(const quadix<T,2>& quadix, const line<T,2>& axis)
   {
      std::vector< point2d<T> > point_list;

      point_list.reserve(4);

      for (std::size_t i = 0; i < wykobi::quadix<T,2>::PointCount; ++i)
      {
         point_list.push_back(closest_point_on_line_from_point(axis,quadix[i]));
      }

      std::sort(point_list.begin(),point_list.end());

      return make_segment(point_list.front(),point_list.back());
   }

   template <typename T>
   inline segment<T,2> project_onto_axis(const circle<T>& circle, const line<T,2>& axis)
   {
      vector2d<T> v = normalize(axis[0] - axis[1]);
      std::vector< point2d<T> > point_list;

      point_list.reserve(3);

      point_list.push_back(closest_point_on_line_from_point(axis,make_point(circle.x,circle.y)));
      point_list.push_back(closest_point_on_line_from_point(axis,make_point(point_list.front().x + (v.x * circle.radius),
                                                                            point_list.front().y + (v.y * circle.radius))));

      point_list.push_back(closest_point_on_line_from_point(axis,make_point(point_list.front().x - (v.x * circle.radius),
                                                                            point_list.front().y - (v.y * circle.radius))));
      std::sort(point_list.begin(),point_list.end());

      return make_segment(point_list.front(),point_list.back());
   }

   template <typename T>
   inline segment<T,2> project_onto_axis(const polygon<T,2>& polygon, const line<T,2>& axis)
   {
      if (polygon.size() == 0)
         return degenerate_segment2d<T>();

      std::vector< point2d<T> > point_list;

      point_list.reserve(polygon.size());

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         point_list.push_back(closest_point_on_line_from_point(axis,polygon[i]));
      }

      std::sort(point_list.begin(),point_list.end());

      return make_segment(point_list.front(),point_list.back());
   }

   template <typename T>
   inline segment<T,3> project_onto_axis(const point3d<T>& point, const line<T,3>& axis)
   {
      wykobi::point3d<T> point_ = closest_point_on_line_from_point(axis,point);
      return make_segment(point_,point_);
   }

   template <typename T>
   inline segment<T,3> project_onto_axis(const triangle<T,3>& triangle, const line<T,3>& axis)
   {
      std::vector< point3d<T> > point_list;

      point_list.reserve(3);

      for (std::size_t i = 0; i < wykobi::triangle<T,2>::PointCount; ++i)
      {
         point_list.push_back(closest_point_on_line_from_point(axis,triangle[i]));
      }

      std::sort(point_list.begin(),point_list.end());

      return make_segment(point_list.front(),point_list.back());
   }

   template <typename T>
   inline segment<T,3> project_onto_axis(const box<T,3>& box, const line<T,3>& axis)
   {
      std::vector< point3d<T> > point_list;

      point_list.reserve(8);

      point_list.push_back(closest_point_on_line_from_point(axis,make_point(box[0].x,box[0].y,box[0].z)));
      point_list.push_back(closest_point_on_line_from_point(axis,make_point(box[1].x,box[0].y,box[0].z)));
      point_list.push_back(closest_point_on_line_from_point(axis,make_point(box[1].x,box[1].y,box[0].z)));
      point_list.push_back(closest_point_on_line_from_point(axis,make_point(box[0].x,box[1].y,box[0].z)));
      point_list.push_back(closest_point_on_line_from_point(axis,make_point(box[0].x,box[0].y,box[1].z)));
      point_list.push_back(closest_point_on_line_from_point(axis,make_point(box[1].x,box[0].y,box[1].z)));
      point_list.push_back(closest_point_on_line_from_point(axis,make_point(box[1].x,box[1].y,box[1].z)));
      point_list.push_back(closest_point_on_line_from_point(axis,make_point(box[0].x,box[1].y,box[1].z)));

      std::sort(point_list.begin(),point_list.end());

      return make_segment(point_list.front(),point_list.back());
   }

   template <typename T>
   inline segment<T,3> project_onto_axis(const quadix<T,3>& quadix, const line<T,3>& axis)
   {
      std::vector< point3d<T> > point_list;

      point_list.reserve(4);

      for (std::size_t i = 0; i < wykobi::quadix<T,2>::PointCount; ++i)
      {
         point_list.push_back(closest_point_on_line_from_point(axis,quadix[i]));
      }

      std::sort(point_list.begin(),point_list.end());

      return make_segment(point_list.front(),point_list.back());
   }

   template <typename T>
   inline segment<T,3> project_onto_axis(const sphere<T>& sphere, const line<T,3>& axis)
   {
      vector3d<T> v = normalize(axis[0] - axis[1]);
      std::vector< point3d<T> > point_list;

      point_list.reserve(3);

      point_list.push_back(closest_point_on_line_from_point(axis,make_point(sphere.x,sphere.y,sphere.z)));

      point_list.push_back(closest_point_on_line_from_point(axis,make_point(point_list.front().x + (v.x * sphere.radius),
                                                                            point_list.front().y + (v.y * sphere.radius),
                                                                            point_list.front().z + (v.z * sphere.radius))));

      point_list.push_back(closest_point_on_line_from_point(axis,make_point(point_list.front().x - (v.x * sphere.radius),
                                                                            point_list.front().y - (v.y * sphere.radius),
                                                                            point_list.front().z - (v.z * sphere.radius))));
      std::sort(point_list.begin(),point_list.end());

      return make_segment(point_list.front(),point_list.back());
   }

   template <typename T>
   inline segment<T,3> project_onto_axis(const polygon<T,3>& polygon, const line<T,3>& axis)
   {
      std::vector< point3d<T> > point_list;

      point_list.reserve(polygon.size());

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         point_list.push_back(closest_point_on_line_from_point(axis,polygon[i]));
      }

      std::sort(point_list.begin(),point_list.end());

      return make_segment(point_list.front(),point_list.back());
   }

   template <typename T>
   inline void calculate_bezier_coefficients(const quadratic_bezier<T,2>& bezier, T& ax, T& bx, T& ay, T& by)
   {
      bx = T(2.0) * (bezier[1].x - bezier[0].x);
      by = T(2.0) * (bezier[1].y - bezier[0].y);
      ax = bezier[2].x - bezier[0].x - bx;
      ay = bezier[2].y - bezier[0].y - by;
   }

   template <typename T>
   inline void calculate_bezier_coefficients(const quadratic_bezier<T,3>& bezier, T& ax, T& bx, T& ay, T& by, T& az, T& bz)
   {
      bx = T(2.0) * (bezier[1].x - bezier[0].x);
      by = T(2.0) * (bezier[1].y - bezier[0].y);
      bz = T(2.0) * (bezier[1].z - bezier[0].z);
      ax = bezier[2].x - bezier[0].x - bx;
      ay = bezier[2].y - bezier[0].y - by;
      az = bezier[2].z - bezier[0].z - bz;
   }

   template <typename T>
   inline void calculate_bezier_coefficients(const cubic_bezier<T,2>& bezier, T& ax, T& bx, T& cx, T& ay, T& by, T& cy)
   {
      cx = T(3.0) * (bezier[1].x - bezier[0].x);
      cy = T(3.0) * (bezier[1].y - bezier[0].y);
      bx = T(3.0) * (bezier[2].x - bezier[1].x) - cx;
      by = T(3.0) * (bezier[2].y - bezier[1].y) - cy;
      ax = bezier[3].x - bezier[0].x - cx - bx;
      ay = bezier[3].y - bezier[0].y - cy - by;
   }

   template <typename T>
   inline void calculate_bezier_coefficients(const cubic_bezier<T,3>& bezier, T& ax, T& bx, T& cx, T& ay, T& by, T& cy, T& az, T& bz, T& cz)
   {
      cx = T(3.0) * (bezier[1].x - bezier[0].x);
      cy = T(3.0) * (bezier[1].y - bezier[0].y);
      cz = T(3.0) * (bezier[1].z - bezier[0].z);
      bx = T(3.0) * (bezier[2].x - bezier[1].x) - cx;
      by = T(3.0) * (bezier[2].y - bezier[1].y) - cy;
      bz = T(3.0) * (bezier[2].z - bezier[1].z) - cz;
      ax = bezier[3].x - bezier[0].x - cx - bx;
      ay = bezier[3].y - bezier[0].y - cy - by;
      az = bezier[3].z - bezier[0].z - cz - bz;
   }

   template <typename T>
   inline void calculate_bezier_coefficients(const quadratic_bezier<T,2>& bezier,
                                             bezier_coefficients<T,2,eQuadraticBezier>& coeffs)
   {
      calculate_bezier_coefficients
      (
        bezier,
        coeffs.value[0].x,coeffs.value[0].y,
        coeffs.value[1].x,coeffs.value[1].y
      );
   }

   template <typename T>
   inline void calculate_bezier_coefficients(const quadratic_bezier<T,3>& bezier,
                                             bezier_coefficients<T,3,eQuadraticBezier>& coeffs)
   {
      calculate_bezier_coefficients
      (
        bezier,
        coeffs.value[0].x,coeffs.value[0].y,coeffs.value[0].z,
        coeffs.value[1].x,coeffs.value[1].y,coeffs.value[1].z
      );
   }

   template <typename T>
   inline void calculate_bezier_coefficients(const cubic_bezier<T,2>& bezier,
                                             bezier_coefficients<T,2,eCubicBezier>& coeffs)
   {
      calculate_bezier_coefficients
      (
        bezier,
        coeffs.value[0].x,coeffs.value[0].y,
        coeffs.value[1].x,coeffs.value[1].y,
        coeffs.value[2].x,coeffs.value[2].y
      );
   }

   template <typename T>
   inline void calculate_bezier_coefficients(const cubic_bezier<T,3>& bezier,
                                             bezier_coefficients<T,3,eCubicBezier>& coeffs)
   {
      calculate_bezier_coefficients
      (
        bezier,
        coeffs.value[0].x,coeffs.value[0].y,coeffs.value[0].z,
        coeffs.value[1].x,coeffs.value[1].y,coeffs.value[1].z,
        coeffs.value[2].x,coeffs.value[2].y,coeffs.value[2].z
      );
   }

   template <typename T>
   inline point2d<T> create_point_on_bezier(const point2d<T>& start_point,
                                            const T& ax, const T& bx,
                                            const T& ay, const T& by,
                                            const T& t)
   {
      const T tsqr = t * t;

      return make_point
             (
               (ax * tsqr) + (bx * t) + start_point.x,
               (ay * tsqr) + (by * t) + start_point.y
             );
   }

   template <typename T>
   inline point3d<T> create_point_on_bezier(const point3d<T>& start_point,
                                            const T& ax, const T& bx,
                                            const T& ay, const T& by,
                                            const T& az, const T& bz,
                                            const T& t)
   {
      const T tsqr = t * t;

      return make_point
             (
               (ax * tsqr) + (bx * t) + start_point.x,
               (ay * tsqr) + (by * t) + start_point.y,
               (az * tsqr) + (bz * t) + start_point.z
             );
   }

   template <typename T>
   inline point2d<T> create_point_on_bezier(const point2d<T>& start_point,
                                            const T& ax, const T& bx, const T& cx,
                                            const T& ay, const T& by, const T& cy,
                                            const T& t)
   {
      const T tsqr  = t * t;
      const T tcube = tsqr * t;

      return make_point
             (
               (ax * tcube) + (bx * tsqr) + (cx * t) + start_point.x,
               (ay * tcube) + (by * tsqr) + (cy * t) + start_point.y
             );
   }

   template <typename T>
   inline point3d<T> create_point_on_bezier(const point3d<T>& start_point,
                                            const T& ax, const T& bx, const T& cx,
                                            const T& ay, const T& by, const T& cy,
                                            const T& az, const T& bz, const T& cz,
                                            const T& t)
   {
      const T tsqr  = t * t;
      const T tcube = tsqr * t;

      return make_point
             (
               (ax * tcube) + (bx * tsqr) + (cx * t) + start_point.x,
               (ay * tcube) + (by * tsqr) + (cy * t) + start_point.y,
               (az * tcube) + (bz * tsqr) + (cz * t) + start_point.z
             );
   }

   template <typename T>
   inline point2d<T> create_point_on_bezier(const point2d<T>& start_point,
                                            const bezier_coefficients<T,2,eQuadraticBezier>& coeffs,
                                            const T& t)
   {
      return create_point_on_bezier
             (
               start_point,
               coeffs.value[0].x, coeffs.value[0].y,
               coeffs.value[1].x, coeffs.value[1].y,
               t
             );
   }

   template <typename T>
   inline point3d<T> create_point_on_bezier(const point3d<T>& start_point,
                                            const bezier_coefficients<T,3,eQuadraticBezier>& coeffs,
                                            const T& t)
   {
      return create_point_on_bezier
             (
               start_point,
               coeffs.value[0].x, coeffs.value[0].y, coeffs.value[0].z,
               coeffs.value[1].x, coeffs.value[1].y, coeffs.value[1].z,
               t
             );
   }

   template <typename T>
   inline point2d<T> create_point_on_bezier(const point2d<T>& start_point,
                                            const bezier_coefficients<T,2,eCubicBezier>& coeffs,
                                            const T& t)
   {
      return create_point_on_bezier
             (
               start_point,
               coeffs.value[0].x, coeffs.value[0].y,
               coeffs.value[1].x, coeffs.value[1].y,
               coeffs.value[2].x, coeffs.value[2].y,
               t
             );
   }

   template <typename T>
   inline point3d<T> create_point_on_bezier(const point3d<T>& start_point,
                                            const bezier_coefficients<T,3,eCubicBezier>& coeffs,
                                            const T& t)
   {
      return create_point_on_bezier
             (
               start_point,
               coeffs.value[0].x, coeffs.value[0].y, coeffs.value[0].z,
               coeffs.value[1].x, coeffs.value[1].y, coeffs.value[1].z,
               coeffs.value[2].x, coeffs.value[2].y, coeffs.value[2].z,
               t
             );
   }

   template <typename T, typename OutputIterator>
   inline void generate_bezier(const quadratic_bezier<T,2>& bezier, OutputIterator out, const std::size_t& point_count)
   {
      if (0 == point_count) return;

      T t  = T(0.0);
      T dt = T(1.0) / (T(1.0) * point_count - T(1.0));
      T ax = T(0.0);
      T ay = T(0.0);
      T bx = T(0.0);
      T by = T(0.0);

      calculate_bezier_coefficients(bezier,ax,bx,ay,by);

      for (std::size_t i = 0; i < point_count; t += dt, ++i)
      {
         (*out++) = create_point_on_bezier(bezier[0],ax,bx,ay,by,t);
      }
   }

   template <typename T, typename OutputIterator>
   inline void generate_bezier(const quadratic_bezier<T,3>& bezier, OutputIterator out, const std::size_t& point_count)
   {
      if (0 == point_count) return;

      T t  = T(0.0);
      T dt = T(1.0) / (T(1.0) * point_count - T(1.0));
      T ax = T(0.0);
      T ay = T(0.0);
      T az = T(0.0);
      T bx = T(0.0);
      T by = T(0.0);
      T bz = T(0.0);

      calculate_bezier_coefficients(bezier,ax,bx,ay,by,az,bz);

      for (std::size_t i = 0; i < point_count; t += dt, ++i)
      {
         (*out++) = create_point_on_bezier(bezier[0],ax,bx,ay,by,az,bz,t);
      }
   }

   template <typename T, typename OutputIterator>
   inline void generate_bezier(const cubic_bezier<T,2>& bezier, OutputIterator out, const std::size_t& point_count)
   {
      if (0 == point_count) return;

      T t  = T(0.0);
      T dt = T(1.0) / (T(1.0) * point_count - T(1.0));
      T ax = T(0.0);
      T ay = T(0.0);
      T bx = T(0.0);
      T by = T(0.0);
      T cx = T(0.0);
      T cy = T(0.0);

      calculate_bezier_coefficients(bezier,ax,bx,cx,ay,by,cy);

      for (std::size_t i = 0; i < point_count; t += dt, ++i)
      {
         (*out++) = create_point_on_bezier(bezier[0],ax,bx,cx,ay,by,cy,t);
      }
   }

   template <typename T, typename OutputIterator>
   inline void generate_bezier(const cubic_bezier<T,3>& bezier, OutputIterator out, const std::size_t& point_count)
   {
      if (0 == point_count) return;

      T t  = T(0.0);
      T dt = T(1.0) / (T(1.0) * point_count - T(1.0));
      T ax = T(0.0);
      T ay = T(0.0);
      T az = T(0.0);
      T bx = T(0.0);
      T by = T(0.0);
      T bz = T(0.0);
      T cx = T(0.0);
      T cy = T(0.0);
      T cz = T(0.0);

      calculate_bezier_coefficients(bezier,ax,bx,cx,ay,by,cy,az,bz,cz);

      for (std::size_t i = 0; i < point_count; t += dt, ++i)
      {
         (*out++) = create_point_on_bezier(bezier[0],ax,bx,cx,ay,by,cy,az,bz,cz,t);
      }
   }

   template <typename T>
   inline T bezier_curve_length(const quadratic_bezier<T,2>& bezier, const std::size_t& point_count)
   {
      std::vector< point2d<T> > curve;

      curve.reserve(point_count);

      generate_bezier(bezier,std::back_inserter(curve),point_count);

      T total_distance = T(0.0);

      for (std::size_t i = 0; i < (curve.size() - 1); ++i)
      {
         total_distance += distance(curve[i],curve[i + 1]);
      }

      return total_distance;
   }

   template <typename T>
   inline T bezier_curve_length(const quadratic_bezier<T,3>& bezier, const std::size_t& point_count)
   {
      std::vector< point3d<T> > curve;

      curve.reserve(point_count);

      generate_bezier(bezier,std::back_inserter(curve),point_count);

      T total_distance = T(0.0);

      for (std::size_t i = 0; i < (curve.size() - 1); ++i)
      {
         total_distance += distance(curve[i],curve[i + 1]);
      }

      return total_distance;
   }

   template <typename T>
   inline T bezier_curve_length(const cubic_bezier<T,2>& bezier, const std::size_t& point_count)
   {
      std::vector< point2d<T> > curve;

      curve.reserve(point_count);

      generate_bezier(bezier,std::back_inserter(curve),point_count);

      T total_distance = T(0.0);

      for (std::size_t i = 0; i < (curve.size() - 1); ++i)
      {
         total_distance += distance(curve[i],curve[i + 1]);
      }

      return total_distance;
   }

   template <typename T>
   inline T bezier_curve_length(const cubic_bezier<T,3>& bezier, const std::size_t& point_count)
   {
      std::vector< point3d<T> > curve;

      curve.reserve(point_count);

      generate_bezier(bezier,std::back_inserter(curve),point_count);

      T total_distance = T(0.0);

      for (std::size_t i = 0; i < (curve.size() - 1); ++i)
      {
         total_distance += distance(curve[i],curve[i + 1]);
      }

      return total_distance;
   }

   template <typename T>
   inline triangle<T,2> bezier_convex_hull(const quadratic_bezier<T,2>& bezier)
   {
      return make_triangle(bezier[0],bezier[1],bezier[2]);
   }

   template <typename T> inline quadix<T,2> bezier_convex_hull(const cubic_bezier<T,2>& bezier)
   {
      if (orientation(bezier[0],bezier[2],bezier[1]) != orientation(bezier[0],bezier[2],bezier[3]))
         return make_quadix(bezier[0],bezier[1],bezier[2],bezier[3]);
      else if (orientation(bezier[0],bezier[3],bezier[1]) != orientation(bezier[0],bezier[3],bezier[2]))
         return make_quadix(bezier[0],bezier[1],bezier[3],bezier[2]);
      else
         return make_quadix(bezier[0],bezier[2],bezier[1],bezier[3]);
   }

   template <typename T>
   inline segment<T,2> center_at_location(const segment<T,2>& segment, const T& x, const T& y)
   {
      T cx = T(0.0);
      T cy = T(0.0);

      segment_mid_point(segment,cx,cy);

      return translate(x - cx,y - cy,segment);
   }

   template <typename T>
   inline segment<T,3> center_at_location(const segment<T,3>& segment, const T& x, const T& y, const T& z)
   {
      T cx = T(0.0);
      T cy = T(0.0);
      T cz = T(0.0);

      segment_mid_point(segment,cx,cy,cz);

      return translate(x - cx,y - cy,z - cz,segment);
   }

   template <typename T>
   inline triangle<T,2> center_at_location(const triangle<T,2>& triangle, const T& x, const T& y)
   {
      T cx = T(0.0);
      T cy = T(0.0);

      centroid(triangle,cx,cy);

      return translate(x - cx,y - cy,triangle);
   }

   template <typename T>
   inline rectangle<T> center_at_location(const rectangle<T>& rectangle, const T& x, const T& y)
   {
      T cx = T(0.0);
      T cy = T(0.0);

      centroid(rectangle,cx,cy);

      return translate(x - cx,y - cy,rectangle);
   }

   template <typename T>
   inline box<T,3> center_at_location(const box<T,3>& box, const T& x, const T& y, const T& z)
   {
      T cx = T(0.0);
      T cy = T(0.0);
      T cz = T(0.0);

      centroid(box,cx,cy,cz);

      return translate(x - cx, y - cy, z - cz, box);
   }

   template <typename T>
   inline quadix<T,2> center_at_location(const quadix<T,2>& quadix, const T& x, const T& y)
   {
      T cx = T(0.0);
      T cy = T(0.0);

      centroid(quadix,cx,cy);

      return translate(x - cx,y - cy,quadix);
   }

   template <typename T>
   inline circle<T> center_at_location(const circle<T>& circle, const T& x, const T& y)
   {
      wykobi::circle<T> circle_;

      circle_.x      = x;
      circle_.y      = y;
      circle_.radius = circle.radius;

      return circle_;
   }

   template <typename T>
   inline polygon<T,2> center_at_location(const polygon<T,2>& polygon, const T& x, const T& y)
   {
      T cx = T(0.0);
      T cy = T(0.0);

      centroid(polygon,cx,cy);

      return translate(x - cx,y - cy,polygon);
   }

   template <typename T>
   inline segment<T,2> center_at_location(const segment<T,2>& segment, const point2d<T>& center_point)
   {
      return center_at_location(segment,center_point.x,center_point.y);
   }

   template <typename T>
   inline segment<T,3> center_at_location(const segment<T,3>& segment, const point3d<T>& center_point)
   {
      return center_at_location(segment,center_point.x,center_point.y,center_point.z);
   }

   template <typename T>
   inline triangle<T,2> center_at_location(const triangle<T,2>& triangle, const point2d<T>& center_point)
   {
      return center_at_location(triangle,center_point.x,center_point.y);
   }

   template <typename T>
   inline rectangle<T> center_at_location(const rectangle<T>& rectangle, const point2d<T>& center_point)
   {
      return center_at_location(rectangle,center_point.x,center_point.y);
   }

   template <typename T>
   inline box<T,3> center_at_location(const box<T,3>& box, const point3d<T>& center_point)
   {
      return center_at_location(box,center_point.x,center_point.y,center_point.z);
   }

   template <typename T>
   inline quadix<T,2> center_at_location(const quadix<T,2>& quadix, const point2d<T>& center_point)
   {
      return center_at_location(quadix,center_point.x,center_point.y);
   }

   template <typename T>
   inline circle<T> center_at_location(const circle<T>& circle, const point2d<T>& center_point)
   {
      return center_at_location(circle,center_point.x,center_point.y);
   }

   template <typename T>
   inline polygon<T,2> center_at_location(const polygon<T,2>& polygon, const point2d<T>& center_point)
   {
      return center_at_location(polygon,center_point.x,center_point.y);
   }

   template <typename T>
   inline void shorten_segment(T& x1, T& y1, T& x2, T& y2, const T& amount)
   {
      const T segment_length = distance(x1,y1,x2,y2);

      if (segment_length < amount)
      {
         segment_mid_point(x1,y1,x2,y2,x1,y1);

         x2 = x1;
         y2 = y1;

         return;
      }

      const T dist_ratio = amount / (2 * segment_length);
      const T dx         = x2 - x1;
      const T dy         = y2 - y1;

      x1 = x1 + dist_ratio * dx;
      y1 = y1 + dist_ratio * dy;
      x2 = x2 - dist_ratio * dx;
      y2 = y2 - dist_ratio * dy;
   }

   template <typename T>
   inline void shorten_segment(T& x1, T& y1, T& z1, T& x2, T& y2, T& z2, const T& amount)
   {
      const T segment_length = distance(x1,y1,x2,y2);

      if (segment_length < amount)
      {
         segment_mid_point(x1,y1,z1,x2,y2,z2,x1,y1,z1);

         x2 = x1;
         y2 = y1;
         z2 = y1;

         return;
      }

      const T dist_ratio = amount / (2 * segment_length);
      const T dx         = x2 - x1;
      const T dy         = y2 - y1;

      x1 = x1 + dist_ratio * dx;
      y1 = y1 + dist_ratio * dy;
      x2 = x2 - dist_ratio * dx;
      y2 = y2 - dist_ratio * dy;
   }

   template <typename T>
   inline segment<T,2> shorten_segment(const segment<T,2>& segment, const T& amount)
   {
      wykobi::segment<T,2> segment_ = segment;
      shorten_segment(segment_[0].x,segment_[0].y,segment_[1].x,segment_[1].y,amount);
      return segment_;
   }

   template <typename T>
   inline segment<T,3> shorten_segment(const segment<T,3>& segment, const T& amount)
   {
      wykobi::segment<T,3> segment_ = segment;
      shorten_segment(segment_[0].x,segment_[0].y,segment_[0].z,segment_[1].x,segment_[1].y,segment_[1].z,amount);
      return segment_;
   }

   template <typename T>
   inline void lengthen_segment(T& x1, T& y1, T& x2, T& y2, const T& amount)
   {
      T cx = T(0.0);
      T cy = T(0.0);

      segment_mid_point(x1,y1,x2,y2,cx,cy);

      const T segment_length = distance(x1,y1,x2,y2);
      const T ratio = (amount + segment_length) / segment_length;

      x1 = cx + ratio * (x1 - cx);
      y1 = cy + ratio * (y1 - cy);
      x2 = cx + ratio * (x2 - cx);
      y2 = cy + ratio * (y2 - cy);
   }

   template <typename T>
   inline void lengthen_segment(T& x1, T& y1, T& z1, T& x2, T& y2, T& z2, const T& amount)
   {
      T cx = T(0.0);
      T cy = T(0.0);
      T cz = T(0.0);

      segment_mid_point(x1,y1,z1,x2,y2,z2,cx,cy,cz);

      const T segment_length = distance(x1,y1,z1,x2,y2,z2);
      const T ratio = (amount + segment_length) / segment_length;

      x1 = cx + ratio * (x1 - cx);
      y1 = cy + ratio * (y1 - cy);
      z1 = cy + ratio * (z1 - cy);
      x2 = cx + ratio * (x2 - cx);
      y2 = cy + ratio * (y2 - cy);
      z2 = cy + ratio * (z2 - cy);
   }

   template <typename T>
   inline segment<T,2> lengthen_segment(const segment<T,2>& segment, const T& amount)
   {
      wykobi::segment<T,2> segment_ = segment;
      lengthen_segment(segment_[0].x,segment_[0].y,segment_[1].x,segment_[1].y,amount);
      return segment_;
   }

   template <typename T>
   inline segment<T,3> lengthen_segment(const segment<T,3>& segment, const T& amount)
   {
      wykobi::segment<T,3> segment_ = segment;
      lengthen_segment(segment_[0].x,segment_[0].y,segment_[0].z,segment_[1].x,segment_[1].y,segment_[1].z,amount);
      return segment_;
   }

   template <typename T>
   inline int out_code(const point2d<T>& point, const rectangle<T>& rectangle)
   {
      int result = 0;
      if (point.y < rectangle[0].y)      result |= CLIP_TOP;
      else if (point.y > rectangle[1].y) result |= CLIP_BOTTOM;

      if (point.x < rectangle[0].x)      result |= CLIP_LEFT;
      else if (point.x > rectangle[1].x) result |= CLIP_RIGHT;

      return result;
   }

   template <typename T>
   inline bool clip(const T&  x1, const T&  y1,
                    const T&  x2, const T&  y2,
                    const T&  x3, const T&  y3,
                    const T&  x4, const T&  y4,
                          T& cx1,       T& cy1,
                          T& cx2,       T& cy2)
   {
      if (rectangle_to_rectangle_intersect(x1,y1,x2,y2,x3,y3,x4,y4))
      {
         if (x1 < x3) cx1 = x3; else cx1 = x1;
         if (x2 > x4) cx2 = x4; else cx2 = x2;
         if (y1 < y3) cy1 = y3; else cy1 = y1;
         if (y2 > y4) cy2 = y4; else cy2 = y2;

         return true;
      }
      else
         return false;
   }

   template <typename T>
   inline bool clip(const T&  x1, const T&  y1, const T&  z1,
                    const T&  x2, const T&  y2, const T&  z2,
                    const T&  x3, const T&  y3, const T&  z3,
                    const T&  x4, const T&  y4, const T&  z4,
                          T& cx1,       T& cy1,       T& cz1,
                          T& cx2,       T& cy2,       T& cz2)
   {
      if (box_to_box_intersect(x1,y1,z1,x2,y2,z2,x3,y3,z3,x4,y4,z4))
      {
         if (x1 < x3) cx1 = x3; else cx1 = x1;
         if (x2 > x4) cx2 = x4; else cx2 = x2;
         if (y1 < y3) cy1 = y3; else cy1 = y1;
         if (y2 > y4) cy2 = y4; else cy2 = y2;
         if (z1 < z3) cz1 = z3; else cz1 = z1;
         if (z2 > z4) cz2 = z4; else cz2 = z2;

         return true;
      }
      else
         return false;
   }

   template <typename T>
   inline bool clip(const segment<T,2>& src_segment, const rectangle<T>& rectangle, segment<T,2>& csegment)
   {
      bool result = false;
      T    x      = T(0.0);
      T    y      = T(0.0);
      csegment    = src_segment;

      int outcode0   = out_code(csegment[0],rectangle);
      int outcode1   = out_code(csegment[1],rectangle);
      int outcodeout = 0;

      while ((outcode0 != 0) || (outcode1 != 0))
      {
         if ((outcode0 & outcode1) != 0)
            return result;
         else
         {
            if (outcode0 != 0)
               outcodeout = outcode0;
            else
               outcodeout = outcode1;

            const T dx = (csegment[1].x - csegment[0].x);
            const T dy = (csegment[1].y - csegment[0].y);

            if ((outcodeout & CLIP_TOP) == CLIP_TOP)
            {
               x = csegment[0].x + dx * (rectangle[0].y - csegment[0].y) / dy;
               y = rectangle[0].y;
            }
            else if ((outcodeout & CLIP_BOTTOM) == CLIP_BOTTOM)
            {
               x = csegment[0].x + dx * (rectangle[1].y - csegment[0].y) / dy;
               y = rectangle[1].y;
            }
            else if ((outcodeout & CLIP_RIGHT) == CLIP_RIGHT)
            {
               y = csegment[0].y + dy * (rectangle[1].x - csegment[0].x) / dx;
               x = rectangle[1].x;
            }
            else if ((outcodeout & CLIP_LEFT) == CLIP_LEFT)
            {
               y = csegment[0].y + dy * (rectangle[0].x - csegment[0].x) / dx;
               x = rectangle[0].x;
            }

            if (outcodeout == outcode0)
            {
               csegment[0].x = x;
               csegment[0].y = y;
               outcode0 = out_code(csegment[0],rectangle);
            }
            else
            {
               csegment[1].x = x;
               csegment[1].y = y;
               outcode1 = out_code(csegment[1],rectangle);
            }
         }
      }

      return true;
   }

   template <typename T>
   inline bool clip(const segment<T,2>& src_segment, const triangle<T,2>& triangle, segment<T,2>& csegment)
   {
      if (!intersect(src_segment,triangle)) return false;

      std::size_t pos = 0;

      csegment = src_segment;

      if (intersect(src_segment,edge(triangle,0),csegment[pos].x,csegment[pos].y)) pos++;

      if (intersect(src_segment,edge(triangle,1),csegment[pos].x,csegment[pos].y)) pos++;

      if ((pos < 2) && intersect(src_segment,edge(triangle,2),csegment[pos].x,csegment[pos].y)) pos++;

      if (pos == 1)
      {
         if (point_in_triangle(src_segment[0],triangle))
            csegment[pos] = src_segment[0];
         else
            csegment[pos] = src_segment[1];
      }
      return true;
   }

   template <typename T>
   inline bool clip(const segment<T,2>& src_segment, const quadix<T,2>& quadix, segment<T,2>& csegment)
   {
      if (!intersect(src_segment,quadix)) return false;

      std::size_t pos = 0;

      csegment = src_segment;

      if (intersect(src_segment,edge(quadix,0),csegment[pos].x,csegment[pos].y)) pos++;
      if (intersect(src_segment,edge(quadix,1),csegment[pos].x,csegment[pos].y)) pos++;

      if ((pos < 2) && (intersect(src_segment,edge(quadix,2),csegment[pos].x,csegment[pos].y))) pos++;
      if ((pos < 2) && (intersect(src_segment,edge(quadix,3),csegment[pos].x,csegment[pos].y))) pos++;

      if (pos == 1)
      {
         if (point_in_quadix(src_segment[0],quadix))
            csegment[pos] = src_segment[0];
         else
            csegment[pos] = src_segment[1];
      }
      return true;
   }

   template <typename T>
   inline bool clip(const segment<T,2>& src_segment, const circle<T>& circle, segment<T,2>& csegment)
   {
      std::vector< point2d<T> > int_point;

      intersection_point(src_segment,circle,std::back_inserter(int_point));

      if (int_point.size() == 2)
      {
         csegment[0] = int_point[0];
         csegment[1] = int_point[1];
         return true;
      }
      else
         return false;
   }

   template <typename T>
   inline bool clip(const rectangle<T>& rectangle1, const rectangle<T>&  rectangle2, rectangle<T>& crectangle)
   {
      return clip
             (
               rectangle1[0].x, rectangle1[0].y, rectangle1[1].x, rectangle1[1].y,
               rectangle2[0].x, rectangle2[0].y, rectangle2[1].x, rectangle2[1].y,
               crectangle[0].x, crectangle[0].y, crectangle[1].x, crectangle[1].y
             );
   }

   template <typename T>
   inline bool clip(const box<T,3>& box1, const box<T,3>& box2, box<T,3>& cbox)
   {
      return clip
             (
               box1[0].x, box1[0].y, box1[0].z, box1[1].x, box1[1].y, box1[1].z,
               box2[0].x, box2[0].y, box2[0].z, box2[1].x, box2[1].y, box2[1].z,
               cbox[0].x, cbox[0].y, cbox[0].z, cbox[1].x, cbox[1].y, cbox[1].z
             );
   }

   template <typename T>
   inline T area(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3)
   {
      return T(0.5) * (
                        (point1.x * (point2.y - point3.y)) +
                        (point2.x * (point3.y - point1.y)) +
                        (point3.x * (point1.y - point2.y))
                      );
   }

   template <typename T>
   inline T area(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3)
   {
      const T dx1 = point2.x - point1.x;
      const T dy1 = point2.y - point1.y;
      const T dz1 = point2.z - point1.z;

      const T dx2 = point3.x - point1.x;
      const T dy2 = point3.y - point1.y;
      const T dz2 = point3.z - point1.z;

      const T cx  = dy1 * dz2 - dy2 * dz1;
      const T cy  = dx2 * dz1 - dx1 * dz2;
      const T cz  = dx1 * dy2 - dx2 * dy1;

      return (sqrt(cx * cx + cy * cy + cz * cz) * T(0.5));
   }

   template <typename T>
   inline T area(const triangle<T,2>& triangle)
   {
      return T(0.5) * (
                        (triangle[0].x * (triangle[1].y - triangle[2].y)) +
                        (triangle[1].x * (triangle[2].y - triangle[0].y)) +
                        (triangle[2].x * (triangle[0].y - triangle[1].y))
                      );
   }

   template <typename T>
   inline T area(const triangle<T,3>& triangle)
   {
      const T dx1 = triangle[1].x - triangle[0].x;
      const T dy1 = triangle[1].y - triangle[0].y;
      const T dz1 = triangle[1].z - triangle[0].z;

      const T dx2 = triangle[2].x - triangle[0].x;
      const T dy2 = triangle[2].y - triangle[0].y;
      const T dz2 = triangle[2].z - triangle[0].z;

      const T cx  = dy1 * dz2 - dy2 * dz1;
      const T cy  = dx2 * dz1 - dx1 * dz2;
      const T cz  = dx1 * dy2 - dx2 * dy1;

      return (sqrt(cx * cx + cy * cy + cz * cz) * T(0.5));
   }

   template <typename T>
   inline T area(const quadix<T,2>& quadix)
   {
      return T(0.5) * (
                        (quadix[0].x * (quadix[1].y - quadix[3].y)) +
                        (quadix[1].x * (quadix[2].y - quadix[0].y)) +
                        (quadix[2].x * (quadix[3].y - quadix[1].y)) +
                        (quadix[3].x * (quadix[0].y - quadix[2].y))
                      );
   }

   template <typename T>
   inline T area(const quadix<T,3>& quadix)
   {
      return (
              area(make_triangle(quadix[0], quadix[1], quadix[2])) +
              area(make_triangle(quadix[2], quadix[3], quadix[0]))
             );
   }

   template <typename T>
   inline T area(const rectangle<T>& rectangle)
   {
      return abs(rectangle[1].x - rectangle[0].x) * abs(rectangle[1].y - rectangle[0].y);
   }

   template <typename T>
   inline T area(const circle<T>& circle)
   {
      return T(PI) * circle.radius * circle.radius;
   }

   template <typename T>
   inline T area(const polygon<T,2>& polygon)
   {
      if (polygon.size() < 3) return T(0.0);

      T result = T(0.0);

      std::size_t j = polygon.size() - 1;

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         result += ((polygon[j].x * polygon[i].y) - (polygon[j].y * polygon[i].x));
         j = i;
      }

      return abs<T>(result * T(0.5));
   }

   template <typename T>
   inline T perimeter(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3)
   {
      return distance(point1,point2) + distance(point2,point3) + distance(point3,point1);
   }

   template <typename T>
   inline T perimeter(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3)
   {
      return distance(point1,point2) + distance(point2,point3) + distance(point3,point1);
   }

   template <typename T>
   inline T perimeter(const triangle<T,2>& triangle)
   {
      return perimeter(triangle[0],triangle[1],triangle[2]);
   }

   template <typename T>
   inline T perimeter(const triangle<T,3>& triangle)
   {
      return perimeter(triangle[0],triangle[1],triangle[2]);
   }

   template <typename T>
   inline T perimeter(const quadix<T,2>& quadix)
   {
      return distance(quadix[0], quadix[1]) +
             distance(quadix[1], quadix[2]) +
             distance(quadix[2], quadix[3]) +
             distance(quadix[3], quadix[0]) ;
   }

   template <typename T>
   inline T perimeter(const quadix<T,3>& quadix)
   {
      return distance(quadix[0], quadix[1]) +
             distance(quadix[1], quadix[2]) +
             distance(quadix[2], quadix[3]) +
             distance(quadix[3], quadix[0]) ;
   }

   template <typename T>
   inline T perimeter(const rectangle<T>& rectangle)
   {
      return T(2.0) * (abs(rectangle[1].x - rectangle[0].x) + abs(rectangle[1].y - rectangle[0].y));
   }

   template <typename T>
   inline T perimeter(const circle<T>& circle)
   {
      return T(2.0) * T(PI) * circle.radius;
   }

   template <typename T>
   inline T perimeter(const polygon<T,2>& polygon)
   {
      if (polygon.size() < 3) return T(0.0);

      T total_perimeter = T(0.0);

      std::size_t j = polygon.size() - 1;

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         total_perimeter += distance(polygon[j],polygon[i]);
         j = i;
      }

      return total_perimeter;
   }

   template <typename T>
   inline void rotate(const T& rotation_angle, const T& x, const T& y, T& nx, T& ny)
   {
      const T sin_val = sin(rotation_angle * T(PIDiv180));
      const T cos_val = cos(rotation_angle * T(PIDiv180));

      nx = (x * cos_val) - (y * sin_val);
      ny = (y * cos_val) + (x * sin_val);
   }

   template <typename T>
   inline void rotate(const T& rotation_angle, const T& x, const T& y, const T& ox, const T& oy, T& nx, T& ny)
   {
      rotate(rotation_angle, x - ox, y - oy, nx, ny);

      nx += ox;
      ny += oy;
   }

   template <typename T>
   inline point2d<T> rotate(const T& rotation_angle, const point2d<T>& point)
   {
      point2d<T> point_;
      rotate(rotation_angle,point.x,point.y,point_.x,point_.y);
      return point_;
   }

   template <typename T>
   inline point2d<T> rotate(const T& rotation_angle, const point2d<T>& point, const point2d<T>& opoint)
   {
      point2d<T> point_;
      rotate(rotation_angle,point.x, point.y,opoint.x,opoint.y,point_.x,point_.y);
      return point_;
   }

   template <typename T>
   inline segment<T,2> rotate(const T& rotation_angle, const segment<T,2>& segment)
   {
      wykobi::segment<T,2> segment_;

      for (std::size_t i = 0; i < wykobi::segment<T,2>::PointCount; ++i)
      {
         segment_[i] = rotate(rotation_angle,segment[i]);
      }

      return segment_;
   }

   template <typename T>
   inline segment<T,2> rotate(const T& rotation_angle, const segment<T,2>& segment, const point2d<T>& opoint)
   {
      wykobi::segment<T,2> segment_;

      for (std::size_t i = 0; i < wykobi::segment<T,2>::PointCount; ++i)
      {
         segment_[i] = rotate(rotation_angle,segment[i],opoint);
      }

      return segment_;
   }

   template <typename T>
   inline triangle<T,2> rotate(const T& rotation_angle, const triangle<T,2>& triangle)
   {
      wykobi::triangle<T,2> triangle_;

      for (std::size_t i = 0; i < wykobi::triangle<T,2>::PointCount; ++i)
      {
         triangle_[i] = rotate(rotation_angle,triangle[i]);
      }

      return triangle_;
   }

   template <typename T>
   inline triangle<T,2> rotate(const T& rotation_angle, const triangle<T,2>& triangle, const point2d<T>& opoint)
   {
      wykobi::triangle<T,2> triangle_;

      for (std::size_t i = 0; i < wykobi::triangle<T,2>::PointCount; ++i)
      {
         triangle_[i] = rotate(rotation_angle,triangle[i],opoint);
      }

      return triangle_;
   }

   template <typename T>
   inline quadix<T,2> rotate(const T& rotation_angle, const quadix<T,2>& quadix)
   {
      wykobi::quadix<T,2> quadix_;

      for (std::size_t i = 0; i < wykobi::quadix<T,2>::PointCount; ++i)
      {
         quadix_[i] = rotate(rotation_angle,quadix[i]);
      }

      return quadix_;
   }

   template <typename T>
   inline quadix<T,2> rotate(const T& rotation_angle, const quadix<T,2>& quadix, const point2d<T>& opoint)
   {
      wykobi::quadix<T,2> quadix_;

      for (std::size_t i = 0; i < wykobi::quadix<T,2>::PointCount; ++i)
      {
         quadix_[i] = rotate(rotation_angle,quadix[i],opoint);
      }

      return quadix_;
   }

   template <typename T>
   inline polygon<T,2> rotate(const T& rotation_angle, const polygon<T,2>& polygon)
   {
      wykobi::polygon<T,2> polygon_;

      polygon_.reserve(polygon.size());

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         polygon_.push_back(rotate(rotation_angle,polygon[i]));
      }

      return polygon_;
   }

   template <typename T>
   inline polygon<T,2> rotate(const T& rotation_angle, const polygon<T,2>& polygon, const point2d<T>& opoint)
   {
      wykobi::polygon<T,2> polygon_;

      polygon_.reserve(polygon.size());

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         polygon_.push_back(rotate(rotation_angle,polygon[i],opoint));
      }

      return polygon_;
   }

   template <typename T>
   inline void rotate(const T& rx, const T& ry, const T& rz,
                      const T&  x, const T&  y, const T&  z,
                            T& nx,       T& ny,       T& nz)
   {
      const T xradang = rx * T(PIDiv180);
      const T yradang = ry * T(PIDiv180);
      const T zradang = rz * T(PIDiv180);

      const T sinx  = sin(xradang);
      const T siny  = sin(yradang);
      const T sinz  = sin(zradang);

      const T cosx  = cos(xradang);
      const T cosy  = cos(yradang);
      const T cosz  = cos(zradang);

      const T tempy = y * cosy -     z * siny;
      const T tempz = y * siny +     z * cosy;
      const T tempx = x * cosx - tempz * sinx;

      nz =     x * sinx + tempz * cosx;
      nx = tempx * cosz - tempy * sinz;
      ny = tempx * sinz + tempy * cosz;
   }

   template <typename T>
   inline void rotate(const T& rx, const T& ry, const T& rz,
                      const T&  x, const T&  y, const T&  z,
                      const T& ox, const T& oy, const T& oz,
                            T& nx,       T& ny,       T& nz)
   {
      rotate(rx,ry,rz,x - ox,y - oy,z - oz,nx,ny,nz);

      nx += ox;
      ny += oy;
      nz += oz;
   }

   template <typename T>
   inline point3d<T> rotate(const T& rx, const T& ry, const T& rz, const point3d<T>& point)
   {
      point3d<T> point_;
      rotate(rx,ry,rz,point.x,point.y,point.z,point_.x,point_.y,point_.z);
      return point_;
   }

   template <typename T>
   inline point3d<T> rotate(const T& rx, const T& ry, const T& rz, const point3d<T>& point, const point3d<T>& opoint)
   {
      point3d<T> point_;
      rotate(rx,ry,rz,point.x,point.y,point.z,opoint.x,opoint.y,opoint.z,point_.x,point_.y,point_.z);
      return point_;
   }

   template <typename T>
   inline segment<T,3> rotate(const T& rx, const T& ry, const T& rz, const segment<T,3>& segment)
   {
      wykobi::segment<T,3> segment_;

      for (std::size_t i = 0; i < wykobi::segment<T,3>::PointCount; ++i)
      {
         segment_[i] = rotate(rx,ry,rz,segment[i]);
      }

      return segment_;
   }

   template <typename T>
   inline segment<T,3> rotate(const T& rx, const T& ry, const T& rz, const segment<T,3>& segment, const point3d<T>& opoint)
   {
      wykobi::segment<T,3> segment_;

      for (std::size_t i = 0; i < wykobi::segment<T,3>::PointCount; ++i)
      {
         segment_[i] = rotate(rx,ry,rz,segment[i],opoint);
      }

      return segment_;
   }

   template <typename T>
   inline triangle<T,3> rotate(const T& rx, const T& ry, const T& rz, const triangle<T,3>& triangle)
   {
      wykobi::triangle<T,3> triangle_;

      for (std::size_t i = 0; i < wykobi::triangle<T,3>::PointCount; ++i)
      {
         triangle_[i] = rotate(rx,ry,rz,triangle[i]);
      }

      return triangle_;
   }

   template <typename T>
   inline triangle<T,3> rotate(const T& rx, const T& ry, const T& rz, const triangle<T,3>& triangle, const point3d<T>& opoint)
   {
      wykobi::triangle<T,3> triangle_;

      for (std::size_t i = 0; i < wykobi::triangle<T,3>::PointCount; ++i)
      {
         triangle_[i] = rotate(rx,ry,rz,triangle[i],opoint);
      }

      return triangle_;
   }

   template <typename T>
   inline quadix<T,3> rotate(const T& rx, const T& ry, const T& rz, const quadix<T,3>& quadix)
   {
      wykobi::quadix<T,3> quadix_;

      for (std::size_t i = 0; i < wykobi::quadix<T,3>::PointCount; ++i)
      {
         quadix_[i] = rotate(rx,ry,rz,quadix[i]);
      }

      return quadix_;
   }

   template <typename T>
   inline quadix<T,3> rotate(const T& rx, const T& ry, const T& rz, const quadix<T,3>& quadix, const point3d<T>& opoint)
   {
      wykobi::quadix<T,3> quadix_;

      for (std::size_t i = 0; i < wykobi::quadix<T,3>::PointCount; ++i)
      {
         quadix_[i] = rotate(rx,ry,rz,quadix[i],opoint);
      }

      return quadix_;
   }

   template <typename T>
   inline polygon<T,3> rotate(const T& rx, const T& ry, const T& rz, const polygon<T,3>& polygon)
   {
      wykobi::polygon<T,3> polygon_;

      polygon_.reserve(polygon.size());

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         polygon_.push_back(rotate(rx,ry,rz,polygon[i]));
      }

      return polygon_;
   }

   template <typename T>
   inline polygon<T,3> rotate(const T& rx, const T& ry, const T& rz, const polygon<T,3>& polygon, const point3d<T>& opoint)
   {
      wykobi::polygon<T,3> polygon_;

      polygon_.reserve(polygon.size());

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         polygon_.push_back(rotate(rx,ry,rz,polygon[i],opoint));
      }

      return polygon_;
   }

   template <typename T>
   inline void fast_rotate(const trig_luts<T>& lut,
                           const int rotation_angle,
                           const T& x, const T& y,
                           T& nx, T& ny)
   {
      int rot_ang = rotation_angle % 360;

      if (rot_ang < 0)
      {
         rot_ang = 360 + rot_ang;
      }

      const T sin_val = T(lut.sin(static_cast<unsigned int>(rot_ang)));
      const T cos_val = T(lut.cos(static_cast<unsigned int>(rot_ang)));

      nx = (x * cos_val) - (y * sin_val);
      ny = (y * cos_val) + (x * sin_val);
   }

   template <typename T>
   inline void fast_rotate(const trig_luts<T>& lut,
                           const int rotation_angle,
                           const T& x, const T& y, const T& ox, const T& oy,
                           T& nx, T& ny)
   {
      fast_rotate(lut, rotation_angle, x - ox, y - oy, nx, ny);

      nx += ox;
      ny += oy;
   }

   template <typename T>
   inline point2d<T> fast_rotate(const trig_luts<T>& lut,
                                 const int rotation_angle,
                                 const point2d<T>& point)
   {
      point2d<T> point_;

      fast_rotate(lut,
                  rotation_angle,
                  point .x, point .y,
                  point_.x, point_.y);

      return point_;
   }

   template <typename T>
   inline point2d<T> fast_rotate(const trig_luts<T>& lut,
                                 const int rotation_angle,
                                 const point2d<T>& point,
                                 const point2d<T>& opoint)
   {
      point2d<T> point_;

      fast_rotate(lut,
                  rotation_angle,
                  point .x, point .y,
                  opoint.x, opoint.y,
                  point_.x, point_.y);

      return point_;
   }

   template <typename T>
   inline segment<T,2> fast_rotate(const trig_luts<T>& lut,
                                   const int rotation_angle,
                                   const segment<T,2>& segment)
   {
      wykobi::segment<T,2> segment_;

      for (std::size_t i = 0; i < wykobi::segment<T,2>::PointCount; ++i)
      {
         segment_[i] = fast_rotate(lut, rotation_angle, segment[i]);
      }

      return segment_;
   }

   template <typename T>
   inline segment<T,2> fast_rotate(const trig_luts<T>& lut,
                                   const int rotation_angle,
                                   const segment<T,2>& segment,
                                   const point2d<T>& opoint)
   {
      wykobi::segment<T,2> segment_;

      for (std::size_t i = 0; i < wykobi::segment<T,2>::PointCount; ++i)
      {
         segment_[i] = fast_rotate(lut, rotation_angle, segment[i], opoint);
      }

      return segment_;
   }

   template <typename T>
   inline triangle<T,2> fast_rotate(const trig_luts<T>& lut,
                                    const int rotation_angle,
                                    const triangle<T,2>& triangle)
   {
      wykobi::triangle<T,2> triangle_;

      for (std::size_t i = 0; i < wykobi::triangle<T,2>::PointCount; ++i)
      {
         triangle_[i] = fast_rotate(lut, rotation_angle, triangle[i]);
      }

      return triangle_;
   }

   template <typename T>
   inline triangle<T,2> fast_rotate(const trig_luts<T>& lut,
                                    const int rotation_angle,
                                    const triangle<T,2>& triangle,
                                    const point2d<T>& opoint)
   {
      wykobi::triangle<T,2> triangle_;

      for (std::size_t i = 0; i < wykobi::triangle<T,2>::PointCount; ++i)
      {
         triangle_[i] = fast_rotate(lut, rotation_angle, triangle[i], opoint);
      }

      return triangle_;
   }

   template <typename T>
   inline quadix<T,2> fast_rotate(const trig_luts<T>& lut,
                                  const int rotation_angle,
                                  const quadix<T,2>& quadix)
   {
      wykobi::quadix<T,2> quadix_;

      for (std::size_t i = 0; i < wykobi::quadix<T,2>::PointCount; ++i)
      {
         quadix_[i] = fast_rotate(lut, rotation_angle, quadix[i]);
      }

      return quadix_;
   }

   template <typename T>
   inline quadix<T,2> fast_rotate(const trig_luts<T>& lut,
                                  const int rotation_angle,
                                  const quadix<T,2>& quadix,
                                  const point2d<T>& opoint)
   {
      wykobi::quadix<T,2> quadix_;

      for (std::size_t i = 0; i < wykobi::quadix<T,2>::PointCount; ++i)
      {
         quadix_[i] = fast_rotate(lut, rotation_angle, quadix[i], opoint);
      }

      return quadix_;
   }

   template <typename T>
   inline polygon<T,2> fast_rotate(const trig_luts<T>& lut,
                                   const int rotation_angle,
                                   const polygon<T,2>& polygon)
   {
      wykobi::polygon<T,2> polygon_;

      polygon_.reserve(polygon.size());

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         polygon_.push_back(fast_rotate(lut, rotation_angle, polygon[i]));
      }

      return polygon_;
   }

   template <typename T>
   inline polygon<T,2> fast_rotate(const trig_luts<T>& lut,
                                   const int rotation_angle,
                                   const polygon<T,2>& polygon,
                                   const point2d<T>& opoint)
   {
      wykobi::polygon<T,2> polygon_;

      polygon_.reserve(polygon.size());

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         polygon_.push_back(fast_rotate(lut, rotation_angle, polygon[i], opoint));
      }

      return polygon_;
   }

   template <typename T>
   inline void fast_rotate(const trig_luts<T>& lut,
                           const int rx, const int ry, const int rz,
                           const T&   x, const T&   y, const T&   z,
                                 T&  nx,        T& ny,        T& nz)
   {
      int rx_ = rx % 360;
      int ry_ = ry % 360;
      int rz_ = rz % 360;

      if (rx_ < 0) rx_ += 360;
      if (ry_ < 0) ry_ += 360;
      if (rz_ < 0) rz_ += 360;

      const T sin_x = T(lut.sin(static_cast<unsigned int>(rx_)));
      const T sin_y = T(lut.sin(static_cast<unsigned int>(ry_)));
      const T sin_z = T(lut.sin(static_cast<unsigned int>(rz_)));

      const T cos_x = T(lut.cos(static_cast<unsigned int>(rx_)));
      const T cos_y = T(lut.cos(static_cast<unsigned int>(ry_)));
      const T cos_z = T(lut.cos(static_cast<unsigned int>(rz_)));

      const T tmp_y = y * cos_y -     z * sin_y;
      const T tmp_z = y * sin_y +     z * cos_y;
      const T tmp_x = x * cos_x - tmp_z * sin_x;

      nz =     x * sin_x + tmp_z * cos_x;
      nx = tmp_x * cos_z - tmp_y * sin_z;
      ny = tmp_x * sin_z + tmp_y * cos_z;
   }

   template <typename T>
   inline void fast_rotate(const trig_luts<T>& lut,
                           const int rx, const int ry, const int rz,
                           const T&   x, const T&   y, const T&   z,
                           const T&  ox, const T&  oy, const T&  oz,
                                 T&  nx,       T&  ny,       T&  nz)
   {
      fast_rotate(lut, rx, ry, rz, x - ox, y - oy, z - oz, nx, ny, nz);

      nx += ox;
      ny += oy;
      nz += oz;
   }

   template <typename T>
   inline point3d<T> fast_rotate(const trig_luts<T>& lut,
                                 const int rx, const int ry, const int rz,
                                 const point3d<T>& point)
   {
      point3d<T> point_;

      fast_rotate(lut,
                  rx, ry, rz,
                  point .x, point .y, point .z,
                  point_.x, point_.y, point_.z);

      return point_;
   }

   template <typename T>
   inline point3d<T> fast_rotate(const trig_luts<T>& lut,
                                 const int rx, const int ry, const int rz,
                                 const point3d<T>& point,
                                 const point3d<T>& opoint)
   {
      point3d<T> point_;

      fast_rotate(lut,
                  rx, ry, rz,
                  point .x, point .y, point .z,
                  opoint.x, opoint.y, opoint.z,
                  point_.x, point_.y, point_.z);

      return point_;
   }

   template <typename T>
   inline segment<T,3> fast_rotate(const trig_luts<T>& lut,
                                   const int rx, const int ry, const int rz,
                                   const segment<T,3>& segment)
   {
      wykobi::segment<T,3> segment_;

      for (std::size_t i = 0; i < wykobi::segment<T,3>::PointCount; ++i)
      {
         segment_[i] = fast_rotate(lut, rx, ry, rz, segment[i]);
      }

      return segment_;
   }

   template <typename T>
   inline segment<T,3> fast_rotate(const trig_luts<T>& lut,
                                   const int rx, const int ry, const int rz,
                                   const segment<T,3>& segment,
                                   const point3d<T>& opoint)
   {
      wykobi::segment<T,3> segment_;

      for (std::size_t i = 0; i < wykobi::segment<T,3>::PointCount; ++i)
      {
         segment_[i] = fast_rotate(lut, rx, ry, rz, segment[i], opoint);
      }

      return segment_;
   }

   template <typename T>
   inline triangle<T,3> fast_rotate(const trig_luts<T>& lut,
                                    const int rx, const int ry, const int rz,
                                    const triangle<T,3>& triangle)
   {
      wykobi::triangle<T,3> triangle_;

      for (std::size_t i = 0; i < wykobi::triangle<T,3>::PointCount; ++i)
      {
         triangle_[i] = fast_rotate(lut, rx, ry, rz, triangle[i]);
      }

      return triangle_;
   }

   template <typename T>
   inline triangle<T,3> fast_rotate(const trig_luts<T>& lut,
                                    const int rx, const int ry, const int rz,
                                    const triangle<T,3>& triangle,
                                    const point3d<T>& opoint)
   {
      wykobi::triangle<T,3> triangle_;

      for (std::size_t i = 0; i < wykobi::triangle<T,3>::PointCount; ++i)
      {
         triangle_[i] = fast_rotate(lut, rx, ry, rz, triangle[i], opoint);
      }

      return triangle_;
   }

   template <typename T>
   inline quadix<T,3> fast_rotate(const trig_luts<T>& lut,
                                  const int rx, const int ry, const int rz,
                                  const quadix<T,3>& quadix)
   {
      wykobi::quadix<T,3> quadix_;

      for (std::size_t i = 0; i < wykobi::quadix<T,3>::PointCount; ++i)
      {
         quadix_[i] = fast_rotate(lut, rx, ry, rz, quadix[i]);
      }

      return quadix_;
   }

   template <typename T>
   inline quadix<T,3> fast_rotate(const trig_luts<T>& lut,
                                  const int rx, const int ry, const int rz,
                                  const quadix<T,3>& quadix,
                                  const point3d<T>& opoint)
   {
      wykobi::quadix<T,3> quadix_;

      for (std::size_t i = 0; i < wykobi::quadix<T,3>::PointCount; ++i)
      {
         quadix_[i] = fast_rotate(lut, rx, ry, rz, quadix[i], opoint);
      }

      return quadix_;
   }

   template <typename T>
   inline polygon<T,3> fast_rotate(const trig_luts<T>& lut,
                                   const int rx, const int ry, const int rz,
                                   const polygon<T,3>& polygon)
   {
      wykobi::polygon<T,3> polygon_;

      polygon_.reserve(polygon.size());

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         polygon_.push_back(fast_rotate(lut, rx, ry, rz, polygon[i]));
      }

      return polygon_;
   }

   template <typename T>
   inline polygon<T,3> fast_rotate(const trig_luts<T>& lut,
                                   const int rx, const int ry, const int rz,
                                   const polygon<T,3>& polygon,
                                   const point3d<T>& opoint)
   {
      wykobi::polygon<T,3> polygon_;

      polygon_.reserve(polygon.size());

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         polygon_.push_back(fast_rotate(lut, rx, ry, rz, polygon[i], opoint));
      }

      return polygon_;
   }

   template <typename T>
   inline point2d<T> translate(const T& dx, const T& dy, const point2d<T>& point)
   {
      wykobi::point2d<T> point_;

      point_.x = point.x + dx;
      point_.y = point.y + dy;

      return point_;
   }

   template <typename T>
   inline line<T,2> translate(const T& dx, const T& dy, const line<T,2>& line)
   {
      wykobi::line<T,2> line_;

      for (std::size_t i = 0; i < wykobi::line<T,2>::PointCount; ++i)
      {
         line_[i] = translate(dx,dy,line[i]);
      }

      return line_;
   }

   template <typename T>
   inline segment<T,2> translate(const T& dx, const T& dy, const segment<T,2>& segment)
   {
      wykobi::segment<T,2> segment_;

      for (std::size_t i = 0; i < wykobi::segment<T,2>::PointCount; ++i)
      {
         segment_[i] = translate(dx,dy,segment[i]);
      }

      return segment_;
   }

   template <typename T>
   inline triangle<T,2> translate(const T& dx, const T& dy, const triangle<T,2>& triangle)
   {
      wykobi::triangle<T,2> triangle_;

      for (std::size_t i = 0; i < wykobi::triangle<T,2>::PointCount; ++i)
      {
         triangle_[i] = translate(dx,dy,triangle[i]);
      }

      return triangle_;
   }

   template <typename T>
   inline quadix<T,2> translate(const T& dx, const T& dy, const quadix<T,2>& quadix)
   {
      wykobi::quadix<T,2> quadix_;

      for (std::size_t i = 0; i < wykobi::quadix<T,2>::PointCount; ++i)
      {
         quadix_[i] = translate(dx,dy,quadix[i]);
      }

      return quadix_;
   }

   template <typename T>
   inline rectangle<T> translate(const T& dx, const T& dy, const rectangle<T>& rectangle)
   {
      wykobi::rectangle<T> rectangle_;

      for (std::size_t i = 0; i < wykobi::rectangle<T>::PointCount; ++i)
      {
         rectangle_[i] = translate(dx,dy,rectangle[i]);
      }

      return rectangle_;
   }

   template <typename T>
   inline circle<T> translate(const T& dx, const T& dy, const circle<T>& circle)
   {
      wykobi::circle<T> circle_;

      circle_.x      = circle.x + dx;
      circle_.y      = circle.y + dy;
      circle_.radius = circle.radius;

      return circle_;
   }

   template <typename T>
   inline polygon<T,2> translate(const T& dx, const T& dy, const polygon<T,2>& polygon)
   {
      wykobi::polygon<T,2> polygon_;

      polygon_.reserve(polygon.size());

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         polygon_.push_back(translate(dx,dy,polygon[i]));
      }

      return polygon_;
   }

   template <typename T>
   inline point2d<T> translate(const T& delta, const point2d<T>& point)
   {
      return translate(delta,delta,point);
   }

   template <typename T>
   inline line<T,2> translate(const T& delta, const line<T,2>& line)
   {
      return translate(delta,delta,line);
   }

   template <typename T>
   inline segment<T,2> translate(const T& delta, const segment<T,2>& segment)
   {
      return translate(delta,delta,segment);
   }

   template <typename T>
   inline triangle<T,2> translate(const T& delta, const triangle<T,2>& triangle)
   {
      return translate(delta,delta,triangle);
   }

   template <typename T>
   inline quadix<T,2> translate(const T& delta, const quadix<T,2>& quadix)
   {
      return translate(delta,delta,quadix);
   }

   template <typename T>
   inline rectangle<T> translate(const T& delta, const rectangle<T>& rectangle)
   {
      return translate(delta,delta,rectangle);
   }

   template <typename T>
   inline circle<T> translate(const T& delta, const circle<T>& circle)
   {
      return translate(delta,delta,circle);
   }

   template <typename T>
   inline polygon<T,2> translate(const T& delta, const polygon<T,2>& polygon)
   {
      return translate(delta,delta,polygon);
   }

   template <typename T>
   inline point2d<T> translate(const vector2d<T>& v, const point2d<T>& point)
   {
      return translate(v.x,v.y,point);
   }

   template <typename T>
   inline line<T,2> translate(const vector2d<T>& v, const line<T,2>& line)
   {
      return translate(v.x,v.y,line);
   }

   template <typename T>
   inline segment<T,2> translate(const vector2d<T>& v, const segment<T,2>& segment)
   {
      return translate(v.x,v.y,segment);
   }

   template <typename T>
   inline triangle<T,2> translate(const vector2d<T>& v, const triangle<T,2>& triangle)
   {
      return translate(v.x,v.y,triangle);
   }

   template <typename T>
   inline quadix<T,2> translate(const vector2d<T>& v, const quadix<T,2>& quadix)
   {
      return translate(v.x,v.y,quadix);
   }

   template <typename T>
   inline rectangle<T> translate(const vector2d<T>& v, const rectangle<T>& rectangle)
   {
      return translate(v.x,v.y,rectangle);
   }

   template <typename T>
   inline circle<T> translate(const vector2d<T>& v, const circle<T>& circle)
   {
      return translate(v.x,v.y,circle);
   }

   template <typename T>
   inline polygon<T,2> translate(const vector2d<T>& v, const polygon<T,2>& polygon)
   {
      return translate(v.x,v.y,polygon);
   }

   template <typename T>
   inline point3d<T> translate(const T& dx, const T& dy, const T& dz, const point3d<T>& point)
   {
      point3d<T> point_;

      point_.x = point.x + dx;
      point_.y = point.y + dy;
      point_.z = point.z + dz;

      return point_;
   }

   template <typename T>
   inline line<T,3> translate(const T& dx, const T& dy, const T& dz, const line<T,3>& line)
   {
      wykobi::line<T,3> line_;

      for (std::size_t i = 0; i < wykobi::line<T,3>::PointCount; ++i)
      {
         line_[i] = translate(dx,dy,dz,line[i]);
      }

      return line_;
   }

   template <typename T>
   inline segment<T,3> translate(const T& dx, const T& dy, const T& dz, const segment<T,3>& segment)
   {
      wykobi::segment<T,3> segment_;

      for (std::size_t i = 0; i < wykobi::segment<T,3>::PointCount; ++i)
      {
         segment_[i] = translate(dx,dy,dz,segment[i]);
      }

      return segment_;
   }

   template <typename T>
   inline triangle<T,3> translate(const T& dx, const T& dy, const T& dz, const triangle<T,3>& triangle)
   {
      wykobi::triangle<T,3> triangle_;

      for (std::size_t i = 0; i < wykobi::triangle<T,3>::PointCount; ++i)
      {
         triangle_[i] = translate(dx,dy,dz,triangle[i]);
      }

      return triangle_;
   }

   template <typename T>
   inline quadix<T,3> translate(const T& dx, const T& dy, const T& dz, const quadix<T,3>& quadix)
   {
      wykobi::quadix<T,3> quadix_;

      for (std::size_t i = 0; i < wykobi::quadix<T,3>::PointCount; ++i)
      {
         quadix_[i] = translate(dx,dy,dz,quadix[i]);
      }

      return quadix_;
   }

   template <typename T>
   inline box<T,3> translate(const T& dx, const T& dy, const T& dz, const box<T,3>& box)
   {
      wykobi::box<T,3> box_;

      for (std::size_t i = 0; i < wykobi::box<T,3>::PointCount; ++i)
      {
         box_[i] = translate(dx,dy,dz,box[i]);
      }

      return box_;
   }

   template <typename T>
   inline sphere<T> translate(const T& dx, const T& dy, const T& dz, const sphere<T>& sphere)
   {
      wykobi::sphere<T> sphere_;

      sphere_.x = sphere.x + dx;
      sphere_.y = sphere.y + dy;
      sphere_.z = sphere.z + dz;
      sphere_.radius = sphere.radius;

      return sphere_;
   }

   template <typename T>
   inline polygon<T,3> translate(const T& dx, const T& dy, const T& dz, const polygon<T,3>& polygon)
   {
      wykobi::polygon<T,3> polygon_;

      polygon_.reserve(polygon.size());

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         polygon_.push_back(translate(dx,dy,dz,polygon[i]));
      }

      return polygon_;
   }

   template <typename T>
   inline point3d<T> translate(const T& delta, const point3d<T>& point)
   {
      return translate(delta,delta,delta,point);
   }

   template <typename T>
   inline line<T,3> translate(const T& delta, const line<T,3>& line)
   {
      return translate(delta,delta,delta,line);
   }

   template <typename T>
   inline segment<T,3> translate(const T& delta, const segment<T,3>& segment)
   {
      return translate(delta,delta,delta,segment);
   }

   template <typename T>
   inline triangle<T,3> translate(const T& delta, const triangle<T,3>& triangle)
   {
      return translate(delta,delta,delta,triangle);
   }

   template <typename T>
   inline quadix<T,3> translate(const T& delta, const quadix<T,3>& quadix)
   {
      return translate(delta,delta,delta,quadix);
   }

   template <typename T>
   inline box<T,3> translate(const T& delta, const box<T,3>& box)
   {
      return translate(delta,delta,delta,box);
   }

   template <typename T>
   inline sphere<T> translate(const T& delta, const sphere<T>& sphere)
   {
      return translate(delta,delta,delta,sphere);
   }

   template <typename T>
   inline polygon<T,3> translate(const T& delta, const polygon<T,3>& polygon)
   {
      return translate(delta,delta,delta,polygon);
   }

   template <typename T>
   inline point3d<T> translate(const vector3d<T>& v, const point3d<T>& point)
   {
      return translate(v.x,v.y,v.z,point);
   }

   template <typename T>
   inline line<T,3> translate(const vector3d<T>& v, const line<T,3>& line)
   {
      return translate(v.x,v.y,v.z,line);
   }

   template <typename T>
   inline segment<T,3> translate(const vector3d<T>& v, const segment<T,3>& segment)
   {
      return translate(v.x,v.y,v.z,segment);
   }

   template <typename T>
   inline triangle<T,3> translate(const vector3d<T>& v, const triangle<T,3>& triangle)
   {
      return translate(v.x,v.y,v.z,triangle);
   }

   template <typename T>
   inline quadix<T,3> translate(const vector3d<T>& v, const quadix<T,3>& quadix)
   {
      return translate(v.x,v.y,v.z,quadix);
   }

   template <typename T>
   inline box<T,3> translate(const vector3d<T>& v, const box<T,3>& box)
   {
      return translate(v.x,v.y,v.z,box);
   }

   template <typename T>
   inline sphere<T> translate(const vector3d<T>& v, const sphere<T>& sphere)
   {
      return translate(v.x,v.y,v.z,sphere);
   }

   template <typename T>
   inline polygon<T,3> translate(const vector3d<T>& v, const polygon<T,3>& polygon)
   {
      return translate(v.x,v.y,v.z,polygon);
   }

   template <typename T>
   inline point2d<T> scale(const T& dx, const T& dy, const point2d<T>& point)
   {
      point2d<T> point_;

      point_.x = point.x * dx;
      point_.y = point.y * dy;

      return point_;
   }

   template <typename T>
   inline line<T,2> scale(const T& dx, const T& dy, const line<T,2>& line)
   {
      wykobi::line<T,2> line_;

      for (std::size_t i = 0; i < wykobi::line<T,2>::PointCount; ++i)
      {
         line_[i] = scale(dx,dy,line[i]);
      }

      return line_;
   }

   template <typename T>
   inline segment<T,2> scale(const T& dx, const T& dy, const segment<T,2>& segment)
   {
      wykobi::segment<T,2> segment_;

      for (std::size_t i = 0; i < wykobi::segment<T,2>::PointCount; ++i)
      {
         segment_[i] = scale(dx,dy,segment[i]);
      }

      return segment_;
   }

   template <typename T>
   inline triangle<T,2> scale(const T& dx, const T& dy, const triangle<T,2>& triangle)
   {
      wykobi::triangle<T,2> triangle_;

      for (std::size_t i = 0; i < wykobi::triangle<T,2>::PointCount; ++i)
      {
         triangle_[i] = scale(dx,dy,triangle[i]);
      }

      return triangle_;
   }

   template <typename T>
   inline quadix<T,2> scale(const T& dx, const T& dy, const quadix<T,2>& quadix)
   {
      wykobi::quadix<T,2> quadix_;

      for (std::size_t i = 0; i < wykobi::quadix<T,2>::PointCount; ++i)
      {
         quadix_[i] = scale(dx,dy,quadix[i]);
      }

      return quadix_;
   }

   template <typename T>
   inline rectangle<T> scale(const T& dx, const T& dy, const rectangle<T>& rectangle)
   {
      wykobi::rectangle<T> rectangle_;

      for (std::size_t i = 0; i < wykobi::rectangle<T>::PointCount; ++i)
      {
         rectangle_[i] = scale(dx,dy,rectangle[i]);
      }

      return rectangle_;
   }

   template <typename T>
   inline circle<T> scale(const T& dr, const circle<T>& circle)
   {
      wykobi::circle<T> circle_;

      circle_.x      = circle.x;
      circle_.y      = circle.y;
      circle_.radius = circle.radius * dr;

      return circle_;
   }

   template <typename T>
   inline polygon<T,2> scale(const T& dx, const T& dy, const polygon<T,2>& polygon)
   {
      wykobi::polygon<T,2> polygon_;

      polygon_.reserve(polygon.size());

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         polygon_.push_back(scale(dx,dy,polygon[i]));
      }

      return polygon_;
   }

   template <typename T>
   inline point3d<T> scale(const T& dx, const T& dy, const T& dz, const point3d<T>& point)
   {
      point3d<T> point_;

      point_.x = point.x * dx;
      point_.y = point.y * dy;
      point_.z = point.z * dz;

      return point_;
   }

   template <typename T>
   inline line<T,3> scale(const T& dx, const T& dy, const T& dz, const line<T,3>& line)
   {
      wykobi::line<T,3> line_;

      for (std::size_t i = 0; i < wykobi::line<T,3>::PointCount; ++i)
      {
         line_[i] = scale(dx,dy,dz,line[i]);
      }

      return line_;
   }

   template <typename T>
   inline segment<T,3> scale(const T& dx, const T& dy, const T& dz, const segment<T,3>& segment)
   {
      wykobi::segment<T,3> segment_;

      for (std::size_t i = 0; i < wykobi::segment<T,3>::PointCount; ++i)
      {
         segment_[i] = scale(dx,dy,dz,segment[i]);
      }

      return segment_;
   }

   template <typename T>
   inline triangle<T,3> scale(const T& dx, const T& dy, const T& dz, const triangle<T,3>& triangle)
   {
      wykobi::triangle<T,3> triangle_;

      for (std::size_t i = 0; i < wykobi::triangle<T,3>::PointCount; ++i)
      {
         triangle_[i] = scale(dx,dy,dz,triangle[i]);
      }

      return triangle_;
   }

   template <typename T>
   inline quadix<T,3> scale(const T& dx, const T& dy, const T& dz, const quadix<T,3>& quadix)
   {
      wykobi::quadix<T,3> quadix_;

      for (std::size_t i = 0; i < wykobi::quadix<T,3>::PointCount; ++i)
      {
         quadix_[i] = scale(dx,dy,dz,quadix[i]);
      }

      return quadix_;
   }

   template <typename T>
   inline box<T,3> scale(const T& dx, const T& dy, const T& dz, const box<T,3>& box)
   {
      wykobi::box<T,3> box_;

      for (std::size_t i = 0; i < wykobi::box<T,3>::PointCount; ++i)
      {
         box_[i] = scale(dx,dy,dz,box[i]);
      }

      return box_;
   }

   template <typename T>
   inline sphere<T> scale(const T& dr, const sphere<T>& sphere)
   {
      wykobi::sphere<T> sphere_;
      sphere_.x = sphere.x;
      sphere_.y = sphere.y;
      sphere_.z = sphere.z;
      sphere_.radius = sphere.radius * dr;
      return sphere_;
   }

   template <typename T>
   inline polygon<T,3> scale(const T& dx, const T& dy, const T& dz, const polygon<T,3>& polygon)
   {
      wykobi::polygon<T,3> polygon_;

      polygon_.reserve(polygon.size());

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         polygon_.push_back(scale(dx,dy,dz,polygon[i]));
      }

      return polygon_;
   }

   template <typename T>
   inline rectangle<T> aabb(const segment<T,2>& segment)
   {
      rectangle<T> rectangle_;

      if (segment[0].x < segment[1].x)
      {
         rectangle_[0].x = segment[0].x;
         rectangle_[1].x = segment[1].x;
      }
      else
      {
         rectangle_[0].x = segment[1].x;
         rectangle_[1].x = segment[0].x;
      }

      if (segment[0].y < segment[1].y)
      {
         rectangle_[0].y = segment[0].y;
         rectangle_[1].y = segment[1].y;
      }
      else
      {
         rectangle_[0].y = segment[1].y;
         rectangle_[1].y = segment[0].y;
      }
      return rectangle_;
   }

   template <typename T>
   inline rectangle<T> aabb(const triangle<T,2>& triangle)
   {
      rectangle<T> rectangle_;

      rectangle_[0].x = triangle[0].x;
      rectangle_[0].y = triangle[0].y;
      rectangle_[1].x = triangle[0].x;
      rectangle_[1].y = triangle[0].y;

      for (std::size_t i = 1; i < wykobi::triangle<T,2>::PointCount; ++i)
      {
         if (triangle[i].x < rectangle_[0].x)
            rectangle_[0].x = triangle[i].x;
         else if (triangle[i].x > rectangle_[1].x)
            rectangle_[1].x = triangle[i].x;
         if (triangle[i].y < rectangle_[0].y)
            rectangle_[0].y = triangle[i].y;
         else if (triangle[i].y > rectangle_[1].y)
            rectangle_[1].y = triangle[i].y;
      }

      return rectangle_;
   }

   template <typename T>
   inline rectangle<T> aabb(const rectangle<T>& rectangle)
   {
      wykobi::rectangle<T> rectangle_;

      rectangle_[0].x = min(rectangle[0].x,rectangle[1].x);
      rectangle_[0].y = min(rectangle[0].y,rectangle[1].y);
      rectangle_[1].x = max(rectangle[0].x,rectangle[1].x);
      rectangle_[1].y = max(rectangle[0].y,rectangle[1].y);

      return rectangle_;
   }

   template <typename T>
   inline rectangle<T> aabb(const quadix<T,2>& quadix)
   {
      rectangle<T> rectangle_;

      rectangle_[0].x = quadix[0].x;
      rectangle_[0].y = quadix[0].y;
      rectangle_[1].x = quadix[0].x;
      rectangle_[1].y = quadix[0].y;

      for (std::size_t i = 1; i < wykobi::quadix<T,2>::PointCount; ++i)
      {
         if (quadix[i].x < rectangle_[0].x)
            rectangle_[0].x = quadix[i].x;
         else if (quadix[i].x > rectangle_[1].x)
            rectangle_[1].x = quadix[i].x;

         if (quadix[i].y < rectangle_[0].y)
            rectangle_[0].y = quadix[i].y;
         else if (quadix[i].y > rectangle_[1].y)
            rectangle_[1].y = quadix[i].y;
      }

      return rectangle_;
   }

   template <typename T>
   inline rectangle<T> aabb(const circle<T>& circle)
   {
      return make_rectangle(circle.x - circle.radius,circle.y - circle.radius,
                            circle.x + circle.radius,circle.y + circle.radius);
   }

   template <typename T>
   inline rectangle<T> aabb(const polygon<T,2>& polygon)
   {
      if (polygon.size() < 3) return make_rectangle(T(0.0),T(0.0),T(0.0),T(0.0));

      rectangle<T> rectangle_;

      rectangle_[0].x = polygon[0].x;
      rectangle_[0].y = polygon[0].y;
      rectangle_[1].x = polygon[0].x;
      rectangle_[1].y = polygon[0].y;

      for (std::size_t i = 0; i < polygon.size(); ++i)
      {
         if (polygon[i].x < rectangle_[0].x)
            rectangle_[0].x = polygon[i].x;
         else if (polygon[i].x > rectangle_[1].x)
            rectangle_[1].x = polygon[i].x;

         if (polygon[i].y < rectangle_[0].y)
            rectangle_[0].y = polygon[i].y;
         else if (polygon[i].y > rectangle_[1].y)
            rectangle_[1].y = polygon[i].y;
      }

      return rectangle_;
   }

   template <typename T>
   inline void aabb(const segment<T,2>& segment, T& x1, T& y1, T& x2, T& y2)
   {
      rectangle<T> rectangle_ = aabb(segment);

      x1 = rectangle_[0].x;
      y1 = rectangle_[0].y;
      x2 = rectangle_[1].x;
      y2 = rectangle_[1].y;
   }

   template <typename T>
   inline void aabb(const triangle<T,2>& triangle, T& x1, T& y1, T& x2, T& y2)
   {
      wykobi::rectangle<T> rectangle_ = aabb(triangle);

      x1 = rectangle_[0].x;
      y1 = rectangle_[0].y;
      x2 = rectangle_[1].x;
      y2 = rectangle_[1].y;
   }

   template <typename T>
   inline void aabb(const rectangle<T>& rectangle, T& x1, T& y1, T& x2, T& y2)
   {
      wykobi::rectangle<T> rectangle_ = aabb(rectangle);

      x1 = rectangle_[0].x;
      y1 = rectangle_[0].y;
      x2 = rectangle_[1].x;
      y2 = rectangle_[1].y;
   }

   template <typename T>
   inline void aabb(const quadix<T,2>& quadix, T& x1, T& y1, T& x2, T& y2)
   {
      wykobi::rectangle<T> rectangle_ = aabb(quadix);

      x1 = rectangle_[0].x;
      y1 = rectangle_[0].y;
      x2 = rectangle_[1].x;
      y2 = rectangle_[1].y;
   }

   template <typename T>
   inline void aabb(const circle<T>& circle, T& x1, T& y1, T& x2, T& y2)
   {
      wykobi::rectangle<T> rectangle_ = aabb(circle);

      x1 = rectangle_[0].x;
      y1 = rectangle_[0].y;
      x2 = rectangle_[1].x;
      y2 = rectangle_[1].y;
   }

   template <typename T>
   inline void aabb(const polygon<T,2>& polygon, T& x1, T& y1, T& x2, T& y2)
   {
      rectangle<T> rectangle_ = aabb(polygon);
      x1 = rectangle_[0].x;
      y1 = rectangle_[0].y;
      x2 = rectangle_[1].x;
      y2 = rectangle_[1].y;
   }

   template <typename T>
   inline box<T,3> aabb(const segment<T,3>& segment)
   {
      box<T,3> box_;

      if (segment[0].x < segment[1].x)
      {
         box_[0].x = segment[0].x;
         box_[1].x = segment[1].x;
      }
      else
      {
         box_[0].x = segment[1].x;
         box_[1].x = segment[0].x;
      }

      if (segment[0].y < segment[1].y)
      {
         box_[0].y = segment[0].y;
         box_[1].y = segment[1].y;
      }
      else
      {
         box_[0].y = segment[1].y;
         box_[1].y = segment[0].y;
      }

      if (segment[0].z < segment[1].z)
      {
         box_[0].z = segment[0].z;
         box_[1].z = segment[1].z;
      }
      else
      {
         box_[0].z = segment[1].z;
         box_[1].z = segment[0].z;
      }

      return box_;

   }

   template <typename T>
   inline box<T,3> aabb(const triangle<T,3>& triangle)
   {
      box<T,3> box_;

      box_[0].x = triangle[0].x;
      box_[0].y = triangle[0].y;
      box_[0].z = triangle[0].z;
      box_[1].x = triangle[0].x;
      box_[1].y = triangle[0].y;
      box_[1].z = triangle[0].z;

      for (std::size_t i = 1; i < wykobi::triangle<T,3>::PointCount; ++i)
      {
         if (triangle[i].x < box_[0].x)
            box_[0].x = triangle[i].x;
         else if (triangle[i].x > box_[1].x)
            box_[1].x = triangle[i].x;

         if (triangle[i].y < box_[0].y)
            box_[0].y = triangle[i].y;
         else if (triangle[i].y > box_[1].y)
            box_[1].y = triangle[i].y;

         if (triangle[i].z < box_[0].z)
            box_[0].z = triangle[i].z;
         else if (triangle[i].z > box_[1].z)
            box_[1].z = triangle[i].z;
      }

      return box_;
   }

   template <typename T>
   inline box<T,3> aabb(const box<T,3>& box)
   {
      wykobi::box<T,3> box_;

      box_[0].x = min(box[0].x,box[1].x);
      box_[0].y = min(box[0].y,box[1].y);
      box_[0].z = min(box[0].z,box[1].z);
      box_[1].x = max(box[0].x,box[1].x);
      box_[1].y = max(box[0].y,box[1].y);
      box_[1].z = max(box[0].z,box[1].z);

      return box_;
   }

   template <typename T>
   inline box<T,3> aabb(const quadix<T,3>& quadix)
   {
      box<T,3> box_;

      box_[0].x = quadix[0].x;
      box_[0].y = quadix[0].y;
      box_[0].z = quadix[0].z;
      box_[1].x = quadix[0].x;
      box_[1].y = quadix[0].y;
      box_[1].z = quadix[0].z;

      for (std::size_t i = 1; i < wykobi::quadix<T,3>::PointCount; ++i)
      {
         if (quadix[i].x < box_[0].x)
            box_[0].x = quadix[i].x;
         else if (quadix[i].x > box_[1].x)
            box_[1].x = quadix[i].x;

         if (quadix[i].y < box_[0].y)
            box_[0].y = quadix[i].y;
         else if (quadix[i].y > box_[1].y)
            box_[1].y = quadix[i].y;

         if (quadix[i].z < box_[0].z)
            box_[0].z = quadix[i].z;
         else if (quadix[i].z > box_[1].z)
            box_[1].z = quadix[i].z;
      }

      return box_;
   }

   template <typename T>
   inline box<T,3> aabb(const sphere<T>& sphere)
   {
      return make_box(sphere.x - sphere.radius, sphere.y - sphere.radius, sphere.z - sphere.radius,
                      sphere.x + sphere.radius, sphere.y + sphere.radius, sphere.z + sphere.radius);
   }

   template <typename T>
   inline box<T,3> aabb(const polygon<T,3>& polygon)
   {
      box<T,3> box_;

      box_[0].x = polygon[0].x;
      box_[0].y = polygon[0].y;
      box_[0].z = polygon[0].z;
      box_[1].x = polygon[0].x;
      box_[1].y = polygon[0].y;
      box_[1].z = polygon[0].z;

      for (std::size_t i = 1; i < polygon.size(); ++i)
      {
         if (polygon[i].x < box_[0].x)
            box_[0].x = polygon[i].x;
         else if (polygon[i].x > box_[1].x)
            box_[1].x = polygon[i].x;

         if (polygon[i].y < box_[0].y)
            box_[0].y = polygon[i].y;
         else if (polygon[i].y > box_[1].y)
            box_[1].y = polygon[i].y;

         if (polygon[i].z < box_[0].z)
            box_[0].z = polygon[i].z;
         else if (polygon[i].z > box_[1].z)
            box_[1].z = polygon[i].z;
      }

      return box_;
   }

   template <typename T>
   inline void aabb(const segment<T,3>& segment, T& x1, T& y1, T& z1, T& x2, T& y2, T& z2)
   {
      box<T,3> box_ = aabb(segment);
      x1 = box_[0].x;
      y1 = box_[0].y;
      z1 = box_[0].z;
      x2 = box_[1].x;
      y2 = box_[1].y;
      z2 = box_[1].z;
   }

   template <typename T>
   inline void aabb(const triangle<T,3>& triangle, T& x1, T& y1, T& z1, T& x2, T& y2, T& z2)
   {
      box<T,3> box_ = aabb(triangle);
      x1 = box_[0].x;
      y1 = box_[0].y;
      z1 = box_[0].z;
      x2 = box_[1].x;
      y2 = box_[1].y;
      z2 = box_[1].z;
   }

   template <typename T>
   inline void aabb(const box<T,3>& box, T& x1, T& y1, T& z1, T& x2, T& y2, T& z2)
   {
      wykobi::box<T,3> box_ = aabb(box);
      x1 = box_[0].x;
      y1 = box_[0].y;
      z1 = box_[0].z;
      x2 = box_[1].x;
      y2 = box_[1].y;
      z2 = box_[1].z;
   }

   template <typename T>
   inline void aabb(const quadix<T,3>& quadix, T& x1, T& y1, T& z1, T& x2, T& y2, T& z2)
   {
      box<T,3> box_ = aabb(quadix);
      x1 = box_[0].x;
      y1 = box_[0].y;
      z1 = box_[0].z;
      x2 = box_[1].x;
      y2 = box_[1].y;
      z2 = box_[1].z;
   }

   template <typename T>
   inline void aabb(const sphere<T>& sphere, T& x1, T& y1, T& z1, T& x2, T& y2, T& z2)
   {
      box<T,3> box_ = aabb(sphere);
      x1 = box_[0].x;
      y1 = box_[0].y;
      z1 = box_[0].z;
      x2 = box_[1].x;
      y2 = box_[1].y;
      z2 = box_[1].z;
   }

   template <typename T>
   inline void aabb(const polygon<T,3>& polygon, T& x1, T& y1, T& z1, T& x2, T& y2, T& z2)
   {
      box<T,3> box_ = aabb(polygon);
      x1 = box_[0].x;
      y1 = box_[0].y;
      z1 = box_[0].z;
      x2 = box_[1].x;
      y2 = box_[1].y;
      z2 = box_[1].z;
   }

   template <typename T>
   inline rectangle<T> update_rectangle(const rectangle<T>& rectangle, point2d<T>& point)
   {
      if (!point_in_rectangle(point,rectangle))
      {
         return make_rectangle
                (
                  min(rectangle[0].x, rectangle[1].x, point.x),
                  min(rectangle[0].y, rectangle[1].y, point.y),
                  max(rectangle[0].x, rectangle[1].x, point.x),
                  max(rectangle[0].y, rectangle[1].y, point.y)
                );
      }
      else
         return rectangle;
   }

   template <typename T>
   inline box<T,3> update_box(const box<T,3>& box, point3d<T>& point)
   {
      if (!point_in_box(point,box))
      {
         return make_box
                (
                  min(box[0].x, box[1].x, point.x),
                  min(box[0].y, box[1].y, point.y),
                  min(box[0].z, box[1].z, point.z),
                  max(box[0].x, box[1].x, point.x),
                  max(box[0].y, box[1].y, point.y),
                  max(box[0].z, box[1].z, point.z)
                );
      }
      else
         return box;
   }

   template <typename T>
   inline circle<T> update_circle(const circle<T>& circle, point2d<T>& point)
   {
      vector2d<T> d = point - make_point(circle.x,circle.y);

      const T dist2 = dot_product(d,d);

      if (dist2 > sqr(circle.radius))
      {
         const T dist       = sqrt(dist2);
         const T new_radius = (circle.radius + dist) * T(0.5);
         const T ratio      = (new_radius - circle.radius) / dist;

         return make_circle
                (
                  circle.x + (d.x * ratio),
                  circle.y + (d.y * ratio),
                  new_radius
                );
      }
      else
         return circle;
   }

   template <typename T>
   inline sphere<T> update_sphere(const sphere<T>& sphere, point3d<T>& point)
   {
      vector3d<T> d = point - make_point(sphere.x,sphere.y,sphere.z);

      const T dist2 = dot_product(d,d);

      if (dist2 > sqr(sphere.radius))
      {
         const T dist       = sqrt(dist2);
         const T new_radius = (sphere.radius + dist) * T(0.5);
         const T ratio      = (new_radius - sphere.radius) / dist;

         return make_sphere
                (
                  sphere.x + (d.x * ratio),
                  sphere.y + (d.y * ratio),
                  sphere.z + (d.z * ratio),
                  new_radius
                );
      }
      else
         return sphere;
   }

   template <typename T>
   inline point2d<T> generate_point_on_segment(const segment<T,2>& segment, const T& t)
   {
      if ((t < T(0.0)) || (t > T(1.0)))
      {
         return degenerate_point2d<T>();
      }

      return make_point
             (
               (T(1.0) - t) * segment[0].x + t * segment[1].x,
               (T(1.0) - t) * segment[0].y + t * segment[1].y
             );
   }

   template <typename T>
   inline point3d<T> generate_point_on_segment(const segment<T,3>& segment, const T& t)
   {
      if ((t < T(0.0)) || (t > T(1.0)))
      {
         return degenerate_point3d<T>();
      }

      return make_point
             (
               (T(1.0) - t) * segment[0].x + t * segment[1].x,
               (T(1.0) - t) * segment[0].y + t * segment[1].y,
               (T(1.0) - t) * segment[0].z + t * segment[1].z
             );
   }

   template <typename T>
   inline point2d<T> generate_point_on_ray(const ray<T,2>& ray, const T& t)
   {
      if (t < T(0.0))
      {
         return degenerate_point2d<T>();
      }

      return make_point
             (
               ray.origin.x + t * ray.direction.x,
               ray.origin.y + t * ray.direction.y
             );
   }

   template <typename T>
   inline point3d<T> generate_point_on_ray(const ray<T,3>& ray, const T& t)
   {
      if (t < T(0.0))
      {
         return degenerate_point3d<T>();
      }

      return make_point
             (
               ray.origin.x + t * ray.direction.x,
               ray.origin.y + t * ray.direction.y,
               ray.origin.z + t * ray.direction.z
             );

   }

   template <typename T>
   inline T generate_random_value(const T& range)
   {
      return T((1.0 * range * rand()) / RAND_MAX);
   }

   template <typename T>
   inline point2d<T> generate_random_point(const T& dx, const T& dy)
   {
      return make_point(generate_random_value(dx),generate_random_value(dy));
   }

   template <typename T>
   inline point3d<T> generate_random_point(const T& dx, const T& dy, const T& dz)
   {
      return make_point(generate_random_value(dx),generate_random_value(dy),generate_random_value(dz));
   }

   template <typename T>
   inline point2d<T> generate_random_point(const segment<T,2>& segment)
   {
      const T t = generate_random_value(T(1.0));

      return make_point(((1 - t) * segment[0].x) + (t * segment[1].x),
                        ((1 - t) * segment[0].y) + (t * segment[1].y));
   }

   template <typename T>
   inline point3d<T> generate_random_point(const segment<T,3>& segment)
   {
      const T t = generate_random_value(T(1.0));

      return make_point(((1 - t) * segment[0].x) + (t * segment[1].x),
                        ((1 - t) * segment[0].y) + (t * segment[1].y),
                        ((1 - t) * segment[0].z) + (t * segment[1].z));
   }

   template <typename T>
   inline point2d<T> generate_random_point(const triangle<T,2>& triangle)
   {
      T a = generate_random_value(T(1.0));
      T b = generate_random_value(T(1.0));

      if ((a + b) > T(1.0))
      {
         a = 1 - a;
         b = 1 - b;
      }

      const T c = (1 - a - b);

      return make_point((triangle[0].x * a) + (triangle[1].x * b) + (triangle[2].x * c),
                        (triangle[0].y * a) + (triangle[1].y * b) + (triangle[2].y * c));
   }

   template <typename T>
   inline point3d<T> generate_random_point(const triangle<T,3>& triangle)
   {
      T a = generate_random_value(T(1.0));
      T b = generate_random_value(T(1.0));

      if ((a + b) > T(1.0))
      {
         a = 1 - a;
         b = 1 - b;
      }

      const T c = (1 - a - b);

      return make_point((triangle[0].x * a) + (triangle[1].x * b) + (triangle[2].x * c),
                        (triangle[0].y * a) + (triangle[1].y * b) + (triangle[2].y * c),
                        (triangle[0].z * a) + (triangle[1].z * b) + (triangle[2].z * c));
   }

   template <typename T>
   inline point2d<T> generate_random_point(const quadix<T,2>& quadix)
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

      return make_point(((r1 * quadix[0].x) + (r2 * quadix[1].x) + (r3 * quadix[2].x) + (r4 * quadix[3].x)) * T(0.25),
                        ((r1 * quadix[0].y) + (r2 * quadix[1].y) + (r3 * quadix[2].y) + (r4 * quadix[3].y)) * T(0.25));
   }

   template <typename T>
   inline point3d<T> generate_random_point(const quadix<T,3>& quadix)
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

      return make_point(((r1 * quadix[0].x) + (r2 * quadix[1].x) + (r3 * quadix[2].x) + (r4 * quadix[3].x)) * T(0.25),
                        ((r1 * quadix[0].y) + (r2 * quadix[1].y) + (r3 * quadix[2].y) + (r4 * quadix[3].y)) * T(0.25),
                        ((r1 * quadix[0].z) + (r2 * quadix[1].z) + (r3 * quadix[2].z) + (r4 * quadix[3].z)) * T(0.25));
   }

   template <typename T>
   inline point2d<T> generate_random_point(const rectangle<T>& rectangle)
   {
      return translate(min(rectangle[0].x,rectangle[1].x),min(rectangle[0].y,rectangle[1].y),
                       generate_random_point(abs(rectangle[1].x - rectangle[0].x),
                                             abs(rectangle[1].y - rectangle[0].y)));
   }

   template <typename T>
   inline point3d<T> generate_random_point(const box<T,3>& box)
   {
      return translate(min(box[0].x,box[1].x),min(box[0].y,box[1].y),min(box[0].z,box[1].z),
                       generate_random_point(abs(box[1].x - box[0].x),
                                             abs(box[1].y - box[0].y),
                                             abs(box[1].z - box[0].z)));
   }

   template <typename T, typename OutputIterator>
   inline void generate_random_points(const T& x1, const T& y1,
                                      const T& x2, const T& y2,
                                      const std::size_t& point_count,
                                      OutputIterator out)
   {
      const T dx = abs(x2 - x1);
      const T dy = abs(y2 - y1);

      for (std::size_t i = 0; i < point_count; ++i)
      {
         (*out++) = translate(x1,y1,generate_random_point(dx,dy));
      }
   }

   template <typename T, typename OutputIterator>
   inline void generate_random_points(const T& x1, const T& y1, const T& z1,
                                      const T& x2, const T& y2, const T& z2,
                                      const std::size_t& point_count,
                                      OutputIterator out)
   {
      T dx = abs(x2 - x1);
      T dy = abs(y2 - y1);
      T dz = abs(z2 - z1);

      for (std::size_t i = 0; i < point_count; ++i)
      {
         (*out++) = translate(x1,y1,z1,generate_random_point(dx,dy,dz));
      }
   }

   template <typename T, typename OutputIterator>
   inline void generate_random_points(const rectangle<T>& rectangle, const std::size_t& point_count, OutputIterator out)
   {
      generate_random_points(rectangle[0].x, rectangle[0].y,
                             rectangle[1].x, rectangle[1].y,
                             point_count,
                             out);
   }

   template <typename T, typename OutputIterator>
   inline void generate_random_points(const box<T,3>& box, const std::size_t& point_count, OutputIterator out)
   {
      generate_random_points(box[0].x, box[0].y, box[0].z,
                             box[1].x, box[1].y, box[1].z,
                             point_count,
                             out);
   }

   template <typename T, typename OutputIterator>
   inline void generate_random_points(const segment<T,2>& segment, const std::size_t& point_count, OutputIterator out)
   {
      for (std::size_t i = 0; i < point_count; ++i)
      {
         (*out++) = generate_random_point(segment);
      }
   }

   template <typename T, typename OutputIterator>
   inline void generate_random_points(const segment<T,3>& segment, const std::size_t& point_count, OutputIterator out)
   {
      for (std::size_t i = 0; i < point_count; ++i)
      {
         (*out++) = generate_random_point(segment);
      }
   }

   template <typename T, typename OutputIterator>
   inline void generate_random_points(const triangle<T,2>& triangle, const std::size_t& point_count, OutputIterator out)
   {
      for (std::size_t i = 0; i < point_count; ++i)
      {
         (*out++) = generate_random_point(triangle);
      }
   }

   template <typename T, typename OutputIterator>
   inline void generate_random_points(const triangle<T,3>& triangle, const std::size_t& point_count, OutputIterator out)
   {
      for (std::size_t i = 0; i < point_count; ++i)
      {
         (*out++) = generate_random_point(triangle);
      }
   }

   template <typename T, typename OutputIterator>
   inline void generate_random_points(const quadix<T,2>& quadix, const std::size_t& point_count, OutputIterator out)
   {
      for (std::size_t i = 0; i < point_count; ++i)
      {
         (*out++) = generate_random_point(quadix);
      }
   }

   template <typename T, typename OutputIterator>
   inline void generate_random_points(const quadix<T,3>& quadix, const std::size_t& point_count, OutputIterator out)
   {
      for (std::size_t i = 0; i < point_count; ++i)
      {
         (*out++) = generate_random_point(quadix);
      }
   }

   template <typename T, typename OutputIterator>
   inline void generate_random_points(const circle<T>& circle, const std::size_t& point_count, OutputIterator out)
   {
      point2d<T> cpoint = make_point(circle.x, circle.y);

      wykobi::point2d<T> point_;

      for (std::size_t i = 0; i < point_count; ++i)
      {
         T random_angle  = generate_random_value(T(360.0));

         point_.x = circle.x + circle.radius * sqrt(generate_random_value(T(1.0)));
         point_.y = circle.y;

         (*out++) = rotate(random_angle, point_, cpoint);
      }
   }

   template <typename T>
   inline void generate_random_object(const T& x1, const T& y1, const T& x2, const T& y2, segment<T,2>& segment)
   {
      const T dx = abs(x2 - x1);
      const T dy = abs(y2 - y1);

      do
      {
         for (std::size_t i = 0; i < wykobi::segment<T,2>::PointCount; ++i)
         {
            segment[i].x = x1 + generate_random_value(dx);
            segment[i].y = y1 + generate_random_value(dy);
         }
      }
      while (is_degenerate(segment));
   }

   template <typename T>
   inline void generate_random_object(const T& x1, const T& y1, const T& x2, const T& y2, rectangle<T>& rectangle)
   {
      const T dx = abs(x2 - x1);
      const T dy = abs(y2 - y1);

      do
      {
         for (std::size_t i = 0; i < wykobi::rectangle<T>::PointCount; ++i)
         {
            rectangle[i].x = x1 + generate_random_value(dx);
            rectangle[i].y = y1 + generate_random_value(dy);
         }
      }
      while (is_degenerate(rectangle));

      if (rectangle[1].x < rectangle[0].x)
      {
         T t = rectangle[1].x;
         rectangle[1].x = rectangle[0].x;
         rectangle[0].x = t;
      }

      if (rectangle[1].y < rectangle[0].y)
      {
         T t = rectangle[1].y;
         rectangle[1].y = rectangle[0].y;
         rectangle[0].y = t;
      }
   }

   template <typename T>
   inline void generate_random_object(const T& x1, const T& y1, const T& x2, const T& y2, triangle<T,2>& triangle)
   {
      const T dx = abs(x2 - x1);
      const T dy = abs(y2 - y1);

      do
      {
         for (std::size_t i = 0; i < wykobi::triangle<T,2>::PointCount; ++i)
         {
            triangle[i].x = x1 + generate_random_value(dx);
            triangle[i].y = y1 + generate_random_value(dy);
         }
      }
      while (is_degenerate(triangle));
   }

   template <typename T>
   inline void generate_random_object(const T& x1, const T& y1, const T& x2, const T& y2, quadix<T,2>& quadix)
   {
      const T dx = abs(x2 - x1);
      const T dy = abs(y2 - y1);

      do
      {
         for (std::size_t i = 0; i < wykobi::quadix<T,2>::PointCount; ++i)
         {
            quadix[i].x = x1 + generate_random_value(dx);
            quadix[i].y = y1 + generate_random_value(dy);
         }
      }
      while (is_degenerate(quadix));
   }

   template <typename T>
   inline void generate_random_object(const T& x1, const T& y1, const T& x2, const T& y2, circle<T>& circle)
   {
      const T dx = abs(x2 - x1);
      const T dy = abs(y2 - y1);

      circle.radius = generate_random_value(min(dx,dy) * T(0.5));
      circle.x      = x1 + circle.radius + generate_random_value(dx - (T(2.0) * circle.radius));
      circle.y      = y1 + circle.radius + generate_random_value(dy - (T(2.0) * circle.radius));
   }

   template <typename T>
   inline void generate_random_object(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, box<T,3>& box)
   {
      wykobi::box<T,3> region = make_box(x1,y1,z1,x2,y2,z2);
      box[0] = generate_random_point(region);
      box[1] = generate_random_point(region);
   }

   template <typename T>
   inline triangle<T,2> right_shift(const triangle<T,2>& triangle, const std::size_t& shift)
   {
      switch(shift % 3)
      {
         case 0  : return triangle;
         case 1  : return make_triangle(triangle[2],triangle[0],triangle[1]);
         case 2  : return make_triangle(triangle[1],triangle[2],triangle[0]);
         default : return triangle;
      }
   }

   template <typename T>
   inline triangle<T,3> right_shift(const triangle<T,3>& triangle, const std::size_t& shift)
   {
      switch(shift % 3)
      {
         case 0  : return triangle;
         case 1  : return make_triangle(triangle[2],triangle[0],triangle[1]);
         case 2  : return make_triangle(triangle[1],triangle[2],triangle[0]);
         default : return triangle;
      }
   }

   template <typename T>
   inline quadix<T,2> right_shift(const quadix<T,2>& quadix, const std::size_t& shift)
   {
      switch(shift % 4)
      {
         case 0  : return quadix;
         case 1  : return make_quadix(quadix[3],quadix[0],quadix[1],quadix[2]);
         case 2  : return make_quadix(quadix[2],quadix[3],quadix[0],quadix[1]);
         case 3  : return make_quadix(quadix[1],quadix[2],quadix[3],quadix[0]);
         default : return quadix;
      }
   }

   template <typename T>
   inline quadix<T,3> right_shift(const quadix<T,3>& quadix, const std::size_t& shift)
   {
      switch(shift % 4)
      {
         case 0  : return quadix;
         case 1  : return make_quadix(quadix[3],quadix[0],quadix[1],quadix[2]);
         case 2  : return make_quadix(quadix[2],quadix[3],quadix[0],quadix[1]);
         case 3  : return make_quadix(quadix[1],quadix[2],quadix[3],quadix[0]);
         default : return quadix;
      }
   }

   template <typename T>
   inline T vector_norm(const vector2d<T>& v)
   {
      return sqrt((v.x * v.x) + (v.y * v.y));
   }

   template <typename T>
   inline T vector_norm(const vector3d<T>& v)
   {
      return sqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
   }

   template <typename T>
   inline vector2d<T> normalize(const vector2d<T>& v)
   {
      return v * (T(1.0) / vector_norm(v));
   }

   template <typename T>
   inline vector3d<T> normalize(const vector3d<T>& v)
   {
      return v * (T(1.0) / vector_norm(v));
   }

   template <typename T>
   inline vector2d<T> perpendicular(const vector2d<T>& v)
   {
      return make_vector(v.y,-v.x);
   }

   template <typename T>
   inline vector3d<T> perpendicular(const vector3d<T>& v)
   {
      return make_vector(v.y,-v.x,v.z);
   }

   template <typename T>
   inline vector2d<T> operator+(const vector2d<T>& v1, const vector2d<T>& v2)
   {
      vector2d<T> v3;
      v3.x = v1.x + v2.x;
      v3.y = v1.y + v2.y;
      return v3;
   }

   template <typename T>
   inline vector3d<T> operator+(const vector3d<T>& v1, const vector3d<T>& v2)
   {
      vector3d<T> v3;
      v3.x = v1.x + v2.x;
      v3.y = v1.y + v2.y;
      v3.z = v1.z + v2.z;
      return v3;
   }

   template <typename T>
   inline vector2d<T> operator-(const vector2d<T>& v1, const vector2d<T>& v2)
   {
      vector2d<T> v3;
      v3.x = v1.x - v2.x;
      v3.y = v1.y - v2.y;
      return v3;
   }

   template <typename T>
   inline vector3d<T> operator-(const vector3d<T>& v1, const vector3d<T>& v2)
   {
      vector3d<T> v3;
      v3.x = v1.x - v2.x;
      v3.y = v1.y - v2.y;
      v3.z = v1.z - v2.z;
      return v3;
   }

   template <typename T>
   inline T operator*(const vector2d<T>& v1, const vector2d<T>& v2)
   {
      return (v1.x * v2.y) - (v1.y - v2.x);
   }

   template <typename T>
   inline vector3d<T> operator*(const vector3d<T>& v1, const vector3d<T>& v2)
   {
      vector3d<T> v3;

      v3.x = v1.y * v2.z - v1.z * v2.y;
      v3.y = v1.z * v2.x - v1.x * v2.z;
      v3.z = v1.x * v2.y - v1.y * v2.x;

      return v3;
   }

   template <typename T>
   inline T dot_product(const vector2d<T>& v1, const vector2d<T>& v2)
   {
      return (v1.x * v2.x) + (v1.y * v2.y);
   }

   template <typename T>
   inline T dot_product(const vector3d<T>& v1, const vector3d<T>& v2)
   {
      return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
   }

   template <typename T>
   inline T perpendicular_product(const vector2d<T>& v1, const vector2d<T>& v2)
   {
      return (v1.x * v2.y) - (v1.y * v2.x);
   }

   template <typename T>
   inline T triple_product(const vector3d<T>& v1, const vector3d<T>& v2, const vector3d<T>& v3)
   {
      return dot_product(v1, v2 * v3);
   }

   template <typename T>
   inline vector2d<T> operator*(const vector2d<T>& v, const T& scale)
   {
      vector2d<T> v_;

      v_.x = v.x * scale;
      v_.y = v.y * scale;

      return v_;
   }

   template <typename T>
   inline vector3d<T> operator*(const vector3d<T>& v, const T& scale)
   {
      vector3d<T> v_;

      v_.x = v.x * scale;
      v_.y = v.y * scale;
      v_.z = v.z * scale;

      return v_;
   }

   template <typename T>
   inline vector2d<T> operator/(const vector2d<T>& v, const T& scale)
   {
      vector2d<T> v_;

      v_.x = v.x / scale;
      v_.y = v.y / scale;

      return v_;
   }

   template <typename T>
   inline vector3d<T> operator/(const vector3d<T>& v, const T& scale)
   {
      vector3d<T> v_;

      v_.x = v.x / scale;
      v_.y = v.y / scale;
      v_.z = v.z / scale;

      return v_;
   }

   template <typename T>
   inline vector2d<T> operator*(const T& scale, const vector2d<T>& v)
   {
      vector2d<T> v_;

      v_.x = v.x * scale;
      v_.y = v.y * scale;

      return v_;
   }

   template <typename T>
   inline vector3d<T> operator*(const T& scale, const vector3d<T>& v)
   {
      vector3d<T> v_;

      v_.x = v.x * scale;
      v_.y = v.y * scale;
      v_.z = v.z * scale;

      return v_;
   }

   template <typename T>
   inline point2d<T> operator*(const point2d<T>& point, const T& scale)
   {
      return wykobi::scale(scale,scale,point);
   }

   template <typename T>
   inline point3d<T> operator*(const point3d<T>& point, const T& scale)
   {
      return wykobi::scale(scale,scale,scale,point);
   }

   template <typename T>
   inline point2d<T> operator*(const T& scale, const point2d<T>& point)
   {
      return wykobi::scale(scale,scale,point);
   }

   template <typename T>
   inline point3d<T> operator*(const T& scale, const point3d<T>& point)
   {
      return wykobi::scale(scale,scale,scale,point);
   }

   template <typename T>
   inline point2d<T> operator+(const point2d<T>& point, const vector2d<T>& v)
   {
      point2d<T> point_;

      point_.x = point.x + v.x;
      point_.y = point.y + v.y;

      return point_;
   }

   template <typename T>
   inline point2d<T> operator+(const vector2d<T>& v, const point2d<T>& point)
   {
      point2d<T> point_;

      point_.x = point.x + v.x;
      point_.y = point.y + v.y;

      return point_;
   }

   template <typename T>
   inline point3d<T> operator+(const point3d<T>& point, const vector3d<T>& v)
   {
      point3d<T> point_;

      point_.x = point.x + v.x;
      point_.y = point.y + v.y;
      point_.z = point.z + v.z;

      return point_;
   }

   template <typename T>
   inline point3d<T> operator+(const vector3d<T>& v, const point3d<T>& point)
   {
      point3d<T> point_;

      point_.x = point.x + v.x;
      point_.y = point.y + v.y;
      point_.z = point.z + v.z;

      return point_;
   }

   template <typename T>
   inline vector2d<T> operator-(const point2d<T>& p1, const point2d<T>& p2)
   {
      vector2d<T> v_;

      v_.x = p1.x - p2.x;
      v_.y = p1.y - p2.y;

      return v_;
   }

   template <typename T>
   inline vector3d<T> operator-(const point3d<T>& p1, const point3d<T>& p2)
   {
      vector3d<T> v_;
      v_.x = p1.x - p2.x;
      v_.y = p1.y - p2.y;
      v_.z = p1.z - p2.z;
      return v_;
   }

   template <typename T>
   inline point2d<T> operator+(const point2d<T>& p1, const point2d<T>& p2)
   {
      point2d<T> p_;

      p_.x = p1.x + p2.x;
      p_.y = p1.y + p2.y;

      return p_;
   }

   template <typename T>
   inline point3d<T> operator+(const point3d<T>& p1, const point3d<T>& p2)
   {
      point3d<T> p_;

      p_.x = p1.x + p2.x;
      p_.y = p1.y + p2.y;
      p_.z = p1.z + p2.z;

      return p_;
   }

   template <typename T>
   inline bool is_equal(const T& val1, const T& val2, const T& epsilon)
   {
      T diff = val1 - val2;

      assert(((-epsilon <= diff) && (diff <= epsilon)) == (abs(diff) <= epsilon));

      return ((-epsilon <= diff) && (diff <= epsilon));
   }

   template <typename T>
   inline bool is_equal(const point2d<T>& point1, const point2d<T>& point2, const T& epsilon)
   {
      return (is_equal(point1.x,point2.x,epsilon) && is_equal(point1.y,point2.y,epsilon));
   }

   template <typename T>
   inline bool is_equal(const point3d<T>& point1, const point3d<T>& point2, const T& epsilon)
   {
      return is_equal(point1.x,point2.x,epsilon) &&
             is_equal(point1.y,point2.y,epsilon) &&
             is_equal(point1.z,point2.z,epsilon) ;
   }

   template <typename T>
   inline bool is_equal(const T& val1, const T& val2)
   {
      return is_equal(val1,val2,T(Epsilon));
   }

   template <typename T>
   inline bool is_equal(const point2d<T>& point1, const point2d<T>& point2)
   {
      return is_equal(point1,point2,T(Epsilon));
   }

   template <typename T>
   inline bool is_equal(const point3d<T>& point1, const point3d<T>& point2)
   {
      return is_equal(point1,point2,T(Epsilon));
   }

   template <typename T>
   inline bool is_equal(const rectangle<T>& rectangle1, const rectangle<T>& rectangle2)
   {
      return (is_equal(rectangle1[0],rectangle2[0]) && is_equal(rectangle1[1],rectangle2[1])) ||
             (is_equal(rectangle1[0],rectangle2[1]) && is_equal(rectangle1[1],rectangle2[0])) ;
   }

   template <typename T>
   inline bool is_equal(const circle<T>& circle1, const circle<T>& circle2)
   {
      return is_equal(circle1.x,circle2.x) &&
             is_equal(circle1.y,circle2.y) &&
             is_equal(circle1.radius,circle2.radius);
   }

   template <typename T>
   inline bool is_equal(const box<T,3>& box1, const box<T,3>& box2)
   {
      return (is_equal(box1[0],box2[0]) && is_equal(box1[1],box2[1])) ||
             (is_equal(box1[0],box2[1]) && is_equal(box1[1],box2[0])) ;
   }

   template <typename T>
   inline bool is_equal(const sphere<T>& sphere1, const sphere<T>& sphere2)
   {
      return is_equal(sphere1.x,sphere2.x) &&
             is_equal(sphere1.y,sphere2.y) &&
             is_equal(sphere1.z,sphere2.z) &&
             is_equal(sphere1.radius,sphere2.radius);
   }

   template <typename T>
   inline bool not_equal(const T& val1, const T& val2, const T& epsilon)
   {
      T diff = val1 - val2;

      assert(((-epsilon > diff) || (diff > epsilon)) == (abs(val1 - val2) > epsilon));

      return ((-epsilon > diff) || (diff > epsilon));
   }

   template <typename T>
   inline bool not_equal(const point2d<T>& point1, const point2d<T>& point2, const T& epsilon)
   {
      return (not_equal(point1.x,point2.x,epsilon) || not_equal(point1.y,point2.y,epsilon));
   }

   template <typename T>
   inline bool not_equal(const point3d<T>& point1, const point3d<T>& point2, const T& epsilon)
   {
      return not_equal(point1.x, point2.x, epsilon) ||
             not_equal(point1.y, point2.y, epsilon) ||
             not_equal(point1.z, point2.z, epsilon) ;
   }

   template <typename T>
   inline bool not_equal(const T& val1, const T& val2)
   {
      return not_equal(val1,val2,T(Epsilon));
   }

   template <typename T>
   inline bool not_equal(const point2d<T>& point1, const point2d<T>& point2)
   {
      return not_equal(point1,point2,T(Epsilon));
   }

   template <typename T>
   inline bool not_equal(const point3d<T>& point1, const point3d<T>& point2)
   {
      return not_equal(point1,point2,T(Epsilon));
   }

   template <typename T>
   inline bool not_equal(const rectangle<T>& rectangle1, const rectangle<T>& rectangle2)
   {
      return (!is_equal(rectangle1,rectangle2));
   }

   template <typename T>
   inline bool not_equal(const circle<T>& circle1, const circle<T>& circle2)
   {
      return (!is_equal(circle1,circle2));
   }

   template <typename T>
   inline bool not_equal(const box<T,3>& box1, const box<T,3>& box2)
   {
      return (!is_equal(box1,box2));
   }

   template <typename T>
   inline bool not_equal(const sphere<T>& sphere1, const sphere<T>& sphere2)
   {
      return (!is_equal(sphere1,sphere2));
   }

   template <typename T>
   inline bool less_than_or_equal(const T& val1, const T& val2, const T& epsilon)
   {
      return (val1 < val2) || is_equal(val1,val2,epsilon);
   }

   template <typename T>
   inline bool less_than_or_equal(const T& val1, const T& val2)
   {
      return (val1 < val2) || is_equal(val1,val2,T(Epsilon));
   }

   template <typename T>
   inline bool greater_than_or_equal(const T& val1, const T& val2, const T& epsilon)
   {
      return (val1 > val2) || is_equal(val1,val2,epsilon);
   }

   template <typename T>
   inline bool greater_than_or_equal(const T& val1, const T& val2)
   {
      return (val1 > val2) || is_equal(val1,val2,T(Epsilon));
   }

   template <typename T>
   inline bool operator < (const point2d<T>& point1, const point2d<T>& point2)
   {
      if (point1.x < point2.x)
         return true;
      else if (point1.x > point2.x)
         return false;
      else if (point1.y < point2.y)
         return true;
      else
         return false;
   }

   template <typename T>
   inline bool operator < (const point3d<T>& point1, const point3d<T>& point2)
   {
      if (point1.x < point2.x)
         return true;
      else if (point1.x > point2.x)
         return false;
      else if (point1.y < point2.y)
         return true;
      else if (point1.y > point2.y)
         return false;
      else if (point1.z < point2.z)
         return true;
      else
         return false;
   }

   template <typename T>
   inline bool operator > (const point2d<T>& point1, const point2d<T>& point2)
   {
      if (point1.x > point2.x)
         return true;
      else if (point1.x < point2.x)
         return false;
      else if (point1.y > point2.y)
         return true;
      else
         return false;
   }

   template <typename T>
   inline bool operator > (const point3d<T>& point1, const point3d<T>& point2)
   {
      if (point1.x > point2.x)
         return true;
      else if (point1.x < point2.x)
         return false;
      else if (point1.y > point2.y)
         return true;
      else if (point1.y < point2.y)
         return false;
      else if (point1.z > point2.z)
         return true;
      else
         return false;
   }

   template <typename T>
   inline bool operator == (const point2d<T>& point1, const point2d<T>& point2)
   {
      return is_equal(point1,point2);
   }

   template <typename T>
   inline bool operator == (const point3d<T>& point1, const point3d<T>& point2)
   {
      return is_equal(point1,point2);
   }

   template <typename T>
   inline bool is_degenerate(const T& x1, const T& y1, const T& x2, const T& y2)
   {
      return is_equal(x1,x2) && is_equal(y1,y2);
   }

   template <typename T>
   inline bool is_degenerate(const segment<T,2>& segment)
   {
      return is_degenerate
             (
               segment[0].x, segment[0].y,
               segment[1].x, segment[1].y
             );
   }

   template <typename T>
   inline bool is_degenerate(const line<T,2>& line)
   {
      return is_degenerate(line[0].x,line[0].y,line[1].x,line[1].y);
   }

   template <typename T>
   inline bool is_degenerate(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2)
   {
      return is_equal(x1,x2) &&
             is_equal(y1,y2) &&
             is_equal(z1,z2) ;
   }

   template <typename T>
   inline bool is_degenerate(const segment<T,3>& segment)
   {
      return is_degenerate
             (
               segment[0].x, segment[0].y, segment[0].z,
               segment[1].x, segment[1].y, segment[1].z
             );
   }

   template <typename T>
   inline bool is_degenerate(const line<T,3>& line)
   {
      return is_degenerate
             (
               line[0].x, line[0].y, line[0].z,
               line[1].x, line[1].y, line[1].z
             );
   }

   template <typename T>
   inline bool is_degenerate(const triangle<T,2>& triangle)
   {
      return is_equal(triangle[0],triangle[1])                    ||
             is_equal(triangle[0],triangle[2])                    ||
             is_equal(triangle[1],triangle[2])                    ||
             robust_collinear(triangle[0],triangle[1],triangle[2])||
             is_equal(distance(edge(triangle,0)),(distance(edge(triangle,1)) + distance(edge(triangle,2)))) ||
             is_equal(distance(edge(triangle,1)),(distance(edge(triangle,2)) + distance(edge(triangle,0)))) ||
             is_equal(distance(edge(triangle,2)),(distance(edge(triangle,0)) + distance(edge(triangle,1))));
   }

   template <typename T>
   inline bool is_degenerate(const triangle<T,3>& triangle)
   {
      return (is_equal(triangle[0],triangle[1]))                   ||
             (is_equal(triangle[0],triangle[2]))                   ||
             (is_equal(triangle[1],triangle[2]))                   ||
             robust_collinear(triangle[0],triangle[1],triangle[2]) ||
             is_equal(distance(edge(triangle,0)),(distance(edge(triangle,1)) + distance(edge(triangle,2)))) ||
             is_equal(distance(edge(triangle,1)),(distance(edge(triangle,2)) + distance(edge(triangle,0)))) ||
             is_equal(distance(edge(triangle,2)),(distance(edge(triangle,0)) + distance(edge(triangle,1))));
   }

   template <typename T>
   inline bool is_degenerate(const quadix<T,2>& quadix)
   {
      return /* stage 1 unique points check */
             is_degenerate(quadix[0].x,quadix[0].y,quadix[1].x,quadix[1].y) ||
             is_degenerate(quadix[0].x,quadix[0].y,quadix[2].x,quadix[2].y) ||
             is_degenerate(quadix[0].x,quadix[0].y,quadix[3].x,quadix[3].y) ||
             is_degenerate(quadix[1].x,quadix[1].y,quadix[2].x,quadix[2].y) ||
             is_degenerate(quadix[1].x,quadix[1].y,quadix[3].x,quadix[3].y) ||
             is_degenerate(quadix[2].x,quadix[2].y,quadix[3].x,quadix[3].y) ||
             /* stage 2 collinearity check  */
             robust_collinear(quadix[0],quadix[1],quadix[2]) ||
             robust_collinear(quadix[1],quadix[2],quadix[3]) ||
             robust_collinear(quadix[2],quadix[3],quadix[0]) ||
             robust_collinear(quadix[3],quadix[0],quadix[1]) ||
             intersect(quadix[0],quadix[1],quadix[2],quadix[3]) ||
             intersect(quadix[0],quadix[3],quadix[1],quadix[2]) ||
             (! convex_quadix(quadix));
   }

   template <typename T>
   inline bool is_degenerate(const quadix<T,3>& quadix)
   {
      return /* stage 1 unique points check */
             is_degenerate(quadix[0].x, quadix[0].y, quadix[1].x, quadix[1].y) ||
             is_degenerate(quadix[0].x, quadix[0].y, quadix[2].x, quadix[2].y) ||
             is_degenerate(quadix[0].x, quadix[0].y, quadix[3].x, quadix[3].y) ||
             is_degenerate(quadix[1].x, quadix[1].y, quadix[2].x, quadix[2].y) ||
             is_degenerate(quadix[1].x, quadix[1].y, quadix[3].x, quadix[3].y) ||
             is_degenerate(quadix[2].x, quadix[2].y, quadix[3].x, quadix[3].y) ||
             /* stage 2 collinearity check  */
             robust_collinear(quadix[0], quadix[1], quadix[2])     ||
             robust_collinear(quadix[1], quadix[2], quadix[3])     ||
             robust_collinear(quadix[2], quadix[3], quadix[0])     ||
             robust_collinear(quadix[3], quadix[0], quadix[1])     ||
             intersect(quadix[0], quadix[1], quadix[2], quadix[3]) ||
             intersect(quadix[0], quadix[3], quadix[1], quadix[2]);
   }

   template <typename T>
   inline bool is_degenerate(const rectangle<T>& rectangle)
   {
      return is_equal(rectangle[0],rectangle[1]);
   }

   template <typename T>
   inline bool is_degenerate(const circle<T>& circle)
   {
      return less_than_or_equal(circle.radius,T(0.0));
   }

   template <typename T>
   inline bool is_degenerate(const sphere<T>& sphere)
   {
      return less_than_or_equal(sphere.radius,T(0.0));
   }

   template <typename T>
   inline bool is_degenerate(const circular_arc<T>& arc)
   {
      return  is_degenerate(arc.x1, arc.y1, arc.x2, arc.y2)                                                ||
              is_degenerate(arc.x1, arc.y1, arc.cx, arc.cy)                                                ||
              is_degenerate(arc.x2, arc.y2, arc.cx, arc.cy)                                                ||
              (lay_distance(arc.x1, arc.y1, arc.cx, arc.cy) != lay_distance(arc.x2,arc.y2,arc.cx,arc.cy))  ||
              (lay_distance(arc.x1, arc.y1, arc.cx, arc.cy) != lay_distance(arc.px,arc.py,arc.cx,arc.cy))  ||
              (cartesian_angle(arc.x1 - arc.cx, arc.y1 - arc.cy ) != arc.angle1)                           ||
              (cartesian_angle(arc.x2 - arc.cx, arc.y2 - arc.cy ) != arc.angle2)                           ||
              (cartesian_angle(arc.px - arc.cx, arc.py - arc.cy ) != abs(arc.angle1 - arc.angle2))         ||
              (orientation (arc.x1,arc.y1,arc.x2,arc.y2,arc.px,arc.py) != arc.orientation);
   }

   template <typename T>
   inline point2d<T> degenerate_point2d()
   {
      return make_point
             (
               infinity<T>(),
               infinity<T>()
             );
   }

   template <typename T>
   inline point3d<T> degenerate_point3d()
   {
      return make_point
             (
               infinity<T>(),
               infinity<T>(),
               infinity<T>()
             );
   }

   template <typename T>
   inline vector2d<T> degenerate_vector2d()
   {
      return make_vector
             (
               infinity<T>(),
               infinity<T>()
             );
   }

   template <typename T>
   inline vector3d<T> degenerate_vector3d()
   {
      return make_vector
             (
               infinity<T>(),
               infinity<T>(),
               infinity<T>()
             );
   }

   template <typename T>
   inline ray<T,2> degenerate_ray2d()
   {
      return make_ray
             (
               degenerate_point2d <T>(),
               degenerate_vector2d<T>()
             );
   }

   template <typename T>
   inline ray<T,3> degenerate_ray3d()
   {
      return make_ray
             (
               degenerate_point3d <T>(),
               degenerate_vector3d<T>()
             );
   }

   template <typename T>
   inline line<T,2> degenerate_line2d()
   {
      return make_line
             (
               degenerate_point2d<T>(),
               degenerate_point2d<T>()
             );
   }

   template <typename T>
   inline line<T,3> degenerate_line3d()
   {
      return make_line
             (
               degenerate_point3d<T>(),
               degenerate_point3d<T>()
             );
   }

   template <typename T>
   inline segment<T,2> degenerate_segment2d()
   {
      return make_segment
             (
               degenerate_point2d<T>(),
               degenerate_point2d<T>()
             );
   }

   template <typename T>
   inline segment<T,3> degenerate_segment3d()
   {
      return make_segment
             (
               degenerate_point3d<T>(),
               degenerate_point3d<T>()
             );
   }

   template <typename T>
   inline triangle<T,2> degenerate_triangle2d()
   {
      return make_triangle
             (
               degenerate_point2d<T>(),
               degenerate_point2d<T>(),
               degenerate_point2d<T>()
             );
   }

   template <typename T>
   inline triangle<T,3> degenerate_triangle3d()
   {
      return make_triangle
             (
               degenerate_point3d<T>(),
               degenerate_point3d<T>(),
               degenerate_point3d<T>()
             );
   }

   template <typename T>
   inline quadix<T,2> degenerate_quadix2d()
   {
      return make_quadix
             (
               degenerate_point2d<T>(),
               degenerate_point2d<T>(),
               degenerate_point2d<T>(),
               degenerate_point2d<T>()
            );
   }

   template <typename T>
   inline quadix<T,3> degenerate_quadix3d()
   {
      return make_quadix
             (
               degenerate_point3d<T>(),
               degenerate_point3d<T>(),
               degenerate_point3d<T>(),
               degenerate_point3d<T>()
            );
   }

   template <typename T>
   inline rectangle<T> degenerate_rectangle()
   {
      return make_rectangle
             (
               degenerate_point2d<T>(),
               degenerate_point2d<T>()
             );
   }

   template <typename T>
   inline circle<T> degenerate_circle()
   {
      return make_circle
             (
               degenerate_point2d<T>(),
               infinity          <T>()
             );
   }

   template <typename T>
   inline sphere<T> degenerate_sphere()
   {
      return make_sphere
            (
              degenerate_point3d<T>(),
              infinity          <T>()
            );
   }

   template <typename T>
   inline point2d<T> positive_infinite_point2d()
   {
      return make_point
             (
               +infinity<T>(),
               +infinity<T>()
             );
   }

   template <typename T>
   inline point2d<T> negative_infinite_point2d()
   {
      return make_point
             (
               -infinity<T>(),
               -infinity<T>()
             );
   }

   template <typename T>
   inline point3d<T> positive_infinite_point3d()
   {
      return make_point
             (
               +infinity<T>(),
               +infinity<T>(),
               +infinity<T>()
             );
   }

   template <typename T>
   inline point3d<T> negative_infinite_point3d()
   {
      return make_point
             (
               -infinity<T>(),
               -infinity<T>(),
               -infinity<T>()
             );
   }

   template <typename T>
   inline void swap(point2d<T>& point1, point2d<T>& point2)
   {
      point2d<T> tmp_point = point1;
      point1 = point2;
      point2 = tmp_point;
   }

   template <typename T>
   inline void swap(point3d<T>& point1, point3d<T>& point2)
   {
      point3d<T> tmp_point = point1;
      point1 = point2;
      point2 = tmp_point;
   }

   template <typename T>
   inline point2d<T> make_point(const T& x, const T& y)
   {
      point2d<T> point;
      point.x = x;
      point.y = y;
      return point;
   }

   template <typename T>
   inline point3d<T> make_point(const T& x, const T& y, const T& z)
   {
      point3d<T> point;
      point.x = x;
      point.y = y;
      point.z = z;
      return point;
   }

   template <typename T>
   inline point2d<T> make_point(const point3d<T> point)
   {
      return make_point(point.x,point.y);
   }

   template <typename T>
   inline point3d<T> make_point(const point2d<T> point, const T& z)
   {
      return make_point(point.x,point.y,z);
   }

   template <typename T>
   inline point2d<T> make_point(const circle<T>& circle)
   {
      return make_point(circle.x,circle.y);
   }

   template <typename T>
   inline point3d<T> make_point(const sphere<T>& sphere)
   {
      return make_point(sphere.x,sphere.y,sphere.z);
   }

   template <typename T>
   inline vector2d<T> make_vector(const T& x, const T& y)
   {
      vector2d<T> v;
      v.x = x;
      v.y = y;
      return v;
   }

   template <typename T>
   inline vector3d<T> make_vector(const T& x, const T& y, const T& z)
   {
      vector3d<T> v;
      v.x = x;
      v.y = y;
      v.z = z;
      return v;
   }

   template <typename T>
   inline vector2d<T> make_vector(const vector3d<T> v)
   {
      return make_vector(v.x,v.y);
   }

   template <typename T>
   inline vector3d<T> make_vector(const vector2d<T> v, const T& z)
   {
      return make_vector(v.x,v.y,z);
   }

   template <typename T>
   inline vector2d<T> make_vector(const point2d<T> point)
   {
      return make_vector(point.x,point.y);
   }

   template <typename T>
   inline vector3d<T> make_vector(const point3d<T> point)
   {
      return make_vector(point.x,point.y,point.z);
   }

   template <typename T>
   inline curve_point<T,2> make_curve_point(const T& x, const T& y, const T& t)
   {
      curve_point<T,2> point;
      point().x = x;
      point().y = y;
      point.t = t;
      return point;
   }

   template <typename T>
   inline ray<T,2> make_ray(const T& ox, const T& oy, const T& dir_x, const T& dir_y)
   {
      ray<T,2> _ray;
      _ray.origin.x    = ox;
      _ray.origin.y    = oy;
      _ray.direction.x = dir_x;
      _ray.direction.y = dir_y;
      _ray.direction   = normalize(_ray.direction);
      return _ray;
   }

   template <typename T>
   inline ray<T,3> make_ray(const T& ox, const T& oy, const T& oz, const T& dir_x, const T& dir_y, const T& dir_z)
   {
      ray<T,3> _ray;
      _ray.origin.x    = ox;
      _ray.origin.y    = oy;
      _ray.origin.z    = oz;
      _ray.direction.x = dir_x;
      _ray.direction.y = dir_y;
      _ray.direction.z = dir_z;
      _ray.direction   = normalize(_ray.direction);
      return _ray;
   }

   template <typename T>
   inline ray<T,2> make_ray(const point2d<T>& origin, const vector2d<T>& direction)
   {
      return make_ray(origin.x,origin.y,direction.x,direction.y);
   }

   template <typename T>
   inline ray<T,3> make_ray(const point3d<T>& origin, const vector3d<T>& direction)
   {
      return make_ray(origin.x,origin.y,origin.z,direction.x,direction.y,direction.z);
   }

   template <typename T>
   inline ray<T,2> make_ray(const point2d<T>& origin, const T& bearing)
   {
      return make_ray(origin,make_vector(project_point<T>(make_point(T(0.0),T(0.0)),bearing,T(1.0))));
   }

   template <typename T>
   inline curve_point<T,3> make_curve_point(const T& x, const T& y, const T& z, const T& t)
   {
      curve_point<T,3> point;
      point().x = x;
      point().y = y;
      point().z = z;
      point.t = t;
      return point;
   }

   template <typename T>
   inline curve_point<T,2> make_curve_point(const point2d<T>& point, const T& t)
   {
      return make_curve_point(point.x,point.y,t);
   }

   template <typename T>
   inline curve_point<T,3> make_curve_point(const point3d<T>& point, const T& t)
   {
      return make_curve_point(point.x,point.y,point.z,t);
   }

   template <typename T>
   inline segment<T,2> make_segment(const T& x1, const T& y1, const T& x2, const T& y2)
   {
      segment<T,2> segment;
      segment[0] = make_point(x1,y1);
      segment[1] = make_point(x2,y2);
      return segment;
   }

   template <typename T>
   inline segment<T,3> make_segment(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2)
   {
      segment<T,3> segment;
      segment[0] = make_point(x1,y1,z1);
      segment[1] = make_point(x2,y2,z2);
      return segment;
   }

   template <typename T>
   inline segment<T,2> make_segment(const point2d<T>& point1, const point2d<T>& point2)
   {
      return make_segment(point1.x,point1.y,point2.x,point2.y);
   }

   template <typename T>
   inline segment<T,3> make_segment(const point3d<T>& point1, const point3d<T>& point2)
   {
      return make_segment(point1.x,point1.y,point1.z,point2.x,point2.y,point2.z);
   }

   template <typename T>
   inline segment<T,2> make_segment(const line<T,2>& line)
   {
      return make_segment(line[0],line[1]);
   }

   template <typename T>
   inline segment<T,3> make_segment(const line<T,3>& line)
   {
      return make_segment(line[0],line[1]);
   }

   template <typename T>
   inline line<T,2> make_line(const T& x1, const T& y1, const T& x2, const T& y2)
   {
      line<T,2> line;
      line[0] = make_point(x1,y1);
      line[1] = make_point(x2,y2);
      return line;
   }

   template <typename T>
   inline line<T,3> make_line(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2)
   {
      line<T,3> line;
      line[0] = make_point(x1,y1,z1);
      line[1] = make_point(x2,y2,z2);
      return line;
   }

   template <typename T>
   inline line<T,2> make_line(const point2d<T>& point1, const point2d<T>& point2)
   {
      return make_line(point1.x,point1.y,point2.x,point2.y);
   }

   template <typename T>
   inline line<T,3> make_line(const point3d<T>& point1, const point3d<T>& point2)
   {
      return make_line(point1.x,point1.y,point1.z,point2.x,point2.y,point2.z);
   }

   template <typename T>
   inline line<T,2> make_line(const segment<T,2>& segment)
   {
      return make_line(segment[0],segment[1]);
   }

   template <typename T>
   inline line<T,3> make_line(const segment<T,3>& segment)
   {
      return make_line(segment[0],segment[1]);
   }

   template <typename T>
   inline line<T,2> make_line(const ray<T,2>& ray)
   {
      return make_line(ray.origin,generate_point_on_ray(ray,T(1.0)));
   }

   template <typename T>
   inline line<T,3> make_line(const ray<T,3>& ray)
   {
      return make_line(ray.origin,generate_point_on_ray(ray,T(1.0)));
   }

   template <typename T>
   inline rectangle<T> make_rectangle(const T& x1, const T& y1, const T& x2, const T& y2)
   {
      rectangle<T> rectangle_;

      rectangle_[0] = make_point(x1, y1);
      rectangle_[1] = make_point(x2, y2);

      if (rectangle_[1].x < rectangle_[0].x) std::swap(rectangle_[0].x,rectangle_[1].x);
      if (rectangle_[1].y < rectangle_[0].y) std::swap(rectangle_[0].y,rectangle_[1].y);

      return rectangle_;
   }

   template <typename T>
   inline rectangle<T> make_rectangle(const point2d<T>& point1, const point2d<T>& point2)
   {
      return make_rectangle(point1.x,point1.y,point2.x,point2.y);
   }

   template <typename T>
   inline box<T,3> make_box(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2)
   {
      box<T,3> box_;

      box_[0] = make_point(x1, y1, z1);
      box_[1] = make_point(x2, y2, z2);

      if (box_[1].x < box_[0].x) std::swap(box_[0].x,box_[1].x);
      if (box_[1].y < box_[0].y) std::swap(box_[0].y,box_[1].y);
      if (box_[1].z < box_[0].z) std::swap(box_[0].z,box_[1].z);

      return box_;
   }

   template <typename T>
   inline box<T,3> make_box(const point3d<T>& point1, const point3d<T>& point2)
   {
      return make_box(point1.x,point1.y,point1.z,point2.x,point2.y,point2.z);
   }

   template <typename T>
   inline triangle<T,2> make_triangle(const T& x1, const T& y1,
                                      const T& x2, const T& y2,
                                      const T& x3, const T& y3)
   {
      triangle<T,2> triangle_;
      triangle_[0] = make_point(x1,y1);
      triangle_[1] = make_point(x2,y2);
      triangle_[2] = make_point(x3,y3);
      return triangle_;
   }

   template <typename T>
   inline triangle<T,3> make_triangle(const T& x1, const T& y1, const T& z1,
                                      const T& x2, const T& y2, const T& z2,
                                      const T& x3, const T& y3, const T& z3)
   {
      triangle<T,3> triangle_;
      triangle_[0] = make_point(x1,y1,z1);
      triangle_[1] = make_point(x2,y2,z2);
      triangle_[2] = make_point(x3,y3,z3);
      return triangle_;
   }

   template <typename T>
   inline triangle<T,2> make_triangle(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3)
   {
      return make_triangle(point1.x,point1.y,
                           point2.x,point2.y,
                           point3.x,point3.y);
   }

   template <typename T>
   inline triangle<T,3> make_triangle(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3)
   {
      return make_triangle(point1.x,point1.y,point1.z,
                           point2.x,point2.y,point2.z,
                           point3.x,point3.y,point3.z);
   }

   template <typename T>
   inline quadix<T,2> make_quadix(const T& x1, const T& y1,
                                  const T& x2, const T& y2,
                                  const T& x3, const T& y3,
                                  const T& x4, const T& y4)
   {
      quadix<T,2> quadix_;

      quadix_[0] = make_point(x1,y1);
      quadix_[1] = make_point(x2,y2);
      quadix_[2] = make_point(x3,y3);
      quadix_[3] = make_point(x4,y4);

      return quadix_;
   }

   template <typename T>
   inline quadix<T,3> make_quadix(const T& x1, const T& y1, const T& z1,
                                  const T& x2, const T& y2, const T& z2,
                                  const T& x3, const T& y3, const T& z3,
                                  const T& x4, const T& y4, const T& z4)
   {
      quadix<T,3> quadix_;

      quadix_[0] = make_point(x1,y1,z1);
      quadix_[1] = make_point(x2,y2,z2);
      quadix_[2] = make_point(x3,y3,z3);
      quadix_[3] = make_point(x4,y4,z4);

      return quadix_;
   }

   template <typename T>
   inline quadix<T,2> make_quadix(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3, const point2d<T>& point4)
   {
      return make_quadix
             (
               point1.x,point1.y,
               point2.x,point2.y,
               point3.x,point3.y,
               point4.x,point4.y
             );
   }

   template <typename T>
   inline quadix<T,3> make_quadix(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const point3d<T>& point4)
   {
      return make_quadix
             (
               point1.x,point1.y,point1.z,
               point2.x,point2.y,point2.z,
               point3.x,point3.y,point3.z,
               point4.x,point4.y,point4.z
             );
   }

   template <typename T>
   inline quadix<T,2> make_quadix(const T& x1, const T& y1, const T& x2, const T& y2)
   {
      quadix<T,2> quadix_;
      quadix_[0] = make_point(x1,y1);
      quadix_[1] = make_point(x2,y1);
      quadix_[2] = make_point(x2,y2);
      quadix_[3] = make_point(x1,y2);
      return quadix_;
   }

   template <typename T>
   inline quadix<T,2> make_quadix(const rectangle<T>& rectangle)
   {
      return make_quadix(rectangle[0].x, rectangle[0].y, rectangle[1].x, rectangle[1].y);
   }

   template <typename T>
   inline circle<T> make_circle(const T& x, const T& y, const T& radius)
   {
      circle<T> circle_;
      circle_.x      = x;
      circle_.y      = y;
      circle_.radius = radius;
      return circle_;
   }

   template <typename T>
   inline circle<T> make_circle(const point2d<T>& point, const T& radius)
   {
      return make_circle(point.x,point.y,radius);
   }

   template <typename T>
   inline circle<T> make_circle(const point2d<T>& point1, const point2d<T>& point2)
   {
      return make_circle
             (
               (point1.x + point2.x)   * T(0.5),
               (point1.y + point2.y)   * T(0.5),
               distance(point1,point2) * T(0.5)
             );
   }

   template <typename T>
   inline circle<T> make_circle(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3)
   {
      return circumcircle(point1,point2,point3);
   }

   template <typename T>
   inline circle<T> make_circle(const triangle<T,2>& triangle)
   {
      return make_circle(triangle[0], triangle[1], triangle[2]);
   }

   template <typename T>
   inline sphere<T> make_sphere(const T& x, const T& y, const T& z, const T& radius)
   {
      sphere<T> sphere_;

      sphere_.x      = x;
      sphere_.y      = y;
      sphere_.z      = z;
      sphere_.radius = radius;

      return sphere_;
   }

   template <typename T>
   inline sphere<T> make_sphere(const point3d<T>& point, const T& radius)
   {
      return make_sphere(point.x, point.y, point.z, radius);
   }

   template <typename T>
   inline sphere<T> make_sphere(const point3d<T>& point1, const point3d<T>& point2)
   {
      return make_sphere
             (
               (point1.x + point2.x)   * T(0.5),
               (point1.y + point2.y)   * T(0.5),
               (point1.z + point2.z)   * T(0.5),
               distance(point1,point2) * T(0.5)
             );
   }

   template <typename T>
   inline plane<T,3> make_plane(const T& x1, const T& y1, const T& z1,
                                const T& x2, const T& y2, const T& z2,
                                const T& x3, const T& y3, const T& z3)
   {
      plane<T,3> plane_;

      vector3d<T> v1 = make_vector(x2 - x1, y2 - y1, z2 - z1);
      vector3d<T> v2 = make_vector(x3 - x1, y3 - y1, z3 - z1);

      plane_.normal   = normalize(v1 * v2);
      plane_.constant = -dot_product(plane_.normal,make_vector(x1,y1,z1));

      return plane_;
   }

   template <typename T>
   inline plane<T,3> make_plane(const T& px, const T& py, const T& pz,
                                const T& nx, const T& ny, const T& nz)
   {
      plane<T,3> plane_;

      plane_.normal   = normalize(make_vector(nx, ny, nz));
      plane_.constant = -dot_product(plane_.normal,make_vector(px, py, pz));

      return plane_;
   }

   template <typename T>
   inline plane<T,3> make_plane(const point3d<T>& point1,
                                const point3d<T>& point2,
                                const point3d<T>& point3)
   {
      return make_plane
             (
               point1.x, point1.y, point1.z,
               point2.x, point2.y, point2.z,
               point3.x, point3.y, point3.z
             );
   }

   template <typename T>
   inline plane<T,3> make_plane(const point3d<T>& point,
                                const vector3d<T>& normal)
   {
      plane<T,3> plane_;

      plane_.normal   = normalize(normal);
      plane_.constant = -dot_product(plane_.normal,make_vector(point));

      return plane_;
   }

   template <typename T>
   inline plane<T,3> make_plane(const triangle<T,3>& triangle)
   {
      return make_plane(triangle[0], triangle[1], triangle[2]);
   }

   template <typename T, std::size_t D, typename InputIterator>
   inline polygon<T,D> make_polygon(const InputIterator begin, const InputIterator end)
   {
      wykobi::polygon<T,D> polygon_;
      polygon_.reserve(std::distance(begin,end));

      InputIterator it = begin;

      while (end != it) { polygon_.push_back((*it++)); }

      return polygon_;
   }

   template <typename T>
   inline polygon<T,2> make_polygon(const std::vector< point2d<T> >& point_list)
   {
      return make_polygon<T,2,typename std::vector< point2d<T> >::const_iterator>(point_list.begin(),point_list.end());
   }

   template <typename T>
   inline polygon<T,3> make_polygon(const std::vector< point3d<T> >& point_list)
   {
      return make_polygon<T,3,typename std::vector< point3d<T> >::const_iterator>(point_list.begin(),point_list.end());
   }

   template <typename T>
   inline polygon<T,2> make_polygon(const triangle<T,2>& triangle)
   {
      wykobi::polygon<T,2> polygon_;

      polygon_.reserve(wykobi::triangle<T,2>::PointCount);

      for (std::size_t i = 0; i < wykobi::triangle<T,2>::PointCount; ++i)
      {
         polygon_.push_back(triangle[i]);
      }

      return polygon_;
   }

   template <typename T>
   inline polygon<T,2> make_polygon(const quadix<T,2>& quadix)
   {
      wykobi::polygon<T,2> polygon_;

      polygon_.reserve(wykobi::quadix<T,2>::PointCount);

      for (std::size_t i = 0; i < wykobi::quadix<T,2>::PointCount; ++i)
      {
         polygon_.push_back(quadix[i]);
      }

      return polygon_;
   }

   template <typename T>
   inline polygon<T,2> make_polygon(const rectangle<T>& rectangle)
   {
      wykobi::polygon<T,2> polygon_;

      polygon_.reserve(4);

      polygon_.push_back(make_point(rectangle[0].x,rectangle[0].y));
      polygon_.push_back(make_point(rectangle[1].x,rectangle[0].y));
      polygon_.push_back(make_point(rectangle[1].x,rectangle[1].y));
      polygon_.push_back(make_point(rectangle[0].x,rectangle[1].y));

      return polygon_;
   }

   template <typename T>
   inline polygon<T,2> make_polygon(const circle<T>& circle, const unsigned int point_count)
   {
      wykobi::polygon<T,2> polygon_;

      polygon_.reserve(point_count);

      T angle = T(360.0 / (1.0 * point_count));

      for (std::size_t i = 0; i < point_count; ++i)
      {
         point2d<T> point_;
         rotate(angle * T(1.0 * i), circle.x + circle.radius, circle.y, circle.x, circle.y, point_.x, point_.y);
         polygon_.push_back(point_);
      }

      return polygon_;
   }

} // wykobi namespace
