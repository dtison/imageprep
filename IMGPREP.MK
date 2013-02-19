#       Windows Image Prep 3.0 Make File
#
PROG          = IMGPREP
PROGFLAGS     = -DCOLORLAB
OBJDIR        = .\obj
INCLUDE       = .\include
CC            = cl -nologo -c -W3 -AM -G2sw -Zpe -Fo$(OBJDIR)\$*.obj
CFLAGS        = -Oas $(PROGFLAGS)
CFLAGSD       = -Od -Zi -DDEBUG $(PROGFLAGS)
CFLAGSO       = -Ocilgt $(PROGFLAGS)
CFLAGSOD      = -Ocilgt -Zi $(PROGFLAGS)
ASM           = masm -W2 -Ml -Mx
AFLAGS        =
AFLAGSD       = -Zi
LINK          = link /NOD/NOE/AL:16
LFLAGS        = /MAP
LFLAGSD       = /CO/INC/PADCODE:512 /PADDATA:64

.c.obj:
  $(CC) $(CFLAGS) $*.c
  touch objtest

$(OBJDIR)\about.obj: about.c
  $(CC) $(CFLAGS) $*.c
  touch objtest

$(OBJDIR)\display.obj: display.c
  $(CC) $(CFLAGSD) $*.c
  touch objtest

$(OBJDIR)\dither.obj: dither.c
  $(CC) $(CFLAGSO) $*.c
  touch objtest

$(OBJDIR)\export.obj: export.c
  $(CC) $(CFLAGSD) $*.c
  touch objtest

$(OBJDIR)\file.obj: file.c
  $(CC) $(CFLAGS) $*.c
  touch objtest

$(OBJDIR)\filters.obj: filters.c
  $(CC) $(CFLAGS) $*.c
  touch objtest

$(OBJDIR)\format.obj: format.c
  $(CC) $(CFLAGS) $*.c
  touch objtest

$(OBJDIR)\imgprep.obj: imgprep.c
  $(CC) $(CFLAGSD) $*.c
  touch objtest
  
$(OBJDIR)\import.obj: import.c
  $(CC) $(CFLAGS) $*.c
  touch objtest

$(OBJDIR)\init.obj: init.c
  $(CC) $(CFLAGS) $*.c
  touch objtest

$(OBJDIR)\logo.obj: logo.c
  $(CC) $(CFLAGS) $*.c
  touch objtest

$(OBJDIR)\open.obj: open.c
  $(CC) $(CFLAGS) $*.c
  touch objtest

$(OBJDIR)\paint.obj: paint.c
  $(CC) $(CFLAGSD) $*.c
  touch objtest

$(OBJDIR)\palette.obj: palette.c
  $(CC) $(CFLAGSD) $*.c
  touch objtest

$(OBJDIR)\pref.obj: pref.c
  $(CC) $(CFLAGS) $*.c
  touch objtest

$(OBJDIR)\quantize.obj: quantize.c
  $(CC) $(CFLAGSOD)  $*.c
  touch objtest

$(OBJDIR)\reduce.obj: reduce.c
  $(CC) $(CFLAGSD) $*.c
  touch objtest

$(OBJDIR)\save.obj: save.c
  $(CC) $(CFLAGS) $*.c
  touch objtest

$(OBJDIR)\setup.obj: setup.c
  $(CC) $(CFLAGS) $*.c
  touch objtest

$(OBJDIR)\squash.obj: squash.c
  $(CC) $(CFLAGS) $*.c
  touch objtest

$(OBJDIR)\state.obj: state.c
  $(CC) $(CFLAGS) $*.c
  touch objtest

$(OBJDIR)\unsquash.obj: unsquash.c
  $(CC) $(CFLAGS) $*.c
  touch objtest

$(OBJDIR)\prev.obj: prev.c
  $(CC) $(CFLAGS) $*.c
  touch objtest

$(OBJDIR)\dith.obj: dith.c
  $(CC) $(CFLAGS) $*.c
  touch objtest

$(OBJDIR)\info.obj: info.c
  $(CC) $(CFLAGS) $*.c
  touch objtest

$(OBJDIR)\iputils.obj: iputils.c
  $(CC) $(CFLAGSD) $*.c
  touch objtest

#
# File Formats gone now..

#
# Link
#

$(PROG).exe: $(PROG).res objtest
   ilink /E "$(LINK) $(LFLAGSD) @$(PROG).lnk;touch objtest;" $(PROG) 
# rc $(PROG).res

$(PROG).exe: objtest
   rc $(PROG).res
