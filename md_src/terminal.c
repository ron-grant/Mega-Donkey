/*  MegaDonkey Library File:  terminal.c   Serial Port Interface To Drawing Functions and Ports
    


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



  serial port controlled interface to drawing functions and ports 
  20 May 2007 - Mark Sims


*/

#define EXTERN extern

#include <ctype.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h> // for PSTR macro   Program Memory Space String
#include <stdio.h>
#include "main.h"

#ifdef TERMINAL_CODE
#include "lcd.h"
#include "timer.h"
#include "adc.h"
#include "menu.h"
#include "uart.h"

// #define TERM_COM1

#ifdef TERM_COM1
#define term_println(s) println1(s)
#define term_print(s)   print1(s)
#define term_send(c)    uart1_send(c)
#define term_haschar()  rx1_haschar()
#define term_getchar()  rx1_getchar()
#else
#define term_println(s) println(s)
#define term_print(s)   print(s)
#define term_send(c)    uart_send(c)
#define term_haschar()  rx_haschar()
#define term_getchar()  rx_getchar()
#endif


#define skip_to_eol()  temp_b=c; while((c != 0x0D) && (c != 0x0A)) {temp_b=c;  c=get_term();}
#define term_delay()   if(term_wait) delay_ms(term_wait)

u08 term_echo;
u16 term_wait;
u16 term_num;



u08 get_term()
{
u08 c;

   while(term_haschar() == 0) ;
   c = term_getchar();
   if(term_echo) term_send(c);
   return c;
}


u08 get_num(u08 hex)
{
u08 c;

   term_num = 0;
   while(1) {
      c = get_term();
      if(hex) {
         if((c >= '0') && (c <= '9')) term_num = (term_num << 4) + (c - '0');
         else if((toupper(c) >= 'A') && (toupper(c) <= 'F')) term_num = (term_num << 4) + (toupper(c) - 'A' + 10);
         else return c;
      }
      else {
         if((c >= '0') && (c <= '9')) term_num = (term_num * 10) + (c - '0');
         else return c;
      }
   }
}


u08 term_string(char *s, u08 len)
{
u08 x1;
u08 c;

   if(len == 0) return 0;
// s[0] = 0;

   x1 = 0;
   while(x1 < len) {
      c = get_term();
      if(c == 0x0D) break;
      else if(c == 0x0A) break;
      s[x1++] = c;
s[x1] = 0;
   }
// s[x1] = 0;
   return c;
}


struct SLIDER_STRUCT {
   u08 t, x, y, w, show, code;
   unsigned long daddr;
   long min, max;
   char label[];
};

struct CHECKBOX_STRUCT {
   u08 t, x, y, code;
   unsigned long daddr;
   char label[];
};

struct BUTTON_STRUCT {
   u08 t, x, y, code;
   char label[];
}; 

struct LABEL_STRUCT {
   u08 t, x, y;
   char label[];
};

struct BUTTONROW_STRUCT {
   u08 t, x, y;
   char buttons[];
};

struct BUTTONCOL_STRUCT {
   u08 t, x, y;
   char buttons[];
};

#define MENU_LABEL     1
#define MENU_BUTTONROW 2
#define MENU_BUTTONCOL 3
#define MENU_BUTTON    4
#define MENU_CHECKBOX  5
#define MENU_SLIDER    6
#define MENU_VSLIDER   7
#define MENU_CALC      8
#define MENU_KBD       9

#define MENU_TEXT_LEN  (COLS/CHAR_WIDTH)


struct EED {
   struct BUTTON_STRUCT bu;
   struct BUTTON_STRUCT xu;
   struct CHECKBOX_STRUCT cb;
   u08 eof;
} xyz PROGMEM = {
   { MENU_BUTTON,   50,50,      5, "Button" },
   { MENU_BUTTON,   COLS-14,0, MENU_EXITCODE,  "X"},
   { MENU_CHECKBOX, 20,20,      3,  EE_SOUND|EEPROM_ADDR, "Beeper" },
   0
};

void stored_menu(unsigned long addr);

void terminal_menu(void)
{
unsigned long addr;
char s[COLS/CHAR_WIDTH+1];

// addr = EE_MENUS | EEPROM_ADDR;    
// eeprom_write_block((void *) &xyz, (void *) (unsigned) addr, (size_t) 22+sizeof xyz);
     addr = (unsigned long) (void *) &xyz;
     addr |= PROG_ADDR;

   MENU_INIT

   do {
      MENU_CONTROLS
      menu_label(0,0, "Terminal Menu");
      stored_menu(addr);   // draw menu from defintions stored in memory

      MENU_COMMANDS

      // menu button/control responses

      if(menu_cmd()) {
         sprintf(s, PS("N=%d"), menu_cmd());
         term_delay();
         term_println(s);
      }

   } while (menu_cmd() != MENU_EXITCODE);       // repeat until exit command
}


void lcd_term(u08 flags)
{
u08 c;
char s[COLS/CHAR_WIDTH+1];
char kbd_buf[80+1];
u08 temp_b;
u16 x1,y1;
u16 x2,y2;
u16 x3,y3;
long val;

   term_echo = flags & 0x01;
   term_wait = 0;
   s[0] = kbd_buf[0] = 0;

   lcd_clear();
   term_println(PS(""));
//   term_print(PS(">"));

   c = 0x0D;
   temp_b = c;
   while(1) {
      skip_to_eol();
      if(term_echo) {
         if(temp_b == 0x0A) term_send(0x0D);
         else if(temp_b == 0x0D) term_send(0x0A);
         term_print(PS(">"));
      }
      c = get_term();  // c = command character
      c = tolower(c);

      if(c == 0x1B)  break;  //!!!!!!

      switch(c) {
         case 0x0D:
//          if(term_echo) term_send(0x0A);
            break;
         case 0x0A:
            if(term_echo) term_send(0x0D);
            break;

         case ':': // calibrate touch
            c = get_term();
            if(c == '#') user_input_init();  // default_touch_cal();
            else calibrate_touch(1);
            break;

         case '$':  // set keyboard title string
            s[0] = 0;
            c = term_string(s, COLS/CHAR_WIDTH);
            break;

         case '#':  // number entry / calculator
            c = get_num(1);  // options,text
            kbd_buf[0] = 0;
            if((c != 0x0D) && (c != 0x0A)) {  // get initial val/title string
               if(COLS >= 160) {
                  c = term_string(kbd_buf, 16);
               }
               else {
                  c = term_string(kbd_buf, 13);
               }
            }
            val = menu_calc(kbd_buf, term_num);
            sprintf(kbd_buf,"#=%ld/%ld", val, calc_scale);
            term_delay();
            term_println(kbd_buf);
            break;

         case '!': // beep msecs,Hz
            next_beep:
            c = get_num(0);
            if(term_num == 0) x1 = 100;
            else x1 = term_num;
            if(c == ',') c = get_num(0);
            else term_num = 2400;
            beep(x1, term_num);
            if(c == ',') {
               beep_wait();
               goto next_beep;
            }
            break;

         case '*':   // test response
            next_test:
            term_delay();
            term_send('>');
            c = get_term();
            if((c == '*') || (c == ',')) goto next_test;
            break;

         case 'a':   // get ADC value
            next_adc:
            c = get_term();
            if((c >= '0') && (c <= '7')) {
               sprintf(s, PS("A%c=%d"), c, get_adc_sample(c-'0'));
               term_delay();
               term_println(s);
               c = get_term();
               if(c == ',') goto next_adc;
            }
            break;

         case 'b':   // boxes BX10,20,30,40  BS10,20,30,40,5,10,15,20
            c = get_term();
            c = toupper(c);
            if(c == 'T') {  // thick_box syntax: BT5,10,20,30,40 - 5 pixel wide box at (10,20)..(30,40)
               c = get_num(0); temp_b = term_num;
               if(c != ',') break;
            }
            else if((c != 'X') && (c != 'F') && (c != 'S')) break;
            else temp_b = c;

            next_box:
            c = get_num(0);  x1=term_num;
            if(c != ',') break;
            c = get_num(0);  y1=term_num;
            if(c != ',') break;
            c = get_num(0);  x2=term_num;
            if(c != ',') break;
            c = get_num(0);  y2=term_num;

            if(temp_b == 'X') box(x1,y1, x2,y2);
            else if(temp_b == 'F') filled_box(x1,y1, x2,y2);
            else if(temp_b == 'S') shaded_box(x1,y1, x2,y2);
            else thick_box(x1,y1, x2,y2, temp_b);
            if(c == ',') {
               x1 = x2;
               y1 = y2;
               goto next_box;
            }
            break;

         case 'c':  // color
            c = get_num(1);
            if(c == '?') {
               sprintf(s, PS("C=%02X"), color);
               term_delay();
               term_println(s);
            }
            else {
               set_color(term_num);   //!!!!! need a way to set bg_color
            }
            break;
            
         case 'd':  // dot
            next_dot:
            c = get_num(0);  x1=term_num;
            if(c != ',') break;
            c = get_num(0);  y1=term_num;

            if(c == '?') {
               x2 = get_dot(x1, y1);
               sprintf(s, PS("D%d,%d=%02X"), x1, y1, x2);
               term_delay();
               term_println(s);
               goto next_dot;
            }
            else {
               dot(x1,y1);
               if(c == ',') goto next_dot;
            }
            break;

         case 'e':  // ellipse
            c = get_num(0);  x1=term_num;
            if(c != ',') break;
            c = get_num(0);  y1=term_num;
            if(c != ',') break;

            c = get_num(0);  x2=term_num;
            if(c != ',') break;

            next_ellipse:
            c = get_num(0);  y2=term_num;

            ellipse(x1,y1, x2, y2);
            if(c == ',') {
               goto next_ellipse;
            }
            break;

         case 'f':   // drawing flags
            c = get_num(1);
            if(c == '?') {
               sprintf(s, PS("F=%02X"), draw_flags);
               term_delay();
               term_println(s);
            }
            else {
               set_draw_flags(term_num);
            }
            break;

         case 'g':  // get_touch:  G or G#
            next_touch:
            c = get_term();
            if((c >= '0') && (c <= '9')) {
               get_touch(c-'0');
               c = get_term();
            }
            else get_touch(5);
            x2 = (~0);
            sprintf(s, PS("S=%d,%d"), touchX, touchY);
            term_delay();
            term_println(s);
            if(c == ',') goto next_touch;
            else if(c == '?') goto next_touch;
            else if(toupper(c) == 'G') goto next_touch;
            break;


         case 'h':  // hline
            c = get_num(0);  y1=term_num;
            if(c != ',') break;

            next_hline:
            c = get_num(0);  x2=term_num;
            if(c != ',') break;
            c = get_num(0);  y2=term_num;

            hline(y1, x2,y2);
            if(c == ',') {
               ++y1;
               goto next_hline;
            }
            break;

         case 'i':  // initialize module
            lcd_init();
            c = get_term();
            break;

         case 'j':  // echo
            c = get_term();
            if(c == '?') {
               sprintf(s, PS("E=%d"), term_echo);
               term_delay();
               term_println(s);
            }
            else {
               c = toupper(c);
               if(c == 'X') term_echo = 0;
               else if(c == '0') term_echo = 0;
               else if(c == '1') term_echo = 1;
               else term_echo ^= 1;
            }
            break;

         case 'k': // keyboard
            c = get_num(0);  // buffer len
            if(term_num > 80) term_num = 80;
//          kbd_buf[0] = 0;
if(c == ',') kbd_buf[0] = 0;
            if((c != 0x0D) && (c != 0x0A)) {
               c = term_string(kbd_buf, term_num);   // get title
            }

            if(term_num > 1) {  // keyboard with edit buffer
               x2 = menu_kbd(s, kbd_buf, term_num);
               term_delay();
               term_print("K=");
               term_println(kbd_buf);
            }
            else {  //live keyboard
               c = 0x0D;
               if(keyboard_drawn == 0) {
                  term_delay();
                  term_print("K:");
               }
               x2 = 0;
               while(x2 == 0) {
                  x2 = menu_kbd(s, kbd_buf, term_num);
                  if(x2) { // a key has been pressed
                     term_send(x2);
                     wait_while_touched();
                     if(x2 == 0x1B) { keyboard_drawn = 0; break; }
                     if(x2 == 0x0D) { keyboard_drawn = 0; break; }
                  }
               }
            }
            break;

         case 'l':  // line
            c = get_num(0);  x1=term_num;
            if(c != ',') break;
            c = get_num(0);  y1=term_num;
            if(c != ',') break;

            next_line:
            c = get_num(0);  x2=term_num;
            if(c != ',') break;
            c = get_num(0);  y2=term_num;

            line(x1,y1, x2,y2);
            if(c == ',') {
               x1 = x2;
               y1 = y2;
               goto next_line;
            }
            break;

         case 'm':  // screen/eeprom memory  MS0,0 MS0,0=AA ME1FF ME1FF=A5,S0,0=66
            next_mem:
            c = get_term();  y2 = toupper(c);
            if(y2 == 'S') {   // read/write screen memory
               c = get_num(0);  x1=term_num;
               if(c != ',') break;
               c = get_num(0);  y1=term_num;
               if(c == '=') {
                  c = get_num(1);  x2=term_num;
                  write_screen(x1,y1, x2);
//                write_screen_byte(x1, x2);
               }
               else {
                  x2 = read_screen(x1, y1);
//                x2 = read_screen_byte(x1);
                  sprintf(s, PS("M%c%d,%d=%02X"), y2, x1, y1, x2);
                  term_delay();
                  term_println(s);
               }
               if(c == ',') goto next_mem;
            }
            else if(y2 == 'E') { // read write eeprom memory
               c = get_num(1);  x1=y1=term_num;
               if(c == '=') {
                  c = get_num(1);  x2=term_num;
                  eeprom_write_byte((uint8_t *) x1,  (uint8_t) x2);
               }
               else {
                  x2 = eeprom_read_byte((uint8_t *) x1);
                  sprintf(s, PS("M%c%03X=%02X"), y2, x1, x2);
                  term_delay();
                  term_println(s);
               }
               if(c == ',') goto next_mem;
            }
            break;

         case 'n':  // menu
            terminal_menu();
            break;

         case 'o':  // circle
            c = get_num(0);  x1=term_num;
            if(c != ',') break;
            c = get_num(0);  y1=term_num;
            if(c != ',') break;

            next_circle:
            c = get_num(0);  x2=term_num;

            circle(x1,y1, x2);
            if(c == ',') goto next_circle;
            break;

         case 'p':  // port
            next_port:
            c = get_term();   // y2 is port id: abcdefg
            y2 = toupper(c);

            c = get_term();  
            if(c == '=') { // set port: PB=hexval
               c = get_num(1);
               if(y2 == 'A')      { DDRA = 0xFF; PORTA=term_num;}
               else if(y2 == 'B') { DDRB = 0xFF; PORTB=term_num;}
               else if(y2 == 'C') { DDRC = 0xFF; PORTC=term_num;}
               else if(y2 == 'D') { DDRD = 0xFF; PORTD=term_num;}
               else if(y2 == 'E') { DDRE = 0xFF; PORTE=term_num;}
               else if(y2 == 'F') { DDRF = 0xFF; PORTF=term_num;}
               else if(y2 == 'G') { DDRG = 0xFF; PORTG=term_num;}
               else break;
               if(c == ',') goto next_port;
            }
            else if((c >= '0') && (c<='7')) {  // get/set port bit
               x2 = c - '0';
               c = get_term();
               if(c == '=') { // set port bit: PB7=1
                  c = get_term();
                  if(c == '1') {
                     if     (y2 == 'A') {DDRA |= (0x01 << x2); sbi(PORTA, x2);}
                     else if(y2 == 'B') {DDRB |= (0x01 << x2); sbi(PORTB, x2);} 
                     else if(y2 == 'C') {DDRC |= (0x01 << x2); sbi(PORTC, x2);} 
                     else if(y2 == 'D') {DDRD |= (0x01 << x2); sbi(PORTD, x2);} 
                     else if(y2 == 'E') {DDRE |= (0x01 << x2); sbi(PORTE, x2);} 
                     else if(y2 == 'F') {DDRF |= (0x01 << x2); sbi(PORTF, x2);} 
                     else if(y2 == 'G') {DDRG |= (0x01 << x2); sbi(PORTG, x2);} 
                     else break;
                     c = get_term();
                     if(c == ',') goto next_port;
                  }
                  else if(c == '0') {  // cleat port bit: PB7=0
                     if     (y2 == 'A') { DDRA |= (0x01 << x2); cbi(PORTA, x2);}
                     else if(y2 == 'B') { DDRB |= (0x01 << x2); cbi(PORTB, x2);} 
                     else if(y2 == 'C') { DDRC |= (0x01 << x2); cbi(PORTC, x2);} 
                     else if(y2 == 'D') { DDRD |= (0x01 << x2); cbi(PORTD, x2);} 
                     else if(y2 == 'E') { DDRE |= (0x01 << x2); cbi(PORTE, x2);} 
                     else if(y2 == 'F') { DDRF |= (0x01 << x2); cbi(PORTF, x2);} 
                     else if(y2 == 'G') { DDRG |= (0x01 << x2); cbi(PORTG, x2);} 
                     else break;
                     c = get_term();
                     if(c == ',') goto next_port;
                  }
               }
               else { // read port bit: PB7<cr>  or PB7,
                  if     (y2 == 'A') { DDRA &= (~(0x01 << x2)); x1 = PINA;}
                  else if(y2 == 'B') { DDRB &= (~(0x01 << x2)); x1 = PINB;} 
                  else if(y2 == 'C') { DDRC &= (~(0x01 << x2)); x1 = PINC;} 
                  else if(y2 == 'D') { DDRD &= (~(0x01 << x2)); x1 = PIND;} 
                  else if(y2 == 'E') { DDRE &= (~(0x01 << x2)); x1 = PINE;} 
                  else if(y2 == 'F') { DDRF &= (~(0x01 << x2)); x1 = PINF;} 
                  else if(y2 == 'G') { DDRG &= (~(0x01 << x2)); x1 = PING;} 
                  else break;
                  if(x1 & (0x01 << x2)) x1 = 1;
                  else x1 = 0;
                  sprintf(s, PS("P%c%d=%d"), y2, x2, x1);
                  term_delay();
                  term_println(s);
                  if(c == ',') goto next_port;
               }
            }
            else { // read port PB<cr> or PB,
               if     (y2 == 'A') { DDRA = 0x00;  x2 = PINA;}
               else if(y2 == 'B') { DDRB = 0x00;  x2 = PINB;} 
               else if(y2 == 'C') { DDRC = 0x00;  x2 = PINC;} 
               else if(y2 == 'D') { DDRD = 0x00;  x2 = PIND;} 
               else if(y2 == 'E') { DDRE = 0x00;  x2 = PINE;} 
               else if(y2 == 'F') { DDRF = 0x00;  x2 = PINF;} 
               else if(y2 == 'G') { DDRG = 0x00;  x2 = PING;} 
               else break;

               sprintf(s, PS("P%c=%02X"), y2, x2);
               term_delay();
               term_println(s);
               if(c == ',') goto next_port;
            }
            break;

         case 'q':  // paint
            c = get_num(0);  x1=term_num;
            if(c != ',') break;
            c = get_num(0);  y1=term_num;
            if(c != ',') break;

            c = get_num(0);  x2=term_num;
            paint(x1,y1, x2);
            break;


         case 'r':   // rotation flags
            c = get_num(1);
            if(c == '?') {
               sprintf(s, PS("R=%02X"), rotate);
               term_delay();
               term_println(s);
            }
            else {
               set_rotate(term_num);
            }
            break;


         case 's':  // get/set screen top line
            c = get_num(0);
            if(c == '?') {
               sprintf(s, PS("S=%d"), top_line);
               term_delay();
               term_println(s);
            }
            else {
               set_lcdtop(term_num);
            }
            break;


         case 't':   // write text
            temp_b = char_size;
            c = get_term();
            if((c > '0') && (c <= '9')) set_char_size(c - '0');
            else if(c == 's') set_char_size(1);
            else if(c == 'm') set_char_size(2);
            else if(c == 'l') set_char_size(3);
            else if(c == 'x') set_char_size(4);

            c = get_num(0);
            if(c == ',') x1 = term_num;
            else break;
            c = get_num(0);
            if(c == ',') y1 = term_num;
            else break;

            lcd_setxy(x1, y1);
            while(1) {
               c = get_term();
               if((c == 0x0D) || (c == 0x0A)) break;
               term_delay();
               printf(PS("%c"), c);
            }
            set_char_size(temp_b);
            break;

        case 'u':   // blit
            c = get_num(0);  x1=term_num;
            if(c != ',') break;
            c = get_num(0);  y1=term_num;
            if(c != ',') break;

            c = get_num(0);  x2=term_num;
            if(c != ',') break;
            c = get_num(0);  y2=term_num;
            if(c != ',') break;

            c = get_num(0);  x3=term_num;
            if(c != ',') break;
            c = get_num(0);  y3=term_num;
             
            blit(x1,y1,  x2,y2,  x3,y3);
            break;


        case 'w':
           c = get_num(0);
           term_wait = term_num;
           break;

        case 'z':   // clear screen
           lcd_clear();
           break;

      }
   }
}
#endif // TERMINAL_CODE
