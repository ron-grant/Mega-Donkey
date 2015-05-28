/*  MegaDonkey Library File:  MD_BOOT.c    MegaDonkey Boot Loader
  


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



   Mega Donkey Boot Loader
   Ron Grant October 2007

   Crypto foo, pimpage, and major shrinkage by Mark Sims - November 2007


   The boot loader gives you the ability to program Mega Donkey via a serial 
   link from a host PC running "Mega Donkey Prog" without the need for a 
   specialized programmer.  This feature is possible because the Atmel 
   ATMega series processors can read/write their own Flash program memory.

   The bootloader can be configured to download only encrypted .HEX files.  This
   allows one to ship firmware update files to customers that are just about
   impossible to reverse engineer.  The crypto bootloader can be set up to use
   DES, Triple DES, XTEA, or AES encryption (56, 168, 128, 256 bit keys).

   The boot loader is a program that stays resident in the microcontroller and 
   is never overwritten provided that the device is not subjected to a 
   Erase Device command  which is easily issued if a programmer, such as 
   the STK500, is connected.

   Note that the Erase Device command pays no attention to Lock bits designed 
   to protect device memory. This protection is offered to prevent 
   overwrite under program control.  Also, in some cases to prevent reading 
   from either of the two sections of memory defined as

   1. Application Section
   2. Boot Section



   The boot loader, as written here, is designed to work with the application,  
   DonkeyProg.  Upon reset of the Mega Donkey, the boot loader begins 
   running the code defined in this file (to be located in the Boot Section).

   It sequences the 4 LEDs on Mega Donkey. If, during that time, (about 2 
   seconds) it is contacted by DonkeyProg (via serial port 0 and/or 1), it will 
   accept a new application program image from MDProg then begin execution 
   of it.

   If DonkeyProg does not contact the Mega Donkey, the existing application in 
   the Mega Donkey will execute.  If no application has been downloaded into
   the MegaDonkey,  the bootloader will restart the cycle again.


   If you inadvertently erase the MegaDonkey memory you can reload the 
   bootloader image using a hardware programmer, e.g. STK500.

   MD1 models (Mega 128)     MD_Bootloader128_Vxxx.hex
   MD2 models (Mega 256x)    MD_Bootloader256_Vxxx.hex

   where xxx is the version number


   You must set the fuses on the device to agree with boot loader code.
   The proper boot loader partition size must be selected.  The 
   standard boot loader fits in 2kb (1K words).

   Also, make sure Boot Reset vector Enabled box is checked.

   Donkey               Byte Address  Word Address
   Model    Processor   (linker uses) (STK-500 uses)
   -------  ----------- ------------- ------------
   MD1-XX    Mega128       1F800         0FC00
   MD2-XX    Mega2561      3F800         1FC00

   The AES and DES crypto bootloaders requires 2K words (4k bytes).
   MD1-XX    Mega128       1F000         0F800
   MD2-XX    Mega2561      3F000         1F800

   The XTEA crypto bootloader fits in 2kb if the linker script (md_boot.x) 
   is used to generate the bootloader code .HEX file.  This script deletes 
   the interrupt vectors from the compiled bootloader image.   The XTEA
   bootloader requires 4kB if the interrupt vectors are left in.  

   The AES bootloader will (just barely) fit in 2Kb.

   The DES bootloaders all require 4kB.

   Download time comparison (80K file,  200MHz PC):
       Plain  7.3 seconds
       DES   12.3 seconds
       AES   13.0 seconds
       XTEA  13.3 seconds
       3DES  22.5 seconds


   If you really, really need it #define TINY_BOOT  and link with the 
   md_boot.x linker script.  This will make a stripped down bootloader 
   (serial port 0 only)  that (hopefully, barely) fits in 1Kb.  
   Make sure the md_crypt.h file is not included.

   To protect the boot loader from modification by any applications you
   write/load, make sure that boot loader protection mode 2 or 3 is
   checked on the LockBits page. This does not protect from a chip erase
   issued from an ISP programmer or JTAG interface.


   !!!!! ----------------------- IMPORTANT ---------------------- !!!!!
   !!!!! LockBit settings required for cryptographic boot loaders !!!!!
   !!!!! -------------------------------------------------------- !!!!!

   Set memory protect mode 3:  No further programmming or verification.
   This prevents reading out or modifiying the application code section via
   the ISP or JTAG ports.  Without this anybody can access your unencrypted
   application code and you might as well not bother encrypting your 
   application...

   Set Application Section protect mode 1:  No lockout of LPM or SPM instruction
   access to the application section.  This allows the bootloader to rewrite
   the application code section so it can update it...  the whole purpose of
   a bootloader.

   Set Boot Section protect mode 3:  Disallow LPM or SPM instructions access
   to the bootloader section.  This prevents applications from wiping out the
   bootloader or accessing the bootloader code and encryption keys.  Without
   this you might as well not bother encrypting your application.

   You must program the boot loader size, boot vector enable, and all other
   configuration fuses BEFORE you program the lock bits.  Once the memory is
   locked down,  you cannot modify these fuses without doing a full chip
   erase.

   If #define CRYPTO_LOCKDOWN is specified,  the bootloader code enforces
   the boot lock bit settings.  If they are not set properly (value should
   be 0xCC),  the bootloader will lock up flashing all the LEDs.  Also the 
   bootloader will flash the LEDs after each time a program is downloaded
   until the board is reset.



   IMPORTANT

   If you modify the boot loader then you need to make sure your project
   generates code with the byte address specified above.  If you modify
   the code or use a different compiler/options also verify the
   bootloader size and use the smallest boot block size that it will fit in.
   This frees up the most FLASH space for applications.  You need to look 
   in the .LSS listing file and check the size of the .text and .data sections.
   The sum of the sizes of these two sections is the total bootloader size.



   Using WINAVR IDE to add/edit the linker command line option

   using Project|ConfigurationOptions
   click [CustomOptions]
   then select [Linker Options] then add -Ttext=3F800
   then make sure you click [Add] before pressing [OK] to close dialog
   also add -Tmd_boot.x (to specify the special linker script file)

   If the option is present and you want to change the address, you will need 
   to press [Edit], make the changes then [Add] back before pressing [OK].

   Before or after programming the device. You must make sure the correct
   boot section size fuse is set and that the Boot reset vector is checked.


--------------------------------------------------------------------------------------
  Some Reference Notes (From Atmel Device Manual)

  Mega 256x  Flash and EEPROM Organization

    128K Words (256K bytes) Flash,
    128 words per page PCWORD [6:0]
    1024 pages PCPAGE[16,7] (PCMSB=16)

    4K bytes EEPROM
    8 bytes per page EEA[2:0]
    512 pages PCPAGE EEA[11:3] (EEAMSB=11)


    Boot loader can modify all memory including itself


    Flash divided into 2 sections
       Application and Boot Section, each with set of protection fuses

       Application section cannot modify boot loader (SPM instruction 
       disabled in that addr space)


   RWW Read While Write  and NRWW No Read While Write (partial details):
      When erasing or writing a page in RWW section (application section) 
      the NRWW section can be read during operation

      When erasing or writing a page in NRWW section (boot section) the CPU 
      is halted during the entire operation.

      BOOTSZ controls size of boot section (4 combos)

      Application can initiate boot loader prog

      Boot Loader Lock Bits any combo of Protect/Not for Application and 
      Boot sections (See Table 29-2 and 29-3)
      BLB02 BLB01 = 11 = Enable Full Read Write
      BLB12 BLB11 = 11 = Enable Full Read Write from Bootloader


     BOOTRST Boot Reset Fuse  1=Application 0x00000  0= Boot Loader Reset 
     Location near top of memory - location depends on BOOTSZ
     (must be cleared to enable loader which looks for sequence of "PPPP.." 
     on serial port,  if not found then switches to application (jumps 
     to application reset vector 0x0000)


   Low Level Detail
   ZL,ZH used by SPM instruction to address 64K
   RAMPZ register used to select 64K pages

   Page erase and Page write operations addressed independently
   Important that boot loader Page Erase and Page Write operations address same page

*/


//#include "md_crypt.h"       // this header required if using CRYPTO features
//#define TINY_BOOT
//#define QUIET_BOOTLOADER



#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/io.h>

#define BOOT_LOADER_VERSION 100

#define CPU_CLOCK 16000000L
#define TIME_OUT  (CPU_CLOCK / 64L)  // about a 2 second download detect window

#define BAUD_RATE 115200L
#define BAUD_VAL ((((CPU_CLOCK/8)+(BAUD_RATE/2)) / BAUD_RATE)-1)

#define P_COUNT 2       // gotta see this many 'P's on a serial port
                        // to enter bootload mode

#define TRY_PORT0       // define this to enable loading from UART0
#define TRY_PORT1       // define this to enable loading from UART1
                        // define both to enable loading from either port
                        // (allowing both ports adds about 120 bytes)
                        // (port0 only is smaller than port1 by 18 bytes)

#define BOOT_LEDS       // define this to enable status leds in bootloader
                        // (LED code uses about 114 bytes)

#define VERIFY_FLASH    // Define this to enable memory verify code.
                        // (verify code takes around 88 bytes)

#define EEPROM_PRGM  1  // if 0, disable eeprom programming code (64 bytes)

//#define QUIET_BOOTLOADER  // if defined the bootloader does not send
                          // get/send fuse/memory info info (saves around 70 bytes)


#define EE_CHECKPOINT (0x1000-6)  // Checkpoint RAM dump to eeprom - only used to find EE_FLASH_SUM
#define EE_FLASH_SUM  (EE_CHECKPOINT-2)  // where the standard donkey code writes
                                         // the CRC of flash memory in the 
                                         // EEPROM.  If not defined, then the 
                                         // bootloader does not clear the CRC.

//#define BOOT_INHIBIT        // Define this to inhibit bootloader via I/O pin
                             // (takes 8-20 bytes depending upon port). 
                          
#define BOOT_INH_PBN 1      // Boot Inhibit Bit on port B (PINB)
                            // default PB1, Jumper ISP 7-8 to inhibit boot by pulling 
							// PB1 low (set to weak pullup in init code)


#define PROTECT_BOOTLOADER  // If defined,  we automatically set the bootloader 
                            // protection fuses to protect the bootloader 
                            // from being overwritten (or read). 
                            // (takes 24 bytes)
                            // Crypto bootloaders still needs the memory protect
                            // fuses to be written using an external programmer
                            // since they cannot be set via software.


#define CRYPTO_LOCKDOWN     // Define this to disable crypto bootloader if the
                            // lock fuses are not properly set (takes 30 bytes)

//
//  These defines should come from the md_crypt.h header file
//  generated by shhh.exe   They are shown here for reference only.
//
//#define CRYPTO_BOOT  // define this for crypto bootloader

//#define XTEA         // define this for TEA encryption (very small and fast code)
                       // ... otherwise DES / 3DES is used

//#define TRIPLE_DES   // define this for 3DES encryption
                       // ... otherwise single DES is used
                       // 3DES code can decrypt single DES if all three keys are the same

//#define AES          // define this for AES 256 bit encryption


#ifdef TINY_BOOT           // force config to get a 1Kb bootloader
   #undef CRYPTO_BOOT      // first undefine all the standard options
   #undef BOOT_LEDS
   #undef VERIFY_FLASH
   #undef EEPROM_PRGM
   #undef TRY_PORT0
   #undef TRY_PORT1
// #undef BOOT_INHIBIT
// #undef EE_FLASH_SUM
// #undef PROTECT_BOOTLOADER

   #define TRY_PORT0      // these options (just) fit in 1Kb
   #define EEPROM_PRGM 1  
   #define BOOT_LEDS      
   #define VERIFY_FLASH
#endif


#define u08 unsigned char
#define u16 unsigned int
#define sbi(reg,bit) reg |= _BV(bit)
#define cbi(reg,bit) reg &= ~_BV(bit)

//
// for now hard assert RTS, but ignore CTS
//
#define enable_rts0()   sbi(DDRG, 1)
#define assert_rts0()   cbi(PORTG, 1)
#define clear_rts0()    sbi(PORTG, 1)
#define enable_cts0()   cbi(DDRG, 0)
#define cts0_asserted() ((PING & 0x01) == 0)    // PG0 Low = OK for Donkey to Send

#define enable_rts1()   sbi(DDRD, 5)
#define assert_rts1()   cbi(PORTD, 5)
#define clear_rts1()    sbi(PORTD, 5)
#define enable_cts1()   cbi(DDRD, 4)
#define cts1_asserted() ((PIND & 0x10) == 0)

#define crc_init() crc=0xFFFF

#define get_byte_with_crc() get_byte(1)
#define get_byte_no_crc()   get_byte(0)
#define get_word_with_crc() get_word(1)
#define get_word_no_crc()   get_word(0)


u08 boot_port;                  // the com port we are booting from
u16 crc;                        // buffer CRC value
u08 PageBuffer[SPM_PAGESIZE];   // page buffer for data written to flash
                                // Arrays take less code if they are global.
                                // Scaler variables take less code if they 
                                // are local to a function.


#if defined(TRY_PORT0) && defined(TRY_PORT1)
   u08 RepeatCount[2];       // how may P's we have seen on each UART channel
   #define has_byte() ((boot_port == 0) ? (UCSR0A & (1<<RXC0)): (UCSR1A & (1<<RXC1)))
#elif defined(TRY_PORT0)
   #define has_byte()  (UCSR0A & (1<<RXC0))
#elif defined(TRY_PORT1)
   #define has_byte() (UCSR1A & (1<<RXC1))
#endif


#define NO_INLINE __attribute__((noinline))
#define INLINE __attribute__((inline))


void NO_INLINE __vector_default()
{
   // This dummy function shuts up a linker error.  We use a linker script
   // that discards the interrupt vector table.  It also winds up discarding
   // this function (which is where undefined interrupts would wind up).
}


#ifdef BOOT_LEDS
   void NO_INLINE led_pat(u08 n)
   {  // bit pattern low 4 bits define LEDs that are on  0000...1111
     cbi(PORTE,2);    //saves 10 bytes by always turning off all LEDs
     cbi(PORTD,6);
     cbi(PORTG,3);
     cbi(PORTD,7);

     if(n & 8) sbi(PORTE,2); // LED 1 ON
     if(n & 4) sbi(PORTD,6); // LED 2 ON
     if(n & 2) sbi(PORTD,7); // LED 3 ON
     if(n & 1) sbi(PORTG,3); // LED 4 ON
   }
#else
   #define led_pat(x)
#endif



// low level polled UART routines

void NO_INLINE do_crc(u08 b)
{
u08 i;

   crc ^= b;
   for(i=0; i<8; i++) {
      if(crc & 1) crc = (crc >> 1) ^ 0xA001;
      else        crc = (crc >> 1);
   }
}

u08 NO_INLINE get_byte(u08 crc_flag)
{
u08 b;

   while(!has_byte());

   #if defined(TRY_PORT0) && defined(TRY_PORT1)
     if(boot_port == 0) b = (UDR0);
     else b = (UDR1);
   #elif defined(TRY_PORT0)
      b = (UDR0);
   #elif defined(TRY_PORT1)
      b = (UDR1);
   #endif

   if(crc_flag) do_crc(b);
   return b;  // shut up compiler warnings
}


u16 NO_INLINE get_word(u08 crc_flag)
{
   u16 L = get_byte(crc_flag);
   u16 H = get_byte(crc_flag);

   return ((H << 8) + L);   // + takes 6 less bytes than |
}


void NO_INLINE send_byte(u08 b) 
{
   #if defined(TRY_PORT0) && defined(TRY_PORT1)
      if(boot_port == 0) {
         while(!( UCSR0A & (1<<UDRE0)) ); // wait for empty transmit
         UDR0 = b;  // send
      }
      else {
         while(!( UCSR1A & (1<<UDRE1)) ); // wait for empty transmit
         UDR1 = b;  // send
      }
   #elif defined(TRY_PORT0)
      while(!( UCSR0A & (1<<UDRE0)) ); // wait for empty transmit
      UDR0 = b;  // send
   #elif defined(TRY_PORT1)
      while(!( UCSR1A & (1<<UDRE1)) ); // wait for empty transmit
      UDR1 = b;  // send
   #endif
}


#ifdef CRYPTO_BOOT

#define ENCODE 0
#define DECODE 1

#ifdef XTEA
   #define CRYPTO_BLOCK_SIZE 8
   #define crypto_init()

   void NO_INLINE crypto(u08 mode, u08 *in, u08 *out)
   {  // XTEA encryption (small and fast, supposedly high quality encryption)
   u08 n;
   unsigned long y, z, sum;
   #define delta 0x9E3779B9L
   #define k(x) (((unsigned long *) &tea_key[0])[x])

      y = *((unsigned long *) &in[0]);
      z = *((unsigned long *) &in[4]);
      n = XTEA_ROUNDS;   // 32 is standard

      #ifdef VERIFY_FLASH_XXX
         if(mode == DECODE) sum = delta * (unsigned long) n;
         else sum = 0;
      #else
         sum = delta * (unsigned long) n;
      #endif

      while(n-- > 0) {
#ifdef VERIFY_FLASH_XXX
         if(mode == DECODE) {
             z -= (((y << 4) ^ (y >> 5)) + y) ^ (sum + k((sum>>11) & 3));
             sum -= delta;
             y -= (((z << 4) ^ (z >> 5)) + z) ^ (sum + k(sum & 3));
          }
          else {
             y += (((z << 4) ^ (z >> 5)) + z) ^ (sum + k(sum & 3));
             sum += delta;
             z += (((y << 4) ^ (y >> 5)) + y) ^ (sum + k((sum>>11) & 3));
          }
#else
          z -= (((y << 4) ^ (y >> 5)) + y) ^ (sum + k((sum>>11) & 3));
          sum -= delta;
          y -= (((z << 4) ^ (z >> 5)) + z) ^ (sum + k(sum & 3));
#endif
      }

      *((unsigned long *) &out[0]) = y;
      *((unsigned long *) &out[4]) = z;
   }
#endif


#if defined(DES) || defined(TRIPLE_DES) // not XTEA (i.e. DES/3DES)
   #define CRYPTO_BLOCK_SIZE 8
   #define crypto_init() make_spbox();

   #ifdef TRIPLE_DES
      #define DES_KEYS 3
   #else
      #define DES_KEYS 1
   #endif

   #define left (t.half[0])
   #define right (t.half[1])

   unsigned char p_perm[] = {   /* Permutation P */
      16+16,   7+8, 20+20, 21+24, 29+32, 12+12, 28+28, 17+20,
        1+4, 15+16, 23+24, 26+28,   5+8, 18+20, 31+32, 10+12,
        2+4,   8+8, 24+24, 14+16, 32+32, 27+28,   3+4,  9+12,
      19+20, 13+16, 30+32,   6+8, 22+24, 11+12,   4+4, 25+28,
      0
   };

   unsigned char s_box[8][32] = {  // the infamous s-boxes  - packed two entries per byte
      {  0xE0, 0x4F, 0xD7, 0x14, 0x2E, 0xF2, 0xBD, 0x81,
         0x3A, 0xA6, 0x6C, 0xCB, 0x59, 0x95, 0x03, 0x78,
         0x4F, 0x1C, 0xE8, 0x82, 0xD4, 0x69, 0x21, 0xB7,
         0xF5, 0xCB, 0x93, 0x7E, 0x3A, 0xA0, 0x56, 0x0D},

      {  0xF3, 0x1D, 0x84, 0xE7, 0x6F, 0xB2, 0x38, 0x4E,
         0x9C, 0x70, 0x21, 0xDA, 0xC6, 0x09, 0x5B, 0xA5,
         0x0D, 0xE8, 0x7A, 0xB1, 0xA3, 0x4F, 0xD4, 0x12,
         0x5B, 0x86, 0xC7, 0x6C, 0x90, 0x35, 0x2E, 0xF9},

      {  0xAD, 0x07, 0x90, 0xE9, 0x63, 0x34, 0xF6, 0x5A,
         0x12, 0xD8, 0xC5, 0x7E, 0xBC, 0x4B, 0x2F, 0x81,
         0xD1, 0x6A, 0x4D, 0x90, 0x86, 0xF9, 0x38, 0x07,
         0xB4, 0x1F, 0x2E, 0xC3, 0x5B, 0xA5, 0xE2, 0x7C},

      {  0x7D, 0xD8, 0xEB, 0x35, 0x06, 0x6F, 0x90, 0xA3,
         0x14, 0x27, 0x82, 0x5C, 0xB1, 0xCA, 0x4E, 0xF9,
         0xA3, 0x6F, 0x90, 0x06, 0xCA, 0xB1, 0x7D, 0xD8,
         0xF9, 0x14, 0x35, 0xEB, 0x5C, 0x27, 0x82, 0x4E},

      {  0x2E, 0xCB, 0x42, 0x1C, 0x74, 0xA7, 0xBD, 0x61,
         0x85, 0x50, 0x3F, 0xFA, 0xD3, 0x09, 0xE8, 0x96,
         0x4B, 0x28, 0x1C, 0xB7, 0xA1, 0xDE, 0x72, 0x8D,
         0xF6, 0x9F, 0xC0, 0x59, 0x6A, 0x34, 0x05, 0xE3},

      {  0xCA, 0x1F, 0xA4, 0xF2, 0x97, 0x2C, 0x69, 0x85,
         0x06, 0xD1, 0x3D, 0x4E, 0xE0, 0x7B, 0x53, 0xB8,
         0x94, 0xE3, 0xF2, 0x5C, 0x29, 0x85, 0xCF, 0x3A,
         0x7B, 0x0E, 0x41, 0xA7, 0x16, 0xD0, 0xB8, 0x6D},

      {  0x4D, 0xB0, 0x2B, 0xE7, 0xF4, 0x09, 0x81, 0xDA,
         0x3E, 0xC3, 0x95, 0x7C, 0x52, 0xAF, 0x68, 0x16,
         0x16, 0x4B, 0xBD, 0xD8, 0xC1, 0x34, 0x7A, 0xE7,
         0xA9, 0xF5, 0x60, 0x8F, 0x0E, 0x52, 0x93, 0x2C},

      {  0xD1, 0x2F, 0x8D, 0x48, 0x6A, 0xF3, 0xB7, 0x14,
         0xAC, 0x95, 0x36, 0xEB, 0x50, 0x0E, 0xC9, 0x72,
         0x72, 0xB1, 0x4E, 0x17, 0x94, 0xCA, 0xE8, 0x2D,
         0x0F, 0x6C, 0xA9, 0xD0, 0xF3, 0x35, 0x56, 0x8B}
   };


   unsigned char mask[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

   /*
      Permute bits in array *FROM* into array *TO*
      Array *XLATE* gives mapping from *FROM* array into *TO* array.
   */
   void NO_INLINE permute(u08 *from, u08 *to, u08 *xlate)
   {
   u08 xlate_byte, to_mask;
   int to_ndx, xlate_ndx;

      to_ndx = xlate_ndx = 0;
      to_mask = 0x80;
      to[0] = 0;
      xlate_byte = xlate[0];

      while(1) {    /* Map each source bit to the dest */
         if(((--xlate_byte) < 64) && (from[xlate_byte >> 3] & mask[xlate_byte & 7])) {
            to[to_ndx] |= to_mask;
         }
         if((xlate_byte = xlate[++xlate_ndx]) == 0) break;  /* End of table */
         if((to_mask >>= 1) == 0) {  /* Finished with one source byte, get next one */
            ++to_ndx;
            to[to_ndx] = 0;
            to_mask = 0x80;
         }
      }
   }


   // expand the s-boxes via the p-perm so that everything runs real fast
   unsigned long sp_box[8][64];
   u08 s_temp[8];

   void NO_INLINE make_spbox(void)
   {
   int i, j, k;
   int col;
   unsigned long perm_word;

      for(i=0; i<8; i++) {
         col = 0;
         for(j=0; j<32; j++) {
            perm_word = 0;
            for(k=0; k<8; k++) s_temp[k] = 0;
            s_temp[i] = s_box[i][j] >> 4;
            permute(s_temp, (u08 *) &perm_word, p_perm);
            sp_box[i][j*2] = perm_word;

            perm_word = 0;
            for(k=0; k<8; k++) s_temp[k] = 0;
            s_temp[i] = s_box[i][j] & 0x0F;
            permute(s_temp, (u08 *) &perm_word, p_perm);
            sp_box[i][j*2+1] = perm_word;
         }
      }
   }

   unsigned long NO_INLINE des_core(u08 *r, unsigned long l, u08 *sk_ptr)
   {   /* The f(r,k) function of course */
   u08 e0;

      e0 = r[0] >> 3;
      if(r[3] & 0x01) e0 |= 0x20;
      l ^= (sp_box[0][(e0 ^ (*sk_ptr++)) & 0x3F]);

      e0 = r[0] << 1;
      if(r[1] & 0x80) e0 |= 0x01;
      l ^= (sp_box[1][(e0 ^ (*sk_ptr++)) & 0x3F]);

      e0 = r[1] >> 3;
      if(r[0] & 0x01) e0 |= 0x20;
      l ^= (sp_box[2][(e0 ^ (*sk_ptr++)) & 0x3F]);

      e0 = r[1] << 1;
      if(r[2] & 0x80) e0 |= 0x01;
      l ^= (sp_box[3][(e0 ^ (*sk_ptr++)) & 0x3F]);

      e0 = r[2] >> 3;
      if(r[1] & 0x01) e0 |= 0x20;
      l ^= (sp_box[4][(e0 ^ (*sk_ptr++)) & 0x3F]);

      e0 = r[2] << 1;
      if(r[3] & 0x80) e0 |= 0x01;
      l ^= (sp_box[5][(e0 ^ (*sk_ptr++)) & 0x3F]);

      e0 = r[3] >> 3;
      if(r[2] & 0x01) e0 |= 0x20;
      l ^= (sp_box[6][(e0 ^ (*sk_ptr++)) & 0x3F]);

      e0 = r[3] << 1;
      if(r[0] & 0x80) e0 |= 0x01;
      l ^= (sp_box[7][(e0 ^ (*sk_ptr)) & 0x3F]);

      return l;
   }

   void NO_INLINE crypto(u08 des_mode, u08 *in, u08 *out)
   {
   u08 i;
   u08 k;
   u08 *sk_ptr;
   unsigned long temp;
   union {     /* Make temp buffer and easy access to the halves */
      u08 txt[8];
      unsigned long half[2];
   } t;

      // We don't do initial/final DES bit permutations since they just take
      // time and code and add no crypto foo powers.  They just shuffle the
      // input/output bits around.  If you are anal about such things,  you
      // can shuffle your own damn bits before/after calling this routine.

      for(i=0; i<8; i++) {   /* initialize DES buffer */
         t.txt[i] = in[i];
      }

      for(k=0; k<DES_KEYS; k++) {
         for(i=0; i<16; i++) {  /* Do 16 iterations */
            #ifdef TRIPLE_DES
               if(k == 0) {
                  if(des_mode == ENCODE) sk_ptr = &sub_key[i][0];
                  else sk_ptr = &sub_key3[15-i][0];
               }
               else if(k == 1) {
                  if(des_mode == DECODE) sk_ptr = &sub_key2[i][0];
                  else sk_ptr = &sub_key2[15-i][0];
               }
               else {
                  if(des_mode == ENCODE) sk_ptr = &sub_key3[i][0];
                  else sk_ptr = &sub_key[15-i][0];
               }
            #else
               if(des_mode == ENCODE) sk_ptr = &sub_key[i][0];
               else sk_ptr = &sub_key[15-i][0];
            #endif // TRIPLE_DES

            temp = des_core((u08 *)&right, left, sk_ptr);
            left = right;
            right = temp;
         }

         temp = left;         /* Final block transformation */
         left = right;
         right = temp;
      }

      for(i=0; i<8; i++) {    /* final "permutation" ... which is nothing */
         out[i] = t.txt[i];
      }
   }

#endif // DES / TRIPLE_DES



#ifdef AES
#define CRYPTO_BLOCK_SIZE (AES_BLOCK_SIZE/8)
#define crypto_init()  gen_aes_tables();  
/*
   ******************************************************************
   **       Advanced Encryption Standard implementation in C.      **
   ******************************************************************
   This is the source code for encryption using the latest AES algorithm.
   AES algorithm is also called Rijndael algorithm. AES algorithm is
   recommended for non-classified by the National Institute of Standards
   and Technology(NIST), USA. Now-a-days AES is being used for almost
   all encryption applications all around the world.

   This code started from the implementation by Niyaz PK,  available
   at hoozie.com.   niyazlife@gmail.com

   The s-box table generation code is based upon Mike Scott's code:
   mike@compapp.dcu.ie

   The MixColumns code is based upon Martin Richard's BCPL code.
   (wow,  when was the last time you coded in BCPL?)

   All those programs were tossed in a blender,  run through a sieve,
   and compressed in a wine press by Mark Sims,  and voila:  a rather
   clean and compact (if not all that efficent) AES routine.
    
   Find the Wikipedia page of AES at:
   http://en.wikipedia.org/wiki/Advanced_Encryption_Standard
   ******************************************************************
*/
 
// The number of columns comprising a state in AES. 
// This is a constant in AES. Value=4
#define Nb 4
 
// The number of 32 bit words in the key
#define Nk (AES_KEY_SIZE/32)
 
// The number of rounds in AES Cipher
#define Nr (Nk+6)
 
// state - the array that holds the intermediate results during encryption.
unsigned char state[4][4];
 
// The array that stores the round keys.  There are Nb(Nr+1) 4 byte round keys. 
// The round keys are used in each round to encrypt the states.
// unsigned char AES_RoundKey[240];
 
// The round constant word array, Rcon[i], contains the values given by
// x to the power (i-1) being powers of x (x is denoted as {02}) in the 
// field GF(28).  
// --- We calculate Rcon on the fly ---
 
// this table maps state[j][i] addresses to linear addresses
// it is used to replace two dimensional access to the state array
// by a simple one dimensional access...  saves a bunch 'o bytes on the AVR
u08 ndx[16] = {
   0x00,   0x04,   0x08,   0x0C,   0x01,   0x05,   0x09,   0x0D,
   0x02,   0x06,   0x0A,   0x0E,   0x03,   0x07,   0x0B,   0x0F
};
 
// this function returns the product of {02} and the argument modulo {1b} 
u08 xtime(u08 x)
{
  return (((x)<<1) ^ ((((x)>>7) & 1) * 0x1b));
}
 
#define MAKE_AES_TABLES   // define this to calculate the sbox tables

#ifdef MAKE_AES_TABLES
static u08 ptab[256], ltab[256];      // power and log tables built here
static u08 sbox[256];                 // encrypt sbox built here
//static u08 inv_sbox[256];             // decrypt sbox built here

#define ROTL(x) (((x)>>7)|((x)<<1))   // rotate x left one bit

void gen_aes_tables(void)
{ /* generate sbox tables */
int i;
u08 x, y;

    /* use 3 as primitive root to generate power and log tables */
    ltab[0] = 0;
    ptab[0] = 1;  ltab[1] = 0;
    ptab[1] = 3;  ltab[3] = 1; 
    for(i=2; i<256; i++) {
        ptab[i] = ptab[i-1] ^ xtime(ptab[i-1]);
        ltab[ptab[i]] = i;
    }
    
    /* affine transformation: each bit is xored with itself shifted one bit */
    sbox[0] = 0x63;
//  inv_sbox[0x63] = 0;
    for(i=1; i<256; i++) {
        y = ptab[255 - ltab[i]];  /* multiplicative inverse */
        x = y;  x = ROTL(x);
        y ^= x; x = ROTL(x);
        y ^= x; x = ROTL(x);
        y ^= x; x = ROTL(x);
        y ^= x; y ^= 0x63;

        sbox[i] = y; 
//      inv_sbox[y] = i;
    }
}
#else  
   #define gen_aes_tables()
   // add hard coded sbox tables here
#endif

 
// This function adds (XOR's) the round key to state.
void AddRoundKey(int round)
{
u08 i, j;
u08 *s;

    s = &state[0][0];
    j = round * Nb * 4;
    for(i=0; i<16; i++) s[ndx[i]] ^= AES_RoundKey[j++];
}
 
 
// The ShiftRows() function shifts the rows in the state to the left.
// Each row is shifted with different offset.
// Offset = Row number. So the first row is not shifted.
void ShiftRow(u08 row)
{
u08 temp;
u08 i;

   temp = state[row][0];
   for(i=0; i<3; i++) state[row][i] = state[row][i+1];
   state[row][3] = temp;
}

void aes_encrypt(u08 *in, u08 *out)
{
u08 i, round;
u08 *s;
u08 t, u;
#define a state[0][i]
#define b state[1][i]
#define c state[2][i]
#define d state[3][i]
 
    //Copy the input PlainText to state array.
    s = &state[0][0];
//  for(i=0; i<16; i++) s[ndx[i]] = in[i];
    for(i=0; i<16; i++) s[i] = in[i];   //smaller, faster, order to filling state array
 
    // Add the First round key to the state before starting the rounds.
    AddRoundKey(0);
   
    // There will be Nr rounds.
    // The first Nr-1 rounds are identical.
    // The MixColumns function is not done in the last round.
    for(round=1; round<=Nr; round++) {
        for(i=0; i<16; i++) s[i] = sbox[s[i]];  // SubBytes();

        ShiftRow(1);   // ShiftRows();
        ShiftRow(2);
        ShiftRow(2);
        ShiftRow(3);
        ShiftRow(3);
        ShiftRow(3);

        if(round != Nr) {   //MixColumns();
           for(i=0; i<4; i++) {   // for each column in the state array
              t = a;
              u = a ^ b ^ c ^ d;

              a = a ^ u ^ xtime(t ^ b);
              b = b ^ u ^ xtime(b ^ c);
              c = c ^ u ^ xtime(c ^ d);
              d = d ^ u ^ xtime(d ^ t);
           }
        }

        AddRoundKey(round);
    }
 
    // The encryption process is over.
    // Copy the state array to output array.
//  for(i=0; i<16; i++) out[i] = s[ndx[i]];
    for(i=0; i<16; i++) out[i] = s[i]; // smaller, faster, order to filling state array

#undef a
#undef b
#undef c
#undef d
}


// Note that for AES, we reverse the usual meanings of
// encrypt and decrypt.  Encryption is smaller and faster in AES
// than decryption.   SHHH scrambles the original input hex file by using 
// the AES decrypt function. This lets us use the smaller and
// faster AES encryption routine in the AVR bootloader to unscramble.
// the incomming hex file.
#define crypto(m,p,c) aes_encrypt(p, c)
 
#endif



#endif // CRYPTO_BOOT


int main(void)
{
u16 i;                  // general purpose loop index
u16 w;                  // general purpose data word
u08 led;                // use to scan LEDs
u08 RepCount;           // how may P's we have seen on the UART channel
long timeout;           // use to time how long we have been looking for P's
u08 cmd;                // 'R'ead or 'W'rite
u16 PageNum;            // the page number of the data block
unsigned long PageAddr; // where the page is in AVR memory
u08 lock_bits;          // the boot loader lock fuses
u08 flash_written;      // flag set if we write flash

   #ifdef BOOT_INHIBIT
      cbi(DDRB,BOOT_INH_PBN);  // make bootloader inhbit pin an input
      sbi(PORTB,BOOT_INH_PBN); // make it a weak pullup - and use following code for delay
   #endif           // before we actually test the pin later on

   #ifdef BOOT_LEDS
      sbi(DDRE,2);  // enable LED output bits
      sbi(DDRD,7);
      sbi(DDRD,6);
      sbi(DDRG,3);
   #endif

   lock_bits = boot_lock_fuse_bits_get(GET_LOCK_BITS);
   #ifndef QUIET_BOOTLOADER
      u08 boot_size;          // boot loader block size fuses
      u08 low_fuse;
      u08 ext_fuse;
      low_fuse  = boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS);
      boot_size = boot_lock_fuse_bits_get(GET_HIGH_FUSE_BITS);
      ext_fuse  = boot_lock_fuse_bits_get(GET_EXTENDED_FUSE_BITS);
   #endif

   RepCount = flash_written = 0;

   #ifdef CRYPTO_BOOT
      #ifdef CRYPTO_LOCKDOWN
         while(lock_bits != 0xCC) {  // lock bits not compatible with keeping secrets secret
            #ifdef BOOT_LEDS
               for(timeout=0; timeout<TIME_OUT/4; timeout++) {  // for loop is shorter than while()
                  led_pat(RepCount);
               }
               RepCount ^= 0x0F;
            #endif
         }
      #endif

      crypto_init();
   #endif

   #ifdef BOOT_INHIBIT

      no
      if(bit_is_clear(PINB,BOOT_INH_PBN)) goto app_code;
   #endif


   // setup UARTs  (defaults to 8 bit chars - no parity check)
   // FIXED BAUD RATE (standard is 115,200 baud)
   //
   // RTS useful for devices that require it
   // e.g. wireless serial device  "Air Cable"
   //
   #ifdef TRY_PORT0
      UBRR0L = (BAUD_VAL & 0xFF);          // baud rate
      UBRR0H = (BAUD_VAL >> 8);
      UCSR0A = (1<<U2X0);                  // 2X clock
      UCSR0B = (1<<TXEN0) | (1<<RXEN0);    // enable xmit and rcvr
      enable_rts0();   // Hard assert RTS output
      assert_rts0();   // but ignore CTS input  to allow 3 wire connection to host
   #endif

   #ifdef TRY_PORT1
      UBRR1L = (BAUD_VAL & 0xFF);          // baud rate
      UBRR1H = (BAUD_VAL >> 8);
      UCSR1A = (1<<U2X1);                  // 2X clock
      UCSR1B = (1<<TXEN1) | (1<<RXEN1);    // enable xmit and rcvr
      enable_rts1();   // Hard assert RTS output
      assert_rts1();   // but ignore CTS input  to allow 3 wire connection to host
   #endif


   // roll a single bit through low 4 bits of variable "led" which is output 
   // to LEDs while looking for two consecutive Ps on a UART channel.  
   // first channel to cough up the P's, wins.

   for(led=0x10; led; led>>=1) {
      led_pat(led);

      // Wait for data to be received
      #if defined(TRY_PORT0) && defined(TRY_PORT1)  // bootload from either port
         timeout = TIME_OUT;
         while(--timeout) {
            if(has_byte())  {
               if(get_byte_no_crc() == 'P') {
                if(++RepeatCount[boot_port] >= P_COUNT) {
                     goto boot_load;
                  }
               }
               else RepeatCount[boot_port] = 0;
            }
            boot_port ^= 1;
         }
       #elif defined(TRY_PORT0)  // bootload from port 0 only
          timeout = TIME_OUT;
          while(--timeout) {
             if(has_byte())  {
                if(get_byte_no_crc() == 'P') {
                   if(++RepCount >= P_COUNT) {
                      goto boot_load;
                   }
                 }
                else RepCount = 0;
             }
          }
       #elif defined(TRY_PORT1)  // bootload from port1 only
          boot_port = 1;
          timeout = TIME_OUT;
          while(--timeout) {
             if(has_byte()) {
                if(get_byte_no_crc() == 'P') {
                   if(++RepCount >= P_COUNT) {
                      goto boot_load;
                   }
                }
                else RepCount = 0;
             }
          }
       #endif
   }
   goto app_code;

   boot_load:
   // Host is expected to send blocks, via serial port, of SPM_PAGESIZE
   // preceded by Page Number (16 bits)
   // e.g. on Mega256x  256 bytes
   //      on Mega128   128 bytes

   #ifdef PROTECT_BOOTLOADER
      // set boot loader lock bits to protect boot loader from overwrite (and read)
      // crypto boot loader needs memory protect mode 3 set so that ISP/JTAG cannot
      // dump the code,  but we can't do this in software... stupid ATMEL dingleberries...
      boot_lock_bits_set(~0xCC);  // lower two bits are memory protect mode 3
   #endif

   #ifndef TINY_BOOT
      led_pat(0xF);   // turn on all LEDs
   #endif

   #ifdef QUIET_BOOTLOADER
      send_byte('!');                   // Ready Prompt
      send_byte(BOOT_LOADER_VERSION);   // boot loader version number
   #else
      send_byte('$');                   // Ready Prompt
      send_byte(BOOT_LOADER_VERSION);   // boot loader version number
      send_byte(SPM_PAGESIZE & 0xFF);   // followed by page size
      send_byte(SPM_PAGESIZE >> 8  );   // which must be used by host   L,H

      // inform host memory size (includes boot loader partition)
      send_byte((FLASHEND+1) >> 12);    // multiples of 4K bytes
                                        // e.g. Mega128  = 32
                                        //      Mega256x = 64

      // send host the boot lock fuse settings
      send_byte(low_fuse);
      send_byte(lock_bits);
      send_byte(ext_fuse);
      send_byte(boot_size);
   #endif

   while(1) {
      crc_init();

      PageNum = get_word_with_crc();  // L,H
      if(PageNum == 0xFFFF) break;    // exit do while(1) loop
      led_pat(PageNum);

      PageAddr = ((long) (PageNum&0x7FFF)) * SPM_PAGESIZE;  // calc address, multiple of block size

      cmd = get_byte_no_crc();  // bootloader command byte: R or W

#ifdef VERIFY_FLASH
      if((cmd == 'W') || (cmd == 'R')) { // 'W'rite command
#else
      if(cmd == 'W') { // 'W'rite command
#endif
         // By erasing the page in the middle of the receive data loop,  we
         // overlap the page erase/program times with the buffer fill time.
         // This way we do not have to explicly waste time waiting for the  
         // page erase or page program to complete.  
         for(i=0; i<SPM_PAGESIZE; i++) {   // fill page buffer with received data
            PageBuffer[i] = get_byte_with_crc();
            if((cmd == 'W') && (i == (SPM_PAGESIZE/2)) && ((PageNum&0x8000) == 0)) {
                flash_written = 1;
                boot_page_erase(PageAddr); // erase the FLASH memory page
            }
         }

         if(get_word_no_crc() == crc) { // if local CRC matches received CRC then write block
            #ifdef CRYPTO_BOOT
               for(i=0; i<SPM_PAGESIZE; i+=CRYPTO_BLOCK_SIZE) {  // decrypt the page's data
                  crypto(DECODE, &PageBuffer[i], &PageBuffer[i]);
               }
            #endif

            if(cmd == 'W') {  // program received data into donkey memory
               if(EEPROM_PRGM && (PageNum & 0x8000)) { //write EEPROM data
                  for(i=0; i<SPM_PAGESIZE; i++) {
                     eeprom_write_byte((uint8_t *) (((unsigned) PageAddr)+i), (uint8_t) PageBuffer[i]);
                  }
               }
               else {   // write FLASH data
                  // erase should have completed during download of last half of buffer
                  for(i=0; i<SPM_PAGESIZE; i+=2) {  // copy received data to chip page buffer
                     w = PageBuffer[i+1];
                     w <<= 8;
                     w += PageBuffer[i];            // + takes 6 less bytes than |
                     boot_page_fill(PageAddr+i,w);  // word at a time L,H
                  }

                  boot_page_write(PageAddr);  // Store chip page buffer in flash page.
                  // write will complete during download of first half of next buffer
               }
            }
#ifdef VERIFY_FLASH
            else {   // compare received data buffer to donkey memory
               for(i=0; i<SPM_PAGESIZE; i++) {
                  if(EEPROM_PRGM && (PageNum & 0x8000)) {  // compare EEPROM data
                     if(PageBuffer[i] != eeprom_read_byte((uint8_t *) ((unsigned)PageAddr+i))) goto nak;
                  }
                  else {  // compare FLASH data
                     if(PageBuffer[i] != pgm_read_byte_far(PageAddr+i)) goto nak;
                  }
               }
            }
#endif
            send_byte('A');  // ACK
         }
         else {
            #ifdef VERIFY_FLASH
            nak:
            #endif
            send_byte('N');  // NAK
         }
      } // end else W)rite
      else {  // bad boot command character,  exit boot loop
         break;
      }
   } // end while(1)  with Page=FFFF causing break in loop

   while(boot_spm_busy());


   // boot data transfers complete 
   #ifndef TINY_BOOT
      led_pat(0x08); // turn on first LED
   #endif

   #ifdef EE_FLASH_SUM    // force recalc of flash checksum when code restarts
      if(flash_written) { 
         // two eeprom_write_bytes are smaller than one eeprom_write_word
         eeprom_write_byte((uint8_t *) (unsigned) EE_FLASH_SUM+0, (uint8_t) 0xFF);
         eeprom_write_byte((uint8_t *) (unsigned) EE_FLASH_SUM+1, (uint8_t) 0xFF);
      }
   #endif

   app_code:
   boot_rww_enable();  // enable application section
   asm("jmp 0");       // jump to application code

   goto app_code;      // should never get here
}

