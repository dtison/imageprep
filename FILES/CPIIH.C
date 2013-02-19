
#include <windows.h>
#include <imgprep.h>
#include <global.h>
#include <error.h>
#include <cpifmt.h>
#include <io.h>       // Using C library "tell" function
#include <memory.h>   // For C 6.0 memory copy etc.
#include <cpi.h>      // CPI library tools

DWORD    dwCPiPalPos;
DWORD    dwCPiDataPos;

void NEAR PASCAL UpdateCPIImageSpec (LPCPIIMAGESPEC, LPFILEINFO);
void NEAR PASCAL InitCPITMPSpec (LPCPIFILESPEC);
void NEAR PASCAL InitCPITMPImageSpec (LPCPIIMAGESPEC);

/*---------------------------------------------------------------------------

   PROCEDURE:        CPiInitHeader
   DESCRIPTION;      Writes header info
   DEFINITION:
   MODIFIED:         1/14/90

----------------------------------------------------------------------------*/

int FAR PASCAL CPIInitHeader (hFile, lpBuffer, hIFInfo, hIBInfo, hEFInfo, hEBInfo)
int      hFile;
LPSTR    lpBuffer;
HANDLE   hIFInfo;      
HANDLE   hIBInfo;      
HANDLE   hEFInfo;      
HANDLE   hEBInfo;      
{
    BYTE            TmpPal[768];
    int             i;
    int             nRetval = TRUE;
    LPBITMAPINFO    lpIBitmapInfo;
    LPBITMAPINFO    lpEBitmapInfo;
    LPFILEINFO      lpIFileInfo;
    LPFILEINFO      lpEFileInfo;
    LPSTR           TmpPtr;
    RGBQUAD FAR *   lpRGBQuadPtr;
    WORD            wBytesPerStrip;
    WORD            wRowsPerStrip;
    WORD            wNumStrips;
    WORD            wBytesPerLastStrip;
    WORD            wRowsPerLastStrip;
    WORD            wRowCount;
    CPIFILESPEC     CPIFileSpec;
    CPIIMAGESPEC    CPIImageSpec;
  
    /*                        Setup buffering stuff                           */ 
  
    if (! (lpIFileInfo = (LPFILEINFO)GlobalLock (hIFInfo)))
        return (EC_MEMORY1);
  
    if(! (lpEFileInfo = (LPFILEINFO)GlobalLock (hEFInfo)))
    {
        GlobalUnlock (hIFInfo);
        return (EC_MEMORY1);
    }
  
    if (! (lpIBitmapInfo = (LPBITMAPINFO) GlobalLock (hIBInfo)))
    {
        GlobalUnlock (hIFInfo);
        GlobalUnlock (hEFInfo);
        return (EC_MEMORY1);
    }
  
    if (! (lpEBitmapInfo = (LPBITMAPINFO) GlobalLock (hEBInfo)))
    {
        GlobalUnlock (hIBInfo);
        GlobalUnlock (hIFInfo);
        GlobalUnlock (hEFInfo);
        return (EC_MEMORY1);
    }
  
    wRowsPerStrip  =  lpIFileInfo->wRowsPerStrip;
    wBytesPerStrip =  wRowsPerStrip * lpEFileInfo->wPaddedScanWidth;
    wNumStrips     =  lpEFileInfo->wScanHeight / wRowsPerStrip;
    wRowCount      =  wNumStrips * wRowsPerStrip;
  
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
  


    InitCPITMPSpec ((LPCPIFILESPEC) &CPIFileSpec);

    if (WriteFile (hFile, (LPSTR) &CPIFileSpec, (LONG) sizeof (CPIFILESPEC)) != (LONG) sizeof (CPIFILESPEC))
        return (EC_FILEWRITE2);


    /*  Write out Image Spec stuff  */

    {
        int nBytes;

        InitCPITMPImageSpec ((LPCPIIMAGESPEC) &CPIImageSpec);

        UpdateCPIImageSpec ((LPCPIIMAGESPEC) &CPIImageSpec, lpEFileInfo);
    
        nBytes = sizeof (CPIIMAGESPEC);

        if (WriteFile (hFile, (LPSTR) &CPIImageSpec, (LONG) nBytes) != (LONG) nBytes)
            return (EC_FILEWRITE2);

	
    }

  
    dwCPiPalPos = tell (hFile);
  
    /*  Now do the palette                             */ 
  
    lpRGBQuadPtr = (RGBQUAD FAR *)GlobalLock(hExpPalette );
  
    TmpPtr = (LPSTR)TmpPal;
  
    for (i = 0; i < 256; i++)
    {   
        *TmpPtr++ = (char)((lpRGBQuadPtr -> rgbRed   >> 2));
        *TmpPtr++ = (char)((lpRGBQuadPtr -> rgbGreen >> 2));
        *TmpPtr++ = (char)((lpRGBQuadPtr -> rgbBlue  >> 2));
        lpRGBQuadPtr++;
    }
  
    GlobalUnlock (hExpPalette);
  
    if (WriteFile (hFile, (LPSTR) TmpPal, 768L) != 768L)
    {
        GlobalUnlock (hEBInfo);
        GlobalUnlock (hIBInfo);
        GlobalUnlock (hIFInfo);
        GlobalUnlock (hEFInfo);
        return (EC_FILEWRITE2);
    }   

  
  
  
  
    /*  Setup DIB (Windows 3.0 style) stuff*******************************/  
    {
      LPBITMAPINFOHEADER lpbmiHeader;
  
      lpbmiHeader = &lpEBitmapInfo->bmiHeader;
  
      lpbmiHeader->biSize     = (DWORD) sizeof (BITMAPINFOHEADER);
      lpbmiHeader->biWidth    = (DWORD) lpEFileInfo->wPaddedScanWidth;
      lpbmiHeader->biHeight   = (DWORD) lpEFileInfo->wRowsPerStrip; 
      lpbmiHeader->biPlanes   = 1;
      lpbmiHeader->biBitCount = lpEFileInfo->wBitsPerPixel;   
  
      lpbmiHeader->biCompression    = 0;
      lpbmiHeader->biSizeImage      = 0;
      lpbmiHeader->biXPelsPerMeter  = 0;
      lpbmiHeader->biYPelsPerMeter  = 0;
      lpbmiHeader->biClrUsed        = 0;
      lpbmiHeader->biClrImportant   = 0;
  
    }
  
  
    dwCPiDataPos = tell (hFile);
    GlobalUnlock (hEBInfo);
    GlobalUnlock (hIBInfo);
    GlobalUnlock (hIFInfo);
    GlobalUnlock (hEFInfo);
    return (0);
}

/*---------------------------------------------------------------------------

   PROCEDURE:         CPIWrConvertData
   DESCRIPTION;      Writes file
   DEFINITION:
   MODIFIED:            1/14/90

----------------------------------------------------------------------------*/

int FAR PASCAL CPIWrConvertData (hFile, lppT, lppD, lpEFileInfo, lpBitmapInfo)
int           hFile;
LPSTR FAR *   lppT;
LPSTR FAR *   lppD;
LPFILEINFO    lpEFileInfo;
LPBITMAPINFO  lpBitmapInfo;
{
    int              nRetval = TRUE;
    LPSTR            lpSrc;
    LPSTR            lpTmp;
    LPSTR            TmpPtr;
    LPSTR            SrcPtr;
    WORD             wBytesThisStrip;
    WORD             wPaddedBytesThisStrip;
    WORD             wRowsThisStrip;
    WORD             wBytesThisRow;
    WORD             wPaddedBytesThisRow;
    WORD             rows;
  
    lpSrc = *lppD;
    lpTmp = *lppT;
  
  
    if (wExportClass == IMON)
        lpEFileInfo -> wBytesPerRow = lpEFileInfo -> wScanWidth;    // Kludge !

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
    else
    {
        wBytesThisStrip          = lpEFileInfo->wBytesPerRow *
                                    lpEFileInfo->wRowsPerStrip;
        wPaddedBytesThisStrip = lpEFileInfo->wPaddedBytesPerRow *
                                      lpEFileInfo->wRowsPerStrip;
        wRowsThisStrip        = lpEFileInfo->wRowsPerStrip;
    }
  
    TmpPtr = lpTmp;
    SrcPtr = lpSrc;
  
    for (rows = 0; rows < wRowsThisStrip; rows++)
    {
        _fmemcpy (TmpPtr, SrcPtr, wBytesThisRow);
        TmpPtr += wBytesThisRow;
        SrcPtr += wPaddedBytesThisRow;
    }
  
    TmpPtr    = lpTmp;                    /* data now in dest so swap */
    lpTmp     = lpSrc;
    lpSrc     = TmpPtr;


    if (ePalType == GP16)
    {
        WORD i, j;
        LPSTR lpSourcePtr;
        LPSTR lpDestPtr;

        lpSourcePtr = lpSrc;
        lpDestPtr   = lpTmp;

        /*  Special case handler for GRAY 16 palette setting.  Just shift data out 4 bits  */

        for (i = 0; i < wRowsThisStrip; i++)
            for (j = 0; j < wBytesThisRow; j++)
                *lpDestPtr++ = (BYTE) (*lpSourcePtr++ << 4);

        TmpPtr    = lpTmp;                    
        lpTmp     = lpSrc;
        lpSrc     = TmpPtr;
    }

  
    if (wExportClass == IRGB)
    {
        RGBToBGR (lpTmp, lpSrc, wRowsThisStrip, lpEFileInfo->wScanWidth, wBytesThisRow);

        TmpPtr    = lpTmp;                    
        lpTmp     = lpSrc;
        lpSrc     = TmpPtr;
    }

  
    if (WriteFile (hFile, (LPSTR) lpSrc, (LONG) wBytesThisStrip) != (LONG) wBytesThisStrip)
        nRetval = EC_FILEWRITE2;
  
    return (nRetval);
}

/*----------------------------------------------------------------------------

   PROCEDURE:         CPIFixupHeader
   DESCRIPTION;      Finished Header
   DEFINITION:
   MODIFIED:            1/14/90

------------------------------------------------------------------------------*/

int FAR PASCAL  CPIFixupHeader  (hFile, lpBuffer, hIFInfo, hIBInfo, hEFInfo, hEBInfo )
int      hFile;
LPSTR    lpBuffer;
HANDLE   hIFInfo;
HANDLE   hEFInfo;
HANDLE   hIBInfo;
HANDLE   hEBInfo;
{
    CPIIMAGESPEC    ImgDescTable;
    int             nRetval = TRUE;
    WORD            bytes;
    int nBytes;
  
    /*          Finish up CPI file header information and close files         */
  
    if (_llseek( hFile, 64L, 0) == -1)
        return (EC_FILEWRITE1);
  
    bytes = _lread ((int)hFile, (LPSTR)&ImgDescTable, sizeof (CPIIMAGESPEC));
  
    if (((int) bytes == -1) || (bytes != sizeof (CPIIMAGESPEC)))
        return (EC_FILEREAD2);
  
    ImgDescTable.ImgDataOffset = dwCPiDataPos; 
    ImgDescTable.PalDataOffset = dwCPiPalPos; 
  
    if (_llseek (hFile, 64L, 0) == -1)
        return (EC_FILEWRITE1);

    nBytes = sizeof (CPIIMAGESPEC);

    if (WriteFile (hFile, (LPSTR) &ImgDescTable, (LONG) nBytes) != (LONG) nBytes)
        return (EC_FILEWRITE2);

  
    return (0);
}

 


void NEAR PASCAL UpdateCPIImageSpec (lpCPIImgSpec, lpFileInfo)
LPCPIIMAGESPEC  lpCPIImgSpec; 
LPFILEINFO      lpFileInfo;
{
    lpCPIImgSpec -> BitsPerPixel    = 8;   // Assume 8 bits by default
    lpCPIImgSpec -> NumberPlanes    = 1;
    lpCPIImgSpec -> ImgSize         = (DWORD)lpFileInfo->wScanWidth 
                                 * (DWORD)lpFileInfo->wScanHeight ;

    switch (wExportClass)
    {
        case ICM:
            lpCPIImgSpec -> ImgType  =  0;             //  Color
            break;

        case IGR:
            lpCPIImgSpec -> ImgType  =  1;             // Gray
            break;

        case IMON:
            lpCPIImgSpec -> ImgType  =  3;             // BW
            break;

        case IRGB:
            lpCPIImgSpec -> ImgType  =  5;             // RGB
            lpCPIImgSpec -> BitsPerPixel    = 24;
            lpCPIImgSpec -> ImgSize    = (DWORD)lpFileInfo->wScanWidth * 3
                                    * (DWORD)lpFileInfo->wScanHeight ;
            break;
    }


    lpCPIImgSpec -> X_Length     = lpFileInfo->wScanWidth;
    lpCPIImgSpec -> Y_Length     = lpFileInfo->wScanHeight;



    switch (ePalType)
    {
        case DP256:
        case IP256:
        case OP256:
        case GP256:
            lpCPIImgSpec -> NumberColors    = 256;
            break;

        case GP64:
            lpCPIImgSpec -> NumberColors    = 64;
            break;

        case DP16:
        case OP16:
        case GP16:
            lpCPIImgSpec -> NumberColors    = 16;
        break;

        case DP8:
        case OP8:
//      case GP8:
            lpCPIImgSpec -> NumberColors    = 8;
        break;

        case BP:
            lpCPIImgSpec -> NumberColors    = 2;
      break;
    }


// These HERE are messed up

    lpCPIImgSpec -> NumberPalEntries       = 256;

    lpCPIImgSpec -> NumberBitsPerEntry    = 24;
    lpCPIImgSpec -> AttrBitsPerEntry       = 0;
    lpCPIImgSpec -> FirstEntryIndex       = NULL;
    lpCPIImgSpec -> RGBSequence          = 0;
    lpCPIImgSpec -> NextDescriptor       = NULL;
    lpCPIImgSpec -> ApplSpecReserved       = NULL;        
}






void NEAR PASCAL InitCPITMPSpec (lpCPIFileSpec)
LPCPIFILESPEC lpCPIFileSpec;
{


    CPIFILESPEC CPIFileSpec;

 
    /*  Use a local copy to save code size  */

    _fmemcpy ((LPSTR) CPIFileSpec.CPiSignature, (LPSTR) "CPI", 4);
    CPIFileSpec.VersionNumber       = 0x200;
    CPIFileSpec.FileSpecLength      = sizeof (CPIFILESPEC);
    CPIFileSpec.ImageCount          = 1;
    CPIFileSpec.ImgDescTableOffset  = sizeof (CPIFILESPEC);
    CPIFileSpec.ApplSpecReserved    = NULL;

    _fmemset ((LPSTR) CPIFileSpec.Filler, 0xFF, sizeof (CPIFileSpec.Filler));

    /*  Now initialized, copy to dest buffer  */

    _fmemcpy ((LPSTR) lpCPIFileSpec, (LPSTR) &CPIFileSpec, sizeof (CPIFILESPEC));


}




void NEAR PASCAL InitCPITMPImageSpec (lpCPIImageSpec)
LPCPIIMAGESPEC lpCPIImageSpec;
{


    CPIIMAGESPEC CPIImageSpec;

 
    /*  Use a local copy to save code size  */

    /*  First set whole thing to zero  */

    _fmemset ((LPSTR) &CPIImageSpec, 0, sizeof (CPIIMAGESPEC));


    /*  Now fill in non-zero default values  */


    CPIImageSpec.ImgDescTag       = 1;
    CPIImageSpec.ImgSpecLength    = sizeof (CPIIMAGESPEC);
    CPIImageSpec.ImgSequence      = 1;  // This is all we have now


    _fmemset ((LPSTR) CPIImageSpec.Filler, 0xFF, sizeof (CPIImageSpec.Filler));

    /*  Now initialized, copy to dest buffer  */

    _fmemcpy ((LPSTR) lpCPIImageSpec, (LPSTR) &CPIImageSpec, sizeof (CPIIMAGESPEC));


}








