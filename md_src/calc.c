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

#include "md.h"  // global defines including  "MENU_CODE"

#ifdef MENU_CODE

#include "lcd.h"
#include "menu.h"
#include "calc.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>



#define CHANGE_SIGN   0xF1    /* the +/- key */

#ifdef RES_128x64
   #define DISPL_FMT "%13s"
   #define TITLE_FMT "%-13s"
#endif
#ifdef RES_132x132
   #define DISPL_FMT "%13s"
   #define TITLE_FMT "%-13s"
#endif

#ifdef RES_160x80
   #define DISPL_FMT "%16s"
   #define TITLE_FMT "%-16s"
#endif

#ifdef RES_240x128
   #define DISPL_FMT "%26s"
   #define TITLE_FMT "%-26s"
#endif

#ifdef RES_320x240
   #define DISPL_FMT "%26s"
   #define TITLE_FMT "%-26s"
#endif

#ifndef DISPL_FMT
   #define DISPL_FMT "%13s"
   #define TITLE_FMT "%-13s"
#endif


u08 charloc(char *s, char c)     // determine character location in string
{                                // index returned as 1,2,3..   0 = char not found
u08 i = 1;

   while(*s) {
      if(*s++ == c) {
         if((i != 1) || (c != '-')) return (i);
      }
      i++;
   }

   return (0);
}

int add_to_display(char *display, char *s)
{
int n;

   n = strlen(display) + strlen(s);
   if(n <= DISPL) strcat(display, s);
   return strlen(display);
}

int format_number(long r, char *display)
{
int sign;

   if(HexMode) {
      sprintf(display, "%lX", r/calc_scale);
   }
   else {
     sign = 0;
     display[0] = display[1] = 0;
     if(r < 0) {
        r = 0 - r;
        sign = 1;
        display[0] = '-';
     }

     if(calc_scale == 1)          sprintf(&display[sign], "%ld", r);
//   else if(calc_scale == 10)    sprintf(&display[sign], "%ld.%01ld", r/calc_scale, r%calc_scale);
     else if(calc_scale == 100)   sprintf(&display[sign], "%ld.%02ld", r/calc_scale, r%calc_scale);
//   else if(calc_scale == 1000)  sprintf(&display[sign], "%ld.%03ld", r/calc_scale, r%calc_scale);
//   else if(calc_scale == 10000) sprintf(&display[sign], "%ld.%04ld", r/calc_scale, r%calc_scale);
   }

   return strlen(display);
}

long eval(char *s)
{
long n;
char c;
int i;
long sign;
long decimal;

   n = 0;
   sign = (1);

   if(HexMode) {
      if(s[0] == '-') {
         sign = (-1);
         ++s;
      }
      sscanf(s, "%lx", &n);
   }
   else {
      decimal = 0;
      i = 0;
      while((c = s[i++])) {
//       if((c >= '0') && (c <= '9') && ((n*calc_scale) < 200000000L)) {
         if((c >= '0') && (c <= '9')) {
            n = (n * 10) + (c - '0');
            decimal *= 10;
         }
         else if(c == '.') {
            if(decimal) break;
            decimal = 1;
         }
         else if((c == '-') && (i == 1)) sign = (-1);
         else break;
      }

      if(decimal < 10) decimal = 1;
      n = ((n * calc_scale) + (decimal/2)) / decimal;
   }

   return n * sign;
}


long menu_calc(char *title, u08 options)  // Function Calculator Integer Hex / Decimal
{
u08 op;            // operator
u08 n;             // number of chars in digit buffer, saves on doing strlen querries...
u08 i;
long n1,n2,r;
unsigned char c;
char s[2];
char chain_op;
char last_was_equal;
char display[DISPL+1];
COORD del_left;

//!!!!! BUTTON_SIZE these
#ifdef RES_128x64
   #define CALC_TOP  13
   #define CALC_LEFT 0
   #define DEL_LEFT  66
   #define DEC_LEFT  (del_left+33)
   #define CALC_H    ((CHAR_HEIGHT+5)*BUTTON_SIZE)
#endif 

#ifdef RES_132x132
   #define CALC_TOP  13
   #define CALC_LEFT 0
   #define DEL_LEFT  66
   #define DEC_LEFT  (del_left+33)
   #define CALC_H    ((CHAR_HEIGHT+5)*BUTTON_SIZE)
#endif 

#ifdef RES_160x80
   #define CALC_TOP  17
   #define CALC_LEFT 5
   #define DEL_LEFT  87
   #define DEC_LEFT  (del_left+35)
   #define CALC_H    ((CHAR_HEIGHT+8)*BUTTON_SIZE)
#endif

#ifdef RES_160x128
   #define CALC_TOP  17
   #define CALC_LEFT 5
   #define DEL_LEFT  87
   #define DEC_LEFT  (del_left+35)
   #define CALC_H    ((CHAR_HEIGHT+8)*BUTTON_SIZE)
#endif

#ifdef RES_240x128
   #define CALC_TOP  24
   #define CALC_LEFT 5
   #define DEL_LEFT  126
   #define DEC_LEFT  (del_left+40)
   #define CALC_H    ((CHAR_HEIGHT+16)*BUTTON_SIZE)
#endif

#ifdef RES_320x240
   #define CALC_TOP  24
   #define CALC_LEFT 5
   #define DEL_LEFT  126
   #define DEC_LEFT  (del_left+40)
   #define CALC_H    ((CHAR_HEIGHT+16)*BUTTON_SIZE)
#endif

#define XCOLS COLS
#define XROWS ROWS

char tbuf[COLS/CHAR_WIDTH+1];
char buf[COLS/CHAR_WIDTH+1];

   // copy title to internal buf so PS macro can be used without worrying
   // about zapping the title if it was a PS string
   strncpy(tbuf, title, COLS/CHAR_WIDTH);
   tbuf[COLS/CHAR_WIDTH] = 0;

   if(options & FIX_MODE) calc_scale = 1;
   else if(options & FRAC_MODE) calc_scale = SCALE;
   else calc_scale = SCALE;

   if(options & HEX_MODE) {
      HexMode = 1;
      calc_scale = 1;
   }
   else HexMode = 0;

   display[0] = 0;
   if(options & EVAL_TITLE) {
      n = add_to_display(display, tbuf);
      tbuf[0] = 0;
      n1 = eval(display);
      n = format_number(n1, display);
      last_was_equal = 1;
   }
   else {
      n = 0;
      last_was_equal = 0;
   }

   n1 = n2 = r = 0;
   op = chain_op = 0;


   MENU_INIT

   do {
      MENU_CONTROLS

#ifdef RES_128x64
      del_left = DEL_LEFT;
      ButtonRowSpacing = 5;
      if(options & NO_MATH) {  // tweak spacing when math keys are not used
         ButtonRowSpacing += 4;
      }
#endif
#ifdef RES_132x132
      del_left = DEL_LEFT;
      ButtonRowSpacing = 5;
      if(options & NO_MATH) {  // tweak spacing when math keys are not used
         ButtonRowSpacing += 4;
      }
#endif
#ifdef RES_160x80
      del_left = DEL_LEFT;
      ButtonRowSpacing = 9;  // optional button spacing,  5 is the default
      if(options & NO_MATH) {  // tweak spacing when math keys are not used
         ButtonRowSpacing += 4;
         del_left += 5;
      }
#endif
#ifdef RES_160x128
      del_left = DEL_LEFT;
      ButtonRowSpacing = 9;  // optional button spacing,  5 is the default
      if(options & NO_MATH) {  // tweak spacing when math keys are not used
         ButtonRowSpacing += 4;
         del_left += 5;
      }
#endif
#ifdef RES_240x128
      del_left = DEL_LEFT;
      ButtonBorder = 4;
      ButtonRowSpacing = 12;  // optional button spacing,  5 is the default
      if(options & NO_MATH) {  // tweak spacing when math keys are not used
         ButtonRowSpacing += 4;
         del_left += 5;
      }
#endif
#ifdef RES_320x240
      del_left = DEL_LEFT;
      ButtonBorder = 4;
      ButtonRowSpacing = 12;  // optional button spacing,  5 is the default
      if(options & NO_MATH) {  // tweak spacing when math keys are not used
         ButtonRowSpacing += 4;
         del_left += 5;
      }
#endif

      if(HexMode) {
         if(options & NO_MATH) {
            menu_buttonrow(CALC_LEFT,CALC_TOP+CALC_H*0,  PS("789 ABC"));
            menu_buttonrow(CALC_LEFT,CALC_TOP+CALC_H*1,  PS("456 DEF"));
            menu_buttonrow(CALC_LEFT,CALC_TOP+CALC_H*2,  PS("123"));
            menu_buttonrow(CALC_LEFT,CALC_TOP+CALC_H*3,  PS("0 ="));
         }
         else {
            menu_buttonrow(CALC_LEFT,CALC_TOP+CALC_H*0,  PS("789- ABC"));
            menu_buttonrow(CALC_LEFT,CALC_TOP+CALC_H*1,  PS("456+ DEF"));
            menu_buttonrow(CALC_LEFT,CALC_TOP+CALC_H*2,  PS("123x"));
            menu_buttonrow(CALC_LEFT,CALC_TOP+CALC_H*3,  PS("0 =/"));
         }
         if((options & NO_BASE_CHANGE) == 0) { // number base can change
            menu_button(DEC_LEFT,CALC_TOP+CALC_H*2, PS("DEC"), 1);
         }
      }
      else {
         if(options & NO_MATH) {
            menu_buttonrow(CALC_LEFT,CALC_TOP+CALC_H*0,  PS("789 "));
            menu_buttonrow(CALC_LEFT,CALC_TOP+CALC_H*1,  PS("456 "));
            menu_buttonrow(CALC_LEFT,CALC_TOP+CALC_H*2,  PS("123 "));
            menu_buttonrow(CALC_LEFT,CALC_TOP+CALC_H*3,  PS("0.= "));
         }
         else {
            menu_buttonrow(CALC_LEFT,CALC_TOP+CALC_H*0,  PS("789-"));
            menu_buttonrow(CALC_LEFT,CALC_TOP+CALC_H*1,  PS("456+"));
            menu_buttonrow(CALC_LEFT,CALC_TOP+CALC_H*2,  PS("123x"));
            menu_buttonrow(CALC_LEFT,CALC_TOP+CALC_H*3,  PS("0.=/"));
         }
         if((options & NO_DP_CHANGE) == 0) { // decimal point mode can change
            if(calc_scale == 1) menu_button(DEC_LEFT,CALC_TOP+CALC_H*1, PS(".00"), 5);
            else                menu_button(DEC_LEFT,CALC_TOP+CALC_H*1, PS("FIX"), 5);
         }
         if((options & NO_BASE_CHANGE) == 0) { // number base can change
            menu_button(DEC_LEFT,CALC_TOP+CALC_H*2, PS("HEX"), 1);
         }
      }
      menu_button(del_left,CALC_TOP+CALC_H*2, PS(" \xF1 "),  2);
      menu_button(del_left,CALC_TOP+CALC_H*3, PS("DEL"),     4);
      menu_button(DEC_LEFT,CALC_TOP+CALC_H*3, PS("CLR"),     3);

      menu_exitbutton();


      if(MenuDraw) {   // draw display window box
         if(display[0]) sprintf(buf, PS(DISPL_FMT), display);
         else           sprintf(buf, PS(TITLE_FMT), tbuf);

         if(COLS <= 132) {
            shaded_box (0,0, XCOLS-(CHAR_WIDTH*2*BUTTON_SIZE),(CHAR_HEIGHT+3)*BUTTON_SIZE);
            lcd_text(5,2, buf);  // display digits, if any
         }
         else {
            shaded_box (0,0, XCOLS-(CHAR_WIDTH*3*BUTTON_SIZE),(CHAR_HEIGHT+5)*BUTTON_SIZE);
            lcd_text(5,3, buf);  // display digits, if any
         }
      }


      MENU_COMMANDS

      c  = menu_cmd();  // shorthand button pressed

      s[0] = c;  // string version of button
      s[1] = 0;

      if(((HexMode && ((c>='A') && (c<='F'))) || ((c>='0') && (c<='9')) || (c == '.'))
                   && (strlen(display)<DISPL)) {
         if(last_was_equal) { // a digit after an equal sign starts a new calculation
            display[0] = 0;
            n = 0;
            last_was_equal = 0;
         }

         n = add_to_display(display, s);
      }
      else switch(c) {
         case 1:   // HEX/DEC Key
            last_was_equal = 0;
            if(op) break;

            n1 = eval(display);
            HexMode ^= 1;
            if(HexMode) { n1 /= calc_scale;   calc_scale = 1; }
            else { calc_scale = 1;   n1 *= calc_scale; }

            re_format:
            if(n) {
               display[0] = 0;   // clr string
               n = format_number(n1, display);
            }
            last_was_equal = 1;
            MenuDraw = 1;
            break;


         case 2:
             c = CHANGE_SIGN;
             goto change_sign;
             break;


         case 3:  // clear
            last_was_equal = 0;
            n = 0;
            if(display[0]) display[0] = 0;  // clear display
            else MenuDraw = 1;  // clearing blank display brings up title
            break;


         case 4:
            last_was_equal = 0;
            backup:
            if(n) {   // backspace
               n--;
               if(op && (display[n] == op)) op = 0; // clear op if erasing
               display[n] = 0;                // shorten by 1
            }
            break;


         case 5:     // decimal point control
            n1 = eval(display);
            if(calc_scale == 1) { calc_scale = 100;   n1 *= calc_scale; }
            else { n1 /= calc_scale;   calc_scale = 1; }
            goto re_format;


         case '-' :
         case '+' :
         case 'x' :
         case '/' :
            last_was_equal = 0;
            if(!op) {   // no operator present
               if(n) {  // numbers are in the string
                  if(display[n-1] == '-') {  // minus minus is plus
                     goto backup;
                  }
                  else {
                     n = add_to_display(display, s);  // add operator to buffer
                     op = c;
                  }
               }
               else {  // no numbers are in the string
                  if(c == '-') {  // operator is a '-'
                     n = add_to_display(display, s);  // add to buffer as minus sign
                  }
               }
            }
            else {  // string has operators in it
               if(c == '-') {
                  if(n && (display[n-1] == '-')) goto backup;
                  else if(n && isdigit(display[n-1])) {
                     chain_op = c;
                     goto chained;
                  }
                  n = add_to_display(display, s);  // add to buffer as minus sign
               }
               else {
                  chain_op = c;
                  goto chained;
               }
            }
            break;


         case CHANGE_SIGN:   // change sign
         case '=' : // parse display string
                    // Number operator Number
                    // perform operation and place result in display
            if(options & NO_MATH) goto calc_exit;
            change_sign:
            if(op) last_was_equal = op;
            else {  // no +-*/ operand
               last_was_equal = 1;
               if(c != CHANGE_SIGN) break;  // !!!!! continue;  // and it's not change sign
               r = eval(display);
               goto show_it;
            }
            chained:
            i = charloc(display, op);   // location of operator

            // extract operands from input string
            if(HexMode) {
               n1 = eval(display);
               if(i < strlen(display)) n2 = eval(&display[i]);
               else n2 = n1;
            }
            else {
               n1 = eval(display);
               if(i < strlen(display)) n2 = eval(&display[i]);
               else if(op == '/') { n2 = n1;  n1 = calc_scale;}  //  /= is 1/x
               else if(op == '-') { n2 = n1;  n1 = 0;}      //  /- is 0-x
               else n2 = n1;
            }


            switch(op) {  // perform the calculation
               case 'x' : r = (n1 * n2) / calc_scale; break;
               case '+' : r = n1 + n2; break;
               case '-' : r = n1 - n2; break;
               case '/' :
                  if(n2) r = (n1 * calc_scale) / n2;
                  else r = 0;
                  break;
            }

            show_it:
            if(c == CHANGE_SIGN) r = 0 - r;
            format_number(r, display);

            op = s[0] = chain_op;
            n = add_to_display(display, s);  // add op to buffer
            chain_op = 0;

            break;
      }

      if(c) { // update display
         sprintf(buf, DISPL_FMT, display);
         if(COLS <= 132) {
            lcd_text(5,2, buf);  // display digits, if any
         }
         else {
            lcd_text(5,3, buf);  // display digits, if any
         }
      }

   } while(menu_cmd() != MENU_EXITCODE);

   calc_exit:
   menu_init();
   return eval(display);
}

#endif
