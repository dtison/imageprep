/****************************************************************************

    COPYRIGHT:      Copyright (c) 1990, Computer Presentations, Inc.
                    All rights reserved.

    PROJECT:        Image Prep 3.0

    MODULE:         testtif.c

    PROCEDURES:     TestTGA () 

*****************************************************************************/                                                        
#include <windows.h>
#include "imgprep.h"
#include "tiffmt.h"

/****************************************************************************

    PROCEDURE:          TestTIF
    DESCRIPTION:        Verifies if passed handle is really a .TIF file
    DEFINITION:         ?
    START:              1/4/90  David T. Ison   
    MODS:

*****************************************************************************/

extern BOOL  bIsMotorola;

BOOL TestTIF (hFile, lpBuffer) 
HANDLE  hFile;
LPSTR   lpBuffer; 
{
  BOOL            Retval = TRUE;
  LPTIFFHEADER    lpTifHeader;
  WORD            wBytes;

  if (_llseek (hFile, 0L, 0))
    return (FALSE);

  lpTifHeader = (LPTIFFHEADER) lpBuffer;

  /*******************  Verify file first by reading from it ****************/

  wBytes = _lread ((int)hFile, (LPSTR)lpTifHeader, sizeof (TIFFHEADER));

  if ((int) wBytes == -1 || wBytes != sizeof (TIFFHEADER))
    Retval = FALSE;
  else
  {
      /************************  Verify header  *****************************/

    if (lpTifHeader -> ByteOrder == 0x4D4D)
        bIsMotorola = TRUE;
    else
        bIsMotorola = FALSE;

//    if (lpTifHeader->ByteOrder != 0x4949)
//      Retval = FALSE;
//    else
        if (ByteOrderWord (lpTifHeader->VersionNumber, bIsMotorola) != 0x002A)
          Retval = FALSE;
  }
  return (Retval);
}

