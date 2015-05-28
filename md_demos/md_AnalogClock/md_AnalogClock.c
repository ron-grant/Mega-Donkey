/* Mega Donkey - Analog Clock
   Mark Sims 2007

   very slight mutation from original demo included in demolcd.c gives clock
   center and radius parameters, also did some shorthand on local vars  - Ron Grant
   
   Line Drawing
   Fast Integer Sine/Cosine 


   Check out md_RadarScope.c for a simpler demo of the math/programming used to
   drawing rotating vectors (rotating clock hands in this demo).

*/


#include "donkey.h"           // general init -- include this FIRST

// digits for clock face display
// choose 1 of 3 (comment out two not used)

#define ROMAN
//#define ARABIC
//#define STARS 


#ifdef ROMAN
  char *roman[] = {  
     "III",   "IIII",  "V",
     "VI",    "VII",   "VIII",
     "IX",    "X",     "XI",
     "XII",   "I",     "II"
  };
#endif
#ifdef ARABIC 
  char *roman[] = {
    "3",   "4",    "5",
    "6",   "7",    "8",
    "9",   "10",   "11",
    "12",  "1",    "2"
  };
#endif
#ifdef STARS
  char *roman[] = {
     "*",   "*",    "*",
     "*",   "*",    "*",
     "*",   "*",    "*",
     "*",   "*",    "*"
  };
#endif



// Resolution defined in md.h (for your panel)

#ifdef RES_128x64            // Default Clock Radius 
   #define ACLOCK_R 28
#endif
#ifdef RES_132x132
   #define ACLOCK_R 56
#endif
#ifdef RES_160x80
   #define ACLOCK_R (47*192/ASPECT)
#endif
#ifdef RES_160x128
   #define ACLOCK_R 56
#endif
#ifdef RES_240x128
   #define ACLOCK_R 56
#endif
#ifdef RES_320x240
   #define ACLOCK_R 110
#endif



void draw_hands(int cx, int cy, int r, int hh, int mm, int ss)
{
int x, y,aa,ra;

   aa = r/8;
   ra = r-aa;
   
   set_draw_flags(BLIT_XOR);

   x = (ra * cos360((ss+30)*6 + 90)  ) >> 7;
   y = (((ra * sin360((ss+30)*6 + 90) ) >> 7) * ASPECT) / 256;
   line(cx,cy,  cx+x,cy+y);

   if(mm != ss) { // if minutes matches seconds... dont do both hands
      x = (ra * cos360((mm+30)*6 + 90)) >> 7;
      y = (((ra * sin360((mm+30)*6 + 90)) >> 7) * ASPECT) / 256;
      line(cx,cy,  cx+x,cy+y);
   }

   if(hh != mm) {
      x =   ((r-aa*2) * cos360((hh+6)*30 + (mm/2) + 90) ) >> 7;
      y = ((((r-aa*2) * sin360((hh+6)*30 + (mm/2) + 90) ) >> 7) * ASPECT) / 256;

      line(cx,cy,  cx+x,cy+y);
   }

   set_draw_flags(FILLED);
   circle(cx,cy,4); // clock hub

   set_draw_flags(0x00);
}

void aclock(int cx, int cy, int r, u08 show_digits)  // center cx cy,  radius r
{
int x0, y0;
int x, y;
int theta;
int lasth, lastmin, lastsec;
int hours, minutes, seconds;
u08 cur;
u08 hands_on;

    
cur = erase_cursor();
 
    /* note: cos360 and sin360 both integer values -127 to 128 hence
	   to scale to 1.0 amplitude, the standard for sine and cosine,
	   we scale functions by dividing by 128 (or shift right 7).
       The trick is scale down by 128 last in the computation since we are dealing
	   with integers. If we did the divide by 128 first, we would end up with a very
	   compact clock more easily implemented with a single call to the dot plot
	   function (dot(x,y).

       note: this is the basic equation pair used to calculate the points
	   on the ends of the hands and also the positions of the numerials.

	   x = r * cos360(angle_degrees) >> 7 
       y = r * sin360(angle_degrees) >> 7



    */ 
     
    // ASPECT = pixel aspect ratio * 256

   if (show_digits)
   for(lasth=0; lasth<12; lasth++) {  // draw the clock numerals
      x = (r*cos360(lasth*30)) >> 7;
      y = (((r*sin360(lasth*30)) >> 7) * ASPECT) / 256;
      if(COLS < 160) {
         lcd_textPS(cx+x-CHAR_WIDTH/2, cy+y-CHAR_HEIGHT/2, "*");
      }
      else {
         lcd_text(cx+x-(strlen(roman[lasth])*CHAR_WIDTH/2), cy+y-CHAR_HEIGHT/2,          roman[lasth]);
      }
   }

   x0 = (r*cos360(0)) >> 7;
   y0 = (((r*sin360(0)) >> 7) * ASPECT) / 256;

   for(theta=1; theta<360; theta++) {  // draw the clock outline
      x = (r*cos360(theta)) >> 7;
      y = (((r*sin360(theta)) >> 7) * ASPECT) / 256;
      line(cx+x0,cy+y0, cx+x,cy+y);
      x0 = x;
      y0 = y;
   }

  
   if(cur) show_cursor();


   lastsec = lastmin = lasth = 99;
   hands_on = 0;

   wait_while_touched();

   while(1) {
      if(get_touch(1)) break;

      getdt();
      seconds = time.secs;
      minutes = time.mins;
      hours = time.hours;

      if(seconds != lastsec) {
         // erase old hands
         if(hands_on) {
            draw_hands(cx,cy,r, lasth, lastmin, lastsec);
         }

         // draw new hands
         draw_hands(cx,cy,r, hours, minutes, seconds);
         hands_on = 1;

         lastsec = seconds;
         lastmin = minutes;
         lasth = hours;
      }
   }

   wait_while_touched();
   //lcd_clear();
}


int main(void)
{  md_init();

   
   // ACLOCK_R is default raduis for full screen clock
   // see defines above.
   
   box (0,0,COLS-1,ROWS-1);           // box around screen
   aclock(COLS/2,ROWS/2,ACLOCK_R,1);  // center x y  radius, show_digits

   // clock runs until panel touched

   lcd_clear();

   // now let's try a 1/4 screen clock without digits

   box (0,0,COLS/2-1,ROWS/2-1);         // 1/4 screen box 
   aclock(COLS/4,ROWS/4,ACLOCK_R/2,0);  // center x y  radius, show_digits

   lcd_clear();

   printf ("That's All Folks");

   while(1);
}



