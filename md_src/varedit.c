/*  MegaDonkey Library File:  varedit.c   Veriable Editor
    


    Copyright 2007,2008  Mark Sims & Ron Grant


    This file is part of The Megadonkey Software Library.

    The Megadonkey Software Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The Megadonkey Software Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
    more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Megadonkey Software Library. If not, see <http://www.gnu.org/licenses/>.



    Variable Editor
    Ron Grant 
    May 2007


-----------------------------------------
NEED AUTOLOAD
POSSIBLE DEFAULT VALUES TOO
IF number of vars changed or version number written changed then
load defaults

Also command to load hard coded defaults 
Maybe File Menu on sep screen

Load 
Save
Load Defaults


Floating Point Vals Support   -- might need new slider?

ve_float(   )

ve_string (  ) -- possible


--------------------------------------------


    Variable Editor implements a browser, using the menu framework, capable of editing
        program parameters (variables) using direct numeric input (via donkulator) or +/-
        buttons to increment or decrement ariables. Also, slider control input is available
        to modify integer values.

        As an alternative, the variable editor can be connected to a serial port via
        ve_set_port() function call, allowing remote terminal control of the variable editor.



    A user defined a function contains a list of function calls which define the groups
        and variables to be browsed. The function is passed to ve_init() at start up. 
    
    Current Support
        
        Integers (any size 8 to 32 bits)
        Bit Field / Boolean values

   

    Variable Editor User Interface

        [G] button cycles through groups
        [>] and [<] buttons allow selection of a given variable within a group.
        [+] and [-] buttons allow for increment or decrement of integer variables OR in the case
        of boolean allow turning the variable (bit within a byte/word) on/off.
        [E] invokes numeric input
        [X] closes the menu
        [R] reads the variables from EEPROM
        [W] write the variables to EEPROM


    Future - Read/Write Named Parameter Sets




    Variables can be excluded from Read and Write using ve_eeprom(enable) function.


    Sample of Variable Editor Usage

    --------------------------------------------------------------------------------------------

    // first, the demo variables are defined - as usual


    int Kp = 50;
        int Ki = 2;
        int Kd = 20;
        int Ktemp,Gain,TFar,TNear;
    u08 Controls;  // bit address example


    // next, a function is written the variables that are available to the variable editor.
        // including optional groups which are used to facilitate quicker selection of the variables
        // of interest.  

    void ve_varlist(void) {                // demo variable list function
      ve_group ("MOTOR CONTROL");          // group def  uppercase recommended from group names
      ve_int   ("Kp",Kp,0,100);
      ve_int   ("Ki",Ki,0,100);
      ve_int   ("Kd",Kd,-100,100);
      ve_group ("MISC");                   // 2nd group def
      ve_bit   ("HeadLight",Controls,3);   // bit toggle  (can use bit 0 for byte boolean) 
      ve_onoff ("Heater",Controls,2);      // simimar to ve_bit except displays ON/OFF
      ve_eeprom(0);
      ve_int   ("Ktemp",Ktemp,0,10);       // example of excluding a variable from read/write from/to EEPROM
      ve_eeprom(1);
      ve_group ("SONAR");
      ve_int   ("Gain",Gain,1,10); 
      ve_int   ("Threshold Far",TFar,1,10);
      ve_int   ("Threshold Near",TNear,1,10);
    }


    int main(void) {           // variable editor shown here in main 
                               // can be run from any function except interrupt functions
        
                  
 
       ve_init(ve_varlist);    // init varaible list editor using function defined above

       ve_set_port(0);         // optional function call to redirect IO to serial port
                                   // default is LCD  ve_set_port(-1) 


       // next we create a loop that executes until the user exits the variable editor.
           // that is until they press the [X] button on LCD or press "X" on remote terminal if being
           // used.


       while (ve_update()) { 

                 // optional user code here.
                 // When no user input to variable editor (pressing buttons...) 
                                 // the ve_update function executes quickly.
                                 // For good performace of the variable editor. Any code placed here 
                                 // should not take too much time before allowing control to continue 
                         // back to the ve_update() call.

           }


       return 0;

    }

*/
      
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <avr/interrupt.h>  // for cli sei

#include "md.h"

#ifdef VAREDIT_CODE
#include "lcd.h"
#include "menu.h"
#include "timer.h"     // beep
#include "adc.h"       // touch pad 
#include "led.h"
#include "varedit.h"
#include "uart.h"
#include "string.h"   // for strcpy
#include "calc.h"     // for direct numerical input

#define XCOLS COLS
#define XROWS ROWS


void (*ve_user_varlist) (void);

// ve  Variable Editor

u08 veFlags;
u08 veStoreSerialBuffer;   // Flag to copy from SerialBuffer to current variable                  

#define veDO_REFRESH 0
#define veCOUNT_VARS 1
#define veREAD 2
#define veWRITE 3
#define veCALL_INPUT_DIALOG 4
#define veDUMP 5
#define veUPDATE_FROM_SLIDER 6
#define veEEPROM_NO_RW 7

// flag handler macros

#define ve_flag(f)    (bit_is_set (veFlags,(f)))
#define ve_setflag(f) (veFlags |= _BV(f))
#define ve_clrflag(f) (veFlags &= ~_BV(f))
   

void ve_varlist (void);  // forward declaration of user supplied  function


                                  
u08 veCurGroup;          // current group
                                          

u08 veCurVar;        // current variable# within group being viewed/edited
s08 veDelta;         // set -1,+1 when -/+ buttons pressed
                     //   decrement,increment variable on next call to ve_process()

u08 veGroupI;        // index as we iterate through calls for each variable
u08 veVarI;          // these are compared against current VarGroup and VarIndex
                   

u08 veGroupCount;   // # of groups and variables in ve_process()
u08 veVarCount;     // these are used for wraparound calc


long veCurMin;      // currently selected variable's Min,Max and Value
long veCurMax;      // passed to slider control
long veCurVal;

#define VE_BUF_SIZE 8       // number buffer used with serial I/O (not used with LCD) 
char veBuf[VE_BUF_SIZE+1];  // buffer + 1 for terminator
u08  veBufIndex;



int EIndex;         // EEPROM address offset


void ve_begin(void)  // start of list 
{
   veGroupI = 0;  
   veVarI   = 0;
} 

void ve_end(void)    // end of list
{
    veGroupCount = veGroupI;                  // tabulate max for index limit/wraparound
    veVarCount   = veVarI;
}


void ve_process(void) 
{
   ve_begin();
   ve_user_varlist();   // user supplied function continaing variable list as function calls
   ve_end();
}


void ve_eeprom(u08 enable) 
{
   if(enable) ve_clrflag(veEEPROM_NO_RW); 
   else ve_setflag(veEEPROM_NO_RW);
}


void ve_group(char *s) 
{

   if (++veGroupI != veCurGroup) return;   // was post +=  !!! why 
   
   if ( ve_flag(veCOUNT_VARS) | ve_flag(veREAD) | ve_flag(veWRITE))  return;

  

   if (vePort==-1) {
     lcd_setxy(0,CHAR_HEIGHT*3/2);

 
    // Note: parameter s is PS using PS buffer
        // Do not use PS( ) on any of the strings below. Will overwrite s since only
        // one buffer used


      #ifdef RES_128x64
        printf("%s ",s);  // skip showing # of vars in group on 128 pixel display
      #else
        if(veVarCount) printf("%s %d/%d", s, veCurVar, veVarCount);  // show current group label
        else           printf("%s 0/0", s);
      #endif

   

      // pad to erase stray chars 
      // if when say 10/10 loops to 1/10
      if((veVarCount > 9) && (veCurVar < 10)) printf(" ");  
          
   } else {

   
    printf("%s %d/%d   ",s, veCurVar, veVarCount) ;

    
    if (!veVarCount) printf ("\n");


   }
}



// integer variable display update and clip variables to min/max limits
// use ve_int() macro to avoid explicit assignment

long ve_int1(char *s, long v, u08 vsize, long min, long max) 
{
char ns[COLS/CHAR_WIDTH+1];
long p;

   p = 0; 

   if(ve_flag(veREAD)) {  // Read From EEPROM
      if(ve_flag(veEEPROM_NO_RW)) return(v); // this var is disabled from EEPROM RW 
      
      while (!eeprom_is_ready());               
      eeprom_read_block(&p,(void*) EIndex,vsize);
      EIndex += vsize;

//    veVarCount++;   

      // sign extension issues??
      // maybe code datatype in list
      // 8 16 32  signed unsigned      

      switch(vsize) {
         case 1: v = (u08)  p;   break;
         case 2: v = (int)  p;   break;
         case 4: v = (long) p;   break;
      }

      return(v);
   }

   
   if(ve_flag(veWRITE)) {  // Write to EEPROM
      if(ve_flag(veEEPROM_NO_RW)) return(v); // this var is disabled from EEPROM RW 

      while(!eeprom_is_ready());               
      eeprom_write_block(&v,(void *) EIndex, vsize);  // a
      EIndex += vsize;
//    veVarCount++;
      return (v);
   }


   if((veGroupI == veCurGroup) && (++veVarI == veCurVar)) {   // if current variable

     if(ve_flag(veCOUNT_VARS)) return(v);

     if (veStoreSerialBuffer)
         {
       veStoreSerialBuffer = 0;                

       // 0x could do hex scan ?

       veBuf[veBufIndex]   = 0;            // add null terminator
       sscanf(veBuf, "%ld", &v);

     }
     

     if(ve_flag(veUPDATE_FROM_SLIDER)) {
        v = veCurVal;
        ve_clrflag(veUPDATE_FROM_SLIDER);
     }


     if (vePort == -1) {

        if(ve_flag(veCALL_INPUT_DIALOG)) {

#ifdef RES_128x64
           sprintf(ns, "%-13s", s);
           v = menu_calc(ns, NO_MATH|FIX_MODE|NO_DP_CHANGE) / calc_scale;     // invoke calculator for number input/edit with no math
#else
           sprintf(ns, "%-16s", s);
           v = menu_calc(ns, NO_MATH|FIX_MODE|NO_DP_CHANGE) / calc_scale;     // invoke calculator for number input/edit with no math
#endif
//     sprintf(ns, "%ld", v);
//     v = menu_calc(ns, EVAL_TITLE|FIX_MODE|NO_DP_CHANGE) / calc_scale;     // invoke calculator for number input/edit with no math

          ve_clrflag(veCALL_INPUT_DIALOG);
          veDelta = 0;

        }      
      }  
       

        v += veDelta;   // add delta to variable 
        veDelta = 0;    // reset delta

        if(v < min) v = min;
        if(v > max) v = max;


      if (vePort == -1) {


      // Note: on printf statements below -- do not use PS on format strings
          // to save RAM. Buffer allocated by PS macro is being used by parameter s
          // s will be overwritten and you will see the format string appear in output 

#ifdef RES_128x64
         lcd_setxy(0,CHAR_HEIGHT*3);                 
         printf("%s %ld", s, v); // show var label and value
#else
         lcd_setxy(0,CHAR_HEIGHT*4);                 
         printf("%s %ld", s, v); // show var label and value
#endif
      } 


      veCurMin = min;
      veCurMax = max;
      veCurVal = v;

      //if (vePort != -1) printf ("%s : %s %ld \n",veCurGroupStr,s,v);

      if (vePort != -1) printf ("%s %ld \n",s,v);
   }

   if(v < min) v = min;   // clip to min max range regardless of match 
   if(v > max) v = max;   // this applied to every var needed ???
  

   return(v);

   // options for load/save
}



// macro eliminates need to write assignment
#define ve_int(s,v,min,max) v = ve_int1(s, (v), sizeof(v), (min), (max))



long ve_bit1(char *s, long v, u08 BitNum, u08 OnOffLabel)
{
   if(ve_flag(veREAD)) {  // Reading  From EEPROM   
      if(ve_flag(veEEPROM_NO_RW)) return(v); // this var is disabled from EEPROM RW 

      while (!eeprom_is_ready());               

      if(eeprom_read_byte((void*) EIndex++)) v |= (1<<BitNum);
   }
   else if(ve_flag(veWRITE)) {  // Writing To EEPROM
      if(ve_flag(veEEPROM_NO_RW)) return(v); // this var is disabled from EEPROM RW 

      while (!eeprom_is_ready());               

      if(bit_is_set(v,BitNum)) eeprom_write_byte((void*) EIndex++,1);
      else                     eeprom_write_byte((void*) EIndex++,0);
   }
   else if((veGroupI == veCurGroup) && (++veVarI == veCurVar)) {   // if current variable

      if(ve_flag(veCOUNT_VARS)) return(v);  // iterating just to get counts

      veCurVal = MAXLONG;   // flag to not draw slider
          
      if(veDelta < 0) v &= ~_BV(BitNum);  // - turn off
      if(veDelta > 0) v |= _BV(BitNum);   // + turn on

      veDelta = 0;

#ifdef RES_128x64
      lcd_setxy(0,CHAR_HEIGHT*3);                 
      printf("%s ", s);
#else
      lcd_setxy(0,CHAR_HEIGHT*4);                 
      printf("%s ", s);
#endif

      if(bit_is_set(v,BitNum)) {
        if(OnOffLabel) printf("ON");
        else           printf("1");
      } 
      else {
        if(OnOffLabel) printf("OFF");
        else           printf("0");
      } 
   

      if (vePort != -1) printf (PS("\n"));
   }

   return(v);
   // options for load/save
}


#define ve_bit(s,v,bit)   v = ve_bit1(s, v, (bit), 0)
#define ve_onoff(s,v,bit) v = ve_bit1(s, v, (bit), 1)



u08 ve_read_write (char cmd, u08 display)
{
   int d;

   EIndex = EE_VAREDITOR;          // starting address

 
   if(cmd=='W') {  // Write to EEPROM

      // write marker first so that erased EEPROM can be detected
      // this will make an assumption that EE_VAREDITOR has not been changed as would
      // be possible if flashing new program code, but leaving EERPOM untouched

          // checksum might be nicer here, but this is simple

      d = 0xFACE;
  
      while(!eeprom_is_ready());               
      eeprom_write_block(&d,(void *) EIndex,2);  // write marker
      EIndex += 2;
   }

   // if Reading, look for marker

   if (cmd=='R') {
      while (!eeprom_is_ready());               
      eeprom_read_block(&d,(void*) EIndex,2);
      if (d != 0xFACE) return (0);    // EEPROM does not contain saved var editor data
      EIndex+=2;
   }      


   if(vePort==-1) {
     if(display) {
        lcd_clear();
        if (cmd=='R') lcd_textPS(0,0,"Reading EEPROM");
                else           lcd_textPS(0,0,"Writing EEPROM");
     }
   }
   else  
   {   if (cmd=='R') printf(PS("\nReading EEPROM..."));
       else           printf(PS("\nWriting EEPROM..."));
 
   }

   if (cmd=='R') veFlags = _BV(veREAD);
   else          veFlags = _BV(veWRITE);

   ve_process();            // iterate through all vars
   veFlags = 0;

   if (vePort==-1)
   { if(display) delay_ms(1000); } 
   else
   printf (PS("Finished\n\n"));

   return (1); // success

}




// CurVal is updated by menu_slider macro

void varedit_slider(void)
{ 
  if (vePort!=-1) return;

#ifdef RES_128x64
   COORD x  = 2;
   COORD y  = XROWS-CHAR_HEIGHT*4+4;
   COORD w  = XCOLS-5*CHAR_WIDTH-4;
#else
   COORD x  = 2;
   COORD y  = XROWS-CHAR_HEIGHT*5+4;
   COORD w  = 160-5*CHAR_WIDTH-4;
#endif

   if(veCurVal == MAXLONG) {  // erase display slider
      set_color(color ^ WHITE);
      filled_box(x,y,x+w,y+CHAR_HEIGHT);
      set_color(color ^ WHITE);
   } 
   else {
      menu_slider(x,y,w,                 //  x,y position, control width
               "", veCurVal,             // optional label, int var
               veCurMin,veCurMax,0,1);   // range min,max , show value, response code
   }
}





void ve_init(void varlist(void))
{
   ve_user_varlist = varlist;
   ve_set_port(-1);  // LCD default   can overrride later with serial ports 0,1,2

   // ve_varlist_func = user_var_list_func;  // assign user function  

// set_color(WHITE);

   veCurGroup = 1;
   veCurVar   = 1;
   veCurVal = MAXLONG;
   veFlags  = 0;       // all clear

//   ve_setflag(veDO_REFRESH);   // used to display first variable
  
   MENU_INIT
}




// menu update function called periodically

// !!! below changed
// if SerialCmd param. is 0 then assmue using menu based LCD interface
// otherwise assume serial console 


u08 ve_update(void )  // do we need param?
{

  COORD mds;   
  u08   cmd;

 
  if (vePort == -1) { // LCD GUI

     MENU_CONTROLS 
  
     menu_exitbutton();

     mds = (CHAR_WIDTH*BUTTON_SIZE) + ButtonBorder + ButtonRowSpacing;   //!!! ButtonBorder*2
     #ifdef RES_128x64
       menu_label    (0,0,PS("Var Editor"));
       menu_buttonrow(5,XROWS-(CHAR_HEIGHT+6),  PS("G \xAE\xAF #WR"));
       menu_button   (5+mds*6,XROWS-CHAR_HEIGHT*4+2, PS("+"),'+');
       menu_button   (5+mds*7,XROWS-CHAR_HEIGHT*4+2, PS("-"),'-');
     #else
       menu_label    (0,0,PS("Variable Editor"));
       menu_buttonrow(5,XROWS-(CHAR_HEIGHT+6),  PS("G \xAE\xAF  # WR"));
       menu_button   (5+mds*8,XROWS-CHAR_HEIGHT*4-6, PS("+"),'+');
       menu_button   (5+mds*9,XROWS-CHAR_HEIGHT*4-6, PS("-"),'-');
     #endif

     
      varedit_slider();

      if(MenuDraw) ve_setflag (veDO_REFRESH);  // if MenuDraw (set by menu_init) then screen was cleared and controls just
                                            // redrawn, so redraw current variable
 
      MENU_COMMANDS  

          cmd = menu_cmd();

   } else  // Serial Console
   {  
      MenuCmd = 0; // make sure disabled

      cmd = 0;        
   
      switch (vePort) { 
        case 0 : if (rx_haschar()) {cmd = rx_getchar(); beep(20,1000); }
                         break;

            case 1 : if (rx1_haschar()) {cmd = rx1_getchar(); beep(20,1000); }
                         break;
      }
           
          if ((cmd>='a') && (cmd<='z')) cmd -=32; // shift to uppercase 
         

      // handle numeric entry using local buffer

          if ((cmd>='0') && (cmd<='9') && (veBufIndex < VE_BUF_SIZE))
          {
             veBuf[veBufIndex++] = cmd;
                 printf("%c",cmd);  // echo character
                 cmd = 0;           // prevent refresh  (display of group / var)
      }
          
      // handle backspace

      if ((cmd ==8) && (veBufIndex>0)) {
              veBufIndex--;
                  printf("%c",cmd); 
                  cmd = 0;   // prevent refresh
      }
                  
      
      if (cmd==13) {
            veStoreSerialBuffer=1;   
            // leave cmd alone to force refresh
      }
   
   }
   // menu button/control responses

  

   switch(cmd) {
      case 1   :  // slider 
         ve_setflag(veUPDATE_FROM_SLIDER);  // current var will set to slider value 
         break;                    // on next call to ve_process()

      // select next group
      case 'G' :  
         if(++veCurGroup>veGroupCount) veCurGroup=1;
                
         veCurVar = 1;
         veCurVal = MAXLONG;

         ve_setflag(veCOUNT_VARS);
         ve_process();
         ve_clrflag(veCOUNT_VARS);
         
       

         break;  // next group

   
      // select previous / next variable 
          // special arrow characters  0xAE  <-    0xAF  ->

      case '<':;  // greater than or ".' char (for Serial Console Support)
          case ',':;
      case 0xAE: if(veVarCount && (--veCurVar==0)) veCurVar=veVarCount;
                     break;  // prev variable

          case '>':;
          case '.':;
      case 0xAF: if(veVarCount && (++veCurVar>veVarCount)) veCurVar=1;  break;  // next variable

      case '=':;
      case '+': veDelta = ( 1); break;   // flag for inc/dec of current variable  

          case '_':;
      case '-': veDelta = (-1); break;

      case '#': ve_setflag(veCALL_INPUT_DIALOG); break;  // flag 

      case 'W':
          case 'R': ve_read_write(cmd,1);      // do 'R'-Read or 'W'-Write of vars from/to EEPROM
                    menu_init();
                                ve_setflag(veDO_REFRESH);
                                break;
      

      case 'X': MenuCmd = MENU_EXITCODE;  // Close Variable Editor  (Remote Serial Console)
                    printf (PS("Exit. Thank You for using Megadonkey Variable Editor\n"));
                                break;

   }

       
   if(ve_flag(veDO_REFRESH) || (((cmd!=0)||menu_cmd()) && (menu_cmd() != MENU_EXITCODE))) {

      if(ve_flag(veDO_REFRESH)) {
         ve_setflag(veCOUNT_VARS);  // do a variable count pass through list 
         ve_process();              // really this could be done one time with static defs
         ve_clrflag(veCOUNT_VARS);  // thinking toward dynamic list
         ve_clrflag(veDO_REFRESH); 
      }


      if (vePort==-1)  // output to LCD 
          {

         // if 'G' command erase group label and variable label
         // if not just erase variable label


         set_color(color ^ WHITE);
         if(menu_cmd() == 'G') {
            filled_box(0,CHAR_HEIGHT*3/2,XCOLS-1, CHAR_HEIGHT*3/2+CHAR_HEIGHT-1); 
         }
         #ifdef RES_128x64
           filled_box(0,CHAR_HEIGHT*3, XCOLS-1,CHAR_HEIGHT*4-1);
         #else
           filled_box(0,CHAR_HEIGHT*4, XCOLS-1,CHAR_HEIGHT*5-1);
         #endif
         set_color(color ^ WHITE);
          
          
         ve_process();      // update display of current var & update slider vals
               
         mds = MenuDraw;    // save MenuDraw state
         MenuDraw = 1;      // force redraw of slider
         varedit_slider();  
         MenuDraw = mds;    // restore MenuDraw state  
       }   
       else // Output to Serial Port
       {
          if (cmd != 0) ve_process(); // update display of current var : output to serial port

       }



   }  
                                       

   if ((menu_cmd() == MENU_EXITCODE) && (vePort != -1))
   { lcd_set_stdout();         // ASSUME LCD was connected to std out (maybe should have saved state)
     printf (PS("VE Exit"));
   }


             
   return (menu_cmd() != MENU_EXITCODE);  // return TRUE unless close [X] 

 
}



void ve_set_port(s08 device)
{
  vePort = device;

  if (vePort != -1) { 
     lcd_clear(); printf (PS("Var Edit Port%d"),vePort); 
  }
  else lcd_set_stdout();  // make sure LCD is selected 
 
  switch (vePort) { 
     case 0 : uart0_set_stdout();
                  break;
     case 1 : uart1_set_stdout();
                  break;

   }

   if (vePort != -1) {
      printf (PS("\fVariable Editor  MegaDonkey COM%d \n"),vePort);
          printf (PS("[G] Next Group  [<][>] prev next var\n"));
          printf (PS("[+/-] change var OR enter digits followed by [Enter]\n"));
          printf (PS("[R] Read EEPROM [W] Write EEPROM\n"));
          printf (PS("[X] Exit\n\n"));
   }      

}





#ifdef VAREDIT_DEMO

// ---------------------------------------------------------------------------------------------------
// Variable Editor Demo Code

// variable processor 
// this is a list of all variables that are browsed in menu_varedit()
// this list is used from eeprom load / save with allowance for exclusion by
// vab_eeprom() function
// 

// later support other types  
// option1 add format string to title
// option2 add unique calls for each type (did this in past)
//
// allow edit range e.g. 0,100 



// demo variables
int Kp,Ki,Kd,Ktemp,Gain,TFar,TNear;
u08 PWM_Max = 85;
u08 Mode;
u08 Controls;  // bit address example
u08 BeepDur = 50;
u16 BeepFrq = 2400;
u08 BeepEnable = 0;  // 
u08 PWMDirect = 1;
u08 UpdateDelay = 5;
int PWML,PWMR;


// varlist function called by variable editor 
// variables appear as a list of function calls 
// labels must be short enough that label and value fit on sample line


void ve_sample_varlist(void)
{
   ve_group (PS("MOTOR CONTROL"));

   ve_onoff (PS("PWM Direct"), PWMDirect,0);
   ve_int   (PS("PWM Left Motor"), PWML,-128,128);
   ve_int   (PS("PWM Right Motor"), PWMR,-128,128);

   ve_int   (PS("Update 100ths"), UpdateDelay,1,50);

   ve_int   (PS("Kp"), Kp,0,100);
   ve_int   (PS("Ki"), Ki,0,100);
   ve_int   (PS("Kd"), Kd,-100,100);
   ve_int   (PS("PWM Max"), PWM_Max,0,127);
   ve_int   (PS("Mode"), Mode,1,3);
   ve_bit   ("HeadLight",Controls,3);   // bit toggle  (can use bit 0 for byte boolean) 

   ve_eeprom(0);
   ve_int   (PS("Ktemp"), Ktemp,0,10);  // don't write to EEPROM
   ve_eeprom(1);

   ve_group (PS("SONAR CONTROL"));
   ve_int   (PS("Gain"), Gain,1,10); 
   ve_int   (PS("Threshold Far"), TFar,1,10);
   ve_int   (PS("Threshold Near"), TNear,1,10);

   ve_group (PS("SOUND CONTROL"));
   ve_onoff (PS("Beep "), BeepEnable,0);
   ve_int   (PS("BeepFrq"), BeepFrq,100,3000);

   ve_group (PS("LED CONTROL"));
   ve_int   (PS("LED2 Inten "), LEDInten[2-1],0,255);
   ve_int   (PS("LED3 Inten "), LEDInten[3-1],0,255);

   ve_group (PS("TOAD"));
   ve_onoff (PS("Toad Heater"),Controls,2);      // simimar to ve_bit except displays ON/OFF
   ve_onoff (PS("Toad Humidifier") ,Controls,3);
   ve_onoff (PS("Toad A/C"),Controls,7);
   ve_eeprom(0);
   ve_int   (PS("ToadCtrlsDiag"),Controls,0,255);  // diagnostic viewer of var
   ve_eeprom(1);
}



void varedit_demo(void)
{
unsigned long LastMainLoopUpdate;
u08 pwm_state;

   LastMainLoopUpdate = 0;
  
#ifdef LED_CODE
   pwm_state = LED_PWM_State;
   led_pwm(PWM_ON);   // enable PWM for LEDs 
   led_set(2, 0);
   led_set(3, 0);     // var editor will operate on LEDInten arrays directly, no code appears in loop
#endif

#ifdef MOTOR_CODE
   motor_init();
#endif

   ve_init(ve_sample_varlist);   // init variable editor pass varlist function ref.(see: above)
   ve_read(0);                   // load variables saved in EEPROM

   while(ve_update()) {          // ve_update called periodically to handle inputs..
      if(get_time_alive_100ths() > (LastMainLoopUpdate + UpdateDelay)) {  
         LastMainLoopUpdate = get_time_alive_100ths();
         // this code executes at rate determined by UpdateDelay  e.g. 5  =  20 Hz
          if(BeepEnable) beep(BeepDur, BeepFrq);

          if(PWMDirect) {
#ifdef MOTOR_CODE
             motor_pwm('L',PWML);
             motor_pwm('R',PWMR);
#endif
          }    
       }
   } 

#ifdef LED_CODE
   if((pwm_state & ENABLED) == 0) {  // we started with PWM off
      led_pwm(PWM_OFF);              // so disable led PWM mode
   }
#endif

   wait_while_touched();
   lcd_clear(); 
}

#endif // VAREDIT_DEMO

#endif // VAREDIT_CODE
