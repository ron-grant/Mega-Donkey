/*  MegaDonkey Library File:  adc.h    ADC, Touch Screen and Mouse Support


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
 



 Mega Donkey ADC Support including Touch Panel,Track Ball, Joy Switch or Mouse Support

 Ron Grant  April 2007
 Touch Screen / Cursor routines: Apr 2007 Mark Sims
 Modified for full speed conversions: 22 May 07 Mark Sims
 Support for Alternate Pointing Devices Jun? 07 Mark Sims 
 Modified for timer call at 10KHz, freeing ADC interrupt for user: Jan 11 2008 - RG

 
 Alternate Pointing Devices include 
 Joy Switch, Canon Ball, Serial Mouse, Alps TrackBall (uses serial mouse protocol).
 Switches for these are found in md.h and require using donkey library as a project OR
 recompiling the library with custom switches set.


 normal conversion 13 ADC clock cycles per channel
 using cpu clock / prescale  definied by ADCSRA(0..2) divider

 ADCSRA  Divide  Conversion Clock rate   CPU Clock 16 MHz divided by N
 (2..0)



 000       2          8MHz
 001       2          8MHz
 010       4          4MHz
 011       8          2Mhz
 100       16         1Mhz
 101       32         500 KHz
 110       64         250 KHz  (allows call at 10KHz  conversion complete in 5.6 ms)
 111       128        125 KHz

 With TIMER_CALL_ADC defined and not TIMER_PRESCALE_7 defined, calling adc_init() will set prescaler to 6
 (divide by 64)  and will set MSB of ADCModeChannels to 1 which flags timer0 10KHz service routine to call
 adc_10KHz_update which takes care of reading an ADC channel and initiating the ADC conversion for the next channel (1 to 8) total and also perfoming operations 
 to sample touch panel (for touch LCD / analog JoySwitch Donkeys).

 With divide by 64, conversion completes in 16MHz/64/14 cycles = .056 ms, well below
 interrupt rate of 0.10 ms.  Divide by 128 just misses at conversion time of 0.112 ms.

 As an alternative, define USING_PRESCALE_7, which includes a divide by 2 in adc_10KHz_update
 and selects prescaler = 7 for ADC clock = CPU divided by 128. This is clock rate used
 for prototype Mega Donkeys with slower channel rate, e.g. before sequential acquisiiton
 at rate of .112 ms per channel, now 0.20 ms per channel.

 You can disable the 10KHz update and define your own ADC service which can include
 use ADC interrupt vector if desired. This is done by clearing MSB of ADCModeChannels is 0.

 calling adc_init(Channels)  where typically Channels = 8, but you can set to lower 
 number to speed rate at which lower channels are digitized. Below 4 and touch screen
 will not function.

 e.g. adc_init(1), digitizes channel 0 only at 10KHz.

 Also, by clearing MSB of ADCModeChannels you can stop timer from calling
 adc update function allowing you manual control of ADC -- touch screen will no longer
 function until you call adc_init again or set MSB of ADCModeChannels.

			  
*/

#ifndef _ADC_H_
#define _ADC_H_

#include "md.h"
#include "lcd.h" // for COORD def

#ifdef ADC_CODE

// TIMER_CALL_ADC defined in "md.h" (by default) to insure that timer.c knows if it needs to look at msb
// ADCModeChannels and call adc_10KHz_update, if set. Also, that function is not defined if TIMER_CALL_ADC is
// not defined. A SIGNAL routine is defined in its place in that case.


// USING_PRESCALE_7 is not defined by default, opting for faster conversion rate, and a conversion initiated
// every 1/10,000 sec.

//#define USING_PRESCALE_7 // forces adc_10KHz_update to run at 5 KHz by adding flip/flop
                         // divide by 2 into function. This allows adc to run at orignal
						 // CPU/128 rate which is the slowest rate and enables conversions
						 // to complete which take 0.112 ms where function call is 
						 // at 0.10 ms, but code executed every 0.2 ms 
						 // (see: adc_10KHz_update -- top of function)
						 // e.g. 0.2 ms per channel x 8 channels = 1.6 ms per scan
						 
#ifdef TIMER_CALL_ADC

                          // not using -- let user define function
//void (*adc_int) (void); // declare pointer to function 
                          // so we can call from SIGNAL (ADC_INT) which is 
						  // available when using timer to call adc update and 
						  // when that feature is disabled at runtime by clearing MSB
						  // of ADCModeChannel


  #ifdef USING_PRESCALE_7
    #define ADC_PRESCALE 7 // see above (don't alter this value)
  #else 
   #define ADC_PRESCALE 6 // Don't change this from 6
                          // this sets prescaler to (CPU/64) which cause a conversion 
						  // to complete in 0.056 ms which is under 0.1 ms, time between 
						  // 10 KHz timer interrupts.

  #endif
						  
#else 
  #define ADC_PRESCALE 7 // if NOT TIMER_CALL_ADC, old method using SIGNAL  
#endif                   // which initiated next conversion after conversion complete 
                         // interrupt for a conversion rate approx 8 KHz. 




//
// ADC Scanner  (digitize Ch 0 .. 7)   if adc_init(0) OR adc_init(8)
// 

 volatile int ADCData[8];  // raw ADC data



 u08 volatile ADCIndex;    // current ADC index 0..7
                           // can be querried to determine if single channel
                           // finished  

 u08 ADCModeChannels;      // number of channels being digitized set by call
                           // to adc_init. Included setting MSB to 1 to flag
                           // timer0 service to call adc_10KHz_update at 10KHz
				                  
                              

#define TOUCH_X_CHAN 2
#define TOUCH_Y_CHAN 3
#define TOUCH_SETUP_CHAN (TOUCH_Y_CHAN+1)
 int touchX, touchY;   // touch screen coords calculated by get_touch()


void adc_init(u08 Channels);    // cycle through channels  0..Channels-1
                                // repetitively filling in ADCData[]
                                // sets prescale for adc clock to 7 (CPU clk divide by 128)
								// flags timer0 to start calling adc_10KHz_update
							
void adc_10KHz_update(void);    // called by timer0 overflow interrupt routine at 10KHz
                                // when MSB of Channels set to 1 by adc_init() call



#define ADC_SAMPLE_INVALID (-1) // marker written to indicate sample read and not yet
                                // updated with new sample

int get_adc_sample(int channel); // get sample on current channel (destructively)
                                 // mark with ADC_SAMPLE_INVALID after reading
                                 // will remain marked until next update from ADC

u08 get_touch(u08 count);   // sample touch panel count times and average to reduce noise
                            // returns 0 if not touched, 1 if touched
							// resulting x,y appears in touchX and touchY
							// if not touched, touchX and touch Y will return -1



u08 calibrate_touch(u08 force_cal);
u08 touch_init(void);
void clear_touch(void);

void wait_until_touched(void);
void wait_while_touched(void);

#define wait_touch_release() {wait_until_touched(); wait_while_touched();}


u08 delay_until_touched(unsigned time);   // wait *time* mS or until touched
u08 delay_while_touched(unsigned time);   // wait up to *time* mS while touched

#ifdef JOY_SWITCH
   #define JOY_CHANGED  0x08
   #define JOY_BRAKED   0x10
   #define JOY_LB       0x80   // left button (joyswitch center)
   #define JOY_RB       0x40   // right button
   #define MouseLB (joy_state & JOY_LB)
   #define MouseRB (joy_state & JOY_RB)
 u08 joy_state;
 volatile u08 joy_qcount;
#endif

#ifdef CANON_BALL
   #define JOY_CHANGED  0x08
   #define JOY_BRAKED   0x10
   #define JOY_LB       0x80   // left button (joyswitch center)
   #define JOY_RB       0x40   // right button
   #define MouseLB (joy_state & JOY_LB)
   #define MouseRB (joy_state & JOY_RB)
 u08 joy_state;
 volatile u08 joy_qcount;
#endif

#ifdef TOUCH_SCREEN
   #define erase_cursor() 0
   #define show_cursor()
    u08 next_touch;
#else
   #ifndef USER_INPUT
      #define erase_cursor() 0
      #define show_cursor()
   #else
      u08 erase_cursor(void);
      void move_cursor(COORD x, COORD y);
   // #define show_cursor()  (touch_avail()? get_touch(1): move_cursor(cursor_x, cursor_y))
      #define show_cursor()  {if(touch_avail()) get_touch(1); move_cursor(MouseX, MouseY);}
       int last_lx, last_ly;
   #endif
#endif

 u08 joy_mode;

 int cursor_x;
 int cursor_y;
 int cursor_on;

void sketch_a_etch(void);
void adc_demo(u08 ts_flag);
void joy_init(void);

#endif // ADC_CODE 
#endif // _ADC_H


/*  Sample User Interrupt Driven service routine (to be included in your program)
    we are able to borrow ADCChannels and ADCIndex variables from system adc
	functions because our user setup has disabled the normal update adc_10KHz_update
	called from timer0 overflow interrupt routine.

    Note that the adc_10KHz update kick in when MSB of ADCChannels is set to 1
	which we don't allow in code below

    See demo program md_UserADCScope.c  for demo usage of this code



 

SIGNAL (SIG_ADC)  // ADC Conversion Complete Interrupt   (10 bit samples)
{
     ADCData[ADCIndex] = ADCW;     // store last conversion result
     if(++ADCIndex >= ADCModeChannels) ADCIndex = 0;   // advance channel index, with wrap around
     ADMUX = ADC_REF | ADCIndex;   // select new channel to digitize
     ADCSRA = 0xC8 | USER_ADC_PRESCALE;  // Start, single conversion 1100 1000
}            



void user_adc_init(u08 Channels)  // start repetitive scan -> ADCData[]
{
    ADCIndex         = 0;         // start on channel 0
    ADCModeChannels  = Channels;  // total number of channels active (MSB=0 10HKz update will not initiate ADC sample)


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

    #define USER_ADC_PRESCALE 7

    #ifdef USER_SINGLE_CONVERSION
       ADCSRA = 0xC8 | USER_ADC_PRESCALE;  // Start, single conversion 1100 1000
    #else
       ADCSRA = 0xE8 | USER_ADC_PRESCALE;  // Start, auto conversions 1110 1000
    #endif
}

*/
