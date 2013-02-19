/****************************************************************************

    COPYRIGHT:      Copyright (c) 1990, Computer Presentations, Inc.
                    All rights reserved.

    PROJECT:        Image Prep 3.0

    MODULE:         testtga.c

    PROCEDURES:     TestTGA () 

*****************************************************************************/                                                        
#include <windows.h>
#include "imgprep.h"
#include "tgafmt.h"

/****************************************************************************

    PROCEDURE:          TestTGA
    DESCRIPTION:        Verifies if passed handle is really a .TGA file
    DEFINITION:         ?
    START:              1/4/90  David T. Ison   
    MODS:

*****************************************************************************/

BOOL TestTGA (hFile, lpBuffer) 
HANDLE   hFile;
LPSTR    lpBuffer; 
{
  BOOL                Retval = TRUE;
  TARGAHEADER FAR *   lpTargaHeader;
  WORD                wBytes;


  if (_llseek (hFile, 0L, 0))
    return( FALSE );

  lpTargaHeader = (TARGAHEADER FAR *) lpBuffer;

  /**************  Verify file first by reading from it *********************/

  wBytes = _lread ( (int)hFile, (LPSTR)lpTargaHeader, 
                    (int)sizeof (TARGAHEADER));

  if ((int) wBytes == -1 || wBytes != sizeof (TARGAHEADER))
     Retval = FALSE;
  else
    {
      /************************  Verify header  *****************************/

      switch (lpTargaHeader->ImageType) 
        {
          case 1:     //  8 bit colormapped
          case 2:     //  RGB 16 or 24 bit
          case 3:     //  8 bit grayscale 
          case 9:     //  8 bit colormapped RLE
          case 10:    //  RGB 16 or 24 bit  RLE
          case 11:    //  8 bit grayscale   RLE
            Retval = TRUE;
          break;

          default:
            Retval = FALSE;
          break;
        }

      if (Retval)
         switch (lpTargaHeader->PixelDepth) 
           {
             case 8:     //  8 bit colormapped or grayscale
             case 16:    //  RGB 16 or 24 bit
             case 24: 
             case 32: 
               Retval = TRUE;
             break;

             default:
               Retval = FALSE;
             break;
           }
    }
  return (Retval);
}

