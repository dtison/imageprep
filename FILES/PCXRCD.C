

#include <windows.h>
#include "imgprep.h"
#include "pcxfmt.h"
#include "error.h"


/*----------------------------------------------------------------------------

   PROCEDURE:         PCXRdConvertData
   DESCRIPTION:       Read PCX file, convert to DIB
   DEFINITION:        5.1.  
   START:             David Ison
   MODS:              1/09/89 Tom Bagford Jr to new spec

  EXIT:                  

----------------------------------------------------------------------------*/

int FAR PASCAL PCXRdConvertData (hFile, lpfpDest, lpfpSource, lpFileInfo, lpBitmapInfo)
int           hFile;
LPSTR FAR *   lpfpDest;                
LPSTR FAR *   lpfpSource;              
LPFILEINFO     lpFileInfo;
LPBITMAPINFO   lpBitmapInfo;
{
    int            nRetval;
    LPPCXFIXUP     lpPCXFixup;
    LPSTR          lpSource;
    LPSTR          lpDest;
    LPSTR          lpTmp;
    WORD           wTempScanRes;
    WORD           wScanWidth;
    WORD           wBytesThisStrip;
    WORD           wRowsThisStrip;
    HANDLE         hDecodeBuf;

  
    lpSource = *lpfpSource;
    lpDest   = *lpfpDest;
   
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
   
    /*  Point to format specific fields right after global file fields  */
  
    lpPCXFixup = (PCXFIXUP FAR *) ((LPSTR) lpFileInfo + sizeof (FILEINFO));
  

    /*  Allocate decode buffer if first strip     */

    if (lpPCXFixup -> bIsFirstStrip)
    {
    
        if (! (hDecodeBuf = GlobalAlloc (GHND, 4096L)))
        {
            return (EC_NOMEM);

        }
        lpPCXFixup -> hDecodeBuf = hDecodeBuf;

        lpPCXFixup -> bIsFirstStrip = FALSE;

    }
  
    wTempScanRes  = (lpFileInfo -> wScanWidth + 1) / 2 * 2;
    wScanWidth    = lpFileInfo->wScanWidth;
  
    if (lpPCXFixup->wPCXImgType == PCX_4BIT)
    {
        /*  This code for 4 plane .PCX 16 color   */
  
        GetPCXLine (hFile, 
                    lpDest,   
                    lpSource,
                    lpPCXFixup, 
                    lpPCXFixup->wBytesPerPlane * 4);
  
        lpTmp     = lpDest;
        lpDest    = lpSource;
        lpSource  = lpTmp;
     
        ToLinear (lpDest, 
                  lpSource, 
                  wScanWidth, 
                  1, 
                  lpPCXFixup->wBytesPerPlane, 
                  0);
      }      
      else

  
          GetPCXLine (hFile, lpDest, lpSource, lpPCXFixup, wTempScanRes);
  
    if (lpFileInfo->bIsLastStrip)
    {
        lpPCXFixup->nBytesRemaining = 0;
        lpPCXFixup->bIsFirstStrip   = TRUE;

        GlobalFree (lpPCXFixup->hDecodeBuf);
    }

    *lpfpSource  = lpSource;
    *lpfpDest    = lpDest;

    nRetval = TRUE;
    return (nRetval);
  }
  



int GetPCXLine (hFile, lpDest, lpInputBuf, lpPCXFixup, wNumBytes) 
int         hFile;
LPSTR       lpInputBuf;
LPSTR       lpDest;
LPPCXFIXUP  lpPCXFixup;
WORD        wNumBytes;
{
    int   total = 0;
    int   i;
    int   inbyte;
    int   incount;
    LPSTR lpDestPtr = lpDest;
    LPSTR lpData;

    lpData = (LPSTR)GlobalLock (lpPCXFixup->hDecodeBuf);
    
    while (total < (int)wNumBytes) 
    {
        if (encget (hFile, &inbyte, &incount, lpInputBuf, lpPCXFixup, lpData) != 0)
            return (0);
        for (i = 0; i < incount; i++)
            *lpDestPtr++ = (BYTE) inbyte;
    
        total += incount;
    }

    GlobalUnlock (lpPCXFixup->hDecodeBuf);
    
    return (1);
}


int encget (hFile, inbyte, incount, lpInputBuf, lpPCXFixup, lpData) 
int              hFile;
int *            inbyte;
int *            incount;
LPSTR            lpInputBuf;
PCXFIXUP FAR *   lpPCXFixup;
LPSTR            lpData;
{
    int          i;
    int           nBytes;
    
    *incount = 1;
    
    if (lpPCXFixup->nBytesRemaining <= 0)
    {
        nBytes = _lread (hFile, (LPSTR)lpData, 4096);
    
        if (nBytes < 0)
        {
            GlobalUnlock (lpPCXFixup->hDecodeBuf);
            return (EC_FILEREAD2);
        }
        lpPCXFixup->nBytesRemaining = nBytes;
        lpPCXFixup->nBytesProcessed = 0;
    }
    
    i = lpData [lpPCXFixup->nBytesProcessed++];
    lpPCXFixup->nBytesRemaining--;
    
    if ((0xC0 & i) == 0xC0)
    {
        *incount = (0x3F & i);
        if (lpPCXFixup->nBytesRemaining <= 0)
        {
            nBytes = _lread (hFile, (LPSTR) lpData, 4096);
            if (nBytes < 0)
            {
                 GlobalUnlock (lpPCXFixup->hDecodeBuf);
                 return (EC_FILEREAD2);
            }  
            lpPCXFixup->nBytesRemaining = nBytes; 
            lpPCXFixup->nBytesProcessed = 0;
        }
    
        i = lpData[ lpPCXFixup->nBytesProcessed++ ];
        lpPCXFixup->nBytesRemaining--;
      }
    
    *inbyte = i;
    
    return (0);
}












































