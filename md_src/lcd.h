/*  MegaDonkey Library File:  lcd.h    Low-Level LCD Graphics Support  - Header File
    


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


   Megadonkey Bitmapped Graphics LCD Module Support
   Mark Sims
   Jan 2007

*/


#ifndef _lcd_h  // include switch for entire file - #endif final line
#define _lcd_h
#ifdef PANEL_CODE


#include <avr/pgmspace.h>


// #define REALMAJOR       /* controls major axis for drawing ellipses */

extern u08 pixel_masks[];



#ifdef KS0107  // Samsung KS0107/KS0108 controller
   #define RES_128x64
   #define ASPECT 256   /* 256 for square pixel panels */
   #define ROWS   64
   #define COLS   128
   #define COORD     unsigned char
   #define INCREMENT char       /* if COORD is not char,  this should be int */
   #define PAGES  (ROWS/8)
   #define NO_OFFSCREEN    // panel has only one full page of display memory
   #define REVERSE_DB
   #define PIXELS_PER_BYTE 8
   #define COLOR unsigned char
   #define RGB(r,g,b) (((r)||(g)||(b))?0xFF:0x00)
   #define get_dot(col, row) ((read_screen(col, row) & pixel_masks[(row)&0x07])?WHITE:0)
#endif

#ifdef KS0713  // Samsung KS01713 and S6B1713 controllers
   #define RES_128x64
   #define ASPECT 256   /* 256 for square pixel panels */
   #define ROWS   64
   #define COLS   128
   #define COORD     unsigned char
   #define INCREMENT char       /* if COORD is not char,  this should be int */
   #define PAGES  (ROWS/8)
   #define NO_OFFSCREEN  // panel has only one full page of display memory
   #define REVERSE_DB
   #define PIXELS_PER_BYTE 8
   #define COLOR unsigned char
   #define RGB(r,g,b) (((r)||(g)||(b))?0xFF:0x00)
   #define get_dot(col, row) ((read_screen(col, row) & pixel_masks[(row)&0x07])?WHITE:0)
#endif

#ifdef SED  // Epson SED1355 controller (used on larger LCD panels)
   #define RES_320x240
   #define ASPECT 256   /* 256 for square pixel panels */
   #define ROWS   240
   #define COLS   320
   #define COORD     unsigned
   #define INCREMENT int     /* if COORD is not char,  this should be int */
   #define PIXELS_PER_BYTE 8
   #define COLOR unsigned char
   #define RGB(r,g,b) (((r)||(g)||(b))?0xFF:0x00)
   #define get_dot(col, row) ((read_screen(col, row) & pixel_masks[(col)&0x07])?WHITE:0)
#endif

#ifdef TOSHIBA    // Toshiba T6963 controller
   #define RES_160x128
   #define ASPECT 256   /* 256 for square pixel panels */
   #define ROWS   128
   #define COLS   160
   #define COORD     unsigned char
   #define INCREMENT char        /* if COORD is not char,  this should be int */
   #define PIXELS_PER_BYTE 8
   #define COLOR unsigned char
   #define RGB(r,g,b) (((r)||(g)||(b))?0xFF:0x00)
   #define get_dot(col, row) ((read_screen(col, row) & pixel_masks[(col)&0x07])?WHITE:0)
#endif      

#ifdef LC7981   // Epson LC7981 controller
   #ifdef BIG_PANEL
      #define RES_240x128
      #define ROWS 128
      #define COLS 240
   #else
      #define RES_160x80
      #define ROWS 80
      #define COLS 160
   #endif

   #ifdef CF
      #define ASPECT 256   // square pixel panel
   #else
      #define ASPECT 192
   #endif

   #define COORD unsigned char
   #define INCREMENT char        /* if COORD is not char,  this should be int */
   #define PIXELS_PER_BYTE 8
   #define COLOR unsigned char
   #define RGB(r,g,b) (((r)||(g)||(b))?0xFF:0x00)
   #define get_dot(col, row) ((read_screen(col, row) & pixel_masks[(col)&0x07])?WHITE:0)
#endif

#ifdef NOKIA    // Epson S1D15G10 controller (cannot read back memory if serial interface)
   #define RES_132x132
   #define ASPECT  256   /* 256 for square pixel panels */
   #define ROW_OFS 2
   #define ROWS    (132-ROW_OFS)   
   #define COLS    (132-ROW_OFS)
   #define COORD     unsigned char
   #define INCREMENT char        /* if COORD is not char,  this should be int */
   #define PIXELS_PER_BYTE 1
   #define COLOR_LCD
// #define COLOR unsigned char
// #define RGB(r,g,b)  ( ((r)&0xE0) | (((g) & 0xE0)>>3) | ((b) >> 6) ) //8-bit pixels
   #define COLOR u16
   #define RGB(r,g,b)  ( (((r)&0xF0)<<8) | (((g) & 0xF0)<<4) | ((b) & 0xF0) )  //16-bit pixels
   #define NO_OFFSCREEN  // panel has only one full page of display memory
   #define get_dot(col, row) read_screen(col, row)
#endif      

#ifdef OLED    // SSD1339 OLED controller
   #define RES_132x132   // display is 128x128 in a 132x132 memory array
   #define ASPECT  256   // 256 for square pixel panels/
   #define ROW_OFS 0     // define offsets of 128x128 display in 132x132 memory 
   #define COL_OFS 4
   #define ROWS    128   // define drawing area size
   #define COLS    128
   #define COORD unsigned char
   #define INCREMENT char      // if COORD is not char,  this should be int
   #define PIXELS_PER_BYTE 1   // (or pixels per word in 16-bit pixel modes)
   #define COLOR_LCD
// #define COLOR u08           // if u08 then use RGB 3:3:2 8-bit pixels
// #define RGB(r,g,b)  ( ((r)&0xE0) | (((g) & 0xE0)>>3) | ((b) >> 6) ) //8-bit pixels
   #define COLOR u16           // if u16 then use RGB 5:6:5 16-bit pixels
   #define RGB(r,g,b)  ( (((r)&0xF8)<<8) | (((g) & 0xFC)<<3) | ((b) >> 3) )  //16-bit pixels
   #define NO_OFFSCREEN        // panel has only one full page of display memory
   #define get_dot(col, row) read_screen(col, row)
#endif      

#ifdef LCD_MDT // No LCD panel,  use serial port terminal
   #define MDT_CODE
   #define RES_160x80
   #define ASPECT 256   /* 256 for square pixel panels */
   #define ROWS   80
   #define COLS   160
   #define COORD     unsigned char
   #define INCREMENT char       /* if COORD is not char,  this should be int */
   #define PAGES  (ROWS/8)
   //#define NO_OFFSCREEN    // panel has only one full page of display memory
   // multiple pages supported (32K buffer default)
   // !!! 

   #define REVERSE_DB
   #define PIXELS_PER_BYTE 8
   #define COLOR unsigned char
   #define RGB(r,g,b) (((r)||(g)||(b))?0xFF:0x00)

   COLOR mdt_read_screen(unsigned addr);
   void mdt_write_screen(unsigned addr, COLOR val);
   void write_screen_byte(unsigned addr, COLOR val);
   #define get_dot(col, row) ((read_screen(col, row) & pixel_masks[(col)&0x07])?WHITE:0)
#endif

#define BLACK RGB(0x00,0x00,0x00)
#define WHITE RGB(0xFF,0xFF,0xFF)

void invert_colors(void);

#define CHAR_HEIGHT 8
#define CHAR_WIDTH  8

#define GRAPH_MODE   1
#define TEXT_MODE    0

#define FILLED       0x01
#define DITHERED     0x02
#define BLIT_OR      0x04
#define BLIT_AND     0x08
#define BLIT_XOR     0x10
#define BLIT_WRITE   0x80  // source of blit is blit_buf (mem to screen blit)
#define BLIT_READ    0x40  // dest blit is blit_buf (screen to mem blit)
#define BLIT_ALIGNED 0x20  // set this flag before mem blits if blit_buf 
                           // contents already aligned with screen
u08 draw_flags;


u08 text_mode;  /* flag set if panel is in native text mode, no graphics mode */
u08 char_size;  /* scale factor for text characters */

// COLOR char_buf[(COLS/PIXELS_PER_BYTE)+1];  // buffer used to blit out special chars
COLOR blit_buf[(COLS/PIXELS_PER_BYTE)+2];   //!!!!!

COLOR color;            // forground color
COLOR bg_color;         // background color
COLOR color_table[16];  // lookup table for IBM PC compatible colors


#ifdef MDT_CODE  // Megadonkey Remote Terminal

   // below functions defined in md_term.c (also proto in md_term.h)
   // takes care of sending values to remote terminal and also updating variables
   // used by drawing functions

   // set global variable and call mdt_ function (proto. in md_term.h)

   // 2/8/2013 - Library 1.09 fixed problem with set_topline(x) sending characters to serial port
   // when mdt not enabled

   // md_term.h functions
   void mdt_set_rotate(u08 x);
   void mdt_set_draw_flags(u08 x);
   void mdt_set_char_size(u08 x);
   void mdt_set_color(u08 x);
   void mdt_set_bg_color(u08 x);
   void mdt_set_topline(int x);

   #define set_rotate(x)       mdt_set_rotate(x)
   #define set_draw_flags(x)   mdt_set_draw_flags(x)
   #define set_color(x)        mdt_set_color(x)
   #define set_bg(x)           mdt_set_bg_color(x)
   #define set_charsize(x)     mdt_set_char_size(x)
   #define set_topline(x)      mdt_set_topline(x)
#else
   #define set_rotate(x)       rotate = (x)
   #define set_draw_flags(x)   draw_flags = (x)
   #define set_color(x)        color = (x)
   #define set_bg(x)           bg_color = (x)
   #define set_charsize(x)     char_size = (x)
   #define set_topline(x)      top_line = (x)
#endif


// support for screen memory display and drawing page access and double buffering
#define MAX_PAGEBUFS 4
#define show_page(n)    set_lcdtop(((unsigned)(n))*ROWS)
#define draw_on_page(n) set_topline(((unsigned)(n))*((COLS/PIXELS_PER_BYTE)*ROWS))


// see above
//void set_topline(unsigned row); // sets line at top of screen in the hardware

void lcd_pageflip(void);        // lcd page flip for flicker free drawing  
u08 page_buffers(u08 pages);    // setup for graphics page flipping on *num* pages
void page_reset(void);          // reset double buffered graphics mode

unsigned int top_line;   // line number at top of screen for drawing 
u08 DrawPage;            // the current page we are drawing on
u08 double_buffers;      // set by page_buffers() to (#pages-1) to draw menu items on


#define ROT_CHAR_90      0x01
#define ROT_CHAR_VERT    0x02
#define ROT_CHAR_HORIZ   0x04
#define ROT_STRING_VERT  0x08
#define ROT_STRING_HORIZ 0x10
#define ROT_CHAR   (ROT_CHAR_90 | ROT_CHAR_VERT | ROT_CHAR_HORIZ)
#define ROT_STRING (ROT_STRING_VERT | ROT_STRING_HORIZ)
u08 rotate;   /* chars: 0x01=90 deg   0x02=bottom-top  0x04=right-left */
                     /* strings: 0x08=verticl  0x10=right->left or bot->top */


#define PRINTF_BEL    0x01   // flag bits for enabling cursor control chars in printf
#define PRINTF_BS     0x02
#define PRINTF_FF     0x04
#define PRINTF_NL     0x08
#define PRINTF_CR     0x10
#define PRINTF_TAB    0x20
#define PRINTF_VT     0x40
#define PRINTF_SCROLL 0x80
#define VCHAR_COMPAT  0x100  // scale vchars to bitmaped char size via char_size
#define VCHAR_ERASE   0x200  // erase area under vchars before drawing
u16 printf_flags;

COORD lcd_col, lcd_row;   // current cursor position

s16 MouseX;   // mouse position
s16 MouseY;


// ps_buf[] is the buffer used by stringPS() and the PS() macro to fetch 
// a string out of program space (flash) memory into real RAM where it can.
// be used by functions like printf() and sscanf().
//
// Be VERY aware that only one PS() string at a time can be used...
// so don't try to call a function with two PS() strings as arguments, etc
// and be aware of calling functions that may also use the PS() macro
//
// ps_buf[] can also be used by programs as a general purpose (very) temporary 
// scratch area as when you are not using any PS() strings
//
#define MAX_PS_LEN 80
char ps_buf[MAX_PS_LEN+1];
#define PS(s)   stringPS(PSTR(s))
char *stringPS(PGM_P ps);

void lcd_set_stdout();          // sets LCD as stdout for printf...
void lcd_scroll(COORD rows);

void lcd_demo(u08 pattern); 
void lcd_init(void);

void fill_screen(COLOR c);   // fill screen to current color 
void lcd_clear(void);        // clear screen, home cursor, set color FF
  
void lcd_setxy (COORD x,COORD y);

void dot(COORD col,COORD row);
void hline(COORD row,COORD left,  COORD right);
void line(COORD x1,COORD y1, COORD x2,COORD y2);
void ellipse(COORD x,COORD y,  COORD major, int aspect);

void blit(COORD left,COORD top,  COORD right,COORD bot,  COORD dest_left,COORD dest_top);

void lcd_char(COORD col,COORD row, unsigned char c);
void lcd_text(COORD col,COORD row, char *s);

void lcd_textPM(COORD col,COORD row, PGM_P s);
#define lcd_textPS(col,row, s)  lcd_textPM(col,row, PSTR(s))

void filled_box(COORD x1,COORD y1,  COORD x2,COORD y2);
void box(COORD x1,COORD y1,  COORD x2,COORD y2);
void shaded_box(COORD x1,COORD y1,  COORD x2,COORD y2);
void thick_box(COORD x1,COORD y1,  COORD x2,COORD y2,  COORD width);
void arc(COORD x,COORD y, COORD r,  u16 a1, u16 a2, COORD t);

int paint(COORD xi,COORD yi, COLOR bound);

void kbd_key(COORD col,COORD row, char *s);

#define circle(x,y, r)  ellipse(x,y, r, ASPECT)  /* !!! 256 for square pixels */

u08 bus_in_use(void);

//
// screen memory access functions
//
COLOR read_screen(COORD col,COORD row);
u08 read_screen_byte(u16 addr);
u16 read_screen_word(u16 addr);
u32 read_screen_dword(u16 addr);

void write_screen(COORD col,COORD row, COLOR data);
void write_screen_byte(u16 addr,  u08 val);
void write_screen_word(u16 addr,  u16 val);
void write_screen_dword(u16 addr, u32 val);

//
//   Functions for accessing different memory spaces.
//
//   The memory space to be accessed is specified by "or"ing the address
//   with one of these memory space flags.
//
#define EEPROM_ADDR  0x80000000L
#define PROG_ADDR    0x40000000L
#define SCREEN_ADDR  0x20000000L

u08 read_byte(unsigned long addr);
u16 read_word(unsigned long addr);
u32 read_dword(unsigned long addr);

unsigned long read_string(unsigned long addr, char *s, u08 max_len);
unsigned long read_block(unsigned long addr, char *s, u16 len);

void write_byte(unsigned long addr, u08 data);
void write_word(unsigned long addr, u16 data);
void write_dword(unsigned long addr, u32 data);
unsigned long write_string(unsigned long addr, char *s, u08 max_len);
unsigned long write_block(unsigned long addr, char *s, u16 len);

#endif // PANEL_CODE
#endif // _lcd_h
