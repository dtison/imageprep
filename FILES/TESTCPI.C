/****************************************************************************

    COPYRIGHT:      Copyright (c) 1990, Computer Presentations, Inc.
                    All rights reserved.

    PROJECT:        Image Prep 3.0

    MODULE:         testcpi.c

    PROCEDURES:     TestCPI ()       5.5.1

*****************************************************************************/                                                        
#include <windows.h>
#include "imgprep.h"
#include "cpifmt.h"

/****************************************************************************

    PROCEDURE:          TestCPI
    DESCRIPTION:        Verifies if Passed handle is really a .CPI file
    DEFINITION:         5.5.1
    START:              1/4/90  David T. Ison   
    MODS:               4/19/90 3.0 review and annotate

*****************************************************************************/

BOOL TestCPI( hFile, lpBuffer ) 
HANDLE   hFile;
LPSTR    lpBuffer; 
{
  BOOL             Retval = TRUE;                        /* return variable */
  LPCPIFILESPEC    lpFileSpecPtr;            /* internal cpi header pointer */
  WORD             wBytes;               /* byte counter variable for reads */

  if (_llseek (hFile , 0L, 0))
    return( FALSE );
  
  lpFileSpecPtr = (LPCPIFILESPEC)lpBuffer;

  /*******************  Verify file first by reading it  ********************/

  wBytes = _lread( (int)hFile, (LPSTR)lpFileSpecPtr, sizeof (CPIFILESPEC) );

  if ( ((int)wBytes == -1 ) || ( wBytes != sizeof (CPIFILESPEC)) )
    {
       Retval = FALSE;
    }
  else
    {      /*************************  Verify header ************************/

      if (lstrcmp (lpFileSpecPtr -> CPiSignature, (LPSTR) "CPI") != 0)
        {
          Retval = FALSE;
        }
      else
        {
          if( lpFileSpecPtr->Filler[0] != (char)0xFF )
            { 
              Retval = FALSE;
            }
        }
     }
  return (Retval);
}

BOOL FAR PASCAL TestCompressedCPI (hFile, lpBuffer)
int    hFile;
LPSTR lpBuffer;
{
    LPCPIFILESPEC         lpCPiFileSpecPtr;
    LPCPIIMAGESPEC        lpCPiImageSpecPtr;
    DWORD                 dwSeekPos;
    WORD                  wBytes;

    int                   Retval;

    Retval = FALSE;

    if (_llseek (hFile, 0L, 0) == 0)    /*  Successful seek */
    {
        wBytes = _lread ((int) hFile, lpBuffer, sizeof (CPIFILESPEC));
//      if (! FailedLRead (wBytes, sizeof (CPIFILESPEC)))
        {
            lpCPiFileSpecPtr = (LPCPIFILESPEC) lpBuffer;
            dwSeekPos = lpCPiFileSpecPtr -> ImgDescTableOffset;

//          if (_llseek (hFile, dwSeekPos, 0) == dwSeekPos)   /*  Successful seek */
            {
                wBytes = _lread ((int) hFile, lpBuffer, sizeof (CPIIMAGESPEC));
//              if (! FailedLRead (wBytes, sizeof (CPIIMAGESPEC)))
                {
                    lpCPiImageSpecPtr = (LPCPIIMAGESPEC) lpBuffer;

                    if (lpCPiImageSpecPtr -> Compression == CPIFMT_OIC)
                        Retval = TRUE;
                }
            }

        }
    }
    return (Retval);
}

