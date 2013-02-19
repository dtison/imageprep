
#include <windows.h>
#include <string.h>
#include <imgprep.h>
#include <resource.h>
#include <strtable.h>
#include <proto.h>
#include <global.h>
#include <error.h>
#include <memory.h>   // For C 6.0 memory copy etc.
#include <filters.h>
#include <prometer.h>
#include "cpi.h"



#define ExportDiscardResource(r)      _asm    mov   ax,r     \
                                      _asm    jmp   ExportDiscardResource


int NEAR CreateFileDIB (LPSTR FAR *, LPSTR FAR *, LPFILEINFO, LPBITMAPINFO);



/*----------------------------------------------------------------------------

   PROCEDURE:         ExportFile
   DESCRIPTION:       Export the opened file and display it
   DEFINITION:        2.2.2
   START:             1/4/90  Tom Bagford Jr.   (stub)
   MODS:              1/24/90 Tom Bagford Jr. fix handle ptr issue

        Get rid of "hImp" this and that!   Ison

                      7/10/90  Corrected wPaddedScanWidth stuff.
                               Put in "DiscardResource" proc section.     D. Ison

-----------------------------------------------------------------------------*/

int ExportFile (int nFlags)
{
    /*  Locals also used as discard flags  */
    int (FAR PASCAL *lpfnRdConvertData)(int, LPSTR FAR *, LPSTR FAR *, LPFILEINFO, LPBITMAPINFO) = 0;
    int (FAR PASCAL *lpfnWrConvertData)(int, LPSTR FAR *, LPSTR FAR *, LPFILEINFO, LPBITMAPINFO) = 0;
    int (FAR PASCAL *lpfnInitHeader)   (int, LPSTR, HANDLE, HANDLE, HANDLE, HANDLE) = 0;
    int (FAR PASCAL *lpfnFixupHeader)  (int, LPSTR, HANDLE, HANDLE, HANDLE, HANDLE) = 0;
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
    WORD          wPaddedBytesPerStrip;
    WORD          wRowsPerStrip;
    WORD          wScrnY;
    DWORD         dwImageSize;
  
/// hExpBitmapInfo, hExpFileInfo NEED TO GO AWAY as globals!!!

    /*  Lock down stuff  */

    if (! (hExpBitmapInfo = GlobalAlloc (GHND, (DWORD) sizeof (BITMAPINFOHEADER) + 1024L)))
        ExportDiscardResource (EC_MEMORY2);
  
    if (!(lpExpBitmapInfo = (LPBITMAPINFO) GlobalLock (hExpBitmapInfo)))
        ExportDiscardResource (EC_MEMORY1);

    if (!(hExpFileInfo = GlobalAlloc (GHND, 1024L)))
        ExportDiscardResource (EC_MEMORY2);
  
    if (! (lpExpFileInfo = (LPFILEINFO)GlobalLock (hExpFileInfo)))
        ExportDiscardResource (EC_MEMORY1);

    if (! (lpBitmapInfo = (LPBITMAPINFO)GlobalLock (hBitmapInfo)))
        ExportDiscardResource (EC_MEMORY1);

    if (! (lpFileInfo = (LPFILEINFO)GlobalLock (hFileInfo)))
        ExportDiscardResource (EC_MEMORY1);

    if (! (lpBufs = (LPSTR)GlobalLock (hGlobalBufs)))
        ExportDiscardResource (EC_MEMORY1);

  
    SetExportStates ();

    InitExpFileInfo (lpExpFileInfo, lpFileInfo);

    /*  Check available disk space     */
    dwImageSize = lpExpFileInfo->wScanWidth * lpExpFileInfo->wBitsPerPixel;
    dwImageSize = ((((DWORD) dwImageSize + 31L) / 32L) * 4L);
    dwImageSize *= (DWORD) lpExpFileInfo->wScanHeight;

    if (! IsDiskSpaceAvailable ((LPSTR)szSaveFileName, dwImageSize))
        ExportDiscardResource (EC_NODISK);

    _fmemcpy ((LPSTR)lpExpBitmapInfo, (LPSTR)lpBitmapInfo, sizeof(BITMAPINFOHEADER)+ 1024);

    SetupFilters (lpFileInfo, EXPORT_CLASS);
    
    /*  Generate the palette   */

    {
        int nRetval;

        nRetval= GenPal (EXPORT_PAL, lpFileInfo, lpBitmapInfo, hImportFile);
        if ((int) nRetval < 0)
            ExportDiscardResource (nRetval);
        else
            hExpPalette = (HANDLE) nRetval;
    }

//  if (wImportClass == ICM)
    if (wImportClass == ICM && nFlags == SAVE_NORMAL)
    {
        LPSTR lpExpPalette;
        LPSTR lpOrgPalette;

        if (! wPal)
        {
            lpExpPalette = GlobalLock (hExpPalette);
            ColorCorrectPalette ((RGBQUAD FAR *) lpExpPalette);
            GlobalUnlock (hExpPalette);
        }

        lpOrgPalette = GlobalLock (hGlobalPalette);
        _fmemcpy ((LPSTR) lpOrgPalette, (LPSTR)lpBitmapInfo->bmiColors, 1024);
        ColorCorrectPalette ((RGBQUAD FAR *) lpOrgPalette);
        GlobalUnlock (hGlobalPalette);
    }
  
    lpfnInitHeader = GetIHdr (wExportType);

    nReturn = (*lpfnInitHeader) (hExportFile, 
                                  lpBufs,
                                  hFileInfo,
                                  hBitmapInfo,
                                  hExpFileInfo,
                                  hExpBitmapInfo); 
  
    if (nReturn != 0)
        ExportDiscardResource (EC_RDHDR);

    /*   Prepare to read write     */
  
    lpFileInfo    ->  bIsLastStrip = FALSE;
    lpExpFileInfo ->  bIsLastStrip = FALSE;
  
    i                       = 0; 
    wScrnY                  = 0;
    wNumStrips              = lpFileInfo->wNumStrips;
//  wPaddedBytesPerStrip    = lpFileInfo-> wPaddedBytesPerRow * lpFileInfo -> wRowsPerStrip;
    wPaddedBytesPerStrip    = (((lpFileInfo-> wScanWidth * lpFileInfo -> wBitsPerPixel + 31) / 32) * 4) * lpFileInfo -> wRowsPerStrip;

    wRowsPerStrip           = lpFileInfo->wRowsPerStrip;
  
    if (_llseek ((int) hImportFile, (DWORD) lpFileInfo -> dwDataOffset, 0) == -1)
        ExportDiscardResource (EC_FILEREAD1);

    /*  Setup status report  */
    if (nFlags == SAVE_NORMAL)
    { 
      FARPROC  lpfnDlg;
      BOOL     success;

      lpfnDlg = MakeProcInstance ((FARPROC)SaveReportProc, hInstIP);
      success = DialogBox (hInstIP, "SAVERPT", hWndIP, lpfnDlg);
      FreeProcInstance (lpfnDlg);
      if (!success){ 
        bAbandon = TRUE;
        ExportDiscardResource (0);
      }
    }

    lpfnRdConvertData = GetRCD (wImportType);
    lpfnWrConvertData = GetWCD (wExportType);    

    lpSource    = lpInputBuf  = lpBufs;
    lpDest      = lpTmpBuf    = (BYTE huge *) lpBufs + LARGEBUFOFFSET;


    /*  Setup Progress meter to watch.. */

    {
        PSTR pStringBuf;

        pStringBuf  = LocalLock (hStringBuf);
        EnableWindow (hWndIP, FALSE);
        if (nFlags == SAVE_NORMAL)
            LoadString (hInstIP, STR_SAVE_AS, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        else
            LoadString (hInstIP, STR_OPEN, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        ProOpen (hWndIP, NULL, MakeProcInstance (Abandon, hInstIP), (LPSTR) pStringBuf);
        ProSetBarRange (wNumStrips);
        if (nFlags == SAVE_NORMAL)
            LoadString (hInstIP, STR_SAVING_IMAGE, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        else
            LoadString (hInstIP, STR_CONVERTING, (LPSTR) pStringBuf, MAXSTRINGSIZE);

        ProSetText (ID_STATUS1, (LPSTR) pStringBuf);
        LocalUnlock (hStringBuf);

    }

    #ifdef FIXDVA
    /*  Reset fileinfo before beginning read  (Put in to fix save from DVA)  */

    {
        int (FAR PASCAL * lpfnReadHeader)(int, LPBITMAPINFO, LPSTR, LPFILEINFO) = 0;

        if (lpfnReadHeader = GetRHdr (wImportType))
        {
            if ((nReturn = (*lpfnReadHeader)(hImportFile, lpBitmapInfo, lpBufs, lpFileInfo)) < 0) 
            {
                FreeProcInstance (lpfnReadHeader);
                ExportDiscardResource (EC_FILEREAD1);
            }
            FreeProcInstance (lpfnReadHeader);
        }
    }
    #endif


    bAbandon = FALSE;

    for (i = 0; i < (int)wNumStrips - 1 && ! bAbandon; i++)
    {
        if (! bAbandon)
        {
            nReturn = (*lpfnRdConvertData) (hImportFile, &lpDest, &lpSource, lpFileInfo, lpBitmapInfo);
            if (nReturn < 0)
                ExportDiscardResource (EC_FILEREAD2);

            if (nFlags == SAVE_NORMAL)      // Color correct only if NOT auto-converting
                ColorCorrectRGB (lpSource, lpDest, lpFileInfo);
            else
                _fmemcpy (lpSource, lpDest, wPaddedBytesPerStrip);

            nReturn = CreateFileDIB (&lpDest, &lpSource, lpFileInfo, lpBitmapInfo);
            if (nReturn < 0)
                ExportDiscardResource (EC_ERROR);

            nReturn = (*lpfnWrConvertData) (hExportFile, &lpSource, &lpDest, lpExpFileInfo, lpExpBitmapInfo);
            if (nReturn < 0)
                ExportDiscardResource (EC_FILEWRITE2);

            ProDeltaPos (1);

        }

        wScrnY += wRowsPerStrip;
    }

    /*   Mark the last strip for the formats that need to know   */
  
    lpFileInfo->bIsLastStrip = TRUE;
    lpExpFileInfo->bIsLastStrip = TRUE;
    lpSource    = lpInputBuf  = lpBufs;
    lpDest      = lpTmpBuf    = (BYTE huge *) lpBufs + LARGEBUFOFFSET;
    nReturn = (*lpfnRdConvertData) (hImportFile, &lpDest, &lpSource, lpFileInfo, lpBitmapInfo);

    if (nReturn < 0)
        ExportDiscardResource (EC_FILEREAD2);

//      ColorCorrectRGB (lpSource, lpDest, lpFileInfo);

        if (nFlags == SAVE_NORMAL)      
            ColorCorrectRGB (lpSource, lpDest, lpFileInfo);
        else
            _fmemcpy (lpSource, lpDest, wPaddedBytesPerStrip);


        nReturn = CreateFileDIB (&lpDest, &lpSource, lpFileInfo, lpBitmapInfo);

        if (nReturn < 0)
            ExportDiscardResource (EC_ERROR);

        nReturn = (*lpfnWrConvertData)  (hExportFile, &lpSource, &lpDest, lpExpFileInfo, lpExpBitmapInfo);
        if (nReturn < 0)
            ExportDiscardResource (EC_FILEWRITE2);

    /*   Adjust file header if necessary      */

    lpfnFixupHeader = GetFHdr (wExportType);          

    nReturn = (*lpfnFixupHeader) (hExportFile, 
                                  lpBufs,
                                  hFileInfo,
                                  hBitmapInfo, 
                                  hExpFileInfo,
                                  hExpBitmapInfo); 
  
    if (nReturn < 0)
        ExportDiscardResource (EC_FILEWRITE2);

    if (! bAbandon)
        ProDeltaPos (1);

    /*  Otherwise successful completion.  Return a passing value (0)  */

    ExportDiscardResource (0);


/*---  Export Resource Discard Section  ---*/

    {

        int nRetVal;

        ExportDiscardResource:

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

        if ((DWORD) lpfnInitHeader > 0L)
            FreeProcInstance (lpfnInitHeader);

        if ((DWORD) lpfnRdConvertData > 0L)
            FreeProcInstance (lpfnRdConvertData);

        if ((DWORD) lpfnWrConvertData > 0L)
            FreeProcInstance (lpfnWrConvertData);

        if ((DWORD) lpfnFixupHeader > 0L)
            FreeProcInstance (lpfnFixupHeader);


        FreeFilters (hExpFileInfo, EXPORT_CLASS);

        hExportFile = 0;

        EnableWindow (hWndIP, TRUE);
        ProClose ();   // Close down Progress meter

        if (bAbandon)
            nRetVal = USER_ABANDON;

        if (nRetVal == 0)       // Completely successful export, remember saved directory and set image_active to no mods
        {
            if (nFlags == SAVE_NORMAL)  // Only want to update SaveDir when doing a normal save, not auto-convert
                SeparateFile ((LPSTR) szSaveDir, (LPSTR) szTempPath, (LPSTR)szSaveFileName); // "szTempPath" is only a placeholder
            bImageModified = FALSE;
        }

        return (nRetVal);
    }


}


BOOL FAR PASCAL SaveReportProc (hWnd, Message, wParam, lParam)
HWND          hWnd;
unsigned      Message;
WORD          wParam;
LONG          lParam;
{
    LPFILEINFO  lpFileInfo;
    switch (Message)
    {
        case WM_INITDIALOG:
        {
            RECT rc;
            PSTR pStringBuf;
            PSTR pStringBuf2;

            pStringBuf  = LocalLock (hStringBuf);
            pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

            /* Center the dialog       */

            GetWindowRect (hWnd,&rc);
            SetWindowPos  (hWnd,NULL,
                          (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2,
                          (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 3,
                           0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

            SetDlgItemText (hWnd, 101, (LPSTR)_fstrlwr ((LPSTR)szSaveFileName));
            LoadString (hInstIP, RC_TYPE_BASE + wExportType, (LPSTR)pStringBuf, MAXSTRINGSIZE);
            SetDlgItemText (hWnd, SR_FILETYPE, (LPSTR)pStringBuf);
            LoadString (hInstIP, RC_CLASS_BASE + wExportClass, (LPSTR)pStringBuf, MAXSTRINGSIZE);
            SetDlgItemText (hWnd, SR_FILECLASS, (LPSTR)pStringBuf);
            if ((wImportClass == IRGB) && (wExportClass == IRGB))
                LoadString (hInstIP, RC_CLASS_RGB, (LPSTR)pStringBuf, MAXSTRINGSIZE);
            else
                LoadString (hInstIP, RCP_DP256 + EPAL[wImportClass][wPal][wDither][0],
                (LPSTR)pStringBuf, MAXSTRINGSIZE);

            /* Create a number string here for lstrcat if OPT palette */

            if (wPal == IOPT)
                wsprintf (pStringBuf2, "%d ",wNumberColors);
            else
                pStringBuf2 [0] = 0;

            lstrcat (pStringBuf2, pStringBuf);
            SetDlgItemText (hWnd, SR_PAL, (LPSTR)pStringBuf2);

            lpFileInfo = (LPFILEINFO)GlobalLock (hExpFileInfo);


            LoadString (hInstIP, STR_UPIXELS, (LPSTR) pStringBuf, MAXSTRINGSIZE);
            wsprintf ((LPSTR)pStringBuf2, (LPSTR)pStringBuf, lpFileInfo->wScanWidth);
            SetDlgItemText (hWnd, SR_WIDTH, (LPSTR)pStringBuf2);
            wsprintf ((LPSTR)pStringBuf2, (LPSTR)pStringBuf, lpFileInfo->wScanHeight);
            SetDlgItemText (hWnd, SR_HEIGHT, (LPSTR)pStringBuf2);

            LoadString (hInstIP, STR_LBYTES, (LPSTR) pStringBuf, MAXSTRINGSIZE);
            wsprintf ((LPSTR)pStringBuf2, (LPSTR)pStringBuf, (long)lpFileInfo->wScanHeight * (long)lpFileInfo->wBytesPerRow);
            SetDlgItemText (hWnd, SR_SIZE, (LPSTR)pStringBuf2);
            GlobalUnlock (hExpFileInfo);

            LocalUnlock (hStringBuf);
        }
        break;

        case WM_COMMAND:
            switch (wParam)
            {
                case IDCANCEL:
                    EndDialog (hWnd, FALSE);
                break;
                case IDOK:
                    EndDialog (hWnd, TRUE);
                break;
                default:
                    return (FALSE);
            }
        break;
        default:
            return (FALSE);
    }
    return (TRUE);
}


/*---------------------------------------------------------------------------

   PROCEDURE:         InitExpFileInfo
   DESCRIPTION:      
   DEFINITION:         
   START:             1/14/90  Tom Bagford Jr.   
   MODS:              7/10/90  David Ison  Corrected for Padded ScanWidths, etc.

-----------------------------------------------------------------------------*/

int InitExpFileInfo (lpEFileInfo, lpIFileInfo)
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
// Whatever lpEFileInfo -> wPaddedBytesPerRow = ((((lpEFileInfo->wScanWidth * 24) + 31) / 32) * 4);
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


int NEAR CreateFileDIB (lpfpDest, lpfpSource, lpFileInfo, lpBitmapInfo)
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