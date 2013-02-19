/****************************************************************************

   PROTOTYPE:

     void FAR PASCAL DitherGrayFS (LPLPSTR, LPLPSTR, LPSTR);


   EXAMPLE CALL:   (Reuses QUANTFSDITHER structure.  
                This is ok since is uses the same fields. 


    lpQuantFSDitherPtr = (QUANTFSDITHER FAR *) lpDest;
    lpQuantFSDitherPtr -> wPaddedBytesPerRow  = wPaddedBytesPerRow;
    lpQuantFSDitherPtr -> wRowsThisStrip      = wRowsThisStrip;
    lpQuantFSDitherPtr -> wScanWidth          = wScanWidth;
    lpQuantFSDitherPtr -> hErrBuf             = hErrBuf;

    DitherGrayFS ((LPLPSTR) &lpDest, (LPLPSTR) &lpSource, lpDest);

*****************************************************************************/

#include <windows.h>
#include "imgprep.h"
#include "proto.h"
#include "global.h"
#include <filters.h>
#include <memory.h>   // For C 6.0 memory copy etc.

int FAR PASCAL DitherGrayFS (lplpDest, lplpSource, lpStruct) 
LPSTR FAR *   lplpDest;
LPSTR FAR *   lplpSource;
LPSTR         lpStruct;
{
    char                  ClipTable[256];
    char  *               pClipTable;
    HANDLE                hErrBuf;
    LPSTR                 lpErrBuf;
    LPSTR                 lpDest;
    LPSTR                 lpSource;
    QUANTFSDITHER FAR *   lpQuantFSDitherPtr;
    WORD                  j;
    WORD                  wPaddedScanWidth;
    WORD                  errbuf_len;
    WORD                  wRowsThisStrip;
    WORD                  wInputBytesPerRow;
  
    /*  Copy parameters to locals                     */
  
    lpQuantFSDitherPtr  = (QUANTFSDITHER FAR *) lpStruct;

    wInputBytesPerRow   = lpQuantFSDitherPtr->wInputBytesPerRow;
    wRowsThisStrip      = lpQuantFSDitherPtr->wRowsThisStrip;    
    wPaddedScanWidth    = lpQuantFSDitherPtr->wPaddedScanWidth;
    hErrBuf             = lpQuantFSDitherPtr->hErrBuf;    
  
    /*  Initialize I/O Buffers                        */
   
    lpDest    = *lplpSource;
    lpSource  = *lplpDest;
  
    /*  Prepare clip table and calculate errbuf_len (for FS dither only now)  */
  
    pClipTable = ClipTable;
  
    for (j = 0; j < 64; j++)
      *pClipTable++ = (char) j;
    
    _fmemset ((LPSTR) pClipTable, 63, 64);
    pClipTable += 64;
    
    _fmemset ((LPSTR) pClipTable, 0, 128);   
  
    errbuf_len = wInputBytesPerRow + 3;      
  
    /*   Do a buffered dither of the image                      */
  
    lpErrBuf = (LPSTR)GlobalLock (hErrBuf); 
  
    dither_fsgray (lpDest, lpErrBuf, wPaddedScanWidth, wRowsThisStrip, errbuf_len, ClipTable);
  
    GlobalUnlock (hErrBuf);
  
    *lplpDest   = lpDest;
    *lplpSource = lpSource;
  
    return (0);
  
  }
  
  
  
  
  
  
  
  
