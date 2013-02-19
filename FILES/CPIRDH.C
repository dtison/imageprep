/*----------------------------------------------------------------------------

   COPYRIGHT:      Copyright (c) 1990, Computer Presentations, Inc.
                   All rights reserved.

   PROJECT:        Image Prep 3.0

   MODULE:         cpirdh.c

   PROCEDURES:     CPiReadHeader()   5.4.1

-----------------------------------------------------------------------------*/                                          
#include <windows.h>
#include <io.h>
#include <cpi.h>
#include <imgprep.h>
#include <proto.h>
#include <error.h>
#include <cpifmt.h>
#include <memory.h>
#include <global.h>

BOOL ReadCPiFileSpec (int, LPCPIFILESPEC, LPFILEINFO);
BOOL ReadCPiImageSpec (int, LPCPIIMAGESPEC, LPSTR, RGBQUAD FAR *, LPFILEINFO);

/*----------------------------------------------------------------------------

   PROCEDURE:         CPiReadHeader
   DESCRIPTION:       Message box procedure for ImgPrep
   DEFINITION:        5.4.1
   START:             David Ison
   MODS:              1/5/90  Tom Bagford Jr.   ( to new spec )
                      1/24/90   Tom Bagford Jr.   (remove global pointers)
                      7/9/90  Started straightening out everything..!
                      7/10/90   Corrected "wPaddedScanWidth" mess

-----------------------------------------------------------------------------*/



int FAR PASCAL CPIReadHeader (hFile, lpBitmapInfo, lpInputBuf, lpFileInfo)
int             hFile;
LPBITMAPINFO    lpBitmapInfo;
LPSTR           lpInputBuf;
LPFILEINFO      lpFileInfo;
{
    CPIFILESPEC           CPiFileSpec;
    CPIIMAGESPEC          CPiImageSpec;
    int                   ret_val= 0;                                   /* OK */
    LPSTR                 lpTmpBuf;
    RGBQUAD FAR *         lpRGBQuadPtr;
    WORD                  wBytesPerStrip;
    WORD                  wRowsPerStrip;
    WORD                  wNumStrips;
    WORD                  wBytesPerLastStrip;
    WORD                  wRowsPerLastStrip;
    WORD                  wRowCount;
    LPCPIFIXUP            lpCPIFixup;
    
    if (_llseek (hFile, 0L, 0))
        return (EC_MEMORY1);

    lpCPIFixup  = (LPCPIFIXUP) ((LPSTR) lpFileInfo + sizeof (FILEINFO));

    lpTmpBuf = (LPSTR) ((BYTE huge *) lpInputBuf + BUFOFFSET);

    lpRGBQuadPtr = ( RGBQUAD FAR * )&lpBitmapInfo ->bmiColors[0];

    if( ! ReadCPiFileSpec (hFile, (LPCPIFILESPEC) &CPiFileSpec, lpFileInfo))
    {
        return (EC_FILEREAD2);
    }
   
    if (! ReadCPiImageSpec (hFile, (LPCPIIMAGESPEC) &CPiImageSpec, lpTmpBuf, 
                           lpRGBQuadPtr, lpFileInfo))
    {
        return (EC_FILEREAD2);
    }

    /*  Setup buffering stuff for read                     */ 
    {
        WORD wStripSize;

        // Set up max strip size

        switch (wImportClass)
        {
            case IMON:
                wStripSize = MAXBUFSIZE >> 3;
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


    /*                     Little Picture Fix                               */

    if (wRowsPerStrip > lpFileInfo->wScanHeight)
       wRowsPerStrip = lpFileInfo->wScanHeight;

    wBytesPerStrip = wRowsPerStrip * lpFileInfo->wBytesPerRow;

    wNumStrips = lpFileInfo->wScanHeight / wRowsPerStrip;
    wRowCount = wNumStrips * wRowsPerStrip;

    if (wRowCount < lpFileInfo->wScanHeight)
    {
       wBytesPerLastStrip  = (lpFileInfo-> wScanHeight - wRowCount) * 
                             lpFileInfo->wBytesPerRow;

       wRowsPerLastStrip   = (lpFileInfo->wScanHeight - wRowCount);
       wNumStrips++;
    }
    else
    {
      wBytesPerLastStrip = wBytesPerStrip;
      wRowsPerLastStrip  = wRowsPerStrip;
    }

    lpFileInfo->wNumStrips          = wNumStrips;
    lpFileInfo->wBytesPerStrip      = wBytesPerStrip;
    lpFileInfo->wRowsPerStrip       = wRowsPerStrip;
    lpFileInfo->wBytesPerLastStrip  = wBytesPerLastStrip;
    lpFileInfo->wRowsPerLastStrip   = wRowsPerLastStrip;
    

  // WHY NOT ????

    /**************** Setup DIB (Windows 3.0 style) stuff ******************
    
    lpbmiHeader             = &lpBitmapInfo->bmiHeader;
    lpbmiHeader->biSize     = (DWORD) sizeof (BITMAPINFOHEADER);
    lpbmiHeader->biWidth    = (DWORD) lpFileInfo->wScanWidth;
    lpbmiHeader->biHeight   = (DWORD) lpFileInfo->wRowsPerStrip; 
    lpbmiHeader->biPlanes   = 1;
    lpbmiHeader->biBitCount = lpFileInfo->wBitsPerPixel;   
    
    lpbmiHeader->biStyle          = 0;
    lpbmiHeader->biSizeImage      = 0;
    lpbmiHeader->biXPelsPerMeter  = 0;
    lpbmiHeader->biYPelsPerMeter  = 0;
    lpbmiHeader->biClrUsed        = 0;
    lpbmiHeader->biClrImportant   = 0;
    
    ************************************************************************/
    
    lpFileInfo->bIsLastStrip      = FALSE;           /* Flag the last "strip" */
    return (ret_val);
}   



/*----------------------------------------------------------------------------

   PROCEDURE:        ReadCPiFileSpec
   DESCRIPTION:      Read the file spec from the CPi File
   DEFINITION:       5.4.1.1
   START:            David Ison

-----------------------------------------------------------------------------*/

BOOL ReadCPiFileSpec (hFile, lpCPiFileSpec, lpFileInfo)
int             hFile;
LPCPIFILESPEC   lpCPiFileSpec;
LPFILEINFO      lpFileInfo;
{
    LONG  lNumBytes;


    lNumBytes = (LONG) sizeof (CPIFILESPEC); 

    if (ReadFile (hFile, (LPSTR) lpCPiFileSpec, lNumBytes) != lNumBytes)
        return (FALSE);

    return (TRUE);
} 

/*----------------------------------------------------------------------------

   PROCEDURE:        ReadCPiImageSpec
   DESCRIPTION:      Read the image spec from the CPi File
   DEFINITION:       5.4.1.2
   START:            David Ison
   MODS:             1/5/90  Tom Bagford Jr.   ( to new spec )
                     8/13/90 David Ison

----------------------------------------------------------------------------*/

BOOL ReadCPiImageSpec (hFile, lpCPIImageSpec, lpPaletteBuf, lpRGBQuadPtr, lpFileInfo) 
int                hFile;
LPCPIIMAGESPEC     lpCPIImageSpec;
LPSTR              lpPaletteBuf;
RGBQUAD FAR *      lpRGBQuadPtr;
LPFILEINFO         lpFileInfo;
{
    int           i;
    LPCPIFIXUP    lpCPIFixup;
    LPDISPINFO    lpDispInfo;
    LPSTR         TmpPtr;
    WORD          wBytes;
    LONG          lNumBytes;


    lpCPIFixup  = (LPCPIFIXUP) ((LPSTR) lpFileInfo + sizeof (FILEINFO));
    lpDispInfo  = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);

    lNumBytes = (LONG) sizeof (CPIIMAGESPEC); 

    if (ReadFile (hFile, (LPSTR) lpCPIImageSpec, lNumBytes) != lNumBytes)
        return (FALSE);


    if (lpCPIImageSpec -> BitsPerPixel == 1)
        lpCPIImageSpec -> BitsPerPixel = 8;

    lpFileInfo -> wBitsPerPixel       = lpCPIImageSpec->BitsPerPixel;
    lpFileInfo -> wScanWidth          = lpCPIImageSpec->X_Length;
    lpFileInfo -> wScanHeight         = lpCPIImageSpec->Y_Length;


    if (lpCPIImageSpec -> ImgType == CPIFMT_MONO)
    {
        lpFileInfo -> wBytesPerRow = (lpFileInfo -> wScanWidth + 31) / 32 * 4;  // DWORD aligned
        lpFileInfo -> wPaddedBytesPerRow  = lpFileInfo -> wBytesPerRow;
    }
    else
    {
        lpFileInfo -> wBytesPerRow = lpFileInfo -> wScanWidth * (lpFileInfo -> wBitsPerPixel >> 3);
        lpFileInfo -> wPaddedBytesPerRow  = (WORD) ((((DWORD) lpFileInfo -> wScanWidth * (DWORD) lpFileInfo -> wBitsPerPixel) + 31L) / 32L * 4L);
    }



    lpDispInfo -> wPaddedScanWidth    = ((lpFileInfo -> wScanWidth * INTERNAL_BITS) + 31) / 32 * 4;

    lpDispInfo -> wPaddedBytesPerRow  = lpDispInfo -> wPaddedScanWidth;  // FOR NOW THEY ARE EQUAL BUT IN 24 BIT DISPLAY THIS WILL BE REFINED
                  

    lpCPIFixup->bIsLinear           = lpCPIImageSpec->StorageType;

    _fmemset ((LPSTR) lpRGBQuadPtr, 0, 1024);

    if (lpCPIImageSpec -> ImgType == CPIFMT_COLOR || lpCPIImageSpec -> ImgType == CPIFMT_GRAY)
    {

        /* Read and reformat palette if 8bit and this is...          */

        if (_llseek ((int) hFile, (long) lpCPIImageSpec->PalDataOffset, 0) == -1)
            return (FALSE);

        wBytes = _lread ((int) hFile, (LPSTR)lpPaletteBuf, 768);

        if (wBytes != 768)
            return (FALSE);

        TmpPtr = lpPaletteBuf;

        for (i = 0; i < 256; i++)                        // THIS IS 8 BIT only!
        {   
            lpRGBQuadPtr->rgbRed    = (BYTE)((int)*TmpPtr++ << 2);
            lpRGBQuadPtr->rgbGreen  = (BYTE)((int)*TmpPtr++ << 2);
            lpRGBQuadPtr->rgbBlue   = (BYTE)((int)*TmpPtr++ << 2);   

            lpRGBQuadPtr++;
        }

    }

    /*  Point to image data  */

    if (_llseek ((int) hFile, (long) lpCPIImageSpec->ImgDataOffset, 0) == -1)
        return (FALSE);

    lpFileInfo->dwDataOffset = (long) tell (hFile);

    switch (lpFileInfo -> wBitsPerPixel)
    {
        case 24:
            SetState (IMPORT_CLASS, IRGB);
            break;

        case 8:
            switch (lpCPIImageSpec -> ImgType)
            {
                case CPIFMT_COLOR:
                    SetState (IMPORT_CLASS, ICM);  
                    break;

                case CPIFMT_GRAY:
                    SetState (IMPORT_CLASS, IGR);  
                    break;
            }
            break;

        case 1:      
            SetState (IMPORT_CLASS, IMON);
            break;
    }

    if (lpCPIImageSpec -> ImgType == CPIFMT_BW || lpCPIImageSpec -> ImgType == CPIFMT_MONO)
        SetState (IMPORT_CLASS, IMON);

    lpCPIFixup -> wCPIType = lpCPIImageSpec -> ImgType;




  return (TRUE);
} 


