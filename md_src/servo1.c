/*  MegaDonkey Library File:  servo1.c    Jitter Free Servo Support using timer 1 for 1 or 2 servos 
    


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



   servo1.c   Mega Donkey   Hi-Res Servo Support
   Ron Grant
   December 2007
   March 3, 2008 integrated into donkey library, created macro for defining SIGNAL function in user 
                 program and not in library which application writer freedom to write their own
				 Interrupt service function without re-compile of library code.


   see header file for general comments

*/
#include "md.h"     // do this to get SERVO1_CODE def


#ifdef SERVO1_CODE  // always defined with library compile

#include "donkey.h"  // needed for SERVO1_CODE def... 
#include <avr/interrupt.h>
#include "timer.h"       // timer helper routines

u16 Servo1Min;     // min value default 1000 = 1 msec pulse width
u16 Servo1Max;     // max value default 2000 = 2 msec pulse width
u08 Servo1Div;     // divider counter default 8 x 2.5 msec = 20 msec
u08 Servo1UseMUX;  // flag indicating servo data is serial

s08 CurChannel1;


void servo1_initX(u08 UseMUX)  // sets up servo controller and PB6 and PB7 as output
{

  u08 i;

  Servo1UseMUX = UseMUX; 

  if (Servo1UseMUX)
    for (i=0; i<8; Servo1Pos[i++]=500);  // center all 8 servos


  cli();                     // mask interrupts

  timer1_set_prescale (2);   //  2 = clk/8 = 0.5us per count @ 16 MHz

  timer1_set_waveform (9);   //  4 = CTC (count 0 to OCR1A then reset)  
                             //  9 = PWM Phase and Freq Correct TOP=OCR1A

  // Set Compare Output Modes for each of three Compare Registers A,B and C 
  // compare output modes (non-pwm) are:
  // 0 output off
  // 1 toggle on match
  // 2 clear on match
  // 3 set on match

  // for waveform 9   compare output 2 = non-inverted pwm   3= inverted 
  // see interrupt routine
  
  timer1_set_output_compare_ABC(0,2,2);   // take over pins with OC1B,OC1C 

  // using macro SERVO1_UPDATE defines a interrupt service SIGNAL function which interrupts
  // when TCNT1 counts down to 0.
  // interupt on count to OCR1A to allow masking of pulses for N time 2.5 msec
  // alternately updating channel (servo pulses time division multiplexed)

  timer1_set_int_overflow(1);  


  OCR1A = 2500;   // interrupt every 2500us = 2.5 mSec      (2500/0.5=5000)

 
  sbi(DDRB,6);  // set PB6 (OC1B) as output pin
  sbi(DDRB,7);  // set PB7 (OC1C) as output pin

  cbi(PORTB,6); // pin low when port not taken over 
  cbi(PORTB,7); // this is important because 7/8ths of the time timer control
                // of pins is released and we need to see output pins set low
				// to fulfill requirement for dead time between servo pulses

  servo1_set_range(1000,2000);
  servo1_set_interval(8);

  CurChannel1 = 1;

  sei();         // enable interrupts


}


                          
// optional settings -- called after init

void servo1_set_range(int min, int max) // default is 1000 to 2000  
{
  Servo1Min = min;
  Servo1Max = max;
}



void servo1_set_interval(u08 counts)    // counts x 2.5 ms, default 8x2.5 = 20ms = 50 Hz
{
  Servo1Div = counts;
}





// functions to set pulsewidth
// for default rage 1000 to 2000 (1.0 to 2.0) the values are 0 to 999 (in microseconds)
// values outside that range are clipped


void servo1_set_pos (u08 channel, int pos)
{
 
  if (pos<0) pos = 0;
  pos += Servo1Min;

  if (pos>Servo1Max) pos = Servo1Max;

  pos = OCR1A-pos;

  if (Servo1UseMUX) Servo1Pos[channel]=pos;
  else
  { if (!channel) OCR1B = pos;
    else          OCR1C = pos;
  }
}


 
void servo1_stop(void)         // stops timer and interrupts
{                              // leaves PB6 and PB7 as output 0 (driven low)
  timer1_set_prescale (0);     // stop timer
  timer1_set_int_overflow(0);  // disable overflow interrupt (not really needed with timer shutdown)

}

/* OC1A Interrupt generated when timer counts up to OC1A value (2.5 ms),
   time to switch to next servo channel 

   if NOT ServoMUX
   want to mask output for 7 of 8 servo time frames to maintain speced update rate 

   otherwise we setup pulse width for each of 8 channels 
   then mask the 9th


*/

// 9/18/2009 RG renamed clr_oc and set_oc macros due to conflict with
// servo3.c (I believe)

#define clr_oc1() TCCR1A = TCCR1A & 3
#define set_oc1(b,c)  TCCR1A = (TCCR1A & 3) | ((b & 3)<<4) | ((c & 3)<<2)



// Interrupt Service Function
// This is called by SIGNAL function defined as macro in header which must be included in user program

void servo1_timer1_overflow(void)
{
  if (Servo1UseMUX)            // Serialized Servo Data output 
  {  if (++CurChannel1 > Servo1Div)
     {  CurChannel1 = 0;           // Servo1Div expected to be 9 or greater
	                              // to insure adequate dead time on output
	    set_oc1(3,3);              // enable  -- ANOTHER option is to dedicate one channel to very short 
		                          // pulse to be recognized as a sync pulse.
     }

     if (CurChannel1<=7) OCR1B = Servo1Pos[CurChannel1];  // set PWM pulse width 
	 else if (CurChannel1==8) clr_oc1();                  // disable output set_oc1(0,0,0);

  while(1) { clr_oc1(); }
     
  }
  else // single channel per output pin
  {
     if (!(--CurChannel1)) {
        CurChannel1 = Servo1Div;
        set_oc1(3,3);                // connect servo pulses to pin
     }
     else clr_oc1();                 // mask servo pulses from output pin (typical 7 out of 8 times)  
  }

}

#endif

// -----------------------------------------------------------------------------------------------------------------


#ifdef SERVO1_DEMO

// servo DEMO used with library code compiled as a project
// If using library code then consider using VarEditServoTimer1 project in md_demos folder


// Servo update interrupt function macro.
// This must be included in your program -- not included within any function.

SERVO1_UPDATE


int s0,s1; // servo position 


void ve_servo1_vars(void) {           // variable list (contained in stand alone function)
   
  ve_group ("Servos PB6 PB7");        // define ground (at least one variable group is required)                                      
  ve_int   ("Servo 0",s0,0,1000);     // defines range for variable s0 as 0 to 1000
  ve_int   ("Servo 1",s1,0,1000);     // 
}  



void servo1_demo(void)
{
  s0 = 500;         // assign default values corresponding to servo center 1.5ms
  s1 = 500;          


  servo1_init();               // initialize RC servo1 controller for 2 channels (uses timer1)
                               // servo1_mux_init() would be used for 8 -- requires added hardware  
  
  ve_init(ve_servo1_vars);     // initialize variable editor passing the function name 
                               // of the variable list defined above to ve_init

  while (ve_update()) {   // update variable list editor -- returns false when user closes menu [X]

    servo1_set_pos(0,s0);   // update servo positions  (used for direct to pin 1 or 2 channel mode)
    servo1_set_pos(1,s1);   

	
  } // end variable list while loop


  // program control reaches this point when user closes variable editor menu

  servo1_stop();  // shut down output to RC servos

  lcd_clear();
  printf ("That's all, servo pulses stopped");
  delay_ms(2000);

} // end servo1_demo 

#endif // end if SERVO1_DEMO defined







