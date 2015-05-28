/*  MegaDonkey Library File:  uart.c   UART support including FIFO buffering, handshaking options 
    


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
   


*/


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h> // for PSTR macro   Program Memory Space String
#include <ctype.h>
#include <stdio.h>
#include "md.h"

#ifdef UART_CODE
#include "lcd.h"
#include "timer.h"
#include "adc.h"
#include "menu.h"
#include "uart.h"
#include "md_term.h"
#include "calc.h"       // direct # input


#ifdef UART0

#define COM0_TIMEOUT  if(com0_protocol & TIMEOUT_MASK) if(get_msecs_alive() > ticker) { Com0Error |= TX_TIMEOUT; return; }
#define force0_out(c)  { if(Tx0Count) force0_char = c; else uart0_send(c); }
u08 force0_char;


unsigned long uart0_init(unsigned long BaudRate, u08 bits, u08 parity, u08 stop, u08 protocol)
{
u08 regb, regc;
long actual_rate;

    if(BaudRate == 0) {  // stop UART 
       UCSR0B = 0x00;
       uart0_flush();
       return BaudRate;
    }



    BaudRate = (((CPU_CLOCK/8) + (BaudRate/2)) / BaudRate);
    if(BaudRate >= 4096L) {
       BaudRate /= 2;
       if(BaudRate == 0) BaudRate = 1;
       actual_rate = ((CPU_CLOCK/16) + (BaudRate/2)) / BaudRate; 
       UCSR0A = 0x00;
    }
    else {
       if(BaudRate == 0) BaudRate = 1;
       actual_rate = ((CPU_CLOCK/8) + (BaudRate/2)) / BaudRate; 
       UCSR0A = 0x02;
    }
    --BaudRate;


    UBRR0L = (BaudRate & 0xFF);
    UBRR0H = ((BaudRate >> 8) & 0x0F);
    
    regb = 0xD8;  // RXint TXint  RXenable  TXenable
    regc = 0x00;  // ASYNC, TXENABLE

    parity = toupper(parity);
    if(parity == 'E') regc |= 0x20;
    else if(parity == 'O') regc |= 0x30;

    if(stop == 2) regc |= 0x08;

    if(bits == 6) regc |= 0x02;
    else if(bits == 7) regc |= 0x04;
    else if(bits >= 8) regc |= 0x06;
    if(bits == 9) regb |= 0x04;  // !!! FIFO code does not support 9 bit chars

    UCSR0C = regc;   // d8 on mega32 ?
    UCSR0B = regb;   // 0xd8;   // default 8 bit char     is 86 on mega32

    com0_protocol = protocol;
    if(bits == 9) com0_protocol |= NINE_BITS;

    uart0_flush();

    if(com0_rx_paused == 1) force0_out(XON);
    com0_rx_paused = 0;
    return actual_rate;
}


void uart0_flush(void)
{
u08 sreg;

   sreg = SREG;
   cli();  // disable interrupts

   Tx0Front = 0;
   Tx0Back  = 0;
   Tx0Count = 0;
   Tx0BusyFirst = 0;            
   force0_char = 0;
    
   Rx0Front = 0;
   Rx0Back  = 0;
   Rx0Count = 0;
   Com0Error = 0;

   com0_rx_paused = com0_tx_paused = 0;
   if(com0_protocol & USE_RTS) {     // we can control the other guys output
      if(com0_protocol & USE_XON) {  // via software
         com0_rx_paused = 1;
      }
      else {                         // via hardware
         enable_rts0();
         assert_rts0();
      }
   }

   if(com0_protocol & USE_CTS) {  // the other guy can control our output
      if(com0_protocol & USE_XON) com0_tx_paused = 0;  // via software
      else enable_cts0();                              // via hardware
   }

   SREG = sreg;
}


// Interrupt Handler 
// character has been received by uart
// put in FIFO

ISR(SIG_USART0_RECV)
{
u08 c;

    if(com0_protocol & USE_RTS) {  // we are throttling the sender
       if(Rx0Count >= (RX0MAX - RX0THRESH)) {  // queue is almost full
          if(com0_protocol & USE_XON) {  // XON/XOFF software flow control
             if(0 || (com0_rx_paused == 0)) {   // tell the guy to shut up
                com0_rx_paused = 1;
                force0_out(XOFF);
             }
          }
          else { // hardware flow control
             clear_rts0();   // the other guy is no longer ClearToSend
          }
       }
    }

    Com0Error |= (UCSR0A & (PARITY_ERROR | FRAME_ERROR | OVERRUN_ERROR));
    c = UDR0;     // get char from uart

    if(com0_protocol & USE_XON) {     // software flow control is in use
       if(com0_protocol & USE_CTS) {  // the recipient is controlling us
          if(c == XON)  { com0_tx_paused = 0; return; }
          if(c == XOFF) { com0_tx_paused = 1; return; }
       }
    }

    if(Rx0Count >= RX0MAX) Com0Error |= RX_OVERFLOW;
    else {
       Rx0Count++;                // total # of chars in queue
       Rx0Buf[Rx0Back] = c;     // store character in queue
       if(++Rx0Back == RX0MAX) Rx0Back = 0;   // advance pointer and wrap around if needed        
    }
}


u08 rx_getchar(void)  // remove char from queue (check rx_haschar() first)       
{  
u08 c;
u08 sreg;

    if(Rx0Count) {
       sreg = SREG;
       cli();  // disable interrupts
       Rx0Count--;   // total # chars in queue - 1                

       c = Rx0Buf[Rx0Front];              // return front of queue char
       if(++Rx0Front == RX0MAX) Rx0Front = 0;  // advance pointer index wrap around  
       SREG = sreg;
    }
    else c = 0;


    if(com0_protocol & USE_RTS) {        // we are throttling the other guy's sends  
       if(Rx0Count <= RX0THRESH) {       // our buffer is about empty                
          if(com0_protocol & USE_XON) {  // we control him via software              
             if(com0_rx_paused == 1) {   // he has been paused                       
                force0_out(XON);         // it is now OK for him to start sending    
                com0_rx_paused = 0; 
             }
          }
          else {              // we are using hardware flow control
             assert_rts0();   // signal him it is OK to start sending again
          }
       }
    }

    return c;  
}


// transmit complete interrupt  

ISR(SIG_USART0_TRANS)
{
//    if(com0_protocol & USE_CTS) {  // only service uart if Clear to Send
//       if(cts0_asserted() == 0) return;  
//    }

   Tx0BusyFirst = 0;
       
   if(force0_char) {      // flow control char goes out next
      UDR0 = force0_char;
      force0_char = 0;
   }
   else if(Tx0Count > 0) {        // queue not empty
      Tx0Count--;                 // remove a char from queue
      UDR0 = Tx0Buf[Tx0Front];    // and feed to UART
      if(++Tx0Front == TX0MAX) Tx0Front = 0; // advance index and wrap around 
   }
}



void uart_send(u08 Data)
{   
u08 sreg;
unsigned long ticker;
unsigned long tocker;

    if(Com0Error & TX_TIMEOUT) return; 

    if(com0_protocol & TIMEOUT_MASK) {
       ticker = (com0_protocol & TIMEOUT_MASK) * 1024L;
       tocker = get_msecs_alive();
       if((ticker + tocker) < ticker) reset_time_alive();
       else ticker += tocker;
    }
    else ticker = 0;

    if(com0_protocol & USE_CTS) {  // wait until Clear to Send
       if(com0_protocol & USE_XON) {
          while(com0_tx_paused) COM0_TIMEOUT;
       }
       else {
          while(cts0_asserted() == 0) COM0_TIMEOUT;  
       }
    }

    while(Tx0Count > (TX0MAX-2)) COM0_TIMEOUT;  // wait for full queue to have an empty spot

//  disable queue 
//  while (Tx0Count>0) COM0_TIMEOUT;   // wait for empty
    
    while(Tx0BusyFirst) COM0_TIMEOUT;  // wait for not busy
    
    sreg = SREG;
    cli();  // disable interrupts

    if((Tx0Count > 0) || Tx0BusyFirst) {    // if queue not empty or xmit busy     
       queue_data:
       Tx0Count++;                         // put data in queue 
       Tx0Buf[Tx0Back] = Data;
       if(++Tx0Back == TX0MAX) Tx0Back = 0;    // wraparound
    }           
    else { 
       Tx0BusyFirst = 1;  // set flag cleared on completion of first char
       if(force0_char) {  // flow control char gets to go first
          UDR0 = force0_char;
          force0_char = 0;
          goto queue_data;  // and queue up the data char
       }
       else {
          UDR0 = Data;    // put directly to uart if empty queue
       }
    }   

    SREG = sreg;
}

//
//
//  Higher level UART0 I/O routines
//
//

void putch(char c)
{
//   while(Tx0Count) ;     // wait for transmit buffer to empty   !!! timeout
   uart0_send((u08) c);   
}


void print_eol(void)
{
   uart0_send(0x0D);
   uart0_send(0x0A);
}


void print(char* pBuf)  // print a null terminated string
{
   while(*pBuf != 0) {  
      uart0_send(*pBuf); 
      pBuf++;
   }
}


void println (char* pBuf)
{
   print(pBuf);
   print_eol();
}


void print_int(long num, short digits)
{
u08 digit;
int i;
long m;

   if(num < 0) { 
      uart0_send('-');
      num = -num;
    }            
    
    m=1;
    for(i=1; i<digits; i++) m=m*10;
                
    for(i=0; i<digits; i++) {
       digit = 0;
       while(num >= m) { 
          num -= m;
          digit++;   
       }
        
       uart0_send('0'+digit);
       m = m / 10;
    }   
}

void print_intb(long num, short digits) // print int followed by a blank
{ 
   print_int(num,digits);
   putch(' ');
}  


void print_hexnibble(u08 nibble)
{
u08 c; 

    c = nibble & 0x0f;
                              
    if(c > 9) c += ('A'-10);
    else      c += '0';
    
    uart0_send((char) c );
}

void print_hexbyte(u08 hexbyte)
{ 
   print_hexnibble(hexbyte >> 4);
   print_hexnibble(hexbyte);
}

void print_hexword(u16 Data) // send a hex word (2 bytes)
{
    print_hexbyte(Data>>8);
    print_hexbyte(Data & 0xFF);
}

#ifdef TELEM_UART
//
// FRAME80 is frame type that begins with 80 NN  where NN = is frame type 
// the frame data is sent where any 0x80 in the frame will be followed with another 0x80
// or else this next byte should be interpreted as the start of a new frame
// The final two bytes in the frame are the total number of bytes in the frame as a 16 bit number.
// The frame terminates with hex 0x80 0x50.
// multi-byte words are sent lowest order byte first.
//
// The functions that realize telemetry sending are  
// send_startframe (n)
// send2 (int)
// send4 (long)
// send_endframe()
//
// note: if 0x80s are in data then the telemetry block will be lengthened
// number of occurances expected to be rare like 1% of data bytes might be 0x80
 

#define FRAME80  

void send_startframe(u08 FrameType)
{ 
//  #ifdef FRAME80      
    uart0_send(0x80);
    uart0_send(FrameType);        
//  #else
//  uart0_send (0xFF);
//  uart0_send (0xFF);
//  #endif // FRAME80
    FrameBytes = 0;
}


void send_endframe(void)    // end of frame
{
#ifdef FRAME80       
   send2(FrameBytes);          // don't count end of frame count bytes
   uart0_send(0x80);
   uart0_send(0x50);
#endif  //FRAME80
}   


void uart_send80(short n)   // special send function that inserts duplicate 0x80 in stream 
{                           // if one is encountered
   uart0_send(n);                                          
   FrameBytes++;
  
#ifdef FRAME80 
  if(n == 0x80) {
     uart0_send(0x80);      // repeat 0x80 if occurs except for start frame
//   FrameBytes++;         // do not count repeated 0x80s 
  }  
#endif // FRAME80
}

void send1(u08 n)
{ 
   uart_send80(n); 
}
   
void send2(int n)
{ 
   uart_send80(n & 0xFF);
   uart_send80((n >> 8) & 0xFF);
}

void send4(long n)
{ 
int i;

  for(i=0; i<4; i++) { 
     uart_send80(n & 0xFF);
     n = (n >> 8);
  } 
}
#endif // TELEM_UART
        



// support of stdout using stdio.h 
// allows use of printf() after calling port0_set_stdout()

static int uart0_putchar(char c, FILE *stream); // fwd declare


// macro FDEV_SETUP_STREAM provided by stdio.h
// put,get,rwflag

static FILE uart0_stdout = FDEV_SETUP_STREAM(uart0_putchar, NULL,_FDEV_SETUP_WRITE);


void uart0_set_stdout(void)
{
   stdout = &uart0_stdout;   
}


static int uart0_putchar(char c, FILE *stream)
{

  switch (c) {
  case '\a': c = 7; break;
  case '\b': c = 8; break; 
  case '\f': c= 12; break;  // form feed
  case '\n': putch(13); c= 10; break;  // newline (CRLF)
  case '\r': c= 13; break;
  case '\t': c=' '; break;  // need tab handler  -- col counter? advance to next tab pos?
  case '\v': c=10;  break;
  }
 

  putch(c);

  return 1;
}

#endif  // UART0



#ifdef UART1

#define COM1_TIMEOUT  if(com1_protocol & TIMEOUT_MASK) if(get_msecs_alive() > ticker) { Com1Error |= TX_TIMEOUT; return; }

#define force1_out(c)  { if(Tx1Count) force1_char = c; else uart1_send(c); }
u08 force1_char;



unsigned long uart1_init(unsigned long BaudRate, u08 bits, u08 parity, u08 stop, u08 protocol)
{
u08 regb, regc;
unsigned long actual_rate;
    
    if(BaudRate == 0) {  // stop UART 
       UCSR1B = 0x00;
       uart1_flush();
       return BaudRate;
    }

    BaudRate = (((CPU_CLOCK/8) + (BaudRate/2)) / BaudRate);
    if(BaudRate >= 4096L) {
       BaudRate /= 2;
       if(BaudRate == 0) BaudRate = 1;
       actual_rate = ((CPU_CLOCK/16) + (BaudRate/2)) / BaudRate; 
       UCSR1A = 0x00;
    }
    else {
       if(BaudRate == 0) BaudRate = 1;
       actual_rate = ((CPU_CLOCK/8) + (BaudRate/2)) / BaudRate; 
       UCSR1A = 0x02;
    }
    --BaudRate;


    UBRR1L = (BaudRate & 0xFF);
    UBRR1H = ((BaudRate >> 8) & 0x0F);
    
    regb = 0xD8;  // RXint TXint  RXenable  TXenable
    regc = 0x00;  // ASYNC, TXENABLE

    parity = toupper(parity);
    if(parity == 'E') regc |= 0x20;
    else if(parity == 'O') regc |= 0x30;

    if(stop == 2) regc |= 0x08;

    if(bits == 6) regc |= 0x02;
    else if(bits == 7) regc |= 0x04;
    else if(bits >= 8) regc |= 0x06;
    if(bits == 9) regb |= 0x04;  //!!! FIFO code does not support 9 bit chars

    UCSR1C = regc;   // d8 on mega32 ?
    UCSR1B = regb;   // 0xd8;   // default 8 bit char     is 86 on mega32

    com1_protocol = protocol;
    if(bits == 9) com1_protocol |= NINE_BITS;

    uart1_flush();

    if(com1_rx_paused == 1) force1_out(XON);
    com1_rx_paused = 0;
    return actual_rate;
}


void uart1_flush(void)
{
u08 sreg;

    sreg = SREG;
    cli();  // disable interrupts
    Tx1Front = 0;
    Tx1Back  = 0;
    Tx1Count = 0;
    Tx1BusyFirst = 0;           
    force1_char = 0;
    
    Rx1Front = 0;
    Rx1Back  = 0;
    Rx1Count = 0;
    Com1Error = 0;

    com1_rx_paused = com1_tx_paused = 0;
    if(com1_protocol & USE_RTS) {      // we can control the other guys output
       if(com1_protocol & USE_XON) {   // via software
          com1_rx_paused = 1;
       }
       else {                          // via hardware
          enable_rts1();
          assert_rts1();
       }
    }

    if(com1_protocol & USE_CTS) {  // the other guy can control our output
       if(com1_protocol & USE_XON) com1_tx_paused = 0;  // via software
       else enable_cts1();                              // via hardware
    }

    SREG = sreg;
}



// Interrupt Handler 
// character has been received by uart
// put in FIFO

ISR(SIG_USART1_RECV)
{
u08 c;

    if(com1_protocol & USE_RTS) {  // we are throttling the sender
       if(Rx1Count >= (RX1MAX - RX1THRESH)) {  // queue is almost full
          if(com1_protocol & USE_XON) {  // XON/XOFF software flow control
             if(0 || (com1_rx_paused == 0)) {   // tell the guy to shut up
                com1_rx_paused = 1;
                force1_out(XOFF);
             }
          }
          else { // hardware flow control
             clear_rts1();   // the other guy is no longer ClearToSend
          }
       }
    }

    Com1Error |= (UCSR1A & (PARITY_ERROR | FRAME_ERROR | OVERRUN_ERROR));
    c = UDR1;     // get char from uart

    if(com1_protocol & USE_XON) {     // software flow control is in use
       if(com1_protocol & USE_CTS) {  // the recipent is controlling us
          if(c == XON)  { com1_tx_paused = 0; return; }
          if(c == XOFF) { com1_tx_paused = 1; return; }
       }
    }

    if(Rx1Count >= RX1MAX) Com1Error |= RX_OVERFLOW;
    else {
       Rx1Count++;            // total # of chars in queue
       Rx1Buf[Rx1Back] = c;     // store character in queue
       if(++Rx1Back == RX1MAX) Rx1Back = 0;    // advance pointer and wrap around if needed     
    }
}


u08 rx1_getchar(void)    // remove char from queue (check rx1_haschar() first)       
{  
u08 c;
u08 sreg;

   if(Rx1Count) {
      sreg = SREG;
      cli();  // disable interrupts
      Rx1Count--;             // total # chars in queue - 1                
      c = Rx1Buf[Rx1Front];   // return front of queue char
      if(++Rx1Front == RX1MAX) Rx1Front = 0;  // advance pointer index wrap around  
      SREG = sreg;
   }
   else c = 0;

   if(com1_protocol & USE_RTS) {       // we are throttling the other guy's sends
      if(Rx1Count <= RX1THRESH) {      // our buffer is about empty
         if(com1_protocol & USE_XON) { // we control him via software
            if(com1_rx_paused == 1) {  // he has been paused
               force1_out(XON);        // it is now OK for him to start sending
               com1_rx_paused = 0;
            }
         }
         else {             // we are using hardware flow control         
            assert_rts1();  // signal him it is OK to start sending again 
         }
      }
   }

   return (c);  
}


// transmit complete interrupt  


ISR(SIG_USART1_TRANS)
{
//    if(com1_protocol & USE_CTS) {  // only service uart if Clear to Send
//       if(cts1_asserted() == 0) return;  
//    }

   Tx1BusyFirst = 0;

   if(force1_char) {    // flow control char goes out next 
      UDR1 = force1_char;
      force1_char = 0;
   }
   else if(Tx1Count > 0) {   //queue not empty
      Tx1Count--;            // remove a char from queue
      UDR1 = Tx1Buf[Tx1Front];                // and feed to UART
      if(++Tx1Front == TX1MAX) Tx1Front = 0;  // advance index and wrap around 
   }
}



void uart1_send(u08 Data)
{   
u08 sreg;
unsigned long ticker;
unsigned long tocker;

    if(Com1Error & TX_TIMEOUT) return; 

    if(com1_protocol & TIMEOUT_MASK) {
       ticker = (com1_protocol & TIMEOUT_MASK) * 1024L;
       tocker = get_msecs_alive();
       if((ticker + tocker) < ticker) reset_time_alive();
       else ticker += tocker;
    }
    else ticker = 0;

    if(com1_protocol & USE_CTS) {  // wait until Clear to Send
       if(com1_protocol & USE_XON) {
          while(com1_tx_paused) COM1_TIMEOUT;
       }
       else {
          while(cts1_asserted() == 0) COM1_TIMEOUT;  
       }
    }

    while(Tx1Count > (TX1MAX-2)) COM1_TIMEOUT;  // wait for full queue to have am empty spot
    
    while(Tx1BusyFirst) COM1_TIMEOUT;  // wait for not busy
    
    sreg = SREG;
    cli();  // disable interrupts

    if((Tx1Count > 0) || Tx1BusyFirst) {   // if queue not empty or xmit busy     
       queue_data:
       Tx1Count++;                         // put data in queue 
       Tx1Buf[Tx1Back] = Data;
       if(++Tx1Back==TX1MAX) Tx1Back = 0; // wraparound
    }           
    else { 
       Tx1BusyFirst = 1;  // set flag cleared on completion of first char
       if(force1_char) {     // flow control char goes out first
          UDR1 = force1_char;
          force1_char = 0;
          goto queue_data;   // queue up data char
       }
       else {
          UDR1 = Data;       // put directly to uart if empty queue
       }
    }   

    SREG = sreg;
}


//
//
//   UART1 higher level I/O routines
//
//

void putch1(char c)
{
//    while(Tx1Count) ;     // wait for transmit buffer to empty    !!! timeout
    uart1_send((u08) c);  
}


void print1_eol(void)
{
  uart1_send(0x0D);
  uart1_send(0x0A);
}


void print1(char* pBuf)  // print a null terminated string
{
   while (*pBuf != 0) {
      uart1_send(*pBuf); 
      pBuf++;
   }
}


void println1(char* pBuf)
{
  print1(pBuf);
  print1_eol();
}


void print1_int(long num, short digits)
{
u08 digit;
int i;
long m;

   if(num < 0) { 
      uart1_send('-');
      num = 0-num;
   }            
    
   m=1;
   for(i=1; i<digits; i++) m=m*10;
                
   for(i=0; i<digits; i++) {
      digit = 0;
      while(num >= m) {
         num -= m;
         digit ++;   
      }
        
      uart1_send('0'+digit);
      m = m / 10;
   }   
}


void print1_hexnibble(u08 nibble)
{
u08 c;

    c = (nibble & 0x0f);
                              
    if(c > 9) c += ('A'-10);
    else      c += '0';
    
    uart1_send((char) c );
}

void print1_hexbyte(u08 hexbyte)
{ 
    print1_hexnibble(hexbyte >> 4);
    print1_hexnibble(hexbyte);
}

void print1_hexword(u16 Data) // send a hex word (2 bytes)
{
    print1_hexbyte(Data>>8);
    print1_hexbyte(Data & 0xFF);
}




// support of stdout using stdio.h 
// allows use of printf() after calling uart1_set_stdout()


static int uart1_putchar(char c, FILE *stream); // fwd declare


// macro FDEV_SETUP_STREAM provided by stdio.h
// put,get,rwflag

static FILE uart1_stdout = FDEV_SETUP_STREAM(uart1_putchar, NULL,_FDEV_SETUP_WRITE);


void uart1_set_stdout(void)
{
   stdout = &uart1_stdout;   
}


static int uart1_putchar(char c, FILE *stream)
{

  switch (c) {
  case '\a': c = 7; break;
  case '\b': c = 8; break; 
  case '\f': c= 12; break;  // form feed
  case '\n': putch1(13); c= 10; break;  // newline (CRLF)
  case '\r': c= 13; break;
  case '\t': c=' '; break;  // need tab handler  -- col counter? advance to next tab pos?
  case '\v': c=10;  break;
  }
  

  putch1(c);

  return 1;
}


#endif  // UART1



struct COM_SETUP {
   long baud_rate;
   u08  data_bits;
   u08  stop_bits;
   u08  parity;
   u08  protocol;
};

void com_setup(u08 port, u08 force_menu)
{
#define KBD_BUF_LEN 30
char kbd_buf[KBD_BUF_LEN+1];
unsigned long baud_rate;
u08 data_bits;
u08 stop_bits;
u08 odd_flag, even_flag;
u08 cts_flag;
u08 rts_flag;
u08 xon_flag;
u08 tx_time;
struct COM_SETUP com_params;

   set_color(WHITE);
   set_bg(BLACK);

   baud_rate = 9600;
   #ifdef MDT_CODE
      if(port == MDT_COM_PORT) baud_rate = MDT_BAUD_RATE;
   #endif
   data_bits = 8;
   odd_flag = even_flag = 0;
   stop_bits = 1;
   rts_flag = cts_flag = xon_flag = 0;
   tx_time = 0;

   if(port == 0)  {
      #ifdef MOUSE_UART0   // force mouse parameters
         baud_rate = 1200;
         data_bits = 7;
         goto use_params;
      #else    // load com port values from EEPROM
         eeprom_read_block((void *) &com_params, (void *) EE_COM0, (size_t) sizeof com_params);
      #endif
   }
   else if(port == 1)  {
      #ifdef MOUSE_UART1   // force mouse parameters
         baud_rate = 1200;
         data_bits = 7;
         goto use_params;
      #else    // load com port values from EEPROM
         eeprom_read_block((void *) &com_params, (void *) EE_COM1, (size_t) sizeof com_params);
      #endif
   }
   else return;

   // validate the com port values
   if((com_params.parity == 'N') || (com_params.parity == 'E') || (com_params.parity == 'O')) {
      baud_rate = com_params.baud_rate;
      data_bits = com_params.data_bits;
      stop_bits = com_params.stop_bits;
      if(stop_bits != 2) stop_bits = 1;
      rts_flag  = com_params.protocol & USE_RTS;
      cts_flag  = com_params.protocol & USE_CTS;
      xon_flag  = com_params.protocol & USE_XON;
      tx_time   = com_params.protocol & TIMEOUT_MASK;
      odd_flag = even_flag = 0;
      if(com_params.parity == 'E') even_flag = 1;
      else if(com_params.parity == 'O') odd_flag = 1;
   }
   else {  // no or bad eeprom data,  start with default com params
      force_menu = 1;
   }

   if(force_menu == 0) goto use_params;

   MENU_INIT


   do {
      MENU_CONTROLS


      if((COLS < 160) || (ROWS < 80)) {
         ButtonBorder = 1;
         sprintf(kbd_buf, PS("COM%d Setup"), port);
         menu_label(0,0,   kbd_buf);  // labels outside of loop

         sprintf(kbd_buf, PS("Baud: %6ld"), baud_rate);
         menu_button   (0,9+12*0, kbd_buf, 1);

         sprintf(kbd_buf, PS("DATA:%d"), data_bits);
         menu_button   (0,9+12*1, kbd_buf, 9);
         sprintf(kbd_buf, PS("STOP:%d"), stop_bits);
         menu_button   (COLS/2-CHAR_WIDTH,9+12*1, kbd_buf, 10);

         menu_label     (0*CHAR_WIDTH+0, 9+12*2, PS("Par:"));
         menu_checkbox  (5*CHAR_WIDTH+4, 9+12*2, PS("ODD"),  odd_flag, 2);
         menu_checkbox (10*CHAR_WIDTH+4, 9+12*2, PS("EVEN"), even_flag, 3);

         if(xon_flag) {
            menu_button    (0*CHAR_WIDTH,   8+12*3, PS("XON"), 7);
            menu_checkbox  (5*CHAR_WIDTH+4, 9+12*3, PS("XMT"), cts_flag, 5);
            menu_checkbox (10*CHAR_WIDTH+4, 9+12*3, PS("RCV"), rts_flag, 6);
         }
         else {
            menu_button    (0*CHAR_WIDTH,   8+12*3, PS("FLOW"), 7);
            menu_checkbox  (5*CHAR_WIDTH+4, 9+12*3, PS("CTS"), cts_flag, 5);
            menu_checkbox (10*CHAR_WIDTH+4, 9+12*3, PS("RTS"), rts_flag, 6);
         }

         menu_slider(0,9+12*4-1, 32,       // position x,y  control width
                     PS("TIMEOUT"), tx_time,  // optional label, int var (any size)
                     0,15,1,8);     // range min,max , show value, response code

         menu_exitbutton();
      }
      else {
         sprintf(kbd_buf, PS("COM%d Setup Menu"), port);
         menu_label(0,0,   kbd_buf);  // labels outside of loop

         sprintf(kbd_buf, PS("Baud rate: %6ld"), baud_rate);
         menu_button   (0,10+14*0, kbd_buf, 1);

         sprintf(kbd_buf, PS("DATA: %d"), data_bits);
         menu_button   (0,10+14*1, kbd_buf, 9);
         sprintf(kbd_buf, PS("STOP: %d"), stop_bits);
         menu_button   (9*CHAR_WIDTH,10+14*1, kbd_buf, 10);

         menu_label    (0+0*CHAR_WIDTH, 12+14*2, PS("Parity: "));
         menu_checkbox (9*CHAR_WIDTH,   12+14*2, PS("ODD"),  odd_flag, 2);
         menu_checkbox (14*CHAR_WIDTH,  12+14*2, PS("EVEN"), even_flag, 3);

         if(xon_flag) {
            menu_button   (0*CHAR_WIDTH,   9+14*3,  PS("XON/XOFF"), 7);
            menu_checkbox (9*CHAR_WIDTH,  12+14*3,  PS("RCV"), rts_flag, 6);
            menu_checkbox (14*CHAR_WIDTH, 12+14*3,  PS("XMIT"), cts_flag, 5);
         }
         else {
            menu_button   (0*CHAR_WIDTH,   9+14*3, PS("FLOW CTL"), 7);
            menu_checkbox (9*CHAR_WIDTH,  12+14*3, PS("CTS"), cts_flag, 5);
            menu_checkbox (14*CHAR_WIDTH, 12+14*3, PS("RTS"), rts_flag, 6);
         }

         menu_slider(0,12+14*4, 32,          // position x,y  control width
                     PS("TIMEOUT"), tx_time, // optional label, int var (any size)
                     0,15,1,8);   // range min,max , show value, response code

         menu_exitbutton();
      }



      MENU_COMMANDS

      // menu button/control responses

      switch(menu_cmd()) {
         case 1: 
            baud_rate = menu_calc(PS("Baud rate?"), 0) / calc_scale;
            if((baud_rate < 300) || (baud_rate > 500000)) {
               baud_rate = 9600;
               #ifdef MDT_CODE
                  if(port == MDT_COM_PORT) baud_rate = MDT_BAUD_RATE;
               #endif
            }
            break;  

         case 2: //odd parity
            if(even_flag) {
               even_flag = 0; 
               menu_init();
            }
            break;

         case 3: //even parity
            if(odd_flag) {
               odd_flag = 0; 
               menu_init();
            }
            break;

         case 5: // cts
            break;

         case 6: // rts
            break;

         case 7: // xon
            if(xon_flag) xon_flag = 0;
            else xon_flag = 1;
            menu_init();
            break;

         case 8: // tx timeout slider
            break;

         case 9: // data bits
            data_bits = menu_calc(PS("DATA Bits?"), NO_MATH|FIX_MODE|NO_DP_CHANGE|NO_BASE_CHANGE) / calc_scale;
            if((data_bits < 5) || (data_bits > 9)) data_bits = 8;
            break;

         case 10: // stop bits
            if(stop_bits == 1) stop_bits = 2;
            else stop_bits = 1;
            menu_init();
            break;
      }

   } while(menu_cmd() != MENU_EXITCODE);       // repeat until exit command

   wait_while_touched();

   use_params:
   com_params.baud_rate = baud_rate;
   com_params.data_bits = data_bits;
   com_params.stop_bits = stop_bits;
   if(even_flag) com_params.parity = 'E';
   else if(odd_flag) com_params.parity = 'O';
   else com_params.parity = 'N';
   com_params.protocol = tx_time;
   if(rts_flag) com_params.protocol |= USE_RTS;
   if(cts_flag) com_params.protocol |= USE_CTS;
   if(xon_flag) com_params.protocol |= USE_XON;

   if(port == 0)  {
      if(force_menu) {
         eeprom_write_block((void *) &com_params, (void *) EE_COM0, (size_t) sizeof com_params);
      }

      uart0_init(
         com_params.baud_rate,
         com_params.data_bits,
         com_params.parity,
         com_params.stop_bits,
         com_params.protocol
      );
   }
   else if(port == 1) {
      if(force_menu) {
         eeprom_write_block((void *) &com_params, (void *) EE_COM1, (size_t) sizeof com_params);
      }

      uart1_init(
         com_params.baud_rate,
         com_params.data_bits,
         com_params.parity,
         com_params.stop_bits,
         com_params.protocol
      );
   }
}

#ifdef UART_TEST
#ifdef UART0
void uart0_test(void)
{
   wait_while_touched();
   lcd_clear();
   while(1) {
      lcd_setxy(0,0);
      printf("err=%02X tp=%02X rp=%02X\n", 
      Com0Error, com0_tx_paused, com0_rx_paused);
      printf("cts=%d\n", cts0_asserted());

      if(get_touch(1)) {
         if(touchX > 140) break;
         while(rx0_haschar()) {
            printf("%c", rx0_getchar());
         }
         wait_while_touched();
      }
   }
   wait_while_touched();
}
#endif  //UART0

#ifdef UART1
void uart1_test(void)
{
   wait_while_touched();
   lcd_clear();
   while(1) {
      lcd_setxy(0,0);
      printf("err=%02X tp=%02X rp=%02X\n", 
      Com1Error, com1_tx_paused, com1_rx_paused);
      printf("cts=%02X\n", cts1_asserted());

      if(get_touch(1)) {
         if(touchX > 140) break;
         while(rx1_haschar()) {
            printf("%c", rx1_getchar());
         }
         wait_while_touched();
      }
   }
   wait_while_touched();
}
#endif  // UART1

#endif  // UART_TEST

#endif  // UART_CODE
