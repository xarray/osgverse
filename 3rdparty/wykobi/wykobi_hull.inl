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
      struct convex_hull_graham_scan < point2d<T> >
      {
      public:

         template <typename InputIterator, typename OutputIterator>
         convex_hull_graham_scan(InputIterator begin, InputIterator end, OutputIterator out)
         {
            if (std::distance(begin,end) <= 3)
            {
               std::copy(begin,end,out);
               return;
            }

            std::vector<gs_point> point;

            for (InputIterator it = begin; it != end; ++it)
            {
               point.push_back(gs_point((*it).x,(*it).y,T(0.0)));
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

            for (typename std::vector<gs_point>::iterator it = ++point.begin(); it != point.end(); ++it)
            {
               (*it).angle = cartesian_angle(static_cast<point2d<T> >(*it),static_cast<point2d<T> >(point.front()));
            }

            sort(++point.begin(),point.end(),gs_point_comparator(&point.front()));

            std::deque< gs_point > pnt_queue;

            pnt_queue.push_front(point[0]);
            pnt_queue.push_front(point[1]);

            typename std::vector<gs_point>::iterator it = (point.begin() + 2);

            while (it != point.end())
            {
               if (pnt_queue.size() > 1)
               {
                  if (orientation((*++pnt_queue.begin()),(*pnt_queue.begin()),(*it)) == CounterClockwise)
                     pnt_queue.push_front((*it++));
                  else
                   pnt_queue.pop_front();
               }
               else
                  pnt_queue.push_front((*it++));
            }

            for (typename std::deque<gs_point>::iterator itr = pnt_queue.begin(); itr != pnt_queue.end(); ++itr)
            {
               (*out++) = make_point<T>((*itr).x, (*itr).y);
            }
         }

      private:

         class gs_point : public point2d<T>
         {
         public:

            gs_point(const T& _x   = T(0.0),
                     const T& _y   = T(0.0),
                     const T& _ang = T(0.0))
            : angle(_ang)
            {
               point2d<T>::x = _x;
               point2d<T>::y = _y;
            }

            T angle;
         };

         class gs_point_comparator
         {
         public:

            gs_point_comparator(gs_point* _anchor)
            : anchor(_anchor)
            {}

            bool operator()(const gs_point& p1, const gs_point& p2)
            {
               if (p1.angle < p2.angle)
                  return true;
               else if (p1.angle > p2.angle)
                  return false;
               else if (is_equal(static_cast< const point2d<T> >(p1),static_cast< const point2d<T> >(p2)))
                  return false;
               else if (
                         lay_distance(static_cast< const point2d<T> >(*anchor),
                                      static_cast< const point2d<T> >(p1)) <
                         lay_distance(static_cast< const point2d<T> >(*anchor),
                                      static_cast< const point2d<T> >(p2))
                       )
                  return true;
               else
                 return false;
            }

         private:

            gs_point* anchor;
         };

      };

      template <typename T>
      struct convex_hull_jarvis_march< point2d<T> >
      {
      public:

         template <typename InputIterator, typename OutputIterator>
         convex_hull_jarvis_march(InputIterator begin, InputIterator end, OutputIterator out)
         {
            const std::size_t point_count = std::distance(begin,end);

            if (point_count <= 3)
            {
               std::copy(begin,end,out);
               return;
            }

            std::vector< point2d<T> >point_list;

            point_list.reserve(point_count);

            point2d<T> lowest_point = *begin;

            for (InputIterator it = begin + 1; it != end; ++it)
            {
               if ((*it).y < lowest_point.y)
               {
                  lowest_point = *it;
               }
            }

            point_list.push_back (lowest_point);

            point2d<T> previous_point = lowest_point;
            point2d<T> current_point;

            do
            {
               current_point = (not_equal(previous_point,*begin)) ? *begin : *(begin + 1);

               for (InputIterator it = begin; it != end; ++it)
               {
                  if (orientation(previous_point, current_point, *it) == RightHandSide)
                  {
                     current_point = *it;
                  }
               }

               if (not_equal(current_point,lowest_point))
               {
                  point_list.push_back(current_point);
               }

               previous_point = current_point;
            }
            while (not_equal(current_point,lowest_point));

            /* Remove consecutive collinear points */
            typedef typename std::vector< point2d<T> >::iterator Iterator;

            Iterator previous_it = point_list.end() - 1;

            for (Iterator it = point_list.begin(); it != (point_list.end() - 1); ++it)
            {
               if (orientation(*previous_it,*it,*(it + 1)) != CollinearOrientation)
               {
                  (*out++) = *it;
                  previous_it = it;
               }
            }

            if (orientation(*previous_it,point_list.back(),point_list.front()) != CollinearOrientation)
            {
               (*out++) = point_list.back();
            }
         }

      };

      template <typename Type>
      struct convex_hull_melkman< point2d<Type> >
      {
      public:

         template <typename InputIterator, typename OutputIterator>
         convex_hull_melkman(InputIterator begin, InputIterator end, OutputIterator out)
         {
            const std::size_t point_count = std::distance(begin,end);

            if (point_count <= 3)
            {
               std::copy(begin,end,out);
               return;
            }

            std::deque< point2d<Type> > deq;

            if (orientation((*(begin + 0)),(*(begin + 1)),(*(begin + 2))) == LeftHandSide)
            {
               deq.push_front((*(begin + 2)));
               deq.push_front((*(begin + 0)));
               deq.push_front((*(begin + 1)));
               deq.push_front((*(begin + 2)));
            }
            else
            {
               deq.push_front((*(begin + 2)));
               deq.push_front((*(begin + 1)));
               deq.push_front((*(begin + 0)));
               deq.push_front((*(begin + 2)));
            }

            for (InputIterator it = (begin + 3); it != end; ++it)
            {
               if (
                    (orientation(deq[deq.size() - 1], deq[deq.size() - 2], (*it)) == LeftHandSide) &&
                    (orientation(deq[1],              deq[0],              (*it)) == LeftHandSide)
                  )
               {
                  continue;
               }

               while (orientation(deq[deq.size() - 1],deq[deq.size() - 2],(*it)) != LeftHandSide)
               {
                  deq.pop_back();
               }

               deq.push_back((*it));

               while (orientation(deq[1],deq[0],(*it)) != LeftHandSide)
               {
                  deq.pop_front();
               }

               deq.push_front((*it));
            }

            std::copy(deq.begin(),deq.end() - 1,out);
         }
      };

   } // namespace wykobi::algorithm

} // namespace wykobi
