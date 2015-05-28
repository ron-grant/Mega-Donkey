/*  MegaDonkey Library File:  md_term.c   Megadonkey Terminal - Link to PC DonkeyTerm  (Part of DonkeyProg)
    


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



    Ron Grant 2007


    2/8/2013 --   added  if(!mdtHooked) return  to set_topline()   
	              during button press causing characters to be sent to serial port.
 
 
*/ 



#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/io.h>
#include "md.h"

#ifdef MDT_CODE        // code enable switch -- must come after #include main.h


#include "lcd.h"       // LCD support
#include "timer.h"     // Timer routines -- for pacing PID update rates...)
#include "adc.h"       // Analog interface (for Accelerometer and Gyro Inputs)
#include "menu.h"      // Menu routines provide simple GUI
#include <math.h>      // floating point / trig support 
#include "uart.h"      // low level serial support
#include "led.h"       // LED support
#include "graph.h"     // 2D graphics support -- also map demo
#include "vchar.h"     // vector characters used by map
#include "md_term.h"   

// Commands Decoded by MD Term

#define MDT_HOOKLCD      01

#define MDT_ROTATION     10
#define MDT_DRAWFLAGS    11
#define MDT_CHARSIZE     12
#define MDT_SETCOLOR     13 
#define MDT_SETBGCOLOR   14

#define MDT_SETXY        20 
#define MDT_CHARXY       21

#define MDT_DOT          30
#define MDT_LINE         31
#define MDT_HLINE        32
#define MDT_BOX          33
#define MDT_FILLEDBOX    34
#define MDT_SHADEDBOX    35
#define MDT_ARC          36
#define MDT_ELLIPSE      37
#define MDT_BEZCURVE     38
#define MDT_BLIT         39

#define MDT_PAINT        48


#define MDT_TOPLINE      40
#define MDT_DISPLAYTOP   41
#define MDT_CLEAR        42
#define MDT_VPCLEAR      43

#define MDT_READSCREEN   44
#define MDT_WRITESCREEN  45
#define MDT_SCROLL       46

// Windows Drawing Functions 

#define MDT_TEXTCOLOR24  50
#define MDT_TEXTCOLOR6   51

#define MDT_PENCOLOR24   52
#define MDT_PENCOLOR6    53

#define MDT_FILLCOLOR24  54
#define MDT_FILLCOLOR6   55

#define MDT_BGCOLOR24    56
#define MDT_BGCOLOR6     57

#define MDT_LINEWIDTH    60
#define MDT_DOTSIZE      61
#define MDT_FILLMODE     62

#define MDT_TEXTSIZE     63
#define MDT_TEXTFONT     64

#define MDT_MOVETO       70
#define MDT_LINETO       71
#define MDT_ELLIPSERECT  72



#define mdt_send(b)   ((mdtPort) ? uart1_send(b): uart0_send(b))
#define mdt_getbyte() ((mdtPort) ? rx1_getchar(): rx0_getchar())

#define MB(b)     mdt_send(b);
#define MW(w)     mdt_send((w) & 0xFF); mdt_send((w) >>8);
#define MCMD(cmd) mdt_send(0xFF); mdt_send(cmd);


u08 mdtPort;        // port 0 or 1
u08 remote_color;   // keep track of PC's remote color
u08 remote_bgcolor; // to limit how often color values sent

volatile u08 mdt_read_ready;
volatile u08 mdt_screen_byte;


void mdt_init(u08 PortNum)
{
   // if PortNum is not 0 or 1,  assume mdtPort is setup and initialized

   if(PortNum == 0) {
      mdtPort = PortNum;
      uart0_init(MDT_BAUD_RATE, 8,'N',1, 0); // USE_CTS|USE_RTS);  // baud bits parity stop protocol
   }
   else if(PortNum == 1) {
      mdtPort = PortNum;
      uart1_init(MDT_BAUD_RATE, 8,'N',1, 0); // USE_CTS|USE_RTS);  // baud bits parity stop protocol
   }

   //sends ROWS,COLS,CHAR_HEIGHT,CHAR_WIDTH so PC can scale
   // graphics to window
   // also sets MDTHooked which sends many of the LCD graphics
   // commands (line,box,dot...) to remote PC terminal

   MCMD(MDT_HOOKLCD)
   MW(ROWS)
   MW(COLS)
   MB(CHAR_HEIGHT)
   MB(CHAR_WIDTH)
   MB(0)
   MB(0) 

   mdtHooked = 1;
   mdtMouseEnabled = 1;

   mdt_forced_setcolor();  // send color and color_bg to MD Term
}


/* Functions used for LCD Emulator and also in some cases Windows Canvas
   Drawing.

 LCD Emulator   -- MD Term simulates LCD panel at LCD panel resolution
 Windows Canvas -- Mode where drawing is supported by drawing on Windows
                   24bpp (32bpp) Canvas at PC resolution
                                   
*/


void mdt_set_rotate(u08 x)  // LCD E. (LCD Emulator)
{
   rotate = x;

   if(!mdtHooked) return;
   MCMD(MDT_ROTATION)
   MB(x)
}

void mdt_set_draw_flags(u08 x) // LCD E.
{
   draw_flags = x;

   if(!mdtHooked) return;
   MCMD(MDT_DRAWFLAGS)
   MB(x)
}


void mdt_set_char_size(u08 x) // LCD E.
{
   char_size = x;

   if(!mdtHooked) return;
   MCMD(MDT_CHARSIZE)
   MB(x)
}


void mdt_set_color(u08 x)  // LCD E.
{
   color = x;    
   
   if(mdtHooked) {
      if(color == remote_color) return;
  
      MCMD(MDT_SETCOLOR)
      MB(color)
      
      remote_color = color;
   }
}


void mdt_set_bg_color(u08 x) // LCD E.
{
   bg_color = x;
        
   if(mdtHooked) {
      if(bg_color == remote_bgcolor) return;
  
      MCMD(MDT_SETBGCOLOR)  
      MB(bg_color)

      remote_bgcolor = bg_color;
   }
}


void mdt_forced_setcolor(void) // LCD E.
{
   remote_color = color + 1;
   set_color(color);

   remote_bgcolor = bg_color + 1;
   set_bg(bg_color);
}


void mdt_setxy(int x,int y)  // LCD E.
{
   MCMD(MDT_SETXY)
   MW(x)
   MW(y)
}


void mdt_charxy(char c, int x, int y)  // LCD E.
{
   MCMD(MDT_CHARXY)
   MB(c)
   MW(x)
   MW(y)
}


void mdt_dot(int x, int y)  // LCD E. and Windows Canvas
{
   MCMD(MDT_DOT)
   MW(x)
   MW(y)
}

void mdt_line(int x1,int y1, int x2,int y2)  // LCD E. and Windows Canvas
{
   MCMD(MDT_LINE)
   MW(x1)
   MW(y1)
   MW(x2)
   MW(y2)
} 

void mdt_hline(int x1, int x2, int y) // LCD E. and Windows Canvas
{
   MCMD(MDT_HLINE)
   MW(x1)
   MW(x2)
   MW(y)
} 

void mdt_box(int x1,int y1, int x2,int y2) // LCD E. and Windows Canvas
{ 
   MCMD(MDT_BOX)
   MW(x1)
   MW(y1)
   MW(x2)
   MW(y2)
}

void mdt_filled_box(int x1,int y1, int x2,int y2) // LCD E. and Windows Canvas
{
   MCMD(MDT_FILLEDBOX)
   MW(x1)
   MW(y1)
   MW(x2)
   MW(y2)
}

void mdt_shaded_box(int x1,int y1, int x2,int y2) // LCD E. and Windows Canvas
{
   MCMD(MDT_SHADEDBOX)
   MW(x1)
   MW(y1)
   MW(x2)
   MW(y2)
}



void mdt_arc(int x,int y,int r, int a1, int a2, int thickness)  // LCD E.
{
   MCMD(MDT_ARC)
   MW(x)
   MW(y)
   MW(r)
   MW(a1)
   MW(a2)
   MB(thickness)
}

void mdt_ellipse(int x,int y, int major, int aspect)  // LCD E.
{ 
   MCMD(MDT_ELLIPSE)
   MW(x)
   MW(y)
   MW(major)
   MW(aspect)
}


// bezier curve   !!! may not be supported yet

void mdt_bezcurve(int x1,int y1, int x2,int y2,
                  int x3,int y3, int x4,int y4,
                  int steps) 
{
   MCMD(MDT_BEZCURVE);
   MW(x1)
   MW(y1)
   MW(x2)
   MW(y2)
   MW(x3)
   MW(y3)
   MW(x4)
   MW(y4)
   MW(steps)
}


void mdt_blit(int left,int top, int right,int bot,  int dest_left,int dest_top)
{
   // LCD E. support
   MCMD(MDT_BLIT)
   MW(left)
   MW(top)
   MW(right)
   MW(bot)
   MW(dest_left)
   MW(dest_top)
}


int  mdt_paint(COORD xi, COORD yi, COLOR bound)
{
  MCMD(MDT_PAINT)
  MW(xi)
  MW(yi)
  MB(bound)
  return 1;
}




 

void mdt_set_topline(int x) // LCD E.   byte offset into display memory for 
{                           // current drawing "virtual screen" top
   top_line = x; 
   
   if(!mdtHooked) return;   // 2/8/2013  RG added this statment.  Yikes! 
          
   MCMD(MDT_TOPLINE)
   MW(x)
}

void mdt_set_lcdtop(unsigned row)
{
   // called from function that sets hardware lcd top display line
   // 2/8/2013  was missing !mdtHooked, yikes!

   if(!mdtHooked) return;   // 2/8/2013  

   MCMD(MDT_DISPLAYTOP)
   MW(row)
}
  


void mdt_clear(u08 color)   // LCD E. and Windows Canvas 
{                           // Windows ignores this color and uses 
   MCMD(MDT_CLEAR)           // preset BGColor
   MB(color)
} 

void mdt_viewport_clear(void)  
{
   MCMD(MDT_VPCLEAR)
   // no params -- prepare PC for filled box that should follow
   // can be useful in determining when PC updates screen bitmap
}


// read_screen / write_screen
// assumed usage: no local LCD panel

#define MMLEN 5    // mouse message length in bytes
#define MMPS ((MDT_BAUD_RATE/10) / MMLEN)   // mouse messages per second
#define MMDLY (((5*1000L + MMPS-1) / MMPS) + 1)  // allow around 5 message times before timing out

COLOR mdt_read_screen(unsigned addr)  // LCD E.
{
unsigned long t;

   mdt_screen_byte = 0xFF;
   mdt_read_ready = 0;

   MCMD(MDT_READSCREEN);  // ask mdt for screen byte
   MW(addr);              // which will be returned in the mouse data stream

   t = get_msecs_alive();
   if((t + MMDLY) < t) {  // timealive is about to wrap
      reset_time_alive();
      t = MMDLY;
   }
   else t += MMDLY;

   while(1) {  // wait for mdt mouse handler to respond with byte
      if(mdt_read_ready) break;         // we have the screen byte
      if(get_msecs_alive() > t) break;  // too much time has passed with nary a byte in sight
   }

   return mdt_screen_byte;
}

void mdt_write_screen(unsigned addr, COLOR val)  // LCD E.
{
   MCMD(MDT_WRITESCREEN);
   MW(addr);
   MB(val);
}


void mdt_scroll(int rows)  
{
   MCMD(MDT_SCROLL)
   MW(rows)
}



#ifdef MDT_WINDOWS_CODE

// Functions used when drawing on Windows 32 bpp canvas and not using 
// LCD emulation

void mdt_textcolor24(u08 r, u08 g, u08 b)  // windows canvas 
{ 
   MCMD(MDT_TEXTCOLOR24)
   MB(r)
   MB(g)
   MB(b)
}

void mdt_textcolor(u08 b)  // windows canvas 
{ 
   MCMD(MDT_TEXTCOLOR6)
   MB(b)
}


void mdt_pencolor24(u08 r, u08 g, u08 b)  // windows canvas 
{ 
   MCMD(MDT_PENCOLOR24)
   MB(r)
   MB(g)
   MB(b)
}

void mdt_pencolor(u08 b) // windows canvas 
{ 
   MCMD(MDT_PENCOLOR6)
   MB(b)
}


void mdt_fillcolor24(u08 r, u08 g, u08 b)   // windows canvas 
{ 
   MCMD(MDT_FILLCOLOR24)
   MB(r)
   MB(g)
   MB(b)
}

void mdt_fillcolor(u08 b) // windows canvas 
{ 
   MCMD(MDT_FILLCOLOR6)
   MB(b)
}


void mdt_bgcolor24(u08 r, u08 g, u08 b)  // windows canvas
{ 
   MCMD(MDT_BGCOLOR24)
   MB(r)
   MB(g)
   MB(b)
}

void mdt_bgcolor(u08 b)  // windows canvas
{ 
   MCMD(MDT_BGCOLOR6)
   MB(b)
}


void mdt_linewidth(u08 width)  // windows canvas
{
   MCMD(MDT_LINEWIDTH)
   MB(width)
}

void mdt_dotsize (u08 size) // windows canvas
{  
   MCMD(MDT_DOTSIZE)
   MB(size)
}

// used for defining if rectangles and ellipses should be filled
// 0 = no fill
// 1 = fill 

void mdt_fillmode(u08 mode)  // windows canvas
{
   MCMD(MDT_FILLMODE)
   MB(mode)
}

void mdt_textsize(u08 size)  // windows canvas
{ 
   MCMD(MDT_TEXTSIZE)
   MB(size)
}

void mdt_font(char *s)  // select font name,  windows canvas
{ 
   MCMD(MDT_TEXTFONT)
   if(mdtPort == 0) { print(s);  print("$");  }
   if(mdtPort == 1) { print1(s); print1("$"); }
}


void mdt_moveto(int x1,int y1)  // windows canvas only
{
   MCMD(MDT_MOVETO)
   MW(x1)
   MW(y1)
}


void mdt_lineto(int x1,int y1)  // windows canvas only
{
   MCMD(MDT_LINETO)
   MW(x1)
   MW(y1)
}


// ellipse_rect, this version not supported by 
// LCD.c used for non-hooked graphics 
// i.e. drawing on Windows 32 bit per pixel canvas and not using
// LCD emulation

void mdt_ellipse_rect(int x1,int y1, int x2,int y2)
{ 
   MCMD(MDT_ELLIPSERECT) // ellipse defined by rectangular extents
   MW(x1)
   MW(y1)
   MW(x2)
   MW(y2)
}

#endif // end ifdef MDT_WINDOWS_CODE


/* 
    mdt_mouse_update():   
    Processes the serial port data stream comming from the PC to the MDT. 
    Mainly used to get PC mouse movement/click data.

    mdt_mouse_update() called from 10Khz timer tick
 

    Message packet from PC Mega Donkey Terminal:

    frame start is only byte in message stream with msb set
    (to allow synchronization)
 
    10LR 0000  L = left button down  R= right button down
               if low 4 bits = 0000 then mouse message

    0XXX 0000  X high 3 bits in D4..D7
    0XXX XXXX  X low  7 bits in D0..D6 

    0YYY 0000  X high 4 bits in D4..D7
    0YYY YYYY  X low  7 bits in D0..D6 

    for screen readback:
    0H00 0000  high bit of screen byte
    0LLL LLLL  low seven bits of screen byte
    0000 0000  ignored
    0000 0000
     
    note: X and Y are ABSOLUTE COORDINATES -- not relative like serial mouse protocol

    example user code to used read mouse data

      if(mdtMouseEvent) {
         if(mdtMouseLeft) { do if button pressed code }
         else             { do remote mouse moved to mdtMouseX,mdtMouseY code }

         mdtMouseEventClear(); // allow update routine to get more events
      }

*/


void mdt_mouse_update(void)  // set mdtMouseState if new mouse message
{
u08 b;
u16 w;

// For now commented out code that attempts to wait until Donkey processes 
// mouse pressed event before reading more mouse messages from PC
// the reasoning: we can miss mouse move events but not mouse press events
// because mouse positioning is ABSOLUTE 

  // if a mouse event with button press has been logged and not used
  // then skip this tick.   !!!concerns about uart queue filling?

  // if(mdtMouseState & (MDT_MOUSEL | MDT_MOUSER)) return;


   // Make sure we have at least enough bytes waiting (5 or more)
   // so that we will be able to process an entire message at once.
   //
   // We also process only one message per tick so that we don't
   // hog all the timer tick time.
   if(mdtPort == 0) {
      if(Rx0Count < 5) return;
      b = rx0_getchar(); 
   }
   else {
      if(Rx1Count < 5) return;
      b = rx1_getchar();
   }

   if((b & 0x8F) == 0x80) {  // we are at start of mouse data frame
      mdtMouseState = MDT_MOUSE_MSG;  // flag that we have a mouse message
      if(bit_is_set(b,5)) mdtMouseState |= MDT_MOUSEL;  // set left button flag
      if(bit_is_set(b,4)) mdtMouseState |= MDT_MOUSER;  // set right button flag

      // get MouseX MouseY from final 4 bytes
      w = mdt_getbyte();
      mdtMouseX = (w << 3) + mdt_getbyte();
      w = mdt_getbyte();
      mdtMouseY = (w << 3) + mdt_getbyte();  
   }
   else if((b & 0x8F) == 0x81) {  // read screen byte frame

      // get screen byte final 4 bytes             
          // byte1 contains most significant 7 bits of message with leading zero (Z)
          // byte2 is all zeros except LSB contains LSB of message
           
      w = mdt_getbyte();                           // Z765 4321 ZZZZ ZZZ0
      mdt_screen_byte = (w << 1) + mdt_getbyte();

      // PC does send byte+1 from LCD buffer (as next two bytes)
	  // not decoded here 

      mdt_getbyte();
      mdt_getbyte();  
      mdt_read_ready = 1;
   }
   
   

   // future:  decode lsbs of 'b' for other message types here
}


// ---------------------------------------------------------------------------------

#ifdef MDT_DEMO


/* Mega Donkey Terminal Demo
   Requires serial link to PC running Mega Donkey Terminal application
   (Part of Mega Donkey Prog)


*/

void side_blit_test(void);  // forward declare (demolcd.c)

void mdt_demo(void) 
{  
   
   lcd_set_stdout(); // select LCD for printf
   lcd_clear();

   mdt_init(MDT_COM_PORT);  // inform mdt_ functions using donkey port0

   // mdtHooked by default

   lcd_clear();

   //side_blit_test();

   lcd_clear();
   box (0,0,COLS-1,ROWS-1);
   lcd_setxy(3,3);
   printf(PS("Mega Donkey\nTerminal Demo\n"));  // display on LCD
  
 
   line(40,40,50,40);
   line(50,40,45,30);
   line(45,30,40,40);

   #define WT wait_until_touched();wait_while_touched();

#ifdef COOTIES
   WT

   mdtHooked=0;
   lcd_clear(); // LOCAL Screen Clear -- Leave Remote Screen Intact
   printf ("local screen cleared -- now touch screen again for read back from remote MD Term");
   mdtHooked=1;

   WT
   
   int addr;

   // read back 
   // could use 2nd byte available in message -- not done yet
   // that is, [addr] and [addr+1] bytes returned in message from pc for each read

   for (addr=0; addr< ((int) COLS>> 3)*ROWS; addr++) 
     write_screen_byte (addr,mdt_read_screen(addr));
  
   WT
#endif
    
   set_draw_flags(0);
 
   // diagnostic test of mouse messages
   /*
    while (1) {
      get_touch(1); // updates mouse?

      lcd_setxy(20,10);

          delay_100ths(50);

      printf ("%4.4d %4.4d \n",MouseX,MouseY);
   }
   */

   
/*
  // future 

   mdt_clear(0);  // color
   mdt_fontsize(20);
   mdt_font(PS("NewTimesRoman"));
   mdt_textcolor(MDT_RED);
   mdt_bgcolor(MDT_BLUE);    // background color

   mdt_setxy(200,200);
  
   printf("Time 00:03 02");
  
   mdt_linewidth(4);
   mdt_pencolor(MDT_YELLOW);
  
   mdt_box(100,100, 300,200);
   mdt_line(100,100, 300,200);


   mdt_moveto(20,20);  // same as setxy
   mdt_lineto(40,40);
   mdt_lineto(20,40);
   mdt_lineto(20,20);


   mdt_fillcolor(MDT_LIME);
   mdt_fillmode(1); // solid 
   mdt_box(100,100,150,150);  // solid box

   */

   //lcd_set_stdout(); // set standard output to LCD
  
 
   //map_demo();
}


#endif // if MDTERM_DEMO
#endif
