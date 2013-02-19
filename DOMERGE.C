/*---------------------------------------------------------------------------

    DOMERGE.C - "Low-level" merge tool.  Accepts parameters (should have
                already been validated) from caller and performs merge
                as directed.

    CREATED:    12/90   D. Ison

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
#include <string.h>
#include <imgprep.h>
#include <resource.h>
#include <proto.h>
#include <global.h>
#include <save.h>
#include <error.h>
#include <cpi.h>      
#include <cpifmt.h>
#include <merge.h>
#include <malloc.h>
#include <io.h>
#include <prometer.h>
#include <menu.h>
#include "strtable.h"



extern BOOL bIsMerge;
extern HANDLE hWndMerge;

/*  Need to tweak this stuff...  */

LPSTR       lpABuf;
LPSTR       lpBBuf;
LPSTR       lpOBuf;

LPSTR       lpTmpBuf;


/*  And this  */

WORD  wAInByteCount;
WORD  wBInByteCount;
WORD  wOutByteCount;

WORD  wABytesPerStrip;
WORD  wBBytesPerStrip;
WORD  wOutBytesPerStrip;


MERGESTRUCT MergeStruct;

#ifdef NEWWAY
int nErr;
#define MergeDiscardResource(r)         nErr=(int)r;goto MergeDiscardResource;
#endif

#define MergeDiscardResource(r)      _asm    mov   ax,r     \
                                     _asm    jmp   MergeDiscardResource


int FAR DoMerge ()
{
    FARPROC   lpfnMergeDlg;
    int nRetval;
    HMENU hMenu;

    if (image_active)
    {
        int     nResponse;
        PSTR    pStringBuf;
        PSTR    pStringBuf2;
        BOOL    bCancel = FALSE;

        pStringBuf  = LocalLock (hStringBuf);
        pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

        LoadString (hInstIP, STR_CONFIRM_EXIT, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        LoadString (hInstIP, STR_APPNAME, (LPSTR) pStringBuf2, MAXSTRINGSIZE);

        nResponse = MessageBox (hWndIP, (LPSTR) pStringBuf, (LPSTR) pStringBuf2, MB_YESNOCANCEL | MB_ICONQUESTION);
        switch (nResponse)
        {
            case IDYES:
                SendMessage (hWndIP, WM_COMMAND, IDM_SAVE, NULL);
                break;
            case IDNO:
                break;
            case IDCANCEL:
                bCancel = TRUE;
                break;

        }
        LocalUnlock (hStringBuf);

        if (bCancel)
            return (FALSE);

//      DestroyWindow (hWndDisplay);
        SendMessage (hWndDisplay, WM_CLOSE, 0, 0);    // Close in well-behaved form
        image_active = FALSE;
    }

    hMenu = GetMenu (hWndIP);
    EnableMenuItem (hMenu, IDM_MERGE, MF_BYCOMMAND | MF_GRAYED);
    lpfnMergeDlg = MakeProcInstance ((FARPROC)MergeDlgProc, hInstIP);

    hWndMerge = CreateDialog (hInstIP, "MERGEDLG", hWndIP, lpfnMergeDlg);
    ShowWindow (hWndMerge, SW_SHOWNORMAL);

    bPollMessages = FALSE; // Stop interactive repaints for merge displays
    while (hWndMerge)
    {
        MSG Msg;

        GetMessage (&Msg, NULL, 0, 0);
        if (! IsDialogMessage (hWndMerge, &Msg))
        {
            TranslateMessage (&Msg);
            DispatchMessage (&Msg);
        }
    }
    bPollMessages = TRUE;

    FreeProcInstance (lpfnMergeDlg);
    EnableMenuItem (hMenu, IDM_MERGE, MF_BYCOMMAND | MF_ENABLED);

    if (bIsMerge)
    {
        #ifdef NEVER
        if (image_active)  // This test probably not needed
        {
            DestroyWindow (hWndDisplay);
            image_active = FALSE;
        }
        #endif
        bIsMerge = FALSE;
        nRetval = MergeImages ((LPSTR) MergeStruct.DestPath,
                 (LPSTR) MergeStruct.SourcePathA,
                MergeStruct.nX1,
                MergeStruct.nY1,
                MergeStruct.nExtX1,
                MergeStruct.nExtY1,
                (LPSTR) MergeStruct.SourcePathB,
                MergeStruct.nX2,
                MergeStruct.nY2,
                MergeStruct.nExtX2,
                MergeStruct.nExtY2,
                MergeStruct.nX3,
                MergeStruct.nY3,
                MergeStruct.wMode);
    }


    return (nRetval);
}




int FAR PASCAL MergeImages (lpszDest, lpszImageA, nX1, nY1, nExtX1, nExtY1,
                            lpszImageB, nX2, nY2, nExtX2, nExtY2, 
                            nX3, nY3, wMode)
LPSTR lpszDest;
LPSTR lpszImageA;
int   nX1;
int   nY1;
int   nExtX1;
int   nExtY1;
LPSTR lpszImageB;
int   nX2;
int   nY2;
int   nExtX2;
int   nExtY2;
int   nX3;
int   nY3;
WORD  wMode;
{
    LPSTR           lpBuffer1 = (LPSTR) 0;
    LPSTR           lpBuffer2 = (LPSTR) 0;
    HANDLE          hBuffer1  = (HANDLE) 0;
    HANDLE          hBuffer2  = (HANDLE) 0;
    HANDLE          hABuf     = (HANDLE) 0;
    HANDLE          hBBuf     = (HANDLE) 0;
    HANDLE          hOBuf     = (HANDLE) 0;
    int             hDestFile = 0;
    int             hAFile    = 0;
    int             hBFile    = 0;
    CPIFILESPEC     CPIFileSpecA;
    CPIIMAGESPEC    CPIImageSpecA;
    CPIFILESPEC     CPIFileSpecB;
    CPIIMAGESPEC    CPIImageSpecB;
    CPIFILESPEC     CPIFileSpec;
    CPIIMAGESPEC    CPIImageSpec;
    LPCPIIMAGESPEC  lpCPIImageSpecA;
    LPCPIIMAGESPEC  lpCPIImageSpecB;
    int             i;
    int             nDestWidth;         // Pixel res. of output image.
    int             nDestHeight;        // Height of output image.
    int             nBytesPerSourceA;   // Number bytes coming in from GetSourceALine()
    int             nBytesPerSourceB;   // Number bytes coming in from GetSourceBLine()
    int             nBytesPerDestRow;   // Number bytes per line going to output image
    int             nXMergeOffset;      // X Coordinate relative to A data where merge begins
    int             nXMergeOffsetBytes; // Byte offset for above
    LONG            lNumBytes;
    LONG            lBuffSize;
    BOOL            bAHigher;

    int             nStart;
    int             nStop;
    int             nLineCount = 0;

    lpABuf = (LPSTR) 0;
    lpBBuf = (LPSTR) 0;
    lpOBuf = (LPSTR) 0;


    /*  Go out and open / create files and read header info  */

    hAFile = _lopen (lpszImageA, OF_READ);
    if (hAFile < 0)
        MergeDiscardResource (EC_FILEREAD1);

    hBFile = _lopen (lpszImageB, OF_READ);
    if (hBFile < 0)
        MergeDiscardResource (EC_FILEREAD1);

    hDestFile = _lcreat (lpszDest, 0);
    if (hDestFile < 0)
        MergeDiscardResource (EC_FILEWRITE1);

    /* Read CPI header info    */

    ReadCPIHdr (hAFile, (LPCPIFILESPEC) &CPIFileSpecA, (LPCPIIMAGESPEC) &CPIImageSpecA);
    ReadCPIHdr (hBFile, (LPCPIFILESPEC) &CPIFileSpecB, (LPCPIIMAGESPEC) &CPIImageSpecB);


    _llseek (hAFile, CPIImageSpecA.ImgDataOffset, 0);
    _llseek (hBFile, CPIImageSpecB.ImgDataOffset, 0);

    lpCPIImageSpecA = (LPCPIIMAGESPEC) &CPIImageSpecA;
    lpCPIImageSpecB = (LPCPIIMAGESPEC) &CPIImageSpecB;

    bAHigher = ((nY3 < 0) ? FALSE : TRUE);
    if (! bAHigher)
        nY3 *= -1;

    if (wMode == UNION)
        if (bAHigher)
            nDestHeight  =  (nY3 - nY1   +    // Starting point of merge
                      max ((nExtY1 -          // Greater of 1st image or second
                            nY3 + nY1),       // Merge Point
                            nExtY2));         // Y Ext from A
        else            
            nDestHeight  =  (nY3 - nY1   +    // Starting point of merge
                      max ((nExtY2 -          // Greater of 1st image or second
                            nY3 + nY1),       // Merge Point
                            nExtY1));         // Y Ext from A


    else
        if (bAHigher)
            nDestHeight  =  min (nExtY2, (nExtY1 - nY3));
        else            
            nDestHeight  =  min (nExtY1, (nExtY2 - nY3));


    nBytesPerSourceA = nExtX1 * 3;      // How many bytes will be coming in 
    nBytesPerSourceB = nExtX2 * 3;      // from the A and B handlers

    nDestWidth  =  (nX3 - nX1  +        // Starting point of merge
                    max ((nExtX1 -      // Greater of 1st image or second
                          nX3 + nX1),   // Merge Point
                          nExtX2));     // Y Ext from A


    if (nDestWidth > 20480)
        MergeDiscardResource (EC_MERGE_TOO_WIDE);
        

    nBytesPerDestRow = 3 * nDestWidth;


    /*  Grab some memory  */

    lNumBytes = GetFreeSpace (0);
    lBuffSize = lNumBytes / 5;
    if (lBuffSize > 61440)
        lBuffSize = 61440;


    if (lBuffSize < (LONG) nBytesPerDestRow)
        MergeDiscardResource (EC_NOMEM);

    hBuffer1 = GlobalAlloc (GHND, lBuffSize);
    if (hBuffer1 == NULL)
        MergeDiscardResource (EC_NOMEM);

    lpBuffer1 = GlobalLock (hBuffer1);
    if (lpBuffer1 == NULL)
        MergeDiscardResource (EC_NOMEM);

    hBuffer2 = GlobalAlloc (GHND, lBuffSize);
    if (hBuffer2 == NULL)
        MergeDiscardResource (EC_NOMEM);

    lpBuffer2 = GlobalLock (hBuffer2);
    if (lpBuffer2 == NULL)
        MergeDiscardResource (EC_NOMEM);

    hABuf = GlobalAlloc (GHND, lBuffSize);
    if (hABuf == NULL)
        MergeDiscardResource (EC_NOMEM);

    lpABuf = GlobalLock (hABuf);
    if (lpABuf == NULL)
        MergeDiscardResource (EC_NOMEM);

    hBBuf = GlobalAlloc (GHND, lBuffSize);
    if (hBBuf == NULL)
        MergeDiscardResource (EC_NOMEM);

    lpBBuf = GlobalLock (hBBuf);
    if (lpBBuf == NULL)
        MergeDiscardResource (EC_NOMEM);

    hOBuf = GlobalAlloc (GHND, lBuffSize);
    if (hOBuf == NULL)
        MergeDiscardResource (EC_NOMEM);

    lpOBuf = GlobalLock (hOBuf);
    if (lpOBuf == NULL)
        MergeDiscardResource (EC_NOMEM);


    wABytesPerStrip = (WORD) (lBuffSize / (3 * CPIImageSpecA.X_Length) * (3 * CPIImageSpecA.X_Length));
    wBBytesPerStrip = (WORD) (lBuffSize / (3 * CPIImageSpecB.X_Length) * (3 * CPIImageSpecB.X_Length));

    /*  wOutBytesPerStrip is unknown right now... */

    wAInByteCount = 0xFFFF;
    wBInByteCount = 0xFFFF;
    wOutByteCount = 0x0;


    wOutBytesPerStrip = (WORD) (lBuffSize / (WORD) nBytesPerDestRow) * (WORD) nBytesPerDestRow;


    nDestHeight = nDestHeight;

    nXMergeOffset       =  nX3 - nX1; 
    nXMergeOffsetBytes  = 3 * nXMergeOffset;

    /*  Initialize Output file   */

    InitCPIFile (hDestFile, (LPCPIFILESPEC) &CPIFileSpec, (LPCPIIMAGESPEC) &CPIImageSpec, nDestWidth, nDestHeight);

    /*  Do lseek to data pos in hFileA and in hFileB that was requested  (nth line seek) */

    if (wMode == UNION)
    {
        lNumBytes = ((LONG) nY1 * (LONG) lpCPIImageSpecA -> X_Length * 3L);
        if (lNumBytes > 0L)
            _llseek (hAFile, lNumBytes, 1);

        lNumBytes = ((LONG) nY2 * (LONG) lpCPIImageSpecB -> X_Length * 3L);
        if (lNumBytes > 0L)
            _llseek (hBFile, lNumBytes, 1);
    }
    else 
    {
        if (bAHigher)
            lNumBytes = ((LONG) nY3 * (LONG) lpCPIImageSpecA -> X_Length * 3L);
        else
            lNumBytes = ((LONG) nY1 * (LONG) lpCPIImageSpecA -> X_Length * 3L);
        
        if (lNumBytes > 0L)
            _llseek (hAFile, lNumBytes, 1);

        if (bAHigher)
            lNumBytes = ((LONG) nY2 * (LONG) lpCPIImageSpecB -> X_Length * 3L);
        else
            lNumBytes = ((LONG) nY3 * (LONG) lpCPIImageSpecB -> X_Length * 3L);

        if (lNumBytes > 0L)
            _llseek (hBFile, lNumBytes, 1);
    }

    _fmemset (lpBuffer1, 255, nBytesPerDestRow);
    _fmemset (lpBuffer2, 255, nBytesPerDestRow);


    /*  Progress meter to watch.. */

    #ifndef NOPRO
    {
        PSTR        pStringBuf;
        PSTR        pStringBuf2;
        pStringBuf  = LocalLock (hStringBuf);
        pStringBuf2 = pStringBuf + MAXSTRINGSIZE;
        EnableWindow (hWndIP, FALSE);
        LoadString (hInstIP, STR_MERGE, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        ProOpen (hWndIP, NULL, MakeProcInstance (Abandon, hInstIP), (LPSTR) pStringBuf);

        LoadString (hInstIP, STR_SOURCE_IMAGE_1, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        wsprintf (pStringBuf2, pStringBuf, lpszImageA);
        ProSetText (ID_STATUS1, (LPSTR) pStringBuf2);

        LoadString (hInstIP, STR_SOURCE_IMAGE_2, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        wsprintf (pStringBuf2, pStringBuf, lpszImageB);
        ProSetText (ID_STATUS2, (LPSTR) pStringBuf2);

        LoadString (hInstIP, STR_MERGED_IMAGE, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        wsprintf (pStringBuf2, pStringBuf, lpszDest);
        ProSetText (ID_STATUS3, (LPSTR) pStringBuf2);

        if (wMode == INTERSECT)
            _fstrcpy ((LPSTR) pStringBuf, (LPSTR) "Clip");
        else
            _fstrcpy ((LPSTR) pStringBuf, (LPSTR) "Combine");
        
        wsprintf (pStringBuf2, "%d,%d %s", nX3, (bAHigher ? nY3 : -nY3), (LPSTR) pStringBuf);
        ProSetText (ID_STATUS4, (LPSTR) pStringBuf2);


        ProSetBarRange (nDestHeight >> 2);
        LocalUnlock (hStringBuf);
    }
    #endif

    bAbandon = FALSE;

    nStart = 0;
    nStop  = nDestHeight;

    if (wMode == INTERSECT)
    {
        nStart += nY3;
        nStop  += nY3;
    }

    nLineCount = 0;
    for (i = nStart; i < nStop && ! bAbandon; i++)
    {
        _fmemset (lpBuffer1, 255, nBytesPerDestRow);

        if (bAHigher)
        {
            if (! GetNextSourceALine (hAFile,
                            lpCPIImageSpecA,
                            lpBuffer1,      // Data returns here
                            i,
                            nY1,
                            nX1,
                            nExtX1,
                            nExtY1))         // For clipping off undesired lines
                MergeDiscardResource (EC_FILEREAD1);

            if ((i + nY1) >= nY3 && (i - nY3 + nY1) < nExtY2)
            {
                if (! GetNextSourceBLine (hBFile,
                                    lpCPIImageSpecB, 
                                    lpBuffer2,      // Data returns here
                                    i - nY3 + nY1,        // Scanline into B
                                    nY2,            // Scanlines skipped in B
                                    nX2,            
                                    nExtX2,
                                    nExtY2))
                    MergeDiscardResource (EC_FILEREAD1);


                /*  Copy two lines together  */


                _fmemcpy ((lpBuffer1 + nXMergeOffsetBytes), lpBuffer2, nBytesPerSourceB);
            }
        }
        else
        {
            if (i >= nY3)
                if (! GetNextSourceALine (hAFile,
                                lpCPIImageSpecA, 
                                lpBuffer1,      // Data returns here
                                i - nY3,
                                nY1,
                                nX1,
                                nExtX1,
                                nExtY1))
                    MergeDiscardResource (EC_FILEREAD1);


            if ((i + nY2) <  nExtY2)
            {
                if (! GetNextSourceBLine (hBFile,
                                    lpCPIImageSpecB, 
                                    lpBuffer2,      // Data returns here
                                    i - nY3,        // Scanline into B
                                    nY2,            // Scanlines skipped in B
                                    nX2,
                                    nExtX2,
                                    nExtY2))
                    MergeDiscardResource (EC_FILEREAD1);

                _fmemcpy ((lpBuffer1 + nXMergeOffsetBytes), lpBuffer2, nBytesPerSourceB);

            }
        }
        
        if (wOutByteCount >= wOutBytesPerStrip)
        {
            if (WriteFile (hDestFile, lpOBuf, wOutBytesPerStrip) != (LONG) wOutBytesPerStrip)
                MergeDiscardResource (EC_FILEWRITE1);
            wOutByteCount = 0;
        }

        _fmemcpy (lpOBuf + wOutByteCount, lpBuffer1, nBytesPerDestRow);
        wOutByteCount += nBytesPerDestRow;

        #ifndef NOPRO
        if ((i % 4) == 0)
            ProDeltaPos (1);
        #endif
        nLineCount++;
    }

    /*  Flush Dest Buffer  */

    if (wOutByteCount > 0)
        if (WriteFile (hDestFile, lpOBuf, wOutByteCount) != (LONG) wOutByteCount)
            MergeDiscardResource (EC_FILEWRITE1);
        
    #ifndef NOPRO
    ProDeltaPos (1);
    #endif

    MergeDiscardResource (TRUE);

/*---  Merge Resource Discard Section  ---*/

    {
        int nRetval;

        MergeDiscardResource:

        _asm    mov nRetval,ax

        /*  Unlocks  */


        if (lpBuffer1)
            GlobalUnlock (hBuffer1);

        if (lpBuffer2)
            GlobalUnlock (hBuffer2);

        if (lpABuf)
            GlobalUnlock (hABuf);

        if (lpBBuf)
            GlobalUnlock (hBBuf);

        if (lpOBuf)
            GlobalUnlock (hOBuf);


        /*  Frees */

        if (hBuffer1)
            GlobalFree (hBuffer1);

        if (hBuffer2)
            GlobalFree (hBuffer2);

        if (hABuf)
            GlobalFree (hABuf);

        if (hBBuf)
            GlobalFree (hBBuf);

        if (hOBuf)
            GlobalFree (hOBuf);


        /*  File close(s)  */
    
        if (hAFile)
            _lclose (hAFile);
        if (hBFile)
            _lclose (hBFile);
        if (hDestFile)
            _lclose (hDestFile);

        if (bAbandon)
        {
            OFSTRUCT Of;

            OpenFile (lpszDest, (LPOFSTRUCT) &Of, OF_DELETE);
            nRetval = USER_ABANDON;
        }

        #ifndef NOPRO
        EnableWindow (hWndIP, TRUE);
        ProClose ();  
        #endif

        if (nRetval > 0)
        {
            wFullView = (TmpVals [3] & 1);  // Only look at low bits
            TmpVals [3] = 0;
            SendMessage (hWndIP, WM_SHOWIMAGE, COMMAND_LINE_OPEN, (DWORD) lpszDest);
        }

        return (nRetval);
    }

} 



int GetNextSourceALine (hFile, lpCPIImageSpec, lpDest, nLineNum, nY1, nX1, nExtX1, nExtY1)
int hFile;
LPCPIIMAGESPEC lpCPIImageSpec;
LPSTR          lpDest;
int            nLineNum;
int            nY1;                // Lines already skipped
int            nX1;
int            nExtX1;
int            nExtY1;
{
    int nClippedBytesPerRow = (nExtX1 * 3);
    int nBytesPerRow = (lpCPIImageSpec -> X_Length * 3);

    /*  Next go out and read the data in and do offsetting / clipping  */

    if ((nLineNum + nY1) <= lpCPIImageSpec -> Y_Length &&  nLineNum < nExtY1)
    {
        if (wAInByteCount >= wABytesPerStrip)
        {
            if (! ReadFile  (hFile, lpABuf, (LONG) wABytesPerStrip))   // Test for a NULL read condition, instead of exact bytes
                return (FALSE);
            wAInByteCount = 0;
        }

        /*  Transfer to output buffer the desired portion  */

        _fmemcpy (lpDest, (lpABuf + wAInByteCount + (3 * nX1)), nClippedBytesPerRow);
        wAInByteCount += (WORD) nBytesPerRow;
    }

    return (1);
}

int GetNextSourceBLine (hFile, lpCPIImageSpec, lpDest, nLineNum, nY2, nX2, nExtX2, nExtY2)
int hFile;
LPCPIIMAGESPEC  lpCPIImageSpec;
LPSTR           lpDest;
int             nLineNum;           // Scanline into B
int             nY2;                // Lines already skipped
int             nX2;                
int             nExtX2;
int             nExtY2;
{

    int nClippedBytesPerRow = (nExtX2 * 3);
    int nBytesPerRow = (lpCPIImageSpec -> X_Length * 3);

    /*  Next go out and read the data in and do offsetting / clipping  */

    if ((nLineNum + nY2) <= lpCPIImageSpec -> Y_Length && nLineNum < nExtY2)
    {
        if (wBInByteCount >= wBBytesPerStrip)
        {
            if (! ReadFile  (hFile, lpBBuf, (LONG) wBBytesPerStrip))
                return (FALSE);
            wBInByteCount = 0;
        }

        /*  Transfer to output buffer the desired portion  */

        _fmemcpy (lpDest, (lpBBuf + wBInByteCount + (3 * nX2)), nClippedBytesPerRow);
        wBInByteCount += (WORD) nBytesPerRow;
    }
    else
        _fmemset (lpDest, 255, nBytesPerRow);     // Return a blank line

    return (1);
}




int FAR InitCPIFile (hFile, lpCPIFileSpec, lpCPIImageSpec, nDestWidth, nDestHeight)
int hFile;
LPCPIFILESPEC lpCPIFileSpec;
LPCPIIMAGESPEC lpCPIImageSpec;
int nDestWidth;
int nDestHeight;
{
    int   nNumBytes;

    InitCPIFileSpec (lpCPIFileSpec);
        
    nNumBytes = sizeof (CPIFILESPEC);

    WriteFile (hFile, (LPSTR) lpCPIFileSpec, (LONG) nNumBytes);
            
    /*  Write out Image Spec stuff  */

    InitCPIImageSpec (lpCPIImageSpec);
            
    lpCPIImageSpec -> ImgDescTag       = 1;
    lpCPIImageSpec -> ImgType          = CPIFMT_RGB;
    lpCPIImageSpec -> ImgSpecLength    = sizeof (CPIIMAGESPEC);
    lpCPIImageSpec -> X_Origin         = 0;
    lpCPIImageSpec -> Y_Origin         = 0;   
    lpCPIImageSpec -> X_Length         = nDestWidth;
    lpCPIImageSpec -> Y_Length         = nDestHeight;
    lpCPIImageSpec -> BitsPerPixel     = 24;
    lpCPIImageSpec -> NumberPlanes     = 1;
    lpCPIImageSpec -> NumberColors     = 256;
    lpCPIImageSpec -> Orientation      = 0;    
    lpCPIImageSpec -> Compression      = 1;    // None
    lpCPIImageSpec -> ImgSize          = (DWORD) ((LONG) nDestWidth * 
                                            (LONG) nDestHeight * 3L);
    lpCPIImageSpec -> ImgSequence      = 0;
    lpCPIImageSpec -> NumberPalEntries = 0;
    lpCPIImageSpec -> NumberBitsPerEntry = 24;
    lpCPIImageSpec -> AttrBitsPerEntry = 0;
    lpCPIImageSpec -> FirstEntryIndex  = NULL;
    lpCPIImageSpec -> RGBSequence      = 0;
    lpCPIImageSpec -> NextDescriptor   = NULL;
    lpCPIImageSpec -> ApplSpecReserved = NULL;        
            
    lpCPIImageSpec -> CompLevel        = 0;
    lpCPIImageSpec -> CodeSize         = 0;

    nNumBytes = (int) (tell (hFile) + (LONG) sizeof (CPIIMAGESPEC));
            
    lpCPIImageSpec -> PalDataOffset    = (LONG) nNumBytes;
    lpCPIImageSpec -> ImgDataOffset    = (LONG) nNumBytes;
    nNumBytes = (LONG) sizeof (CPIIMAGESPEC);
    WriteFile  (hFile, (LPSTR) lpCPIImageSpec, (LONG) nNumBytes);

    return (1);
}




