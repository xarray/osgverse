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
#include "wykobi_matrix.hpp"


namespace wykobi
{
   namespace algorithm
   {

      template <typename T>
      struct generate_axis_projection_descriptor
      {
      public:

         template <typename OutputIterator>
         generate_axis_projection_descriptor(const polygon<T,2>& polygon, OutputIterator descriptor)
         {
            const std::size_t axis_count = 36;

            if (polygon.size() == 0)
               return;

            std::vector<T> value;
            value.reserve(axis_count);

            point2d<T> origin_point  = make_point(T(0.0),T(0.0));
            point2d<T> rotated_point = make_point(T(1.0),T(0.0));

            for (std::size_t i = 0; i < axis_count; ++i)
            {
               value.push_back(distance(project_onto_axis(polygon,make_line(origin_point,rotated_point))));
               rotated_point = rotate(T(10.0),rotated_point,origin_point);
            }

            T largest_value = -infinity<T>();

            for (typename std::vector<T>::iterator it = value.begin(); it != value.end(); ++it)
            {
               if (*it > largest_value) largest_value = *it;
            }

            for (typename std::vector<T>::iterator it = value.begin(); it != value.end(); ++it)
            {
               (*it) /= largest_value;
            }

            typename std::vector<T>::iterator smallest_it = value.begin();

            for (typename std::vector<T>::iterator it = value.begin() + 1; it != value.end(); ++it)
            {
               if ((*it) < (*smallest_it)) smallest_it = it;
            }

            std::rotate(value.begin(),smallest_it,value.end());
            std::copy(value.begin(),value.end(),descriptor);
         }
      };

   } // namespace wykobi::algorithm

} // namespace wykobi
