/*  MegaDonkey Library File:  menu.h   Support for simple GUI style controls  - Header File
    


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



    Megadonkey Menu Support
    Ron Grant  April 16,2007

    Pimped    27 APR 07 - Mark Sims
    Keyboard by Mark Sims -> moved to kbd.c/.h


    Support for simple GUI style controls
    using a code framework that minimizes storage requirements

    menu_demo() provides example usage of the menu functions.


    DonkeyWiz, Windows Application, is an interactive tool that allows you create menus then generates code
	for you to build upon. See: Mega Donkey User's Guide 

*/

#ifndef _MENU_H_
#define _MENU_H_

#include "md.h"
#include "lcd.h"


#ifdef MENU_CODE

#define BUTTON_SIZE  char_size
#define MENU_EXITCODE     255
#define MENU_BUTTONLABEL  254
 u08 MenuCmd;    // button press or control changed code
                       // each menu item should have unique response code defined
 u08 MenuDraw;   // global menu state 



// u08  ButtonPressed;  // current button pressed
 u08   ButtonBorder;     // button text to frame border size 
 u08   ButtonRowSpacing; // number of pixels between buttons beyond CHAR_WIDTH+ButtonBorder
 u08   ButtonColSpacing; // number of pixels between buttons beyond CHAR_HEIGHT+ButtonBorder
 u08   MenuBeep;         // beep on menu press 0=OFF 1=ON   2..255 variable duration in 100ths sec

 COLOR invert_buttons;   // set to WHITE for white on black buttons



u08 ScrollTouched;                 // flag set if scrollbox is touched
int ScrollFirstX, ScrollFirstY;    // where the box was first touched
int ScrollLastX, ScrollLastY;      // where the box is currently touched
int ScrollDeltaX, ScrollDeltaY;    // change in touch position since last check
int ScrollSpeedupX, ScrollSpeedupY;// distance from initial touch (always positive)
                                   // (can be used to scale touch deltas)




#define MENU_INIT     menu_init(); 
#define MENU_CONTROLS menu_begin();
#define MENU_COMMANDS MenuDraw = 0;


// slider control   Note: Use menu_slider() macro which  does implicit assignment of result


long menu_slider1(
   COORD x,COORD y, 
   COORD width, 
   char *label,            // optional label, empty string OK for no label
   long value,             // input data -- modified result returned as function result
   long min,long max,      // range of values to return from one extent of control to other
   u08 ShowValue,          // 0=Hide Value
   u08 ResponseCode        // user response code that will be returned when calling menu_cmd() 
                           // if action need be taken when slider position is changed -- other than the variable 
                                                   // getting changed.
);

long menu_slider_v1(     // vertical slider -- use menu_sliderV macro
   COORD x,COORD y,      // slider upper left corner
   COORD height,         // slider height  label placed above and value if placed below
   char *label,          // label -- typically 1 or 2 chars recommended (empty string OK)
   long value,           // slider's value -- passed to function and returned and assigned  
   long min,long max,    // slider range
   u08 ShowValue,        // 0= Hide Value 
   u08 ResponseCode
);

#define menu_sliderV(x,y,h,t,d,min,max,ShowVal,r) d=menu_slider_v1((x),(y),(h),(t),(d),(min),(max), (ShowVal),(r))
#define menu_vslider(x,y,h,t,d,min,max,ShowVal,r) d=menu_slider_v1((x),(y),(h),(t),(d),(min),(max), (ShowVal),(r))
#define menu_slider(x,y,w,t,d,min,max,ShowVal,r)  d=menu_slider1((x),(y),(w),(t),(d),(min),(max), (ShowVal),(r))

// close button: "X" in  top right corner,  standard MENU_EXITCODE
#define menu_exitbutton()  menu_button(COLS-((CHAR_WIDTH*BUTTON_SIZE)+(ButtonBorder*2)),0, PS("X"), MENU_EXITCODE);


 u08 keyboard_drawn;

void menu_init(void);
void menu_begin(void);

void SoundMenuBeep(void);


void menu_button(COORD x,COORD y, char *label, u08 ResponseCode);

u08  menu_checkbox_1(COORD x, COORD y, char *label, u08 data, u08 ResponseCode);

#define menu_checkbox(x,y,label,d,r) d = menu_checkbox_1(x,y,label,d,r)


void menu_buttonrow(COORD x,COORD y, char* buttons);
void menu_buttoncol(COORD x,COORD y, char *buttons);


void init_scrollbox();
u08  menu_scrollbox(int left,int top,  int right,int bottom,  u08 draw_box, u08 ResponseCode);


long menu_calc(char *title, u08 math_ok);

void menu_call(void p(void));
void menu_label(COORD x,COORD y, char *label);
u08  menu_cmd(void);

#ifdef MENU_DEMO
void menu_demo(void);
#endif


void stored_menu_demo(void);

#endif // MENU_CODE

#endif // _MENU_H
