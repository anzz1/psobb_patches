@echo off
IF "%1"=="" goto err
fxc /nologo /Od /Zi /T fx_2_0 /Fo "%~n1.fxo" /Fc "%~n1.asm" "%1"
del %~n1.asm 2>NUL
del %~n1.fxo 2>NUL
exit /b 0
:err
echo No input file.
exit /b 1
