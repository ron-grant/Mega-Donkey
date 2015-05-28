/*

 sonar.c   Sonar Support


 Copyright (c) 2008 by Ron Grant, all rights reserved



 
 Tested using single sonar SR04 module and servro


 Pins Used

 PE7 Sonar Echo
 PE6 Sonar Trigger

 PB6 RC Servo Output


 +5 and GND to Servo and Sonar


*/

#include "donkey.h"         // Mega Donkey Init Code (now includes servo code)
#include "sonar.h"          // sonar support SR04


SERVO1_UPDATE // THIS MUST BE INCLUDED -- SERVO1 INTERRUPT SERVICE ROUTINE



void sonar_sweep(void);  // forward


u08 CurSonar; 
u08 SonarChannels;



#define MAXANGLE 120  // total angle of servo rotation
#define DELTA 1


u08 DistTable[MAXANGLE];  // saved distances for analysis & ray erase

int  CurSweepAngle;  // current sweep angle
s08  DeltaAngle;     // delta added to sweep angle


// --------------------------------------------------------------------------------
 
void sonar_sweep_update()
{
 u08 r;
 int offset;

 // call this function frequently 
 if  (SonarDist[0] == -1) return;   // no sample ready

 int d = SonarDist[0];
 int t;

 cli();
 SonarDist[0] = -1;   // mark as read
 sei();

 set_charsize(2);
 lcd_setxy (0,ROWS-CHAR_HEIGHT*2-4);              // show value
 printf (PS("%04d"),d);
 set_charsize(1);

  r = d/6;   // SCALE DIST 
  
  ;
  if(r>35) r = 35;  // clip

  offset = 270-MAXANGLE/2;
  t = CurSweepAngle+offset;

  color = BLACK;
  polar_ray(DistTable[CurSweepAngle],t); 

  DistTable[CurSweepAngle] = r;             // new distance 

  color = WHITE;                // black on LCD
  polar_ray(r,t);               // draw new ray




 if (!DeltaAngle) DeltaAngle = DELTA;

 if (CurSweepAngle >= MAXANGLE) { DeltaAngle = -DELTA; CurSweepAngle=MAXANGLE; }
 if (CurSweepAngle <= 0) {DeltaAngle =DELTA; CurSweepAngle=0; }

 CurSweepAngle += DeltaAngle;
 long a = CurSweepAngle;
 if (a<0) a = 0;

 servo1_set_pos(0,a * 999 / MAXANGLE);    // Servo Channel 0 PB6

 // erase old and draw new point along vector


}



void sonar_sweep(void)     
                          // 4 Wire connection to servo module   PE6 Trigger,  PE7 Echo, +5 volts and GND
{
   led_set(2, LED_OFF);

   lcd_clear();
   set_color(WHITE);
   set_bg(BLACK);
   printf(PS("Sonar Init...."));

   sbi(DDRE, 6);  // PE6 Output
   cbi(DDRE, 7);  // PE7 Input

   sonar_init();                   // uses sonar.c code 
   sonar_set_channel_ms (100);     // channel time (repeat rate for single channel)
   sonar_def (0,'E',6);            // channel 0  PE6 trigger   (echo expected on PE7 for all)

   servo1_init();                  // init servo on timer1 PB6
   sbi (DDRB,6); // PB6            // output for servo channel 0
  

   lcd_clear();
   lcd_textPS(0,0, "Sonar        Sweep");

   polar_grid_fullscreen(4);  // draw full screen polar grid (centered at screen center)
                              // does not fill screen..

   //polar_grid (COLS/2,ROWS-16,  COORD w, COORD h, u08 bands);  // draw a polar grid  


    

   while (!get_touch(1)) {

      sonar_sweep_update();
     
	  delay_ms(20);  // !!! SLOW DOWN
	 	       
   }



   sonar_stop(); 
   servo1_stop();
   
}


/*

polar_ray  (int theta_deg, int r);
polar_grid (COORD cenx, COORD ceny,COORD w, COORD h, u08 bands);  // draw a polar grid
polar_grid_fullscreen(u08 bands);  // draw full screen polar grid (centered at screen center)

*/




int main (void)
{
  md_init();
  sonar_sweep();   // sweep sonar (sonar module attached to RC servo)
                   // servo need not be present to test sonar functionality
  while (1);
}
