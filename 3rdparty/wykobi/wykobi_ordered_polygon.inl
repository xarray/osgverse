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
#include "wykobi_algorithm.hpp"

#include <vector>
#include <deque>
#include <algorithm>


namespace wykobi
{
   namespace algorithm
   {
      template <typename T>
      struct ordered_polygon< point2d<T> >
      {
      public:

         template <typename InputIterator, typename OutputIterator>
         ordered_polygon(InputIterator begin, InputIterator end, OutputIterator out)
         {
            const std::size_t point_count = std::distance(begin,end);

            if (point_count <= 3)
            {
               std::copy(begin,end,out);
               return;
            }

            std::vector< o_point > point;

            for (InputIterator it = begin; it != end; ++it)
            {
               point.push_back(o_point((*it).x,(*it).y,T(0.0)));
            }

            std::size_t j = 0;

            for (std::size_t i = 1; i < point.size(); ++i)
            {
               if (point[i].y < point[j].y)
                  j = i;
               else if (point[i].y == point[j].y)
                  if (point[i].x < point[j].x)
                     j = i;
            }

            std::iter_swap(point.begin(),(point.begin() + j));

            for (typename std::vector< o_point >::iterator it = ++point.begin(); it != point.end(); ++it)
            {
               (*it).angle = cartesian_angle(static_cast< point2d<T> >(*it),static_cast<point2d<T> >(point.front()));
            }

            sort(++point.begin(),point.end(),point_comparator(&point.front()));

            for (typename std::vector< o_point >::iterator it = point.begin(); it != point.end(); ++it)
            {
               (*out++) = make_point<T>((*it).x, (*it).y);
            }
         }

      private:

         class o_point : public point2d<T>
         {
         public:

            o_point(const T& _x   = T(0.0),
                    const T& _y   = T(0.0),
                    const T& _ang = T(0.0))
            : angle(_ang)
            {
               point2d<T>::x = _x;
               point2d<T>::y = _y;
            }

            T angle;
         };

         class point_comparator
         {
         public:

            point_comparator(o_point* _anchor)
            : anchor(_anchor)
            {}

            bool operator()(const o_point& p1, const o_point& p2)
            {
               if (p1.angle < p2.angle)
                  return true;
               else if (p1.angle > p2.angle)
                  return false;
               else if (is_equal(static_cast< const point2d<T> >(p1),static_cast< const point2d<T> >(p2)))
                  return false;
               else if (lay_distance(static_cast< const point2d<T> >(*anchor),
                                     static_cast< const point2d<T> >(p1)) <
                        lay_distance(static_cast< const point2d<T> >(*anchor),
                                     static_cast< const point2d<T> >(p2)))
                  return true;
               else
                 return false;
            }

         private:

            o_point* anchor;
         };

      };

   } // namespace wykobi::algorithm

} // namespace wykobi
