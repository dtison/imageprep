set temp=c:\
del *.sts
win /s  cvw /Ld:\imgprep\tools\tools.dll imgprep /nologo %1
del *.sts
set temp=d:\
