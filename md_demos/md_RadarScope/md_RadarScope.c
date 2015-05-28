/* Mega Donkey - Radar Scope
   Ron Grant December 2007
   
   Line Drawing
   Fast Integer Sine/Cosine 


   lacks code to erase blips

   
   Check out md_AnalogClock.c for analog clock face using same 
   type of math

*/


#include "donkey.h"           // general init -- include this FIRST


int CenterX,CenterY,SweepRadius;  // Radar Screen 
int PrevX,PrevY;                  // Previous Sweep Endpoint (for Sweep Erase)

/* simulate N BLIPS 

   In the real world, a distance sensor combined with a pulse detector 
   might detect a target with a beacon then use the distance sensor to measure 
   its distance.

   Here, we fake the data by maintaing a list of targets and their X,Y coordinates
   which are converted to angle and distance as would be perceived by the radar.

*/

#define NBLIPS 3

#define BLIP_DX -1       // velocity for all blips 
#define BLIP_DY 2
 
int BlipAngle[NBLIPS];
u08 BlipDist [NBLIPS];
int BlipX    [NBLIPS];  // position in 16 ths of pixel
int BlipY    [NBLIPS];


void radar_sim_init(void) {

  int i;

  // init position within square extents of radar scope

  for (i=0;i<NBLIPS;i++) {
 	BlipX[i] = (i-1)*10;
	BlipY[i] = (i-1)*10;
 
  }

  //BlipX[0] = 20;
  //BlipY[0] = 0;



}


void radar_sim_update(void)  // called once per sweep
{
  u08 i;
  int d;
  int x,y;

  for (i=0;i<NBLIPS;i++) {

     // update x,y position of blips (all moving same velocity)

     BlipX[i] += BLIP_DX;      
	 BlipY[i] += BLIP_DY;   

  
     x = BlipX[i];
	 y = BlipY[i];

     // calculate blip range with respect to sweep center (0,0)

     d = x*x+y*y;
	 if (d) d = sqrt(d);
     BlipDist[i] = d;

     // calculate 
     // angle of blip with respect to 0,0
	 // angle in radians = atan2(yoffset,xoffset)
	 // atan2 handles all 4 quadrants whereas atan does not
    
	 BlipAngle[i] = 90+ atan2(y,x)*180/3.1415;
  }
}




#define abs(x)((x)<0?-(x):(x))


// return only one sample


u08 radar_get_sample(int angle)
{
    u08 i;

    for (i=0;i<NBLIPS;i++)
      if ( (abs(BlipAngle[i]-angle)<10) &&
	       (BlipDist[i] < SweepRadius) && (BlipDist[i]))  return BlipDist[i];

    return 0;
}





void radar_init(int cx, int cy, int r)
{
  int dy,rc;

 
  CenterX = cx;
  CenterY = cy;
  SweepRadius = r;

  for (rc=0;rc<5;rc++)
    ellipse (CenterX,CenterY,SweepRadius*rc/4+1,ASPECT);
  
  dy = (r * ASPECT) >> 8;

  box (cx-r-2,cy-dy-2,cx+r+2,cy+dy+2);           // box around radar scope

  //flag: scope has not been updated yet, used for XOR line draw 
  PrevX = -1;

}



void radar_update(int angle, u08 dist)
{
  int x,y;

  x = CenterX + (SweepRadius * sin360(angle) >> 7);
  y = CenterY - ((SweepRadius * cos360(angle) >> 7)* ASPECT >>8);

  draw_flags = BLIT_XOR;
  if (PrevX != -1)
    line (CenterX,CenterY,PrevX,PrevY);

  line (CenterX,CenterY,x,y);
  draw_flags = 0;  // normal COPY mode
  PrevX = x;
  PrevY = y;

  if (dist)   // draw blip if dist is nonzero
  {
     beep (40,2500);  // emit sound when blip is encountered

     x = CenterX + (dist * sin360(angle) >> 7);
	 y = CenterY - ((dist * cos360(angle) >> 7) * ASPECT >> 8);

     // blip x,y is relative to scope center @ (CenterX,CenterY)
	 // this offset already added in above

	 filled_box (x-2,y-2,x+2,y+2);  
  }

   lcd_setxy((COLS-CHAR_WIDTH*3)/2,ROWS-CHAR_HEIGHT-1); // center angle 3 chars
   printf ("%3d",angle);  


}

 
int main(void)
{ 

   long LastRadarUpdate;
   int  angle;
   int  value;

   md_init();   // always call md_init, first

   angle = 0;
   LastRadarUpdate = 0;
      
   lcd_clear();
   lcd_setxy((COLS-CHAR_WIDTH*5)/2,0); // center "Radar" title 5 chars
   printf ("Radar");

   
   radar_init(COLS/2,ROWS/2,ROWS*2/5);   // center x y  radius
   radar_sim_init();                     // simulation init (provides blip locations)

   // radar initial sweep angle = CurSweepAngle = 0

   while (!get_touch(1))
   {
   
     // update only after 4/100 a second has passed
	 
     if (get_time_alive_100ths() > (LastRadarUpdate+3))
	 {
	    LastRadarUpdate = get_time_alive_100ths(); 
	    
		
		value = radar_get_sample(angle);  // only one target for given angle
        radar_update(angle,value); 

        // here we advance radar sensor angle and assume it roates in a circle,
		// other options might be a sensor that sweeps back and forth from say
		// -45 to 45
        
        angle+=2;                    // advance sensor some delta angle (8 here)

	    if (angle>359) {
		   angle -= 360;    // keep angle 0 to 359
		   radar_sim_update();  // move simulated targets
		} 

     }

     // can do other processing here
          

   }

   lcd_clear();
   printf ("end Radar Scope");
   while(1);
}

