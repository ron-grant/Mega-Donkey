/*  MegaDonkey Library File:  md.h    MegaDonkey Init Code & Main for non-library compilation -- Header File 
                                                                                               

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


   LIBRARY / DEMO PROJECT BUILD INSTRUCTIONS


   This file contains switches for building different versions of Mega Donkey code supporting different
   displays and input devcies.

   It is designed to compile into a single executable program - OR - into a set of modules that can be
   archived into library.


   FIRST, this file "md.h" is the "key header file" for the Mega Donkey project "donkey.aps".
   Load the donkey project (found in md_src folder) into AVR Studio AND MAKE SURE the 
   Configuration Options set in the Project Menu have the correct processor selected.
   
   YOU WON'T WANT/NEED  header file path to md_inc OR have the libdonkeyXXX_VER.a included in your library path
   that is needed for all of the stand alone demos when building this project as either library or
   stand alone project.


   If LIBRARY_BUILD is left undefined (defined below) this file contains a main function
   and compiles into a project which will contain demos specified in md.h and all supporting 
   code. This large project was used during the development of MegaDonkey and is very useful
   if doing library code development saving the step of building a library each time you compile.
  
   If LIBRARY_BUILD is defined the main function will be left out of the compile and thus a 
   "successful" compile will generate one error "undefined reference to 'main'.
   After the compile, executing "_MakeDonkeyLib.bat" will copy all object files into a library file
   (archive .a file).

   NOTE: Verify the correct path to WinAVR is present in the _MakeDonkeyLib.bat file. 
   This release used: \WinAVR-20071221

   
   RENAME THE libdonkey.a  FILE CREATED BY THE COMPILE OF THE DONKEY PROJECT to 

   libdonkey2561_VER.a  if you are using Mega2561
   libdonkey128_VER.a   if you are using Mega128

   where VER=version# for distributions made by original authors
   e.g. libdonkey2561_103  for version 1.03 


   OR some other name suiting your purpose

   
   EXECUTE _COPYLIBSHEADERS.BAT to copy libs to md_lib  and headers to md_inc  
   

   
   CREATING YOUR PROJECTS
   
   Study demo projects in md_demos folder - looking at their Project Configuration Options...
   Note that:
   
   When creating a project using libDonkey.a you must add it to the list of libraries
   using the Project command followed by Configuration Options then Libraries.

   A library search path must be added to the md_lib folder OR a particular libdonkeyXXX.a
   can be copied to the local folder you are using for your project.
  
   When adding the path, you will see the libdonkeyXXX.a files available in md_lib
   Select the appropriate libdonkeyXXX.a file  then press "Add Library ->" button.

   Make sure libDonkeyXXX.a precedes libm.a (why? I don't know). Clicking on libDonkey2561.a for example
   then pressing MoveUp to move it above libm.a

   lib.c
   libDonkey2561.a
   lib.m  
  
   
   An Include Directories search path must be added to "md_inc" folder.


   REMEMBER TO EDIT HEADERS IN MD_SRC not in MD_INC since they are copied from
   md_src to md_inc.


   Using AVR Studio 4.19 encountered problems (bug) with AVR Studio finding compiler / make


   In Project|Configuration Options Custom Options (bottom left icon in left window pane)

   UnCheck USE AVR Tool Chain and set these paths


   C:\WinAVR-20071221\bin\avr-gcc.exe
   C:\WinAVR-20071221\utils\bin\make.exe


   See Also, AVRStuio_WinAVR_Readme.txt in \MD folder




   ---------------------------------------------------------------------------------------------------------
  

   Mega Donkey Touch LCD Controller
  
       
   Features:
   

   Graphics Primatives (AGM Panel, Samsung Panel and others)
   Lines/Circles/Rectangular Fills/Region Fill Paint/BitBlts/BigFont/FontRotation

   Vector Font Support (Uppercase Only -- variable scale)

   Menu System Buttons/Sliders/Checkboxes 
   (Supported by Windows Application "Donkey Wiz" -- Interactive Menu Designer)

   Keyboard / Line Editor
   Donkulator (Calculator / Numeric Entry Terminal)
   Sketch Pad
   
   RC Servo Control
   I2C Interface Driver

   Vector Graphics -- 2D Rotation, Clipping 
   
   LED Dimmer (PWM Control)

   Fast integer Sine/Cosine via lookup table
    
   Device  Setup Menus
   Enhanced UART code RTS/CTS and Xon Xoff protocols supported
   Variable Editor

   Note: All features are included in one or more demos.

   Demos are included paired with supporting module and also separately from
   this project where the core code is compiled into a library then linked to demos.


   Revision Authors  
   RG Ron Grant
   MS Mark Sims
 

12/22/2007  RG - added indirect calls for timer 1 and timer 3 output compare interrupts
            SIGNAL routines defined in timer.c for all 6 interrupts.
            servo1.c and servo3.c illustrate usage. No attempt is made to chain calls except
			servo1 and servo3 both allow user routine to be linked to their update handler
			which executes at 400 Hz (2.5 ms)

         
3/03/2008   RG - revised above with macro definition in header file for each servo1 and servo3
            where user program includes SIGNAL function defintion via inclusion of macro defined in 
			servo1.h and servo3.h.
			See demo code at end of servo1.c and servo3.c OR see stand alone demo code in md_demos folder.

8/21/2008   RG - restored code for 100Hz interrupts
            in timer.c  10KHz service routine (Timer0 Overflow routine)
            Library 1.03 

9/16/2007   MS - revised vector character generator for full "PC Character Set"
            Library 1.04

			Old vector modules retained within library  vchar1.c vchar1.h

02/06/2009  RG corrected timer1_set_input_capture interrupt enable for MEGA128
            (timer.h), added md_init_no_logo(); as alternate to md_init
			Library 1.05  


3/30/2009   RG added md_InhibitPortPullUp variable. Default, 0, leaves all ports
            pulled up and input when hw_init executes. Specific ports can have MCU 
			pullups disabled e.g. md_InhibitPortPullUp=2 prevents port B from being 
			pulled up on init. This was done to address a particular situation with 
			PWM output controlling a laser power supply that also responds to analog 
			value. That is, even pulling the pin down with 2K resistor resulted in 
			voltage to supply firing laser.
			Library 1.06


10/12/2009  RG added  duParamMode to curves.h allowing      
            du values to be passed versus steps to curve functions.
			du (32 bit float) must be cast to long  without conversion 
			union might be preferred method -- not used.


02/08/2012  Library 1.09
            RG fixed md_term, sending chars to serial port when not enabled
            see md_term.c. When button pressed was sending FF 28 00 00 to serial port
			via call to set_topline() from menu_button()

			Added Mark's code for scrollbox to menu.c
			allows controls to be scrolled by touching and dragging screen
			must be last control defined.



			
*/

#ifndef _md_H_
#define _md_H_



// TRACKBALL ifdef below -- commented out???


/* if LIBRARY_BUILD not defined then this project will compile 
   as an application with whatever demos selected.

   otherwise (if defined) forces all demos to be undefined  and all moudles 
   to be defined (included) in compile

   See notes above in header
*/


#define LIBRARY_BUILD  // see: above
#define LIBRARY_VERSION "1.09" 

// IF YOU MAKE MODS, YOU MIGHT WANT TO ADD A DISTINGUISHING LETTER TO VERSION #....
// SO AS TO IDENTIFY TO YOURSELF YOU ARE WORKING WITH MODIFIED LIBRARY CODE.



//#define LED2_PF5  // define this if you burned out LED2's drive pin (PD6)
                    // and wired PF5 up as a substitute
					// a bit of humor? I dropped 24 volts on an LED pin and it killed the pin, but the 
					// MCU lived. (one donkey in the world needs this define)


// define one of these two for standard Donkey Models
//#define AGM_DONKEY
#define EL_DONKEY


// Define what kind of LCD controller chip the panel uses
// You must select (only) one type of controller chip
// AS OF MARCH 2008 YOU PROBABLY HAVE ONE OF THE FIRST TWO MODELS

// Added this auto select code (see AGM and EL donkey defs above)

#ifdef AGM_DONKEY
  #define KS0107
#endif

#ifdef EL_DONKEY
  #define LC7981
#endif


//#define LC7981        // Epson LC7981 controller (Samsung EL panel)       
//#define KS0107      // Samsung KS0107/0108 controller (AGM1264 panel)

//#define KS0713      // Samsung KS0713/S6B1713 controllers (CrystalFontz CFAX panels)
//#define TOSHIBA     // Toshiba T6393 controller (CrystalFontz panel)
//#define SED         // Epson SED1355 controller (larger LCD panels)
//#define NOKIA       // Epson S1D15G10 controller (small color LCDs)
//#define OLED        // SSD1339 OLED controller chip
//#define LCD_MDT     // no LCD panel - serial port MegaDonkeyTerminal only



#ifdef LC7981
//#define CF          // CrystalFontz 160x80 and 240x128 panels on AGM Bus
//#define BIG_PANEL   //!!!!! CrystalFontz 240x128 panel
                      // Note: Do not connect contrast POT to 240x128 panel
					  // With pot connected you will get poor contrast at best.
              
#endif

#define TIMER_CALL_ADC  // timer0 10KHz service routine calls adc_10KHz_update function
                        // versus using interrupt routine SIGNAL(ADC_INT)
						// allowing user to disable timer call and define their own
						// interrupt service routine OR directly poll ADC for results...
						// New: January 2008 (RG)  see adc.h,adc.c
						// and timer.c for addition of #ifdef TIMER_CALL_ADC and call

//  INLINE_HW_ACCESS
//  If #defined, uses macros to generate inline code that accesses the panel
//  hardware.  Runs (usually 10-15%) faster, generates bigger code:
//      KS0107  +1.3 Kb
//      OLED    +1.3 Kb
//      LC7981  +2.8 Kb
//      TOSHIBA +3.6 Kb
//      SED     +6.0 Kb
//  Tradeoff may not be worthwhile on the SED controller (5% speedup for 6Kb).
//  (but then it is a big display so every little bit helps).  An alternative
//  would be to have both inline macros and subroutine code.  Call the 
//  subroutines in init/seldom used code and use the inline macros in the
//  drawing functions.

//#define INLINE_HW_ACCESS



// Define what kind of user input device you are using
// You cannot enable more than one type of device
// note: see auto defines that follow for AGM_DONKEY and EL_DONKEY

//#define TOUCH_SCREEN   // define this for analog touchscreen read via ADC2, ADC3 
//#define MOUSE          // define this for mouse/trackball - select what uart below
//#define JOY_SWITCH     // define this for 5-way joyswitch on touch panel connector
//#define CANON_BALL
//#define DONKEY_ALPS_BALL // define this for mega donkey alps mini track ball (Ball/Joy Board)
                         // Note: this causes MOUSE to be defined, see below

#ifdef AGM_DONKEY
  #undef TOUCH_SCREEN
  #define MOUSE
#endif

#ifdef EL_DONKEY
#ifndef TOUCH_SCREEN
  #define TOUCH_SCREEN
#endif
#endif


// Define Port and Bit assignments for Software UART (SW_UART)
// Note: PE7 is generally used for Software UART receive.
// It is located in middle of PORT0/PORT1 jumper header along with +5 and GND
// Software UART commonly used on non-touch Mega Donkeys by Donkey Ball typically
// Also, available as 3rd serial port (TTL level) to Touch Panel versions of Mega Donkey.

                              // Receive Port Defs     (PE7 standard)
#define SW_UART_RPORT PORTE   // receive port
#define SW_UART_RPIN  PINE    // input port for receive
#define SW_UART_RDDR  DDRE    // DDR for port
#define SW_UART_RBIT 7        // bit number for port  

                              // Transmit Port Defs 
#define SW_UART_TPORT PORTD   // xmit port
#define SW_UART_TDDR DDRD     // DDR for xmit
#define SW_UART_TBIT 6        // bit number for xmit


#ifdef DONKEY_ALPS_BALL
  #define MOUSE
#endif


#ifdef MOUSE             // the following for non-touch screen Donkey, e.g. AGM_DONKEY 
// #define MOUSE_UART0   // mouse on hardware com0
   #define MOUSE_UART1   // mouse on hardware com1
// #define MOUSE_UART3   // software uart (default PE7, see SW_UART_xx setting above)

   // note: there is no UART2

   #define USER_INPUT
   #define user_input_init() mouse_init()
   #define touch_avail()   (0)  //!!!!(mouse_rcvcount >= 3)
#endif

#ifdef TOUCH_SCREEN
   #define USER_INPUT
   #define user_input_init() touch_init()
   #define MouseLB 0
   #define MouseRB 0
   #define touch_avail()   ((ADCData[TOUCH_X_CHAN] >= 0) && (ADCData[TOUCH_Y_CHAN >= 0))
#endif

#ifdef JOY_SWITCH
   #define USER_INPUT
   #define user_input_init() joy_init()
// #define MouseLB 0
// #define MouseRB 0
   #define JOYSWITCH_PIN_POWER  // power joyswitch from two CPU pins
   #define touch_avail()  (joy_qcount > 0)
#endif

#ifdef CANON_BALL
   #define USER_INPUT
   #define user_input_init() joy_init()

// #define MouseLB 0
// #define MouseRB 0

//   #define JOYSWITCH_PIN_POWER  // power canonball from two CPU pins
   #define touch_avail()  (0) // (joy_qcount > 4)
#endif

#ifndef USER_INPUT
   #define user_input_init()
   #define touch_avail() (0)
#endif


// CPU clock in Hz
#define CPU_CLOCK 16000000L


// COM port protocol bits
#define NINE_BITS     0x80    /* flag set if attempt to use 9 bit chars */
#define USE_RTS       0x40    /* use RTS line to shut up the sender */
#define USE_CTS       0x20    /* use CTS line to make us shut up */
#define USE_XON       0x10    /* use XON/XOF to make us shut up */
#define TIMEOUT_MASK  0x0F    /* 0..15 seconds for com transmit timeout */

#define XON  0x11
#define XOFF 0x13

// COM port error conditions
#define RX_OVERFLOW    0x80
#define TX_TIMEOUT     0x40
#define FRAME_ERROR    0x10
#define OVERRUN_ERROR  0x08
#define PARITY_ERROR   0x04


// EEPROM data locations
#define EE_TOUCH_CAL   0x10   /* eeprom address of touch screen cal factors */
#define EE_SET_TIME    0x1C   /* initial time set button state */
#define EE_INVERT_BUT  0x1D   /* initial invert button state */
#define EE_SOUND       0x1E   /* initial sound state */
#define EE_BACKLIGHT   0x1F   /* initial backlight state */
#define EE_COM0        0x20   /* COM0 parameters */
#define EE_COM1        0x28   /* COM1 parameters */
#define EE_CLKDRIFT    0x30   /* time clock drift adjust (msecs/hr: -999..999) */
#define EE_VAREDITOR   0x0400 /* Start of Variable Editor Space  -- move this down as needed */
#define EE_MENUS       0x0800 /* stored menu demos */
#define EE_CHECKPOINT (0x1000-6)  // checkpoint RAM dump to eeprom
#define EE_FLASH_SUM  (EE_CHECKPOINT-2)


// set bit and clear bit macros 
// These macros gen optimal code when gcc compiler optimization is turned on.
// _BV macro transforms bit# to bit mask  e.g. 4 -> 0x10
#define sbi(reg,bit) reg |= _BV(bit)
#define cbi(reg,bit) reg &= ~_BV(bit)


// useful data type definitions
typedef unsigned char u08;
typedef unsigned int  u16;
typedef unsigned long u32;
typedef signed char   s08;
typedef signed int    s16;
typedef signed long   s32;

#define MAXINT 0x7FFF
#define MAXLONG 0x7FFFFFFF

                             
// global functions
void misc_setup(u08 force_menu);
void lcd_backlight(u08 val);

u08 check_point(u08 flag);
u08 check_flash(u08 halt_flag);


// md_InhibitPortPullUp  new 3/30/2009
// default,0,  is to set all ports to input and pull up
// setting this variable before calling md_init() 
// allows removal of pull up e.g. InhibitPortPullUp=2 cancels
// port B pullup.  arbitrary masking of pullups on other ports might cause
// unexpected results. PortB is safe because used for speaker only
						    
u08 md_InhibitPortPullUp;      // Mask D6:D0   xGFE DCBA


void md_init(void);            // initialize Mega Donkey  incl. LCD / port setup..
void md_init_no_logo(void);    // same as above, but skip logo  new in v1.05
       
void do_demos(void);

void lcd_term(u08 flags);
void trackball_demo(void);
u08 mouse_init(void);


// 
// These #defines control what module contents are compiled.
// If you don't need a module, you can save space by disabling it here.
//
// NOTE: See List Below of code included if LIBRARY_BUILD

#ifndef LIBRARY_BUILD
  #define PANEL_CODE         // the LCD panel driver / graphics
  #define ADC_CODE           // a/d converter (enabled automatically for TOUCH_SCREEN and JOY_SWITCH)
  #define TIMER_CODE         // hardware timer (a lot of stuff uses this)
  #define TIME_CLOCK         // time-of-day clock (needs TIMER_CODE)
  #define UART_CODE          // hardware uarts (finer code control in uart.h)
  #define LED_CODE           // light emitting diodes


  #define CKPT_CODE          // system RAM checkpoint dump
  //#define CKSUM_CODE         // flash rom checksum verification

  #define VCHAR_CODE         // vector characters 
  #define GRAPH_CODE         // 2D graphics

  #define MENU_CODE          // menu functions (a lot of stuff depends upon these
  #define VAREDIT_CODE       // variable editor
 


  //#define TRACKBALL_CODE     // trackball/mouse support (handled by dependencies listed below)
  //#define SINE360_TABLE      // 360 sample 8 bit sine wave table (handled by dependencies listed below)
  //#define SINE256_TABLE      // 256 sample 8 bit sine wave table (handled by dependencies listed below) 
  
  //#define TWI_CODE           // two wire (IIC) interface
  //#define TERMINAL_CODE      // serial port graphics terminal
  //#define BEZIER_CODE        // bezier curves (uses floating point)
  //#define SERVO_CODE         // model servo contoller
//#define SERVO1_CODE        // RC servo controller Timer 1 Jitter Free 2 channel only 
//#define SERVO3_CODE        // RC servo controller Timer 3 Jitter Free 2 channel only
                               // note: can use both SERVO1 and SERVO3
  //#define MDT_CODE           // Mega Donkey Terminal Code (serial link to PC Mega Donkey Term)
                               // code auto included if MDT_DEMO included
#else // IF LIBRARY_BUILD

  /* We want all the code to be compiled into the library, hence all the code
     switches are turned on when doing a library build.
	 
     When using the library, if NO functions or globals for a given module are called/
	 referenced then the linker excludes that module and its variables from the link;
	 otherwise, everything from the module is included.

	 Note: that if you include a .c file in your project that the code will be placed 
	 in the executable regardless if it is referenced.

 */

  #define LOGO_DEMO          // the one demo we include to draw donkey

  #define PANEL_CODE         // the LCD panel driver / graphics
  #define ADC_CODE           // a/d converter (enabled automatically for TOUCH_SCREEN and JOY_SWITCH)
  #define TIMER_CODE         // hardware timer (a lot of stuff uses this)
  #define TIME_CLOCK         // time-of-day clock (needs TIMER_CODE)
  #define UART_CODE          // hardware uarts (finer code control in uart.h)
  #define LED_CODE           // light emitting diodes

  #define CKPT_CODE          // system RAM checkpoint dump
  #define CKSUM_CODE         // flash rom checksum verification

  #define BEZIER_CODE        // Bezier Curves
  #define VCHAR_CODE         // vector characters 
  #define GRAPH_CODE         // 2D graphics

  #define MAP_DEMO           // consider pulling this 

  #define MENU_CODE          // menu functions (a lot of stuff depends upon these
  #define VAREDIT_CODE       // variable editor

//#define TRACKBALL_CODE     // trackball/mouse support (handled by dependencies listed below)
  // !!!  Above Include?

  #define SINE360_TABLE      // 360 sample 8 bit sine wave table (handled by dependencies listed below)
  #define SINE256_TABLE      // 256 sample 8 bit sine wave table (handled by dependencies listed below) 

  #define SPI_CODE           // serial peripheral interface
  #define TWI_CODE           // two wire (IIC) interface
  #define TERMINAL_CODE      // serial port graphics terminal
  #define BEZIER_CODE        // bezier curves (uses floating point)


  #define SERVO1_CODE        // RC servo controller Timer 1 2 channel only OR 3to8 using hardware demux 
  #define SERVO3_CODE        // RC servo controller Timer 3 2 channel only OR 3to8 using hardward demux
                             // note: can use both SERVO1 and SERVO3
							 // problem before 9/18/2009  -- expect macro rename in 
							 // servo1.c and 3.c to solve

  #define MDT_CODE           // Mega Donkey Terminal Code (serial link to PC Mega Donkey Term)
                             // code auto included if MDT_DEMO included
#endif


#define DEMO_CODE       // any of the various demo routines... selected below


//
//  Define the various demo routines to compile and execute
//  unless LIBRARY_BUILD defined
//
#ifndef LIBRARY_BUILD  
#ifdef DEMO_CODE               
   #define MDT_DEMO    // this needed if panel defined and want to 
                       // have MD Term hooked


   #define LOGO_DEMO           // brings in 6KB of floating point code   (code in curves.c)
 
        
//   #define SERVO1_DEMO    // RC servo demo using timer1 (PB6 and PB7 pins) 
//   #define SERVO3_DEMO    // RC servo demo using timer3 (PE4 and PE5 pins)  

   #define LED_PWM_DEMO 
   #define GRAPH_DEMO

//  #define MDT_DEMO    // Mega Donkey Terminal -- serial link to PC Mega Donkey Term application
  #define MAP_DEMO
  #define LCD_DEMO
   //#define UART_DEMO
   #define VCHAR_DEMO
  
   #ifdef USER_INPUT        // all these demos require user interaction
      #define VAREDIT_DEMO
      #define MENU_DEMO
      #define CURVE_DEMO      //!!! brings in 6Kb of floating point code
      #define SKETCH_DEMO
   // #define SERVO_DEMO
   // #define STORED_MENU_DEMO
   // #define ADC_DEMO
   // #define TERMINAL_DEMO
   // #define TRACKBALL_DEMO

   #endif // USER_INPUT

   #ifdef OLED     
      #define HCDEMO
   #endif
   #ifdef NOKIA    
      #define HCDEMO
   #endif
#endif  // DEMO_CODE
#endif // NDEF LIBRARY_BUILD

                        
// some common intermodule dependencies
#ifdef LOGO_DEMO
   #define BEZIER_CODE
#endif
#ifdef CURVE_DEMO
   #define BEZIER_CODE
#endif

#ifdef GRAPH_DEMO
   #define VCHAR_CODE
#endif

#ifdef TERMINAL_CODE
   #define UART_CODE
   #define ADC_CODE
#endif

#ifdef VCHAR_CODE
   #define GRAPH_CODE
#endif

#ifdef LED_PWM_DEMO
   #define SINE256_TABLE
#endif
#ifdef GRAPH_CODE
   #define SINE360_TABLE
#endif
#ifdef DEMO_LCD
   #define SINE360_TABLE
#endif
#ifdef SICKLMS_DEMO
   #define SINE360_TABLE
#endif

#ifdef MOTOR_CODE
   #define TWI_CODE
#endif

#ifdef TIME_CLOCK
   #define TIMER_CODE
#endif

#ifdef TOUCH_SCREEN
   #define ADC_CODE
#endif
#ifdef JOY_SWITCH
   #define ADC_CODE
#endif

#ifdef MOUSE_UART0
   #define UART_CODE
   #define TRACKBALL_CODE
#endif
#ifdef MOUSE_UART1
   #define UART_CODE
   #define TRACKBALL_CODE
#endif
#ifdef MOUSE_UART3
   #define TRACKBALL_CODE
#endif


#ifdef MDT_DEMO
  #define MDT_CODE
#endif

#ifdef SERVO1_DEMO    
  #define SERVO1_CODE
#endif 

#ifdef SERVO3_DEMO    
  #define SERVO3_CODE
#endif 


#endif  //_md_H_
