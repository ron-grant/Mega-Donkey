/*  MegaDonkey Library File:  curves.h     Parameteric Line/Arc/BezierCurve Generator   Header File
    


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

#ifndef _CLH_
#define _CLH_

#include "md.h"

#ifdef BEZIER_CODE
#include <math.h>

                
// Draw (move) Commands
// Drill command is special case handled by main, but acts
// like other drawing commands

#define DC_NULL      1
#define DC_PAR_LINE  2
#define DC_PAR_ARC   3
#define DC_DELTA_ARC 4
#define DC_BEZ       5
#define DC_DRILL     6   

#define NULLCmd DrawCmdActive=DC_NULL

 long CurX,CurY;   // Current X,Y point as entity drawing progresses
               
 float u,du,       // parameter u varried from 0 to 1
                  // du = delta u per point evaluation
      X1,Y1,      // 4 points defining bezier curve hull
      X2,Y2,      // also used by other drawing routines
      X3,Y3,      // e.g. X1,Y1 Circle Center
      X4,Y4;

 float ParArcRadius,       // Parametric Arc Params     
      ParArcStartAngle,
      ParArcEndAngle;

 u08  DrawCmdActive;       // Indicate Drawing Command Active 0=Inactive 

                           // new 10/2009
 u08  duParamMode;         // du specified versus steps  as param to line,arc,bez curve init
                           // long int param interpreted as 32 bit float
						   // (actual 32 bit floating point # is passed to func, that is)

u08  next_point(void);    // generic next point calls appropriate next point function
                          // returns DrawCmdActive value, 0 if drawing finished

void bezier_init(long steps);
void bezier_nextpoint (void);  // eval next point setting CurX CurY 
void bezier_test(long steps);

// parametric line uses
// BezX1,BezY1  BezX2,BezY2
void par_line_init (long steps);
void par_line_nextpoint (void);     

        
// parametric arc uses
// BezX1,Y1  for Center 
// ParArcRadius,ParArcStartAngle,ParArcEndAngle 
void par_arc_init (long steps);
void par_arc_nextpoint (void);     


void draw_donkey_logo(void);
void bezier_edit_demo(void);


#endif // BEZIER_CODE

#endif // _CLH_
