/*  MegaDonkey Library File:  demolcd.c    LCD Graphics and Text Demos
    


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




    LCD Graphics and Text Demos  
    Mark Sims 2007
 
*/ 

#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "md.h"         // for sbi cbi macros

#ifdef LCD_DEMO
#include "lcd.h"
#include "timer.h"
#include "twi.h"
#include "uart.h"
#include "timer.h"
#include "adc.h"
#include "menu.h"
#include "kbd.h"
#include "graph.h"     // sine tables 
#include "trakball.h"

#define XCOLS COLS
#define XROWS ROWS



/* ------------------------ test patterns -------------------------- */

char kbhit(void)
{
#ifdef USER_INPUT
    if(get_touch(1)) return 1;
    else return 0;
#else
   return 1;
#endif
}


char getch(void)
{
#ifdef USER_INPUT
   while(kbhit() == 0);
   while(kbhit()) ;
#endif
   return '*';
}

#ifdef TIME_CLOCK

char *months[] = {
   "---",
   "Jan", "Feb", "Mar", "Apr", "May", "Jun",
   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

char *days[] = {
   "Sunday",
   "Monday",
   "Tuesday",
   "Wednesday",
   "Thursday",
   "Friday",
   "Saturday"
};

void dclock(void)
{
char s[COLS/CHAR_WIDTH+1];
u08 len;
u08 rr;
u08 last_d;
u08 last_s;
u08 i;

// time.hours = 1;
// time.mins = 59;
// time.secs = 55;
// time.day = 18;
// time.month = 5;
// time.year = 2007;
// time.weekday = day_of_week(time.day, time.month, time.year);

   last_d = last_s = 99;

#ifdef NO_OFFSCREEN
   page_buffers(1);   // we are not using double buffered graphics
#else
   page_buffers(2);   // we are using double buffered graphics
#endif

   wait_while_touched();

   while(1) {
      if(get_touch(1)) break;

      getdt();
      if(time.day != last_d) { // redraw the screens
         last_d = time.day;

         if(XCOLS < 160) set_charsize(1);
         else if(XCOLS < 240) set_charsize(2);
         else set_charsize(3);
         rr = XROWS/4 - ((CHAR_HEIGHT*char_size)/2);
         
         for(i=0; i<2; i++) {
            lcd_pageflip();

            lcd_clear();
            if(XCOLS >= 160) thick_box(0,0,  XCOLS-1, XROWS-1, 3);

            sprintf(s, PS("%d %s %02d"), time.day, months[(int) time.month], time.year%100);
            len = strlen(s)*CHAR_WIDTH*char_size;
            len = (XCOLS-len) / 2;
            lcd_setxy(len,XROWS/2-((CHAR_HEIGHT*char_size)/2));
            printf(PS("%s"), s);

            sprintf(s, PS("%s"), days[(int) time.weekday]);
            len = strlen(s)*CHAR_WIDTH*char_size;
            len = (XCOLS-len) / 2;
            lcd_setxy(len,XROWS/2+rr);
            printf(PS("%s"), s);
         }
      }

      if(time.secs != last_s) { // update the time display
         last_s = time.secs;

         if(XCOLS < 240) set_charsize(2);
         else set_charsize(3);
         rr = XROWS/4 - ((CHAR_HEIGHT*char_size)/2);

         lcd_pageflip();
         sprintf(s, PS("%02d:%02d:%02d"), time.hours, time.mins, time.secs);
         len = strlen(s)*CHAR_WIDTH*char_size;
         len = (XCOLS-len) / 2;
         lcd_setxy(len,0+rr);
         printf(PS("%s"), s);
      }
   }

   wait_while_touched();
   page_reset();  // end of double buffered grahics
   lcd_clear();
   set_charsize(1);
}



char *roman[] = {
   "III",   "IIII",  "V",
   "VI",    "VII",   "VIII",
   "IX",    "X",     "XI",
   "XII",   "I",     "II"
};
//char *roman[] = {
//   "3",   "4",    "5",
//   "6",   "7",    "8",
//   "9",   "10",   "11",
//   "12",  "1",    "2"
//};
//char *roman[] = {
//   "*",   "*",    "*",
//   "*",   "*",    "*",
//   "*",   "*",    "*",
//   "*",   "*",    "*"
//};

#define XCLK (XCOLS/2)
#define YCLK ((XROWS/2)+1)

#ifdef RES_128x64
   #define ACLOCK_R 28
#endif
#ifdef RES_132x132
   #define ACLOCK_R 56
#endif
#ifdef RES_160x80
   #define ACLOCK_R (47*192/ASPECT)
#endif
#ifdef RES_160x128
   #define ACLOCK_R 56
#endif
#ifdef RES_240x128
   #define ACLOCK_R 56
#endif
#ifdef RES_320x240
   #define ACLOCK_R 110
#endif

void draw_hands(int hh, int mm, int ss)
{
int x, y;
#define AA (ACLOCK_R/8)

   set_draw_flags(BLIT_XOR);

   x = ((ACLOCK_R-AA) * cos360((ss+30)*6 + 90)) >> 7;
   y = ((((ACLOCK_R-AA) * sin360((ss+30)*6 + 90)) >> 7) * ASPECT) / 256;
   line(XCLK,YCLK,  XCLK+x,YCLK+y);

   if(mm != ss) { // if minutes matches seconds... dont do both hands
      x = ((ACLOCK_R-AA) * cos360((mm+30)*6 + 90)) >> 7;
      y = ((((ACLOCK_R-AA) * sin360((mm+30)*6 + 90)) >> 7) * ASPECT) / 256;
      line(XCLK,YCLK,  XCLK+x,YCLK+y);
   }

   if(hh != mm) {
      x = ((ACLOCK_R-AA*2) * cos360((hh+6)*30 + (mm/2) + 90)) >> 7;
      y = ((((ACLOCK_R-AA*2) * sin360((hh+6)*30 + (mm/2) + 90)) >> 7) * ASPECT) / 256;
      line(XCLK,YCLK,  XCLK+x,YCLK+y);
   }

   set_draw_flags(FILLED);
   circle(XCLK,YCLK,4); // clock hub

   set_draw_flags(0x00);
}

void aclock(void)
{
int x0, y0;
int x, y;
int theta;
int lasth, lastmin, lastsec;
int hours, minutes, seconds;
u08 cur;
u08 hands_on;

cur = erase_cursor();
   lcd_clear();

   for(lasth=0; lasth<12; lasth++) {  // draw the clock numerals
      x = (ACLOCK_R * cos360(lasth*30)) >> 7;
      y = (((ACLOCK_R * sin360(lasth*30)) >> 7) * ASPECT) / 256;
      if(COLS < 160) {
         lcd_textPS(XCLK+x-CHAR_WIDTH/2, YCLK+y-CHAR_HEIGHT/2, "*");
      }
      else {
         lcd_text(XCLK+x-(strlen(roman[lasth])*CHAR_WIDTH/2), YCLK+y-CHAR_HEIGHT/2, roman[lasth]);
      }
   }

   x0 = (ACLOCK_R * cos360(0)) >> 7;
   y0 = (((ACLOCK_R * sin360(0)) >> 7) * ASPECT) / 256;
   for(theta=1; theta<360; theta++) {  // draw the clock outline
      x = (ACLOCK_R * cos360(theta)) >> 7;
      y = (((ACLOCK_R * sin360(theta)) >> 7) * ASPECT) / 256;
      line(XCLK+x0,YCLK+y0, XCLK+x,YCLK+y);
      x0 = x;
      y0 = y;
   }

   box(0,0, XCOLS-1,XROWS-1);  // put a box around the screen
if(cur) show_cursor();


   lastsec = lastmin = lasth = 99;
   hands_on = 0;

   wait_while_touched();

   while(1) {
      if(get_touch(1)) break;

      getdt();
      seconds = time.secs;
      minutes = time.mins;
      hours = time.hours;

      if(seconds != lastsec) {
         // erase old hands
         if(hands_on) {
            draw_hands(lasth, lastmin, lastsec);
         }

         // draw new hands
         draw_hands(hours, minutes, seconds);
         hands_on = 1;

         lastsec = seconds;
         lastmin = minutes;
         lasth = hours;
      }
   }

   wait_while_touched();
   lcd_clear();
}
#endif //TIME_CLOCK


#ifdef COLOR_LCD
void color_bars(void)
{
COORD w;
COORD col;
COLOR temp_color;

   temp_color = color;
   w = COLS/8;

   fill_screen(BLACK);

   for(col=0; col<8; col++) {
      set_color(color_table[col]);
      filled_box((col*w)+1,1,  (col*w)+w-1,ROWS/2-1-1);

      set_color(color_table[col+8]);
      filled_box((col*w)+1,ROWS/2+2,  (col*w)+w-1,ROWS-1-1);
   }

   delay_until_touched(3000);
   wait_while_touched();

   set_color(temp_color);
}

void color_palette(void)
{
u16 col;
COLOR temp_color;

   temp_color = color;

   fill_screen(BLACK);

   for(col=0; col<128; col++) {
      set_color(RGB(col*2, 0, 0));
      filled_box(col,0, col+1,ROWS/4-2);

      set_color(RGB(0, col*2, 0));
      filled_box(col,ROWS/4, col+1,ROWS/2-2);

      set_color(RGB(0, 0, col*2));
      filled_box(col,ROWS/2, col+1,(ROWS*3)/4-2);

      set_color(RGB(col*2, col*2, col*2));
      filled_box(col,(ROWS*3)/4, col+1,ROWS-1);
   }

   delay_until_touched(3000);
   wait_while_touched();

   set_color(temp_color);
}
#endif // COLOR_LCD

void star_test(void)
{
COORD x1, y1;
unsigned i, j;

   x1 = COLS/4;
   y1 = ROWS/2;
   j = ROWS/64;
   i = ((((unsigned long) j) * (unsigned long) COLS) / (unsigned long) ROWS);
   line(x1-63*i/6,y1-14*j/4, x1+63*i/6,y1-14*j/4);
   line(x1+63*i/6,y1-14*j/4, x1-39*i/6,y1+60*j/4);
   line(x1-39*i/6,y1+60*j/4, x1,y1-60*j/4);
   line(x1,y1-60*j/4,        x1+39*i/6,y1+60*j/4);
   line(x1+39*i/6,y1+60*j/4, x1-63*i/6,y1-14*j/4);

#ifndef NOKIA
   paint(x1, y1, WHITE);
#endif
}

void fishnet_test(void)
{
int i, j, k, y1;
unsigned aspect;
COLOR temp_color;

   aspect = 15 * COLS / ROWS;
   aspect /= 2;
   k = 10;
   if(COLS >= 800) {
      k *= 2;
      aspect *= 2;
   }
   else if((COLS >= 640) && (ROWS >= 350)) {
      k *= 3;
      k /= 2;
      aspect *= 3;
      aspect /= 2;
   }

   y1 = ROWS/3;
   j = COLS/2;


   temp_color = color;
   set_color(color_table[11]);

   line(0, ROWS-1, j, ROWS-1);
   for(i=ROWS-1; j>0; i-=k, j-=aspect) {
      line(0,i, j,ROWS-1);
   }
   line(0,y1, 0,ROWS-1);

   set_color(temp_color);
}

void tree_test(void)
{
int i, j, xinc;
COLOR temp_color;

   temp_color = color;

   xinc = (5 * COLS);
   xinc /= (6 * ROWS);
   if(xinc <= 0) xinc = 1;
   for(i=0, j=0; i<ROWS/4; i++, j+=xinc) { /* h_line test */
      if(i&1) {
         #ifdef COLOR_LCD
            set_color(color_table[1+((i>>1)&0x07)]);
         #endif
         hline(ROWS/8+i,COLS/2, COLS/2+j);
      }
      else {
         #ifdef COLOR_LCD
            set_color(color_table[8+((i>>1)&0x07)]);
         #endif
         hline(ROWS/8+i,COLS/2, COLS/2-j);
      }
   }
    
   set_color(temp_color);
}

void ellipses_test(void)
{
int i, j;
int x1, y1;
COLOR temp_color;
u16 aspect;

   temp_color = color;
   j = 1;

   x1 = (35*COLS) >> 6;

   y1 = (23*ROWS) >> 5;
   i = (6 * COLS) >> 5;
   x1 -= 4;
   y1 -= 2;
   aspect = ((long)ROWS)*384L/(long)COLS;
   while(aspect > 12) {
      #ifdef COLOR_LCD
         set_color(RGB(aspect&0xFF, (aspect<<3) & 0xFF,  (aspect<<6)&0xFF));
      #endif
      ellipse(x1,y1, i, aspect);

      aspect -= (aspect >> 2) + 8;
   }
   set_color(temp_color);
}

void pattern_test(void)
{
COLOR temp_color;
u08 c;

   temp_color = color;
   box(0,0, COLS-1,ROWS-1);

   #ifdef COLOR_LCD
      int row;
      c = BLACK;
      for(row=ROWS/2-ROWS/3; row<=ROWS/2+ROWS/3+3; row++) {
         c += 3;
         set_color(RGB(c&0xFF, (c<<3) & 0xFF, (c<<6)&0xFF));
         hline(row, COLS-COLS/4, COLS-COLS/16+2);   
      }
   #else
      set_color(0xAA); c = 0xAA;  set_draw_flags(draw_flags | DITHERED);
      filled_box(COLS-COLS/4,ROWS/2-ROWS/3, COLS-COLS/16+2,ROWS/2);
      set_color(WHITE); set_draw_flags(0x00);
      filled_box(COLS-COLS/4,ROWS/2, COLS-COLS/16+2,ROWS/2+ROWS/3+3);
   #endif

   set_color(temp_color);

   star_test();
   fishnet_test();
   tree_test();
   ellipses_test();
}


void thermo_test()
{
unsigned y;

#define BULB_SIZE    (ROWS/4)        // radius
#define COLUMN_WIDTH (BULB_SIZE/2)   // half width
#define COLUMN_TOP   4
#define BULB_CENTER  ((BULB_SIZE*ASPECT)/256)

   // draw thermometer outline
   set_draw_flags(draw_flags & (~FILLED));
   circle(50,ROWS-BULB_CENTER, BULB_SIZE);
   box(50-COLUMN_WIDTH, 0, 50+COLUMN_WIDTH, ROWS-BULB_SIZE);

   // trim thermometer bulb/column interior
   set_draw_flags(draw_flags | FILLED);
   invert_colors();
   circle(50,ROWS-BULB_CENTER, BULB_SIZE-1);
   filled_box(50-(COLUMN_WIDTH-1),1, 50+(COLUMN_WIDTH-1),ROWS-BULB_SIZE);
   invert_colors();

   while(kbhit() == 0) {
      circle(50, ROWS-BULB_CENTER, BULB_SIZE-4);   // fill in bulb

      for(y=ROWS-BULB_CENTER*2+4+3; y>=COLUMN_TOP; y--) {  // raise temp
         hline(y,50-(COLUMN_WIDTH-3), 50+(COLUMN_WIDTH-3));
         delay_ms(20);
         if(get_touch(1)) goto thermo_exit;
      }

      invert_colors();
      for(y=COLUMN_TOP; y<ROWS-BULB_CENTER*2+4+3; y++) {  // lower temp
         hline(y,50-(COLUMN_WIDTH-3), 50+(COLUMN_WIDTH-3));
         delay_ms(20);
         if(get_touch(1)) {
            invert_colors();
            goto thermo_exit;
         }
      }
      invert_colors();
   }

   thermo_exit:
   set_draw_flags(0x00);
}


// this demo scrolls a screen full of text to the right then to the left
// where the column about to be scrolled off the screen is saved by copying to 
// spare page (if available) then copied back.


void side_blit_test(void)
{
unsigned x;
unsigned row, col;

   set_draw_flags(0x00);
   x = 0x00;
   for(row=0; row<ROWS; row+=CHAR_HEIGHT) {
      for(col=0; col<COLS; col+=CHAR_WIDTH) {
         lcd_char(col,row, x);
         if(++x > 255) x = 0;
      }
   }

   // This demo cheats on panels with only one full page of display memory.  
   // One column of pixels will be corrupted since the panel does not 
   // have offscreen memory to blit it to/from.
   for(col=0; col<COLS; col++) {
#ifdef NO_OFFSCREEN
      blit(COLS-1,0, COLS-1,ROWS-1, 0,0);     // copy right most col to left
      blit(0,     0, COLS-2,ROWS-1, 1,0);     // scroll screen to right 
#else
      blit(COLS-1,0, COLS-1,ROWS-1, 0,ROWS);  // save right most col
      blit(0,     0, COLS-2,ROWS-1, 1,0);     // scroll screen to the right
      blit(0,ROWS,   0,ROWS+ROWS-1, 0,0);     // copy saved col to leftmost col
#endif

      #ifdef LCD_MDT   // NO LCD Panel 
	    delay_ms(50);  // delay 1/20th second 
	  #endif           // because above blits are very fast not having to 
	                   // draw locally -- just time to buffer mdt_blit command

#ifdef USER_INPUT
      if(kbhit()) break;
#endif
   }
   if(kbhit()) getch();

   for(col=0; col<COLS; col++) {
#ifdef NO_OFFSCREEN
      blit(0,0, 0,ROWS-1,       COLS-1, 0);
      blit(1,0, COLS-1,ROWS-1,  0,0);
#else
      blit(0,0,    0,ROWS-1,      0,ROWS);      // save left most col
      blit(1,0,    COLS-1,ROWS-1, 0,0);         // scroll to the left
      blit(0,ROWS, 0,ROWS+ROWS-1, COLS-1, 0);   // copy saved col to right most col
#endif

      #ifdef LCD_MDT   // NO LCD Panel 
	    delay_ms(50);  // delay 1/20th second 
	  #endif           // because above blits are very fast not having to 
	                   // draw locally -- just time to buffer mdt_blit command

#ifdef USER_INPUT
      if(kbhit()) break;
#endif
   }
}

void blit_chars_test(void)
{
unsigned x;
unsigned row, col;
COLOR temp_c;
u08 cur;

#define BCW (CHAR_WIDTH*2-1)
//#define BCW (CHAR_WIDTH*2+2)

   x = 0;
   lcd_char(x,0,   'A');
   lcd_char(CHAR_WIDTH+x,0, 'B');
   lcd_char(x,CHAR_HEIGHT,   'C');
   lcd_char(CHAR_WIDTH+x,CHAR_HEIGHT, 'D');

   box(x,0, x+BCW,CHAR_HEIGHT-1);

   col = CHAR_WIDTH*3;
   for(row=8; row<80; row+=8) {  // do small blits around critical boundaries
      blit(x,0, x+BCW,8-1, col,row);
      blit(x,0, x+BCW,8-1, 81-col,row);
//    blit(x,0, x+BCW,8-1, 24+81-col,row);
      col++;
   }

if(1) {
cur = erase_cursor();
   temp_c = color;
   for(row=0; row<CHAR_HEIGHT*2; row++) {
      for(col=0; col<BCW; col++) {
         set_color(get_dot(col, row));
         dot(COLS-BCW-1+col, ROWS-CHAR_HEIGHT*2+row);
      }
   }
   set_color(temp_c);
if(cur) show_cursor();
}

if(1) {
cur = erase_cursor();
   for(row=0; row<CHAR_HEIGHT*2; row++) {
      x = read_screen(0,row);
      write_screen(0,ROWS-CHAR_HEIGHT*2+row, x);
      x = read_screen(1,row);
      write_screen(1,ROWS-CHAR_HEIGHT*2+row, x);
   }
if(cur) show_cursor();
}
}


void hline_test(void)
{
COORD row;
COLOR temp_color;

   temp_color = color;

   for(row=0; row<40; row++) {
      #ifdef COLOR_LCD
         set_color(RGB(row&0xFF, (row<<3) & 0xFF,  (row<<6)&0xFF));
      #endif
      hline(row,1+80,        1+80+row);

      #ifdef COLOR_LCD
         set_color(RGB((255-row)&0xFF, ((255-row)<<3) & 0xFF,  ((255-row)<<6)&0xFF));
      #endif
      hline(row+40,1+79-row, 1+79);
   }

   set_color(temp_color);
}


void paint_test(void)
{
#define BOT ROWS-10

   set_color(WHITE);
   set_bg(BLACK);
   box(10,10, BOT,BOT);

   box(10,25, 20,35);
   box(BOT-10,BOT-20, BOT,BOT-10);
   invert_colors();
   line(10,25+1, 10,35-1);
   line(BOT,BOT-20+1, BOT,BOT-10-1);
   invert_colors();

   if(0) {   // little crown on top causes paint to break
      box(20,10, 30,20);
      box(BOT-20,10, BOT-10,20);

      invert_colors();
      hline(10,21, 29);
      hline(10,BOT-20+1, BOT-20+9);
      invert_colors();
   }

   delay_until_touched(500);
   wait_while_touched();

   paint(40,40, WHITE);
 
}


void rotated_char_test(void)
{
COLOR temp_color;
COLOR temp_bg;
u08 rot;

   temp_color = color;
   temp_bg = bg_color;

   set_charsize(COLS/80); //!!!!
   if(char_size < 2) set_charsize(2);

   for(rot=0; rot<8; rot++) {
      set_rotate(rot);
      set_color(WHITE);
      set_bg(BLACK);
      lcd_textPS(CHAR_WIDTH+CHAR_WIDTH*char_size*rotate,0*CHAR_HEIGHT*char_size, "J");

      invert_colors();
      lcd_textPS(CHAR_WIDTH+CHAR_WIDTH*char_size*rotate,1*CHAR_HEIGHT*char_size, "J");
      invert_colors();

      set_draw_flags(draw_flags | DITHERED);
      set_color(0xAA);
      lcd_textPS(CHAR_WIDTH+CHAR_WIDTH*char_size*rotate,2*CHAR_HEIGHT*char_size, "J");
      set_color(0x55);
      lcd_textPS(CHAR_WIDTH+CHAR_WIDTH*char_size*rotate,3*CHAR_HEIGHT*char_size, "J");
      set_draw_flags(draw_flags & (~DITHERED));
   }

   set_rotate(0x00);
   set_color(temp_color);
   set_bg(temp_bg);
   set_charsize(1);
}


void shaded_circle_test(void)
{
COLOR temp_color;
COLOR temp_bg;

   temp_color = color;
   temp_bg = bg_color;

   #ifdef COLOR_LCD
      set_color(RGB(0x00, 0x00, 0xFF));     //BLUE
      set_bg(RGB(0xFF, 0x00, 0x00));  //RED
   #else
      set_color(0xAA);
   #endif

   set_draw_flags(FILLED | DITHERED);
   ellipse(COLS/2,ROWS/2, (ROWS/2)-1, ASPECT);
   set_draw_flags(0x00);

   set_color(temp_color);
   set_bg(temp_bg);
}


void char_size_test(void)
{
COORD col;

   if(COLS >= (20*CHAR_WIDTH)) {
      col = (COLS - (20*CHAR_WIDTH)) / 2;
      invert_colors();
      lcd_textPS(col,ROWS-CHAR_HEIGHT, "Bubble Headed Ninny!");
      invert_colors();

      line(0, ROWS-CHAR_HEIGHT-1, COLS-1, ROWS-CHAR_HEIGHT-1);

      set_charsize(2);
      col = char_size * CHAR_WIDTH;
      lcd_textPS(0,0*CHAR_HEIGHT*char_size, "-");
      lcd_textPS(0,1*CHAR_HEIGHT*char_size, "-");
      lcd_textPS(0,2*CHAR_HEIGHT*char_size, "-");
      lcd_textPS(0,3*CHAR_HEIGHT*char_size, "-");
   }
   else {
      set_charsize(2);
      col = 0;
   }


   lcd_textPS(col,0*CHAR_HEIGHT*char_size,  "-DANGER--");
   lcd_textPS(col,1*CHAR_HEIGHT*char_size,  "--WILL---");
   lcd_textPS(col,2*CHAR_HEIGHT*char_size,  "ROBINSON-");
   lcd_textPS(col,3*CHAR_HEIGHT*char_size,  "-DANGER--");

   set_charsize(1);
}


void live_kbd_test(void)
{
COORD col, x;

   set_color(WHITE);
   set_bg(BLACK);

   col = 0;
   lcd_setxy(col,0);
   wait_while_touched();

   keyboard_drawn = 0; // first call to keyboard will draw the keyboard, then set this flag
   while(1) {
      x = menu_kbd("Who's your daddy?", 0, 0);
      if(x) { // a key has been pressed
         if(x == 0x1B) break;
         if(x == 0x0D) break;

         printf("%c", x);
         col += CHAR_WIDTH;
         if(col >= (COLS-CHAR_WIDTH)) {
            col = 0;
            lcd_setxy(col,0);
         }

         wait_while_touched();
      }
   }

   wait_while_touched();
   keyboard_drawn = 0;  // flag keyboard has been closed
}


void blc_test(void)
{
#ifdef RES_160x80
   #define BCOLS (COLS*192/ASPECT)
#else
   #define BCOLS COLS
#endif
   thick_box(0,0, COLS-1,ROWS-1, 6);

   line(0,0,      COLS-1,ROWS-1);
   line(COLS-1,0, 0,ROWS-1);

   set_draw_flags(FILLED);
#ifdef COLOR_LCD
   u08 ec = 9;
   #define ECOLOR color_table[ec++];
#else
   #define ECOLOR WHITE
#endif
#define EDITHER FILLED
#define EDIV 2

   if(ROWS >= 80) {
      set_color(ECOLOR);
      ellipse(COLS/EDIV,ROWS/EDIV, (10*BCOLS)/32, ASPECT);
      set_color(BLACK);
      ellipse(COLS/EDIV,ROWS/EDIV, (9*BCOLS)/32, ASPECT);
   }

   set_color(ECOLOR);
   ellipse(COLS/EDIV,ROWS/EDIV, (8*BCOLS)/32, ASPECT);
   set_color(BLACK);
   ellipse(COLS/EDIV,ROWS/EDIV, (7*BCOLS)/32, ASPECT);
   set_color(ECOLOR);
   ellipse(COLS/EDIV,ROWS/EDIV, (6*BCOLS)/32, ASPECT);
   set_color(BLACK);
   ellipse(COLS/EDIV,ROWS/EDIV, (5*BCOLS)/32, ASPECT);
   set_color(ECOLOR);
   ellipse(COLS/EDIV,ROWS/EDIV, (4*BCOLS)/32, ASPECT);
   set_color(BLACK);
   ellipse(COLS/EDIV,ROWS/EDIV, (3*BCOLS)/32, ASPECT);
   set_color(ECOLOR);
   ellipse(COLS/EDIV,ROWS/EDIV, (2*BCOLS)/32, ASPECT);
   set_color(BLACK);

   ellipse(COLS/EDIV,ROWS/EDIV, (1*BCOLS)/32, ASPECT);
//    ellipse(COLS/EDIV, ROWS/EDIV, 52, ASPECT);
   set_color(WHITE);

   set_draw_flags(FILLED | BLIT_XOR);
   set_color(WHITE);
   set_bg(BLACK);
   blit(20,20, 60,60, 20,20);
   set_draw_flags(0x00);
}


void text_test(void)
{
#define CC 0*CHAR_WIDTH

   invert_colors();
   lcd_textPS(CC,0*CHAR_HEIGHT,  "--------------------");
   lcd_textPS(CC,1*CHAR_HEIGHT,  "|     ACHTUNG!     |");
   lcd_textPS(CC,2*CHAR_HEIGHT,  "--------------------");
   invert_colors();

   lcd_textPS(CC,3*CHAR_HEIGHT,  "'Twas brillig, and  ");
   lcd_textPS(CC,4*CHAR_HEIGHT,  "the slithy toves did");
   lcd_textPS(CC,5*CHAR_HEIGHT,  "gyre and gimble in  ");
   lcd_textPS(CC,6*CHAR_HEIGHT,  "the wabe:  All mimsy");
   lcd_textPS(CC,7*CHAR_HEIGHT,  "were the borogoves, ");
   lcd_textPS(CC,8*CHAR_HEIGHT,  "and the mome raths  ");
   lcd_textPS(CC,9*CHAR_HEIGHT,  "outgrabbe.          ");
}


void font_test(void)
{
unsigned x;
unsigned row, col;

   x = 0x00;
   for(row=0; row<ROWS; row+=CHAR_HEIGHT) {
      for(col=0; col<COLS; col+=CHAR_WIDTH) {
         lcd_char(col,row, x);
         if(++x > 255) x = 0;
      }
   }
}


#ifdef UART_CODE
void demo_uarts(u08 pattern)
{
char c;

#ifdef UART0
   while(rx_haschar()) {
      c = rx_getchar();
      PP ("RECEIVED CHAR :");
      putch(c);
      print_eol();
   }

   PP ("LCD UART0 Demo #");
   print_int(pattern, 2);
   print_eol();
#endif //UART0

#ifdef UART1
   while(rx1_haschar()) {
      c = rx1_getchar();
      PP1("RECEIVED CHAR 1:");
      putch1(c);
      print1_eol();
   }

   PP1("LCD UART1 Demo #");
   print1_int(pattern, 2);
   print1_eol();
#endif //UART1
}
#endif //UART_CODE


void demo_loop(u08 repeat)
{
u08 pattern;

   lcd_clear();

// PPL ("MEGADONKEY TL     -- Prototype Version 0.1");
// PPL ("LCD INIT");

   #ifdef COLOR_LCD
      color_bars();
      color_palette();
   #endif

   while(1) {
     for(pattern=4; pattern<=16; pattern++) {
#ifndef MOUSE
#ifdef UART_CODE
        demo_uarts(pattern);
        if(0 && (com1_protocol & USE_RTS)) {  // loop this one test for flow control diagnostics
           lcd_clear();
           blc_test();
           continue;
        }
#endif //UART_CODE
#endif

        lcd_clear();

        if(pattern == 0xFF) ;
#ifndef NOKIA  // NOKIA display can't read back screen
        else if(pattern == 16) side_blit_test();
        else if(pattern == 15) blit_chars_test();
        else if(pattern == 14) paint_test();
#endif
        else if(pattern == 13) hline_test();
        else if(pattern == 12) continue;//thermo_test();
//      else if(pattern == 12) thermo_test();
        else if(pattern == 11) rotated_char_test();
        else if(pattern == 10) pattern_test();
#ifdef TIME_CLOCK
#ifdef USER_INPUT
        else if(pattern == 9)  dclock();
        else if(pattern == 8)  aclock();
#endif
#endif
        else if(pattern == 7)  shaded_circle_test();
        else if(pattern == 6)  char_size_test();
#ifdef USER_INPUT
        else if(pattern == 5)  {
           live_kbd_test();
           wait_while_touched();
           continue;
        }
#endif
        else if(pattern == 4)  blc_test();
        else if(pattern == 3)  text_test();
        else if(pattern == 2)  font_test();
        else if(pattern == 1)  fill_screen(WHITE);
        else if(pattern == 0)  fill_screen(BLACK);
        else continue;

        beep(250, pattern*200);

        delay_until_touched(2000);
        wait_while_touched();
     }

     if(repeat == 0) break;
  } // end while(1)
}

#endif // LCD_DEMO 
