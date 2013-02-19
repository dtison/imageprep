/****************************************************************************

   COPYRIGHT:      Copyright (c) 1990, Computer Presentations, Inc.
                   All rights reserved.

   PROJECT:        ImagePrep 3.0

   MODULE:         gifrcd.c

   PROCEDURES:     

*****************************************************************************/                                          
#include <windows.h>
#include "imgprep.h"
#include "giffmt.h"
#include "error.h"
#include <cpi.h>


/*  (This is GIF format specific stuff for reads)  */


/*  These two are defined global purely to save pushing on the stack all
    the time, saving a great deal of performance  */


/*---------------------------------------------------------------------------

      PROCEDURE:     GIFRdConvertData

----------------------------------------------------------------------------*/

int FAR PASCAL GIFRdConvertData (hFile, lpfpDest, lpfpSource, lpFileInfo, lpBitmapInfo)
int           hFile;
LPSTR FAR *   lpfpDest;   
LPSTR FAR *   lpfpSource;  
LPFILEINFO    lpFileInfo;
LPBITMAPINFO  lpBitmapInfo;
{
    BOOL                 bIsOverflow = FALSE;
    int                  nRetval = 0;
    LPGIFFIXUP           lpGIFFixup;
    LPSTR                lpDest;
    LPSTR                lpSource;
    LPSTR                lpTmp;
    WORD                 wBytesThisStrip;
    WORD                 wRowsThisStrip;
    WORD                 wFlags;
    
    
    lpGIFFixup = (LPGIFFIXUP) ((LPSTR) lpFileInfo + sizeof (FILEINFO));
      
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
    
    lpSource = *lpfpDest;
    lpDest   = *lpfpSource;
    
    if (lpGIFFixup -> bIsFirstStrip)
    {
    
    
    
        HANDLE hDecodeBuf;
        HANDLE hLocalBuf;
        PSTR   pLocalBuf;
    
    
        /*  Allocate space for "stack"  */
    
        wFlags      = 0;
    
        hLocalBuf = LocalAlloc( LHND,( (lpFileInfo->wScanWidth + 1) << 1));
    
        if( ! hLocalBuf)
          {
            GlobalFree (hDecodeBuf);
            return (EC_MEMORY2);
          }
    
        pLocalBuf = LocalLock ( hLocalBuf); 
    
        if( _llseek( hFile , lpGIFFixup->dwGIFDataOffset, 0) != 
                     (LONG)lpGIFFixup->dwGIFDataOffset)
          {
           GlobalFree (hDecodeBuf);
           LocalUnlock (hLocalBuf);
           LocalFree (hLocalBuf);
           return (EC_FILEREAD1);
          }
    
        lpGIFFixup->bIsFirstStrip = FALSE;
        lpGIFFixup->hLocalBuf  = hLocalBuf;
        lpGIFFixup->pLocalBuf  = pLocalBuf;
    
    
    
      }
    else
      if (lpFileInfo -> bIsLastStrip)
        wFlags = 2;
      else
        wFlags = 1;
    
    
    
    DecompressLZW (hFile, (LPSTR) lpGIFFixup->pLocalBuf, lpSource, 
               lpFileInfo->wScanWidth, wFlags, LZW_GIF);
    
    
    
    if( lpFileInfo->wBytesPerRow != lpFileInfo->wPaddedBytesPerRow) 
      {
        PadScanlines( lpDest, lpSource, lpFileInfo->wBytesPerRow, 
                      lpFileInfo->wPaddedBytesPerRow, wRowsThisStrip);
    
        lpTmp     = lpDest;
        lpDest    = lpSource;
        lpSource  = lpTmp;
      }
    
    *lpfpSource = lpSource;
    *lpfpDest   = lpDest;
    
    nRetval = TRUE;
    
    if (lpFileInfo->bIsLastStrip){
      LocalUnlock( lpGIFFixup->hLocalBuf);
      LocalFree  ( lpGIFFixup->hLocalBuf);   
      lpGIFFixup->bIsFirstStrip = TRUE;
    
    }

    *lpfpSource  = lpDest;
    *lpfpDest    = lpSource;


    return (nRetval);
}   



/****************************************************************************

   PROCEDURE:   GIFLineOut

*****************************************************************************/

int GIFLineOut( pBuffer, wBytesPerRow, lpDest)
PSTR   pBuffer;
WORD   wBytesPerRow;
LPSTR  lpDest;
{

  lmemcpy( lpDest,( LPSTR) pBuffer, wBytesPerRow);

  return( TRUE);
}




