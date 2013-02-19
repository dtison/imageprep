 

#include <windows.h>
#include <string.h>
#include <imgprep.h>
#include <strtable.h>
#include <proto.h>
#include <global.h>
#include <error.h>
#include <cpi.h>
#include <menu.h>

#define ImportDiscardResource(r)    { nRetval = r;   \
                                      goto DiscardResource;}

/*----------------------------------------------------------------------------

   PROCEDURE:        ImportFile
   DESCRIPTION:      Import the opened file and display it
   DEFINITION:       2.1.3
   START:            1/4/90  TBJ   
   MODS:

----------------------------------------------------------------------------*/

int ImportFile()
{
    int (FAR PASCAL * lpfnReadHeader)(int, LPBITMAPINFO, LPSTR, LPFILEINFO) = 0;
    int             nReturn         = 0;
    LPSTR           lpBufs          = (LPSTR) 0;
    LPFILEINFO      lpFileInfo      = (LPFILEINFO) 0;
    LPBITMAPINFO    lpBitmapInfo    = (LPBITMAPINFO) 0;
    int             nRetval;
    BOOL            bDisplay        = TRUE;

    lpBufs = GlobalLockDiscardable (hGlobalBufs);
    if (! lpBufs)
        ImportDiscardResource (EC_NOMEM);

    if (! (hBitmapInfo = GlobalAlloc (GHND, 
                        (long)sizeof(BITMAPINFOHEADER) + 1024L)))
        ImportDiscardResource (EC_NOMEM);

    if (! (hFileInfo = GlobalAlloc (GHND, 1024L)))
        ImportDiscardResource (EC_NOMEM);

    lpFileInfo   = (LPFILEINFO) GlobalLock (hFileInfo);
    lpBitmapInfo = (LPBITMAPINFO) GlobalLock (hBitmapInfo);


    if (lpfnReadHeader = GetRHdr (wImportType))
    {
      if ((nReturn = (*lpfnReadHeader)(hImportFile, lpBitmapInfo, lpBufs, lpFileInfo)) < 0)
        ImportDiscardResource (nReturn);

    }

  /*  Do some basic FileInfo checking to see if we actually can read the image  */

    nReturn    = ValidateImage (lpFileInfo);
    if (nReturn < 0)
        ImportDiscardResource (EC_INVALIDIMAGE);

        /*  Try to allocate a memory buffer for image  */
    //  #define EXPERIMENT
        #ifdef EXPERIMENT
        {
            DWORD dwSize;
            DWORD dwSizeNeeded;

            dwSizeNeeded = (DWORD) ((DWORD) lpFileInfo -> wPaddedBytesPerRow *
                           (DWORD)          lpFileInfo -> wScanHeight);

            dwSize = GetFreeSpace (0);

            if (dwSize >= dwSizeNeeded)
                hGlobalImageBuf = GlobalAlloc (GMEM_MOVEABLE, dwSizeNeeded);
            if (hGlobalImageBuf)
                bMemoryPaint = TRUE;
        }
        #endif



    /*  Save the image palette away for potential future use  */

    if (wImportClass == ICM)
    {
        LPSTR lpPalette;

        if (! (hGlobalPalette = GlobalAlloc (GHND, 1024L)))
            ImportDiscardResource (EC_NOMEM);

        lpPalette = GlobalLock (hGlobalPalette);

        _fmemcpy ((LPSTR) lpPalette, (LPSTR)lpBitmapInfo->bmiColors, 1024);

        GlobalUnlock (hGlobalPalette);
    }

    SetImportStates ();
    SetState (DITH_STATE, DDSM[0][wImportClass][wPal][wDither][SETDITH]);
    SetState (PAL_STATE, DDSM[0][wImportClass][wPal][wDither][SETPAL]);

    if (bAutoConvert && wImportType != IDFMT_CPI)
        bDisplay = FALSE;
    else
        bDisplay = bDoDisplay;
    
    if (bDisplay)
    {
        if (!(hWndDisplay = CreateDisplayWindow (hFileInfo)))
            ImportDiscardResource (EC_ERROR);

        InitDispInfo (hFileInfo);
        bIsFirstPaint = TRUE;

        ShowWindow (hWndDisplay, SW_SHOW);
    }


    ImportDiscardResource (nReturn);

    {
        DiscardResource:

        /*  Do any local memory handles allocated in this fn that would be freed anyway  */

        if (lpFileInfo)
            GlobalUnlock (hFileInfo);

        if (lpBitmapInfo)
            GlobalUnlock (hBitmapInfo);

        if (lpBufs)
            GlobalUnlock (hGlobalBufs);

        /*  Do global memory handles allocated in this fn that would not be freed anyway  */

        if (nRetval < 0)
        {
            GLOBALFREE (hBitmapInfo);
            GLOBALFREE (hFileInfo);
        }

        if (lpfnReadHeader)
            FreeProcInstance (lpfnReadHeader);

        return (nRetval);
    }
}   


/*---------------------------------------------------------------------------

   PROCEDURE:        CreateDisplayWindow
   DESCRIPTION:       
   DEFINITION:       2.1.3.2
   START:            TBJ
   MODS:

----------------------------------------------------------------------------*/

HWND   CreateDisplayWindow (hFileInfo)
HANDLE          hFileInfo;
{
    HWND          hWnd;
    LPFILEINFO    lpFileInfo;
    RECT          ipRect;
    RECT          rcWindow;
    int           nClientWidth;
    int           nClientHeight;
    int           dY;
    int           dX;
    float         fAspect;

    if (!(lpFileInfo = (LPFILEINFO) GlobalLock (hFileInfo)))
        return (EC_MEMORY1);
    
    SetRect ((LPRECT)&rcWindow,0,0,lpFileInfo->wScanWidth,lpFileInfo->wScanHeight);
    
    GlobalUnlock (hFileInfo);

    /*  Calculate displayable client area    */

    GetClientRect (hWndIP, (LPRECT)&ipRect); 
    
    dX  = 2 * GetSystemMetrics (SM_CXBORDER);
    dY  = GetSystemMetrics (SM_CYBORDER);
    dY += GetSystemMetrics (SM_CYCAPTION);
    
    nClientWidth  =  ipRect.right  - ipRect.left - dX;
    nClientHeight =  ipRect.bottom - ipRect.top  - dY;
    
    bIsTooBig = FALSE;
    

    /*   Adjust window size if necessary  */

    if (rcWindow.bottom > nClientHeight)
    {
        bIsTooBig = TRUE;
        fAspect = ((float)nClientHeight / (float)rcWindow.bottom);
        rcWindow.bottom = nClientHeight;
        rcWindow.right = (int)((float)rcWindow.right * fAspect);
    }

    if (rcWindow.right > nClientWidth)
    {
        bIsTooBig = TRUE;
        fAspect = ((float)nClientWidth / (float)rcWindow.right);
        rcWindow.right = nClientWidth;
        rcWindow.bottom = (int)((float)rcWindow.bottom * fAspect);
    }


    hWnd = CreateWindow ((LPSTR) "Display",
                         (LPSTR) szImageName,
                //       WS_CHILD | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
                   WS_CHILD | WS_BORDER | WS_CAPTION | WS_SYSMENU,
                         0,          // rcWindow.left,
                         0,          // rcWindow.top,
                         rcWindow.right + dX,
                         rcWindow.bottom + dY,
                         hWndIP,                                   /* parent */
                         NULL,                                      /* hMenu */
                         hInstIP,                               /* hInstance */
                         NULL);   

    
    if (hWnd)
        ShowWindow (hWnd, SW_SHOW);
    return (hWnd);
}   
    
    
/*-----------------------------------------------------------------------------
    
   PROCEDURE:        InitDispInfo
   DESCRIPTION:      Initialize the DisplayInfo structure
   DEFINITION:       2.1.3.3
   START:            1/4/90  TBJ
   MODS:

----------------------------------------------------------------------------*/

int InitDispInfo (hFileInfo)
HANDLE    hFileInfo;
{
  LPDISPINFO    lpDispInfo;
  LPFILEINFO    lpFileInfo;
  WORD          wBytesPerPlane;
  WORD          wDisplayBitsPerPixel;
  /*
  **
  */
  if (!(lpFileInfo = (LPFILEINFO)GlobalLock (hFileInfo))){
    return (EC_MEMORY1);
  }
  lpDispInfo = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);

  wBytesPerPlane = ((lpFileInfo->wScanWidth + 7) / 8);
  if ((wBytesPerPlane % 2) != 0)
    wBytesPerPlane++;
  lpDispInfo->wBytesPerPlane = wBytesPerPlane;

  switch (wDisplay){
    case IEVGA:                                         /* ... and I8514 */
      wDisplayBitsPerPixel = 8;
      break;
    case IVGA:
      wDisplayBitsPerPixel = 3; 
      break;
  }

  lpDispInfo->wBitsPerPixel = wDisplayBitsPerPixel;
  GlobalUnlock (hFileInfo);
  return (0);
}

