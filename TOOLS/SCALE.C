/*---------------------------------------------------------------------------
    SCALE.C - 
                
               

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

 
void NEAR ScaleLineTruncate (PSCALE, LPSTR, LPSTR, LPSTR, WORD, double, double, WORD);
void NEAR ScaleLineAverage  (PSCALE, LPSTR, LPSTR, LPSTR, WORD, double, double, WORD);
void NEAR ScaleLineInterpolate (PSCALE, LPSTR, LPSTR, LPSTR, WORD, double, double, WORD);

int FAR PASCAL ScaleCPI (hWndImage, lpDestPath, lpSourcePath, nFlags)
HWND hWndImage;
LPSTR lpDestPath;         // Path to put output file.
LPSTR lpSourcePath;       // Path of active CPI file.  Points to szOpenFileName with a locked DS.
int   nFlags;
{
    FARPROC     lpfnScaleDlg;
    HWND        hWnd  = GetParent (hWndImage);
    OFSTRUCT    Of;
    int         nRetVal = TRUE;

    lpfnScaleDlg = MakeProcInstance ((FARPROC)ScaleDlgProc, hInstance);
    nGlobalErr   = USER_ABANDON;

    Scale.pSource = pNameBuf;
    Scale.pDest   = Scale.pSource + MAXPATHSIZE;
    Scale.pTemp   = Scale.pDest   + MAXPATHSIZE;

    _fstrcpy ((LPSTR) Scale.pSource, lpSourcePath);
    _fstrcpy ((LPSTR) Scale.pTemp, lpDestPath);
    _fstrcpy ((LPSTR) Scale.pDest, lpDestPath);

    if (DialogBoxParam (hInstance, "SCALEDLG", hWnd, lpfnScaleDlg, (DWORD) lpSourcePath))
    {
        if (ScaleImage (hWnd, &Scale))                      // All successful ?
        {
            SendMessage (hWndImage, WM_CLOSE, 0, 0);    // Close in well-behaved form
            PostMessage (hWnd, WM_SHOWIMAGE, TOOLS_OPEN, (DWORD) (LPSTR) Scale.pDest);// Treat like a command-line open
        }
        else 
            nRetVal = FALSE;
    }
    else
        nRetVal = FALSE;

    FreeProcInstance (lpfnScaleDlg);

    if (! nRetVal)
    {
        /*  Delete temp file  */

        if (OpenFile ((LPSTR) Scale.pTemp, (LPOFSTRUCT)&Of, OF_EXIST) != -1)
            OpenFile ((LPSTR) Scale.pTemp, (LPOFSTRUCT)&Of, OF_DELETE);

        ErrorMsg (nGlobalErr, STR_SCALE);
    }

    return (nRetVal);
}


#define ScaleDiscardResource(r)    {  nGlobalErr = r; nRetval = 0;   \
                                      goto DiscardResource;}

int NEAR ScaleImage (hWnd, pScale)
HWND         hWnd;
PSCALE pScale;
{
    HANDLE      hDest       = (HANDLE) 0;
    HANDLE      hCurr       = (HANDLE) 0;
    HANDLE      hNext       = (HANDLE) 0;
    LPSTR       lpDest      = (LPSTR)  0;
    LPSTR       lpCurr      = (LPSTR)  0;
    LPSTR       lpNext      = (LPSTR)  0;
    HIMAGE      hSourceFile = (HIMAGE) 0;
    HIMAGE      hDestFile   = (HIMAGE) 0;
    IMAGEINFO   ImageInfo;
    WORD        i;
    WORD        j;

    WORD        wBytesPerRow;
    WORD        wBytesPerPixel;
    WORD        wScanWidth;
    WORD        wScanHeight;
    WORD        wCurrLine = 1;
    WORD        wBytesPerDestRow;
    double      ScaleH;
    double      ScaleV;
    double      SPixelRowAddr, RowDelta;
 
    WORD        SPixelRowNum;
    WORD        DestWidth, DestHeight;
    WORD        wDivisor;


    if (pScale -> wXPixels == 0 || pScale -> wYPixels == 0)
        ScaleDiscardResource (USER_ABANDON);
    
    
    #ifdef OLDWAY
    if (pScale -> wXPercent > 0)
    {
        ScaleH = (double) pScale -> wXPercent / (double) 100;
        ScaleV = (double) pScale -> wXPercent / (double) 100;  // For now only
    }
    #endif

    nRetval     = TRUE;          // Assume successful return
    nGlobalErr  = USER_ABANDON;  // Default error code

    {
        WORD wCMAP;

        wCMAP = pScale -> ScaleProcess == TRUNCATION ? CMAP_TO_8 : CMAP_TO_24;

        hSourceFile = OpenImage   ((LPSTR) pScale -> pSource, wCMAP);
    }
    if (! hSourceFile)
        ScaleDiscardResource (ERR_OPENIMAGE);

    GetImageInfo (hSourceFile, (LPIMAGEINFO) &ImageInfo);

    wBytesPerRow    = ImageInfo.wBytesPerRow;
    wBytesPerPixel  = ImageInfo.wBytesPerPixel;
    wScanWidth      = ImageInfo.wScanWidth;
    wScanHeight     = ImageInfo.wScanHeight;

    /*  Setup scaling ratios  */

    ScaleH = (double) pScale -> wXPixels / (double) wScanWidth;
    ScaleV = (double) pScale -> wYPixels / (double) wScanHeight;

    DestWidth  = pScale -> wXPixels;
    DestHeight = pScale -> wYPixels;

    wBytesPerDestRow = (DestWidth * wBytesPerPixel);

    hDestFile   = CreateImage ((LPSTR) pScale -> pTemp, hSourceFile, DestWidth, DestHeight, NULL); 
    if (! hDestFile)
        ScaleDiscardResource (ERR_CREATEIMAGE);


    /*  Let's do it !      */

    hDest = GlobalAlloc (GHND, (DWORD) wBytesPerDestRow);
    if (! hDest)
        ScaleDiscardResource (ERR_NOMEM);

    lpDest = GlobalLock (hDest);
    if (lpDest == NULL) 
        ScaleDiscardResource (ERR_NOMEM);

    hCurr = GlobalAlloc (GHND, (DWORD) (wBytesPerRow + ImageInfo.wBytesPerPixel));
    if (! hCurr)
        ScaleDiscardResource (ERR_NOMEM);

    lpCurr = GlobalLock (hCurr);
    if (lpCurr == NULL) 
        ScaleDiscardResource (ERR_NOMEM);

    hNext = GlobalAlloc (GHND, (DWORD) (wBytesPerRow + ImageInfo.wBytesPerPixel));
    if (! hNext)
        ScaleDiscardResource (ERR_NOMEM);

    lpNext = GlobalLock (hNext);
    if (lpNext == NULL) 
        ScaleDiscardResource (ERR_NOMEM);


    /*  Setup Progress meter to watch.. */

    {
        char Buffer [128];
        PSTR pStringBuf;
        PSTR pStringBuf2;
        int  nString;

        pStringBuf  = LocalLock (hStringBuf);
        pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

        if (GetWinFlags () & WF_80x87)
            wDivisor = 16;
        else
            wDivisor = 1;

        EnableWindow (hWnd, FALSE);
        LoadString (hInstance, STR_SCALE, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        ProOpen (hWnd, NULL, MakeProcInstance (Abandon, hInstance), (LPSTR) pStringBuf);
        ProSetBarRange (DestHeight / wDivisor);

        LoadString (hInstance, STR_SCALING_IMAGE, (LPSTR) pStringBuf, MAXSTRINGSIZE);

        if (pScale -> ScaleProcess == TRUNCATION)
            nString = STR_TRUNCATION;
        else
            if (pScale -> ScaleProcess == AVERAGING)
                nString = STR_AVERAGING;
            else
                nString = STR_AVGINTERPOLATE;

        LoadString (hInstance, nString, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
        wsprintf ((LPSTR) Buffer, (LPSTR) pStringBuf, (LPSTR) pStringBuf2);

        ProSetText (ID_STATUS1, (LPSTR) Buffer);

        #ifdef OLDWAY
        GetNameFromPath ((LPSTR) pStringBuf2, pScale -> pSource);
        LoadString (hInstance, STR_SOURCE_PATH, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        _fstrcat ((LPSTR) pStringBuf, (LPSTR) pStringBuf2);
        ProSetText (ID_STATUS2, (LPSTR) pStringBuf);

        GetNameFromPath ((LPSTR) pStringBuf2, pScale -> pDest);
        LoadString (hInstance, STR_DEST_PATH, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        _fstrcat ((LPSTR) pStringBuf, (LPSTR) pStringBuf2);
        ProSetText (ID_STATUS3, (LPSTR) pStringBuf);
        #endif

        LocalUnlock (hStringBuf);
    }


    /*  Read in first line   */

    bAbandon = FALSE;

    if (! ReadLine  (hSourceFile, lpNext))
        ScaleDiscardResource (ERR_FILEREAD1);

    _fmemcpy ((LPSTR) lpCurr, (LPSTR) lpNext, wBytesPerRow);

    for (i = 1; i < DestHeight && ! bAbandon; i++)  // Skipped first line....
    {

        SPixelRowAddr = i / ScaleV;
        SPixelRowNum  = (WORD) SPixelRowAddr;
        RowDelta      = SPixelRowAddr - (double) SPixelRowNum;

        /*  Advance the scanline number as necessary  */

        #ifdef NEWWAY
        if (wCurrLine <= SPixelRowNum)
        {
            _fmemcpy (lpCurr, lpNext, wBytesPerRow);

            for (j = wCurrLine; j <= SPixelRowNum; j++)
            {
                if (! ReadLine (hSourceFile, lpNext))
                    ScaleDiscardResource (ERR_FILEREAD1);

                wCurrLine++;
            }
        }
        #else
        for (j = wCurrLine; j <= SPixelRowNum; j++)
        {
            _fmemcpy (lpCurr, lpNext, wBytesPerRow);
            if (! ReadLine (hSourceFile, lpNext))
                ScaleDiscardResource (ERR_FILEREAD1);

            wCurrLine++;
        }
        #endif

        switch (pScale -> ScaleProcess)
        {
            case TRUNCATION:
                ScaleLineTruncate (pScale, lpDest, lpCurr, lpNext, DestWidth, RowDelta, ScaleH, wBytesPerPixel);
                break;

            case AVERAGING:
                ScaleLineAverage (pScale, lpDest, lpCurr, lpNext, DestWidth, RowDelta, ScaleH, wBytesPerPixel);
                break;

            case AVGINTERPOLATE:
                ScaleLineInterpolate (pScale, lpDest, lpCurr, lpNext, DestWidth, RowDelta, ScaleH, wBytesPerPixel);
                break;
        }

        if (! WriteLine (hDestFile, lpDest))
            ScaleDiscardResource (ERR_FILEWRITE1);

        if ((i % wDivisor) == 0)
            ProDeltaPos (1);

    }

    /*  Take care of writing last line  */

//  ScaleLine (lpDest, lpPrev, lpCurr, lpNext, wScanWidth, wBytesPerPixel, wBytesPerRow, pScale);
    if (! WriteLine (hDestFile, lpDest))
        ScaleDiscardResource (ERR_FILEWRITE1);

    ProDeltaPos (1);

    if (bAbandon)
        ScaleDiscardResource (USER_ABANDON);

/*---  Export Resource Discard Section  ---*/

    {
        DiscardResource:

        /*  Pointers  */
    
        if (lpDest)
            GlobalUnlock (hDest);

        if (lpCurr)
            GlobalUnlock (hCurr);

        if (lpNext)
            GlobalUnlock (hNext);

        /*  Handles  */

        if (hDest)
            GlobalFree (hDest);
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


void NEAR ScaleLineTruncate (pScale, lpDest, lpCurr, lpNext, DestWidth, RowDelta, ScaleH, wBytesPerPixel)
PSCALE pScale;
LPSTR lpDest;
LPSTR lpCurr;
LPSTR lpNext;
WORD  DestWidth;
double RowDelta;
double ScaleH;
WORD    wBytesPerPixel;
{

    WORD k;
    double       SPixelColAddr;
    WORD        SPixelColNum;
    WORD        PtA;

    for (k = 0; k < DestWidth; k++)
    {
        SPixelColAddr = k / ScaleH;
        SPixelColNum  = (WORD) SPixelColAddr;

        SPixelColNum *= wBytesPerPixel;

        PtA = (BYTE) *(lpCurr + SPixelColNum);

        *(lpDest + (k * wBytesPerPixel)) = (BYTE) (PtA);
            
        SPixelColNum++;

        if (wBytesPerPixel == 1)
            continue;

        PtA = (BYTE) *(lpCurr + SPixelColNum);

        *(lpDest + (k * 3) + 1) = (BYTE) (PtA);

        SPixelColNum++;

        PtA = (BYTE) *(lpCurr + SPixelColNum);

        *(lpDest + (k * 3) + 2) = (BYTE) (PtA);
            
    }

}


void NEAR ScaleLineAverage (pScale, lpDest, lpCurr, lpNext, DestWidth, RowDelta, ScaleH, wBytesPerPixel)
PSCALE pScale;
LPSTR lpDest;
LPSTR lpCurr;
LPSTR lpNext;
WORD  DestWidth;
double RowDelta;
double ScaleH;
WORD    wBytesPerPixel;
{

    WORD k;
    double       SPixelColAddr;
    WORD        SPixelColNum;
    WORD        PtA, PtB, PtC, PtD;

    for (k = 0; k < DestWidth; k++)
    {
        SPixelColAddr = k / ScaleH;
        SPixelColNum  = (WORD) SPixelColAddr;

        SPixelColNum *= wBytesPerPixel;

        PtA = (BYTE) *(lpCurr + SPixelColNum);
        PtB = (BYTE) *(lpCurr + SPixelColNum + wBytesPerPixel);

        PtC = (BYTE) *(lpNext + SPixelColNum);
        PtD = (BYTE) *(lpNext + SPixelColNum + wBytesPerPixel);

        *(lpDest + (k * wBytesPerPixel)) = (BYTE) ((PtA + PtB + PtC + PtD) >> 2);
            

        SPixelColNum++;

        if (wBytesPerPixel == 1)
            continue;

        PtA = (BYTE) *(lpCurr + SPixelColNum);
        PtB = (BYTE) *(lpCurr + SPixelColNum + 3);

        PtC = (BYTE) *(lpNext + SPixelColNum);
        PtD = (BYTE) *(lpNext + SPixelColNum + 3);

        *(lpDest + (k * 3)+1) = (BYTE) ((PtA + PtB + PtC + PtD) >> 2);

        SPixelColNum++;

        PtA = (BYTE) *(lpCurr + SPixelColNum);
        PtB = (BYTE) *(lpCurr + SPixelColNum + 3);

        PtC = (BYTE) *(lpNext + SPixelColNum);
        PtD = (BYTE) *(lpNext + SPixelColNum + 3);

        *(lpDest + (k * 3)+2) = (BYTE) ((PtA + PtB + PtC + PtD) >> 2);
            
    }

}

void NEAR ScaleLineInterpolate (pScale, lpDest, lpCurr, lpNext, DestWidth, RowDelta, ScaleH, wBytesPerPixel)
PSCALE pScale;
LPSTR lpDest;
LPSTR lpCurr;
LPSTR lpNext;
WORD  DestWidth;
double RowDelta;
double ScaleH;
WORD    wBytesPerPixel;
{

//  #define FLOAT

    WORD k;
    double      SPixelColAddr;
    double      ColDelta;
    #ifdef FLOAT
    double      ContribFromAandB, ContribFromCandD;
    #endif
    WORD        SPixelColNum;
    WORD        PtA, PtB, PtC, PtD, PixelValue;
    WORD        wIndex;

    LONG        lColDelta;
    LONG        lRowDelta;
    LONG        lContribFromAandB;
    LONG        lContribFromCandD;

    #define     CONSTANT   16384
    #define     BITS         14

    lRowDelta = (DWORD) (RowDelta * CONSTANT);

    wIndex = 0;
    for (k = 0; k < DestWidth; k++)
    {

        SPixelColAddr = k / ScaleH;
        SPixelColNum  = (WORD) SPixelColAddr;
        ColDelta      = SPixelColAddr - (double) SPixelColNum;

        lColDelta    = (DWORD) ((SPixelColAddr - (double) SPixelColNum) * CONSTANT);

        SPixelColNum *= wBytesPerPixel;

        PtA = (BYTE) *(lpCurr + SPixelColNum);
        PtB = (BYTE) *(lpCurr + SPixelColNum + wBytesPerPixel);

        PtC = (BYTE) *(lpNext + SPixelColNum);
        PtD = (BYTE) *(lpNext + SPixelColNum + wBytesPerPixel);


        #ifdef FLOAT
        {
            ContribFromAandB = ColDelta * ((double) PtB - PtA) + PtA;
            ContribFromCandD = ColDelta * ((double) PtD - PtC) + PtC;
            PixelValue = (WORD) (0.5 + ContribFromAandB + 
                         (ContribFromCandD - ContribFromAandB) * RowDelta);
            *(lpDest + wIndex++) = (BYTE) PixelValue;
        }
        #else
        {
            LONG lTmp;

            lTmp = ((LONG) PtB - (LONG) PtA);
            lContribFromAandB = ((lColDelta * lTmp) >> BITS) + PtA;
            lTmp = ((LONG) PtD - (LONG) PtC);
            lContribFromCandD = ((lColDelta * lTmp) >> BITS) + PtC;
            PixelValue = (WORD) (       lContribFromAandB + 
                       (((lContribFromCandD - lContribFromAandB) * lRowDelta) >> BITS));
            *(lpDest + wIndex++) = (BYTE) PixelValue;
        }
        #endif
            

        SPixelColNum++;

        if (wBytesPerPixel == 1)
            continue;

        PtA = (BYTE) *(lpCurr + SPixelColNum);
        PtB = (BYTE) *(lpCurr + SPixelColNum + 3);

        PtC = (BYTE) *(lpNext + SPixelColNum);
        PtD = (BYTE) *(lpNext + SPixelColNum + 3);

        #ifdef FLOAT
        {
            ContribFromAandB = ColDelta * ((double) PtB - PtA) + PtA;
            ContribFromCandD = ColDelta * ((double) PtD - PtC) + PtC;
            PixelValue = (WORD) (0.5 + ContribFromAandB + 
                (ContribFromCandD - ContribFromAandB) * RowDelta);
            *(lpDest + wIndex++) = (BYTE) PixelValue;
        }
        #else
        {

            LONG lTmp;

            lTmp = ((LONG) PtB - (LONG) PtA);
            lContribFromAandB = ((lColDelta * lTmp) >> BITS) + PtA;
            lTmp = ((LONG) PtD - (LONG) PtC);
            lContribFromCandD = ((lColDelta * lTmp) >> BITS) + PtC;
            PixelValue = (WORD) (       lContribFromAandB + 
                                (((lContribFromCandD - lContribFromAandB) * lRowDelta) >> BITS));
            *(lpDest + wIndex++) = (BYTE) PixelValue;
        }
        #endif

        SPixelColNum++;

        PtA = (BYTE) *(lpCurr + SPixelColNum);
        PtB = (BYTE) *(lpCurr + SPixelColNum + 3);

        PtC = (BYTE) *(lpNext + SPixelColNum);
        PtD = (BYTE) *(lpNext + SPixelColNum + 3);

        #ifdef FLOAT
        {
            ContribFromAandB = ColDelta * ((double) PtB - PtA) + PtA;
            ContribFromCandD = ColDelta * ((double) PtD - PtC) + PtC;
            PixelValue = (WORD) (0.5 + ContribFromAandB + 
                (ContribFromCandD - ContribFromAandB) * RowDelta);
            *(lpDest + wIndex++) = (BYTE) PixelValue;
        }
        #else
        {
            LONG lTmp;

            lTmp = ((LONG) PtB - (LONG) PtA);
            lContribFromAandB = ((lColDelta * lTmp) >> BITS) + PtA;
            lTmp = ((LONG) PtD - (LONG) PtC);
            lContribFromCandD = ((lColDelta * lTmp) >> BITS) + PtC;
            PixelValue = (WORD) (       lContribFromAandB + 
                         (((lContribFromCandD - lContribFromAandB) * lRowDelta) >> BITS));
            *(lpDest + wIndex++) = (BYTE) PixelValue;
        }
        #endif
            
    }
}
