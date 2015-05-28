/* servo1.h

   Timer1 dedicated to 2 RC servo channels creating jitter free output on
   PB6 and PB7 of servo pulses from 1.0 to 2.0 ms (default range) at rate of 50 Hz 
   (default rate).

   Ron Grant 
   December 2007


*/

#ifndef _XSERVO1_H
#define _XSERVO1_H


void servo1_init(u08 ChanNumA,u08 ChanNumB,int *ServoData);  // see notes in .c
void servo1_sr_mode(void);   // set up shift register mode for output (call after init)
      
                          
// optional settings -- called after init

void servo1_set_range(int min, int max); // default is 1000 to 2000    max max is 2500

void servo1_set_interval(u08 counts);    // counts x 2.5 ms, default 8x2.5 = 20ms = 50 Hz

// functions to set pulsewidth
// for default rage 1000 to 2000 the values are 0 to 999
// values outside that range are clipped

void servo1_pos (u08 channel, int pos);  // for 8 channel muxed operation

 
void servo1_stop(void);        // stops timer and interrupts
                               // leaves PB6 and PB7 as output 0 (driven low)
 
#endif
