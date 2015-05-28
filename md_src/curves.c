/*  MegaDonkey Library File:  curves.c     Parameteric Line/Arc/BezierCurve Generator
    


    Copyright 2007,2008  Mark Sims & Ron Grant


    This file is part of The Megadonkey Software Library.

    The Megadonkey Software Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The Megadonkey Software Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
    more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Megadonkey Software Library. If not, see <http://www.gnu.org/licenses/>.




    Parameteric Line/Arc/BezierCurve Generator
    Ron Grant 
    August 2006

    parameter u is varried from 0 to 1.0 in number of steps supplied 
    (du = 1/steps)
   
    Lines or Curves generated one point at a time suitable for motion profile generation.
    That is, periodic update routine can call associated "XXX_nextpoint" function or 
    next_point() to obtain each successive point stored in global CurX,CurY   
    
*/

#include "md.h"            // global typedefs e.g. u08

#ifdef BEZIER_CODE
#include <avr/pgmspace.h>  // support for vars stored in prog mem 
#include <math.h>          // trig functions 
#include "lcd.h"           // low level graphics 
#include "graph.h"         // clipped_line
#include "timer.h"         // delay functions
#include "adc.h"           // touch
#include "trakball.h"      // mouse / cursor
#include "menu.h"
#include "curves.h"


float dx,dy; // used for arc/line calc


// sample test params for bezier curve

void bezier_test(long steps)
{ 
   X1 = 0;   Y1 = 0;
   X2 = 50;  Y2 = 50;
   X3 = 100; Y3 = 50;
   X4 = 150; Y4 = 0;
   bezier_init(steps);
}  


u08 next_point(void) 
{
  switch (DrawCmdActive) {
     case DC_PAR_LINE: par_line_nextpoint();
     case DC_BEZ:      bezier_nextpoint();
     case DC_PAR_ARC:  par_arc_nextpoint();
  }
  return DrawCmdActive;
}



// next_u: advance to next u value, if not right at end
// if u beyond end make = to end such that next call to nextpoint
// routine will gen point right at end of curve or line.

void next_u(void)
{       
  if(u == 1.0) {  // are we at the end of the parametric line/curve?
     DrawCmdActive = 0; 
  }
  else { 
    u += du;               // advance to next point
    if(u > 1.0) u = 1.0;   // if beyond end of curve, force end of curve to be next point
  }     
}       
        



void bezier_init(long steps)
{       
  u = 0.0;

  if (duParamMode)  du = (float) * (float*) (&steps) ;
  else      
    du = 1.0 / steps;

  DrawCmdActive = DC_BEZ;
}
        
void bezier_nextpoint(void) 
{
float u2,u3;
float a,b,c,d;

   // u squared and cubed
   u2 = u*u;
   u3 = u2*u;
   
   // cubic weight terms
   a = 1-u3 +3*(u2-u);  
   b = 3*(u+u3-2*u2);
   c = 3*(u2-u3);
   d = u3;

   // calc XY point on curve at current u 

   CurX = a * X1 + b * X2 + c * X3 + d * X4;
   CurY = a * Y1 + b * Y2 + c * Y3 + d * Y4;
   
   next_u();  // advance to next point
}


// parametric line  


void par_line_init(long steps)
{       
  u = 0.0;      

  if (duParamMode) du = (float) * (float*) &steps;
  else
    du = 1.0 / steps;

  DrawCmdActive = DC_PAR_LINE;
  dx = X2-X1;
  dy = Y2-Y1;
   
}


void par_line_nextpoint(void)   // calc next point on parameteric line
{
   CurX = X1 + u * dx;    // set global CurX,CurY
   CurY = Y1 + u * dy;
   next_u();
}


void par_arc_init(long steps)
{
   if (duParamMode) du = (float) * (float*) &steps;
   else du = 1.0/steps;       

   u = 0.0;      
   DrawCmdActive = DC_PAR_ARC;
   dx = ParArcEndAngle-ParArcStartAngle;  // really da delta angle
}


void par_arc_nextpoint(void)      // sets global LineX LineY  
{
float a;      
        
  // X1,Y1  Center 
    
  a = ParArcStartAngle + u * dx;
  // parametric angle varies from Start to End linearly
  
  CurX = X1 + ParArcRadius * cos(a);    
  CurY = Y1 + ParArcRadius * sin(a);    
  
  next_u();
}       




// logo extents for scale to full screen calc

#define LOGO_W (82+2)
#define LOGO_H (76+2)


// scale factors to scale logo to full screen with 4 bit fractional
// part

#define BEZ_SCALE_X ((int) COLS * 16 / LOGO_W)
#define BEZ_SCALE_Y ((int) ROWS * 16 / LOGO_H) 


void xform (int x, int y, int* xt,int* yt) {

  long scx,scy;

  scx = BEZ_SCALE_X;
  scy = BEZ_SCALE_Y;

  // if keep scaling uniform in x and y 
  //if (scx>scy) scx = scy;
  //else         scy = scx;

  // note translation factor of y-2 added to move demo logo up
  // 2 pixels
  // y= ROWS-y  flips drawing making bottom of screen y=0
  // (left handed coordinate system -- origin in lower-left screen 
  //  corner)

  *xt = ((x * scx)>>4);
  *yt = (ROWS-2-(y * scy>>4));  
  
}




void thick_line(int x1,int y1, int x2,int y2)
{ 

   clipped_line(x1,y1, x2,y2);
   clipped_line(x1+1,y1, x2+1,y2);

}


#ifdef LOGO_DEMO

// Demo logo drawn using values stored in a constant array.
// Lines and Bezier curves are translated from Adobe Illustrator format using 
// the MegaDonkey_AI_Translator utility which generates source code for an array that is 
// accessed by this function.

u08 DonkeyLogo[] PROGMEM = {
   // Donkey Outline
   253,51,54,
   51,54,55,60,58,58,59,59,
   59,59,60,60,50,63,63,75,
   63,75,64,76,61,64,60,60,
   60,60,60,59,62,59,63,59,
   63,59,68,59,72,52,76,50,
   76,50,76,50,80,47,79,45,
   79,45,79,45,78,43,78,43,
   78,43,76,43,75,47,73,44,
   73,44,72,42,77,43,76,40,
   76,40,75,37,71,41,70,41,
   70,41,59,43,46,43,49,20,
   49,20,50,17,58,0,57,0,
   57,0,55,1,50,0,48,3,
   48,3,45,10,45,23,43,25,
   43,25,39,12,23,8,10,21,
   10,21,5,25,10,0,9,0,
   254,4,0,
   4,0,0,1,5,32,8,41,
   8,41,8,41,6,41,6,40,
   6,40,4,37,2,20,1,21,
   1,21,0,21,1,41,4,44,
   4,44,6,46,8,44,8,44,
   8,44,29,35,39,38,51,54,

   // EYE and MD ----------------
   253,65,56,
   65,56,63,56,62,56,61,56,
   254,64,54,
   64,54,64,55,65,55,65,55,
   65,55,66,55,67,54,67,53,
   67,53,67,53,66,52,65,52,
   65,52,64,52,63,53,63,53,
   63,53,63,54,63,54,64,54,
   254,61,55,
   61,55,61,55,61,55,61,55,
   61,55,61,53,62,51,65,51,
   65,51,67,51,69,51,69,53,
   69,53,69,54,67,56,65,56,
   253,14,35,
   254,37,35,
   37,35,38,35,38,35,38,34,
   254,38,23,
   38,23,38,22,38,22,37,22,
   254,33,22,
   33,22,40,24,39,34,27,33,
   27,33,28,29,28,25,28,22,
   254,26,22,
   26,22,26,25,26,29,27,33,
   254,23,33,
   254,21,27,
   254,19,33,
   254,15,33,
   15,33,15,29,15,25,15,22,
   254,14,22,
   14,22,14,22,14,22,14,23,
   254,14,34,
   14,34,14,35,14,35,14,35,
   254,14,35,
   253,24,22,
   254,24,26,
   254,21,22,
   254,24,22,
   254,24,22,
   253,21,22,
   254,18,26,
   254,18,22,
   254,21,22,
   254,21,22,
   253,30,31,
   254,30,23,
   30,23,37,24,35,31,30,31,
   255
};


#define rd(v) v=pgm_read_byte_near(&DonkeyLogo[LogoI++])
#define rdxy(x,y) {rd(x); rd(y);}

void draw_donkey_logo(void) 
{
u08 b;
int PrevX,PrevY;
int x,y;
int xt,yt;
int LogoI;

   LogoI = 0;

//   lcd_clear();
   viewport_init();
   set_color(WHITE);
   set_bg(BLACK);

   wait_while_touched();

u08 cur = erase_cursor();

   // X1,Y1,X2.. float values
 
   b = 0; 
   while(b != 255) {
#ifdef USER_INPUT
      if(get_touch(1)) break;
#endif

      rd(b);
      switch (b) {
         case 254:     // Draw Line To XY
            rdxy(x,y); 
            xform(x,y, &xt,&yt);

            X2 = xt;
            Y2 = yt;

            thick_line (CurX,CurY,X2,Y2);  // quick LCD line for now later maybe parametric option

            CurX = X2;
            CurY = Y2;
            break;

         case 253:    // MoveTo XY
            rdxy(x,y);
            xform (x,y, &xt,&yt);

            CurX = xt;
            CurY = yt;
            break;

         case 255:   // drawing sequence terminator 
            break; 

         default: 
            x = b; rd(y); 
            xform(x,y, &xt,&yt);
            X1 = xt;
            Y1 = yt;

            rdxy(x,y); 
            xform(x,y, &xt,&yt);
            X2 = xt;
            Y2 = yt;

            rdxy(x,y); 
            xform(x,y, &xt,&yt);
            X3 = xt;
            Y3 = yt;

            rdxy(x,y); 
            xform(x,y, &xt,&yt);
            X4 = xt;
            Y4 = yt;

            PrevX = X1;
            PrevY = Y1;

            bezier_init(16);   // 16 steps per curve regardless of length
 
            while(DrawCmdActive) {
               bezier_nextpoint();
               thick_line(PrevX,PrevY, CurX,CurY);
               PrevX = CurX;
               PrevY = CurY;
            }
            break;
      } // end switch
   } // end while


#ifndef LIBRARY_BUILD
  #ifdef USER_INPUT
     if(b == 255) {  // logo not interrupted
        wait_until_touched();
     }
     wait_while_touched();
  #else
     delay_ms(2000);
  #endif
#endif

if(cur) show_cursor();

}

#endif // LOGO_DEMO



#ifdef CURVE_DEMO

#define sqr(x) ((x)*(x))

void fat_dot(int x, int y) 
{
int x1,y1;
u08 cur;

cur = erase_cursor();
  for(x1=x-2; x1<x+3; x1++) {
     for(y1=y-2; y1<y+3; y1++) dot(x1,y1);
  }
if(cur) show_cursor();
}


void bezier_edit_demo(void) 
{
int PrevX,PrevY;
int x,y;
int d,min,minI;

   // define initial curve
   X1 = 0;
   Y1 = ROWS/2;
   X2 = COLS/3;
   Y2 = 0;
   X3 = COLS*2/3;
   Y3 = 0;
   X4 = COLS-1;
   Y4 = ROWS-1; 

   page_buffers(2);

   while(1) {
//u08 cur;
//cur = erase_cursor();
      lcd_pageflip();  // double buffered graphics (flip flop two display pages)
      lcd_clear();

      menu_init();         // kludge for drawing standard exit button
      menu_exitbutton();   // ... this demo does not really use menus
   
      // draw the convex hull bounding the curve
      line(X1,Y1,X2,Y2);
      line(X2,Y2,X3,Y3);
      line(X3,Y3,X4,Y4);

      fat_dot (X1,Y1);
      fat_dot (X2,Y2);
      fat_dot (X3,Y3);
      fat_dot (X4,Y4);

      // draw the curve 

      bezier_init(24);   // 24 segments
      PrevX = X1;
      PrevY = Y1;

      while(DrawCmdActive) {                   // draw entire curve right now
         bezier_nextpoint();                   // this could be done in a throttled fashion using 
         thick_line(PrevX,PrevY, CurX,CurY);   // interrupts or looking at a timer in a main program loop
         PrevX = CurX;
         PrevY = CurY;
      }
//if(cur) show_cursor();

     if(get_touch(5)) {
         fat_dot(touchX,touchY);

         x = touchX;  // shorthand
         y = touchY;

         if((y < (CHAR_HEIGHT+4)) && (x > (COLS-CHAR_WIDTH-4))) break;

         // find control point with least min dist squared to touch XY
         min = sqr(X1-x) + (Y1-y);
         minI = 1;
                 
         if((d=sqr(X2-x)+(Y2-y)) < min)  { min = d; minI = 2; }
         if((d=sqr(X3-x)+(Y3-y)) < min)  { min = d; minI = 3; }
         if((d=sqr(X4-x)+(Y4-y)) < min)  { min = d; minI = 4; }

         // if touchXY is close to control point, make control point equal to touch point
         // allows control dragging points

         if(min < 70) { // min dist squared
            switch(minI) {
               case 1: X1 = x; Y1=y; break;
               case 2: X2 = x; Y2=y; break;
               case 3: X3 = x; Y3=y; break;
               case 4: X4 = x; Y4=y; break;
            } 
         }
      }       
   }

   wait_while_touched();
   page_reset();
   lcd_clear();
   return;
}

#endif // CURVE_DEMO

#endif // BEZIER_CODE
