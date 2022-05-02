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


#ifndef INCLUDE_WYKOBI_GUI
#define INCLUDE_WYKOBI_GUI


#include <iostream>
#include <string>

#include <GL/glut.h>


class wykobi_window;
static wykobi_window* handle;


enum eDisplayMode {
                   eDisplayMode2D,
                   eDisplayMode3D
                  };

class wykobi_window
{
public:

   wykobi_window(int argc, char* argv[])
   {
      display_mode = eDisplayMode2D;
      handle = this;
      glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
      glutInitWindowSize(window_width(),window_height());
      glutInitWindowPosition(window_position_x(),window_position_y());
      glutInit(&argc,argv);
      glutCreateWindow(window_title().c_str()  );
      glutKeyboardFunc(glut_keyboard_handler   );
      glutSpecialFunc (glut_special_key_handler);
      glutReshapeFunc (glut_reshape_handler    );
      glutDisplayFunc (glut_display_handler    );
      glutMouseFunc   (glut_mouse_handler      );
      glutMotionFunc  (glut_motion_handler     );
      glutCreateMenu  (glut_menu_handler       );
      glutIdleFunc    (glut_idle_handler       );

      if (handle->display_mode == eDisplayMode2D)
      {
         glClearColor (0.0, 0.0, 0.0, 0.0);
         gluOrtho2D (0.0, handle->window_width()-1, handle->window_height()-1,0.0);
         glDisable(GL_DEPTH_TEST);
      }

   }

   virtual ~wykobi_window(){};

   virtual void keyboard_handler(unsigned char key, int x, int y)    {}
   virtual void special_key_handler(int key, int x, int y)           {}
   virtual void reshape_handler(int width, int height)               {}
   virtual void display_handler(void)                                {}
   virtual void mouse_handler(int button, int state, int x, int y)   {}
   virtual void motion_handler(int x, int y)                         {}
   virtual void menu_handler(int menu_choice)                        {}
   virtual void timmer_handler(int ms_time)                          {}
   virtual void idle_handler(void)                                   {}

   virtual std::string window_title() { return std::string("Wykobi Window"); }
   virtual int window_width()         { return 700;                            }
   virtual int window_height()        { return 700;                            }
   virtual int window_position_x()    { return 10;                             }
   virtual int window_position_y()    { return 10;                             }
   virtual int world_width()          { return 700;                            }
   virtual int world_height()         { return 700;                            }

   void clear_black() { glClearColor(0.0,0.0,0.0,0); glClear(GL_COLOR_BUFFER_BIT); }
   void clear_white() { glClearColor(1.0,1.0,1.0,0); glClear(GL_COLOR_BUFFER_BIT); }

private:
   eDisplayMode display_mode;

   static void glut_keyboard_handler(unsigned char key, int x, int y)   { handle->keyboard_handler(key,x,y);       }
   static void glut_special_key_handler(int key, int x, int y)          { handle->special_key_handler(key,x,y);    }
   static void glut_reshape_handler(int width, int height)              { handle->reshape_handler(width,height);   }
   static void glut_mouse_handler(int button, int state, int x, int y)  { handle->mouse_handler(button,state,x,y); }
   static void glut_motion_handler(int x, int y)                        { handle->motion_handler(x,y);             }
   static void glut_menu_handler(int menu_choice)                       { handle->menu_handler(menu_choice);       }
   static void glut_timmer_handler(int ms_time)                         { handle->timmer_handler(ms_time);         }
   static void glut_idle_handler(void)                                  { handle->idle_handler();                  }

   static void glut_display_handler(void)
   {
      glClear (GL_COLOR_BUFFER_BIT);
      handle->display_handler();
      glutSwapBuffers();
   }
};

#endif
