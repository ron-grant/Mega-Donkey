/* USER ADC Oscilloscope 


 Ron Grant
 January 2008
  



 Demonstrates taking over ADC sampling from Mega Donkey
 complete code for setting up interrupt driven ADC sampling

 refer to Atmel Mega128/Mega2561 documentation for details on
 ADC - Analog to Digital Converter 

 IIR filter computes signal average used for bias offset calaculation keeping signal 
 right on screen center without vertical offset adjustments required


 Ron Grant January 2008



 Sampled Audio from PC Speaker AMP Headphone Output  HP and common connected as follows
 where R1 and R2 form a voltage divider with C1 capacitively coupling signal to circuit
 0 signal biased to 2.5 volts.

 Shows voice waveforms well.


                    ^ +5
                    |  
                    R1 10K
                    | 
HeadPhone-----||----+----PF0 Input
             0.1uF  |
                   R2  10K
    Com------------ +
	                |
	               GND





*/

#include "donkey.h"


u08 UserPrescale;
u08 ADCChannels;


//#define SAMPLE8  // 8 bit samples versus 10 bit

#define ADC_REF (_BV(REFS0))    // AVCC    0100 0000   5v ref


#ifdef SAMPLE8
  volatile u08 ADCSamples[8];
#else 
  volatile int ADCSamples[8];
#endif


// ADC Conversion Complete Interrupt   (8/10 bit samples)


SIGNAL(SIG_ADC) 
{
     #ifdef SAMPLE8
       ADCSamples[ADCIndex] = ADCH;     // read single byte (requires left adjusted result)
	 #else  
       ADCSamples[ADCIndex] = ADCW;     // store last conversion result
	 #endif


     if(++ADCIndex >= ADCChannels) ADCIndex = 0;   // advance channel index, with wrap around
     ADMUX = ADC_REF | ADCIndex;    // select new channel to digitize
     ADCSRA = 0xC8 | UserPrescale;  // Start, next conversion 1100 1000, interrupt enabled
}            



void user_adc_init(u08 Channels, u08 Prescale)  // start repetitive scan -> ADCData[]
{
    ADCModeChannels = 0;         // shutdown donkey ADC service

    UserPrescale  = Prescale;
    ADCIndex      = 0;           // start on channel 0
    ADCChannels   = Channels;    // total number of channels active
    sbi(ACSR, ACD );             // turn off analog comparator

    ADMUX = ADC_REF | ADCIndex;  // 5v ref and result is right adjusted ADLR=0
                                 // these bits are preserved in future setting
                                 // of ADMUX reg

    #ifdef SAMPLE8               // if 8 bit sample mode, left adjust result
	ADMUX |= (1<<ADLAR);         // so that upper 8 bits of 10 bit value appears
	#endif                       // in ADCH (lose 2 LSBs)

    // ADCSRA bit fields:
    // 7 ADC Enable
    // 6 Start Conversion
    // 5 Free Run (when 1)
    // 4 Interrupt Flag - set 1 when conversion completes
    // 3 Interrupt Enable
	// 2,1,0 Channel

    // Prescale sets ADC clock    e.g. 6 = 64
	// e.g.  CPU clock rate /64 /14 clocks per conversion   
	// 16,000,000 / 64 / 14 = 17,857 conversions / seconde
	// T = 1/f = 1/17857 = 5.6E-5 seconds OR 56 microseconds per sample      
    //  
    
	// ADC Enable,Start Conversion,Enable Completion Interrupt

    ADCSRA = 0xC8 | UserPrescale;  // Start, single conversion 1100 1000
                                   // will be restarted in routine
   }


int Buf[160];


int main (void) {
  int bias,x,y,v,xo,yo,h2;

  md_init();
  
  // mdt_init(0);  // Mega Donkey Terminal 

  user_adc_init(2,5); // takes over system ADC    channels,prescale


  sbi(PORTF,0); // no pull up 
  
  bias = 512;  
  h2 = ROWS/2;
  
  while(1) {

     x = 0;
     lcd_pageflip();  // double buffered graphics 
     lcd_clear();

     // capture 1 screen of data then plot it

	 while (x<COLS) {

        // -1 better marker, but won't work with 8 bit samples..

	    while (ADCSamples[0] == 0); // wait while no sample present
		cli();
		v = ADCSamples[0];
		ADCSamples[0] = 0; // mark sample as read
		sei();

        bias = (bias * 15 + v) >> 4;  // calc bias using IIR filter
		y = (v - bias)/2 + h2;
        if (y > ROWS-1) y =ROWS-1;
        if (y<0) y = 0;

        Buf[x++] = y;
     }
     
	 // plot the data captured
	   
     x = 0;
	 while (x<COLS) {
        y = Buf[x];
        if (x) line (xo,yo,x,y); // draw line from prevoius x,y to current
	
		xo = x;
		yo = y;

		x++;
     }
  
   }
}



  




