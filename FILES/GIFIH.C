
#include <windows.h>
#include <memory.h>
#include <cpi.h>
#include <imgprep.h>
#include <giffmt.h>
#include <global.h>
#include <error.h>

BOOL GIFScreenHeader (int, LPGIFSCREENDESC, LPFILEINFO);
BOOL GIFImageHeader  (int, LPGIFIMAGEDESC, WORD, WORD);
BOOL PackageGIFBytes (int, LPSTR, LPSTR, int);
//  void ReverseWord (WORD, WORD *);  OUT!!!

HANDLE hHashBuf;
HANDLE hPackageBuf;
LPSTR  lpHashBuf;
LPSTR  lpPackageBuf;


int FAR PASCAL GIFInitHeader (
  int           hFile,
  LPSTR         lpBuffer,
  HANDLE        hIFInfo,    
  HANDLE        hIBInfo,    
  HANDLE        hEFInfo,    
  HANDLE        hEBInfo)    
{
  BYTE          TmpPal [768];
  int           i;
  LPBITMAPINFO  lpIBitmapInfo;
  LPBITMAPINFO  lpEBitmapInfo;
  LPFILEINFO    lpIFileInfo;
  LPFILEINFO    lpEFileInfo;
  RGBQUAD FAR * lpRGBQuadPtr;
  WORD          wBytesPerStrip;
  WORD          wRowsPerStrip;
  WORD          wNumStrips;
  WORD          wBytesPerLastStrip;
  WORD          wRowsPerLastStrip;
  WORD          wRowCount;
  WORD          bytes;
  LPSTR         lpPalPtr;
  GIFSCREENDESC GIFScreenDesc;
  GIFIMAGEDESC  GIFImageDesc;
  LPGIFFIXUP    lpGIFFixup;
  /* 
  **  Setup buffering
  */ 
  if (!(lpIFileInfo = (LPFILEINFO)GlobalLock (hIFInfo)))
    return (EC_MEMORY1);
  if (!(lpEFileInfo = (LPFILEINFO)GlobalLock (hEFInfo))){
    GlobalUnlock (hIFInfo);
    return (EC_MEMORY1);
  }
  if (!(lpIBitmapInfo = (LPBITMAPINFO)GlobalLock (hIBInfo))){
    GlobalUnlock (hIFInfo);
    GlobalUnlock (hEFInfo);
    return (EC_MEMORY1);
  }
  if (!(lpEBitmapInfo = (LPBITMAPINFO)GlobalLock (hEBInfo))){
    GlobalUnlock (hIBInfo);
    GlobalUnlock (hIFInfo);
    GlobalUnlock (hEFInfo);
    return (EC_MEMORY1);
  }
  wRowsPerStrip  =  lpIFileInfo->wRowsPerStrip;
  wBytesPerStrip =  wRowsPerStrip * lpEFileInfo->wPaddedScanWidth;
  wNumStrips     =  lpEFileInfo->wScanHeight / wRowsPerStrip;
  wRowCount      =  wNumStrips * wRowsPerStrip;
  /*
  **  Set internal counts
  */
  if (wRowCount < lpEFileInfo->wScanHeight){
    wBytesPerLastStrip  = (lpEFileInfo->wScanHeight - wRowCount) * 
                          lpEFileInfo->wPaddedScanWidth;
    wRowsPerLastStrip   = (lpEFileInfo->wScanHeight - wRowCount);
    wNumStrips++;
  }
  else{
    wBytesPerLastStrip = wBytesPerStrip;
    wRowsPerLastStrip  = wRowsPerStrip;
  }
  /*
  **  Write the GIF file  signature
  */
  bytes = _lwrite (hFile, (LPSTR) "GIF87a", 6);
  if (FailedLWrite (bytes, 6)){
    GlobalUnlock (hEBInfo);
    GlobalUnlock (hIBInfo);
    GlobalUnlock (hIFInfo);
    GlobalUnlock (hEFInfo);
    return (EC_FILEWRITE2);
  }
  /*
  **  Write the GIF screen header
  */
  if (!GIFScreenHeader (hFile, (LPGIFSCREENDESC) &GIFScreenDesc, 
    lpEFileInfo)){
    GlobalUnlock (hEBInfo);
    GlobalUnlock (hIBInfo);
    GlobalUnlock (hIFInfo);
    GlobalUnlock (hEFInfo);
    return (EC_FILEWRITE2);
  }
  /*
  **  Format and write out global palette
  */

  lpPalPtr = (LPSTR) TmpPal;
  lpRGBQuadPtr = (RGBQUAD FAR *)GlobalLock (hExpPalette);

  for (i = 0; i < 256; i++)
  {
    *lpPalPtr++ = (lpRGBQuadPtr->rgbRed);
    *lpPalPtr++ = (lpRGBQuadPtr->rgbGreen);
    *lpPalPtr++ = (lpRGBQuadPtr->rgbBlue);
    lpRGBQuadPtr++;
  }

  GlobalUnlock (hExpPalette);
  bytes = _lwrite (hFile, (LPSTR) TmpPal, 768);
  if (FailedLWrite (bytes, 768)){
    GlobalUnlock (hEBInfo);
    GlobalUnlock (hIBInfo);
    GlobalUnlock (hIFInfo);
    GlobalUnlock (hEFInfo);
    return (EC_FILEWRITE2);
  }
  /*
  **  Write the GIF image header
  */
  if (!GIFImageHeader (hFile, (LPGIFIMAGEDESC) &GIFImageDesc, 
    lpEFileInfo->wScanWidth, lpEFileInfo->wScanHeight)){
    GlobalUnlock (hEBInfo);
    GlobalUnlock (hIBInfo);
    GlobalUnlock (hIFInfo);
    GlobalUnlock (hEFInfo);
    return (EC_FILEWRITE2);
  }
  /*
  **  Need to write an 8 (for decoder)
  */
  TmpPal [0] = 8;               /*  Throw code size into palette buffer  */
  bytes = _lwrite (hFile, (LPSTR) TmpPal, 1);
  if (FailedLWrite (bytes, 1)){
    GlobalUnlock (hEBInfo);
    GlobalUnlock (hIBInfo);
    GlobalUnlock (hIFInfo);
    GlobalUnlock (hEFInfo);
    return (EC_FILEWRITE2);
  }
  lpGIFFixup = (LPGIFFIXUP)((LPSTR)lpEFileInfo + sizeof(FILEINFO));
  lpGIFFixup->bIsFirstStrip = TRUE;
  /*
  **  Cleanup
  */
  GlobalUnlock (hEBInfo);
  GlobalUnlock (hIBInfo);
  GlobalUnlock (hIFInfo);
  GlobalUnlock (hEFInfo);
  return (0);
}

/****************************************************************************

  PROCEDURE:      GIFWrConvertData
  DESCRIPTION;    Writes file
  DEFINITION:
  CREATED:        02/21/90   David Ison

*****************************************************************************/

int FAR PASCAL GIFWrConvertData (
  int           hFile,
  LPSTR FAR *   lppT,
  LPSTR FAR *   lppD,
  LPFILEINFO    lpEFileInfo,
  LPBITMAPINFO  lpBitmapInfo)
{
  LPSTR         lpSrc;
  LPSTR         lpTmp;
  LPSTR         TmpPtr;
  LPSTR         SrcPtr;
  WORD          bytes;
  WORD          wBytesThisStrip;
  WORD          wPaddedBytesThisStrip;
  WORD          wRowsThisStrip;
  WORD          wBytesThisRow;
  WORD          wPaddedBytesThisRow;
  WORD          rows;
  LPGIFFIXUP    lpGIFFixup;
  WORD          wCompressedBytes;
  WORD          wFlags;
  /*
  **
  */
  lpSrc = *lppD;
  lpTmp = *lppT;
  wBytesThisRow         = lpEFileInfo->wBytesPerRow;
  wPaddedBytesThisRow   = lpEFileInfo->wPaddedBytesPerRow;

  if (lpEFileInfo->bIsLastStrip)
  {
    wPaddedBytesThisStrip = lpEFileInfo->wPaddedBytesPerRow *
                            lpEFileInfo->wRowsPerLastStrip;
    wBytesThisStrip       = lpEFileInfo->wBytesPerRow *
                            lpEFileInfo->wRowsPerLastStrip;
    wRowsThisStrip        = lpEFileInfo->wRowsPerLastStrip;
  }
  else{
    wBytesThisStrip       = lpEFileInfo->wBytesPerRow *
                            lpEFileInfo->wRowsPerStrip;
    wPaddedBytesThisStrip = lpEFileInfo->wPaddedBytesPerRow *
                            lpEFileInfo->wRowsPerStrip;
    wRowsThisStrip        = lpEFileInfo->wRowsPerStrip;
  }
  TmpPtr = lpTmp;
  SrcPtr = lpSrc;
  /*
  **  Clip data from padded to non-padded scan width, if necessary
  */
  if (wBytesThisRow != wPaddedBytesThisRow){
    for (rows = 0; rows < wRowsThisStrip; rows++){
      _fmemcpy (TmpPtr, SrcPtr, wBytesThisRow);
      TmpPtr += wBytesThisRow;
      SrcPtr += wPaddedBytesThisRow;
    }
    TmpPtr    = lpTmp;                    /* Data now in dest so swap */
    lpTmp     = lpSrc;
    lpSrc     = TmpPtr;
  }
  /*  
  **  Now compress this strip  
  */
  lpGIFFixup = (LPGIFFIXUP)((LPSTR)lpEFileInfo + sizeof(FILEINFO));
  wCompressedBytes = 0;
  if (lpGIFFixup->bIsFirstStrip){
    lpGIFFixup->bIsFirstStrip = FALSE;
    wFlags = 0;
    /*  
    **  Allocate memory for hash buffer, tmp buffer, etc 
    */
    if (!(hHashBuf = GlobalAlloc (GHND, 20485L))){
      return (EC_NOMEM);
    }  
    if (!(lpHashBuf = GlobalLock (hHashBuf))){
      GlobalFree (hHashBuf);
      return (EC_NOMEM);
    }  
//  if (!(hPackageBuf = GlobalAlloc (GHND, 10240L))){
    if (!(hPackageBuf = GlobalAlloc (GHND, 25000L))){
      GlobalUnlock (hHashBuf);
      GlobalFree (hHashBuf);
      return (EC_NOMEM);
    }  
    if (!(lpPackageBuf = GlobalLock (hPackageBuf))){
      GlobalUnlock (hHashBuf);
      GlobalFree (hHashBuf);
      GlobalFree (hPackageBuf);
      return (EC_NOMEM);
    }  
  }
  else
    if (!lpEFileInfo->bIsLastStrip)
      wFlags = 1;
    else    
      wFlags = 2;

  CompressLZW   (12, lpTmp, lpSrc, lpHashBuf, wBytesThisStrip, (WORD FAR *) &wCompressedBytes, wFlags);

  PackageGIFBytes (hFile, lpTmp, lpPackageBuf, wCompressedBytes);
  if (lpEFileInfo->bIsLastStrip){
    /*  
    **  Write out the "0" count package and ";" terminator  
    */
    if (!PackageGIFBytes (hFile, lpTmp, lpHashBuf, 0)){
      GlobalUnlock (hHashBuf);
      GlobalUnlock (hPackageBuf);
      GlobalFree (hHashBuf);
      GlobalFree (hPackageBuf);
      return (EC_FILEWRITE2);
    }
    *lpTmp = ';'; 
    bytes = _lwrite (hFile, lpTmp, 1);
    if (FailedLWrite (bytes, 1)){
      GlobalUnlock (hHashBuf);
      GlobalUnlock (hPackageBuf);
      GlobalFree (hHashBuf);
      GlobalFree (hPackageBuf);
      return (EC_FILEWRITE2);
    }
    lpGIFFixup->bIsFirstStrip = TRUE;
    /*
    **  Get rid of internal buffers
    */
    GlobalUnlock (hHashBuf);
    GlobalUnlock (hPackageBuf);
    GlobalFree (hHashBuf);
    GlobalFree (hPackageBuf);
  }
  return (TRUE);
}   
    



/****************************************************************************

  PROCEDURE:      GIFFixupHeader
  DESCRIPTION;    Finished Header
  DEFINITION:
  CREATED:        02/21/90  David Ison

*****************************************************************************/

int FAR PASCAL  GIFFixupHeader  (hFile, lpBuffer, hIFInfo, hIBInfo, hEFInfo, hEBInfo)
int     hFile;
LPSTR   lpBuffer;
HANDLE  hIFInfo;
HANDLE  hEFInfo;
HANDLE  hIBInfo;
HANDLE  hEBInfo;
{
  /*
  **  stub functions
  */
  return (0);
}

BOOL GIFScreenHeader (hOutFile, lpGIFScreenDesc, lpFileInfo)
int                hOutFile;
LPGIFSCREENDESC    lpGIFScreenDesc;
LPFILEINFO         lpFileInfo;
{
  BYTE       bDescriptor;
  unsigned   wBytes;
//  WORD       wTmpVal; 
  WORD       wImageWidth;
  WORD       wImageHeight;

  wImageWidth = lpFileInfo->wScanWidth;
  wImageHeight = lpFileInfo->wScanHeight;

//ReverseWord (wImageWidth, &wTmpVal);
//lpGIFScreenDesc->wScreenWidth   = wTmpVal;
  lpGIFScreenDesc->wScreenWidth   = wImageWidth;

//ReverseWord (wImageHeight, &wTmpVal);
//lpGIFScreenDesc->wScreenHeight  = wTmpVal;
  lpGIFScreenDesc->wScreenHeight  = wImageHeight;

  bDescriptor = 0xF7;  /*  TEMP only  */

  lpGIFScreenDesc->bDescriptor    = 0xF7;
  lpGIFScreenDesc->bBackGround   = 0;
  lpGIFScreenDesc->bFiller        = 0;

  wBytes = _lwrite( hOutFile, (LPSTR)lpGIFScreenDesc, sizeof (GIFSCREENDESC));

  if( ( wBytes <= NULL ) || (wBytes != sizeof (GIFSCREENDESC)))
    return (0);

  return (1);
}

BOOL GIFImageHeader (hOutFile, lpGIFImageDesc, wImageWidth, wImageHeight)
int               hOutFile;
LPGIFIMAGEDESC    lpGIFImageDesc; 
WORD              wImageWidth;
WORD              wImageHeight;
{
//  WORD wTmpVal;
  unsigned wBytes;

  lpGIFImageDesc->bSeparator   = ',';
  lpGIFImageDesc->wImageLeft   = 0;
  lpGIFImageDesc->wImageTop    = 0;

//ReverseWord (wImageWidth, &wTmpVal);
//lpGIFImageDesc->wImageWidth  = wTmpVal;
  lpGIFImageDesc->wImageWidth  = wImageWidth; 

//ReverseWord (wImageHeight, &wTmpVal);
//lpGIFImageDesc->wImageHeight = wTmpVal;

  lpGIFImageDesc->wImageHeight = wImageHeight;

  lpGIFImageDesc->bImageDescriptor = 0x07;

  wBytes = _lwrite (hOutFile, (LPSTR) lpGIFImageDesc, sizeof (GIFIMAGEDESC));
  if (wBytes != sizeof(GIFIMAGEDESC))
    return (0);

  return (1);
}   

/*  "Packages" bytes according to GIF manual Pg. 12   */

/****************************************************************************

      PROCEDURE:   PackageGIFBytes

*****************************************************************************/

BOOL PackageGIFBytes (hFile, lpDataPtr, lpTmpBuf, ByteCount)
int     hFile;
LPSTR   lpDataPtr;
LPSTR   lpTmpBuf;
int     ByteCount;
{
  BYTE      bCurrBlockSize;
  int       TmpByteCount;
  LPSTR     lpTmpPtr;
  unsigned  wBytes;
  WORD      wNumBlocks;

  TmpByteCount = ByteCount;
  wNumBlocks   = 0;
  lpTmpPtr     = lpTmpBuf;

  /**********************  Test for zero length package  *******************/

  if (ByteCount == 0)
    {
      *lpTmpBuf = 0;
      wBytes = _lwrite( hFile, lpTmpPtr, 1 );
      if( ( wBytes <= 0 ) || ( wBytes != 1 ) ) 
         return (0);
      else
        return( 1 );
    }             
  else
  do
    {
      if( TmpByteCount > 255 )
        {
          bCurrBlockSize = 255;
          TmpByteCount -= 255;
        }   
      else 
        {
          bCurrBlockSize = (BYTE) TmpByteCount;
          TmpByteCount = 0;
        }
    
      *lpTmpPtr++ = bCurrBlockSize;
      _fmemcpy (lpTmpPtr, lpDataPtr, (int) bCurrBlockSize);
      lpTmpPtr += bCurrBlockSize;
      lpDataPtr += bCurrBlockSize;  
      wNumBlocks++;
    } 
  while (TmpByteCount > 0);
    
  lpTmpPtr  = lpTmpBuf;   /*  Point back to beginning  */


  wBytes = _lwrite( hFile, lpTmpPtr, (WORD)(ByteCount + wNumBlocks) );

  if( (wBytes <= 0 ) || (wBytes != (WORD)(ByteCount + wNumBlocks)) )
    {
      return (0);
    }
  return (1);
}   

