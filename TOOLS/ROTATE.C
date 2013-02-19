/*---------------------------------------------------------------------------

    ROTATE.C - 
                
               

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

int FAR PASCAL RotateCPI (hWndImage, lpDestPath, lpSourcePath, nFlags)
HWND  hWndImage;           // Window handle of active image (Same as hWndDisplay)
LPSTR lpDestPath;          // Path to put output file.
LPSTR lpSourcePath;        // Path of active CPI file.  Points to szOpenFileName with a locked DS.
int   nFlags;
{
    HWND        hWnd  = GetParent (hWndImage);
    OFSTRUCT    Of;
    int         nRetVal = TRUE;

//  lpfnRotateDlg = MakeProcInstance ((FARPROC)RotateDlgProc, hInstance);

    Rotate.Direction = (BYTE) (nFlags ? CCLOCKWISE : CLOCKWISE);

//  if (DialogBoxParam (hInstance, "ROTATEDLG", hWnd, lpfnRotateDlg, (DWORD) lpSourcePath))
    {
        Rotate.pSource = pNameBuf;
        Rotate.pDest   = Rotate.pSource + MAXPATHSIZE;
        Rotate.pTemp   = Rotate.pDest   + MAXPATHSIZE;

        _fstrcpy ((LPSTR) Rotate.pSource, lpSourcePath);
        _fstrcpy ((LPSTR) Rotate.pTemp, lpDestPath);
        _fstrcpy ((LPSTR) Rotate.pDest, lpDestPath);

        if (RotateImage (hWnd, &Rotate))                      // All successful ?
        {
            SendMessage (hWndImage, WM_CLOSE, 0, 0);    // Close in well-behaved form
            PostMessage (hWnd, WM_SHOWIMAGE, TOOLS_OPEN, (DWORD) (LPSTR) Rotate.pDest);// Treat like a command-line open
        }
        else 
        {
            /*  Delete temp.cpi file  */

            if (OpenFile ((LPSTR) Rotate.pTemp, (LPOFSTRUCT)&Of, OF_EXIST) != -1)
                OpenFile ((LPSTR) Rotate.pTemp, (LPOFSTRUCT)&Of, OF_DELETE);

            ErrorMsg (nGlobalErr, STR_ROTATE);
            nRetVal = FALSE;
        }
    }
//  FreeProcInstance (lpfnRotateDlg);

    return (nRetVal);
}


#define RotateDiscardResource(r)    {  nGlobalErr = r; nRetval = 0;   \
                                      goto DiscardResource;}



int NEAR RotateImage (hWnd, pRotate)
HWND    hWnd;
PROTATE pRotate;
{
    HANDLE      hSourceBuf   = (HANDLE) 0;
    HANDLE      hDestBuf     = (HANDLE) 0;
    LPSTR       lpSourceBuf  = (LPSTR)  0;
    LPSTR       lpDestBuf    = (LPSTR)  0;
    HIMAGE      hSourceImage = (HIMAGE) 0;
    HIMAGE      hDestImage   = (HIMAGE) 0;
    LPSTR       lpSourcePtr;
    LPSTR       lpDestPtr;
    IMAGEINFO   ImageInfo;
    WORD        wBytesPerRow;
    WORD        wBytesPerPixel;
    WORD        wScanWidth;
    WORD        wScanHeight;
    WORD        wRowsPerStrip;
    WORD        wRowsPerOutputStrip;
    WORD        wBytesPerOutputRow;
    WORD        wBytesPerOutputStrip;
    WORD        wCurrentOffset;
    WORD        wBytesPerStrip;
    LPSTR       lpInPtr, lpOutPtr;
    LPSTR       lpTmpInPtr, lpTmpOutPtr;
    WORD        j, k, x;
    DWORD       dwSize;
    WORD        wLinesRead;
    WORD        wRowsThisStrip;
    BYTE        Direction;
    WORD        wLinesLeft;
    WORD        wRowsThisOutputStrip;

    nRetval     = TRUE;          // Assume successful return
    nGlobalErr  = USER_ABANDON;  // Default error code

    hSourceImage = OpenImage   ((LPSTR) pRotate -> pSource, CMAP_TO_8); 
    if (! hSourceImage)
        RotateDiscardResource (ERR_OPENIMAGE);

    GetImageInfo (hSourceImage, (LPIMAGEINFO) &ImageInfo);

    hDestImage   = CreateImage ((LPSTR) pRotate -> pTemp, hSourceImage, ImageInfo.wScanHeight, ImageInfo.wScanWidth, NULL); 
    if (! hDestImage)
        RotateDiscardResource (ERR_CREATEIMAGE);
    

    /*  Now grab some rotation memory  (maybe ?) */

    /*  Set up input buffer  */

    dwSize = GetFreeSpace (0);
    if (dwSize > 65535L)
        dwSize = 65535L;

    if (dwSize < (DWORD) wBytesPerRow)  // Can we get at least 1 scanline ?
        return (FALSE);

    wBytesPerRow    = ImageInfo.wBytesPerRow;
    wBytesPerPixel  = ImageInfo.wBytesPerPixel;
    wScanWidth      = ImageInfo.wScanWidth;
    wScanHeight     = ImageInfo.wScanHeight;
    wRowsPerStrip   = (WORD) (dwSize / (DWORD) wBytesPerRow);
    wBytesPerStrip  = wBytesPerRow * wRowsPerStrip;

    hSourceBuf = GlobalAlloc (GHND, dwSize);
    if (! hSourceBuf)
        return (FALSE);
    lpSourceBuf = GlobalWire (hSourceBuf);

    /*  Set up output buffer  */

    dwSize = GetFreeSpace (0);

    if (dwSize > 65535L)
        dwSize = 65535L;

    if (dwSize < (DWORD) wBytesPerRow)  // Can we get at least 1 scanline ?
        return (FALSE);

    wBytesPerOutputRow    = wScanHeight * wBytesPerPixel;
    wRowsPerOutputStrip   = (WORD) (dwSize / (DWORD) wBytesPerOutputRow);
    wBytesPerOutputStrip  = wBytesPerOutputRow * wRowsPerOutputStrip;
    wRowsThisOutputStrip  = wRowsPerOutputStrip; // Default

    hDestBuf = GlobalAlloc (GHND, dwSize);
    if (! hDestBuf)
        return (FALSE);
    lpDestBuf = GlobalWire (hDestBuf);

    /*  Setup Progress meter to watch.. */

    {
        char Buffer [128];
        PSTR pStringBuf;
        PSTR pStringBuf2;
        int  nString;

        pStringBuf  = LocalLock (hStringBuf);
        pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

        EnableWindow (hWnd, FALSE);
        LoadString (hInstance, STR_ROTATE, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        ProOpen (hWnd, NULL, MakeProcInstance (Abandon, hInstance), (LPSTR) pStringBuf);
        ProSetBarRange (wBytesPerRow / (wRowsPerOutputStrip * wBytesPerPixel));

        LoadString (hInstance, STR_ROTATING_IMAGE, (LPSTR) pStringBuf, MAXSTRINGSIZE);

        if (pRotate -> Direction == CCLOCKWISE)
            nString = STR_CCLOCKWISE;
        else
            nString = STR_CLOCKWISE;

        LoadString (hInstance, nString, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
        wsprintf ((LPSTR) Buffer, (LPSTR) pStringBuf, (LPSTR) pStringBuf2);

        ProSetText (ID_STATUS1, (LPSTR) Buffer);

        #ifdef OLDWAY
        GetNameFromPath ((LPSTR) pStringBuf2, pRotate -> pSource);
        LoadString (hInstance, STR_SOURCE_PATH, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        _fstrcat ((LPSTR) pStringBuf, (LPSTR) pStringBuf2);
        ProSetText (ID_STATUS2, (LPSTR) pStringBuf);

        GetNameFromPath ((LPSTR) pStringBuf2, pRotate -> pDest);
        LoadString (hInstance, STR_DEST_PATH, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        _fstrcat ((LPSTR) pStringBuf, (LPSTR) pStringBuf2);
        ProSetText (ID_STATUS3, (LPSTR) pStringBuf);
        #endif

        LocalUnlock (hStringBuf);
    }

    wCurrentOffset  = 0;      // From right
    wLinesRead      = 0;
    bAbandon        = FALSE;
    wLinesLeft      = ImageInfo.wScanWidth; // Width is height for output image
    Direction       = pRotate -> Direction;

    do
    {
        lpOutPtr   = lpDestBuf;
        do   
        {
            if (Direction == CCLOCKWISE)
            {
                lpInPtr   = (lpSourceBuf + wBytesPerRow - wCurrentOffset);   // Point to rightmost byte of current box 
                lpInPtr--;
            }
            else
                lpInPtr = lpSourceBuf + wCurrentOffset;
    
            lpSourcePtr = lpSourceBuf;
            for (x = 0; x < wRowsPerStrip; x++)
            {
                ReadLine (hSourceImage, lpSourcePtr);
                lpSourcePtr += wBytesPerRow;
            }

            if ((wScanHeight - wLinesRead) > wRowsPerStrip)
                wRowsThisStrip = wRowsPerStrip;
            else
                wRowsThisStrip  = (wScanHeight - wLinesRead);

            /*  Transform the "box" into output buffer  */ 
    
            for (j = 0; j < wRowsThisStrip; j++)
            {
                if (Direction == CCLOCKWISE)
                    lpTmpOutPtr = lpOutPtr;
                else  
                {
                    lpTmpOutPtr = lpOutPtr + wBytesPerOutputRow;
                    lpTmpOutPtr -= wBytesPerPixel;
                }

                lpTmpInPtr  = lpInPtr;

                for (k = 0; k < wRowsPerOutputStrip; k++)
                {
                    if (Direction == CCLOCKWISE)
                        if (wBytesPerPixel == 1)
                            *lpTmpOutPtr = *lpTmpInPtr--;
                        else
                        {
                            *(lpTmpOutPtr + 2)  = *lpTmpInPtr--;
                            *(lpTmpOutPtr + 1)  = *lpTmpInPtr--;
                            *(lpTmpOutPtr    )  = *lpTmpInPtr--;
                        }
                    else
                        if (wBytesPerPixel == 1)
                            *lpTmpOutPtr = *lpTmpInPtr++;
                        else
                        {
                            *(lpTmpOutPtr    )  = *lpTmpInPtr++;
                            *(lpTmpOutPtr + 1)  = *lpTmpInPtr++;
                            *(lpTmpOutPtr + 2)  = *lpTmpInPtr++;
                        }

                    lpTmpOutPtr += wBytesPerOutputRow;
                }
                lpInPtr   += wBytesPerRow;

                if (Direction == CCLOCKWISE)
                    lpOutPtr  += wBytesPerPixel;
                else  
                    lpOutPtr  -= wBytesPerPixel;
            }
    
            wLinesRead += wRowsThisStrip; 

        } while (wLinesRead < wScanHeight);

        /*  At this point, we have read and processed for an entire output strip, so we
            write the strip, reset wLinesRead, pointers, etc. 
        */

        lpDestPtr = lpDestBuf;

        if (wLinesLeft < wRowsPerOutputStrip)
            wRowsThisOutputStrip = wLinesLeft;

        for (x = 0; x < wRowsThisOutputStrip; x++)
        {
            WriteLine (hDestImage, lpDestPtr);
            lpDestPtr += wBytesPerOutputRow;
        }

        wLinesLeft -= wRowsPerOutputStrip;
        wLinesRead = 0;
        wCurrentOffset += wRowsPerOutputStrip * wBytesPerPixel;

        RewindImage (hSourceImage);

        ProDeltaPos (1);

    }  while (wCurrentOffset < wBytesPerRow && ! bAbandon);

    ProDeltaPos (1);

    if (bAbandon)
        RotateDiscardResource (USER_ABANDON);

/*---  Rotate Resource Discard Section  ---*/

    {
        DiscardResource:

        /*  Pointers  */
    
        if (lpSourceBuf)
            GlobalUnWire (hSourceBuf);

        if (lpDestBuf)
            GlobalUnWire (hDestBuf);

        /*  Handles  */

        if (hDestBuf)
            GlobalFree (hDestBuf);
        if (hSourceBuf)
            GlobalFree (hSourceBuf);

        if (hSourceImage)
            CloseImage (hSourceImage, FALSE);
        if (hDestImage)
           CloseImage (hDestImage, TRUE);

        EnableWindow (hWnd, TRUE);
        ProClose ();   

        return (nRetval);
    }
}



