#include <windows.h>
#include <string.h>
#include <imgprep.h>
#include <strtable.h>
#include <proto.h>
#include <global.h>
#include <error.h>
#include <memory.h>   // For C 6.0 memory copy etc.
#include <filters.h>
#include <prometer.h>
#include <print.h>
#include <cpi.h>

#define CopyPrintDiscardResource(r)     _asm    mov   ax,r     \
                                        _asm    jmp   CopyPrintDiscardResource


int NEAR CreatePrintFileDIB (LPSTR FAR *, LPSTR FAR *, LPFILEINFO, LPBITMAPINFO);
int NEAR InitPrintFileInfo  (LPFILEINFO, LPFILEINFO);


/*-------------------------------------------------------------------------

CopyPrint ()

What it does (or is intended to do) should be self explanatory.


D. Ison  4/91

--------------------------------------------------------------------------*/


int FAR CopyPrint (hWnd, bCopy)
HANDLE  hWnd;
BOOL    bCopy;      // TRUE = Copy, FALSE = CUT
{

    /*  Locals also used as discard flags  */

    int (FAR PASCAL *lpfnRdConvertData)(int, LPSTR FAR *, LPSTR FAR *, LPFILEINFO, LPBITMAPINFO) = 0;
    int (FAR PASCAL *lpfnWrConvertData)(int, LPSTR FAR *, LPSTR FAR *, LPFILEINFO, LPBITMAPINFO) = 0;
    LPFILEINFO    lpFileInfo        = (LPFILEINFO) 0L;
    LPFILEINFO    lpExpFileInfo     = (LPFILEINFO) 0L;
    LPBITMAPINFO  lpBitmapInfo      = (LPBITMAPINFO) 0L;
    LPBITMAPINFO  lpExpBitmapInfo   = (LPBITMAPINFO) 0L;
    LPSTR         lpBufs            = (LPSTR) 0L;

    int           nReturn = 0;
    int           i;
    LPSTR         lpSource;
    LPSTR         lpDest;
    LPSTR         lpInputBuf;
    LPSTR         lpTmpBuf;
    WORD          wNumStrips;
    WORD          wBytesPerStrip;
    WORD          wRowsPerStrip;
    WORD          wScrnY;
    int           nPrintError;
    BOOL          bCopyTrueColor;
  
    /*  Lock down stuff  */

    if (! (hExpBitmapInfo = GlobalAlloc (GHND, (DWORD) sizeof (BITMAPINFOHEADER) + 1024L)))
        CopyPrintDiscardResource (EC_MEMORY2);
  
    if (! (lpExpBitmapInfo = (LPBITMAPINFO) GlobalLock (hExpBitmapInfo)))
        CopyPrintDiscardResource (EC_MEMORY1);

    if (! (hExpFileInfo = GlobalAlloc (GHND, 1024L)))
        CopyPrintDiscardResource (EC_MEMORY2);
  
    if (! (lpExpFileInfo = (LPFILEINFO)GlobalLock (hExpFileInfo)))
        CopyPrintDiscardResource (EC_MEMORY1);

    if (! (lpBitmapInfo = (LPBITMAPINFO)GlobalLock (hBitmapInfo)))
        CopyPrintDiscardResource (EC_MEMORY1);

    if (! (lpFileInfo = (LPFILEINFO)GlobalLock (hFileInfo)))
        CopyPrintDiscardResource (EC_MEMORY1);

    if (! (lpBufs = (LPSTR)GlobalLock (hGlobalBufs)))
        CopyPrintDiscardResource (EC_MEMORY1);

    /*  When input image is true color, if set to default palette no dither, 
        or running in true color display, copy true color.  */

//  if ((! wDither && ! wPal && wImportClass == IRGB) || bTrueColor)

    /*  Alt:  Default palette alone gets you true color to clipboard */
  if (             ! wPal && wImportClass == IRGB)
    {
        bCopyTrueColor = TRUE;
        SetState (SAVE_TYPE, XCPI24);
    }
    else
    {
        bCopyTrueColor = FALSE;
        SetState (SAVE_TYPE, XCPI);
    }
  
    SetExportStates ();

    InitPrintFileInfo (lpExpFileInfo, lpFileInfo);

    /*  Initialize Print stuff  */

    _fmemcpy ((LPSTR)lpExpBitmapInfo, (LPSTR)lpBitmapInfo, sizeof(BITMAPINFOHEADER)+ 1024);

    SetupFilters (lpFileInfo, EXPORT_CLASS);
    
    /*  Generate the palette   */

    {
        int nRetval;

        nRetval= GenPal (EXPORT_PAL, lpFileInfo, lpBitmapInfo, hImportFile);
        if ((int) nRetval < 0)
            CopyPrintDiscardResource (nRetval);
        else
            hExpPalette = (HANDLE) nRetval;
    }

    if (! bCopyTrueColor)
    {
        LPSTR lpExpPalette;
        LPSTR lpOrgPalette;

        lpExpPalette = GlobalLock (hExpPalette);
        ColorCorrectPalette ((RGBQUAD FAR *) lpExpPalette);
        GlobalUnlock (hExpPalette);

        if (wImportClass == ICM)   /*  For de-indexing  */
        {
            lpOrgPalette = GlobalLock (hGlobalPalette);
            _fmemcpy ((LPSTR) lpOrgPalette, (LPSTR)lpBitmapInfo->bmiColors, 1024);
            ColorCorrectPalette ((RGBQUAD FAR *) lpOrgPalette);
            GlobalUnlock (hGlobalPalette);
        }

        /*  Also remember the clipboard  */

        if (nPrintError = BeginPrintSave (lpFileInfo -> wScanWidth, lpFileInfo -> wScanHeight, 8, (lpExpFileInfo -> wBitsPerPixel >> 3), 256))
        {
            PrintDisplayErrorMsg (hWnd, nPrintError);
            CopyPrintDiscardResource (USER_ABANDON);
        }

        if (nPrintError = SavePrintPalette (256, (RGBQUAD FAR *) lpExpPalette))
        {
            PrintDisplayErrorMsg (hWnd, nPrintError);
            CopyPrintDiscardResource (USER_ABANDON);
        }

    }
    else {
        if (nPrintError = BeginPrintSave (lpFileInfo -> wScanWidth, lpFileInfo -> wScanHeight, 8, (lpExpFileInfo -> wBitsPerPixel >> 3), 0))
        {
            PrintDisplayErrorMsg (hWnd, nPrintError);
            CopyPrintDiscardResource (USER_ABANDON);
        }


    }
    
    /*   Prepare to read write     */
  
    lpFileInfo    ->  bIsLastStrip = FALSE;
    lpExpFileInfo ->  bIsLastStrip = FALSE;
  
    i                   = 0; 
    wScrnY              = 0;
    wNumStrips          = lpFileInfo->wNumStrips;
    wBytesPerStrip      = lpFileInfo->wBytesPerStrip;
    wRowsPerStrip       = lpFileInfo->wRowsPerStrip;
  
    if (_llseek ((int) hImportFile, (DWORD) lpFileInfo -> dwDataOffset, 0) == -1)
        CopyPrintDiscardResource (EC_FILEREAD1);
  
    lpfnRdConvertData = GetRCD (wImportType);

    lpSource    = lpInputBuf  = lpBufs;
    lpDest      = lpTmpBuf    = (BYTE huge *) lpBufs + LARGEBUFOFFSET;

    /*  Setup Progress meter to watch.. */

    {
        PSTR pStringBuf;
        int  nString1;
        int  nString2;

        nString1 = STR_PRINT;
        nString2 = STR_PRINTING;

        pStringBuf  = LocalLock (hStringBuf);
        EnableWindow (hWndIP, FALSE);
        LoadString (hInstIP, nString1, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        ProOpen (hWndIP, NULL, MakeProcInstance (Abandon, hInstIP), (LPSTR) pStringBuf);
        ProSetBarRange (wNumStrips);
        LoadString (hInstIP, nString2, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        ProSetText (ID_STATUS1, (LPSTR) pStringBuf);
        LocalUnlock (hStringBuf);
    }

    bAbandon = FALSE;

    for (i = 0; i < (int)wNumStrips - 1 && ! bAbandon; i++)
    {
        if (! bAbandon)
        {
            nReturn = (*lpfnRdConvertData) (hImportFile,
                                            &lpDest,
                                            &lpSource,
                                            lpFileInfo, 
                                            lpBitmapInfo);

            if (nReturn < 0)
                CopyPrintDiscardResource (EC_FILEREAD2);

            ColorCorrectRGB (lpSource, lpDest, lpFileInfo);
  
            nReturn = CreatePrintFileDIB (&lpDest, &lpSource, lpFileInfo, lpBitmapInfo);
            if (nReturn < 0)
                CopyPrintDiscardResource (EC_ERROR);

            InvertScanlines (lpSource, lpDest, lpExpFileInfo->wPaddedBytesPerRow,
                lpFileInfo->wRowsPerStrip);

            if (nPrintError = SavePrintData (wScrnY, lpFileInfo -> wRowsPerStrip, lpSource))
            {
                PrintDisplayErrorMsg (hWnd, nPrintError);
                CopyPrintDiscardResource (USER_ABANDON);
            }

            ProDeltaPos (1);

        }

        wScrnY += wRowsPerStrip;

    }
  
    /*   Mark the last strip for the formats that need to know   */
  
    lpFileInfo->bIsLastStrip = TRUE;
    lpSource    = lpInputBuf  = lpBufs;
    lpDest      = lpTmpBuf    = (BYTE huge *) lpBufs + LARGEBUFOFFSET;
    nReturn = (*lpfnRdConvertData) (hImportFile,
                                    &lpDest,
                                    &lpSource,
                                    lpFileInfo, 
                                    lpBitmapInfo);

    if (nReturn < 0)
        CopyPrintDiscardResource (EC_FILEREAD2);

        ColorCorrectRGB (lpSource, lpDest, lpFileInfo);

        nReturn = CreatePrintFileDIB (&lpDest, &lpSource, lpFileInfo, lpBitmapInfo);
      
        if (nReturn < 0)
            CopyPrintDiscardResource (EC_ERROR);
        InvertScanlines (lpSource, lpDest, lpExpFileInfo->wPaddedBytesPerRow,
                         lpFileInfo->wRowsPerLastStrip);

        if (nPrintError = SavePrintData (wScrnY, lpFileInfo -> wRowsPerLastStrip, lpSource))
        {
            PrintDisplayErrorMsg (hWnd, nPrintError);
            CopyPrintDiscardResource (USER_ABANDON);
        }

        if (nReturn < 0)
            CopyPrintDiscardResource (EC_FILEWRITE2);


    if (! bAbandon)
        ProDeltaPos (1);

    /*  Otherwise successful completion.  Return a passing value (0)  */

    CopyPrintDiscardResource (0);


/*---  CopyPrint Resource Discard Section  ---*/

    {

        int nRetVal;

        CopyPrintDiscardResource:

        _asm    mov nRetVal,ax


        /*  Do any local memory handles allocated in this fn that would be freed anyway  */

        /*  Use the pointers as a flag of the global handles being locked  */

        if (lpFileInfo)
            GlobalUnlock (hFileInfo);

        if (lpBitmapInfo)
            GlobalUnlock (hBitmapInfo);

        if (lpExpFileInfo)
            GlobalUnlock (hExpFileInfo);

        if (lpExpBitmapInfo)
            GlobalUnlock (hExpBitmapInfo);

        if (lpBufs)
            GlobalUnlock (hGlobalBufs);



        /*  Do global memory handles allocated in this fn that would not be freed anyway  */

        GLOBALFREE (hExpBitmapInfo);
        GLOBALFREE (hExpFileInfo);
        GLOBALFREE (hExpPalette);
        
        /*  Do other general cleanup  */


        if ((DWORD) lpfnRdConvertData > 0L)
            FreeProcInstance (lpfnRdConvertData);

        if ((DWORD) lpfnWrConvertData > 0L)
            FreeProcInstance (lpfnWrConvertData);


        FreeFilters (hExpFileInfo, EXPORT_CLASS);

        hExportFile = 0;


        EnableWindow (hWndIP, TRUE);
        ProClose ();   // Close down Progress meter

        if (bAbandon)
            nRetVal = USER_ABANDON;



        if (nRetVal == 0)       // No error so far
        {
            /*  nRetval below means if there was already and error we better say TRUE on EndPrintSave  */

            if (nPrintError = EndPrintSave (hWnd, (! nRetVal)))
            {
                PrintDisplayErrorMsg (hWnd, nPrintError);
                return (USER_ABANDON);
            }

            if (nPrintError = SendPage (hWnd, (LPSTR) szImageName)) {
                PrintDisplayErrorMsg(hWnd, nPrintError);
                return USER_ABANDON;
            }
        
            return (nRetVal);
        }
    }


}


int NEAR InitPrintFileInfo (lpEFileInfo, lpIFileInfo)
LPFILEINFO lpEFileInfo;
LPFILEINFO lpIFileInfo;
{

  /*  1st copy In to Export  (ok) */

  _fmemcpy ((LPSTR)lpEFileInfo, (LPSTR)lpIFileInfo, 1024);

  /*   Modify Export to Reflect changes   */

  if (wImportClass != wExportClass)
  {
    switch (wImportClass)
    {
      case IRGB:
        switch (wExportClass)
        {
          case ICM:
            lpEFileInfo->wBitsPerPixel       = 8;
            lpEFileInfo->wBytesPerRow        = lpEFileInfo->wScanWidth;
            lpEFileInfo->wPaddedBytesPerRow  = ((((lpEFileInfo->wScanWidth * INTERNAL_BITS) + 31) / 32) * 4);

            break;

          case IGR:
            lpEFileInfo->wBitsPerPixel       = 8;
            lpEFileInfo->wBytesPerRow        = lpEFileInfo->wScanWidth;
            lpEFileInfo->wPaddedBytesPerRow  = ((((lpEFileInfo->wScanWidth * INTERNAL_BITS) + 31) / 32) * 4);
            break;
          case IMON:
            lpEFileInfo->wBitsPerPixel       = 8;
            lpEFileInfo->wBytesPerRow        = lpEFileInfo->wScanWidth;
            lpEFileInfo->wPaddedBytesPerRow  = ((((lpEFileInfo->wScanWidth * INTERNAL_BITS) + 31) / 32) * 4);
            break;

          default:
            lpEFileInfo->wBitsPerPixel       = 8;
            lpEFileInfo->wBytesPerRow        = lpEFileInfo->wScanWidth;
            lpEFileInfo->wPaddedBytesPerRow  = ((((lpEFileInfo->wScanWidth * INTERNAL_BITS) + 31) / 32) * 4);
            break;
        }
        break;

      case ICM:
        switch (wExportClass) 
        {
          case IRGB:
            lpEFileInfo->wBitsPerPixel    = 24;
            lpEFileInfo->wBytesPerRow     = lpEFileInfo->wScanWidth * 3;
            lpEFileInfo->wPaddedBytesPerRow   *= 3;
            break;
          default:
            break;
        }
        break;
      case IGR:
        switch (wExportClass)
        {
          case IMON:
            break;
          default:
            break;
        }
        break;
      case IMON:
        break;
    }
  }
    if (wImportClass == IMON && wExportClass == IMON)
        lpEFileInfo->wPaddedBytesPerRow  = ((((lpEFileInfo->wScanWidth * INTERNAL_BITS) + 31) / 32) * 4);

  lpEFileInfo->wRowsPerStrip     = lpIFileInfo->wRowsPerStrip;
  lpEFileInfo->wRowsPerLastStrip = lpIFileInfo->wRowsPerLastStrip;
  return (0);
}


int NEAR CreatePrintFileDIB (lpfpDest, lpfpSource, lpFileInfo, lpBitmapInfo)
LPSTR FAR *      lpfpDest;
LPSTR FAR *      lpfpSource;
LPFILEINFO       lpFileInfo;
LPBITMAPINFO     lpBitmapInfo;
{
    DITHERBAYER FAR *   lpDB;
    int (FAR PASCAL *lpfnFilter)(LPSTR FAR *, LPSTR FAR *, LPSTR);
    FMT8TO24 FAR *      lp8to24;
    int                 n = 0;  

    LPSTR               lpFS;
    
    LPSTR               lpSource;
    LPSTR               lpDest;
    LPSTR               lpTmp;


    lpSource = *lpfpDest;
    lpDest   = *lpfpSource;

  
    while (ExportFilters[n])
    {
        lpfnFilter = GetFilterProc (ExportFilters[n]);

        if (lpFileInfo->bIsLastStrip)
            lpFS = (LPSTR)GlobalLock (ExportFilterStruct[n][1]);
        else
            lpFS = (LPSTR)GlobalLock (ExportFilterStruct[n][0]);

        switch (ExportFilters[n])
        {
            case DIDX:
                lp8to24 = (FMT8TO24 FAR *) lpFS; 
            //  lp8to24->lpPalette = (LPSTR) lpBitmapInfo->bmiColors;
                break;

        }


        lpTmp    = lpDest;
        lpDest   = lpSource;
        lpSource = lpTmp;


        (*lpfnFilter)((LPSTR FAR *) &lpDest,
                      (LPSTR FAR *) &lpSource,
                      lpFS);

        switch (ExportFilters[n])
        {
            case BAY8:   
            case GBAY:   
                lpDB = (DITHERBAYER FAR *)lpFS;
                lpDB->wScrnY += lpFileInfo->wRowsPerStrip;
            break;
        }
        n++;

        if (lpFileInfo->bIsLastStrip)
            GlobalUnlock (ExportFilterStruct[n][1]);
        else
            GlobalUnlock (ExportFilterStruct[n][0]);

        FreeProcInstance (lpfnFilter);
    }

    *lpfpDest   = lpDest;
    *lpfpSource = lpSource;


    return (0);
}

