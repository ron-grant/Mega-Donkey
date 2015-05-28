/* servo1.c   Mega Donkey  Hobby RC Servo Support using Timer 1


   Ron Grant
   December 2007

   Options for Direct Connection of 1 or 2 servos or 
   adding one or two decoders (where servo data is serialized) on 
   port pins PB6 and or PB7

   Earlier rendition of code allowed using any IO pin. Downside was latency caused by other
   interrupt routines caused jitter.

   Now timer output compare hardware controls pulsewidth on one or two pins allowing for 1 or 2 
   jitter free servos. For more servos, pulses are time division multiplexed with options for 
   short sync pulse (default) or using PB5 as a data line and PB6 and PB7 as shift register clock lines



   servo1_init(u08 ChanNumA,u08 ChanNumB,int *ServoData); 
                                              

   For Time Division Multiplex with leading sync pulse:
   (See Below for Shift Register Mode)

   ChanNumA number of channels assigned to PB6
   ChanNumB number of channels assigned to PB7
   ServoData array large enough to hold servo data


   0 = disable output to pin
   1 = direct connect to pin
   8 = decoder connected to pin
   
   numbers larger than 8 are permitted for special applications, but servo performance may degrade
   as servo refresh time is increased 
   

   int ServoTable[8];
   servo1_init(8,0,ServoTable);   // using servo DEMUX on pin PB6  for upto 8 servos

   int ServoTable[12]
   servo_init(8,4,ServoTable);    // using 2 servo DEMUXes first on PB6 with 8 servos
                                  // second DEMUX on PB7 with 4 servos attached
   
   int ServoTable[2];
   servo_init(1,1,ServoTable);    // using 2 servos direct connected, one on PB6, other on PB7

   int Servo;
   servo_init(0,1,&Servo);        // single servo attached to PB7
                                  // note special case, we had to pass address explicitly here
                                  // examples above do that implicitly since passing arrays

    ------
	Shift Register Mode
    after init call servo1_shift_register_mode()

    setup timer in CTC mode to count from 0 to TOP= Compare Reg A
    output OC1A to port pin (PB5) use as shift register clock
	place a 1 on data line (PB6) at start of servo frame then clear after

    this outputs servo pulses consecutively, hence if all 8 channels are running a 1ms
	then frame will be output in 8 ms.

	considering final frame to calculate "fill out the rest of the frame" time e.g. might have
	up to 12 ms clocking a 0 into now empty shift register
		
*/


#define SERVO1_CODE


#define EXTERN extern
#include "md.h"

#ifdef SERVO1_CODE  // always defined with library compile
 
#include "servo1.h"
#include <avr/interrupt.h>
#include "timer.h"       // timer helper routines



//int *ServoPos;            // pointer to caller's ServoTable array (set in init)

int ServoPos[16];

u08 ChannelsA,ChannelsB;  // number of channels on A and B

u16 Servo1Min;        // min value default 1000 = 1 msec pulse width
u16 Servo1Max;        // max value default 2000 = 2 msec pulse width
u08 Servo1FrameSize;  // divider counter default 8 x 2.5 msec = 20 msec

u08 Servo1ChanA;   // total channels defined including mode bit in msb
u08 Servo1ChanB;

#define CA Servo1ChanA   // shorthand aliases
#define CB Servo1ChanB

void servo1_update(void);             // forward declare
void servo1_update_shiftreg(void);    // forward declare


// diagnostic output for scope trigger

#define L41 {sbi(DDRG,3); sbi(PORTG,3);}
#define L40 cbi(PORTG,3);


/* OC1A Interrupt generated when timer counts up to OC1A value (2.5 ms),
   time to switch to next servo channel 

   if NOT ServoMUX
   want to mask output for 7 of 8 servo time frames to maintain speced update rate 

   otherwise we setup pulse width for each of 8 channels 
   then mask the 9th


*/


void servo1_init(u08 ChanModeNumA,u08 ChanModeNumB,int *ServoData) 
{
  u08 i;

  Servo1ChanA = ChanModeNumA;
  Servo1ChanB = ChanModeNumB;

//  ServoPos  = ServoData;  // point to caller's storage area = ChannelsA+ChannelsB words


  if (CA) { sbi(DDRB,6); cbi(PORTB,6);}   // set PB6 (OC1B) as output pin and set LOW
  if (CB) { sbi(DDRB,7); cbi(PORTB,7);}   // set PB7 (OC1C) as output pin
  
  // Note:  important that output bits are set low in single servo mode because 7/8 of the time
  // we disconnect timer from port and expect output to be low 

 
  if (CA) for (i=0; i<CA; ServoPos[i++]=500);  // center all N servos on channel A
  if (CB) for (i=0; i<CB; i++) ServoPos[CA+i] = 500;


  cli();                     // mask interrupts

  timer1_set_prescale (2);   //  2 = clk/8 = 0.5us per count @ 16 MHz

  timer1_set_waveform (9);   //  4 = CTC (count 0 to OCR1A then reset)  
                             //  9 = PWM Phase and Freq Correct TOP=OCR1A

  // Set Compare Output Modes for each of three Compare Registers A,B and C 
  // compare output modes (non-pwm) are:
  // 0 output off
  // 1 toggle on match
  // 2 clear on match
  // 3 set on match

  // for waveform 9   compare output 2 = non-inverted pwm   3= inverted 
  // see interrupt routine
  
//!!! depending on coounts take over 
// B and or C

  timer1_set_output_compare_ABC(0,3,3);   // take over pins with OC1B,OC1C 

  // interupt on count to OCR1A to allow masking of pulses for N time 2.5 msec
  // alternately updating channel (servo pulses time division multiplexed)

  // direct timer output compare 1a interrupt handler to our service routine  
  timer1_output_compareA = servo1_update;


  timer1_set_output_compare_int_ABC (1,0,0);  // A B

  OCR1A = 2500;   // interrupt every 2500us = 2.5 mSec      (2500/0.5*2=2500)

  sei();          // enable interrupts


  servo1_set_range(1000,2000);
  servo1_set_interval(9);


}


void servo1_sr_mode(void)
{   sbi(DDRB,5); // if shift register mode then set PB5 output for data line
    cli();
    timer1_set_waveform (4);   //  4 = CTC (count 0 to OCR1A then reset)  

// toggle on match for now

    timer1_set_output_compare_ABC(1,0,0); // take over pin OC1A // with OC1B,OC1C 
    timer1_set_output_compare_int_ABC (1,0,0);  // A B
	timer1_output_compareA = servo1_update_shiftreg;
	sei();
}



                          
// optional settings -- called after init

void servo1_set_range(int min, int max) // default is 1000 to 2000  
{
  Servo1Min = min;
  Servo1Max = max;
}



void servo1_set_interval(u08 counts)    // counts x 2.5 ms, default 8x2.5 = 20ms = 50 Hz
{
  Servo1FrameSize = counts;
}


void servo1_pos (u08 channel, int pos)
{
  if (pos<0) pos = 0;                   // clip

// !!! TEMP use channel 0 as sync   NO adding Min bias 

  if (channel)  
  {
    pos += Servo1Min;
    if (pos>Servo1Max) pos = Servo1Max;   // clip
  }


  ServoPos[channel]=pos;     // not needed for non MUX mode
  
  // single channel
  
  if ((CA==1)&&(!channel)) OCR1B = OCR1A-pos;    // # on A defined as 1  and channel = 0 
  else 
  if ((!CA) && (!channel)) OCR1C=OCR1A-pos;
  else
  if ((CB==1)&&(channel==1)) OCR1C = OCR1A-pos;  // # on B 

}

 
void servo1_stop(void)         // stops timer and interrupts
{                              // leaves PB6 and PB7 as output 0 (driven low)
  timer1_set_prescale (0);     // stop timer
}


// thinking if sr_mode then require both outputs to be sr mode if not 0

#define set_oc(a,b,c)  TCCR1A = (TCCR1A & 3) | ((a &3) <<6) | ((b & 3)<<4) | ((c & 3)<<2)


// we need clock pulse to shift register to rise on count up compare match
// then we reverse match when counter reaches A
// so that when counter counts back down and matches B or C they rise once again 
// and clock shift register


SIGNAL ( SIG_OVERFLOW1)
{
// servo1_update_shiftreg();  // !!! need flag for non-SR mode mask
}


// if shift register mode int

void servo1_ocb (void)
{


}


// SHIFT REGISTER interrupt service ONLY

void servo1_update_shiftreg()
{
   static unsigned int tsum;
   static u08 CurChannel;

   // interrupt  counter reached OCA=TOP and has been (will be) reset to 0
   // we now define next interval

   L40  // LED4 0 or 1  scope trigger

   if (++CurChannel > Servo1FrameSize) // total number of slots, e.g. typical 8
   {  CurChannel = 0;      
      tsum=0;
	  L41
   }

   if (CurChannel == Servo1FrameSize) sbi(PORTB,6);   // clock out 1 at start of frame
   else cbi(PORTB,6);

   // define width of next interval
   // two considerations -- could handle small number of servos then use large value of OC1A
   // to fill out remainder of frame
   // 
   // counter rate .5us/count   65535 = 32ms

   if (CurChannel<CA) OCR1A = ServoPos[CurChannel];
   else               OCR1A = 1000; 

   tsum += OCR1A;

}



void servo1_update(void)  // output compare 1a service -- this is called from 
                          // timer1 output compare service interrupt 
                          // single channel and time division multiplex with sync pulse modes
						    
{
  static u08 CurChannel;
  
 
  if ((CA>1) | (CB>1))
  {  if (++CurChannel > Servo1FrameSize)
     {  CurChannel = 0;           // Servo1Div expected to be 9 or greater
	                              // to insure adequate dead time on output
	    set_oc(0,3,3);            // enable  
     }

     if (CurChannel<CA) OCR1B = OCR1A-ServoPos[CurChannel];      // set PWM pulse width OCRB
	 if (CurChannel<CB) OCR1C = OCR1A-ServoPos[CA+CurChannel];  // set PWM pulse width OCRC


	 if (CurChannel==8) TCCR1A=TCCR1A&3;                // disable output set_oc(0,0,0);
     
  }

  // else 1 channel on each of two timer output pins 
  // OCR1B and/or OCR1C remains constant in this case and are set by calling servoX_pos()
  else
  if (!(CurChannel--)) {
     CurChannel = Servo1FrameSize;

     if ((CA) & (CB)) set_oc(0,2,2);  // both channels enabled
	 else if (CA)     set_oc(0,2,0);  // Channel A > 0  
	 else             set_oc(0,0,2);  // Channel B > 0
  }
  else set_oc(0,0,0);

}






#endif

