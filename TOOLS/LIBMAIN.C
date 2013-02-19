#include <windows.h>
#include "internal.h"

/*  Here's where we will keep the public data definitions for now  */

HANDLE      hInstance;               
HANDLE      hStringBuf;
HANDLE      hStructBuf;
HANDLE      hNameBuf;
PSTR        pNameBuf;
char        *FileSpec = "*.cpi";
BYTE        ModiFlags;
BYTE        bSharpFilter;
BYTE        bSmoothFilter;
BYTE        bDirection;  // (For rotation)
BYTE        bScaleProcess;
BYTE        bScaleUnits;
BYTE        bMaintainAspect = TRUE;
char        SourcePath [128];
char        DestPath   [128];
PSTR        pFullSourceName;        // Places to put the actual filenames to be processed.
PSTR        pFullDestName;
PSTR        pFullTempName;
int         nRetval;        // Public return value
BOOL        bAbandon;
int         nGlobalErr;     // Public error code value
BOOL        b1stSharpen     = TRUE; 
BOOL        b1stSmooth      = TRUE;
BOOL        b1stEnhanceEdge = TRUE;
BOOL        b1stRotate      = TRUE;
BOOL        b1stScale       = TRUE;
BOOL        b1stFlip        = TRUE;
BOOL        b1stMirror      = TRUE;
char        CurrWDir [128]; // Actual system current working dir / drive
SHARP       Sharp;
SMOOTH      Smooth;
EDGE        Edge;
ROTATE      Rotate;
SCALE       Scale;
FLIP        Flip;
MIRROR      Mirror;

int FAR PASCAL LibMain (hModule, wDataSegment, wHeapSize, lpszCmdLine)
HANDLE  hModule;
WORD    wDataSegment;
WORD    wHeapSize;
LPSTR   lpszCmdLine;
{
    if (wHeapSize)
    {
        hInstance = hModule;
        UnlockData(0);
    }

    hNameBuf = LocalAlloc (LHND, (3 * MAXPATHSIZE));
    if (! hNameBuf)
        return (1);

    pNameBuf = LocalLock (hNameBuf);

    hStringBuf = LocalAlloc (LHND, (MAXSTRINGSIZE << 1));
    if (! hStringBuf)
        return (1);

    hStructBuf = LocalAlloc (LHND, MAXSTRUCTSIZE);
    if (! hStructBuf)
        return (1);

	return (1);

}

int FAR PASCAL WEP (bSystemExit)
BOOL  bSystemExit;
{
    return (1);
}

void FAR PASCAL Abandon ()
{
    bAbandon = TRUE;
}


int FAR PASCAL FlushTools ()
{
    LocalFree (hStructBuf);
    LocalFree (hStringBuf);
    LocalUnlock (hNameBuf);
    LocalFree (hNameBuf);
    return (TRUE);
}

