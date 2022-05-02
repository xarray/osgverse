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


#ifndef INCLUDE_WYKOBI_GRAPHICS_NET
#define INCLUDE_WYKOBI_GRAPHICS_NET


#include <iostream>
#include <iterator>
#include <vector>
#include <string>

#include <math.h>
#include <vcclr.h>

#include "wykobi.hpp"


using namespace System;
using namespace System::Drawing;


namespace wykobi
{
   template <typename T>
   class wykobi_graphics_net
   {
   public:

      wykobi_graphics_net(Graphics^ gc, const unsigned int& w, const unsigned int& h)
      : gc_    (gc),
        width_ (w),
        height_(h)
      {
         pen_ = gcnew Pen(System::Drawing::Color::Black,1);
      }

     ~wykobi_graphics_net()
      {
         delete gc_;
      }

      inline void set_pen(Pen^ _pen) const
      {
         pen_ = _pen;
      }

      inline float get_pen_width() const
      {
         return pen_->Width;
      }

      inline void set_pen_width(const unsigned int& w) const
      {
         pen_->Width = float(w);
      }

      inline void set_dash_mode() const
      {
         array<Single>^ temp0 = {5.0F, 5.0F, 5.0F, 5.0F};
         pen_->DashPattern = temp0;
      }

      inline void set_nodash_mode() const
      {
         array<Single>^ temp0 = {1.0F, 0.0001F};
         pen_->DashPattern = temp0;
      }

      inline void set_color(const unsigned int& color) const
      {
         switch(color % 15)
         {
            case  0 : pen_->Color = System::Drawing::Color::Aqua;      break;
            case  1 : pen_->Color = System::Drawing::Color::Black;     break;
            case  2 : pen_->Color = System::Drawing::Color::Blue;      break;
            case  3 : pen_->Color = System::Drawing::Color::Brown;     break;
            case  4 : pen_->Color = System::Drawing::Color::Cyan;      break;
            case  5 : pen_->Color = System::Drawing::Color::Gray;      break;
            case  6 : pen_->Color = System::Drawing::Color::Green;     break;
            case  7 : pen_->Color = System::Drawing::Color::Indigo;    break;
            case  8 : pen_->Color = System::Drawing::Color::LimeGreen; break;
            case  9 : pen_->Color = System::Drawing::Color::Magenta;   break;
            case 10 : pen_->Color = System::Drawing::Color::Orange;    break;
            case 11 : pen_->Color = System::Drawing::Color::Purple;    break;
            case 12 : pen_->Color = System::Drawing::Color::Red;       break;
            case 13 : pen_->Color = System::Drawing::Color::Violet;    break;
            case 14 : pen_->Color = System::Drawing::Color::White;     break;
            case 15 : pen_->Color = System::Drawing::Color::Yellow;    break;
         }
      }

      inline unsigned int width()    const { return width_;          }
      inline unsigned int height()   const { return height_;         }
      inline unsigned int center_x() const { return width_  * 0.5;   }
      inline unsigned int center_y() const { return height_ * 0.5;   }

      inline void draw_pixel(const T& x, const T& y) const
      {
         gc_->DrawLine(pen_,float(x),float(y),float(x + 1.0f),float(y + 1.0f));
      }

      inline void draw_pixel(const point2d<T>& point) const
      {
         draw_pixel(point.x,point.y);
      }

      inline void draw_segment(const T& x1, const T& y1, const T& x2, const T& y2) const
      {
         gc_->DrawLine(pen_, float(x1), float(y1), float(x2), float(y2));
      }

      inline void draw_segment(const point2d<T>& point1, const point2d<T>& point2) const
      {
         draw_segment(point1.x, point1.y, point2.x, point2.y);
      }

      inline void draw_line(const T& x1, const T& y1, const T& x2, const T& y2) const
      {
         T dx = x2 - x1;
         T dy = y2 - y1;

         if (dx != 0.0)
         {
            T m = dy / dx;
            T c = y1 - m * x1;

            draw_segment(0.0,c,width_,m * width_ + c);
         }
         else
            draw_segment(x1,0,x1,height_);
      }

      inline void draw_triangle(const T& x1, const T& y1,
                                const T& x2, const T& y2,
                                const T& x3, const T& y3) const
      {
         draw_segment(x1, y1, x2, y2);
         draw_segment(x2, y2, x3, y3);
         draw_segment(x3, y3, x1, y1);
      }

      inline void draw_rectangle(const T& x1, const T& y1, const T& x2, const T& y2) const
      {
         gc_->DrawRectangle(pen_,float(x1),float(y1),fabs(float(x2 - x1)),fabs(float(y2 - y1)));
      }

      inline void draw_quadix(const T& x1, const T& y1,
                              const T& x2, const T& y2,
                              const T& x3, const T& y3,
                              const T& x4, const T& y4) const
      {
         draw_segment(x1, y1, x2, y2);
         draw_segment(x2, y2, x3, y3);
         draw_segment(x3, y3, x4, y4);
         draw_segment(x4, y4, x1, y1);
      }

      inline void draw_circle(const T& x, const T& y, const T& radius) const
      {
         gc_->DrawEllipse
               (
                 pen_,
                 float(   x - radius), float(   y - radius),
                 float(2.0f * radius), float(2.0f * radius)
               );
      }

      inline void draw_circle(const point2d<T> center, const T& radius) const
      {
         draw_circle(center.x,center.y,radius);
      }

      inline void draw_polyline(const std::vector< point2d<T> >& point_list) const
      {
         for (std::size_t i = 0; i < point_list.size() - 1; ++i)
         {
            draw_segment(point_list[i],point_list[i+1]);
         }
      }

      inline void draw_polyline(const std::vector< point3d<T> >& point_list) const
      {
         for (std::size_t i = 0; i < point_list.size() - 1; ++i)
         {
            draw_segment(point_list[i],point_list[i+1]);
         }
      }

      inline void draw_crosshair(const point2d<T>& p, const T r) const
      {
         draw_segment(p.x - r, p.y, p.x + r, p.y);
         draw_segment(p.x, p.y - r, p.x, p.y + r);
      }

      inline void clear(System::Drawing::Color color) const
      {
         gc_->Clear(color);
      }

      inline void clear() const
      {
         clear(System::Drawing::Color::White);
      }

      inline void clear(const unsigned int& color) const
      {
         switch(color)
         {
            case  0 : clear(System::Drawing::Color::Aqua     ); break;
            case  1 : clear(System::Drawing::Color::Black    ); break;
            case  2 : clear(System::Drawing::Color::Blue     ); break;
            case  3 : clear(System::Drawing::Color::Brown    ); break;
            case  4 : clear(System::Drawing::Color::Cyan     ); break;
            case  5 : clear(System::Drawing::Color::Gray     ); break;
            case  6 : clear(System::Drawing::Color::Green    ); break;
            case  7 : clear(System::Drawing::Color::Indigo   ); break;
            case  8 : clear(System::Drawing::Color::LimeGreen); break;
            case  9 : clear(System::Drawing::Color::Magenta  ); break;
            case 10 : clear(System::Drawing::Color::Orange   ); break;
            case 11 : clear(System::Drawing::Color::Purple   ); break;
            case 12 : clear(System::Drawing::Color::Red      ); break;
            case 13 : clear(System::Drawing::Color::Violet   ); break;
            case 14 : clear(System::Drawing::Color::White    ); break;
            case 15 : clear(System::Drawing::Color::Yellow   ); break;
            default : clear(System::Drawing::Color::White    ); break;
         }
      }

      inline void draw(const point2d<T>& point) const
      {
         draw_pixel(point.x,point.y);
      }

      inline void draw(const segment<T,2>& segment) const
      {
         draw_segment(segment[0].x, segment[0].y, segment[1].x, segment[1].y);
      }

      inline void draw(const triangle<T,2>& triangle) const
      {
         draw_triangle
            (
              triangle[0].x,triangle[0].y,
              triangle[1].x,triangle[1].y,
              triangle[2].x,triangle[2].y
            );
      }

      inline void draw(const rectangle<T>& rectangle) const
      {
         draw_rectangle(rectangle[0].x,rectangle[0].y,rectangle[1].x,rectangle[1].y);
      }

      inline void draw(const quadix<T,2>& quadix) const
      {
         draw_quadix
            (
              quadix[0].x,quadix[0].y,
              quadix[1].x,quadix[1].y,
              quadix[2].x,quadix[2].y,
              quadix[3].x,quadix[3].y
            );
      }

      inline void draw(const circle<T>& circle) const
      {
         draw_circle(circle.x, circle.y, circle.radius);
      }

      inline void draw(const polygon<T,2>& polygon) const
      {
         if (polygon.size() < 3) return;

         std::size_t j = polygon.size() - 1;

         for (std::size_t i = 0; i < polygon.size(); ++i)
         {
            draw_segment(polygon[i], polygon[j]);

            j = i;
         }
      }

      inline void draw(const cubic_bezier<T,2>& bezier, const std::size_t& point_count) const
      {
         std::vector< point2d<T> > point_list;
         point_list.reserve(point_count);

         wykobi::generate_bezier(bezier, std::back_inserter(point_list), point_count);

         draw_polyline(point_list);
      }

      inline void draw(const quadratic_bezier<T,2>& bezier, const std::size_t& point_count) const
      {
         std::vector< point2d<T> > point_list;
         point_list.reserve(point_count);

         wykobi::generate_bezier(bezier, std::back_inserter(point_list), point_count);

         draw_polyline(point_list);
      }

      template <typename InputIterator>
      inline void draw(const InputIterator begin, const InputIterator end) const
      {
         for (InputIterator it = begin; it != end; ++it)
         {
            draw(*it);
         }
      }

      const static unsigned int clAqua      =  0;
      const static unsigned int clBlack     =  1;
      const static unsigned int clBlue      =  2;
      const static unsigned int clBrown     =  3;
      const static unsigned int clCyan      =  4;
      const static unsigned int clGray      =  5;
      const static unsigned int clGreen     =  6;
      const static unsigned int clIndigo    =  7;
      const static unsigned int clLimeGreen =  8;
      const static unsigned int clMagenta   =  9;
      const static unsigned int clOrange    = 10;
      const static unsigned int clPurple    = 11;
      const static unsigned int clRed       = 12;
      const static unsigned int clViolet    = 13;
      const static unsigned int clWhite     = 14;
      const static unsigned int clYellow    = 15;

      private:

         unsigned int      width_;
         unsigned int      height_;

         gcroot<Graphics^> gc_;
         gcroot<Pen^>      pen_;
   };

} // namespace wykobi

#endif
