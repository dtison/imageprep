/*---------------------------------------------------------------------------

    SMOOTH.C    
                
               

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
#include <search.h>
#include "internal.h"
#include "cpifile.h"
#include "strtable.h"
#include "errtable.h"
#include "global.h"

int FAR compare (int *, int *);



int FAR PASCAL SmoothCPI (hWndImage, lpDestPath, lpSourcePath, nFlags)
HWND  hWndImage;           // Window handle of active image (Same as hWndDisplay)
LPSTR lpDestPath;          // Path to put output file.
LPSTR lpSourcePath;        // Path of active CPI file.  Points to szOpenFileName with a locked DS.
int   nFlags;
{
    FARPROC     lpfnSmoothDlg;
    LPSTR       lpPtr = NULL;
    HWND        hWnd  = GetParent (hWndImage);
    OFSTRUCT    Of;
    int         nRetVal = TRUE;

    lpfnSmoothDlg = MakeProcInstance ((FARPROC)SmoothDlgProc, hInstance);
    nGlobalErr   = USER_ABANDON;

    Smooth.pSource = pNameBuf;
    Smooth.pDest   = Smooth.pSource + MAXPATHSIZE;
    Smooth.pTemp   = Smooth.pDest   + MAXPATHSIZE;

    _fstrcpy ((LPSTR) Smooth.pSource, lpSourcePath);
    _fstrcpy ((LPSTR) Smooth.pTemp, lpDestPath);
    _fstrcpy ((LPSTR) Smooth.pDest, lpDestPath);

    Smooth.SmoothFilter = (BYTE) (nFlags ? MEDIAN : LOWPASS);

    if (nFlags)
        if (! DialogBoxParam (hInstance, "SMOOTHDLG", hWnd, lpfnSmoothDlg, (DWORD) lpSourcePath))
            nRetVal = FALSE;        // Dialog cancel (same as USER_ABANDON)
    
    if (nRetVal)
    {
        if (SmoothImage (hWnd, &Smooth))                      // All successful ?
        {
            SendMessage (hWndImage, WM_CLOSE, 0, 0);    // Close in well-behaved form
            PostMessage (hWnd, WM_SHOWIMAGE, TOOLS_OPEN, (DWORD) (LPSTR) Smooth.pDest);// Treat like a command-line open
        }
        else 
        {
            /*  Delete temp file  */

            if (OpenFile ((LPSTR) Smooth.pTemp, (LPOFSTRUCT)&Of, OF_EXIST) != -1)
                OpenFile ((LPSTR) Smooth.pTemp, (LPOFSTRUCT)&Of, OF_DELETE);

            ErrorMsg (nGlobalErr, (Smooth.SmoothFilter == LOWPASS ? STR_SMOOTH : STR_REMOVE_NOISE));
            nRetVal = FALSE;      // Tools failure
        }
    }
    return (nRetVal);
}


#define SmoothDiscardResource(r)    {  nGlobalErr = r; nRetval = 0;   \
                                      goto DiscardResource;}

int NEAR SmoothImage (hWnd, pSmooth)
HWND         hWnd;
PSMOOTH pSmooth;
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

    hSourceFile = OpenImage   ((LPSTR) pSmooth -> pSource, CMAP_TO_24); 
    if (! hSourceFile)
        SmoothDiscardResource (ERR_OPENIMAGE);

    hDestFile   = CreateImage ((LPSTR) pSmooth -> pTemp, hSourceFile, NULL, NULL, NULL); 
    if (! hDestFile)
        SmoothDiscardResource (ERR_CREATEIMAGE);

    GetImageInfo (hDestFile, (LPIMAGEINFO) &ImageInfo);

    wBytesPerRow    = ImageInfo.wBytesPerRow;
    wBytesPerPixel  = ImageInfo.wBytesPerPixel;
    wScanWidth      = ImageInfo.wScanWidth;
    wScanHeight     = ImageInfo.wScanHeight;

    /*  Let's do it !      */

    hDest = GlobalAlloc (GHND, (DWORD) wBytesPerRow);
    if (! hDest)
        SmoothDiscardResource (ERR_NOMEM);

    lpDest = GlobalLock (hDest);
    if (lpDest == NULL) 
        SmoothDiscardResource (ERR_NOMEM);

    hPrev = GlobalAlloc (GHND, (DWORD) (wBytesPerRow + ImageInfo.wBytesPerPixel));
    if (! hPrev)
        SmoothDiscardResource (ERR_NOMEM);

    lpPrev = GlobalLock (hPrev);
    if (lpPrev == NULL) 
        SmoothDiscardResource (ERR_NOMEM);

    hCurr = GlobalAlloc (GHND, (DWORD) (wBytesPerRow + ImageInfo.wBytesPerPixel));
    if (! hCurr)
        SmoothDiscardResource (ERR_NOMEM);

    lpCurr = GlobalLock (hCurr);
    if (lpCurr == NULL) 
        SmoothDiscardResource (ERR_NOMEM);

    hNext = GlobalAlloc (GHND, (DWORD) (wBytesPerRow + ImageInfo.wBytesPerPixel));
    if (! hNext)
        SmoothDiscardResource (ERR_NOMEM);

    lpNext = GlobalLock (hNext);
    if (lpNext == NULL) 
        SmoothDiscardResource (ERR_NOMEM);


    /*  Setup Progress meter to watch.. */
    {
        PSTR pStringBuf;
        PSTR pStringBuf2;
        int  nString;

        pStringBuf  = LocalLock (hStringBuf);
        pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

        EnableWindow (hWnd, FALSE);

        if (pSmooth -> SmoothFilter == LOWPASS)
            nString = STR_SMOOTH;
        else
            nString = STR_REMOVE_NOISE;
        
        LoadString (hInstance, nString, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        ProOpen (hWnd, NULL, MakeProcInstance (Abandon, hInstance), (LPSTR) pStringBuf);
        ProSetBarRange (wScanHeight >> 4);

//      LoadString (hInstance, STR_SMOOTHING_IMAGE, (LPSTR) pStringBuf, MAXSTRINGSIZE);

        if (pSmooth -> SmoothFilter == LOWPASS)
            nString = STR_SMOOTHING_IMAGE;
        else
            nString = STR_REMOVING_NOISE;

        LoadString (hInstance, nString, (LPSTR) pStringBuf, MAXSTRINGSIZE);

        ProSetText (ID_STATUS1, (LPSTR) pStringBuf);

        #ifdef OLDWAY
        GetNameFromPath ((LPSTR) pStringBuf2, pSmooth -> pSource);
        LoadString (hInstance, STR_SOURCE_PATH, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        _fstrcat ((LPSTR) pStringBuf, (LPSTR) pStringBuf2);
        ProSetText (ID_STATUS2, (LPSTR) pStringBuf);

        GetNameFromPath ((LPSTR) pStringBuf2, pSmooth -> pDest);
        LoadString (hInstance, STR_DEST_PATH, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        _fstrcat ((LPSTR) pStringBuf, (LPSTR) pStringBuf2);
        ProSetText (ID_STATUS3, (LPSTR) pStringBuf);
        #endif

        LocalUnlock (hStringBuf);
    }

    /*  Read in first line   */

    bAbandon = FALSE;

    if (! ReadLine  (hSourceFile, lpPrev))
        SmoothDiscardResource (ERR_FILEREAD1);

    _fmemcpy ((LPSTR) lpCurr, (LPSTR) lpPrev, wBytesPerRow);

    for (i = 1; i < wScanHeight && ! bAbandon; i++)  // Skipped first line....
    {
        if (! ReadLine (hSourceFile, lpNext))
            SmoothDiscardResource (ERR_FILEREAD1);

        SmoothLine (lpDest, lpPrev, lpCurr, lpNext, wScanWidth, wBytesPerPixel, wBytesPerRow, pSmooth);

        if (! WriteLine (hDestFile, lpDest))
            SmoothDiscardResource (ERR_FILEWRITE1);
        
        _fmemcpy (lpPrev, lpCurr, wBytesPerRow);
        _fmemcpy (lpCurr, lpNext, wBytesPerRow);

        Yield ();
        if ((i % 16) == 0)
            ProDeltaPos (1);

    }

    /*  Take care of writing last line  */

    SmoothLine (lpDest, lpPrev, lpCurr, lpNext, wScanWidth, wBytesPerPixel, wBytesPerRow, pSmooth);
    if (! WriteLine (hDestFile, lpDest))
        SmoothDiscardResource (ERR_FILEWRITE1);

    ProDeltaPos (1);

    if (bAbandon)
        SmoothDiscardResource (USER_ABANDON);

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



void NEAR SmoothLine (lpDest, lpPrev, lpCurr, lpNext, wScanWidth, wBytesPerPixel, wBytesPerRow, pSmooth)
LPSTR   lpDest;
LPSTR   lpPrev;
LPSTR   lpCurr;
LPSTR   lpNext;
WORD    wScanWidth;
WORD    wBytesPerPixel;
WORD    wBytesPerRow;
PSMOOTH pSmooth;
{
    WORD i;
    int nTmp, nTmp2;
    int     nVals [9];

    LPSTR  lpPrevPtr = lpPrev;
    LPSTR  lpCurrPtr = lpCurr;
    LPSTR  lpNextPtr = lpNext;

    if (pSmooth -> SmoothFilter == LOWPASS)
    {
        /*  Lowpass Filter.  Create a blur window and average  */

        for (i = 0; i < wScanWidth; i++)
        {
            
            /*  Gen an average for the 8 surrounding pixels  */
            
            if (i > 0)
            {
                nTmp    = (BYTE) *(lpCurrPtr - wBytesPerPixel) + (BYTE) *(lpCurrPtr + wBytesPerPixel);
                nTmp   += (BYTE) *lpPrevPtr + (BYTE) *lpNextPtr;
                nTmp  <<= 1;

                nTmp2 = nTmp;

                nTmp    = (BYTE) *(lpPrevPtr - wBytesPerPixel) + (BYTE) *(lpPrevPtr + wBytesPerPixel);
                nTmp   += (BYTE) *(lpNextPtr - wBytesPerPixel) + (BYTE) *(lpNextPtr + wBytesPerPixel);

                nTmp2 += nTmp;

                nTmp2 += (int) (((BYTE) *lpCurrPtr) << 2);

            }
            else
            {
                nTmp2   = ((BYTE)*(lpCurrPtr    ) + (BYTE)*(lpCurrPtr + wBytesPerPixel));
                nTmp2  += ((BYTE)*(lpPrevPtr    ) + (BYTE)*lpPrevPtr  + (BYTE)*(lpPrevPtr + wBytesPerPixel));
                nTmp2  += ((BYTE)*(lpNextPtr    ) + (BYTE)*lpNextPtr  + (BYTE)*(lpNextPtr + wBytesPerPixel));
            }
        
            *lpDest++ = (BYTE) (nTmp2 >> 4);
            
            lpCurrPtr++;
            lpPrevPtr++;
            lpNextPtr++;
            
            if (wBytesPerPixel == 1)
                continue;
            
            if (i > 0)
            {
                nTmp    = (BYTE) *(lpCurrPtr - wBytesPerPixel) + (BYTE) *(lpCurrPtr + wBytesPerPixel);
                nTmp   += (BYTE) *lpPrevPtr + (BYTE) *lpNextPtr;
                nTmp  <<= 1;

                nTmp2 = nTmp;

                nTmp    = (BYTE) *(lpPrevPtr - wBytesPerPixel) + (BYTE) *(lpPrevPtr + wBytesPerPixel);
                nTmp   += (BYTE) *(lpNextPtr - wBytesPerPixel) + (BYTE) *(lpNextPtr + wBytesPerPixel);

                nTmp2 += nTmp;

                nTmp2 += (int) (((BYTE) *lpCurrPtr) << 2);

            }
            else
            {
                nTmp2   = ((BYTE)*(lpCurrPtr    ) + (BYTE)*(lpCurrPtr + wBytesPerPixel));
                nTmp2  += ((BYTE)*(lpPrevPtr    ) + (BYTE)*lpPrevPtr  + (BYTE)*(lpPrevPtr + wBytesPerPixel));
                nTmp2  += ((BYTE)*(lpNextPtr    ) + (BYTE)*lpNextPtr  + (BYTE)*(lpNextPtr + wBytesPerPixel));
            }
        
            *lpDest++ = (BYTE) (nTmp2 >> 4);
            
            lpCurrPtr++;
            lpPrevPtr++;
            lpNextPtr++;
            
            if (i > 0)
            {
                nTmp    = (BYTE) *(lpCurrPtr - wBytesPerPixel) + (BYTE) *(lpCurrPtr + wBytesPerPixel);
                nTmp   += (BYTE) *lpPrevPtr + (BYTE) *lpNextPtr;
                nTmp  <<= 1;

                nTmp2 = nTmp;

                nTmp    = (BYTE) *(lpPrevPtr - wBytesPerPixel) + (BYTE) *(lpPrevPtr + wBytesPerPixel);
                nTmp   += (BYTE) *(lpNextPtr - wBytesPerPixel) + (BYTE) *(lpNextPtr + wBytesPerPixel);

                nTmp2 += nTmp;

                nTmp2 += (int) (((BYTE) *lpCurrPtr) << 2);

            }
            else
            {
                nTmp2   = ((BYTE)*(lpCurrPtr    ) + (BYTE)*(lpCurrPtr + wBytesPerPixel));
                nTmp2  += ((BYTE)*(lpPrevPtr    ) + (BYTE)*lpPrevPtr  + (BYTE)*(lpPrevPtr + wBytesPerPixel));
                nTmp2  += ((BYTE)*(lpNextPtr    ) + (BYTE)*lpNextPtr  + (BYTE)*(lpNextPtr + wBytesPerPixel));
            }
        
            *lpDest++ = (BYTE) (nTmp2 >> 4);
            
            lpCurrPtr++;
            lpPrevPtr++;
            lpNextPtr++;
        }
    }
    else    // Median filter with threshold
    {
        int nLevel = (BYTE) (100 - pSmooth -> Level + 1);

        for (i = 0; i < wScanWidth; i++)
        {
            if (i > 0)
            {
                nVals [0] = (BYTE) *(lpCurrPtr - wBytesPerPixel);
                nVals [1] = (BYTE) *(lpCurrPtr + wBytesPerPixel);
                nVals [2] = (BYTE) *(lpPrevPtr - wBytesPerPixel);
                nVals [3] = (BYTE) *lpPrevPtr;
                nVals [4] = (BYTE) *(lpPrevPtr + wBytesPerPixel);
                nVals [5] = (BYTE) *(lpNextPtr - wBytesPerPixel);
                nVals [6] = (BYTE) *lpNextPtr;
                nVals [7] = (BYTE) *(lpNextPtr + wBytesPerPixel);
                nVals [8] = (BYTE) *lpCurrPtr;
            }
            else
            {
                nVals [0] = (BYTE) *(lpCurrPtr                 );
                nVals [1] = (BYTE) *(lpCurrPtr + wBytesPerPixel);
                nVals [2] = (BYTE) *(lpPrevPtr                 );
                nVals [3] = (BYTE) *lpPrevPtr;
                nVals [4] = (BYTE) *(lpPrevPtr + wBytesPerPixel);
                nVals [5] = (BYTE) *(lpNextPtr                 );
                nVals [6] = (BYTE) *lpNextPtr;
                nVals [7] = (BYTE) *(lpNextPtr + wBytesPerPixel);
                nVals [8] = (BYTE) *lpCurrPtr;
            }

            nTmp = (BYTE) nVals [8];
    
            #ifdef QSORT
            qsort (nVals, 9, 2, compare);
            #else
            {
                int i, j;
                int v;

                for (i = 1; i < 9; i++)
                {
                
                    v = nVals [i];
                    j = i;
                
                    while (nVals [j - 1] > v && j > 0)
                    {
                
                        nVals [j] = nVals [j - 1];
                        j--;
                
                    }
                    nVals [j] = v;
                }

            }
            #endif

            nTmp2 = nTmp - nVals [4];        // Difference between pixel and median
            if (nTmp2 < 0)
                nTmp2 *= -1;


            if (nTmp2 > nLevel)
                nTmp = (BYTE) nVals [4];

            *lpDest++ = (BYTE) nTmp;

//          *lpDest++ = (BYTE) nVals [4];
    
            lpCurrPtr++;
            lpPrevPtr++;
            lpNextPtr++;
    
            if (wBytesPerPixel == 1)
                continue;
    
            if (i > 0)
            {
                nVals [0] = (BYTE) *(lpCurrPtr - 3);
                nVals [1] = (BYTE) *(lpCurrPtr + 3);
                nVals [2] = (BYTE) *(lpPrevPtr - 3);
                nVals [3] = (BYTE) *lpPrevPtr;
                nVals [4] = (BYTE) *(lpPrevPtr + 3);
                nVals [5] = (BYTE) *(lpNextPtr - 3);
                nVals [6] = (BYTE) *lpNextPtr;
                nVals [7] = (BYTE) *(lpNextPtr + 3);
                nVals [8] = (BYTE) *lpCurrPtr;
            }
            else
            {
                nVals [0] = (BYTE) *(lpCurrPtr                 );
                nVals [1] = (BYTE) *(lpCurrPtr + 3);
                nVals [2] = (BYTE) *(lpPrevPtr                 );
                nVals [3] = (BYTE) *lpPrevPtr;
                nVals [4] = (BYTE) *(lpPrevPtr + 3);
                nVals [5] = (BYTE) *(lpNextPtr                 );
                nVals [6] = (BYTE) *lpNextPtr;
                nVals [7] = (BYTE) *(lpNextPtr + 3);
                nVals [8] = (BYTE) *lpCurrPtr;
            }
    
            nTmp = (BYTE) nVals [8];

            #ifdef QSORT
            qsort (nVals, 9, 2, compare);
            #else
            {
                int i, j;
                int v;

                for (i = 1; i < 9; i++)
                {
                
                    v = nVals [i];
                    j = i;
                
                    while (nVals [j - 1] > v && j > 0)
                    {
                
                        nVals [j] = nVals [j - 1];
                        j--;
                
                    }
                    nVals [j] = v;
                }

            }
            #endif
    
            if (nTmp2 > nLevel)
                nTmp = (BYTE) nVals [4];

            *lpDest++ = (BYTE) nTmp;

//          *lpDest++ = (BYTE) nVals [4];
    
            lpCurrPtr++;
            lpPrevPtr++;
            lpNextPtr++;
    
            if (i > 0)
            {
                nVals [0] = (BYTE) *(lpCurrPtr - 3);
                nVals [1] = (BYTE) *(lpCurrPtr + 3);
                nVals [2] = (BYTE) *(lpPrevPtr - 3);
                nVals [3] = (BYTE) *lpPrevPtr;
                nVals [4] = (BYTE) *(lpPrevPtr + 3);
                nVals [5] = (BYTE) *(lpNextPtr - 3);
                nVals [6] = (BYTE) *lpNextPtr;
                nVals [7] = (BYTE) *(lpNextPtr + 3);
                nVals [8] = (BYTE) *lpCurrPtr;
            }
            else
            {
                nVals [0] = (BYTE) *(lpCurrPtr                 );
                nVals [1] = (BYTE) *(lpCurrPtr + 3);
                nVals [2] = (BYTE) *(lpPrevPtr                 );
                nVals [3] = (BYTE) *lpPrevPtr;
                nVals [4] = (BYTE) *(lpPrevPtr + 3);
                nVals [5] = (BYTE) *(lpNextPtr                 );
                nVals [6] = (BYTE) *lpNextPtr;
                nVals [7] = (BYTE) *(lpNextPtr + 3);
                nVals [8] = (BYTE) *lpCurrPtr;
            }

            nTmp = (BYTE) nVals [8];
    
            #ifdef QSORT
            qsort (nVals, 9, 2, compare);
            #else
            {
                int i, j;
                int v;

                for (i = 1; i < 9; i++)
                {
                
                    v = nVals [i];
                    j = i;
                
                    while (nVals [j - 1] > v && j > 0)
                    {
                
                        nVals [j] = nVals [j - 1];
                        j--;
                
                    }
                    nVals [j] = v;
                }

            }
            #endif
    
            if (nTmp2 > nLevel)
                nTmp = (BYTE) nVals [4];

            *lpDest++ = (BYTE) nTmp;

//          *lpDest++ = (BYTE) nVals [4];
    
            lpCurrPtr++;
            lpPrevPtr++;
            lpNextPtr++;
    
        }
    }
    return;
}





#ifdef QSORT

int FAR compare (int *elem1, int *elem2)
{

    int p1;
    int p2;

    p1 = (int) *elem1;
    p2 = (int) *elem2;

    if (p1 > p2)
        return (1);

    if (p1 == p2)
        return (0);

    if (p1 < p2)
        return (-1);

}

#endif
