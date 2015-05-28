/*  MegaDonkey Library File:  led.c    LED Support
    


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

    
	Ron Grant / Mark Sims
	May 2007


    May be problems with LED PWM Demo
	See comments in code below



*/


#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include "md.h"

#ifdef LED_CODE
#include "lcd.h"
#include "adc.h"
#include "timer.h"
#include "graph.h"
#include "led.h"



//
// LED Pulse Width Modulation (PWM) routines.
//
// Uses 10KHz timer interrupt to control the brightness of each of the
// four LEDs via Pulse Width Modulation.  Varies the LED drive pulse width 
// in 0.1mS steps.
//
//

void (*LED_PWM_Chain)(void);  // pointer to current 10KHz service routine
u08 PWMCount;                 // 10 KHz counter 
u08 LEDPWM[NUM_LEDS];         // thresholds used by pwm controller running
                              // in timer overflow interrupt chain


void LED_PWM_Service(void) 
{
   if(LED_PWM_State & ENABLED) {
      PWMCount -= 2;   // count by two so LEDs don't flicker
      if(PWMCount == 0) {
#ifndef MOUSE_UART3    //!!!!!
         cbi(PORTD,6);   // tricky allowing individual channel enable may cost too much time
         cbi(PORTD,7);   // maybe single enable bit test  if (PWM_LED_Enabled)

#ifdef LED2_PF5
         cbi(PORTF,5);   
#endif


#endif
         cbi(PORTG,3);     
         cbi(PORTE,2);   // hard code all 4 channels OFF
#ifdef PWM_BACKLIGHT
         cbi(PORTC,2);
         LEDPWM[4] = LEDInten[4] & 0xFE;
#endif
   
         LEDPWM[0] = LEDInten[0] & 0xFE;  // update LED PWM counters 
         LEDPWM[1] = LEDInten[1] & 0xFE;  // ...force even values
         LEDPWM[2] = LEDInten[2] & 0xFE;  // ...since we count down by twos
         LEDPWM[3] = LEDInten[3] & 0xFE;  // ...so LEDs don't flicker
      }  
      else {
         if(PWMCount == LEDPWM[0]) sbi (PORTE,2);  // turn on when threshold crossed

#ifdef LED2_PF5
         if(PWMCount == LEDPWM[1]) sbi (PORTF,5);  // Alternate 
#else 
         if(PWMCount == LEDPWM[1]) sbi (PORTD,6);
#endif
         if(PWMCount == LEDPWM[2]) sbi (PORTD,7);
         if(PWMCount == LEDPWM[3]) sbi (PORTG,3);
#ifdef PWM_BACKLIGHT
         if(PWMCount == LEDPWM[4]) sbi (PORTC,2);
#endif
      }
   }

   if(LED_PWM_Chain) {  // there is another routine hooked into the 10KHz interrupt
      LED_PWM_Chain();  // ...so call it
   }
}


//
// PWM LED control routine.
// Turns on/off LED PWM control.
//
// PWM mode supports DIM leds by pulse width modulating the LED drive signals
// Simple LED control supports DIM by turning on input pullup resistor
//
void led_pwm(u08 state)
{
u08 sreg;

   if(state) {  // turn on LED PWM control
      sbi(DDRE,2);    // set all LED lines to outputs
#ifdef MOUSE_UART3  //!!!!!
      sbi(DDRD,7);
      sbi(DDRD,6);

#ifdef LED2_PF5     // LED2 alternate port assignment 
      sbi(DDRF,5);
#endif

#endif
      sbi(DDRG,3); 
#ifdef PWM_BACKLIGHT
      sbi(DDRC,2);
#endif

      if((LED_PWM_State & HOOKED) == 0) { // !!!! hook led PWM routine into 10KHz timer chain
         sreg = SREG;  // save global interrupt status
         cli();        // disable interrupts

         LED_PWM_State |= HOOKED;
         LED_PWM_Chain = service_10KHz;
         service_10KHz = LED_PWM_Service;

         SREG = sreg;  // resotore global interrupt status
      }

      LED_PWM_State |= ENABLED;   
   }
   else {  // turn off LED PWM control
      if(LED_PWM_State & ENABLED) {    // LED PWM controller was running
         LED_PWM_State &= (~ENABLED);  // turn off LED PWM controller
         led_set(255, 255);            // set all LED's to equivalent last PWM levels (OFF/DIM/ON)
      }

      if(LED_PWM_State & HOOKED) {  // unhook LED PWM service routine if possible
         sreg = SREG;  // save global interrupt status
         cli();        // disable interrupts
         if(service_10KHz == LED_PWM_Service) {  // PWM controller is first in the timer chain
            service_10KHz = LED_PWM_Chain;       // unlink us from the chain
            LED_PWM_State &= (~HOOKED);
         }
         SREG = sreg;  // restore interrupt state
      }
   }
}


//
// LED control routine
//
// if led_num == 0     set all LED's to *val*
// if led_num == 1..4  set LED led_num to *val*
// if led_num == 5     control backlight
// if led_num == 255   set all LED's to last state 
//                     (used when switching from PWM to simple LED control)
//
// val==0:      OFF
// val==1..254: DIM (standard LED_DIM value is 10)
// val==255:    Full ON 
//
void led_set(u08 led_num, u08 val)
{
   if((led_num == 0) || (led_num == 1) || (led_num == 255)) {
      if(led_num == 255) val = LEDInten[0];
      else LEDInten[0] = val;

      if((LED_PWM_State & ENABLED) == 0) {  // simple LED control
         if(val == LED_ON)       { sbi(DDRE, 2); sbi(PORTE, 2); }  // PE2
         else if(val == LED_OFF) { sbi(DDRE, 2); cbi(PORTE, 2); }
         else /* LED_DIM */      { cbi(DDRE, 2); sbi(PORTE, 2); }
      }
   }

#ifndef MOUSE_UART3   //!!!!!!
   if((led_num == 0) || (led_num == 2) || (led_num == 255)) {
      if(led_num == 255) val = LEDInten[1];
      else LEDInten[1] = val;

      if((LED_PWM_State & ENABLED) == 0) {  // simple LED control
         #ifdef LED2_PF5
           if(val == LED_ON)       { sbi(DDRF, 5); sbi(PORTF, 5); }  // PD6
           else if(val == LED_OFF) { sbi(DDRF, 5); cbi(PORTF, 5); }
           else /* LED_DIM */      { cbi(DDRF, 5); sbi(PORTF, 5); }
         #else 
           if(val == LED_ON)       { sbi(DDRD, 6); sbi(PORTD, 6); }  // PD6
           else if(val == LED_OFF) { sbi(DDRD, 6); cbi(PORTD, 6); }
           else /* LED_DIM */      { cbi(DDRD, 6); sbi(PORTD, 6); }
         #endif


      }
   }

   if((led_num == 0) || (led_num == 3) || (led_num == 255)) {
      if(led_num == 255) val = LEDInten[2];
      else LEDInten[2] = val;

      if((LED_PWM_State & ENABLED) == 0) {  // simple LED control
         if(val == LED_ON)       { sbi(DDRD, 7); sbi(PORTD, 7); }  // PD7
         else if(val == LED_OFF) { sbi(DDRD, 7); cbi(PORTD, 7); }
         else /* LED_DIM */      { cbi(DDRD, 7); sbi(PORTD, 7); }
      }
   }
#endif

   if((led_num == 0) || (led_num == 4) || (led_num == 255)) {
      if(led_num == 255) val = LEDInten[3];
      else LEDInten[3] = val;

      if((LED_PWM_State & ENABLED) == 0) {  // simple LED control
         if(val == LED_ON)       { sbi(DDRG, 3); sbi(PORTG, 3); }  // PG3
         else if(val == LED_OFF) { sbi(DDRG, 3); cbi(PORTG, 3); }
         else /* LED_DIM */      { cbi(DDRG, 3); sbi(PORTG, 3); }
      }
   }

#ifdef PWM_BACKLIGHT
// if((led_num == 0) || (led_num == 5) || (led_num == 255)) {
   if((led_num == 5) || (led_num == 255)) {
      if(led_num == 255) val = LEDInten[4];
      else LEDInten[4] = val;

      if((LED_PWM_State & ENABLED) == 0) {  // simple LED control
         if(val == LED_OFF) { sbi(DDRC, 2); cbi(PORTC, 2); }
         else               { sbi(DDRC, 2); sbi(PORTC, 2); }  // PC2
      }
   }
#else
   if(led_num == 5) {  // LED_DIM not available with simple LED control of backlight
      if(val == LED_OFF) { sbi(DDRC, 2); cbi(PORTC, 2); }
      else               { sbi(DDRC, 2); sbi(PORTC, 2); }  // PC2
   }
#endif
}



#ifdef LED_PWM_DEMO
// ------------------------------------------------------------------------------------

// Example of linking a user 100Hz interrupt routine into chain of routines -------------------


void (*after_led_100Hz)(void) = NULL;  // pointer to next service routine in chain
u08 led_demo_count;
u08 led_demo_state;
 
// note: this routine, once running, will take over LED display

void led_100Hz_update(void)  // function to be linked into chain of routines running at 100Hz
{
static u16 t;
u08 i;

   if(led_demo_state & ENABLED) {  // led demo is enabled
      if(--led_demo_count == 0) {  // we are adjusing LED's every 10mS
         led_demo_count = 100;

         i = t >> 1; 

         led_set(1,(68+ (sin256(i)>>1)));    // +/-64
         led_set(2,(68+ (sin256(i+32)>>1)));  
         led_set(3,(68+ (sin256(i+64)>>1)));  
         led_set(4,(68+ (sin256(i+96)>>1)));  

         t++;  // advance theta
      }
   }
    
   if(after_led_100Hz) {  // call next routine in 10KHz timer chain
      after_led_100Hz(); 
   }  
}



// start LED demo running
// takes over display -- except for brief flashes if other functions attempt to set/clear LEDs

u08 demo_pwm_state;

void led_pwm_demo(u08 flag)
{
u08 sreg;

  if(flag) { // start PWM led demo,  install handler if needed
     led_demo_count = 100;
     led_demo_state |= ENABLED;

     if(led_demo_state & HOOKED) { // LED demo timer routine already running -- exit
        return;
     }

     demo_pwm_state = LED_PWM_State;
     led_pwm(PWM_ON);  // make sure LED PWM handler is hooked in and running first

     sreg = SREG;
     cli();

     // RG 8/2008
	 // something confusing going on in below code
	 // should be?
	 //   after_led_100Hz=service_100Hz
	 //   service_100Hz= led_100Hz_update


     after_led_100Hz = service_10KHz;  // save pointer to old service routine 
     service_10KHz = led_100Hz_update; // pointer to new service routine
     led_demo_state |= HOOKED;
     SREG = sreg;

     // led_100Hz_update is now being called 100 times per sec. provided that interrupts are enabled
  }
  else { // stop led demo routine,  unhook handler if possible
     led_demo_state &= (~ENABLED);
     if((led_demo_state & HOOKED) == 0) return;

     // if LED demo timer routine is first thing in the timer chain
     // we can fully unhook it

     // RG 8/2008 below should be service_100Hz ?

     if(service_10KHz == led_100Hz_update) { 
        sreg = SREG;
        cli();

        // RG 8/2008  below should be 100 Hz

        service_10KHz = after_led_100Hz;
        after_led_100Hz = NULL;
        led_demo_state &= (~HOOKED);
        SREG = sreg;
        if((demo_pwm_state & ENABLED) == 0) {  // we started demo with PWM off
           led_pwm(PWM_OFF);                   // so disable led PWM mode
        }
     }
  }
}


// ------------------------------------------------------------------------------------

#endif // LED_PWM_DEMO
#endif // LED_CODE
