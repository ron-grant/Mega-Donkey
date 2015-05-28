/*  MegaDonkey Library File:  twi.c  TwoWire Interface
    


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




    At this point using polled transmission, not interrupt driven

*/

#include <avr/io.h>
#include "md.h"

#ifdef TWI_CODE
#include "timer.h"
#include "twi.h"

/*

 TWI Control Register (TWCR)
 setting TWINT=1 starts TWI Hardware
 I am pretty sure it then is set to zero by Hardware then returns to 1 when
 operation complete
 
 TWEN turns on TWI hardware and takes over IO pins (always 1 for TWI ops)






*/ 


void twi_init(void)
{

    // SCL frequency  TWBR:  
        // 10  444 kHz
        // 20  222 kHz
    // 40  166 kHz
        // 100  74 kHz

        
        TWBR = 20;              // cpu_clk / (16+2 TWBR *4^TWPS)  
        TWSR = 0x0;             // TWPS in D1,D0   0..3   4^TWPS = 1,4,16,64    
        TWCR = 0x4;             //TWEN = twi enable take over PC0,PC1
                                // bit0 = int enable

}

#define START 0x08

unsigned int timecount;


void timeout_clr(void)
{ timecount = 0; }

char timeout (void)
{  if (++ timecount > 30000)
   {
         if (TWIErrors<255) TWIErrors++;
     return (1);
   }  
   else return(0);
}


void twi_stop (void)
{  TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);      // Transmit STOP
}


void twi_start_write (u08 address)
{
 timeout_clr(); 
        
 TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);       //Send START condition
  while (!(TWCR & (1<<TWINT)) && !timeout());  // wait for start to be sent

  //if ((TWSR & 0xF8) != START)
  //{  } // error 
  
  TWDR = address; // slave address  SLA_W;    // put address byte in data register
  TWCR = (1<<TWINT) | (1<<TWEN);              // start transmit of address
  timeout_clr();
  while (!(TWCR & (1<<TWINT)) && !timeout());   // wait for TWINT indicates address sent and ACK/NACK
                                                // received

  //  if ((TWSR & 0xF8) != MT_SLA_ACK)   // no error det for now
  //  {  } // error
  
  //println ("write mode");
  //print_hexbyte(address);
  //print_eol();
  
}


void twi_start_read (u08 address)
{
 TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);  //Send START condition
 timeout_clr();
 while (!(TWCR & (1<<TWINT)) && !timeout());  // wait for start to be sent

  //if ((TWSR & 0xF8) != START)
  //{  } // error 
  
 TWDR = (address+1);   // slave address  SLA_W; // address?;
 TWCR = (1<<TWINT) | (1<<TWEN);  // start transmit of address
 timeout_clr();
 while (!(TWCR & (1<<TWINT)) && !timeout());   // wait for TWINT indicates address sent and ACK/NACK

  //  if ((TWSR & 0xF8) != MT_SLA_ACK)   // no error det for now
  //  {  } // error
  
 // println ("read mode");
 // print_hexbyte(address);
 // print_eol();
  
}


u08 twi_read (char ack)
{
  u08 rdata;    
        
  // TWDR = 0; // don't care should be?
    
  if (ack)
  TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);    // enable ack -- RG
  else          
  TWCR = (1<<TWINT) | (1<<TWEN);  // send junk data
                                  // writing 1 to TWINT = Clear Flag

  timeout_clr(); 
  while (!(TWCR & (1<<TWINT)) && !timeout());   // Wait for Data Send and ACK/NACK received
                                      // (TWINT bit set in TWCR)
 
  rdata = TWDR; 
 
    
  // if ((TWSR & 0xF8) != MT_DATA_ACK)  // Check TWI Status Reg for ACK  
  // {  } // error    no error det for now      
  
  return(rdata);
}


u08  twi_read_ack (void)            // read a byte of a----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- multi byte read 
{ return (twi_read(1)); }

u08  twi_read_nack (void)           // read a final byte of multi or single byte read
{ return (twi_read(0)); }





void twi_write (u08 data)
{
  TWDR = data;
  TWCR = (1<<TWINT) | (1<<TWEN);  // send data
  timeout_clr();
  while (!(TWCR & (1<<TWINT)) && !timeout());   // Wait for Data Send and ACK/NACK received
  // if ((TWSR & 0xF8) != MT_DATA_ACK)  // Check TWI Status Reg for ACK  
  // {  } // error    no error det for now      
}


void twi_write_ack (u08 data)  // MAYBE BS NOT NEEDED == RG  EXPERIMENTAL ADD TWEa
{
  TWDR = data;
  TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);  // send data with ACK enabled
  
  timeout_clr();
  while (!(TWCR & (1<<TWINT)) && !timeout());   // Wait for Data Send and ACK/NACK received

  // if ((TWSR & 0xF8) != MT_DATA_ACK)  // Check TWI Status Reg for ACK  
  // {  } // error    no error det for now      
}


#ifdef TWI_DEMO
void twi_demo(void)
{
}
#endif // TWI_DEMO

#endif // TWI_CODE
