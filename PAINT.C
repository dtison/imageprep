 /*---------------------------------------------------------------------------
    COPYRIGHT:    Copyright (c) 1990, Computer Presentations, Inc.
                  All rights reserved.
    PROJECT:      Image Prep 3.0
    MODULE:       paint.c
    PROCEDURES:   PaintPicture    3.1
-----------------------------------------------------------------------------*/                            

#include <windows.h>
#include <memory.h>
#include <imgprep.h>
#include <cpi.h>
#include <proto.h>
#include <error.h>
#include <global.h>
#include <filters.h>
#include <gamma.h>
#include <color.h>
#include <strtable.h>

#define PaintDiscardResource(r)      _asm    mov   ax,r     \
                                     _asm    jmp   PaintDiscardResource

/*--- Notes on memory paint:  If memory paints requested, we must render
      the WHOLE IMAGE no matter what the window size so the thing will be 
      copied to the buffer completely.  --- */

#define SCROLL   // Scrollbars enabled

/*---------------------------------------------------------------------------
    PROCEDURE:    PaintPicture
    DESCRIPTION:  Initialization procedure for ImgPrep
    DEFINITION:   3.1
    START:        1/5/90  Tom Bagford Jr. (based on David Ison's original work )
    MODS:         3/90     Win 3.0 conversion  David Ison 
                  4/20/90 3.0 review & annotate TBjr
                  6/12/90 Optimization DMize
                  7/09/90 Straighten out Padded Widths etc...  D. Ison
                  7/12/90 Put in Discard Section    D. Ison
                  7/12/90 Added intelligence to repaints by looking at the RCpaint values

-----------------------------------------------------------------------------*/

int FAR PASCAL PaintPicture (hWnd)
HWND hWnd;
{                                     
    /*  Function pointers   */

    int (FAR PASCAL *lpfnRdConvertData)(int, LPSTR FAR *, LPSTR FAR *, LPFILEINFO, LPBITMAPINFO) = 0;
    int (FAR PASCAL * lpfnReadHeader)(int, LPBITMAPINFO, LPSTR, LPFILEINFO) = 0;

    /*   Locals also used as discard flags      */

    HDC                 hDC               = 0;
    int                 nDC               = 0;
    HDC                 hMemoryDC         = 0;
    HBITMAP             hBitmap           = 0;
    HBITMAP             hOldBitmap        = 0;
    HPALETTE            hPalette          = 0;
    HPALETTE            hOldPalette       = 0;
    HPALETTE            hOldMemPalette    = 0;
    LPBITMAPINFOHEADER  lpPaintbmiHeader  = (LPBITMAPINFOHEADER) 0L;
    LPBITMAPINFOHEADER  lpbmiHeader       = (LPBITMAPINFOHEADER) 0L;
    LPBITMAPINFO        lpPaintBitmapInfo = (LPBITMAPINFO) 0L;
    LPBITMAPINFO        lpBitmapInfo      = (LPBITMAPINFO) 0L;
    LPFILEINFO          lpFileInfo        = (LPFILEINFO) 0L;
    LPSTR               lpBufs            = (LPSTR) 0L;
    RGBQUAD FAR *       lpRGBQuadPtr      = (RGBQUAD FAR *) 0L;
    HANDLE              hPaintBitmapInfo  = (HANDLE)  0;
    HCURSOR             hOldCursor        = (HCURSOR) 0;
    int                 nTmpDC            = 0;

    /*   Locals not used as discard flags     */

    WORD                wScrnX = 0;
    WORD                wScrnY = 0;
    int                 nReturn = 0;
    HANDLE              hInternalPal;
    LPDISPINFO          lpDispInfo;
    PAINTSTRUCT         Ps;
    int                 hFile;
    LPLOGPALETTE        lpLogPalette;
    LPSTR               lpDest;
    LPSTR               lpInputBuf;
    LPSTR               lpSource;
    LPSTR               lpTmpBuf;
    RECT                rcClient;
    WORD FAR *          pw;
    int                 i;
    int                 nPaintAreaTop;
    int                 nPaintAreaBottom;
    BOOL                bDonePainting = FALSE;
    BOOL                bAbandonPaint = FALSE;
    RECT                rcUpdate;
    char huge *         lpGlobalImageBuf;
    char huge *         lpGlobalImagePtr;
    WORD                wPaddedBytesPerStrip;
    MSG                 Msg;
    BOOL                bNestedPaint = FALSE;

    hDC = BeginPaint (hWnd, &Ps);

    if (hDC == NULL)
    {
        EndPaint (hWnd, &Ps);
        return (nReturn);
    }
    
    if (IsRectEmpty ((LPRECT)&Ps.rcPaint))  // Now check for a (silly) null rect.
    {
        EndPaint (hWnd, &Ps);
        return (nReturn);
    }

    if (bIsSaving)
    {
        EndPaint (hWnd, &Ps);
        return (nReturn);
    }

    if (! (lpBufs = (LPSTR) GlobalLock (hGlobalBufs)))
        PaintDiscardResource (EC_MEMORY1);

    if (! (lpBitmapInfo = (LPBITMAPINFO) GlobalLock (hBitmapInfo)))
        PaintDiscardResource (EC_MEMORY1);

    if (! (hPaintBitmapInfo = GlobalAlloc (GMEM_MOVEABLE,
                              (DWORD)(sizeof(BITMAPINFOHEADER) + 1024))))
        PaintDiscardResource (EC_MEMORY2);

    if (!(lpPaintBitmapInfo = (LPBITMAPINFO)GlobalLock (hPaintBitmapInfo)))
        PaintDiscardResource (EC_MEMORY1);

    /*   Lock and allocate data buffers   */

    if (! (lpFileInfo = (LPFILEINFO) GlobalLock (hFileInfo)))
    {
        EndPaint (hWnd, &Ps);
        return (EC_MEMORY1);
    }

    lpDispInfo  = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);

    /*  Reset fileinfo before beginning read    */

    if (lpfnReadHeader = GetRHdr (wImportType))
    {
      if ((nReturn = (*lpfnReadHeader)(hImportFile, lpBitmapInfo, lpBufs, lpFileInfo)) < 0) 
      {
        GLOBALFREE (hBitmapInfo);
        GLOBALFREE (hFileInfo);
        FreeProcInstance (lpfnReadHeader);
        EndPaint (hWnd, &Ps);
        return (nReturn);
      }
      FreeProcInstance (lpfnReadHeader);
    }

    InitDispInfo (hFileInfo);

    /*   Generate a palette    */
  
    hFile = hImportFile;
    if (bIsFirstPaint)
    {
        int nRetval;

        GLOBALFREE (hBhist);

        nRetval = GenPal (IMPORT_PAL, lpFileInfo, lpBitmapInfo, hFile);

        if (nRetval <= 0)
            PaintDiscardResource (nRetval);

        GLOBALFREE (hImpPalette);
        hInternalPal  = (WORD) nRetval;
        hImpPalette   = hInternalPal;

        if (wImportClass == ICM)
        {
            LPSTR lpImpPalette;
            LPSTR lpOrgPalette;

            if (! wPal)
            {
                lpImpPalette = GlobalLock (hImpPalette);
                ColorCorrectPalette ((RGBQUAD FAR *) lpImpPalette);
                GlobalUnlock (hImpPalette);
            }

            lpOrgPalette = GlobalLock (hGlobalPalette);
            _fmemcpy ((LPSTR) lpOrgPalette, (LPSTR)lpBitmapInfo->bmiColors, 1024);
            ColorCorrectPalette ((RGBQUAD FAR *) lpOrgPalette);
            GlobalUnlock (hGlobalPalette);

            /*  Delete current realized palette */

            if (hDispPalette)
            {
                DeleteObject (hDispPalette);
                hDispPalette = 0;
            }
        }
    }   

    #ifdef MEMPAINT
    if (bMemoryPaint)
    {
        lpGlobalImageBuf = GlobalLock (hGlobalImageBuf);
        lpGlobalImagePtr = lpGlobalImageBuf;
        wPaddedBytesPerStrip =  lpFileInfo -> wPaddedBytesPerRow * lpFileInfo -> wRowsPerStrip;
    }
    #endif

    /*  Create a logical palette   */
    
    if (bIsFirstPaint)  
    {
        RGBQUAD FAR *lpRGBQuadPal;

        lpRGBQuadPal = (RGBQUAD FAR *) GlobalLock (hImpPalette);
        lpRGBQuadPtr = lpRGBQuadPal;
        lpLogPalette = (LPLOGPALETTE)lpBufs;
        lpLogPalette -> palVersion    = 0x300;
//      lpLogPalette -> palNumEntries = ((wDevice == DVGA) ? 8 : 256);
        lpLogPalette -> palNumEntries = 256;

        for (i = 0; i < (int)lpLogPalette->palNumEntries; i++)
        {
            lpLogPalette->palPalEntry[i].peRed    = lpRGBQuadPtr->rgbRed;
            lpLogPalette->palPalEntry[i].peGreen  = lpRGBQuadPtr->rgbGreen;
            lpLogPalette->palPalEntry[i].peBlue   = lpRGBQuadPtr->rgbBlue;
            lpLogPalette->palPalEntry[i].peFlags  = 0;
            lpRGBQuadPtr++;
        }

        if (! (hPalette = CreatePalette (lpLogPalette))){
            PaintDiscardResource (EC_MEMORY3);
        }
        else
        {
            if (hDispPalette)
                DeleteObject (hDispPalette);

            hDispPalette = hPalette;
        }

        GlobalUnlock (hImpPalette);
        lpRGBQuadPtr = (RGBQUAD FAR *) 0L;  // Reset ptr because it flags the lock
    }


    /*  Set up device context & map modes  */

    nDC = SaveDC (hDC);
    GetClientRect (hWnd, (LPRECT) &rcClient);

    if (! wFullView)
    {
        SetMapMode (hDC, MM_ANISOTROPIC);
        SetStretchBltMode (hDC, COLORONCOLOR);
        SetWindowOrg (hDC, 0, 0);
        SetWindowExt (hDC, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
        SetViewportOrg (hDC, 0, 0);
        SetViewportExt (hDC, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
        OffsetWindowOrg (hDC, (GetScrollPos (hWnd, SB_HORZ)), (GetScrollPos (hWnd, SB_VERT)));
    }
    else
    {
        SetMapMode (hDC, MM_ANISOTROPIC);
        SetStretchBltMode (hDC, COLORONCOLOR);
        SetWindowOrg (hDC, 0, 0);
        SetWindowExt (hDC, lpFileInfo->wScanWidth, lpFileInfo->wScanHeight);
        SetViewportOrg (hDC, 0, 0);
        SetViewportExt (hDC, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
    }


    /*  Setup bitmap stuff  */

    if (! (hMemoryDC = CreateCompatibleDC (hDC)))
        PaintDiscardResource (EC_MEMORY4);

//  #define VIDEOTEST
    #ifdef VIDEOTEST
    if (! (hBitmap = CreateCompatibleBitmap (hDC, lpFileInfo->wScanWidth, lpFileInfo->wRowsPerStrip)))
    #else
    if (! (hBitmap = CreateCompatibleBitmap (hDC, lpDispInfo->wPaddedScanWidth, lpFileInfo->wRowsPerStrip)))
    #endif
        PaintDiscardResource (EC_MEMORY5);

    hOldBitmap = SelectObject (hMemoryDC, hBitmap);

    /*   Set palette   */

    hOldPalette = SelectPalette (hDC, hDispPalette, 0);
    hOldMemPalette = SelectPalette (hMemoryDC, hDispPalette, 0);

    RealizePalette (hDC);


    /*  Setup FILEINFO and DISPINFO stuff    */

    lpDispInfo = (LPDISPINFO)((LPSTR)lpFileInfo + 512L);
    lpDispInfo->hDispPalette = hInternalPal;
    lpFileInfo->bIsLastStrip = FALSE;

    /*  Setup bitmap info for paint     */
    _fmemcpy ((LPSTR)lpPaintBitmapInfo, (LPSTR)lpBitmapInfo, (sizeof(BITMAPINFOHEADER) + 1024));


    /*   READ CONVERT AND DISPLAY IMAGE    */

    /*  This stuff is going back into file readers or somewhere else  */

    lpPaintbmiHeader                  = &lpPaintBitmapInfo->bmiHeader;
    lpPaintbmiHeader->biSize          = (DWORD)sizeof(BITMAPINFOHEADER);

    #ifdef VIDEOTEST
    lpPaintbmiHeader->biWidth         = (DWORD)lpFileInfo->wScanWidth;
    #else
    lpPaintbmiHeader->biWidth         = (DWORD)lpDispInfo->wPaddedScanWidth;
    #endif

    lpPaintbmiHeader->biHeight        = (DWORD)lpFileInfo->wRowsPerStrip;
    lpPaintbmiHeader->biPlanes        = 1;
    lpPaintbmiHeader->biBitCount      = 8;
    lpPaintbmiHeader->biCompression   = 0;    
    lpPaintbmiHeader->biSizeImage     = 0;
    lpPaintbmiHeader->biXPelsPerMeter = 0;
    lpPaintbmiHeader->biYPelsPerMeter = 0;
    lpPaintbmiHeader->biClrUsed       = 0;
    lpPaintbmiHeader->biClrImportant  = 0;

    _fmemcpy ((LPSTR) lpBitmapInfo, (LPSTR) lpPaintBitmapInfo, (sizeof(BITMAPINFOHEADER) + 1024));

    lpbmiHeader                       = &lpBitmapInfo->bmiHeader;
    lpbmiHeader->biHeight             = (DWORD)lpFileInfo->wScanHeight;


    /*  Reformat of DIB colortable to indexes as in SHOWDIB program     */

    pw = (WORD FAR *)((LPSTR)lpPaintBitmapInfo + 
                     lpPaintBitmapInfo->bmiHeader.biSize);

    for (i = 0; i < 256; i++)
        *pw++ = i;

    /*   Check and set file access    */ 

    if (_llseek (hFile, lpFileInfo->dwDataOffset, 0)  != (LONG) lpFileInfo->dwDataOffset) 
        PaintDiscardResource (EC_FILEREAD1);

    /*  Setup the file read module    */

    if (! bIsPainting)
        SetupFilters (lpFileInfo, IMPORT_CLASS);

    bIsPainting++     ;  // Increment global paint flag
    lpfnRdConvertData = GetRCD (wImportType);


    /*   Set up buffers    */

    lpSource    = lpInputBuf  = lpBufs;
    lpDest      = lpTmpBuf    = (LPSTR) ((BYTE huge *) lpBufs + LARGEBUFOFFSET);

    CopyRect ((LPRECT)&rcUpdate, (LPRECT)&Ps.rcPaint);
    DPtoLP (hDC, (LPPOINT)&rcUpdate, 2);

    nPaintAreaTop     = rcUpdate.top;
    nPaintAreaBottom  = rcUpdate.bottom;

    nPaintAreaTop -= lpFileInfo -> wRowsPerStrip;
    if (nPaintAreaTop < 0)
        nPaintAreaTop = 0;

    if (! bPollMessages)
      hOldCursor  = SetCursor (LoadCursor (NULL, IDC_WAIT));

    /*  Read and display image     */

    for (i = 0; (i < (int)(lpFileInfo->wNumStrips - 1) && !bDonePainting); i++)
    {
        if (bIsFirstPaint || (! bMemoryPaint))
        {
            if ((nReturn = (*lpfnRdConvertData)(hFile, &lpDest, &lpSource, lpFileInfo, lpBitmapInfo)) < 0)
                                    //                    Now this is your return position..  9/90  D. Ison
                PaintDiscardResource (EC_FILEREAD2);

            if (bMemoryPaint)
            {
                _fmemcpy ((LPSTR) lpGlobalImagePtr, lpDest, wPaddedBytesPerStrip);
                lpGlobalImagePtr += wPaddedBytesPerStrip;
            }
        }
        else
        {
            _fmemcpy (lpDest, (LPSTR) lpGlobalImagePtr, wPaddedBytesPerStrip);
            lpGlobalImagePtr += wPaddedBytesPerStrip;
        }

        if (wScrnY >= (WORD) nPaintAreaTop)
            if (wScrnY <= (WORD) nPaintAreaBottom)
            {
                ColorCorrectRGB (lpSource, lpDest, lpFileInfo);
                CreateDisplayBitmap (&lpDest, &lpSource, lpFileInfo, lpBitmapInfo);
                InvertScanlines (lpSource, lpDest, lpDispInfo->wPaddedBytesPerRow, lpFileInfo->wRowsPerStrip);
                #ifdef STRETCHDIBITS
                StretchDIBits (hDC, wScrnX, wScrnY, lpDispInfo -> wPaddedScanWidth, lpFileInfo -> wRowsPerStrip, 0, 0, lpDispInfo -> wPaddedScanWidth, lpFileInfo -> wRowsPerStrip, lpSource, lpPaintBitmapInfo, DIB_PAL_COLORS, SRCCOPY);
                #else
                SetDIBits (hDC, hBitmap, 0, lpFileInfo->wRowsPerStrip, lpSource, lpPaintBitmapInfo, DIB_PAL_COLORS);
                BitBlt (hDC, wScrnX, wScrnY, lpDispInfo->wPaddedScanWidth, lpFileInfo->wRowsPerStrip, hMemoryDC, 0, 0, SRCCOPY);
                #endif

            }
            else
                bDonePainting = TRUE;
      
        wScrnY += lpFileInfo->wRowsPerStrip;

        bIsFirstPaint    = FALSE;  // Let's say false and see if it gets changed while processing message loop..
        if (bPollMessages)
        while (PeekMessage (&Msg, NULL, 0, 0, PM_REMOVE))
        {
            if (! image_active)   // A WM_CLOSE message came in "behind" our repaint
            {
                bDonePainting = TRUE;
                bAbandonPaint = TRUE;
                PaintDiscardResource (0); 
            }

            if (Msg.message == WM_KEYUP)
                if (Msg.wParam == VK_ESCAPE && ! bFullScreen)
                {
                    PSTR pStringBuf;
                    PSTR pStringBuf2;

                    pStringBuf  = LocalLock (hStringBuf);
                    pStringBuf2 = pStringBuf + MAXSTRINGSIZE;
            
                    LoadString (hInstIP, STR_ABANDON_DISPLAY, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                    LoadString (hInstIP, STR_IMAGE_DISPLAY,   (LPSTR) pStringBuf2, MAXSTRINGSIZE);

                    if (MessageBox (hWnd, (LPSTR)pStringBuf, (LPSTR)pStringBuf2, MB_YESNO) == IDYES)
                    {
                        bDonePainting = TRUE;
                        bAbandonPaint = TRUE;
                    }
                    LocalUnlock (hStringBuf);
                }

                if (Msg.message == WM_PAINT && Msg.hwnd == hWnd)
                {
                    PAINTSTRUCT Ps;
                    
                    BeginPaint (hWnd, &Ps);
                    EndPaint   (hWnd, &Ps);
                    bNestedPaint = TRUE;
                }
                else
                {
                    TranslateMessage(&Msg);
                    DispatchMessage(&Msg);
                }
        }
        
        if (bNestedPaint)
            PaintDiscardResource (0);  // Might as well throw away this paint
    }
    
    /*   Mark the last strip for the formats that need to know    */

    lpFileInfo->bIsLastStrip = TRUE;

    wPaddedBytesPerStrip =   lpFileInfo -> wPaddedBytesPerRow * 
                             lpFileInfo -> wRowsPerLastStrip;

    if (bIsFirstPaint || (! bMemoryPaint))
    {
        if (nReturn = (*lpfnRdConvertData)(hFile, &lpDest, &lpSource, lpFileInfo, lpBitmapInfo) < 0)
                                //                    Now this is your return position..  9/90  D. Ison
            PaintDiscardResource (EC_FILEREAD2);

        if (bMemoryPaint)
        {
            _fmemcpy ((LPSTR) lpGlobalImagePtr, lpDest, wPaddedBytesPerStrip);
            lpGlobalImagePtr += wPaddedBytesPerStrip;
        }
    }
    else
    {
        _fmemcpy (lpDest, (LPSTR) lpGlobalImagePtr, wPaddedBytesPerStrip);
        lpGlobalImagePtr += wPaddedBytesPerStrip;
    }

    if (! bDonePainting)
        if (wScrnY >= (WORD) nPaintAreaTop)
            if (wScrnY <= (WORD) nPaintAreaBottom)
            {
                lpPaintBitmapInfo->bmiHeader.biHeight = (DWORD)lpFileInfo->wRowsPerLastStrip;

                ColorCorrectRGB (lpSource, lpDest, lpFileInfo);
                CreateDisplayBitmap (&lpDest, &lpSource, lpFileInfo, lpBitmapInfo);
                InvertScanlines (lpSource, lpDest, lpDispInfo->wPaddedBytesPerRow, lpFileInfo->wRowsPerLastStrip);
                SetDIBits (hDC, hBitmap, 0, lpFileInfo->wRowsPerLastStrip, lpSource, lpPaintBitmapInfo, DIB_PAL_COLORS);
   
                BitBlt (hDC, wScrnX, wScrnY, 
                        lpDispInfo->wPaddedScanWidth,
                        lpFileInfo->wRowsPerLastStrip, 
                        hMemoryDC, 
                        0, 
                        0, 
                        SRCCOPY); 
            }

    lpGlobalImagePtr = lpGlobalImageBuf;

    /*  Clean up  (successful completion)  */

        TmpVals [15] = FALSE;   // For batch processing experiment

    PaintDiscardResource (0);


/*---  Paint Resource Discard Section  ---*/

    {

        int nRetVal;

        PaintDiscardResource:

        _asm    mov nRetVal,ax

        bIsPainting = 0;

        /*  Do any local memory handles allocated in this fn that would be freed anyway  */

        /*  Use the pointers as a flag of the global handles being locked  */


        if (lpPaintBitmapInfo)
            GlobalUnlock (hPaintBitmapInfo);

        if (hPaintBitmapInfo)
            GLOBALFREE (hPaintBitmapInfo);

        if (lpBitmapInfo)
            GlobalUnlock (hBitmapInfo);

        if (lpRGBQuadPtr)
            GlobalUnlock (hImpPalette);

        if (lpFileInfo)
            GlobalUnlock (hFileInfo);

        /*  Do global memory handles allocated in this fn that would not be freed anyway  */

        if (lpBufs)
            GlobalUnlock (hGlobalBufs);

        if (lpGlobalImageBuf)
            GlobalUnlock (hGlobalImageBuf);

        /*  Do other general cleanup  */

        FreeFilters (hFileInfo, IMPORT_CLASS);

        if (hMemoryDC)
        {
            if (hOldMemPalette)
              SelectPalette (hMemoryDC, hOldMemPalette, 0);

            if (hOldBitmap)
            {
                SelectObject  (hMemoryDC, hOldBitmap);
                DeleteObject  (hBitmap);
            }
            DeleteDC (hMemoryDC);
        }


        if (lpfnRdConvertData)
            FreeProcInstance (lpfnRdConvertData);

        if (hDC)
        {
          if (nDC)
            RestoreDC (hDC, nDC);
          if (hOldPalette)
            SelectPalette (hDC, hOldPalette, 0);
        }

        if (hOldCursor)
          SetCursor (hOldCursor);

        hExportFile = 0;
        EndPaint (hWnd, &Ps);

        /* See if paint was prematurely abandoned, and send message to destroy window if so.  */

        if (bAbandonPaint)
        {
            DestroyWindow (hWndDisplay);  // Probably don't have to wait for message loop in this case
//          SendMessage (hWndDisplay, WM_CLOSE, NULL, NULL);
            nRetVal = 0;
        }

        if (bNestedPaint)
            InvalidateRect (hWnd, NULL, FALSE);

        return (nRetVal);
    }
}



  
int NEAR PASCAL CreateDisplayBitmap (lpfpDest, lpfpSource, lpFileInfo, lpBitmapInfo)
LPSTR FAR *   lpfpDest; 
LPSTR FAR *   lpfpSource;
LPFILEINFO    lpFileInfo;
LPBITMAPINFO  lpBitmapInfo;
{

    int (FAR PASCAL *lpfnFilter) (LPSTR FAR *, LPSTR FAR *, LPSTR);
    DITHERBAYER FAR * lpDB;
    FMT8TO24 FAR *    lp8to24;
    LPSTR             lpFS;
    int               n = 0;  

    LPSTR lpSource;
    LPSTR lpDest;
    LPSTR lpTmp;

    lpSource = *lpfpDest;
    lpDest   = *lpfpSource;
  
    while (ImportFilters[n])
    {
        lpfnFilter = GetFilterProc (ImportFilters[n]);
        if (lpFileInfo->bIsLastStrip)
            lpFS = (LPSTR)GlobalLock (ImportFilterStruct[n][1]);
        else
            lpFS = (LPSTR)GlobalLock (ImportFilterStruct[n][0]);

        switch (ImportFilters[n])
        {
            case DIDX:
                lp8to24 = (FMT8TO24 FAR *)lpFS; 
        //      lp8to24->lpPalette = (LPSTR)lpBitmapInfo->bmiColors;

                break;
        }

        lpTmp    = lpDest;
        lpDest   = lpSource;
        lpSource = lpTmp;


        (*lpfnFilter)((LPSTR FAR *) &lpDest,
                      (LPSTR FAR *) &lpSource,
                      lpFS);




        switch (ImportFilters[n])
        {
            case BAY8:  
            case GBAY:  
                lpDB = (DITHERBAYER FAR *)lpFS;
                lpDB->wScrnY += lpFileInfo->wRowsPerStrip;
                break;

        }
        if (lpFileInfo->bIsLastStrip)
            GlobalUnlock (ImportFilterStruct[n][1]);
        else
            GlobalUnlock (ImportFilterStruct[n][0]);
        FreeProcInstance (lpfnFilter);
        n++;
  }


    *lpfpDest   = lpDest;
    *lpfpSource = lpSource;

  return (0);
}


FARPROC GetFilterProc (int nIndex)
{
  FARPROC lpfn;
  /*
  **
  */
  switch (nIndex){
    case 1:
      lpfn = MakeProcInstance (QuantColorReduced, hInstIP);
      break;
    case 2:
      lpfn = MakeProcInstance (QuantFSDither, hInstIP);
      break;
    case 3:
      lpfn = MakeProcInstance (DitherBayer_A, hInstIP);
      break;
    case 4:
      lpfn = MakeProcInstance (UniformQuant_A, hInstIP);
      break;
    case 5:
      lpfn = MakeProcInstance (UniformQuant8_A, hInstIP);
      break;
    case 6:
      lpfn = MakeProcInstance (GraySum, hInstIP);
      break;
    case 7:
      lpfn = MakeProcInstance (ShiftGrayLevels, hInstIP);
      break;
    case 8:
      lpfn = MakeProcInstance (Format_8To24, hInstIP);
      break;
    case 9:
      lpfn = MakeProcInstance (GrayToBW, hInstIP);
      break;
    case 10:
      lpfn = MakeProcInstance (DitherGrayBayer, hInstIP);
      break;
    case 11:
      lpfn = MakeProcInstance (DitherGrayFS, hInstIP);
      break;
    default:
      lpfn = (FARPROC)0; 
      break;
  }
  return (lpfn);
}




