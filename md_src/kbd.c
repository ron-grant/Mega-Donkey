/*  MegaDonkey Library File:  kbd.c    Keyboard for text entry / editing 
    


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



     Keyboard for text entry and editing
	 Mark Sims 
	 Mayapril 2007

*/


#include "md.h"     // typedefs and defines, e.g. defines MENU_CODE

#ifdef MENU_CODE

#include "menu.h"
#include <string.h> // strlen...
#include "adc.h"    // touch functions 
#include "timer.h"
#include <ctype.h>  // misc functions  tolower, isdigit ... 

#define XCOLS COLS
#define XROWS ROWS


//!!! make these BUTTON_SIZE friendly
#ifdef RES_128x64
   #define KEY_WIDTH  11
   #define KEY_HEIGHT 11
   #define KBD_TOP_LINE 9 
#endif

#ifdef RES_132x132
   #define KEY_WIDTH  11
   #define KEY_HEIGHT 14
   #define KBD_TOP_LINE 12 
#endif

#ifdef RES_160x80
   #define KEY_WIDTH  14
   #define KEY_HEIGHT 14
   #define KBD_TOP_LINE 10
#endif

#ifdef RES_160x128
   #define KEY_WIDTH  14
   #define KEY_HEIGHT 20
   #define KBD_TOP_LINE 13
#endif

#ifdef RES_240x128
   #define KEY_WIDTH  21
   #define KEY_HEIGHT 20
   #define KBD_TOP_LINE 13
#endif

#ifdef RES_320x240
   #define KEY_WIDTH  26
   #define KEY_HEIGHT 26
   #define KBD_TOP_LINE 13
#endif

#ifndef KEY_WIDTH
   #define KEY_WIDTH  11
   #define KEY_HEIGHT 11
   #define KBD_TOP_LINE 9
#endif

#define EDIT_NULL_CHAR 0xFE
#define EDIT_WIDTH ((XCOLS/CHAR_WIDTH)-edit_scroll)
// #define RAM_KEYS  // define this to put keys in RAM for redefineable keyboard

#ifdef RAM_KEYS
   char *kbd_row1   = "1234567890";
   char *kbd_row2   = "QWERTYUIOP";
   char *kbd_row3   = "ASDFGHJKL;";
   char *kbd_row4   = "ZXCVBNM+-=";
   char *kbd_row5   = "!@#$%^&*/.";

   #define kbd_row1_s kbd_row1
   #define kbd_row2_s kbd_row2
   char *kbd_row3_s = "ASDFGHJKL:";
   char *kbd_row4_s = "ZXCVBNM,./";
   char *kbd_row5_s = "~'\"\\<>{}[]";
#else
   #define kbd_row1   PS("1234567890")
   #define kbd_row2   PS("QWERTYUIOP")
   #define kbd_row3   PS("ASDFGHJKL;")
   #define kbd_row4   PS("ZXCVBNM+-=")
   #define kbd_row5   PS("!@#$%^&*/.")

   #define kbd_row1_s kbd_row1
   #define kbd_row2_s kbd_row2
   #define kbd_row3_s  PS("ASDFGHJKL:")
   #define kbd_row4_s  PS("ZXCVBNM,./")
   #define kbd_row5_s  PS("~'\"\\<>{}[]")
#endif

u08 kbd_shift = 0;

void kbd_key(
   COORD col, COORD row,
   char *s
)
{
COORD right;
COORD label_ofs;
COORD row_ofs;
COORD key_ofs;

   right = strlen(s);

   if(COLS <= 132) {
      if(right > 1) label_ofs = 1;
      else label_ofs = 2;

      row_ofs = 2;
      if(ROWS > 64) key_ofs = 2;
      else key_ofs = 1;
   }
   else {
      if(right > 1) label_ofs = 2;
      else label_ofs = 3;

      if(COLS > 160) {
         label_ofs += 2;
         row_ofs = 3+1;
         key_ofs = 3;
      }
      else {
         row_ofs = 3+0;
         key_ofs = 2;
      }
   }


   right = col + (right*KEY_WIDTH) - key_ofs;
   if(right >= XCOLS) right = XCOLS-1;
   lcd_text(col+label_ofs, row+row_ofs, s);
   shaded_box(col,row, right,row+KEY_HEIGHT-key_ofs);
}


void kbd_row(
   COORD col,COORD row,
   char *keys
)
{
char s[2];
u08 i;

   s[1] = 0;
   for(i=0; keys[i]; i++) {
      s[0] = keys[i];
      if(kbd_shift) s[0] = tolower(s[0]);
      kbd_key(col,row, s);
      col += KEY_WIDTH;
      if(col >= XCOLS) break;
   }
}

void show_edit_text(char *s, u08 kbd_buf_len, u08 edit_scroll, u08 edit_char, short edit_left, int edit_len)
{
u08 i;
COORD col;
unsigned char c;

   col = 0;
   if(edit_scroll) {  // line is longer than screen,  enable scroll arrows
      if(edit_scroll > 1) {  // show left and right arrows
         lcd_char(0*CHAR_WIDTH, 0*CHAR_HEIGHT, 0xAE);  //27
         col = CHAR_WIDTH;
      }
      lcd_char(XCOLS-CHAR_WIDTH, 0*CHAR_HEIGHT, 0xAF);  //26
   }

   for(i=edit_left; i<edit_left+EDIT_WIDTH; i++) {  // show the edit line
      if(i >= kbd_buf_len) c = ' ';
      else if(i >= edit_len) c = EDIT_NULL_CHAR;
      else c = s[i];

//    if((i == edit_char) || (i >= kbd_buf_len)) {
      if(i == edit_char) {
         lcd_char(col, 0*CHAR_HEIGHT, c);
      }
      else {  // invert editable chars on edit line (except the cursor char)
         invert_colors();
         lcd_char(col, 0*CHAR_HEIGHT, c);
         invert_colors();
      }
      col += CHAR_WIDTH;
   }
}

void edit_beep(u08 num)
{
   num = 1;    //!!!!
   while(num--) {
      beep(20, 400);
      delay_ms(20); 
   }
}

int menu_kbd(
   char *title,  // if 0 then start with five row keyboard
   char *buf,
   u08 kbd_buf_len  // NOT including ending \0 byte
)
{
COORD x, y;
unsigned char c;
u08 erase_flag;
u08 touch_down;
u08 edit_char;
int edit_len;
short edit_left;
u08 edit_scroll;
char tbuf[COLS/CHAR_WIDTH+1];

   // copy title to internal buf so PS macro can be used without worrying
   // about zapping the title if it was a PS string
   if(buf || (keyboard_drawn == 0)) {
      strncpy(tbuf, title, COLS/CHAR_WIDTH);
      tbuf[COLS/CHAR_WIDTH] = 0;
   }
   else title = 0;
// if(buf == 0) return (-1); // no buffer pointer passed,  use default keyboard buffer
// if(kbd_buf_len <= 1) return (-2);

   // kbd_shift = 0;

   if((buf == 0) || (kbd_buf_len <= 1)) {  // no edit buffer was passed to us
      buf = 0;         // return each keystroke,  no editing allowed
      edit_len = edit_char = edit_scroll = edit_left = 0;
      if(keyboard_drawn) goto next_key;
   }
   else {
      edit_len = strlen(buf);  // see if buffer was passed with contents to edit
      edit_char = edit_len;    // start with cursor char at end of line
      edit_scroll = edit_left = 0;

      if(edit_len >= XCOLS/CHAR_WIDTH) {
         if(edit_char == 0) edit_scroll = 1;
         else edit_scroll = 2;
      }
   }
   erase_flag = 1; // dont erase the edit buffer

   title_redraw:
   lcd_clear();

   shift_redraw:
   if(kbd_shift) kbd_row(KEY_WIDTH*0, KBD_TOP_LINE+KEY_HEIGHT*0, kbd_row1_s);
   else          kbd_row(KEY_WIDTH*0, KBD_TOP_LINE+KEY_HEIGHT*0, kbd_row1);
   if(kbd_shift) kbd_row(KEY_WIDTH*0, KBD_TOP_LINE+KEY_HEIGHT*1, kbd_row2_s);
   else          kbd_row(KEY_WIDTH*0, KBD_TOP_LINE+KEY_HEIGHT*1, kbd_row2);
   if(kbd_shift) kbd_row(KEY_WIDTH*0, KBD_TOP_LINE+KEY_HEIGHT*2, kbd_row3_s);
   else          kbd_row(KEY_WIDTH*0, KBD_TOP_LINE+KEY_HEIGHT*2, kbd_row3);
   if(kbd_shift) kbd_row(KEY_WIDTH*0, KBD_TOP_LINE+KEY_HEIGHT*3, kbd_row4_s);
   else          kbd_row(KEY_WIDTH*0, KBD_TOP_LINE+KEY_HEIGHT*3, kbd_row4);

   kbd_key(KEY_WIDTH*10, KBD_TOP_LINE+KEY_HEIGHT*0, PS("ES"));
   kbd_key(KEY_WIDTH*10, KBD_TOP_LINE+KEY_HEIGHT*1, PS("BS"));
   kbd_key(KEY_WIDTH*10, KBD_TOP_LINE+KEY_HEIGHT*2, PS("CR"));
   kbd_key(KEY_WIDTH*10, KBD_TOP_LINE+KEY_HEIGHT*3, PS("  "));

   if((title == 0) || (ROWS > 80)) { // no keyboard title given,  activate bottom row of keys
      if(kbd_shift) {
         kbd_row(KEY_WIDTH*0,  KBD_TOP_LINE+KEY_HEIGHT*4, kbd_row5_s);
         kbd_key(KEY_WIDTH*10, KBD_TOP_LINE+KEY_HEIGHT*4, PS("sh"));
      }
      else { 
         kbd_row(KEY_WIDTH*0,  KBD_TOP_LINE+KEY_HEIGHT*4, kbd_row5);
         kbd_key(KEY_WIDTH*10, KBD_TOP_LINE+KEY_HEIGHT*4, PS("SH"));
      }
   }
                                   
   erase_buf:
   if(erase_flag == 0) {  // erase the edit buffer
      if(buf) buf[0] = 0;
      edit_char = edit_len = edit_left = edit_scroll = 0;
   }
   erase_flag = 0;

   if(buf) {
      show_edit_text(buf, kbd_buf_len, edit_scroll, edit_char, edit_left, edit_len);   // draw the edit buffer in inverted video
   }

   if(title) {   // we have a keyboard title string to print on bottom line
      if(COLS <= 132) { 
         invert_colors();
         lcd_char(XCOLS-CHAR_WIDTH, XROWS-CHAR_HEIGHT, 0x19);
         invert_colors();
         lcd_text(0, XROWS-CHAR_HEIGHT, tbuf);
      }
      else {
         invert_colors();
         lcd_char(XCOLS-CHAR_WIDTH, XROWS-CHAR_HEIGHT-1, 0x19);
         invert_colors();
         lcd_text(0, XROWS-CHAR_HEIGHT-1, tbuf);
      }
   }
#ifndef USER_INPUT      // user has no way of talking to us
   if(buf) buf[0] = 0;
   return 0;
#endif

   next_key:
   dragged:
   if(buf) {
      wait_while_touched();  // wait for an untouched screen
   }
   keyboard_drawn = 1;

   touch_down = 0;
   while(1) {  // now process the keystrokes
      if(get_touch(5) == 0) {
         if(buf) continue;   // get the touch
         else return 0;      // no key pressed
      }

      if(touchY < KBD_TOP_LINE) {  // the edit (top) line was touched
         if(buf) {
            x = touchX / CHAR_WIDTH;
            if((edit_scroll > 1) && (x == 0)) {   //left scroll arrow touched
               if(--edit_left < 0) {
                  edit_left = 0;
                  edit_beep(1); 
               }
               edit_char = edit_left;
               if(edit_char == 0) {  // we are at first char in the line
                  edit_scroll = 1;   // drop left left scroll arrow
               }
            }
            else if(edit_scroll && (x >= (XCOLS/CHAR_WIDTH-1))) {  // right scroll arrow was touched 
               if(++edit_left >= (edit_len - EDIT_WIDTH)) {
                  edit_left = edit_len - EDIT_WIDTH + 1;
                  edit_beep(2); 
               }
               edit_char = edit_left + EDIT_WIDTH - 1;
               if(edit_scroll == 1) --edit_char;
               if(edit_char > edit_len) edit_char = edit_len;
               if(edit_char == 0) edit_scroll = 1;
               else edit_scroll = 2;
            }
            else {  // set edit cursor within visible part of line
               edit_char = edit_left + x;
               if(edit_scroll == 2) --edit_char;
               if(edit_char > edit_len) { // attempt to edit past eol
                  edit_char = edit_len;
                  edit_beep(3); 
               }
            }

            if(touch_down == 0) delay_ms(250);  // delay 1/4 sec then autorepeat
            else delay_ms(50);  // auto repeat at 20 cps
            touch_down = 1;

            goto show_buf;
         }
         else {  // top line touched in live keyboard mode
            continue;  //!!!!
         }
      }

      touch_down = 0;
      y = (touchY - KBD_TOP_LINE) / KEY_HEIGHT;  // convert touch coord to key coord
      x = touchX / KEY_WIDTH;

      while(1) {
         if(get_touch(5) == 0) break;
         if(x != (touchX / KEY_WIDTH)) goto dragged;
         if(y != ((touchY - KBD_TOP_LINE) / KEY_HEIGHT)) goto dragged;
      }

      c = '?';
      if(y == 0) {  // keyboard line 1
         if(x < strlen(kbd_row1)) {  // digit
            if(kbd_shift) c = kbd_row1_s[x];
            else          c = kbd_row1[x];
         }
         else {  // EScape
            if(buf) {
               if(edit_len) { // EScape a line with text is erase text
                  goto erase_buf; 
               }
               else { // EScape an empty line is abort
                  edit_len = 1;
                  buf[0] = 0x1B;
                  buf[1] = 0;
                  break;
               }
            }
            else c = 0x1B;
         }
      }
      else if(y == 1) { // keyboard line 2
         if(x < strlen(kbd_row2)) {  
            if(kbd_shift) c = kbd_row2_s[x];
            else          c = kbd_row2[x];
         }
         else {  // Back Space
            if(buf) {
               if(edit_len <= 0) {  // buffer is empty
                  edit_len = 0;
                  edit_beep(4); 
                  goto erase_buf;
               }
               else {  // delete char under cursor
                  for(x=edit_char; x<edit_len; x++) {  // shift end of buffer down one char
                     buf[x] = buf[x+1];
                  }
                  buf[--edit_len] = 0;

                  if(edit_char > edit_len) edit_char = edit_len;
                  if(edit_len >= EDIT_WIDTH) {  // we are scrolling
                     if(--edit_left < 0) edit_left = 0;   // keep text in same place on line
                  }
               }
               goto show_buf;
            }
            else c = 0x08;
         }
      }
      else if(y == 2) {  // keyboard line 3
         if(x < strlen(kbd_row3)) {
            if(kbd_shift) c = kbd_row3_s[x];
            else          c = kbd_row3[x];
         }
         else if(buf) break;  // Carriage Return
         else c = 0x0D;
      }
      else if(y == 3) {  // keyboard line 4
         if(x < strlen(kbd_row4)) {
            if(kbd_shift) c = kbd_row4_s[x];
            else          c = kbd_row4[x];
         }
         else c = ' ';
      }
      else if(y == 4) {   // title line was touched
         if(title && (ROWS <= 80)) {      // erase title and bring up the full keyboard
            title = 0;
            erase_flag = 2;   // don't erase edited text
            goto title_redraw;
         }
         else if(x < strlen(kbd_row5)) {
            if(kbd_shift) c = kbd_row5_s[x];
            else          c = kbd_row5[x]; 
         }
         else {     // sticky shift key pressed 
            kbd_shift ^= 1;    // toggle shift mode
            erase_flag = 1;    // don't erase edited text
            goto shift_redraw; // update the keyboard
         }
      }
      else continue;

      if(kbd_shift) c = tolower(c);
      if(buf == 0) return c;

      if(edit_len >= kbd_buf_len) {  // buffer is already full of ASCII goodness
         edit_beep(5); 
      }
      else {   // insert char at cursor
         if((edit_len == 0) || (edit_char >= edit_len)) {  // insert at end of line
            buf[edit_len++] = c;
            edit_char = edit_len;
         }
         else {  // inserting char in line
            for(x=edit_len; x>edit_char; x--) buf[x] = buf[x-1]; 
            buf[++edit_len] = 0;
            buf[edit_char++] = c;
         }
         if(edit_len >= EDIT_WIDTH) edit_left = edit_len-EDIT_WIDTH+1;
      }

      show_buf:
      if(edit_len >= XCOLS/CHAR_WIDTH) { // edit buffer exceeds display width
         if(edit_scroll == 0) {    // start scrolling
            if(edit_char == 0) {   // only need right scroll right now
               edit_scroll = 1;
               edit_left = 0;
            }
            else {   // enable left and right scrolling
               edit_scroll = 2;
               edit_left = 3;
            }
         }
         else if(edit_scroll == 1) edit_left = 0;  // only right scroll right now
      }
      else edit_scroll = edit_left = 0;  // edit buffer fits on screen

      show_edit_text(buf, kbd_buf_len, edit_scroll, edit_char, edit_left, edit_len);   // draw the edit buffer in inverted video
   }

   buf[edit_len] = 0;
   menu_init();
   keyboard_drawn = 0;
   return edit_len;
}

#endif // end ifdef MENU_CODE
