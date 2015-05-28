/*  MegaDonkey Library File:  adc.c    ADC, Touch Screen and Mouse Support


    


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
 


*/


//#define ADC_DEMO    //!!!!!    *LIB  Commented Out

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "md.h"    // for bit set clear  sbi cbi functions

#ifdef ADC_CODE
#include "lcd.h"
#include "adc.h"
#include "timer.h"
#include "menu.h"
#include "trakball.h"
#include "md_term.h" // for access to mdHooked -- so we can 
                     // turn it off during cursor draw/erase if MDT_CODE defined               
                     // enabling remote mega donkey terminal


// if BLIT_CURSOR not defined draw plus cursor using get_dot() / dot();
// which does not work for color panels....

#define BLIT_CURSOR 1  // draw block cursor using blit()
//#define BLIT_CURSOR 0    // draw plus cursor using blit() (75% faster then dot by dot)
//#define BLIT_CURSOR '*'    // draw char cursor using blit()
// adc.c

							 
#define SINGLE_CONVERSION


#define ADC_REF _BV(REFS0)   // 5 volt ref for ADCSRA register

// REFS1 REFS0
//   0    0    External REF on AREF
//   0    1    AVCC with external capacitor at AREF
//   1    1    2.56 internal ref -- NO ALLOWED with Voltage applied to AREF


// need selections for different reference voltages
// possibly different modes  gains...



struct TOUCH_CAL {
   u16 tx_cpp;  // touch panel scaled counts per pixel
   u16 ty_cpp;
   u16 tx_min;  // touch panel scaled top/left minimum counts
   u16 ty_min;
   u16 check;
} cal = {
   120, 200,    
   4350, 5800,
   0
};

// #define ZAPPED (-1)   Now ADC_SAMPLE_INVALID with global scope



#ifdef JOY_SWITCH

#define JOY_TIME       50
#define JOY_ACCEL_STEP 2
#define JOY_ACCEL_MAX  6

#define JOY_QSIZE 16
volatile u08 joy_xq[JOY_QSIZE];
volatile u08 joy_yq[JOY_QSIZE];
volatile u16 joy_xsample = (-1);
volatile u08 joy_front, joy_back;
volatile u08 joy_time;
volatile u08 joy_accel;


void flush_joyq(void)
{
u08 sreg;

   sreg = SREG;
   cli();
   joy_front = joy_back = joy_qcount = 0;
   SREG = sreg;
}

void joy_init(void)
{
   // We waste two pins here to provide +5 and GND to the joy switch
   // resistor ladder.  If power is available (like via the extended six pin 
   // touch screen header), we can use these pins for more constructive uses.
#ifdef JOYSWITCH_PIN_POWER
   sbi(DDRC,1);
   cbi(PORTC, 1);
   sbi(DDRC,0);
   sbi(PORTC, 0);
#endif
   flush_joyq();
   joy_time = joy_accel = JOY_TIME;
   joy_xsample = (-1);
   joy_state = 0;
}


u08 get_joy()
{
u16 lx, ly;
u08 sreg;

    if(joy_qcount == 0) {  // sample queue is empty
       if(joy_state & (JOY_LB | JOY_RB)) { // last sample had a button pressed
          return 1;        // maintain the button state until next sample arrives
       }
       else return 0;
    }

    // we have some joyswitch ADC samples pending in the queue
    // so remove and process them
    joy_state &= (~JOY_BRAKED);
    while(joy_qcount) {  // work until the queue is drained
       sreg = SREG;
       cli();  // disable ints
       lx = joy_xq[joy_back]; 
       ly = joy_yq[joy_back];
       if(++joy_back >= JOY_QSIZE) joy_back = 0;
       --joy_qcount;
       SREG = sreg;   // restore interrupt state

       joy_state &= (~(JOY_LB | JOY_RB));

       if(lx < (0x300>>2)) { // a move switch is closed
          if((joy_state & JOY_BRAKED) == 0) {  // don't move cursor if brakes applied
             if(lx > (0x250>>2)) { // down
                if(++MouseY >= ROWS) MouseY = ROWS-1;
             }
             else if(lx > (0x1C0>>2)) { // left
                if(--MouseX < 0) MouseX = 0;
             }
             else if(lx > (0x100>>2)) { // right
                if(++MouseX >= COLS) MouseX = COLS-1;
             }
             else { // up
                if(--MouseY < 0) MouseY = 0;
             }
          }
       }
       else if(ly >= (0x0300>>2)) {  // no button or move switch is closed
          joy_state |= JOY_BRAKED;   // reduce cursor "skid" during heavy drawing
       }                             // by ignoring any move samples that occur
                                     // a no-switch-pressed state is seen

       if((ly < (0x0300>>2)) || joy_mode) {  // a button switch is closed
          joy_state |= JOY_LB;
       }

       joy_state |= JOY_CHANGED;
    }

    if(joy_state & JOY_CHANGED) {
       joy_state &= (~JOY_CHANGED);
       return 1;
    }
    return 0;
}


u08 get_touch(u08 count)
{
   if(get_joy()) {
//lcd_setxy(0,56);
//printf("%3d,%-3d %02X %d", MouseX, MouseY, joy_state, MouseLB);
      move_cursor(MouseX, MouseY);
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

#endif // JOY_SWITCH



#ifdef CANON_BALL

#define BALL_UP     0x01
#define BALL_DOWN   0x02
#define BALL_LEFT   BALL_UP
#define BALL_RIGHT  BALL_DOWN
#define BALL_BUTTON 0x10
#define BALL_IDLE   0x20
#define BALL_LOW    0x40
#define BALL_HIGH   0x80


u08 ball_state[256] PROGMEM = {  // converts ADC reading into ball movement code
   0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 
   0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 
   0x40, 0x40, 0x40, 0x10, 0x10, 0x10, 0x10, 0x10, 
   0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
   0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
   0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
   0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
   0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
   0x10, 0x10, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 
   0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 
   0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 
   0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 
   0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 
   0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 
   0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
   0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
   0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
   0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
   0x20, 0x20, 0x20, 0x11, 0x11, 0x11, 0x11, 0x11, 
   0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 
   0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 
   0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 
   0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x02, 
   0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
   0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
   0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
   0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
   0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
   0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
   0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
   0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x80, 0x80, 
   0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
};

#define JOY_TIME       1
#define JOY_ACCEL_STEP 2
#define JOY_ACCEL_MAX  6

#define JOY_QSIZE 16 
volatile u08 joy_xq[JOY_QSIZE];
volatile u08 joy_yq[JOY_QSIZE];
volatile u16 joy_xsample = (-1);
volatile u08 joy_front, joy_back;
volatile u08 joy_time;
volatile u08 joy_accel;

u08 last_xxx, last_yyy, last_bbb;
u08 last_sx, last_sy;

unsigned long ball_vtime;
unsigned long ball_htime;

void flush_joyq(void)
{
u08 sreg;

   sreg = SREG;
   cli();
   joy_front = joy_back = joy_qcount = 0;
   SREG = sreg;
}

void joy_init(void)
{
   // We waste two pins here to provide +5 and GND to the joy switch
   // resistor ladder.  If power is available (like via the extended six pin 
   // touch screen header), we can use these pins for more constructive uses.
#ifdef JOYSWITCH_PIN_POWER
   sbi(DDRC,1);
   cbi(PORTC, 1);
   sbi(DDRC,0);
   sbi(PORTC, 0);
#endif
   flush_joyq();
   joy_time = joy_accel = JOY_TIME;
   joy_xsample = (-1);
   joy_state = 0;

   last_xxx = last_yyy = last_bbb = 0;
   ball_vtime = ball_htime = get_msecs_alive();
}

#endif // CANON_BALL



void adc_init(u08 Channels)  // start repetitive scan -> ADCData[]
{
    ADCIndex    = 0;               // start on channel 0
	if (!Channels) Channels = 8;   // 0 -> 8
   
    sbi(ACSR, ACD );    // turn off analog comparator

    ADMUX = ADC_REF | ADCIndex;  // 5v ref and result is right adjusted ADLR=0
                                 // these bits are preserved in future setting
                                 // of ADMUX reg

    // ADCSRA bit fields:
    // 7 ADC Enable
    // 6 Start Conversion
    // 5 Free Run (when 1)
    // 4 Interrupt Flag - set 1 when conversion completes
    // 3 Interrupt Enable

    // 2
    // 1 2..0 -> prescale cpu clk  (see table above)
    // 0


    #ifdef TIMER_CALL_ADC
      ADCModeChannels = Channels | 0x80;  // set global ADC Channels including MSB=1 which flags
	                                     // timer0 10KHz interrupt routine to start calling 
		                                // adc_10KHz_update versus using SIGNAL(ADC
    #else 
       ADCModeChannels = Channels;

       #ifdef SINGLE_CONVERSION
         ADCSRA = 0xC8 | ADC_PRESCALE;    // Start, single conversion 1100 1000
       #else
         ADCSRA = 0xE8 | ADC_PRESCALE;  // Start, auto conversions 1110 1000
       #endif 
    #endif
}




#ifdef TIMER_CALL_ADC

  /* TIMER_CALL_ADC is a switch that modifies the below SIGNAL(SIG_ADC) interrupt service
     function into a standard C function that is then called from the timer0 10KHz
	 service routine.
	 
	 This frees up the interrupt vector to be taken over by a user program if they
	 choose to shutdown the donkey ADC service.
	 
	 For example, if you need a very lean ADC capture on one channel at a very high
	 rate it might not be practical to use existing donkey interrupt service routine.

     Using the TIMER_CALL_ADC switch does result in an alteration in the rate at which
	 the ADC does run (without getting into changing the prescaler for faster conversion).
	 The 10KHz rate is too fast for the conversion which can only run at about 8 KHz.

	 Therefore one option is to perform conversions at 1/2 the 10 KHz rate - OR -
	 bump the prescaler down a notch to get conversions to run at about 16 KHz.

*/


 void adc_10KHz_update(void)
#else 

 // when not calling the below code from 10 KHz service routine 
 // the below code calls itself upon each conversion complete at about 8 KHz

 SIGNAL(SIG_ADC)  // ADC Conversion Complete Interrupt 
#endif
{
   // timer0 calls this function at 10KHz provided that ADCModeChannels MSB is set.
 

   #ifdef USING_PRESCALE_7
   
   // divide by 2 on executing code in this function reducing
   // effective rate to 5 KHz

   if (ADCModeChannels & 0x40)
   {
     ADCModeChannels &= ~0x40;
	 return;
   }
   else ADCModeChannels |= 0x40;
   #endif



   #ifdef TOUCH_SCREEN

      if(ADCIndex == TOUCH_X_CHAN) {
         if(next_touch == TOUCH_X_CHAN) {
            ADCData[ADCIndex] = ADCW;   // store last conversion result
         }
      }
      else if(ADCIndex == TOUCH_Y_CHAN) {
         if(next_touch == TOUCH_Y_CHAN) {
            ADCData[ADCIndex] = ADCW;   // store last conversion result
         }
      }
      else ADCData[ADCIndex] = ADCW;   // store last conversion result
   #else
      ADCData[ADCIndex] = ADCW;   // store last conversion result

      #ifdef JOY_SWITCH
         if(ADCIndex == TOUCH_X_CHAN) {
            if(--joy_time == 0) {
               joy_time = joy_accel;

               joy_xsample = ADCData[ADCIndex];
               if(joy_xsample <= 0x0300) {  // a movement switch is closed
                  if(joy_accel > JOY_ACCEL_MAX) joy_accel -= JOY_ACCEL_STEP;
               }
               else joy_accel = JOY_TIME;
            }
         }
         else if(ADCIndex == TOUCH_Y_CHAN) {
            if(joy_xsample <= 0x03FF) {  // its time to take a sample
               joy_xq[joy_front] = joy_xsample >> 2;   // save state in the joy queue
               joy_yq[joy_front] = (ADCData[ADCIndex] >> 2);
               if(++joy_front >= JOY_QSIZE) joy_front = 0;
               if(joy_qcount < JOY_QSIZE) ++joy_qcount;
              
               joy_xsample = (-1);
            }
         }
      #endif // JOY_SWITCH

      #ifdef CANON_BALL
	     if(ADCIndex == TOUCH_X_CHAN) {
            if(--joy_time == 0) {
               joy_time = joy_accel;

               joy_xsample = ADCData[ADCIndex];
               if(0 && (joy_xsample <= 0x0300)) {  // a movement switch is closed
                  if(joy_accel > JOY_ACCEL_MAX) joy_accel -= JOY_ACCEL_STEP;
               }
               else joy_accel = JOY_TIME;
            }
         }
         else if(ADCIndex == TOUCH_Y_CHAN) {
            if(joy_xsample <= 0x03FF) {  // its time to take a sample
               joy_xq[joy_front] = joy_xsample >> 2;   // save state in the joy queue
               joy_yq[joy_front] = (ADCData[ADCIndex] >> 2);
               if(++joy_front >= JOY_QSIZE) joy_front = 0;
               if(joy_qcount < JOY_QSIZE) ++joy_qcount;
              
               joy_xsample = (-1);
            }
         }
      #endif // CANON_BALL
   #endif


   // advance channel index, with wrap around
   // masking off MSB which is flag indicating timer to call this routine
   // also bit 6 used for divide call rate by 2
   // hence mask off upper 4 bits
      
   if(++ADCIndex >= (ADCModeChannels & 0xF)) ADCIndex = 0;  
   ADMUX = ADC_REF | ADCIndex;   // select new channel to digitize

   // If allowing periodic ADC scan to capture touch screen,
   // set drive signals for drive touch screen plane.  
   // Also configure pins for touch screen sense plane.
   //
   // Touch screen pins as they appear from component side of board with
   // touch connector on right.  Top to bottom:
   //
   // PC0 Drive / weak pull up
   // PF2 Analog In / Drive
   // PF3 Analog In / Drive
   // PC1 Drive / weak pull up
   //
   //  2 pins drive  +5,GND
   //  1 pin ADC input with weak pullup,  1 pin weak pullup
   //  Different combination for each axis X and Y
   //
   #ifdef TOUCH_SCREEN
      if(ADCIndex == TOUCH_SETUP_CHAN) {
         if(next_touch == TOUCH_Y_CHAN) {   // setup for capturing X on ADC2
            next_touch = TOUCH_X_CHAN;
            cbi(DDRF,2);   sbi(PORTF,2);  // analog input w/pull up on X sense
            cbi(DDRC,1);   sbi(PORTC,1);  // pull up one end of sense plane
            sbi(DDRF,3);   sbi(PORTF,3);  // output +5 on drive plane
            sbi(DDRC,0);   cbi(PORTC,0);  // output GND on drive plane
         }
         else if(next_touch == TOUCH_X_CHAN) {  // setup for capturing Y on ADC3
            next_touch = TOUCH_Y_CHAN;
            cbi(DDRF,3);   sbi(PORTF,3);      // analog in w/pull up on Y sense
            cbi(DDRC,0);   sbi(PORTC,0);      // pull up one end of sense plane
            sbi(DDRF,2);   sbi(PORTF,2);      // output +5V lines on drive plane
            sbi(DDRC,1);   cbi(PORTC,1);      // output GND on drive plane
         }
//       else next_touch = TOUCH_X_CHAN;
      }
   #endif


    // ADCSRA bit fields:
    // 7 ADC Enable
    // 6 Start Conversion
    // 5 Free Run (when 1)
    // 4 Interrupt Flag - set 1 when conversion completes
    // 3 Interrupt Enable

    // 2
    // 1 2..0 -> prescale cpu clk  (see table above)
    // 0

    #ifdef TIMER_CALL_ADC
	  // Start Conversion with NO INTERRUPT  1100 0000 + PRESCALE
     ADCSRA = 0xC0 | ADC_PRESCALE;  // start next single conversion 
                                          // timer calls this function at 10KHz 
                                         
    #else
       // start next conversion
       #ifdef SINGLE_CONVERSION
          ADCSRA = 0xC8 | ADC_PRESCALE;  // Start, single conversion 1100 1000
       #endif




    #endif
}


int get_adc_sample(int channel)
{
int sample;
u08 sreg;

   sreg = SREG;
   cli();  // disable ints
   sample = ADCData[channel];
   ADCData[channel] = ADC_SAMPLE_INVALID;     // -1 
   SREG = sreg;  // restore interrupt state

   return sample;
}


//
// touch panel user waits and delays
//
void wait_while_touched(void)
{
   while(1) {
      if(get_touch(1) == 0) break;
   }
}

void wait_until_touched(void)
{
   while(1) {
      if(get_touch(1)) break;
   }
}


#ifdef TIMER_CODE
u08 delay_until_touched(unsigned d)
{
unsigned long t;
   
   if(d == 0) return 0;

   t = get_msecs_alive();
   if((t + d) < t) { // timealive is about to wrap
      reset_time_alive();
      t = d;
   }
   else t += d;

   while(t > get_msecs_alive()) {
      if(get_touch(1)) return 1;
   }
   return 0;
}


u08 delay_while_touched(unsigned d)
{
unsigned long t;
   
   if(d == 0) return 0;

   t = get_msecs_alive();
   if((t + d) < t) { // timealive is about to wrap
      reset_time_alive();
      t = d;
   }
   else t += d;

   while(t > get_msecs_alive()) {
      if(get_touch(1) == 0) return 1;
   }
   return 0;
}
#endif  // TIMER_CODE



#ifndef USER_INPUT
u08 get_touch(u08 count)
{
   return 0;
}
#endif // no USER_INPUT



#ifdef TOUCH_SCREEN

//  
//
//  Touch panel calibration stuff
//
//

#define CORNER_OFS  10

u08 save_touch_cal(void)
{
   cal.check = (cal.tx_cpp + cal.ty_cpp + cal.tx_min + cal.ty_min);
   eeprom_write_block((void *) &cal, (void *) EE_TOUCH_CAL, (size_t) sizeof cal);

   return 0;
}

u08 default_touch_cal(u08 reason)
{
   cal.tx_cpp = 120;  // touch panel scaled counts per pixel
   cal.ty_cpp = 200;

   cal.tx_min = 4350;  // touch panel scaled top/left minimum counts
   cal.ty_min = 5800;

   return reason;
}


u08 touch_init(void)                     
{
unsigned int v1, v2, v3, v4, v5;

   next_touch = TOUCH_X_CHAN;

   // Read EEPROM cal constants
   v1 = eeprom_read_word((uint16_t *)(EE_TOUCH_CAL+0));
   v2 = eeprom_read_word((uint16_t *)(EE_TOUCH_CAL+2));
   v3 = eeprom_read_word((uint16_t *)(EE_TOUCH_CAL+4));
   v4 = eeprom_read_word((uint16_t *)(EE_TOUCH_CAL+6));
   v5 = eeprom_read_word((uint16_t *)(EE_TOUCH_CAL+8));

   // validate the EEPROM data
   if((v1 == 0) || (v1 == 0xFFFF)) return default_touch_cal(1);
   if((v2 == 0) || (v2 == 0xFFFF)) return default_touch_cal(2);
   if((v3 == 0) || (v3 == 0xFFFF)) return default_touch_cal(3);
   if((v4 == 0) || (v4 == 0xFFFF)) return default_touch_cal(4);
   if((v1+v2+v3+v4) != v5) return default_touch_cal(5);

   // EEPROM data looks good, use it
   cal.tx_cpp = v1;
   cal.ty_cpp = v2;
   cal.tx_min = v3;
   cal.ty_min = v4;

   return 0;
}

void clear_touch(void)
{
u08 sreg;

   sreg = SREG;
   cli();  // disable ints
   ADCData[TOUCH_X_CHAN] = ADCData[TOUCH_Y_CHAN] = ADC_SAMPLE_INVALID;// zap current ADC samples
   SREG = sreg;   // restore interrupt state
}

u08 get_touch(u08 count)
{
int lx, ly, lc;
int x, y;
u08 sreg;

  #ifdef MDT_CODE           // remote terminal (Donkey Term) sent a mouse message
      if(mdtMouseEvent) {
          if(mdtMouseLB) {
             touchX = mdtMouseX;
             touchY = mdtMouseY;
             return 1;
          } 

          mdtMouseEventClear(); // allow update routine to get more events
      }
   #endif



    if((count < 1) || (count > 50)) count = 5;

    touchX = touchY = (-1);
    lx = ly = lc = 0;
    while(lc < count) {
       clear_touch();
       while(1) {   // wait for ADC interrupts to get fresh touch values
          sreg = SREG;
          cli();  // disable ints
          x = ADCData[TOUCH_X_CHAN];
          y = ADCData[TOUCH_Y_CHAN];
          SREG = sreg;   // restore interrupt state

          if((x >= 0) && (y >= 0)) break;
       }

       x = ((x << 5) - cal.tx_min) / cal.tx_cpp;  // scale ADC reading to screen pixels
       if(x < 0) return 0;
       else if(x >= COLS) return 0;

       y = ((y << 5) - cal.ty_min) / cal.ty_cpp;
       if(y < 0) return 0;
       else if(y >= ROWS) return 0;
       
       lx += x;  // average readings to reduce noise
       ly += y;
       lc += 1;
    }
    if(lc == 0) lc = 1;

    touchX = lx / lc;
    touchY = ly / lc;
    return 1;
}


u08 calibrate_touch(u08 force_cal)
{
long x, y;
long x1cal, y1cal, x1_count;
long x2cal, y2cal, x2_count;
u08 canceled;
u08 sreg;

    if(force_cal == 0) {  // don't do touch cal if already done
       if(touch_init() == 0) return 0;  // valid touch params were loaded from EEPROM
    }

    lcd_clear();      
    default_touch_cal(0);  /* start with default touch cal params */
                           /* in case the saved values are bogus */

    lcd_textPS(CORNER_OFS-CHAR_WIDTH/2,     CORNER_OFS-CHAR_HEIGHT/2,        "+");
    lcd_textPS(COLS-CORNER_OFS-CHAR_WIDTH/2,ROWS-CORNER_OFS-CHAR_HEIGHT/2,   "+");
    lcd_textPS(COLS/2-CHAR_WIDTH/2,         ROWS/2-CHAR_HEIGHT/2,            "X");

    lcd_textPS(3*CHAR_WIDTH, 0*CHAR_HEIGHT,   "Touch Calibration");
    lcd_textPS(3*CHAR_WIDTH, 1*CHAR_HEIGHT,   "   Touch each '+'");
    lcd_textPS(3*CHAR_WIDTH, 2*CHAR_HEIGHT,   "   Then touch 'X'");

    lcd_textPS(0*CHAR_WIDTH+CHAR_WIDTH/2,ROWS-CHAR_HEIGHT-2, "CANCEL");
    shaded_box(0, ROWS-CHAR_HEIGHT-4, 7*CHAR_WIDTH, ROWS-1);

    recal:
    canceled = 0;
    x1cal = y1cal = x1_count = 0;
    x2cal = y2cal = x2_count = 0;

    while(1) {
        sreg = SREG;
        cli();  // disable ints
        x = ADCData[TOUCH_X_CHAN];
        y = ADCData[TOUCH_Y_CHAN];
        SREG = sreg;  // restore interrupt state


//#define MAX_TOUCHX 955
//#define MAX_YOUCHY 950

//#define MAX_TOUCHX 910
//#define MAX_TOUCHY 910

#define MAX_TOUCHX 0x3E0
#define MAX_TOUCHY 0x3E0

        if(x < 100) continue;  /* bogusness */
        if(x > MAX_TOUCHX) continue;  /* nothing touched */
        if(y < 100) continue;
        if(y > MAX_TOUCHY) continue;

        clear_touch();  /* flush current touch samples so they don't get reused */

        if(1) {   //!!!!!! debug print
           lcd_setxy(0,ROWS-CHAR_HEIGHT*3); 
           printf(PS("ADC:%4ld,%-4ld"), x,y);
        }

        if(x < 400) { /* cal left */
           if(y > 750) {
              touch_init();   /* !!! reload saved touch values */
              canceled = 1;
              break;
           }
           else if(y > 300) continue;
           x1cal += (x << 5);   // top left cal spot
           y1cal += (y << 5);
           ++x1_count;
           beep(20, 300);
        }
        else if(x > 600) { /* cal right */
           if(y < 750) continue;
           x2cal += (x << 5);    // bottom right cal spot
           y2cal += (y << 5);
           ++x2_count;
           beep(20, 200);
        }
        else {  /* screen center area - exit cal */
           if(y < 300) continue;
           else if(y > 700) continue;
           beep(50, 250);
           if(x1_count == 0) goto recal;   // cal spot was not touched
           if(x2_count == 0) goto recal;

           x1cal /= x1_count;  // average the touch coord values
           y1cal /= x1_count;
           x2cal /= x2_count;
           y2cal /= x2_count;

           x = x2cal - x1cal;  // ADC counts per COLS-20 x pixels
           y = y2cal - y1cal;  // ADC counts per ROWS-20 y pixels

           lcd_setxy(0,ROWS-CHAR_HEIGHT*3); 
           printf(PS("CAL:%4ld,%-4ld"), x>>5,y>>5);

           if(x < (560L<<5)) goto recal;
           if(x > (880L<<5)) goto recal;
           if(y < (500L<<5)) goto recal;
           if(y > (800L<<5)) goto recal;
//         if(y > (660L<<5)) goto recal;

           cal.tx_cpp = x / (COLS - CORNER_OFS*2);  // ADC counts per pixel
           cal.ty_cpp = y / (ROWS - CORNER_OFS*2);

           cal.tx_min = x1cal - (cal.tx_cpp * CORNER_OFS);  // right margin offset
           cal.ty_min = y1cal - (cal.ty_cpp * CORNER_OFS);  // top margin offset

           save_touch_cal();  // put touch values into eeprom

           break;
        }
    }

    wait_while_touched();  

    lcd_clear();      
    if(canceled) lcd_textPS(0*CHAR_WIDTH,0*CHAR_HEIGHT, "Calibration CANCELED");
    else         lcd_textPS(0*CHAR_WIDTH,0*CHAR_HEIGHT, "Calibration complete");
    lcd_textPS(0*CHAR_WIDTH,2*CHAR_HEIGHT, "Touch screen to exit");

    lcd_setxy(0, ROWS-CHAR_HEIGHT*3);
    printf("CAL X: %d:%d\n", cal.tx_cpp, cal.tx_min);
    printf("CAL Y: %d:%d\n", cal.ty_cpp, cal.ty_min);

    wait_until_touched();
    wait_while_touched();
    lcd_clear();      

    return canceled;
}
                                                                                                                             
#else  // not TOUCH_SCREEN:

u08 calibrate_touch(u08 force_cal)
{
   return 2;
}

#ifdef PANEL_CODE
#ifdef BLIT_CURSOR
void blit_cursor(COORD cx, COORD cy)
{
u08 temp_draw;
u08 temp_big;
COORD lx;
COORD ty;

   temp_draw = draw_flags;
   set_draw_flags(FILLED | BLIT_XOR);
   set_color(WHITE);     // color saved/restored by calling routine
   set_bg(BLACK);

   if(cx < 3) lx = 0;
   else lx = cx-3;

   if(cy < 3) ty = 0;
   else ty = cy-3;

   if(BLIT_CURSOR > 1) {  // cursor is a text char
      temp_big = char_size;
      set_charsize(1);
      lcd_char(lx, ty, BLIT_CURSOR);
      set_charsize(temp_big);
   }
   else if(BLIT_CURSOR == 1) {
      blit(lx,ty, cx+3,cy+3, lx,ty);   // block cursor
   }
   else {
      blit(lx,cy, cx+3,cy,   lx,cy);   // plus cursor
      blit(cx,ty, cx,cy+3,   cx,ty);
   }

   set_draw_flags(temp_draw);
}
#else  // not BLIT_CURSOR:
   #define cdot(x, y, c)  if(c) set_color(WHITE);else set_color(BLACK); dot(x,y);
   u08 x_dots[MAX_PAGEBUFS], y_dots[MAX_PAGEBUFS];
#endif

#ifndef erase_cursor
u08 erase_cursor(void)
{
u16 temp_top;
COLOR temp_color;
COLOR temp_bg;
u08 i;

   if(cursor_on) {

      #ifdef MDT_CODE
      u08 hs = mdtHooked;  // save MDHooked state

      // possible future cursor
      // if(mdtHooked) mdt_cursor_erase(cursor_x,cursor_y)
        
      mdtHooked = 0;  // inhibit mdt_ calls from dot and blit below
      #endif


      cursor_on = 0;

      temp_top = top_line;
      temp_color = color;
      temp_bg = bg_color;

      for(i=0; i<=double_buffers; i++) {
         draw_on_page(i);
#ifdef BLIT_CURSOR
         blit_cursor(cursor_x, cursor_y);
#else
         cdot(cursor_x, cursor_y-3, y_dots[i] & 0x40);
         cdot(cursor_x, cursor_y-2, y_dots[i] & 0x20);
         cdot(cursor_x, cursor_y-1, y_dots[i] & 0x10);
         cdot(cursor_x, cursor_y+1, y_dots[i] & 0x04);
         cdot(cursor_x, cursor_y+2, y_dots[i] & 0x02);
         cdot(cursor_x, cursor_y+3, y_dots[i] & 0x01);

         cdot(cursor_x-3, cursor_y, x_dots[i] & 0x40);
         cdot(cursor_x-2, cursor_y, x_dots[i] & 0x20);
         cdot(cursor_x-1, cursor_y, x_dots[i] & 0x10);
//       cdot(cursor_x+0, cursor_y, x_dots[i] & 0x08);
         cdot(cursor_x+1, cursor_y, x_dots[i] & 0x04);
         cdot(cursor_x+2, cursor_y, x_dots[i] & 0x02);
         cdot(cursor_x+3, cursor_y, x_dots[i] & 0x01);
#endif
      }

      set_color(temp_color);
      set_bg(temp_bg);
      set_topline(temp_top);

      #ifdef MDT_CODE
      mdtHooked = hs;  // restore MDHooked state
      #endif

      return 1;
   }
   return 0;
}
#endif  // erase_cursor

void move_cursor(COORD x, COORD y)
{
u16 temp_top;
COLOR temp_color;
COLOR temp_bg;
u08 i;

   if(cursor_on) {
      if((x == cursor_x) && (y == cursor_y)) return;
      #ifndef erase_cursor
         erase_cursor();
      #endif
   }

   #ifdef MDT_CODE
   u08 hs = mdtHooked;  // save MDHooked state

    // possible future cursor
    // if(mdtHooked) mdt_cursor_move(x,y)
        
   mdtHooked = 0;  // inhibit mdt_ calls from dot and blit below
   #endif



   // save screen under the cursor
   temp_top = top_line;
   temp_color = color;
   temp_bg = bg_color;

   for(i=0; i<=double_buffers; i++) {
      draw_on_page(i);
#ifdef BLIT_CURSOR
      blit_cursor(x,y);
#else
      x_dots[i] = y_dots[i] = 0;
      if(get_dot(x-3, y)) x_dots[i] |= 0x40;
      if(get_dot(x-2, y)) x_dots[i] |= 0x20;
      if(get_dot(x-1, y)) x_dots[i] |= 0x10;
//    if(get_dot(x-0, y)) x_dots[i] |= 0x08;
      if(get_dot(x+1, y)) x_dots[i] |= 0x04;
      if(get_dot(x+2, y)) x_dots[i] |= 0x02;
      if(get_dot(x+3, y)) x_dots[i] |= 0x01;

      if(get_dot(x, y-3)) y_dots[i] |= 0x40;
      if(get_dot(x, y-2)) y_dots[i] |= 0x20;
      if(get_dot(x, y-1)) y_dots[i] |= 0x10;
      if(get_dot(x, y+1)) y_dots[i] |= 0x04;
      if(get_dot(x, y+2)) y_dots[i] |= 0x02;
      if(get_dot(x, y+3)) y_dots[i] |= 0x01;

      // draw cursor
      y_dots[i] ^= 0xFF;
      cdot(x, y-3, y_dots[i] & 0x40);
      cdot(x, y-2, y_dots[i] & 0x20);
      cdot(x, y-1, y_dots[i] & 0x10);
      cdot(x, y+1, y_dots[i] & 0x04);
      cdot(x, y+2, y_dots[i] & 0x02);
      cdot(x, y+3, y_dots[i] & 0x01);
      y_dots[i] ^= 0xFF;

      x_dots[i] ^= 0xFF;
      cdot(x-3, y, x_dots[i] & 0x40);
      cdot(x-2, y, x_dots[i] & 0x20);
      cdot(x-1, y, x_dots[i] & 0x10);
//    cdot(x+0, y, x_dots[i] & 0x08);
      cdot(x+1, y, x_dots[i] & 0x04);
      cdot(x+2, y, x_dots[i] & 0x02);
      cdot(x+3, y, x_dots[i] & 0x01);
      x_dots[i] ^= 0xFF;
#endif
   }

   set_color(temp_color);
   set_bg(temp_bg);
   set_topline(temp_top);

   cursor_x = x;  cursor_y = y;
   cursor_on = 1;

   #ifdef MDT_CODE
     mdtHooked = hs;  // restore saved status
   #endif


}
#endif  // TOUCH_SCREEN
#endif  // PANEL_CODE



#ifdef SKETCH_DEMO

void sketch_a_etch(void)
{
COORD tx,ty;  // local touch
u08 touched;
u08 NumVis;
COLOR temp_color, temp_bg;
u08 temp_joy;
int temp, avg, sum, x;
COORD bw,bh;

   // setup for reading temp sensor on ADC1
   cbi(DDRF,1);   cbi(PORTF,1);  // analog in no pull

   NumVis = 0;
   bw = (CHAR_WIDTH*BUTTON_SIZE) + (ButtonBorder*2);
   bh = (CHAR_HEIGHT*BUTTON_SIZE) + (ButtonBorder*2);

   temp_color = color;
   temp_bg = bg_color;
   temp_joy = joy_mode;

#ifdef JOY_SWITCH
   joy_mode = 1;
   MouseX = (COLS/2);
   MouseY = (ROWS/2);
#endif

   re_draw:
   set_color(BLACK);
   set_bg(WHITE);
   lcd_clear();      

   menu_init();         // kludge for drawing standard exit button
   menu_exitbutton();   // ... this demo does not really use menus
   menu_button(0, ROWS-bh, PS("#"), 1);
   menu_button(COLS-bw, ROWS-bh, PS("C"), 2);

   NumVis=1;
   avg = sum = 0;

   while(1) {   // loop until touch bottom right corner of panel causes exit
      touched = get_touch(3);
      tx = touchX;  ty = touchY;
      if(NumVis >= 1) {
         lcd_setxy (0,0);
         if(NumVis == 3) {  // reading temp sensor on channel 1
            x = get_adc_sample(1);
            if(x >= 0) {
               sum += x;
               if(++avg >= 10) {
                  temp = sum / avg;
                  printf(PS("%4do%4d"), temp,((((unsigned)temp)*900+512)>>10)+32);
                  avg = sum = 0;
               }
            }
         }
         else if(NumVis == 2) printf(PS("%4d,%4d"),ADCData[TOUCH_X_CHAN],ADCData[TOUCH_Y_CHAN]);
         else printf(PS("%4d:%4d"),tx,ty);
      }
      if(touched == 0) continue;

      if((tx<bw) && (ty>ROWS-bh)) {  // toggle show numbers switch
         NumVis = (NumVis+1) % 4;
         wait_while_touched();
         if(NumVis == 0) goto re_draw;
         else continue;
      }
      else if((tx>COLS-bw) && (ty<bh)) {  // exit
         break;
      }
      else if((tx >= (COLS-bw)) && (ty >=(ROWS-bh))) {  // clear screen
         wait_while_touched();
         goto re_draw;
      }

      dot(tx,ty);
      beep(20, tx*30);  // dur,freq  -- interrupt driven tone
   }

   set_color(temp_color);
   set_bg(temp_bg);
   joy_mode = temp_joy;
}

#endif // SKETCH_DEMO


#ifdef ADC_DEMO

void adc_demo(u08 ts_flag)
{
int i;

   if(ts_flag == 0) {
      wait_while_touched();
   }
   lcd_clear();

   while(1) {
      if(ts_flag == 0) {
         if(get_touch(1)) break;
      }

      for(i=0; i<8; i++) {
         lcd_setxy(0, i*CHAR_HEIGHT);
         if(ADCData[i] >= 0) {
//          printf(PS("ADC %d: %04X"), i, ADCData[i]);
            printf(PS("ADC %d: %4d"), i, ADCData[i]);
         }
      }
      if(ts_flag == 2) {
         if(get_touch(1)) printf("\nTOUCH: %3d,%-3d", touchX, touchY);
      }
   }

   wait_while_touched();
}
#endif // ADC_DEMO

#endif // ADC_CODE


#ifdef CANON_BALL

u08 joy_step = 4;

u08 get_touch(u08 count)
{
int lx, ly, lc;
u08 sreg;
u08 sx, sy;
u08 last_sx, last_sy;
unsigned long stime;

    if((count < 1) || (count > 50)) count = 5;

    touchX = touchY = (-1);

    last_sx = last_sy = 0;
    lc = 0;
count = 3-1;
    while(lc < count) {  // get stable ball ADC readings
       while(joy_qcount == 0) ;

       sreg = SREG;
       cli();        // disable ints
       lx = joy_xq[joy_back];    // get sample from queue
       ly = joy_yq[joy_back];
       if(++joy_back >= JOY_QSIZE) joy_back = 0;
       --joy_qcount;
       SREG = sreg;   // restore interrupt state

       sx = pgm_read_byte_near(&ball_state[lx]);
       sy = pgm_read_byte_near(&ball_state[ly]);
       if((last_sx == sx) && (last_sy == sy)) ++lc;
       else {  // samples are not stable,  try again
          lc = 0;
          last_sx = sx;
          last_sy = sy;
       }
    }


    joy_state = 0;

    if(sx & BALL_BUTTON) {   // button is down
       if(last_bbb != 'B') {    // button was just pressed
          last_bbb = 'B';
          joy_state = 1;
       }
    }
    else {                   // button is up
       if(last_bbb == 'B') {    // button was just released
          last_bbb = 0;
          joy_state = 2;
       }
    }

    if((sy & (BALL_UP | BALL_DOWN | BALL_IDLE | BALL_HIGH | BALL_LOW)) != last_yyy) {
       joy_state |= 0x04;
       last_yyy = sy & (BALL_UP | BALL_DOWN | BALL_IDLE | BALL_HIGH | BALL_LOW);
    }
    if((sx & (BALL_LEFT | BALL_RIGHT | BALL_IDLE | BALL_HIGH | BALL_LOW)) != last_xxx) {
       joy_state |= 0x08;
       last_xxx = sx & (BALL_LEFT | BALL_RIGHT | BALL_IDLE | BALL_HIGH | BALL_LOW);
    }

    if(joy_state == 0) {  // nothing has changed since last call
       return (0);
    }


    if((last_xxx != BALL_IDLE) || (last_yyy != BALL_IDLE)) {
        stime = get_msecs_alive();
#define FAST_STEP 8
#define FAST_TIME 50

        if(sy & BALL_DOWN) { // down
           if((stime - ball_vtime) > FAST_TIME) joy_step = 1;
else if((stime - ball_htime) > FAST_TIME) joy_step = 1;
           else joy_step = FAST_STEP;
           MouseY += joy_step;
           if(MouseY >= ROWS) MouseY = 0; // ROWS-1;
           ball_vtime = stime;
        }
        else if(sy & BALL_UP) { // up
           if((stime - ball_vtime) > FAST_TIME) joy_step = 1;
else if((stime - ball_htime) > FAST_TIME) joy_step = 1;
           else joy_step = FAST_STEP;
           MouseY -= joy_step;
           if(MouseY < 0) MouseY = ROWS-1;  // 0
           ball_vtime = stime;
        }
        else if(sx & BALL_LEFT) { // left
           if((stime - ball_htime) > FAST_TIME) joy_step = 1;
else if((stime - ball_vtime) > FAST_TIME) joy_step = 1;
           else joy_step = FAST_STEP;
           MouseX -= joy_step;
           if(MouseX < 0) MouseX = COLS-1;  //0;
           ball_htime = stime;
        }
        else if(sx & BALL_RIGHT) { // right
           if((stime - ball_htime) > FAST_TIME) joy_step = 1;
else if((stime - ball_vtime) > FAST_TIME) joy_step = 1;
           else joy_step = FAST_STEP;
           MouseX += joy_step;
           if(MouseX >= COLS) MouseX = 0;  //COLS-1;
           ball_htime = stime;
        }
//if(joy_step == 1) ball_vtime = ball_htime = stime-FAST_TIME-2;
        move_cursor(MouseX, MouseY);

        if(sx & BALL_BUTTON) {
           touchX = MouseX;
           touchY = MouseY;
           return 1;
        }
    }

    return 0;
}

#endif  // CANON_BALL
