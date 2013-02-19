/*---------------------------------------------------------------------------

    DOSHARP.C - 
                
               

    CREATED:    2/91   D. Ison

--------------------------------------------------------------------------*/

/*  Undefs to expedite compilation */

  
#define  NOKANJI
#define  NOPROFILER
#define  NOSOUND
#define  NOCOMM
#define  NOGDICAPMASKS
#define  NOVIRTUALKEYCODES
#define  NOSYSMETRICS
#define  NOKEYSTATES   
#define  NOSYSCOMMANDS 
#define  NORASTEROPS   
#define  OEMRESOURCE   
#define  NOATOM       
#define  NOCLIPBOARD   
#define  NOCOLOR          
#define  NODRAWTEXT   
#define  NOMETAFILE      
#define  NOSCROLL        
#define  NOTEXTMETRIC     
#define  NOWH
#define  NOHELP           
#define  NODEFERWINDOWPOS 

#include <windows.h>
#include <cpi.h>      
#include <cpifmt.h>
#include <prometer.h>
#include <tools.h>
#include <string.h>
#include <prometer.h>
#include "internal.h"
#include "cpifile.h"
#include "strtable.h"
#include "errtable.h"
#include "global.h"

int FAR PASCAL SharpenCPI (hWndImage, lpDestPath, lpSourcePath, nFlags)
HWND hWndImage;
LPSTR lpDestPath;         // Path to put output file.
LPSTR lpSourcePath;       // Path of active CPI file.  Points to szOpenFileName with a locked DS.
int   nFlags;
{
    FARPROC     lpfnSharpDlg;
    HWND        hWnd  = GetParent (hWndImage);
    OFSTRUCT    Of;
    int         nRetVal = TRUE;

    lpfnSharpDlg = MakeProcInstance ((FARPROC)SharpDlgProc, hInstance);
    nGlobalErr   = USER_ABANDON;

    Sharp.pSource = pNameBuf;
    Sharp.pDest   = Sharp.pSource + MAXPATHSIZE;
    Sharp.pTemp   = Sharp.pDest   + MAXPATHSIZE;

    _fstrcpy ((LPSTR) Sharp.pSource, lpSourcePath);
    _fstrcpy ((LPSTR) Sharp.pTemp, lpDestPath);
    _fstrcpy ((LPSTR) Sharp.pDest, lpDestPath);

    if (DialogBoxParam (hInstance, "SHARPENDLG", hWnd, lpfnSharpDlg, (DWORD) lpSourcePath))
    {
        if (SharpenImage (hWnd, &Sharp))                      // All successful ?
        {
            SendMessage (hWndImage, WM_CLOSE, 0, 0);    // Close in well-behaved form
            PostMessage (hWnd, WM_SHOWIMAGE, TOOLS_OPEN, (DWORD) (LPSTR) Sharp.pDest);// Treat like a command-line open
        }
        else 
            nRetVal = FALSE;    // Tools failure
    }
    else
        nRetVal = FALSE;        // Dialog cancel (same as USER_ABANDON)

    FreeProcInstance (lpfnSharpDlg);

    if (! nRetVal)
    {
        /*  Delete temp file if error */

        if (OpenFile ((LPSTR) Sharp.pTemp, (LPOFSTRUCT)&Of, OF_EXIST) != -1)
            OpenFile ((LPSTR) Sharp.pTemp, (LPOFSTRUCT)&Of, OF_DELETE);

        ErrorMsg (nGlobalErr, STR_SHARPEN);
    }

    return (nRetVal);
}


#define SharpenDiscardResource(r)    {  nGlobalErr = r; nRetval = 0;   \
                                      goto DiscardResource;}

int NEAR SharpenImage (hWnd, pSharp)
HWND         hWnd;
PSHARP pSharp;
{
    HANDLE      hDest       = (HANDLE) 0;
    HANDLE      hPrev       = (HANDLE) 0;
    HANDLE      hCurr       = (HANDLE) 0;
    HANDLE      hNext       = (HANDLE) 0;
    LPSTR       lpDest      = (LPSTR)  0;
    LPSTR       lpPrev      = (LPSTR)  0;
    LPSTR       lpCurr      = (LPSTR)  0;
    LPSTR       lpNext      = (LPSTR)  0;
    HIMAGE      hSourceFile = (HIMAGE) 0;
    HIMAGE      hDestFile   = (HIMAGE) 0;
    IMAGEINFO   ImageInfo;
    WORD        i;

    WORD        wBytesPerRow;
    WORD        wBytesPerPixel;
    WORD        wScanWidth;
    WORD        wScanHeight;

    nRetval     = TRUE;          // Assume successful return
    nGlobalErr  = USER_ABANDON;  // Default error code

    hSourceFile = OpenImage   ((LPSTR) pSharp -> pSource, CMAP_TO_24); 
    if (! hSourceFile)
        SharpenDiscardResource (ERR_OPENIMAGE);

    hDestFile   = CreateImage ((LPSTR) pSharp -> pTemp, hSourceFile, NULL, NULL, NULL); 
    if (! hDestFile)
        SharpenDiscardResource (ERR_CREATEIMAGE);

    GetImageInfo (hDestFile, (LPIMAGEINFO) &ImageInfo);

    wBytesPerRow    = ImageInfo.wBytesPerRow;
    wBytesPerPixel  = ImageInfo.wBytesPerPixel;
    wScanWidth      = ImageInfo.wScanWidth;
    wScanHeight     = ImageInfo.wScanHeight;

    /*  Let's do it !      */

    hDest = GlobalAlloc (GHND, (DWORD) wBytesPerRow);
    if (! hDest)
        SharpenDiscardResource (ERR_NOMEM);

    lpDest = GlobalLock (hDest);
    if (lpDest == NULL) 
        SharpenDiscardResource (ERR_NOMEM);

    hPrev = GlobalAlloc (GHND, (DWORD) (wBytesPerRow + ImageInfo.wBytesPerPixel));
    if (! hPrev)
        SharpenDiscardResource (ERR_NOMEM);

    lpPrev = GlobalLock (hPrev);
    if (lpPrev == NULL) 
        SharpenDiscardResource (ERR_NOMEM);

    hCurr = GlobalAlloc (GHND, (DWORD) (wBytesPerRow + ImageInfo.wBytesPerPixel));
    if (! hCurr)
        SharpenDiscardResource (ERR_NOMEM);

    lpCurr = GlobalLock (hCurr);
    if (lpCurr == NULL) 
        SharpenDiscardResource (ERR_NOMEM);

    hNext = GlobalAlloc (GHND, (DWORD) (wBytesPerRow + ImageInfo.wBytesPerPixel));
    if (! hNext)
        SharpenDiscardResource (ERR_NOMEM);

    lpNext = GlobalLock (hNext);
    if (lpNext == NULL) 
        SharpenDiscardResource (ERR_NOMEM);


    /*  Setup Progress meter to watch.. */

    {
        char Buffer [128];
        PSTR pStringBuf;
        PSTR pStringBuf2;
        int  nString;

        pStringBuf  = LocalLock (hStringBuf);
        pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

        EnableWindow (hWnd, FALSE);
        LoadString (hInstance, STR_SHARPEN, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        ProOpen (hWnd, NULL, MakeProcInstance (Abandon, hInstance), (LPSTR) pStringBuf);
        ProSetBarRange (wScanHeight >> 4);

        LoadString (hInstance, STR_SHARPENING_IMAGE, (LPSTR) pStringBuf, MAXSTRINGSIZE);

        if (pSharp -> SharpFilter == UNSHARP)
            nString = STR_UNSHARP_MASK;
        else
            nString = STR_HIGHPASS_FILTER;

        LoadString (hInstance, nString, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
        wsprintf ((LPSTR) Buffer, (LPSTR) pStringBuf, pSharp -> Level, (LPSTR) pStringBuf2);

        ProSetText (ID_STATUS1, (LPSTR) Buffer);

        #ifdef OLDWAY
        GetNameFromPath ((LPSTR) pStringBuf2, pSharp -> pSource);
        LoadString (hInstance, STR_SOURCE_PATH, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        _fstrcat ((LPSTR) pStringBuf, (LPSTR) pStringBuf2);
        ProSetText (ID_STATUS2, (LPSTR) pStringBuf);

        GetNameFromPath ((LPSTR) pStringBuf2, pSharp -> pDest);
        LoadString (hInstance, STR_DEST_PATH, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        _fstrcat ((LPSTR) pStringBuf, (LPSTR) pStringBuf2);
        ProSetText (ID_STATUS3, (LPSTR) pStringBuf);
        #endif

        LocalUnlock (hStringBuf);
    }


    /*  Read in first line   */

    bAbandon = FALSE;

    if (! ReadLine  (hSourceFile, lpPrev))
        SharpenDiscardResource (ERR_FILEREAD1);

    _fmemcpy ((LPSTR) lpCurr, (LPSTR) lpPrev, wBytesPerRow);

    for (i = 1; i < wScanHeight && ! bAbandon; i++)  // Skipped first line....
    {
        if (! ReadLine (hSourceFile, lpNext))
            SharpenDiscardResource (ERR_FILEREAD1);

        SharpenLine (lpDest, lpPrev, lpCurr, lpNext, wScanWidth, wBytesPerPixel, wBytesPerRow, pSharp);

        if (! WriteLine (hDestFile, lpDest))
            SharpenDiscardResource (ERR_FILEWRITE1);
        
        _fmemcpy (lpPrev, lpCurr, wBytesPerRow);
        _fmemcpy (lpCurr, lpNext, wBytesPerRow);

        if ((i % 16) == 0)
        {
            ProDeltaPos (1);
        }

    }

    /*  Take care of writing last line  */

    SharpenLine (lpDest, lpPrev, lpCurr, lpNext, wScanWidth, wBytesPerPixel, wBytesPerRow, pSharp);
    if (! WriteLine (hDestFile, lpDest))
        SharpenDiscardResource (ERR_FILEWRITE1);

    ProDeltaPos (1);

    if (bAbandon)
        SharpenDiscardResource (USER_ABANDON);

/*---  Export Resource Discard Section  ---*/

    {
        DiscardResource:

        /*  Pointers  */
    
        if (lpDest)
            GlobalUnlock (hDest);

        if (lpPrev)
            GlobalUnlock (hPrev);

        if (lpCurr)
            GlobalUnlock (hCurr);

        if (lpNext)
            GlobalUnlock (hNext);

        /*  Handles  */

        if (hDest)
            GlobalFree (hDest);
        if (hPrev)
            GlobalFree (hPrev);
        if (hCurr)
            GlobalFree (hCurr);
        if (hNext)
            GlobalFree (hNext);

        if (hSourceFile)
            CloseImage (hSourceFile, FALSE);
        if (hDestFile)
           CloseImage (hDestFile, TRUE);
 
        EnableWindow (hWnd, TRUE);
        ProClose ();   

        return (nRetval);
    }
}



void NEAR SharpenLine (lpDest, lpPrev, lpCurr, lpNext, wScanWidth, wBytesPerPixel, wBytesPerRow, pSharp)
LPSTR   lpDest;
LPSTR   lpPrev;
LPSTR   lpCurr;
LPSTR   lpNext;
WORD    wScanWidth;
WORD    wBytesPerPixel;
WORD    wBytesPerRow;
PSHARP  pSharp;
{
    WORD i;
    int nRedVal;
    int nGrnVal;
    int nBluVal;
    int nTmp;
    int nSharpness;

    LPSTR  lpPrevPtr = lpPrev;
    LPSTR  lpCurrPtr = lpCurr;
    LPSTR  lpNextPtr = lpNext;

//  nSharpness = (pSharp -> Level / 10) + 1;

    nSharpness = pSharp -> Level;

    if (pSharp -> SharpFilter == UNSHARP)
    {

        /*  Unsharp Mask.  Create a blur window, find difference, exaggerate and 
            add back    */

        for (i = 0; i < wScanWidth; i++)
        {
            nRedVal = (BYTE) *lpCurrPtr;
            
            /*  Gen an average for the 8 surrounding pixels  */
            
            if (i > 0)
            {
                nTmp    = ((BYTE)*(lpCurrPtr - wBytesPerPixel) + (BYTE)*(lpCurrPtr + wBytesPerPixel));
                nTmp   += ((BYTE)*(lpPrevPtr - wBytesPerPixel) + (BYTE)*lpPrevPtr  + (BYTE)*(lpPrevPtr + wBytesPerPixel));
                nTmp   += ((BYTE)*(lpNextPtr - wBytesPerPixel) + (BYTE)*lpNextPtr  + (BYTE)*(lpNextPtr + wBytesPerPixel));
            }
            else
            {
                nTmp    = ((BYTE)*(lpCurrPtr    ) + (BYTE)*(lpCurrPtr + wBytesPerPixel));
                nTmp   += ((BYTE)*(lpPrevPtr    ) + (BYTE)*lpPrevPtr  + (BYTE)*(lpPrevPtr + wBytesPerPixel));
                nTmp   += ((BYTE)*(lpNextPtr    ) + (BYTE)*lpNextPtr  + (BYTE)*(lpNextPtr + wBytesPerPixel));
            }
            nTmp   >>= 3;
            
            nTmp   = (nRedVal - nTmp);
            if (nTmp < 0)
                nTmp -= nSharpness;
            else
                nTmp += nSharpness;
            
            nRedVal += nTmp;
            
            if (nRedVal < 0)
                nRedVal = 0;
            
            if (nRedVal > 255)
                nRedVal = 255;
            
            *lpDest++ = (BYTE) nRedVal;
            
            lpCurrPtr++;
            lpPrevPtr++;
            lpNextPtr++;
            
            if (wBytesPerPixel == 1)
                continue;
            
            nGrnVal = (BYTE) *lpCurrPtr;
            if (i > 0)
            {
                nTmp    = ((BYTE)*(lpCurrPtr - 3) + (BYTE)*(lpCurrPtr + 3));
                nTmp   += ((BYTE)*(lpPrevPtr - 3) + (BYTE)*lpPrevPtr  + (BYTE)*(lpPrevPtr + 3));
                nTmp   += ((BYTE)*(lpNextPtr - 3) + (BYTE)*lpNextPtr  + (BYTE)*(lpNextPtr + 3));
            }
            else
            {
                nTmp    = ((BYTE)*(lpCurrPtr    ) + (BYTE)*(lpCurrPtr + 3));
                nTmp   += ((BYTE)*(lpPrevPtr    ) + (BYTE)*lpPrevPtr  + (BYTE)*(lpPrevPtr + 3));
                nTmp   += ((BYTE)*(lpNextPtr    ) + (BYTE)*lpNextPtr  + (BYTE)*(lpNextPtr + 3));
            }
            nTmp   >>= 3;
            
            nTmp   = (nGrnVal - nTmp);
            if (nTmp < 0)
                nTmp -= nSharpness;
            else
                nTmp += nSharpness;
            
            nGrnVal += nTmp;
            
            if (nGrnVal < 0)
                nGrnVal = 0;
            
            if (nGrnVal > 255)
                nGrnVal = 255;
            
            *lpDest++ = (BYTE) nGrnVal;
            
            lpCurrPtr++;
            lpPrevPtr++;
            lpNextPtr++;
            
            nBluVal = (BYTE) *lpCurrPtr;
            if (i > 0)
            {
                nTmp    = ((BYTE)*(lpCurrPtr - 3) + (BYTE)*(lpCurrPtr + 3));
                nTmp   += ((BYTE)*(lpPrevPtr - 3) + (BYTE)*lpPrevPtr  + (BYTE)*(lpPrevPtr + 3));
                nTmp   += ((BYTE)*(lpNextPtr - 3) + (BYTE)*lpNextPtr  + (BYTE)*(lpNextPtr + 3));
            }
            else
            {
                nTmp    = ((BYTE)*(lpCurrPtr    ) + (BYTE)*(lpCurrPtr + 3));
                nTmp   += ((BYTE)*(lpPrevPtr    ) + (BYTE)*lpPrevPtr  + (BYTE)*(lpPrevPtr + 3));
                nTmp   += ((BYTE)*(lpNextPtr    ) + (BYTE)*lpNextPtr  + (BYTE)*(lpNextPtr + 3));
            }
            nTmp   >>= 3;
            
            nTmp   = (nBluVal - nTmp);
            if (nTmp < 0)
                nTmp -= nSharpness;
            else
                nTmp += nSharpness;
            
            nBluVal += nTmp;
            
            if (nBluVal < 0)
                nBluVal = 0;
            
            if (nBluVal > 255)
                nBluVal = 255;
            
            *lpDest++ = (BYTE) nBluVal;
            
            lpCurrPtr++;
            lpPrevPtr++;
            lpNextPtr++;
        }
    }
    else    // Highpass filter
    {
        int nDelta;

        for (i = 0; i < wScanWidth; i++)
        {
            nRedVal = (BYTE) *lpCurrPtr;
        
            if (i > 0)
            {
                nTmp    = ((BYTE)*(lpCurrPtr - wBytesPerPixel) + (BYTE)*(lpCurrPtr + wBytesPerPixel));
                nTmp   += ((BYTE)*(lpPrevPtr - wBytesPerPixel) + (BYTE)*lpPrevPtr  + (BYTE)*(lpPrevPtr + wBytesPerPixel));
                nTmp   += ((BYTE)*(lpNextPtr - wBytesPerPixel) + (BYTE)*lpNextPtr  + (BYTE)*(lpNextPtr + wBytesPerPixel));
            }
            else
            {
                nTmp    = ((BYTE)*(lpCurrPtr    ) + (BYTE)*(lpCurrPtr + wBytesPerPixel));
                nTmp   += ((BYTE)*(lpPrevPtr    ) + (BYTE)*lpPrevPtr  + (BYTE)*(lpPrevPtr + wBytesPerPixel));
                nTmp   += ((BYTE)*(lpNextPtr    ) + (BYTE)*lpNextPtr  + (BYTE)*(lpNextPtr + wBytesPerPixel));
            }
            nDelta = (nRedVal << 3) - nTmp;

            nRedVal += ((nDelta * nSharpness) >> 4);
            if (nRedVal < 0)
                nRedVal = 0;
        
            if (nRedVal > 255)
                nRedVal = 255;
        
            *lpDest++ = (BYTE) nRedVal;
        
            lpCurrPtr++;
            lpPrevPtr++;
            lpNextPtr++;
        
            if (wBytesPerPixel == 1)
                continue;
        
            nGrnVal = (BYTE) *lpCurrPtr;
        
            if (i > 0)
            {
                nTmp    = ((BYTE)*(lpCurrPtr - 3) + (BYTE)*(lpCurrPtr + 3));
                nTmp   += ((BYTE)*(lpPrevPtr - 3) + (BYTE)*lpPrevPtr  + (BYTE)*(lpPrevPtr + 3));
                nTmp   += ((BYTE)*(lpNextPtr - 3) + (BYTE)*lpNextPtr  + (BYTE)*(lpNextPtr + 3));
            }
            else
            {
                nTmp    = ((BYTE)*(lpCurrPtr    ) + (BYTE)*(lpCurrPtr + 3));
                nTmp   += ((BYTE)*(lpPrevPtr    ) + (BYTE)*lpPrevPtr  + (BYTE)*(lpPrevPtr + 3));
                nTmp   += ((BYTE)*(lpNextPtr    ) + (BYTE)*lpNextPtr  + (BYTE)*(lpNextPtr + 3));
            }
            nDelta = (nGrnVal << 3) - nTmp;
        
            nGrnVal += ((nDelta * nSharpness) >> 4);
            if (nGrnVal < 0)
                nGrnVal = 0;
        
            if (nGrnVal > 255)
                nGrnVal = 255;
        
            *lpDest++ = (BYTE) nGrnVal;
        
            lpCurrPtr++;
            lpPrevPtr++;
            lpNextPtr++;
        
            nBluVal = (BYTE) *lpCurrPtr;
        
            if (i > 0)
            {
                nTmp    = ((BYTE)*(lpCurrPtr - 3) + (BYTE)*(lpCurrPtr + 3));
                nTmp   += ((BYTE)*(lpPrevPtr - 3) + (BYTE)*lpPrevPtr  + (BYTE)*(lpPrevPtr + 3));
                nTmp   += ((BYTE)*(lpNextPtr - 3) + (BYTE)*lpNextPtr  + (BYTE)*(lpNextPtr + 3));
            }
            else
            {
                nTmp    = ((BYTE)*(lpCurrPtr    ) + (BYTE)*(lpCurrPtr + 3));
                nTmp   += ((BYTE)*(lpPrevPtr    ) + (BYTE)*lpPrevPtr  + (BYTE)*(lpPrevPtr + 3));
                nTmp   += ((BYTE)*(lpNextPtr    ) + (BYTE)*lpNextPtr  + (BYTE)*(lpNextPtr + 3));
            }
            nDelta = (nBluVal << 3) - nTmp;
        
            nBluVal += ((nDelta * nSharpness) >> 4);
            if (nBluVal < 0)
                nBluVal = 0;
        
            if (nBluVal > 255)
                nBluVal = 255;
        
            *lpDest++ = (BYTE) nBluVal;

            lpCurrPtr++;
            lpPrevPtr++;
            lpNextPtr++;
        }
    }
    return;
}

