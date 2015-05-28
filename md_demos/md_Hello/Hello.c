#include "donkey.h"

int main (void) 
{   md_init(); 
    // mdt_init(0);     // OPTIONAL enable output to Mega Donkey Terminal (PC)

    box (0,0,COLS-1,ROWS-1);     // draw a box around screen
	lcd_setxy(30,34);
    printf ("HELLO WORLD");
	beep(50,1000);               // duration =50/100ths  freq 1000Hz
    
    wait_touch_release();        // wait for touch screen press

	lcd_clear();
	printf ("That's all folks");

	while (1);     // loop forever -- don't allow return from main

}
