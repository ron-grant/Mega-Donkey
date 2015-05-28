/* Simple Oscilloscope
   Ron Grant January 2008

   For faster ADC check out md_UserADCScope


*/



#include "donkey.h"

int main (void) {
  int x,y,v,xo,yo;

  md_init();


  adc_init(4);  // channels 0,1,2,3    doubles default scan rate to about 2KHz
                // if leaving ADCClockPrescale at 7 =  CPU clk  divide by 128

 
  sbi(PORTF,0); // no pull up 
  
  
  while(1) {

  while (!get_touch(1)) {
   
     x = 0;
     delay_ms(50);
     lcd_clear();

	 while (x<COLS) {
        while ((v = get_adc_sample(0)) == ADC_SAMPLE_INVALID);    
		
	    v = v+40;
	    if (v > ROWS-1) v =ROWS-1;
		y = v;
    
	    
	 
        if (x) line (xo,yo,x,y);
		xo = x;
		yo = y;
		x++;
     }
   }

   wait_while_touched();

   
   }

}



  




