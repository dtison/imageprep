

#include <windows.h>
#include <cpi.h>
#include <imgprep.h>
#include <tiffmt.h>
#include <error.h>
#include <cpi.h>

/*-------------------------------------------------------------------------------

   PROCEDURE:         TIFRdConvertData
   DESCRIPTION:       Read Tif file, convert to DIB
   DEFINITION:        5.1.2
   START:             David Ison
   MODS:              
                      10/16/90  D. Ison  Add 1 bit read

----------------------------------------------------------------------------*/

extern BOOL  bIsMotorola;

int FAR PASCAL TIFRdConvertData (hFile, lpfpDest, lpfpSource, lpFileInfo, lpBitmapInfo)
int           hFile;
LPSTR FAR *   lpfpDest;     
LPSTR FAR *   lpfpSource; 
LPFILEINFO            lpFileInfo;
LPBITMAPINFO          lpBitmapInfo;
{
    DWORD                 dwTmpDataPos;
    DWORD   FAR *         lpdwStripOffsets;
    DWORD   FAR *         lpdwStripByteCounts;
    int                   nRetval = FALSE;
    LPBITMAPINFOHEADER    lpbmiHeader;
    LPSTR                 lpSource;
    LPSTR                 lpDest;
    LPSTR                 lpTmp;
    LPTIFFIXUP            lpTIFFixup;
    WORD                  wTmpBytesThisStrip;
    WORD                  bytes;
    WORD                  wBytesThisStrip;
    WORD                  wRowsThisStrip;
      
    lpSource = *lpfpDest;
    lpDest   = *lpfpSource;
    
    if (lpFileInfo->bIsLastStrip)
    {
        wBytesThisStrip = lpFileInfo->wBytesPerLastStrip;
        wRowsThisStrip  = lpFileInfo->wRowsPerLastStrip;
    }
    else
    {
        wBytesThisStrip = lpFileInfo->wBytesPerStrip;
        wRowsThisStrip  = lpFileInfo->wRowsPerStrip;
    }
      
    lpbmiHeader = (LPBITMAPINFOHEADER )&lpBitmapInfo -> bmiHeader;
    lpTIFFixup  = (LPTIFFIXUP) ((LPSTR) lpFileInfo + sizeof (FILEINFO));
      
    /*  First read some data into the buffer     */
      
    if (! lpTIFFixup->bIsSingleStrip)
    {
        if (!(lpdwStripOffsets = (DWORD FAR *)GlobalLock (lpTIFFixup->hStripOffsets)))
            return (EC_MEMORY1);
      
        if (!(lpdwStripByteCounts = (DWORD FAR *)GlobalLock(lpTIFFixup->hStripByteCounts)))
        {
            GlobalUnlock (lpTIFFixup->hStripOffsets);
            return (EC_MEMORY1);
        }

        /*   Get strip offset and size based on size of stripbyte and offsets data        */

        if (lpTIFFixup -> wStripOffsetSize == 2)
//          dwTmpDataPos = (DWORD)*((WORD FAR *)lpdwStripOffsets + lpTIFFixup->wCurrentStrip);
            dwTmpDataPos = (DWORD) ByteOrderWord (*((WORD FAR *)lpdwStripOffsets + lpTIFFixup->wCurrentStrip), bIsMotorola);

        else
//          dwTmpDataPos = *(lpdwStripOffsets + lpTIFFixup->wCurrentStrip);
          dwTmpDataPos = ByteOrderLong ((lpdwStripOffsets + lpTIFFixup->wCurrentStrip), bIsMotorola);
    
        if (lpTIFFixup -> wStripByteSize == 2)
//          wTmpBytesThisStrip = *((WORD FAR *)lpdwStripByteCounts + lpTIFFixup->wCurrentStrip);
            wTmpBytesThisStrip = ByteOrderWord (*((WORD FAR *)lpdwStripByteCounts + lpTIFFixup->wCurrentStrip), bIsMotorola);
        else
//          wTmpBytesThisStrip = (WORD)*(lpdwStripByteCounts + lpTIFFixup->wCurrentStrip);
            wTmpBytesThisStrip = ByteOrderWord ((WORD)*(lpdwStripByteCounts + lpTIFFixup->wCurrentStrip), bIsMotorola);

    
        lpTIFFixup -> wCurrentStrip++;
      
        if (_llseek ((int)hFile, dwTmpDataPos, 0) == -1)
        {
            GlobalUnlock (lpTIFFixup->hStripByteCounts) ;
            GlobalUnlock (lpTIFFixup->hStripOffsets);
            return (EC_FILEREAD1);
        }
    }
    else     /*  Read it the "one strip" way  */
        wTmpBytesThisStrip = wBytesThisStrip;
    
    bytes = _lread ((int)hFile, lpSource, (int)wTmpBytesThisStrip);
      
    if (bytes != wTmpBytesThisStrip)
    {
        GlobalUnlock (lpTIFFixup->hStripByteCounts);
        GlobalUnlock (lpTIFFixup->hStripOffsets);
        return (EC_FILEREAD2);
    }


    #ifdef LZW
    /*  Uncompress LZW if applicable  */
    if (lpTIFFixup -> wCompression == TIF_LZW)
    {
        WORD wFlags;
        extern int hLZWFile;  // Needed by LZW decompressor...
        WORD  wNumBytes = wRowsThisStrip * lpFileInfo -> wScanWidth;  // TEMP!!

        if (lpTIFFixup -> bIsFirstStrip)
        {
    
            HANDLE hDecodeBuf;
            HANDLE hLocalBuf;
            PSTR   pLocalBuf;
    
            /*  Allocate space for "stack"  */
    
            wFlags    = 0;
    
            hLocalBuf = LocalAlloc (LHND, ((wNumBytes + 1) << 3));
    
            if (! hLocalBuf)
            {
                GlobalFree (hDecodeBuf);
                return (EC_MEMORY2);
            }
    
            pLocalBuf = LocalLock (hLocalBuf); 
    
            lpTIFFixup->bIsFirstStrip = FALSE;
            lpTIFFixup->hLocalBuf  = hLocalBuf;
            lpTIFFixup->pLocalBuf  = pLocalBuf;

            hLZWFile = hFile;
        }
        else
            if (lpFileInfo -> bIsLastStrip)
                wFlags = 2;
            else
                wFlags = 1;

        DecompressLZW (hFile, (LPSTR) lpTIFFixup->pLocalBuf, lpSource, wNumBytes, wFlags, LZW_TIF);

    }
    #endif


    /*  Test for MONO read  */

    if (lpTIFFixup -> wTiffClass == TIF_BILEVEL)
    {
        LPSTR lpSourcePtr;
        LPSTR lpDestPtr;
        WORD  i, j, k;
        WORD  wSourceIndex = 0;
        WORD  wDestIndex   = 0;
        BYTE  TmpVal;
        LPDISPINFO lpDispInfo;

        lpDispInfo  = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);

        /*  Reformat out to 8 bits per pixel */

        lpSourcePtr = lpSource;
        lpDestPtr   = lpDest;

        for (i = 0; i < wRowsThisStrip; i++)
        {
            for (j = 0; j < lpFileInfo -> wBytesPerRow; j++)
            {

                TmpVal = (BYTE) *lpSourcePtr++;

                for (k = 0; k < 8; k++)
                {
                    *lpDestPtr++ = (char) ((TmpVal & 0x80) >> 7);
                    TmpVal <<= 1;
                }
            }
            wSourceIndex += lpFileInfo -> wBytesPerRow;
            wDestIndex   += lpDispInfo -> wPaddedScanWidth;
            lpSourcePtr   = lpSource + wSourceIndex;
            lpDestPtr     = lpDest   + wDestIndex;

        }
        lpTmp     = lpDest;                       
        lpDest    = lpSource;
        lpSource  = lpTmp;
    }

      
    /*  Reformat data in lpSource to lpDest (RGB to BGR) if not 8 bit */
      
    if (lpFileInfo->wBitsPerPixel == 24)
    {
        RGBToBGR (lpDest, lpSource, wRowsThisStrip, lpFileInfo -> wScanWidth, lpFileInfo -> wBytesPerRow);
      
        lpTmp     = lpDest;      
        lpDest    = lpSource;   
        lpSource  = lpTmp;      
    }
      
    /* Either way we have data in lpSource     */ 
      
    if (lpFileInfo -> wBytesPerRow != lpFileInfo -> wPaddedBytesPerRow)
    {
        PadScanlines (lpDest, 
                      lpSource,   
                      lpFileInfo->wBytesPerRow, 
                      lpFileInfo->wPaddedBytesPerRow, 
                      wRowsThisStrip);
      
        lpTmp     = lpDest;                                      /* save data */
        lpDest    = lpSource;                          /* set Dest to useable */
        lpSource  = lpTmp;                             /* Source has the data */
    }
      
    nRetval = TRUE;
      
    GlobalUnlock (lpTIFFixup->hStripByteCounts);
    GlobalUnlock (lpTIFFixup->hStripOffsets);
      
    /* If this is the last strip, we need to reset the strip counter  */
      
    if (lpFileInfo->bIsLastStrip)
        lpTIFFixup->wCurrentStrip = 0;
      
    *lpfpSource = lpDest;
    *lpfpDest   = lpSource;
      
    return (0);
}     
      
    
    
    
    
    
    
    
    
    
    
    
    
