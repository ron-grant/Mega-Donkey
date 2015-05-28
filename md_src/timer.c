/*  MegaDonkey Library File:  timer.c     System Timer (using timer0)     


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


*/

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/crc16.h>
#include <stdio.h>
#include "md.h"

#ifdef TIMER_CODE
#include "lcd.h"
#include "adc.h"
#include "timer.h"
#include "graph.h"
#include "menu.h"
#include "kbd.h"       // time set  calls menu_kbd
#include "calc.h"
#include "md_term.h"


#ifdef _AVR_IOM2561_H_
#define OCIE0     OCIE0A
#define TCCR0WGM  TCCR0A
#define TCCR0CS   TCCR0B
#define OCR0      OCR0A
#define TIMSK     TIMSK0
#define SIG_OUTPUT_COMPARE0 SIG_OUTPUT_COMPARE0A
#else
#define TCCR0CS  TCCR0
#define TCCR0WGM TCCR0
#endif


void time_hook(void);

volatile unsigned long TimeAliveMsecs;     // milliseconds since reset 
volatile unsigned long TimeAliveSecs;     
#ifdef ANAL_ABOUT_TIME
volatile u16 TimeAliveWraps
#endif

int beep_freq;               // speaker tone divider
volatile int beep_freqcnt;   // reset val for tone divider
volatile int beep_dur;

int div_1KHz;

u08 div_100Hz;  // RG changed from int to u08  8/21/2008   
u08 div_1Hz;    // RG changed from int to u08  8/21/2008

void fire_timer(void);
//void (*user_timer_function)(void) = &fire_timer;


// Note:
// changed sense for direct drive speaker on port 
// where speaker is connected to +V and pin
// Mega Donkey uses capactively coupled transistor driver, which shuts off regardless of 
// leaving port pin high or low. 
 

#define IsBeepON bit_is_clear(PORTB,4)
#define BeepON   cbi(PORTB,4)
#define BeepOFF  sbi(PORTB,4)


void timer0_init(void)
{
u08 sreg;

   sreg = SREG;
   cli();

   ASSR = 0x00;  // Timer0 Clocked from IO Clock vs TOSC1 pin

   // Operate Timer in CTC (Clear on Timer Compare) Mode
   // Counter is Cleared when TCNT=OCR0
   // f = CLK/(2*Prescale*(1+OCR0))
   //
   // For 10KHz Operation:
   //
   // CLK=16M, Prescale = 8, f = 10,000 (f= desired interrupt rate)
   //
   // OCR0 = (1M/10,000) - 1 = 99
   // TCCR0 (COM01,COM00 00 = OC0 not connected to PIN
   // 
   // Note that the frequency f appears to be 2X the rate
   // 16MHz/8/200 = 10 KHz
  
#ifdef TIMER0_10KHZ    
    TCCR0WGM = (1<<WGM01);  // WGM01=1 CTC Mode (count to OCR0 and reset)
    TCCR0CS |= (1<<CS01);   // CS01=1  Prescale CPU Clock / 8

    OCR0  = 199;
#else
    // note: a prescale of 8 and count to 255 would yeild at 7812.5 Hz rate
    // this would allow OCR0 to be controlled with a res of 0.5 uSec per count
    // where interrupts could be generated and/or OCR0 could be coupled to output pin
    // In this case timer0 interrupt would need to be SIG_OVERFLOW0


    TCCR0WGM = (1<<WGM01);  // CTC Mode
    TCCR0CS |= (1<<CS00);  // prescaler=1, timer running   (62.5 KHz at 16MHz CPU Clock)

    OCR0  = 255;         // count to 255 (using CTC mode and Compare Match Interrupt)          
                         // could use normal mode, here, but would need to use Overflow Interrupt
                         // would provide possibility of using OCR0 for waveform gen 
#endif

    sbi(TIMSK,OCIE0);  // enable  timer0 output compare match 
    cbi(TIMSK,TOIE0);  // disable timer0 overflow (not enabled on powerup -- not really needed)
                        
    service_100Hz = 0;  // system_service_100Hz;  // assign service routine to be called
    SREG = sreg;        // at 100Hz from timer0 interrupt routine
}


void speaker_tick()
{  
   if(bit_is_set(PORTB,4)) cbi(PORTB,4);
   else sbi(PORTB,4); 
}



void beep(int dur, int freq)  // millisecs, Hz
{
u08 sreg;

   if(beep_disabled) return;

   if(freq == 0) ; // dur = 0;
   else freq = (TIMER0_FREQ + freq) / (freq*2);
   dur *= 10;   // make milliseconds into timer ticks

   sreg = SREG;
   cli();
   beep_dur = dur;   // timer 0 controlled beep
   beep_freq = freq; 
// beep_freqcnt = freq;
   SREG = sreg;
}

void beep_wait(void)
{
   while(beep_dur > 0) ;
}

u08 beeping(void)
{
   if(beep_dur > 0) return 1;
   else return 0;
}


unsigned long get_time_alive_secs(void)
{
unsigned long t;
u08 s;

   s = SREG;            // save interrupt status
   cli();               // disable interrupts                  
   t = TimeAliveSecs;
   SREG=s;              // restore interrupt status 
   return(t);
}

unsigned long get_msecs_alive(void)
{
unsigned long t;
u08 s;

   s = SREG;           
   cli();                                 
   t = TimeAliveMsecs;
   SREG=s;
   return(t);
}

void reset_time_alive(void)
{
u08 s;

   s = SREG;           
   cli();                                 
   TimeAliveMsecs = 0;
   TimeAliveSecs = 0;
#ifdef ANAL_ABOUT_TIME
   ++TimeAliveWraps;
#endif
   SREG = s;
}


void delay_ms(unsigned long d)
{
unsigned long t;
   
   if(d == 0) return;

   t = get_msecs_alive();
   if((t + d) < t) { // timealive is about to wrap
      reset_time_alive();
      t = d;
   }
   else t += d;

   while(t > get_msecs_alive()) ;
}


/*

Timer0

time alive functions & delay functions  reference time alive counter
adc update includes touch panel control and scan of adc channels in singled ended mode
user 10KHz routines
mdt mouse (mega dongkey terminal) mouse message receiver

*/



SIGNAL(SIG_OUTPUT_COMPARE0)
{ // This code executes at 62.5KHz or 10 KHz if TIMER0_10KHZ is defined 
 
  // Note: 10 KHz IS THE STANDARD!

 /* 
  #ifndef TIMER0_10KHZ
    Intentional error -- TIMER0_10KHz not defined,  calling adc_10KHz_update from this
	point require adjusting  adc digitization rate OR adding a divider here to
	throttle calls to adc_10KHz update	which currently requires 13x256 cpu cycles
	using CPU clock / 128 for ADC clock (prescaler=7)
  #endif
*/

//  #ifdef ADC_CODE
#ifdef TIMER_CALL_ADC  // operating in timer call adc get next sample mode (preferred)
                       // versus self interrupt mode (old method)

      if (ADCModeChannels & 0x80) adc_10KHz_update();
#endif
//  #endif


#ifdef USER_TIMER_FUNCTION
//USER_TIMER_FUNCTION
//if(user_timer_function) user_timer_function();
// user_timer_function();
#endif

   if(beep_dur > 0) {     // we are beeping
      if(--beep_dur <= 0) BeepOFF;   // count down beep duration
      else if(beep_freq && (--beep_freqcnt < 0)) {
         beep_freqcnt = beep_freq;   // reset time to next speaker wave transition
         if(IsBeepON) BeepOFF; 
         else BeepON;
      }
   } 
                
    
  // this counter decremented at 10KHz (if 10KHz interrupt rate)
  // OR at 62.5 KHz

   if(--div_1KHz <= 0) {
      div_1KHz = K_1KHz; 
      ++TimeAliveMsecs;
#ifdef ANAL_ABOUT_TIME
      if(TimeAliveMsecs == 0) ++TimeAliveWraps;
#endif


    // RG 8/21/2008 this code was removed sometime after 5/2007 
    // restored now to enable use of 100 Hz service routines
       
     if(--div_100Hz == 0) { 
        // this code executed at 100 Hz
        div_100Hz = 10; 
    
	    if (service_100Hz)    // RG added this test 8/21/2008 
          service_100Hz();    // 100 Hz interrupt service routine hook 
                              // if not NULL then call
                              // allowing user to link in their own routines 
                              // their responsibility to check vector (value of service_100Hz)
		                      // and save then call after or before their routine allowing a chain
						      // of calls to be created 

        if(--div_1Hz <= 0) {  // this code executed at 1 Hz
           div_1Hz = 100;
           TimeAliveSecs++;              
#ifdef TIME_CLOCK
           time_hook();
#endif 
         } // end code executed at 1 Hz   
  
     } // end code executed at 100 Hz

   } // end code executed at 1000 Hz


   // resume code executed at 10 Khz


   if(service_10KHz) { // 10KHz interrupt service routine hook
      service_10KHz();
   }

   #ifdef MDT_CODE
     if(mdtMouseEnabled) {   // read PC mouse message from mdt port
        mdt_mouse_update();  // creates mdt mouse message  - read in get_touch()
     }
   #endif

} // End 10 KHz Interrupt



u16 rand16()
{
   last_rand ^= (div_1KHz << 8);
   last_rand ^= TimeAliveMsecs;
#ifdef ADC_CODE   
   last_rand ^= ADCData[last_rand&0x07];
#endif
   last_rand = _crc16_update(last_rand, 0x69);
   return last_rand;
}



// TIMER 1 


// timer 1 and 3 near identical (except dif registers), supported with separate set of functions
// some fields use diferent positions within different registers



void timer1_set_prescale(u08 n)    // set low 3 bits,  mega 128 & 256 
{
   TCCR1B = (TCCR1B & 0xF8) | n;      
}


void timer3_set_prescale(u08 n)    // set low 3 bits, mega 128 & 256
{
   TCCR3B = (TCCR3B & 0xF8) | n;
}

    
void timer1_set_waveform(u08 n ) 
{  // set 1 of 16 timer operating modes
   // WGM13 WGM12 WGM11 WGM10  in two registers
   // same for both mega 128 & 256
   
   TCCR1B = (TCCR1B & 0xE7) | ((n>>2) << 3);   // set WGM13 and WGM12   1110 0111
   TCCR1A = (TCCR1A & 0xFC) | (n&3);           // set WGM11 and WGM10   1111 1100
}     

void timer3_set_waveform(u08 n ) 
{  // set 1 of 16 timer operating modes
   // WGM13 WGM12 WGM11 WGM10  in two registers
   // same for both mega 128 & 256
   
   TCCR3B = (TCCR3B & 0xE7) | ((n>>2) << 3);   // set WGM13 and WGM12   1110 0111
   TCCR3A = (TCCR3A & 0xFC) | (n&3);           // set WGM11 and WGM10   1111 1100
}     


// Set Compare Output Modes for each of three Compare Registers A,B and C 
// compare output modes (non-pwm) are:
// 0 output off
// 1 toggle on match
// 2 clear on match
// 3 set on match


void timer1_set_output_compare_ABC(u08 a, u08 b, u08 c)  // don't take over any pins with OC1x
{
   // preserve low two bits (WGM11 and WGM10)
   // set remaining 6 bits
   // note: each mode variable a,b,c is limited to range (0..3) to limit erroneous operation of channels if
   // one channel is specified outside the range 0..3.  

   // these bit fields are the same on mega 128 and mega 256

   TCCR1A = (TCCR1A & 3) | ((a &3) <<6) | ((b & 3)<<4) | ((c & 3)<<2);

   // DDR must be set to output accommodate output -- done here
   // note if mode is 0 DDR is not touched

   if(a) sbi(DDRB,5);
   if(b) sbi(DDRB,6);
   if(c) sbi(DDRB,7);
}


void timer3_set_output_compare_ABC(u08 a, u08 b, u08 c)
{
   TCCR3A = (TCCR3A & 3) | ((a &3) <<6) | ((b & 3)<<4) | ((c&3)<<2);

   // DDR must be set to output accommodate output -- done here
   // note if mode is 0 DDR is not touched

   if(a) sbi(DDRE,3);
   if(b) sbi(DDRE,4);
   if(c) sbi(DDRE,5);
}


void timer1_set_output_compare_int_ABC(u08 a,u08 b,u08 c)
{

   #ifdef _AVR_IOM2561_H_
   // RG 12/05/2007  corrected TIMSK1 field mapping
   TIMSK1 = (TIMSK1 & 1) | ((c & 1) <<3) | ((b & 1) << 2) | ((a & 1)<<1);  

   #else
   TIMSK  = (TIMSK  & 0xE7) | ((a & 1) << 4) | ((b & 1) << 3);
   ETIMSK = (ETIMSK & 0xFE) | (c & 1);

//temp stmt to make sure 2561 compile

   #endif
}



void timer3_set_output_compare_int_ABC(u08 a,u08 b,u08 c)
{
 
   #ifdef _AVR_IOM2561_H_
   // RG 12/05/2007  corrected TIMSK3 field mapping 
   // RG 12/14/2007  corrected the correction of TIMSK3 field mapping 
   TIMSK3 = (TIMSK3 & 1) | ((c & 1) <<3) | ((b & 1) << 2) | ((a & 1) <<1);  
   #else
    // preserve 1110 0101 (E5)
   ETIMSK  = (ETIMSK & 0xE5) | (a << 4) | (b << 3) | (c << 1); 
   #endif
}


// interrupt routines use indirect call to user function.
// before interrupts enabled assign given function to user_output_compareNN
// e.g.  timer_output_compareA = my_output_compare_function;
//

/*
not used 
if do use would need to be sure long pointers used 


SIGNAL(SIG_OUTPUT_COMPARE1A) { timer1_output_compareA(); }
SIGNAL(SIG_OUTPUT_COMPARE1B) { timer1_output_compareB(); }
SIGNAL(SIG_OUTPUT_COMPARE1C) { timer1_output_compareC(); }

SIGNAL(SIG_OUTPUT_COMPARE3A) { timer3_output_compareA(); }
SIGNAL(SIG_OUTPUT_COMPARE3B) { timer3_output_compareB(); }
SIGNAL(SIG_OUTPUT_COMPARE3C) { timer3_output_compareC(); }
*/



void timer1_set_int_overflow(u08 overflow)
{
   #ifdef _AVR_IOM2561_H_
     if (overflow) sbi (TIMSK1,TOIE1); else cbi (TIMSK1,TOIE1);    // mega 256x
   #else  // mega 128
     TIMSK = (TIMSK & 0xFB)| (overflow << 2);   // 11111011   
   #endif
}


void timer3_set_int_overflow(u08 overflow)
{
   #ifdef _AVR_IOM2561_H_
     if (overflow) sbi (TIMSK3,TOIE3); else cbi (TIMSK3,TOIE3);    // mega 256x
   #else  // mega 128
     ETIMSK = (ETIMSK & 0xFB)| (overflow << 2);   // 11111011  
   #endif
}



void timer1_set_int_inputcapture(u08 icap)
{
   #ifdef _AVR_IOM2561_H_
     if (icap) sbi (TIMSK1,ICIE1); else cbi (TIMSK1,ICIE1);       // mega 256x
   #else  // mega 128
     TIMSK = (TIMSK & 0xDF)  | ((icap &1) << 5);   // 1101 1111 
   #endif
}


void timer3_set_int_inputcapture(u08 icap)
{
   #ifdef _AVR_IOM2561_H_
     if (icap) sbi (TIMSK3,ICIE3); else cbi (TIMSK3,ICIE3);       // mega 256x
   #else  // mega 128
     ETIMSK = (ETIMSK & 0xDF)  | (icap << 5);   // 1101 1111 
   #endif
}

//
//
//  Time of Day / Date clock routines.
//  Keeps track of time of day and date by using the timer interrupt.
//  Time and date are kept in the global structure "time".
//
//
#ifdef TIME_CLOCK

u08 PROGMEM month_len[] = {
   0,
   31, 29, 31, 30,
   31, 30, 31, 31,
   30, 31, 30, 31
};


void time_hook(void)
{
u08 ml;

   if(++time.secs < 60) return;
   time.secs = 0;

   if(++time.mins < 60) return;
   time.mins = 0;
   div_1Hz -= time.adjust;  // adjust time clock drift by msecs/hr

   if(++time.hours < 24) return;
   time.hours = 0;

   if(++time.weekday >= 7) time.weekday = 0;

   ml = pgm_read_byte_near(&month_len[time.month]);
   if(time.month == 2) {  // that bastard February,  it's even spelled funny
      if((time.year & 0x03) == 0) {   // possible leap year every four years
#ifdef ANAL_ABOUT_TIME
         if((time.year % 100) == 0) { // every 100 years, no leap
            if(time.year % 400) --ml; // every 400 years, leap
         }      
#endif
      }
      else --ml;
   }
   if(++time.day <= ml) return;

   time.day = 1;
   if(++time.month <= 12) return;
   time.month = 1;

   ++time.year;
   return;
}


u08 day_of_week(u08 d, u08 m, int y)      /* 0 = Sunday */
{
static u08 PROGMEM dow_info[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};

   if(m < 3) y -= 1;
   return (y + y/4 - y/100 + y/400 + pgm_read_byte_near(&dow_info[m-1]) + d) % 7;
}

void set_time(void)
{
u08 sreg;
char s[COLS/CHAR_WIDTH+1];
int hours, mins, secs;
int day, month;
int year;
int adjust;
char x;

   getdt();     // dummy routine to read hardware time chip if installed
   sreg = SREG;
   cli();
   hours = time.hours;
   mins = time.mins;
   secs = time.secs;

   day = time.day;
   month = time.month;
   year = time.year;
   adjust = time.adjust;
   SREG = sreg;
   

   MENU_INIT

   do {

      MENU_CONTROLS

      menu_label    (0,0, PS("Set Time Menu"));  // labels outside of loop

      sprintf(s, PS("%02d:%02d:%02d"), hours, mins, secs);
      menu_button   (16,16, s, 1);

      sprintf(s, PS("%02d/%02d/%02d"), month, day, year%100);
      menu_button   (16,32, s, 2);

      menu_button   (16,48, PS(" ADJUST "), 3);

      menu_exitbutton();  // close button top right


      MENU_COMMANDS

      // menu button/control responses
      switch(menu_cmd()) {
         case 1:
            s[0] = 0;
            time_again:
            menu_kbd(PS("TIME: HH:MM:SS?"), s, 8+1);
            sscanf(s, "%d%c%d%c%d", &hours,&x, &mins,&x, &secs);
            if(hours >= 24) goto time_again;
            if(mins >= 60) goto time_again;
            if(secs >= 60) goto time_again;
            break;

         case 2:
            s[0] = 0;
            date_again:
#ifdef ANAL_ABOUT_TIME
            menu_kbd(PS("DATE: MM/DD/YYYY?"), s, 10+1);  //10+1 for four digit year
#else
            menu_kbd(PS("DATE: MM/DD/YY?"), s, 8+1);  //10+1 for four digit year
#endif
            sscanf(s, PS("%d%c%d%c%d"), &month,&x, &day,&x, &year);
            if((month < 1) || (month > 12)) goto date_again;
            if((day < 1) || (day > pgm_read_byte_near(&month_len[month]))) goto date_again;
            if(year < 100) year += 2000;
            break;

         case 3:
            adjust_again:
            if(adjust == 0) sprintf(s, PS("+/- msecs/hr "));
            else sprintf(s, PS("%d msecs/hr"), adjust);
            adjust = menu_calc(s, FIX_MODE|NO_DP_CHANGE|NO_BASE_CHANGE);
            if(adjust > 999) { adjust = 0; goto adjust_again;}
            else if(adjust < (-999)) { adjust = 0; goto adjust_again;}
            break;

      }

   } while (menu_cmd() != MENU_EXITCODE);       // repeat until exit command

   sreg = SREG;
   cli();
   time.hours = hours;
   time.mins = mins;
   time.secs = secs;

   time.day = day;
   time.month = month;
   time.year = year;

   time.weekday = day_of_week(day, month, year);
   time.adjust = adjust;

   reset_time_alive();
   SREG = sreg;

   wait_while_touched();
   setdt();   // dummy routine to set time chip is using harware time chip

   lcd_clear();
}

void setdt(void)
{
   // tweak adjust value by 1 so that 0 is saved as (-1)... unprogrammed EEPROM value
   eeprom_write_word((uint16_t *) EE_CLKDRIFT,  (uint16_t) time.adjust-1);
}

void getdt(void)
{
   // tweak adjust value by 1 so that an unprogrammed EEPROM 
   // value (0xFFFF) yields a 0 adjust factor
   time.adjust = eeprom_read_word((uint16_t *) EE_CLKDRIFT) + 1;
   if(time.adjust > 999) time.adjust = 0;
   else if(time.adjust < (-999)) time.adjust = 0;
}

#endif   // TIME_CLOCK

#endif   // TIMER_CODE
