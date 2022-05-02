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

#include <algorithm>
#include <vector>
#include <iterator>


namespace wykobi
{
   namespace algorithm
   {
      template <typename T>
      struct randomized_minimum_bounding_ball < point2d<T> >
      {
      public:
         template <typename InputIterator>
         randomized_minimum_bounding_ball(InputIterator begin,
                                          InputIterator end,
                                          circle<T>& circle)
         {
            std::size_t point_count = std::distance(begin,end);

            switch (point_count)
            {
               case 0 : return;

               case 1 : circle = make_circle(*begin,T(0.0));
                        return;

               case 2 : circle = make_circle(*begin,*(begin + 1));
                        return;

               case 3 : circle = make_circle(*begin,*(begin + 1),*(begin + 2));
                        return;
            }

            std::vector< point2d<T> > point_list;

            point_list.reserve(point_count);

            std::copy(begin,end,std::back_inserter(point_list));

            std::random_shuffle(point_list.begin(),point_list.end());

            circle = make_circle(*point_list.begin(),*(point_list.begin() + 1));

            for (InputIterator it = point_list.begin() + 2; it != point_list.end(); ++it)
            {
               if (!point_in_circle(*it,circle))
               {
                  circle = minimum_ball_with_1_point(point_list.begin(),it - 1,*it);
               }
            }
         }

      private:
         template <typename InputIterator>
         circle<T> minimum_ball_with_1_point(InputIterator begin,
                                             InputIterator end,
                                             const point2d<T>& q)
         {
            std::random_shuffle(begin,end);

            circle<T> circle = make_circle(q,*begin);

            for (InputIterator it = begin + 1; it != end; ++it)
            {
               if (!point_in_circle(*it,circle))
               {
                  circle = minimum_ball_with_2_points(begin,it - 1,q,*it);
               }
            }

            return circle;
         }

         template <typename InputIterator>
         circle<T> minimum_ball_with_2_points(InputIterator begin,
                                              InputIterator end,
                                              const point2d<T>& q1,
                                              const point2d<T>& q2)
         {
            std::random_shuffle(begin,end);

            circle<T> circle = make_circle(q1,q2);

            for (InputIterator it = begin; it != end; ++it)
            {
               if (!point_in_circle(*it,circle))
               {
                  circle = make_circle(q1,q2,*it);
               }
            }

            return circle;
         }
      };

      template <typename T>
      struct randomized_minimum_bounding_ball_with_ch_filter < point2d<T> >
      {
      public:

         template <typename InputIterator>
         randomized_minimum_bounding_ball_with_ch_filter(InputIterator begin,
                                                         InputIterator end,
                                                         circle<T>& circle)
         {
            std::vector< point2d<T> > convex_hull;

            convex_hull_graham_scan< point2d<T> >(begin,end,std::back_inserter(convex_hull));

            randomized_minimum_bounding_ball< point2d<T> >(convex_hull.begin(),convex_hull.end(),circle);
         }
      };

      template <typename T>
      struct naive_minimum_bounding_ball < point2d<T> >
      {
      public:

         template <typename InputIterator>
         naive_minimum_bounding_ball(InputIterator begin,
                                     InputIterator end,
                                     circle<T>& circle)
         {
            switch (std::distance(begin,end))
            {
               case 0 : return;

               case 1 : circle = make_circle(*begin,T(0.0));
                        return;

               case 2 : circle = make_circle(*begin,*(begin + 1));
                        return;

               case 3 : circle = make_circle(*begin,*(begin + 1),*(begin + 2));
                        return;
            }

            circle = degenerate_circle<T>();

            // Expected complexity O(n^4)
            for (InputIterator it1 = begin; it1 != end; ++it1)
            {
               for (InputIterator it2 = it1 + 1; it2 != end; ++it2)
               {
                  for (InputIterator it3 = it2 + 1; it3 != end; ++it3)
                  {
                     wykobi::circle<T> current_circle = make_circle((*it1),(*it2),(*it3));

                     bool contains_all_points = true;

                     for (InputIterator n = begin; n != end; ++n)
                     {
                        if (
                             (n!= it1) && (n!= it2) && (n!= it3) &&
                             (!point_in_circle((*n),current_circle))
                           )
                        {
                           contains_all_points = false;

                           break;
                        }
                     }

                     if (contains_all_points && (current_circle.radius < circle.radius))
                     {
                        circle = current_circle;
                     }
                  }
               }
            }
         }
      };

      template <typename T>
      struct naive_minimum_bounding_ball_with_ch_filter < point2d<T> >
      {
      public:

         template <typename InputIterator>
         naive_minimum_bounding_ball_with_ch_filter(InputIterator begin,
                                                    InputIterator end,
                                                    circle<T>& circle)
         {
            std::vector< point2d<T> > convex_hull;

            convex_hull_graham_scan< point2d<T> >(begin,end,std::back_inserter(convex_hull));

            naive_minimum_bounding_ball< point2d<T> >(convex_hull.begin(),convex_hull.end(),circle);
         }
      };

      template <typename T>
      struct ritter_minimum_bounding_ball < point2d<T> >
      {
      public:

         template <typename InputIterator>
         ritter_minimum_bounding_ball(InputIterator begin,
                                      InputIterator end,
                                      circle<T>& circle)
         {
            switch (std::distance(begin,end))
            {
               case 0 : return;

               case 1 : circle = make_circle(*begin, T(0.0));
                        return;

               case 2 : circle = make_circle(*begin, *(begin + 1));
                        return;

               case 3 : circle = make_circle(*begin, *(begin + 1), *(begin + 2));
                        return;
            }

            point2d<T> min_x = positive_infinite_point2d<T>();
            point2d<T> min_y = positive_infinite_point2d<T>();
            point2d<T> max_x = negative_infinite_point2d<T>();
            point2d<T> max_y = negative_infinite_point2d<T>();

            for (InputIterator it = begin; it != end; ++it)
            {
               point2d<T> current_point = *it;

               if (current_point.x < min_x.x) min_x = current_point;
               if (current_point.x > max_x.x) max_x = current_point;
               if (current_point.y < min_y.y) min_y = current_point;
               if (current_point.y > max_y.y) max_y = current_point;
            }

            T span_x   = distance(max_x,min_x);
            T span_y   = distance(max_y,min_y);

            point2d<T> dia1 = negative_infinite_point2d<T>();
            point2d<T> dia2 = negative_infinite_point2d<T>();

            if (span_x > span_y)
            {
               dia1     = min_x;
               dia2     = max_x;
            }
            else
            {
               dia1     = min_y;
               dia2     = max_y;
            }

            circle = make_circle(dia1,dia2);
            T radius_sqr = sqr(circle.radius);

            for (InputIterator it = begin; it != end; ++it)
            {
               point2d<T> current_point = *it;

               T lay_dist = lay_distance_from_point_to_circle_center(current_point,circle);

               if (lay_dist > radius_sqr)
               {
                  T dist        = sqrt(lay_dist);
                  circle.radius = (circle.radius + dist) * T(0.5);
                  radius_sqr    = sqr(circle.radius);
                  T difference  = dist - circle.radius;
                  T ratio       = T(1.0) / dist;
                  circle        = make_circle
                                  (
                                    (circle.radius * circle.x + difference * current_point.x) * ratio,
                                    (circle.radius * circle.y + difference * current_point.y) * ratio,
                                    circle.radius
                                  );
               }
            }
         }
      };

      template <typename T>
      struct ritter_minimum_bounding_ball_with_ch_filter < point2d<T> >
      {
      public:

         template <typename InputIterator>
         ritter_minimum_bounding_ball_with_ch_filter(InputIterator begin,
                                                     InputIterator end,
                                                     circle<T>& circle)
         {
            std::vector< point2d<T> > convex_hull;

            convex_hull_graham_scan< point2d<T> >(begin,end,std::back_inserter(convex_hull));

            ritter_minimum_bounding_ball< point2d<T> >(convex_hull.begin(),convex_hull.end(),circle);
         }
      };

      template <typename T>
      struct ritter_minimum_bounding_ball < point3d<T> >
      {
      public:

         template <typename InputIterator>
         ritter_minimum_bounding_ball(InputIterator begin,
                                      InputIterator end,
                                      sphere<T>& sphere)
         {
            switch (std::distance(begin,end))
            {
               case 0 : return;

               case 1 : sphere = make_sphere(*begin,T(0.0));
                        return;

               case 2 : sphere = make_sphere(*begin,*(begin + 1));
                        return;
            }

            point3d<T> min_x = positive_infinite_point3d<T>();
            point3d<T> min_y = positive_infinite_point3d<T>();
            point3d<T> min_z = positive_infinite_point3d<T>();
            point3d<T> max_x = negative_infinite_point3d<T>();
            point3d<T> max_y = negative_infinite_point3d<T>();
            point3d<T> max_z = negative_infinite_point3d<T>();

            for (InputIterator it = begin; it != end; ++it)
            {
               point3d<T> current_point = *it;

               if (current_point.x < min_x.x) min_x = current_point;
               if (current_point.x > max_x.x) max_x = current_point;
               if (current_point.y < min_y.y) min_y = current_point;
               if (current_point.y > max_y.y) max_y = current_point;
               if (current_point.z < min_z.z) min_z = current_point;
               if (current_point.z > max_z.z) max_z = current_point;
            }

            T span_x   = distance(max_x,min_x);
            T span_y   = distance(max_y,min_y);
            T span_z   = distance(max_z,min_z);

            T max_span = span_x;

            point3d<T> dia1 = min_x;
            point3d<T> dia2 = max_x;

            if (span_y > max_span)
            {
               max_span = span_y;
               dia1     = min_y;
               dia2     = max_y;
            }

            if (span_z > max_span)
            {
               max_span = span_z;
               dia1     = min_z;
               dia2     = max_z;
            }

            sphere = make_sphere(dia1,dia2);
            T radius_sqr = sqr(sphere.radius);

            for (InputIterator it = begin; it != end; ++it)
            {
               point3d<T> current_point = *it;

               T lay_dist = lay_distance_from_point_to_sphere_center(current_point,sphere);

               if (lay_dist > radius_sqr)
               {
                  T dist        = sqrt(lay_dist);
                  sphere.radius = (sphere.radius + dist) * T(0.5);
                  radius_sqr    = sqr(sphere.radius);
                  T difference  = dist - sphere.radius;
                  T ratio       = T(1.0) / dist;

                  sphere = make_sphere
                           (
                             (sphere.radius * sphere.x + difference * current_point.x) * ratio,
                             (sphere.radius * sphere.y + difference * current_point.y) * ratio,
                             (sphere.radius * sphere.z + difference * current_point.z) * ratio,
                             sphere.radius
                           );
               }
            }
         }
      };

   } // namespace wykobi::algorithm

} // namespace wykobi
