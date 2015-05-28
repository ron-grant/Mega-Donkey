
Notes on setting up AVR Studio with WinAVR to compile code for Megadonkey 
February 4, 2013


AVR Studio 4.19 appears to have a bug making it not play well (harder to confgure your project)
with WinAVR - or  at least older WinAVR Like 20071221 version using with MegaDonkey.





Megadonkey source files are assumed to be located in C:\MD with subfolder structure 
  md_bin
  md_demos
  md_doc
  md_hex
  md_inc     *
  md_lib     *
  md_projects
  md_src     

where * folders are critical to compiling your code




To Configure your new project setup in AVR Studio


Menu Command : Project|Configuration Options


Select Device in Device Pull down, .i.e atmega2561 or atmega128 depending on your Donkey's controller.
Frequency 16000000  (not sure if critical)



Setup each Section  Library Paths, Include Paths, Custom Options that appear as icon buttons on the left
side of Project Options window  as detailed below:

-------
Library paths 

C:\WinAVR20071221\avr\lib\avr6
C:\MD\lib

then libc.a and libm.a  show up in list along with donkeylibs for various donkey versions

use add lib option to create a list of  for example 


libc.a
libdonkey2561_108a.a    
libm.a



note had problem selecting other lib  ended up with "__mulhi" erros 
I believe libm.a must be last in the list - can't recall the problem.



------

Include Paths - define 

C:\MD\md_inc

-------

Custom Options
  Uncheck Use AVR ToolChain and supply 
  paths to compiler and make manually  


C:\WinAVR-20071221\bin\avr-gcc.exe
C:\WinAVR-20071221\utils\bin\make.exe


SAVE YOUR PROJECT to save these options


you should be able to create a simple project and get it to compile
e.g. the code below should work.


#include "donkey.h"

int main (void)
{
  md_init_no_logo();   // older library supported md_init() only with no option to cancel logo
  printf ("Hello World \n");
  while (1);
}


