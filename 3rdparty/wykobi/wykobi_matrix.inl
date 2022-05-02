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
#include "wykobi_matrix.hpp"

#include <algorithm>


namespace wykobi
{
   template <typename T, std::size_t M, std::size_t N>
   inline matrix<T,M,N>::matrix(const matrix<T,M,N>& m)
   : dptr(reinterpret_cast<T*>(&data))
   {
      std::copy(m.dptr,m.dptr + (M * N), dptr);
   }

   template <typename T, std::size_t M, std::size_t N>
   inline matrix<T,M,N>& matrix<T,M,N>::operator=(const matrix<T,M,N>& m)
   {
      if (this == &m) return *this;

      std::copy(m.dptr,m.dptr + (M * N), dptr);

      return *this;
   }

   template <typename T, std::size_t M, std::size_t N>
   inline matrix<T,M,N>& matrix<T,M,N>::operator+=(const T& value)
   {
      for (std::size_t i = 0; i < size(); ++i)
      {
         dptr[i] += value;
      }

      return (*this);
   }

   template <typename T, std::size_t M, std::size_t N>
   inline matrix<T,M,N>& matrix<T,M,N>::operator-=(const T& value)
   {
      for (std::size_t i = 0; i < size(); ++i)
      {
         dptr[i] -= value;
      }

      return (*this);
   }

   template <typename T, std::size_t M, std::size_t N>
   inline matrix<T,M,N>& matrix<T,M,N>::operator*=(const T& value)
   {
      for (std::size_t i = 0; i < size(); ++i)
      {
         dptr[i] *= value;
      }

      return (*this);
   }

   template <typename T, std::size_t M, std::size_t N>
   inline matrix<T,M,N>& matrix<T,M,N>::operator/=(const T& value)
   {
      for (std::size_t i = 0; i < size(); ++i)
      {
         dptr[i] /= value;
      }

      return (*this);
   }

   template <typename T, std::size_t M, std::size_t N>
   inline matrix<T,M,N>& matrix<T,M,N>::operator+=(const matrix<T,M,N>& _matrix)
   {
      for (std::size_t i = 0; i < size(); ++i)
      {
         dptr[i] += _matrix.dptr[i];
      }

      return (*this);
   }

   template <typename T, std::size_t M, std::size_t N>
   inline matrix<T,M,N>& matrix<T,M,N>::operator-=(const matrix<T,M,N>& _matrix)
   {
      for (std::size_t i = 0; i < size(); ++i)
      {
         dptr[i] -= _matrix.dptr[i];
      }

      return (*this);
   }

   template <typename T, std::size_t M, std::size_t N>
   inline void matrix<T,M,N>::zero()
   {
      for (std::size_t i = 0; i < size(); ++i)
      {
         dptr[i] = T(0.0);
      }
   }

   template <typename T, std::size_t M, std::size_t N>
   inline void matrix<T,M,N>::identity()
   {
      for (std::size_t x = 0; x < M; ++x)
      {
         for (std::size_t y = 0; y < N; ++y)
         {
            data[x][y] = ((x == y) ? T(1.0) : T(0.0));
         }
      }
   }

   template <typename T, std::size_t M, std::size_t N>
   inline void matrix<T,M,N>::swap(const unsigned int& x1,const unsigned int& y1,
                                   const unsigned int& x2,const unsigned int& y2)
   {
      T temp  = data[x1][y1];
      data[x1][y1] = data[x2][y2];
      data[x2][y2] = temp;
   }

   template <typename T>
   inline void transpose(matrix<T,1,1>&)
   {}

   template <typename T>
   inline void transpose(matrix<T,2,2>& matrix)
   {
      matrix.swap(0,1,1,0);
   }

   template <typename T> inline void transpose(matrix<T,3,3>& matrix)
   {
      matrix.swap(0,1,1,0);
      matrix.swap(0,2,2,0);
      matrix.swap(1,2,2,1);
   }

   template <typename T> inline void transpose(matrix<T,4,4>& matrix)
   {
      matrix.swap(0,1,1,0);
      matrix.swap(0,2,2,0);
      matrix.swap(0,3,3,0);
      matrix.swap(1,2,2,1);
      matrix.swap(1,3,3,1);
      matrix.swap(2,3,3,2);
   }

   template <typename T>
   inline T det(const matrix<T,1,1>& matrix)
   {
      return matrix(0,0);
   }

   template <typename T>
   inline T det(const matrix<T,2,2>& m)
   {
      return m[0] * m[3] - m[1] * m[2];
   }

   template <typename T>
   inline T det(const matrix<T,3,3>& m)
   {
      return (m(0,0) * (m(1,1) * m(2,2) - m(1,2) * m(2,1)) -
              m(1,0) * (m(0,1) * m(2,2) - m(0,2) * m(2,1)) +
              m(2,0) * (m(0,1) * m(1,2) - m(0,2) * m(1,1)));
   }

   template <typename T>
   inline T det(const matrix<T,4,4>& m)
   {
      T A0 = m[ 0] * m[ 5] - m[ 1] * m[ 4];
      T A1 = m[ 0] * m[ 6] - m[ 2] * m[ 4];
      T A2 = m[ 0] * m[ 7] - m[ 3] * m[ 4];
      T A3 = m[ 1] * m[ 6] - m[ 2] * m[ 5];
      T A4 = m[ 1] * m[ 7] - m[ 3] * m[ 5];
      T A5 = m[ 2] * m[ 7] - m[ 3] * m[ 6];
      T B0 = m[ 8] * m[13] - m[ 9] * m[12];
      T B1 = m[ 8] * m[14] - m[10] * m[12];
      T B2 = m[ 8] * m[15] - m[11] * m[12];
      T B3 = m[ 9] * m[14] - m[10] * m[13];
      T B4 = m[ 9] * m[15] - m[11] * m[13];
      T B5 = m[10] * m[15] - m[11] * m[14];

      return A0 * B5 - A1 * B4 + A2 * B3 + A3 * B2 - A4 * B1 + A5 * B0;
   }

   template <typename T>
   inline matrix<T,2,2> inverse(const matrix<T,2,2>& m)
   {
      T d = det(m);

      if (d != T(0.0))
      {
         matrix<T,2,2> m_;

         d = T(1.0) / d;

         m_(0,0)  = m(1,1) * d;
         m_(1,1)  = m(0,0) * d;
         m_(1,0) *= T(-1.0) * m(1,0) * d;
         m_(0,1) *= T(-1.0) * m(0,1) * d;

         return  m_;
      }
      else
         return matrix<T,2,2>();
   }

   template <typename T>
   inline matrix<T,3,3> inverse(const matrix<T,3,3>& m)
   {
      T d = det(m);

      if (d != T(0.0))
      {
         matrix<T,3,3> m_;

         d = T(1.0) / d;

         m_(0,0) = (m(1,1) * m(2,2) - m(1,2) * m(2,1))* d;
         m_(0,1) = (m(0,2) * m(2,1) - m(0,1) * m(2,2))* d;
         m_(0,2) = (m(0,1) * m(1,2) - m(0,2) * m(1,1))* d;

         m_(1,0) = (m(1,2) * m(2,0) - m(1,0) * m(2,2))* d;
         m_(1,1) = (m(0,0) * m(2,2) - m(0,2) * m(2,0))* d;
         m_(1,2) = (m(0,2) * m(1,0) - m(0,0) * m(1,2))* d;

         m_(2,0) = (m(1,0) * m(2,1) - m(1,1) * m(2,0))* d;
         m_(2,1) = (m(0,1) * m(2,0) - m(0,0) * m(2,1))* d;
         m_(2,2) = (m(0,0) * m(1,1) - m(0,1) * m(1,0))* d;

         return m_;
      }
      else
         return matrix<T,3,3>();
   }

   template <typename T>
   inline matrix<T,4,4> inverse(const matrix<T,4,4>& m)
   {
      T A0 = m[ 0] * m[ 5] - m[ 1] * m[ 4];
      T A1 = m[ 0] * m[ 6] - m[ 2] * m[ 4];
      T A2 = m[ 0] * m[ 7] - m[ 3] * m[ 4];
      T A3 = m[ 1] * m[ 6] - m[ 2] * m[ 5];
      T A4 = m[ 1] * m[ 7] - m[ 3] * m[ 5];
      T A5 = m[ 2] * m[ 7] - m[ 3] * m[ 6];
      T B0 = m[ 8] * m[13] - m[ 9] * m[12];
      T B1 = m[ 8] * m[14] - m[10] * m[12];
      T B2 = m[ 8] * m[15] - m[11] * m[12];
      T B3 = m[ 9] * m[14] - m[10] * m[13];
      T B4 = m[ 9] * m[15] - m[11] * m[13];
      T B5 = m[10] * m[15] - m[11] * m[14];

      T d = A0 * B5 - A1 * B4 + A2 * B3 + A3 * B2 - A4 * B1 + A5 * B0;

      if (not_equal(d, T(0.0)))
      {
         matrix<T,4,4> m_;

         d = T(1.0) / d;

         m_[ 0] = (+m[ 5] * B5 - m[ 6] * B4 + m[ 7] * B3) * d;
         m_[ 1] = (-m[ 1] * B5 + m[ 2] * B4 - m[ 3] * B3) * d;
         m_[ 2] = (+m[13] * A5 - m[14] * A4 + m[15] * A3) * d;
         m_[ 3] = (-m[ 9] * A5 + m[10] * A4 - m[11] * A3) * d;
         m_[ 4] = (-m[ 4] * B5 + m[ 6] * B2 - m[ 7] * B1) * d;
         m_[ 5] = (+m[ 0] * B5 - m[ 2] * B2 + m[ 3] * B1) * d;
         m_[ 6] = (-m[12] * A5 + m[14] * A2 - m[15] * A1) * d;
         m_[ 7] = (+m[ 8] * A5 - m[10] * A2 + m[11] * A1) * d;
         m_[ 8] = (+m[ 4] * B4 - m[ 5] * B2 + m[ 7] * B0) * d;
         m_[ 9] = (-m[ 0] * B4 + m[ 1] * B2 - m[ 3] * B0) * d;
         m_[10] = (+m[12] * A4 - m[13] * A2 + m[15] * A0) * d;
         m_[11] = (-m[ 8] * A4 + m[ 9] * A2 - m[11] * A0) * d;
         m_[12] = (-m[ 4] * B3 + m[ 5] * B1 - m[ 6] * B0) * d;
         m_[13] = (+m[ 0] * B3 - m[ 1] * B1 + m[ 2] * B0) * d;
         m_[14] = (-m[12] * A3 + m[13] * A1 - m[14] * A0) * d;
         m_[15] = (+m[ 8] * A3 - m[ 9] * A1 + m[10] * A0) * d;

         return m_;
      }
      else
         return matrix<T,4,4>();
   }

   template <typename T, std::size_t N>
   inline void inverse(matrix<T,N,N>& out_matrix, const matrix<T,N,N>& in_matrix)
   {
      out_matrix = inverse(in_matrix);
   }

   template <typename T>
   inline void eigenvalues(const matrix<T,2,2>& matrix, T& eigenvalue1, T& eigenvalue2)
   {
      T delta = sqrt<T>(sqr<T>(matrix(0,0) - matrix(1,1)) + T(4.0) * matrix(1,0) * matrix(0,1));

      eigenvalue1 = T(0.5) * (matrix(0,0) + matrix(1,1) + delta);
      eigenvalue2 = T(0.5) * (matrix(0,0) + matrix(1,1) - delta);
   }

   template <typename T>
   inline void eigenvector(const matrix<T,2,2>& matrix,
                                 vector2d<T>& eigenvector1,
                                 vector2d<T>& eigenvector2)
   {
      T eigenvalue1;
      T eigenvalue2;

      eigenvalues(matrix,eigenvalue1,eigenvalue2);

      eigenvector1 = normalize(make_vector(T(-1.0) * matrix(1,0), matrix(0,0) - eigenvalue1));
      eigenvector2 = normalize(make_vector(T(-1.0) * matrix(1,0), matrix(0,0) - eigenvalue2));
   }

} // namespace wykobi
