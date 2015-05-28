echo This copies libraries .a to md_lib  and  headers to md_inc
echo
echo Before this Batch File is run
echo Donkey Project Must be compiled with BUILD_LIBRARY switch enabled in md.h
echo then project must be built for Mega128 then execute _MakeDonkeyLib.bat then rename libdonkey.a to 
echo libdonkey128.a
echo then build project for Mega2561 and execute _MakeDonkeyLib.bat then rename libdonkey.a to 
echo libdonkey2561.a
echo
pause
copy *.h ..\md_inc
copy *.a ..\md_lib
pause