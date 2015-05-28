/* twi.c  Megadonkey Two Wire Serial Interface Support (I2C)


   Polled transmission, not interrupt driven

*/

#include "donkey.h"

#ifdef TWI_CODE

#include "Btwi.h"


/*
 TWI Two Wire Interface is compatible with I2C

 Pull-Up Resistors on 2-Wire bus should be selected based on frequency
 


 See Chip Doc for complete explanation:
 Some reminder notes here:

 Master issues START,  commands, (including possible repeated START) STOP


 START and repeated START : SDA pulled low while SCL high
 STOP SDA released while SCL high

 otherwise SDA transitions expected while SCL low.


 Address Packets are 9 bits total   =  7 address + R/W + ack
 Slave pulls SDA low in ack bit frame

 If Master does not see slave ack (e.g. times out waiting for the ack), then
 Master can transmit a STOP or repeated START
 
 Address packet consisting of slave address and READ or WRITE is called SLA+R or 
 SLA+W, respectively.

 Most sig byte of address byte sent first.
 Address 00 is reserved for general call to all slaves.
 (see doc)

All data packets are 9 bits long.  8 data + ack
Receiver pulls SDA low during ack
Leaving SDA is considered a NACK

Combining Address and Data Packets

  START
  SLA+R/W
  one or more data packets
  STOP


Slave can extend SCL low period by holding SCL low (for as long as it wants)
SCL high time controlled by master.

Multi-Master Protocol (see chip doc)


TWDR  Data and Address Shift Register
NACK/ACK register (not directly accessible) for NACK/ACK to be transmitted/received
When Receiving it can be set cleared by bit in TWCR (TWI Control Register)

If TWI has initiated transmission as Master, Aribtration Detection  hardware continuously
monitors the transmission trying to determine if Arbitration is in progress.

Address Match Unit (used for Slave Mode)
checks received address bytes against TWAR (even if MCU asleep)

Conrol Unit Monitors Bus and generates responses based on TWCR settings (Control Register)
TWINT (Interrupt flag set)
When TWINT set, SCL held low, status register set (TWSR) with various status flags

• After the TWI has transmitted a START/REPEATED START condition.
• After the TWI has transmitted SLA+R/W.
• After the TWI has transmitted an address byte.
• After the TWI has lost arbitration.
• After the TWI has been addressed by own slave address or general call.
• After the TWI has received a data byte.
• After a STOP or REPEATED START has been received while still addressed as a Slave.
• When a bus error has occurred due to an illegal START or STOP condition.



 TWI Control Register (TWCR)
 writing a 1 to TWINT starts TWI Hardware, and causes the bit to be cleared until
 the operation completes (then the bit is set again)
 (Note: this is a little counterintuitive)
 
 TWEN turns on TWI hardware and takes over I/O pins (SDA&SCL) (always 1 for TWI ops)


*/ 


u08 twPrintError;
u08 twError;

#define twTIMEOUT 1

#define P(s) {if (twPrintError) printf(PS(s));} 
#define IP(id,s) {if (twPrintError) printf("%d ",id); printf(PS(s));}

#define START  0x08
#define STOP   0x01
#define MT_SLA_ACK 0x18 
#define MT_DATA_ACK 0x28
#define MT_DATA_NACK 0x30    



// future interrupt support
//SIGNAL (TWI_vect)




void Btwi_init(u08 show_errors)
{
  
   twPrintError = show_errors;
   twError = 0;
   twErrorCount = 0; 

   PRR0 &= ~(1<<PRTWI);    // set TWI power reduction register0 bit to 0 to enable TWI
                           // just in case set

    // SCL frequency  TWBR:  
        // 10  444 kHz
        // 20  222 kHz
    // 40  166 kHz
        // 100  74 kHz

        
        TWBR = 10;              // cpu_clk / (16+2 TWBR *4^TWPS)  
        TWSR = 0x0;             // TWPS in D1,D0   0..3   4^TWPS = 1,4,16,64    

                         
        TWCR = (1<<TWEN);       // TWI enable  take over PD0 and PD1 (mega128/2561)
                                // using polled (d0 = TWIE)
}


u08 twi_wait(u08 id, u08 code) 
{
    u16 time = 0;

    while (!(TWCR & (1<<TWINT)))
	{
	  delay_us(10);
	  if (time++ > 10000)
	  { twError = twTIMEOUT;    // 100ms 
	    if (twPrintError) P("TWI Timeout\n");
		twErrorCount++;
        return(0);
	  }
    }

    if ((TWSR & 0xF8) != code)
	{
       switch (code) {
       case START      : IP(id,"TWI Start not sent\n");    break;
	   case STOP       : IP(id,"TWI Stop not sent\n");     break;
       case MT_SLA_ACK : IP(id,"No Slave Ack\n");          break;
       case MT_DATA_ACK: IP(id,"No Data Ack\n");           break;
	   case MT_DATA_NACK:IP(id,"No Data NACK received\n"); break;
	   }
	} 
    else
	{
       return(1); 
	}

	// follows switch stmt
	twErrorCount++;
    return(0);
}
  

void twi_cmd(u08 par)
{
   // par=0 would set TWIE

   if (par) TWCR = (1<<TWINT)|(1<<TWEN)|(1<<par);
   else     TWCR = (1<<TWINT)|(1<<TWEN);

}



void Btwi_stop (void)
{  twi_cmd (TWSTO);     // Transmit STOP
   
}


void Btwi_start_write (u08 address)
{
  twi_cmd (TWSTA);                            // Send START condition
  twi_wait(0,START);
  TWDR = address;                             // slave address  SLA_W    
  twi_cmd(0);                                 // start transmit of address
  twi_wait(1,MT_SLA_ACK);                         
}



void Btwi_start_read (u08 address)
{
 twi_cmd(TWSTA);                 // Send Start
 twi_wait(2,START);
 TWDR = (address|1);             // slave address  SLA_W 
 twi_cmd(0);                     // start transmit of address
 twi_wait(3,MT_SLA_ACK);
}


u08 Btwi_read (char ack)
{
  u08 rdata;    
        
  // TWDR = 0; // don't care should be?
    
  if (ack) twi_cmd(TWEA);
  else     twi_cmd(0);

  if (ack) twi_wait(4,MT_DATA_ACK);            // wait for ACK/NACK to be received
  else     twi_wait(5,MT_DATA_NACK);
 
  rdata = TWDR; 
  
  return(rdata);
}


u08  Btwi_read_ack (void)            // read a byte of a multi byte read 
{ return (Btwi_read(1)); }

u08  Btwi_read_nack (void)           // read a final byte of multi or single byte read
{ return (Btwi_read(0)); }



void Btwi_write (u08 data)
{
  TWDR = data;
  twi_cmd(0);               // send data
  twi_wait (6,MT_DATA_ACK);
}


void Btwi_write_ack (u08 data)  // MAYBE NOT NEEDED == RG  EXPERIMENTAL ADD TWEa
{
  TWDR = data;
  twi_cmd(TWEA);            // send data with ACK enabled
   twi_wait (7,MT_DATA_ACK);
}


#endif // TWI_CODE
