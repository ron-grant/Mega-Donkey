/*  MegaDonkey Library File:  servo1.h    RC Servo Support using timer 1  for 1 or 2 servos - Header File
                                          uses dedicated pins PB6 and PB7 
										  Note: this module is nearly identical to servo3 that uses timer 3
										  with its associated dedicated pins.
    


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


   ----

   Servo1   Mega Donkey   RC Servo Support using timer 1 for 1 or 2 servos
   Ron Grant
   December 2007


   Here,Timer1 is dedicated to generating 2 RC servo channels, creating hi-res jitter free output on
   PB6 and PB7 of servo pulses from 1.0 to 2.0 ms (default range) at rate of 50 Hz (20ms from pulse to pulse)
   (default rate).


   Alternately this module can serialize 8 servo channels on PB6 (could do 8 more on PB7)
   In this case added hardware is needed to demultiplex the data for delivery to multiple servos.

   A Sync pulse scheme is needed for this case.
   


   More Details:


   Timer1 (mode9) counts up to OCR1A then back down to 0, then repeats counting
   back up.In the process when TCNT1=OCR1B the OC1B PB6 pin is set and when TCNT1
   is on the way back down the OC1B output is cleared -- with one added detail:
   The OC1B compare can be disconnected from the pin which we do most of the 
   time to keep from generating too many servo pulses.

   This allows pulses of precise width to be defined on PB6 and PB7 using Timer1 with not
   latency issues involved as would if you were setting a bit manually with the possibility
   of an interrupt taking control away from a time critical piece of code that
   would reset the bit.

   When the counter reaches 0 each time, an overflow interrupt is generated. This is the time 
   when a next/new servo channel can be selected in the case of serializing servo data for more than
   a single servo tied to an output pin.

   If only one servo is present we mask the output by disconnecting the pin from the output compare
   register for 7 out of 8 pulse times, otherwise the servo would receive a 1.0 to 2.0 ms pulse every
   2.5 ms which would cause a problem for many hobby servos to the best of my knowledge.

   The same applies to OCR1C which is connected to PB7.


   DETAILS ON OUTPUT COMPARE


   The value for the compare registers is OCRA1-Width(in microseconds).
   where the timer is running at 16 MHz / 8 or 2 MHz.

   For OCR1A=2500 the counter counts from 0 to 2500 then back down to 0 for a total of 5000 counts at 
   2MHz for 2.50 ms time 

   The difference between OCR1B and OCR1A is also doubled as far as counts go because compare match on way up
   and on way down e.g.  OCR1B=1500     2500-1500 = 1000 counts for pulse width timed at 1000 x2 = 2000 counts =
   1000 us = 1.0 ms.

   Hence OCR1B = OCR1A - Desired B PulseWidth (microsec)
   
   Same for compare reg C  (OCR1C)


   NOTE ONE CATCH WHEN IMPLEMENTING MULTI-CHANNEL SERVO OUTPUT
   when changing OCR1B and OCR1C, the changes are buffered then loaded when counter overflow 
   occurs at 0
   
   Ideally then would be good to use OutputCompareA match interrupt to change OCR1B and C values
   and if a direct address demux is used then use the overflow interupt to change MUX address because at
   that point the servo outputs are not high whereas when OutputComareA match interrupt is happening at the 
   MAX count the OCR1B and OCR1C outputs are high.

   Consult the chip manual for a very concise definition of how the timers operate.
   
  
*/

#ifndef _SERVO1_H
#define _SERVO1_H

#ifdef SERVO1_CODE

u16 Servo1Pos[8]; // current value for each of 8 servos


 
void servo1_initX(u08 UseMUX);     // sets up servo controller (timer1) for either MUX mode nor non-MUX
                                   // this function is called by servo1_init() and by servo1_mux_init()
                                   // if 1 OR 2 then PB6 and PB7 are used for output
								 							   
                                   
#define servo1_init() servo1_initX(0)        // servo init with direct output to PB6 and PB8 for channels 0,1


// not fully supported yet

#define servo1_mux_init() servo1_initX(1)    // servo init using demultiplexer tied to PB6
                                             // requires a sync pulse 
 
								 				 				     
               
// optional settings -- can be called after servo1_init is called

void servo1_set_range(int min, int max); // default is 1000 to 2000    largest max is 2500
void servo1_set_interval(u08 counts);    // counts x 2.5 ms, default 8x2.5 = 20ms = 50 Hz





// functions to set pulsewidth 
// for default rage 1000 to 2000 the values are 0 to 999
// values outside that range are clipped

void servo1_set_pos (u08 channel, int pos);   
                                            


 
void servo1_stop(void);        // stops timer and interrupts
                               // leaves PB6 and PB7 as output 0 (driven low)

                                 
void servo1_timer1_overflow(void);   // function called by SERVO_UPDATE which must have one instance placed 
                                     // in user program -- outside any functions

#define SERVO1_UPDATE SIGNAL(SIG_OVERFLOW1){servo1_timer1_overflow();}  // defines timer1 overflow function


#ifdef SERVO1_DEMO
void servo1_demo(void);
#endif


#endif 
#endif
