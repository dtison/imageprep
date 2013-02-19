/****************************************************************************

   COPYRIGHT:      Copyright (c) 1990, Computer Presentations, Inc.
                   All rights reserved.

   PROJECT:        Image Prep 3.0

   MODULE:         testgif.c

   PROCEDURES:     TestGIF()   5.5.6

*****************************************************************************/                                          
#include <windows.h>
#include <memory.h>
#include "cpi.h"
#include "imgprep.h"
#include "giffmt.h"
#include "error.h"

/****************************************************************************

   PROCEDURE:         TestGIF
   DESCRIPTION:       Message box procedure for ImgPrep
   DEFINITION:        5.5.6
   START:             03/05/90  David Ison
   MODS:

*****************************************************************************/

BOOL TestGIF (hFile, lpBuffer) 
HANDLE   hFile;
LPSTR    lpBuffer; 
{
  char     Buffer [8];
  WORD     wBytes;
  BOOL     Retval = TRUE;


  if (_llseek (hFile , 0L, 0))
    return( FALSE );
  
  /*************  Verify file first by reading from it **********************/

  wBytes = _lread ((int) hFile, (LPSTR) Buffer, 6);

  if( FailedLRead (wBytes, 6) )
      Retval = FALSE;
  else
    {
      /*****************  Verify signature header ***************************/

      if (_fmemcmp ((LPSTR)Buffer, (LPSTR)"GIF87a", 6) != 0)
        Retval = FALSE;
    }
  return (Retval);
}
