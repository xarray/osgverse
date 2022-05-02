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


#ifndef INCLUDE_WYKOBI_GRAPHICS_OPENGL
#define INCLUDE_WYKOBI_GRAPHICS_OPENGL


#include <iostream>
#include <iterator>
#include <vector>
#include <string>
#include <cmath>

#include <GL/gl.h>
#include <GL/glu.h>

#include "wykobi.hpp"
#include "wykobi_utilities.hpp"


namespace wykobi
{
   enum DrawingMode  {
                        eNoDraw,
                        eSolid,
                        eOutLine
                     };

   const GLfloat basic_color[14][3]= {
                                       {    0.0 / 255.0,   0.0 / 255.0,   0.0 / 255.0},
                                       {  255.0 / 255.0,   0.0 / 255.0,   0.0 / 255.0},
                                       {    0.0 / 255.0, 255.0 / 255.0,   0.0 / 255.0},
                                       {    0.0 / 255.0,   0.0 / 255.0, 255.0 / 255.0},
                                       {  255.0 / 255.0,   0.0 / 255.0, 255.0 / 255.0},
                                       {    0.0 / 255.0, 255.0 / 255.0, 255.0 / 255.0},
                                       {    0.0 / 255.0,   0.0 / 255.0, 255.0 / 255.0},
                                       {  255.0 / 255.0, 255.0 / 255.0,   0.0 / 255.0},
                                       {    0.0 / 255.0, 255.0 / 255.0, 255.0 / 255.0},
                                       {  255.0 / 255.0, 192.0 / 255.0,  64.0 / 255.0},
                                       {  255.0 / 255.0,   0.0 / 255.0, 255.0 / 255.0},
                                       {  255.0 / 255.0,   0.0 / 255.0,   0.0 / 255.0},
                                       {  255.0 / 255.0, 255.0 / 255.0, 255.0 / 255.0},
                                       {  255.0 / 255.0, 255.0 / 255.0,   0.0 / 255.0}
                                    };

   template <typename T>
   class wykobi_graphics_opengl
   {
   public:

         wykobi_graphics_opengl(const unsigned int& w, const unsigned int& h, DrawingMode dm)
         : _width(w),
           _height(h),
           drawing_mode(dm),
           red(1.0),
           green(1.0),
           blue(1.0)
         {
         }

         ~wykobi_graphics_opengl(){}

         inline void set_color(const unsigned int& _red, const unsigned int& _green, const unsigned int& _blue)
         {
            red   = (1.0 * _red)   / 256.0;
            green = (1.0 * _green) / 256.0;
            blue  = (1.0 * _blue)  / 256.0;

            color[0] = red;
            color[1] = green;
            color[2] = blue;

            glColor3fv(color);
         }

         inline void set_color(const unsigned int& color_index)
         {
            color[0] = basic_color[0][color_index % 14];
            color[1] = basic_color[1][color_index % 14];
            color[2] = basic_color[2][color_index % 14];

            red   = color[0];
            green = color[1];
            blue  = color[2];

            glColor3fv(color);
         }

         inline void draw_text(const T& x, const T& y, const std::string& text)
         {
         }

         inline void draw_pixel(const T& x, const T& y) const
         {
            glBegin(GL_POINTS);
              glVertex2d(double(x),double(y));
            glEnd();
         }

         inline void draw_pixel(const T& x, const T& y, const T& z) const
         {
            glBegin(GL_POINTS);
              glVertex3d(double(x),double(y),double(z));
            glEnd();
         }

         inline void draw_segment(const T& x1, const T& y1, const T& x2, const T& y2) const
         {
            glBegin(GL_LINES);
             glVertex2d(double(x1),double(y1));
             glVertex2d(double(x2),double(y2));
            glEnd();
         }

         inline void draw_segment(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2) const
         {
            glBegin(GL_LINES);
             glVertex3d(x1,y1,z1);
             glVertex3d(x2,y2,z2);
            glEnd();
         }

         inline void draw_triangle(const T& x1, const T& y1,
                                   const T& x2, const T& y2,
                                   const T& x3, const T& y3) const
         {
            switch (drawing_mode)
            {
               case eSolid   : glBegin(GL_TRIANGLES); break;
               case eOutLine : glBegin(GL_LINE_LOOP); break;
               case eNoDraw  : return;
            }
            glVertex2d(double(x1),double(y1));
            glVertex2d(double(x2),double(y2));
            glVertex2d(double(x3),double(y3));
           glEnd();
         }

         inline void draw_triangle(const T& x1, const T& y1, const T& z1,
                                   const T& x2, const T& y2, const T& z2,
                                   const T& x3, const T& y3, const T& z3) const
         {
            switch (drawing_mode)
            {
               case eSolid   : glBegin(GL_TRIANGLES); break;
               case eOutLine : glBegin(GL_LINE_LOOP); break;
               case eNoDraw  : return;
            }
             glVertex3d(double(x1),double(y1),double(z1));
             glVertex3d(double(x2),double(y2),double(z2));
             glVertex3d(double(x3),double(y3),double(z3));
            glEnd();
         }

         inline void draw_quadix(const T& x1, const T& y1,
                                 const T& x2, const T& y2,
                                 const T& x3, const T& y3,
                                 const T& x4, const T& y4) const
         {
            switch (drawing_mode)
            {
               case eSolid   : glBegin(GL_QUADS);     break;
               case eOutLine : glBegin(GL_LINE_LOOP); break;
               case eNoDraw  : return;
            }
              glVertex2d(double(x1),double(y1));
             glVertex2d(double(x2),double(y2));
             glVertex2d(double(x3),double(y3));
             glVertex2d(double(x4),double(y4));
            glEnd();
         }

         inline void draw_quadix(const T& x1, const T& y1, const T& z1,
                                 const T& x2, const T& y2, const T& z2,
                                 const T& x3, const T& y3, const T& z3,
                                 const T& x4, const T& y4, const T& z4) const
         {
            switch (drawing_mode)
            {
               case eSolid   : glBegin(GL_QUADS);     break;
               case eOutLine : glBegin(GL_LINE_LOOP); break;
               case eNoDraw  : return;
            }
             glVertex3d(double(x1),double(y1),double(z1));
             glVertex3d(double(x2),double(y2),double(z2));
             glVertex3d(double(x3),double(y3),double(z3));
             glVertex3d(double(x4),double(y4),double(z4));
            glEnd();
         }

         inline void draw_rectangle(const T& x1, const T& y1, const T& x2, const T& y2) const
         {
            draw_quadix(x1,y1,x2,y1,x2,y2,x1,y2);
         }

         inline void draw_circle(const T& x, const T& y, const T& radius) const
         {
            wykobi::polygon<T,2> _polygon = make_polygon(make_circle(x,y,radius));
            glColor3fv (color);
            switch (drawing_mode)
            {
               case eSolid   : glBegin(GL_TRIANGLE_FAN); break;
               case eOutLine : glBegin(GL_LINE_LOOP);    break;
               case eNoDraw  : return;
            }
            if (drawing_mode == eSolid)
            {
               glVertex2d(double(_polygon[0].x),double(_polygon[0].y));
            }
            for (std::size_t i = (drawing_mode == eSolid)?1:0; i < _polygon.size(); ++i)
            {
               glVertex2d(double(_polygon[i].x),double(_polygon[i].y));
            }
            glEnd();
         }

         inline void draw_sphere(const T& x, const T& y, const T& radius) const
         {
         }

         inline void draw_pixel(const point2d<T>& point) const
         {
            draw_pixel(point.x,point.y);
         }

         inline void draw_pixel(const point3d<T>& point) const
         {
            draw_pixel(point.x,point.y,point.z);
         }

         inline void draw_segment(const point2d<T>& point1, const point2d<T>& point2) const
         {
            draw_segment(point1.x,point1.y,point2.x,point2.y);
         }

         inline void draw_triangle(const point2d<T>& point1, const point2d<T>& point2, const point2d<T>& point3) const
         {
            draw_triangle(point1.x,point1.y,
                          point2.x,point2.y,
                          point3.x,point3.y);
         }

         inline void draw_triangle(const point3d<T>& point1, const point3d<T>& point2, const point3d<T>& point3) const
         {
            draw_triangle(point1.x,point1.y,point1.z,
                          point2.x,point2.y,point2.z,
                          point3.x,point3.y,point3.z);
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

         inline void draw_quadix(const point3d<T>& point1,
                                 const point3d<T>& point2,
                                 const point3d<T>& point3,
                                 const point3d<T>& point4) const
         {
            draw_quadix(point1.x,point1.y,point1.z,
                        point2.x,point2.y,point2.z,
                        point3.x,point3.y,point3.z,
                        point4.x,point4.y,point4.z);
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

         inline void draw_polyline(std::vector< point3d<T> > point_list)
         {
            for (std::size_t i = 0; i < point_list.size() - 1; ++i)
            {
               draw_segment(point_list[i],point_list[i+1]);
            }
         }

         inline void clear()
         {
            glClear(GL_COLOR_BUFFER_BIT);
         }


         inline void draw(const point2d<T>&       point) { draw_pixel(point);                                    }
         inline void draw(const segment<T,2>&   segment) { draw_segment(segment[0],segment[1]);                  }
         inline void draw(const triangle<T,2>& triangle) { draw_triangle(triangle[0],triangle[1],triangle[2]);   }
         inline void draw(const rectangle<T>& rectangle) { draw_rectangle(rectangle[0],rectangle[1]);            }
         inline void draw(const quadix<T,2>&     quadix) { draw_quadix(quadix[0],quadix[1],quadix[2],quadix[3]); }
         inline void draw(const circle<T>&       circle) { draw_circle(circle.x,circle.y,circle.radius);         }


         inline void draw(const point3d<T>&       point) { draw_pixel(point);                                    }
         inline void draw(const segment<T,3>&   segment) { draw_segment(segment[0],segment[1]);                  }
         inline void draw(const triangle<T,3>& triangle) { draw_triangle(triangle[0],triangle[1],triangle[2]);   }
         inline void draw(const quadix<T,3>&     quadix) { draw_quadix(quadix[0],quadix[1],quadix[2],quadix[3]); }
         inline void draw(const sphere<T>&       sphere) { draw_sphere(sphere.x,sphere.y,sphere.radius);         }

         inline void draw(const polygon<T,2>& polygon, const bool convex = false)
         {
            if (polygon.size() < 3) return;
            if (!convex)
            {
               std::size_t j = polygon.size() - 1;
               for (std::size_t i = 0; i < polygon.size(); ++i)
               {
                  draw_segment(polygon[i],polygon[j]);
                  j = i;
               }
            }
            else
            {
               switch (drawing_mode)
               {
                  case eSolid   : glBegin(GL_POLYGON);   break;
                  case eOutLine : glBegin(GL_LINE_LOOP); break;
                  case eNoDraw  : return;
               }
               for (std::size_t i = 0; i < polygon.size(); ++i)
               {
                glVertex3d(polygon[i].x1,polygon[i].y1,polygon[i].z1);
               }
               glEnd();
            }
         }

         inline void draw(const polygon<T,3>& polygon)
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

         inline void draw(const cubic_bezier<T,3>& bezier, const std::size_t& point_count)
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

         inline void draw(const quadratic_bezier<T,3>& bezier, const std::size_t& point_count)
         {
            std::vector< point2d<T> > point_list;
            wykobi::generate_bezier(bezier,point_count,point_list);
            draw_polyline(point_list);
         }

      private:
         unsigned int  _width;
         unsigned int  _height;
         float         red;
         float         green;
         float         blue;
         GLfloat       color[3];
         DrawingMode   drawing_mode;
   };

}// namespace wykobi

#endif
