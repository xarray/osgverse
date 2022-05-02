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


#ifndef INCLUDE_WYKOBI_GRAPHICS_VCL
#define INCLUDE_WYKOBI_GRAPHICS_VCL


#include <iostream>
#include <iterator>
#include <vector>
#include <string>
#include <cmath>

#include <Graphics.hpp>

#include "wykobi.hpp"


namespace wykobi
{
   template <typename T>
   class wykobi_graphics_vcl
   {
   public:

      wykobi_graphics_vcl(TCanvas* c, unsigned int w, unsigned int h)
      : canvas(c),
        _width(w),
        _height(h),
        color(clBlack)
      {
         canvas->Brush->Style = bsClear;
      }

      ~wykobi_graphics_vcl(){}

      inline void set_pen   (TPen*        p) { canvas->Pen        = p; }
      inline void set_brush (TBrush*      b) { canvas->Brush      = b; }
      inline void set_color (TColor       c) { canvas->Pen->Color = c; }
      inline void set_width (unsigned int w) { _width             = w; }
      inline void set_height(unsigned int h) { _height            = h; }
      inline TPen* get_pen()                 { return canvas->Pen;     }
      inline unsigned int width()            const { return _width;    }
      inline unsigned int height()           const { return _height;   }
      inline unsigned int center_x() const   { return _width  * 0.5;   }
      inline unsigned int center_y() const   { return _height * 0.5;   }

      inline void set_pen_width(int w) { canvas->Pen->Width = w;   }

      inline void draw_text(const T& x, const T& y, std::string text)
      {
         AnsiString s = text.c_str();
         canvas->TextOut(static_cast<int>(x),static_cast<int>(y),s);
      }

      inline void draw_pixel(const T& x, const T& y) const
      {
         canvas->Pixels[static_cast<int>(x)][static_cast<int>(y)] = canvas->Pen->Color;
      }

      inline void draw_pixel(const point2d<T>& point) const
      {
         canvas->MoveTo(static_cast<int>(point.x),static_cast<int>(point.y));
         canvas->LineTo(static_cast<int>(point.x+1),static_cast<int>(point.y+1));
      }

      inline void draw_segment(const T& x1, const T& y1, const T& x2, const T& y2) const
      {
         canvas->MoveTo(static_cast<int>(x1),static_cast<int>(y1));
         canvas->LineTo(static_cast<int>(x2),static_cast<int>(y2));
      }

      inline void draw_line(const T& x1, const T& y1, const T& x2, const T& y2) const
      {
         T dx = x2 - x1;
         T dy = y2 - y1;

         if (dx != 0.0)
         {
            T m = dy / dx;
            T c = y1 - m * x1;
            draw_segment(0.0,c,_width,m * _width + c);
         }
         else
            draw_segment(x1,0,x1,_height);
      }

      inline void draw_triangle(const T& x1, const T& y1,
                                const T& x2, const T& y2,
                                const T& x3, const T& y3) const
      {
         draw_segment(x1,y1,x2,y2);
         draw_segment(x2,y2,x3,y3);
         draw_segment(x3,y3,x1,y1);
      }

      inline void draw_quadix(const T& x1, const T& y1,
                              const T& x2, const T& y2,
                              const T& x3, const T& y3,
                              const T& x4, const T& y4) const
      {
         draw_segment(x1,y1,x2,y2);
         draw_segment(x2,y2,x3,y3);
         draw_segment(x3,y3,x4,y4);
         draw_segment(x4,y4,x1,y1);
      }

      inline void draw_rectangle(const T x1, const T& y1, const T& x2, const T& y2) const
      {
         canvas->Rectangle(static_cast<int>(x1),static_cast<int>(y1),
                           static_cast<int>(x2),static_cast<int>(y2));
      }

      inline void draw_circle(const T x, const T& y, const T& radius) const
      {
         canvas->Ellipse(static_cast<int>(x - radius),static_cast<int>(y - radius),static_cast<int>(x + radius),static_cast<int>(y + radius));
      }

      inline void draw_segment(const point2d<T>& point1, const point2d<T>& point2) const
      {
         draw_segment(point1.x,point1.y,point2.x,point2.y);
      }

      inline void draw_line(const point2d<T>& point1, const point2d<T>& point2) const
      {
         draw_line(point1.x,point1.y,point2.x,point2.y);
      }

      inline void draw_triangle(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3) const
      {
         draw_triangle(point1.x,point1.y,
                       point2.x,point2.y,
                       point3.x,point3.y);
      }

      inline void draw_quadix(const point2d<T>& point1,
                              const point2d<T>& point2,
                              const point2d<T>& point3,
                              const point2d<T>& point4) const
      {
         draw_quadix(point1.x,point1.y,
                     point2.x,point2.y,
                     point3.x,point3.y,
                     point4.x,point4.y);
      }

      inline void draw_rectangle(const point2d<T>& point1, const point2d<T>& point2) const
      {
         draw_rectangle(point1.x,point1.y,point2.x,point2.y);
      }

      inline void draw_circle(const point2d<T>& point, const T& radius) const
      {
         draw_circle(point.x,point.y,radius);
      }

      inline void draw_polyline(std::vector< point2d<T> > point_list)
      {
         for (std::size_t i = 0; i < point_list.size() - 1; ++i)
         {
            draw_segment(point_list[i],point_list[i+1]);
         }
      }

      inline void clear(TColor color)
      {
         TColor      TmpBrushColor = canvas->Brush->Color;
         TBrushStyle TmpBrushStyle = canvas->Brush->Style;

         canvas->Brush->Style = bsSolid;
         canvas->Brush->Color = color;
         canvas->FillRect(Rect(0,0,_width,_height));
         canvas->Brush->Color = TmpBrushColor;
         canvas->Brush->Style = TmpBrushStyle;
      }

      inline void clear_white() { clear(clWhite); }
      inline void clear_black() { clear(clBlack); }

      inline void draw(const point2d<T>&       point) { draw_pixel(point);                                    }
      inline void draw(const segment<T,2>&   segment) { draw_segment(segment[0],segment[1]);                  }
      inline void draw(const line<T,2>&         line) { draw_line(line[0],line[1]);                           }
      inline void draw(const triangle<T,2>& triangle) { draw_triangle(triangle[0],triangle[1],triangle[2]);   }
      inline void draw(const rectangle<T>& rectangle) { draw_rectangle(rectangle[0],rectangle[1]);            }
      inline void draw(const quadix<T,2>&     quadix) { draw_quadix(quadix[0],quadix[1],quadix[2],quadix[3]); }
      inline void draw(const circle<T>&       circle) { draw_circle(circle.x,circle.y,circle.radius);         }

      inline void draw(const polygon<T,2>& polygon)
      {
         if (polygon.size() < 3) return;

         std::size_t j = polygon.size() - 1;

         for (std::size_t i = 0; i < polygon.size(); ++i)
         {
            draw_segment(polygon[i],polygon[j]);
            j = i;
         }
      }

      inline void draw(const cubic_bezier<T,2>& bezier, const std::size_t& point_count)
      {
         std::vector< point2d<T> > point_list;
         wykobi::generate_bezier(bezier,point_count,point_list);
         draw_polyline(point_list);
      }

      inline void draw(const quadratic_bezier<T,2>& bezier, const std::size_t& point_count)
      {
         std::vector< point2d<T> > point_list;
         wykobi::generate_bezier(bezier,point_count,point_list);
         draw_polyline(point_list);
      }

      private:

         TCanvas*      canvas;
         unsigned int  _width;
         unsigned int  _height;
         TColor        color;
   };
}// namespace wykobi

#endif
