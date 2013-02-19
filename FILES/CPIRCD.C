/*----------------------------------------------------------------------------

   COPYRIGHT:      Copyright (c) 1990, Computer Presentations, Inc.
                   All rights reserved.

   PROJECT:        Image Prep 3.0

   MODULE:         cpircd.c

   PROCEDURES:     CPIRdConvertData

----------------------------------------------------------------------------*/                                          

#include <windows.h>
#include "cpi.h"
#include "imgprep.h"
#include "error.h"
#include "cpifmt.h"


/*--------------------------------------------------------------------------------

   PROCEDURE:         CPIRdConvertData
   DESCRIPTION:       Read Cpi file, convert to DIB
   DEFINITION:        5.1.1
   START:             David Ison
   MODS:              3.0 TBjr 4/23/90
                      8/13/90  David Ison

----------------------------------------------------------------------------*/

int FAR PASCAL CPIRdConvertData (hFile, lpfpDest, lpfpSource, lpFileInfo, lpBitmapInfo)
int           hFile;
LPSTR FAR *   lpfpDest;                                            
LPSTR FAR *   lpfpSource;                    
LPFILEINFO    lpFileInfo;
LPBITMAPINFO  lpBitmapInfo;
{

    /*  "lpSource" may actually be a misnomer.  Maybe we should say lpTmp or something..
         The important thing is that lpfpDest receives the output data  */

    int                       nRetval = 0;
    LPCPIFIXUP                lpCPIFixup;
    LPDISPINFO                lpDispInfo;
    LPSTR                     lpDest;
    LPSTR                     lpSource;
    LPSTR                     lpTmp;
    WORD                      wBytesThisStrip;
    WORD                      wRowsThisStrip;
    
    
    lpSource = *lpfpDest;
    lpDest   = *lpfpSource;
    
    lpCPIFixup  = (LPCPIFIXUP) ((LPSTR) lpFileInfo + sizeof (FILEINFO));
    lpDispInfo  = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);
    
    if (lpFileInfo -> bIsLastStrip)
    {
        wBytesThisStrip = lpFileInfo -> wBytesPerLastStrip;
        wRowsThisStrip  = lpFileInfo -> wRowsPerLastStrip;
    }
    else
    {
        wBytesThisStrip = lpFileInfo->wBytesPerStrip;
        wRowsThisStrip  = lpFileInfo->wRowsPerStrip;
    }
    
    /*  Read some data into the buffer    */

    if (ReadFile (hFile, lpSource, (LONG) wBytesThisStrip) != (LONG) wBytesThisStrip)
    {
        return (EC_FILEREAD2);
    }
    

    if (lpCPIFixup -> wCPIType == CPIFMT_MONO)
    {

        LPSTR lpSourcePtr;
        LPSTR lpDestPtr;
        WORD  i, j, k;
        WORD  wSourceIndex = 0;
        WORD  wDestIndex   = 0;
        BYTE  TmpVal;

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

    if (lpFileInfo -> wBitsPerPixel == 24)
    {
        if (lpCPIFixup -> bIsLinear)
        {
            #ifdef DHC
            int     i;
            LPSTR   lineIn;
            LPSTR   lineOut;
            #endif
            
            LinesToTriplets (lpDest, lpSource, lpFileInfo -> wScanWidth, wRowsThisStrip);

            #ifdef DHC
            lineIn = lpSource;
            lineOut = lpDest;
  
            for(i = 0; i < wRowsThisStrip; i++)
            {
                int     j;
                LPSTR   srcRed, destRed;
                LPSTR   srcGreen, destGreen;
                LPSTR   srcBlue, destBlue;
        
                srcRed   = lineIn;
                srcGreen = srcRed + lpFileInfo->wScanWidth;
                srcBlue  = srcGreen + lpFileInfo->wScanWidth;

                destRed   = lineOut;
                destGreen = destRed + 1;
                destBlue  = destGreen + 1;
        
                for (j = 0; j < lpFileInfo->wScanWidth; j++) {
                    *destRed = *srcRed++;
                    destRed += 3;
                    *destGreen = *srcGreen++;
                    destGreen += 3;
                    *destBlue = *srcBlue++;
                    destBlue += 3;
                }
        
                lineIn += lpFileInfo->wBytesPerRow;
                lineOut += lpFileInfo->wBytesPerRow;
                
     	    }       
            #endif
	    
            lpTmp     = lpDest;                   
            lpDest    = lpSource;
            lpSource  = lpTmp;
        }
    
        RGBToBGR (lpDest, lpSource, wRowsThisStrip, lpFileInfo -> wScanWidth, lpFileInfo -> wBytesPerRow);
    
        lpTmp     = lpDest;                       
        lpDest    = lpSource;
        lpSource  = lpTmp;

    }
    
    if (lpFileInfo -> wBytesPerRow != lpFileInfo -> wPaddedBytesPerRow) 
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
    
    *lpfpSource  = lpDest;
    *lpfpDest    = lpSource;
    

    return (nRetval);
}   
    
    
    
    
    
    
    
    
    
    
    
