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


#ifndef INCLUDE_WYKOBI_MATRIX
#define INCLUDE_WYKOBI_MATRIX


#include <vector>
#include <limits>
#include <cstdlib>
#include <cassert>

#include "wykobi.hpp"
#include "wykobi_math.hpp"


namespace wykobi
{
   template <typename T, std::size_t M, std::size_t N>
   class matrix
   {
   public:

      matrix()
      : dptr(reinterpret_cast<T*>(&data))
      {
         zero();
      }

     ~matrix()
      {}

      matrix(const matrix<T,M,N>& m);

      // column major
      const T& operator()(std::size_t x, std::size_t y) const
      {
         return data[y][x];
      }

      T& operator()(std::size_t x, std::size_t y)
      {
         return data[y][x];
      }

      const T& operator()(std::size_t i) const
      {
         return dptr[i];
      }

      T& operator()(std::size_t i)
      {
         return dptr[i];
      }

      const T& operator[](std::size_t i) const
      {
         return dptr[i];
      }

      T& operator[](std::size_t i)
      {
         return dptr[i];
      }

      matrix<T,M,N>& operator=(const matrix<T,M,N>& m);
      matrix<T,M,N>& operator+=(const T& value);
      matrix<T,M,N>& operator-=(const T& value);
      matrix<T,M,N>& operator*=(const T& value);
      matrix<T,M,N>& operator/=(const T& value);
      matrix<T,M,N>& operator+=(const matrix<T,M,N>& _matrix);
      matrix<T,M,N>& operator-=(const matrix<T,M,N>& _matrix);

      void zero();
      void identity();
      void swap(const unsigned int& x1,const unsigned int& y1,
                const unsigned int& x2,const unsigned int& y2);

      std::size_t size() const
      {
         return M * N;
      }

   private:
      T  data[M][N];
      T* dptr;
   };

   template <typename T> inline T det(const matrix<T,1,1>& matrix);
   template <typename T> inline T det(const matrix<T,2,2>& matrix);
   template <typename T> inline T det(const matrix<T,3,3>& matrix);
   template <typename T> inline T det(const matrix<T,4,4>& matrix);

   template <typename T> inline void transpose(matrix<T,1,1>& matrix);
   template <typename T> inline void transpose(matrix<T,2,2>& matrix);
   template <typename T> inline void transpose(matrix<T,3,3>& matrix);
   template <typename T> inline void transpose(matrix<T,4,4>& matrix);

   template <typename T> inline void inverse(matrix<T,2,2>& out_matrix, const matrix<T,2,2>& in_matrix);
   template <typename T> inline void inverse(matrix<T,3,3>& out_matrix, const matrix<T,3,3>& in_matrix);
   template <typename T> inline void inverse(matrix<T,4,4>& out_matrix, const matrix<T,4,4>& in_matrix);

   template <typename T, std::size_t N> inline void inverse(matrix<T,N,N>& out_matrix, const matrix<T,N,N>& in_matrix);

   template <typename T> inline void eigen_values(const matrix<T,2,2>& matrix, T& eigen_value1, T& eigen_value2);
   template <typename T> inline void eigenvector(const matrix<T,2,2>& matrix, vector2d<T>& eigenvector1, vector2d<T>& eigenvector2);

} // namespace wykobi

#include "wykobi_matrix.inl"


#endif
