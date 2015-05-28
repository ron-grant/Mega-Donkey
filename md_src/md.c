/* md.c    MegaDonkey Init Code & Main for non-library compilation


   
    MegaDonkey Library File:  md.c 


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




   If LIBRARY_BUILD is left undefined (defined in md.h) this file contains a main function
   and compiles into a project which will contain demos specified in md.h and all supporting 
   code. This large project was used during the development of MegaDonkey and can still be used.
  
   If LIBRARY_BUILD is defined the main function will be left out of the compile and thus a 
   "successful" compile will generate one error "undefined reference to 'main'.
   After the compile, executing MakeDonkeyLib.bat will copy all object files into a library file
   (archive .a file).

 
  

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

  
*/
						   // AVR Suport part of GCC AVR libraries	
#include <avr/pgmspace.h>  // store strings and arrays in flash program memory space versus load into RAM at runtime
#include <avr/interrupt.h> // interrupt support
#include <avr/eeprom.h>    // EEPROM support
#include <util/crc16.h>    // CRC generator
#include <stdio.h>         // printf support

                             
#include "md.h"           // global/misc stuff & Master Control for Building Demo Application / Library Object Code 
#include "uart.h"         // UART support
#include "timer.h"        // System Timer (timer 0) & low-level support for other timers
#include "lcd.h"          // LCD hardware/graphics/bitmapped text support
#include "adc.h"          // adc sampler and touch screen support 
#include "twi.h"          // Two Wire Interface (I2C)
#include "led.h"          // LED support

#include "demolcd.h"      // LCD graphics demos (also has uart demo)

#include "menu.h"         // menu support (light GUI) 
#include "kbd.h"          // keyboard support
#include "calc.h"         // calculator support


#include "vchar.h"        // vector character drawing
#include "graph.h"        // cartesian / polar graph utils / 2D graphics / clipping
                          // also map demo
#include "servo1.h"       // hobby RC servo support JitterFree Timer1 fixed 2 Channel PB6 & PB7
#include "servo3.h"       // hobby RC servo support JitterFree Timer3 fixed 2 Channel PE4 & PE5

#include "varedit.h"      // variable editor
#include "curves.h"       // Bezier curves
#include "trakball.h"     // mouse/trackball
#include "md_term.h"      // Remote Mega Donkey Terminal (MD Term) with LCD
                          // emulation capability (MD Term is Part of DonkeyProg Windows Application)


// flags to control behavior of check_flash()
#define DELAY_FLAG 0x01   // pause 1 sec after checking flash checksum
#define HALT_FLAG  0x02   // halt cpu if flash checksum is bad



u08 time_not_set;



void hw_init(void)
{
   u08 t = md_InhibitPortPullUp;  // see md.h

#ifdef _AVR_IOM2561_H_     
    PRR0 = 0x00;           // turn off power reduction registers on Mega2561
    PRR1 = 0x00;
#endif

    /* setup all ports as pulled up inputs */

    DDRA  = 0x00;   // PortA
    if ((t&1)==0) PORTA = 0xFF;

    DDRB  = 0x00;   // PortB
    if ((t&2)==0) PORTB = 0xFF;

    DDRC  = 0x00;   // PortC
    if ((t&4)==0) PORTC = 0xFF;

    DDRD  = 0x00;   // PortD
    if ((t&8)==0) PORTD = 0xFF;

    DDRE  = 0x00;   // PortE
    if ((t&0x10)==0) PORTE = 0xFF;

    DDRF  = 0x00;   // PortF ADC Port all Input
    if ((t&0x20)==0) PORTF = 0xFF;

    DDRG  = 0x00;   // PortG
    if ((t&0x40)==0) PORTG = 0xFF;

    DDRA = 0x03;    //  PA0 PA1 output
    DDRB |= 0x10;   //  (1<<4);  // PB4 output  Speaker
    DDRC |= 0x04;   //   

    // Watchdog Enabled, Prescaler: OSC/16k
    //  wdt_enable(WDTO_15MS);


    led_set(0, LED_OFF); // turn off all 4 leds

#ifdef TIMER_CODE
    time_not_set = 1;
    timer0_init();
#endif

#ifdef UART0
    uart0_init(9600L, 8, 'N', 1, 0);
    #ifdef MDT_CODE
       if(MDT_COM_PORT == 0) uart0_init(MDT_BAUD_RATE, 8, 'N', 1, 0);
    #endif 
#endif

#ifdef UART1
    uart1_init(9600L, 8, 'N', 1, 0);
    #ifdef MDT_CODE
       if(MDT_COM_PORT == 1) uart1_init(MDT_BAUD_RATE, 8, 'N', 1, 0);
    #endif 
#endif

#ifdef TWI_CODE
//    twi_init();
#endif

#ifdef ADC_CODE
    ACSR = 0x80;       // Analog Comparator Disabled
    adc_init(8);       // start up continuous cycle of ADC capture, channels 0..7
#endif                 // including touch screen digitize
}                      // now relies on timer0 call at 10 KHz

#ifndef LED_CODE
void led_set(u08 led_num, u08 val)
{
   // dummy LED routine if LED hardware not enabled
}
#endif


void lcd_backlight(u08 val)
{
#ifdef LED_CODE    // control backlight via LED interface
   led_set(5, val);

   //!!! treating LED1 as a power light.  
   //!!! if backlight is off, make it bright 
   //!!! if backlight is on, make it dim 
   if(val == LED_OFF) led_set(1, LED_ON);
   else               led_set(1, LED_DIM); 
#else   // direct backlight control
   if(val == 0) { sbi(DDRC, 2); cbi(PORTC, 2); }
   else         { sbi(DDRC, 2); sbi(PORTC, 2); } 
#endif
}


void md_init_no_logo(void)
{
u08 force_setup;

    cli();    // disable interrupts
    hw_init();

#ifdef MOUSE_UART3
sbi(PORTD, 7); cbi(DDRD, 7);   //!!!! mouse uart
sbi(PORTD, 6); sbi(DDRD, 6);   //!!!! mouse uart
#endif

    sei();    // enable interrupts - the Donkey is off and running

    #ifdef TIME_CLOCK
       if(1) {   //!!! for time clock maximum wrap test
          time.hours= 23;
          time.mins = 59;
          time.secs = 00;
          time.month = 12;
          time.day  = 31;
          time.year = 2007;
          time.weekday = 6;
          time.adjust = 0;
       } 
    #endif

#ifdef PANEL_CODE
    lcd_init();          // initialize LCD hardware
    lcd_set_stdout();    // make printf() use bitmapped chars
#endif

#ifdef VCHAR_CODE
//  vchar_init();        // initialize vector characters
//  vchar_set_stdout();  // make printf use vector chars
#endif

    lcd_backlight(0xFF);

    force_setup = 0;
    user_input_init();

#ifdef CKSUM_CODE
    check_flash(DELAY_FLAG);   // verify FLASH checksum - can also add HALT_FLAG
#endif

#ifdef CKPT_CODE
    check_point(0);   // see if last run produced a system RAM checkpoint dump to eeprom
#endif

void contrast_demo(void);
//contrast_demo();

//zadc_demo(2);
//sketch_a_etch();

//void gps_demo(void);
//gps_demo();

//fill_screen(BLACK);
//arc(80,40, 20, 30,170, 10);
//arc(80,40, 10, 40,160, 1);
//wait_until_touched();
//wait_while_touched();


#ifdef USER_INPUT
    delay_ms(10);  // allow some time to pass
	               // might be needed for first get_touch?

    if(get_touch(1) || MouseRB || MouseLB) {   // if screen touched on powerup,  force setup menus
       lcd_clear();
       force_setup = 1;
       set_charsize(3);
       lcd_textPS(0, 0, "Setup");
       set_charsize(1);
//     wait_while_touched();        // not a good idea.  touch cal could be off
       delay_ms(2000);
    }

    calibrate_touch(force_setup);   // calibrate touch screen if not already done 

    misc_setup(force_setup);        // setup backlight, etc, if not already done

    #ifdef UART0   // do com0 setup menu, if not already done or forced setup
       com_setup(0, force_setup);     
    #endif

    #ifdef UART1   // do com1 setup menu, if not already done or forced setup
       com_setup(1, force_setup);
    #endif
#endif   // no user input device: set default com port parameters


// if library build then do donkey splash screen for 2.5 seconds

#ifdef LIBRARY_BUILD
  lcd_clear();
#endif

#ifdef MENU_CODE
 MenuBeep = 1; 
#endif


}


void md_init(void)
{
  
  md_init_no_logo();

  #ifdef LIBRARY_BUILD
    lcd_setxy(CHAR_WIDTH*4,ROWS-CHAR_HEIGHT);
    printf (PS(LIBRARY_VERSION));

    draw_donkey_logo();  // - bezier curves

    delay_ms(1500);
    lcd_clear();
  #endif

}




//
//
// System setup menu
//
//
void misc_setup(u08 force_menu)
{
#ifdef MENU_CODE
u08 bl;
u08 sound;
u08 invert;
u08 clock;

   // load backlight value in EEPROM
   bl = eeprom_read_byte((uint8_t *) EE_BACKLIGHT);

   // validate the sound value in EEPROM
   sound = eeprom_read_byte((uint8_t *) EE_SOUND);
   if(sound == 'N') sound = 1;
   else if(sound == 'F') sound = 0;
   else {
      force_menu = 1;
      sound = 1;
   }

   // validate the time value in EEPROM
   clock = eeprom_read_byte((uint8_t *) EE_SET_TIME);
   if(clock == 'N') clock = 1;
   else if(clock == 'F') clock = 0;
   else {
      force_menu = 1;
      clock = 0;
   }

   // validate the button color value in EEPROM
   invert = eeprom_read_byte((uint8_t *) EE_INVERT_BUT);
   if((invert != 0x00) && (invert != 0xFF)) {
      force_menu = 1;
      invert = 0xFF;
   }


   time_not_set = 1;
   if(force_menu == 0) goto use_params;

   MENU_INIT

   do {
      MENU_CONTROLS


      if((COLS < 160) || (ROWS < 80)) {
         menu_label    (0,0,   PS("Misc Setup")); 
         menu_checkbox (1*CHAR_WIDTH-4,  (CHAR_HEIGHT+CHAR_HEIGHT/2)*1, PS("Backlight"),  bl, 1);
         menu_checkbox (1*CHAR_WIDTH-4,  (CHAR_HEIGHT+CHAR_HEIGHT/2)*2, PS("Beeper"),  sound, 2);
         menu_checkbox (1*CHAR_WIDTH-4,  (CHAR_HEIGHT+CHAR_HEIGHT/2)*3, PS("Invert buttons"), invert, 3);
         #ifdef TIME_CLOCK
            menu_checkbox (1*CHAR_WIDTH-4,  (CHAR_HEIGHT+CHAR_HEIGHT/2)*4, PS("Set clock"), clock, 4);
         #endif
         menu_exitbutton();
      }
      else {
         menu_label    (0,0,   PS("Misc Setup Menu")); 
         menu_checkbox (4*CHAR_WIDTH,  CHAR_HEIGHT*2, PS("Backlight"),  bl, 1);
         menu_checkbox (4*CHAR_WIDTH,  CHAR_HEIGHT*4, PS("Beeper"),  sound, 2);
         menu_checkbox (4*CHAR_WIDTH,  CHAR_HEIGHT*6, PS("Invert buttons"), invert, 3);
         #ifdef TIME_CLOCK
            menu_checkbox (4*CHAR_WIDTH,  CHAR_HEIGHT*8, PS("Set clock"), clock, 4);
         #endif
         menu_exitbutton();
      }

      MENU_COMMANDS

      // menu button/control responses

      switch(menu_cmd()) {
         case 1: 
            break;

         case 2: 
            break;

         case 3: 
            break;

         case 4: 
            #ifdef TIME_CLOCK
               if(clock) {
                  menu_call(set_time);
                  time_not_set = 0;
               }
            #endif
            break;
      }
   } while(menu_cmd() != MENU_EXITCODE) ;  // repeat until exit command

   wait_while_touched();

   if(bl) bl = 0xFF;  //!!!  
   eeprom_write_byte((uint8_t *) EE_BACKLIGHT,  bl);

   if(sound)  eeprom_write_byte((uint8_t *) EE_SOUND,  (uint8_t) 'N');
   else       eeprom_write_byte((uint8_t *) EE_SOUND,  (uint8_t) 'F');

   if(invert)  eeprom_write_byte((uint8_t *) EE_INVERT_BUT,  (uint8_t) 0xFF);
   else        eeprom_write_byte((uint8_t *) EE_INVERT_BUT,  (uint8_t) 0x00);

   if(clock)   eeprom_write_byte((uint8_t *) EE_SET_TIME,  (uint8_t) 'N');
   else        eeprom_write_byte((uint8_t *) EE_SET_TIME,  (uint8_t) 'F');

   

   use_params:
   lcd_backlight(bl);

   #ifdef TIME_CLOCK
      if(eeprom_read_byte((uint8_t *) EE_SET_TIME) == 'N') {
         if(time_not_set) set_time();
      }
   #endif

   if(sound) beep_disabled = 0;
   else      beep_disabled = 1;

   if(invert) invert_buttons = WHITE;
   else       invert_buttons = 0x00;
#endif
}


#ifdef CKPT_CODE
//
//
// System RAM checkpoint code
//
// If flag==0x00 read checkpoint flag from EEPROM and bring up 
// checkpoint menu if a checkpoint dump exists.  From the checkpoint menu
// you can dump the checkpoint data to COM0,9600,n,8,1
//
// If flag==0x01..0x7F - write RAM to EEPROM and flag as user resetable
// from the checkpint menu.  Note that a standard checkpoint dump does wipe
// out anything stored in EEPROM!
//
// If flag==0x80..0xFF - write RAM to EEPROM and make it non-resettable.
// The only way to clear the checkpoint and use the system again is to
// reprogram the FLASH memory.
//
#define CKPT_FIRST 0x0000

struct CHECK_POINT {
   char a,b,c,d;
   u08  flag1;
   u08  flag2;
} ckpt = {
   'C', 'K', 'P', 'T',
    0, (~0)
} ;

u08 check_point(u08 flag)
{
u16 addr;
char s[COLS/CHAR_WIDTH+1];

   if(flag) {  // write checkpoint to EEPROM
      if((flag & 0x80) == 0x00) {
         lcd_clear();
         lcd_setxy(0,0);
         printf(PS("DUMPING RAM: %02X"), flag);
      }
      ckpt.a = 'C';
      ckpt.b = 'K';
      ckpt.c = 'P';
      ckpt.d = 'T';
      ckpt.flag1 = flag;
      ckpt.flag2 = (~flag);
      eeprom_write_block((void *)CKPT_FIRST, (void *) CKPT_FIRST, (size_t) (EE_CHECKPOINT-CKPT_FIRST));
      eeprom_write_block((void *) &ckpt, (void *) EE_CHECKPOINT, (size_t) sizeof ckpt);
   }

   // see if checkpoint dump has been saved in EEPROM
   eeprom_read_block((void *) &ckpt, (void *) EE_CHECKPOINT, (size_t) sizeof ckpt);

   if(ckpt.a != 'C') return 1;
   if(ckpt.b != 'K') return 1;
   if(ckpt.c != 'P') return 1;
   if(ckpt.d != 'T') return 1;
   if((ckpt.flag1 ^ ckpt.flag2) != 0xFF) return 2;

   MENU_INIT

   do {
      MENU_CONTROLS

      if(COLS >= 160) {
         sprintf(s, PS("CheckPoint Dump: %02X"), ckpt.flag1);
      }
      else {
         sprintf(s, PS("CKPT dump: %02X"), ckpt.flag1);
      }
      menu_label(0,0, s);

      menu_button(0,CHAR_HEIGHT*3, PS("DUMP to COM0"), 1); 

      if((ckpt.flag1 & 0x80) == 0x00) {  // non-fatal checkpoints can be cleared
         menu_button(0,CHAR_HEIGHT*5, PS("CLEAR CHECKPOINT"), 2); 
         menu_button(COLS-(CHAR_WIDTH*4*BUTTON_SIZE)-(ButtonBorder*2)-1, ROWS-14, PS("EXIT"), 99);
      }


      MENU_COMMANDS

      // menu button/control responses pt

      switch(menu_cmd()) {
         case 1: 
            uart0_init(9600L, 8, 'N', 1, 0x00);
            for(addr=CKPT_FIRST; addr<0x1000; addr++) {
               if((addr%16) == 0) {
                  println("");
                  sprintf(s, PS("%04X:"), addr);
                  print(s);
               }
               sprintf(s, PS(" %02X"), eeprom_read_byte((uint8_t *) addr)); 
               print(s);
            }
            println("");
            break;

         case 2: 
            ckpt.a = ckpt.b = ckpt.c = ckpt.d = ckpt.flag1 = ckpt.flag2 = 0;
            eeprom_write_block((void *) &ckpt, (void *) EE_CHECKPOINT, sizeof ckpt);
            break;
      }

   } while (menu_cmd() != 99);       // repeat until exit command

   return 3;
}
#endif // CKPT_CODE


#ifdef CKSUM_CODE
//
//   Flash memory checksum code.
//
//   Calculates a checksum of all of flash memory and saves it into EEPROM
//   the first time the routine is called (and EEPROM is 0xFFFFFFFF).
//
//   Later calls do the memory checksum and compare it to that saved in
//   EEPROM.  
//
//
u08 check_flash(u08 flag)
{
unsigned flash_sum;
unsigned ee_sum;
unsigned long addr;

   flash_sum = 0xFFFF;
#ifdef _AVR_IOM2561_H_
   for(addr=0; addr<(65536L*4); addr++) {
      flash_sum = _crc16_update(flash_sum, pgm_read_byte_far(addr));
   }
#else
   for(addr=0; addr<(65536L*2); addr++) {
      flash_sum = _crc16_update(flash_sum, pgm_read_byte_far(addr));
   }
#endif
   if(flash_sum == 0xFFFF) flash_sum = 0x1234;

   ee_sum = read_word(EEPROM_ADDR | EE_FLASH_SUM);

   printf(PS("CHECKSUM is:%04X\n"), flash_sum);
   printf(PS("CHECKSUM sb:%04X\n"), ee_sum);

   if(ee_sum == 0xFFFFL) { // sum not saved... write it to eeprom
      write_word(EEPROM_ADDR | EE_FLASH_SUM, flash_sum);
      printf(PS("\nCHECKSUM SAVED\n"));
      if(flag & DELAY_FLAG) delay_ms(1000);    
   }
   else if(ee_sum == flash_sum) { 
      printf(PS("\nCHECKSUM OK\n"));
      if(flag & DELAY_FLAG) delay_ms(1000);    
   }
   else {
      printf(PS("\nCHECKSUM ERROR\n"));
      if(flag & HALT_FLAG) while(1);
      wait_until_touched();
      wait_while_touched();
      delay_ms(2000);  // delay here to give time to retouch screen to enter setup mode
   }

   return 0;
}

#endif // CKSUM_CODE


#ifdef DEMO_CODE

void do_demos(void)
{
#ifdef LOGO_DEMO
   lcd_clear();
   draw_donkey_logo();  // donkey logo - bezier curves

#endif

#ifdef MDT_DEMO
  mdt_demo();          // serial interfave to PC Mega Domnkey Term 
                       // supports graphics,font control, colors...
#endif


#ifdef VCHAR_DEMO
    vchar_demo();       // vector character demo
#endif


#ifdef SERVO1_DEMO
   servo1_demo();       // RC Servo Demo using Timer1 output on PB6 and PB7
#endif


#ifdef SERVO3_DEMO
   servo3_demo();       // RC Servo Demo using Timer1 output on PB6 and PB7
#endif



#ifdef SVS_DEMO
   svs_demo();          // servo sequencer demos
#endif


#ifdef GRAPH_DEMO
#ifdef LED_PWM_DEMO
    led_pwm_demo(1);    // run pulsating LED demo while 2D vector graphics demo runs
    graph_demo();       // 2D vector graphics demo
    led_pwm_demo(0);    // stop pulsating LEDs
#else
    graph_demo();       // 2D vector graphics demo
#endif
#endif


#ifdef MAP_DEMO
   map_demo();          // moving map 2D graphics demo
#endif


#ifdef CURVE_DEMO
   bezier_edit_demo();  // bezier curves
#endif


#ifdef STORED_MENU_DEMO
   stored_menu_demo();  // menus stored in memory
#endif


#ifdef VAREDIT_DEMO
    varedit_demo();    // variable editor
#endif


#ifdef ADC_DEMO       
   adc_demo(0);         // dump ADC channels
#endif


#ifdef MENU_DEMO
    menu_demo();       // menu demo: buttons checkboxes sliders
#endif


#ifdef SKETCH_DEMO    
    sketch_a_etch();   // touchpad demo
	delay_100ths(100);        // !! for MD_TERM
#endif


#ifdef TRACKBALL_DEMO
   trackball_demo();   // trackball demo
#endif


#ifdef TERMINAL_DEMO
   lcd_term(0x01);     // serial graphics terminal
#endif


#ifdef SPI_DEMO
   spi_demo();         // serial peripheral interface demo
#endif


#ifdef TWI_DEMO
   twi_demo();         // two wire (IIC) peripheral interface
#endif

#ifdef MOTOR_DEMO
#endif



#ifdef LCD_DEMO
   demo_loop(1);    // loop forever doing various drawing routines and uart demo
// demo_loop(0);    // do various drawing routines and uart demo once
#else
   #ifdef UART_DEMO
      demo_loop(1);    // loop forever doing various drawing routines and uart demo
   // demo_loop(0);    // do various drawing routines and uart demo once
   #endif
#endif

}
#endif // DEMO_CODE


void gps_demo(void)
{
   uart0_init(4800L, 8, 'N', 1, 0);
   uart1_init(4800L, 8, 'N', 1, 0);

   printf("GPS DEMO\n\n");
   delay_ms(1000);

   println( "$PASHS,NME,GSA,A,ON");
   println1("$PASHS,NME,GSA,A,ON");
   delay_ms(1000);
   println(" $PASHS,NME,SAT,A,ON");
   println1("$PASHS,NME,SAT,A,ON");
   delay_ms(1000);

   while(1) {
      if(rx1_haschar()) {
         printf("%c", rx1_getchar());
      }
      if(rx_haschar()) {
         printf("%c", rx_getchar());
      }
      wait_while_touched();
   }
}


#ifndef LIBRARY_BUILD
int main(void)
{
  md_init();

  #ifdef DEMO_CODE
    do_demos();
  #endif

  #ifdef TERMINAL_CODE
    while(1) {
       lcd_term(0x01);
    }
  #endif

  while(1); // should not reach this 
}
#else // if building library code 
  // no main for library build -- will generate one error, 
  //int main(void) { while(1);}   
#endif


