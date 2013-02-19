

#include <windows.h>
#include <string.h>
#include "imgprep.h"
#include "strtable.h"
#include "proto.h"
#include "global.h"
#include "error.h"
#include <io.h>
#include <cpi.h>




#define PrevDiscardResource(r)      _asm    mov   ax,r     \
                                     _asm    jmp   PrevDiscardResource


int FAR PASCAL PreviewPicture (hWnd)
HWND hWnd;
{                                     
    /*  Function pointers */
    int (FAR PASCAL *lpfnRdConvertData)(int, LPSTR FAR *, LPSTR FAR *, LPFILEINFO, LPBITMAPINFO) = 0;
    int (FAR PASCAL * lpfnReadHeader)(int, LPBITMAPINFO, LPSTR, LPFILEINFO) = 0;
    
    /*  Locals also used as discard flags */
    HDC                 hDC               = 0;
    int                 nDC               = 0;
    HDC                 hMemoryDC         = 0;
    
    HBITMAP             hBitmap           = 0;
    HBITMAP             hOldBitmap        = 0;
    
    LPBITMAPINFOHEADER  lpbmiHeader       = (LPBITMAPINFOHEADER) 0L;
    LPBITMAPINFO        lpBitmapInfo      = (LPBITMAPINFO) 0L;
    LPFILEINFO          lpFileInfo        = (LPFILEINFO) 0L;
    LPDISPINFO          lpDispInfo        = 0;
    LPSTR               lpBufs            = (LPSTR) 0L;
    RGBQUAD FAR *       lpRGBQuadPtr      = (RGBQUAD FAR *) 0L;
    HANDLE              hPaintBitmapInfo  = 0;
    HCURSOR             hOldCursor = (HCURSOR) 0;
    
    /*  Locals initialized but not used as flags  */
    WORD                wScrnX = 0;
    WORD                wScrnY = 0;
    int                 nReturn = 0;
    
    PAINTSTRUCT         Ps;
    int                 hFile;
    
    LPSTR               lpDest;
    LPSTR               lpInputBuf;
    LPSTR               lpSource;
    LPSTR               lpTmpBuf;
    
    int                 i;
    int                 nPaintAreaTop;
    int                 nPaintAreaBottom;
    BOOL                bDonePainting = FALSE;
    BOOL                bAbandonPaint = FALSE;
    RECT                rcUpdate;
    RECT                rcClient;
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
    
  // bIsPainting = TRUE;
    
    hFile = hImportFile;

    /*   Lock and allocate data buffers   */

    if (! (lpBufs = (LPSTR) GlobalLock (hGlobalBufs)))
        PrevDiscardResource (EC_MEMORY1);

    if (! (lpFileInfo = (LPFILEINFO) GlobalLock (hFileInfo)))
    {
        EndPaint (hWnd, &Ps);
        return (EC_MEMORY1);
    }

    if (!(lpBitmapInfo = (LPBITMAPINFO) GlobalLock (hBitmapInfo)))
        PrevDiscardResource (EC_MEMORY1);

    lpFileInfo->bIsLastStrip = FALSE;

    /*  Reset fileinfo before beginning read     */

    if (lpfnReadHeader = GetRHdr (wImportType))
    {
        if ((nReturn = (*lpfnReadHeader)(hImportFile, lpBitmapInfo, lpBufs, lpFileInfo)) < 0)
        {
            GLOBALFREE (hBitmapInfo);
            GLOBALFREE (hFileInfo);
            GlobalUnlock (hGlobalBufs);
            FreeProcInstance (lpfnReadHeader);
            EndPaint (hWnd, &Ps);
            return (nReturn);
        }
        FreeProcInstance (lpfnReadHeader);
  }
  InitDispInfo (hFileInfo);


    nDC = SaveDC(hDC);
    GetClientRect (hWnd, (LPRECT)&rcClient);

    if (!wFullView)
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
        PrevDiscardResource (EC_MEMORY3);

    if (! (hBitmap = CreateCompatibleBitmap (hDC, lpFileInfo->wScanWidth, lpFileInfo->wRowsPerStrip)))
        PrevDiscardResource (EC_MEMORY4);

    hOldBitmap = SelectObject (hMemoryDC, hBitmap);

    /*  This stuff is going back into file readers or somewhere else  */

    lpbmiHeader                  = &lpBitmapInfo->bmiHeader;
    lpbmiHeader->biSize          = (DWORD)sizeof(BITMAPINFOHEADER);
    lpbmiHeader->biWidth         = (DWORD)lpFileInfo->wScanWidth;
    lpbmiHeader->biHeight        = (DWORD)lpFileInfo->wRowsPerStrip;
    lpbmiHeader->biPlanes        = 1;
    lpbmiHeader->biBitCount      = 24;
    lpbmiHeader->biCompression   = 0;    
    lpbmiHeader->biSizeImage     = 0;
    lpbmiHeader->biXPelsPerMeter = 0;
    lpbmiHeader->biYPelsPerMeter = 0;
    lpbmiHeader->biClrUsed       = 0;
    lpbmiHeader->biClrImportant  = 0;

    /*   Check and set file access    */ 
    if (_llseek (hFile, lpFileInfo->dwDataOffset, 0)  != (LONG) lpFileInfo->dwDataOffset) 
      PrevDiscardResource (EC_FILEREAD1);

    bIsPainting++     ;  // Increment global paint flag

    /*  Setup the file read module    */
    lpfnRdConvertData = GetRCD (wImportType);

    /*   Set up buffers    */
    lpSource    = lpInputBuf  = lpBufs;
    lpDest      = lpTmpBuf    = (BYTE huge *) lpBufs + LARGEBUFOFFSET;
    
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
                PrevDiscardResource (EC_FILEREAD2);

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
                {
                    int i;
                    LPSTR lpPtr = lpSource;

                    for (i = 0; i < (int) lpFileInfo -> wRowsPerStrip; i++)
                    {
                        StretchDIBits (hDC, wScrnX, wScrnY + i, lpFileInfo -> wScanWidth, 1, 0, 0, lpFileInfo -> wScanWidth, 1, lpPtr, lpBitmapInfo, DIB_RGB_COLORS, SRCCOPY);
                        lpPtr += lpFileInfo -> wPaddedBytesPerRow;
                    }
                }

            }
            else
                bDonePainting = TRUE;
        wScrnY += lpFileInfo->wRowsPerStrip;

        if (bPollMessages)
        while (PeekMessage (&Msg, NULL, 0, 0, PM_REMOVE))
        {
            if (! image_active)   // A WM_CLOSE message came in "behind" our repaint
            {
                bDonePainting = TRUE;
                bAbandonPaint = TRUE;
                PrevDiscardResource (0); 
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
            PrevDiscardResource (0);  // Might as well throw away this paint
    }

    /*   Mark the last strip for the formats that need to know    */

    lpFileInfo->bIsLastStrip = TRUE;

    lpSource    = lpInputBuf  = lpBufs;
    lpDest      = lpTmpBuf    = (BYTE huge *) lpBufs + LARGEBUFOFFSET;

    if ((nReturn = (*lpfnRdConvertData)(hFile, &lpDest, &lpSource, lpFileInfo, lpBitmapInfo)) < 0)
        PrevDiscardResource (EC_FILEREAD2);

    if (! bDonePainting)
        if (wScrnY >= (WORD)nPaintAreaTop)
            if (wScrnY <= (WORD)nPaintAreaBottom)
            {
                ColorCorrectRGB (lpSource, lpDest, lpFileInfo);
                {
                    int i;
                    LPSTR lpPtr = lpSource;

                    for (i = 0; i < (int) lpFileInfo -> wRowsPerLastStrip; i++)
                    {
                        StretchDIBits (hDC, wScrnX, wScrnY + i, lpFileInfo -> wScanWidth, 1, 0, 0, lpFileInfo -> wScanWidth, 1, lpPtr, lpBitmapInfo, DIB_RGB_COLORS, SRCCOPY);
                        lpPtr += lpFileInfo -> wPaddedBytesPerRow;
                    }

                }
            }

 
    bIsFirstPaint &= 0x8000;   // Strip lo bit, leave high indicating possibly no actual first "paint"

    /*  Clean up  (successful completion)  */
    PrevDiscardResource (0);


    /*---  Paint Resource Discard Section  ---*/
    {
        int nRetVal;

        PrevDiscardResource:

        _asm    mov nRetVal,ax

        /*  Do any local memory handles allocated in this fn that would be freed anyway  */
        /*  Use the pointers as a flag of the global handles being locked  */

    if (hPaintBitmapInfo)  // DEBUG WINDOWS....?
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

    if (lpfnRdConvertData)
      FreeProcInstance (lpfnRdConvertData);

    if (hMemoryDC){
      if (hOldBitmap){
        SelectObject(hMemoryDC, hOldBitmap);
        DeleteObject(hBitmap);
      }
      DeleteDC (hMemoryDC);
    }

    if (hDC)
      RestoreDC (hDC, nDC);

    EndPaint (hWnd, &Ps);

    if (hOldCursor)
      SetCursor (hOldCursor);

    hExportFile = 0;
    bIsPainting = FALSE;


        /* See if paint was prematurely abandoned, and send message to destroy window if so.  */

        if (bAbandonPaint)
        {
          //PostMessage (hWndDisplay, WM_CLOSE, NULL, NULL);  // Probably don't have to wait
            DestroyWindow (hWndDisplay);
            nRetVal = 0;
        }

    if (bNestedPaint)
      InvalidateRect (hWnd, NULL, FALSE);

    return (nRetVal);
  }
}
