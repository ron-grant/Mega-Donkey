/*


  Periodic Interrupt Demo
  Ron Grant 8/2008
  
  This demo requires libdonkeyXXX_103 or later.
  Note starting with library version 1.03 the library version# is appended to library name.

  If you create your own library compilation you might want to select a name variation that will not be confused with
  distribution library.



  Reminder: if you are using a demo project, make sure Project|Configuration Options are set to your processor
  model, e.g. mega128 OR  mega2561  

  Also, always check to see if your project has the correct library selected.


  RG: Code Tested MD-T1 (Mega128 based donkey)  using libdonkey128_103.a
                  MD-T2 (Mega2561 based donkey) using libdonkey2561_103.a


 
*/


#include "donkey.h"



void (*after_my_100Hz)(void) = NULL;  // pointer to next service routine in chain


void my_100Hz_update(void)      // function to be linked into chain of routines running at 100Hz
{
                                                    // user code here (should not be too time consumptive!)
                                                  // this is being called from an interrupt routine, hence should use care in doing I/O
                                                   // don't allow  early "return" must execute code at bottom of this funciton
  
   speaker_tick();                                     // simple test 
 


   // call next routine in timer chain ONLY if defined (not NULL)    
   if(after_my_100Hz)  after_my_100Hz();
}




void init_my100Hz_demo(void)
{
     u08 sreg;

     sreg = SREG;                                          // save status register
     cli();                                                        // disable interrupts
                                                                   //   after_my_100Hz = service_100Hz;  
                                                                   // save pointer to old service routine to be called after 
                                                                    // our routine finishes 
    
	 after_my_100Hz = service_100Hz;

     service_100Hz = my_100Hz_update;          // pointer to our new service routine - timer0 service function
                                                                    // is not set to call my_100Hz_update 100 times per sec. 
     SREG = sreg;                                          // re enable interrupts

     

                        // my_100Hz_update is now being called 100 times per sec. provided that interrupts are enabled
     
   }



int main (void) 
{   md_init(); 
    // mdt_init(0);     // OPTIONAL enable output to Mega Donkey Terminal (PC)

   
   
    init_my100Hz_demo();  //initialize interrupt
 
    printf (" 100Hz interrupt \n enabled"  );

    long t =0;

    while (1)     // loop forever -- don't allow return from main
    {

      if (t != get_time_alive_secs())
	  {  t = get_time_alive_secs();
	     lcd_setxy(20,20);
	     printf ("Time Alive %ld",t);   
      }

    }
                    

}

