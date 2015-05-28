
#include "donkey.h"
#include "sonar.h"


u08 CurSonar;
u08 SonarChannels;


void sonar_init(void)
{
u08 i;


    cli();  // disable interrupts

    for (i=0; i<SONAR_MAX_CHANNELS; i++); SonarPortBit[i]=0;  // clear all sonar definitions


    timer3_set_prescale (4);   //  4 = clk/256 = 32 us per count
    timer3_set_waveform (4);   //  4 = CTC (count 0 to OCR3A then reset and interrupt)  

    timer3_set_output_compare_ABC(0,0,0);       // don't take over any pins with OC1x signals 
    timer3_set_output_compare_int_ABC (1,0,0);  // OC3A interrupt
    timer3_set_input_capture(0,1);              // NoiseCan,PosEdge   ICP3 input capture interrupt
    sonar_set_channel_ms(50);              // default select new channel every 50 mSec

    CurSonar = 0;  // interrupt channel auto incremented and wrap around after being
                   // incremented one beyond highest channel defined

    sei();         // re enable interrupts 
}


void sonar_set_channel_ms (u16 ch_time)  // set channel time in mSec
{
long t;           // place in time 32 bit int to avoid overflow 

   t = ch_time;    
   cli();
   TCNT3 = 0;
   OCR3A = t * 125/4;    // e.g.  50 ms   50*1000/32 = 1562
   sei();
}



void sonar_stop()
{
   timer3_set_prescale(0);                     // stop timer and all associated interrupts
   timer3_set_output_compare_int_ABC (0,0,0);  // OC3A interrupt
   timer3_stop_input_capture();                // disable input capture interrupt
}



void sonar_def(u08 ch, char port1, u08 bit1)
{   
    if(port1 >= 'a') port1 -= 32;  // shift to uppercase

    if(ch>SonarChannels) SonarChannels = ch;  // interrupt service  loops from 0 to the largest
                                              // channel defined, therefore best to define 
                                              // skipping a channel number will generate 
                                              // dead time between channels

    // define PORT and BIT to pulse for trigger for this sonar channel
 
    SonarPortBit[ch] = ((port1 & 0xF) << 4) | (bit1 & 7);  // 0PPP0BBB  PPP 001=A 010=B ...
}



SIGNAL(SIG_OUTPUT_COMPARE3A)
{
char p;
u08  b;

    
   if(++CurSonar >= SonarChannels) CurSonar = 0;  // next channel, wrap around to zero
   
   b = SonarPortBit[CurSonar];
   p = (b >> 4) + 64;            // 1 -> 'A'  2-> 'B'  ...
   b &= 7;     

   if(!p) return;

   switch (p) {
      case 'B' : sbi (PORTB,b); break; // set servo pulse high
      case 'E' : sbi (PORTE,b); break; 
      case 'D' : sbi (PORTD,b); break;
      case 'F' : sbi (PORTF,b); break;
      case 'A' : sbi (PORTA,b); break;
      case 'C' : sbi (PORTC,b); break;
      case 'G' : sbi (PORTG,b); break;
   } // end switch

   // delay minimum 10 us
  
 
   // for(i=0; i<8; i++)   asm volatile ("nop");
   delay_us(10);  // new static inline function defined in delay.h
     
   led_set(2, LED_ON);     // delay about 10 us


   switch (p) {
     case 'B' : cbi (PORTB,b); break;     // set servo output low (end of pulse)
     case 'E' : cbi (PORTE,b); break; 
     case 'D' : cbi (PORTD,b); break;
     case 'F' : cbi (PORTF,b); break;
     case 'A' : cbi (PORTA,b); break;
     case 'C' : cbi (PORTC,b); break;
     case 'G' : cbi (PORTG,b); break;
   }
}         


u16 SonarTimeStart;


// !!! ICR3  input capture register - timer 3 

SIGNAL(SIG_INPUT_CAPTURE3)
{
   if(bit_is_set(PINE, 7)) {   // check state of input capture pin (ICP3 = PE7)
      SonarTimeStart = ICR3;   // Low to High, Start Timer
   }
   else {    // High to Low -- record pulse width 
      SonarDist[CurSonar] = ICR3 - SonarTimeStart;
   }
}



