/****************************************************************************
**  COPYRIGHT:      Copyright (c) 1990, Computer Presentations, Inc.
**                  All rights reserved.
**  PROJECT:        Image Prep 3.0
**  MODULE:         dibrdh.c
**  PROCEDURES:     DIBReadHeader()   5.4.1
*****************************************************************************/                                          
#include <windows.h>
#include <stdio.h>
#include "imgprep.h"
#include "proto.h"
#include "dibfmt.h"
#include "global.h"
#include "error.h"


/****************************************************************************
**  PROCEDURE:         DIBReadHeader
**  DESCRIPTION:       Message box procedure for ImgPrep
*****************************************************************************/

int FAR PASCAL DIBReadHeader( 
  int                 hFile,
  LPBITMAPINFO        lpBitmapInfo,
  LPSTR               lpInputBuf, 
  LPFILEINFO          lpFileInfo)
{
  BITMAPFILEHEADER    BitmapFileHeader;
  LPSTR               lpTmpBuf;
  LPFILEINFO          lpReadFileInfo;
  RGBQUAD FAR *       lpRGBQuadPtr;



  lpReadFileInfo = (LPFILEINFO)((LPSTR)lpFileInfo + sizeof(FILEINFO));
  lpTmpBuf       = (LPSTR)((BYTE huge *) lpInputBuf + BUFOFFSET);
  lpRGBQuadPtr   = (RGBQUAD FAR *)&lpBitmapInfo->bmiColors[0];
  /*
  **  Read DIB header and palette
  */
  if (!ReadDIBHeader (hFile, (LPBITMAPFILEHEADER)&BitmapFileHeader,
    (LPBITMAPINFOHEADER)&lpBitmapInfo->bmiHeader, lpTmpBuf, lpRGBQuadPtr, 
    lpFileInfo)){
    return (EC_FILEREAD2);
  }
  return (0);
}

/****************************************************************************
**  PROCEDURE:        ReadDIBHeader
**  DESCRIPTION:      Read the file header for a DIB
**  DEFINITION:       5.4.1.2
**  START:            David W. Mize 05-02-90
**  MODS:             
*****************************************************************************/

BOOL NEAR PASCAL ReadDIBHeader(
  int                 hFile,
  LPBITMAPFILEHEADER  lpBitmapFileHeader,
  LPBITMAPINFOHEADER  lpBitmapInfoHeader,
  LPSTR               lpPalette,
  RGBQUAD FAR *       lpRGBQuad,
  LPFILEINFO          lpFileInfo)
{
  int                 i;
  LPFILEINFO          lpReadFileInfo;
  RGBTRIPLE FAR *     lpRGBTriple;
  WORD                wPaletteSize;
  WORD                wDIBType;
  WORD                wRowsPerStrip;
  WORD                wNumStrips;
  WORD                wRowsPerLastStrip;
  WORD                wRowCount;
  LPDISPINFO          lpDispInfo;


  /*
  **  Setup internal data
  */
  lpReadFileInfo  = (LPFILEINFO)((LPSTR)lpFileInfo + sizeof(FILEINFO));
  lpDispInfo      = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);
  /*
  **  Start at beginning of file
  */
  if (_llseek ((int)hFile, 0, SEEK_SET) != 0)
    return (FALSE);
  /*
  **  Read BITMAPFILEHEADER and check for valid DIB
  */
  if (_lread ((int)hFile, (LPSTR)lpBitmapFileHeader, 
    (int)sizeof(BITMAPFILEHEADER)) != sizeof(BITMAPFILEHEADER))
    return (FALSE);
  if (lpBitmapFileHeader->bfType != 0x4D42)       /* "BM" */
    return (FALSE);
  /*
  **  Assume Windows 3.0 DIB and read BITMAPINFOHEADER. Find out whether 
  **  it is a Windows 3.0 or Presentation Manager DIB by checking first
  **  field, biSize to determine if it is the size of a BITMAPINFOHEADER
  **  or the size of a BITMAPCOREHEADER. Header fields are different sizes 
  **  and palettes are formatted differently. Windows 3.0 DIBs have RGBQUAD 
  **  palettes, while PM DIBs have RGBTRIPLE palettes.
  */
  if (_lread ((int)hFile, (LPSTR)lpBitmapInfoHeader, 
    sizeof(BITMAPINFOHEADER)) != sizeof(BITMAPINFOHEADER))
    return (FALSE);
  /*
  **  Determine DIB type
  */
  wDIBType = (WORD)(lpBitmapInfoHeader->biSize == sizeof(BITMAPINFOHEADER) ?
    WNDIB : PMDIB);
  /*
  **  Set information in FILEINFO structure based on DIB type
  */
  switch (wDIBType){
    case WNDIB:                                   /***  Windows 3.0 DIB ***/
      lpReadFileInfo->wScanWidth    = (WORD)lpBitmapInfoHeader->biWidth;
      lpReadFileInfo->wScanHeight   = (WORD)lpBitmapInfoHeader->biHeight;
      lpReadFileInfo->wBitsPerPixel = lpBitmapInfoHeader->biBitCount;
      break;
    case PMDIB:                         /***  Presentation Manager DIB ****/
      lpReadFileInfo->wScanWidth    = 
        (WORD)((LPBITMAPCOREHEADER)lpBitmapInfoHeader)->bcWidth;
      lpReadFileInfo->wScanHeight   =
        (WORD)((LPBITMAPCOREHEADER)lpBitmapInfoHeader)->bcHeight;
      lpReadFileInfo->wBitsPerPixel =
        ((LPBITMAPCOREHEADER)lpBitmapInfoHeader)->bcBitCount;
      break;
    default:
      return (FALSE);
  }


  /*
  **  Finish up read data
  */
  lpReadFileInfo->wPaddedScanWidth    = (WORD)lpReadFileInfo->wScanWidth;
  lpReadFileInfo->wBytesPerRow        = (WORD) ((((DWORD) lpReadFileInfo -> wScanWidth * (DWORD) lpReadFileInfo -> wBitsPerPixel) + 31L) / 32L * 4L);

  lpReadFileInfo->wPaddedBytesPerRow  = (WORD)lpReadFileInfo->wBytesPerRow;   
  /*
  **  Set imageprep file information
  */
  lpFileInfo->wScanWidth          = lpReadFileInfo->wScanWidth;
  lpFileInfo->wScanHeight         = lpReadFileInfo->wScanHeight;
  lpFileInfo->wBitsPerPixel       = (lpReadFileInfo->wBitsPerPixel < 8 ? 8 :
                                    lpReadFileInfo->wBitsPerPixel);

  lpFileInfo->wBytesPerRow        = lpFileInfo->wScanWidth *
                                    (lpFileInfo->wBitsPerPixel >> 3);

  lpFileInfo->wPaddedBytesPerRow  = ((lpFileInfo -> wScanWidth * lpFileInfo -> wBitsPerPixel) + 31) / 32 * 4;  // DWORD ALIGN MAN!!!!!!!!

  lpDispInfo->wPaddedScanWidth    = ((lpFileInfo -> wScanWidth * INTERNAL_BITS) + 31) / 32 * 4;

  lpDispInfo->wPaddedBytesPerRow  = lpDispInfo -> wPaddedScanWidth;  // FOR NOW THEY ARE EQUAL BUT IN 24 BIT DISPLAY THIS WILL BE REFINED

  /*
  **  Set import state  
  */
  SetState (IMPORT_CLASS, lpReadFileInfo->wBitsPerPixel == 24 ? IRGB :
    lpReadFileInfo->wBitsPerPixel == 1 ? IMON : ICM);

    /*  Setup buffering stuff for read                     */ 
    {
        WORD wStripSize;

        // Set up max strip size

        switch (wImportClass)
        {
            case IMON:
                wStripSize = MAXBUFSIZE >> 2;
                break;
            case ICM:
            case IGR:
                wStripSize = MAXBUFSIZE / 3;
                break;
            case IRGB:
                wStripSize = MAXBUFSIZE;
                break;
        }
        wRowsPerStrip = wStripSize / lpFileInfo -> wBytesPerRow;
    }

//wRowsPerStrip = MAXBUFSIZE / lpFileInfo->wPaddedBytesPerRow;


  if (wRowsPerStrip > lpFileInfo->wScanHeight)
    wRowsPerStrip = lpFileInfo->wScanHeight;
  wNumStrips = lpFileInfo->wScanHeight / wRowsPerStrip;
  wRowCount  = wNumStrips * wRowsPerStrip;
  if (wRowCount < lpFileInfo->wScanHeight){
    wRowsPerLastStrip = lpFileInfo->wScanHeight - wRowCount;
    wNumStrips++;
  }
  else
    wRowsPerLastStrip = wRowsPerStrip;
  /*
  **  Finish up read file info
  */
  lpReadFileInfo->wNumStrips          = wNumStrips;
  lpReadFileInfo->wRowsPerStrip       = wRowsPerStrip;
  lpReadFileInfo->wRowsPerLastStrip   = wRowsPerLastStrip;
  lpReadFileInfo->wBytesPerStrip      = wRowsPerStrip * 
                                        lpReadFileInfo->wBytesPerRow;
  lpReadFileInfo->wBytesPerLastStrip  = wRowsPerLastStrip * 
                                        lpReadFileInfo->wBytesPerRow;
  /*
  **  Point to beginning of dib image data
  */
  lpReadFileInfo->dwDataOffset        = lpBitmapFileHeader->bfOffBits;
  /*
  **  Finish up imageprep file info
  */
  lpFileInfo->wNumStrips              = wNumStrips;
  lpFileInfo->wRowsPerStrip           = wRowsPerStrip;
  lpFileInfo->wRowsPerLastStrip       = wRowsPerLastStrip;
  lpFileInfo->wBytesPerStrip          = wRowsPerStrip * 
                                        lpFileInfo->wPaddedBytesPerRow;
  lpFileInfo->wBytesPerLastStrip      = wRowsPerLastStrip *
                                        lpFileInfo->wPaddedBytesPerRow;
  /*  
  **  Point to beginning of image data for reading image. Because images
  **  are "updside down", this will be reset to end of file when read
  **  begins.
  */
  lpFileInfo->dwDataOffset = (DWORD)lpBitmapFileHeader->bfOffBits + 
    ((DWORD)lpReadFileInfo->wPaddedBytesPerRow * lpReadFileInfo->wScanHeight);
  /*
  **  Check for palette and palette size. Normally palette size 
  **  corresponds to bits per pixel color resolution. 24-bit Windows 3.0
  **  DIBs, however, may have a palette for optimizing the display. The
  **  size of the palette in this case is indicated in the biClrUsed 
  **  field. PM DIBs do not have this flexibility, so if it is a 24-bit
  **  DIB, it doesn't have a palette
  */
  wPaletteSize = (WORD)((lpReadFileInfo->wBitsPerPixel == 24) ? 
    ((wDIBType == WNDIB) ? (WORD)lpBitmapInfoHeader->biClrUsed : 0) :
    1 << lpReadFileInfo->wBitsPerPixel);
  /*
  **  If the DIB has a palette, read it into RGBQUAD DIB palette  
  */
  if (wPaletteSize){
    _llseek ((int)hFile, (long)(sizeof(BITMAPFILEHEADER) + 
      lpBitmapInfoHeader->biSize), SEEK_SET);
    switch (wImportType){
      case IDFMT_WNDIB:   
        /*
        **  read palette directly into DIB palette
        */
        if ((_lread ((int)hFile, (LPSTR)lpRGBQuad, wPaletteSize * 
          sizeof(RGBQUAD))) != wPaletteSize * sizeof(RGBQUAD))
          return (FALSE);
          break;
      case IDFMT_PMDIB:
        if ((_lread ((int)hFile, (LPSTR)lpPalette, wPaletteSize * 
          sizeof(RGBTRIPLE))) != wPaletteSize * sizeof(RGBTRIPLE))
          return (FALSE);
        /*  
        **  reformat palette from RGBTRIPLE to RGBQUAD in DIB palette 
        */
        lpRGBTriple = (RGBTRIPLE FAR *)lpPalette;
        for (i = 0; i < (int)wPaletteSize; i++){
          lpRGBQuad[i].rgbBlue      = lpRGBTriple[i].rgbtBlue;
          lpRGBQuad[i].rgbGreen     = lpRGBTriple[i].rgbtGreen;
          lpRGBQuad[i].rgbRed       = lpRGBTriple[i].rgbtRed;
          lpRGBQuad[i].rgbReserved  = 0;
        }
        break;
      default:
        return (FALSE);
    }
  }


//SetState (IMPORT_CLASS, lpReadFileInfo->wBitsPerPixel == 24 ? IRGB :
//  lpReadFileInfo->wBitsPerPixel == 1 ? IMON : ICM);


  /*
  **  Fill out BitmapInfoHeader 
  */
  lpBitmapInfoHeader->biSize          = sizeof(BITMAPINFOHEADER);
  lpBitmapInfoHeader->biWidth         = lpFileInfo->wScanWidth;
  lpBitmapInfoHeader->biHeight        = lpFileInfo->wScanHeight;
  lpBitmapInfoHeader->biPlanes        = 1;
  lpBitmapInfoHeader->biBitCount      = lpFileInfo->wBitsPerPixel;
  lpBitmapInfoHeader->biCompression   = 0;
  lpBitmapInfoHeader->biXPelsPerMeter = 0;
  lpBitmapInfoHeader->biYPelsPerMeter = 0;
  lpBitmapInfoHeader->biClrUsed       = 0;
  lpBitmapInfoHeader->biClrImportant  = 0;
  return (TRUE);
} 

