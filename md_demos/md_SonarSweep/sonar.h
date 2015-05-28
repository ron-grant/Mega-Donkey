/*

  SR04 Sonar Module Support


  This provides a demonstration of Timer Input Capture (using 16 bit Timer 3)


  This code will most likely be incorporated into Mega Donkey Library and thus "donkey.h" will
  include "sonar.h". In such case, Interrupt functions used by sonar.c would be defined within macros
  in sonar.h to allow user freedom to define their
  own interrupt functions.
  


  Copyright (c) 2007,2008 by Ron Grant



  all sonar modules echo pulse must be fed to PE7 (ICP3) 
  inputs should be logically ORed  (POSSIBLY WIRE ORed -- not sure about hardware specs)

  This code supports SR04 model
  10 us trigger on designated trigger pin (normally low, high for 10us on trigger)

  start timing echo on low to high input on PE7
  stop timing echo on high to low transition on input PE7


  see: md_SonarSweep.c  for demo project using this code

*/


#define SONAR_MAX_CHANNELS 8


u08 SonarPortBit[SONAR_MAX_CHANNELS];   // port|bit   0PPP0BBB  
int SonarDist[SONAR_MAX_CHANNELS];      // Distance in inches


                         // Supports Deventech SRO4 sonar module
void sonar_init(void);   // set up timer 3 to trigger and acquire data
                         // sets up periodic interrupt  20Hz (default 50mSec)
                                                 // interrupt on echo start and end
                                                 // writes distances to Sonar array


void sonar_set_channel_ms (u16 ch_time);  // set channel time in mSec
                                          // call after sonar_init to change default


void sonar_stop(void);   // stops timer3 and interrupts

void sonar_def (u08 ch, char port1, u08 bit1);  // assigns port and bit to trigger for
                                                // given sonar channel
                                                // channel should be range 0..SONAR_MAX_CHANNELS-1
                                                // e.g. 'E',6   for PE6
                                                     

