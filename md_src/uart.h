/*  MegaDonkey Library File:  uart.h   UART support including FIFO buffering, handshaking options - Header File
    


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



   UART.C   Hardware UART Support - UART0 and UART1
            (COM0 COM1)
 
   Basic interface, FIFO queues              Sept 2004, Ron Grant
   Flow control and fancy UART init / menus  14 May  2007, Mark Sims
   Mega256x support                          Sept 2007, Mark Sims
   stdout support for printf()               1 Oct 2007, Ron Grant


   Note there are legacy functions present, e.g. print_int(value,digits).
   These functions are easily duplicated using printf, except that printf has presumably higher
   overhead, hence they remain. 

   
 
   RTS output   Donkey : "Clear to Send to Donkey"     TTL signal High (at jumper header)
   RTS input             



   Note: Early prototype (before MD REV0) has CTS RTS label reversed 
   Rev number printed on component side of board by I2C Connector
   

*/

#ifndef UART_H
#define UART_H


#include "md.h"  
#include <avr/pgmspace.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef UART_CODE

#define UART0       // make code for UART 0 
#define UART1       // make code for UART 1
#define UART_TEST   // make code for UART test routines

#ifdef  UART0
#define UART_CODE
#define TX0MAX 120   // problem with 128?  thinking about 8 bit signed index
#define RX0MAX 120
#define RX0THRESH  (RX0MAX/12)  // threshold where flow control kicks in/out

#define rx_haschar()  Rx0Count     /* if true then can call rx_getchar */
#define rx0_haschar() Rx0Count 

// FIFO Queue for transmitted chars
 volatile u08 Tx0Front;        // transmit queue output index
 volatile u08 Tx0Back;         // transmit queue input index
 volatile u08 Tx0Count;        // #of chars in queue - simplifies logic
 volatile u08 Tx0BusyFirst;
 volatile u08 Tx0Buf[TX0MAX];       

 volatile u08 Rx0Front,     // FIFO Queue for received chars
                    Rx0Back,
                    Rx0Count,
                    Rx0Buf[RX0MAX];

 volatile u08 Com0Error;

 u08 com0_protocol;
 volatile u08 com0_rx_paused;
 volatile u08 com0_tx_paused;

#define enable_rts0()   sbi(DDRG, 1)
#define assert_rts0()   cbi(PORTG, 1)
#define clear_rts0()    sbi(PORTG, 1)
#define enable_cts0()   cbi(DDRG, 0)
#define cts0_asserted() ((PING & 0x01) == 0)    // PG0 Low = OK for Donkey to Send


 int FrameBytes;


//
// Global functions
//
void com_setup(u08 port, u08 force_menu);

unsigned long uart0_init(unsigned long BaudRate, u08 bits, u08 parity, u08 stop, u08 protocol);

void uart0_flush(void);

void uart_send(u08 Data);

#define uart0_send(Data) uart_send((Data))

u08  rx_getchar(void);   
#define rx0_getchar() rx_getchar()

                      
#define rx_peek() RxBuf[RxFront]   /* return front of queue char */

void uart0_test(void);

void putch(char c);
void print_eol(void);
void print(char* pBuf);
void println(char* pBuf);

void print_int(long num, short digits);
void print_intb (long num, short digits);   // print int followed by a blank

void print_hexnibble(u08 nibble);
void print_hexbyte(u08 hexbyte);
void print_hexword(u16 Data);      // send a hex word (2 bytes)


void uart0_set_stdout(void);        // direct standard output from printf
                                    // to serial port 0


// --- telemetry functions  
// see .c file for more info on format which uses 0x80 to indicate start of frame

void send_startframe(u08 FrameType);   // 0 std telemetry  1= sonar  0x80NN
void send_endframe(void);              // sends number of bytes in frame as 2 bytes then x8050

                       // special functions that send bytes, but send two 0x80's for in a row when
                       // one is encountered within byte(s) they are passed 
                       
void send1 (u08 n);    // single byte
void send2 (int n);    // two bytes  low order (1st) high order (2nd)
void send4 (long n);   // four bytes low to high  b1,b2,b3,b4

// Macros
// note: no white space between macro name and params


#define PPL(s) println(PS(s))
#define PPL0(s) println(PS(s)) // alias for PPL

#define PP(s)  print(PS(s))
#define PP0(s) print(PS(s))   // alias for PP


// PSTR is a Macro that stores literal string in Program Space Only
// otherwise, string is copied into RAM at program start using valuable 
// RAM space. 
// The resultant pointer must be operated on by strcpy_P function to copy
// to RAM based string for print routine
#endif   //UART0


#ifdef UART1
#define UART_CODE
#define TX1MAX 120   // problem with 128?  thinking about 8 bit signed index
#define RX1MAX 120
#define RX1THRESH  (RX1MAX/12)   // threshold where flow control kicks in/out

#define rx1_haschar()  Rx1Count  /* if non-zero then can call rx1_getchar */

// FIFO Queue for transmitted chars
 volatile u08 Tx1Front,        // transmit queue output index
                    Tx1Back,         // transmit queue input index
                    Tx1Count,        // #of chars in queue - simplifies logic
                    Tx1BusyFirst, 
                    Tx1Buf[TX1MAX];     

 volatile u08 Rx1Front,        // FIFO Queue for received chars
                    Rx1Back,
                    Rx1Count,
                    Rx1Buf[RX1MAX];

 volatile u08 Com1Error;

 u08 com1_protocol;
 u08 volatile com1_rx_paused;
 u08 volatile com1_tx_paused;

#define enable_rts1()   sbi(DDRD, 5) 
#define assert_rts1()   cbi(PORTD, 5)
#define clear_rts1()    sbi(PORTD, 5)
#define enable_cts1()   cbi(DDRD, 4)
#define cts1_asserted() ((PIND & 0x10) == 0)



//
// Global functions
//
void com_setup(u08 port, u08 force_menu);

unsigned long uart1_init(unsigned long BaudRate, u08 bits, u08 parity, u08 stop, u08 protocol);
void uart1_flush(void);
void uart1_send(u08 Data);
u08 rx1_getchar(void);                        
#define rx1_peek() Rx1Buf[Rx1Front]   /* return front of queue char */
void uart1_test(void);

void putch1(char c);
void print1_eol(void);
void print1(char *pBuf);
void println1(char *pBuf);

void print1_int(long num, short digits);
void print1_intb (long num, short digits);   // print int followed by a blank

void print1_hexnibble (u08 nibble);
void print1_hexbyte(u08 hexbyte);
void print1_hexword(u16 Data);      // send a hex word (2 bytes)

void uart1_set_stdout(void);        // direct standard output from printf
                                    // to serial port 1


// Macros
// note: no white space between macro name and params


#define PPL1(s) println1(PS(s))
#define PP1(s) print1(PS(s))


// PSTR is a Macro that stores literal string in Program Space Only
// otherwise, string is copied into RAM at program start using valuable 
// RAM space. (Valuable on Mega16/32 anyway).
// The resultant pointer must be operated on by strcpy_P function to copy
// to RAM based string for print routine

#endif  // UART1

#endif  // UART_CODE
#endif  // UART_H
