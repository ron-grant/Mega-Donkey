/*  MegaDonkey Library File:  varedit.h   Veriable Editor - Header File
    


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

#ifndef _VAREDIT_H
#define _VAREDIT_H

#include "md.h"

#ifdef VAREDIT_CODE


s08 vePort;                  // -1=LCD (default)  0,1,2 use serial port #0,#1,#2


void ve_init (void varlist(void));  // init variable list must be called before using functions documented here
                                    
u08 ve_update(void) ;               // called periodically -- process input update browser
                                    
void ve_set_port(s08 device);       // select IO device -1 = LCD (default)   0,1,2 serial port



void ve_group(char *s); 

long ve_int1(char *s, long v, u08 vsize, long min, long max); 

#define ve_int(s,v,min,max) v = ve_int1(s, (v), sizeof(v), (min), (max))


long ve_bit1 (char *s, long v, u08 BitNum, u08 OnOffLabel);

#define ve_bit(s,v,bit)   v = ve_bit1(s, v, (bit), 0)
#define ve_onoff(s,v,bit) v = ve_bit1(s, v, (bit), 1)


#define ve_read(display) ve_read_write ('R',(display))  // read  variables from EEPROM 
#define ve_write(display) ve_read_write('W',(display))  // write variables  display  0=quiet
                                                        //                           1=display
  
u08 ve_read_write (char cmd, u08 display);  // return 1 if success 
                                            // return 0 if read fails to find marker
											// indicating no save performed before

void varedit_demo(void);    // sample usage of Variable Editor


#endif // VAREDIT_CODE
#endif // _VAREDIT_H
