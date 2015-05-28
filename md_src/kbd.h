/*  MegaDonkey Library File:  kbd.h    Keyboard for text entry / editing    - Header File
    


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



     Keyboard for text entry and editing

	 Mark Sims 
	 Mayapril 2007
    


	 Buffer passed to keyboard is displayed on top line upon call to keyboard.
     Set length to zero before calling if you want empty string, e.g.

     char buf[16];
	 buf[0]=0;

     keyboard function does not clear screen -- call lcd_clear() after calling.


*/


#ifndef _KBD_H
#define _KBD_H


int menu_kbd(
   char *title,     // if 0 then start with five row keyboard, else display title on bottom of screen
   char *buf,       // caller provided buffer (null terminated string)
   u08 kbd_buf_len  // NOT including ending \0 byte
);                   // returns number of characters in string


#endif
