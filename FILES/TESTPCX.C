/****************************************************************************

    COPYRIGHT:      Copyright (c) 1990, Computer Presentations, Inc.
                    All rights reserved.

    PROJECT:        Image Prep 3.0

    MODULE:         testpcx.c

    PROCEDURES:     TestPCX () 

*****************************************************************************/                                                        
#include <windows.h>
#include "imgprep.h"
#include "pcxfmt.h"
#include "error.h"

/****************************************************************************

    PROCEDURE:          TestPCX
    DESCRIPTION:        Verifies if passed handle is really a .PCX file
    DEFINITION:         ?
    START:              1/4/90  David T. Ison   
    MODS:

*****************************************************************************/

BOOL TestPCX (hFile, lpBuffer) 
HANDLE   hFile;
LPSTR    lpBuffer; 
{
  BOOL             Retval = TRUE;
  PCXHEADER FAR *  lpPCXHeader;
  WORD             wBytes;

  if (_llseek (hFile, 0L, 0)){
    return (FALSE);
  }

  lpPCXHeader = (PCXHEADER FAR *) lpBuffer;

  /*************  Verify file first by reading from it **********************/

  wBytes = _lread ( (int)hFile, (LPSTR)lpPCXHeader, (int)sizeof (PCXHEADER));

  if ( ((int) wBytes == -1) || (wBytes != sizeof (PCXHEADER)))
    Retval = FALSE;
  else
    {
       /************************  Verify header *****************************/

      if (lpPCXHeader -> Manuf != 0x0A)
        Retval = FALSE;
      else
        {
          switch (lpPCXHeader -> Hard)
            {
              case 0:
              case 2:
              case 3:
              case 5:
                Retval = TRUE;
              break;

              default:
                break;
            }

          if (Retval)
            if (lpPCXHeader -> Encod != 1)
              Retval = FALSE;
        }
    }
  return (Retval);
}

