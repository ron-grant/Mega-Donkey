/*  MegaDonkey Library File:  trakball.c    Track Ball Support
    


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


    Mark Sims
	2007

 
*/ 



#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "md.h"

#ifdef TRACKBALL_CODE
#include "lcd.h"
#include "adc.h"
#include "uart.h"
#include "led.h"
#include "timer.h"
#include "trakball.h"
#include "md_term.h"

/*

   Every time mouse (trackball) state changes (motion or button state change) 
   the following 3 byte packet is sent. Note that D6=1 in byte one allows 
   synchronization test.

   MSB received first (1200 baud)

   D7      D6      D5      D4      D3      D2      D1      D0
   X       1       LB      RB      Y7      Y6      X7      X6
   X       0       X5      X4      X3      X2      X1      X0      
   X       0       Y5      Y4      Y3      Y2      Y1      Y0


   LB is the state of the left button (1 means pressed down)
   RB is the state of the right button (1 means pressed down)
   X7-X0 movement in X direction since last packet (signed byte)
   Y7-Y0 movement in Y direction since last packet (signed byte)

*/
   
s08 mouse_dx;  // mouse movement deltas
s08 mouse_dy;  


#ifdef MOUSE_UART3       // 1200 bps basic software uart via 10KHz timer
#define UART_BITS 7      // mice use 7 data bits
#define SW_UART_RCV      // enable receiver code
//#define SW_UART_SEND   // enable transmitter code

void (*after_mouse_10KHz)(void) = NULL;  // pointer to next service routine in chain
u08 mouse_int_state;

// software UART states
#define UART_IDLE  0                  // waiting for something to do
#define RCV_START  11                 // delay 1/2 bit time for receive
#define SEND_START 12                 // sending start bit for transmit
#define UART_DATA  (UART_BITS+1)  // states 8/9..2  processing data bits
#define UART_STOP  1                  // processing stop bit

// 1200 baud - number of timer ticks to spend in each state
u08 mouse_uart_ticks[] = {   
    1,   // 0 - UART_IDLE: look for start bit or data to send every tick
    9,   // 1 - UART_STOP: send/receive stop bit
    8,   // 2 - D0
    8,   // 3 - D1
    9,   // 4 - D2
    8,   // 5 - D3
    8,   // 6 - D4
    9,   // 7 - D5
    8,   // 8 - D6
    8,   // 9 - D7
    8,   // 10 - SEND_START bit
    4    // 11 - RCV_START - delay half a bit time to verify start bit
};

#ifdef SW_UART_SEND   // enable sofware uart transmitter code
#define mouse_send_bit(d) if(d) sbi(PIND, 6); else cbi(PIND, 6)
#define setup_send_pin()  mouse_send_bit(1); sbi(DDRD, 6)

volatile u08 mouse_send_tick;  // current time tick within the current state
volatile u08 mouse_send_state;
volatile u08 mouse_send_data;
volatile u08 mouse_send_mask;

#define MOUSE_SEND_QSIZE 8
volatile u08 mouse_sendq[MOUSE_SEND_QSIZE];
volatile u08 mouse_send_front;
volatile u08 mouse_send_back;
volatile u08 mouse_send_count;

void mouse_send(u08 c)
{
u08 sreg;

   while(mouse_send_count >= MOUSE_SEND_QSIZE) ;     //!!! wait for queue space

   sreg = SREG;
   cli();
   ++mouse_send_count;                         
   mouse_sendq[mouse_send_back] = c;   // put data in queue 
   if(++mouse_send_back >= MOUSE_SEND_QSIZE) mouse_send_back = 0;    // wraparound
   SREG = sreg;
}
#endif // SW_UART_SEND




#ifdef SW_UART_RCV  // enable sofware uart receiver code
#define mouse_haschar() mouse_rcvcount

// see: md.h for SW_UART_xxx  port and bit defs

#define mouse_rcv_bit()  ((SW_UART_RPIN & _BV(SW_UART_RBIT)))

#define setup_rcv_pin()  sbi(SW_UART_RPORT,SW_UART_RBIT); cbi(SW_UART_RDDR,SW_UART_RBIT)
#define setup_send_pin() sbi(SW_UART_TPORT,SW_UART_TBIT); sbi(SW_UART_TDDR,SW_UART_TBIT)



u08 mouse_getchar()
{
u08 sreg;
u08 c;

   if(mouse_rcvcount) {
      sreg = SREG;
      cli();  // disable interrupts
      mouse_rcvcount--;   // total # chars in queue - 1                

      c = mouse_rcvq[mouse_rcvfront];  // return front of queue char
      if(++mouse_rcvfront >= MOUSE_RCV_QSIZE) mouse_rcvfront = 0;  // advance pointer index wrap around  
      SREG = sreg;
   }
   else c = 0;

   return c;
}
#endif  // SW_UART_RCV



void mouse_10KHz_update(void)
{
   if((mouse_int_state & ENABLED) == 0) goto mouse_exit;

#ifdef SW_UART_RCV
   if(--mouse_rcv_tick) goto rcv_exit;

setup_rcv_pin();   //!!!!
   if(mouse_rcv_state == UART_IDLE) {  // check for start bit
      if(mouse_rcv_bit() == 0) {      // start bit seen
         mouse_rcv_state = RCV_START;
      }
   }
   else if(mouse_rcv_state == RCV_START) {   // verify start bit
      if(mouse_rcv_bit() == 0) {      // start bit still seen
         mouse_rcv_state = UART_DATA;
         mouse_rcv_data = 0;
      }
      else {   // false start bit
         mouse_rcv_state = UART_IDLE;
      }
   }
   else if(mouse_rcv_state == UART_STOP) {  // character received, add to queue
      mouse_rcv_state = UART_IDLE;
      if(mouse_rcvcount < MOUSE_RCV_QSIZE) {  // there is room in the queue
         mouse_rcvcount++; // total # of chars in queue
         mouse_rcvq[mouse_rcvback] = mouse_rcv_data;  // store character in queue
         if(++mouse_rcvback >= MOUSE_RCV_QSIZE) mouse_rcvback = 0;   // advance pointer and wrap around if needed        
      }
      else { // !!! queue overflow
      }
   }
   else {  // shift in a data bit
      --mouse_rcv_state;
      mouse_rcv_data >>= 1;
      if(mouse_rcv_bit()) mouse_rcv_data |= _BV(UART_BITS-1);
   }

   mouse_rcv_tick = mouse_uart_ticks[mouse_rcv_state];

   rcv_exit:
#endif // SW_UART_RCV

#ifdef SW_UART_SEND
   if(--mouse_send_tick) goto send_exit;
   mouse_send_tick = mouse_uart_ticks[mouse_send_state];

   if(mouse_send_state == UART_IDLE) { 
      if(mouse_send_count) {  // we have something to send
         --mouse_send_count;  // remove a char from queue and prepare to send it
         mouse_send_data = mouse_sendq[mouse_send_front++];
         if(mouse_send_front >= MOUSE_SEND_QSIZE) mouse_send_front = 0; // advance index and wrap around 

         mouse_send_mask = _BV(UART_BITS-1);
         mouse_send_state = SEND_START;
      }
   }
   else if(mouse_send_state == SEND_START) { 
      mouse_send_bit(0);    // send start bit
      mouse_send_state = UART_DATA;
   }
   else if(mouse_send_state == UART_STOP) { 
      mouse_send_bit(1);    // send stop bit
      mouse_send_state = UART_IDLE;
   }
   else {  // shift out a data bit
      if(mouse_send_data & mouse_send_mask) mouse_send_bit(1);
      else mouse_send_bit(0);
      mouse_send_mask >>= 1;
      --mouse_send_state;
   }

   send_exit:
#endif

   mouse_exit:
   if(after_mouse_10KHz) after_mouse_10KHz();
}


u08 mouse_init(void)
{
u08 sreg;

   mouse_state = 0;

#ifdef SW_UART_RCV
   setup_rcv_pin();
#endif

#ifdef SW_UART_SEND
   setup_send_pin();
#endif

   mouse_int_state |= ENABLED;

   if((mouse_int_state & HOOKED) == 0) { // mouse uart timer routine already running -- exit
      sreg = SREG;
      cli();
      after_mouse_10KHz = service_10KHz;  // save pointer to old service routine 
      service_10KHz = mouse_10KHz_update; // pointer to new service routine
      mouse_int_state |= HOOKED;

#ifdef SW_UART_RCV
      mouse_rcv_state = UART_IDLE;
      mouse_rcv_tick = mouse_uart_ticks[mouse_rcv_state];
      mouse_rcvfront = mouse_rcvback = mouse_rcvcount = 0;
#endif

#ifdef SW_UART_SEND
      mouse_send_state = UART_IDLE;
      mouse_send_tick = mouse_uart_ticks[mouse_send_state];
      mouse_send_front = mouse_send_back = mouse_send_count = 0;
#endif

      SREG = sreg;
   }
   
   enable_rts1();       //!!!!!! testing with serial mouse
   assert_rts1();
   delay_ms(300);
   clear_rts1();
   delay_ms(300);
   assert_rts1();
   delay_ms(300);

   MouseX = cursor_x = COLS/2;
   MouseY = cursor_y = ROWS/2;
   return 0;
}

void disable_mouse(void)
{
u08 sreg;

  mouse_int_state &= (~ENABLED);
  if((mouse_int_state & HOOKED) == 0) return;

  // if mouse uart timer routine is first thing in the timer chain
  // we can fully unhook it
  if(service_10KHz == mouse_10KHz_update) { 
     sreg = SREG;
     cli();
     service_10KHz = after_mouse_10KHz;
     after_mouse_10KHz = NULL;
     mouse_int_state &= (~HOOKED);
     SREG = sreg;
  }
}

#endif // MOUSE_UART3


#ifdef MOUSE_UART0
#define mouse_getchar() rx_getchar()
#define mouse_haschar() rx_haschar()
#define disable_mouse()

u08 mouse_init(void)
{
  mouse_state = 0;
  uart0_init(1200,7,'N',1,0);  // baud,bits,parity,stop,protocol
  enable_rts0();
  assert_rts0();
  delay_ms(300);
  clear_rts0();
  delay_ms(300);
  assert_rts0();
  delay_ms(300);

  MouseX = cursor_x = COLS/2;
  MouseY = cursor_y = ROWS/2;
  return 0;
}
#endif // MOUSE_UART0


#ifdef MOUSE_UART1
#define mouse_getchar() rx1_getchar()
#define mouse_haschar() rx1_haschar()
#define disable_mouse()

u08 mouse_init(void)
{
  mouse_state = 0;
  uart1_init(1200,7,'N',1,0);  // baud,bits,parity,stop,protocol
  enable_rts1();
  assert_rts1();
  delay_ms(300);
  clear_rts1();
  delay_ms(300);
  assert_rts1();
  delay_ms(300);

  MouseX = cursor_x = COLS/2;
  MouseY = cursor_y = ROWS/2;
  return 0;
}
#endif // MOUSE_UART1





#ifdef MOUSE

u08 get_touch(u08 count)
{
   #ifdef MDT_CODE           // remote terminal (Donkey Term) sent a mouse message
      if(mdtMouseEvent) {
          MouseX = mdtMouseX;           // force cursor to Donkey Term cursor pos
          MouseY = mdtMouseY;           // which has been scaled to LCD coords

          move_cursor(MouseX,MouseY);

          if(mdtMouseLB) {
             touchX = MouseX;
             touchY = MouseY;
             return 1;
          } 

          mdtMouseEventClear(); // allow update routine to get more events
      }
   #endif

   if(get_mouse()) {
      move_cursor(MouseX,MouseY);
      if(MouseLB) {
         touchX = MouseX;
         touchY = MouseY;
         return 1;
      }
   }

   if(MouseRB) {  // slow down drawing so you can see what is happening
      delay_ms(200);  
   }

   touchX = touchY = (-1);
   return 0;
}


u08 get_mouse(void) 
{   
u08 c;
 
   // returns 0 if mouse has not changed or still proceesing message
   // returns 1 if mouse has changed,  sets MouseX, MouseY, and mouse_state

   while(mouse_haschar()) {  // processes all pending mouse messages
      c = mouse_getchar();

      if(c & _BV(6)) {  // test synchronization bit - first byte of message
         // start of message - reset byte index and button state
         mouse_state &= (~(MOUSE_MSG_BYTE | MOUSE_LB | MOUSE_RB));
#ifdef RVS_MOUSE_BUTTONS
         if(c & _BV(4)) mouse_state |= MOUSE_LB;
         if(c & _BV(5)) mouse_state |= MOUSE_RB;
#else
         if(c & _BV(5)) mouse_state |= MOUSE_LB;
         if(c & _BV(4)) mouse_state |= MOUSE_RB;
#endif
         mouse_dx = (c << 6);
         mouse_dy = (c << 4) & 0xC0;
      }
      else if((mouse_state & MOUSE_MSG_BYTE) == 0) {  // second byte of message
         ++mouse_state;
         mouse_dx |= c; 
#ifdef RVS_MOUSE_X
         mouse_dx = 0 - mouse_dx;
#endif
      }
      else if((mouse_state & MOUSE_MSG_BYTE) == 1) { // final byte of message
         ++mouse_state;
         mouse_dy |= c;
#ifdef RVS_MOUSE_Y
         mouse_dy = 0 - mouse_dy;
#endif

#ifdef SWAP_MOUSE_XY
         MouseY += mouse_dx;
         MouseX += mouse_dy;
#else 
         MouseX += mouse_dx;
         MouseY += mouse_dy;
#endif



         // clip mouse position to the screen
         if(MouseX < 0) MouseX = 0;
         if(MouseX >= COLS) MouseX = COLS-1;
         if(MouseY < 0) MouseY = 0;
         if(MouseY >= ROWS) MouseY = ROWS-1;
         mouse_state |= MOUSE_CHANGED;
      }
   }

   

   if(mouse_state & MOUSE_CHANGED) {
      mouse_state &= (~MOUSE_CHANGED);
      return 1;

   } // end while mouse has char
   return 0;

} // end get_mouse()

#endif


#ifdef TRACKBALL_DEMO

void trackball_demo(void) 
{
char s[3];

  set_color(WHITE);
  set_bg(BLACK);
  lcd_clear();

  lcd_setxy(0,0);
  printf(PS("Trackball"));
  mouse_init();

  s[2] = 0;                 // mouse button state string
  while(1)  {
     if(get_mouse()) {
        if(MouseLB) s[0] = 'L';
        else        s[0] = ' ';

        if(MouseRB) s[1] = 'R';
        else        s[1] = ' ';

        if(MouseLB && MouseRB) break;

        lcd_setxy(0,0);
        printf(PS("%3d,%-3d %s"), MouseX,MouseY ,s);

        dot(MouseX,MouseY);
     }
  
  } // end while

  wait_while_touched();
  lcd_clear();
}

#endif // TRACKBALL_DEMO

#endif // TRACKBALL_CODE
