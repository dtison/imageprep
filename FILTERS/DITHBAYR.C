/*----------------------------------------------------------------------------

   PROTOTYPE:

     void FAR PASCAL DitherBayer_A (LPLPSTR, LPLPSTR, LPSTR);   // Replacement for old one...


----------------------------------------------------------------------------*/

#include <windows.h>
#include "imgprep.h"
#include "proto.h"
#include "global.h"
#include <filters.h>

#define MATRIXSIZE 16
#define MAXCOLOR    255

                                  
int FAR PASCAL DitherBayer_A (lplpDest, lplpSource, lpStruct) 
LPSTR FAR *lplpDest;
LPSTR FAR *lplpSource;
LPSTR   lpStruct;
{
    LPSTR   lpSource;
    LPSTR   lpDest;
    LPSTR   lpSourcePtr;
    LPSTR   lpDestPtr;
    register WORD    wTemp;
    WORD    wInputBytesPerRow;
    WORD    wOutputBytesPerRow;
    WORD    wRowsThisStrip;
    WORD    wScrnY;
    WORD    i, j;
    WORD    wColors;
    DITHERBAYER FAR *lpDitherBayerPtr;
    WORD    wScanWidth;
    WORD    wDitherVal;
    WORD    wRed;
    WORD    wGreen;
    WORD    wBlue;

    int     nIndex;

    /*  Copy parameters to locals                       */
  
    lpDitherBayerPtr    = (DITHERBAYER FAR *) lpStruct;
    wInputBytesPerRow   = lpDitherBayerPtr->wInputBytesPerRow;
    wScanWidth          = lpDitherBayerPtr->wInputBytesPerRow / 3;

    wOutputBytesPerRow  = lpDitherBayerPtr->wPaddedScanWidth;
    wRowsThisStrip      = lpDitherBayerPtr->wRowsThisStrip;    
    wScrnY              = lpDitherBayerPtr->wScrnY;
    wColors             = lpDitherBayerPtr->wColors;

    /*  Initialize I/O Buffers                          */
   
    lpSource    = *lplpSource;
    lpDest      = *lplpDest;

    if (wColors == 256)
        for (i = wScrnY; i < (wScrnY + wRowsThisStrip); i++)
        {
            lpSourcePtr = lpSource;
            lpDestPtr   = lpDest;
            for (j = 0; j < wScanWidth; j++)
            {
                  /*  Get dither matrix threshold */
    
                wDitherVal = (BYTE) Pattern [j % MATRIXSIZE][i % MATRIXSIZE];
    
                wTemp = wBlue = (BYTE) (*lpSourcePtr++);
    
                wBlue <<= 2;
                wBlue -= wTemp;
                wBlue += wDitherVal;
                wBlue >>= 8;
    
                wTemp = wGreen = (BYTE) (*lpSourcePtr++);
                wGreen <<= 3;
                wGreen -= wTemp;
                wGreen += wDitherVal;
                wGreen >>= 8;
    
                wGreen <<= 2;
    
                wTemp = wRed  = (BYTE) (*lpSourcePtr++);
                wRed <<= 3;
                wRed  -= wTemp;
                wRed  += wDitherVal;
                wRed >>= 8;
    
                wRed <<= 5;
    
                /* Convert to palette index */
                /* wTemp = nGreenLevel * nBlueLevel */
    
                nIndex = wRed + wGreen + wBlue;
    
                *lpDestPtr++ = (BYTE)nIndex;
            }
            lpSource += wInputBytesPerRow;
            lpDest   += wOutputBytesPerRow;
        }
    else
        for (i = wScrnY; i < (wScrnY + wRowsThisStrip); i++)
        {
            lpSourcePtr = lpSource;
            lpDestPtr   = lpDest;
            for (j = 0; j < wScanWidth; j++)
            {
                  /*  Get dither matrix threshold */
    
                wDitherVal = (BYTE) Pattern [j % MATRIXSIZE][i % MATRIXSIZE];
    
                wTemp = wBlue = (BYTE) (*lpSourcePtr++);
    
                wBlue <<= 1;
                wBlue -= wTemp;
                wBlue += wDitherVal;
                wBlue >>= 8;
    
                wTemp = wGreen = (BYTE) (*lpSourcePtr++);
                wGreen <<= 1;
                wGreen -= wTemp;
                wGreen += wDitherVal;
                wGreen >>= 8;
    
                wGreen <<= 1;
    
                wTemp = wRed  = (BYTE) (*lpSourcePtr++);
                wRed <<= 1;
                wRed  -= wTemp;
                wRed  += wDitherVal;
                wRed >>= 8;
    
                wRed <<= 2;
    
                /* Convert to palette index */
                /* wTemp = nGreenLevel * nBlueLevel */
    
                nIndex = wRed + wGreen + wBlue;
    
                *lpDestPtr++ = (BYTE)nIndex;
            }
            lpSource += wInputBytesPerRow;
            lpDest   += wOutputBytesPerRow;
        }

    return (0);
}
  
  
  
  
  
  
  
  
