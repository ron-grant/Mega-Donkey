/* Mega Donkey Demo   md_TimerSound.c

  How to generate variable frequency interrupts using a 16 bit timer &
  using these interrupts to produce wider range of sound output to Donkey speaker 
  than possible with built-in function (beep) which is tied to fixed 10KHz interrupt.
    
  Ron Grant 
  December 28, 2007

  copyright(c) 2007, All Rights Reserved


  Here, the frequency of the speaker square wave is adjusted in 0.5 microsecond 
  increments. 

  Using summation of sine waves of different frequencies, some interesting sounds can 
  be produced with .5 us resolution on speaker output toggling frequency.
  
  

  This example is coded for timer 1. Any other 16 bit timer can be used, but would
  require recoding of low-level code.


*/


#include "donkey.h"  // Mega Donkey Library


// soundfx1_   low-level supporting code
// --------------------------------------------------------------------------------------
// Here we bundle all of the code that gives us a variable frequency output tied to the
// speaker. The interrupt service routine is bundled into a macro, and hence is included
// with the main function (BUT defined OUTSIDE of MAIN -- since it is a function definition)


// SIGNAL(OUTPUT_COMPARE1A) is a special interrupt service routine function
// which is called when Timer1 matches the value in register OCR1A
// when running in CTC mode (waveform mode 9)

#define SOUNDFX1_SERVICE \
SIGNAL(SIG_OUTPUT_COMPARE1A){if(bit_is_set(PORTB,4))cbi(PORTB,4); else sbi(PORTB,4);}

  
// prescale 2 = cpu clock /8  = 0.5 us per count
// waveform 9 = CTC mode (count to output compare register A and reset)
// OCR1A=500 used to set safe initial freq for OCR1A

void soundfx1_init(void)
{   timer1_set_prescale(2);   // counter counts at cpu clock divided by 8 (prescale=2)
    timer1_set_waveform(9);   // count up to OCR1A, generate interrupt, and reset 
	OCR1A=500;                
	timer1_set_output_compare_int_ABC (1,0,0);  // start interrupting  
}


#define soundfx1_stop() timer1_set_prescale(0)  // stops timer - and hence sound


void soundfx1_period(int t)
{
   if (t<50) t=50;  // clip high frequencies to prevent interrupts from hogging CPU time
   cli();
   OCR1A = t;       // disable interrupts while we update timer 
   sei();           
}

// end of low level supporting code
// --------------------------------------------------------------------------------------
    



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

				   
SOUNDFX1_SERVICE  // Required if using soundfx1  
                  // defines timer interrupt service routine SIGNAL OUTPUT_COMPARE1A
				  // must appear outside main (or any function)

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
           
  soundfx1_init(); // use timer1 for sound effects


  ve_init(ve_timer_vars);    // initialize variable editor using the function name 
                             // of the variable list defined above


  while (ve_update()) {   // update variable list editor -- returns false when user closes menu [X]
    
    // speaker frequency (1/T) is varied by the sum of two sine waves
	// sin256 is a fast sine function, where one period = 0..255 and result amplitude is +/-127

    delay_ms(1+v3);

    s1 = sin256(t1>>v6);  // here we divide theta by 32 to scale the varrying 
	s2 = sin256(t2>>v6);  // values allow for low frequency wave synthesis
	                     // creating ability to gradually change speaker frequency if desired 

    T = v4*8 + ((v5 *(s1+s2)) >> 5); 

    soundfx1_period(T); // output time period to timer (interrupt routine toggles speaker)

    // advance theta for sine functions
	// v2 is frequency differential

	t1+=v1;       
	t2+=v1+v2;

  } // end variable list while loop


  // program control reaches this point when user closes variable editor menu

  soundfx1_stop();               // stop timer - silence speaker     

  lcd_clear();
  printf ("timers stopped");
   

  while(1);  // loop forever
} // end main 









