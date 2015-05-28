/*  MegaDonkey Library File:  vchar.c   Vector Character Support 
    


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

  Ron Grant
  April 18, 2007

  03 May 2007 - 16 bit coords & using clipped_line  
  25 May 2007 - modified text/printf routines for greater compatibility
                with bitmapped text routines - Mark Sims

*/


#include <avr/pgmspace.h>  // support for vars stored in prog mem 
                           // including vector character table
#include <stdio.h>
#include <ctype.h>
#include "md.h"

#ifdef VCHAR_CODE
#include "lcd.h"
#include "timer.h"
#include "adc.h"
#include "vchar.h"
#include "graph.h"   // 2D line clipper
#include "trakball.h"

// Vector Character Table
// this table remains in program memory
// requires special functions to access

#define BYTES_PER_VCHAR 6

unsigned int VCharTable[] PROGMEM = { 
  0x05,0xF8,0xAF,0xFF,0xFF,0xFF, // 33 "!" ,
  0x1E,0xF2,0x4F,0xFF,0xFF,0xFF, // 34 """ ,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // 35 "#" ,
  0x41,0x39,0xB8,0xBF,0xFF,0xFF, // 36 "$" ,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // 37 "%" ,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // 38 "&" ,
  0x1E,0xFF,0xFF,0xFF,0xFF,0xFF, // 39 "'" ,
  0x2E,0xDC,0xFF,0xFF,0xFF,0xFF, // 40 "(" ,
  0x0E,0xDA,0xFF,0xFF,0xFF,0xFF, // 41 ")" ,
  0x0C,0xF2,0xAF,0x57,0xFF,0xFF, // 42 "*" ,
  0x57,0xFE,0xDF,0xFF,0xFF,0xFF, // 43 "+" ,
  0xDA,0xFF,0xFF,0xFF,0xFF,0xFF, // 44 "," ,
  0x57,0xFF,0xFF,0xFF,0xFF,0xFF, // 45 "-" ,
  0x8A,0xFF,0xFF,0xFF,0xFF,0xFF, // 46 "." ,
  0x2A,0xFF,0xFF,0xFF,0xFF,0xFF, // 47 "/" ,
  0x83,0x14,0x9B,0x8F,0xFF,0xFF, // 48 "0" ,
  0x31,0xBF,0xAC,0xFF,0xFF,0xFF, // 49 "1" ,
  0x31,0x47,0x8A,0xCF,0xFF,0xFF, // 50 "2" ,
  0x01,0x49,0xBA,0xF5,0x7F,0xFF, // 51 "3" ,
  0xB6,0x51,0x67,0xFF,0xFF,0xFF, // 52 "4" ,
  0x20,0x56,0x9B,0xAF,0xFF,0xFF, // 53 "5" ,
  0x21,0x58,0xB9,0x75,0xFF,0xFF, // 54 "6" ,
  0x02,0xAF,0xFF,0xFF,0xFF,0xFF, // 55 "7" ,
  0x63,0x14,0x69,0xB8,0x6F,0xFF, // 56 "8" ,
  0x8B,0x94,0x13,0x5D,0x7F,0xFF, // 57 "9" ,
  0x1E,0xFD,0xBF,0xFF,0xFF,0xFF, // 58 ":" ,
  0x1E,0xFD,0xAF,0xFF,0xFF,0xFF, // 59 ";" ,
  0x25,0xCF,0xFF,0xFF,0xFF,0xFF, // 60 "<" ,
  0x57,0xF8,0x9F,0xFF,0xFF,0xFF, // 61 "=" ,
  0x07,0xAF,0xFF,0xFF,0xFF,0xFF, // 62 ">" ,
  0xBD,0x74,0x13,0xFF,0xFF,0xFF, // 63 "?" ,
  0x9B,0x83,0x14,0x7D,0x5E,0xD5, // 64 "@" ,
  0xA5,0x17,0xCF,0x57,0xFF,0xFF, // 65 "A" ,
  0xA0,0x14,0x69,0xBA,0x0F,0x56, // 66 "B" ,
  0x41,0x38,0xB9,0xFF,0xFF,0xFF, // 67 "C" ,
  0x0A,0xB9,0x41,0x0F,0xFF,0xFF, // 68 "D" ,
  0x20,0xAC,0xF5,0x6F,0xFF,0xFF, // 69 "E" ,
  0x20,0xAF,0x56,0xFF,0xFF,0xFF, // 70 "F" ,
  0x41,0x38,0xB9,0x76,0xFF,0xFF, // 71 "G" ,
  0x0A,0xF2,0xCF,0x75,0xFF,0xFF, // 72 "H" ,
  0x02,0xFA,0xCF,0x1B,0xFF,0xFF, // 73 "I" ,
  0x29,0xB8,0xFF,0xFF,0xFF,0xFF, // 74 "J" ,
  0x0A,0xF2,0x5C,0xFF,0xFF,0xFF, // 75 "K" ,
  0x0A,0xCF,0xFF,0xFF,0xFF,0xFF, // 76 "L" ,
  0xA0,0xE2,0xCF,0xFF,0xFF,0xFF, // 77 "M" ,
  0xA0,0xC2,0xFF,0xFF,0xFF,0xFF, // 78 "N" ,
  0x31,0x49,0xB8,0x3F,0xFF,0xFF, // 79 "O" ,
  0xA0,0x14,0x65,0xFF,0xFF,0xFF, // 80 "P" ,
  0xDC,0xF8,0x31,0x49,0xB8,0xFF, // 81 "Q" ,
  0xA0,0x14,0x65,0xCF,0xFF,0xFF, // 82 "R" ,
  0x41,0x39,0xB8,0xFF,0xFF,0xFF, // 83 "S" ,
  0x02,0xF1,0xBF,0xFF,0xFF,0xFF, // 84 "T" ,
  0x08,0xB9,0x2F,0xFF,0xFF,0xFF, // 85 "U" ,
  0x0B,0x2F,0xFF,0xFF,0xFF,0xFF, // 86 "V" ,
  0x0A,0x6C,0x2F,0xFF,0xFF,0xFF, // 87 "W" ,
  0x0C,0xF2,0xAF,0xFF,0xFF,0xFF, // 88 "X" ,
  0x0E,0x2B,0xFF,0xFF,0xFF,0xFF, // 89 "Y" ,
  0x02,0xAC,0xFF,0xFF,0xFF,0xFF, // 90 "Z" ,
  0x21,0xBC,0xFF,0xFF,0xFF,0xFF, // 91 "[" ,
  0x0C,0xFF,0xFF,0xFF,0xFF,0xFF, // 92 "\" ,
  0x01,0xBA,0xFF,0xFF,0xFF,0xFF, // 93 "]" ,
  0x51,0x7F,0xFF,0xFF,0xFF,0xFF, // 94 "^" ,
  0xAC,0xFF,0xFF,0xFF,0xFF,0xFF, // 95 "_" ,
  0x0E,0xFF,0xFF,0xFF,0xFF,0xFF, // 96 "`" 
};



/*
   Star Burst Vertices that form skeleton of vector characters
   defines 15 x,y coords  0..0xE
   These tables are loaded into SRAM at runtime 32 bytes

  Starburst Vertex Org is shown 4x6 grid

    0   1   2             +-----> x+
    3       4             |
                          |
    5   6   7             y+
        D
    8       9
    A   B   C       15 vertices  F=terminator
*/


char VCharStarX[] = {
  0x00,0x02,0x04,0x00,0x04,0x00,0x02,0x04,0x00,0x04,0x00,0x02,0x04,0x02,0x02};

char VCharStarY[] = {
  0x00,0x00,0x00,0x01,0x01,0x03,0x03,0x03,0x05,0x05,0x06,0x06,0x06,0x04,0x02};


#define VCHAR_W 4 // elemental width and height of starburst pattern 
#define VCHAR_H 6 // scaled by VCharScaleX and Y


// vector character attributes
// set generally by function calls

signed char VCharScaleX;
signed char VCharScaleY;


u08 VCharHeight;     // character height computed
u08 VCharWidth;      // character width  computed


void vchar_init(void) 
{
   viewport_init();
   vchar_set_fontsize(1,1);
   vchar_set_thickness(1,1);
   vchar_set_spacing(2,2);

   lcd_setxy (0,0);
}


void vchar_set_fontsize(u08 scaleX, u08 scaleY)
{
   VCharScaleX = scaleX;
   VCharScaleY = scaleY;

   VCharHeight = VCHAR_H * scaleY;
   VCharWidth  = VCHAR_W * scaleX;
}
   

void vchar_set_thickness(u08 thick_x, u08 thick_y)
{ 
   VCharThicknessX = thick_x;
   VCharThicknessY = thick_y;
}


void vchar_set_spacing(u08 space_x, u08 space_y)
{ 
   VCharSpaceX = space_x;
   VCharSpaceY = space_y;
}




int  VIndex;  // char gen table offset
u08  VByte;   // VertexByte (really nibble)
char VCount;  // counter tracking offset into given character



void vchar_erase(COORD col, COORD row)
{
u08 i;
u08 cur;

cur = erase_cursor();

   invert_colors();

   for(i=0; i<VCharHeight+VCharSpaceY; i++) {
      hline(row+i, col, col+VCharWidth+VCharSpaceX-1);
   }

   invert_colors();

if(cur) show_cursor();
   return;
}


char get_vchar_vertex(void)  // get next vertex nibble from char gen table
{
   if(++VCount & 1){ 
      VByte = pgm_read_byte_near(&VCharTable[VIndex++]);
      return (VByte >> 4); 
   }
   else return (VByte & 0xF); 
}


void vchar_char(int xoffset,int yoffset,char  c)  // draw a vector character
{ 
int x1,y1, x2,y2, x,y;  
u08 v1,v2;
u08 cur;

cur = erase_cursor();

   if(printf_flags & VCHAR_ERASE) {
       vchar_erase(xoffset, yoffset);
   }

   // First Character in table is 33 ASCII "!"
   if(c < '!') {
if(cur) show_cursor();
      return;
   }
   c = toupper(c);

   // loops allow overprinting of vector character with x,y
   // offsets, default is thickness 1 in x and y
   // result is character is drawn one time

   for(y=yoffset;y<yoffset+VCharThicknessY;y++) {
      for(x=xoffset;x<xoffset+VCharThicknessX;x++) {
       
         VIndex =  (c-33) * BYTES_PER_VCHAR; // byte offset into table (x6)
         VCount = 0;
  
         // Sample Char
         // 0xA5,0x17,0xCF,0x57,0xFF,0xFF, // 65 "A" ,
         //
         // vertices are drawn in a chain with F used to start
         // new chain FF or 12 vertices ends character
         //
         // for "A" vertices are A 5 1 7 C <break> 5 7 

         v1 = get_vchar_vertex();   // get first vertex
         if(v1 == 0xF) {
if(cur) show_cursor();
            return;     // undefined character 
         }

         while(VCount <13) { // 12 nibbles vertices is max allowed for character
            v2 = get_vchar_vertex();
            if(v2 == 0xF) {
               v1 = get_vchar_vertex();
               if(v1 == 0xF) goto exit;
               v2 = get_vchar_vertex();
               if(v2 == 0xF) goto exit;
            } 
   
            // lookup x,y coords in starburst tables
            // then scale to screen coordinates & add current offset
    
            x1 = VCharStarX[v1] * VCharScaleX + x;
            y1 = VCharStarY[v1] * VCharScaleY + y;

            x2 = VCharStarX[v2] * VCharScaleX + x;
            y2 = VCharStarY[v2] * VCharScaleY + y;
  
            clipped_line(x1,y1,x2,y2);

            v1 = v2;
         }

         exit:;
      } // end for x
   } // end for y 
if(cur) show_cursor();
}  




// PRELIM support of stdout using stdio.h 

static int vchar_putchar(char c, FILE *stream); // fwd declare


// macro FDEV_SETUP_STREAM provided by stdio.h
// put,get,rwflag

static FILE vchar_stdout = FDEV_SETUP_STREAM(vchar_putchar, NULL,
                                             _FDEV_SETUP_WRITE);


void vchar_set_stdout(void)
{
   stdout = &vchar_stdout;   
   printf_flags = 0xFFFF;
}

static int vchar_putchar(char c, FILE *stream)
{
u08 cur;

cur = erase_cursor();

   if(printf_flags & VCHAR_COMPAT) {  // force char sizes to match bitmapped chars
      vchar_set_fontsize(char_size, char_size); 
      vchar_set_thickness(char_size, char_size);
      vchar_set_spacing(4*char_size, 2*char_size);
   }

   if((printf_flags & PRINTF_BEL) && (c == '\a')) {  // beep
      beep(200,2000); 
   }
   else if((printf_flags & PRINTF_BS) && (c == '\b')) {  // back space
      if(lcd_col < (VCharWidth+VCharSpaceX)) lcd_col = 0;
      else lcd_col -= (VCharWidth+VCharSpaceX);
   }
   else if((printf_flags & PRINTF_FF) && (c == '\f')) { // form feed
      lcd_col = lcd_row = 0;
   }
   else if((printf_flags & PRINTF_NL) && (c == '\n')) { // newline (CRLF)
      lcd_col = 0;
      lcd_row += (VCharHeight+VCharSpaceY);
   }
   else if((printf_flags & PRINTF_CR) && (c == '\r')) { // carriage return
      lcd_col = 0;
   }
   else if((printf_flags & PRINTF_TAB) && (c == '\t')) {  //!!! how best to handle TAB?
      lcd_col += (VCharWidth+VCharSpaceX);
   }
   else if((printf_flags & PRINTF_VT) && (c == '\v')) {  // vertical tab (line feed)
      lcd_row += (VCharWidth+VCharSpaceX);
   }
   else { // output all other characters
      vchar_char(lcd_col, lcd_row, c);
      lcd_col += (VCharWidth+VCharSpaceX);
   }

   if(lcd_col > COLS-(VCharWidth+VCharSpaceX)) {  // wrap to next line
      lcd_col = 0;
      lcd_row += (VCharHeight+VCharSpaceY);
   } 
   
   if(lcd_row > ROWS-(VCharHeight+VCharSpaceY)) {  // scroll screen up
      if(printf_flags & PRINTF_SCROLL) {
         lcd_row -= (VCharHeight+VCharSpaceY);
         lcd_scroll(VCharHeight+VCharSpaceY);
      }
      else lcd_row = 0;
   }

if(cur) show_cursor();
   return 0;
}


// output a text string
// does not use stdout
// mimics behavior of putchar except does not decode \n

void vchar_text(COORD col,COORD row,  char *s)
{
unsigned char c;
u08 cur;

cur = erase_cursor();

   if(printf_flags & VCHAR_COMPAT) {  // force char sizes to match bitmapped chars
      vchar_set_fontsize(char_size, char_size); 
      vchar_set_thickness(char_size, char_size);
      vchar_set_spacing(4*char_size, 2*char_size);
   }

   while((c=*s++)) {
      vchar_char(col, row, c);

      if(rotate & ROT_STRING_VERT) {  /* draw strings vertically */
         if(rotate & ROT_STRING_HORIZ) row -= (VCharHeight+VCharSpaceY);
         else row += (VCharHeight+VCharSpaceY);
         if(row >= ROWS) break;   /* !!! clipping */
      }
      else { /* draw strings horizontally */
         if(rotate & ROT_STRING_HORIZ) col -= (VCharWidth+VCharSpaceX);
         else col += (VCharWidth+VCharSpaceX);
         if(col >= COLS) break;   /* !!! clipping */
      }
   }
if(cur) show_cursor();
}


void vchar_textPM(COORD col,COORD row,  PGM_P s)
{
unsigned char c;
u08 i;
u08 cur;

cur = erase_cursor();

   if(printf_flags & VCHAR_COMPAT) {  // force char sizes to match bitmapped chars
      vchar_set_fontsize(char_size, char_size);   
      vchar_set_thickness(char_size, char_size);
      vchar_set_spacing(4*char_size, 2*char_size);
   }

   i = 0;
   while(1) {
      c = pgm_read_byte(s+i);
      if(c == 0) break;
      ++i;

      vchar_char(col, row, c);

      if(rotate & ROT_STRING_VERT) {  /* draw strings vertically */
         if(rotate & ROT_STRING_HORIZ) row -= (VCharHeight+VCharSpaceY);
         else row += (VCharHeight+VCharSpaceY);
         if(row >= ROWS) break;   /* !!! clipping */
      }
      else { /* draw strings horizontally */
         if(rotate & ROT_STRING_HORIZ) col -= (VCharWidth+VCharSpaceX);
         else col += (VCharWidth+VCharSpaceX);
         if(col >= COLS) break;   /* !!! clipping */
      }
   }
if(cur) show_cursor();
}


#ifdef VCHAR_DEMO

void vchar_demo(void)
{
u08 i;

   set_color(WHITE);
   set_bg(BLACK);
   lcd_clear();

   vchar_init();         /* vector LCD font init  */
   vchar_set_stdout();
   printf_flags &= (~VCHAR_COMPAT);   // don't force vchar cells to bitmapped char size

   vchar_set_fontsize(1,1);
   VCharSpaceX = 3;
   printf(PS("ABCDEFGHIJKLMNOPQRSTUVWXYZ\n"));
   printf(PS("01234567890 !@#$%%^&*()_+-=\n"));
   printf(PS("[]|\\:;""',.?/\n"));

   vchar_set_thickness(2,2);
   vchar_set_fontsize(2,1);
   VCharSpaceX = 4;
   printf(PS("ABCDEFGHI"));

   delay_until_touched(2000);
   wait_while_touched();

   lcd_clear();
   vchar_set_thickness(2,2);
   vchar_set_fontsize (4,4);

   for(i=0; i<6; i++) {
     lcd_clear();
     vchar_set_thickness(i+1,i+1);
     vchar_set_fontsize(i,i);

     printf(PS("601"));
     delay_ms(1000);

#ifdef USER_INPUT
     if(get_touch(1)) break;
#endif
   }

   lcd_set_stdout();    // make printf() use bitmapped chars
}

#endif  // VCHAR_DEMO
#endif  // VCHAR_CODE
