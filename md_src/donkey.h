/*  MegaDonkey Library File:  donkey.h    Master Header File - Include this in your projects if using Donkey Library



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
 


*/



#ifndef _MDLIB_H
#define _MDLIB_H

#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/crc16.h>
#include <stdio.h>

#include "md.h"           // Init code & global types  
#include "uart.h"         // UART support
#include "timer.h"        // System Timer (timer 0) & low-level support for other timers
#include "lcd.h"          // LCD hardware/graphics/bitmapped text support
#include "adc.h"          // adc sampler and touch screen support 
#include "twi.h"          // Two Wire Interface (I2C)
#include "led.h"          // LED support

#include "delay.h"        // delay delay_us(n) 2..65535 microseconds 

#include "menu.h"         // menu support (light GUI) 

#include "vchar.h"        // vector character drawing
#include "graph.h"        // cartesian / polar graph utils / 2D graphics / clipping
                          // also map demo
#include "servo1.h"       // hobby RC servo support JitterFree Timer1 fixed 2 Channel PB6 & PB7
#include "servo3.h"       // hobby RC servo support JitterFree Timer3 fixed 2 Channel PE4 & PE5

#include "varedit.h"      // variable editor
#include "curves.h"       // Bezier curves

//#include "trakball.h"     // mouse/trackball

#include "md_term.h"      // Remote Mega Donkey Terminal (MD Term) with LCD
                          // emulation capability

#include "kbd.h"          // keyboard
#include "calc.h"         // calculator / numerical input



#endif
