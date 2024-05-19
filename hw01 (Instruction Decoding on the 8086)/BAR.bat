:::::::::::::::::::
:: BUILD AND RUN ::
:::::::::::::::::::

@echo off
set hwName=hw01

cls
set arg1=%1
gcc %hwName%.c -o %hwName%.exe
%hwName%.exe %arg1%