/*  MegaDonkey Library File:  trakball.c    Track Ball Support - Header File
    


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


    Mark Sims
	2007

 
*/ 

#ifndef _TRAKBALL_H_
#define _TRAKBALL_H_

#include "md.h"

#ifdef TRACKBALL_CODE

#ifdef MOUSE_UART3

#ifdef DONKEY_ALPS_BALL            // Donkey ALPS ball no swapXY and no reverse X or Y
  #define RVS_MOUSE_BUTTONS
#else 
  #define RVS_MOUSE_BUTTONS 
  #define SWAP_MOUSE_XY
  #define RVS_MOUSE_X
  //#define RVS_MOUSE_Y
#endif
#endif

// mouse message/state info -- packed into a byte
#define MOUSE_MSG_BYTE  0x07  // current byte number in mouse message (0..7)  we only do three byte messages
#define MOUSE_CHANGED   0x08
#define MOUSE_LB        0x80  // mouse left,right button  1=pressed 0=not pressed
#define MOUSE_RB        0x40
 u08 mouse_state;

#define MouseLB (mouse_state & MOUSE_LB)
#define MouseRB (mouse_state & MOUSE_RB)

#define MOUSE_RCV_QSIZE 33  //!!!!
 volatile u08 mouse_rcvq[MOUSE_RCV_QSIZE];
 volatile u08 mouse_rcvfront;
 volatile u08 mouse_rcvback;
 volatile u08 mouse_rcvcount;

 volatile u08 mouse_rcv_tick;  // current time tick within the current state
 volatile u08 mouse_rcv_state;
 volatile u08 mouse_rcv_data;

u08 get_mouse(void);

#endif // TRAKCBALL_CODE
#endif // _TRAKBALL_H
