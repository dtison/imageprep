
#include <windows.h>
#include <io.h>
#include <cpi.h>
#include <imgprep.h>
#include <tgafmt.h>
#include <global.h>
#include <error.h>

// New prototypes...

void FAR TGA24To16 (LPSTR, LPSTR, WORD);
void FAR TGA24To32 (LPSTR, LPSTR, WORD);
int WriteTGAHeader (HANDLE, TARGAHEADER FAR *, LPFILEINFO);

LONG FAR PASCAL WriteFile ();

/****************************************************************************

  PROCEDURE:      TGAInitHeader
  DESCRIPTION;    Writes TGAHeader
  DEFINITION:
  MODIFIED:       1/15/90 - David Ison (graft into ImagePrep)
                  6/18/90 - David Ison (Write upside down)
                  7/06/90 - David Ison (Fixed bug in write of last strip)


*****************************************************************************/

int FAR PASCAL TGAInitHeader (hFile, lpBuffer, hIFInfo, hIBInfo,hEFInfo, hEBInfo)
int      hFile;
LPSTR    lpBuffer;
HANDLE   hIFInfo;      
HANDLE   hIBInfo;      
HANDLE   hEFInfo;      
HANDLE   hEBInfo;      
{
  int                    i;
  LPFILEINFO             lpIFileInfo;
  LPBITMAPINFO           lpIBitmapInfo;
  LPFILEINFO             lpEFileInfo;
  LPBITMAPINFO           lpEBitmapInfo;
  LPSTR                  lpPalette;
  LPTGAFIXUP             lpTGAFixup;
  RGBQUAD FAR    *       lpRGBQuadPtr;
  TARGAHEADER            TGAHeader;
  WORD                   wBytesPerStrip;
  WORD                   wRowsPerStrip;
  WORD                   wNumStrips;
  WORD                   wBytesPerLastStrip;
  WORD                   wRowsPerLastStrip;
  WORD                   wRowCount;


  /***  Point to format specific fields right after global file fields  *****/

  if( !( lpIFileInfo = (LPFILEINFO)GlobalLock( hIFInfo )))
    {
      return( EC_MEMORY1 );
    }

  if( !( lpEFileInfo = (LPFILEINFO)GlobalLock( hEFInfo )))
    {
      GlobalUnlock( hIFInfo );
      return( EC_MEMORY1 );
    }

  if( !( lpIBitmapInfo = (LPBITMAPINFO) GlobalLock( hIBInfo )))
    {
      GlobalUnlock( hIFInfo );
      GlobalUnlock( hEFInfo );
      return( EC_MEMORY1 );
    }

  if( !( lpEBitmapInfo = (LPBITMAPINFO) GlobalLock( hEBInfo )))
    {
      GlobalUnlock( hIFInfo );
      GlobalUnlock( hEFInfo );
      GlobalUnlock( hIBInfo );
      return( EC_MEMORY1 );
    }

  lpTGAFixup = (LPTGAFIXUP) ((LPSTR) lpEFileInfo + sizeof (FILEINFO));

  /**************************** Setup buffering stuff ***********************/ 

  wRowsPerStrip   = lpIFileInfo->wRowsPerStrip;
  wBytesPerStrip  = wRowsPerStrip * lpEFileInfo->wScanWidth;
  wNumStrips      = lpEFileInfo->wScanHeight / wRowsPerStrip;
  wRowCount       = wNumStrips * wRowsPerStrip;

  if (wRowCount < lpEFileInfo->wScanHeight)
  {
      wBytesPerLastStrip  = (lpEFileInfo->wScanHeight - wRowCount) * 
                               lpEFileInfo->wScanWidth;
      wRowsPerLastStrip   = (lpEFileInfo->wScanHeight - wRowCount);
      wNumStrips++;
  }
  else
    {
      wBytesPerLastStrip = wBytesPerStrip;
      wRowsPerLastStrip  = wRowsPerStrip;
    }

  lpEFileInfo->wNumStrips      = wNumStrips;
  lpEFileInfo->wBytesPerStrip  = wBytesPerStrip;
  lpEFileInfo->wRowsPerStrip   = wRowsPerStrip;

  lpEFileInfo->wBytesPerLastStrip  = wBytesPerLastStrip;
  lpEFileInfo->wRowsPerLastStrip   = wRowsPerLastStrip;


  /***********************  Create the TGA file header **********************/

  if (! WriteTGAHeader (hFile, (LPTARGAHEADER) &TGAHeader, lpEFileInfo))
    {
      GlobalUnlock( hIFInfo );
      GlobalUnlock( hEFInfo );
      GlobalUnlock( hIBInfo );
      GlobalUnlock( hEBInfo );
      return (EC_FILEWRITE2);
    }

  /***********************  Reformat the colormap for TGA  ******************/

  if( wExportClass > IRGB )  /* ie. = CM */
    {
      lpRGBQuadPtr = (RGBQUAD FAR *)GlobalLock( hExpPalette );

      lpPalette = lpBuffer;

      for (i = 0; i < 256; i++) 
        {
          *lpPalette++ = lpRGBQuadPtr->rgbBlue ;
          *lpPalette++ = lpRGBQuadPtr->rgbGreen ;
          *lpPalette++ = lpRGBQuadPtr->rgbRed ;
          lpRGBQuadPtr++;
        }

      GlobalUnlock( hExpPalette );

    if (WriteFile (hFile, lpBuffer, 768L) != 768L)
    {
          GlobalUnlock( hIFInfo );
          GlobalUnlock( hEFInfo );
          GlobalUnlock( hIBInfo );
          GlobalUnlock( hEBInfo );
          return (EC_FILEWRITE2);
    }
  }



    /*  Now write the file out blank so we can then write it upside down  */


    {
 
        WORD    wBytesPerPixel;
        DWORD   dwImgDataSize; 

//old      wBytesPerPixel  = lpEFileInfo->wBitsPerPixel >> 3;

        wBytesPerPixel  = TGAHeader.PixelDepth >> 3;  // Get actual Bytes / Pixel

        dwImgDataSize   = ((DWORD) lpEFileInfo -> wScanWidth * 
                           (DWORD) lpEFileInfo -> wScanHeight *
                           (DWORD) wBytesPerPixel);

        lpTGAFixup -> dwImgDataPos = tell (hFile);

        {
            DWORD dwSize;

            dwSize = 20480L;
 
            if (! AppendFile (hFile, lpBuffer, dwImgDataSize, dwSize))
                return (EC_FILEWRITE2);

        }

        dwImgDataSize = _llseek (hFile, 0L, 2);

        lpTGAFixup -> dwWriteDataPos = dwImgDataSize - (DWORD) (lpEFileInfo -> wScanWidth * lpEFileInfo -> wRowsPerStrip * wBytesPerPixel);

    }   

  GlobalUnlock (hIFInfo);
  GlobalUnlock (hEFInfo);
  GlobalUnlock (hIBInfo);
  GlobalUnlock (hEBInfo);
  return (0);                                // OK return
}

/*---------------------------------------------------------------------------

  PROCEDURE:      TGAWrConvertData
  DESCRIPTION;    Writes TGA Data
  DEFINITION:
  MODIFIED:       1/15/90 - David Ison (graft into ImagePrep)
                  7/11/90 - Eliminated some overhead (handles)  D. Ison

----------------------------------------------------------------------------*/

int FAR PASCAL TGAWrConvertData (hFile, lppT, lppD, lpEFileInfo, lpBitmapInfo)
int           hFile;
LPSTR FAR *   lppT;
LPSTR FAR *   lppD;
LPFILEINFO    lpEFileInfo;
LPBITMAPINFO  lpBitmapInfo;
{
    int               nRetval = TRUE;
    LPSTR             lpSrc;
    LPSTR             lpTmp;
    LPSTR             TmpPtr;
    LPSTR             SrcPtr;
    WORD              wBytesThisStrip;

    WORD              wBytesThisRow;
    WORD              wPaddedBytesThisRow;
    WORD              wPaddedBytesThisStrip;
    WORD              wRowsThisStrip;
    WORD              rows;
    LPTGAFIXUP        lpTGAFixup;
    
    
    lpSrc = *lppD;
    lpTmp = *lppT;
    
    
    lpTGAFixup = (LPTGAFIXUP) ((LPSTR) lpEFileInfo + sizeof (FILEINFO));
    
    
    wBytesThisRow         = lpEFileInfo->wBytesPerRow;
    wPaddedBytesThisRow   = lpEFileInfo->wPaddedBytesPerRow;
    
    if (lpEFileInfo->bIsLastStrip)
    {
        wPaddedBytesThisStrip = lpEFileInfo->wPaddedBytesPerRow *
                                lpEFileInfo->wRowsPerLastStrip;
        wBytesThisStrip       = lpEFileInfo->wBytesPerRow *
                                lpEFileInfo->wRowsPerLastStrip;
        wRowsThisStrip        = lpEFileInfo->wRowsPerLastStrip;

        lpTGAFixup -> dwWriteDataPos = lpTGAFixup -> dwImgDataPos; 

    }
    else
    {
        wBytesThisStrip       = lpEFileInfo->wBytesPerRow *
                                lpEFileInfo->wRowsPerStrip;
        wPaddedBytesThisStrip = lpEFileInfo->wPaddedBytesPerRow *
                                lpEFileInfo->wRowsPerStrip;
        wRowsThisStrip        = lpEFileInfo->wRowsPerStrip;
    }



    InvertScanlines (lpTmp, 
                     lpSrc, 
                     lpEFileInfo -> wPaddedBytesPerRow,
                     wRowsThisStrip);

    TmpPtr    = lpTmp;  
    lpTmp     = lpSrc;
    lpSrc     = TmpPtr;


    TmpPtr = lpTmp;
    SrcPtr = lpSrc;
    
    for (rows = 0; rows < wRowsThisStrip; rows++)
    {
        lmemcpy (TmpPtr, SrcPtr, wBytesThisRow);
        TmpPtr += wBytesThisRow;
        SrcPtr += wPaddedBytesThisRow;
    }

    TmpPtr    = lpTmp;                    /* Data now in dest so swap */
    lpTmp     = lpSrc;
    lpSrc     = TmpPtr;

    if (wSaveType == XTGA16)
    {
        TGA24To16 (lpSrc, lpTmp, (lpEFileInfo -> wScanWidth * wRowsThisStrip)); 
     
        wBytesThisStrip = ((wBytesThisStrip << 1) / 3);
      
        /*  Correct the dwWriteDataPos in the Fixup structure for handling 16 bit TGA's  */

        TmpPtr    = lpTmp;                    
        lpTmp     = lpSrc;
        lpSrc     = TmpPtr;
    }

    if (wSaveType == XTGA32)
    {
        TGA24To32 (lpSrc, lpTmp, (lpEFileInfo -> wScanWidth * wRowsThisStrip)); 
     
        wBytesThisStrip = (WORD) (((DWORD) wBytesThisStrip << 2) / 3L);
      
        /*  Correct the dwWriteDataPos in the Fixup structure for handling 32 bit TGA's  */

        TmpPtr    = lpTmp;                    
        lpTmp     = lpSrc;
        lpSrc     = TmpPtr;
    }


    _llseek (hFile, lpTGAFixup -> dwWriteDataPos, 0);

    lpTGAFixup -> dwWriteDataPos -= (DWORD) wBytesThisStrip;   

    if (WriteFile (hFile, lpSrc, (LONG) wBytesThisStrip) != (LONG) wBytesThisStrip)
        nRetval = EC_FILEWRITE2;

    return (nRetval);
}

/****************************************************************************

  PROCEDURE:      TGAFixupHeader
  DESCRIPTION;    Finished Header
  DEFINITION:
  MODIFIED:       1/15/90 - David Ison - Graft into Imageprep

*****************************************************************************/

int FAR PASCAL TGAFixupHeader (hFile, lpBuffer, hIFInfo, hIBInfo, hEFInfo, hEBInfo)
int           hFile;
LPSTR           lpBuffer;
HANDLE        hIFInfo;
HANDLE        hEFInfo;
HANDLE        hIBInfo;
HANDLE        hEBInfo;
{
  int         nRetval = FALSE;
  LPFILEINFO  lpEFileInfo;
  LPTGAFIXUP  lpTgaFixup;

  /****  Point to format specific fields right after global file fields  ****/

  if( !( lpEFileInfo = (LPFILEINFO)GlobalLock( hEFInfo )))
    {
      return ( EC_MEMORY1 );
    }

  lpTgaFixup = (TGAFIXUP FAR *) ((LPSTR) lpEFileInfo + sizeof (FILEINFO));

  /************************  Targa header fixup  ****************************/

  GlobalUnlock( hEFInfo );

  if (_llseek (hFile, 0L, 0) == -1)
    {
      return (EC_FILEWRITE1);
    }

 return( 0 );
}

/****************************************************************************

  PROCEDURE:      WriteTGAHeader
  DESCRIPTION;    Writes TGAHeader
  DEFINITION:
  MODIFIED:       1/15/90 - David Ison (graft into ImagePrep)

*****************************************************************************/

int WriteTGAHeader (hFile, lpTGAHeader, lpFileInfo)
HANDLE             hFile; 
LPTARGAHEADER      lpTGAHeader;
LPFILEINFO         lpFileInfo;
{
  WORD bytes;

  lpTGAHeader->IDLength = 0;

  switch (wExportClass){
    case ICM:
    case IGR:
      lpTGAHeader->ColorMapType = 1;
      break;
    default:
      lpTGAHeader->ColorMapType = 0;
      break;
  }

  /************* Uncompressed, colormapped or grayscale image ***************/

  switch( wExportClass )
    {
      case ICM:

      case IGR:
        lpTGAHeader->ImageType = 1;
      break;

      case IRGB:
        lpTGAHeader->ImageType = 2; 
      break;
    }

  lpTGAHeader->CMIndex = 0;       

  switch (wExportClass)
    {
      case ICM:
      case IGR:
        lpTGAHeader->CMLength = 256;
      break;

      default:
        lpTGAHeader->CMLength = 0;
      break;
    }

    switch (wExportClass)
    {
      case ICM:
      case IGR:
        lpTGAHeader->CMEntrySize = 24;  // lpFileInfo->wBitsPerPixel; ??
      break;

      default:
        lpTGAHeader->CMEntrySize = 0;
      break;
    }

  lpTGAHeader->X_Origin    = 0;
  lpTGAHeader->Y_Origin    = 0;
  lpTGAHeader->ImageWidth  = lpFileInfo->wScanWidth;
  lpTGAHeader->ImageHeight = lpFileInfo->wScanHeight;

  switch( wExportClass )
    {
      case ICM:
      case IGR:
        lpTGAHeader->PixelDepth = 8;
      break;

      case IRGB:
        switch( wSaveType )
          {

            case XTGA32:
              lpTGAHeader->PixelDepth  = 32;
              break;

            case XTGA24:
              lpTGAHeader->PixelDepth  = 24;
            break;

            case XTGA16:
              lpTGAHeader->PixelDepth  = 16;
            break;
          }
      break;
    }

    lpTGAHeader->ImageDesc   = 0;   // UPSIDE DOWN
//lpTGAHeader -> ImageDesc   = 0x20;
    
  bytes  = _lwrite (hFile, (LPSTR) lpTGAHeader, sizeof (TARGAHEADER));

  if (FailedLWrite (bytes, sizeof (TARGAHEADER)))
    return (FALSE);

  return (TRUE);
}





void FAR TGA24To32 (lpSource, lpDest, wNumPixels)
LPSTR lpSource;
LPSTR lpDest;
WORD  wNumPixels;
{

    WORD i;

    LPSTR lpSourcePtr;
    LPSTR lpDestPtr;

    lpSourcePtr = lpSource;
    lpDestPtr   = lpDest;

    for (i = 0; i < wNumPixels; i++)
    {
        *lpDestPtr++ = *lpSourcePtr++;
        *lpDestPtr++ = *lpSourcePtr++;
        *lpDestPtr++ = *lpSourcePtr++;

        *lpDestPtr++ = 0;
    }


}
