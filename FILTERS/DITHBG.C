/*----------------------------------------------------------------------------

   PROTOTYPE:

     void FAR PASCAL DitherGrayBayer (LPLPSTR, LPLPSTR, LPSTR);



   EXAMPLE CALL:   

     (Reuses DITHERBAYER structure.  This is ok since is uses the same fields. The only
     difference is that wPaddedBytesPerRow field must be set to 
     wPaddedScanWidth, not wPaddedBytesPerRow, as in the Bayer color
     dither filter)

    lpDitherBayerPtr = (DITHERBAYER FAR *) lpDest;
    lpDitherBayerPtr->wPaddedBytesPerRow  = lpInFileInfo->wPaddedScanWidth;
    lpDitherBayerPtr->wRowsThisStrip      = wRowsThisStrip;
    lpDitherBayerPtr->wScrnY              = wScrnY;
    DitherGrayBayer ((LPLPSTR) &lpDest, (LPLPSTR) &lpSource, lpDest);


    MODS    Corrected "wPaddedScanWidth" mess.  D. Ison  7-11-90

----------------------------------------------------------------------------*/

#include <windows.h>
#include "imgprep.h"
#include "proto.h"
#include "global.h"
#include <filters.h>


int FAR PASCAL DitherGrayBayer (lplpDest, lplpSource, lpStruct) 
LPSTR FAR *lplpDest;
LPSTR FAR *lplpSource;
LPSTR   lpStruct;
{
    BYTE    InVal;
    BYTE    TestVal;
    LPSTR   lpSource;
    LPSTR   lpDest;
    LPSTR   lpSourcePtr;
    LPSTR   lpDestPtr;
    WORD    wInputBytesPerRow;
    WORD    wRowsThisStrip;
    WORD    wScrnY;
    WORD    i, j;
    DITHERBAYER FAR *lpDitherBayerPtr;
  
  
    /*  Copy parameters to locals                       */
  
    lpDitherBayerPtr    = (DITHERBAYER FAR *) lpStruct;
    wInputBytesPerRow  = lpDitherBayerPtr->wInputBytesPerRow;
    wRowsThisStrip      = lpDitherBayerPtr->wRowsThisStrip;    
    wScrnY              = lpDitherBayerPtr->wScrnY;
  
    /*  Initialize I/O Buffers                          */
   
    lpSource    = *lplpSource;
    lpDest      = *lplpDest;
  
    lpSourcePtr = lpSource;
    lpDestPtr   = lpDest;
  
  
    for (i = wScrnY; i < (wScrnY + wRowsThisStrip); i++)
        for (j = 0; j < wInputBytesPerRow; j++)
        {
            InVal     = *lpSourcePtr;
            InVal   >>= 2;
            TestVal   = (BYTE) Pattern2[j & 7][i & 7];
  
            if (InVal > TestVal)
                *lpDestPtr = 1;
            else
                *lpDestPtr = 0;
    
            lpSourcePtr++;
            lpDestPtr++;
        }
  
      return (0);
}
  
  
  
  
  
  
  
  
