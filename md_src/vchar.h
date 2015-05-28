/*  MegaDonkey Library File:  vchar.h   Vector Character Support - Header File
    


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



  vchar.c   Megadonkey Vector Character Set Support
            Used mainly with 2D Wireframe graphics

  Ron Grant
  April 18, 2007

  03 May 2007 - 16 bit coords & using clipped_line  
  25 May 2007 - modified text/printf routines for greater compatibility
                with bitmapped text routines - Mark Sims

*/

#ifndef _vchar_h
#define _vchar_h


#ifdef VCHAR_CODE

u08 VCharSpaceX;     // horz spacing between chars
u08 VCharSpaceY;     // vert spacing between lines

u08 VCharThicknessX; // horz repeat with 1 pixel spacing
u08 VCharThicknessY; // vert repeat with 1 pixel spacing



void vchar_init(void);                               // call this before using vector character functions 

void vchar_set_fontsize(u08 scaleX,  u08 scaleY);    // multiple of starburst 4x6 pixel scale
void vchar_set_thickness(u08 thick_x, u08 thick_y);  // stroke thickness default 1,1
void vchar_set_spacing(u08 space_x, u08 space_y);    // character spacing default 2,2
void vchar_set_slant(int degrees);     // set char slant angle (from vertical)

void vchar_char(int x,int y, u08 c);           // print a single character at x,y

void vchar_erase(COORD x,COORD y);             // erase character cell at x,y
void vchar_text(COORD x,COORD y, char * s);    // print a string starting at x,y
void vchar_textPM(COORD x,COORD y, PGM_P s);   // print a PROGMEM string starting at x,y
#define vchar_textPS(col, row, s)  vchar_textPM(col, row, PSTR(s))

void vchar_set_stdout(void);   // direct stdout to vector char printer
                               // to enable printf("  ") 

void vchar_demo(void);
void show_vchar_set(u08 sx, u08 sy);
void show_huge_vchars();
 

#endif // VCHAR_CODE
#endif // _vchar_h
