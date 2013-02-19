

#include <windows.h>
#include <memory.h>
#include <cpi.h>
#include <imgprep.h>
#include <pcxfmt.h>
#include <error.h>
#include <global.h>

/* PaddedScanWidth not being used **** clean up */

/*---------------------------------------------------------------------------

  PROCEDURE:      PCXInitHeader
  DESCRIPTION;    Writes PCXHeader
  DEFINITION:
  MODIFIED:       1/17/90 - David Ison (graft into ImagePrep)
                  1/18/90 - Tom mods for Design implementation
                  1/22/90 - David Ison (more fixes and added VGA .PCX save)
                  1/24/90 - Tom mods for HANDLES not pointers
                  1/26/90 - Tom mods for correct response to state control
                  1/30/90 - David Ison (Fixed last strip bug and padding bug)

----------------------------------------------------------------------------*/

int FAR PASCAL PCXInitHeader (hFile, lpBuffer, hIFInfo, hIBInfo,hEFInfo, hEBInfo)
int     hFile;
LPSTR   lpBuffer;
HANDLE  hIFInfo;    
HANDLE  hIBInfo;    
HANDLE  hEFInfo;    
HANDLE  hEBInfo;    
{
  HANDLE              hLocalBuf = 0;
  HANDLE              hEncodeBuf;
  LPFILEINFO          lpIFileInfo;
  LPFILEINFO          lpEFileInfo;
  LPPCXFIXUP          lpPCXFixup;
  PCXHEADER           PCXHeader;
  RGBQUAD FAR    *    lpRGBQuadPtr;
  WORD                wBytesPerStrip;
  WORD                wRowsPerStrip;
  WORD                wNumStrips;
  WORD                wBytesPerLastStrip;
  WORD                wRowsPerLastStrip;
  WORD                wRowCount;

  /****  Point to format specific fields right after global file fields  ****/

  if( !( lpEFileInfo = (LPFILEINFO)GlobalLock( hEFInfo )))
    {
      return ( EC_MEMORY1 );
    }

  if( !( lpIFileInfo = (LPFILEINFO)GlobalLock( hIFInfo )))
    {
      GlobalUnlock( hEFInfo );
      return ( EC_MEMORY1 );
    }

  lpPCXFixup = (LPPCXFIXUP) ((LPSTR) lpEFileInfo + sizeof (FILEINFO));

  if (!(hEncodeBuf = GlobalAlloc (GHND, 4096L)))
    {
      GlobalUnlock( hIFInfo );
      GlobalUnlock( hEFInfo );
      return ( EC_NOMEM );
    }

  if (wDither == INO16 || wDither == IFS16) 
    {
      LPDISPINFO  lpDispInfo;

      lpDispInfo = (LPDISPINFO)((LPSTR)lpEFileInfo + 512);
      /*  Need a local buffer of wPaddedScanWidth size  */

      if( !( hLocalBuf = LocalAlloc (LHND, lpDispInfo->wPaddedScanWidth)))
        {
          GlobalUnlock( hIFInfo );
          GlobalUnlock( hEFInfo );
          GlobalUnlock( hEncodeBuf );
          GlobalFree (hEncodeBuf);
          return (EC_NOMEM);
        }

      lpPCXFixup->hLocalBuf = hLocalBuf;
    }

  lpPCXFixup->hEncodeBuf  = hEncodeBuf;

  /********************* Setup buffering stuff ******************************/ 

  wRowsPerStrip   = lpIFileInfo->wRowsPerStrip;
  wBytesPerStrip  = wRowsPerStrip * 
                    lpEFileInfo->wPaddedScanWidth;

  wNumStrips      = lpEFileInfo->wScanHeight / wRowsPerStrip;
  wRowCount       = wNumStrips * wRowsPerStrip;

  if (wRowCount < lpEFileInfo->wScanHeight)
    {
      wBytesPerLastStrip  = (lpEFileInfo->wScanHeight - wRowCount) * 
                            lpEFileInfo->wPaddedScanWidth;
      wRowsPerLastStrip   = (lpEFileInfo->wScanHeight - wRowCount);
      wNumStrips++;
    }
  else
    {
      wBytesPerLastStrip = wBytesPerStrip;
      wRowsPerLastStrip  = wRowsPerStrip;
    }

  /****************************  Create the PCX file header **********************/

  lpRGBQuadPtr = (RGBQUAD FAR *)GlobalLock (hExpPalette);

  lpPCXFixup->lpPalette = (LPSTR) lpRGBQuadPtr;  // ???

  if (! WritePCXHeader ( hFile, (LPPCXHEADER) &PCXHeader, lpRGBQuadPtr, 
                         lpEFileInfo))
    {
      GlobalUnlock( hIFInfo );
      GlobalUnlock( hEFInfo );
      GlobalUnlock( hEncodeBuf );
      GlobalUnlock (hExpPalette);
      if (hLocalBuf){
        LocalUnlock( hLocalBuf );
        LocalFree( hLocalBuf );
      }
      return ( EC_WRHDR );
    }

  GlobalUnlock (hIFInfo);
  GlobalUnlock (hEFInfo);
  GlobalUnlock (hExpPalette);
  if (hLocalBuf)
    LocalUnlock (hLocalBuf);
  return (0);                                              // OK return
}

/*--------------------------------------------------------------------------

  PROCEDURE:      PCXWrConvertData
  DESCRIPTION;    Writes PCX Data
  DEFINITION:
  MODIFIED:       1/15/90 - David Ison (graft into ImagePrep)

----------------------------------------------------------------------------*/

int FAR PASCAL PCXWrConvertData (hFile, lppT, lppD, lpEFileInfo, lpBitmapInfo)
int           hFile;
LPSTR FAR *   lppT;
LPSTR FAR *   lppD;
LPFILEINFO    lpEFileInfo;
LPBITMAPINFO  lpBitmapInfo;
{
  int           nRetval = TRUE;
  LPPCXFIXUP    lpPCXFixup;
  LPSTR         lpEncodeBuf;
  LPSTR         lpSrc;
  LPSTR         lpTmp;
  LPSTR         SrcPtr;
  LPSTR         TmpPtr;
  PSTR          pLocalBuf;
  WORD          rows;
  WORD          wBytesThisStrip;
  WORD          wBytesThisRow;
  WORD          wPaddedBytesThisRow;
  WORD          wPaddedBytesThisStrip;
  WORD          wRowsThisStrip;

  lpPCXFixup = (LPPCXFIXUP) ((LPSTR) lpEFileInfo + sizeof (FILEINFO));

  lpSrc = *lppD;
  lpTmp = *lppT;

  wBytesThisRow         = lpEFileInfo->wBytesPerRow;
  wPaddedBytesThisRow   = lpEFileInfo->wPaddedBytesPerRow;

  if (lpEFileInfo->bIsLastStrip)
  {
    wBytesThisStrip       = lpEFileInfo->wBytesPerRow *
                            lpEFileInfo->wRowsPerLastStrip;
    wPaddedBytesThisStrip = lpEFileInfo->wPaddedBytesPerRow *
                            lpEFileInfo->wRowsPerLastStrip;
    wRowsThisStrip        = lpEFileInfo->wRowsPerLastStrip;
  }
  else
  {
    wBytesThisStrip       = lpEFileInfo->wBytesPerRow *
                            lpEFileInfo->wRowsPerStrip;
    wPaddedBytesThisStrip = lpEFileInfo->wPaddedBytesPerRow *
                            lpEFileInfo->wRowsPerStrip;
    wRowsThisStrip        = lpEFileInfo->wRowsPerStrip;
  }
  lpEncodeBuf = (LPSTR)GlobalLock (lpPCXFixup->hEncodeBuf);

  if (wDither == INO16 || wDither == IFS16){
    /*  Need to lock local buffer of wPaddedScanWidth size  */
    pLocalBuf       = LocalLock (lpPCXFixup->hLocalBuf);
  }

  TmpPtr = lpTmp;
  SrcPtr = lpSrc;

  for (rows = 0; rows < wRowsThisStrip; rows++)
  {
    if (wDither == INO16 || wDither == IFS16) 
      enc_pcx16 (SrcPtr, lpEncodeBuf, pLocalBuf, wBytesThisRow, hFile);
    else
      enc_pcx (SrcPtr, (wBytesThisRow + 1)/2*2 , hFile, lpEncodeBuf);
    SrcPtr += wPaddedBytesThisRow;
  }
  /*  Need to unlock local buffer of wPaddedScanWidth size  */

  if (wDither == INO16 || wDither == IFS16) 
    LocalUnlock (lpPCXFixup->hLocalBuf);

  GlobalUnlock (lpPCXFixup->hEncodeBuf);  
  return (0);
}

/****************************************************************************

  PROCEDURE:      PCXFixupHeader
  DESCRIPTION;    Finished Header
  DEFINITION:
  MODIFIED:       1/15/90 - David Ison - Graft into Imageprep

*****************************************************************************/

int FAR PASCAL  PCXFixupHeader  ( hFile, lpBuffer, hIFInfo, hIBInfo, hEFInfo, 
                                  hEBInfo )
int     hFile;
LPSTR   lpBuffer;
HANDLE  hIFInfo;
HANDLE  hEFInfo;
HANDLE  hIBInfo;
HANDLE  hEBInfo;
{
  int               i;
  int               nRetval = FALSE;
  LPBITMAPINFO      lpEBitmapInfo;
  LPFILEINFO        lpEFileInfo;
  LPPCXFIXUP        lpPCXFixup;
  LPSTR             lpPtr;
  RGBQUAD FAR *     lpRGBQuadPtr;
  WORD              bytes;
  
  if (!(lpEFileInfo = (LPFILEINFO)GlobalLock (hEFInfo))){
    return (EC_MEMORY1);
  }
  if (!(lpEBitmapInfo = (LPBITMAPINFO)GlobalLock (hEBInfo))){
    GlobalUnlock( hEFInfo );
    return (EC_MEMORY1);
  }
  /*  Point to format specific fields right after global file fields  */

  lpPCXFixup = (LPPCXFIXUP) ((LPSTR) lpEFileInfo + sizeof (FILEINFO));

  /*  Do the palette */ 

  if (!(lpRGBQuadPtr = (RGBQUAD FAR *)GlobalLock (hExpPalette))){
    GlobalUnlock (hEFInfo);
    GlobalUnlock (hEBInfo);
    return (EC_MEMORY1);
  }

  lpPCXFixup->lpEncodeBuf = (LPSTR)GlobalLock (lpPCXFixup->hEncodeBuf);
  pcxbuf_flush (hFile, (LPSTR) lpPCXFixup->lpEncodeBuf);
  GlobalUnlock (lpPCXFixup->hEncodeBuf);
  GlobalFree (lpPCXFixup->hEncodeBuf);

  if (wDither == INO16 || wDither == IFS16) 
    LocalFree (lpPCXFixup->hLocalBuf);
  else{
    if (_llseek (hFile, 0L, 2) == -1){
      GlobalUnlock (hEFInfo);
      GlobalUnlock (hEBInfo);
      return (EC_FILEWRITE1);
    }

    *lpBuffer = 12;
    lpPtr = (LPSTR)(lpBuffer + 1);

    for (i = 0; i < 256; i++){
      *lpPtr++ = lpRGBQuadPtr->rgbRed ;  
      *lpPtr++ = lpRGBQuadPtr->rgbGreen;  
      *lpPtr++ = lpRGBQuadPtr->rgbBlue ;  
      lpRGBQuadPtr++;
    }
    bytes = _lwrite (hFile, (LPSTR) lpBuffer, 769);
    if (bytes != 769){
      GlobalUnlock (hEFInfo);
      GlobalUnlock (hEBInfo);
      return ( EC_WRHDR );
    }
  }
  GlobalUnlock (hEFInfo);
  GlobalUnlock (hEBInfo);
  return( 0 );
}


int WritePCXHeader (hFile, lpPCXHeader, lpRGBQuadPtr, lpFileInfo)
int        hFile; 
LPPCXHEADER   lpPCXHeader;
RGBQUAD FAR * lpRGBQuadPtr;
LPFILEINFO    lpFileInfo;
{
  int     i;
  LPSTR   lpPalette;
  WORD    bytes;
  WORD    wBitsPerPixel;
  WORD    wNumberPlanes; 
  WORD    wBytesPerPlane;
  WORD    wTmpVal;
  WORD    wPaddedScanWidth;

  /***************************  Zero out the header  ***********************/ 

  _fmemset ((LPSTR) lpPCXHeader, 0, sizeof (PCXHEADER));

  /********************  Fill in mode-specific information  *****************/

  if (wDither == INO16 || wDither == IFS16) 
    {
      wPaddedScanWidth = (lpFileInfo->wScanWidth +1)/2*2;

      wBitsPerPixel   = 1;
      wNumberPlanes   = 4;
      wTmpVal         = wPaddedScanWidth / 8;
      if ((wPaddedScanWidth % 8) != 0)
        wTmpVal++;

      if ((wTmpVal % 2) != 0)
        wTmpVal++;

      wBytesPerPlane  = wTmpVal;

      /***************************  Fill in palette  ************************/

      lpPalette = lpPCXHeader->Clrmap;
      for (i = 0; i < 16; i++)
      {
//        *lpPalette++ = lpRGBQuadPtr->rgbBlue << 2;  
//        *lpPalette++ = lpRGBQuadPtr->rgbGreen << 2;
//        *lpPalette++ = lpRGBQuadPtr->rgbRed << 2;
/*
          *lpPalette++ = lpRGBQuadPtr->rgbBlue;  
          *lpPalette++ = lpRGBQuadPtr->rgbGreen;
          *lpPalette++ = lpRGBQuadPtr->rgbRed;
*/
          *lpPalette++ = lpRGBQuadPtr->rgbRed;  
          *lpPalette++ = lpRGBQuadPtr->rgbGreen;
          *lpPalette++ = lpRGBQuadPtr->rgbBlue;

          lpRGBQuadPtr++;
      }
    }
  else       /********************* it's a 256 color mode *****************/
    {
      wBitsPerPixel   = 8;
      wNumberPlanes   = 1;
//      wBytesPerPlane  = lpFileInfo->wPaddedScanWidth;
      wBytesPerPlane  = (lpFileInfo->wScanWidth +1)/2*2;
    }

  lpPCXHeader->Manuf        = 0xA;
  lpPCXHeader->Hard         = 0x5;
  lpPCXHeader->Encod        = 0x1;
  lpPCXHeader->BitsPerPixel = (BYTE) wBitsPerPixel;
  lpPCXHeader->X1           = 0x0;
  lpPCXHeader->Y1           = 0x0;
  lpPCXHeader->X2           = lpFileInfo->wScanWidth   - 1;
  lpPCXHeader->Y2           = lpFileInfo->wScanHeight  - 1;
  lpPCXHeader->Hres         = 640;
  lpPCXHeader->Vres         = 480;
  lpPCXHeader->Nplanes      = (BYTE) wNumberPlanes;
  lpPCXHeader->bplin        = wBytesPerPlane;

  bytes  = _lwrite (hFile, (LPSTR) lpPCXHeader, sizeof (PCXHEADER));

  if (FailedLWrite (bytes, sizeof (PCXHEADER)))
      return (FALSE);

  return (TRUE);
}

/****************************************************************************

  PROCEDURE:      enc_pcx16
  DESCRIPTION;    
  DEFINITION:
  MODIFIED:       

*****************************************************************************/

void FAR enc_pcx16  (scan_buf, enc_buf, local_buf, scanbytes, outfh) 
LPSTR     scan_buf; 
LPSTR     enc_buf;
PSTR      local_buf;
WORD      scanbytes;
int       outfh;
{
  #define BITMASK 0x01 

  char    inbyte;
  char    outbyte;
  char *  scanptr;
  LPSTR   outptr;
  int     num_eights;
  int     tmp_val;
  int     i, j;

  tmp_val = (scanbytes / 8);

  if ((scanbytes % 8) != 0)
      tmp_val++; 

  if ((tmp_val % 2) != 0)                       /*  Make sure even number */
      tmp_val++; 

  num_eights = tmp_val;

  /****** Transfer input data to local buffer for better performance *****/
  /* (The extra bytes are for any "overhang" from padding)  */

  _fmemcpy ((LPSTR) local_buf, scan_buf, scanbytes);

  /***************************** Do LSB first  ****************************/

  scanptr = local_buf;
  outptr  = scan_buf;

  for (i = 0; i < num_eights; i++)
    {
      outbyte = 0;

      for (j = 0; j < 8; j++)
        {
          inbyte = *scanptr++;
          inbyte = (char)((int)inbyte & BITMASK);
          outbyte = (outbyte | inbyte);

          if (j < 7)
            outbyte <<= 1;
        }
      *outptr++ = outbyte;
    }   

  enc_pcx (scan_buf,num_eights,outfh,enc_buf);

  /******************************* Then 2nd LSB ***************************/

  scanptr = local_buf;
  outptr  = scan_buf;

  for (i = 0; i < num_eights; i++)
    {
      outbyte = 0;

      for (j = 0; j < 8; j++)
        {
          inbyte = *scanptr++;
          inbyte >>= 1;
          inbyte  = (char)((int)inbyte & BITMASK);
          outbyte = (outbyte | inbyte);

          if (j < 7)
            outbyte <<= 1;
        }
      *outptr++ = outbyte;
    }   

  enc_pcx( scan_buf, num_eights, outfh, enc_buf );

  /*************************** Then 3rd LSB ********************************/

  scanptr = local_buf;
  outptr  = scan_buf;

  for (i = 0; i < num_eights; i++)
    {
      outbyte = 0;

      for (j = 0; j < 8; j++)
        {
          inbyte = *scanptr++;
          inbyte >>= 2;
          inbyte = (char)((int)inbyte & BITMASK);
          outbyte = (outbyte | inbyte);
      
          if (j < 7)
            outbyte <<= 1;
        }
      *outptr++ = outbyte;
    }   

  enc_pcx (scan_buf,num_eights,outfh,enc_buf);

  /******************************** Then MSB *******************************/

  scanptr = local_buf;
  outptr = scan_buf;
  
  for (i = 0; i < num_eights; i++)
    {
      outbyte = 0;
   
      for (j = 0; j < 8; j++)
        {
          inbyte = *scanptr++;
          inbyte >>= 3;
          inbyte = (char)((int)inbyte & BITMASK);
          outbyte = (outbyte | inbyte);
      
          if (j < 7)
            outbyte <<= 1;
        }
      *outptr++ = outbyte;
    }   

  enc_pcx (scan_buf,num_eights,outfh,enc_buf);
}















