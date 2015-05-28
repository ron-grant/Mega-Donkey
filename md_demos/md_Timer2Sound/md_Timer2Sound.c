/* Mega Donkey Demo   md_Timer2Sound.c

  Example of using 8 bit timer#2 to drive speaker.

  Timer2 can take control over speaker output pin which is normally under control of
  Mega Donkey's system timer (timer0) running a 10KHz service routine. This allows the 
  generation of variable frequency output to speaker with finer high frequency 
  selection than the 10KHz routine which is limited to frequencies that are 10 KHz
  divided by N, where N=1,2,3,4 ...  output 10KHz 5KHz 3.3KHz 2.5Khz ...

       
  Ron Grant 
  December 28, 2007

  copyright(c) 2007 by Ron Grant, All Rights Reserved


  Tested with Lib 1.02 March 3,2008



  See also: md_TimerSound which uses Timer1, but requires an interrupt routine to
  handle changing speaker port pin -- done automatically with this timer.

      
*/


#include "donkey.h"  // Mega Donkey Library


// Timer2 Helper Macros
// Not Complete Support (see Atmel documentation)
// These may become part of timer.c
// Also, would want to add interrupt support

#define timer2_set_prescale(p) TCCR2B = ((TCCR2B & 0xF8) + (p))

// used in NON-PWM mode, e.g. CTC mode 
// 0=disconnect pin 1=toggle OC2A 2=clr on match 3=set on match
#define timer2_set_compare_output_mode(m) TCCR2A = ( (TCCR2A & 0x3F) + ((m)<<6))

// set waveform on timer 2
#define timer2_set_waveform(w) TCCR2A= ((TCCR2A & 0xFC) + ((w)&3) ); \
  TCCR2B= ((TCCR2B & 0xF7) + (((w)&4) <<1)) 


    
void sound2_init(void)  // using timer 2 to generate sound  OC2A = PB4 = speaker pin
{
  // note PB4 already setup as output pin

  timer2_set_prescale(5);  // 0=stop 1=no div, 2= cpu/8, 3 = cpu/32
  timer2_set_waveform(2);  // CTC mode
  timer2_set_compare_output_mode(1); // toggle output pin on match (take over PB4)
  OCR2A=128;
}

// return control of pin PB4 and shutdown timer
#define sound2_stop()  timer2_set_compare_output_mode(0);timer2_set_prescale(0)



int v1,v2,v3,v4,v5,v6; 

void ve_timer_vars(void) {           // user variable list    
  ve_group ("TIMERSOUND DEMO");                // at least one variable group is required                                      
  ve_int   ("v1 f",v1,0,100);    // label,var,min,max
  ve_int   ("v2 f dif",v2,0,100);    // 
  ve_int   ("v3 delay",v3,0,100);
  ve_int   ("v4 base f",v4,0,400);
  ve_int   ("v5 freq amp ",v5,1,400);
  ve_int   ("v6 f dif scale",v6,1,5);
}  


int main (void) {
  md_init();        // Mega Donkey Initialize

  int t1,t2;
  int T,s1,s2;
  
  v1 = 50;
  v2 = 10;
  v3 = 2;  
  v4 = 150; // base freq 
  v5 = 100;
  v6 = 4;
           
  sound2_init();  // speaker controlled by 8 bit timer2 
                  // beep will not function until sound_stop() 


  ve_init(ve_timer_vars);    // initialize variable editor using the function name 
                             // of the variable list defined above


  while (ve_update()) {   // update variable list editor -- returns false when user closes menu [X]
    
    // speaker frequency (1/T) is varied by the sum of two sine waves
	// sin256 is a fast sine function, where one period = 0..255 and result amplitude is +/-127

    delay_ms(1+v3);

    s1 = sin256(t1>>v6);  // here we divide theta by 2^v6 to scale the varrying 
	s2 = sin256(t2>>v6);  // values allow for low frequency wave synthesis
	                      // creating ability to gradually change speaker frequency if desired 

    T = v4*8 + ((v5 *(s1+s2)) >> 5); 

    OCR2A = T>>4;  // output to timer

    // advance theta for sine functions
	// v2 is frequency differential

	t1+=v1;       
	t2+=v1+v2;

  } // end variable list while loop


  sound2_stop();

  lcd_clear();
  printf ("timers stopped");
     

  while(1);  // loop forever
} // end main 









