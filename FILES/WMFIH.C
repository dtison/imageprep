
#include <windows.h>
#include <imgprep.h>
#include <global.h>
#include <error.h>
#include <dibfmt.h>
#include <io.h>       // Using C library "tell" function
#include <memory.h>   // For C 6.0 memory copy etc.
#include <cpi.h>      // CPI library tools
#include "dibio.h"
#include "metafile.h"

typedef struct tagWMFFIXUP {
    HANDLE          hDibImage;
    int             wCurrentLine;
    char            lpTempName[1];
} WMFFIXUP;

typedef WMFFIXUP FAR * LPWMFFIXUP;


int FAR PASCAL WMFInitHeader (
int           hFile,
LPSTR         lpBuffer,
HANDLE        hIFInfo,
HANDLE        hIBInfo,
HANDLE        hEFInfo,  
HANDLE        hEBInfo        )
{


    LPFILEINFO          lpEFileInfo;

    LPWMFFIXUP          lpWMFFixup;

    if (! (lpEFileInfo = (LPFILEINFO) GlobalLock (hEFInfo)))
        return (EC_MEMORY1);

    lpWMFFixup  = (LPWMFFIXUP) ((LPSTR) lpEFileInfo + sizeof (FILEINFO));

    /*  Create the temporary file.  */

    GetTempFileName(0, "WMF", (WORD) GetTickCount(),
        lpWMFFixup->lpTempName);
    
    lpWMFFixup->hDibImage = dibNewImage(
        lpWMFFixup->lpTempName,
        lpEFileInfo->wScanWidth,
        lpEFileInfo->wScanHeight,
        (lpEFileInfo->wBitsPerPixel > 8) ? 8 : lpEFileInfo->wBitsPerPixel,
        (lpEFileInfo->wBitsPerPixel > 8) ? 3 : 1,
      (lpEFileInfo->wBitsPerPixel > 8) ? 0 : (1 << lpEFileInfo->wBitsPerPixel),
        0, 0);


    if (lpEFileInfo -> wBitsPerPixel == 8)
    {
        RGBQUAD FAR *lpRGBQuadPtr;

        lpRGBQuadPtr = (RGBQUAD FAR *)GlobalLock(hExpPalette );

        dibPutPalette (lpWMFFixup -> hDibImage, (1 << lpEFileInfo->wBitsPerPixel), lpRGBQuadPtr);

        GlobalUnlock (hExpPalette);
    }   
    lpWMFFixup -> wCurrentLine = 0;
    
    GlobalUnlock (hEFInfo);

    return (0);
}


int FAR PASCAL WMFWrConvertData (
int           hFile,
LPSTR FAR *   lplpTemp,
LPSTR FAR *   lplpData,
LPFILEINFO    lpExportFileInfo,
LPBITMAPINFO  lpBitmapInfo)
{
  int         nReturn = TRUE;
  LPWMFFIXUP  lpWMFFixup;
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
  WORD        wError;
  int         i;


  lpSource  = *lplpData;
  lpDest    = *lplpTemp;

  lpWMFFixup  = (LPWMFFIXUP) ((LPSTR) lpExportFileInfo + sizeof (FILEINFO));

  wBytesThisRow           = lpExportFileInfo->wBytesPerRow;
  wPaddedBytesThisRow     = lpExportFileInfo->wPaddedBytesPerRow;
  if (lpExportFileInfo->bIsLastStrip)
  {
    wPaddedBytesThisStrip = lpExportFileInfo->wPaddedBytesPerRow *
                            lpExportFileInfo->wRowsPerLastStrip;
    wBytesThisStrip       = lpExportFileInfo->wBytesPerRow *
                            lpExportFileInfo->wRowsPerLastStrip;
    wRowsThisStrip        = lpExportFileInfo->wRowsPerLastStrip;
  }
  else
  {
    wBytesThisStrip       = lpExportFileInfo->wBytesPerRow *
                            lpExportFileInfo->wRowsPerStrip;
    wPaddedBytesThisStrip = lpExportFileInfo->wPaddedBytesPerRow *
                            lpExportFileInfo->wRowsPerStrip;
    wRowsThisStrip        = lpExportFileInfo->wRowsPerStrip;
  }



  /*   Invert image buffer buffer: Data in lpDest   */
  InvertDIB (lpDest, lpSource, wPaddedBytesThisRow, wRowsThisStrip);
  lpTmp    = lpDest;
  lpDest   = lpSource;
  lpSource = lpTmp;


  /*   Reformat for color resolution: Data in lpSource  */


  switch (wExportClass)
  {
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
    default:
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
  }

  lpTmp    = lpDest;
  lpDest   = lpSource;
  lpSource = lpTmp;

  /* Write strip     */

  wDIBBytesPerStrip = wDIBBytesPerRow * wRowsThisStrip;

    wError = dibPutRect(lpWMFFixup->hDibImage,
        0, lpWMFFixup ->wCurrentLine,
        lpExportFileInfo->wScanWidth - 1,
        lpWMFFixup ->wCurrentLine + wRowsThisStrip - 1,
        lpSource);
    if (wError)
        wmfDisplayErrorMsg(hWndIP, wError);
        
    lpWMFFixup->wCurrentLine += wRowsThisStrip;
    
    /*  Finally, ask DLL to convert DIB to WMF  */

    if (lpExportFileInfo -> bIsLastStrip)
    {

        OFSTRUCT ofs;

        wmfExportToMetaFile(hWndIP, lpWMFFixup->hDibImage, hFile);

        dibEndImage(lpWMFFixup->hDibImage);

        OpenFile(lpWMFFixup->lpTempName, &ofs,
            OF_DELETE);
    }

//*lplpData = lpSource;
//*lplpTemp = lpDest;
  return (nReturn);
}


int FAR PASCAL  WMFFixupHeader  (hFile, lpBuffer, hIFInfo, hIBInfo, hEFInfo, hEBInfo )
int      hFile;
LPSTR    lpBuffer;
HANDLE   hIFInfo;
HANDLE   hEFInfo;
HANDLE   hIBInfo;
HANDLE   hEBInfo;
{
  
    return (0);
}

 

