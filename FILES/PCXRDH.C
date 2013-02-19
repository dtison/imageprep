
#include <windows.h>
#include <io.h>
#include <memory.h>
#include "cpi.h"
#include "imgprep.h"
#include "proto.h"
#include "pcxfmt.h"
#include "error.h"


int FAR PASCAL PCXReadHeader (hFile, lpBitmapInfo, lpInputBuf, lpFileInfo)
int             hFile;
LPBITMAPINFO    lpBitmapInfo;
LPSTR           lpInputBuf;
LPFILEINFO      lpFileInfo;
{
    int             nRetval = 0;
    LPSTR           lpTmpBuf;
    PCXFIXUP FAR *  lpPCXFixup;
    RGBQUAD FAR *   lpRGBQuadPtr;
    WORD            wBytesPerStrip;
    WORD            wRowsPerStrip;
    WORD            wNumStrips;
    WORD            wBytesPerLastStrip;
    WORD            wRowsPerLastStrip;
    WORD            wRowCount;
    
    
    lpTmpBuf = (LPSTR) ((BYTE huge *) lpInputBuf + BUFOFFSET);
    
    /*     Point to format specific fields right after global file fields     */
    
    lpPCXFixup = (PCXFIXUP FAR *) ((LPSTR) lpFileInfo + sizeof (FILEINFO));

    lpPCXFixup -> nBytesRemaining = 0;              // (Used for PCX decode buffering)
    lpPCXFixup -> bIsFirstStrip   = TRUE;          


    lpRGBQuadPtr = &lpBitmapInfo->bmiColors [0];
    
    nRetval = ReadPCXHeader (hFile, lpTmpBuf, lpRGBQuadPtr, lpFileInfo);

    if (nRetval < 0)
    {
        return (nRetval);
    }
    
    /*  Setup buffering stuff for read */
    
    wRowsPerStrip  = 1;
    
    wBytesPerStrip    = wRowsPerStrip * lpFileInfo -> wScanWidth;
    wNumStrips        = lpFileInfo -> wScanHeight / wRowsPerStrip;
    wRowCount         = wNumStrips * wRowsPerStrip;
    
    if (wRowCount < lpFileInfo->wScanHeight)
    {
        wBytesPerLastStrip  = (lpFileInfo->wScanHeight - wRowCount) * 
                               lpFileInfo->wScanWidth;
        wRowsPerLastStrip   = (lpFileInfo->wScanHeight - wRowCount);
        wNumStrips++;
    }
    else
    {
        wBytesPerLastStrip = wBytesPerStrip;
        wRowsPerLastStrip  = wRowsPerStrip;
    }
    
    if  (lpFileInfo->wScanWidth == 0 || lpFileInfo->wScanHeight == 0)
    {
    
        return (EC_INVALIDIMAGE);
    }
    
    lpFileInfo->wNumStrips            = wNumStrips;
    lpFileInfo->wBytesPerStrip        = wBytesPerStrip;
    lpFileInfo->wRowsPerStrip         = wRowsPerStrip;
    lpFileInfo->wBytesPerLastStrip    = wBytesPerLastStrip;
    lpFileInfo->wRowsPerLastStrip     = wRowsPerLastStrip;
    
    /*  This format needs to flag the last "strip", so mark FALSE here  */
    
    lpFileInfo->bIsLastStrip = FALSE;
    
    SetState (IMPORT_CLASS, ICM);
    return (0);
}


/*---------------------------------------------------------------------------  

   PROCEDURE:        PCXReadHeader
   DESCRIPTION:      File "can opener" for ImgPrep (PCX files)
   DEFINITION:       5.4.4
   START:            David Ison
   MODS:             1/16/90  David Ison        (To new spec)

----------------------------------------------------------------------------*/

int ReadPCXHeader (hFile, lpPaletteBuf, lpRGBQuadPtr, lpFileInfo)
int             hFile;
LPSTR           lpPaletteBuf;
RGBQUAD FAR *   lpRGBQuadPtr;
LPFILEINFO      lpFileInfo;
{
    int           i;
    LPPCXFIXUP    lpPCXFixup;
    LPSTR         TmpPtr;
    PCXHEADER     PCXHeader;
    WORD          wBytes;
    LPDISPINFO    lpDispInfo;


    lpPCXFixup = (PCXFIXUP FAR *) ((LPSTR) lpFileInfo + sizeof (FILEINFO));
    lpDispInfo  = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);
    
    TmpPtr = (LPSTR) &PCXHeader;
    
    if (_llseek (hFile , 0L, 0))
        return (EC_FILEREAD1);
    
    wBytes = _lread ((int) hFile, (LPSTR) &PCXHeader, sizeof (PCXHEADER));
    
    if (FailedLRead (wBytes, sizeof (PCXHEADER)))
        return (EC_FILEREAD2);

    switch (PCXHeader.BitsPerPixel)
    {
        case 8:
        break;

        case 1:
            if (PCXHeader.Nplanes == 4)
            break;

        default:
            return (EC_SAMPIX);
    }
    
    
    lpFileInfo -> wBitsPerPixel   = 8;  // ALWAYS use 8 bit DIB
                  
    lpFileInfo -> wScanWidth      = PCXHeader.X2 - PCXHeader.X1 + 1;
    lpFileInfo -> wScanHeight     = PCXHeader.Y2 - PCXHeader.Y1 + 1;
    lpPCXFixup -> wBytesPerPlane  = PCXHeader.bplin;
    lpPCXFixup -> wPCXImgType     = ((PCXHeader.BitsPerPixel == 8) 
                                    ? PCX_8BIT : PCX_4BIT);
    
    /*  DWORD Padded DIBs                          */
    
    lpFileInfo->wBytesPerRow        = lpFileInfo->wScanWidth * (lpFileInfo->wBitsPerPixel >> 3);

    lpFileInfo -> wPaddedBytesPerRow  = (WORD) ((((DWORD) lpFileInfo -> wScanWidth * (DWORD) lpFileInfo -> wBitsPerPixel) + 31L) / 32L * 4L);

    lpDispInfo->wPaddedScanWidth    = ((lpFileInfo -> wScanWidth * INTERNAL_BITS) + 31) / 32 * 4;

    lpDispInfo->wPaddedBytesPerRow  = lpDispInfo -> wPaddedScanWidth;  // FOR NOW THEY ARE EQUAL BUT IN 24 BIT DISPLAY THIS WILL BE REFINED
    
    
    if (PCXHeader.BitsPerPixel == 8)  // 8 Bit .PCX file
    {
        /*  Read and reformat palette for 8 bit  */
    
        _llseek (hFile, -768L, 2 );
    
        wBytes = _lread (hFile, lpPaletteBuf, 768 );
        if ( FailedLRead( wBytes, 768 ))
            return (EC_FILEREAD2);
    
        TmpPtr = lpPaletteBuf;
        for (i = 0; i < 256; i++)  // THIS IS 8 BIT only!
        {   
            lpRGBQuadPtr->rgbRed    = *TmpPtr++;
            lpRGBQuadPtr->rgbGreen  = *TmpPtr++;
            lpRGBQuadPtr->rgbBlue   = *TmpPtr++;   
    
            lpRGBQuadPtr++;
        }
    }
    else
    {
          /*      Read and reformat palette for 4 bit EGA / VGA file         */
    
        if (_llseek (hFile, 16L, 0) == -1)
            return (EC_FILEREAD1);

        wBytes = _lread (hFile, lpPaletteBuf, 48);
        if (FailedLRead (wBytes, 48))
            return (EC_FILEREAD1);
    
          TmpPtr = lpPaletteBuf;
          for (i = 0; i < 16; i++)  // THIS IS 4 BIT
          {   
              lpRGBQuadPtr->rgbRed    = *TmpPtr++;
              lpRGBQuadPtr->rgbGreen  = *TmpPtr++;
              lpRGBQuadPtr->rgbBlue   = *TmpPtr++;   
    
              lpRGBQuadPtr++;
          }
    
          lmemset ((LPSTR) lpRGBQuadPtr, 255, 960);
      }
    
      /*  Point to image data                            */
    
      _llseek (hFile, (long) sizeof (PCXHEADER), 0);
      lpFileInfo -> dwDataOffset = (DWORD) tell (hFile);
    
      return (0);
}   




