/*  MegaDonkey Library File:  LCDMDT.c    Mega Donkey Terminal Interface (Donkey to PC connection with LCD emulation)

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


  

   Graphic LCD Interface Code Donkey without LCD = communication with Mega Donkey Term -- Windows App.
   Mark Sims  Dec 2007
 
     
   All Functions -- send commands to MDT (see md_term.c)
   Screen Read/Write
   Character Generator (character bitmap)
   Character Drawing 
   Dot Plotting
   Blts


 This file is included in lcd.c and should not be included in IDE project, but
 instead may be added in the Other Files folder of the IDE.
 If the file is added to the IDE project you will get all kinds of errors.

 ALSO, if editing in IDE -- BE SURE to explicity save the file before 
 recompiling project -- IDE/MakeUtility will not notice if this file included 
 within LCD.C is out of date.

 This .c file does not have an associated header file.



  Graphic LCD Interface Code for Bareback Mega Donkey (No LCD Panel)
  This code is designed to use remote Mega Donkey Terminal (MD Term) applicaiton

  Note that MDT_CODE must be included and mdt_init() should be called
  Low level graphics functions are routed to MD Term


    Mark Sims  Dec 2007
    

*/


#define EXTERN extern

u08 pixel_masks[] = {
   0x01, 0x02, 0x04, 0x08,  0x10, 0x20, 0x40, 0x80
};


void set_lcdtop(unsigned row)
{  // set which scanline in the display buffer to start showing on the screen
u08 cur;

cur = erase_cursor();
  mdt_set_lcdtop(row);
if(cur) show_cursor();
}

void lcd_xinit(u08 mode)
{
   mdtNoLocalDisplay = 1;

   #ifdef MDT_CODE
      mdt_init(MDT_COM_PORT); 
   #endif

}

void fill_screen(COLOR x)
{
u08 cur;

cur = erase_cursor();
   mdt_clear(x);
if(cur) show_cursor();
}

void lcd_char(COORD col,COORD row, unsigned char c)
{
u08 cur;

cur = erase_cursor();
   mdt_charxy(c, col,row);
if(cur) show_cursor();
}


COLOR read_screen(COORD col,COORD row)
{  /* get screen byte that contains screen coordinate (col, row) */
unsigned addr;

   addr = top_line + (((((unsigned)row) * COLS) + (unsigned) col) >> 3);    /* !!! * multiply */
   return mdt_read_screen(addr);
}

void write_screen(COORD col,COORD row, COLOR val)
{  /* get screen byte that contains screen coordinate (col, row) */
unsigned addr;

   addr = top_line + (((((unsigned)row) * COLS) + (unsigned) col) >> 3);    /* !!! * multiply */
   mdt_write_screen(addr, val);
}


COLOR read_screen_byte(unsigned addr)
{
   return mdt_read_screen(addr);
}


void write_screen_byte(unsigned addr, COLOR val)
{
   mdt_write_screen(addr, val);
}


void dot(COORD col,COORD row)
{
u08 cur;

   if(col >= COLS) return;  /* !!!  cliping */
   else if(row >= ROWS) return;

cur = erase_cursor();
   mdt_dot(col,row);
if(cur) show_cursor();
}


void blit(
   COORD left,COORD top,  
   COORD right,COORD bot,  
   COORD dest_left,COORD dest_top
)
{
u08 cur;
cur = erase_cursor();
   mdt_blit(left,top,  right,bot,  dest_left,dest_top);
if(cur) show_cursor();
}

