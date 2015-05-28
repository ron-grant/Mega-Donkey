/*  MegaDonkey Library File:  calc.c    4 Function Calculator & Numerical Input Keypad
    


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

 


   Mega Donkey Donkulator (4 Function Calculator / Numerical Input Keypad) 

   Ron Grant  2007
   Mark Sims  Major Enhancementage

   Example use of menu system -- somewhat complex.

   *** BEWARE:  all math uses 32 bit scaled integers
                Be aware overflows and limited decimal precision...
                Be very aware...

   Good illustration of menu_buttonrow function

   
   Note: Check Out use of DonkeyWiz Menu Designer Windows Application
         Allows building a menu framework interactively.
   	


*/

#ifndef _CALC_H
#define _CALC_H

#ifdef MENU_CODE        // calculator supported only if menu code present

#ifdef RES_128x64
#define DISPL 13        // calculator edit/display box length 
#else                   // shorten to 13 digits for AGM LCD Panel  
#define DISPL 16        
#endif

#define SCALE 100     // default calculator scaled fraction scale factor 
 long calc_scale;     // calculator scaled fraction scale factor
 u08 HexMode;         // calculator hex mode flag

#define NO_MATH         0x01
#define EVAL_TITLE      0x02
#define HEX_MODE        0x04
#define FIX_MODE        0x08
#define FRAC_MODE       0x10
#define NO_DP_CHANGE    0x20
#define NO_BASE_CHANGE  0x40



long menu_calc(char *title, u08 options); // Function Calculator Integer Hex / Decimal

#endif
#endif
