/****************************************************************************
**  COPYRIGHT:      Copyright (c) 1990, Computer Presentations, Inc.
**                  All rights reserved.
**  PROJECT:        Image Prep 3.0
**  MODULE:         cpircd.c
**  PROCEDURES:     DIBRdConvertData
**  DATE:           05/20/90
**  DEVELOPER:      David W. Mize
*****************************************************************************/
#include <windows.h>
#include <stdio.h>
#include <memory.h>
#include "imgprep.h"
#include "dibfmt.h"
#include "error.h"

/****************************************************************************
**  PROCEDURE:      DIBRdConvertData
**  DESCRIPTION:    Read Cpi file, convert to DIB
**  DEFINITION:     5.1.1
**  DEVELOPER:      David W. Mize
*****************************************************************************/

int FAR PASCAL DIBRdConvertData( 
  int           hFile,
  LPSTR FAR *   lpfpDest, 
  LPSTR FAR *   lpfpSource,
  LPFILEINFO    lpFileInfo,
  LPBITMAPINFO  lpBitmapInfo)
{
  LPFILEINFO    lpReadFileInfo;
  LPSTR         lpDest;
  LPSTR         lpSource;
  LPSTR         lpTmp;
  WORD          wBytes;
  WORD          wBytesThisStrip;
  WORD          wRowsThisStrip;
  int           nReturn = 0;



  lpReadFileInfo = (LPFILEINFO)((LPSTR)lpFileInfo + sizeof(FILEINFO));
  /*
  **  Set data pointers
  */
  lpSource    = *lpfpDest;
  lpDest      = *lpfpSource;
  /*
  **  Determine size of strip to read
  */
  if (lpFileInfo->bIsLastStrip){
    wBytesThisStrip = lpReadFileInfo->wBytesPerLastStrip;
    wRowsThisStrip  = lpReadFileInfo->wRowsPerLastStrip;
  }
  else{
    wBytesThisStrip = lpReadFileInfo->wBytesPerStrip;
    wRowsThisStrip  = lpReadFileInfo->wRowsPerStrip;
  } 

  /*
  **  Rewind to next buffer and read data. Then reset file pointer to 
  **  beginning of read. 
  */
  _llseek (hFile, - (long)wBytesThisStrip, SEEK_CUR);
  wBytes = _lread (hFile, lpSource, (int)wBytesThisStrip);
  _llseek (hFile, - (long)wBytesThisStrip, SEEK_CUR);
  if (wBytes != wBytesThisStrip){
    return (EC_FILEREAD2);
  }
  /*
  **  Invert input data
  */
  InvertDIB (lpDest, lpSource, lpReadFileInfo->wPaddedBytesPerRow, 
             wRowsThisStrip);
  lpTmp       = lpDest;
  lpDest      = lpSource;
  lpSource    = lpTmp;
  /*
  **  Reformat data for color resolution of display
  */
  switch (lpReadFileInfo->wBitsPerPixel){
    case 24:
    case 8:
      /*
      **  Pad scanlines if necessary for image prep
      */
      if( lpReadFileInfo->wPaddedBytesPerRow != 
        lpFileInfo->wPaddedBytesPerRow){
        LPBYTE  lpIn  = (LPBYTE)lpSource;
        LPBYTE  lpOut = (LPBYTE)lpDest;
        int     i;
        /*
        **
        */
        for (i = 0; i < (int)wRowsThisStrip; i++){
          _fmemcpy (lpOut, lpIn, lpReadFileInfo->wBytesPerRow);
          lpIn  += lpReadFileInfo->wPaddedBytesPerRow;
          lpOut += lpFileInfo->wPaddedBytesPerRow;
        }
        lpTmp     = lpDest;
        lpDest    = lpSource;
        lpSource  = lpTmp;
      }
      break;
    case 4:{
        LPBYTE  lpIn;
        LPBYTE  lpOut;
        int     i;
        int     j;
        /*
        **  Convert 4-bit to 8-bit data
        */
        for (i = 0; i < (int)wRowsThisStrip; i++){
          lpIn  = (LPBYTE)lpSource + (i * lpReadFileInfo->wPaddedBytesPerRow);
          lpOut = (LPBYTE)lpDest + (i * lpFileInfo->wPaddedBytesPerRow);
          for (j = 0; j < (int)lpReadFileInfo->wBytesPerRow; j++){
            *lpOut++ = (BYTE)((int)*lpIn >> 0x04);
            *lpOut++ = (BYTE)((int)*lpIn & 0x0F);
            lpIn++;
          }
        }
        lpTmp       = lpDest;
        lpDest      = lpSource;
        lpSource    = lpTmp;
      }
      break;
    case 1:{
        LPBYTE  lpIn  = (LPBYTE)lpSource;
        LPBYTE  lpOut = (LPBYTE)lpDest;
        int     i;
        int     j;
        int     k;
        /*
        **  Convert 1-bit to 8-bit data 
        */
        for (i = 0; i < (int)wRowsThisStrip; i++){
          lpIn  = (LPBYTE)lpSource + (i * lpReadFileInfo->wPaddedBytesPerRow);
          lpOut = (LPBYTE)lpDest   + (i * lpFileInfo->wPaddedBytesPerRow);
          for (j = 0; j < (int)lpReadFileInfo->wBytesPerRow; j++){
            for (k = 7; k >= 0; k--){
              *lpOut++ = (BYTE)(((int)*lpIn >> k) & 0x01);
            }
            lpIn++;
          }
        }
        lpTmp       = lpDest;
        lpDest      = lpSource;
        lpSource    = lpTmp;
      }
      break;
  }
  /*
  **  Reset data pointers  
  */
  *lpfpSource    = lpDest;
  *lpfpDest     = lpSource;      
  return (nReturn);
}

/****************************************************************************
**  PROCEDURE:     InvertDIB
*****************************************************************************/

BOOL FAR PASCAL InvertDIB(
  LPSTR lpDest, 
  LPSTR lpSource,
  WORD  wBytesPerRow, 
  WORD  wRowsThisStrip)
{
  int   i;
  LPSTR lpSourceRow;
  LPSTR lpDestRow;
  /*
  **
  */
  lpDestRow   = lpDest;
  lpSourceRow = lpSource + (wBytesPerRow * (wRowsThisStrip - 1));
  for (i = 0; i < (int)wRowsThisStrip; i++){
    _fmemcpy (lpDestRow, lpSourceRow, wBytesPerRow);
    lpDestRow   += wBytesPerRow;
    lpSourceRow -= wBytesPerRow;
  }
  return (TRUE);
}

