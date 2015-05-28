/*  MegaDonkey Library File:  graph.c    2D Wireframe Graphics 
    


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




    MegaDonkey 2D Graphics
    Ron Grant - May 2007

    World Coordinate System (signed 16 bit coordinates)
    2D Viewing Transforms
    User Definable Viewport (2D position and viewport rotation)

    Line Clipper (clip to viewport defined as rectangle on screen)

    Object (defined in 8 bit coordinates) with instance transform into 16 bit world coordinates 

*/

#include <stdio.h>
#include <avr/pgmspace.h>  // support for vars stored in prog mem 
#include "md.h"

#ifdef SINE360_TABLE
signed char SinT360[] PROGMEM = {  // 360 sample sine table  0..359 degrees
   0,2,4,7,9,11,13,15,18,20,22,24,26,29,31,33,
   35,37,39,41,43,46,48,50,52,54,56,58,60,62,64,65,
   67,69,71,73,75,76,78,80,82,83,85,87,88,90,91,93,
   94,96,97,99,100,101,103,104,105,107,108,109,110,111,112,113,
   114,115,116,117,118,119,119,120,121,121,122,123,123,124,124,125,
   125,125,126,126,126,127,127,127,127,127,127,127,127,127,127,127,
   126,126,126,125,125,125,124,124,123,123,122,121,121,120,119,119,
   118,117,116,115,114,113,112,111,110,109,108,107,105,104,103,101,
   100,99,97,96,94,93,91,90,88,87,85,83,82,80,78,76,
   75,73,71,69,67,65,64,62,60,58,56,54,52,50,48,46,
   43,41,39,37,35,33,31,29,26,24,22,20,18,15,13,11,
   9,7,4,2,0,-2,-4,-7,-9,-11,-13,-15,-18,-20,-22,-24,
   -26,-29,-31,-33,-35,-37,-39,-41,-43,-46,-48,-50,-52,-54,-56,-58,
   -60,-62,-63,-65,-67,-69,-71,-73,-75,-76,-78,-80,-82,-83,-85,-87,
   -88,-90,-91,-93,-94,-96,-97,-99,-100,-101,-103,-104,-105,-107,-108,-109,
   -110,-111,-112,-113,-114,-115,-116,-117,-118,-119,-119,-120,-121,-121,-122,-123,
   -123,-124,-124,-125,-125,-125,-126,-126,-126,-127,-127,-127,-127,-127,-127,-127,
   -127,-127,-127,-127,-126,-126,-126,-125,-125,-125,-124,-124,-123,-123,-122,-121,
   -121,-120,-119,-119,-118,-117,-116,-115,-114,-113,-112,-111,-110,-109,-108,-107,
   -105,-104,-103,-101,-100,-99,-97,-96,-94,-93,-91,-90,-88,-87,-85,-83,
   -82,-80,-78,-76,-75,-73,-71,-69,-67,-65,-63,-62,-60,-58,-56,-54,
   -52,-50,-48,-46,-43,-41,-39,-37,-35,-33,-31,-29,-26,-24,-22,-20,
   -18,-15,-13,-11,-9,-7,-4,-2
};
#endif // SINE360_TABLE


#ifdef SINE256_TABLE
signed char SinT256[] PROGMEM = {  // 256 sample sine wave
   0,3,6,9,12,16,19,22,25,28,31,34,37,40,43,46,
   49,51,54,57,60,63,65,68,71,73,76,78,81,83,85,88,
   90,92,94,96,98,100,102,104,106,107,109,111,112,113,115,116,
   117,118,120,121,122,122,123,124,125,125,126,126,126,127,127,127,
   127,127,127,127,126,126,126,125,125,124,123,122,122,121,120,118,
   117,116,115,113,112,111,109,107,106,104,102,100,98,96,94,92,
   90,88,85,83,81,78,76,73,71,68,65,63,60,57,54,51,
   49,46,43,40,37,34,31,28,25,22,19,16,12,9,6,3,
   0,-3,-6,-9,-12,-16,-19,-22,-25,-28,-31,-34,-37,-40,-43,-46,
   -49,-51,-54,-57,-60,-63,-65,-68,-71,-73,-76,-78,-81,-83,-85,-88,
   -90,-92,-94,-96,-98,-100,-102,-104,-106,-107,-109,-111,-112,-113,-115,-116,
   -117,-118,-120,-121,-122,-122,-123,-124,-125,-125,-126,-126,-126,-127,-127,-127,
   -127,-127,-127,-127,-126,-126,-126,-125,-125,-124,-123,-122,-122,-121,-120,-118,
   -117,-116,-115,-113,-112,-111,-109,-107,-106,-104,-102,-100,-98,-96,-94,-92,
   -90,-88,-85,-83,-81,-78,-76,-73,-71,-68,-65,-63,-60,-57,-54,-51,
   -49,-46,-43,-40,-37,-34,-31,-28,-25,-22,-19,-16,-12,-9,-6,-3
};
#endif // SINE256_TABLE


#ifdef MAP_DEMO
// x,y pairs describing an arrow terminated with 127
s08 Arrow[] = {-1,-2, -1,0,-2,0,0,2,2,0,1,0,1,-2,-1,-2,127,127};
#else
#ifdef GRAPH_DEMO
// x,y pairs describing an arrow terminated with 127
s08 Arrow[] = {-1,-2, -1,0,-2,0,0,2,2,0,1,0,1,-2,-1,-2,127,127};
#endif
#endif


#ifdef GRAPH_CODE
#include "lcd.h"
#include "graph.h"
#include "vchar.h"   // vector char
#include "adc.h"     // touch pad
#include "timer.h"
#include "menu.h"
#include "md_term.h"  // output to remote terminal -- if hooked 


// map global vars

                         // map flags / switches  
						 // left as separate bytes mainly because checkbox does not allow
						 // bit fields 

u08  mapInfoDisplay;     // reserve top line for status info, e.g. coords 
u08  mapPageFlip;        // graphics double buffer mode (flicker free)
u08  mapFullScreen;      // full screen graphics
u08  mapNorthUp;         // lock north "up" on map versus rotate map to align with heading
u08  mapShowInfo;        // show coords and heading
                         // might be redundant -- demo switches on coords after movement


u08  mapZoom;

long mapLocX;
long mapLocY;
int  mapHeading;

// end map global vars









void viewport_init(void)   // sets up default clipping to full screen
{
   VP.WinX1 = 0;
   VP.WinX2 = COLS-1;
   VP.WinY1 = 0;
   VP.WinY2 = ROWS-1;
   select_viewport(VP);  // inform viewing transforms and clipper about current viewport
}




void map_waypoint_init(void)  // clear waypoint list
{  
u08 i;

  for(i=0; i<MAX_WAYPOINTS; i++) waypoints[i].x=MAXINT;
}






void polar_ray(int r, int theta_deg)
{
int x,y;
    
   x = GraphOrgX + (r * cos360(theta_deg) >> 7);
   y = GraphOrgY + (r * sin360(theta_deg) >> 7);

   line(GraphOrgX,GraphOrgY, x,y);
}

void polar_grid(COORD cenx,COORD ceny, COORD r, u08 bands)
{
int i;

   GraphOrgX = cenx;   // set origin for plot functions
   GraphOrgY = ceny;

   for(i=1; i<=bands; i++)  circle(cenx,ceny, r*i/bands);

   // draw cartesian axes 

   hline(ceny, cenx-r,cenx+r);      // y,left,right
//   line(cenx-r,ceny, cenx+r,ceny);
   line(cenx,ceny-r, cenx,ceny+r);  
}

void polar_grid_fullscreen(u08 bands)
{
   polar_grid(COLS>>1,ROWS>>1,ROWS/4,bands);
}




// ------------------------------------------------------------------------ 
// Line Clipper - using Cohen & Sutherland's 4 bit Outcodes 
// Ron Grant 1987
// use clipping extents defined within viewport structure pointed to by pVP


#define CS_LEFT   1
#define CS_RIGHT  2
#define CS_BOTTOM 4
#define CS_TOP    8



u08 out_code(int x, int y)  // clipped line endpoint region test
{
u08 c = 0;
   
   if(x < pVP->WinX1)       c = CS_LEFT;
   else if(x > pVP->WinX2)  c = CS_RIGHT;

   if(y > pVP->WinY2)       c |= CS_BOTTOM;
   else if(y < pVP->WinY1)  c |= CS_TOP;

   return (c);
}    




u08 clipped_line(int x1,int y1, int x2,int y2)   // clipped line
{
u08 c,c1,c2;
int x,y;
long dx,dy;
u08 clip_count;
  
    // coordinates in window coordinates 
    x = 0;
    y = 0; 

    c1 = out_code(x1,y1);
    c2 = out_code(x2,y2);

    clip_count = 0;


    // while either endpoint outside window, we may need to clip provided there 
    // is a chance that the line is not completely outside.

    // Also, number of clips is limited to 4 -- in case a degenerate case 
    // locks algorithm into an infinite loop. 

    while((c1 | c2) && (clip_count++ < 4)) {
      if(c1 & c2) return(0);  // line completely outside window 
                              // e.g. both endpoints above,left,right,or below window
                              // -- other combos too, e.g. both above and left.. 

      dx = x2 - x1;
      dy = y2 - y1;

      if(c1)  c = c1;
      else    c = c2;

      // reason for this???
                                             // if p1 in or p2 out  
      //if((!c1) || (c2 & CS_TOP)) c = c2;   // on top then take p2 
      //else c = c1; 

      // clip line against one window edge
      // then update out code for that end and repeat

      // by nature of outcodes TOP and BOTTOM clips will never have a 0 dy, likewise
      // LEFT and RIGHT clips will never have a 0 dx, hence no testing for dx==0 or dy==0 is required
    
      if(c & CS_TOP) {    // clip to top FIRST 
         y = pVP->WinY1;
         x = x1 + ((dx * (long)(y-y1)) / dy);   // calc x for which y = VPminY
      }
      else if(c & CS_LEFT) {
         x = pVP->WinX1;
         y = y1 + ((dy * (long)(x-x1)) / dx);
      }
      else if(c & CS_RIGHT) {
         x = pVP->WinX2;
         y = y1 + ((dy * (long)(x-x1)) / dx);
      }
      else if(c & CS_BOTTOM) {
         y = pVP->WinY2;
         x = x1 + ((dx * (long)(y-y1)) / dy);
      }

      if(c == c1) {
          x1 = x;
          y1 = y;
          c1 = out_code(x,y);
       }
       else {
          x2 = x;
          y2 = y;
          c2 = out_code(x,y);
       }
    }

    line(x1,y1, x2,y2);  // draw line
    return 1;
}



long XFsx = 2<<7;   // fixed point scale factor  
long XFsy = 2<<7;

int XFtx = 0;
int XFty = 0;

int XFtheta = 0;


// quick transform using 8 bit sine (cosine) look up tables
// angular resolution is somewhat limited as is amplitude precision,
// good for displaying small objects

union mixed_long { 
   long Int32;
   struct { int L; int H; } Word16;
};     


void x2d_xform(int x,int y, int* xp,int* yp)
{
union mixed_long xf,yf;
long ca;
long sa;

   ca = cos360(XFtheta);  // cos & sin tables scaled by 128
   sa = sin360(XFtheta);

   xf.Int32 = ((x * ca - y * sa) * XFsx);   // perform 2D rotation & scale 
   yf.Int32 = ((y * ca + x * sa) * XFsy);   


   // could extract high words without shift here
   // scale out /128 (>>7) for sine wave amplitude and /512 (>>9)
   // for Scale fractional scale

   // xf>>16  yf>>16

   // data stored lo,hi
   // access upper 16 bits of xf,yf  effective divide by 65536  (e.g. xf>>16)
   *xp = xf.Word16.H + XFtx;   
   *yp = yf.Word16.H + XFty; 
}


void x2d_grid(int x1,int y1, int x2,int y2, int stepx,int stepy)  // draw grid using current xform
{
int x,y;
int xa,ya;
int xb,yb;

   x = x1;    // vertical grid lines 
   while(x <= x2) {
      x2d_xform(x,y1, &xa,&ya); 
      x2d_xform(x,y2, &xb,&yb);
      clipped_line(xa,ya, xb,yb);
      x += stepx;
   }

   y = y1;    // horz grid lines
   while(y <= y2) {
      x2d_xform(x1,y, &xa,&ya);
      x2d_xform(x2,y, &xb,&yb);
      clipped_line(xa,ya, xb,yb);
      y += stepy;
   }
}


void x2d_polygon(s08 *DisplayList)
{ 
int x,y;
int xp,yp;
u08 i = 0;

   xp = 0;  // elim compiler warning 
   yp = 0; 

   while((x = *DisplayList++) != 127) {
      y = *DisplayList++;
      x2d_xform(x,y, &x,&y); 
    
      vchar_char(x+5,y, (char) '0'+i);    // char 

      if(i++) clipped_line(xp,yp, x,y);   // draw line from previous x,y to current

      xp = x;
      yp = y;
   }
}



/*
2D Viewport transformations for world defined using 16 bit coordinates

question  to use canonical view port (unit) view and scale to screen coordinates?

               viewport
             +----------+
             |          |   
    Y        |          |  
    ^        |          | 
    |        +----------+ 
    |
    +--->X 
   (0,0)

*/

void view2d_viewport_clear(void) 
{
  #ifdef MDT_CODE
  mdt_viewport_clear();  // lets MD Term know that a viewport clear is following
  #endif 

  set_color(color ^ WHITE);
  filled_box(pVP->WinX1,pVP->WinY1,  pVP->WinX2,pVP->WinY2);
  set_color(color ^ WHITE);
}

void view2d_viewport_frame(void) 
{  // draw frame around outside of viewport
u08 i;
u16 temp_top;

   temp_top = top_line;

   set_draw_flags(0x00);  // fill OFF
   for(i=0; i<=double_buffers; i++) {
      draw_on_page(i);
      box(pVP->WinX1-1,pVP->WinY1-1,  pVP->WinX2+1,pVP->WinY2+1);
   }

   set_topline(temp_top);
}


void view2d_xform(int x,int y, int *xp,int *yp)
{
long ca,sa;
union mixed_long xf,yf;

   x -= pVP->OrgX;
   y -= pVP->OrgY; 

   //if (VP.Theta) {
   // could allow case for no theta
   // would need to compensate for missing *128 
   ca = cos360(pVP->Theta);       // cos & sin  -- tables scaled by 128
   sa = sin360(pVP->Theta);
   xf.Int32 = ((long) (x * ca - y * sa) * pVP->ScaleX);   // perform 2D rotation & scale 
   yf.Int32 = ((long) (y * ca + x * sa) * pVP->ScaleY);   
   //}
   
   // data stored lo,hi
   // access upper 16 bits of xf,yf  effective divide by 65536  (e.g. xf>>16)
   *xp = xf.Word16.H + pVP->EyeX;   
   *yp = pVP->EyeY - yf.Word16.H; 
}



void view2d_line (x1,y1,x2,y2) 
{
int xa,ya;
int xb,yb;

   view2d_xform(x1,y1, &xa,&ya);
   view2d_xform(x2,y2, &xb,&yb);
   clipped_line(xa,ya, xb,yb);
}



void view2d_polygon(s08 * DisplayList)
{ 
int x,y;
int xp,yp;
u08 i = 0;

   xp = 0;  // elim compiler warning 
   yp = 0; 

   while((x = *DisplayList++) != 127) {
      y = *DisplayList++;
      view2d_xform(x*10,y*10, &x,&y);    //!!! SCALE 10X

//    vchar_char(x+5,y,(char) '0'+i); 
   
      if(i++) clipped_line(xp,yp, x,y);   // draw line from previous x,y to current

      xp = x;
      yp = y;
   }
}


void view2d_grid(int x1,int y1, int x2,int y2, int stepx,int stepy)  
{   // draw grid using current xform
int x,y;
int xa,ya;
int xb,yb;

   x = x1;         // vertical grid lines 
   while(x <= x2) {
      view2d_xform(x,y1, &xa,&ya); 
      view2d_xform(x,y2, &xb,&yb);
      clipped_line(xa,ya, xb,yb);
      x += stepx;
   }

   y = y1;         // horz grid lines
   while(y <= y2) {
      view2d_xform(x1,y, &xa,&ya);
      view2d_xform(x2,y, &xb,&yb);
      clipped_line(xa,ya, xb,yb);
      y += stepy;
   }
}


void clipped_box(int x1,int y1, int x2,int y2) 
{
   clipped_line(x1,y1, x2,y1);
   clipped_line(x2,y1, x2,y2);
   clipped_line(x2,y2, x1,y2);
   clipped_line(x1,y2, x1,y1);
}



#ifdef MAP_DEMO

void view2d_draw_waypoints(void)
{
u08 i;
int xa,ya;
int prev_x,prev_y;

   prev_x = MAXINT;
   prev_y = MAXINT;
  
   i = 0;
   while ((i<=MAX_WAYPOINTS) && (waypoints[i].x != MAXINT)) {
      view2d_xform(waypoints[i].x,waypoints[i].y,&xa,&ya);
  
      if(prev_x != MAXINT) {
         clipped_line(prev_x,prev_y, xa,ya);     // BOLD line
         clipped_line(prev_x+1,prev_y, xa+1,ya);
         clipped_line(prev_x,prev_y+1, xa,ya+1);
      }

      clipped_box(xa-3,ya-3, xa+3,ya+3);
      vchar_char(xa+5,ya, waypoints[i].id); // char  

      prev_x = xa;
      prev_y = ya;

      i++;
   }  
}


// 2D Map
// for example:  can be used to locate a robot on a grid
// map_init is called then map_update is called periodically 
// map_demo provides an example of this
// Also, robot example in balance.c provides an example of combining the map with 
// functions to update robot



long mapLastUpdate;


//#define map_flag(f)      (bit_is_set (mapFlags,(f)))
//#define map_setflag(f)   (mapFlags |= _BV(f))
//#define map_clrflag(f)   (mapFlags &= ~_BV(f))
//#define map_toggleflag(f)(mapFlags ^= _BV(f))     // bit toggle XOR



void map_home(void) {
  mapHeading = 0;     // default heading 0 = NORTH  (90=EAST 180=SOUTH 270=WEST)
  mapLocX = 0;        // location  fixed point 24.8 
  mapLocY = 0;         

}



void map_re_init(void) {

//   re_init:

   if(mapFullScreen) {
      VP.WinX1 = 1;
      if(mapShowInfo) VP.WinY1 = CHAR_HEIGHT+2;
      else VP.WinY1 = 1;

      VP.WinX2 = COLS-2;
      VP.WinY2 = ROWS-2;
   }
   else {
      VP.WinX1 = 1;

      if(mapShowInfo) VP.WinY1 = CHAR_HEIGHT+2;
      else VP.WinY1 = 1;

      #ifdef RES_128x64
        VP.WinX2 = COLS-CHAR_WIDTH*4;
        VP.WinY2 = ROWS-CHAR_HEIGHT*2;
      #else
        VP.WinX2 = COLS-CHAR_WIDTH*3;
        VP.WinY2 = ROWS-CHAR_HEIGHT*2;
      #endif
   }
 
   VP.EyeX  = (VP.WinX1+VP.WinX2)/2;
   VP.EyeY  = (VP.WinY1+VP.WinY2)/2;

   VP.Theta = 0;      // rotated view

   select_viewport(VP);  // set current viewport for line clipper and viewing transforms
   
 
   
   // limited double buffer support for now
   // using checkbox
   // is there reasonable means handle button press updates.. and 
   // draw support double buffer

   set_color(WHITE);

   page_reset();
   if(mapPageFlip) {
      page_buffers(2);   // we are drawing with two pages
   }


   MENU_INIT

}



void map_init(void) {

   mapInfoDisplay = 1;
   mapPageFlip = 1;
   mapFullScreen = 0;
   mapShowInfo  = 1;
   mapNorthUp = 0;    // lock north "up" on map versus rotate map to align with heading

   map_home();

   // set default zoom

   #ifdef RES_128x64   // e.g lower res AGM Panel default zoom
      mapZoom = 33;
   #else
      mapZoom = 50;   
   #endif


   vchar_init();      // int vector chars used with waypoints
   printf_flags &= (~VCHAR_ERASE);
 
   VP.OrgX = 0;          // set viewport center at origin 
   VP.OrgY = 0;
   VP.ScaleX = (2 *128); // scale in 8.8 fixed point  
   VP.ScaleY = (2 *128);

   mapLastUpdate = 0;     // time of last update used for throttling map redraw

   map_re_init();

  

}




char map_update(void) {   // return command char

  char cmd = 0;


   // touch top of screen to toggle number display

   if((touchY >= 0) && (touchY <= CHAR_HEIGHT)) {

      wait_while_touched();

      mapShowInfo ^= 1; // toggle ShowInfo 
          
          map_re_init();
      return ('X'); // flag  re_init;  view change
          
   }

   // touch viewport to toggle full screen mode
   if((touchX >= VP.WinX1) && (touchX <= VP.WinX2)) {
      if((touchY >= VP.WinY1) && (touchY <= VP.WinY2)) {
         wait_while_touched();
             mapFullScreen ^=1;  // toggle full screen
         map_re_init();
         return ('X');  // flag re_init;
      }
   }


      MENU_CONTROLS

      if(!(mapFullScreen)) {
         if(mapShowInfo) menu_label(0,0, PS(" Map Demo"));
#ifdef RES_128x64
         menu_buttonrow(0,ROWS-CHAR_HEIGHT-4, PS("\x1B\x18\x19\x1ASH"));
         menu_sliderV(COLS-CHAR_WIDTH*3,CHAR_HEIGHT*2,  // x,y
                      CHAR_HEIGHT*3,                    // height
                      PS("Z"),     // label  
                      mapZoom,1,99,   // variable, min, max
                      1,3);        // show value,response code
         menu_checkbox(COLS-CHAR_WIDTH*5,ROWS-CHAR_HEIGHT*1-2, PS("N"), mapNorthUp,1);  
         menu_checkbox(COLS-CHAR_WIDTH*2-CHAR_WIDTH/2,ROWS-CHAR_HEIGHT*1-2, PS("P"), mapPageFlip,2);
         menu_exitbutton();
#else
         menu_buttonrow(0,ROWS-CHAR_HEIGHT-4, PS("\x1B\x18\x19\x1A S H"));
         menu_sliderV(COLS-CHAR_WIDTH*2+2,CHAR_HEIGHT*3,  // x,y
                      CHAR_HEIGHT*3,                    // height
                      PS("ZM"),   // label  
                      mapZoom,1,99,
                      1,3);   // show value,response code
         menu_checkbox(COLS-CHAR_WIDTH*2-CHAR_WIDTH/2,ROWS-CHAR_HEIGHT*1-0, PS("N"), mapNorthUp,1);  //xy label var response code
         menu_checkbox(COLS-CHAR_WIDTH*2-CHAR_WIDTH/2,ROWS-CHAR_HEIGHT*2-2, PS("P"), mapPageFlip,2);
         menu_exitbutton();
#endif


      }

 
      if(MenuDraw) {
         view2d_viewport_frame();
      }

   
      MENU_COMMANDS

      // menu button/control responses

      switch(menu_cmd()) {
         case  2 : page_reset();
                   if(mapPageFlip) page_buffers(2);
                   MenuDraw = 1;
                   break;

         case 0x1B: cmd = 'L'; break;  // left arrow pressed (let caller handle)
         case 0x18: cmd = 'U'; break;  // up arrow 
         case 0x19: cmd = 'D'; break;  // down arrow
         case 0x1A: cmd = 'R'; break;  // right arrow
       

         case 'H': cmd = 'H'; break;
                 case 'S': cmd = 'S'; break;
        


      }
#ifndef USER_INPUT
      if(VP.OrgY >= 127) return; // !!! changed break;   //!!! since no touchscreen to exit us
#endif

      // if time to update (5 Hz)
      if((mapLastUpdate-get_msecs_alive() ) > 200) {
         mapLastUpdate = get_msecs_alive();

   

         if(mapNorthUp) {  // North Up Checkbox  []N
            VP.Theta = 0;        
         }
         else {  
            VP.Theta = mapHeading;   // !!! was 90-
            if(VP.Theta < 0)   VP.Theta += 360;
            if(VP.Theta > 359) VP.Theta -=360;
         }
    

         VP.OrgX = mapLocX >> 8;   
         VP.OrgY = mapLocY >> 8;

         VP.ScaleX = mapZoom*20;
         VP.ScaleY = mapZoom*20;;



         if(mapPageFlip) lcd_pageflip();

         view2d_viewport_clear(); 

         //view2d_line(0,0,0,500);  // axes
         //view2d_line(0,0,500,0);

         view2d_polygon(Arrow);
         view2d_grid(-96,-96,96,96,12,12);  // 2D grid step 12,12    e,g, 16'x 16'  grid with 12"x12" tiles
     

         vchar_set_fontsize(1,1);     // multiple of starburst 4x6 pixel scale
         vchar_set_thickness(2,1); 

         view2d_draw_waypoints();

      } // end if time to update


   return (cmd);
}



// interactive map demo


void map_demo(void)
{
  char cmd = 0;
 
   map_init();

   s08 dh = 0;            // delta heading and delta speed for a virtual vehicle moving
   s08 ds = 0;            // on map


   #ifndef USER_INPUT
     ds = 1;
   #endif


   map_waypoint_init();                // define some optional waypoints on the map
   map_waypoint_def(0,'A', 0,0);
   map_waypoint_def(1,'B', 50,25);
   map_waypoint_def(2,'C', 0,80);
   map_waypoint_def(3,'D', -80,30);


  
   do {
      cmd = map_update();  // Draw Map and Read User Input (button presses) 

      // more drawing can be done here
 
      if ((dh || ds)) 
          if (mapShowInfo) mapInfoDisplay = 1;


      if(mapInfoDisplay) {
         lcd_setxy(0,0);
         printf(PS("%4d %4d H%3d"), VP.OrgX,VP.OrgY, mapHeading);
      }


      #ifdef MDT_CODE
         // if(mdtHooked) delay_100ths(20); // throttle temporary for debug !!!
      #endif


      // handle map commands
          // LRFB used to accelerate motion in this example (through repeated button presses)
      
          switch (cmd) {
          case 'L' : dh--; break;   // left arrow  = accelerate left : counterclockwise 
      case 'U' : ds++; break;   // up arrow    = forward (accel.)  
      case 'D' : ds--; break;   // down arrow  = back (accel.)
      case 'R' : dh++; break;   // right arrow = accelerate right

      case 'H' : map_home();        // GoHome and continue to "Stop" code (no break)
      case 'S': dh=0; ds=0; break;  // Stop
          } // end switch


      mapHeading += dh;
      if(mapHeading > 359) mapHeading -= 360;    // keep in range 0..359
      if(mapHeading < 0)   mapHeading += 360;

      //             Y 
      //     0       |
      //  270+090    +->X
      //    180
   
      // calculate new x,y position based on speed and direction
      // ds = distance traveled in one unit of time e.g. 1/5th of a second
      // Heading angle in degrees 
    
      mapLocX += (long) ds*sin360(mapHeading);  // sin360 & cos360  range +/-127 (fixed pt. 0.8)
      mapLocY += (long) ds*cos360(mapHeading);


     


   } while (menu_cmd() != MENU_EXITCODE);   // repeat until exit command

   wait_while_touched();

   viewport_init();
   vchar_init();
   page_reset();  // end of double buffered graphics drawing
   lcd_clear();
   set_color(WHITE);
}
#endif // MAP_DEMO



#ifdef GRAPH_DEMO

void graph_demo(void)
{
int i;
int sc;

   XFtx = COLS/2;
   XFty = ROWS/2;

   XFsx = 512*10;
   XFsy = 512*10;

   XFtheta = 45;
   sc = i = 0;


   vchar_init(); 
   vchar_set_fontsize (1,1);  
   page_buffers(2);   // drawing with two pages


   while(1) {  
     if(get_touch(1)) break;
  
     lcd_pageflip();

     lcd_clear();

     x2d_grid (-10,-10,10,10,2,2);  // 2D grid step 2

     // draw multiple arrows to make arrow stand out
     
     // note scale is fixed point 25.7  (128=1.0) 
         // sin360 is fixed point 0.8  range is (+127 to - 127)
   
     // arrow reversal is normal here using sine function for scale 
         // note: needed to cast 400 long to prevent overflow
         // XFsx and sy are 32 bit (25.7 bit fixed point)

     XFsx = 400L * sin360(sc);
     XFsy = XFsx;
  
     //for(i=0; i<3; i++) {
     {
       XFtx = (int) (COLS/2) + sin360(XFtheta);  // note: XFtx and ty are integer values
       x2d_polygon(Arrow);                       // sin360 range (+127 to -127)
       XFtheta = (XFtheta+2) % 360; 
     }

     sc = sc + 1;   
#ifndef USER_INPUT
     if(sc >= 180) break;  // no input device, terminate demo after 180 frames
#endif
     if(sc >= (360*2)) sc = 0;
   } 

   wait_while_touched();
   page_reset();  // end of double buffered graphics drawing
   lcd_clear();
}

#endif // GRAPH_DEMO
#endif // GRAPH_CODE
