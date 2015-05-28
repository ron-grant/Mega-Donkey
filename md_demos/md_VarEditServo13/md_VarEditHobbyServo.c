/* Mega Donkey Demo   md_VarEditHobbyServo.c

   This demo is an example use of the variable editor and hobby servo control.
   It will let you adjust servo values using sliders.

   
   Ron Grant 
   December 5, 2007
   March 3,2008         


   Microsecond resolution timing for two servos using timer1
   with output on PB6 (channel 0) and PB7 (channel 1) where these pins are
   connected to the control line of each of two hobby servos.

   Also Timer3 is used (Pins PE4 and PE5) to provide two more channels


   A separate +5 volt suppy is recommended for the servos.

   Verify the color coding of your servos.
   Typically RED (+5 volts) and BLACK (Ground)
   Yellow or white is used for the servo signal pulse.

   You must connect mega donkey's ground to the separate 5 volt supply ground. 

   See Also md_VarEditHobbyServo1 and md_VarEditHobbyServo3 which each use one timer 
   and provide only two servo outputs.


*/


#include "donkey.h"      // Mega Donkey Init Code


int s0,s1,s2,s3; // servo position 


// Servo update interrupt function macro call
// This defines an interrupt service function (see servo1.h, and servo3.h)
// This must be included in the program -- not within any function.

SERVO1_UPDATE

SERVO3_UPDATE


// Following is a variable list (defined as a function) for the variable editor.
// The name is your choice -- you must pass it to ve_init, see below.
  

//int s0,s1,s2,s3; // servo position 


void ve_servo1_vars(void) {           // variable list (contained in stand alone function)
   
  ve_group ("ServoT1");     // define ground (at least one variable group is required)                                      
  ve_int   ("Servo PB6",s0,0,1000);   // defines range for variable s0 as 0 to 1000
  ve_int   ("Servo PB7",s1,0,1000);   // 
  ve_group ("ServoT3");
  ve_int   ("Servo PE4",s2,0,1000);   // defines range for variable s0 as 0 to 1000
  ve_int   ("Servo PE5",s3,0,1000);   // 


}  




int main(void)
{
  md_init();        // mega donkey init called first
  //mdt_init(0);      // optional megadonkey term init port 0 

  s0 = 500;         // assign default values corresponding to servo center 1.5ms
  s1 = 500;    
  
  s2 = 500;
  s3 = 500;      


  
  // last one below inited works
  // first one stops working

  
 
  servo1_init();               // initialize RC servo1 controller for 2 channels (uses timer1)
                               // servo1_mux_init() would be used for 8 -- requires added hardware  

  servo3_init();               // init RC servo3 controller for 2 channels (uses timer 3)

 
  ve_init(ve_servo1_vars);     // initialize variable editor passing the function name 
                               // of the variable list defined above to ve_init

  while (ve_update()) {   // update variable list editor -- returns false when user closes menu [X]

    servo1_set_pos(0,s0);   // update servo positions  (used for direct to pin 1 or 2 channel mode)
    servo1_set_pos(1,s1);   

    servo3_set_pos(0,s2);   // update servo positions  (used for direct to pin 1 or 2 channel mode)
    servo3_set_pos(1,s3);   

	
  } // end variable list while loop


  // program control reaches this point when user closes variable editor menu

  servo1_stop();  // shut down output to RC servos
  servo3_stop();

  lcd_clear();
  printf ("That's all, servo pulses stopped");

  while(1); // loop forever
 

} // end



