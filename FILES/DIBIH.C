/****************************************************************************
**  MODULE:        dibih.c
**  PROJECT:       Image Prep 3.0
**  DESCRIPTION:   CPi specific Header Routines
**  STARTED:       David W. Mize
**  MODIFIED:      05/20/90
*****************************************************************************/
#include <windows.h>
#include <stdio.h>
#include <memory.h>
#include <io.h>
#include <cpi.h>
#include <imgprep.h>
#include <dibfmt.h>
#include <error.h>
#include <global.h>


/****************************************************************************
**  PROCEDURE:        WriteDIBFileHeader
**  DESCRIPTION;      Writes file header and palette
**  DEFINITION:       David W. Mize
**  MODIFIED:         04/03/90
*****************************************************************************/

BOOL NEAR PASCAL WriteDIBFileHeader (
  int               hFile, 
  LPFILEINFO        lpFileInfo,
  RGBQUAD FAR *     lpRGBQuad,
  LPSTR             lpBuffer)
{
  BITMAPFILEHEADER  BitmapFileHeader;
  BITMAPINFOHEADER  BitmapInfoHeader;
  BITMAPCOREHEADER  BitmapCoreHeader;
  WORD              wPaletteSize;
  /*
  **  Fill out BITMAPFILEHEADER
  */
  BitmapFileHeader.bfType             = 0x4D42;         /* 'BM' */
  BitmapFileHeader.bfSize             = 0;              /* unknown now */
  BitmapFileHeader.bfReserved1        = 0;
  BitmapFileHeader.bfReserved2        = 0;
  BitmapFileHeader.bfOffBits          = 0;              /* unknown now */
  /*
  **  Fill out BITMAPINFOHEADER
  */
  BitmapInfoHeader.biSize   = sizeof (BITMAPINFOHEADER);
  BitmapInfoHeader.biWidth  = lpFileInfo->wScanWidth;
  BitmapInfoHeader.biHeight = lpFileInfo->wScanHeight;
  BitmapInfoHeader.biPlanes = 1;
  /*
  **  Determine color resolution and bit depth
  */
  switch (wExportClass){
    case IRGB:
      BitmapInfoHeader.biBitCount = 24;
      wPaletteSize = 0;
      break;
    case ICM:
    case IGR:
      switch (ePalType){
        case DP256:
        case IP256:
        case OP256:
        case GP256:
        case GP64:
          BitmapInfoHeader.biBitCount = 8;
          wPaletteSize = 256;
          break;
        case DP16:
        case OP16:
        case GP16:
        case DP8:
        case OP8:
//      case GP8:
          BitmapInfoHeader.biBitCount = 4;
          wPaletteSize = 16;
          break;
        case BP:
          BitmapInfoHeader.biBitCount = 1;
          wPaletteSize = 2;
          break;
      }
      break;
    case IMON:
      BitmapInfoHeader.biBitCount = 1;
      wPaletteSize = 2;
      break;
  }
  /*
  **  Determine image size
  */
  BitmapInfoHeader.biSizeImage      = (DWORD)WIDTHBYTES((DWORD)BitmapInfoHeader.biWidth 
                                      * (DWORD)BitmapInfoHeader.biBitCount) 
                                      * (DWORD)BitmapInfoHeader.biHeight;
  BitmapInfoHeader.biCompression    = 0;      /* BI_RGB; */
  BitmapInfoHeader.biXPelsPerMeter  = 0;
  BitmapInfoHeader.biYPelsPerMeter  = 0;
  BitmapInfoHeader.biClrUsed        = 0;
  BitmapInfoHeader.biClrImportant   = 0;
  /*
  **  Fixup file header 
  */
  switch (wExportType){
    case IDFMT_WNDIB:
      BitmapFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + 
        sizeof(BITMAPINFOHEADER) + (wPaletteSize * sizeof(RGBQUAD));
      break;
    case IDFMT_PMDIB:
      BitmapFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) +
        sizeof(BITMAPCOREHEADER) + (wPaletteSize * sizeof(RGBTRIPLE));
      /*
      **  Fill out BITMAPCOREHEADER
      */
      BitmapCoreHeader.bcSize     = sizeof(BITMAPCOREHEADER);
      BitmapCoreHeader.bcWidth    = (WORD)BitmapInfoHeader.biWidth;
      BitmapCoreHeader.bcHeight   = (WORD)BitmapInfoHeader.biHeight;
      BitmapCoreHeader.bcPlanes   = 1;
      BitmapCoreHeader.bcBitCount = BitmapInfoHeader.biBitCount;
      break;
  }
  /*
  **  Determine file size = offset to image data + image data size
  */
  BitmapFileHeader.bfSize = BitmapFileHeader.bfOffBits + 
                            BitmapInfoHeader.biSizeImage;
  /*
  **  Write file header and palette
  */
  if (_lwrite (hFile, (LPSTR)&BitmapFileHeader, sizeof(BITMAPFILEHEADER)) !=
    sizeof(BITMAPFILEHEADER))
    return (FALSE);
  switch (wExportType){
    case IDFMT_WNDIB:
      /*
      **  Write BITMAPINFOHEADER
      */
      if (_lwrite (hFile, (LPSTR)&BitmapInfoHeader, 
        sizeof(BITMAPINFOHEADER)) != sizeof(BITMAPINFOHEADER))
        return (FALSE);
      /*
      **  Write palette
      */
      if (_lwrite (hFile, (LPSTR)lpRGBQuad, wPaletteSize * 
        sizeof(RGBQUAD)) != wPaletteSize * sizeof(RGBQUAD))
        return (FALSE);
        break;
    case IDFMT_PMDIB:{
        RGBTRIPLE rgbTriple;
        int       i;
        /*
        **  Write BITMAPCOREHEADER
        */
        if (_lwrite (hFile, (LPSTR)&BitmapCoreHeader,
          sizeof(BITMAPCOREHEADER)) != sizeof(BITMAPCOREHEADER))
          return (FALSE);
        /*
        **  Write palette
        */
        for (i = 0; i < (int)wPaletteSize; i++){
          rgbTriple.rgbtBlue  = lpRGBQuad[i].rgbBlue;
          rgbTriple.rgbtGreen = lpRGBQuad[i].rgbGreen;
          rgbTriple.rgbtRed   = lpRGBQuad[i].rgbRed;
          if( _lwrite (hFile, (LPSTR)&rgbTriple, sizeof(RGBTRIPLE)) != 
            sizeof(RGBTRIPLE))
            return (FALSE);
          }
        }
        break;
    default:
      return (FALSE);
  }

  /*
  **  Write out file so we can invert image
  */
//if (chsize (hFile, (long)BitmapFileHeader.bfSize) == -1)
//  return (FALSE);


        {
            DWORD dwSize;
            DWORD dwBytesWritten;

            dwBytesWritten = tell (hFile);  
            dwSize   = 20480L; // TEMP!
 
            if (! AppendFile (hFile, lpBuffer, (BitmapFileHeader.bfSize - dwBytesWritten), dwSize))
            {
                return (EC_FILEWRITE2);

            }
        }




  if(_llseek (hFile, (long)BitmapFileHeader.bfSize, SEEK_SET) != 
    (long)BitmapFileHeader.bfSize)
    return (FALSE);
  return (TRUE);
}

/****************************************************************************
**  PROCEDURE:        DIBInitHeader
**  DESCRIPTION;      Writes header info
**  DEFINITION:
**  MODIFIED:         1/14/90
*****************************************************************************/

int FAR PASCAL DIBInitHeader (
  int           hFile,
  LPSTR         lpBuffer,
  HANDLE        hIFInfo,
  HANDLE        hIBInfo,
  HANDLE        hEFInfo,
  HANDLE        hEBInfo)
{
  LPBITMAPINFO  lpImportBitmapInfo;
  LPBITMAPINFO  lpExportBitmapInfo;
  LPFILEINFO    lpImportFileInfo;
  LPFILEINFO    lpExportFileInfo;
  RGBQUAD FAR * lpRGBQuad;
  /*
  **  Setup buffering stuff
  */
  if (!(lpImportFileInfo = (LPFILEINFO)GlobalLock (hIFInfo)))
    return (EC_MEMORY1);
  if (!(lpExportFileInfo = (LPFILEINFO)GlobalLock (hEFInfo))){
    GlobalUnlock (hIFInfo);
    return (EC_MEMORY1);
  }
  if (!(lpImportBitmapInfo = (LPBITMAPINFO)GlobalLock (hIBInfo))){
    GlobalUnlock (hEFInfo);
    GlobalUnlock (hIFInfo);
    return (EC_MEMORY1);
  }
  if (!(lpExportBitmapInfo = (LPBITMAPINFO)GlobalLock (hEBInfo))){
    GlobalUnlock (hIBInfo);
    GlobalUnlock (hEFInfo);
    GlobalUnlock (hIFInfo);
    return (EC_MEMORY1);
  }
  if (!(lpRGBQuad = (RGBQUAD FAR *)GlobalLock (hExpPalette))){
    GlobalUnlock (hEBInfo);
    GlobalUnlock (hIBInfo);
    GlobalUnlock (hEFInfo);
    GlobalUnlock (hIFInfo);
    return (EC_MEMORY1);
  }  
  if (!WriteDIBFileHeader (hFile, lpExportFileInfo, lpRGBQuad, lpBuffer)){
    GlobalUnlock (hExpPalette);
    GlobalUnlock (hEBInfo);
    GlobalUnlock (hIBInfo);
    GlobalUnlock (hEFInfo);
    GlobalUnlock (hIFInfo);
    return (EC_FILEWRITE2);
  }
  GlobalUnlock (hExpPalette);
  GlobalUnlock (hEBInfo);
  GlobalUnlock (hIBInfo);
  GlobalUnlock (hEFInfo);
  GlobalUnlock (hIFInfo);
  return (0);
}

/****************************************************************************
**  PROCEDURE:         CPiWrConvertData
**  DESCRIPTION;      Writes file
**  DEFINITION:
**  MODIFIED:            1/14/90
*****************************************************************************/

int FAR PASCAL DIBWrConvertData (
  int         hFile,
  LPSTR FAR * lplpTemp,
  LPSTR FAR * lplpData,
  LPFILEINFO  lpExportFileInfo,
  LPBITMAPINFO lpBitmapInfo)
{
  int         nReturn = TRUE;
  LPSTR       lpSource;
  LPSTR       lpDest;
  LPSTR       lpTmp;
  LPSTR       lpDst;
  LPSTR       lpSrc;
  WORD        wBytesThisStrip;
  WORD        wPaddedBytesThisStrip;
  WORD        wRowsThisStrip;
  WORD        wBytesThisRow;
  WORD        wPaddedBytesThisRow;
  WORD        wDIBBytesPerRow;
  WORD        wDIBBytesPerStrip;
  BYTE        bTemp;
  int         i;
  int         j;
  int         k;
  /*
  **  Set pointers
  */
  lpSource  = *lplpData;
  lpDest    = *lplpTemp;

  wBytesThisRow           = lpExportFileInfo->wBytesPerRow;
  wPaddedBytesThisRow     = lpExportFileInfo->wPaddedBytesPerRow;
  if (lpExportFileInfo->bIsLastStrip){
    wPaddedBytesThisStrip = lpExportFileInfo->wPaddedBytesPerRow *
                            lpExportFileInfo->wRowsPerLastStrip;
    wBytesThisStrip       = lpExportFileInfo->wBytesPerRow *
                            lpExportFileInfo->wRowsPerLastStrip;
    wRowsThisStrip        = lpExportFileInfo->wRowsPerLastStrip;
  }
  else{
    wBytesThisStrip       = lpExportFileInfo->wBytesPerRow *
                            lpExportFileInfo->wRowsPerStrip;
    wPaddedBytesThisStrip = lpExportFileInfo->wPaddedBytesPerRow *
                            lpExportFileInfo->wRowsPerStrip;
    wRowsThisStrip        = lpExportFileInfo->wRowsPerStrip;
  }
  /*
  **  Invert image buffer buffer: Data in lpDest
  */
  InvertDIB (lpDest, lpSource, wPaddedBytesThisRow, wRowsThisStrip);
  lpTmp    = lpDest;
  lpDest   = lpSource;
  lpSource = lpTmp;
  /*
  **  Reformat for color resolution: Data in lpSource
  */
  switch (wExportClass){
    case IRGB:
      /*
      **  format for dib padding data in lpDest and will be put 
      **  back in lpSource
      */
      wDIBBytesPerRow   = WIDTHBYTES(lpExportFileInfo->wScanWidth * 24);
      lpSrc    = lpSource;
      lpDst    = lpDest;
      for (i = 0; i < (int)wRowsThisStrip; i++){
        _fmemcpy (lpDst, lpSrc, wBytesThisRow);
        lpDst += wDIBBytesPerRow;
        lpSrc += wPaddedBytesThisRow;
      }
      break;
    case ICM:
    case IGR:
      switch (ePalType){
        case DP256:
        case IP256:
        case OP256:
        case GP256:
        case GP64:
          /*
          **  format for dib padding
          */
          wDIBBytesPerRow   = WIDTHBYTES(lpExportFileInfo->wScanWidth * 8);
          lpSrc    = lpSource;
          lpDst    = lpDest;
          for (i = 0; i < (int)wRowsThisStrip; i++){
            _fmemcpy (lpDst, lpSrc, wBytesThisRow);
            lpDst += wDIBBytesPerRow;
            lpSrc += wPaddedBytesThisRow;
          }
          break;
        case DP16:
        case OP16:
        case GP16:
        case DP8:
        case OP8:
//      case GP8:
          /*
          **  format 8-bit data to 4-bit dib and padd for dib
          */
          wDIBBytesPerRow = WIDTHBYTES(lpExportFileInfo->wScanWidth * 4);
          for (i = 0; i < (int)wRowsThisStrip; i++){
            lpSrc = lpSource + (i * wPaddedBytesThisRow);
            lpDst = lpDest + (i * wDIBBytesPerRow);
            for (j = 0; j < (int)wDIBBytesPerRow; j++){
              bTemp     = *lpSrc++;
              bTemp   <<= 4;
              bTemp    += *lpSrc++;
              *lpDst++  = bTemp;
            }
          }
          break;
      }
      break;
    case IMON:
      /*
      **  format 8-bit to 1-bit dib and add dib padding
      */
      wDIBBytesPerRow   = WIDTHBYTES(lpExportFileInfo->wScanWidth);
      for (i = 0; i < (int)wRowsThisStrip; i++){
        lpSrc = lpSource + (i * wPaddedBytesThisRow);
        lpDst = lpDest + (i * wDIBBytesPerRow);
        j     = wDIBBytesPerRow;
        while (j){
          bTemp = 0;
          for (k = 0; k < 8; k++){ 
            bTemp <<= 1;
            if (*lpSrc++)
              bTemp |= 1;
          }
          *lpDst++ = bTemp;
          j--;
        }
      }
      break;
  }
  /*
  **  Swap buffers
  */
  lpTmp    = lpDest;
  lpDest   = lpSource;
  lpSource = lpTmp;
  /**  
  **  write strip  
  */
  wDIBBytesPerStrip = wDIBBytesPerRow * wRowsThisStrip;
  _llseek (hFile, - (long)wDIBBytesPerStrip, SEEK_CUR);
  if( _lwrite (hFile, (LPSTR)lpSource, wDIBBytesPerStrip) != 
    wDIBBytesPerStrip)
    nReturn = FALSE;
  _llseek (hFile, - (long)wDIBBytesPerStrip, SEEK_CUR);
  /*
  **  Reset pointers  
  */
  *lplpData = lpSource;
  *lplpTemp = lpDest;
  return (nReturn);
}

/***************************************************************************
**  PROCEDURE:        DIBFixupHeader
**  DESCRIPTION;      Unused
**  DEFINITION:
**  MODIFIED:            1/14/90
****************************************************************************/

int FAR PASCAL  DIBFixupHeader  (
  int               hFile,
  LPSTR             lpBuffer,
  HANDLE            hIFinfo,
  HANDLE            hEFInfo,
  HANDLE            hIBInfo,
  HANDLE            hEBInfo)
{
  /*
  **  NOT NECESSARY, ALL INFORMATION WAS PROPERLY PLACED IN INITIALIZING
  **  DIB HEADER.
  */
  return (0);
}



