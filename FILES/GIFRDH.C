/****************************************************************************

   COPYRIGHT:      Copyright (c) 1990, Computer Presentations, Inc.
                   All rights reserved.

   PROJECT:        ImagePrep 3.0

   MODULE:         gifrdh.c

   PROCEDURES:     GIFReadHeader()   5.4.5

*****************************************************************************/                                          
#include <windows.h>
#include <io.h>
#include <memory.h>
#include "cpi.h"
#include "imgprep.h"
#include "proto.h"
#include "giffmt.h"
#include "error.h"

int      hGifTmp;                      //  Temporary decompressed data file
int      hGifFile;                     //  Actual GIF file
BOOL     bIsBufEmpty;
BOOL     bIsLastRead;
WORD     wInByteCount;
LPSTR    lpInData;   
WORD     CurrChunkSize;
int      BadCodeCount;  //  Must have from decode.c  don't know what for yet.
//HANDLE   hDecodeBuf;


/*---------------------------------------------------------------------------

   PROCEDURE:   GIFReadHeader

-----------------------------------------------------------------------------*/

int FAR PASCAL GIFReadHeader (hFile, lpBitmapInfo, lpInputBuf, lpFileInfo)
int             hFile;
LPBITMAPINFO    lpBitmapInfo;
LPSTR           lpInputBuf;
LPFILEINFO      lpFileInfo;
{
  GIFSCREENDESC         GIFScreenDesc;
  GIFIMAGEDESC          GIFImageDesc; 
  int                   ret_val= 0;                               /* OK */
  LPSTR                 lpTmpBuf;
  LPGIFFIXUP            lpGIFFixup;
  RGBQUAD FAR *         lpRGBQuadPtr;
  WORD                  wBytesPerStrip;
  WORD                  wRowsPerStrip;
  WORD                  wNumStrips;
  WORD                  wBytesPerLastStrip;
  WORD                  wRowsPerLastStrip;
  WORD                  wRowCount;

  if (_llseek (hFile , 0L, 0))
      return (EC_FILEREAD1);

  /*  LOCK INTERNAL POINTERS   */


  lpTmpBuf       = (LPSTR)(  lpInputBuf + 32768L );
  lpGIFFixup     = (LPGIFFIXUP)( (LPSTR) lpFileInfo + sizeof( FILEINFO));
  lpRGBQuadPtr   = (RGBQUAD FAR * )&lpBitmapInfo ->bmiColors[0];

  if(! ReadGIFScreenDesc( hFile,( LPGIFSCREENDESC) &GIFScreenDesc, 
                          lpFileInfo))
    {
      return(  EC_UNGIF );
    }

  ret_val = ReadGIFImageDesc( hFile,( LPGIFIMAGEDESC) &GIFImageDesc, 
                                 lpTmpBuf, lpRGBQuadPtr, lpFileInfo);
  if( ret_val < 0 )
    { 
      return( ret_val);
    }

  /***** Setup buffering stuff for read( from Temp file in this case)  ******/ 

  wRowsPerStrip  =   8192 / lpFileInfo->wBytesPerRow;
  wRowsPerStrip  = 1;
  wBytesPerStrip = wRowsPerStrip * lpFileInfo->wBytesPerRow;

  /*****************************************************
  wNumStrips     = lpFileInfo->wScanHeight / wRowsPerStrip;
  wRowCount      = wNumStrips * wRowsPerStrip;
  ********************************************************/

  wNumStrips     = lpFileInfo->wScanHeight ;
  wRowCount      = lpFileInfo->wScanHeight ;

  /***************************************************************** 
  if( wRowCount < lpFileInfo->wScanHeight)
    {
      wBytesPerLastStrip  =( lpFileInfo->wScanHeight - wRowCount) * 
                             lpFileInfo->wScanWidth;
      wRowsPerLastStrip   =( lpFileInfo->wScanHeight - wRowCount);
      wNumStrips++;
    }
  else
    {
      wBytesPerLastStrip = wBytesPerStrip;
      wRowsPerLastStrip  = wRowsPerStrip;
    }
  *******************************************************************/

  wBytesPerLastStrip = wBytesPerStrip;
  wRowsPerLastStrip  = wRowsPerStrip;
 
  lpFileInfo->wNumStrips         = wNumStrips;
  lpFileInfo->wBytesPerStrip     = wBytesPerStrip;
  lpFileInfo->wRowsPerStrip      = wRowsPerStrip;
  lpFileInfo->wBytesPerLastStrip = wBytesPerLastStrip;
  lpFileInfo->wRowsPerLastStrip  = wRowsPerLastStrip;
 
  /*  Setup DIB( Windows 3.0 style) stuff  
      [THIS WAS IN CALLER - MOVED HERE AS PLANNED] */
  /*  This format needs to flag the last "strip", so mark FALSE here  */

  lpFileInfo->bIsLastStrip    = FALSE;
  lpGIFFixup->bIsFirstStrip   = TRUE;
  lpGIFFixup->dwGIFDataOffset = (DWORD)tell( hFile);

  bIsBufEmpty   = TRUE;
  bIsLastRead   = FALSE;
  wInByteCount  = 2048;
  CurrChunkSize = 2048;

  SetState( IMPORT_CLASS, ICM);
  return( ret_val);
}

/****************************************************************************

   PROCEDURE:      ReadGIFScreenDesc()

*****************************************************************************/

int ReadGIFScreenDesc( hFile, lpGIFScreenDesc, lpFileInfo)
int                 hFile;
LPGIFSCREENDESC     lpGIFScreenDesc;
LPFILEINFO          lpFileInfo;
{
  unsigned      bytes;
  LPGIFFIXUP    lpGIFFixup;

  lpGIFFixup =( LPGIFFIXUP)( (LPSTR) lpFileInfo + 512);

  _llseek( hFile, 6L, 0 );

  bytes = _lread( hFile,( LPSTR) lpGIFScreenDesc, sizeof( GIFSCREENDESC) );
  if (bytes != sizeof(GIFSCREENDESC))
    return (FALSE);

  lpGIFFixup->wBitsPerPixel =( lpGIFScreenDesc->bDescriptor & 0x07) + 1;
  lpGIFFixup->bIsPalette    =( lpGIFScreenDesc->bDescriptor & 0x80);

  return( 1);
} 

/****************************************************************************

   PROCEDURE:      ReadGIFImageDesc

*****************************************************************************/

int ReadGIFImageDesc( hFile, lpGIFImageDesc, lpPaletteBuf, lpRGBQuadPtr, 
                      lpFileInfo) 
int             hFile;
LPGIFIMAGEDESC  lpGIFImageDesc;
LPSTR           lpPaletteBuf;
RGBQUAD FAR *   lpRGBQuadPtr;
LPFILEINFO      lpFileInfo;
{
  int            i;
  LPGIFFIXUP     lpGIFFixup;
  LPSTR          lpTmpPtr;
  unsigned       bytes;
  WORD           wPaletteLen;
  WORD           wPaletteEntries;
  LPDISPINFO    lpDispInfo;


  lpGIFFixup  = (LPGIFFIXUP) ((LPSTR) lpFileInfo + 512);
  lpDispInfo  = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);


  /******************  Read Global Colormap, if it exists *******************/

  if( lpGIFFixup->bIsPalette)
    {
      wPaletteEntries    =( 1 << lpGIFFixup->wBitsPerPixel);
      wPaletteLen       =( 3 * wPaletteEntries);

      _fmemset( (LPSTR) lpRGBQuadPtr, 255, 1024);

      bytes = _lread( hFile,( LPSTR) lpPaletteBuf, wPaletteLen );
      if (bytes != wPaletteLen)
        return (EC_FILEREAD2);

      lpTmpPtr = lpPaletteBuf;
      
      for( i = 0; i < (int)wPaletteEntries; i++)  // Format to WIN 3.0 / ImagePrep convention
        {   
          lpRGBQuadPtr->rgbRed    = *lpTmpPtr++;
          lpRGBQuadPtr->rgbGreen  = *lpTmpPtr++;
          lpRGBQuadPtr->rgbBlue   = *lpTmpPtr++;
          lpRGBQuadPtr++;
        }
    }

  /***********************  Now read image descriptor  **********************/
  
  bytes = _lread( hFile,( LPSTR) lpGIFImageDesc, sizeof( GIFIMAGEDESC) );
  if (bytes != sizeof(GIFIMAGEDESC))
    return (EC_FILEREAD2);

  /******* First test for interlaced.  We do not read interlaced files ******/

  if (lpGIFImageDesc->bImageDescriptor & 0x40)   // Interlaced file
      return (EC_UNGIF);

  lpFileInfo->wBitsPerPixel    = 8;        // Always...
  lpFileInfo->wScanWidth       = lpGIFImageDesc->wImageWidth;
  lpFileInfo->wScanHeight      = lpGIFImageDesc->wImageHeight;


  lpFileInfo->wBytesPerRow     = lpFileInfo->wScanWidth * 
                                  (lpFileInfo->wBitsPerPixel >> 3);

  lpFileInfo -> wPaddedBytesPerRow  = (WORD) ((((DWORD) lpFileInfo -> wScanWidth * (DWORD) lpFileInfo -> wBitsPerPixel) + 31L) / 32L * 4L);

  lpDispInfo->wPaddedScanWidth    = ((lpFileInfo -> wScanWidth * INTERNAL_BITS) + 31) / 32 * 4;

  lpDispInfo->wPaddedBytesPerRow  = lpDispInfo -> wPaddedScanWidth;  // FOR NOW THEY ARE EQUAL BUT IN 24 BIT DISPLAY THIS WILL BE REFINED



  
  /*********  We now point to zero pos (of temp file) for repaints  *********/

  lpFileInfo->dwDataOffset = 0L;  
  return( 0);
} 




