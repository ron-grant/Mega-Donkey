/*  MegaDonkey Library File:  md_term.h   Megadonkey Terminal - Link to PC DonkeyTerm  -- Header File                                                                                       


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



   Mega Donkey Terminal Support
   Ron Grant November 2007

   Mark Sims pimpage December 2007  


   md_term Provides interface to serially linked PC Windows Applicaiton,
   MegaDonkey Terminal -- aka MD Term.

   
   Two modes of operation are possible.

   1. Graphics and Text can be sent to MD Term independently of what is sent to LCD.

   2. Local LCD drawing functions can have a hook enabled
   that allows both local and remote display of graphics. Also, mouse messages generated 
   by remote MD Term can be read by Mega Donkey. This allows remote operation of Mega
   Donkey.

   If you are using the mega donkey library with your application then you will need
   to include this header in your application and call mdt_init(PORT) after md_init().
      




   02/08/2013 these macros defined in lcd.h must have test to insure no chars sent if
              mdtHooked=0

              got burned by set_topline called when button pressed sending 
			  chars to serial port when MD Term not enabled , FF 28 00 00 was getting sent
			  ack!




   #define set_rotate(x)       mdt_set_rotate(x)
   #define set_draw_flags(x)   mdt_set_draw_flags(x)
   #define set_color(x)        mdt_set_color(x)
   #define set_bg(x)           mdt_set_bg_color(x)
   #define set_charsize(x)     mdt_set_char_size(x)
   #define set_topline(x)      mdt_set_topline(x)     









*/


#ifndef H_MD_TERM
#define H_MD_TERM

#ifdef MDT_CODE

#define MDT_BAUD_RATE 115200L
#define MDT_COM_PORT  0



// 6 bit color constants  00RR GGBB
// not full list of color combinations
// used with mdt_textcolor, mdt_fillcolor, mdt_bgcolor, and mdt_pencolor functions

#define MDT_BLACK     0
#define MDT_WHITE     0x3F
#define MDT_LIGHTGRAY 0x2A
#define MDT_DARKGRAY  0x15
#define MDT_RED       0x30
#define MDT_YELLOW    0x3C
#define MDT_GREEN     0x0C
#define MDT_CYAN      0x0F
#define MDT_BLUE      0x03
#define MDT_MAGENTA   0x33
#define MDT_DARKBLUE  0x01
#define MDT_DARKRED   0x10
#define MDT_DARKGREEN 0x04
#define MDT_LIME      0x1D  // 01 11 01


// mouse variables updated by mdt_mouse_update() which must be called peridoically
// (call embedded in mouse OR touch panel routines)
// when data is used by mouse or touch panel routines mdtMouse state should be cleared
// via mdtMouseEventClear()


#define MDT_MOUSEL    0x01
#define MDT_MOUSER    0x02
#define MDT_MOUSE_MSG 0x04

#define mdtMouseLB    (mdtMouseState & MDT_MOUSEL)
#define mdtMouseRB    (mdtMouseState & MDT_MOUSER)
#define mdtMouseEvent (mdtMouseState & MDT_MOUSE_MSG)
#define mdtMouseEventClear() mdtMouseState = 0


u08 mdtHooked;          // when set to 1  calls to LCD primitives are sent also to MD Term
u08 mdtNoLocalDisplay;  // set to 1 if using MDT without any LCD panel

u08 mdtMouseEnabled;           
volatile u08 mdtMouseState;
volatile u16 mdtMouseX;
volatile u16 mdtMouseY;




void mdt_mouse_update(void);  // called from 10 KHz timer tick routine (timer.c)
                              // PC based Donkey Term, throttles events
                              // so as not to overload Donkey


void mdt_init (u08 PortNum);  // Start up MD Term communicaton on serial port 0 or 1

void mdt_send (u08 b);        // low level byte send to selected port 0 or 1


#ifdef MDT_DEMO
void mdt_demo(void);          // demo how commands can be sent in both hooked and not
#endif                        // hooked state  



/* 
   Functions to Hook into LCD graphics functions
   allowing graphics to be "drawn" on remote PC screen via serial data link

   The functions below can also be called directly.
   Also, the LCD output need not be hooked.
   
   That is, you may wish to output text and/or draw something entirely different
   on remote PC than is drawn on local LCD.  

   Note that if MD Term is not used. The symbol MDT_CODE can be left undefined which
   completely removes any overhead from LCD graphics routines. 

*/


void mdt_lcd_hook(void);  // sends ROWS,COLS,CHAR_HEIGHT,CHAR_WIDTH so PC can scale
                          // graphics to window
                          // also sets MDTHooked which sends LCD primatives to remote PC terminal

#define mdt_lcd_unhook() mdtHooked=0          // unhook graphics
#define mdt_mouse_unhook() mdtMouseEnabled=0  // unhook mouse


// these functions are are called by hooks in LCD functions

void mdt_set_rotate(u08 x);
void mdt_set_draw_flags(u08 x);
void mdt_set_char_size(u08 x);
void mdt_set_color(u08 x);
void mdt_set_bg_color(u08 x);

void mdt_setxy(int x,int y);
void mdt_charxy(char c, int x, int y);  // output char at X,Y  (col,row)

void mdt_dot(int x, int y);    // draw a dot
void mdt_line(int x1, int y1, int x2, int y2);    // draw line from x1,y1 to x2,y2
void mdt_hline(int x1, int x2, int y); 
void mdt_box(int x1, int y1, int x2, int y2);
void mdt_filled_box(int x1, int y1, int x2, int y2);
void mdt_shaded_box(int x1,int y1, int x2,int y2); // LCD E. and Windows Canvas
void mdt_arc(int x,int y,int r, int a1, int a2, int thickness);
void mdt_ellipse(int x, int y, int major, int aspect);
void mdt_bezcurve (int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4,int steps); 
void mdt_blit(int left,int top, int right,int bot, int dest_left,int dest_top);

// !!! for now returns true always -- not value from PC 

int  mdt_paint(COORD xi, COORD yi, COLOR bound);


void mtd_set_topline(unsigned x);
void mdt_set_lcdtop (unsigned row);

//void mdt_set_displaytop(unsigned top);


void mdt_clear(u08 color);   

void mdt_scroll(int rows);     // scroll display in vertical direction                                
void mdt_setcolor(void);       // sets color and bg_color



void mdt_viewport_clear(void);  // followed by filled box
                                // flags terminal to update screen before clear


#define mdt_putchar(c) mdt_send(c)


// more functions -- these called directly 
// not by any of the LCD hooked calls

void mdt_fontsize(u08 size);                      // set font size in points
void mdt_textcolor24(u08 r, u08 g, u08 b);        // text color full 24 bit spec
void mdt_textcolor(u08 b);                        // text color 6 bit see color constants
void mdt_fillcolor24(u08 r, u08 g, u08 b);
void mdt_fillcolor(u08 b);
void mdt_pencolor24(u08 r, u08 g, u08 b);
void mdt_pencolor(u08 b);
void mdt_bgcolor24(u08 r, u08 g, u08 b);
void mdt_bgcolor(u08 b);
void mdt_font(char *s);
void mdt_ellipse2(int x1, int y1, int x2, int y2);
void mdt_lineto(int x1,int y1);
void mdt_moveto(int x1,int y1);

void mdt_forced_setcolor(void);


void mdt_linewidth(u08 width);
void mdt_fillmode(u08 mode);   // 0=NO FILL, 1=SOLID FILL  (applied to box and ellipse)



#endif
#endif
