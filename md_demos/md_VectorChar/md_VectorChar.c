/* Simple Mega Donkey Vector Character Demo


   Vector Characters are Line Drawn which allows them to be rotated and scaled 
   for easier integration into 2D wireframe graphics



*/


#include "donkey.h"

void vchar_demo(void)
{
u08 i;

   set_color(WHITE);
   set_bg(BLACK);
   lcd_clear();

   vchar_init();         /* vector LCD font init  */
   vchar_set_stdout();
   printf_flags &= (~VCHAR_COMPAT);   // don't force vchar cells to bitmapped char size

   vchar_set_fontsize(1,1);
   VCharSpaceX = 3;
   printf(PS("ABCDEFGHIJKLMNOPQRSTUVWXYZ\n"));
   printf(PS("01234567890 !@#$%%^&*()_+-=\n"));
   printf(PS("[]|\\:;""',.?/\n"));

   vchar_set_thickness(2,2);
   vchar_set_fontsize(2,1);
   VCharSpaceX = 4;
   printf(PS("ABCDEFGHI"));

   delay_until_touched(2000);
   wait_while_touched();

   lcd_clear();
   vchar_set_thickness(2,2);
   vchar_set_fontsize (4,4);

   for(i=0; i<6; i++) {
     lcd_clear();
     vchar_set_thickness(i+1,i+1);
     vchar_set_fontsize(i,i);

     printf(PS("601"));
     delay_ms(1000);


     if(get_touch(1)) break;

   }

   lcd_set_stdout();    // make printf() use bitmapped chars
}


int main(void) {
  md_init();
  vchar_demo();

  while(1);
}
