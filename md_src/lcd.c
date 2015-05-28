/*  MegaDonkey Library File:  lcd.c    Low-Level LCD Graphics Support
    


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

  

   Note: Do not include low level LCD device support modules (e.g. lcdagm.c or 
   lcdel.c) in project as they are included into this file below.

   Instead, any given LCD module may be added into the "Other Files" folder so it/they
   can be accessed easily within the IDE.

*/

#include <stdio.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <string.h>
#include <ctype.h>
#include "md.h"     // defines bit set/clear macros  sbi cbi

#ifdef PANEL_CODE
#include "lcd.h"
#include "timer.h"
#include "adc.h"
#include "trakball.h"
#include "graph.h"
#include "led.h"
#include "md_term.h"  // output primatives to MD Terminal if hooked


// low level device support modules included here:

#ifdef KS0107
   #include "lcdagm.c"   // Samsung KS0107/0108 controller - AGM1264 Panel
#endif
#ifdef KS0713
   #include "lcdagm.c"   // Samsung KS0173/S6B1713 controllers
#endif

#ifdef TOSHIBA
   #include "lcdtosh.c"  // Toshiba T6963 controller
#endif

#ifdef LC7981
   #include "lcdel.c"    // Epson LC7981 controller - Samsung 160x80 Panel (Touch Screen/ EL Backlight)
#endif

#ifdef SED
   #include "lcdsed.c"    // Epson SED1335 controller
#endif

#ifdef NOKIA
   #include "lcdnok.c"    // EPSON S1D15G10 Color LCDs
#endif

#ifdef OLED
   #include "lcdoled.c"    // SSD1339 OLED Color LCDs
#endif

#ifdef LCD_MDT
   #include "lcdmdt.c"    // no LCD panel - serial port Donkey terminal
#endif


#ifdef COLOR_LCD
// COLOR color_table[16] = {
//    0x00, 0x01, 0x10, 0x11,  // black  blue     green  cyan
//    0x80, 0x81, 0x90, 0xFF,  // red    magenta  brown  WHITE
//    0x91, 0x03, 0x1C, 0x1F,  // grey   BLUE     GREEN  CYAN
//    0xE0, 0xE3, 0xFC, 0xFF   // RED    MAGENTA  YELLOW WHITE
// };
   COLOR color_table[16] = {
      RGB(0x00,0x00,0x00), RGB(0x00,0x00,0x80), RGB(0x00, 0x80,0x00), RGB(0x00,0x80,0x80),  // black  blue  green  cyan
      RGB(0x80,0x00,0x00), RGB(0x80,0x00,0x80), RGB(0x80, 0x80,0x00), RGB(0xC0,0xC0,0xC0),  // red    magenta  brown  white  
      RGB(0x80,0x80,0x80), RGB(0x00,0x00,0xFF), RGB(0x00, 0xFF,0x00), RGB(0x00,0xFF,0xFF),  // grey   BLUE     GREEN  CYAN   
      RGB(0xFF,0x00,0x00), RGB(0xFF,0x00,0xFF), RGB(0xFF, 0xFF,0x00), RGB(0xFF,0xFF,0xFF)   // RED    MAGENTA  YELLOW WHITE  
   };
#else
   COLOR color_table[16] = {
      BLACK, WHITE, WHITE, WHITE,
      WHITE, WHITE, WHITE, WHITE,
      WHITE, WHITE, WHITE, WHITE,
      WHITE, WHITE, WHITE, WHITE
   };
#endif


// basic support of stdout using stdio.h 
// allows use of printf() after calling lcd_set_stdout()


static int lcd_putchar(char c, FILE *stream); // fwd declare


// macro FDEV_SETUP_STREAM provided by stdio.h
// put,get,rwflag

static FILE lcd_stdout = FDEV_SETUP_STREAM(lcd_putchar, NULL,
                                             _FDEV_SETUP_WRITE);

void lcd_init(void)
{
   lcd_xinit(GRAPH_MODE);


   set_charsize(1);      

   draw_on_page(0);    // set where to start drawing in the screen buffer
   show_page(0);    // set where to start showing the screen buffer

   set_color(WHITE);
   set_bg(BLACK);
   fill_screen(BLACK);

   show_cursor();
}

void invert_colors()
{
COLOR temp;

   temp = color;
   set_color(bg_color);
   set_bg(temp);
}

void lcd_set_stdout(void)
{
   stdout = &lcd_stdout;   
   printf_flags = 0xFFFF;
}

void lcd_scroll(COORD rows)
{
u08 cur;
u16 temp_flags;

   cur = erase_cursor();

   #ifdef MDT_CODE
      if(mdtHooked) mdt_scroll(rows);
   #endif

   temp_flags = draw_flags;

   set_draw_flags(0x00);
   blit(0,rows, COLS-1,ROWS-1, 0,0);
    
   set_draw_flags(FILLED);
   invert_colors();
   blit(0, ROWS-rows, COLS-1,ROWS-1, 0,ROWS-rows);
   invert_colors();

   set_draw_flags(temp_flags);

   if(cur) show_cursor();
}


static int lcd_putchar(char c, FILE *stream)
{
u08 cur;

   cur = erase_cursor();

   if((printf_flags & PRINTF_BEL) && (c == '\a')) {  // beep
      beep(200,2000); 
   }
   else if((printf_flags & PRINTF_BS) && (c == '\b')) {  // back space
      if(lcd_col < (CHAR_WIDTH*char_size)) lcd_col = 0;
      else lcd_col -= (CHAR_WIDTH*char_size);
   }
   else if((printf_flags & PRINTF_FF) && (c == '\f')) { // form feed
      lcd_col = lcd_row = 0;
   }
   else if((printf_flags & PRINTF_NL) && (c == '\n')) { // newline (CRLF)
      lcd_col = 0;
      lcd_row += (CHAR_HEIGHT*char_size);
   }
   else if((printf_flags & PRINTF_CR) && (c == '\r')) { // carriage return
      lcd_col = 0;
   }
   else if((printf_flags & PRINTF_TAB) && (c == '\t')) {  //!!! how best to handle TAB?
      lcd_col += (CHAR_WIDTH*char_size);
   }
   else if((printf_flags & PRINTF_VT) && (c == '\v')) {  // vertical tab (line feed)
      lcd_row += (CHAR_HEIGHT*char_size);
   }
   else { // output all other characters
      lcd_char(lcd_col, lcd_row, c);   // low-level LCD character drawing function
      lcd_col += (CHAR_WIDTH*char_size);
   }

   if(lcd_col > COLS-(CHAR_WIDTH*char_size)) {  // wrap to next line
      lcd_col = 0;
      lcd_row += (CHAR_HEIGHT*char_size);
   } 
   
   if(lcd_row > ROWS-(CHAR_HEIGHT*char_size)) {  // scroll screen up
      if(printf_flags & PRINTF_SCROLL) {
         lcd_row -= (CHAR_HEIGHT*char_size);
         lcd_scroll(CHAR_HEIGHT*char_size);
      }
      else lcd_row = 0;
   }

   if(cur) show_cursor();
   return 0;
}



void lcd_setxy(COORD x, COORD y)
{ 
   lcd_col = x;     // set xy location of next character (char's upper-left corner)
   lcd_row = y;

   #ifdef MDT_CODE
      if(mdtHooked) mdt_setxy(x,y);
   #endif
}


void lcd_clear(void)
{
   lcd_setxy(0,0);
   fill_screen(bg_color);  // device dependant fill_screen 
   
}

// ------------- screen memory access functions -------------- 
//        *** byte functions are device dependent ***          
//    *** !!!!!! also problems if color width > 8 bits ***         

void write_screen_word(u16 addr, u16 word)
{
   write_screen_byte(addr+0, (word >> 0) & 0xFF);
   write_screen_byte(addr+1, (word >> 8) & 0xFF);
}

void write_screen_dword(u16 addr, u32 word)
{
   write_screen_byte(addr+0, (word >> 0) & 0xFF);
   write_screen_byte(addr+1, (word >> 8) & 0xFF);
   write_screen_byte(addr+2, (word >> 16) & 0xFF);
   write_screen_byte(addr+3, (word >> 24) & 0xFF);
}

u16 read_screen_word(u16 addr)
{
u16 word;

   word = read_screen_byte(addr+1);
   word <<= 8;
   word |= read_screen_byte(addr+0);
   return word;
}

u32 read_screen_dword(u16 addr)
{
u32 word;

   word = read_screen_byte(addr+3);
   word <<= 8;
   word |= read_screen_byte(addr+2);
   word <<= 8;
   word |= read_screen_byte(addr+1);
   word <<= 8;
   word |= read_screen_byte(addr+0);
   return word;
}

//
//   Functions for accessing different memory spaces.
//
//   The memory space to be accessed is specified by "or"ing the
//   base address with one of these memory space flags:
//
//   #define EEPROM_ADDR  0x80000000L
//   #define PROG_ADDR    0x40000000L
//   #define SCREEN_ADDR  0x20000000L
//

u08 read_byte(unsigned long addr)
{
   if(addr & EEPROM_ADDR) return eeprom_read_byte((uint8_t *) (unsigned) (addr & 0x7FFF));
   else if(addr & PROG_ADDR) return pgm_read_byte((unsigned) (addr & 0x0FFFFFFFL));
   else if(addr & SCREEN_ADDR) return read_screen_byte((unsigned) (addr & 0x0FFFFFFFL));
   else return *((unsigned char *) (void *) (unsigned) addr);
}

u16 read_word(unsigned long addr)
{
   if(addr & EEPROM_ADDR) return eeprom_read_word((uint16_t *) (unsigned) (addr & 0x7FFF));
   else if(addr & PROG_ADDR) return pgm_read_word((unsigned) (addr & 0x0FFFFFFFL));
   else if(addr & SCREEN_ADDR) return read_screen_word((unsigned) (addr & 0x0FFFFFFFL));
   else return *((unsigned *) (void *) (unsigned) addr);
}

u32 read_dword(unsigned long addr)
{
u32 val;

   if(addr & EEPROM_ADDR)  {
      eeprom_read_block(&val, (uint8_t *) (unsigned) (addr & 0x7FFF), 4);
      return val;
   }
   else if(addr & PROG_ADDR) return pgm_read_dword((unsigned) (addr & 0x0FFFFFFFL));
   else if(addr & SCREEN_ADDR) return read_screen_dword((unsigned) (addr & 0x0FFFFFFFL));
   else return *((u32 *) (unsigned) addr);
}

unsigned long read_string(unsigned long addr, char *s, u08 max_len)
{
char c;
u08 i;

   for(i=0; i<max_len; i++) {
      c = read_byte(addr++);
      if(c == 0) break;
      s[i] = c;
   }
   s[i] = 0;
   return addr;
}

unsigned long read_block(unsigned long addr, char *s, u16 len)
{
   while(len--) {
      *s++ = read_byte(addr++);
   }
   return addr;
}



void write_byte(unsigned long addr, u08 data)
{
   if(addr & EEPROM_ADDR) eeprom_write_byte((uint8_t *) (unsigned) (addr & 0x7FFF),  (uint8_t) data);
   else if(addr & SCREEN_ADDR) write_screen_byte((u16) (addr & 0x0FFFFFFFL),  data);
   else if(addr & PROG_ADDR) return;  // cant write program memory space
   else *((u08 *) (void *) (unsigned) addr) = data;
}

void write_word(unsigned long addr, u16 data)
{
   if(addr & EEPROM_ADDR) eeprom_write_word((uint16_t *) (unsigned) (addr & 0x7FFF),  (uint16_t) data);
   else if(addr & SCREEN_ADDR) write_screen_word((u16) (addr & 0x0FFFFFFFL),  data);
   else if(addr & PROG_ADDR) return;  // cant write program memory space
   else *((u16 *) (void *) (unsigned) addr) = data;
}

void write_dword(unsigned long addr, u32 data)
{
   if(addr & EEPROM_ADDR) eeprom_write_block(&data, (uint8_t *) (unsigned) (addr & 0x7FFF), 4);
   else if(addr & SCREEN_ADDR) write_screen_dword((u16) (addr & 0x0FFFFFFFL),  data);
   else if(addr & PROG_ADDR) return;  // cant write program memory space
   else *((u32 *) (void *) (unsigned) addr) = data;
}

unsigned long write_string(unsigned long addr, char *s, u08 max_len)
{
char c;
u08 i;

   for(i=0; i<max_len; i++) {
      c = s[i];
      write_byte(addr++, c);
      if(c == 0) break;
   }
   return addr;
}

unsigned long write_block(unsigned long addr, char *s, u16 len)
{
   while(len--) {
      write_byte(addr++, *s++);
   }
   return addr;
}


/*
   Graphics Double Buffer - flickerless drawing using two pages of LCD graphics.
  
   draw on the page not in view, show it, then draw on the page just viewed 

   top_line is the current drawing origin (byte offset into display memory)
   e.g. top_line=0 for drawing on page 0
   top_line = total # bytes on a page (e.g. COLS/8*ROWS) for drawing on page 1


   What is displayed as current page is controlled by calling show_page()
   which in turn calls  set_lcdtop() which is specified as a row#
   
   calling set_lcdtop(row#) allows one to vertically scroll the page
   adding #rows = #rows on display jumps to the next display page    
    

   A more descriptive name for top_line might be drawing_offset
                           for set_lcdtop() might be set_display_top_row()

   
*/


void lcd_pageflip(void)    // lcd page flip for flicker free drawing  
{                          
#ifdef NO_OFFSCREEN
   return;
#else
   show_page(DrawPage);       // show page just drawn
   DrawPage ^= 1;        
   draw_on_page(DrawPage);    // draw on page just shown
#endif
}


u08 page_buffers(u08 num)
{
u08 cur;

#ifdef NO_OFFSCREEN
   num = 1;   // e.g. AGM panel can't double buffer
#else
   if(num > MAX_PAGEBUFS) num = MAX_PAGEBUFS;
   else if(num < 1) num = 1;
#endif

   cur = erase_cursor();
   double_buffers = (num - 1);
   if(cur) show_cursor();

   return num;
}

void page_reset(void)
{
   show_page(0);     // exit reset to page 0 (incase double buffered)
   draw_on_page(0);
   page_buffers(1);
}


/* ------------------- higher level drawing routines ------------------- */

void lcd_textPM(COORD col,COORD row, PGM_P s)
{  // print string from progam memory space (flash memory)
unsigned char c;
u08 i;
u08 cur;

   cur = erase_cursor();

   i = 0;
   while(1) {
      c = pgm_read_byte(s+i);
      ++i;

      if(c == 0) break;
      lcd_char(col, row, c);
    

      if(rotate & ROT_STRING_VERT) {  /* draw strings vertically */
         if(rotate & ROT_STRING_HORIZ) row -= (CHAR_HEIGHT * char_size);
         else row += (CHAR_HEIGHT * char_size);
         if(row >= ROWS) break;   /* !!! clipping */
      }
      else { /* draw strings horizontally */
         if(rotate & ROT_STRING_HORIZ) col -= (CHAR_WIDTH * char_size);  /* !!! multiply */
         else col += (CHAR_WIDTH * char_size);  /* !!! multiply */
         if(col >= COLS) break;   /* !!! clipping */
      }
   }

   if(cur) show_cursor();
}


void lcd_text(COORD col,COORD row,  char *s)
{
unsigned char c;
u08 cur;

   cur = erase_cursor();

   while((c=*s++)) {
      lcd_char(col, row, c);

      if(rotate & ROT_STRING_VERT) {  /* draw strings vertically */
         if(rotate & ROT_STRING_HORIZ) row -= (CHAR_HEIGHT * char_size);
         else row += (CHAR_HEIGHT * char_size);
         if(row >= ROWS) break;   /* !!! clipping */
      }
      else { /* draw strings horizontally */
         if(rotate & ROT_STRING_HORIZ) col -= (CHAR_WIDTH * char_size);  /* !!! multiply */
         else col += (CHAR_WIDTH * char_size);  /* !!! multiply */
         if(col >= COLS) break;   /* !!! clipping */
      }
   }

   if(cur) show_cursor();
}


void line(COORD x1,COORD y1, COORD x2,COORD y2)
{
INCREMENT i1, i2;
int d1, d2, d;
u08 cur;

   cur = erase_cursor();

   #ifdef MDT_CODE
      u08 hs = mdtHooked;  // save MDHooked state
      if(mdtHooked) mdt_line(x1,y1,  x2,y2);
      if(mdtNoLocalDisplay) {
         if(cur) show_cursor();
         return;
      }
      mdtHooked = 0;  // inhibit dot from calling mdt_dot
   #endif


   i1 = i2 = 1;

   d1 = (int) x2 - (int) x1;
   if(d1 < 0) {
      i1 = (-1);
      d1 = 0 - d1;
   }

   d2 = (int) y2 - (int) y1;
   if(d2 < 0) {
      i2 = (-1);
      d2 = 0 - d2;
   }

   if(d1 > d2) {
      d = d2 + d2 - d1;
      while(1) {
         dot(x1, y1);
         if(x1 == x2) break;
         if(d >= 0) {
            d = d - d1 - d1;
            y1 += i2;
         }
         d = d + d2 + d2;
         x1 += i1;
      }
   }
   else {
      d = d1 + d1 - d2;
      while (1) {
         dot(x1, y1);
         if(y1 == y2) break;
         if(d >= 0) {
            d = d - d2 - d2;
            x1 += i1;
         }
         d = d + d1 + d1;
         y1 += i2;
      }
   }

   if(cur) show_cursor();

   #ifdef MDT_CODE
     mdtHooked = hs;  // restore MDHooked state
   #endif
}


void hline(COORD row, COORD left, COORD right)
{
u16 temp_draw;
COORD temp;

   if(left > right) {
      temp = left;
      left = right;
      right = temp;
   }

   #ifdef MDT_CODE
      // note: param order left,right,row  on mdt_hline
      u08 hs = mdtHooked;
      if(mdtHooked) mdt_hline(left,right,row);  // note:remote MD Term 
      if(mdtNoLocalDisplay) return;             // duplicates this code including 
      mdtHooked = 0;  // disable mdtHook on blit
   #endif                                       // blit -- much more compact
                                                // data transmission-wise
   temp_draw = draw_flags;
   set_draw_flags(draw_flags | FILLED);
   blit(left,row, right,row,  left,row);
   set_draw_flags(temp_draw);

   #ifdef MDT_CODE
   mdtHooked = hs;  // restore status
   #endif

}


void filled_box(COORD x1,COORD y1, COORD x2,COORD y2)
{
u16 t;

   #ifdef MDT_CODE
      u08 hs = mdtHooked;
      if(mdtHooked) mdt_filled_box(x1,y1,x2,y2);
      if(mdtNoLocalDisplay) return;
      mdtHooked = 0;
   #endif

   if(y2 < y1) { 
      t=y2; y2=y1; y1=t;
   }
   if(x2 < x1) { 
      t=x2; x2=x1; x1=t;
   }

   t = draw_flags;
   set_draw_flags(draw_flags | FILLED);
   blit(x1,y1,  x2,y2,  x1,y1);
   set_draw_flags(t);

   #ifdef MDT_CODE
   mdtHooked = hs;  // restore saved state
   #endif
}


void box(COORD x1,COORD y1, COORD x2,COORD y2)
{
u08 cur;

   if(draw_flags & FILLED) return filled_box(x1,y1, x2,y2);

   cur = erase_cursor();

   #ifdef MDT_CODE
      u08 hs = mdtHooked;   // save hooked state
      if(mdtHooked) {
         mdt_box(x1,y1,x2,y2);
         if(mdtNoLocalDisplay) {
            if(cur) show_cursor();
            return;
         }
         mdtHooked = 0; // turn off for remainder of this function
      }   
   #endif

   hline(y1, x1,x2);
   hline(y2, x1,x2);
   line(x2,y1,x2,y2);
   line(x1,y1,x1,y2);

   if(cur) show_cursor();

   #ifdef MDT_CODE
      mdtHooked = hs;  // restore saved state
   #endif
}


void shaded_box(COORD x1,COORD y1, COORD x2,COORD y2)
{
u08 cur;
  
   cur = erase_cursor();

   #ifdef MDT_CODE
      u08 hs = mdtHooked;   // save hooked state
      if(mdtHooked) {
         mdt_shaded_box(x1,y1,x2,y2);
         if(mdtNoLocalDisplay) {
            if(cur) show_cursor();
            return;
         }
         mdtHooked = 0; // turn off for remainder of this function
      }   
   #endif

   hline(y1, x1,x2);
   hline(y2, x1,x2);
   hline(y2-1,x1,x2);
   line(x2,y1,x2,y2);
   line(x1,y1,x1,y2);
   line(x2-1,y1,x2-1,y2);
//line(x2+1,y1,x2+1,y2);  // triple thick right edge
   dot(x1+1,y1+1);
   invert_colors();
   dot(x1,y1);
   dot(x1,y2);

   dot(x2,y1);
   dot(x2,y2);
//dot(x2+1,y2);   // triple thick right edge
//dot(x2+1,y1);
   invert_colors();

   if(cur) show_cursor();

   #ifdef MDT_CODE
      mdtHooked = hs;  // restore saved state
   #endif
}


void thick_box(COORD x1,COORD y1, COORD x2,COORD y2,  COORD width)
{  /* draw a box with fat borders */
u08 cur;

   cur = erase_cursor();
   if(width) --width;

   filled_box(x1, y1,  x2, y1+width);  // top
   filled_box(x1, y2-width,  x2, y2);  // bottom
   filled_box(x1, y1+width,  x1+width, y2-width);  // left
   filled_box(x2, y1+width,  x2-width, y2-width);  // right

   if(cur) show_cursor();
}


void arc(COORD x,COORD y, COORD r,  u16 a1,u16 a2, COORD thickness)
{
u16 t;
u16 x1,y1;
u16 x2,y2;
u08 cur;

   cur = erase_cursor();

   #ifdef MDT_CODE
      u08 hs = mdtHooked;
      if(mdtHooked) {
         mdt_arc(x,y,r,a1,a2,thickness);
         if(mdtNoLocalDisplay) {
            if(cur) show_cursor();
            return;
         }
         mdtHooked = 0; // turn off for remainder of this function
      }
   #endif

   a1 += 90;
   a2 += 90;
   if(a1 > a2) {
      t = a1;
      a1 = a2;
      a2 = t;
   }

   if(thickness <= 1) { // simple arc
      x1 = ((r * sin360(a1)) + 64) >> 7;
      y1 = ((r * cos360(a1)) + 64) >> 7;
      y1 = (y1 * ASPECT) / 256;
      dot(x+x1,y+y1);

      while(++a1 <= a2) {
         x2 = ((r * sin360(a1)) + 64) >> 7;
         y2 = ((r * cos360(a1)) + 64) >> 7;
         y2 = (y2 * ASPECT) / 256;

         line(x+x1,y+y1, x+x2,y+y2);

         x1 = x2; y1 = y2;
      }
   }
   else { // thick arc    !!! needs work for bigger panels
      while(a1 <= a2) {
         x1 = ((r * sin360(a1)) + 64) >> 7;
         y1 = ((r * cos360(a1)) + 64) >> 7;
         y1 = (y1 * ASPECT) / 256;

         x2 = (((r+thickness) * sin360(a1)) + 64) >> 7;
         y2 = (((r+thickness) * cos360(a1)) + 64) >> 7;
         y2 = (y2 * ASPECT) / 256;

         line(x+x1,y+y1,  x+x2,y+y2);

         ++a1;
      }
   }

   if(cur) show_cursor();

   #ifdef MDT_CODE
      mdtHooked = hs; // restore mdtHooked state
   #endif
}

void ellipse(
   COORD x,COORD y,
   COORD major,    /* x axis length */
   int   aspect    /* aspect ratio*256 */
)
{
COORD row, col;
unsigned two_x, two_y;
unsigned long two_a, two_b, alpha, beta;  //!!! long
long d, cincr, rincr;  //!!! long
u08 cur;

   cur = erase_cursor();

   #ifdef MDT_CODE
      u08 hs = mdtHooked;
      if(mdtHooked) {
        mdt_ellipse(x,y,major,aspect);
        if(mdtNoLocalDisplay) {
           if(cur) show_cursor();
           return;
        }
        mdtHooked = 0; // turn off for remainder of this function
      }   
   #endif


   d = ((long)major) * ((long)aspect);

#ifdef REALMAJOR   /* the following code applies if "major" is the major axis length */
   if(aspect < 256) {
      two_a = ((long)major) * ((long)major)*2L;
      two_b = ((d >> 3) * (d >> 3)) >> 9;
      d = (d+64) >> 7;
   }
   else {
      two_a = ((d >> 4) * (d >> 4)) >> 7;
      two_b = ((long)major) * ((long)major)*2L;
      d = major * 2;
   }
#else /* the following code applies if "major" is the x-axis length */
   two_a = ((long)major) * ((long)major)*2L;
   two_b = ((d >> 4) * (d >> 4)) >> 7;
   d = (d+64) >> 7;
#endif

   row = y + (d >> 1);
   col = x;
   two_x = x << 1;
   two_y = y << 1;
   alpha = two_a >> 1;
   rincr = (- (alpha * d));
   cincr = beta = (two_b >> 1);
   d = ((beta - alpha) << 1) + (alpha >> 1);

/*
 *  For the remaining part of the routine, only 'col', 'row', 'two_y',
 *  'two_x', 'color', 'two_a', 'two_b', 'cincr', 'rincr' and 'd' are
 *  used.
 */

   while(cincr + rincr < 0) {
      if(draw_flags & (FILLED | DITHERED)) {
         hline(row,  two_x-col,col);
         hline(two_y-row,  two_x-col,col);
      }
      else {
         dot(col, row);
         dot(col, two_y-row);
         dot(two_x-col, row);
         dot(two_x-col, two_y-row);
      }
      d += (cincr += two_b);
      col++;
      if(d >= 0) {
         d += (rincr += two_a);
         row--;
      }
   }

   while(row >= y) {
      if(draw_flags & (FILLED | DITHERED)) {
         hline(row,  two_x-col,col);
         hline(two_y-row,  two_x-col,col);
      }
      else {
         dot(col, row);
         dot(col, two_y-row);
         dot(two_x-col, row);
         dot(two_x-col, two_y-row);
      }
      if(d <= 0) {
         d += (cincr += two_b);
         col++;
      }
      if(row == 0) break;  //!!! prevent runaway if center a 0,0
      row--;
      d += (rincr += two_a);
   }

   if(cur) show_cursor();

   #ifdef MDT_CODE
     mdtHooked = hs; // restore mdtHooked
   #endif
}


/********************************************************************
 *
 * PAINT: Fill in the convex area containing (x,y) bounded by color (bound)
 *              with color set by last call to set_color().
 *
 *******************************************************************/

#define FIND_COLOR(x, y, c) (get_dot(x, y) - c)

u08 pixels_per_byte = PIXELS_PER_BYTE;

int pass_paint(COORD xi, COORD yi, COLOR bound, int yinc)
{
int xl, xp, yp;
u08 cur;

   cur = erase_cursor();

   yp = yi;
   xp = xi + 1;
   xl = xi - 1;
   if(FIND_COLOR(xi, yp, bound) == 0) {
      do {
         xl -= pixels_per_byte;
         if(xl < 0) {
            xl = 0;
            break;
         }
      } while(FIND_COLOR(xl, yp, bound) == 0);
      xl += pixels_per_byte;

      do {
         xp += pixels_per_byte;
         if(xp > COLS) {
            xp = COLS;
            break;
         }
      } while(FIND_COLOR(xp, yp, bound) == 0);
      xp -= pixels_per_byte;
   }

   while(get_dot(xl, yp) != bound) {
      if(--xl < 0) break;
   }
   xl++;

   while(get_dot(xp, yp) != bound) {
      if(++xp > COLS) break;
   }
   xp--;

   while(1) {
      hline(yp, xl, xp);

      yp += yinc;
      if((yp >= ROWS) || (yp < 0)) {
         if(cur) show_cursor();
         return 0;
      }
#ifdef NONCONVEX
      while(get_dot(xl, yp) == bound) {  /* loss on the left */
         if(++xl > xp) {
            if(cur) show_cursor();
            return 0;
         }
      }
      while(get_dot(xl - 1, yp) != bound) { /* growth to left */
         if(--xl == 0) break;
      }
      while(get_dot(xp, yp) == bound) { /* loss on the right */
         --xp;
      }
      while(get_dot(xp + 1, yp) != bound) { /* growth to right */
         if(++xp == COLS) break;
      }

      pass_loop:
      xk = (xl + pixels_per_byte - 1)&(- pixels_per_byte);
      if(xk >= xp) xk = xp;
      for(xn=xl; xn<xk; xn++) {
         if(get_dot(xn, yp) == bound) {
//          printf(PS("Recursive Paint Call (%d-%d, %d)\n"), xl, xn-1, yp);
            pass_paint(xl, yp, bound, yinc);
            while(get_dot(xn, yp) == bound) xn++;
            xl = xn;
         }
      }
      for(xn=xk; xn<xp; xn+=pixels_per_byte) {
         if(FIND_COLOR(xn, yp, bound)) {
            while(get_dot(xn, yp) != bound) xn++;
            xn--;
            if(xn < xk) {
//             printf(PS("Recursive Paint Call (%d-%d, %d)\n"), xl, xn-1, yp);
               pass_paint(xl, yp, bound, yinc);
               while(get_dot(xn, yp) == bound) xn++;
               xl = xn;
               goto pass_loop;
            }
            break;
         }
      }
#else
      if(get_dot(xl, yp) != bound) {
         do {
            xl--;
         } while((xl >= 0) && (get_dot(xl, yp) != bound));
         xl++;
      }
      else {
         do {
            xl++;
         } while((xl < xp) && (get_dot(xl, yp) == bound));
      }

      if(get_dot(xp, yp) == bound) {
         do {
            xp--;
         } while((xp >= xl) && (get_dot(xp, yp) == bound));
      }
      else {
         do {
            xp++;
         } while((xp < COLS) && (get_dot(xp, yp) != bound));
         xp--;
      }

      if(xl > xp) break;
#endif
   }

   if(cur) show_cursor();
   return 1;
}

int paint(COORD xi, COORD yi, COLOR bound)
{
u08 cur;

   cur = erase_cursor();


   #ifdef MDT_CODE
      u08 hs = mdtHooked;
      if(mdtHooked) {
        mdt_paint(xi,yi,bound);
        if(mdtNoLocalDisplay) {
           if(cur) show_cursor();
           return 1;                  // !!! returning true regardless for now
        }
        mdtHooked = 0; // turn off for remainder of this function
      }   
   #endif


   if(get_dot(xi, yi) == bound) {
      if(cur) show_cursor();
      return 0;
   }

   pass_paint(xi, yi, bound, -1);
   pass_paint(xi, yi+1, bound, 1);

   if(cur) show_cursor();

   #ifdef MDT_CODE
     mdtHooked = hs; // restore mdtHooked
   #endif

   return 1;
}

char *stringPS(PGM_P ps)
{
   strncpy_P(ps_buf, ps, MAX_PS_LEN);
   return &ps_buf[0];
}

#endif // PANEL_CODE
