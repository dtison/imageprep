#include <windows.h>
#include <memory.h>
#include <io.h>
#include <cpi.h>
#include <imgprep.h>
#include <tiffmt.h>
#include <global.h>
#include <error.h>

LPINT  int_ptr;

/*---------------------------------------------------------------------------

  PROCEDURE:      TIFInitHeader
  DESCRIPTION;    Writes TifHeader
  DEFINITION:
  MODIFIED:       1/15/90 - David Ison (graft into ImagePrep)

----------------------------------------------------------------------------*/

int FAR PASCAL TIFInitHeader (hFile, lpBuffer, hIFInfo, hIBInfo,hEFInfo, hEBInfo)
int      hFile;
LPSTR    lpBuffer;
HANDLE   hIFInfo;      
HANDLE   hIBInfo;      
HANDLE   hEFInfo;      
HANDLE   hEBInfo;      
{
    DWORD                  dwCurrStripOffset;
    DWORD                  dwStripListSize;
    DWORD FAR *            dwLongPtr; 
    int                    i;
    LPBITMAPINFO           lpIBitmapInfo;
    LPBITMAPINFO           lpEBitmapInfo;
    LPFILEINFO             lpIFileInfo;
    LPFILEINFO             lpEFileInfo;
    LPINT                  wIntPtr;
    LPSTR                  lpTBuf;
    LPSTR                  lpExpPalette;
    LPTIFFHEADER           TifHdrPtr;
    LPTIFFIXUP             lpTIFFixup;
    RGBQUAD FAR *          lpRGBQuadPtr;
    WORD                   wBytesPerStrip;
    WORD                   wRowsPerStrip;
    WORD                   wNumStrips;
    WORD                   wBytesPerLastStrip;
    WORD                   wRowsPerLastStrip;
    WORD                   wRowCount;
    WORD                   bytes;
    WORD                   wNumberTags;   
    WORD                   wWriteBytesPerRow;
    WORD                   wWriteBytesPerStrip;
    WORD                    wPixels;
    WORD                    wDPI;
    
    if (! (lpIFileInfo = (LPFILEINFO) GlobalLock (hIFInfo)))
        return (EC_MEMORY1);
    
    if (! (lpEFileInfo = (LPFILEINFO) GlobalLock (hEFInfo)))
    {
        GlobalUnlock (hIFInfo);
        return (EC_MEMORY1);
    }
    
    if (! (lpIBitmapInfo = (LPBITMAPINFO) GlobalLock (hIBInfo)))
    {
        GlobalUnlock (hIFInfo);
        GlobalUnlock (hEFInfo);
        return (EC_MEMORY1);
    }
    
    if (! (lpEBitmapInfo = (LPBITMAPINFO) GlobalLock (hEBInfo)))
    {
        GlobalUnlock (hIBInfo);
        GlobalUnlock (hIFInfo);
        GlobalUnlock (hEFInfo);
        return (EC_MEMORY1);
    }
    
    /*  Point to format specific fields right after global file fields     */
    
    lpTIFFixup = (TIFFIXUP FAR *) ((LPSTR) lpEFileInfo + sizeof (FILEINFO));
    
    /*  Setup buffering stuff  */
    
    wRowsPerStrip   = lpIFileInfo->wRowsPerStrip;
    
    if (wRowsPerStrip > lpIFileInfo->wScanHeight)
        wRowsPerStrip = lpIFileInfo->wScanHeight;
    
    wBytesPerStrip  = wRowsPerStrip * lpEFileInfo -> wScanWidth;

    if (wExportClass == IMON)
    {
        wWriteBytesPerRow = (lpEFileInfo->wScanWidth >> 3);
    
        if (lpEFileInfo->wScanWidth % 8)
            wWriteBytesPerRow++;
    
        wWriteBytesPerStrip = wRowsPerStrip * wWriteBytesPerRow; 
    
        lpTIFFixup->wWriteBytesPerRow   = wWriteBytesPerRow;
        lpTIFFixup->wWriteBytesPerStrip = wWriteBytesPerStrip;
    }
    
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
    
    lpTIFFixup->dwRowsPerStrip       = (DWORD) wRowsPerStrip;
    lpTIFFixup->dwNumStrips          = (DWORD) wNumStrips;
    
    if (bIsCapturing)
    {
          lpTBuf = (LPSTR) (lpBuffer + 4096L);
    }
    else
        lpTBuf = (LPSTR) ((BYTE huge *) lpBuffer + BUFOFFSET);
    
    /*  Create the TIF file header  */
    
    TifHdrPtr                 = (TIFFHEADER FAR *) lpTBuf;
    TifHdrPtr->ByteOrder      = 0x4949;
    TifHdrPtr->VersionNumber  = 0x002A;
    TifHdrPtr->IFDOffset      = 0L;
    
    bytes  = _lwrite (hFile, lpTBuf, sizeof (TIFFHEADER));
    
    if (FailedLWrite (bytes, sizeof (TIFFHEADER)))
    {
        GlobalUnlock (hEBInfo);
        GlobalUnlock (hIBInfo);
        GlobalUnlock (hIFInfo);
        GlobalUnlock (hEFInfo);
        return (EC_FILEWRITE2);
    }
    
    /* Write out the IFD stuff as blanks for now  */
    
    lpTIFFixup->dwIFDOffset = tell (hFile); 
    
    if (wExportClass != ICM)
        wNumberTags = 14;
    else
        wNumberTags = 15;  
    
    _fmemset (lpTBuf, 0, (wNumberTags * sizeof (IFDSTRUCT) + 2 + 4));
    
    bytes = _lwrite (hFile, lpTBuf, ((wNumberTags * sizeof (IFDSTRUCT)) + 2 + 4));
    
    if (FailedLWrite (bytes, wNumberTags * sizeof (IFDSTRUCT) + 2 + 4))
    {
        GlobalUnlock (hEBInfo);
        GlobalUnlock (hIBInfo);
        GlobalUnlock (hIFInfo);
        GlobalUnlock (hEFInfo);
        return (EC_FILEWRITE2);
    }
    
    /*  First write BitsPerSample values (for 24 bit TIFF)  */
    
    if (wExportClass == IRGB)
    {
        lpTIFFixup->dwBitsPerSample = tell (hFile);
        wIntPtr = (int FAR *) lpTBuf;
        *wIntPtr++ = 8;
        *wIntPtr++ = 8;
        *wIntPtr++ = 8;
    
        bytes = _lwrite (hFile, lpTBuf, 6);
    
        if (FailedLWrite (bytes, 6))
        {
            GlobalUnlock (hEBInfo);
            GlobalUnlock (hIBInfo);
            GlobalUnlock (hIFInfo);
            GlobalUnlock (hEFInfo);
            return (EC_FILEWRITE2);
        }
    }
    
    /*  Put the software message, and X and Y resolution */
    
    lpTIFFixup->dwSoftwareOffset = tell (hFile);
    
    bytes = _lwrite (hFile, (LPSTR) "ImagePrep     ",  15);
    
    if (FailedLWrite (bytes, 15))
    {
        GlobalUnlock (hEBInfo);
        GlobalUnlock (hIBInfo);
        GlobalUnlock (hIFInfo);
        GlobalUnlock (hEFInfo);
        return (EC_FILEWRITE2);
    }
    
    dwLongPtr = (DWORD FAR *) lpTBuf;


    /*  Kludge the stupid X Res and Y Res  */

    wPixels = max (lpEFileInfo -> wScanWidth, lpEFileInfo -> wScanHeight);
    wDPI = (wPixels >> 2);    // Divide by 4, meaning 4 inches
    if (wDPI > 300)
        wDPI = 300;

    if (wDPI < 30)
        wDPI = 30;
    
    *dwLongPtr++ = (DWORD) wDPI;
    *dwLongPtr++ = 1L;
    *dwLongPtr++ = (DWORD) wDPI;
    *dwLongPtr++ = 1L;


    lpTIFFixup->dwXResOffset = tell (hFile);
    
    bytes = _lwrite (hFile, lpTBuf, 16); 
    
    if (FailedLWrite (bytes, 16))
    {
        GlobalUnlock (hEBInfo);
        GlobalUnlock (hIBInfo);
        GlobalUnlock (hIFInfo);
        GlobalUnlock (hEFInfo);
        return (EC_FILEWRITE2);
    }
    
    /*  Format the windows style palette to Tiff if one exists */
    
    if (wExportClass == ICM)                   
    {
        if (! (lpExpPalette = (LPSTR) GlobalLock (hExpPalette)))
        {
            GlobalUnlock (hEBInfo);
            GlobalUnlock (hIBInfo);
            GlobalUnlock (hIFInfo);
            GlobalUnlock (hEFInfo);
            return (EC_MEMORY1);
        }
          
        lpTIFFixup->dwColorMapOffset = tell (hFile);
    
        wIntPtr = (LPINT) lpTBuf;
        lpRGBQuadPtr = (RGBQUAD FAR *)lpExpPalette;
    
        for (i = 0; i < 256; i++)    //  Do Reds
        {
            *wIntPtr = (BYTE) lpRGBQuadPtr->rgbRed;
            *wIntPtr <<= 8;
    
            wIntPtr++;
            lpRGBQuadPtr++;
        }
    
        bytes = _lwrite (hFile, lpTBuf, (256 * sizeof (int)));
    
        if (FailedLWrite (bytes, (256 * sizeof (int))))
        {
            GlobalUnlock (hEBInfo);
            GlobalUnlock (hIBInfo);
            GlobalUnlock (hIFInfo);
            GlobalUnlock (hEFInfo);
            GlobalUnlock (hExpPalette);
            return (EC_FILEWRITE2);
        }
      
        wIntPtr = (LPINT) lpTBuf;
    
        lpRGBQuadPtr = (RGBQUAD FAR *)lpExpPalette;
    
        for (i = 0; i < 256; i++)    //  Do Greens
        {
            *wIntPtr = (BYTE) lpRGBQuadPtr->rgbGreen;
            *wIntPtr <<= 8;
    
            wIntPtr++;
            lpRGBQuadPtr++;
        }
    
        bytes = _lwrite (hFile, lpTBuf, (256 * sizeof (int)));
        if (FailedLWrite (bytes, (256 * sizeof (int))))
        {
            GlobalUnlock (hEBInfo);
            GlobalUnlock (hIBInfo);
            GlobalUnlock (hIFInfo);
            GlobalUnlock (hEFInfo);
            GlobalUnlock (hExpPalette);
            return (EC_FILEWRITE2);
        }
    
        wIntPtr = (LPINT) lpTBuf;
        lpRGBQuadPtr = (RGBQUAD FAR *)lpExpPalette;
    
        for (i = 0; i < 256; i++)    //  Do Blues
        {
            *wIntPtr = (BYTE) lpRGBQuadPtr->rgbBlue ;
            *wIntPtr <<= 8;
    
            wIntPtr++;
            lpRGBQuadPtr++;
        }
    
        bytes = _lwrite (hFile, lpTBuf, (256 * sizeof (int)));
        if (FailedLWrite (bytes, (256 * sizeof (int))))
        {
            GlobalUnlock (hEBInfo);
            GlobalUnlock (hIBInfo);
            GlobalUnlock (hIFInfo);
            GlobalUnlock (hEFInfo);
            GlobalUnlock (hExpPalette);
            return (EC_FILEWRITE2);
        }
        GlobalUnlock (hExpPalette);
    }
    
    /*  Pre-calculate strip offsets and save in file  */
    
    dwStripListSize   = (DWORD) (lpEFileInfo->wNumStrips * sizeof (DWORD));
    dwCurrStripOffset = tell (hFile);
    dwCurrStripOffset += (2 * dwStripListSize);
    
    lpTIFFixup->dwDataOffset = dwCurrStripOffset;
    dwLongPtr = (DWORD FAR *) lpTBuf;
    
    for (i = 0; i < (int)(lpEFileInfo->wNumStrips); i++) 
    {
        *dwLongPtr++ = dwCurrStripOffset;
        if (wExportClass == IMON)
            dwCurrStripOffset += (DWORD)  lpTIFFixup->wWriteBytesPerStrip;
        else
            dwCurrStripOffset += (DWORD)  lpEFileInfo->wBytesPerRow *
                                          lpEFileInfo->wRowsPerStrip;
    }
    
    lpTIFFixup->dwStripOffsets = tell (hFile);
    
    bytes = _lwrite (hFile, lpTBuf, (WORD) dwStripListSize);
    
    if (FailedLWrite (bytes, (WORD) dwStripListSize))
    {
            GlobalUnlock (hEBInfo);
            GlobalUnlock (hIBInfo);
            GlobalUnlock (hIFInfo);
            GlobalUnlock (hEFInfo);
            return (EC_FILEWRITE2);
    }
    
    dwLongPtr = (DWORD FAR *) lpTBuf;
    
    if (wExportClass == IMON)
    {
        for (i = 0; i < (int)(lpEFileInfo->wNumStrips - 1); i++) 
            *dwLongPtr++ = (DWORD)  lpTIFFixup->wWriteBytesPerStrip;
    
        *dwLongPtr = (DWORD) lpTIFFixup->wWriteBytesPerRow * 
                             lpEFileInfo->wRowsPerLastStrip;
    }
    else
    {
        for (i = 0; i < (int)(lpEFileInfo->wNumStrips); i++) 
        {
            if (i < ((int)lpEFileInfo->wNumStrips - 1))
                *dwLongPtr++ = (DWORD)  lpEFileInfo->wBytesPerRow *
                                        lpEFileInfo->wRowsPerStrip;
            else    
                *dwLongPtr++ = (DWORD)  lpEFileInfo->wBytesPerRow *
                                        lpEFileInfo->wRowsPerLastStrip;
        }
    }
    
    lpTIFFixup->dwStripByteCounts = tell (hFile);
    
    bytes = _lwrite (hFile, lpTBuf, (WORD) dwStripListSize);
    
    if (FailedLWrite (bytes, (WORD) dwStripListSize))
    {
        GlobalUnlock (hEBInfo);
        GlobalUnlock (hIBInfo);
        GlobalUnlock (hIFInfo);
        GlobalUnlock (hEFInfo);
        return (EC_FILEWRITE2);
    }
    
    /*  Another Kludge for 1 strip StripByteCounts and StripOffsets  */


    if (lpTIFFixup -> dwNumStrips == 1L)
    {
        lpTIFFixup -> dwStripByteCounts =  (DWORD)  lpEFileInfo->wBytesPerRow * lpEFileInfo->wRowsPerLastStrip;
        lpTIFFixup -> dwStripOffsets    =  (DWORD)  tell (hFile);
    }
    
    /*  Finish up other "fixup" fields  */
    
    switch (wExportClass)
    { 
        case IRGB:
    
          lpTIFFixup->wBitsPerSampleCount   = 3;
          lpTIFFixup->dwPhotometric         = 2L;
          lpTIFFixup->dwSamplesPerPixel     = 3L;
    
        break;
    
        case ICM:
    
          lpTIFFixup->dwBitsPerSample       = 8L;
          lpTIFFixup->wBitsPerSampleCount   = 1;
          lpTIFFixup->dwPhotometric         = 3L;
          lpTIFFixup->dwSamplesPerPixel     = 1;
    
        break;
    
        case IGR:
    
          lpTIFFixup->dwBitsPerSample       = 8L;
          lpTIFFixup->wBitsPerSampleCount   = 1;
          lpTIFFixup->dwPhotometric         = 1L;
          lpTIFFixup->dwSamplesPerPixel     = 1;
    
        break;
    
        case IMON:
    
          lpTIFFixup->dwBitsPerSample       = 1L;
          lpTIFFixup->wBitsPerSampleCount   = 1;
          lpTIFFixup->dwPhotometric         = 1L;
          lpTIFFixup->dwSamplesPerPixel     = 1;
    
        break;
    }
    
    GlobalUnlock (hEBInfo);
    GlobalUnlock (hIBInfo);
    GlobalUnlock (hIFInfo);
    GlobalUnlock (hEFInfo);
    GlobalUnlock (hExpPalette);
    
    return (0);  
}   
    

int FAR PASCAL TIFWrConvertData (hFile, lppT, lppD, lpEFileInfo, lpBitmapInfo) 
int           hFile;
LPSTR FAR *   lppT;
LPSTR FAR *   lppD;
LPFILEINFO    lpEFileInfo;
LPBITMAPINFO  lpBitmapInfo;
{
    int           nRetval = TRUE;
    LPTIFFIXUP    lpTIFFixup;
    LPSTR         lpSrc;
    LPSTR         lpTmp;
    LPSTR         lpTmpPtr;
    LPSTR         lpSrcPtr;
    WORD          wBytesThisStrip;
    WORD          bytes;
    WORD          wBytesThisRow;
    WORD          wPaddedBytesThisRow;
    WORD          wPaddedBytesThisStrip;
    WORD          wRowsThisStrip;
    WORD          wRows;
    
    lpSrc = *lppD;
    lpTmp = *lppT;
    
    
    lpTIFFixup = (TIFFIXUP FAR *) ((LPSTR) lpEFileInfo + sizeof (FILEINFO));
    
    wBytesThisRow         = lpEFileInfo->wBytesPerRow;
    wPaddedBytesThisRow   = lpEFileInfo->wPaddedBytesPerRow;
    
    if (lpEFileInfo->bIsLastStrip)
    {
        wBytesThisStrip       = lpEFileInfo->wBytesPerRow *
                                lpEFileInfo->wRowsPerLastStrip;
        wPaddedBytesThisStrip = lpEFileInfo->wPaddedBytesPerRow *
                                lpEFileInfo->wRowsPerLastStrip;
        wRowsThisStrip        = lpEFileInfo->wRowsPerLastStrip;
    }
    else
    { 
        wBytesThisStrip       = lpEFileInfo->wBytesPerRow *
                                lpEFileInfo->wRowsPerStrip;
        wPaddedBytesThisStrip = lpEFileInfo->wPaddedBytesPerRow *
                                lpEFileInfo->wRowsPerStrip;
        wRowsThisStrip        = lpEFileInfo->wRowsPerStrip;
    }
    

    if (wExportClass != IMON)
    if (wBytesThisRow != wPaddedBytesThisRow)           // Clipping necessary Mod 1-15-90
    {
        lpTmpPtr = lpTmp;
        lpSrcPtr = lpSrc;
        for (wRows = 0; wRows < wRowsThisStrip; wRows++)   // Clip each scanline
        {
            _fmemcpy (lpTmpPtr, lpSrcPtr, wBytesThisRow);
            lpTmpPtr += (long)wBytesThisRow;
            lpSrcPtr += (long)wPaddedBytesThisRow;
        }
        lpTmpPtr  = lpTmp;                            /* Data now in dest so swap */
        lpTmp     = lpSrc;
        lpSrc     = lpTmpPtr;
    }
    
    
    if (wExportClass == IRGB)
    {
        RGBToBGR  (lpTmp, lpSrc, wRowsThisStrip, lpEFileInfo->wScanWidth,wBytesThisRow);
        lpTmpPtr  = lpTmp;                    
        lpTmp     = lpSrc;
        lpSrc     = lpTmpPtr;
    }
    
    if (wExportClass == IGR && wDither == INO16)
    {
        WORD j;

        lpTmpPtr = lpTmp;
        lpSrcPtr = lpSrc;
        for (wRows = 0; wRows < wRowsThisStrip; wRows++)   
        {
            for (j = 0; j < lpEFileInfo -> wBytesPerRow; j++)
                *lpTmpPtr++ = (BYTE) (*lpSrcPtr++ << 4);

        }
        lpTmpPtr  = lpTmp;
        lpTmp     = lpSrc;
        lpSrc     = lpTmpPtr;
    }








    if (wExportClass == IMON)
    {
        /*  Reformat from DIB to 1 bit  */
    
//      cm2bilvl (lpSrc,lpTmp,wPaddedBytesThisRow,lpTIFFixup->wWriteBytesPerRow,wRowsThisStrip);

        { 

            WORD i;
            WORD j;
            WORD k;
            LPSTR lpTmpSrc;
            LPSTR lpTmpDest;
            BYTE  TmpVal;

            for (i = 0; i < wRowsThisStrip; i++)
            {
                lpTmpSrc  = lpSrc + (i * wPaddedBytesThisRow);
                lpTmpDest = lpTmp + (i * lpTIFFixup -> wWriteBytesPerRow);

                for (j = 0; j < lpTIFFixup->wWriteBytesPerRow; j++)
                {
                    TmpVal = 0;
                    for (k = 0; k < 8; k++)
                    {

                        TmpVal <<= 1;
                        if (*lpTmpSrc++)
                            TmpVal |= 1;
                    }
                    *lpTmpDest++ = TmpVal;
                }
            }
        }
    
        lpTmpPtr  = lpTmp;                    
        lpTmp     = lpSrc;
        lpSrc     = lpTmpPtr;
    
        bytes = _lwrite  (hFile, 
                         (LPSTR) lpSrc, 
                         lpTIFFixup->wWriteBytesPerStrip);
    
        if (FailedLWrite (bytes, lpTIFFixup->wWriteBytesPerStrip))
            return (EC_FILEWRITE2);
    
        return (nRetval);
      }
    
    bytes = _lwrite (hFile, (LPSTR) lpSrc, wBytesThisStrip);
    
    
    if (FailedLWrite (bytes, wBytesThisStrip))
        return (EC_FILEWRITE2);
    
    return (nRetval);
}   
    
/*---------------------------------------------------------------------------
    
    PROCEDURE:      TIFFixupHeader
    DESCRIPTION;    Finished Header
    DEFINITION:
    MODIFIED:       1/14/90 - David Ison - Graft into Imageprep
    
----------------------------------------------------------------------------*/

int FAR PASCAL  TIFFixupHeader  (hFile, lpBuffer, hIFInfo, hIBInfo, hEFInfo, 
                                  hEBInfo )
int      hFile;
LPSTR    lpBuffer;
HANDLE   hIFInfo;
HANDLE   hEFInfo;
HANDLE   hIBInfo;
HANDLE   hEBInfo;
{
  int               i;
  int               nRetval = FALSE;
  LPIFDSTRUCT       IFD_EntryPtr;
  LPFILEINFO        lpEFileInfo;
  LPSTR             lpPtr;
  LPSTR             lpTBuf;
  LPTIFFHEADER      TifHdrPtr;
  LPTIFFIXUP        lpTIFFixup;
  WORD              bytes;
  WORD              wNumberTags;   

  /****  Point to format specific fields right after global file fields  ****/

  if( !( lpEFileInfo = (LPFILEINFO)GlobalLock( hEFInfo )))
    {
      return( EC_MEMORY1 );
    }

  lpTIFFixup = (TIFFIXUP FAR *) ((LPSTR) lpEFileInfo + sizeof (FILEINFO));

  if (wExportClass != ICM )
    wNumberTags = 14;
  else
    wNumberTags = 15;  

  /****************************  Tiff header  *******************************/

  if (_llseek (hFile, 0L, 0) == -1)
    {
      GlobalUnlock( hEFInfo );  
      return (EC_FILEWRITE1);
    }

  if( bIsCapturing )
    {
        lpTBuf = (LPSTR)( lpBuffer + 4096L );
    }
  else
    lpTBuf = (LPSTR) ((BYTE huge *) lpBuffer + BUFOFFSET) ;

  bytes = _lread (hFile, lpTBuf, sizeof (TIFFHEADER));
 
  if (FailedLRead (bytes, sizeof (TIFFHEADER)))
    {
      GlobalUnlock( hEFInfo );  
      return (EC_FILEREAD2);
    }

  TifHdrPtr = (LPTIFFHEADER) lpTBuf;
  TifHdrPtr->IFDOffset = lpTIFFixup->dwIFDOffset;

  if (_llseek (hFile, 0L, 0) == -1)
    {
      GlobalUnlock( hEFInfo );  
      return (EC_FILEWRITE1);
    }

  bytes = _lwrite (hFile, lpTBuf, sizeof (TIFFHEADER));
  
  if (FailedLWrite (bytes, sizeof (TIFFHEADER)))
    {
      GlobalUnlock( hEFInfo );  
      return (EC_FILEWRITE2);
    }

  /******************************** Tags ************************************/

  if (_llseek (hFile, lpTIFFixup->dwIFDOffset, 0 ) == -1)
    {
      GlobalUnlock( hEFInfo );  
      return (EC_FILEWRITE1);
    }

  /***************************  Write the # of IFD Entries ******************/

  lpPtr      = lpTBuf;
  int_ptr    = (LPINT) lpPtr;
  *(int_ptr) = wNumberTags;

  lpPtr++;         /******************* Pass the integer ********************/
  lpPtr++;

  /**************  Do the TIF IFD Entries (Just after Tif Header)  **********/

  IFD_EntryPtr              = (IFDSTRUCT far *) lpPtr;
  IFD_EntryPtr->wTag        = 254;                      /*  NewSubFileType  */
  IFD_EntryPtr->nType       = TIF_LONG;
  IFD_EntryPtr->Length      = 1;
  IFD_EntryPtr->ValueOffset = 0;

  lpPtr += sizeof (IFDSTRUCT);

  IFD_EntryPtr = (LPIFDSTRUCT) lpPtr;
  IFD_EntryPtr->wTag        = 256;                            /* ImageWidth */
  IFD_EntryPtr->nType       = TIF_LONG;
  IFD_EntryPtr->Length      = 1;

  if( wExportType == IMON )
//    IFD_EntryPtr->ValueOffset = lpEFileInfo->wPaddedScanWidth;
    IFD_EntryPtr->ValueOffset = lpEFileInfo->wScanWidth;
  else
    IFD_EntryPtr->ValueOffset = lpEFileInfo->wScanWidth;
  lpPtr += sizeof (IFDSTRUCT);

  IFD_EntryPtr = (LPIFDSTRUCT) lpPtr;
  IFD_EntryPtr->wTag        = 257;                           /* ImageLength */
  IFD_EntryPtr->nType       = TIF_LONG;
  IFD_EntryPtr->Length      = 1;
  IFD_EntryPtr->ValueOffset = lpEFileInfo->wScanHeight;
  lpPtr += sizeof (IFDSTRUCT);

  IFD_EntryPtr = (LPIFDSTRUCT) lpPtr;
  IFD_EntryPtr->wTag        = 258;                       /*  BitsPerSample  */
  IFD_EntryPtr->nType       = TIF_SHORT;
  IFD_EntryPtr->Length      = lpTIFFixup->wBitsPerSampleCount;
  IFD_EntryPtr->ValueOffset = lpTIFFixup->dwBitsPerSample;
  lpPtr += sizeof (IFDSTRUCT);

  IFD_EntryPtr = (LPIFDSTRUCT) lpPtr;
  IFD_EntryPtr->wTag        = 259;                       /*  Compression    */
  IFD_EntryPtr->nType       = TIF_SHORT;
  IFD_EntryPtr->Length      = 1;
  IFD_EntryPtr->ValueOffset = 1;
  lpPtr += sizeof (IFDSTRUCT);

  IFD_EntryPtr = (LPIFDSTRUCT) lpPtr;
  IFD_EntryPtr->wTag        = 262;                       /*  Photometric    */
  IFD_EntryPtr->nType       = TIF_SHORT;
  IFD_EntryPtr->Length      = 1;
  IFD_EntryPtr->ValueOffset = lpTIFFixup->dwPhotometric;
  lpPtr += sizeof (IFDSTRUCT);

  IFD_EntryPtr = (LPIFDSTRUCT) lpPtr;
  IFD_EntryPtr->wTag        = 273;                /*  StripOffsets   */
  IFD_EntryPtr->nType       = TIF_LONG;
  IFD_EntryPtr->Length      = lpTIFFixup->dwNumStrips; 
  IFD_EntryPtr->ValueOffset = lpTIFFixup->dwStripOffsets;
  lpPtr += sizeof (IFDSTRUCT);

  IFD_EntryPtr = (LPIFDSTRUCT) lpPtr;
  IFD_EntryPtr->wTag        = 277;                /*  SamplesPerPixel  */
  IFD_EntryPtr->nType       = TIF_SHORT;
  IFD_EntryPtr->Length      = 1;
  IFD_EntryPtr->ValueOffset = lpTIFFixup->dwSamplesPerPixel;
  lpPtr += sizeof (IFDSTRUCT);

  IFD_EntryPtr = (LPIFDSTRUCT) lpPtr;
  IFD_EntryPtr->wTag        = 278;                /*  RowsPerStrip   */
  IFD_EntryPtr->nType       = TIF_LONG;
  IFD_EntryPtr->Length      = 1;
  IFD_EntryPtr->ValueOffset = lpTIFFixup->dwRowsPerStrip;
  lpPtr += sizeof (IFDSTRUCT);

  IFD_EntryPtr = (LPIFDSTRUCT) lpPtr;
  IFD_EntryPtr->wTag        = 279;                /*  StripByteCounts*/
  IFD_EntryPtr->nType       = TIF_LONG;
  IFD_EntryPtr->Length      = lpTIFFixup->dwNumStrips; 
  IFD_EntryPtr->ValueOffset = lpTIFFixup->dwStripByteCounts;
  lpPtr += sizeof (IFDSTRUCT);

  IFD_EntryPtr = (LPIFDSTRUCT) lpPtr;
  IFD_EntryPtr->wTag        = 282;                /*  XResolution    */
  IFD_EntryPtr->nType       = TIF_RATIONAL;
  IFD_EntryPtr->Length      = 1;
  IFD_EntryPtr->ValueOffset = lpTIFFixup->dwXResOffset;
  lpPtr += sizeof (IFDSTRUCT);

  IFD_EntryPtr = (LPIFDSTRUCT) lpPtr;
  IFD_EntryPtr->wTag        = 283;                /*  YResolution    */
  IFD_EntryPtr->nType       = TIF_RATIONAL;
  IFD_EntryPtr->Length      = 1;
  IFD_EntryPtr->ValueOffset = lpTIFFixup->dwXResOffset + 8;
  lpPtr += sizeof (IFDSTRUCT);

  IFD_EntryPtr = (LPIFDSTRUCT) lpPtr;
  IFD_EntryPtr->wTag        = 296;                /*  ResolutionUnit */
  IFD_EntryPtr->nType       = TIF_SHORT; 
  IFD_EntryPtr->Length      = 1;
  IFD_EntryPtr->ValueOffset = 2;
  lpPtr += sizeof (IFDSTRUCT);

  IFD_EntryPtr = (LPIFDSTRUCT) lpPtr;
  IFD_EntryPtr->wTag        = 305;                /*  Software       */
  IFD_EntryPtr->nType       = TIF_ASCII; 
  IFD_EntryPtr->Length      = 1;
  IFD_EntryPtr->ValueOffset = lpTIFFixup->dwSoftwareOffset;
  lpPtr += sizeof (IFDSTRUCT);

  if (wExportClass == ICM)
    {
      IFD_EntryPtr = (LPIFDSTRUCT) lpPtr;
      IFD_EntryPtr->wTag        = 320;              /*  Colormap       */
      IFD_EntryPtr->nType       = TIF_SHORT; 
      IFD_EntryPtr->Length      = 768;
      IFD_EntryPtr->ValueOffset = lpTIFFixup->dwColorMapOffset;
    }

  bytes = _lwrite( hFile, lpTBuf, ((wNumberTags * sizeof (IFDSTRUCT)) + 2) );
  
  if (FailedLWrite (bytes, wNumberTags * sizeof (IFDSTRUCT) + 2))
  {
      GlobalUnlock(hEFInfo);  
      return (EC_FILEWRITE2);
  }

  /***********************  Write the 4 bytes of zero ***********************/

  for (i = 0; i < 4; i++)
     *(lpTBuf + i) = 0;

  bytes = _lwrite (hFile, lpTBuf, 4);
  
  if (FailedLWrite (bytes, 4))
  {
      GlobalUnlock( hEFInfo );  
      return (EC_FILEWRITE2);
  }

  GlobalUnlock( hEFInfo );  
  return( 0 );
}







