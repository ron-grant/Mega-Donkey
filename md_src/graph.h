/*  MegaDonkey Library File:  graph.h    2D Wireframe Graphics   -- Header File
    


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




#ifndef _GRAPH_H
#define _GRAPH_H

#include <avr/pgmspace.h>  // support for vars stored in prog mem 
#include "md.h"
#include "lcd.h"           // def of COORD


// fast 8 bit lookup integer sine & cosine functions
// range +/-127 
#ifdef SINE360_TABLE
extern signed char SinT360[];
#define sin360(t) (s08) pgm_read_byte_near(&SinT360[(t) % 360])
#define cos360(t) (s08) pgm_read_byte_near(&SinT360[((t)+90) % 360])
// single cycle sine look up (theta must not exceed 1 cycle limits  359 for sin360  or 255 for sin256
#define sin360_1(i) pgm_read_byte_near(&SinT360[i])
#endif

#ifdef SINE256_TABLE
extern signed char SinT256[];
#define sin256(t) (s08) pgm_read_byte_near(&SinT256[(t) & 0xFF])
#define cos256(t) (s08) pgm_read_byte_near(&SinT256[((t)+64) & 0xFF])
#define sin256_1(i) pgm_read_byte_near(&SinT256[i])
#endif


#ifdef GRAPH_CODE

 long XFsx;   // fixed point scale factor  
 long XFsy;

 int XFtx;
 int XFty;

 int XFtheta;


 COORD GraphOrgX;   // graph origin (in screen coordinates)
 COORD GraphOrgY;





typedef struct {  // waypoint list element
   int x,y;  // coordinate (inches)   (approx 1 square mile addressable space)
   char id;  // identifier
} waypoint;


#define MAX_WAYPOINTS 8
waypoint waypoints[MAX_WAYPOINTS];  // map waypoint list



typedef int int88;

typedef struct {
   int OrgX,OrgY;       // view center in world coordinates
   int Theta;           // viewport rotation with respect to world (0..359)
   int Width,Height;    // viewport width and height in world coordinates
                        // OR do we specify scale directly rather than doing   
                        // indirect calculations to map world viewport to screen viewport

   int88 ScaleX,ScaleY; // fixed point scale

   int   WinX1,WinY1,WinX2,WinY2; // viewport window screen coordinates 
                                  // Note: using unsigned values here caused problems with
                                                                  // clipper outcode function, that is, comparison of 
                                                                  // negative ordinate against window edge was failing --
                                                                  // e.g. x=-1  left=10   (x<left) was generating FALSE

   int EyeX,EyeY;
} viewport;


 viewport *pVP;  // pointer to current viewport used by clipper...
 viewport VP;    // Default Instance of viewport (more can be defined)




  
#define select_viewport(v) pVP=&v    /* point to current vieport */


void polar_ray  (int r, int theta_deg);  // r, theta
void polar_grid (COORD cenx, COORD ceny,COORD r, u08 bands);
void polar_grid_fullscreen(u08 bands);  // draw full screen polar grid (centered at screen center)

u08 clipped_line(int x1, int y1, int x2, int y2);   // clipped line

void viewport_init(void);
void x2d_xform(int x, int y, int* xp, int* yp);
void x2d_grid (int x1, int y1,int x2,int y2,int stepx,int stepy);  // draw grid using current xform
void x2d_polygon (s08 * DisplayList);


// map globals

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


void map_init(void);       // call when preparing to use map
void map_home(void);       // moves center of view to origin with heading north (along +Y)
void map_re_init(void);    // called when toggling from full screen to window view
char map_update(void);     // called periodically to redraw map and get button presses


void map_waypoint_init(void);
#define map_waypoint_def(n,c, x1,y1) waypoints[n].id=(c); waypoints[n].x=(x1); waypoints[n].y=(y1);

 
void map_demo(void);       // demo usage of map including waypoints
                           

void graph_demo(void);


void waypoint_init(void);  // clear waypoint list
void view2d_viewport_frame(void); 
void view2d_xform(int x,int y, int *xp,int *yp);
void view2d_viewport_clear(void);
void view2d_polygon(s08 * DisplayList);
  
void view2d_grid(int x1,int y1, int x2,int y2, int stepx,int stepy);
void view2d_draw_waypoints(void);



#endif  // GRAPH_CODE
#endif  // _GRAPH_H
