#include <windows.h>
#include <imgprep.h>
#include <proto.h>
#include <reduce.h>
#include <error.h>
#include <global.h>
#include <filters.h>
#include <memory.h>   // For C 6.0 memory copy etc.
#include <cpi.h>

void FAR PASCAL QuantizeOPR (LPSTR, LPSTR, WORD, WORD, WORD, WORD);


/*----------------------------------------------------------------------------

   PROCEDURE:        QuantColorReduced
   DESCRIPTION:       
   DEFINITION:       5.3.4
   START:            David Ison   
   MODS:             1/10/90  Tom Bagford Jr. (to spec)

   typedef struct
   {
      WORD wPaddedBytesPerRow;  //  Bytes per row (WORD aligned)
      WORD wRowsThisStrip;      //  Number scanlines this strip

   }   UNIFORMQUANT;

------------------------------------------------------------------------------*/

int FAR PASCAL QuantColorReduced (plpDest, plpSource, lpF) 
LPSTR FAR *   plpDest;
LPSTR FAR *   plpSource;
LPSTR         lpF;
{
    LPSTR               lpSmallhist;
    LPSTR               lpDest;
    LPSTR               lpSource;
    QUANTOPR FAR        *lpFS;
    WORD                wInputBytesPerRow;
    WORD                wRowsThisStrip;
    WORD                wPaddedScanWidth;

    /*  Lock and set data pointers   */

    if (! (lpSmallhist = (LPSTR) GlobalLock (hBhist)))
        return (EC_NOMEM);


    lpFS      = (QUANTOPR FAR *) lpF;


    lpDest    = *plpDest;
    lpSource  = *plpSource;

    /*  Set defaults     */

    wInputBytesPerRow   = lpFS->wInputBytesPerRow;
    wPaddedScanWidth    = lpFS->wPaddedScanWidth;
    wRowsThisStrip      = lpFS->wRowsThisStrip;

    RGBToBGR (lpDest, lpSource, wRowsThisStrip, (wInputBytesPerRow / 3), wInputBytesPerRow);


    /*  Quantize colors to predetermined indexes     */

    QuantizeOPR   (lpDest,              
                   lpSmallhist, 
                   wPaddedScanWidth,
                   wRowsThisStrip,
                   wInputBytesPerRow,
                   3);


    /*  Setup return pointers   */

    GlobalUnlock (hBhist);
    return (0);
}

/*---------------------------------------------------------------------------

   PROCEDURE:         QuantFSDither
   DESCRIPTION:       
   DEFINITION:        5.3.4
   START:             David Ison   
   MODS:              1/10/90  Tom Bagford Jr. ( to spec )
                      7/90     D. Ison

----------------------------------------------------------------------------*/

int FAR PASCAL QuantFSDither (lplpDest, lplpSource, lpStruct) 
LPSTR FAR * lplpDest;
LPSTR FAR * lplpSource;
LPSTR       lpStruct;
{
    char                  ClipTable [256];
    char  *               pClipTable;
    HANDLE                hErrBuf;
    LPSTR                 lpDest;
    LPSTR                 lpErrBuf;
    LPSTR                 lpSmallhist;
    LPSTR                 lpSource;
    QUANTFSDITHER FAR *   lpQuantFSDitherPtr;
    WORD                  errbuf_len;
    WORD                  j;
    WORD                  wRowsThisStrip;
    WORD                  wPaddedScanWidth;

    WORD                  wInputBytesPerRow;
    RGBQUAD  FAR *        lpRGBQuadPtr;
    HANDLE                hPaletteMem;
    


    /*  Copy parameters to locals               */
    
    lpQuantFSDitherPtr  = (QUANTFSDITHER FAR *) lpStruct;
    wInputBytesPerRow   = lpQuantFSDitherPtr->wInputBytesPerRow;
    wRowsThisStrip      = lpQuantFSDitherPtr->wRowsThisStrip;    
    wPaddedScanWidth    = lpQuantFSDitherPtr->wPaddedScanWidth;
    hErrBuf             = lpQuantFSDitherPtr->hErrBuf;    
    

    /*  Initialize I/O Buffers                  */
    

    
    lpSource  = *lplpDest;
    lpDest    = *lplpSource;

    /*  Prepare clip table and calculate errbuf_len (for FS dither only now)  */



    pClipTable = ClipTable;
    
    for (j = 0; j < 64; j++)
        *pClipTable++ = (char) j;
    _fmemset ((LPSTR) pClipTable, 63, 64);
    pClipTable += 64;
    _fmemset ((LPSTR) pClipTable, 0, 128);   

    /*  errbuf_len is the size of ONE LINE of the error buffer.  The allocation 
        on the other hand is for the (* current *) and (* forward *) buffers 
        for the entire strip.  (There is a note in FILTERS.C concerning this.  D. Ison  */

    errbuf_len = (wPaddedScanWidth + 2) * 3; // "1" = FS, 2 = BK.  Others will be greater

    /*  Do a buffered dither of the image                   */
    
    if (!(lpErrBuf = GlobalLock (hErrBuf))) 
      return( EC_NOMEM );
    
    if (!(lpSmallhist = GlobalLock (hBhist)))
      return (EC_NOMEM);

    hPaletteMem = (bIsSaving ? hExpPalette : hImpPalette);  // KLUDGE!   D. Ison to fix VGA save bug   10/90
    if (! (lpRGBQuadPtr = (RGBQUAD FAR *) GlobalLock (hPaletteMem)))
        return (EC_NOMEM);

    switch (wCurrDither)
    {
    
        case DITHER_FS_COLOR:

            dither_fscolor (lpDest, lpErrBuf, wPaddedScanWidth, wRowsThisStrip, errbuf_len, ClipTable, lpSmallhist, lpRGBQuadPtr, wInputBytesPerRow);
            break;

        case DITHER_BURKE_COLOR:

            dither_bkcolor (lpDest, lpErrBuf, wPaddedScanWidth, wRowsThisStrip, errbuf_len, ClipTable, lpSmallhist, lpRGBQuadPtr, wInputBytesPerRow);
            break;


        #ifdef NOTNOW
        case DITHER_JARVIS_COLOR:

            dither_jvcolor (lpDest, lpErrBuf, wPaddedScanWidth, wRowsThisStrip, errbuf_len, ClipTable, lpSmallhist, lpRGBQuadPtr, wInputBytesPerRow);
            break;
        #endif

        case DITHER_FAST_COLOR:

            dither_facolor (lpDest, lpErrBuf, wPaddedScanWidth, wRowsThisStrip, errbuf_len, ClipTable, lpSmallhist, lpRGBQuadPtr, wInputBytesPerRow);
            break;


        default:

            dither_fscolor (lpDest, lpErrBuf, wPaddedScanWidth, wRowsThisStrip, errbuf_len, ClipTable, lpSmallhist, lpRGBQuadPtr, wInputBytesPerRow);
            break;

    }


    *lplpDest   = lpDest;
    *lplpSource = lpSource;


    GlobalUnlock (hImpPalette);
    GlobalUnlock (hBhist);
    GlobalUnlock (hErrBuf);
    return (0);
}

//#define CDITHER

#ifdef CDITHER
void FAR dither_fscolor (lpData, lpErrBuf, wPaddedScanWidth, wRowsThisStrip, wErrBufLen, ClipTable, lpSmallhist, lpRGBQuadPal, wInputBytesPerRow)
LPSTR lpData;
LPSTR lpErrBuf;
WORD  wPaddedScanWidth;
WORD  wRowsThisStrip;
WORD  wErrBufLen;
BYTE  *ClipTable;
LPSTR lpSmallhist;
RGBQUAD FAR *lpRGBQuadPal;
WORD  wInputBytesPerRow;
{

    //  (* Next *) error filters.  We will have as many of these as (*next*) places 
    //   appear in current data line in filters.  Place (*nexts*) at end of erro
    //   buf in each color plane.

    //  We only handle error from one line at a time, even though our dither is buffered...

    LPSTR   lpRedErrNext;
    LPSTR   lpGrnErrNext;
    LPSTR   lpBluErrNext;

    LPSTR   lpRedErrCurr;
    LPSTR   lpGrnErrCurr;
    LPSTR   lpBluErrCurr;

    LPSTR   lpRedErrFwd;
    LPSTR   lpGrnErrFwd;
    LPSTR   lpBluErrFwd;


    LPSTR   lpCurrErrBuf;
    LPSTR   lpFwdErrBuf;
    LPSTR   lpSourcePtr;
    LPSTR   lpDestPtr;

    int     Red, Grn, Blu;
    int     Color;
    int     RedErr, GrnErr, BluErr;

    WORD    i, j;

    WORD    wPlaneLen;        // Length of a dist buffer for a single color plane including (*next*) space
    WORD    wErrLineLen;      // Length of a dist buffer for all color planes
    WORD    wErrLen;

    int     RedErrNext;
    int     GrnErrNext;
    int     BluErrNext;

    int     nErr;
    int     nTotalErr;

    wPlaneLen   = wPaddedScanWidth + 1;  // 1 because only 1 (*next*)
    wErrLineLen = (3 * wPlaneLen);
    wErrLen     = (1 * wErrLineLen);  // FS only has 1 line below, thus (*1*)

    lpSourcePtr = lpData;
    lpDestPtr   = lpData;

    lpCurrErrBuf = lpErrBuf;
    lpFwdErrBuf  = lpErrBuf + wErrLen;

    lpRedErrCurr = lpCurrErrBuf;
    lpGrnErrCurr = lpRedErrCurr + wPlaneLen;
    lpBluErrCurr = lpGrnErrCurr + wPlaneLen;

    lpRedErrFwd  = lpFwdErrBuf;
    lpGrnErrFwd  = lpRedErrFwd + wPlaneLen;
    lpBluErrFwd  = lpGrnErrFwd + wPlaneLen;


    /*  Curr2 and Fwd2 will be defined here  */


    lpRedErrNext = lpRedErrFwd + wPaddedScanWidth;
    lpGrnErrNext = lpGrnErrFwd + wPaddedScanWidth;
    lpBluErrNext = lpBluErrFwd + wPaddedScanWidth;

    

    /*  Next2, 3, etc., will be defined here */




    for (i = 0; i < wRowsThisStrip; i++)
    {

        /*  Transfer forward error lines to current error lines & blank fwd err line */

        _fmemcpy (lpRedErrCurr, lpRedErrFwd, wErrLen);
        _fmemset (lpRedErrFwd, 0, wErrLen);


        for (j = 0; j < wPaddedScanWidth; j++)
        {

            Blu = ((BYTE) *lpSourcePtr++ >> 3);
            Grn = ((BYTE) *lpSourcePtr++ >> 3);
            Red = ((BYTE) *lpSourcePtr++ >> 3);
    
    
            /*  Add errors from dist buffer to these values  */
    
            Blu += lpBluErrCurr [j];
            Grn += lpGrnErrCurr [j];
            Red += lpRedErrCurr [j];
    
    
            /*  Handle (*next*) positions */
    
            Blu += *lpBluErrNext;
            Grn += *lpGrnErrNext;
            Red += *lpRedErrNext;
    
    
            Blu = ClipTable [(BYTE) Blu];
            Grn = ClipTable [(BYTE) Grn];
            Red = ClipTable [(BYTE) Red];

    
            *lpRedErrNext = 0;
            *lpGrnErrNext = 0;
            *lpBluErrNext = 0;


            /*  Find closest color  */
    
    
            Color = (BYTE) lpSmallhist [((Red >> 1) << 10) + ((Grn >> 1) << 5) + (Blu >> 1)];
    
            *lpDestPtr++ = (BYTE) Color;
    
            RedErr = Red - (BYTE) (lpRGBQuadPal [(BYTE) Color].rgbRed   >> 3);
            GrnErr = Grn - (BYTE) (lpRGBQuadPal [(BYTE) Color].rgbGreen >> 3);
            BluErr = Blu - (BYTE) (lpRGBQuadPal [(BYTE) Color].rgbBlue  >> 3);
    
    
    
            /*  Distribute error to FORWARD list  */
    
    
    
            /*  Red  */
    
            nErr = (int) ((WORD) RedErr * 5) >> 4;

            lpRedErrFwd [j] += nErr; 
            nTotalErr        = nErr;

            if (j < (wPaddedScanWidth - 1))
            {
                nErr = (RedErr * 7) >> 4;
                *lpRedErrNext = nErr; 
                nTotalErr += nErr;
    
                nErr = (RedErr >> 4);
                lpRedErrFwd [j + 1] += nErr; 
                nTotalErr += nErr;

                if (j > 0)
                {
    
                    nErr = (RedErr * 3) >> 4;
                    lpRedErrFwd [j - 1] += nErr; 
                    nTotalErr += nErr;
    

                }

            }

                    RedErr = RedErr - nTotalErr;
                    lpRedErrFwd [j + 1] += RedErr;

    
    
            /*  Green  */
    
            nErr = (int) ((WORD) GrnErr * 5) >> 4;
            lpGrnErrFwd [j] += nErr; 
            nTotalErr        = nErr;

            if (j < (wPaddedScanWidth - 1))
            {
    
                nErr = (GrnErr * 7) >> 4;
                *lpGrnErrNext = nErr; 
                nTotalErr += nErr;
    
                nErr = (GrnErr >> 4);
                lpGrnErrFwd [j + 1] += nErr; 
                nTotalErr += nErr;

                if (j > 0)
                {
                    nErr = (GrnErr * 3) >> 4;
                    lpGrnErrFwd [j - 1] += nErr; 
                    nTotalErr += nErr;

                }

            }
    
                    GrnErr = GrnErr - nTotalErr;
                    lpGrnErrFwd [j + 1] += GrnErr;


            /*  Blue  */
    
            nErr = (int) ((WORD) BluErr * 5) >> 4;

            lpBluErrFwd [j] += nErr; 
            nTotalErr        = nErr;

            if (j < (wPaddedScanWidth - 1))
            {

    
                nErr = (BluErr * 7) >> 4;
                *lpBluErrNext = nErr; 
                nTotalErr += nErr;
    
                nErr = (BluErr) >> 4;
                lpBluErrFwd [j + 1] += nErr; 
                nTotalErr += nErr;
    
                if (j > 0)
                {
                    nErr = (BluErr * 3) >> 4;
                    lpBluErrFwd [j - 1] += nErr; 
                    nTotalErr += nErr;

                }

            }

                    BluErr = BluErr - nTotalErr;
                    lpBluErrFwd [j + 1] += BluErr;

        }
    
        lpSourcePtr -= (3 * wPaddedScanWidth);
        lpSourcePtr += wInputBytesPerRow;
    
    }
    
}
#endif





void FAR dither_bkcolor (lpData, lpErrBuf, wPaddedScanWidth, wRowsThisStrip, wErrBufLen, ClipTable, lpSmallhist, lpRGBQuadPal, wInputBytesPerRow)
LPSTR lpData;
LPSTR lpErrBuf;
WORD  wPaddedScanWidth;
WORD  wRowsThisStrip;
WORD  wErrBufLen;
PSTR  ClipTable;
LPSTR lpSmallhist;
RGBQUAD FAR *lpRGBQuadPal;
WORD  wInputBytesPerRow;
{

    //  (* Next *) error filters.  We will have as many of these as "next" places 
    //   appear in current data line in filters.  Place "nexts" at end of erro
    //   buf in each color plane.

    //  We only handle error from one line at a time, even though our dither is buffered...

    LPSTR   lpRedErrNext;
    LPSTR   lpGrnErrNext;
    LPSTR   lpBluErrNext;

    LPSTR   lpRedErrNext2;
    LPSTR   lpGrnErrNext2;
    LPSTR   lpBluErrNext2;

    LPSTR   lpRedErrCurr;
    LPSTR   lpGrnErrCurr;
    LPSTR   lpBluErrCurr;

    LPSTR   lpRedErrFwd;
    LPSTR   lpGrnErrFwd;
    LPSTR   lpBluErrFwd;


    LPSTR   lpCurrErrBuf;
    LPSTR   lpFwdErrBuf;
    LPSTR   lpSourcePtr;
    LPSTR   lpDestPtr;

    int     Red, Grn, Blu;
    int     Color;
    int     RedErr, GrnErr, BluErr;

    WORD    i, j;

    WORD    wPlaneLen;        // Length of a dist buffer for a single color plane including "next" space
    WORD    wErrLineLen;      // Length of a dist buffer for all color planes
    WORD    wErrLen;


    int     nErr;
    int     nTotalErr;

    wPlaneLen   = wPaddedScanWidth + 2;  // 2 because 2 "next"'s
    wErrLineLen = (3 * wPlaneLen);
    wErrLen     = (1 * wErrLineLen);  // FS only has 1 line below, thus "1"

    lpSourcePtr = lpData;
    lpDestPtr   = lpData;

    lpCurrErrBuf = lpErrBuf;
    lpFwdErrBuf  = lpErrBuf + wErrLen;

    lpRedErrCurr = lpCurrErrBuf;
    lpGrnErrCurr = lpRedErrCurr + wPlaneLen;
    lpBluErrCurr = lpGrnErrCurr + wPlaneLen;

    lpRedErrFwd  = lpFwdErrBuf;
    lpGrnErrFwd  = lpRedErrFwd + wPlaneLen;
    lpBluErrFwd  = lpGrnErrFwd + wPlaneLen;


    /*  Curr2 and Fwd2 will be defined here  */


    lpRedErrNext  = lpRedErrFwd + wPaddedScanWidth;
    lpGrnErrNext  = lpGrnErrFwd + wPaddedScanWidth;
    lpBluErrNext  = lpBluErrFwd + wPaddedScanWidth;

    lpRedErrNext2 = lpRedErrNext + 1;
    lpGrnErrNext2 = lpGrnErrNext + 1;
    lpBluErrNext2 = lpBluErrNext + 1;

    /*  Next2, 3, etc., will be defined here */


    *lpRedErrNext = 0;
    *lpGrnErrNext = 0;
    *lpBluErrNext = 0;

    for (i = 0; i < wRowsThisStrip; i++)
    {

        /*  Transfer forward error lines to current error lines & blank fwd err line */

        _fmemcpy (lpRedErrCurr, lpRedErrFwd, wErrLen);
        _fmemset (lpRedErrFwd, 0, wErrLen);


        for (j = 0; j < wPaddedScanWidth; j++)
        {

            Blu = ((BYTE) *lpSourcePtr++ >> 2);
            Grn = ((BYTE) *lpSourcePtr++ >> 2);
            Red = ((BYTE) *lpSourcePtr++ >> 2);

    

//      goto one;


            /*  Add errors from dist buffer to these values  */
    
            Blu += lpBluErrCurr [j];
            Grn += lpGrnErrCurr [j];
            Red += lpRedErrCurr [j];
    
    
            /*  Handle "next" positions */
    
            Blu += *lpBluErrNext;
            Grn += *lpGrnErrNext;
            Red += *lpRedErrNext;
    
    
            Blu = ClipTable [(BYTE) Blu];
            Grn = ClipTable [(BYTE) Grn];
            Red = ClipTable [(BYTE) Red];

    
            *lpRedErrNext  = *lpRedErrNext2;
            *lpGrnErrNext  = *lpGrnErrNext2;
            *lpBluErrNext  = *lpBluErrNext2;

            /*  Find closest color  */
    


//      one:

            Color = (BYTE) lpSmallhist [((Red >> 1) << 10) + ((Grn >> 1) << 5) + (Blu >> 1)];
    
            *lpDestPtr++ = (BYTE) Color;
    
//      goto noerr;


            RedErr = Red - (BYTE) (lpRGBQuadPal [(BYTE) Color].rgbRed   >> 2);
            GrnErr = Grn - (BYTE) (lpRGBQuadPal [(BYTE) Color].rgbGreen >> 2);
            BluErr = Blu - (BYTE) (lpRGBQuadPal [(BYTE) Color].rgbBlue  >> 2);
    
    
            /*  Distribute error to FORWARD list  */
    
    
            /*  Red  */
    
            nErr = (int) ((WORD) RedErr * 8) >> 5;

            lpRedErrFwd [j] += nErr; 
            nTotalErr        = nErr;

            if (j < (wPaddedScanWidth - 1))
            {
                nErr = (RedErr * 8) >> 5;
                *lpRedErrNext += nErr;   // Add to Next, move to Next2...
                nTotalErr += nErr;
    
                nErr = (RedErr * 4) >> 5;
                *lpRedErrNext2 = (BYTE) nErr;   // Add to Next, move to Next2...
                nTotalErr += nErr;

                nErr = (RedErr * 4) >> 5;
                lpRedErrFwd [j + 1] += nErr; 
                nTotalErr += nErr;

                nErr = (RedErr * 2) >> 5;
                lpRedErrFwd [j + 2] += nErr; 
                nTotalErr += nErr;

//              if (j > 0)
                {
                    nErr = (RedErr * 2) >> 5;
                    lpRedErrFwd [j - 2] += nErr; 
                    nTotalErr += nErr;

                    nErr = (RedErr * 4) >> 5;
                    lpRedErrFwd [j - 1] += nErr; 
                    nTotalErr += nErr;

                }

            }

                    RedErr = RedErr - nTotalErr;
                    lpRedErrFwd [j + 1] += RedErr;


            /*  Grn  */
    
            nErr = (int) ((WORD) GrnErr * 8) >> 5;

            lpGrnErrFwd [j] += nErr; 
            nTotalErr        = nErr;

            if (j < (wPaddedScanWidth - 1))
            {
                nErr = (GrnErr * 8) >> 5;
                *lpGrnErrNext += nErr;   // Add to Next, move to Next2...
                nTotalErr += nErr;
    
                nErr = (GrnErr * 4) >> 5;
                *lpGrnErrNext2 = (BYTE) nErr;   // Add to Next, move to Next2...
                nTotalErr += nErr;

                nErr = (GrnErr * 4) >> 5;
                lpGrnErrFwd [j + 1] += nErr; 
                nTotalErr += nErr;

                nErr = (GrnErr * 2) >> 5;
                lpGrnErrFwd [j + 2] += nErr; 
                nTotalErr += nErr;

//              if (j > 0)
                {
                    nErr = (GrnErr * 2) >> 5;
                    lpGrnErrFwd [j - 2] += nErr; 
                    nTotalErr += nErr;

                    nErr = (GrnErr * 4) >> 5;
                    lpGrnErrFwd [j - 1] += nErr; 
                    nTotalErr += nErr;

                }

            }


                    GrnErr = GrnErr - nTotalErr;
                    lpGrnErrFwd [j + 1] += GrnErr;




            /*  Blu  */
    
            nErr = (int) ((WORD) BluErr * 8) >> 5;

            lpBluErrFwd [j] += nErr; 
            nTotalErr        = nErr;

            if (j < (wPaddedScanWidth - 1))
            {
                nErr = (BluErr * 8) >> 5;
                *lpBluErrNext += nErr;   // Add to Next, move to Next2...
                nTotalErr += nErr;
    
                nErr = (BluErr * 4) >> 5;
                *lpBluErrNext2 = (BYTE) nErr;   // Add to Next, move to Next2...
                nTotalErr += nErr;

                nErr = (BluErr * 4) >> 5;
                lpBluErrFwd [j + 1] += nErr; 
                nTotalErr += nErr;

                nErr = (BluErr * 2) >> 5;
                lpBluErrFwd [j + 2] += nErr; 
                nTotalErr += nErr;

//              if (j > 0)
                {
                    nErr = (BluErr * 2) >> 5;
                    lpBluErrFwd [j - 2] += nErr; 
                    nTotalErr += nErr;

                    nErr = (BluErr * 4) >> 5;
                    lpBluErrFwd [j - 1] += nErr; 
                    nTotalErr += nErr;


                }

            }

                    BluErr = BluErr - nTotalErr;
                    lpBluErrFwd [j + 1] += BluErr;

//noerr:
//          BluErr = BluErr;

        }

        lpSourcePtr -= (3 * wPaddedScanWidth);
        lpSourcePtr += wInputBytesPerRow;
    
    }
    
}
    
