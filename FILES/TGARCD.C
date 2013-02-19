
#include <windows.h>
#include "cpi.h"
#include "imgprep.h"
#include "proto.h"
#include "tgafmt.h"
#include "error.h"
#include <memory.h>

/*----------------------------------------------------------------------------

   PROCEDURE:         TGARdConvertData
   DESCRIPTION:       Read TGA file, convert to DIB
   DEFINITION:        5.1.4
   START:             David Ison
   MODS:              1/10/89 Tom Bagford Jr to new spec
                      3/08/90  David Ison (Fix Targa 16 bug)
----------------------------------------------------------------------------*/

int FAR PASCAL TGARdConvertData (hFile, lpfpDest, lpfpSource, lpFileInfo, lpBitmapInfo)
int              hFile;
LPSTR FAR *      lpfpDest;   
LPSTR FAR *      lpfpSource;  
LPFILEINFO            lpFileInfo;
LPBITMAPINFO          lpBitmapInfo;
{
    DWORD                 dwSeekVal;
    int                   nRetval = FALSE;
    LPBITMAPINFOHEADER    lpbmiHeader;
    LPSTR                 lpSource;
    LPSTR                 lpDest;
    LPSTR                 lpTmp;
    LPTGAFIXUP            lpTGAFixup;
    WORD                  bytes;
    WORD                  wBytesThisStrip;
    WORD                  wRowsThisStrip;
    WORD                  wReadBytesThisStrip;
    int                   hReadFile = hFile;
    
    lpSource = *lpfpDest;
    lpDest   = *lpfpSource;
    

    lpbmiHeader = (LPBITMAPINFOHEADER)&lpBitmapInfo->bmiHeader;
    lpTGAFixup  = (LPTGAFIXUP) ((LPSTR) lpFileInfo + sizeof( FILEINFO));
    
    if (lpFileInfo->bIsLastStrip)
    {
        wBytesThisStrip = lpFileInfo->wBytesPerLastStrip;
        wRowsThisStrip  = lpFileInfo->wRowsPerLastStrip;
        wReadBytesThisStrip = lpTGAFixup -> wInBytesPerLastStrip;
    }
    else
    {
        wBytesThisStrip = lpFileInfo->wBytesPerStrip;
        wRowsThisStrip  = lpFileInfo->wRowsPerStrip;
        wReadBytesThisStrip = lpTGAFixup -> wInBytesPerStrip;
    }

    if (lpTGAFixup -> bIsRLE)
        hReadFile = lpTGAFixup -> hDecompressedFile;

    /*  First seek to position to read from and adjust Next Data Offset */
    
    if (lpTGAFixup -> bIsInverted)
        lpTGAFixup -> dwNextDataOffset -= (DWORD) wReadBytesThisStrip;
    else
        lpTGAFixup -> dwNextDataOffset += (DWORD) lpTGAFixup -> wInBytesPerStrip;
    
    dwSeekVal = lpTGAFixup -> dwNextDataOffset;
    
    if (_llseek (hReadFile, dwSeekVal, 0) != (LONG)dwSeekVal)
        return (EC_FILEREAD2);
    
    
    /*  Next read the data into the buffer                  */
    
    bytes = _lread (hReadFile, lpSource, (int) wReadBytesThisStrip);
    
    if (FailedLRead (bytes, wReadBytesThisStrip))
        return (EC_FILEREAD2);
    
    /*  If 16 bit, we must reformat to 24 bit                 */
    
    if (lpTGAFixup->bIsTGA16)
    {
        TGA16To24 (lpDest, lpSource, lpFileInfo->wScanWidth, wRowsThisStrip);
    
        lpTmp     = lpDest;
        lpDest    = lpSource;
        lpSource  = lpTmp;
    }
    if (lpTGAFixup->bIsTGA32)
    {
        TGA32To24 (lpDest, lpSource, lpFileInfo->wScanWidth, wRowsThisStrip);
    
        lpTmp     = lpDest;
        lpDest    = lpSource;
        lpSource  = lpTmp;
    }
    
    if (lpFileInfo->wBytesPerRow != lpFileInfo-> wPaddedBytesPerRow)
    {
        PadScanlines (lpDest, 
                      lpSource, 
                      lpFileInfo->wBytesPerRow, 
                      lpFileInfo->wPaddedBytesPerRow, 
                      wRowsThisStrip);
    
        lpTmp     = lpDest;
        lpDest    = lpSource;
        lpSource  = lpTmp;
    }
    
    /*  Finally, if data needs to be inverted, invert  */
    
    if (lpTGAFixup -> bIsInverted)
    {
        InvertScanlines (lpDest, lpSource, lpFileInfo -> wPaddedBytesPerRow, 
                          wRowsThisStrip);
    
        lpTmp     = lpDest;
        lpDest    = lpSource;
        lpSource  = lpTmp;
    }
    
    *lpfpSource  = lpDest;
    *lpfpDest    = lpSource;
    
    if (lpFileInfo -> bIsLastStrip)
    {
        lpFileInfo -> dwDataOffset      =             
        lpTGAFixup -> dwNextDataOffset  = FindTGAData (hReadFile, lpFileInfo, lpTGAFixup -> wHeaderSize);
        if (lpTGAFixup -> bIsRLE)
        {
            _lclose (lpTGAFixup -> hDecompressedFile);
            lpTGAFixup -> hDecompressedFile = 0;
        }
    }
    
    return (0);
}   


void FAR TGA16To24 (lpDest, lpSource, wNumPixels, wRows)   
LPSTR lpDest;
LPSTR lpSource; 
WORD  wNumPixels;
WORD  wRows;
{
    BYTE          bRed;
    BYTE          bGreen;
    BYTE          bBlue;
    LPSTR         lpOutptr; 
    WORD          i;
    WORD          j;
    WORD          wRGBVal;
    WORD          t1;
    WORD          t2;
    WORD          t3; 
    WORD  FAR *   lpInptr;
    
    lpOutptr  = lpDest;
    lpInptr   = (WORD FAR *) lpSource;
    
    for (i = 0; i < wRows; i++)
    {
        for (j = 0; j < wNumPixels; j++) 
        {
            wRGBVal = *lpInptr;
    
            t1      = (wRGBVal & 0x7C00);
            t2      = (wRGBVal & 0x03E0);
            t3      = (wRGBVal & 0x001F);
    
            bRed     = (BYTE) (((t1 >> 10) << 3));
            bGreen   = (BYTE) (((t2 >>  5) << 3));
            bBlue    = (BYTE) (t3 << 3);  
    
            *lpOutptr++ = bBlue;
            *lpOutptr++ = bGreen;
            *lpOutptr++ = bRed;
    
            lpInptr++;
        }   
    }
}   
    
    

void FAR TGA32To24 (lpDest, lpSource, wScanWidth, wRowsThisStrip)
LPSTR lpDest;
LPSTR lpSource;
WORD  wScanWidth;
WORD  wRowsThisStrip;
{
    WORD i, j;

    LPSTR lpSourcePtr;
    LPSTR lpDestPtr;

    lpSourcePtr = lpSource;
    lpDestPtr   = lpDest;

    for (i = 0; i < wRowsThisStrip; i++)
        for (j = 0; j < wScanWidth; j++)
        {
            *lpDestPtr++ = *lpSourcePtr++;
            *lpDestPtr++ = *lpSourcePtr++;
            *lpDestPtr++ = *lpSourcePtr++;

            *lpSourcePtr++;
        }
}

