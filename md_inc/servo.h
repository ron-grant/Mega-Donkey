/*  MegaDonkey Library File:  servo.h    RC Servo Support - Header File
    


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


    Mega Donkey RC Servo Control  
    Ron Grant
    April 2007


    Assume 16 MHz CPU clock
   
    This module is subject to jitter on output pins due to other interrupt routines executing
    e.g. timer0, uart and ADC service routines
   
    Direct timer synthesis of two servo channels is possible with 16 bit timer.
	This support is offered by another module.



   Control 8 RC servos
   Trapezoidal motion profile available to move from one position to another allowing
   variable speed rotation rotational including acceleration/deceleration 
   
   positions may be set directly

   once a move is initiated it will execute without any direct intervention

   query of motion complete status is provided by inspecting motion ticks counts


   Timer1 is programmed to count at 4us per pulse using /64 cpu clock prescale.


   prescale /64 gives 4us resolution

   alternative would be /8 for 0.5 uSec resolution
   this might be a good choice if output compare pin enabled and fed to demux where each servo would receive "exact"
   pulse time. In this implementation, servo pulse bits are set manually within interrupt routine which 
   has variable interrupt latency issues to contend with.

   CurChannel would need to be written to select address line pins


servo_  functions:  basic low level control
svs_    functions:  define a high level framework to assist in scheduling events
                    see: svs_demo()  for example code



   Revisions 

   12/13/2007  added code to servo_def to set port bit to output
               also changed default setting to 125, for pulsewidth of 1.5 ms


*/


#ifndef _SERVO_H
#define _SERVO_H

#include "md.h"

#ifdef SERVO_CODE

#define SERVO_CHANNELS 8 // number of available channels 
                         // not all need to be defined
                         // altering this value from 8 would require consideration for 
                         // running servo frame that is longer/shorter than 20 ms
                         // approx 2.5 mSec x 8 = 20 mSec


// # fractional bits
// for position,velocity and acceleration

#define SERVO_FRAC_BITS 9



typedef struct {                        
  char dport;           // servo port e.g. 'B'    future: may become port address
  u08  dbit;            // servo port bit         0..7 

  unsigned long pos;    // position    fixed point  e.g. 10.6   #FRAC_BITS defined in servo.c
  int vel;              // velocity      
  int acc;              // accel     

  u08 acc_ticks;        // acceleration steps          any of these can be set to zero for a simplified 
  int run_ticks;        // run constant speed steps    motion profile e.g. acc/dec = 0 for constant speed 
  u08 dec_ticks;        // deceleration steps          move not "easing" from or to a position

} servo_type;


servo_type Servo[SERVO_CHANNELS];   // Servo Data
                        

unsigned int ServoMinTime;   // set to 1 mSec (250 with 4us count rate) in servo_init()
                                    // can be overridden after init

void servo_init(void);       // set up timer and servo defaults
void servo_stop(void);       // stops output to servos (stops timer and interrupts)



void servo_def (u08 ch, char  port1, u08 bit1); // define port and bit used by servo
                                                // for given channel 0.. SERVO_CHANNELS-1
												// port bit is set to output
 		                                                                                   // e.g. port 'A' 'B' ...

// query motion control to see if move is complete.
// if acceleration rates reasonable and velocities not too great, servo will be able to keep up
// and this function will provide good indication that servo has finished its move.

int servo_move_is_complete (u08 ch);


// hard set position -- servo runs to destination as fast as possible
// servo_is_move_complete(ch) would not yield any useful information if called after this call
// position is whole number here (not fractional)
// 0 = 1.0 ms  125 = 1.5 ms  250 = 2.0 ms
// 125 is default position set when servo_def called

void servo_pos (u08 ch, long position); 


// use trapezoidal motion profile to move to a target position 
// each step is 20 mSec (1/50th of a second)
// 5.1 seconds max time to accel and decel -- symmetrical profile 
// 65535 steps (21 minutes MAX) for runtime to target
//   -- need to verify if that range is reasonable without going to extended precision arithmetic
//      in position calc -- i.e. 32 bits

// NOTE: at present position error results if using acceleration 
// setting ramp_ticks to 0 disables acceleration starting with immediate demand for servo to run
// at computed max velocity (rectangular motion profile)


void servo_moveto (u08 ch, int target,unsigned int total_ticks, u08 ramp_ticks);



/* svs Servo Sequencer -------------------------------------------------------------------------------
                 
   svs provides framework that allows servo events to be placed in what is referred to as a "state string"
   e.g. "A      B     C D E"
 
   each character position is one svsTick which by default is 50 servo updates (50 x 20 ms = 1 sec)
   Changing svsQuanta allows speeding up or slowing down sequencer. e.g. setting svsQuanta = 10 would
   increase rate to 1/5th of a second per tick (per character position in state string).
   
   svs_getstate is used to retreive current state which can be presented to a switch statement for 
   processing -- see svs_demo() for example code. 

*/

 u08 svsQuanta;      // number of servo updates per svsTick
                           // default is 50 =  (50 x 20 ms = 1 second per svsTick

 u16 svsTick;        // current position in state string
 u08 svsRun;         // pause run flag for servo sequencer 1=Run 0=Pause 

 u08 svsState;        // calling svs_seq,   e.g. svs_seq("A  B  C")   current state -> svsState
                            // program should use destructive read, svs_getstate to access.
                                              
 u08 svsGoto;         // set to force state on next call to svs_seq
 char *svsStateStr;   // pointer to string defined by svs_seq(), allows other functions to access
                            // state string

void svs_init(void);        // initialize sequencer, reset states..
u16  svs_get_tick(void);    // get current tick, equal to character position in state string
void svs_seq (char s[]);    // sequence that Sequencer is following -- this must be called 
char svs_getstate(void);    // get state with destructive read
void svs_goto(char c);      // jump to state
void svs_moveto (u08 ch, int target, char EndState); // move to target position defined by position
                                                     // of EndState char in state string

                            // when using donkey code library the following functions
							// are not defined
							// use, separate demo files instead  

#ifdef SERVO_DEMO
void servo_demo(void);      // low level servo demo
void svs_demo(void);        // Demo of servo sequencer framework
#endif // SERVO_DEMO


#endif // SERVO_CODE

#endif // _SERVO_H
