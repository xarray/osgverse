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

#include <algorithm>


namespace wykobi
{
   namespace algorithm
   {
      template <typename T>
      struct polygon_triangulate < point2d<T> >
      {
      public:

         template <typename OutputIterator>
         polygon_triangulate(const polygon<T,2>& polygon, OutputIterator out)
         {
            if (polygon.size() < 3)
               return;
            else if (polygon.size() == 3)
            {
               (*out++) = make_triangle(polygon[0], polygon[1], polygon[2]);
               return;
            }

            wykobi::polygon<T,2> internal_polygon;

            internal_polygon.reserve(polygon.size());

            std::copy(polygon.begin(),polygon.end(),std::back_inserter(internal_polygon));

            if (wykobi::polygon_orientation(internal_polygon) != Clockwise)
            {
               internal_polygon.reverse();
            }

            while (internal_polygon.size() > 3)
            {
               for (std::size_t i = 0; i < internal_polygon.size(); ++i)
               {
                  if (convex_vertex(i,internal_polygon,Clockwise) && vertex_is_ear(i,internal_polygon))
                  {
                     (*out++) = vertex_triangle(i,internal_polygon);

                     internal_polygon.erase(i);

                     break;
                  }
               }
            }

            (*out++) = vertex_triangle(1,internal_polygon);
         }
      };

   } // namespace wykobi::algorithm

} // namespace wykobi
