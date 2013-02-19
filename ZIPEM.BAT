pkzip -arp   -x*.exe -x*.dll ip
rem ask want to zip lib directory ?
rem if errorlevel 1 goto nolib
rem pkzip -a              lib f:\lib\*.*
:nolib
rem ask want to zip include directory ?
rem if errorlevel 1 goto noincl
rem pkzip -a              include f:\incl\*.*
:noincl
pkzip -a              cpilib f:\cpilib\*.*
pkzip -a              cpiinc f:\cpiinc\*.*
pkzip -arp -x*.exe -x*.obj -x*.zip oicsrc d:\ms\*.* 
pkzip -arp -x*.exe -x*.obj         libsrc e:\cpi\*.* 
pkzip -a              dll  *.dll
pkzip -a              prometer  d:\dll\prometer\*.*
pkzip -a   -x*.cpi    oic       d:\oic\*.*
