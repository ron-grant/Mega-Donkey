/*  MegaDonkey Library File:  led.h    LED Support  - Header File
    


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

*/



#ifndef _LED_H
#define _LED_H


#ifdef LED_CODE

#define PWM_BACKLIGHT   /* led #5 is backlight */

#ifdef PWM_BACKLIGHT
#define NUM_LEDS 5
#else
#define NUM_LEDS 4
#endif

#define LED_OFF 0
#define LED_DIM 10
#define LED_ON  255

#define PWM_ON  1
#define PWM_OFF 0

#define HOOKED  0x01  // flag set when PWM service interrupt is installed
#define ENABLED 0x02  // flag set/cleared by calling led_pwm or led_set 

 u08 LED_PWM_State;            

 u08 LEDInten[NUM_LEDS];   // 0..3  for led 1 to 4 

void led_pwm(u08 state);             // 0=simple LED control  1=PWM led control
void led_set(u08 led_num, u08 val);  // led_num 1..4 or 0 for all  val = 0..255

#define led_on(led_num) led_set(led_num,LED_ON)
#define led_off(led_num) led_set(led_num,LED_OFF)



void led_pwm_demo(u08 flag); // demo system timer generated LED PWM dimming with 100Hz 
                             // update provided by linking 100Hz user function  
                             // into chain called by system timer (timer0)
void LED_PWM_Service(void);

#endif // LED_CODE

#endif // _LED_H
