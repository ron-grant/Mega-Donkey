/*

  Demo of draw code used to draw mega donkey logo.

  Code has been copied to this module from curves.c
  Might be reasonable to recode allowing Array name of curve/line data to be passed as 
  parameter to drawing function.






*/

#include "donkey.h"  // headers present in donkey libdonkey2561_108A 
                     
// Table Built from Adobe Illustrator File built using Corel Draw 8 (export .ai format file)
// .ai file translated by Delphi App. AITranslator.exe into following array constant 
// targeted for storage in Flash (Program Memory) rather then RAM.
// Note use of rd() macro below to read from program memory
 
u08 AI_Table[] PROGMEM = {
253,50,2,
50,2,52,2,53,1,53,1,
53,1,53,0,52,0,50,0,
50,0,48,0,46,0,46,1,
46,1,46,1,48,2,50,2,
253,62,2,
62,2,64,2,66,1,66,1,
66,1,66,0,64,0,62,0,
62,0,60,0,59,0,59,1,
59,1,59,1,60,2,62,2,
253,98,54,
254,89,58,
254,88,59,
254,90,59,
254,104,65,
254,89,65,
254,83,65,
254,82,66,
254,77,73,
254,72,80,
254,74,71,
254,75,65,
254,74,65,
254,75,59,
254,75,58,
254,72,49,
254,29,49,
254,29,46,
254,17,56,
254,27,52,
254,0,74,
254,13,59,
254,5,62,
254,29,41,
254,29,27,
254,35,27,
254,41,17,
254,49,4,
254,46,19,
254,45,27,
254,66,27,
254,64,18,
254,62,3,
254,70,16,
254,76,27,
254,83,27,
254,83,49,
254,77,49,
254,79,55,
254,88,55,
254,98,54,
253,84,64,
84,64,85,64,85,64,85,63,
85,63,85,62,85,61,84,61,
84,61,82,61,82,62,82,63,
82,63,82,64,82,64,84,64,
253,83,63,
83,63,84,63,84,63,84,63,
84,63,84,62,84,62,83,62,
83,62,83,62,83,62,83,63,
83,63,83,63,83,63,83,63,
253,34,35,
254,35,35,
35,35,35,34,35,33,36,33,
36,33,36,32,37,32,38,32,
38,32,39,32,39,32,40,33,
40,33,40,33,41,34,41,35,
41,35,41,36,40,37,38,38,
38,38,37,38,37,38,37,38,
37,38,36,39,35,39,35,40,
35,40,34,40,34,41,34,42,
34,42,34,43,34,44,35,45,
35,45,36,46,37,46,38,46,
38,46,39,46,40,46,41,45,
41,45,42,44,42,43,42,42,
254,41,42,
41,42,40,43,40,44,40,44,
40,44,39,44,39,45,38,45,
38,45,37,45,37,44,36,44,
36,44,36,43,36,43,36,42,
36,42,36,42,36,41,36,41,
36,41,36,41,36,40,36,40,
36,40,37,40,37,40,38,39,
38,39,39,39,39,39,39,39,
39,39,40,39,41,38,42,38,
42,38,42,37,42,36,42,35,
42,35,42,34,42,33,41,32,
41,32,40,31,39,30,38,30,
38,30,37,30,36,31,35,32,
35,32,34,32,34,34,34,35,
253,46,44,
254,46,39,
254,48,39,
48,39,49,39,50,39,51,39,
51,39,51,40,51,40,51,41,
51,41,51,42,51,43,51,44,
51,44,50,44,49,44,48,44,
254,46,44,
254,46,44,
253,44,46,
254,47,46,
47,46,48,46,49,46,50,46,
50,46,50,46,51,45,51,45,
51,45,52,45,52,44,53,44,
53,44,53,43,53,42,53,41,
53,41,53,41,53,40,53,39,
53,39,52,39,52,38,51,38,
51,38,51,38,50,37,50,37,
50,37,49,37,49,37,48,37,
254,46,37,
254,46,31,
254,44,31,
254,44,46,
253,56,38,
56,38,56,36,57,35,58,34,
58,34,59,32,60,32,62,32,
62,32,64,32,65,32,67,34,
67,34,68,35,68,37,68,38,
68,38,68,40,68,42,66,43,
66,43,65,44,64,45,62,45,
62,45,61,45,61,45,60,44,
60,44,59,44,59,44,58,43,
58,43,58,43,57,42,57,41,
57,41,56,40,56,39,56,38,
254,56,38,
253,54,38,
54,38,54,39,55,40,55,41,
55,41,55,42,56,43,57,44,
57,44,57,45,58,45,59,46,
59,46,60,46,61,46,62,46,
62,46,63,46,64,46,65,46,
65,46,66,45,67,45,67,44,
67,44,68,43,69,42,69,41,
69,41,70,40,70,39,70,38,
70,38,70,37,70,36,69,35,
69,35,69,34,68,34,68,33,
68,33,67,32,66,31,65,31,
65,31,64,31,63,30,62,30,
62,30,60,30,58,31,57,33,
57,33,55,34,54,36,54,38,
253,74,31,
254,73,31,
254,73,44,
254,70,44,
254,70,46,
254,77,46,
254,77,44,
254,74,44,
254,74,31,
255};  // 837 bytes



void thick_line(int x1,int y1, int x2,int y2); // forward  defined in curves.c


/*
{ 

   clipped_line(x1,y1, x2,y2);
   clipped_line(x1+1,y1, x2+1,y2);

}
*/


// scale factors to scale logo to full screen with 4 bit fractional
// part

//#define BEZ_SCALE_X ((int) COLS * 16 / LOGO_W)
//#define BEZ_SCALE_Y ((int) ROWS * 16 / LOGO_H) 

#define BEZ_SCALE_X 24
#define BEZ_SCALE_Y 16



void xform2 (int x, int y, int* xt,int* yt) {

  long scx,scy;

  scx = BEZ_SCALE_X;
  scy = BEZ_SCALE_Y;

  // if keep scaling uniform in x and y 
  //if (scx>scy) scx = scy;
  //else         scy = scx;

  // note translation factor of y-2 added to move demo logo up
  // 2 pixels
  // y= ROWS-y  flips drawing making bottom of screen y=0
  // (left handed coordinate system -- origin in lower-left screen 
  //  corner)

  *xt = ((x * scx)>>4);
  *yt = (ROWS-2-(y * scy>>4));  
  
}





#define rd(v) v=pgm_read_byte_near(&AI_Table[LogoI++])
#define rdxy(x,y) {rd(x); rd(y);}

void draw_pattern(void) 
{
u08 b;
int PrevX,PrevY;
int x,y;
int xt,yt;
int LogoI;

   LogoI = 0;

//   lcd_clear();
   viewport_init();
   set_color(WHITE);
   set_bg(BLACK);

   wait_while_touched();

u08 cur = erase_cursor();

   // X1,Y1,X2.. float values
 
   b = 0; 
   while(b != 255) {

      if(get_touch(1)) break;

      rd(b);
      switch (b) {
         case 254:     // Draw Line To XY
            rdxy(x,y); 
            xform2(x,y, &xt,&yt);

            X2 = xt;
            Y2 = yt;

            thick_line (CurX,CurY,X2,Y2);  // quick LCD line for now later maybe parametric option

            CurX = X2;
            CurY = Y2;
            break;

         case 253:    // MoveTo XY
            rdxy(x,y);
            xform2(x,y, &xt,&yt);

            CurX = xt;
            CurY = yt;
            break;

         case 255:   // drawing sequence terminator 
            break; 

         default: 
            x = b; rd(y); 
            xform2(x,y, &xt,&yt);
            X1 = xt;
            Y1 = yt;

            rdxy(x,y); 
            xform2(x,y, &xt,&yt);
            X2 = xt;
            Y2 = yt;

            rdxy(x,y); 
            xform2(x,y, &xt,&yt);
            X3 = xt;
            Y3 = yt;

            rdxy(x,y); 
            xform2(x,y, &xt,&yt);
            X4 = xt;
            Y4 = yt;

            PrevX = X1;
            PrevY = Y1;

            bezier_init(16);   // 16 steps per curve regardless of length
 
            while(DrawCmdActive) {
               bezier_nextpoint();
               thick_line(PrevX,PrevY, CurX,CurY);
               PrevX = CurX;
               PrevY = CurY;
            }
            break;
      } // end switch
   } // end while

   if(b == 255) {  // logo not interrupted
      wait_until_touched();
   }
   wait_while_touched();

   if(cur) show_cursor();

}

int main(void)
{
  md_init();

  draw_pattern();  

  while(1);
}
