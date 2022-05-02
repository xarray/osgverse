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
      struct isotropic_normalization < point2d<T> >
      {
      public:

         template <typename InputIterator>
         isotropic_normalization(InputIterator begin, InputIterator end)
         {
            T mean_x = T(0.0);
            T mean_y = T(0.0);
            T n      = T(1.0 * std::distance(begin,end));

            for (InputIterator it = begin; it != end; ++it)
            {
               mean_x += (*it).x;
               mean_y += (*it).y;
            }

            mean_x /= n;
            mean_y /= n;

            T total_distance = T(0.0);

            for (InputIterator it = begin; it != end; ++it)
            {
               total_distance += sqrt(sqr((*it).x - mean_x) + sqr((*it).y - mean_y));
            }

            T scale       = n * sqrt(T(2.0)) / total_distance;
            T translate_x = -mean_x * scale;
            T translate_y = -mean_y * scale;

            for (InputIterator it = begin; it != end; ++it)
            {
               (*it).x = (*it).x * scale + translate_x;
               (*it).y = (*it).y * scale + translate_y;
            }
         }
      };

      template <typename T>
      struct isotropic_normalization < point3d<T> >
      {
      public:

         template <typename InputIterator>
         isotropic_normalization(InputIterator begin, InputIterator end)
         {
            T mean_x = T(0.0);
            T mean_y = T(0.0);
            T mean_z = T(0.0);
            T n      = T(1.0 * std::distance(begin,end));

            for (InputIterator it = begin; it != end; ++it)
            {
               mean_x += (*it).x;
               mean_y += (*it).y;
               mean_z += (*it).z;
            }

            mean_x /= n;
            mean_y /= n;
            mean_z /= n;

            T total_distance = T(0.0);

            for (InputIterator it = begin; it != end; ++it)
            {
               total_distance += sqrt(sqr((*it).x - mean_x) + sqr((*it).y - mean_y) + sqr((*it).z - mean_z));
            }

            T scale       = n * sqrt(T(2.0)) / total_distance;
            T translate_x = -mean_x * scale;
            T translate_y = -mean_y * scale;
            T translate_z = -mean_z * scale;

            for (InputIterator it = begin; it != end; ++it)
            {
               (*it).x = (*it).x * scale + translate_x;
               (*it).y = (*it).y * scale + translate_y;
               (*it).z = (*it).z * scale + translate_z;
            }
         }
      };

      template <typename T>
      struct covariance_matrix< point2d<T> >
      {
      public:

         template <typename InputIterator>
         matrix<T,2,2> operator()(InputIterator begin, InputIterator end)
         {
            T mean_x = T(0.0);
            T mean_y = T(0.0);
            T n      = T(1.0 * std::distance(begin,end));

            for (InputIterator it = begin; it != end; ++it)
            {
               mean_x += (*it).x;
               mean_y += (*it).y;
            }

            mean_x /= n;
            mean_y /= n;

            matrix<T,2,2> matrix;

            for (InputIterator it = begin; it != end; ++it)
            {
               point2d<T> point = translate(-mean_x,-mean_y,(*it));
               matrix(0,0) += (point.x * point.x); matrix(1,0) += (point.x * point.y);
               matrix(0,1) += (point.x * point.y); matrix(1,1) += (point.y * point.y);
            }

            matrix /= n;

            return matrix;
         }
      };

      template <typename T>
      struct covariance_matrix< point3d<T> >
      {
      public:
         template <typename InputIterator>
         matrix<T,3,3> operator()(InputIterator begin, InputIterator end)
         {
            T mean_x = T(0.0);
            T mean_y = T(0.0);
            T mean_z = T(0.0);
            T n      = T(1.0 * std::distance(begin,end));

            for (InputIterator it = begin; it != end; ++it)
            {
               mean_x += (*it).x;
               mean_y += (*it).y;
               mean_z += (*it).z;
            }

            mean_x /= n;
            mean_y /= n;
            mean_z /= n;

            matrix<T,3,3> matrix;

            for (InputIterator it = begin; it != end; ++it)
            {
               point3d<T> point = translate(-mean_x,-mean_y,-mean_z,(*it));

               matrix(0,0) += (point.x * point.x); matrix(1,0) += (point.x * point.y); matrix(2,0) += (point.x * point.z);
               matrix(0,1) += (point.x * point.y); matrix(1,1) += (point.y * point.y); matrix(2,1) += (point.y * point.z);
               matrix(0,2) += (point.x * point.z); matrix(1,2) += (point.y * point.z); matrix(2,2) += (point.z * point.z);
            }

            matrix /= n;

            return matrix;
         }
      };

   } // namespace wykobi::algorithm

} // namespace wykobi
