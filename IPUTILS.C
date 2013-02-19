#include <windows.h>
#include <imgprep.h>
#include <global.h>
#include <proto.h>
#include <error.h>
#include <cpi.h>
#include <gamma.h>
#include <color.h>
#include <memory.h>
#include <strtable.h>
#include <string.h>
#include <menu.h>

/*-------------------------------------------------------------------------

  IPUTILS.C  - A file with functions that are called by more that one module
               in Imageprep. (Or just don't fit nicely in any other module)

--------------------------------------------------------------------------*/



void FAR PASCAL ColorCorrectRGB (lpDest, lpSource, lpFileInfo)
LPSTR lpDest;
LPSTR lpSource;
LPFILEINFO lpFileInfo;
{
    LPSTR       lpSourcePtr;
    LPSTR       lpDestPtr;
    LPDISPINFO  lpDispInfo;
    RGBQUAD FAR *lpTransEntries;  // Combined color correction table

    lpDispInfo  = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);
    lpTransEntries = (RGBQUAD FAR *) GlobalLock (hGlobalCTable);

    if (lpFileInfo -> wBitsPerPixel == 24)
        RGBFilter (lpDest, lpSource, lpTransEntries, lpFileInfo -> wScanWidth, lpFileInfo -> wRowsPerStrip, lpFileInfo -> wPaddedBytesPerRow);
    else
        switch (wImportClass)
        {
            case IGR:
            {
                WORD i,j;

                lpSourcePtr = lpSource;
                lpDestPtr   = lpDest;

                for (i = 0; i < lpFileInfo -> wRowsPerStrip; i++)
                {
                    for (j = 0; j < lpFileInfo -> wScanWidth; j++)
                        *lpDestPtr++ = lpTransEntries [(BYTE) *lpSourcePtr++].rgbGreen;
    
                    lpSource += lpDispInfo -> wPaddedBytesPerRow;
                    lpDest   += lpDispInfo -> wPaddedBytesPerRow;
    
                    lpSourcePtr = lpSource;
                    lpDestPtr   = lpDest;
                }
            }
            break;

//          #ifdef NEVER
            case IMON:
            {
                WORD i,j;
                int  nValue;

                lpSourcePtr = lpSource;
                lpDestPtr   = lpDest;

                for (i = 0; i < lpFileInfo -> wRowsPerStrip; i++)
                {
                    for (j = 0; j < lpFileInfo -> wScanWidth; j++)
                    {
                        nValue = (BYTE) *lpSourcePtr++;
                        if (nValue == 1)
                            nValue = 255;

                        *lpDestPtr++ = lpTransEntries [nValue].rgbGreen;
                    }
                    lpSource += lpDispInfo -> wPaddedBytesPerRow;
                    lpDest   += lpDispInfo -> wPaddedBytesPerRow;
    
                    lpSourcePtr = lpSource;
                    lpDestPtr   = lpDest;
                }
            }
            break;
//          #endif

            default:
                _fmemcpy (lpDest, lpSource, (lpDispInfo -> wPaddedBytesPerRow * lpFileInfo -> wRowsPerStrip));

        }

    GlobalUnlock (hGlobalCTable);

}


void FAR PASCAL ColorCorrectPalette (lpRGBQuadPtr)
RGBQUAD FAR *lpRGBQuadPtr;
{
    RGBQUAD FAR *lpTransEntries;   // Combined table
    WORD i;

    lpTransEntries = (RGBQUAD FAR *) GlobalLock (hGlobalCTable);

    for (i = 0; i < 256; i++)
    {
        lpRGBQuadPtr -> rgbBlue  = lpTransEntries [(BYTE) lpRGBQuadPtr -> rgbBlue].rgbBlue;
        lpRGBQuadPtr -> rgbGreen = lpTransEntries [(BYTE) lpRGBQuadPtr -> rgbGreen].rgbGreen;
        lpRGBQuadPtr -> rgbRed   = lpTransEntries [(BYTE) lpRGBQuadPtr -> rgbRed].rgbRed;

        lpRGBQuadPtr++;
    }

    GlobalUnlock (hGlobalCTable);
}



void FAR PASCAL UpdateCorrectionTable (hTransTable)
HANDLE hTransTable;
{

    int i;

    int Red, Grn, Blu, Gra;

    LPGAMMATABLE lpGammaTable;
    LPCOLORTABLE lpColorTable;

    RGBQUAD FAR *lpTransEntries; // Combined
    RGBQUAD FAR *lpGammaEntries; // Gamma
    RGBQUAD FAR *lpColorEntries; // RGB & B/C

    /*  (Add other (* Tables *) as needed...)   */

    lpColorTable    = (LPCOLORTABLE) GlobalLock (hGlobalColor);
    lpGammaTable    = (LPGAMMATABLE) GlobalLock (hGlobalGamma);

    lpColorEntries  = (RGBQUAD FAR *) &lpColorTable -> ctRGBEntry [0];
    lpGammaEntries  = (RGBQUAD FAR *) &lpGammaTable -> gtGammaEntry [0];
    lpTransEntries  = (RGBQUAD FAR *) GlobalLock (hGlobalCTable);

    for (i = 0; i < 256; i++)
    {
        /*  First "apply" gamma   */

        Red = (BYTE) lpGammaEntries [i].rgbRed;
        Grn = (BYTE) lpGammaEntries [i].rgbGreen;
        Blu = (BYTE) lpGammaEntries [i].rgbBlue;
        Gra = (BYTE) lpGammaEntries [i].rgbReserved;

        /*  Then "apply" brightness / contrast  */

        lpTransEntries [i].rgbRed      =   lpColorEntries [Red].rgbRed;
        lpTransEntries [i].rgbGreen    =   lpColorEntries [Grn].rgbGreen;
        lpTransEntries [i].rgbBlue     =   lpColorEntries [Blu].rgbBlue;
        lpTransEntries [i].rgbReserved =   lpColorEntries [Gra].rgbReserved;

        /*  Finally "apply" color inversion, if applicable.  Note that even 
            though this is not THE most efficient way to do this, it was 
            chosen for its simplicity / readability  */

        if (bInvertColors)
        {
            lpTransEntries [i].rgbRed      =   (BYTE) (255 - lpTransEntries [i].rgbRed);
            lpTransEntries [i].rgbGreen    =   (BYTE) (255 - lpTransEntries [i].rgbGreen);
            lpTransEntries [i].rgbBlue     =   (BYTE) (255 - lpTransEntries [i].rgbBlue);
            lpTransEntries [i].rgbReserved =   (BYTE) (255 - lpTransEntries [i].rgbReserved);
        }
    }

    GlobalUnlock (hGlobalCTable);
    GlobalUnlock (hGlobalGamma);
    GlobalUnlock (hGlobalColor);
    if (image_active)
        bImageModified = TRUE;
}




/*----------------------

This eventually belongs in CPI.LIB!

------------------------*/

int FAR ValidateImage (lpFileInfo)
LPFILEINFO lpFileInfo;
{

    int nRetval = 0;



    /*  Check:  0 file width or height
                Image width too large  */

    if (lpFileInfo -> wScanWidth == 0)
        nRetval = EC_INVALIDIMAGE;

    if (lpFileInfo -> wScanHeight == 0)
        nRetval = EC_INVALIDIMAGE;


    if (lpFileInfo -> wScanWidth > 6800)
       nRetval = EC_IMAGE_TOO_WIDE;
    else
        if (lpFileInfo -> wScanWidth > 4400)
        {
            PSTR pStringBuf;
            PSTR pStringBuf2;

            pStringBuf  = LocalLock (hStringBuf);
            pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

            LoadString (hInstIP, STR_IMAGE_IS_WIDE, (LPSTR) pStringBuf, MAXSTRINGSIZE);
            LoadString (hInstIP, STR_IMAGE_SIZE_WARNING, (LPSTR) pStringBuf2, MAXSTRINGSIZE);

            if (MessageBox (hWndIP,
                           (LPSTR)pStringBuf,
                           (LPSTR)pStringBuf2,
                            MB_OKCANCEL | MB_ICONSTOP) == IDCANCEL)

                nRetval = USER_ABANDON;
            else
                nRetval = TRUE;
            LocalUnlock (hStringBuf);
        }

    return (nRetval);
}



void ReportError (nErrorCode, lpCaption)
int    nErrorCode;
LPSTR  lpCaption;
{

    PSTR  pStringBuf;

    /*  Load error string from stringtable and display message    */

    if (nErrorCode == USER_ABANDON)
        return;

    pStringBuf  = LocalLock (hStringBuf);

    if (! LoadString (hInstIP, -nErrorCode, (LPSTR)pStringBuf, MAXSTRINGSIZE))
    {
        /*  If invalid error code, general purpose error message     */
        LoadString (hInstIP, -EC_ERROR, (LPSTR)pStringBuf, MAXSTRINGSIZE);
    }
    MessageBox (hWndIP, (LPSTR)pStringBuf, lpCaption, MB_OK | MB_ICONSTOP);
    LocalUnlock (hStringBuf);
}


void FAR PASCAL Abandon ()
{
    bAbandon = TRUE;
}



void FAR SetupDispWnd (hWnd, hFileInfo)
HWND hWnd;
HANDLE hFileInfo;
{

    LPFILEINFO      lpFileInfo;
    RECT            Rect;
    RECT            rcWindow;
    int             nAvailClientWidth;
    int             nAvailClientHeight;
    WORD            wHorizRange = 0;
    WORD            wVertRange  = 0;
    BOOL            bXTooBig = FALSE;
    BOOL            bYTooBig = FALSE;
        
    if (IsWindow (hWnd))
    {
        if (! wFullView)        // Actual Size....
        {
            /*  Calculate total needed area to show the image + window attributes  */
            
            lpFileInfo = (LPFILEINFO) GlobalLock (hFileInfo);
            SetRect ((LPRECT)&rcWindow,0,0,lpFileInfo->wScanWidth,lpFileInfo->wScanHeight);
            if (! bFullScreen)  // Not in "Flip Out" view mode...
            {
                rcWindow.right += 2 * GetSystemMetrics (SM_CXBORDER);
            //  rcWindow.right += 2 * GetSystemMetrics (SM_CXFRAME);
            }

            rcWindow.bottom += GetSystemMetrics (SM_CYBORDER);
            rcWindow.bottom += GetSystemMetrics (SM_CYCAPTION);
         // rcWindow.bottom += 2 * GetSystemMetrics (SM_CYFRAME);
         // rcWindow.bottom--;

            GlobalUnlock (hFileInfo);
    
            /*  Now see actually how much space we have to work with  */
            
            GetClientRect (hWndIP, (LPRECT)&Rect); 
            nAvailClientWidth  =  Rect.right  - Rect.left;
            nAvailClientHeight =  Rect.bottom - Rect.top;
            
            /*   Adjust window size if total display window dimensions exceed available client area */
    
            if (rcWindow.bottom > nAvailClientHeight)
            {
                bYTooBig = TRUE;
                wVertRange      = (rcWindow.bottom - nAvailClientHeight);
                rcWindow.bottom = nAvailClientHeight;
            }
            
            if (rcWindow.right > nAvailClientWidth)
            {
                bXTooBig = TRUE;
                wHorizRange     = (rcWindow.right - nAvailClientWidth);
                rcWindow.right  = nAvailClientWidth;
            }


            if (bXTooBig && ! bYTooBig)   // Y MIGHT be too big also
            {
                int nAddY;

                nAddY = GetSystemMetrics (SM_CYHSCROLL);

                if ((rcWindow.bottom + nAddY) > nAvailClientHeight)
                {
                    bYTooBig = TRUE;
                    wVertRange       = (rcWindow.bottom - nAvailClientHeight);
                    rcWindow.bottom  = nAvailClientHeight;
                }
                else
                    rcWindow.bottom += nAddY;
            }

            if (bYTooBig && ! bXTooBig)   // X MIGHT be too big also
            {
                int nAddX;

                nAddX = GetSystemMetrics (SM_CXVSCROLL);

                if ((rcWindow.right + nAddX) > nAvailClientWidth)
                {
                    bXTooBig        = TRUE;
                    wHorizRange     = (rcWindow.right - nAvailClientWidth);
                    rcWindow.right  = nAvailClientWidth;
                }
                else
                    rcWindow.right += nAddX;
            }

            if (bXTooBig)
            {
                rcWindow.bottom -= GetSystemMetrics (SM_CYBORDER);
                if (bYTooBig)
                    wHorizRange += GetSystemMetrics (SM_CXVSCROLL);
            }
    
            if (bYTooBig)
            {
                rcWindow.right -= GetSystemMetrics (SM_CXBORDER);
                if (bXTooBig)
                    wVertRange  += GetSystemMetrics (SM_CYHSCROLL); 
            }

        //  if (bXTooBig || bYTooBig)
                SetWindowPos (hWnd, NULL, 0, 0, rcWindow.right, rcWindow.bottom, SWP_NOZORDER);

            SetScrollRange (hWnd, SB_HORZ, 0, wHorizRange, FALSE);
            SetScrollRange (hWnd, SB_VERT, 0, wVertRange, FALSE);
            
            SetScrollPos (hWnd, SB_HORZ, 0, TRUE);
            SetScrollPos (hWnd, SB_VERT, 0, TRUE);

            if (bFullScreen)    // We don't want scrollbars if we are looking a full screen, do we?
            {
                SetScrollRange (hWnd, SB_HORZ, 0, 0, FALSE);
                SetScrollRange (hWnd, SB_VERT, 0, 0, FALSE);
                return;
            }
        }
        else        // Old "Fit In Window" style...
        {

            int           nClientWidth;
            int           nClientHeight;
            int           dY;
            int           dX;
            float         fAspect;

            lpFileInfo = (LPFILEINFO) GlobalLock (hFileInfo);
            SetRect ((LPRECT)&rcWindow,0,0,lpFileInfo->wScanWidth,lpFileInfo->wScanHeight);
            GlobalUnlock (hFileInfo);
            
            /*  Calculate displayable client area    */
            
            GetClientRect (hWndIP, (LPRECT)&Rect); 
            dX  = 2 * GetSystemMetrics (SM_CXBORDER);
            dY  = GetSystemMetrics (SM_CYBORDER);
            dY += GetSystemMetrics (SM_CYCAPTION);
            nClientWidth  =  Rect.right  - Rect.left - dX;
            nClientHeight =  Rect.bottom - Rect.top  - dY;
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
    
            /*  Eliminate scrollbars  */
  
            SetScrollRange (hWnd, SB_HORZ, 0, 0, FALSE);
            SetScrollRange (hWnd, SB_VERT, 0, 0, FALSE);
            SetWindowPos (hWnd, NULL, 0, 0, rcWindow.right + dX,rcWindow.bottom + dY, SWP_NOZORDER);

        }
    }
}       



LPSTR FAR GlobalLockDiscardable (hBuffer)  // Maybe pass address of hBuffer so can update ...
HANDLE hBuffer;
{
    LPSTR lpRetval;
    HANDLE hNewBuffer;
    DWORD  dwBytes;

    if (GlobalFlags (hBuffer) & GMEM_DISCARDED) // Memory thrown away, try to re-allocate
    {
        dwBytes = GlobalCompact (0L);
        hNewBuffer = GlobalReAlloc (hBuffer, (GLOBALBUFSIZE * 2L), GMEM_MOVEABLE | GMEM_DISCARDABLE);
        if (hNewBuffer != hBuffer)
        {
            char Buffer [128];

            MessageBeep (NULL);
            wsprintf ((LPSTR) Buffer, (LPSTR) "I can only get %ld bytes.",dwBytes);
            MessageBox (hWndIP, (LPSTR)Buffer, "Memory", MB_OK | MB_ICONSTOP);
        }

        if (! hNewBuffer)
            lpRetval = NULL;
        else
            lpRetval = GlobalLock (hNewBuffer);
    }
    else
        lpRetval = GlobalLock (hBuffer);

    return (lpRetval);

}


void FAR PASCAL GetNameFromPath (lpFileName, lpPathName)
LPSTR lpFileName;
LPSTR lpPathName;
{
    PSTR          pStringBuf;
    PSTR          pStringBuf2;

    pStringBuf  = LocalLock (hStringBuf);
    pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

    SeparateFile ((LPSTR)pStringBuf2, (LPSTR) pStringBuf, (LPSTR) lpPathName);

    _fstrcpy (lpFileName, (LPSTR) pStringBuf);

    LocalUnlock (hStringBuf);
}


void FAR CleanFiles (void)
{
    OFSTRUCT Of;

      if (bIsCurrTemp)
      {

        if (hImportFile)    // File is also open
        {
          _lclose (hImportFile);
          hImportFile = 0;
        }

        /*  Current active image is a temp, so delete it  */
        if (OpenFile ((LPSTR)szCurrFileName, (LPOFSTRUCT)&Of, OF_EXIST) != -1)
            OpenFile ((LPSTR)szCurrFileName, (LPOFSTRUCT)&Of, OF_DELETE);
        bIsCurrTemp = FALSE;
      }

      if (szUndoFileName [0] && bIsUndoTemp)
      {
        /* Remove the undo file if it was (is) a temp file  */

        if (OpenFile ((LPSTR)szUndoFileName, (LPOFSTRUCT)&Of, OF_EXIST) != -1)
            OpenFile ((LPSTR)szUndoFileName, (LPOFSTRUCT)&Of, OF_DELETE);
        szUndoFileName [0] = 0; 
        bIsUndoTemp = FALSE;
      }

}

