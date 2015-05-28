/*
   String input demo using Donkey Keyboard




   
   Note if you are new to C:  strings (arrays) passed to a function are passed by reference.
   That is the address of the array is passed allowing the array to be modified.

   This differs from integer or floating point values that are passed by value and cannot be
   modified unless passed by address using &variable. In such case the function must be
   declared to accept such parameters.

   
*/


#include "donkey.h"

int main (void) {
  md_init();		 // donkey system initialize 


  int n;             // used to record number of characters placed into keyboard buffer
  char s[17];        // define a character buffer (string) to receive keboard input

  s[0] = 0;          // set string length to zero (empty string)
                     // quicker than  strcpy (s,"");

                         // call keyboard 
  n =  menu_kbd(0,s,16);  // title (or 0 for 5 row kbd), *buffer, buffer length not including
                         // terminating null character, function returns number of chars in string

		
  lcd_clear();           // clear screen after keyboard exit

  printf ("[%s]\n  #chars=%d",s,n);  // print string entered 

  wait_touch_release();   
   
  // s[0] = 0; // clear string

  strcpy (s,"Ron");  // enter a default or initial string

  menu_kbd ("your name?",s,16);  // call keyboard, ignoring # chars return value 
  lcd_clear();
  printf("name:%s",s);
  

  while (1);

}
