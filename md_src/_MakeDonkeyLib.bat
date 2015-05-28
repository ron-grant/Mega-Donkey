rem  Note: path to winavr required in this batch file

echo invoking avr archiver to build library (libdonkey.a) of all donkey object files in default sub-directory
\winavr-20071221\bin\avr-ar rsv libdonkey.a .\default\*.o
pause