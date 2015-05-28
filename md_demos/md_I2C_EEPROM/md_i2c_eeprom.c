/*
   Using Mega Donkey to Read from / Write to I2C EEPROM (27C256) 
    
   Ron Grant
   February 2008
   
*/

#include "donkey.h"
#include "Btwi.h"

#define Btwi_disable() TWCR &= ~(1<<TWEN)


int main (void) {

 
   u08 T[64]; // one page of data
   u08 i;
   u16 page;
   u16 addr;

   md_init();  // donkey init
   //timer0_init();

   lcd_clear();

   // disable TWI don't think TWI is being enabled

   Btwi_disable(); 

   sbi(DDRD,1);  
   sbi(DDRD,0);
   sbi(DDRE,5);  // 1=output

 
   Btwi_init(1);  // show errors
   //delay_ms(1000);


 // page = 64 bytes in 256K EEPROM

   if (1==1)
   {
     printf("start write\n");


     // test write entire 32K

     for (page=0;page<512;page++) 
	 {
        Btwi_start_write (0xA0);  // control byte   LSB=0 write );  // begin write sequence 
        // 24C256  32Kx8    512 64 byte pages
        //  15 bit address   _EDC BA98 7654 3210
        //  9 bit page        876 5432 10   
 
        addr = page*64;
        Btwi_write(addr>>8);     // address high 7 bits 
        Btwi_write(addr&0xFF);   // write address low 8 bits 

	    for (i=0;i<64;i++)
          Btwi_write((i+page)&0xFF);   // write data byte   
        Btwi_stop();             // start the page write
        
		delay_ms(7);
     }
     printf ("32K written\n");
     
     // wait for ACK
     // possible automatic method for repeated send of START
   
    // do {
    // Btwi_start_write (0xA0); 
    // delay_ms(1);
    // } while (!(TWSR & 0x18));

  
     printf ("write complete \n");
     printf ("errors %d \n",twErrorCount);
  
   }


   for (page=0;page<512;page++)
   { 

     Btwi_start_write (0xA0);  // EEPROM control byte 0xA0 + (addr<<1) + 0
     addr = page*64;
     Btwi_write(addr>>8);     // address high 7 bits 
     Btwi_write(addr&0xFF);   // write address low 8 bits 
     Btwi_start_read  (0xA0);  // Request Read Byte (note  1 ORed into address )
   
     for (i=0;i<63;i++)
       T[i] = Btwi_read_ack();  // was nack
     T[63] = Btwi_read_nack();
    
     Btwi_stop();


     printf ("errors %d \n",twErrorCount);

	 // verify 
     for (i=0;i<64;i++)
       if (T[i] != ((i+page)&0xFF)) printf("mismatch T[%d]=%d\n",i,T[i]); 

   }

   printf ("end 32K \n EEPROM verify\n");


}

