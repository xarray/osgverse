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


#ifndef INCLUDE_WYKOBI
#define INCLUDE_WYKOBI


#include <cstddef>
#include <limits>
#include <algorithm>
#include <iterator>
#include <vector>
#include <cassert>

#include "wykobi_math.hpp"


namespace wykobi
{

   static const char VERSION_INFORMATION[] = "Wykobi Version 0.0.5";
   static const char AUTHOR_INFORMATION[]  = "Arash Partow";
   static const char EPOCH_VERSION[]       = "C578AC5A:35A4123B:DF32F721";

   #ifndef WYKOBI
    #define WYKOBI
   #endif


   /****************************************************************************/
   /********************[ Basic Geometric Structure Types ]*********************/
   /****************************************************************************/

   /************[ Geometric Entity ]*************/
    class geometric_entity{};

    enum geometric_type  {
                          ePoint2D,
                          ePoint3D,
                          eSegment2D,
                          eSegment3D,
                          eRectangle,
                          eBox,
                          eLine2D,
                          eLine3D,
                          eTriangle2D,
                          eTriangle3D,
                          eQuadix2D,
                          eQuadix3D,
                          eRay2D,
                          eRay3D,
                          eCircle,
                          eSphere
                         };


   /**************[ Vertex type ]***************/

   template <typename T, std::size_t D>
   class pointnd;

   template <typename T = Float>
   class point2d : public geometric_entity
   {
   public:

      typedef T           type;
      typedef const type& const_reference;
      typedef       type& reference;

      point2d() : x(T(0.0)), y(T(0.0)){}
      point2d(const pointnd<T,2>& point) : x(point[0]), y(point[1]){}
     ~point2d(){}

      inline point2d<T>& operator=(const pointnd<T,2>& point)
      {
         x = point[0];
         y = point[1];
         return *this;
      }

      inline reference       operator()(const std::size_t& index)       { return ((0 == index)? x : y); }
      inline const_reference operator()(const std::size_t& index) const { return ((0 == index)? x : y); }

      inline reference       operator[](const std::size_t& index)       { return ((0 == index)? x : y); }
      inline const_reference operator[](const std::size_t& index) const { return ((0 == index)? x : y); }

      T x,y;
   };

   template <typename T = Float>
   class point3d : public geometric_entity
   {
   public:

      typedef T           Type;
      typedef const Type& const_reference;
      typedef       Type& reference;

      point3d() : x(T(0.0)), y(T(0.0)), z(T(0.0)){}
      point3d(const pointnd<T,3>& point) : x(point[0]), y(point[1]), z(point[2]){}
     ~point3d(){}

      inline point3d<T>& operator=(const pointnd<T,3>& point)
      {
         x = point[0];
         y = point[1];
         z = point[2];
         return *this;
      }

      inline reference       operator()(const std::size_t& index)       { return value(index); }
      inline const_reference operator()(const std::size_t& index) const { return value(index); }

      inline reference       operator[](const std::size_t& index)       { return value(index); }
      inline const_reference operator[](const std::size_t& index) const { return value(index); }

      T x,y,z;
   private:
      inline reference value(const std::size_t& index)
      {
         switch(index)
         {
            case 0  : return x;
            case 1  : return y;
            case 2  : return z;
            default : return x;
         }
      }

      inline const_reference value(const std::size_t& index) const
      {
         switch(index)
         {
            case 0  : return x;
            case 1  : return y;
            case 2  : return z;
            default : return x;
         }
      }
   };

   template <typename T, std::size_t D>
   class pointnd : public geometric_entity
   {
   public:

      typedef const T& const_reference;
      typedef       T& reference;

      pointnd(){ clear(); }
      pointnd(const T& v0) { v[0] = v0; }
      pointnd(const T& v0, const T& v1) { v[0] = v0; v[1] = v1; }
      pointnd(const T& v0,const T& v1, const T& v2) { v[0] = v0; v[1] = v1; v[2] = v2; }
      pointnd(const T& v0,const T& v1, const T& v2, const T& v3) { v[0] = v0; v[1] = v1; v[2] = v2; v[3] = v3; }
      pointnd(const pointnd<T,D>& point)
      {
         for (std::size_t i = 0; i < D; ++i) v[i] = point.v[i];
      }

      pointnd(const point2d<T>& point)
      {
         for (std::size_t i = 0; i < D; ++i) v[i] = point[i];
      }

      pointnd(const point3d<T>& point)
      {
         for (std::size_t i = 0; i < D; ++i) v[i] = point[i];
      }

     ~pointnd(){}

      void clear()
      {
         for (std::size_t i = 0; i < D; ++i) v[i] = T(0.0);
      }

      inline pointnd<T,D>& operator=(const pointnd<T,D>& point)
      {
         if (this == &point) return *this;
         for (std::size_t i = 0; i < D; ++i) v[i] = point.v[i];
         return *this;
      }

      inline pointnd<T,D>& operator=(const point2d<T>& point)
      {
         if (D == 2)
         {
            v[0] = point.x;
            v[1] = point.y;
         }
         return *this;
      }

      inline pointnd<T,D>& operator=(const point3d<T>& point)
      {
         if (D == 3)
         {
            v[0] = point.x;
            v[1] = point.y;
            v[2] = point.z;
         }
         return *this;
      }

      inline reference       operator()(const std::size_t& index)       { return v[index]; }
      inline const_reference operator()(const std::size_t& index) const { return v[index]; }

      inline reference       operator[](const std::size_t& index)       { return v[index]; }
      inline const_reference operator[](const std::size_t& index) const { return v[index]; }

   protected:
      T v[D];
   };

   template <typename T, std::size_t Dimension>
   class define_point_type      { public: typedef pointnd<T,Dimension> PointType; };

   template <typename T>
   class define_point_type<T,2> { public: typedef point2d<T> PointType; };

   template <typename T>
   class define_point_type<T,3> { public: typedef point3d<T> PointType; };


   /************[      Segment Type     ]************/
   template <typename T, std::size_t Dimension>
   class segment : public geometric_entity
   {
   public:

      const static std::size_t PointCount = 2;

      segment(){}
     ~segment(){}

      typedef typename define_point_type<T,Dimension>::PointType PointType;
      typedef const PointType& const_reference;
      typedef       PointType& reference;

   private:

      PointType _data[PointCount];

   public:

      inline reference       operator [](const std::size_t& index)       { return _data[index]; }
      inline const_reference operator [](const std::size_t& index) const { return _data[index]; }
      inline std::size_t     size       ()                               { return PointCount;   }
   };


   /************[       Line Type       ]************/
   template <typename T, std::size_t Dimension>
   class line : public geometric_entity
   {
   public:

      const static std::size_t PointCount = 2;

      line(){}
     ~line(){}

      typedef typename define_point_type<T,Dimension>::PointType PointType;
      typedef const PointType& const_reference;
      typedef       PointType& reference;

   private:

      PointType _data[PointCount];

   public:

      inline reference       operator [](const std::size_t& index)       { return _data[index]; }
      inline const_reference operator [](const std::size_t& index) const { return _data[index]; }
      inline std::size_t     size       ()                               { return PointCount;   }
   };


   /************[     Triangle Type     ]************/
   template <typename T, std::size_t Dimension>
   class triangle : public geometric_entity
   {
   public:

      const static std::size_t PointCount = 3;

      triangle(){}
     ~triangle(){}

      typedef typename define_point_type<T,Dimension>::PointType PointType;
      typedef const PointType& const_reference;
      typedef       PointType& reference;

   private:
      PointType _data[PointCount];

   public:
      inline reference       operator [](const std::size_t& index)       { return _data[index]; }
      inline const_reference operator [](const std::size_t& index) const { return _data[index]; }
      inline std::size_t     size       ()                         const { return PointCount;   }
   };


   /************[       Rectangle       ]************/
   template <typename T>
   class rectangle : public geometric_entity
   {
   public:

      const static std::size_t PointCount = 2;

      rectangle(){}
     ~rectangle(){}

      typedef typename define_point_type<T,2>::PointType PointType;
      typedef const PointType& const_reference;
      typedef       PointType& reference;

   private:

      PointType _data[PointCount];

   public:

      inline reference       operator [](const std::size_t& index)       { return _data[index]; }
      inline const_reference operator [](const std::size_t& index) const { return _data[index]; }
      inline std::size_t     size       ()                         const { return PointCount;   }
   };


   /************[      Quadix Type      ]************/
   template <typename T, std::size_t Dimension>
   class quadix : public geometric_entity
   {
   public:

      const static std::size_t PointCount = 4;

      quadix(){}
     ~quadix(){}

      typedef typename define_point_type<T,Dimension>::PointType PointType;
      typedef const PointType& const_reference;
      typedef       PointType& reference;

   private:

      PointType _data[PointCount];

   public:

      inline reference       operator [](const std::size_t& index)       { return _data[index]; }
      inline const_reference operator [](const std::size_t& index) const { return _data[index]; }
      inline std::size_t     size       ()                         const { return PointCount;   }
   };

   /************[     Polygon Type      ]************/
   template <typename T, std::size_t Dimension>
   class polygon : public geometric_entity
   {
   public:

      polygon(const std::size_t initial_size = 0) : _data(initial_size){}
     ~polygon(){}

      typedef typename define_point_type<T,Dimension>::PointType PointType;
      typedef const PointType& const_reference;
      typedef PointType& reference;

   private:

      std::vector<PointType> _data;

   public:

      typedef typename std::vector<PointType>::iterator iterator;
      typedef typename std::vector<PointType>::const_iterator const_iterator;
      typedef PointType value_type;

   public:

      inline reference       operator [](const std::size_t& index)       { return _data[index];                }
      inline const_reference operator [](const std::size_t& index) const { return _data[index];                }
      inline void            push_back  (const PointType&  value)        { _data.push_back(value);             }
      inline void            reserve    (const std::size_t amount)       { _data.reserve(amount);              }
      inline void            clear      ()                         const { _data.clear();                      }
      inline void            clear      ()                               { _data.clear();                      }
      inline void            erase      (const std::size_t index)        { _data.erase(_data.begin() + index); }
      inline std::size_t     size       ()                         const { return _data.size();                }
      inline const_iterator  begin      ()                         const { return _data.begin();               }
      inline iterator        begin      ()                               { return _data.begin();               }
      inline const_iterator  end        ()                         const { return _data.end();                 }
      inline iterator        end        ()                               { return _data.end();                 }
      inline reference       front      ()                               { return _data.front();               }
      inline const_reference front      ()                         const { return _data.front();               }
      inline reference       back       ()                               { return _data.back();                }
      inline const_reference back       ()                         const { return _data.back();                }
      inline void            reverse    ()                               { std::reverse(_data.begin(),_data.end());}
   };

   /************[      Circle Type      ]************/
   template <typename T>
   class circle : public geometric_entity { public: T x,y,radius; };


   /************[      Sphere Type      ]************/
   template <typename T>
   class sphere : public geometric_entity { public: T x,y,z,radius; };

   /************[   Hypersphere Type    ]************/
   template <typename T, std::size_t Dimension>
   class hypersphere : public geometric_entity
   {
   public:

      typedef typename define_point_type<T,Dimension>::PointType PointType;
      typedef const PointType& const_reference;
      typedef PointType& reference;

      PointType center;
      T radius;
   };

   /************[  CircularArc Type   ]**************/
   template <typename T>
   class circular_arc : public geometric_entity
   {
   public:

      T   x1,y1;
      T   x2,y2;
      T   cx,cy;
      T   px,py;
      T   angle1;
      T   angle2;
      int orientation;
   };

   /************[      Bezier Type      ]************/
   enum BezierType {
                    eQuadraticBezier = 2,
                    eCubicBezier     = 3
                   };

   /************[ Quadratic Bezier Type ]************/
   template <typename T, std::size_t Dimension>
   class quadratic_bezier : public geometric_entity
   {
   public:

      const static std::size_t PointCount = 3;
      const static BezierType  Type       = eQuadraticBezier;

      quadratic_bezier(){}
     ~quadratic_bezier(){}

      typedef typename define_point_type<T,Dimension>::PointType PointType;
      typedef const PointType& const_reference;
      typedef       PointType& reference;

   private:

      PointType _data[PointCount];

   public:

      inline reference       operator [](const std::size_t& index)       { return _data[index]; }
      inline const_reference operator [](const std::size_t& index) const { return _data[index]; }
      inline std::size_t     size     ()                           const { return PointCount;   }
   };


   /************[   Cubic Bezier Type   ]************/
   template <typename T, std::size_t Dimension>
   class cubic_bezier : public geometric_entity
   {
   public:

      const static std::size_t PointCount = 4;
      const static BezierType  Type       = eCubicBezier;

      cubic_bezier(){}
     ~cubic_bezier(){}

      typedef typename define_point_type<T,Dimension>::PointType PointType;
      typedef const PointType& const_reference;
      typedef       PointType& reference;

   private:

      PointType _data[PointCount];

   public:

      inline reference       operator [](const std::size_t& index)       { return _data[index]; }
      inline const_reference operator [](const std::size_t& index) const { return _data[index]; }
      inline std::size_t     size     ()                           const { return PointCount;   }
   };

   template <typename T, unsigned int Dimension, BezierType BType>
   class define_bezier_type;

   template <typename T>
   class define_bezier_type<T,2,eQuadraticBezier> { public: typedef quadratic_bezier<T,2> BezierType; };

   template <typename T>
   class define_bezier_type<T,3,eQuadraticBezier> { public: typedef quadratic_bezier<T,3> BezierType; };

   template <typename T>
   class define_bezier_type<T,2,eCubicBezier>     { public: typedef cubic_bezier<T,2> BezierType;     };

   template <typename T>
   class define_bezier_type<T,3,eCubicBezier>     { public: typedef cubic_bezier<T,3> BezierType;     };


   /************[  Bezier Coefficients  ]************/
   template <typename T, unsigned int Dimension, BezierType Type>
   struct bezier_coefficients
   {
      typedef typename define_point_type<T,Dimension>::PointType PointType;
      typedef const PointType& const_reference;
      typedef       PointType& reference;

      PointType value[Type];
   };


   /************[    Curve Point Type   ]************/
   template <typename T, std::size_t Dimension>
   class curve_point : public geometric_entity
   {
   public:

      curve_point(){}
     ~curve_point(){}

      const static std::size_t PointCount = 1;
      typedef typename define_point_type<T,Dimension>::PointType PointType;
      typedef const PointType& const_reference;
      typedef       PointType& reference;

   private:

      PointType _data[PointCount];

   public:

      inline reference       operator ()()       { return _data[0];     }
      inline const_reference operator ()() const { return _data[0];     }
      inline std::size_t     size     ()   const { return PointCount;   }

      T t;
   };


   /************[      Vector Type      ]************/

   template <typename T, std::size_t D>
   class vectornd;

   template <typename T>
   class vector2d : public point2d<T>
   {
   public:

      vector2d(const T& _x = T(0.0), const T& _y = T(0.0))
      {
         point2d<T>::x = _x;
         point2d<T>::y = _y;
      }

      inline vector2d<T>& operator=(const vectornd<T,2>& vec)
      {
         point2d<T>::x = vec[0];
         point2d<T>::y = vec[1];
         return *this;
      }
   };

   template <typename T>
   class vector3d : public point3d<T>
   {
   public:

      vector3d(const T& _x = T(0.0), const T& _y = T(0.0), const T& _z = T(0.0))
      {
         point3d<T>::x = _x;
         point3d<T>::y = _y;
         point3d<T>::z = _z;
      }

      inline vector3d<T>& operator=(const vectornd<T,3>& vec)
      {
         point3d<T>::x = vec[0];
         point3d<T>::y = vec[1];
         point3d<T>::z = vec[2];
         return *this;
      }
   };

   template <typename T, std::size_t D>
   class vectornd : public pointnd<T,D>
   {
   public:

      vectornd()
      {
         pointnd<T,D>::clear();
      }

      vectornd(const T& v0)
      {
         pointnd<T,D>::v[0] = v0;
      }

      vectornd(const T& v0, const T& v1)
      {
         pointnd<T,D>::v[0] = v0;
         pointnd<T,D>::v[1] = v1;
      }

      vectornd(const T& v0,const T& v1, const T& v2)
      {
         pointnd<T,D>::v[0] = v0;
         pointnd<T,D>::v[1] = v1;
         pointnd<T,D>::v[2] = v2;
      }

      vectornd(const T& v0,const T& v1, const T& v2, const T& v3)
      {
         pointnd<T,D>::v[0] = v0;
         pointnd<T,D>::v[1] = v1;
         pointnd<T,D>::v[2] = v2;
         pointnd<T,D>::v[3] = v3;
      }

      vectornd(const vectornd<T,D>& vec)
      : pointnd<T,D>()
      {
         for (std::size_t i = 0; i < D; ++i) (*this)[i] = vec[i];
      }

      vectornd(const vector2d<T>& vec)
      {
         (*this)[0] = vec.x;
         (*this)[1] = vec.y;
      }

      vectornd(const vector3d<T>& vec)
      {
         (*this)[0] = vec.x;
         (*this)[1] = vec.y;
         (*this)[2] = vec.z;
      }
   };

   template <typename T, std::size_t Dimension>
   class define_vector_type { public: typedef vectornd<T,Dimension> VectorType; };

   template <typename T>
   class define_vector_type<T,2> { public: typedef vector2d<T> VectorType; };

   template <typename T>
   class define_vector_type<T,3> { public: typedef vector3d<T> VectorType; };

   /************[        Ray Type       ]************/
   template <typename T, std::size_t Dimension>
   class ray : public geometric_entity
   {
   public:

      ray(){}
     ~ray(){}

     typedef typename define_point_type<T,Dimension>::PointType   PointType;
     typedef typename define_vector_type<T,Dimension>::VectorType VectorType;

      PointType  origin;
      VectorType direction;
   };

   /************[       Plane Type      ]************/
   template <typename T,std::size_t Dimension>
   class plane : public geometric_entity
   {
   public:

      plane(){}
     ~plane(){}

     typedef typename define_point_type<T,Dimension>::PointType   PointType;
     typedef typename define_vector_type<T,Dimension>::VectorType VectorType;

      T          constant;
      VectorType normal;
   };

   /************[        Box Type       ]************/
   template <typename T, std::size_t Dimension>
   class box : public geometric_entity
   {
   public:

      const static std::size_t PointCount = 2;

      box(){}
     ~box(){}

      typedef typename define_point_type<T,Dimension>::PointType PointType;
      typedef const PointType& const_reference;
      typedef       PointType& reference;

   private:

      PointType _data[PointCount];

   public:

      inline reference       operator [](const std::size_t& index)       { return _data[index]; }
      inline const_reference operator [](const std::size_t& index) const { return _data[index]; }
      inline std::size_t     size     ()                           const { return PointCount;   }
   };

   enum eInclusion {
                    eFully,
                    ePartially,
                    eOutside,
                    eUnknown
                   };

   enum eTriangleType {
                       etEquilateral,
                       etIsosceles,
                       etRight,
                       etScalene,
                       etObtuse,
                       etUnknown
                      };


   /**********[ Orientation constants ]**********/
   const int RightHandSide        = -1;
   const int LeftHandSide         = +1;
   const int Clockwise            = -1;
   const int CounterClockwise     = +1;
   const int CollinearOrientation =  0;
   const int AboveOrientation     = +1;
   const int BelowOrientation     = -1;
   const int CoplanarOrientation  =  0;
   const int PointInside          = +1;
   const int PointOutside         = -1;
   const int Cocircular           =  0;
   const int Cospherical          =  0;

   /********[       Clipping Codes        ]********/
   const int CLIP_BOTTOM = 1;
   const int CLIP_TOP    = 2;
   const int CLIP_LEFT   = 4;
   const int CLIP_RIGHT  = 8;

   /************[ Trigonometry Tables ]************/
   template <typename T>
   class trig_luts
   {
   public:

      const static unsigned int TableSize = 360;

      trig_luts()
      {
         for (std::size_t i = 0; i < 360; ++i)
         {
            sin_[i] = T(std::sin((1.0 * i) * PIDiv180));
            cos_[i] = T(std::cos((1.0 * i) * PIDiv180));
            tan_[i] = T(std::tan((1.0 * i) * PIDiv180));
         }
      }

      inline const T& sin(const unsigned int angle) const { return sin_[angle]; }
      inline const T& cos(const unsigned int angle) const { return cos_[angle]; }
      inline const T& tan(const unsigned int angle) const { return tan_[angle]; }

   private:

      std::vector<T> sin_;
      std::vector<T> cos_;
      std::vector<T> tan_;
   };

   /************[ General Definitions ]************/
   typedef segment <Float,2> segment2d;
   typedef line    <Float,2> line2d;
   typedef triangle<Float,2> triangle2d;
   typedef quadix  <Float,2> quadix2d;

   typedef segment <Float,3> segment3d;
   typedef line    <Float,3> line3d;
   typedef triangle<Float,3> triangle3d;
   typedef quadix  <Float,3> quadix3d;

   template <typename T> T epsilon();
   template<> inline double epsilon<double>() { return static_cast<double>(Epsilon_Medium); }
   template<> inline  float epsilon<float> () { return static_cast<float> (Epsilon_Low   ); }

   template <typename T>
   inline int orientation(const T& x1, const T& y1,
                          const T& x2, const T& y2,
                          const T& px, const T& py);

   template <typename T>
   inline int orientation(const T& x1, const T& y1, const T& z1,
                          const T& x2, const T& y2, const T& z2,
                          const T& x3, const T& y3, const T& z3,
                          const T& px, const T& py, const T& pz);

   template <typename T>
   inline int robust_orientation(const T& x1, const T& y1,
                                 const T& x2, const T& y2,
                                 const T& px, const T& py);

   template <typename T>
   inline int robust_orientation(const T& x1, const T& y1, const T& z1,
                                 const T& x2, const T& y2, const T& z2,
                                 const T& x3, const T& y3, const T& z3,
                                 const T& px, const T& py, const T& pz);

   template <typename T>
   inline int orientation(const point2d<T>& point1, const point2d<T>& point2, const T& px, const T& py);

   template <typename T>
   inline int orientation(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);

   template <typename T>
   inline int orientation(const line<T,2>& line, const point2d<T>& point);

   template <typename T>
   inline int orientation(const segment<T,2>& segment, const point2d<T>& point);

   template <typename T>
   inline int orientation(const triangle<T,2>& triangle);

   template <typename T>
   inline int orientation(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const T& px, const T& py, const T& pz);

   template <typename T>
   inline int orientation(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const point3d<T>& point4);

   template <typename T>
   inline int orientation(const triangle<T,3>& triangle, const point3d<T>& point);

   template <typename T>
   inline bool differing_orientation(const T& x1,  const T& y1,
                                     const T& x2,  const T& y2,
                                     const T& p1x, const T& p1y,
                                     const T& p2x, const T& p2y);

   template <typename T>
   inline bool differing_orientation(const point2d<T>& p1, const point2d<T>& p2,
                                     const point2d<T>& q1, const point2d<T>& q2);

   template <typename T>
   inline int in_circle(const T& x1, const T& y1,
                        const T& x2, const T& y2,
                        const T& x3, const T& y3,
                        const T& px, const T& py);

   template <typename T>
   inline int in_circle(const point2d<T>& point1,
                        const point2d<T>& point2,
                        const point2d<T>& point3,
                        const point2d<T>& point4);

   template <typename T>
   inline int in_circle(const triangle<T,2>& triangle, const point2d<T>& point);

   template <typename T>
   inline int in_sphere(const T& x1, const T& y1, const T& z1,
                        const T& x2, const T& y2, const T& z2,
                        const T& x3, const T& y3, const T& z3,
                        const T& x4, const T& y4, const T& z4,
                        const T& px, const T& py, const T& pz);

   template <typename T>
   inline int in_sphere(const point3d<T>& point1,
                        const point3d<T>& point2,
                        const point3d<T>& point3,
                        const point3d<T>& point4,
                        const point2d<T>& point5);

   template <typename T>
   inline int in_sphere(const quadix<T,3>& quadix, const point3d<T>& point);

   template <typename T>
   inline T signed_area(const T& x1, const T& y1,
                        const T& x2, const T& y2,
                        const T& px, const T& py);

   template <typename T>
   inline T signed_area(const point2d<T>& point1, const point2d<T>& point2, const T& px, const T& py);

   template <typename T>
   inline T signed_area(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);

   template <typename T>
   inline T signed_area(const segment<T,2>& segment, const point2d<T>& point);

   template <typename T>
   inline T signed_volume(const T& x1, const T& y1, const T& z1,
                          const T& x2, const T& y2, const T& z2,
                          const T& x3, const T& y3, const T& z3,
                          const T& px, const T& py, const T& pz);

   template <typename T>
   inline T signed_volume(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const T& px, const T& py, const T& pz);

   template <typename T>
   inline T signed_volume(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const point3d<T>& point4);

   template <typename T>
   inline T signed_volume(const triangle<T,3>& triangle, const point3d<T>& point);


   template <typename T>
   inline bool collinear(const T& x1, const T& y1,
                         const T& x2, const T& y2,
                         const T& x3, const T& y3,
                         const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool collinear(const T& x1, const T& y1, const T& z1,
                         const T& x2, const T& y2, const T& z2,
                         const T& x3, const T& y3, const T& z3,
                         const T& epsilon = T(Epsilon));


   template <typename T>
   inline bool collinear(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);

   template <typename T>
   inline bool collinear(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3);

   template <typename T>
   inline bool robust_collinear(const T& x1, const T& y1,
                                const T& x2, const T& y2,
                                const T& x3, const T& y3, const T& epsilon = T(Epsilon));
   template <typename T>
   inline bool robust_collinear(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3, const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool robust_collinear(const line<T,2>& line, const point2d<T>& point, const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool robust_collinear(const line<T,3>& line, const point3d<T>& point, const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool robust_collinear(const T& x1, const T& y1, const T& z1,
                                const T& x2, const T& y2, const T& z2,
                                const T& x3, const T& y3, const T& z3, const T& epsilon = T(Epsilon));
   template <typename T>
   inline bool robust_collinear(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool is_point_collinear(const T& x1, const T& y1,
                                  const T& x2, const T& y2,
                                  const T& px, const T& py,
                                  const bool robust = false);
   template <typename T>
   inline bool is_point_collinear(const point2d<T>& point1,
                                  const point2d<T>& point2,
                                  const point2d<T>& point3,
                                  const bool robust = false);

   template <typename T>
   inline bool is_point_collinear(const point2d<T>& point1,
                                  const point2d<T>& point2,
                                  const T& px, const T& py,
                                  const bool robust = false);
   template <typename T>
   inline bool is_point_collinear(const segment<T,2>& segment,
                                  const point2d<T>&   point,
                                  const bool robust = false);

   template <typename T>
   inline bool is_point_collinear(const T& x1, const T& y1, const T& z1,
                                  const T& x2, const T& y2, const T& z2,
                                  const T& px, const T& py, const T& pz,
                                  const bool robust = false);

   template <typename T>
   inline bool is_point_collinear(const point3d<T>& point1,
                                  const point3d<T>& point2,
                                  const point3d<T>& point3,
                                  const bool robust = false);

   template <typename T>
   inline bool is_point_collinear(const segment<T,3>& segment,
                                  const point3d<T>& point,
                                  const bool robust = false);

   template <typename T> inline bool robust_coplanar(const point3d<T> point1,
                                                     const point3d<T> point2,
                                                     const point3d<T> point3,
                                                     const point3d<T> point4,
                                                     const T& epsilon = T(Epsilon));

   template <typename T> inline bool coplanar(const ray<T,3>& ray1, const ray<T,3>& ray2);
   template <typename T> inline bool coplanar(const segment<T,3>& segment1, const segment<T,3>& segment2);
   template <typename T> inline bool coplanar(const line<T,3>& line1, const line<T,3>& line2);
   template <typename T> inline bool coplanar(const triangle<T,3>& triangle1, const triangle<T,3>& triangle2);
   template <typename T> inline bool coplanar(const quadix<T,3>& quadix1, const quadix<T,3>& quadix2);

   template <typename T> inline bool cocircular(const T& x1, const T& y1,
                                                const T& x2, const T& y2,
                                                const T& x3, const T& y3,
                                                const T& x4, const T& y4,
                                                const T& epsilon = T(Epsilon));

   template <typename T> inline bool cocircular(const point2d<T>& point1,
                                                const point2d<T>& point2,
                                                const point2d<T>& point3,
                                                const point2d<T>& point4,
                                                const T& epsilon = T(Epsilon));

   template <typename T> inline bool cocircular(const triangle<T,2>& triangle,
                                                const point2d<T>& point,
                                                const T& epsilon = T(Epsilon));

   template <typename T> inline bool cocircular(const circle<T>& circle,
                                                const point2d<T>& point,
                                                const T& epsilon = T(Epsilon));

   template <typename T> inline bool is_skinny_triangle(const T& x1, const T& y1,
                                                        const T& x2, const T& y2,
                                                        const T& x3, const T& y3);

   template <typename T> inline bool is_skinny_triangle(const point2d<T>& point1,
                                                        const point2d<T>& point2,
                                                        const point2d<T>& point3);

   template <typename T> inline bool is_skinny_triangle(const triangle<T,2>& triangle);

   template <typename T>
   inline bool intersect(const T& x1, const T& y1,
                         const T& x2, const T& y2,
                         const T& x3, const T& y3,
                         const T& x4, const T& y4);

   template <typename T>
   inline bool intersect(const T& x1, const T& y1,
                         const T& x2, const T& y2,
                         const T& x3, const T& y3,
                         const T& x4, const T& y4,
                              T& ix,      T& iy);
   template <typename T>
   inline bool intersect(const point2d<T>& point1,
                         const point2d<T>& point2,
                         const point2d<T>& point3,
                         const point2d<T>& point4);

   template <typename T>
   inline bool intersect(const point2d<T>& point1,
                         const point2d<T>& point2,
                         const point2d<T>& point3,
                         const point2d<T>& point4,
                               point2d<T>& int_point);

   template <typename T>
   inline bool intersect(const segment<T,2>& segment1, const segment<T,2>& segment2);

   template <typename T>
   inline bool intersect(const segment<T,2>& segment1, const segment<T,2>& segment2,T& ix, T& iy);

   template <typename T>
   inline bool intersect(const segment<T,2>& segment1, const segment<T,2>& segment2,point2d<T>& i_point);

   template <typename T>
   inline bool intersect(const T& x1, const T& y1, const T& z1,
                         const T& x2, const T& y2, const T& z2,
                         const T& x3, const T& y3, const T& z3,
                         const T& x4, const T& y4, const T& z4,
                         const T& fuzzy = T(0.0));

   template <typename T>
   inline bool intersect(const point3d<T>& point1,
                         const point3d<T>& point2,
                         const point3d<T>& point3,
                         const point3d<T>& point4,
                         const T& fuzzy = T(0.0));

   template <typename T> inline bool intersect(const segment<T,3>& segment1, const segment<T,3>&  segment2, const T& fuzzy = T(0.0));
   template <typename T> inline bool intersect(const segment<T,2>& segment, const rectangle<T>& rectangle);
   template <typename T> inline bool intersect(const segment<T,2>& segment, const triangle<T,2>& triangle);
   template <typename T> inline bool intersect(const segment<T,2>& segment, const quadix<T,2>& quadix);
   template <typename T> inline bool intersect(const segment<T,2>& segment, const line<T,2>& line);
   template <typename T> inline bool intersect(const segment<T,2>& segment, const circle<T>& circle);
   template <typename T> inline bool intersect(const segment<T,2>& segment, const quadratic_bezier<T,2>& bezier, const std::size_t& steps = 1000);
   template <typename T> inline bool intersect(const segment<T,2>& segment, const cubic_bezier<T,2>& bezier, const std::size_t& steps = 1000);
   template <typename T> inline bool intersect(const segment<T,3>& segment, const line<T,3>& line, const T& fuzzy = T(0.0));
   template <typename T> inline bool intersect(const segment<T,3>& segment, const box<T,3>& box);
   template <typename T> inline bool intersect(const segment<T,3>& segment, const sphere<T>& sphere);
   template <typename T> inline bool intersect(const segment<T,3>& segment, const plane<T,3>& plane);
   template <typename T> inline bool intersect(const segment<T,3>& segment, const quadratic_bezier<T,3>& bezier, const std::size_t& steps = 1000);
   template <typename T> inline bool intersect(const segment<T,3>& segment, const cubic_bezier<T,3>& bezier, const std::size_t& steps = 1000);
   template <typename T> inline bool intersect(const line<T,2>& line, const triangle<T,2>& triangle);
   template <typename T> inline bool intersect(const line<T,2>& line, const quadix<T,2>& quadix);
   template <typename T> inline bool intersect(const line<T,2>& line1, const line<T,2>& line2);
   template <typename T> inline bool intersect(const line<T,2>& line, const circle<T>& circle);
   template <typename T> inline bool intersect(const line<T,2>& line, const quadratic_bezier<T,2>& bezier, const std::size_t& steps = 1000);
   template <typename T> inline bool intersect(const line<T,2>& line, const cubic_bezier<T,2>& bezier, const std::size_t& steps = 1000);
   template <typename T> inline bool intersect(const line<T,3>& line, const triangle<T,3>& triangle);
   template <typename T> inline bool intersect(const line<T,3>& line, const plane<T,3>& plane);
   template <typename T> inline bool intersect(const line<T,3>& line, const sphere<T>& sphere);
   template <typename T> inline bool intersect(const line<T,3>& line, const quadratic_bezier<T,3>& bezier, const std::size_t& steps = 1000);
   template <typename T> inline bool intersect(const line<T,3>& line, const cubic_bezier<T,3>& bezier, const std::size_t& steps = 1000);
   template <typename T> inline bool intersect(const triangle<T,2>& triangle, const circle<T>& circle);
   template <typename T> inline bool intersect(const triangle<T,2>& triangle, const rectangle<T>& rectangle);
   template <typename T> inline bool intersect(const triangle<T,2>& triangle, const quadratic_bezier<T,2>& bezier, const std::size_t& steps = 1000);
   template <typename T> inline bool intersect(const triangle<T,2>& triangle, const cubic_bezier<T,2>& bezier, const std::size_t& steps = 1000);
   template <typename T> inline bool intersect(const rectangle<T>& rectangle1, const rectangle<T>& rectangle2);
   template <typename T> inline bool intersect(const triangle<T,2>& triangle1, const triangle<T,2>& triangle2);
   template <typename T> inline bool intersect(const rectangle<T>& rectangle, const circle<T>& circle);
   template <typename T> inline bool intersect(const rectangle<T>& rectangle, const quadratic_bezier<T,2>& bezier, const std::size_t& steps = 1000);
   template <typename T> inline bool intersect(const rectangle<T>& rectangle, const cubic_bezier<T,2>& bezier, const std::size_t& steps = 1000);
   template <typename T> inline bool intersect(const quadix<T,2>& quadix, const quadratic_bezier<T,2>& bezier, const std::size_t& steps = 1000);
   template <typename T> inline bool intersect(const quadix<T,2>& quadix, const cubic_bezier<T,2>& bezier, const std::size_t& steps = 1000);
   template <typename T> inline bool intersect(const circle<T>& circle1, const circle<T>& circle2);
   template <typename T> inline bool intersect(const circle<T>& circle, const quadratic_bezier<T,2>& bezier, const std::size_t& steps = 1000);
   template <typename T> inline bool intersect(const circle<T>& circle, const cubic_bezier<T,2>& bezier, const std::size_t& steps = 1000);
   template <typename T> inline bool intersect(const box<T,3>& box, const sphere<T>& sphere);
   template <typename T> inline bool intersect(const sphere<T>& sphere1, const sphere<T>& sphere2);
   template <typename T> inline bool intersect(const sphere<T>& sphere, const quadratic_bezier<T,3>& bezier, const std::size_t& steps = 1000);
   template <typename T> inline bool intersect(const sphere<T>& sphere, const cubic_bezier<T,3>& bezier, const std::size_t& steps = 1000);
   template <typename T> inline bool intersect(const ray<T,2>& ray1, const ray<T,2>& ray2);
   template <typename T> inline bool intersect(const ray<T,3>& ray1, const ray<T,3>& ray2);
   template <typename T> inline bool intersect(const ray<T,2>& ray, const segment<T,2>& segment);
   template <typename T> inline bool intersect(const ray<T,3>& ray, const segment<T,3>& segment);
   template <typename T> inline bool intersect(const ray<T,2>& ray, const rectangle<T>& rectangle);
   template <typename T> inline bool intersect(const ray<T,3>& ray, const box<T,3>& box);
   template <typename T> inline bool intersect(const ray<T,2>& ray, const triangle<T,2>& triangle);
   template <typename T> inline bool intersect(const ray<T,3>& ray, const triangle<T,3>& triangle);
   template <typename T> inline bool intersect(const ray<T,2>& ray, const quadix<T,2>& quadix);
   template <typename T> inline bool intersect(const ray<T,2>& ray, const circle<T>& circle);
   template <typename T> inline bool intersect(const ray<T,3>& ray, const sphere<T>& sphere);
   template <typename T> inline bool intersect(const ray<T,3>& ray, const plane<T,3>& plane);
   template <typename T> inline bool intersect(const ray<T,2>& ray, const polygon<T,2>& polygon);
   template <typename T> inline bool intersect(const plane<T,3>& plane1, const plane<T,3>& plane2);
   template <typename T> inline bool intersect(const plane<T,3>& plane, const sphere<T>& sphere);
   template <typename T> inline bool intersect(const plane<T,3>& plane, const line<T,3>& line);

   template <typename T>
   inline bool simple_intersect(const T& x1, const T& y1,
                                const T& x2, const T& y2,
                                const T& x3, const T& y3,
                                const T& x4, const T& y4);

   template <typename T>
   inline bool simple_intersect(const point2d<T>& point1, const point2d<T>& point2,
                                const point2d<T>& point3, const point2d<T>& point4);

   template <typename T>
   inline bool simple_intersect(const segment<T,2>& segment1, const segment<T,2>& segment2);

   template <typename T> inline bool intersect_vertical_horizontal(const segment<T,2>& segment1, const segment<T,2>& segment2);
   template <typename T> inline bool intersect_vertical_vertical(const segment<T,2>& segment1, const segment<T,2>& segment2);
   template <typename T> inline bool intersect_horizontal_horizontal(const segment<T,2>& segment1, const segment<T,2>& segment2);

   template <typename T>
   inline void intersection_point(const T& x1, const T& y1,
                                  const T& x2, const T& y2,
                                  const T& x3, const T& y3,
                                  const T& x4, const T& y4,
                                        T& ix,       T& iy);

   template <typename T>
   inline void intersection_point(const point2d<T>& point1,
                                  const point2d<T>& point2,
                                  const point2d<T>& point3,
                                  const point2d<T>& point4,
                                        T& ix,       T& iy);

   template <typename T>
   inline point2d<T> intersection_point(const point2d<T>& point1,
                                        const point2d<T>& point2,
                                        const point2d<T>& point3,
                                        const point2d<T>& point4);
   template <typename T>
   inline point2d<T> intersection_point(const segment<T,2>& segment1,
                                        const segment<T,2>& segment2);

   template <typename T>
   inline void intersection_point(const T& x1, const T& y1, const T& z1,
                                  const T& x2, const T& y2, const T& z2,
                                  const T& x3, const T& y3, const T& z3,
                                  const T& x4, const T& y4, const T& z4,
                                        T& ix,       T& iy,       T& iz, const T& fuzzy = T(0.0));

   template <typename T>
   inline void intersection_point(const point3d<T>& point1,
                                  const point3d<T>& point2,
                                  const point3d<T>& point3,
                                  const point3d<T>& point4,
                                        T& ix, T& iy, T& iz, const T& fuzzy = T(0.0));

   template <typename T>
   inline point3d<T> intersection_point(const point3d<T>& point1,
                                        const point3d<T>& point2,
                                        const point3d<T>& point3,
                                        const point3d<T>& point4, const T& fuzzy = T(0.0));

   template <typename T>
   inline point3d<T> intersection_point(const segment<T,3>& segment1,
                                        const segment<T,3>& segment2, const T& fuzzy = T(0.0));

   template <typename T>
   inline point2d<T> intersection_point(const segment<T,2>& segment,
                                        const line<T,2>& line);

   template <typename T>
   inline point3d<T> intersection_point(const segment<T,3>& segment,
                                        const line<T,3>& line, const T& fuzzy = T(0.0));

   template <typename T>
   inline point3d<T> intersection_point(const segment<T,3>& segment,
                                        const plane<T,3>& plane);

   template <typename T, typename OutputIterator>
   inline void intersection_point(const segment<T,2>& segment,
                                  const quadratic_bezier<T,2>& bezier,
                                  OutputIterator out,
                                  const std::size_t& steps = 1000);

   template <typename T, typename OutputIterator>
   inline void intersection_point(const segment<T,2>& segment,
                                  const cubic_bezier<T,2>& bezier,
                                  OutputIterator out,
                                  const std::size_t& steps = 1000);

   template <typename T, typename OutputIterator>
   inline void intersection_point(const segment<T,3>& segment,
                                  const quadratic_bezier<T,3>& bezier,
                                  OutputIterator out,
                                  const std::size_t& steps = 1000);

   template <typename T, typename OutputIterator>
   inline void intersection_point(const segment<T,3>& segment,
                                  const cubic_bezier<T,3>& bezier,
                                  OutputIterator out,
                                  const std::size_t& steps = 1000);

   template <typename T>
   inline point2d<T> intersection_point(const line<T,2>& line1,
                                        const line<T,2>& line2);

   template <typename T>
   inline point3d<T> intersection_point(const line<T,3>& line1,
                                        const line<T,3>& line2, const T& fuzzy = T(0.0));

   template <typename T>
   inline void intersection_point(const circle<T>&  circle1,
                                  const circle<T>&  circle2,
                                        point2d<T>& point1,
                                        point2d<T>& point2);

   template <typename T, typename OutputIterator>
   inline void intersection_point(const segment<T,2>&  segment,
                                  const triangle<T,2>& triangle,
                                  OutputIterator out);

   template <typename T>
   inline void intersection_point(const line<T,3>&     line,
                                  const triangle<T,3>& triangle,
                                  point3d<T>&          ipoint);

   template <typename T>
   inline point3d<T> intersection_point(const line<T,3>& line,
                                  const plane<T,3>&      plane);

   template <typename T, typename OutputIterator>
   inline void intersection_point(const T& x1, const T& y1,
                                  const T& x2, const T& y2,
                                  const T& cx, const T& cy,
                                  const T& radius,
                                  OutputIterator out);

   template <typename T, typename OutputIterator>
   inline void intersection_point(const segment<T,2>& segment,
                                  const circle<T>&    circle,
                                  OutputIterator out);

   template <typename T, typename OutputIterator>
   inline void intersection_point(const line<T,2>& line,
                                  const circle<T>& circle,
                                  OutputIterator out);

   template <typename T, typename OutputIterator>
   inline void intersection_point(const segment<T,3>& segment,
                                  const sphere<T>&    sphere,
                                  OutputIterator out);

   template <typename T, typename OutputIterator>
   inline void intersection_point(const line<T,3>& line,
                                  const sphere<T>& sphere,
                                  OutputIterator out);

   template <typename T>
   inline point2d<T> intersection_point(const ray<T,2>& ray1, const ray<T,2>& ray2);

   template <typename T>
   inline point3d<T> intersection_point(const ray<T,3>& ray, const triangle<T,3>& triangle);

   template <typename T>
   inline point3d<T> intersection_point(const ray<T,3>& ray, const plane<T,3>& plane);

   template <typename T, typename OutputIterator>
   inline void intersection_point(const ray<T,2>& ray, const circle<T>& circle, OutputIterator out);

   template <typename T, typename OutputIterator>
   inline void intersection_point(const ray<T,3>& ray, const sphere<T>& sphere, OutputIterator out);

   template <typename T>
   inline void intersection_point_line_to_line(const T& x1, const T& y1, const T& z1,
                                               const T& x2, const T& y2, const T& z2,
                                               const T& x3, const T& y3, const T& z3,
                                               const T& x4, const T& y4, const T& z4,
                                                     T& Ix,       T& Iy,       T& Iz, const T& fuzzy = T(0.0));

   template <typename T>
   inline T normalize_angle(const T& angle);

   template <typename T>
   inline T vertical_mirror(const T& angle);

   template <typename T>
   inline T horizontal_mirror(const T& angle);

   template <typename T>
   inline unsigned int quadrant(const T& angle);

   template <typename T>
   inline unsigned int quadrant(const T& x, const T& y);

   template <typename T>
   inline unsigned int quadrant(const point2d<T>& point);

   template <typename T>
   inline T vertex_angle(const T& x1, const T& y1,
                         const T& x2, const T& y2,
                         const T& x3, const T& y3);

   template <typename T>
   inline T vertex_angle(const point2d<T>& point1,
                         const point2d<T>& point2,
                         const point2d<T>& point3);

   template <typename T>
   inline T vertex_angle(const T& x1, const T& y1, const T& z1,
                         const T& x2, const T& y2, const T& z2,
                         const T& x3, const T& y3, const T& z3);

   template <typename T>
   inline T vertex_angle(const point3d<T>& point1,
                         const point3d<T>& point2,
                         const point3d<T>& point3);

   template <typename T>
   inline T oriented_vertex_angle(const T& x1, const T& y1,
                                  const T& x2, const T& y2,
                                  const T& x3, const T& y3,
                                  const int orient = Clockwise);
   template <typename T>
   inline T oriented_vertex_angle(const point2d<T>& point1,
                                  const point2d<T>& point2,
                                  const point2d<T>& point3,
                                  const int orient = Clockwise);
   template <typename T>
   inline T cartesian_angle(const T& x, const T& y);

   template <typename T>
   inline T cartesian_angle(const point2d<T>& point);

   template <typename T>
   inline T robust_cartesian_angle(const T& x, const T& y);

   template <typename T>
   inline T robust_cartesian_angle(const point2d<T>& point);

   template <typename T>
   inline T cartesian_angle(const T& x, const T& y, const T& ox, const T& oy);

   template <typename T>
   inline T cartesian_angle(const point2d<T>& point, const point2d<T>& origin);

   template <typename T>
   inline T robust_cartesian_angle(const T& x, const T& y, const T& ox, const T& oy);

   template <typename T>
   inline T robust_cartesian_angle(const point2d<T>& point, const point2d<T>& origin);


   template <typename T>
   inline bool parallel(const T& x1, const T& y1,
                        const T& x2, const T& y2,
                        const T& x3, const T& y3,
                        const T& x4, const T& y4,
                        const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool parallel(const point2d<T>& point1,
                        const point2d<T>& point2,
                        const point2d<T>& point3,
                        const point2d<T>& point4,
                        const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool parallel(const segment<T,2>& segment1,
                        const segment<T,2>& segment2,
                        const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool parallel(const line<T,2>& line1,
                        const line<T,2>& line2,
                        const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool parallel(const T& x1, const T& y1, const T& z1,
                        const T& x2, const T& y2, const T& z2,
                        const T& x3, const T& y3, const T& z3,
                        const T& x4, const T& y4, const T& z4,
                        const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool parallel(const point3d<T>& point1,
                        const point3d<T>& point2,
                        const point3d<T>& point3,
                        const point3d<T>& point4,
                        const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool parallel(const segment<T,3>& segment1,
                        const segment<T,3>& segment2,
                        const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool parallel(const line<T,3>& line1,
                        const line<T,3>& line2,
                        const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool robust_parallel(const T& x1, const T& y1,
                               const T& x2, const T& y2,
                               const T& x3, const T& y3,
                               const T& x4, const T& y4,
                               const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool robust_parallel(const point2d<T>& point1,
                               const point2d<T>& point2,
                               const point2d<T>& point3,
                               const point2d<T>& point4,
                               const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool robust_parallel(const segment<T,2>& segment1,
                               const segment<T,2>& segment2,
                               const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool robust_parallel(const line<T,2>& line1,
                               const line<T,2>& line2,
                               const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool robust_parallel(const line<T,2>& line,
                               const segment<T,2>& segment,
                               const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool robust_parallel(const T& x1, const T& y1, const T& z1,
                               const T& x2, const T& y2, const T& z2,
                               const T& x3, const T& y3, const T& z3,
                               const T& x4, const T& y4, const T& z4,
                               const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool robust_parallel(const point3d<T>& point1,
                               const point3d<T>& point2,
                               const point3d<T>& point3,
                               const point3d<T>& point4,
                               const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool robust_parallel(const segment<T,3>& segment1,
                               const segment<T,3>& segment2,
                               const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool robust_parallel(const line<T,3>& line1,
                               const line<T,3>& line2,
                               const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool robust_parallel(const line<T,3>& line,
                               const segment<T,3>& segment,
                               const T& epsilon = T(Epsilon));


   template <typename T>
   inline bool perpendicular(const T& x1, const T& y1,
                             const T& x2, const T& y2,
                             const T& x3, const T& y3,
                             const T& x4, const T& y4,
                             const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool perpendicular(const point2d<T>& point1,
                             const point2d<T>& point2,
                             const point2d<T>& point3,
                             const point2d<T>& point4,
                             const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool perpendicular(const segment<T,2>& segment1,
                             const segment<T,2>& segment2,
                             const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool perpendicular(const line<T,2>& line1,
                             const line<T,2>& line2,
                             const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool perpendicular(const line<T,2>& line,
                             const segment<T,2>& segment,
                             const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool perpendicular(const T& x1, const T& y1, const T& z1,
                             const T& x2, const T& y2, const T& z2,
                             const T& x3, const T& y3, const T& z3,
                             const T& x4, const T& y4, const T& z4,
                             const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool perpendicular(const point3d<T>& point1,
                             const point3d<T>& point2,
                             const point3d<T>& point3,
                             const point3d<T>& point4,
                             const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool perpendicular(const segment<T,3>& segment1,
                             const segment<T,3>& segment2,
                             const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool perpendicular(const line<T,3>& line1,
                             const line<T,3>& line2,
                             const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool robust_perpendicular(const T& x1, const T& y1,
                                    const T& x2, const T& y2,
                                    const T& x3, const T& y3,
                                    const T& x4, const T& y4,
                                    const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool robust_perpendicular(const point2d<T>& point1,
                                    const point2d<T>& point2,
                                    const point2d<T>& point3,
                                    const point2d<T>& point4,
                                    const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool robust_perpendicular(const segment<T,2>& segment1,
                                    const segment<T,2>& segment2,
                                    const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool robust_perpendicular(const line<T,2>& line1,
                                    const line<T,2>& line2,
                                    const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool robust_perpendicular(const T& x1, const T& y1, const T& z1,
                                    const T& x2, const T& y2, const T& z2,
                                    const T& x3, const T& y3, const T& z3,
                                    const T& x4, const T& y4, const T& z4,
                                    const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool robust_perpendicular(const point3d<T>& point1,
                                    const point3d<T>& point2,
                                    const point3d<T>& point3,
                                    const point3d<T>& point4,
                                    const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool robust_perpendicular(const segment<T,3>& segment1,
                                    const segment<T,3>& segment2,
                                    const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool robust_perpendicular(const line<T,3>& line1,
                                    const line<T,3>& line2,
                                    const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool robust_perpendicular(const line<T,2>& line,
                                    const segment<T,2>& segment,
                                    const T& epsilon = T(Epsilon));

   template <typename T>
   inline bool line_to_line_intersect(const T& x1, const T& y1,
                                      const T& x2, const T& y2,
                                      const T& x3, const T& y3,
                                      const T& x4, const T& y4);
   template <typename T>
   inline bool line_to_line_intersect(const line<T,2>& line1, const line<T,2>& line2);

   template <typename T>
   inline bool rectangle_to_rectangle_intersect(const T& x1, const T& y1,
                                                const T& x2, const T& y2,
                                                const T& x3, const T& y3,
                                                const T& x4, const T& y4);

   template <typename T>
   inline bool rectangle_to_rectangle_intersect(const rectangle<T>& rectangle1,
                                                const rectangle<T>& rectangle2);

   template <typename T>
   inline bool box_to_box_intersect(const T& x1, const T& y1, const T& z1,
                                    const T& x2, const T& y2, const T& z2,
                                    const T& x3, const T& y3, const T& z3,
                                    const T& x4, const T& y4, const T& z4);

   template <typename T>
   inline bool box_to_box_intersect(const box<T,3>& box1, const box<T,3>& box2);

   template< typename T, unsigned int Dimension, typename Simplex, typename Bezier>
   inline bool simplex_to_bezier_intersect(const Simplex& simplex,
                                           const Bezier& bezier,
                                           const std::size_t& steps);

   template< typename T, unsigned int Dimension, typename Bezier, typename Iterator>
   inline bool simplex_to_bezier_intersect(const Iterator& begin,
                                           const Iterator& end,
                                           const Bezier& bezier,
                                           const std::size_t& steps);

   template <typename T>
   inline bool rectangle_within_rectangle(const T& x1, const T& y1,
                                          const T& x2, const T& y2,
                                          const T& x3, const T& y3,
                                          const T& x4, const T& y4);

   template <typename T>
   inline bool rectangle_within_rectangle(const rectangle<T>& rectangle1,
                                          const rectangle<T>& rectangle2);

   template <typename T>
   inline bool box_within_box(const T& x1, const T& y1, const T& z1,
                              const T& x2, const T& y2, const T& z2,
                              const T& x3, const T& y3, const T& z3,
                              const T& x4, const T& y4, const T& z4);

   template <typename T>
   inline bool box_within_box(const box<T,3>& box1, const box<T,3>& box2);


   template <typename T>
   inline bool circle_within_rectangle(const T&  x, const T&  y, const T& radius,
                                       const T& x1, const T& y1,
                                       const T& x2, const T& y2);

   template <typename T>
   inline bool circle_within_rectangle(const circle<T>& circle, const rectangle<T>& rectangle);

   template <typename T>
   inline bool triangle_within_rectangle(const T& x1, const T& y1,
                                         const T& x2, const T& y2,
                                         const T& x3, const T& y3,
                                         const T& x4, const T& y4,
                                         const T& x5, const T& y5);
   template <typename T>
   inline bool triangle_within_rectangle(const triangle<T,2>& triangle, const rectangle<T>& rectangle);

   template <typename T>
   inline bool segment_within_rectangle(const T& x1, const T& y1,
                                        const T& x2, const T& y2,
                                        const T& x3, const T& y3,
                                        const T& x4, const T& y4);

   template <typename T>
   inline bool segment_within_rectangle(const segment<T,2>& segment, const rectangle<T>& rectangle);


   template <typename T>
   inline bool quadix_within_rectangle(const T& x1, const T& y1,
                                       const T& x2, const T& y2,
                                       const T& x3, const T& y3,
                                       const T& x4, const T& y4,
                                       const T& x5, const T& y5,
                                       const T& x6, const T& y6);

   template <typename T>
   inline bool quadix_within_rectangle(const quadix<T,2>& quadix, const rectangle<T>& rectangle);

   template <typename T>
   inline bool polygon_within_rectangle(const polygon<T,2>& polygon, const rectangle<T>& rectangle);

   template <typename T>
   inline bool sphere_within_box(const T&  x, const T&  y, const T&  z, const T& radius,
                                 const T& x1, const T& y1, const T& z1,
                                 const T& x2, const T& y2, const T& z2);

   template <typename T>
   inline bool sphere_within_box(const sphere<T>& sphere, const box<T,3>& box);

   template <typename T>
   inline bool triangle_within_box(const T& x1, const T& y1, const T& z1,
                                   const T& x2, const T& y2, const T& z2,
                                   const T& x3, const T& y3, const T& z3,
                                   const T& x4, const T& y4, const T& z4,
                                   const T& x5, const T& y5, const T& z5);
   template <typename T>
   inline bool triangle_within_box(const triangle<T,3>& triangle, const box<T,3>& box);

   template <typename T>
   inline bool segment_within_box(const T& x1, const T& y1, const T& z1,
                                  const T& x2, const T& y2, const T& z2,
                                  const T& x3, const T& y3, const T& z3,
                                  const T& x4, const T& y4, const T& z4) ;

   template <typename T>
   inline bool segment_within_box(const segment<T,3>& segment, const box<T,3>& box);


   template <typename T>
   inline bool quadix_within_box(const T& x1, const T& y1, const T& z1,
                                 const T& x2, const T& y2, const T& z2,
                                 const T& x3, const T& y3, const T& z3,
                                 const T& x4, const T& y4, const T& z4,
                                 const T& x5, const T& y5, const T& z5,
                                 const T& x6, const T& y6, const T& z6);

   template <typename T>
   inline bool quadix_within_box(const quadix<T,3>& quadix, const box<T,3>& box);

   template <typename T>
   inline bool polygon_within_box(const polygon<T,3>& polygon, const box<T,3>& box);

   template <typename T>
   inline bool circle_in_circle(const circle<T>& circle1, const circle<T>& circle2);

   template <typename T>
   inline bool is_tangent(const segment<T,2>& segment, const circle<T>& circle);

   template <typename T>
   inline bool point_of_reflection(const T& sx1, const T& sy1,
                                   const T& sx2, const T& sy2,
                                   const T& p1x, const T& p1y,
                                   const T& p2x, const T& p2y,
                                         T& rpx,       T& rpy);
   template <typename T>
   inline bool point_of_reflection(const segment<T,2>& segment,
                                   const point2d<T>&   point1,
                                   const point2d<T>&   point2,
                                         point2d<T>&   reflection_point);

   template <typename T> inline segment<T,2> edge(const triangle<T,2>& triangle, const std::size_t& edge_index);
   template <typename T> inline segment<T,3> edge(const triangle<T,3>& triangle, const std::size_t& edge_index);
   template <typename T> inline segment<T,2> edge(const quadix<T,2>& quadix, const std::size_t& edge_index);
   template <typename T> inline segment<T,3> edge(const quadix<T,3>& quadix, const std::size_t& edge_index);
   template <typename T> inline segment<T,2> edge(const rectangle<T>& rectangle, const std::size_t& edge);
   template <typename T> inline segment<T,2> edge(const polygon<T,2>& polygon, const std::size_t& edge);
   template <typename T> inline segment<T,3> edge(const polygon<T,3>& polygon, const std::size_t& edge);

   template <typename T> inline segment<T,2> opposing_edge(const triangle<T,2>& triangle, const std::size_t& corner);
   template <typename T> inline segment<T,3> opposing_edge(const triangle<T,3>& triangle, const std::size_t& corner);

   template <typename T> inline segment<T,2> reverse_segment(const segment<T,2>& segment);
   template <typename T> inline segment<T,3> reverse_segment(const segment<T,3>& segment);

   template <typename T> inline point2d<T> rectangle_corner(const rectangle<T>& rectangle, const std::size_t& corner_index);
   template <typename T> inline point3d<T> box_corner(const box<T,3>& box, const std::size_t& corner_index);

   template <typename T> inline line<T,2> triangle_bisector(const triangle<T,2>& triangle, const std::size_t& bisector);
   template <typename T> inline line<T,3> triangle_bisector(const triangle<T,3>& triangle, const std::size_t& bisector);

   template <typename T>
   inline line<T,2> triangle_external_bisector(const triangle<T,2>& triangle,
                                               const std::size_t& corner,
                                               const std::size_t& opposing_corner);

   template <typename T>
   inline line<T,3> triangle_external_bisector(const triangle<T,3>& triangle,
                                               const std::size_t& corner,
                                               const std::size_t& opposing_corner);

   template <typename T> inline line<T,2> triangle_median(const triangle<T,2>& triangle, const std::size_t& median);
   template <typename T> inline line<T,3> triangle_median(const triangle<T,3>& triangle, const std::size_t& median);

   template <typename T> inline line<T,2> triangle_symmedian(const triangle<T,2>& triangle, const std::size_t& symmedian);
   template <typename T> inline line<T,3> triangle_symmedian(const triangle<T,3>& triangle, const std::size_t& symmedian);

   template <typename T> inline line<T,2> euler_line(const triangle<T,2>& triangle);
   template <typename T> inline line<T,3> euler_line(const triangle<T,3>& triangle);

   template <typename T> inline point2d<T> exmedian_point(const triangle<T,2>& triangle, const std::size_t& corner);
   template <typename T> inline point3d<T> exmedian_point(const triangle<T,3>& triangle, const std::size_t& corner);

   template <typename T> inline point2d<T> feuerbach_point(const triangle<T,2>& triangle);

   template <typename T>
   inline line<T,2> confined_triangle_median(const triangle<T,2>& triangle,const point2d<T>& point, const std::size_t& median);

   template <typename T>
   inline line<T,3> confined_triangle_median(const triangle<T,3>& triangle,const point3d<T>& point, const std::size_t& median);

   template <typename T> inline line<T,2> create_parallel_line_on_point(const line<T,2>& line, const point2d<T>& point);
   template <typename T> inline line<T,3> create_parallel_line_on_point(const line<T,3>& line, const point3d<T>& point);

   template <typename T> inline segment<T,2> create_parallel_segment_on_point(const line<T,2>& line, const point2d<T>& point);
   template <typename T> inline segment<T,3> create_parallel_segment_on_point(const line<T,3>& line, const point3d<T>& point);

   template <typename T>
   inline bool point_in_rectangle(const T& px, const T& py,
                                  const T& x1, const T& y1,
                                  const T& x2, const T& y2);

   template <typename T>
   inline bool point_in_rectangle(const point2d<T>& point,
                                  const T& x1, const T& y1,
                                  const T& x2, const T& y2);

   template <typename T>
   inline bool point_in_rectangle(const T& px, const T& py, const rectangle<T>& rectangle);

   template <typename T>
   inline bool point_in_rectangle(const point2d<T>& point, const rectangle<T>& rectangle);

   template <typename T>
   inline bool point_in_rectangle(const point2d<T>& point, const point2d<T>& rect_point1, point2d<T>& rect_point2);

   template <typename T>
   inline bool point_in_rectangle(const point2d<T>& point, const segment<T,2>& segment);

   template <typename T>
   inline bool point_in_box(const T& px, const T& py, const T& pz,
                            const T& x1, const T& y1, const T& z1,
                            const T& x2, const T& y2, const T& z2);

   template <typename T>
   inline bool point_in_box(const point3d<T>& point,
                            const T& x1, const T& y1, const T& z1,
                            const T& x2, const T& y2, const T& z2);

   template <typename T>
   inline bool point_in_box(const T& px, const T& py, const T& pz, const box<T,3>& box);

   template <typename T>
   inline bool point_in_box(const point3d<T>& point, const box<T,3>& box);

   template <typename T>
   inline bool point_in_box(const point3d<T>& point, const point3d<T>& box_point1, const point3d<T>& box_point2);

   template <typename T>
   inline bool point_in_box(const point3d<T>& point, const segment<T,3>& segment);

   template <typename T>
   inline bool point_in_triangle(const T& px, const T& py,
                                 const T& x1, const T& y1,
                                 const T& x2, const T& y2,
                                 const T& x3, const T& y3);
   template <typename T>
   inline bool point_in_triangle(const point2d<T>& point,
                                 const point2d<T>& point1,
                                 const point2d<T>& point2,
                                 const point2d<T>& point3);


   template <typename T>
   inline bool point_in_triangle(const T& px, const T& py, const triangle<T,2>& triangle);

   template <typename T>
   inline bool point_in_triangle(const point2d<T>& point, const triangle<T,2>& triangle);

   template <typename T>
   inline bool point_in_quadix(const T& px, const T& py,
                               const T& x1, const T& y1,
                               const T& x2, const T& y2,
                               const T& x3, const T& y3,
                               const T& x4, const T& y4);

   template <typename T>
   inline bool point_in_quadix(const point2d<T>& point,
                               const point2d<T>& point1,
                               const point2d<T>& point2,
                               const point2d<T>& point3,
                               const point2d<T>& point4);

   template <typename T>
   inline bool point_in_quadix(const T& px, const T& py,
                               const quadix<T,2>& quadix);
   template <typename T>
   inline bool point_in_quadix(const point2d<T>&  point,
                               const quadix<T,2>& quadix);

   template <typename T>
   inline bool point_in_circle(const T& px, const T& py, const T& cx, const T& cy, const T& radius);

   template <typename T>
   inline bool point_in_circle(const T& px, const T& py, const circle<T>& circle);

   template <typename T>
   inline bool point_in_circle(const point2d<T>& point, const circle<T>& circle);

   template <typename T>
   inline bool point_in_sphere(const T& px, const T& py, const T& pz, const T& cx, const T& cy, const T& cz, const T& radius);

   template <typename T>
   inline bool point_in_sphere(const T& px, const T& py, const T& pz, const sphere<T>& sphere);

   template <typename T>
   inline bool point_in_sphere(const point3d<T>& point, const sphere<T>& sphere);

   template <typename T>
   inline bool point_in_three_point_circle(const T& px, const T& py,
                                           const T& x1, const T& y1,
                                           const T& x2, const T& y2,
                                           const T& x3, const T& y3);

   template <typename T>
   inline bool point_in_three_point_circle(const point2d<T>& point,
                                           const point2d<T>& point1,
                                           const point2d<T>& point2,
                                           const point2d<T>& point3);

   template <typename T>
   inline bool point_in_three_point_circle(const point2d<T>& point, const triangle<T,2> triangle);

   template <typename T>
   inline bool point_in_focus_area(const T& px, const T& py,
                                   const T& x1, const T& y1,
                                   const T& x2, const T& y2,
                                   const T& x3, const T& y3);
   template <typename T>
   inline bool point_in_focus_area(const point2d<T>& point,
                                   const point2d<T>& point1,
                                   const point2d<T>& point2,
                                   const point2d<T>& point3);

   template <typename T>
   inline bool point_on_segment(const point2d<T>& point, const segment<T,2>& segment);

   template <typename T>
   inline bool point_on_segment(const point3d<T>& point, const segment<T,3>& segment);

   template <typename T>
   inline bool point_on_ray(const T& px, const T& py,
                            const T& ox, const T& oy,
                            const T& dx, const T& dy);

   template <typename T>
   inline bool point_on_ray(const T& px, const T& py, const T& pz,
                            const T& ox, const T& oy, const T& oz,
                            const T& dx, const T& dy, const T& dz);

   template <typename T>
   inline bool point_on_ray(const point2d<T>& point, const ray<T,2>& ray);

   template <typename T>
   inline bool point_on_ray(const point3d<T>& point, const ray<T,3>& ray);

   template <typename T>
   inline bool point_on_rectangle(const T& px, const T& py,
                                  const T& x1, const T& y1,
                                  const T& x2, const T& y2);

   template <typename T>
   inline bool point_on_rectangle(const point2d<T>& point,
                                  const T& x1, const T& y1,
                                  const T& x2, const T& y2);

   template <typename T>
   inline bool point_on_rectangle(const T& px, const T& py, const rectangle<T>& rectangle);

   template <typename T>
   inline bool point_on_rectangle(const point2d<T>& point, const rectangle<T>& rectangle);

   template <typename T>
   inline bool point_on_triangle(const T& px, const T& py,
                                 const T& x1, const T& y1,
                                 const T& x2, const T& y2,
                                 const T& x3, const T& y3);
   template <typename T>
   inline bool point_on_triangle(const point2d<T>& point,
                                 const point2d<T>& point1,
                                 const point2d<T>& point2,
                                 const point2d<T>& point3);


   template <typename T>
   inline bool point_on_triangle(const T& px, const T& py, const triangle<T,2>& triangle);

   template <typename T>
   inline bool point_on_triangle(const point2d<T>& point, const triangle<T,2>& triangle);

   template <typename T>
   inline bool point_on_quadix(const T& px, const T& py,
                               const T& x1, const T& y1,
                               const T& x2, const T& y2,
                               const T& x3, const T& y3,
                               const T& x4, const T& y4);

   template <typename T>
   inline bool point_on_quadix(const point2d<T>& point,
                               const point2d<T>& point1,
                               const point2d<T>& point2,
                               const point2d<T>& point3,
                               const point2d<T>& point4);

   template <typename T>
   inline bool point_on_quadix(const T& px, const T& py,
                               const quadix<T,2>& quadix);
   template <typename T>
   inline bool point_on_quadix(const point2d<T>&  point,
                               const quadix<T,2>& quadix);

   template <typename T>
   inline bool point_on_circle(const T& px, const T& py, const T& cx, const T& cy, const T& radius);

   template <typename T>
   inline bool point_on_circle(const T& px, const T& py, const circle<T>& circle);

   template <typename T>
   inline bool point_on_circle(const point2d<T>& point, const circle<T>& circle);

   template <typename T>
   inline bool point_on_bezier(const point2d<T>& point, const quadratic_bezier<T,2>& bezier, const std::size_t& steps = 1000, const T& fuzzy = T(Epsilon));

   template <typename T>
   inline bool point_on_bezier(const point2d<T>& point, const cubic_bezier<T,2>& bezier, const std::size_t& steps = 1000, const T& fuzzy = T(Epsilon));

   template <typename T>
   inline bool point_on_bezier(const point3d<T>& point, const quadratic_bezier<T,3>& bezier, const std::size_t& steps = 1000, const T& fuzzy = T(Epsilon));

   template <typename T>
   inline bool point_on_bezier(const point3d<T>& point, const cubic_bezier<T,3>& bezier, const std::size_t& steps = 1000, const T& fuzzy = T(Epsilon));

   template <typename T>
   inline point2d<T> isogonal_conjugate(const point2d<T>& point, const triangle<T,2>& triangle);

   template <typename T>
   inline point3d<T> isogonal_conjugate(const point3d<T>& point, const triangle<T,3>& triangle);

   template <typename T>
   inline point2d<T> cyclocevian_conjugate(const point2d<T>& point, const triangle<T,2>& triangle);

   template <typename T>
   inline point2d<T> symmedian_point(const triangle<T,2>& triangle);

   template <typename T>
   inline point3d<T> symmedian_point(const triangle<T,3>& triangle);

   template <typename T>
   inline void create_equilateral_triangle(const T& x1, const T& y1,
                                           const T& x2, const T& y2,
                                                 T& x3,       T& y3);

   template <typename T>
   inline void create_equilateral_triangle(const point2d<T>& point1,
                                           const point2d<T>& point2,
                                                 point2d<T>& point3);

   template <typename T>
   inline triangle<T,2> create_equilateral_triangle(const T& x1, const T& y1,
                                                    const T& x2, const T& y2);

   template <typename T>
   inline triangle<T,2> create_equilateral_triangle(const point2d<T>& point1,
                                                    const point2d<T>& point2);

   template <typename T> inline triangle<T,2> create_equilateral_triangle(const T& cx, const T& cy, const T& side_length);
   template <typename T> inline triangle<T,2> create_equilateral_triangle(const point2d<T>& center_point, const T& side_length);

   template <typename T> inline triangle<T,2> create_isosceles_triangle(const point2d<T>& point1, const point2d<T>& point2, const T& angle);
   template <typename T> inline triangle<T,2> create_isosceles_triangle(const segment<T,2>& segment, const T& angle);

   template <typename T> inline triangle<T,2> create_triangle(const point2d<T>& point1, const point2d<T>& point2, const T& angle1, const T& angle2);
   template <typename T> inline triangle<T,2> create_triangle(const segment<T,2>& segment, const T& angle1, const T& angle2);

   template <typename T> inline triangle<T,2> create_morley_triangle(const triangle<T,2>& triangle);

   template <typename T> inline triangle<T,2> create_cevian_triangle(const triangle<T,2>& triangle, const point2d<T>& point);
   template <typename T> inline triangle<T,3> create_cevian_triangle(const triangle<T,3>& triangle, const point3d<T>& point);

   template <typename T> inline triangle<T,2> create_anticevian_triangle(const triangle<T,2>& triangle, const point2d<T>& point);
   template <typename T> inline triangle<T,3> create_anticevian_triangle(const triangle<T,3>& triangle, const point3d<T>& point);

   template <typename T> inline triangle<T,2> create_anticomplementary_triangle(const triangle<T,2>& triangle);
   template <typename T> inline triangle<T,3> create_anticomplementary_triangle(const triangle<T,3>& triangle);

   template <typename T> inline triangle<T,2> create_inner_napoleon_triangle(const triangle<T,2>& triangle);

   template <typename T> inline triangle<T,2> create_outer_napoleon_triangle(const triangle<T,2>& triangle);

   template <typename T> inline triangle<T,2> create_inner_vecten_triangle(const triangle<T,2>& triangle);

   template <typename T> inline triangle<T,2> create_outer_vecten_triangle(const triangle<T,2>& triangle);

   template <typename T> inline triangle<T,2> create_medial_triangle(const triangle<T,2>& triangle);
   template <typename T> inline triangle<T,3> create_medial_triangle(const triangle<T,3>& triangle);

   template <typename T> inline triangle<T,2> create_contact_triangle(const triangle<T,2>& triangle);
   template <typename T> inline triangle<T,3> create_contact_triangle(const triangle<T,3>& triangle);

   template <typename T> inline triangle<T,2> create_symmedial_triangle(const triangle<T,2>& triangle, const point2d<T>& point);

   template <typename T> inline triangle<T,2> create_orthic_triangle(const triangle<T,2>& triangle);
   template <typename T> inline triangle<T,3> create_orthic_triangle(const triangle<T,3>& triangle);

   template <typename T> inline triangle<T,2> create_pedal_triangle(const point2d<T>& point, const triangle<T,2>& triangle);
   template <typename T> inline triangle<T,3> create_pedal_triangle(const point3d<T>& point, const triangle<T,3>& triangle);

   template <typename T> inline triangle<T,2> create_antipedal_triangle(const point2d<T>& point, const triangle<T,2>& triangle);

   template <typename T> inline triangle<T,2> create_excentral_triangle(const triangle<T,2>& triangle);
   template <typename T> inline triangle<T,3> create_excentral_triangle(const triangle<T,3>& triangle);

   template <typename T> inline triangle<T,2> create_incentral_triangle(const triangle<T,2>& triangle);
   template <typename T> inline triangle<T,3> create_incentral_triangle(const triangle<T,3>& triangle);

   template <typename T> inline triangle<T,2> create_intouch_triangle(const triangle<T,2>& triangle);

   template <typename T> inline triangle<T,2> create_extouch_triangle(const triangle<T,2>& triangle);
   template <typename T> inline triangle<T,3> create_extouch_triangle(const triangle<T,3>& triangle);

   template <typename T> inline triangle<T,2> create_feuerbach_triangle(const triangle<T,2>& triangle);

   template <typename T> inline triangle<T,2> create_circumcevian_triangle(const triangle<T,2>& triangle, const point2d<T>& point);

   template <typename T> inline triangle<T,2> create_circummedial_triangle(const triangle<T,2>& triangle);

   template <typename T> inline triangle<T,2> create_first_brocard_triangle(const triangle<T,2>& triangle);

   template <typename T> inline void create_right_triangle(const wykobi::point2d<T>& p1, const wykobi::point2d<T>& p2,
                                                          wykobi::point2d<T>& c1, wykobi::point2d<T>& c2);

   template <typename T>
   inline void create_equilateral_quadix(const T& x1, const T& y1,
                                         const T& x2, const T& y2,
                                               T& x3,       T& y3,
                                               T& x4,       T& y4);

   template <typename T>
   inline void create_equilateral_quadix(const point2d<T>& point1,
                                         const point2d<T>& point2,
                                               point2d<T>& point3,
                                               point2d<T>& point4);

   template <typename T>
   inline quadix<T,2> create_equilateral_quadix(const T& x1, const T& y1,
                                                const T& x2, const T& y2);

   template <typename T>
   inline quadix<T,2> create_equilateral_quadix(const point2d<T>& point1,
                                                const point2d<T>& point2);

   template <typename T>
   inline quadix<T,2> create_equilateral_quadix(const segment<T,2>& segment);

   template <typename T>
   inline quadix<T,2> create_equilateral_quadix(const T& cx, const T& cy, const T& side_length);

   template <typename T>
   inline quadix<T,2> create_equilateral_quadix(const point2d<T>& center_point, const T& side_length);

   template <typename T>
   inline void torricelli_point(const T& x1, const T& y1,
                                const T& x2, const T& y2,
                                const T& x3, const T& y3,
                                      T& px,       T& py);

   template <typename T>
   inline point2d<T> torricelli_point(const point2d<T>& point1,
                                      const point2d<T>& point2,
                                      const point2d<T>& point3);

   template <typename T>
   inline point2d<T> torricelli_point(const triangle<T,2>& triangle);

   template <typename T> inline bool trilateration(const T& c0x, const T& c0y, const T& c0r,
                                                   const T& c1x, const T& c1y, const T& c1r,
                                                   const T& c2x, const T& c2y, const T& c2r,
                                                         T&  px,       T&  py);

   template <typename T> inline point2d<T> trilateration(const circle<T>& c0, const circle<T>& c1, const circle<T>& c2);

   template <typename T>
   inline void incenter(const T& x1, const T& y1,
                        const T& x2, const T& y2,
                        const T& x3, const T& y3,
                              T& px,       T& py);

   template <typename T>
   inline void incenter(const T& x1, const T& y1, const T& z1,
                        const T& x2, const T& y2, const T& z2,
                        const T& x3, const T& y3, const T& z3,
                              T& px,       T& py,       T& pz);

   template <typename T>
   inline point2d<T> incenter(const point2d<T>& point1,
                              const point2d<T>& point2,
                              const point2d<T>& point3);

   template <typename T>
   inline point3d<T> incenter(const point3d<T>& point1,
                              const point3d<T>& point2,
                              const point3d<T>& point3);

   template <typename T>
   inline point2d<T> incenter(const triangle<T,2>& triangle);

   template <typename T>
   inline point3d<T> incenter(const triangle<T,3>& triangle);

   template <typename T>
   inline void circumcenter(const T& x1, const T& y1,
                            const T& x2, const T& y2,
                            const T& x3, const T& y3,
                                  T& px,       T& py);

   template <typename T>
   inline void circumcenter(const T& x1, const T& y1, const T& z1,
                            const T& x2, const T& y2, const T& z2,
                            const T& x3, const T& y3, const T& z3,
                                  T& px,       T& py,       T& pz);


   template <typename T>
   inline point2d<T> circumcenter(const point2d<T>& point1,
                                  const point2d<T>& point2,
                                  const point2d<T>& point3);

   template <typename T>
   inline point3d<T> circumcenter(const point3d<T>& point1,
                                  const point3d<T>& point2,
                                  const point3d<T>& point3);

   template <typename T>
   inline point2d<T> circumcenter(const triangle<T,2>& triangle);

   template <typename T>
   inline point3d<T> circumcenter(const triangle<T,3>& triangle);

   template <typename T>
   inline circle<T> circumcircle(const T& x1, const T& y1,
                                 const T& x2, const T& y2,
                                 const T& x3, const T& y3);

   template <typename T>
   inline circle<T> circumcircle(const point2d<T>& point1,
                                 const point2d<T>& point2,
                                 const point2d<T>& point3);

   template <typename T>
   inline circle<T> circumcircle(const triangle<T,2>& triangle);

   template <typename T>
   inline sphere<T> circumsphere(const T& x1, const T& y1, const T& z1,
                                 const T& x2, const T& y2, const T& z2,
                                 const T& x3, const T& y3, const T& z3);

   template <typename T>
   inline sphere<T> circumsphere(const point3d<T>& point1,
                                 const point3d<T>& point2,
                                 const point3d<T>& point3);

   template <typename T>
   inline sphere<T> circumsphere(const triangle<T,3>& triangle);


   template <typename T>
   inline circle<T> inscribed_circle(const T& x1, const T& y1,
                                     const T& x2, const T& y2,
                                     const T& x3, const T& y3);

   template <typename T>
   inline circle<T> inscribed_circle(const point2d<T>& point1,
                                     const point2d<T>& point2,
                                     const point2d<T>& point3);

   template <typename T>
   inline circle<T> inscribed_circle(const triangle<T,2>& triangle);

   template <typename T>
   inline sphere<T> inscribed_sphere(const T& x1, const T& y1, const T& z1,
                                     const T& x2, const T& y2, const T& z2,
                                     const T& x3, const T& y3, const T& z3);

   template <typename T>
   inline sphere<T> inscribed_sphere(const point3d<T>& point1,
                                     const point3d<T>& point2,
                                     const point3d<T>& point3);

   template <typename T>
   inline sphere<T> inscribed_sphere(const triangle<T,3>& triangle);

   template <typename T>
   inline circle<T> nine_point_circle(const T& x1, const T& y1,
                                      const T& x2, const T& y2,
                                      const T& x3, const T& y3);

   template <typename T>
   inline circle<T> nine_point_circle(const point2d<T>& point1,
                                      const point2d<T>& point2,
                                      const point2d<T>& point3);

   template <typename T>
   inline circle<T> nine_point_circle(const triangle<T,2>& triangle);

   template <typename T>
   inline point2d<T> orthocenter(const triangle<T,2>& triangle);

   template <typename T>
   inline point3d<T> orthocenter(const triangle<T,3>& triangle);

   template <typename T>
   inline point2d<T> excenter(const triangle<T,2>& triangle, const std::size_t& corner);

   template <typename T>
   inline point3d<T> excenter(const triangle<T,3>& triangle, const std::size_t& corner);

   template <typename T>
   inline circle<T> excircle(const triangle<T,2>& triangle, const std::size_t& i);

   template <typename T>
   inline circle<T> mandart_circle(const triangle<T,2>& triangle);

   template <typename T>
   inline circle<T> brocard_circle(const triangle<T,2>& triangle);

   template <typename T>
   inline circle<T> invert_circle_across_circle(const circle<T>& circle1, const circle<T>& circle2);

   template <typename T>
   inline sphere<T> invert_sphere_across_sphere(const sphere<T>& sphere1, const sphere<T>& sphere2);

   template <typename T>
   inline void circle_tangent_points(const circle<T>& circle, const point2d<T>& point, point2d<T>& point1, point2d<T>& point2);

   template <typename T>
   inline void circle_internal_tangent_lines(const circle<T>& circle0,
                                             const circle<T>& circle1,
                                             std::vector< line<T,2> >& lines);

   template <typename T>
   inline void circle_internal_tangent_segments(const circle<T>& circle0,
                                                const circle<T>& circle1,
                                                std::vector< segment<T,2> >& segments);

   template <typename T>
   inline void circle_outer_tangent_lines(const circle<T>& circle0,
                                          const circle<T>& circle1,
                                          std::vector< line<T,2> >& lines);

   template <typename T>
   inline void circle_outer_tangent_segments(const circle<T>& circle0,
                                             const circle<T>& circle1,
                                             std::vector< segment<T,2> >& segments);

   template <typename T>
   inline line<T,2> tangent_line(const circle<T>& circle, const point2d<T>& point);

   template <typename T>
   inline line<T,2> create_line_from_bisector(const T& x1, const T& y1,
                                              const T& x2, const T& y2,
                                              const T& x3, const T& y3);

   template <typename T>
   inline segment<T,2> create_segment_from_bisector(const T& x1, const T& y1,
                                                    const T& x2, const T& y2,
                                                    const T& x3, const T& y3);

   template <typename T>
   inline ray<T,2> create_ray_from_bisector(const T& x1, const T& y1,
                                            const T& x2, const T& y2,
                                            const T& x3, const T& y3);

   template <typename T>
   inline line<T,3> create_line_from_bisector(const T& x1, const T& y1, const T& z1,
                                              const T& x2, const T& y2, const T& z2,
                                              const T& x3, const T& y3, const T& z3);

   template <typename T>
   inline segment<T,3> create_segment_from_bisector(const T& x1, const T& y1, const T& z1,
                                                    const T& x2, const T& y2, const T& z2,
                                                    const T& x3, const T& y3, const T& z3);

   template <typename T>
   inline ray<T,3> create_ray_from_bisector(const T& x1, const T& y1, const T& z1,
                                            const T& x2, const T& y2, const T& z2,
                                            const T& x3, const T& y3, const T& z3);

   template <typename T> inline line<T,2> create_line_from_bisector(const point2d<T>& point1,const point2d<T>& point2,const point2d<T>& point3);
   template <typename T> inline segment<T,2> create_segment_from_bisector(const point2d<T>& point1,const point2d<T>& point2,const point2d<T>& point3);
   template <typename T> inline ray<T,2> create_ray_from_bisector(const point2d<T>& point1,const point2d<T>& point2,const point2d<T>& point3);
   template <typename T> inline line<T,3> create_line_from_bisector(const point3d<T>& point1,const point3d<T>& point2,const point3d<T>& point3);
   template <typename T> inline segment<T,3> create_segment_from_bisector(const point3d<T>& point1,const point3d<T>& point2,const point3d<T>& point3);
   template <typename T> inline ray<T,3> create_ray_from_bisector(const point3d<T>& point1,const point3d<T>& point2,const point3d<T>& point3);

   template <typename T> inline line<T,2> create_perpendicular_bisector(const T& x1, const T& y1,const T& x2, const T& y2);
   template <typename T> inline line<T,2> create_perpendicular_bisector(const point2d<T>& point1, const point2d<T>& point2);
   template <typename T> inline line<T,2> create_perpendicular_bisector(const segment<T,2>& segment);

   template <typename T> inline line<T,2> create_perpendicular_line_at_end_point(const line<T,2>& line);

   template <typename T>
   inline void closest_point_on_segment_from_point(const T& x1, const T& y1,
                                                   const T& x2, const T& y2,
                                                   const T& px, const T& py,
                                                         T& nx,       T& ny);

   template <typename T>
   inline void closest_point_on_segment_from_point(const T& x1, const T& y1, const T& z1,
                                                   const T& x2, const T& y2, const T& z2,
                                                   const T& px, const T& py, const T& pz,
                                                         T& nx,       T& ny,       T& nz);

   template <typename T>
   inline void closest_point_on_line_from_point(const T& x1, const T& y1,
                                                const T& x2, const T& y2,
                                                const T& px, const T& py,
                                                      T& nx,       T& ny);

   template <typename T>
   inline void closest_point_on_line_from_point(const T& x1, const T& y1, const T& z1,
                                                const T& x2, const T& y2, const T& z2,
                                                const T& px, const T& py, const T& pz,
                                                      T& nx,       T& ny,       T& nz);

   template <typename T>
   inline void order_sensitive_closest_point_on_segment_from_point(const T& x1, const T& y1,
                                                                   const T& x2, const T& y2,
                                                                   const T& px, const T& py,
                                                                         T& nx,       T& ny);

   template <typename T>
   inline void order_sensitive_closest_point_on_segment_from_point(const T& x1, const T& y1, const T& z1,
                                                                   const T& x2, const T& y2, const T& z2,
                                                                   const T& px, const T& py, const T& pz,
                                                                         T& nx,       T& ny,       T& nz);

   template <typename T>
   inline void order_sensitive_closest_point_on_line_from_point(const T& x1, const T& y1,
                                                                const T& x2, const T& y2,
                                                                const T& px, const T& py,
                                                                      T& nx,       T& ny);

   template <typename T>
   inline void order_sensitive_closest_point_on_line_from_point(const T& x1, const T& y1, const T& z1,
                                                                const T& x2, const T& y2, const T& z2,
                                                                const T& px, const T& py, const T& pz,
                                                                      T& nx,       T& ny,       T& nz);

   template <typename T>
   inline void closest_point_on_ray_from_point(const T& ox, const T& oy,
                                               const T& dx, const T& dy,
                                               const T& px, const T& py,
                                                     T& nx,       T& ny);

   template <typename T>
   inline void closest_point_on_ray_from_point(const T& ox, const T& oy, const T& oz,
                                               const T& dx, const T& dy, const T& dz,
                                               const T& px, const T& py, const T& pz,
                                                     T& nx,       T& ny,       T& nz);

   template <typename T>
   inline point2d<T> closest_point_on_segment_from_point(const T& x1, const T& y1,
                                                         const T& x2, const T& y2,
                                                         const T& px, const T& py);

   template <typename T>
   inline point3d<T> closest_point_on_segment_from_point(const T& x1, const T& y1, const T& z1,
                                                         const T& x2, const T& y2, const T& z2,
                                                         const T& px, const T& py, const T& pz);

   template <typename T>
   inline point2d<T> closest_point_on_segment_from_point(const segment<T,2>& segment, const point2d<T>& point);

   template <typename T>
   inline point3d<T> closest_point_on_segment_from_point(const segment<T,3>& segment, const point3d<T>& point);

   template <typename T>
   inline point2d<T> closest_point_on_line_from_point(const T& x1, const T& y1,
                                                      const T& x2, const T& y2,
                                                      const T& px, const T& py);

   template <typename T>
   inline point3d<T> closest_point_on_line_from_point(const T& x1, const T& y1, const T& z1,
                                                      const T& x2, const T& y2, const T& z2,
                                                      const T& px, const T& py, const T& pz);

   template <typename T>
   inline point2d<T> closest_point_on_line_from_point(const line<T,2>& line, const point2d<T>& point);

   template <typename T>
   inline point3d<T> closest_point_on_line_from_point(const line<T,3>& line, const point3d<T>& point);


   template <typename T>
   inline point2d<T> closest_point_on_ray_from_point(const T& ox, const T& oy,
                                                     const T& dx, const T& dy,
                                                     const T& px, const T& py);

   template <typename T>
   inline point3d<T> closest_point_on_ray_from_point(const T& ox, const T& oy, const T& oz,
                                                     const T& dx, const T& dy, const T& dz,
                                                     const T& px, const T& py, const T& pz);

   template <typename T>
   inline point2d<T> closest_point_on_ray_from_point(const ray<T,2>& ray, const point2d<T>& point);

   template <typename T>
   inline point3d<T> closest_point_on_ray_from_point(const ray<T,3>& ray, const point3d<T>& point);

   template <typename T>
   inline void closest_point_on_triangle_from_point(const T& x1, const T& y1,
                                                    const T& x2, const T& y2,
                                                    const T& x3, const T& y3,
                                                    const T& px, const T& py,
                                                          T& nx,       T& ny);

   template <typename T>
   inline point2d<T> closest_point_on_triangle_from_point(const T& x1, const T& y1,
                                                          const T& x2, const T& y2,
                                                          const T& x3, const T& y3,
                                                          const T& px, const T& py);

   template <typename T>
   inline point2d<T> closest_point_on_triangle_from_point(const triangle<T,2>& triangle, const T& px, const T& py);

   template <typename T>
   inline point2d<T> closest_point_on_triangle_from_point(const triangle<T,2>& triangle, const point2d<T>& point);

   template <typename T>
   inline void closest_point_on_triangle_from_point(const T& x1, const T& y1, const T& z1,
                                                    const T& x2, const T& y2, const T& z2,
                                                    const T& x3, const T& y3, const T& z3,
                                                    const T& px, const T& py, const T& pz,
                                                          T& nx,       T& ny,       T& nz);

   template <typename T>
   inline point3d<T> closest_point_on_triangle_from_point(const T& x1, const T& y1, const T& z1,
                                                          const T& x2, const T& y2, const T& z2,
                                                          const T& x3, const T& y3, const T& z3,
                                                          const T& px, const T& py, const T& pz);

   template <typename T>
   inline point3d<T> closest_point_on_triangle_from_point(const triangle<T,3>& triangle, const T& px, const T& py, const T& pz);

   template <typename T>
   inline point3d<T> closest_point_on_triangle_from_point(const triangle<T,3>& triangle, const point3d<T>& point);

   template <typename T>
   inline void closest_point_on_rectangle_from_point(const T& x1, const T& y1,
                                                     const T& x2, const T& y2,
                                                     const T& px, const T& py,
                                                           T& nx,       T& ny);

   template <typename T>
   inline point2d<T> closest_point_on_rectangle_from_point(const T& x1, const T& y1,
                                                           const T& x2, const T& y2,
                                                           const T& px, const T& py);

   template <typename T>
   inline point2d<T> closest_point_on_rectangle_from_point(const rectangle<T>& rectangle, const T& px, const T& py);

   template <typename T>
   inline point2d<T> closest_point_on_rectangle_from_point(const rectangle<T>& rectangle, const point2d<T>& point);

   template <typename T>
   inline void closest_point_on_box_from_point(const T& x1, const T& y1, const T& z1,
                                               const T& x2, const T& y2, const T& z2,
                                               const T& px, const T& py, const T& pz,
                                                     T& nx,       T& ny,       T& nz);

   template <typename T>
   inline point3d<T> closest_point_on_box_from_point(const T& x1, const T& y1, const T& z1,
                                                     const T& x2, const T& y2, const T& z2,
                                                     const T& px, const T& py, const T& pz);

   template <typename T>
   inline point3d<T> closest_point_on_box_from_point(const box<T,3>& box, const T& px, const T& py, const T& pz);

   template <typename T>
   inline point3d<T> closest_point_on_box_from_point(const box<T,3>& box, const point3d<T>& point);

   template <typename T>
   inline void closest_point_on_quadix_from_point(const T& x1, const T& y1,
                                                  const T& x2, const T& y2,
                                                  const T& x3, const T& y3,
                                                  const T& x4, const T& y4,
                                                  const T& px, const T& py,
                                                        T& nx,       T& ny);

   template <typename T>
   inline point2d<T> closest_point_on_quadix_from_point(const T& x1, const T& y1,
                                                        const T& x2, const T& y2,
                                                        const T& x3, const T& y3,
                                                        const T& x4, const T& y4,
                                                        const T& px, const T& py);
   template <typename T>
   inline point2d<T> closest_point_on_quadix_from_point(const quadix<T,2>& quadix, const point2d<T>& point);

   template <typename T>
   inline point2d<T> closest_point_on_circle_from_point(const circle<T>&  circle,
                                                        const point2d<T>& point);

   template <typename T>
   inline point3d<T> closest_point_on_sphere_from_point(const sphere<T>&  sphere,
                                                        const point3d<T>& point);

   template <typename T>
   inline point2d<T> closest_point_on_aabbb_from_point(const rectangle<T>& rectangle,
                                                       const point2d<T>&   point);

   template <typename T>
   inline point2d<T> closest_point_on_circle_from_segment(const circle<T>&    circle,
                                                          const segment<T,2>& segment);

   template <typename T>
   inline point3d<T> closest_point_on_sphere_from_segment(const sphere<T>&    sphere,
                                                          const segment<T,3>& segment);

   template <typename T>
   inline point3d<T> closest_point_on_plane_from_point(const plane<T,3>& plane,
                                                       const point3d<T>& point);

   template <typename T>
   inline point2d<T> closest_point_on_bezier_from_point(const quadratic_bezier<T,2>& bezier,
                                                        const point2d<T>& point,
                                                        const std::size_t& steps = 1000);

   template <typename T>
   inline point2d<T> closest_point_on_bezier_from_point(const cubic_bezier<T,2>& bezier,
                                                        const point2d<T>& point,
                                                        const std::size_t& steps = 1000);

   template <typename T>
   inline point3d<T> closest_point_on_bezier_from_point(const quadratic_bezier<T,3>& bezier,
                                                        const point3d<T>& point,
                                                        const std::size_t& steps = 1000);

   template <typename T>
   inline point3d<T> closest_point_on_bezier_from_point(const cubic_bezier<T,3>& bezier,
                                                        const point3d<T>& point,
                                                        const std::size_t& steps = 1000);

   template <typename T>
   inline point2d<T> closest_point_on_circle_from_circle(const circle<T>& circle1, const circle<T>& circle2);

   template <typename T>
   inline point3d<T> closest_point_on_sphere_from_sphere(const sphere<T>& sphere1, const sphere<T>& sphere2);

   template <typename T>
   inline point2d<T> closest_point_on_polygon_from_point(const polygon<T,2>& polygon, const point2d<T>& point);

   template <typename T>
   inline T minimum_distance_from_point_to_segment(const T& px, const T& py,
                                                   const T& x1, const T& y1,
                                                   const T& x2, const T& y2);

   template <typename T>
   inline T minimum_distance_from_point_to_segment(const T& px, const T& py, const T& pz,
                                                   const T& x1, const T& y1, const T& z1,
                                                   const T& x2, const T& y2, const T& z2);

   template <typename T>
   inline T minimum_distance_from_point_to_segment(const point2d<T>& point, const segment<T,2>& segment);

   template <typename T>
   inline T minimum_distance_from_point_to_segment(const point3d<T>& point, const segment<T,3>& segment);

   template <typename T>
   inline T minimum_distance_from_point_to_line(const T& px, const T& py,
                                                const T& x1, const T& y1,
                                                const T& x2, const T& y2);

   template <typename T>
   inline T minimum_distance_from_point_to_line(const T& px, const T& py, const T& pz,
                                                const T& x1, const T& y1, const T& z1,
                                                const T& x2, const T& y2, const T& z2);

   template <typename T>
   inline T minimum_distance_from_point_to_line(const point2d<T>& point, const line<T,2>& line);

   template <typename T>
   inline T minimum_distance_from_point_to_line(const point3d<T>& point, const line<T,3>& line);


   template <typename T>
   inline T minimum_distance_from_point_to_triangle(const T& px, const T& py,
                                                    const T& x1, const T& y1,
                                                    const T& x2, const T& y2,
                                                    const T& x3, const T& y3);

   template <typename T>
   inline T minimum_distance_from_point_to_triangle(const point2d<T>& point, const triangle<T,2>& triangle);

   template <typename T>
   inline T minimum_distance_from_point_to_rectangle(const T& px, const T& py,
                                                     const T& x1, const T& y1,
                                                     const T& x2, const T& y2);

   template <typename T>
   inline T minimum_distance_from_point_to_rectangle(const point2d<T>& point, const rectangle<T>& rectangle);

   template <typename T>
   inline void segment_mid_point(const T&   x1, const T&   y1,
                                 const T&   x2, const T&   y2,
                                       T& midx,       T& midy);

   template <typename T>
   inline void segment_mid_point(const segment<T,2>& segment, T& midx, T& midy);

   template <typename T>
   inline point2d<T> segment_mid_point(const point2d<T>& point1, const point2d<T>& point2);

   template <typename T>
   inline point2d<T> segment_mid_point(const segment<T,2>& segment);

   template <typename T>
   inline void segment_mid_point(const T&   x1, const T&   y1, const T&   z1,
                                 const T&   x2, const T&   y2, const T&   z2,
                                       T& midx,       T& midy,       T& midz);

   template <typename T>
   inline void segment_mid_point(const segment<T,3>& segment, T& midx, T& midy, T& midz);

   template <typename T>
   inline point3d<T> segment_mid_point(const point3d<T>& point1, const point3d<T>& point2);

   template <typename T>
   inline point3d<T> segment_mid_point(const segment<T,3>& segment);

   template <typename T>
   inline void centroid(const T& x1, const T& y1,
                        const T& x2, const T& y2,
                              T&  x,       T&  y);

   template <typename T>
   inline point2d<T> centroid(const point2d<T>& point1, const point2d<T>& point2);

   template <typename T>
   inline point2d<T> centroid(const segment<T,2>& segment);

   template <typename T>
   inline void centroid(const T& x1, const T& y1,
                        const T& x2, const T& y2,
                        const T& x3, const T& y3,
                              T&  x,       T&  y);

   template <typename T>
   inline void centroid(const T& x1, const T& y1, const T& z1,
                        const T& x2, const T& y2, const T& z2,
                        const T& x3, const T& y3, const T& z3,
                              T&  x,       T&  y,       T& z);

   template <typename T>
   inline void centroid(const T& x1, const T& y1,
                        const T& x2, const T& y2,
                        const T& x3, const T& y3,
                        const T& x4, const T& y4,
                              T&  x,       T&  y);

   template <typename T> inline void centroid(const triangle<T,2>& triangle, T& x, T& y);
   template <typename T> inline void centroid(const triangle<T,3>& triangle, T& x, T& y,T& z);
   template <typename T> inline void centroid(const quadix<T,2>& quadix, T& x, T& y);
   template <typename T> inline void centroid(const rectangle<T>& rectangle, T& x, T& y);
   template <typename T> inline void centroid(const box<T,3>& box, T& x, T& y, T& z);
   template <typename T> inline void centroid(const polygon<T,2>& polygon, T& x, T& y);

   template <typename T> inline point2d<T> centroid(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);
   template <typename T> inline point2d<T> centroid(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3, const point2d<T>& point4);
   template <typename T> inline point2d<T> centroid(const triangle<T,2>& triangle);
   template <typename T> inline point3d<T> centroid(const triangle<T,3>& triangle);
   template <typename T> inline point2d<T> centroid(const quadix<T,2>& quadix);
   template <typename T> inline point2d<T> centroid(const rectangle<T>& rectangle);
   template <typename T> inline point3d<T> centroid(const box<T,3>& box);
   template <typename T> inline point2d<T> centroid(const polygon<T,2>& polygon);

   template <typename T> inline bool common_center(const circle<T>& circle1, const circle<T>& circle2);
   template <typename T> inline bool common_center(const sphere<T>& sphere1, const sphere<T>& circle2);

   template <typename T> inline bool point_in_convex_polygon(const T& px, const T& py, const polygon<T,2>& polygon);
   template <typename T> inline bool point_in_convex_polygon(const point2d<T>& point, const polygon<T,2>& polygon);

   template <typename T> inline bool point_on_polygon_edge(const T& px, const T& py, const polygon<T,2>& polygon);
   template <typename T> inline bool point_on_polygon_edge(const point2d<T>& point, const polygon<T,2>& polygon);

   template <typename T> inline bool point_in_polygon(const T& px, const T& py, const polygon<T,2>& polygon);
   template <typename T> inline bool point_in_polygon(const point2d<T>& point, const polygon<T,2>& polygon);

   template <typename T> inline bool point_in_polygon_winding_number(const T& px, const T& py, const polygon<T,2>& polygon);
   template <typename T> inline bool point_in_polygon_winding_number(const point2d<T>& point, const polygon<T,2>& polygon);

   template <typename T> inline bool convex_quadix(const quadix<T,2>& quadix);
   template <typename T> inline bool convex_quadix(const quadix<T,3>& quadix);

   template <typename T> inline bool is_convex_polygon(const polygon<T,2>& polygon);

   template <typename T> inline polygon<T,2> remove_consecutive_collinear_points(const polygon<T,2>& polygon);

   template <typename T, typename InputIterator, typename OutputIterator>
   inline void remove_consecutive_collinear_points(const InputIterator begin, const InputIterator end, OutputIterator out);

   template <typename T> inline bool convex_vertex(const std::size_t& index, const polygon<T,2>& polygon, const int& polygon_orientation = LeftHandSide);

   template <typename T> inline bool collinear_vertex(const std::size_t& index, const polygon<T,2>& polygon);

   template <typename T> inline bool vertex_is_ear(const std::size_t& index, const polygon<T,2>& polygon);

   template <typename T> inline triangle<T,2> vertex_triangle(const std::size_t& index, const polygon<T,2> polygon);

   template <typename T> inline int polygon_orientation(const polygon<T,2>& polygon);

   template <typename T> inline bool is_equilateral_triangle(const triangle<T,2>& triangle);
   template <typename T> inline bool is_equilateral_triangle(const triangle<T,3>& triangle);

   template <typename T> inline bool is_isosceles_triangle(const triangle<T,2>& triangle);
   template <typename T> inline bool is_isosceles_triangle(const triangle<T,3>& triangle);

   template <typename T> inline bool is_right_triangle(const wykobi::triangle<T,2>& triangle);
   template <typename T> inline bool is_right_triangle(const wykobi::triangle<T,3>& triangle);

   template <typename T> inline bool are_perspective_triangles(const triangle<T,2>& triangle1, const triangle<T,2>& triangle2);
   template <typename T> inline bool are_perspective_triangles(const triangle<T,3>& triangle1, const triangle<T,3>& triangle2);

   template <typename T> inline line<T,2> perspectrix(const triangle<T,2>& triangle1, const triangle<T,2>& triangle2);
   template <typename T> inline line<T,3> perspectrix(const triangle<T,3>& triangle1, const triangle<T,3>& triangle2);

   template <typename T>
   inline void mirror(const T& px, const T& py,
                      const T& x1, const T& y1,
                      const T& x2, const T& y2,
                            T& nx,       T& ny);

   template <typename T>
   inline void mirror(const T& px, const T& py, const T& pz,
                      const T& x1, const T& y1, const T& z1,
                      const T& x2, const T& y2, const T& z2,
                            T& nx,       T& ny,       T& nz);

   template <typename T> inline point2d<T> mirror(const point2d<T>& point, const line<T,2>& mirror_axis);
   template <typename T> inline segment<T,2> mirror(const segment<T,2>& segment, const line<T,2>& mirror_axis);
   template <typename T> inline line<T,2> mirror(const line<T,2>& line, const wykobi::line<T,2>& mirror_axis);
   template <typename T> inline rectangle<T> mirror(const rectangle<T>& rectangle, const line<T,2>& mirror_axis);
   template <typename T> inline triangle<T,2> mirror(const triangle<T,2>& triangle, const line<T,2>& mirror_axis);
   template <typename T> inline quadix<T,2> mirror(const quadix<T,2>& quadix, const line<T,2>& mirror_axis);
   template <typename T> inline circle<T> mirror(const circle<T>& circle, const line<T,2>& mirror_axis);
   template <typename T> inline polygon<T,2> mirror(const polygon<T,2>& polygon, const line<T,2>& mirror_axis);

   template <typename T> inline point3d<T> mirror(const point3d<T>& point, const line<T,3>& mirror_axis);
   template <typename T> inline segment<T,3> mirror(const segment<T,3>& segment, const line<T,3>& mirror_axis);
   template <typename T> inline line<T,3> mirror(const line<T,3>& line, const wykobi::line<T,3>& mirror_axis);
   template <typename T> inline box<T,3> mirror(const box<T,3>& box, const line<T,3>& mirror_axis);
   template <typename T> inline triangle<T,3> mirror(const triangle<T,3>& triangle, const line<T,3>& mirror_axis);
   template <typename T> inline quadix<T,3> mirror(const quadix<T,3>& quadix, const line<T,3>& mirror_axis);
   template <typename T> inline sphere<T> mirror(const sphere<T>& sphere, const line<T,3>& mirror_axis);
   template <typename T> inline polygon<T,3> mirror(const polygon<T,3>& polygon, const line<T,3>& mirror_axis);

   template <typename T> inline point3d<T> mirror(const point3d<T>& point, const plane<T,3>& mirror_plane);
   template <typename T> inline segment<T,3> mirror(const segment<T,3>& segment, const plane<T,3>& mirror_plane);
   template <typename T> inline line<T,3> mirror(const line<T,3>& line, const plane<T,3>& mirror_plane);
   template <typename T> inline box<T,3> mirror(const box<T,3>& box, const plane<T,3>& mirror_plane);
   template <typename T> inline triangle<T,3> mirror(const triangle<T,3>& triangle, const plane<T,3>& mirror_plane);
   template <typename T> inline quadix<T,3> mirror(const quadix<T,3>& quadix, const plane<T,3>& mirror_plane);
   template <typename T> inline sphere<T> mirror(const sphere<T>& sphere, const plane<T,3>& mirror_plane);
   template <typename T> inline polygon<T,3> mirror(const polygon<T,3>& polygon, const plane<T,3>& mirror_plane);

   template <typename T>
   inline void nonsymmetric_mirror(const T& px, const T& py,
                                   const T& x1, const T& y1,
                                   const T& x2, const T& y2,
                                   const T& ratio,
                                         T& nx,       T& ny);

   template <typename T> inline point2d<T> nonsymmetric_mirror(const point2d<T>& point, const T& ratio, const line<T,2>& line);
   template <typename T> inline segment<T,2> nonsymmetric_mirror(const segment<T,2>& segment, const T& ratio, const line<T,2>& line);
   template <typename T> inline rectangle<T> nonsymmetric_mirror(const rectangle<T>& rectangle, const T& ratio, const line<T,2>& line);
   template <typename T> inline triangle<T,2> nonsymmetric_mirror(const triangle<T,2>& triangle, const T& ratio, const line<T,2>& line);
   template <typename T> inline quadix<T,2> nonsymmetric_mirror(const quadix<T,2>& quadix, const T& ratio, const line<T,2>& line);
   template <typename T> inline circle<T> nonsymmetric_mirror(const circle<T>& circle, const T& ratio, const line<T,2>& line);
   template <typename T> inline polygon<T,2> nonsymmetric_mirror(const polygon<T,2>& polygon, const T& ratio, const line<T,2>& line);

   template <typename T> inline point3d<T> nonsymmetric_mirror(const point3d<T>& point, const T& ratio, const plane<T,3>& plane);
   template <typename T> inline segment<T,3> nonsymmetric_mirror(const segment<T,3>& segment, const T& ratio, const plane<T,3>& plane);
   template <typename T> inline box<T,3> nonsymmetric_mirror(const box<T,3>& box, const T& ratio, const plane<T,3>& plane);
   template <typename T> inline triangle<T,3> nonsymmetric_mirror(const triangle<T,3>& triangle, const T& ratio, const plane<T,3>& plane);
   template <typename T> inline quadix<T,3> nonsymmetric_mirror(const quadix<T,3>& quadix, const T& ratio, const plane<T,3>& plane);
   template <typename T> inline circle<T> nonsymmetric_mirror(const sphere<T>& sphere, const T& ratio, const plane<T,3>& plane);
   template <typename T> inline polygon<T,3> nonsymmetric_mirror(const polygon<T,3>& polygon, const T& ratio, const plane<T,3>& plane);

   template <typename T> inline point2d<T> invert_point(const point2d<T>& point, const circle<T>& circle);
   template <typename T> inline point3d<T> invert_point(const point3d<T>& point, const sphere<T>& sphere);

   template <typename T> inline point2d<T> antipodal_point(const point2d<T>& point, const circle<T>& circle);
   template <typename T> inline point3d<T> antipodal_point(const point3d<T>& point, const sphere<T>& sphere);

   template <typename T> inline T distance(const T& x1, const T& y1, const T& x2, const T& y2);
   template <typename T> inline T distance(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2);
   template <typename T> inline T distance(const point2d<T>& point1, const point2d<T>& point2);
   template <typename T> inline T distance(const point3d<T>& point1, const point3d<T>& point2);
   template <typename T> inline T distance(const curve_point<T,2>& point1, const curve_point<T,2>& point2);
   template <typename T> inline T distance(const curve_point<T,3>& point1, const curve_point<T,3>& point2);
   template <typename T> inline T distance(const point2d<T>& point, const segment<T,2>& segment);
   template <typename T> inline T distance(const point3d<T>& point, const segment<T,3>& segment);
   template <typename T> inline T distance(const point2d<T>& point, const rectangle<T>& rectangle);
   template <typename T> inline T distance(const point2d<T>& point, const triangle<T,2>& triangle);
   template <typename T> inline T distance(const point2d<T>& point, const quadix<T,2>& quadix);
   template <typename T> inline T distance(const point2d<T>& point, const ray<T,2>& ray);
   template <typename T> inline T distance(const point3d<T>& point, const ray<T,3>& ray);
   template <typename T> inline T distance(const point3d<T>& point, const plane<T,3>& plane);
   template <typename T> inline T distance(const line<T,2>& line1, const line<T,2>& line2);
   template <typename T> inline T distance(const line<T,3>& line1, const line<T,3>& line2);
   template <typename T> inline T distance(const segment<T,2>& segment1, const segment<T,2>& segment2);
   template <typename T> inline T distance(const segment<T,3>& segment1, const segment<T,3>& segment2);
   template <typename T> inline T distance(const segment<T,2>& segment);
   template <typename T> inline T distance(const segment<T,3>& segment);
   template <typename T> inline T distance(const segment<T,2>& segment, const triangle<T,2>& triangle);
   template <typename T> inline T distance(const segment<T,3>& segment, const triangle<T,3>& triangle);
   template <typename T> inline T distance(const segment<T,2>& segment, const rectangle<T>& rectangle);
   template <typename T> inline T distance(const segment<T,2>& segment, const circle<T>& circle);
   template <typename T> inline T distance(const triangle<T,2>& triangle1, const triangle<T,2>& triangle2);
   template <typename T> inline T distance(const triangle<T,2>& triangle, const rectangle<T>& rectangle);
   template <typename T> inline T distance(const rectangle<T>& rectangle1, const rectangle<T>& rectangle2);
   template <typename T> inline T distance(const triangle<T,2>& triangle, const circle<T>& circle);
   template <typename T> inline T distance(const rectangle<T>& rectangle, const circle<T>& circle);
   template <typename T> inline T distance(const point2d<T>& point, const circle<T>& circle);
   template <typename T> inline T distance(const circle<T>& circle1, const circle<T>& circle2);
   template <typename T> inline T distance(const sphere<T>& sphere1, const sphere<T>& sphere2);

   template <typename T> inline T lay_distance(const T& x1, const T& y1, const T& x2, const T& y2);
   template <typename T> inline T lay_distance(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2);
   template <typename T> inline T lay_distance(const point2d<T>& point1, const point2d<T>& point2);
   template <typename T> inline T lay_distance(const point3d<T>& point1, const point3d<T>& point2);
   template <typename T> inline T lay_distance(const point2d<T>& point, const triangle<T,2>& triangle);
   template <typename T> inline T lay_distance(const point2d<T>& point, const quadix<T,2>& triangle);
   template <typename T> inline T lay_distance(const point2d<T>& point, const ray<T,2>& ray);
   template <typename T> inline T lay_distance(const point3d<T>& point, const ray<T,3>& ray);
   template <typename T> inline T lay_distance(const point3d<T>& point, const plane<T,3>& plane);
   template <typename T> inline T lay_distance(const segment<T,2>& segment1, const segment<T,2>& segment2);
   template <typename T> inline T lay_distance(const segment<T,3>& segment1, const segment<T,3>& segment2);
   template <typename T> inline T lay_distance(const line<T,3>& line1, const line<T,3>& line2);
   template <typename T> inline T lay_distance(const segment<T,2>& segment);
   template <typename T> inline T lay_distance(const segment<T,3>& segment);
   template <typename T> inline T lay_distance(const segment<T,2>& segment, const triangle<T,2>& triangle);
   template <typename T> inline T lay_distance(const segment<T,3>& segment, const triangle<T,3>& triangle);

   template <typename T> inline T manhattan_distance(const T& x1, const T& y1, const T& x2, const T& y2);
   template <typename T> inline T manhattan_distance(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2);
   template <typename T> inline T manhattan_distance(const point2d<T>& point1, const point2d<T>& point2);
   template <typename T> inline T manhattan_distance(const point3d<T>& point1, const point3d<T>& point2);
   template <typename T> inline T manhattan_distance(const point2d<T>& point, const ray<T,2>& ray);
   template <typename T> inline T manhattan_distance(const point3d<T>& point, const ray<T,3>& ray);
   template <typename T> inline T manhattan_distance(const segment<T,2>& segment);
   template <typename T> inline T manhattan_distance(const segment<T,3>& segment);
   template <typename T> inline T manhattan_distance(const circle<T>& circle1, const circle<T>& circle2);

   template <typename T> inline T chebyshev_distance(const T& x1, const T& y1, const T& x2, const T& y2);
   template <typename T> inline T chebyshev_distance(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2);
   template <typename T> inline T chebyshev_distance(const point2d<T>& point1, const point2d<T>& point2);
   template <typename T> inline T chebyshev_distance(const point3d<T>& point1, const point3d<T>& point2);
   template <typename T> inline T chebyshev_distance(const segment<T,2>& segment);
   template <typename T> inline T chebyshev_distance(const segment<T,3>& segment);
   template <typename T> inline T chebyshev_distance(const circle<T>& circle1, const circle<T>& circle2);

   template <typename T> inline T inverse_chebyshev_distance(const T& x1, const T& y1, const T& x2, const T& y2);
   template <typename T> inline T inverse_chebyshev_distance(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2);
   template <typename T> inline T inverse_chebyshev_distance(const point2d<T>& point1, const point2d<T>& point2);
   template <typename T> inline T inverse_chebyshev_distance(const point3d<T>& point1, const point3d<T>& point2);
   template <typename T> inline T inverse_chebyshev_distance(const segment<T,2>& segment);
   template <typename T> inline T inverse_chebyshev_distance(const segment<T,3>& segment);
   template <typename T> inline T inverse_chebyshev_distance(const circle<T>& circle1, const circle<T>& circle2);

   template <typename T> inline point2d<T> minkowski_sum(const point2d<T>& point1, const point2d<T>& point2);
   template <typename T> inline polygon<T,2> minkowski_sum(const rectangle<T>& rectangle1, const rectangle<T>& rectangle2);
   template <typename T> inline polygon<T,2> minkowski_sum(const triangle<T,2>& triangle1, const triangle<T,2>& triangle2);
   template <typename T> inline polygon<T,2> minkowski_sum(const quadix<T,2>& quadix1, const quadix<T,2>& quadix2);
   template <typename T> inline polygon<T,2> minkowski_sum(const circle<T>& triangle, const circle<T>& circle);

   template <typename T> inline polygon<T,2> minkowski_sum(const triangle<T,2>& triangle, const rectangle<T>& rectangle);
   template <typename T> inline polygon<T,2> minkowski_sum(const triangle<T,2>& triangle, const quadix<T,2>& quadix);
   template <typename T> inline polygon<T,2> minkowski_sum(const triangle<T,2>& triangle, const circle<T>& circle);
   template <typename T> inline polygon<T,2> minkowski_sum(const quadix<T,2>& quadix, const circle<T>& circle);
   template <typename T> inline polygon<T,2> minkowski_sum(const quadix<T,2>& quadix, const rectangle<T>& rectangle);
   template <typename T> inline polygon<T,2> minkowski_sum(const rectangle<T>& rectangle, const circle<T>& circle);
   template <typename T> inline polygon<T,2> minkowski_sum(const polygon<T,2>& polygon1, const polygon<T,2>& polygon2);

   template <typename T> inline point2d<T> minkowski_difference(const point2d<T>& point1, const point2d<T>& point2);
   template <typename T> inline polygon<T,2> minkowski_difference(const rectangle<T>& rectangle1, const rectangle<T>& rectangle2);
   template <typename T> inline polygon<T,2> minkowski_difference(const triangle<T,2>& triangle1, const triangle<T,2>& triangle2);
   template <typename T> inline polygon<T,2> minkowski_difference(const quadix<T,2>& quadix1, const quadix<T,2>& quadix2);
   template <typename T> inline polygon<T,2> minkowski_difference(const circle<T>& triangle, const circle<T>& circle);

   template <typename T> inline polygon<T,2> minkowski_difference(const triangle<T,2>& triangle, const rectangle<T>& rectangle);
   template <typename T> inline polygon<T,2> minkowski_difference(const triangle<T,2>& triangle, const quadix<T,2>& quadix);
   template <typename T> inline polygon<T,2> minkowski_difference(const triangle<T,2>& triangle, const circle<T>& circle);
   template <typename T> inline polygon<T,2> minkowski_difference(const quadix<T,2>& quadix, const circle<T>& circle);
   template <typename T> inline polygon<T,2> minkowski_difference(const quadix<T,2>& quadix, const rectangle<T>& rectangle);
   template <typename T> inline polygon<T,2> minkowski_difference(const rectangle<T>& rectangle, const circle<T>& circle);
   template <typename T> inline polygon<T,2> minkowski_difference(const polygon<T,2>& polygon1, const polygon<T,2>& polygon2);

   template <typename T>
   inline T distance_segment_to_segment(const T& x1, const T& y1,
                                        const T& x2, const T& y2,
                                        const T& x3, const T& y3,
                                        const T& x4, const T& y4);

   template <typename T>
   inline T distance_segment_to_segment(const T& x1, const T& y1, const T& z1,
                                        const T& x2, const T& y2, const T& z2,
                                        const T& x3, const T& y3, const T& z3,
                                        const T& x4, const T& y4, const T& z4);

   template <typename T>
   inline T lay_distance_segment_to_segment(const T& x1, const T& y1,
                                            const T& x2, const T& y2,
                                            const T& x3, const T& y3,
                                            const T& x4, const T& y4);


   template <typename T>
   inline T lay_distance_segment_to_segment(const T& x1, const T& y1, const T& z1,
                                            const T& x2, const T& y2, const T& z2,
                                            const T& x3, const T& y3, const T& z3,
                                            const T& x4, const T& y4, const T& z4);

   template <typename T>
   inline T distance_line_to_line(const T& x1, const T& y1,
                                  const T& x2, const T& y2,
                                  const T& x3, const T& y3,
                                  const T& x4, const T& y4);

   template <typename T>
   inline T distance_line_to_line(const T& x1, const T& y1, const T& z1,
                                  const T& x2, const T& y2, const T& z2,
                                  const T& x3, const T& y3, const T& z3,
                                  const T& x4, const T& y4, const T& z4);

   template <typename T>
   inline T lay_distance_line_to_line(const T& x1, const T& y1,
                                      const T& x2, const T& y2,
                                      const T& x3, const T& y3,
                                      const T& x4, const T& y4);

   template <typename T>
   inline T lay_distance_line_to_line(const T& x1, const T& y1, const T& z1,
                                      const T& x2, const T& y2, const T& z2,
                                      const T& x3, const T& y3, const T& z3,
                                      const T& x4, const T& y4, const T& z4);

   template <typename T>
   inline T lay_distance_from_point_to_circle_center(const point2d<T>& point, const circle<T>& circle);

   template <typename T>
   inline T lay_distance_from_point_to_sphere_center(const point3d<T>& point, const sphere<T>& sphere);

   template <typename T>
   inline T distance_from_point_to_circle_center(const point2d<T>& point, const circle<T>& circle);

   template <typename T>
   inline T distance_from_point_to_sphere_center(const point3d<T>& point, const sphere<T>& sphere);

   template <typename T>
   inline T span_length(const rectangle<T>& rectangle);

   template <typename T>
   inline T span_length(const box<T,3>& box);

   template <typename T>
   inline void project_point_t(const T&  srcx, const T&  srcy,
                               const T& destx, const T& desty,
                               const T& t,
                               T& nx, T& ny);

   template <typename T>
   inline void project_point_t(const T&  srcx, const T&  srcy, const T&  srcz,
                               const T& destx, const T& desty, const T& destz,
                               const T& t,
                               T& nx, T& ny, T& nz);

   template <typename T>
   inline void project_point(const T&  srcx, const T&  srcy,
                             const T& destx, const T& desty,
                             const T& dist,
                             T& nx, T& ny);

   template <typename T>
   inline void project_point(const T&  srcx, const T&  srcy, const T&  srcz,
                             const T& destx, const T& desty, const T& destz,
                             const T& dist,
                             T& nx, T& ny, T& nz);

   template <typename T>
   inline void project_point(const T& px, const T& py, const T& angle, const T& distance, T& nx, T& ny);

   template <typename T> inline void project_point0  (const T& px, const T& py, const T& distance, T& nx, T& ny);
   template <typename T> inline void project_point45 (const T& px, const T& py, const T& distance, T& nx, T& ny);
   template <typename T> inline void project_point90 (const T& px, const T& py, const T& distance, T& nx, T& ny);
   template <typename T> inline void project_point135(const T& px, const T& py, const T& distance, T& nx, T& ny);
   template <typename T> inline void project_point180(const T& px, const T& py, const T& distance, T& nx, T& ny);
   template <typename T> inline void project_point225(const T& px, const T& py, const T& distance, T& nx, T& ny);
   template <typename T> inline void project_point270(const T& px, const T& py, const T& distance, T& nx, T& ny);
   template <typename T> inline void project_point315(const T& px, const T& py, const T& distance, T& nx, T& ny);

   template <typename T>
   inline point2d<T> project_point_t(const point2d<T>& source_point,
                                     const point2d<T>& destination_point,
                                     const T& t);

   template <typename T>
   inline point3d<T> project_point_t(const point3d<T>& source_point,
                                     const point3d<T>& destination_point,
                                     const T& t);

   template <typename T>
   inline point2d<T> project_point(const point2d<T>& source_point,
                                   const point2d<T>& destination_point,
                                   const T& distance);

   template <typename T>
   inline point3d<T> project_point(const point3d<T>& source_point,
                                   const point3d<T>& destination_point,
                                   const T& distance);

   template <typename T>
   inline point2d<T> project_point(const point2d<T>& point,
                                   const T& angle,
                                   const T& distance);

   template <typename T> inline point2d<T> project_point0  (const point2d<T>& point, const T& distance);
   template <typename T> inline point2d<T> project_point45 (const point2d<T>& point, const T& distance);
   template <typename T> inline point2d<T> project_point90 (const point2d<T>& point, const T& distance);
   template <typename T> inline point2d<T> project_point135(const point2d<T>& point, const T& distance);
   template <typename T> inline point2d<T> project_point180(const point2d<T>& point, const T& distance);
   template <typename T> inline point2d<T> project_point225(const point2d<T>& point, const T& distance);
   template <typename T> inline point2d<T> project_point270(const point2d<T>& point, const T& distance);
   template <typename T> inline point2d<T> project_point315(const point2d<T>& point, const T& distance);

   template <typename T> inline point2d<T> project_object(const point2d<T>& point, const T& angle, const T& distance);
   template <typename T> inline segment<T,2> project_object(const segment<T,2>& segment, const T& angle, const T& distance);
   template <typename T> inline triangle<T,2> project_object(const triangle<T,2>& triangle, const T& angle, const T& distance);
   template <typename T> inline quadix<T,2> project_object(const quadix<T,2>& quadix, const T& angle, const T& distance);
   template <typename T> inline circle<T> project_object(const circle<T>& circle, const T& angle, const T& distance);
   template <typename T> inline polygon<T,2> project_object(const polygon<T,2>& polygon, const T& angle, const T& distance);

   template <typename T> inline segment<T,2> project_onto_axis(const point2d<T>& point, const line<T,2>& axis);
   template <typename T> inline segment<T,2> project_onto_axis(const triangle<T,2>& triangle, const line<T,2>& axis);
   template <typename T> inline segment<T,2> project_onto_axis(const rectangle<T>& rectangle, const line<T,2>& axis);
   template <typename T> inline segment<T,2> project_onto_axis(const quadix<T,2>& quadix, const line<T,2>& axis);
   template <typename T> inline segment<T,2> project_onto_axis(const circle<T>& circle, const line<T,2>& axis);
   template <typename T> inline segment<T,2> project_onto_axis(const polygon<T,2>& polygon, const line<T,2>& axis);

   template <typename T> inline segment<T,3> project_onto_axis(const point3d<T>& point, const line<T,3>& axis);
   template <typename T> inline segment<T,3> project_onto_axis(const triangle<T,3>& triangle, const line<T,3>& axis);
   template <typename T> inline segment<T,3> project_onto_axis(const box<T,3>& box, const line<T,3>& axis);
   template <typename T> inline segment<T,3> project_onto_axis(const quadix<T,3>& quadix, const line<T,3>& axis);
   template <typename T> inline segment<T,3> project_onto_axis(const sphere<T>& sphere, const line<T,3>& axis);
   template <typename T> inline segment<T,3> project_onto_axis(const polygon<T,3>& polygon, const line<T,3>& axis);

   template <typename T> inline void calculate_bezier_coefficients(const quadratic_bezier<T,2>& bezier, T& ax, T& bx, T& ay, T& by);
   template <typename T> inline void calculate_bezier_coefficients(const quadratic_bezier<T,3>& bezier, T& ax, T& bx, T& ay, T& by, T& az, T& bz);
   template <typename T> inline void calculate_bezier_coefficients(const cubic_bezier<T,2>& bezier, T& ax, T& bx, T& cx, T& ay, T& by, T& cy);
   template <typename T> inline void calculate_bezier_coefficients(const cubic_bezier<T,3>& bezier, T& ax, T& bx, T& cx, T& ay, T& by, T& cy, T& az, T& bz, T& cz);

   template <typename T> inline void calculate_bezier_coefficients(const quadratic_bezier<T,2>& bezier,
                                                                         bezier_coefficients<T,2,eQuadraticBezier>& coeffs);

   template <typename T> inline void calculate_bezier_coefficients(const quadratic_bezier<T,3>& bezier,
                                                                         bezier_coefficients<T,3,eQuadraticBezier>& coeffs);

   template <typename T> inline void calculate_bezier_coefficients(const cubic_bezier<T,2>& bezier,
                                                                         bezier_coefficients<T,2,eCubicBezier>& coeffs);

   template <typename T> inline void calculate_bezier_coefficients(const cubic_bezier<T,3>& bezier,
                                                                         bezier_coefficients<T,3,eCubicBezier>& coeffs);

   template <typename T> inline point2d<T> create_point_on_bezier(const point2d<T>& start_point,
                                                                  const T& ax, const T& bx,
                                                                  const T& ay, const T& by,
                                                                  const T& t);

   template <typename T> inline point3d<T> create_point_on_bezier(const point3d<T>& start_point,
                                                                  const T& ax, const T& bx,
                                                                  const T& ay, const T& by,
                                                                  const T& az, const T& bz,
                                                                  const T& t);

   template <typename T> inline point2d<T> create_point_on_bezier(const point2d<T>& start_point,
                                                                 const T& ax, const T& bx, const T& cx,
                                                                 const T& ay, const T& by, const T& cy, const T& t);

   template <typename T> inline point3d<T> create_point_on_bezier(const point3d<T>& start_point,
                                                                  const T& ax, const T& bx, const T& cx,
                                                                  const T& ay, const T& by, const T& cy,
                                                                  const T& az, const T& bz, const T& cz,
                                                                  const T& t);

   template <typename T> inline point2d<T> create_point_on_bezier(const point2d<T>& start_point,
                                                                  const bezier_coefficients<T,2,eQuadraticBezier>& coeffs,
                                                                  const T& t);

   template <typename T> inline point3d<T> create_point_on_bezier(const point3d<T>& start_point,
                                                                  const bezier_coefficients<T,3,eQuadraticBezier>& coeffs,
                                                                  const T& t);

   template <typename T> inline point2d<T> create_point_on_bezier(const point2d<T>& start_point,
                                                                  const bezier_coefficients<T,2,eCubicBezier>& coeffs,
                                                                  const T& t);

   template <typename T> inline point3d<T> create_point_on_bezier(const point3d<T>& start_point,
                                                                  const bezier_coefficients<T,3,eCubicBezier>& coeffs,
                                                                  const T& t);

   template <typename T, typename OutputIterator> inline void generate_bezier(const quadratic_bezier<T,2>& bezier, OutputIterator out, const std::size_t& point_count = 1000);
   template <typename T, typename OutputIterator> inline void generate_bezier(const quadratic_bezier<T,3>& bezier, OutputIterator out, const std::size_t& point_count = 1000);
   template <typename T, typename OutputIterator> inline void generate_bezier(const cubic_bezier<T,2>& bezier, OutputIterator out, const std::size_t& point_count = 1000);
   template <typename T, typename OutputIterator> inline void generate_bezier(const cubic_bezier<T,3>& bezier, OutputIterator out, const std::size_t& point_count = 1000);

   template <typename T> inline T bezier_curve_length(const quadratic_bezier<T,2>& bezier, const std::size_t& point_count);
   template <typename T> inline T bezier_curve_length(const quadratic_bezier<T,3>& bezier, const std::size_t& point_count);
   template <typename T> inline T bezier_curve_length(const cubic_bezier<T,2>& bezier, const std::size_t& point_count);
   template <typename T> inline T bezier_curve_length(const cubic_bezier<T,3>& bezier, const std::size_t& point_count);

   template <typename T> inline triangle<T,2> bezier_convex_hull(const quadratic_bezier<T,2>& bezier);
   template <typename T> inline quadix<T,2> bezier_convex_hull(const cubic_bezier<T,2>& bezier);

   template <typename T> inline segment<T,2> center_at_location(const segment<T,2>& segment, const T& x, const T& y);
   template <typename T> inline segment<T,3> center_at_location(const segment<T,3>& segment, const T& x, const T& y, const T& z);
   template <typename T> inline triangle<T,2> center_at_location(const triangle<T,2>& triangle, const T& x, const T& y);
   template <typename T> inline rectangle<T> center_at_location(const rectangle<T>& rectangle, const T& x, const T& y);
   template <typename T> inline box<T,3> center_at_location(const box<T,3>& box, const T& x, const T& y, const T& z);
   template <typename T> inline quadix<T,2> center_at_location(const quadix<T,2>& quadix, const T& x, const T& y);
   template <typename T> inline circle<T> center_at_location(const circle<T>& circle, const T& x, const T& y);
   template <typename T> inline polygon<T,2> center_at_location(const polygon<T,2>& polygon, const T& x, const T& y);

   template <typename T> inline segment<T,2> center_at_location(const segment<T,2>& segment, const point2d<T>& center_point);
   template <typename T> inline segment<T,3> center_at_location(const segment<T,3>& segment, const point3d<T>& center_point);
   template <typename T> inline triangle<T,2> center_at_location(const triangle<T,2>& triangle, const point2d<T>& center_point);
   template <typename T> inline rectangle<T> center_at_location(const rectangle<T>& rectangle, const point2d<T>& center_point);
   template <typename T> inline box<T,3> center_at_location(const box<T,3>& box, const point3d<T>& center_point);
   template <typename T> inline quadix<T,2> center_at_location(const quadix<T,2>& quadix, const point2d<T>& center_point);
   template <typename T> inline circle<T> center_at_location(const circle<T>& circle, const point2d<T>& center_point);
   template <typename T> inline polygon<T,2> center_at_location(const polygon<T,2>& polygon, const point2d<T>& center_point);

   template <typename T> inline void shorten_segment(T& x1, T& y1, T& x2, T& y2, const T& amount);
   template <typename T> inline void shorten_segment(T& x1, T& y1, T& z1, T& x2, T& y2, T& z2, const T& amount);
   template <typename T> inline segment<T,2> shorten_segment(const segment<T,2>& segment, const T& amount);
   template <typename T> inline segment<T,3> shorten_segment(const segment<T,3>& segment, const T& amount);

   template <typename T> inline void lengthen_segment(T& x1, T& y1, T& x2, T& y2, const T& amount);
   template <typename T> inline void lengthen_segment(T& x1, T& y1, T& z1, T& x2, T& y2, T& z2, const T& amount);
   template <typename T> inline segment<T,2> lengthen_segment(const segment<T,2>& segment, const T& amount);
   template <typename T> inline segment<T,3> lengthen_segment(const segment<T,3>& segment, const T& amount);

   template <typename T> inline int out_code(const point2d<T>& point, const rectangle<T>& rectangle);

   template <typename T> inline bool clip(const T& x1, const T& y1,
                                          const T& x2, const T& y2,
                                          const T& x3, const T& y3,
                                          const T& x4, const T& y4,
                                                T& cx1,      T& cy1,
                                                T& cx2,      T& cy2);

   template <typename T> inline bool clip(const T& x1, const T& y1, const T& z1,
                                          const T& x2, const T& y2, const T& z2,
                                          const T& x3, const T& y3, const T& z3,
                                          const T& x4, const T& y4, const T& z4,
                                                T& cx1,      T& cy1,      T& cz1,
                                                T& cx2,      T& cy2,      T& cz2);

   template <typename T> inline bool clip(const segment<T,2>& src_segment, const rectangle<T>&  rectangle,  segment<T,2>& csegment);
   template <typename T> inline bool clip(const segment<T,2>& src_segment, const triangle<T,2>& triangle,   segment<T,2>& csegment);
   template <typename T> inline bool clip(const segment<T,2>& src_segment, const quadix<T,2>&   quadix,     segment<T,2>& csegment);
   template <typename T> inline bool clip(const segment<T,2>& src_segment, const circle<T>&     circle,     segment<T,2>& csegment);
   template <typename T> inline bool clip(const rectangle<T>&  rectangle1, const rectangle<T>&  rectangle2, rectangle<T>& crectangle);
   template <typename T> inline bool clip(const box<T,3>&              box1, const box<T,3>&        box2,       box<T,3>&       cbox);

   template <typename T> inline T area(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);
   template <typename T> inline T area(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3);
   template <typename T> inline T area(const triangle<T,2>& triangle);
   template <typename T> inline T area(const triangle<T,3>& triangle);
   template <typename T> inline T area(const quadix<T,2>& quadix);
   template <typename T> inline T area(const quadix<T,3>& quadix);
   template <typename T> inline T area(const rectangle<T>& rectangle);
   template <typename T> inline T area(const circle<T>& circle);
   template <typename T> inline T area(const polygon<T,2>& polygon);

   template <typename T> inline T perimeter(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);
   template <typename T> inline T perimeter(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3);
   template <typename T> inline T perimeter(const triangle<T,2>& triangle);
   template <typename T> inline T perimeter(const triangle<T,3>& triangle);
   template <typename T> inline T perimeter(const quadix<T,2>& quadix);
   template <typename T> inline T perimeter(const quadix<T,3>& quadix);
   template <typename T> inline T perimeter(const rectangle<T>& rectangle);
   template <typename T> inline T perimeter(const circle<T>& circle);
   template <typename T> inline T perimeter(const polygon<T,2>& polygon);

   template <typename T> inline void rotate(const T& rotation_angle, const T& x, const T& y, T& nx, T& ny);
   template <typename T> inline void rotate(const T& rotation_angle, const T& x, const T& y, const T& ox, const T& oy, T& nx, T& ny);

   template <typename T> inline point2d<T> rotate(const T& rotation_angle, const point2d<T>& point);
   template <typename T> inline point2d<T> rotate(const T& rotation_angle, const point2d<T>& point, const point2d<T>& opoint);

   template <typename T> inline segment<T,2> rotate(const T& rotation_angle, const segment<T,2>& segment);
   template <typename T> inline segment<T,2> rotate(const T& rotation_angle, const segment<T,2>& segment, const point2d<T>& opoint);

   template <typename T> inline triangle<T,2> rotate(const T& rotation_angle, const triangle<T,2>& triangle);
   template <typename T> inline triangle<T,2> rotate(const T& rotation_angle, const triangle<T,2>& triangle, const point2d<T>& opoint);

   template <typename T> inline quadix<T,2> rotate(const T& rotation_angle, const quadix<T,2>& quadix);
   template <typename T> inline quadix<T,2> rotate(const T& rotation_angle, const quadix<T,2>& quadix, const point2d<T>& opoint);

   template <typename T> inline polygon<T,2> rotate(const T& rotation_angle, const polygon<T,2>& polygon);
   template <typename T> inline polygon<T,2> rotate(const T& rotation_angle, const polygon<T,2>& polygon, const point2d<T>& opoint);

   template <typename T> inline void fast_rotate(const trig_luts<T>& lut,
                                                 const int rotation_angle,
                                                 const T& x, const T& y, T& nx, T& ny);

   template <typename T> inline void fast_rotate(const trig_luts<T>& lut,
                                                 const int rotation_angle,
                                                 const T& x, const T& y, const T& ox, const T& oy, T& nx, T& ny);

   template <typename T> inline point2d<T> fast_rotate(const trig_luts<T>& lut, const int rotation_angle, const point2d<T>& point);
   template <typename T> inline point2d<T> fast_rotate(const trig_luts<T>& lut, const int rotation_angle, const point2d<T>& point, const point2d<T>& opoint);

   template <typename T> inline segment<T,2> fast_rotate(const trig_luts<T>& lut, const int rotation_angle, const segment<T,2>& segment);
   template <typename T> inline segment<T,2> fast_rotate(const trig_luts<T>& lut, const int rotation_angle, const segment<T,2>& segment, const point2d<T>& opoint);

   template <typename T> inline triangle<T,2> fast_rotate(const trig_luts<T>& lut, const int rotation_angle, const triangle<T,2>& triangle);
   template <typename T> inline triangle<T,2> fast_rotate(const trig_luts<T>& lut, const int rotation_angle, const triangle<T,2>& triangle, const point2d<T>& opoint);

   template <typename T> inline quadix<T,2> fast_rotate(const trig_luts<T>& lut, const int rotation_angle, const quadix<T,2>& quadix);
   template <typename T> inline quadix<T,2> fast_rotate(const trig_luts<T>& lut, const int rotation_angle, const quadix<T,2>& quadix, const point2d<T>& opoint);

   template <typename T> inline polygon<T,2> fast_rotate(const trig_luts<T>& lut, const int rotation_angle, const polygon<T,2>& polygon);
   template <typename T> inline polygon<T,2> fast_rotate(const trig_luts<T>& lut, const int rotation_angle, const polygon<T,2>& polygon, const point2d<T>& opoint);

   template <typename T> inline void fast_rotate(const trig_luts<T>& lut,
                                                 const int rx, const int ry, const int rz,
                                                 const T& x, const T& y, const T& z, T& nx, T& ny, T& nz);

   template <typename T> inline void fast_rotate(const trig_luts<T>& lut,
                                                 const int rx, const int ry, const int rz,
                                                 const T& x, const T& y, const T& z, const T& ox, const T& oy, const T& oz, T& nx, T& ny, T& nz);

   template <typename T> inline point3d<T> fast_rotate(const trig_luts<T>& lut, const int rx, const int ry, const int rz, const point3d<T>& point);
   template <typename T> inline point3d<T> fast_rotate(const trig_luts<T>& lut, const int rx, const int ry, const int rz, const point3d<T>& point, const point3d<T>& opoint);

   template <typename T> inline segment<T,3> fast_rotate(const trig_luts<T>& lut, const int rx, const int ry, const int rz, const segment<T,3>& segment);
   template <typename T> inline segment<T,3> fast_rotate(const trig_luts<T>& lut, const int rx, const int ry, const int rz, const segment<T,3>& segment, const point3d<T>& opoint);

   template <typename T> inline triangle<T,3> fast_rotate(const trig_luts<T>& lut, const int rx, const int ry, const int rz, const triangle<T,3>& triangle);
   template <typename T> inline triangle<T,3> fast_rotate(const trig_luts<T>& lut, const int rx, const int ry, const int rz, const triangle<T,3>& triangle, const point3d<T>& opoint);

   template <typename T> inline quadix<T,3> fast_rotate(const trig_luts<T>& lut, const int rx, const int ry, const int rz, const quadix<T,3>& quadix);
   template <typename T> inline quadix<T,3> fast_rotate(const trig_luts<T>& lut, const int rx, const int ry, const int rz, const quadix<T,3>& quadix, const point3d<T>& opoint);

   template <typename T> inline polygon<T,3> fast_rotate(const trig_luts<T>& lut, const int rx, const int ry, const int rz, const polygon<T,3>& polygon);
   template <typename T> inline polygon<T,3> fast_rotate(const trig_luts<T>& lut, const int rx, const int ry, const int rz, const polygon<T,3>& polygon, const point3d<T>& opoint);


   template <typename T> inline point2d<T> translate(const T& dx, const T& dy, const point2d<T>& point);
   template <typename T> inline line<T,2> translate(const T& dx, const T& dy, const line<T,2>& line);
   template <typename T> inline segment<T,2> translate(const T& dx, const T& dy, const segment<T,2>& segment);
   template <typename T> inline triangle<T,2> translate(const T& dx, const T& dy, const triangle<T,2>& triangle);
   template <typename T> inline quadix<T,2> translate(const T& dx, const T& dy, const quadix<T,2>& quadix);
   template <typename T> inline rectangle<T> translate(const T& dx, const T& dy, const rectangle<T>& rectangle);
   template <typename T> inline circle<T> translate(const T& dx, const T& dy, const circle<T>& circle);
   template <typename T> inline polygon<T,2> translate(const T& dx, const T& dy, const polygon<T,2>& polygon);

   template <typename T> inline point2d<T> translate(const T& delta, const point2d<T>& point);
   template <typename T> inline line<T,2> translate(const T& delta, const line<T,2>& line);
   template <typename T> inline segment<T,2> translate(const T& delta, const segment<T,2>& segment);
   template <typename T> inline triangle<T,2> translate(const T& delta, const triangle<T,2>& triangle);
   template <typename T> inline quadix<T,2> translate(const T& delta, const quadix<T,2>& quadix);
   template <typename T> inline rectangle<T> translate(const T& delta, const rectangle<T>& rectangle);
   template <typename T> inline circle<T> translate(const T& delta, const circle<T>& circle);
   template <typename T> inline polygon<T,2> translate(const T& delta, const polygon<T,2>& polygon);

   template <typename T> inline point2d<T> translate(const vector2d<T>& v, const point2d<T>& point);
   template <typename T> inline line<T,2> translate(const vector2d<T>& v, const line<T,2>& line);
   template <typename T> inline segment<T,2> translate(const vector2d<T>& v, const segment<T,2>& segment);
   template <typename T> inline triangle<T,2> translate(const vector2d<T>& v, const triangle<T,2>& triangle);
   template <typename T> inline quadix<T,2> translate(const vector2d<T>& v, const quadix<T,2>& quadix);
   template <typename T> inline rectangle<T> translate(const vector2d<T>& v, const rectangle<T>& rectangle);
   template <typename T> inline circle<T> translate(const vector2d<T>& v, const circle<T>& circle);
   template <typename T> inline polygon<T,2> translate(const vector2d<T>& v, const polygon<T,2>& polygon);

   template <typename T> inline point3d<T> translate(const T& dx, const T& dy, const T& dz, const point3d<T>& point);
   template <typename T> inline line<T,3> translate(const T& dx, const T& dy, const T& dz, const line<T,3>& line);
   template <typename T> inline segment<T,3> translate(const T& dx, const T& dy, const T& dz, const segment<T,3>& segment);
   template <typename T> inline triangle<T,3> translate(const T& dx, const T& dy, const T& dz, const triangle<T,3>& triangle);
   template <typename T> inline quadix<T,3> translate(const T& dx, const T& dy, const T& dz, const quadix<T,3>& quadix);
   template <typename T> inline box<T,3> translate(const T& dx, const T& dy, const T& dz, const box<T,3>& box);
   template <typename T> inline sphere<T> translate(const T& dx, const T& dy, const T& dz, const sphere<T>& sphere);
   template <typename T> inline polygon<T,3> translate(const T& dx, const T& dy, const T& dz, const polygon<T,3>& polygon);

   template <typename T> inline point3d<T> translate(const T& delta, const point3d<T>& point);
   template <typename T> inline line<T,3> translate(const T& delta, const line<T,3>& line);
   template <typename T> inline segment<T,3> translate(const T& delta, const segment<T,3>& segment);
   template <typename T> inline triangle<T,3> translate(const T& delta, const triangle<T,3>& triangle);
   template <typename T> inline quadix<T,3> translate(const T& delta, const quadix<T,3>& quadix);
   template <typename T> inline box<T,3> translate(const T& delta, const box<T,3>& box);
   template <typename T> inline sphere<T> translate(const T& delta, const sphere<T>& sphere);
   template <typename T> inline polygon<T,3> translate(const T& delta, const polygon<T,3>& polygon);

   template <typename T> inline point3d<T> translate(const vector3d<T>& v, const point3d<T>& point);
   template <typename T> inline line<T,3> translate(const vector3d<T>& v, const line<T,3>& line);
   template <typename T> inline segment<T,3> translate(const vector3d<T>& v, const segment<T,3>& segment);
   template <typename T> inline triangle<T,3> translate(const vector3d<T>& v, const triangle<T,3>& triangle);
   template <typename T> inline quadix<T,3> translate(const vector3d<T>& v, const quadix<T,3>& quadix);
   template <typename T> inline box<T,3> translate(const vector3d<T>& v, const box<T,3>& box);
   template <typename T> inline sphere<T> translate(const vector3d<T>& v, const sphere<T>& sphere);
   template <typename T> inline polygon<T,3> translate(const vector3d<T>& v, const polygon<T,3>& polygon);

   template <typename T> inline point2d<T> scale(const T& dx, const T& dy, const point2d<T>& point);
   template <typename T> inline line<T,2> scale(const T& dx, const T& dy, const line<T,2>& line);
   template <typename T> inline segment<T,2> scale(const T& dx, const T& dy, const segment<T,2>& segment);
   template <typename T> inline triangle<T,2> scale(const T& dx, const T& dy, const triangle<T,2>& triangle);
   template <typename T> inline quadix<T,2> scale(const T& dx, const T& dy, const quadix<T,2>& quadix);
   template <typename T> inline rectangle<T> scale(const T& dx, const T& dy, const rectangle<T>& rectangle);
   template <typename T> inline circle<T> scale(const T& dr, const circle<T>& circle);
   template <typename T> inline polygon<T,2> scale(const T& dx, const T& dy, const polygon<T,2>& polygon);

   template <typename T> inline point3d<T> scale(const T& dx, const T& dy, const T& dz, const point3d<T>& point);
   template <typename T> inline line<T,3> scale(const T& dx, const T& dy, const T& dz, const line<T,3>& line);
   template <typename T> inline segment<T,3> scale(const T& dx, const T& dy, const T& dz, const segment<T,3>& segment);
   template <typename T> inline triangle<T,3> scale(const T& dx, const T& dy, const T& dz, const triangle<T,3>& triangle);
   template <typename T> inline quadix<T,3> scale(const T& dx, const T& dy, const T& dz, const quadix<T,3>& quadix);
   template <typename T> inline box<T,3> scale(const T& dx, const T& dy, const T& dz, const box<T,3>& box);
   template <typename T> inline sphere<T> scale(const T& dr, const sphere<T>& sphere);
   template <typename T> inline polygon<T,3> scale(const T& dx, const T& dy, const T& dz, const polygon<T,3>& polygon);

   template <typename T> inline rectangle<T> aabb(const segment<T,2>& segment);
   template <typename T> inline rectangle<T> aabb(const triangle<T,2>& triangle);
   template <typename T> inline rectangle<T> aabb(const rectangle<T>& rectangle);
   template <typename T> inline rectangle<T> aabb(const quadix<T,2>& quadix);
   template <typename T> inline rectangle<T> aabb(const circle<T>& circle);
   template <typename T> inline rectangle<T> aabb(const polygon<T,2>& polygon);

   template <typename T> inline void aabb(const segment<T,2>& segment,   T& x1, T& y1, T& x2, T& y2);
   template <typename T> inline void aabb(const triangle<T,2>& triangle, T& x1, T& y1, T& x2, T& y2);
   template <typename T> inline void aabb(const rectangle<T>& rectangle, T& x1, T& y1, T& x2, T& y2);
   template <typename T> inline void aabb(const quadix<T,2>& quadix,     T& x1, T& y1, T& x2, T& y2);
   template <typename T> inline void aabb(const circle<T>& circle,       T& x1, T& y1, T& x2, T& y2);
   template <typename T> inline void aabb(const polygon<T,2>& polygon,   T& x1, T& y1, T& x2, T& y2);

   template <typename T> inline box<T,3> aabb(const segment<T,3>& segment);
   template <typename T> inline box<T,3> aabb(const triangle<T,3>& triangle);
   template <typename T> inline box<T,3> aabb(const box<T,3>& rectangle);
   template <typename T> inline box<T,3> aabb(const quadix<T,3>& quadix);
   template <typename T> inline box<T,3> aabb(const sphere<T>& sphere);
   template <typename T> inline box<T,3> aabb(const polygon<T,3>& polygon);

   template <typename T> inline void aabb(const segment<T,3>& segment,   T& x1, T& y1, T& z1, T& x2, T& y2, T& z2);
   template <typename T> inline void aabb(const triangle<T,3>& triangle, T& x1, T& y1, T& z1, T& x2, T& y2, T& z2);
   template <typename T> inline void aabb(const box<T,3>& box,             T& x1, T& y1, T& z1, T& x2, T& y2, T& z2);
   template <typename T> inline void aabb(const quadix<T,3>& quadix,     T& x1, T& y1, T& z1, T& x2, T& y2, T& z2);
   template <typename T> inline void aabb(const sphere<T>& sphere,       T& x1, T& y1, T& z1, T& x2, T& y2, T& z2);
   template <typename T> inline void aabb(const polygon<T,3>& polygon,   T& x1, T& y1, T& z1, T& x2, T& y2, T& z2);

   template <typename T> inline rectangle<T> update_rectangle(const rectangle<T>& rectangle, point2d<T>& point);
   template <typename T> inline box<T,3> update_box(const box<T,3>& box, point3d<T>& point);
   template <typename T> inline circle<T> update_circle(const circle<T>& circle, point2d<T>& point);
   template <typename T> inline sphere<T> update_sphere(const sphere<T>& sphere, point3d<T>& point);

   template <typename T> inline point2d<T> generate_point_on_segment(const segment<T,2>& segment, const T& t);
   template <typename T> inline point3d<T> generate_point_on_segment(const segment<T,3>& segment, const T& t);
   template <typename T> inline point2d<T> generate_point_on_ray(const ray<T,2>& ray, const T& t);
   template <typename T> inline point3d<T> generate_point_on_ray(const ray<T,3>& ray, const T& t);

   template <typename T> inline T generate_random_value(const T& range);

   template <typename T> inline point2d<T> generate_random_point(const T& dx, const  T& dy);
   template <typename T> inline point3d<T> generate_random_point(const T& dx, const T& dy, const T& dz);
   template <typename T> inline point2d<T> generate_random_point(const segment<T,2>& segment);
   template <typename T> inline point3d<T> generate_random_point(const segment<T,3>& segment);
   template <typename T> inline point2d<T> generate_random_point(const triangle<T,2>& triangle);
   template <typename T> inline point3d<T> generate_random_point(const triangle<T,3>& triangle);
   template <typename T> inline point2d<T> generate_random_point(const quadix<T,2>& quadix);
   template <typename T> inline point3d<T> generate_random_point(const quadix<T,3>& quadix);
   template <typename T> inline point2d<T> generate_random_point(const rectangle<T>& rectangle);
   template <typename T> inline point3d<T> generate_random_point(const box<T,3>& box);

   template <typename T, typename OutputIterator> inline void generate_random_points(const T& x1, const T& y1, const  T& x2, const  T& y2, const std::size_t& point_count, OutputIterator out);
   template <typename T, typename OutputIterator> inline void generate_random_points(const T& x1, const T& y1, const T& z1, const T& x2, const  T& y2, const  T& z2, const std::size_t& point_count, OutputIterator out);
   template <typename T, typename OutputIterator> inline void generate_random_points(const rectangle<T>& rectangle, const std::size_t& point_count, OutputIterator out);
   template <typename T, typename OutputIterator> inline void generate_random_points(const box<T,3>& box,           const std::size_t& point_count, OutputIterator out);
   template <typename T, typename OutputIterator> inline void generate_random_points(const segment<T,2>& segment,   const std::size_t& point_count, OutputIterator out);
   template <typename T, typename OutputIterator> inline void generate_random_points(const segment<T,3>& segment,   const std::size_t& point_count, OutputIterator out);
   template <typename T, typename OutputIterator> inline void generate_random_points(const triangle<T,2>& triangle, const std::size_t& point_count, OutputIterator out);
   template <typename T, typename OutputIterator> inline void generate_random_points(const triangle<T,3>& triangle, const std::size_t& point_count, OutputIterator out);
   template <typename T, typename OutputIterator> inline void generate_random_points(const quadix<T,2>& quadix,     const std::size_t& point_count, OutputIterator out);
   template <typename T, typename OutputIterator> inline void generate_random_points(const quadix<T,3>& quadix,     const std::size_t& point_count, OutputIterator out);
   template <typename T, typename OutputIterator> inline void generate_random_points(const circle<T>& circle,       const std::size_t& point_count, OutputIterator out);

   template <typename T> inline void generate_random_object(const T& x1, const T& y1, const T& x2, const T& y2, segment<T,2>& segment);
   template <typename T> inline void generate_random_object(const T& x1, const T& y1, const T& x2, const T& y2, rectangle<T>& rectangle);
   template <typename T> inline void generate_random_object(const T& x1, const T& y1, const T& x2, const T& y2, triangle<T,2>& triangle);
   template <typename T> inline void generate_random_object(const T& x1, const T& y1, const T& x2, const T& y2, quadix<T,2>& quadix);
   template <typename T> inline void generate_random_object(const T& x1, const T& y1, const T& x2, const T& y2, circle<T>& circle);
   template <typename T> inline void generate_random_object(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2, box<T,3>& box);

   template <typename T>
   inline triangle<T,2> right_shift(const triangle<T,2>& triangle, const std::size_t& shift);

   template <typename T>
   inline triangle<T,3> right_shift(const triangle<T,3>& triangle, const std::size_t& shift);

   template <typename T>
   inline quadix<T,2> right_shift(const quadix<T,2>& quadix, const std::size_t& shift);

   template <typename T>
   inline quadix<T,3> right_shift(const quadix<T,3>& quadix, const std::size_t& shift);

   template <typename T> inline T vector_norm(const vector2d<T>& v);
   template <typename T> inline T vector_norm(const vector3d<T>& v);

   template <typename T> inline vector2d<T> normalize(const vector2d<T>& v);
   template <typename T> inline vector3d<T> normalize(const vector3d<T>& v);

   template <typename T> inline vector2d<T> perpendicular(const vector2d<T>& v);
   template <typename T> inline vector3d<T> perpendicular(const vector3d<T>& v);

   template <typename T> inline vector2d<T> operator+(const vector2d<T>& v1, const vector2d<T>& v2);
   template <typename T> inline vector3d<T> operator+(const vector3d<T>& v1, const vector3d<T>& v2);

   template <typename T> inline vector2d<T> operator-(const vector2d<T>& v1, const vector2d<T>& v2);
   template <typename T> inline vector3d<T> operator-(const vector3d<T>& v1, const vector3d<T>& v2);

   template <typename T> inline T           operator*(const vector2d<T>& v1, const vector2d<T>& v2);
   template <typename T> inline vector3d<T> operator*(const vector3d<T>& v1, const vector3d<T>& v2);

   template <typename T> inline T dot_product(const vector2d<T>& v1, const vector2d<T>& v2);
   template <typename T> inline T dot_product(const vector3d<T>& v1, const vector3d<T>& v2);

   template <typename T> inline T perpendicular_product(const vector2d<T>& v1, const vector2d<T>& v2);
   template <typename T> inline T triple_product(const vector3d<T>& v1, const vector3d<T>& v2, const vector3d<T>& v3);

   template <typename T> inline vector2d<T> operator*(const vector2d<T>& v1, const T& scale);
   template <typename T> inline vector3d<T> operator*(const vector3d<T>& v1, const T& scale);
   template <typename T> inline vector2d<T> operator*(const T& scale, const vector2d<T>& v1);
   template <typename T> inline vector3d<T> operator*(const T& scale, const vector3d<T>& v1);

   template <typename T> inline vector2d<T> operator/(const vector2d<T>& v1, const T& scale);
   template <typename T> inline vector3d<T> operator/(const vector3d<T>& v1, const T& scale);

   template <typename T> inline point2d<T> operator*(const point2d<T>& point, const T& scale);
   template <typename T> inline point3d<T> operator*(const point3d<T>& point, const T& scale);
   template <typename T> inline point2d<T> operator*(const T& scale, const point2d<T>& point);
   template <typename T> inline point3d<T> operator*(const T& scale, const point3d<T>& point);

   template <typename T> inline point2d<T> operator+(const point2d<T>& point, const vector2d<T>& v);
   template <typename T> inline point2d<T> operator+(const vector2d<T>& v, const point2d<T>& point);

   template <typename T> inline point3d<T> operator+(const point3d<T>& point, const vector3d<T>& v);
   template <typename T> inline point3d<T> operator+(const vector3d<T>& v, const point3d<T>& point);

   template <typename T> inline vector2d<T> operator-(const point2d<T>& p1, const point2d<T>& p2);
   template <typename T> inline vector3d<T> operator-(const point3d<T>& p1, const point3d<T>& p2);

   template <typename T> inline point2d<T> operator+(const point2d<T>& p1, const point2d<T>& p2);
   template <typename T> inline point3d<T> operator+(const point3d<T>& p1, const point3d<T>& p2);

   template <typename T> inline bool is_equal(const T& val1, const T& val2, const T& epsilon);
   template <typename T> inline bool is_equal(const point2d<T>& point1, const point2d<T>& point2, const T& epsilon);
   template <typename T> inline bool is_equal(const point3d<T>& point1, const point3d<T>& point2, const T& epsilon);

   template <typename T> inline bool is_equal(const T& val1, const T& val2);
   template <typename T> inline bool is_equal(const point2d<T>& point1, const point2d<T>& point2);
   template <typename T> inline bool is_equal(const point3d<T>& point1, const point3d<T>& point2);

   template <typename T> inline bool is_equal(const rectangle<T>& rectangle1, const rectangle<T>& rectangle2);
   template <typename T> inline bool is_equal(const circle<T>& circle1, const circle<T>& circle2);

   template <typename T> inline bool is_equal(const box<T,3>& box1, const box<T,3>& box2);
   template <typename T> inline bool is_equal(const sphere<T>& sphere1, const sphere<T>& sphere2);

   template <typename T> inline bool not_equal(const T& val1, const T& val2, const T& epsilon);
   template <typename T> inline bool not_equal(const point2d<T>& point1, const point2d<T>& point2, const T& epsilon);
   template <typename T> inline bool not_equal(const point3d<T>& point1, const point3d<T>& point2, const T& epsilon);

   template <typename T> inline bool not_equal(const T& val1, const T& val2);
   template <typename T> inline bool not_equal(const point2d<T>& point1, const point2d<T>& point2);
   template <typename T> inline bool not_equal(const point3d<T>& point1, const point3d<T>& point2);

   template <typename T> inline bool not_equal(const rectangle<T>& rectangle1, const rectangle<T>& rectangle2);
   template <typename T> inline bool not_equal(const circle<T>& circle1, const circle<T>& circle2);

   template <typename T> inline bool not_equal(const box<T,3>& box1, const box<T,3>& box2);
   template <typename T> inline bool not_equal(const sphere<T>& sphere1, const sphere<T>& sphere2);

   template <typename T> inline bool less_than_or_equal(const T& val1, const T& val2, const T& epsilon);
   template <typename T> inline bool less_than_or_equal(const T& val1, const T& val2);

   template <typename T> inline bool greater_than_or_equal(const T& val1, const T& val2, const T& epsilon);
   template <typename T> inline bool greater_than_or_equal(const T& val1, const T& val2);

   template <typename T> inline bool operator < (const point2d<T>& point1, const point2d<T>& point2);
   template <typename T> inline bool operator < (const point3d<T>& point1, const point3d<T>& point2);
   template <typename T> inline bool operator > (const point2d<T>& point1, const point2d<T>& point2);
   template <typename T> inline bool operator > (const point3d<T>& point1, const point3d<T>& point2);

   template <typename T> inline bool operator == (const point2d<T>& point1, const point2d<T>& point2);
   template <typename T> inline bool operator == (const point3d<T>& point1, const point3d<T>& point2);

   template <typename T> inline bool is_degenerate(const T& x1, const T& y1, const T& x2, const T& y2);
   template <typename T> inline bool is_degenerate(const segment<T,2>& segment);
   template <typename T> inline bool is_degenerate(const line<T,2>& line);

   template <typename T> inline bool is_degenerate(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2);
   template <typename T> inline bool is_degenerate(const segment<T,3>& segment);
   template <typename T> inline bool is_degenerate(const line<T,3>& line);

   template <typename T> inline bool is_degenerate(const triangle<T,2>& triangle);
   template <typename T> inline bool is_degenerate(const triangle<T,3>& triangle);

   template <typename T> inline bool is_degenerate(const quadix<T,2>& quadix);
   template <typename T> inline bool is_degenerate(const quadix<T,3>& quadix);

   template <typename T> inline bool is_degenerate(const rectangle<T>& rectangle);
   template <typename T> inline bool is_degenerate(const circle<T>& circle);
   template <typename T> inline bool is_degenerate(const sphere<T>& sphere);
   template <typename T> inline bool is_degenerate(const circular_arc<T>& arc);

   template <typename T> inline point2d<T> degenerate_point2d();
   template <typename T> inline point3d<T> degenerate_point3d();
   template <typename T> inline vector2d<T> degenerate_vector2d();
   template <typename T> inline vector3d<T> degenerate_vector3d();
   template <typename T> inline ray<T,2> degenerate_ray2d();
   template <typename T> inline ray<T,3> degenerate_ray3d();
   template <typename T> inline line<T,2> degenerate_line2d();
   template <typename T> inline line<T,3> degenerate_line3d();
   template <typename T> inline segment<T,2> degenerate_segment2d();
   template <typename T> inline segment<T,3> degenerate_segment3d();
   template <typename T> inline triangle<T,2> degenerate_triangle2d();
   template <typename T> inline triangle<T,3> degenerate_triangle3d();
   template <typename T> inline quadix<T,2> degenerate_quadix2d();
   template <typename T> inline quadix<T,3> degenerate_quadix3d();
   template <typename T> inline rectangle<T> degenerate_rectangle();
   template <typename T> inline circle<T> degenerate_circle();
   template <typename T> inline sphere<T> degenerate_sphere();

   template <typename T> inline point2d<T> positive_infinite_point2d();
   template <typename T> inline point2d<T> negative_infinite_point2d();
   template <typename T> inline point3d<T> positive_infinite_point3d();
   template <typename T> inline point3d<T> negative_infinite_point3d();

   template <typename T> inline void swap(point2d<T>& point1, point2d<T>& point2);
   template <typename T> inline void swap(point3d<T>& point1, point3d<T>& point2);

   template <typename T> inline point2d<T> make_point(const T& x, const T& y);
   template <typename T> inline point3d<T> make_point(const T& x, const T& y, const T& z);

   template <typename T> inline point2d<T> make_point(const point3d<T> point);
   template <typename T> inline point3d<T> make_point(const point2d<T> point, const T& z = T(0.0));

   template <typename T> inline point2d<T> make_point(const circle<T>& circle);
   template <typename T> inline point3d<T> make_point(const sphere<T>& sphere);

   template <typename T> inline vector2d<T> make_vector(const T& x, const T& y);
   template <typename T> inline vector3d<T> make_vector(const T& x, const T& y, const T& z);

   template <typename T> inline vector2d<T> make_vector(const vector3d<T> v);
   template <typename T> inline vector3d<T> make_vector(const vector2d<T> v, const T& z = T(0.0));

   template <typename T> inline vector2d<T> make_vector(const point2d<T> point);
   template <typename T> inline vector3d<T> make_vector(const point3d<T> point);

   template <typename T> inline ray<T,2> make_ray(const T& ox, const T& oy, const T& dir_x, const T& dir_y);
   template <typename T> inline ray<T,3> make_ray(const T& ox, const T& oy, const T& oz, const T& dir_x, const T& dir_y, const T& dir_z);

   template <typename T> inline ray<T,2> make_ray(const point2d<T>& origin, const vector2d<T>& direction);
   template <typename T> inline ray<T,3> make_ray(const point3d<T>& origin, const vector3d<T>& direction);

   template <typename T> inline ray<T,2> make_ray(const point2d<T>& origin, const T& bearing);

   template <typename T> inline curve_point<T,2> make_curve_point(const T& x, const T& y, const T& t);
   template <typename T> inline curve_point<T,3> make_curve_point(const T& x, const T& y, const T& z, const T& t);

   template <typename T> inline curve_point<T,2> make_curve_point(const point2d<T>& point, const T& t);
   template <typename T> inline curve_point<T,3> make_curve_point(const point3d<T>& point, const T& t);

   template <typename T> inline segment<T,2> make_segment(const T& x1, const T& y1, const T& x2, const T& y2);
   template <typename T> inline segment<T,3> make_segment(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2);

   template <typename T> inline segment<T,2> make_segment(const point2d<T>& point1, const point2d<T>& point2);
   template <typename T> inline segment<T,3> make_segment(const point3d<T>& point1, const point3d<T>& point2);

   template <typename T> inline segment<T,2> make_segment(const line<T,2>& line);
   template <typename T> inline segment<T,3> make_segment(const line<T,3>& line);

   template <typename T> inline line<T,2> make_line(const T& x1, const T& y1, const T& x2, const T& y2);
   template <typename T> inline line<T,3> make_line(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2);

   template <typename T> inline line<T,2> make_line(const point2d<T>& point1, const point2d<T>& point2);
   template <typename T> inline line<T,3> make_line(const point3d<T>& point1, const point3d<T>& point2);

   template <typename T> inline line<T,2> make_line(const segment<T,2>& segment);
   template <typename T> inline line<T,3> make_line(const segment<T,3>& segment);

   template <typename T> inline line<T,2> make_line(const ray<T,2>& ray);
   template <typename T> inline line<T,3> make_line(const ray<T,3>& ray);

   template <typename T> inline rectangle<T> make_rectangle(const T& x1, const T& y1, const T& x2, const T& y2);
   template <typename T> inline rectangle<T> make_rectangle(const point2d<T>& point1, const point2d<T>& point2);

   template <typename T> inline box<T,3> make_box(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2);
   template <typename T> inline box<T,3> make_box(const point3d<T>& point1, const point3d<T>& point2);

   template <typename T> inline triangle<T,2> make_triangle(const T& x1, const T& y1,
                                                            const T& x2, const T& y2,
                                                            const T& x3, const T& y3);

   template <typename T> inline triangle<T,3> make_triangle(const T& x1, const T& y1, const T& z1,
                                                            const T& x2, const T& y2, const T& z2,
                                                            const T& x3, const T& y3, const T& z3);

   template <typename T> inline triangle<T,2> make_triangle(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);
   template <typename T> inline triangle<T,3> make_triangle(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3);

   template <typename T> inline quadix<T,2> make_quadix(const T& x1, const T& y1,
                                                        const T& x2, const T& y2,
                                                        const T& x3, const T& y3,
                                                        const T& x4, const T& y4);

   template <typename T> inline quadix<T,3> make_quadix(const T& x1, const T& y1, const T& z1,
                                                        const T& x2, const T& y2, const T& z2,
                                                        const T& x3, const T& y3, const T& z3,
                                                        const T& x4, const T& y4, const T& z4);

   template <typename T> inline quadix<T,2> make_quadix(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3, const point2d<T>& point4);
   template <typename T> inline quadix<T,3> make_quadix(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3, const point3d<T>& point4);

   template <typename T> inline quadix<T,2> make_quadix(const T& x1, const T& y1, const T& x2, const T& y2);
   template <typename T> inline quadix<T,2> make_quadix(const rectangle<T>& rectangle);

   template <typename T> inline circle<T> make_circle(const T& x, const T& y, const T& radius);
   template <typename T> inline circle<T> make_circle(const point2d<T>& point, const T& radius);
   template <typename T> inline circle<T> make_circle(const point2d<T>& point1, const point2d<T>& point2);
   template <typename T> inline circle<T> make_circle(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3);
   template <typename T> inline circle<T> make_circle(const triangle<T,2>& triangle);

   template <typename T> inline sphere<T> make_sphere(const T& x, const T& y, const T& z, const T& radius);
   template <typename T> inline sphere<T> make_sphere(const point3d<T>& point, const T& radius);
   template <typename T> inline sphere<T> make_sphere(const point3d<T>& point1, const point3d<T>& point2);

   template <typename T> inline plane<T,3> make_plane(const T& x1, const T& y1, const T& z1,
                                                      const T& x2, const T& y2, const T& z2,
                                                      const T& x3, const T& y3, const T& z3);

   template <typename T> inline plane<T,3> make_plane(const T& px, const T& py, const T& pz,
                                                      const T& nx, const T& ny, const T& nz);

   template <typename T> inline plane<T,3> make_plane(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3);
   template <typename T> inline plane<T,3> make_plane(const point3d<T>& point, const vector3d<T>& normal);
   template <typename T> inline plane<T,3> make_plane(const triangle<T,3>& triangle);

   template <typename T, std::size_t D, typename InputIterator> inline polygon<T,D> make_polygon(const InputIterator begin, const InputIterator end);

   template <typename T> inline polygon<T,2> make_polygon(const std::vector< point2d<T> >& point_list);
   template <typename T> inline polygon<T,3> make_polygon(const std::vector< point3d<T> >& point_list);

   template <typename T> inline polygon<T,2> make_polygon(const triangle<T,2>& triangle);
   template <typename T> inline polygon<T,2> make_polygon(const quadix<T,2>& quadix);
   template <typename T> inline polygon<T,2> make_polygon(const rectangle<T>& rectangle);
   template <typename T> inline polygon<T,2> make_polygon(const circle<T>& circle, const unsigned int point_count = 360);

} // wykobi namespace

#include "wykobi.inl"

#endif
