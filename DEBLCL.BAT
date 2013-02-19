set temp=c:\
del *.sts
win /s  cvw /Ld:\imgprep\tools\tools.dll colorlab /nologo %1
del *.sts
set temp=h:\
