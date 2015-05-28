#ifndef _BTWI_H_
#define _BTWI_H_

#include "md.h"

//#ifdef TWI_CODE

u08 twErrorCount;

void Btwi_init(u08 show_errors);
void Btwi_stop(void);


void Btwi_start_write (u08 address);  // begin write sequence 
void Btwi_start_read  (u08 address);  // addr+1 taken care of internally

void Btwi_write_ack (u08 data);       // write a byte of multi byte sequence 
void Btwi_write (u08 data);           // write a byte 

   
//u08  Btwi_read (char ack);            // read a byte set ack for mid stream  not at end

u08  Btwi_read_ack (void);            // read a byte of a multi byte read 
u08  Btwi_read_nack (void);           // read a final byte of multi byte read & disconnect



//u08 twi_rw (char cmd, u08 address, u08 data);

//void twi_write_reg (u08 address, u08 reg, u08 data);
//void Btwi_demo(void);

//#endif // TWI_CODE

#endif // _TWI_H
