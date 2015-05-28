/*  MegaDonkey Library File:  servo.c    RC Servo Support
    


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


*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "md.h"

#ifdef SERVO_CODE
#include "lcd.h"
#include "servo.h"
#include "timer.h"
#include "adc.h"
#include "led.h"


u08 CurChannel;  // current channel 0..SERVO_CHANNELS-1 
                 // auto round-robin advanced every 2.7 mSec by interrupt routine



void servo_output_compareA(void); // forward declare - service routine called from SIGNAL
void servo_output_compareB(void); // routine in timer.c


void servo_init(void)
{
u08 i;

    cli(); 

    for (i=0; i<SERVO_CHANNELS; Servo[i++].dport = 0);  // disable all channels

    timer1_set_prescale (3);   //  3 = clk/64 = 4us per count
    timer1_set_waveform (4);   //  4 = CTC (count 0 to OCR1A then reset)  

    timer1_set_output_compare_ABC(0,0,0);       // don't take over any pins with OC1x signals 

    // enable interrupts on output compare A and B of timer1
	
	timer1_set_output_compare_int_ABC (1,1,0);  // A B


    OCR1A = 625;        // interrupt every 2500us = 2.5 mSec      (2500/4=625)   (16 MHz clock)
              
    CurChannel = 0;

    ServoMinTime = 250; // at 4us per count  250x4 = 1.0 mSec

    sei();
}

void servo_stop(void)
{
   timer1_set_prescale(0);  // stop timer and all associated interrupts
   timer1_set_output_compare_int_ABC (0,0,0);  // disable interruts -- extra measure
}



void servo_def (u08 ch, char port1, u08 bit1)
{   
   if (port1 >= 'a') port1 -= 32;  // shift to uppercase

   Servo[ch].dport = port1;
   Servo[ch].dbit  = bit1;

   // set given port bit to output

   switch (port1) {
      case 'B' : sbi (DDRB,bit1); break; 
      case 'E' : sbi (DDRE,bit1); break; 
      case 'D' : sbi (DDRD,bit1); break;
      case 'F' : sbi (DDRF,bit1); break;
      case 'A' : sbi (DDRA,bit1); break;
      case 'C' : sbi (DDRC,bit1); break;
      case 'G' : sbi (DDRG,bit1); break;
   } 

   servo_pos (ch,125); // neutral position
}

 


// query motion control to see if move is complete.
// if acceleration rates reasonable and velocities not too great, servo will be able to keep up
// and this function will provide good indication that servo has in fact finished its move.

int servo_move_is_complete (u08 ch)
{ 
   return (!(Servo[ch].acc_ticks + Servo[ch].run_ticks + Servo[ch].dec_ticks)); 
}


// hard set position -- servo runs to destination as fast as possible
// servo_is_move_complete(ch) would not yield any useful information if called after this call


void servo_pos (u08 ch, long position) 
{ 
   cli();

   Servo[ch].acc_ticks = 0;
   Servo[ch].dec_ticks = 0;
   Servo[ch].run_ticks = 0;
  
   Servo[ch].pos = position << SERVO_FRAC_BITS;   // 1ms + position  in 10.6 fixec point format

   sei();
}



#define iabs(n) ((n)<0?(-(n)):(n))

/*
   Use trapezoidal motion profile to move servo shaft through an angular distance
   (from current position to target position)
    
   Velocity starts and ends at 0 with symetrical acceleration and deceleration ramps

   Ramp and run time is described in ticks. The tick time is not part of the solution except 
   it can be used to compute actual ramp and run times, e.g. 100 ticks = 2 seconds.
      _______ 
     /        \      ^ vel 
    /          \     | 
   /            \    +--> t
   
   equations of motion are realized by discrete approximation.

   s = vt

   dist = total area under trapezoidal curve (total_ticks x vmax)


   5.1 seconds max accel and decel time at 20 mSec per tick
  

   very long run times would require more precision, that is, more fractional bits


   NOTE: At present, problems with error when using velocity ramping
         SET ramp_ticks to 0 for non-ramped motion

*/

void servo_moveto (u08 ch, int target, unsigned int total_ticks, u08 ramp_ticks)
{  
long vmax,dp;
servo_type *p;

   p = &Servo[ch];

   
   cli(); // disable interrupts

   dp = target; 

   dp = (dp << SERVO_FRAC_BITS) - p->pos;

   
   p->vel = 0;  // initial velocity

   // limit ramp time to total_time

   if(total_ticks < (ramp_ticks*2))
    ramp_ticks = total_ticks >> 1;  
        
   p->acc_ticks = ramp_ticks;
   p->dec_ticks = ramp_ticks;

   p->run_ticks = total_ticks - (ramp_ticks <<1);

   
   // given the needed delta position and motion profile, compute an acceleration rate that
   // will accomplish the move


   // dist = area under trapezoid = base * height = total_ticks * vmax.
   // solving for vmax and scaling to fixed point W.F format where F=SERVO_FRAC_BITS
   // e.g. F=6   1/64ths 

   // sign preserved to get proper rotation direction sense

   vmax = dp / total_ticks;  

   // accel needed to realize vmax within ramp_ticks time
   // OR if ramp_ticks=0 then set velocity to vmax immediately  

   if(ramp_ticks) {
      p->acc = (vmax / ramp_ticks);   // acc rate (10.6 format)
   }
   else {
     p->acc = 0;
     p->vel = vmax;
   }                    


   sei(); // re enable interrupts
}


// svs Servo Sequencer ------------------------------------------------------------------------------
// see: servo.h for some explaination, also svs_demo()
 

u08 svsQuantaDiv;             // quanta divider counter  used by program 


void svs_timer_update(void)   // called by timer interrupt at start of servo frame
{  
   if(!svsRun) return;      // that is when servo channel 0 is being processed
                            // Note: this function is not accessed externally
   if(!svsQuantaDiv--) {       
      svsQuantaDiv = svsQuanta;
      svsTick++;
   }
}


u16 svs_get_tick() 
{ 
u16 t;

   cli();
   t = svsTick;    // protect acquisition of current tick from interrupt modification
   sei();

   return t;
}


void svs_init(void)
{
   svsQuanta = 50;    // default 1 second per svsTick  (50 = 50*20 mSec = 50/50th sec = 1 sec) 

   svsRun    = 0;     // disable increment of svs timer ,alternatively could do cli() / sei() 
   svsTick   = 0;     // time incremented a start of servo processing loop
   svsRun    = 1;     // enable svs timer

   svsGoto   = 0;     // reset goto state 
   svsState  = 1;     // state processed flag 
}


//
// sequence that Sequencer is following -- this must be called 
// on each iteration of the processor
//
void svs_seq(char s[]) 
{
u08 i;

   svsStateStr = s;
   
   // !!! might be helpful to insure state is not missed 

   // goto processor
   if(svsGoto) {
      i = 0;
      while((s[i] != svsGoto) && s[i]) i++;  // find state

      cli();
      svsTick = i;  // set state or 0 if end of string encountered
      sei();

      svsState = svsGoto;    
      svsGoto  = 0;

      return;
   }

   if(svsState == 1) {
      svsState = s[svsTick];
   }

   // note that spaces in svs seq are returned and can be used for dead time processing
   // see demo code
}



char svs_getstate(void)  // get state with destructive read
{
u08 s;

   s = svsState;
   if(svsState) svsState = 1;

   return s;
}


void svs_goto(char c)
{
   svsGoto = c;
}


void svs_moveto(u08 ch, int target, char EndState) 
{
u08 i;
char *s;
int t;

   s = svsStateStr;

   if(EndState == ' ') {
      servo_pos (ch,target);  // no end state specified --  direct goto position as fast as possible
   }
   else {
      i = 0;
      while(*s++ != EndState) i++;
   
      t = (i - svs_get_tick()) * svsQuanta;
  
      servo_moveto(ch,target,t,0);    // channel,target,time,ramp time (ramp time must be 0 for now)
   }

#ifdef SVS_DIAGNOSTIC_PRINT
   lcd_setxy(0,50);
   printf (PS("ch %d to %3d in %3d"),ch,target,t);
#endif
}




// OC1A Interrupt generated when timer counts up to OC1A value (2.7 ms),
// time to switch to next servo channel and set output bit for that servo
// if channel is defined
// 
// Have not figured out how to set port directly -- maybe could do with inline asm code
// so relying on case statement for spec. of which port to use.
//
// Ideally would like to put port address in Servo[].dport then set without switch statement
//
// As it is now on Megadonkey, ports B and E would be most popular choices for servos,hence listed 
// first in switch statement for efficiency
//
// note: pointer p used mainly for purpose of abbreviating references to current servo
// Servo[CurChannel] 


void servo_output_compareA(void) // called by timer SIGNAL function (see servo_init)
{
servo_type *p;
u08 b;


   if(++CurChannel >= SERVO_CHANNELS) {  
      CurChannel = 0;        // next channel, wrap around to zero
      svs_timer_update();    // update sequencer if enabled
   }

   p = &Servo[CurChannel];            
   b = p->dbit;

   // if servo channel defined then process it
   // otherwise nothing happens until next OC1A interupt in 2.7 ms
   if(p->dport) {   
      switch (p->dport) {
        case 'B' : sbi (PORTB,b); break; // set servo pulse high
        case 'E' : sbi (PORTE,b); break; 
        case 'D' : sbi (PORTD,b); break;
        case 'F' : sbi (PORTF,b); break;
        case 'A' : sbi (PORTA,b); break;
        case 'C' : sbi (PORTC,b); break;
        case 'G' : sbi (PORTG,b); break;
      } // end switch

      OCR1B = ServoMinTime + (p->pos >> SERVO_FRAC_BITS);   // set high time for bit    10.6  fixed point    4us resolution whole part   
 

      // now process trapezoidal motion profile   

      if(p->acc_ticks) {         // accelerate
         p->acc_ticks--;
         p->vel += p->acc;
         p->pos += (p->vel);  
      } 
      else if(p->run_ticks) {   // run constant speed
         p->run_ticks--;
         p->pos += (p->vel);
      }   
      else if(p->dec_ticks) {   // decelerate
         p->dec_ticks--;
         p->vel -= p->acc;
         p->pos += (p->vel);
      } 
   } 
}


void servo_output_compareB()
{
u08 b;

   b = Servo[CurChannel].dbit; 

   switch (Servo[CurChannel].dport) {
      case 'B' : cbi (PORTB,b); break;     // set servo output low (end of pulse)
      case 'E' : cbi (PORTE,b); break; 
      case 'D' : cbi (PORTD,b); break;
      case 'F' : cbi (PORTF,b); break;
      case 'A' : cbi (PORTA,b); break;
      case 'C' : cbi (PORTC,b); break;
      case 'G' : cbi (PORTG,b); break;
   }
}

#ifdef SERVO_DEMO

// --------------------------------------------------------------------------------------------------
//
//  low level servo demo
//
// --------------------------------------------------------------------------------------------------
void servo_demo(void) 
{
int target;
long tend, tstart;
long f, t;

   servo_init();

   servo_def(0,'B',7);
   servo_def(1,'B',6);  // used in last half of demo

   delay_ms(1000);
   lcd_clear();
   printf(PS("Servo Demo"));

   servo_pos(0,0);
   led_set(3, LED_ON);

   delay_ms(2000);
   led_set(2, LED_OFF);
 
   // 400 ticks is 8 seconds

   target = 300;

   servo_moveto(0,target,100,0);  // ch,target, total time 200=4 sec total , ramp time 1 sec up 1 sec dn
   tstart = get_time_alive_100ths();
 
   do { 
       t = Servo[0].pos >> SERVO_FRAC_BITS; // report position
       f = Servo[0].pos & ((1<<SERVO_FRAC_BITS)-1); // fractional bit  bitmask   e.g 3   00000111

       lcd_setxy(0,12);
       printf(PS("pos %4ld.%4.4ld"),t,f);
   } while (!servo_move_is_complete(0));

   tend =  get_time_alive_100ths()-tstart;

   printf(PS("\nexpected pos %4d"), target);
   printf(PS("\n\ntime %4ld"), tend);
   printf(PS("\n acc %d "), Servo[0].acc);

   beep(50,3000);
   led_set(2, LED_ON);

   wait_until_touched();

   lcd_clear();
   lcd_textPS(0,0,"Touch Servo XY Demo");

   delay_ms(2000);

   touchX = 100;

   while(touchX > 10) {
      get_touch(3);

      t = 2 * touchX;
      if(t > 255) t = 255;

      if(t > (-1)) {
        lcd_setxy(0,12);
        printf(PS("%4ld"),t);
         
        servo_pos(0,t);
      }

      t = touchY;

      if(t > (-1)) {
        lcd_setxy(0,24);
        printf(PS("%4ld"),t);

        servo_pos(1,t);
     }

     led_set(2, LED_ON);
     delay_ms(15);
     led_set(2, LED_OFF);
     delay_ms(15);
   }
}
#endif // SERVO_DEMO


#ifdef SVS_DEMO
// --------------------------------------------------------------------------------------------------
//
// Demo of servo sequencer framework
// this builds on basic servo control 
//
// --------------------------------------------------------------------------------------------------
void svs_demo(void)
{
char st;

  lcd_clear();
  lcd_setxy(0,0);
  printf(PS("SVS Servo Demo"));

  servo_init();

  DDRB = 0xFF;   // all output
  
  servo_def(0,'B',6);  // channel port bit
  servo_def(1,'B',7);
  servo_def(2,'B',5);

  servo_pos(0,0);
  servo_pos(1,0);
  servo_pos(2,0);

  delay_ms(1000);

  
  svs_init();
  svsQuanta = 20; // default = 50 = 1 second

  while(!get_touch(1)) {
     // Note: In this sequence, motion control states are in UPPERCASE
     // some of the servo motion termination states are written lowercase
     // and are not processed within the switch statement, but are located
     // by the svs_moveto function.

     svs_seq("A  B a C b D    E   F");

     st = svs_getstate();

     // diagnostic display of state and tick
     lcd_setxy(0,12);                       
     if(st != ' ') printf(PS("state %c at %d"),st,svs_get_tick());
    
     lcd_setxy(0,24);
     printf(PS("tick %4d "),svs_get_tick());

     if(st != ' ') led_set(3, LED_OFF);      

     switch (st) {
       case 'A': svs_moveto (0,350,'a'); // servo channel 0 move to 350 at the time 'a' is encountered  
                 break;                  // in svs_seq() state string

       case 'B': svs_moveto (1,350,'b');
                 break;
                         
       case 'C': svs_moveto (2,350,'D');
                 break;

       case 'D': svs_moveto (0,0,'E');
                 svs_moveto (1,0,'E');
                 break;

       case 'E': svs_moveto (2,0,' '); // example of no state where motion is targeted to complete
                 break;                // this results in instant set of target servo pulse width
                                       // causing servo to travel at full speed to that target
       case 'F': svs_goto('A');
                 break;

       case ' ': 
                 led_set(3, LED_ON);   // optional processing when no servo state events happening
                 break;
  
    } // end switch

  }

}  // end of SVS demo 

#endif // SVS_DEMO

#endif // SERVO_CODE
