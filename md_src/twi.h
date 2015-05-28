/*  MegaDonkey Library File:  twi.h  TwoWire Interface - Header File
    


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


    
    Ron Grant 2006
    At this point using polled transmission, not interrupt driven
    Minimal code at this point -- tested with Deventech MD-22 H-Bridge and also their 
	sonar modules..


*/

#ifndef _TWI_H_
#define _TWI_H_

#include "md.h"

#ifdef TWI_CODE
void twi_init(void);
void twi_stop(void);


void twi_start_write (u08 address);  // begin write sequence 
void twi_start_read  (u08 address);  // addr+1 taken care of internally

void twi_write_ack (u08 data);       // write a byte of multi byte sequence 
void twi_write (u08 data);           // write a byte 

   
u08  twi_read (char ack);            // read a byte set ack for mid stream  not at end

u08  twi_read_ack (void);            // read a byte of a multi byte read 
u08  twi_read_nack (void);           // read a final byte of multi byte read & disconnect

u08  TWIErrors;  // 0..255 saturate at 255


//u08 twi_rw (char cmd, u08 address, u08 data);

//void twi_write_reg (u08 address, u08 reg, u08 data);
void twi_demo(void);

#endif // TWI_CODE

#endif // _TWI_H
