@echo off
ask Do you have the make file set up for COLORLAB ? 
echo .
if errorlevel 1 goto done
ask Do you also want to re-compile sources used in DECIPHER ? 
echo .
if errorlevel 1 goto nodec
del obj\format.obj
del obj\open.obj
del obj\save.obj
:nodec
call de
cd res
call r
cd..
del obj\imgprep.obj
del obj\menu.obj
del obj\settings.obj
del obj\about.obj
del obj\copyp.obj
call mk
call rr
ren imgprep.exe colorlab.exe
:done
