/*  MegaDonkey Library File:  timer.h     System Timer (using timer0)  - Header File


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



 Ron Grant 2005
 Mark Sims 2007  -- Time of day and improved delay functions
 Ron Grant 2007  -- Slowed interrupt rate to 10KHz


 timer.c   Mega Donkey Timer Support

 Provides system timer support and some helper functions for timer #1 and timer #3.

 Timer 0 is the default system timer, initialized by call to timer0_init(). 
 Normally timer 0 operates generating a 10KHz periodic interrupt used for real time clock,
 delay routines, speaker routines and LED PWM dimming.
 Also, Timer 0 interupt can be hooked into user routine.  
 
 Note there are a few differences between Mega 128 and Mega 256x processors.
 At present, symbol, _AVR_IOM2561_H_ is used to determine if compiling for 256x processor.


 12/05/2007 RG corrected timer1_set_output_compare_int_ABC and
                         timer3_set_output_compare_int_ABC bit mapping for MEGA2561 

 02/06/2009 RG corrected timer1_set_input_capture interrupt enable for MEGA128


*/



#ifndef timer_H
#define timer_H

#include "md.h"
#include "adc.h" // for call of timer_10KHz_update

#ifdef TIMER_CODE


// Timer0 running with periodic interrupt rate of 10 KHz is standard for Megadonkey

#define TIMER0_10KHZ

#ifdef TIMER0_10KHZ
  #define TIMER0_FREQ 10000   // timer 0 interrupt rate in HZ  (CPU Clock 16 MHz)
  #define K_100Hz 10        
  #define K_1KHz  10
#else
  // older rate -- not used now
  // this tends to eat too much time + 10KHz is nice round number
  // see also timer.c code
  #define TIMER0_FREQ 62500   // timer 0 interrupt rate in HZ  (CPU Clock 16 MHz)
  #define K_100Hz 62        
  #define K_1KHz  62          // msecs are a little off
#endif

/* define this if you want better leap year code and count of time alive wraps */
// #define ANAL_ABOUT_TIME  


// use this macro definition to enable support for a chain of
// user timer routines called by the main timer tick interrupt handler

// #define USER_TIMER_FUNCTION fire_timer();

#ifdef USER_TIMER_FUNCTION
// void USER_TIMER_FUNCTION
#endif


#ifdef TIME_CLOCK
void getdt(void);
void setdt(void);

struct TIME {  // either updated by timer interrupt to loaded/set from hardware time chip
   u08 secs; 
   u08 mins;
   u08 hours;

   u08 day;
   u08 month;
   u16 year;

   u08 weekday;
   int adjust;   // clock speed adjust in msecs/hr
}  volatile time;

u08 day_of_week(u08 d, u08 m, int y);     /* 0 = Sunday */

void set_time(void);
#endif



 u08 beep_disabled; // flag set to disable speaker

 u16 last_rand;
u16 rand16(void);         // Random Number - non-determinstic

 void (*service_10KHz)(void);  // pointer to current 10KHz service routine
 void (*service_100Hz)(void);  // pointer to current 100Hz service routine
                                     // see example on how to link additional service
                                     // routines into chain

#define delay_100ths(d) delay_msecs(d*10)
#define get_time_alive_100ths(t) (get_msecs_alive() / 10L)
#define delay_ms(d) delay_msecs(d)

void reset_time_alive(void);
unsigned long get_msecs_alive(void);
unsigned long get_time_alive_secs(void);
void delay_msecs(unsigned long time);  // wait (multiples of 1ms)

void beep(int dur, int freq );  // dur 1/100ths
void beep_wait(void);           // wait for beep to complete
u08 beeping(void);              // returns 1 if beep in progress
void speaker_tick(void);        // toggle speaker bit

void timer0_init(void);         // timer0 required for beep, time alive and delays

#define timer0_restart() {TCCR0CS |= (1<<CS01);}  // CS01=1  Prescale CPU Clock / 8
#define timer0_stop()    {TCCR0CS &= 0x78;}       // Prescaler = 0 = stop  0111 1000


/*

 Timer 1 Waveform Modes (Timer 3 identical except OCR3A,ICR3... )

 B=Bottom T=Top M=MAX I=Immediate
 ctc = clear timer on compare match

 waveform modes                 TOP   Update_OCR1x  TOV1_ Flag Set
  0 normal                      FFFF     I          M 
  1 pwm 8  bit                    FF     T          B 
  2 pwm 9  bit                   1FF     T          B
  3 pwm 10 bit                   3FF     T          B
  4 ctc                        OCR1A     I          M   (used for rc servos)
  5 fast pwm 8                    FF     B          T
  6 fast pwm 9                   1FF     B          T
  7 fast pwm 10                  3FF     B          T
  8 pwm phase & freq correct    ICR1     B          B 
  9 pwm phase & freq correct    ORC1A    B          B
 10 pwm phase correct           ICR1     T          B 
 11 pwm phase correct           OCR1A    T          B 
 12 pwm ctc                     ICR1     I          M
 13 x                          
 14 fast pwm                    ICR1     B          T
 15 fast pwm                    OCR1A    B          T



 compare output modes  non-pwm

 0 output off
 1 toggle on match
 2 clear on match
 3 set on match

 
 servo.c and sonar.c provide example usage of timer1 and timer3 helper functions

*/


// timer 1 helper functions & variables (pointer to service routines)

void timer1_set_prescale(u08 n);                              // prescale 1=1 2=8 ...
void timer1_set_waveform(u08 n );                             // 1 of 16 operating modes

void timer1_set_output_compare_ABC(u08 a, u08 b, u08 c);     // don't take over any pins with OC1x signals 
void timer1_set_output_compare_int_ABC (u08 a,u08 b,u08 c);  // output compare interrupt enables
                                                             // 0=disable 1=enable
// user timer service routine pointers
// assign these to used service routine
// e.g. user_output_compare1a = my_output_compare1a;
// where prototype should be:  void my_output_compare1a(void)
// see servo1.c for example code

// PROBLEM HERE -- NEED TO USE FAR POINTERS
// reconsidering using indirects
// thinking about building declaration of SIGNAL(  ) into say servo_init that uses timer1
// thus if you don't use servo code you can define your own signal routine OR
// use same type of method used 

//void (*timer1_output_compareA) (void);    
//void (*timer1_output_compareB) (void); 
//void (*timer1_output_compareC) (void); 


void timer1_set_int_overflow (u08 overflow);                 // 0=disable 1=enable



//  Enabled Timer 1 Input Capture 
//  calls user interrupt functionn  SIGNAL(SIG_INPUT_CAPTURE1)  
 
static inline void timer1_set_input_capture(u08 NoiseCan, u08 PosEdge) // enable interrupt
{
     TCCR1B = (TCCR1B & 0x3F) | (NoiseCan<<8) | (PosEdge <<7);

   #ifdef _AVR_IOM2561_H_
     sbi (TIMSK1,ICIE1);     // mega 256x
   #else  
      sbi(TIMSK,TICIE1);    // mega 128      was ETIMSK  fixed 2/6/2009
   #endif
}	                 

static inline void timer1_stop_input_capture(void)  // clear input capture 1 interrupt enable
{
   #ifdef _AVR_IOM2561_H_
     cbi (TIMSK1,ICIE1);       // mega 256x
   #else  
   	 cbi (TIMSK,TICIE1);      // mega 128     was ETIMSK fixed 2/6/2009
   #endif
}




// timer 3 helper functions & variables (pointer to service routines)

void timer3_set_prescale(u08 n);                             // prescale 1=1 2=8 ...
void timer3_set_waveform(u08 n );                            // 1 of 16 operating modes

void timer3_set_output_compare_ABC(u08 a, u08 b, u08 c);     // don't take over any pins with OC1x signals 
void timer3_set_output_compare_int_ABC (u08 a,u08 b,u08 c);  // output compare interrupt enables
                                                             // 0=disable 1=enable

void timer3_set_int_overflow (u08 overflow);                 // 0=disable 1=enable


  
//  Enabled Timer 3 Input Capture 
//  calls user interrupt functionn  SIGNAL(SIG_INPUT_CAPTURE3)  

static inline void timer3_set_input_capture(u08 NoiseCan, u08 PosEdge)  // enable interrupt  
{
   TCCR3B = (TCCR3B & 0x3F) | (NoiseCan<<8) | (PosEdge <<7);

   #ifdef _AVR_IOM2561_H_
     sbi (TIMSK3,ICIE3);     // mega 256x
   #else  
   	 sbi (ETIMSK,TICIE3);    // mega 128
   #endif
}	                 

static inline void timer3_stop_input_capture(void) // clear input capture 3 interrupt enable
{
   #ifdef _AVR_IOM2561_H_
     cbi (TIMSK3,ICIE3);       // mega 256x
   #else  
     cbi (ETIMSK,TICIE3);      // mega 128
   #endif
}







#endif // TIMER_CODE
#endif // timer_H
