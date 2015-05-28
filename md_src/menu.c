/*  MegaDonkey Library File:  menu.c   Support for simple GUI style controls 
    


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
    Keyboard by Mark Sims


    Support for simple GUI style controls
    using a code framework that minimizes storage requirements

    menu_demo() provides example usage of the menu functions.


    DonkeyWiz, Windows Application, is an interactive tool that allows you create menus then generates code
	for you to build upon. See: Mega Donkey User's Guide 

*/

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "md.h"

#ifdef MENU_CODE
#include "lcd.h"
#include "adc.h"
#include "timer.h"
#include "graph.h"
#include "uart.h"
#include "menu.h"
#include "kbd.h"   // keyboard
#include "led.h"


COLOR invert_buttons = WHITE;

#define RESPONSE_CODES 256
u08 Pressed[RESPONSE_CODES/8];   //  bitfield
                               
#define is_Pressed(x) ((Pressed[(x)>>3] & (0x01<<((x)&7))) != 0)

void set_Pressed(u08 button, u08 val)
{
// if(button >= RESPONSE_CODES) return;

   if(val) Pressed[button>>3] |= (0x01 << (button&0x07));
   else    Pressed[button>>3] &= (~(0x01 << (button&0x07)));
}

void menu_init(void)
{
u08 i;

   for(i=0; i<RESPONSE_CODES/8; i++) Pressed[i] = 0;  // clear pressed state info

   ButtonBorder = (2*BUTTON_SIZE);     // default border size can be overridden in menu code
   ButtonRowSpacing = (5*BUTTON_SIZE); // default button spacing in menu_buttonrow control def.

   // MenuBeep = 1;       RG moved to md_init  11/14/2010
   //                     also added code where if =1 normal 50/100ths duration
   //                     else MenuBeep duration 2..255/100ths duration
   MenuCmd = 0;
   MenuDraw  = 1;        // cause repaint of all controls first iteration through menu loop
}


void SoundMenuBeep()
{
   beep(50, 1000);
}




void menu_begin(void) 
{
u16 temp_top;
u08 i;

   if(MenuDraw) {
      temp_top = top_line;
      for(i=0; i<=double_buffers; i++) {
         draw_on_page(i);
         lcd_clear();
      }
      set_topline(temp_top);
   }

   MenuCmd = 0;
   get_touch(5);
   // touchX,touchY  filtered touch coordinates for menu routines..
}


void menu_button(COORD x,COORD y, char *label, u08 ResponseCode)
{
COORD nc;
COORD w,h;
u08 PressedNow;
COLOR temp_color;
COLOR temp_bg;
u08 i;
u16 temp_top;

   nc = strlen(label);   // number of chars in label   
   
   if(ResponseCode == MENU_BUTTONLABEL) {
      ResponseCode = *label; // used with buttonrow where label char = response code
      if(ResponseCode == ' ') return;   // skip blanks
   }                                      

   w = (nc * CHAR_WIDTH * BUTTON_SIZE) + (ButtonBorder*2) - 1;  // determine size of button rectangle
   h = (CHAR_HEIGHT * BUTTON_SIZE) + (ButtonBorder*2) - 1; 
   
   // does touch x,y fall within button rectangle?
   PressedNow = ((touchX>=x) && (touchX<(x+w)) && (touchY>=y) && (touchY<(y+h)));

    
   // if initial iteration through menu loop (painting menu)
   // or pressed state has changed then draw button  

   // note: ButtonPressed==ResponseCode  indicates THIS button was pressed
   // previously

   
   if(MenuDraw || (PressedNow != is_Pressed(ResponseCode))) {
      temp_color = color;
      temp_bg = bg_color;
      temp_top = top_line;

      for(i=0; i<=double_buffers; i++) {
         if(PressedNow) {set_color(temp_color); set_bg(temp_bg);}
         else           {set_color(temp_bg);  set_bg(temp_color);}

         draw_on_page(i);
         if(invert_buttons) invert_colors();
         filled_box(x,y, x+w,y+h);      // Pressed = Dark background

         invert_colors();
         lcd_text(x+ButtonBorder,y+ButtonBorder, label);   // draw label text   

         set_color(temp_color);
         set_bg(temp_bg);

         shaded_box(x,y, x+w,y+h);      // always draw a frame
      }
      set_topline(temp_top);

      set_Pressed(ResponseCode, PressedNow);
   }
 
   if(PressedNow) {
      while(1) {  // wait for touch to lift
         if(get_touch(5) == 0) break;

         PressedNow = ((touchX>=x) && (touchX<(x+w)) && (touchY>=y) && (touchY<(y+h)));
         if(!PressedNow) { // pen was dragged off of the button
            ResponseCode = 0; 
            break;
         }
      }

      MenuCmd = ResponseCode;


      if(MenuCmd && MenuBeep) if (MenuBeep==1) beep(50, 1000); else beep(MenuBeep,1000);

      // Mark's code   if(MenuCmd && MenuBeep) SoundMenuBeep();


   }
}
 

// [ ] Label
// if touch in box change state but require relase before next
// state change
 

u08 menu_checkbox_1(COORD x, COORD y, char *label, u08 data, u08 ResponseCode) 
{
COORD nc;
COORD w2;
u08 PressedNow;
COLOR temp_color;
COLOR temp_bg;
u16 temp_top;
u08 i;
       
#define CHECK_W (CHAR_WIDTH * BUTTON_SIZE)
#define CHECK_H ((CHAR_HEIGHT-1) * BUTTON_SIZE)
#define CHECK_SPACE (3*BUTTON_SIZE)   //!!! do we want to scale this

   nc = strlen(label);
   w2 = (nc * CHAR_WIDTH * BUTTON_SIZE) + CHECK_W + CHECK_SPACE;
   
   PressedNow = ((touchX>=x) && (touchX<(x+w2)) && (touchY>=y) && (touchY<(y+CHECK_H)));


   // if initial iteration through menu loop (painting menu)
   // or checkbox state has changed then draw 
   
   if(MenuDraw || PressedNow) {
      temp_color = color;
      temp_bg = bg_color;
      temp_top = top_line;

      for(i=0; i<=double_buffers; i++) {
         draw_on_page(i);
         set_color(WHITE);       //!!!!
         set_bg(BLACK);
         if(PressedNow) { // data value will be toggled on exit
            if(data) lcd_textPS(x+1,y, " ");
            else      lcd_textPS(x+1,y, "X");
         }
         else {  // drawing inital menu - use current data value
            if(data) lcd_textPS(x+1,y, "X");
            else      lcd_textPS(x+1,y, " ");
         }

         shaded_box(x,y, x+CHECK_W+1,y+CHECK_H);  // draw checkbox 

         lcd_text(x+CHECK_W+CHECK_SPACE,y, label);   // draw label text   
      }

      set_topline(temp_top);
      set_color(temp_color);
      set_bg(temp_bg);
   }
 
   // if this button was previously pressed -- clear code
   // then set it again if PressedNow

   //if(ButtonPressed==ResponseCode) ButtonPressed = 255;
    
   if(PressedNow) {
      while(1) {  // wait for touch to lift
         if(get_touch(5) == 0) break;

         PressedNow = ((touchX>=x) && (touchX<(x+w2)) && (touchY>=y) && (touchY<(y+CHECK_H)));
         if(!PressedNow) { // pen was dragged off of the button
            temp_color = color;
            temp_bg = bg_color;
            temp_top = top_line;

            for(i=0; i<=double_buffers; i++) {
               draw_on_page(i);
               set_color(WHITE);
               set_bg(BLACK);

               if(data) lcd_textPS(x+1,y, "X");
               else      lcd_textPS(x+1,y, " ");
               shaded_box(x,y, x+CHECK_W+1,y+CHECK_H);  // draw checkbox 
            }

            set_topline(temp_top);
            set_color(temp_color);
            set_bg(temp_bg);
            ResponseCode = 0; 
            break;
         }
      }

      MenuCmd = ResponseCode;
      if(MenuCmd) {
         if(data) data = 0;
         else data = 1;
         if(MenuBeep) if (MenuBeep==1) beep(50, 1000); else beep(MenuBeep,1000);
      }
   }

  return (data);

}

long menu_slider_v1(     // see header -- for menu_slider() macro def -- use that
   COORD x,COORD y, 
   COORD height, 
   char *label,
   long value,
   long min,long max,
   u08 ShowValue,
   u08 ResponseCode
)
{
u08 PressedNow;
COORD t;                 // thumb position
char *s;
char buf[COLS/CHAR_WIDTH+1];
u08 i;
u16 temp_top;
#define SLIDER_W ((CHAR_WIDTH-1) * BUTTON_SIZE)
  
   PressedNow = ((touchX>=x) && (touchX<(x+SLIDER_W)) && (touchY>=y) && (touchY<=(y+height)));

   if(MenuDraw || PressedNow)  {    
      if(value < min) value=min;   // clip value
      if(value > max) value=max;
      temp_top = top_line;

      for(i=0; i<=double_buffers; i++) {
         draw_on_page(i);
         box(x,y, x+SLIDER_W,y+height);  // draw control outline

         if(PressedNow) {   // update control value
            value = min + (((y+height-touchY) * (max-min) + (height/2)) / height);
            if(value > max) value = max;
         }

         // compute thumb position based on data value 
         // because might be drawing controls 
      
         t = y + height - (((value-min) * height + (max-min)/2) / (max-min));
         if(t > (y+height)) t = y + height;
         if(t < y) t = y;

         filled_box(x,y+height-1, x+SLIDER_W-1,t);

         invert_colors();
         filled_box(x+1,y+1,x+SLIDER_W-1,t-1);    
         invert_colors();
          
         // center lable above slider (ideally short label 1 or 2 chars)
         // print label only if not empty (t>0)
         s = label;
         if(s) t = strlen(s);
         else t = 0;
         if(t) lcd_text(x-(t-1)*(CHAR_WIDTH*BUTTON_SIZE)/2,y-(CHAR_HEIGHT*BUTTON_SIZE), label); 
           
         // can't use PS macro here because *label* could be a PS string
         if(ShowValue) {
            if(max<10)        t=0;          
            else if(max<100)  t=1;
            else if(max<1000) t=2;
            else              t=3; 

            if(max<10)          sprintf(buf, "%ld",value); 
            else if(max < 100)  sprintf(buf, "%-2ld",value);
            else if(max < 1000) sprintf(buf, "%-3ld",value);
            else                sprintf(buf, "%ld",value);
            lcd_text(x-t*(CHAR_WIDTH*BUTTON_SIZE)/2,y+height+2, buf);   //  print value under slider
         }
      }
      set_topline(temp_top);
     
      MenuCmd = ResponseCode;
  }

  return(value);
}


long menu_slider1(     // see header -- for menu_slider() macro def -- use that
   COORD x,COORD y, 
   COORD width, 
   char *label,
   long value,
   long min,long max,
   u08 ShowValue,
   u08 ResponseCode
)
{
u08 PressedNow;
COORD t;                 // thumb position
char buf[COLS/CHAR_WIDTH+1];
u08 i;
u16 temp_top;
#define SLIDER_H ((CHAR_HEIGHT-1)*BUTTON_SIZE)
  
   PressedNow = ((touchX>=x) && (touchX<(x+width)) && (touchY>=y) && (touchY<=(y+SLIDER_H)));

   if(MenuDraw || PressedNow)  {    
      if(value<min) value=min;   // clip value
      if(value>max) value=max;
      temp_top = top_line;

      for(i=0; i<=double_buffers; i++) {
         draw_on_page(i);
         box(x,y, x+width,y+SLIDER_H);  // draw control outline

         if(PressedNow) {   // update control value
            value = min + (((touchX-x) * (max-min) + (width/2)) / width);
            if(value > max) value = max;
         }

         // compute thumb position based on data value 
         // because might be drawing controls 
      
         t = x + (((value-min) * width + (max-min)/2) / (max-min));
         if(t > (x+width)) t = x + width;

         filled_box(x,y+1, t,y+SLIDER_H-1);

         invert_colors();
         filled_box(t+1,y+1, x+width-1,y+SLIDER_H-1);   
         invert_colors();
          

         // can't use PS macro here becuase *label* could be a PS string
         if(ShowValue) {  //!!!! potential to overflow buf exists here
            if(max<10)          sprintf(buf, "%s %ld",   label, value);
            else if(max < 100)  sprintf(buf, "%s %-2ld", label, value);
            else if(max < 1000) sprintf(buf, "%s %-3ld", label, value);
            else                sprintf(buf, "%s %ld",   label, value);
         }
         else                   sprintf(buf, "%s",label);
         lcd_text(x+width+2,y, buf);   //!!! 2*BUTTON_SIZE?
      }
      set_topline(temp_top);
   
      MenuCmd = ResponseCode;
  }

  return(value);
}



void init_scrollbox()
{
   ScrollTouched = 0;
   ScrollLastX = ScrollLastY = 0;
   ScrollFirstX = ScrollFirstY = 0;
   ScrollDeltaX = ScrollDeltaY = 0;
   ScrollSpeedupX = ScrollSpeedupY = 0;
}


u08 menu_scrollbox(int left,int top,  int right,int bottom,  u08 draw_box, u08 ResponseCode)
{
u16 temp_top;
u08 i;

   // This implements a basic scrollbox control.  Labeling, etc is up to the user.
   // 
   // While touching in a scrollbox,  you drag your finger.  This changes
   // the scrollbox variables:
   //   ScrollTouched:                  ResponseCode of the box currently being
   //                                   touched (or 0 if none touched)
   //   ScrollFirstX, ScrollFirstY:     initial touch position in the scrollbox
   //   ScrollLastX, ScrollLastY:       current touch position in the scrollbox
   //   ScrollDeltaX, ScrollDeltaY:     change in touch position since last call
   //                                   (use these to alter the variables that
   //                                    you want the scrollbox to control)
   //   ScrollSpeedupX, ScrollSpeedupY: distance of current touch from initial touch
   //                                   (use to scale the delta values depending
   //                                    upon how far the touch has changed from
   //                                    from the initial place.  Speedup values
   //                                    are always positive).  Typically the
   //                                    farther the touch is from the initial
   //                                    position,  the greater you want the
   //                                    controlled variable to change.
   //
   // menu_scrollboxes should be the last menu controls defined.
   // You can then define the whole screen as one big scrollbox, so that
   // touching anywhere there is not another menu control defined can be 
   // used to change the scrollbox variables.  

   if(MenuDraw && draw_box) {    // draw an outline around the scrollbox area
      temp_top = top_line;
      for(i=0; i<=double_buffers; i++) {
         draw_on_page(i);
         box(left,top, right,bottom);
      }
      set_topline(temp_top);
   }

   if(MenuCmd) {        // another menu control has been touched
      init_scrollbox();
      return 0;
   }

   if((touchX >= left) && (touchX <= right) && (touchY >= top) && (touchY <= bottom)) {  // scrollbox area touched
      if(ScrollTouched) {    // touch not lifted since last time
         ScrollDeltaX = (ScrollLastX-touchX);
         ScrollDeltaY = (ScrollLastY-touchY);

         ScrollSpeedupX = (touchX-ScrollFirstX);
         if(ScrollSpeedupX < 0) ScrollSpeedupX = 0-ScrollSpeedupX;

         ScrollSpeedupY = (touchY-ScrollFirstY);
         if(ScrollSpeedupY < 0) ScrollSpeedupY = 0-ScrollSpeedupY;
      }
      else {
         ScrollTouched = ResponseCode;    // this is the first touch in the scroll box
         ScrollFirstX = touchX;
         ScrollFirstY = touchY;
         ScrollDeltaX = ScrollDeltaY = 0;
         ScrollSpeedupX = ScrollSpeedupY = 0;
      }

      ScrollLastX = touchX;
      ScrollLastY = touchY;
      MenuCmd = ResponseCode;
   }
   else {  // scrollbox is no longer being touched
      init_scrollbox();
   }

   return ScrollTouched;
}









void menu_buttonrow(COORD x,COORD y, char *buttons)
{
char s[2];
COORD bw;

   bw = (CHAR_WIDTH * BUTTON_SIZE) + ButtonBorder + ButtonRowSpacing; 

   s[1] = 0;
   while((s[0] = *buttons++)) { // assign one char from buttons followed by s[1]=0 terminator
      menu_button(x,y, s, MENU_BUTTONLABEL); // 254 menu_button will used label char response code
      x += bw;                  // next button position
   }
}


void menu_buttoncol(COORD x,COORD y, char *buttons)
{
char s[2];
COORD bh;

   bh = (CHAR_HEIGHT * BUTTON_SIZE) + ButtonBorder + ButtonColSpacing; 

   s[1] = 0;
   while((s[0] = *buttons++)) { // assign one char from buttons followed by s[1]=0 terminator
      menu_button(x,y, s, MENU_BUTTONLABEL); // 254 menu_button will use label char response code
      y += bh;                  // next button position down
   }
}

  
void menu_call(void p(void)) 
{
   p();                  
   menu_init();  // force redraw on return to caller
}


void menu_label(COORD x,COORD y, char *label)
{
u16 temp_top;
u08 i;

   if(MenuDraw) {
      temp_top = top_line;
      for(i=0; i<=double_buffers; i++) {
         draw_on_page(i);
         lcd_text(x,y, label);
      }
      set_topline(temp_top);
   }
}


u08 menu_cmd (void)
{ 
   return (MenuCmd);  // later add any control touched
}

//
//  Process menu control definitions stored in memory
//  Menus can be stored in RAM, EEPROM, FLASH, or SCREEN memory
//

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

// parse button descriptions stored in memory at *addr*
// if(addr & EEPROM_ADDR) addr points to EEPROM,  
// if(addr & PROG_ADDR) addr points to flash memory program space,  
// else addr is in RAM.  slider and check_box value variables can only be in 
// RAM or EEPROM
void stored_menu(unsigned long addr)
{
u08 button;
COORD x, y;
char s[MENU_TEXT_LEN+1];
u08 data;
u32 daddr;
u08 code;
COORD w;
u08 show;
s32 sval, oldval;
s32 smin, smax;

   while(1) {
      button = read_byte(addr);  // button type code
      ++addr;
      if(button == 0) break;

      if((COLS >= 256) || (ROWS >= 256)) {  //!!!!!!
         x = read_word(addr);       // button coordinates
         addr+=2;
         y = read_word(addr);
         addr+=2;
      }
      else {
         x = read_byte(addr);       // button coordinates
         ++addr;
         y = read_byte(addr);
         ++addr;
      }

      if(button == MENU_LABEL) {
         addr = read_string(addr, s, MENU_TEXT_LEN);
         menu_label(x, y, s);
      }
      else if(button == MENU_BUTTONROW) {
         addr = read_string(addr, s, MENU_TEXT_LEN);
         menu_buttonrow(x, y, s);
      }
      else if(button == MENU_BUTTONCOL) {
         addr = read_string(addr, s, MENU_TEXT_LEN);
         menu_buttoncol(x, y, s);
      }
      else if(button == MENU_BUTTON) {
         code =  read_byte(addr);  // button response code
         ++addr;
         addr = read_string(addr, s, MENU_TEXT_LEN);
         menu_button(x, y, s, code);
      }
      else if(button == MENU_CHECKBOX) {
         code =  read_byte(addr);
         ++addr;
         daddr = read_dword(addr);
         addr += 4;
         addr = read_string(addr, s, MENU_TEXT_LEN);

         data = read_byte(daddr);
if(data == 'N') data = 1;
else if(data == 'F') data = 0;
button = data;
         menu_checkbox(x, y, s, data, code); // data implicit call by reference
         if(data != button) {
            write_byte(daddr, data);
         }
      }
      else if((button == MENU_SLIDER) || (button == MENU_VSLIDER)) {
          w = read_byte(addr);
          ++addr;
          show = read_byte(addr);
          ++addr;
          code = read_byte(addr);
          ++addr;

          daddr = read_dword(addr);
          addr += 4;

          smin = read_dword(addr);
          addr += 4;
          smax = read_dword(addr);
          addr += 4;

          addr = read_string(addr, s, MENU_TEXT_LEN);

          sval = read_dword(daddr);
          oldval = sval;
          if(button == MENU_SLIDER) {
             menu_slider(x, y, w, s, sval, smin, smax, show, code);
          }
          else {
             menu_vslider(x, y, w, s, sval, smin, smax, show, code);
          }
          if(sval != oldval) {
             write_dword(daddr, sval);
          }
      }
      else if(button == MENU_CALC) {  //!!!! do something with value
         sval = menu_calc(s, x);
      }
      else if(button == MENU_KBD) {  //!!!! do something with value,  use a better buffer
         menu_kbd(s, s, MENU_TEXT_LEN);
      }
   }
}

#ifdef STORED_MENU_DEMO

struct SLIDER_STRUCT {
   u08 t;
   COORD x, y;
   u08 w;       //!!!! COORD?
   u08 show, code;
   unsigned long daddr;
   long min, max;
   char label[];
};

struct CHECKBOX_STRUCT {
   u08 t;
   COORD x, y;
   u08 code;
   unsigned long daddr;
   char label[];
};

struct BUTTON_STRUCT {
   u08 t;
   COORD x, y;
   u08 code;
   char label[];
}; 

struct LABEL_STRUCT {
   u08 t;
   COORD x, y;
   char label[];
};

struct BUTTONROW_STRUCT {
   u08 t;
   COORD x, y;
   char buttons[];
};

struct BUTTONCOL_STRUCT {
   u08 t;
   COORD x, y;
   char buttons[];
};


struct EED {
   struct BUTTON_STRUCT bu;
   struct BUTTON_STRUCT xu;
   struct CHECKBOX_STRUCT cb;
   u08 eof;
} xyz PROGMEM = {
   { MENU_BUTTON,   50,50,      5, "Button" },
   { MENU_BUTTON,   XCOLS-14,0, MENU_EXITCODE,  "X"},
   { MENU_CHECKBOX, 20,20,      3,  EE_SOUND|EEPROM_ADDR, "Beeper" },
   0
};


void stored_menu_demo(void)
{
unsigned long addr;

// addr = EE_MENUS | EEPROM_ADDR;    
// eeprom_write_block((void *) &xyz, (void *) (unsigned) addr, (size_t) 22+sizeof xyz);
   addr = (unsigned long) (void *) &xyz;
   addr |= PROG_ADDR;

   MENU_INIT

   do {
      MENU_CONTROLS

      stored_menu(addr);   // draw menu from defintions stored in memory

      MENU_COMMANDS

      // menu button/control responses

      switch(menu_cmd()) {
         case 3:
         if(eeprom_read_byte((uint8_t *)EE_SOUND) == 0) eeprom_write_byte((uint8_t *) EE_SOUND,  (uint8_t) 'F');
         else if(eeprom_read_byte((uint8_t *)EE_SOUND) == 1) eeprom_write_byte((uint8_t *) EE_SOUND,  (uint8_t) 'N');
      }

   } while (menu_cmd() != MENU_EXITCODE);       // repeat until exit command
}

#endif // STORED_MENU_DEMO


#ifdef MENU_DEMO

// simple control menu -- part of Demo Menu
// 3 sliders

#define HHH  // use horizontal sliders

void menu_control(void) // Demo Menu with slider controls
{
u08 Kf = 10;  // beep freq/100Hz
u08 Kd = 100; // beep dur
u08 Ki = 40;  // led level
u08 pwm_state;

// joy_mode = 1;
   MENU_INIT
#ifdef LED_CODE
   pwm_state = LED_PWM_State;
   led_pwm(PWM_ON);
#endif

   do {
      MENU_CONTROLS

      menu_label(5,5, PS("Spkr/LED Test"));

#ifdef HHH
      menu_slider(10,20, 64,       // position x,y  control width
                  PS(" F"), Kf,   // optional label, 8 bit unsigned var
                  0, 40,1, 1);   // range min,max , show value, response code

      menu_slider(10,35, 64,   
                  PS(" D"), Kd,    
                  0,999,1, 2);   

      menu_slider(10,50, 64,    
                  PS(" I"), Ki,     
                  0,127,1, 3);  
#else
      menu_vslider(5,5, 64,    // position x,y  control width
                   PS("F"), Kf,    // optional label, 8 bit unsigned var
                   0,40,1, 1);   // range min,max , show value, response code

      menu_vslider(55,5, 64,   
                   PS("D"), Kd,    
                   0,999,1, 2);   

      menu_vslider(105,5, 64,    
                   PS("I"), Ki,     
                   0,127,1, 3);  
#endif

     menu_exitbutton();


     MENU_COMMANDS

      // here would go optional code to deal with slider changes
     switch(menu_cmd()) {
        case 1: 
        case 2: 
           beep(Kd, Kf*100); // duration, frequency
           break;
        case 3:
           led_set(0, Ki*2);   // #0 = ALL LEDs
           led_set(5, Ki*2);   // backlight
           break;
      }

   } while (menu_cmd() != MENU_EXITCODE);       // repeat until exit command

// joy_mode = 0;
#ifdef LED_CODE
   if((pwm_state & ENABLED) == 0) {  // we started with PWM off
      led_pwm(PWM_OFF);              // so disable led PWM mode
   }
#endif
}

void menu_demo(void)
{
   u08 led[5];  // led 1 to 4, 0 not used
   u08 cmd;
   u08 i;

   long calc_val;
   #define KBD_BUF_LEN 30
   char kbd_buf[KBD_BUF_LEN+1];
   unsigned long secs, last_secs;

   for (i=1;i<5;i++) {    
     led[i] = 0;                  // set LED state variable to zero
     led_set(i, LED_OFF);         // physically turn off LED
   }
 
  
// set_color(WHITE);
// set_bg(BLACK);
   secs = last_secs = 0;

   MENU_INIT

   do {

      MENU_CONTROLS

      menu_label    (0,0,         PS("Control"));  // labels outside of loop

      menu_checkbox (4,12*1,  PS("LED 1"), led[1], 1);  
      menu_checkbox (4,12*2,  PS("LED 2"), led[2], 2);
      menu_checkbox (4,12*3,  PS("LED 3"), led[3], 3);
      menu_checkbox (4,12*4,  PS("LED 4"), led[4], 4);

      if(COLS <= 132) {  // AGM Panel
      
         menu_button   (4, ROWS-12,  PS("CALC"), 7);
         menu_button   (48,ROWS-12,  PS("KBD "), 8);
         menu_button   (92,ROWS-12,  PS("CTRL"), 6);
      }
      else {  // 160x80 Touch Panel
         menu_button   (10,ROWS-14,  PS("CALC"), 7);
         menu_button   (60,ROWS-14,  PS("KBD "), 8);
         menu_button   (110,ROWS-14, PS("CTRL"), 6);
      }

      menu_exitbutton();  // close button top right

      // non-menu code
      secs = get_time_alive_secs() % 10000;
      if(secs != last_secs) {
         lcd_setxy(COLS-CHAR_WIDTH*7,0);
         printf(PS("%4lu"), secs);
         last_secs = secs;
      }


      MENU_COMMANDS

      // menu button/control responses

      cmd=menu_cmd();
	     
      switch(cmd) {

         // checkbox responses
		 // note: response code 1,2,3,4 just happens to correspond to LED#  == cmd

         case 1:
		 case 2:
		 case 3:
         case 4: if (led[cmd]) led_set(cmd,LED_ON); else led_set(cmd,LED_OFF); break;
         

         // slider change 5 -- don't do anything

         case 6: menu_call(menu_control);   // call slider control demo
                 break;

         case 7: calc_val = menu_calc(PS("Donkulator"), 0);  // call calculator
                 break;

         case 8: 
                strcpy(kbd_buf, PS("Gumby"));
                menu_kbd(PS("Who's your daddy?"), kbd_buf, KBD_BUF_LEN);
                break;
       }


   } while (menu_cmd() != MENU_EXITCODE);       // repeat until exit command

   wait_while_touched();

   lcd_clear();
   set_color(WHITE);
   set_bg(BLACK);

   lcd_textPS(0,0*BUTTON_SIZE,           "ALL YOUR BASE   ");
   lcd_textPS(0,CHAR_HEIGHT*BUTTON_SIZE, "ARE BELONG TO US");
   delay_ms(1000);    // delay 1 sec
   lcd_clear();
}
#endif  // MENU_DEMO

#endif  // MENU_CODE
