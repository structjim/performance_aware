:::::::::::::::::::
:: BUILD AND RUN ::
:::::::::::::::::::
@echo off
cls

::Set hwName manually for each homework.
set hwName=hw02
::DELETE old output (So it won't run if it doesn't build.)
del %hwName%.exe
del %hwName%_disass_output.asm
del %hwName%_disass_output

::arg1 is the bin passed to this .bat
set arg1=%1
gcc %hwName%.c -o %hwName%.exe

::Test run
::1st param is input bin (passed to this .bat)
::2nd param is output .asm filename
%hwName%.exe %arg1% %hwName%_disass_output.asm

::Reassemble and compare with examble bin
echo.
echo.
echo.
echo [Re-assembling and comparing with source file...]
nasm.exe %hwName%_disass_output.asm
FC /B %arg1% %hwName%_disass_output