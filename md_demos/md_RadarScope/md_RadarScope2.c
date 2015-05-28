/* Mega Donkey - Radar Scope
   Ron Grant December 2007
   
   Line Drawing
   Fast Integer Sine/Cosine 


   Incomplete Demo
   






   Check out md_AnalogClock.c for analog clock face using same 
   type of math

*/


#include "md.h"           // general init -- include this FIRST
#define EXTERN extern     // this is required for now, ACK!

#include <stdio.h>        // printf

#include "graph.h"        // Quick Sine/Cosine 
#include "timer.h"        // system timer  delays ..  random number gen
#include "lcd.h"          // LCD hardware/graphics/bitmapped text support
#include <stdio.h>        // printf support
#include "adc.h"          // adc sampler and touch screen support 
#include "math.h"


// draw radar screen

int CenterX,CenterY,SweepRadius;
int CurSweepAngle;                 // local 


u08 RadarData[360];  // 1 sample per degree 


void radar_init(int cx, int cy, int r)
{
  int i;
  for (i=0; i<360; RadarData[i++]=0);  // clear radar data table
  CurSweepAngle = 0;




}

void radar_update(int angle, u08 value)
{





}



// simulate a wall with a horizontal line segment
// this is a little bit complicated for the demo
// in real life this function would be replaced by reading a sensor


u08 radar_get_simulated_data(int angle)
{
  long yL = 25;
  int  u;

  int x,y,xp,yp,d;
  
  // calculate coordinates of sweep line 0,0 to x,y
  // based on current sweep angle and scope's defined radius
  
  x = SweepRadius * sin360(angle) >> 7;
  y = SweepRadius * cos360(angle) >> 7;

  // find out if sweep ray intersects yL
  // using parameteric line equations xp = x1 + u(x2-x1)
  //                                  yp = y1 + u(y2-y1)
  // where u is varied from 0 to 1
  // in this case x1,y1 = 0 so
  // xp = u*x
  // yp = u*y

  if (!y) return 0;

  u =  yL * 256 / y;   // solve above for u where yL is substituted for yp  scale by 256

  if ((u>0) && (u<256))  // u in range 0 to 1.0 ? (scaled)
  { 
     // OK, intersection is on the line segment from 0,0 to x,y
     // solve for x

	 xp = u * x / 256;
	 yp = yL;

	 // now calculate distance from 0,0 to that point

	 d = xp*xp+yp*yp;
	 if (d>0) d = sqrt(d);

	 return d;
   }

   return 0;
}
   	   
    



int main(void)
{ 

   long LastRadarUpdate;
   int  angle;
   int  value;

   md_init();   // always call md_init, first

   wait_while_touched();

   angle = 0;
   LastRadarUpdate = 0;
      
   lcd_clear();
   box (0,0,COLS-1,ROWS-1);           // box around screen
   radar_init(COLS/2,ROWS/2,ROWS/3);   // center x y  radius

   // radar initial sweep angle = CurSweepAngle = 0

   while (!get_touch(1))
   {
   

     if (get_time_alive_100ths() > LastRadarUpdate+10)
	 {
	    value = radar_get_simulated_data(angle);
        radar_update(angle,value); 

        angle+=1;
	    if (angle>360) angle -= 360;


        LastRadarUpdate = get_time_alive_100ths(); 
     }

     // can do other processing
     
    
     

   }

   lcd_clear();
   printf ("That's All Folks");
   while(1);
}



