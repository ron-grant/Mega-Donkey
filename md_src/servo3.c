/*  MegaDonkey Library File:  servo3.c    Jitter Free Servo Support using timer 3 for 1 or 2 servos 
    


    Copyright 2007-2009  Mark Sims & Ron Grant


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



   servo3.c   Mega Donkey   Hi-Res Servo Support
   Ron Grant
   December 2007
   March 3, 2008 integrated into donkey library, created macro for defining SIGNAL function in user 
                 program and not in library which application writer freedom to write their own
				 Interrupt service function without re-compile of library code.

   September 18,2009  renamed macros in servo1.c and servo3.c to allow both modules to
                      be used at same time for support of 4 (or 16) servos using timer1
					  and timer3 


   see header file for general comments

*/

#include "md.h"     // do this to get SERVO3_CODE def or not def
#include "servo3.h" // try


#ifdef SERVO3_CODE  // always defined with library compile



//#include "md.h"  // needed for SERVO3_CODE def... 

#include <avr/interrupt.h>
#include "timer.h"       // timer helper routines
//#include "servo3.h"

u16 Servo3Min;     // min value default 1000 = 1 msec pulse width
u16 Servo3Max;     // max value default 2000 = 2 msec pulse width
u08 Servo3Div;     // divider counter default 8 x 2.5 msec = 20 msec
u08 Servo3UseMUX;  // flag indicating servo data is serial

s08 CurChannel3;


void servo3_initX(u08 UseMUX)  // sets up servo controller and PE4 and PE5 as output
{

  u08 i;

  Servo3UseMUX = UseMUX; 

  if (Servo3UseMUX)
    for (i=0; i<8; Servo3Pos[i++]=500);  // center all 8 servos


  cli();                     // mask interrupts

  timer3_set_prescale (2);   //  2 = clk/8 = 0.5us per count @ 16 MHz

  timer3_set_waveform (9);   //  4 = CTC (count 0 to OCR1A then reset)  
                             //  9 = PWM Phase and Freq Correct TOP=OCR1A

  // Set Compare Output Modes for each of three Compare Registers A,B and C 
  // compare output modes (non-pwm) are:
  // 0 output off
  // 1 toggle on match
  // 2 clear on match
  // 3 set on match

  // for waveform 9   compare output 2 = non-inverted pwm   3= inverted 
  // see interrupt routine
  
  timer3_set_output_compare_ABC(0,2,2);   // take over pins with OC1B,OC1C 

  // using macro SERVO1_UPDATE defines a interrupt service SIGNAL function which interrupts
  // when TCNT1 counts down to 0.
  // interupt on count to OCR1A to allow masking of pulses for N time 2.5 msec
  // alternately updating channel (servo pulses time division multiplexed)

  timer3_set_int_overflow(1);  


  OCR3A = 2500;   // interrupt every 2500us = 2.5 mSec      (2500/0.5=5000)

 
  sbi(DDRE,4);  // set PE4 (OC3B) as output pin
  sbi(DDRE,5);  // set PE5 (OC3C) as output pin

  cbi(PORTE,4); // pin low when port not taken over 
  cbi(PORTE,5); // this is important because 7/8ths of the time timer control
                // of pins is released and we need to see output pins set low
				// to fulfill requirement for dead time between servo pulses

  servo3_set_range(1000,2000);
  servo3_set_interval(8);

  CurChannel3 = 1;

  sei();         // enable interrupts


}


                          
// optional settings -- called after init

void servo3_set_range(int min, int max) // default is 1000 to 2000  
{
  Servo3Min = min;
  Servo3Max = max;
}



void servo3_set_interval(u08 counts)    // counts x 2.5 ms, default 8x2.5 = 20ms = 50 Hz
{
  Servo3Div = counts;
}





// functions to set pulsewidth
// for default rage 1000 to 2000 (1.0 to 2.0) the values are 0 to 999 (in microseconds)
// values outside that range are clipped


void servo3_set_pos (u08 channel, int pos)
{
 
  if (pos<0) pos = 0;
  pos += Servo3Min;

  if (pos>Servo3Max) pos = Servo3Max;

  pos = OCR3A-pos;

  if (Servo3UseMUX) Servo3Pos[channel]=pos;
  else
  { if (!channel) OCR3B = pos;
    else          OCR3C = pos;
  }
}


 
void servo3_stop(void)         // stops timer and interrupts
{                              // leaves PB6 and PB7 as output 0 (driven low)
  timer3_set_prescale (0);     // stop timer
  timer3_set_int_overflow(0);  // disable overflow interrupt (not really needed with timer shutdown)

}

/* OC3A Interrupt generated when timer counts up to OC3A value (2.5 ms),
   time to switch to next servo channel 

   if NOT ServoMUX
   want to mask output for 7 of 8 servo time frames to maintain speced update rate 

   otherwise we setup pulse width for each of 8 channels 
   then mask the 9th


*/

#define clr_oc3() TCCR3A = TCCR3A & 3
#define set_oc3(b,c)  TCCR3A = (TCCR3A & 3) | ((b & 3)<<4) | ((c & 3)<<2)



// Interrupt Service Function
// This is called by SIGNAL function defined as macro in header which must be included in user program

void servo3_timer3_overflow(void)
{
  if (Servo3UseMUX)            // Serialized Servo Data output 
  {  if (++CurChannel3 > Servo3Div)
     {  CurChannel3 = 0;           // Servo1Div expected to be 9 or greater
	                              // to insure adequate dead time on output
	    set_oc3(3,3);              // enable  -- ANOTHER option is to dedicate one channel to very short 
		                          // pulse to be recognized as a sync pulse.
     }

     if (CurChannel3<=7) OCR3B = Servo3Pos[CurChannel3];  // set PWM pulse width 
	 else if (CurChannel3==8) clr_oc3();                  // disable output set_oc(0,0,0);

  // ???? what is this
  while(1) { clr_oc3(); }
     
  }
  else // single channel per output pin
  {
     if (!(--CurChannel3)) {
        CurChannel3 = Servo3Div;
        set_oc3(3,3);                // connect servo pulses to pin
     }
     else clr_oc3();                 // mask servo pulses from output pin (typical 7 out of 8 times)  
  }

}

#endif

// -----------------------------------------------------------------------------------------------------------------


#ifdef SERVO3_DEMO

// servo DEMO used with library code compiled as a project
// If using library code then consider using VarEditServoTimer1 project in md_demos folder


// Servo update interrupt function macro.
// This must be included in your program -- not included within any function.

SERVO3_UPDATE


int s0,s1; // servo position 


void ve_servo3_vars(void) {           // variable list (contained in stand alone function)
   
  ve_group ("Servos PE4 PE5");        // define ground (at least one variable group is required)                                      
  ve_int   ("Servo 0",s0,0,1000);     // defines range for variable s0 as 0 to 1000
  ve_int   ("Servo 1",s1,0,1000);     // 
}  



void servo3_demo(void)
{
  s0 = 500;         // assign default values corresponding to servo center 1.5ms
  s1 = 500;          


  servo3_init();               // initialize RC servo1 controller for 2 channels (uses timer3)
                               // servo1_mux_init() would be used for 8 -- requires added hardware  
  
  ve_init(ve_servo3_vars);     // initialize variable editor passing the function name 
                               // of the variable list defined above to ve_init

  while (ve_update()) {   // update variable list editor -- returns false when user closes menu [X]

    servo3_set_pos(0,s0);   // update servo positions  (used for direct to pin 1 or 2 channel mode)
    servo3_set_pos(1,s1);   

	
  } // end variable list while loop


  // program control reaches this point when user closes variable editor menu

  servo3_stop();  // shut down output to RC servos

  lcd_clear();
  printf ("That's all, servo pulses stopped");
  delay_ms(2000);

} // end servo1_demo 

#endif // end if SERVO3_DEMO defined



