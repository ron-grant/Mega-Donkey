/*  MegaDonkey Library File:  delay.h    Inline functions for brief delays 


    


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
 

    Short Inline Delay Functions
    Ron Grant
	Jan 2008
     

    If using these functions to generate short delays in code that can be interrupted, you may want to consider
	masking interrupts.
	
	cli(); // mask interrupts
	<code to do something>
	<inline delay>
	<code to do something else>
	sei(); // enable interrupts
	 


*/




#ifndef _DELAY_H_
#define _DELAY_H_

static __inline__ void delay_us (unsigned int d)   // delay d microseconds (16MHz CPU) 
                                 // using compiler Optimization -Os set under 
                                 // Project|Configuration Options
{

   asm volatile ("push R24 \n\t"
                 "pop  R24 \n\t"
				 "push R24 \n\t"
				 "pop  R24 \n\t");
 
   if (d<2)return;
   
   d--;                                // subtract 1 us call overhead
   
   // 16 bit var loop overhead 6 cycles
   // 2 decrement
   // 2 test
   // 2 branch

   while (--d) 
     asm volatile("push R24\n\t"       // 2 cycles     10 cycle delay
				  "push R24\n\t"   // 2 cycles
				  "pop  R24\n\t"   // 2 cycles	
                  "nop \n\t"       // 1 cycle 
			      "nop\n\t"            // 1 cycle 
     		      "pop R24\n\t");      // 2 cycles
}



#endif
