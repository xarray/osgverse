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
#include <iterator>
#include <vector>


namespace wykobi
{
   namespace algorithm
   {
      /*
        Note: Clip boundries must be convex.
      */

      template <typename T>
      struct sutherland_hodgman_polygon_clipper< point2d<T> >
      {
      public:

         sutherland_hodgman_polygon_clipper (const rectangle<T>& clip_boundry,
                                             const polygon<T,2>& input_polygon,
                                                   polygon<T,2>& clipped_polygon)
         {
            if (input_polygon.size() < 3) return;

            sutherland_hodgman_polygon_clipper_engine< point2d<T> > clipper_engine;

            clipper_engine.register_edge(edge(clip_boundry, 0)[1], edge(clip_boundry, 0)[0]);
            clipper_engine.register_edge(edge(clip_boundry, 1)[1], edge(clip_boundry, 1)[0]);
            clipper_engine.register_edge(edge(clip_boundry, 2)[1], edge(clip_boundry, 2)[0]);
            clipper_engine.register_edge(edge(clip_boundry, 3)[1], edge(clip_boundry, 3)[0]);

            clipped_polygon.clear();

            clipper_engine.clip
                           (
                             input_polygon.begin(),
                             input_polygon.end  (),
                             std::back_inserter(clipped_polygon)
                           );
         }

         template <typename ClipObject>
         sutherland_hodgman_polygon_clipper (const ClipObject& clip_boundry,
                                             const polygon<T,2>& input_polygon,
                                                   polygon<T,2>& clipped_polygon)
         {
            if (input_polygon.size() < 3) return;

            sutherland_hodgman_polygon_clipper_engine< point2d<T> > clipper_engine;

            if (orientation(clip_boundry[0],clip_boundry[1],clip_boundry[2]) == LeftHandSide)
            {
               std::size_t j = 0;

               for (std::size_t i = clip_boundry.size() - 1; (0 <= i) && (i < clip_boundry.size()); i--)
               {
                  clipper_engine.register_edge(clip_boundry[j],clip_boundry[i]);
                  j = i;
               }
            }
            else
            {
               std::size_t j = clip_boundry.size() - 1;

               for (std::size_t i = 0; i < clip_boundry.size(); ++i)
               {
                  clipper_engine.register_edge(clip_boundry[j],clip_boundry[i]);
                  j = i;
               }
            }

            clipped_polygon.clear();

            clipper_engine.clip
                           (
                             input_polygon.begin(),
                             input_polygon.end  (),
                             std::back_inserter(clipped_polygon)
                           );
         }
      };

      template <typename T>
      struct sutherland_hodgman_polygon_clipper_engine< point2d<T> >
      {
      public:

         void register_edge(const point2d<T>& point1, const point2d<T>& point2)
         {
            edge_list.push_back(std::make_pair/*<const point2d<T>, const point2d<T> >*/(point1,point2));
         }

         template <typename InputIterator, typename OutputIterator>
         void clip(InputIterator begin, InputIterator end, OutputIterator out)
         {
            if (std::distance(begin,end) < 3) return;

            if (edge_list.size() < 3) return;

            std::vector< point2d<T> > clip_poly1;
            std::vector< point2d<T> > clip_poly2;

            std::copy(begin,end,std::back_inserter(clip_poly1));

            for (std::size_t i = 0; i < edge_list.size(); ++i)
            {
               point2d<T> point1 = edge_list[i].first;
               point2d<T> point2 = edge_list[i].second;

               switch (i & 0x01)
               {
                  case 0 : clip_against_edge(half_plane_edge(point1, point2), clip_poly1, clip_poly2);
                           break;

                  case 1 : clip_against_edge(half_plane_edge(point1, point2), clip_poly2, clip_poly1);
                           break;
               }
            }

            switch (edge_list.size() & 0x01)
            {
               case 0 : std::copy(clip_poly1.begin(), clip_poly1.end(), out);
                        break;

               case 1 : std::copy(clip_poly2.begin(), clip_poly2.end(), out);
                        break;
            }
         }

      private:

         class half_plane_edge
         {
         public:

            half_plane_edge(const point2d<T>& point1, const point2d<T>& point2)
            : a(point2.y - point1.y),
              b(point1.x - point2.x),
              c(-a * point1.x - b * point1.y)
            {}

            bool inside_half_plane(const point2d<T>& point) const
            {
               return (T(0.0) < (a * point.x + b * point.y + c));
            }

            point2d<T> intersection_point(const point2d<T>& point1, const point2d<T>& point2) const
            {
               const T d = point2.y - point1.y;
               const T e = point1.x - point2.x;
               const T f = -d * point1.x - e * point1.y;

               const T ratio = T(1.0) / (e * a - b * d);

               return make_point((b * f - e * c) * ratio, (d * c - a * f) * ratio);
            }

         private:

            T a;
            T b;
            T c;
         };

         void clip_against_edge(const half_plane_edge& edge,
                                std::vector< point2d<T> >& input_poly,
                                std::vector< point2d<T> >& clipped_poly)
         {
            if (input_poly.size() < 2)
            {
               input_poly  .clear();
               clipped_poly.clear();

               return;
            }

            point2d<T> previous_point = input_poly.back();

            for (typename std::vector< point2d<T> >::iterator it = input_poly.begin(); it != input_poly.end(); ++it)
            {
               point2d<T> current_point = (*it);

               bool current_point_in  = edge.inside_half_plane(current_point );
               bool previous_point_in = edge.inside_half_plane(previous_point);

               if (current_point_in && previous_point_in)
               {
                  clipped_poly.push_back(current_point);
               }
               else if (!current_point_in && previous_point_in)
               {
                  clipped_poly.push_back(edge.intersection_point(current_point, previous_point));
               }
               else if (current_point_in && !previous_point_in)
               {
                  clipped_poly.push_back(edge.intersection_point(previous_point, current_point));
                  clipped_poly.push_back(current_point);
               }

               previous_point = current_point;
            }

            input_poly.clear();
         }

         std::vector< std::pair< point2d<T>,point2d<T> > > edge_list;

      };

   } // namespace wykobi::algorithm

} // namespace wykobi
