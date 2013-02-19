/*---------------------------------------------------------------------------

    FLIP.C    
                
               

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


int FAR PASCAL FlipCPI (hWndImage, lpDestPath, lpSourcePath, nFlags)
HWND  hWndImage;           // Window handle of active image (Same as hWndDisplay)
LPSTR lpDestPath;          // Path to put output file.
LPSTR lpSourcePath;        // Path of active CPI file.  Points to szOpenFileName with a locked DS.
int   nFlags;
{
    OFSTRUCT    Of;
    HWND        hWnd  = GetParent (hWndImage);
    char        Buffer [128];
    int         nRetVal = TRUE;


    /*  No more dialog; use temp files instead  */

    Flip.pSource = pNameBuf;
    Flip.pDest   = Flip.pSource + MAXPATHSIZE;
    Flip.pTemp   = Flip.pDest   + MAXPATHSIZE;

    _fstrcpy ((LPSTR) Flip.pSource, lpSourcePath);
    _fstrcpy ((LPSTR) Flip.pTemp, (LPSTR) lpDestPath);
    _fstrcpy ((LPSTR) Flip.pDest, (LPSTR) lpDestPath);

    if (FlipImage (hWnd, &Flip))                      // All successful ?
    {
        SendMessage (hWndImage, WM_CLOSE, 0, 0);
        PostMessage (hWnd, WM_SHOWIMAGE, TOOLS_OPEN, (DWORD) (LPSTR) Flip.pDest);// Treat like a command-line open
    }
    else 
    {
        /*  Delete temp.cpi file  */

        if (OpenFile ((LPSTR) Flip.pTemp, (LPOFSTRUCT)&Of, OF_EXIST) != -1)
            OpenFile ((LPSTR) Flip.pTemp, (LPOFSTRUCT)&Of, OF_DELETE);

        ErrorMsg (nGlobalErr, STR_FLIP);
        nRetVal = FALSE;
    }

    return (nRetVal);
}


#define FlipDiscardResource(r)    {  nGlobalErr = r; nRetval = 0;   \
                                      goto DiscardResource;}

int NEAR FlipImage (hWnd, pFlip)
HWND         hWnd;
PFLIP pFlip;
{
    HANDLE      hDest       = (HANDLE) 0;
    HANDLE      hCurr       = (HANDLE) 0;
    LPSTR       lpDest      = (LPSTR)  0;
    LPSTR       lpCurr      = (LPSTR)  0;
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

    hSourceFile = OpenImage   ((LPSTR) pFlip -> pSource, CMAP_TO_8); 
    if (! hSourceFile)
        FlipDiscardResource (ERR_OPENIMAGE);

    hDestFile   = CreateImage ((LPSTR) pFlip -> pTemp, hSourceFile, NULL, NULL, CI_FLIP); 
    if (! hDestFile)
        FlipDiscardResource (ERR_CREATEIMAGE);

    GetImageInfo (hDestFile, (LPIMAGEINFO) &ImageInfo);

    wBytesPerRow    = ImageInfo.wBytesPerRow;
    wBytesPerPixel  = ImageInfo.wBytesPerPixel;
    wScanWidth      = ImageInfo.wScanWidth;
    wScanHeight     = ImageInfo.wScanHeight;

    /*  Let's do it !      */

    hDest = GlobalAlloc (GHND, (DWORD) wBytesPerRow);
    if (! hDest)
        FlipDiscardResource (ERR_NOMEM);

    lpDest = GlobalLock (hDest);
    if (lpDest == NULL) 
        FlipDiscardResource (ERR_NOMEM);

    hCurr = GlobalAlloc (GHND, (DWORD) (wBytesPerRow + ImageInfo.wBytesPerPixel));
    if (! hCurr)
        FlipDiscardResource (ERR_NOMEM);

    lpCurr = GlobalLock (hCurr);
    if (lpCurr == NULL) 
        FlipDiscardResource (ERR_NOMEM);

    /*  Setup Progress meter to watch.. */

    {
        char Buffer [128];
        PSTR pStringBuf;
        PSTR pStringBuf2;
        int  nString;

        pStringBuf  = LocalLock (hStringBuf);
        pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

        EnableWindow (hWnd, FALSE);
        LoadString (hInstance, STR_FLIP, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        ProOpen (hWnd, NULL, MakeProcInstance (Abandon, hInstance), (LPSTR) pStringBuf);
        ProSetBarRange (wScanHeight >> 4);
        LoadString (hInstance, STR_FLIPPING_IMAGE, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        ProSetText (ID_STATUS1, (LPSTR) pStringBuf);
        #ifdef OLDWAY
        GetNameFromPath ((LPSTR) pStringBuf2, pFlip -> pSource);
        LoadString (hInstance, STR_SOURCE_PATH, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        _fstrcat ((LPSTR) pStringBuf, (LPSTR) pStringBuf2);
        ProSetText (ID_STATUS2, (LPSTR) pStringBuf);
        GetNameFromPath ((LPSTR) pStringBuf2, pFlip -> pDest);
        LoadString (hInstance, STR_DEST_PATH, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        _fstrcat ((LPSTR) pStringBuf, (LPSTR) pStringBuf2);
        ProSetText (ID_STATUS3, (LPSTR) pStringBuf);
        #endif

        LocalUnlock (hStringBuf);
    }


    /*  Read and flip image  */

    bAbandon = FALSE;

    for (i = 0; i < wScanHeight && ! bAbandon; i++)  // Skipped first line....
    {
        if (! ReadLine (hSourceFile, lpCurr))
            FlipDiscardResource (ERR_FILEREAD1);

        if (! WriteLine (hDestFile, lpCurr))
            FlipDiscardResource (ERR_FILEWRITE1);
        
        if ((i % 16) == 0)
            ProDeltaPos (1);

    }

    ProDeltaPos (1);

    if (bAbandon)
        FlipDiscardResource (USER_ABANDON);

/*---  Export Resource Discard Section  ---*/

    {
        DiscardResource:

        /*  Pointers  */
    
        if (lpDest)
            GlobalUnlock (hDest);

        if (lpCurr)
            GlobalUnlock (hCurr);

        /*  Handles  */

        if (hDest)
            GlobalFree (hDest);
        if (hCurr)
            GlobalFree (hCurr);

        if (hSourceFile)
            CloseImage (hSourceFile, FALSE);
        if (hDestFile)
           CloseImage (hDestFile, TRUE);

        EnableWindow (hWnd, TRUE);
        ProClose ();   

        return (nRetval);
    }
}


