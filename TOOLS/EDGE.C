/*---------------------------------------------------------------------------

    EDGE.C - 
                
               

    CREATED:    3/91   D. Ison

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

int FAR PASCAL EnhanceEdgeCPI (hWndImage, lpDestPath, lpSourcePath, nFlags)
HWND hWndImage;
LPSTR lpDestPath;         // Path to put output file.
LPSTR lpSourcePath;       // Path of active CPI file.  Points to szOpenFileName with a locked DS.
int   nFlags;
{
    FARPROC     lpfnEdgeDlg;
    HWND        hWnd  = GetParent (hWndImage);
    OFSTRUCT    Of;
    int         nRetVal = TRUE;
    char        DialogString [80];

    lpfnEdgeDlg = MakeProcInstance ((FARPROC)EdgeDlgProc, hInstance);
    nGlobalErr  = USER_ABANDON;

    Edge.Filter = (BYTE) nFlags;                            // Pass in filter

    Edge.pSource = pNameBuf;
    Edge.pDest   = Edge.pSource + MAXPATHSIZE;
    Edge.pTemp   = Edge.pDest   + MAXPATHSIZE;

    _fstrcpy ((LPSTR) Edge.pSource, lpSourcePath);
    _fstrcpy ((LPSTR) Edge.pTemp, lpDestPath);
    _fstrcpy ((LPSTR) Edge.pDest, lpDestPath);

    if (Edge.Filter == EDGE_ENHANCE)
        _fstrcpy ((LPSTR) DialogString, "EDGEDLG");
    else
        _fstrcpy ((LPSTR) DialogString, "CONTDLG");

    if (DialogBoxParam (hInstance, DialogString, hWnd, lpfnEdgeDlg, (DWORD) lpSourcePath))
    {

        if (EnhanceEdgeImage (hWnd, &Edge))                 // All successful ?
        {
            SendMessage (hWndImage, WM_CLOSE, 0, 0);    // Close in well-behaved form
            PostMessage (hWnd, WM_SHOWIMAGE, TOOLS_OPEN, (DWORD) (LPSTR) Edge.pDest);// Treat like a command-line open
        }
        else 
            nRetVal = FALSE;    // Tools failure
    }
    else
        nRetVal = FALSE;        // User abandon

    FreeProcInstance (lpfnEdgeDlg);

    if (! nRetVal)
    {
        /*  Delete temp file if error */

        if (OpenFile ((LPSTR) Edge.pTemp, (LPOFSTRUCT)&Of, OF_EXIST) != -1)
            OpenFile ((LPSTR) Edge.pTemp, (LPOFSTRUCT)&Of, OF_DELETE);

        ErrorMsg (nGlobalErr, (Edge.Filter == EDGE_ENHANCE ? STR_ENHANCE_EDGE : STR_CREATE_LINE_DRAWING));
    }

    return (nRetVal);
}


#define EnhanceEdgeDiscardResource(r)    {  nGlobalErr = r; nRetval = 0;   \
                                      goto DiscardResource;}

int NEAR EnhanceEdgeImage (hWnd, pEdge)
HWND  hWnd;
PEDGE pEdge;
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

    hSourceFile = OpenImage   ((LPSTR) pEdge -> pSource, CMAP_TO_24); 
    if (! hSourceFile)
        EnhanceEdgeDiscardResource (ERR_OPENIMAGE);

    hDestFile   = CreateImage ((LPSTR) pEdge -> pTemp, hSourceFile, NULL, NULL, NULL); 
    if (! hDestFile)
        EnhanceEdgeDiscardResource (ERR_CREATEIMAGE);

    GetImageInfo (hDestFile, (LPIMAGEINFO) &ImageInfo);

    wBytesPerRow    = ImageInfo.wBytesPerRow;
    wBytesPerPixel  = ImageInfo.wBytesPerPixel;
    wScanWidth      = ImageInfo.wScanWidth;
    wScanHeight     = ImageInfo.wScanHeight;

    /*  Let's do it !      */

    hDest = GlobalAlloc (GHND, (DWORD) wBytesPerRow);
    if (! hDest)
        EnhanceEdgeDiscardResource (ERR_NOMEM);

    lpDest = GlobalLock (hDest);
    if (lpDest == NULL) 
        EnhanceEdgeDiscardResource (ERR_NOMEM);

    hPrev = GlobalAlloc (GHND, (DWORD) (wBytesPerRow + ImageInfo.wBytesPerPixel));
    if (! hPrev)
        EnhanceEdgeDiscardResource (ERR_NOMEM);

    lpPrev = GlobalLock (hPrev);
    if (lpPrev == NULL) 
        EnhanceEdgeDiscardResource (ERR_NOMEM);

    hCurr = GlobalAlloc (GHND, (DWORD) (wBytesPerRow + ImageInfo.wBytesPerPixel));
    if (! hCurr)
        EnhanceEdgeDiscardResource (ERR_NOMEM);

    lpCurr = GlobalLock (hCurr);
    if (lpCurr == NULL) 
        EnhanceEdgeDiscardResource (ERR_NOMEM);

    hNext = GlobalAlloc (GHND, (DWORD) (wBytesPerRow + ImageInfo.wBytesPerPixel));
    if (! hNext)
        EnhanceEdgeDiscardResource (ERR_NOMEM);

    lpNext = GlobalLock (hNext);
    if (lpNext == NULL) 
        EnhanceEdgeDiscardResource (ERR_NOMEM);


    /*  Setup Progress meter to watch.. */

    {
        char Buffer [128];
        PSTR pStringBuf;
        PSTR pStringBuf2;
        int  nString;


        pStringBuf  = LocalLock (hStringBuf);
        pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

        EnableWindow (hWnd, FALSE);

        nString = (pEdge -> Filter == EDGE_ENHANCE) ? STR_ENHANCE_EDGE : STR_CREATE_LINE_DRAWING;
        LoadString (hInstance, nString, (LPSTR) pStringBuf, MAXSTRINGSIZE);

        ProOpen (hWnd, NULL, MakeProcInstance (Abandon, hInstance), (LPSTR) pStringBuf);
        ProSetBarRange (wScanHeight >> 4);

        nString = (pEdge -> Filter == EDGE_ENHANCE) ? STR_EDGE_ENHANCING_IMAGE : STR_CREATING_LINE_DRAWING;
        
        LoadString (hInstance, nString, (LPSTR) pStringBuf, MAXSTRINGSIZE);

        if (pEdge -> Filter == EDGE_ENHANCE)
            wsprintf ((LPSTR) Buffer, (LPSTR) pStringBuf, pEdge -> Level, pEdge -> Sensitivity);
        else
            wsprintf ((LPSTR) Buffer, (LPSTR) pStringBuf, pEdge -> Sensitivity);

        ProSetText (ID_STATUS1, (LPSTR) Buffer);

        #ifdef OLDWAY
        GetNameFromPath ((LPSTR) pStringBuf2, pEdge -> pSource);
        LoadString (hInstance, STR_SOURCE_PATH, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        _fstrcat ((LPSTR) pStringBuf, (LPSTR) pStringBuf2);
        ProSetText (ID_STATUS2, (LPSTR) pStringBuf);

        GetNameFromPath ((LPSTR) pStringBuf2, pEdge -> pDest);
        LoadString (hInstance, STR_DEST_PATH, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        _fstrcat ((LPSTR) pStringBuf, (LPSTR) pStringBuf2);
        ProSetText (ID_STATUS3, (LPSTR) pStringBuf);
        #endif

        LocalUnlock (hStringBuf);
    }


    /*  Read in first line   */

    bAbandon = FALSE;

    if (! ReadLine  (hSourceFile, lpPrev))
        EnhanceEdgeDiscardResource (ERR_FILEREAD1);

    _fmemcpy ((LPSTR) lpCurr, (LPSTR) lpPrev, wBytesPerRow);

    for (i = 1; i < wScanHeight && ! bAbandon; i++)  // Skipped first line....
    {
        if (! ReadLine (hSourceFile, lpNext))
            EnhanceEdgeDiscardResource (ERR_FILEREAD1);

        EnhanceEdgeLine (lpDest, lpPrev, lpCurr, lpNext, wScanWidth, wBytesPerPixel, wBytesPerRow, pEdge);

        if (! WriteLine (hDestFile, lpDest))
            EnhanceEdgeDiscardResource (ERR_FILEWRITE1);
        
        _fmemcpy (lpPrev, lpCurr, wBytesPerRow);
        _fmemcpy (lpCurr, lpNext, wBytesPerRow);

        if ((i % 16) == 0)
            ProDeltaPos (1);

    }

    /*  Take care of writing last line  */

    EnhanceEdgeLine (lpDest, lpPrev, lpCurr, lpNext, wScanWidth, wBytesPerPixel, wBytesPerRow, pEdge);
    if (! WriteLine (hDestFile, lpDest))
        EnhanceEdgeDiscardResource (ERR_FILEWRITE1);

    ProDeltaPos (1);

    if (bAbandon)
        EnhanceEdgeDiscardResource (USER_ABANDON);

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


void NEAR EnhanceEdgeLine (lpDest, lpPrev, lpCurr, lpNext, wScanWidth, wBytesPerPixel, wBytesPerRow, pEdge)
LPSTR   lpDest;
LPSTR   lpPrev;
LPSTR   lpCurr;
LPSTR   lpNext;
WORD    wScanWidth;
WORD    wBytesPerPixel;
WORD    wBytesPerRow;
PEDGE  pEdge;
{
    WORD    i;
    int     nRedVal;
    int     nGrnVal;
    int     nBluVal;
    int     nTmp;
    int     nTmp1;
    int     nTmp2;
    int     nOldVal;
    LPSTR   lpPrevPtr  = lpPrev;
    LPSTR   lpCurrPtr  = lpCurr;
    LPSTR   lpNextPtr  = lpNext;
    int     nThreshold = (100 - (BYTE) pEdge -> Sensitivity);
    int     nLevel     = (BYTE) pEdge -> Level;
    BOOL    bNeg;

    if (pEdge -> Filter == EDGE_ENHANCE)
    {

        /*  For each pixel, do the gradient  */

        for (i = 0; i < wScanWidth; i++)
        {
            nOldVal = nRedVal = (BYTE) *lpCurrPtr;
    
            nTmp1   = nRedVal - (int) (BYTE)*(lpCurrPtr + wBytesPerPixel);       // "x + 1"
    
            bNeg = FALSE;
            if (nTmp1 < 0)
            {
                nTmp1 *= -1;
                bNeg = TRUE;
            }
    
            nTmp2   = nRedVal - (int) (BYTE)*lpNextPtr;             // " y + 1"
            if (nTmp2 < 0)
            {
                nTmp2 *= -1;
                bNeg = TRUE;
            }
    
            nTmp = (nTmp1 + nTmp2);
    
            if (nTmp > nThreshold)
            {
                if (bNeg)
                    nRedVal -= nLevel;
                else
                    nRedVal += nLevel;
    
                if (nRedVal < 0)
                    nRedVal = 0;
                else
                    if (nRedVal > 255)
                        nRedVal = 255;
            }
            else
                nRedVal = nOldVal;
    
            *lpDest++ = (BYTE) nRedVal;
    
            lpCurrPtr++;
            lpPrevPtr++;
            lpNextPtr++;
    
            if (wBytesPerPixel == 1)
                continue;
    
            nOldVal = nGrnVal = (BYTE) *lpCurrPtr;
    
            nTmp1   = nGrnVal - (int) (BYTE)*(lpCurrPtr + 3);       // "x + 1"
            bNeg = FALSE;
            if (nTmp1 < 0)
            {
                nTmp1 *= -1;
                bNeg = TRUE;
            }
    
            nTmp2   = nGrnVal - (int) (BYTE)*lpNextPtr;             // " y + 1"
            if (nTmp2 < 0)
            {
                nTmp2 *= -1;
                bNeg = TRUE;
            }
    
            nTmp = (nTmp1 + nTmp2);
    
            if (nTmp > nThreshold)
            {
                if (bNeg)
                    nGrnVal -= nLevel;
                else
                    nGrnVal += nLevel;
    
                if (nGrnVal < 0)
                    nGrnVal = 0;
                else
                    if (nGrnVal > 255)
                        nGrnVal = 255;
            }
            else
                nGrnVal = nOldVal;
    
            *lpDest++ = (BYTE) nGrnVal;
    
            lpCurrPtr++;
            lpPrevPtr++;
            lpNextPtr++;
    
            nOldVal = nBluVal = (BYTE) *lpCurrPtr;
    
            nTmp1   = nBluVal - (int) (BYTE)*(lpCurrPtr + 3);       // "x + 1"
            bNeg = FALSE;
            if (nTmp1 < 0)
            {
                nTmp1 *= -1;
                bNeg = TRUE;
            }
    
            nTmp2   = nBluVal - (int) (BYTE)*lpNextPtr;             // " y + 1"
            if (nTmp2 < 0)
            {
                nTmp2 *= -1;
                bNeg = TRUE;
            }
    
            nTmp = (nTmp1 + nTmp2);
    
            if (nTmp > nThreshold)
            {
                if (bNeg)
                    nBluVal -= nLevel;
                else
                    nBluVal += nLevel;
    
                if (nBluVal < 0)
                    nBluVal = 0;
                else
                    if (nBluVal > 255)
                        nBluVal = 255;
            }
            else
                nBluVal = nOldVal;

            *lpDest++ = (BYTE) nBluVal;

            lpCurrPtr++;
            lpPrevPtr++;
            lpNextPtr++;
        }
    }
    else
    {

        for (i = 0; i < wScanWidth; i++)
        {
            nOldVal = nRedVal = (BYTE) *lpCurrPtr;
    
            nTmp1   = nRedVal - (int) (BYTE)*(lpCurrPtr + wBytesPerPixel);       // "x + 1"
            if (nTmp1 < 0)
                nTmp1 *= -1;
    
            nTmp2   = nRedVal - (int) (BYTE)*lpNextPtr;             // " y + 1"
            if (nTmp2 < 0)
                nTmp2 *= -1;
    
            nTmp = (nTmp1 + nTmp2);
    
            if (nTmp > nThreshold)
                nRedVal = 0;
            else
                nRedVal = 255;
    
            *lpDest++ = (BYTE) nRedVal;
    
            lpCurrPtr++;
            lpPrevPtr++;
            lpNextPtr++;
    
            if (wBytesPerPixel == 1)
                continue;
    
            nOldVal = nGrnVal = (BYTE) *lpCurrPtr;
    
            nTmp1   = nGrnVal - (int) (BYTE)*(lpCurrPtr + wBytesPerPixel);       // "x + 1"
            if (nTmp1 < 0)
                nTmp1 *= -1;
    
            nTmp2   = nGrnVal - (int) (BYTE)*lpNextPtr;             // " y + 1"
            if (nTmp2 < 0)
                nTmp2 *= -1;
    
            nTmp = (nTmp1 + nTmp2);
    
            if (nTmp > nThreshold)
                nGrnVal = 0;
            else
                nGrnVal = 255;
    
            *lpDest++ = (BYTE) nGrnVal;
    
            lpCurrPtr++;
            lpPrevPtr++;
            lpNextPtr++;
    
    
    
            nOldVal = nBluVal = (BYTE) *lpCurrPtr;
    
            nTmp1   = nBluVal - (int) (BYTE)*(lpCurrPtr + wBytesPerPixel);       // "x + 1"
            if (nTmp1 < 0)
                nTmp1 *= -1;
    
            nTmp2   = nBluVal - (int) (BYTE)*lpNextPtr;             // " y + 1"
            if (nTmp2 < 0)
                nTmp2 *= -1;
    
            nTmp = (nTmp1 + nTmp2);
    
            if (nTmp > nThreshold)
                nBluVal = 0;
            else
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
