/*---------------------------------------------------------------------------

    MIRROR.C    
                
               

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


int FAR PASCAL MirrorCPI (hWndImage, lpDestPath, lpSourcePath, nFlags)
HWND  hWndImage;           // Window handle of active image (Same as hWndDisplay)
LPSTR lpDestPath;          // Path to put output file.
LPSTR lpSourcePath;        // Path of active CPI file.  Points to szOpenFileName with a locked DS.
int   nFlags;
{
    LPSTR       lpPtr = NULL;
    HWND        hWnd  = GetParent (hWndImage);
    OFSTRUCT    Of;
    int         nRetVal = TRUE;


    Flip.pSource = pNameBuf;
    Flip.pDest   = Flip.pSource + MAXPATHSIZE;
    Flip.pTemp   = Flip.pDest   + MAXPATHSIZE;

    _fstrcpy ((LPSTR) Flip.pSource, lpSourcePath);
    _fstrcpy ((LPSTR) Flip.pTemp, (LPSTR) lpDestPath);
    _fstrcpy ((LPSTR) Flip.pDest, (LPSTR) lpDestPath);

    if (MirrorImage (hWnd, &Flip))                      // All successful ?
    {
        SendMessage (hWndImage, WM_CLOSE, 0, 0);    // Close in well-behaved form
        PostMessage (hWnd, WM_SHOWIMAGE, TOOLS_OPEN, (DWORD) (LPSTR) Flip.pDest);// Treat like a command-line open
    }
    else 
    {
        /*  Delete temp file  */

        if (OpenFile ((LPSTR) Flip.pTemp, (LPOFSTRUCT)&Of, OF_EXIST) != -1)
            OpenFile ((LPSTR) Flip.pTemp, (LPOFSTRUCT)&Of, OF_DELETE);
        ErrorMsg (nGlobalErr, STR_MIRROR);
        nRetVal = FALSE;
    }

    return (nRetVal);
}


#define MirrorDiscardResource(r)    {  nGlobalErr = r; nRetval = 0;   \
                                      goto DiscardResource;}

int NEAR MirrorImage (hWnd, pFlip)
HWND         hWnd;
PFLIP   pFlip  ;
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

    hSourceFile = OpenImage   ((LPSTR) pFlip -> pSource, CMAP_TO_8); 
    if (! hSourceFile)
        MirrorDiscardResource (ERR_OPENIMAGE);

    hDestFile   = CreateImage ((LPSTR) pFlip -> pTemp, hSourceFile, NULL, NULL, CI_MIRROR); 
    if (! hDestFile)
        MirrorDiscardResource (ERR_CREATEIMAGE);

    GetImageInfo (hDestFile, (LPIMAGEINFO) &ImageInfo);

    wBytesPerRow    = ImageInfo.wBytesPerRow;
    wBytesPerPixel  = ImageInfo.wBytesPerPixel;
    wScanWidth      = ImageInfo.wScanWidth;
    wScanHeight     = ImageInfo.wScanHeight;

    /*  Let's do it !      */

    hDest = GlobalAlloc (GHND, (DWORD) wBytesPerRow);
    if (! hDest)
        MirrorDiscardResource (ERR_NOMEM);

    lpDest = GlobalLock (hDest);
    if (lpDest == NULL) 
        MirrorDiscardResource (ERR_NOMEM);

    hPrev = GlobalAlloc (GHND, (DWORD) (wBytesPerRow + ImageInfo.wBytesPerPixel));
    if (! hPrev)
        MirrorDiscardResource (ERR_NOMEM);

    lpPrev = GlobalLock (hPrev);
    if (lpPrev == NULL) 
        MirrorDiscardResource (ERR_NOMEM);

    hCurr = GlobalAlloc (GHND, (DWORD) (wBytesPerRow + ImageInfo.wBytesPerPixel));
    if (! hCurr)
        MirrorDiscardResource (ERR_NOMEM);

    lpCurr = GlobalLock (hCurr);
    if (lpCurr == NULL) 
        MirrorDiscardResource (ERR_NOMEM);

    hNext = GlobalAlloc (GHND, (DWORD) (wBytesPerRow + ImageInfo.wBytesPerPixel));
    if (! hNext)
        MirrorDiscardResource (ERR_NOMEM);

    lpNext = GlobalLock (hNext);
    if (lpNext == NULL) 
        MirrorDiscardResource (ERR_NOMEM);


    /*  Setup Progress meter to watch.. */

    {
        PSTR pStringBuf;

        pStringBuf  = LocalLock (hStringBuf);
        EnableWindow (hWnd, FALSE);
        LoadString (hInstance, STR_MIRROR, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        ProOpen (hWnd, NULL, MakeProcInstance (Abandon, hInstance), (LPSTR) pStringBuf);
        ProSetBarRange (wScanHeight >> 4);
        LoadString (hInstance, STR_MIRRORING_IMAGE, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        ProSetText (ID_STATUS1, (LPSTR) pStringBuf);

        #ifdef OLDWAY
        LoadString (hInstance, STR_SOURCE_PATH, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        _fstrcat ((LPSTR) pStringBuf, (LPSTR) pFlip -> pSource);
        ProSetText (ID_STATUS2, (LPSTR) pStringBuf);

        LoadString (hInstance, STR_DEST_PATH, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        _fstrcat ((LPSTR) pStringBuf, (LPSTR) pFlip -> pDest);
        ProSetText (ID_STATUS3, (LPSTR) pStringBuf);
        #endif

        LocalUnlock (hStringBuf);
    }


    /*  Read and flip image  */

    bAbandon = FALSE;

    for (i = 0; i < wScanHeight && ! bAbandon; i++)  // Skipped first line....
    {
        if (! ReadLine (hSourceFile, lpNext))
            MirrorDiscardResource (ERR_FILEREAD1);

        if (! WriteLine (hDestFile, lpNext))
            MirrorDiscardResource (ERR_FILEWRITE1);
        
        if ((i % 16) == 0)
            ProDeltaPos (1);

    }

    ProDeltaPos (1);

    if (bAbandon)
        MirrorDiscardResource (USER_ABANDON);

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

