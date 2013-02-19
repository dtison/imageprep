

#include <windows.h>
#include <memory.h>
#include <imgprep.h>
#include <global.h>
#include <cpi.h>
#include <epsfmt.h>
#include <error.h>

int NEAR PASCAL WriteEPSHeader (int, LPFILEINFO, BOOL);
int NEAR PASCAL WriteEPSStrip (int, LPSTR, LPSTR, LPFILEINFO);
int NEAR PASCAL WriteEPSGrayStrip (int, LPSTR, LPSTR, LPFILEINFO);

BOOL FAR PASCAL FormatEPSMono (LPSTR, LPSTR, WORD);
void FAR fmt_eps_gray (LPSTR, LPSTR, WORD);


/*---------------------------------------------------------------------------

  PROCEDURE:      EPSInitHeader
  DESCRIPTION;    Writes EPS header 
  DEFINITION:
  MODIFIED:       1/16/90 - David Ison (graft into ImagePrep)
                  3/14/90 - David Ison (fixed EPS save bug)

---------------------------------------------------------------------------*/

int FAR PASCAL EPSInitHeader (hFile, lpBuffer, hIFInfo, hIBInfo,hEFInfo, hEBInfo)
int     hFile;
LPSTR   lpBuffer;
HANDLE  hIFInfo;    
HANDLE  hIBInfo;    
HANDLE  hEFInfo;    
HANDLE  hEBInfo;    
{
    LPFILEINFO    lpIFileInfo;
    LPBITMAPINFO  lpIBitmapInfo;
    LPFILEINFO    lpEFileInfo;
    LPBITMAPINFO  lpEBitmapInfo;
    WORD          wBytesPerStrip;
    WORD          wRowsPerStrip;
    WORD          wNumStrips;
    WORD          wBytesPerLastStrip;
    WORD          wRowsPerLastStrip;
    WORD          wRowCount;
    HANDLE        hLocalBuf;
    LPEPSFIXUP    lpEPSFixup;
    
    BOOL          bIsColor = TRUE;


    if (! (lpIFileInfo = (LPFILEINFO) GlobalLock (hIFInfo)))
        return (EC_MEMORY1 );
    
    if (! (lpEFileInfo = (LPFILEINFO) GlobalLock (hEFInfo)))
    {
        GlobalUnlock (hIFInfo);
        return (EC_MEMORY1);
    }
    
    if (! (lpIBitmapInfo = (LPBITMAPINFO) GlobalLock (hIBInfo)))
    {
        GlobalUnlock (hIFInfo);
        GlobalUnlock (hEFInfo);
        return (EC_MEMORY1);
    }
    
    if (! (lpEBitmapInfo = (LPBITMAPINFO) GlobalLock (hEBInfo)))
    {
        GlobalUnlock (hIFInfo);
        GlobalUnlock (hEFInfo);
        GlobalUnlock (hIBInfo);
        return (EC_MEMORY1);
    }
    
    /*  Point to format specific fields right after global file fields  */
    
    lpEPSFixup = (LPEPSFIXUP) ((LPSTR) lpEFileInfo + sizeof (FILEINFO));
    
    /*  First get some memory space to work with  */
    
    hLocalBuf = LocalAlloc (LHND, 512);
    if (! hLocalBuf)
    {
        GlobalUnlock (hIFInfo);
        GlobalUnlock (hEFInfo);
        GlobalUnlock (hIBInfo);
        GlobalUnlock (hEBInfo);
        return (EC_MEMORY2);
    }
    
    lpEPSFixup -> hLocalBuf = hLocalBuf;
    
    if (wExportClass == IMON || wExportClass == IGR)
        bIsColor = FALSE;

    /* Setup buffering stuff */ 
    
    wRowsPerStrip   = lpIFileInfo -> wRowsPerStrip;
    wBytesPerStrip  = wRowsPerStrip * lpEFileInfo -> wPaddedScanWidth;
    wNumStrips      = lpEFileInfo -> wScanHeight / wRowsPerStrip;
    wRowCount       = wNumStrips * wRowsPerStrip;
    
    if (wRowCount < lpEFileInfo->wScanHeight)
    {
        wBytesPerLastStrip  = (lpEFileInfo->wScanHeight - wRowCount) * lpEFileInfo->wPaddedScanWidth;
        wRowsPerLastStrip   = (lpEFileInfo->wScanHeight - wRowCount);
        wNumStrips++;
    }
    else
    {
        wBytesPerLastStrip = wBytesPerStrip;
        wRowsPerLastStrip  = wRowsPerStrip;
    }
    
    lpEFileInfo->wNumStrips      = wNumStrips;
    lpEFileInfo->wBytesPerStrip  = wBytesPerStrip;
    lpEFileInfo->wRowsPerStrip   = wRowsPerStrip;
    
    lpEFileInfo->wBytesPerLastStrip  = wBytesPerLastStrip;
    lpEFileInfo->wRowsPerLastStrip   = wRowsPerLastStrip;
    
    
    /* Create the EPS file header     */
    
    if (! WriteEPSHeader (hFile, lpEFileInfo, bIsColor))
    {
        GlobalUnlock (hIFInfo);
        GlobalUnlock (hEFInfo);
        GlobalUnlock (hIBInfo);
        GlobalUnlock (hEBInfo);
        return (EC_FILEWRITE2);
    }
    
    GlobalUnlock (hIFInfo);
    GlobalUnlock (hEFInfo);
    GlobalUnlock (hIBInfo);
    GlobalUnlock (hEBInfo);
    return (0);  // OK return
}   
    

int FAR PASCAL EPSWrConvertData (hFile, lppT, lppD, lpEFileInfo, lpEBitmapInfo)
int           hFile;
LPSTR FAR *   lppT;
LPSTR FAR *   lppD;
LPFILEINFO    lpEFileInfo;     
LPBITMAPINFO  lpEBitmapInfo;   
{
    WORD        wBytesThisStrip;
    int         nRetval = TRUE;
    LPSTR       lpSrc;
    LPSTR       lpSrcPtr;
    LPSTR       lpTmp;
    LPSTR       lpTmpPtr;
    WORD        wBytesThisRow;
    WORD        wPaddedBytesThisRow;
    WORD        wPaddedBytesThisStrip;
    WORD        wRowsThisStrip;
    WORD        wRows;
    LPEPSFIXUP  lpEPSFixup;
    BOOL        bIsColor = TRUE;
    
    lpSrc = *lppD;
    lpTmp = *lppT;


    lpEPSFixup = (LPEPSFIXUP) ((LPSTR) lpEFileInfo + sizeof (FILEINFO));

    if (wExportClass == IMON || wExportClass == IGR)
        bIsColor = FALSE;

    
    wBytesThisRow         = lpEFileInfo->wBytesPerRow;
    wPaddedBytesThisRow   = lpEFileInfo->wPaddedBytesPerRow;

    if (lpEFileInfo->bIsLastStrip)
    {
        wPaddedBytesThisStrip = lpEFileInfo->wPaddedBytesPerRow *
                                lpEFileInfo->wRowsPerLastStrip;
        wBytesThisStrip       = lpEFileInfo->wBytesPerRow *
                                lpEFileInfo->wRowsPerLastStrip;
        wRowsThisStrip        = lpEFileInfo->wRowsPerLastStrip;
    }
    else
    {
        wBytesThisStrip       = lpEFileInfo->wBytesPerRow *
                                lpEFileInfo->wRowsPerStrip;
        wPaddedBytesThisStrip = lpEFileInfo->wPaddedBytesPerRow *
                                lpEFileInfo->wRowsPerStrip;
        wRowsThisStrip        = lpEFileInfo->wRowsPerStrip;
    }
    
    if (wBytesThisRow != wPaddedBytesThisRow)               // Need to clip 
    {
        lpTmpPtr = lpTmp;
        lpSrcPtr = lpSrc;

        for (wRows = 0; wRows < wRowsThisStrip; wRows++)    // Clip each scanline
        {
            _fmemcpy (lpTmpPtr, lpSrcPtr, wBytesThisRow);
            lpTmpPtr += wBytesThisRow;
            lpSrcPtr += wPaddedBytesThisRow;
        }
    

        lpTmpPtr  = lpSrc;
        lpSrc     = lpTmp;
        lpTmp     = lpTmpPtr;    
    }



    /*    Now have data ready, (in lpSrc) format 24 bit data to eps  */

    if (bIsColor)
        if (! WriteEPSStrip (hFile, lpTmp,  lpSrc, lpEFileInfo))
             nRetval = EC_FILEWRITE2;
        else;
    else
        if (! WriteEPSGrayStrip (hFile, lpTmp,  lpSrc, lpEFileInfo))
             nRetval = EC_FILEWRITE2;
        
    return (nRetval);
}


/*---------------------------------------------------------------------------

  PROCEDURE:      EPSFixupHeader
  DESCRIPTION;    Finished Header
  DEFINITION:
  MODIFIED:       1/15/90 - David Ison - Graft into Imageprep

---------------------------------------------------------------------------*/

int FAR PASCAL EPSFixupHeader (hFile, lpBuffer, hIFInfo, hIBInfo, hEFInfo, hEBInfo)
int     hFile;
LPSTR   lpBuffer;
HANDLE  hIFInfo;
HANDLE  hEFInfo;
HANDLE  hIBInfo;
HANDLE  hEBInfo;
{
    LPFILEINFO        lpEFileInfo;
    LPEPSFIXUP        lpEPSFixup;
    WORD              bytes;
    PSTR              EPS_Str;
    
    /*  Point to format specific fields right after global file fields  */
    
    if (! (lpEFileInfo = (LPFILEINFO) GlobalLock (hEFInfo)))
        return (EC_MEMORY1);
    
    lpEPSFixup = (LPEPSFIXUP) ((LPSTR) lpEFileInfo + sizeof (FILEINFO));
    
    EPS_Str = LocalLock (lpEPSFixup -> hLocalBuf);
    
    wsprintf ((LPSTR)EPS_Str,"showpage\r\n");
    
    bytes = _lwrite (hFile, (LPSTR) EPS_Str, lstrlen (EPS_Str));
    if (bytes != (WORD)lstrlen (EPS_Str))
    {
        LocalUnlock (lpEPSFixup -> hLocalBuf);
        GlobalUnlock (hEFInfo);
        return (FALSE); 
    }
    
    LocalUnlock (lpEPSFixup->hLocalBuf);
    LocalFree (lpEPSFixup->hLocalBuf);
    GlobalUnlock( hEFInfo );
    
    return (0);
}   
    

int NEAR PASCAL WriteEPSHeader (hFile, lpFileInfo, bIsColor)
int         hFile; 
LPFILEINFO  lpFileInfo;
BOOL        bIsColor;
{

    HANDLE hLocalBuf;
    PSTR   pLocalBuf;
    PSTR   EPS_Str;
    WORD   wScanWidth;
    WORD   wScanHeight;
    WORD   bytes;

    hLocalBuf = LocalAlloc (LHND, 512);
    if (! hLocalBuf)
        return (FALSE);

    pLocalBuf = LocalLock (hLocalBuf);
    EPS_Str   = pLocalBuf;

    wScanWidth  = lpFileInfo -> wScanWidth;
    wScanHeight = lpFileInfo -> wScanHeight;


    /*  Do the EPS header                 */

    EPS_Str = pLocalBuf;
    wsprintf ((LPSTR)EPS_Str,(LPSTR)"%%!PS-Adobe-2.0 EPSF-1.2\r\n");
    EPS_Str += lstrlen (EPS_Str);

    wsprintf ((LPSTR)EPS_Str,(LPSTR)"%%%%Creator: ImagePrep 3.00.00\r\n");
    EPS_Str += lstrlen (EPS_Str);

//    wsprintf ((LPSTR)EPS_Str,(LPSTR)"%%%%Title: ImagePrep\r\n");
    wsprintf ((LPSTR)EPS_Str,(LPSTR)"%%%%Title: %s\r\n", (LPSTR)szSaveFileName);
    EPS_Str += lstrlen (EPS_Str);

    wsprintf ((LPSTR)EPS_Str,(LPSTR)"%%%%BoundingBox: 0 0 %d %d\r\n",
      wScanWidth,wScanHeight);
    EPS_Str += lstrlen (EPS_Str);

    wsprintf ((LPSTR)EPS_Str,(LPSTR)"%%%%DocumentFonts: Helvetica Helvetica-Bold Helvetica-Oblique Times-Roman\r\n");
    EPS_Str += lstrlen (EPS_Str);

    wsprintf ((LPSTR)EPS_Str,(LPSTR)"%%%%Pages: 1\r\n");
    EPS_Str += lstrlen (EPS_Str);

    wsprintf ((LPSTR)EPS_Str,(LPSTR)"%%%%EndComments\r\n");
    EPS_Str += lstrlen (EPS_Str);

    wsprintf ((LPSTR)EPS_Str,(LPSTR)"/rstr %d string def\r\n",wScanWidth);
    EPS_Str += lstrlen (EPS_Str);


    if (bIsColor) 
    {
        wsprintf ((LPSTR)EPS_Str,(LPSTR)"/gstr %d string def\r\n",wScanWidth);
        EPS_Str += lstrlen (EPS_Str);

        wsprintf ((LPSTR)EPS_Str,(LPSTR)"/bstr %d string def\r\n",wScanWidth);
        EPS_Str += lstrlen (EPS_Str);
    }


    wsprintf ((LPSTR)EPS_Str,(LPSTR)"%d %d scale\r\n",wScanWidth,wScanHeight);
    EPS_Str += lstrlen (EPS_Str);
  
    wsprintf ((LPSTR)EPS_Str,(LPSTR)"%d %d 8\r\n",wScanWidth,wScanHeight);
    EPS_Str += lstrlen (EPS_Str);

    wsprintf ((LPSTR)EPS_Str,(LPSTR)"[%d 0 0 -%d 0 %d]\r\n",wScanWidth,wScanHeight,wScanHeight);
    EPS_Str += lstrlen (EPS_Str);

    wsprintf ((LPSTR)EPS_Str,(LPSTR)"{currentfile rstr readhexstring pop}\r\n");
    EPS_Str += lstrlen (EPS_Str);

    if (bIsColor) 
    {
        wsprintf ((LPSTR)EPS_Str,(LPSTR)"{currentfile gstr readhexstring pop}\r\n");
        EPS_Str += lstrlen (EPS_Str);
    
        wsprintf ((LPSTR)EPS_Str,(LPSTR)"{currentfile bstr readhexstring pop}\r\n");
        EPS_Str += lstrlen (EPS_Str);
    
        wsprintf ((LPSTR)EPS_Str,(LPSTR)"true 3\r\n");
        EPS_Str += lstrlen (EPS_Str);
    
        wsprintf ((LPSTR)EPS_Str,(LPSTR)"colorimage\r\n");
    
    }
    else
        wsprintf ((LPSTR)EPS_Str, (LPSTR)"image\r\n");

    EPS_Str = pLocalBuf;

    bytes = _lwrite (hFile, (LPSTR) EPS_Str, lstrlen (EPS_Str));
    if (bytes != (WORD)lstrlen (EPS_Str))
      return (FALSE);

}




int NEAR PASCAL WriteEPSStrip (hFile, lpEPSBuf, lpDataBuf, lpFileInfo)
int     hFile;
LPSTR   lpEPSBuf;
LPSTR   lpDataBuf;
LPFILEINFO lpFileInfo;
{ 
   
    int       wBytes;
    WORD      wEPSLineSize;
    WORD      wRowsThisStrip;
    WORD      wBytesBuffered;
    WORD      wBytesPerRow;
    int       i;

    LPSTR     lpSource;
    LPSTR     lpDest;
    LPSTR     lpTmp;

    LPSTR     lpSourcePtr; 
    LPSTR     lpDestPtr;

    if (lpFileInfo -> bIsLastStrip)
        wRowsThisStrip = lpFileInfo -> wRowsPerLastStrip;
    else
        wRowsThisStrip = lpFileInfo -> wRowsPerStrip;


    wBytesPerRow = (lpFileInfo -> wScanWidth * 3); 

    wEPSLineSize   = (2 * lpFileInfo -> wBytesPerRow) + 6;


    /*  First get from bgr to rgb format  */
    /*            (source)      (dest)    */


    lpDest    = lpEPSBuf;
    lpSource  = lpDataBuf;

    RGBToBGR (lpDest, lpSource, wRowsThisStrip, lpFileInfo->wScanWidth, wBytesPerRow);

    lpTmp     = lpSource;
    lpSource  = lpDest;
    lpDest    = lpTmp;


    lpSourcePtr = lpSource;
    lpDestPtr   = lpDest;

    wBytesBuffered  = 0;


    /*  Format this strip into EPS line by line, buffering as we go.. */

    for (i = 0; i < (int)wRowsThisStrip; i++)
    {

        fmt_rgb_eps (lpSourcePtr, lpDestPtr, lpFileInfo->wScanWidth, 1);

        lpSourcePtr     += wBytesPerRow;
        lpDestPtr       += wEPSLineSize;
        wBytesBuffered  += wEPSLineSize;

        if (wBytesBuffered > 8000) 
        {
            wBytes = _lwrite (hFile, lpDest, wBytesBuffered);
            if (wBytes != (int)wBytesBuffered)
              return (FALSE);

            /*  Reset output buffer pointer and byte counter  */

            lpDestPtr       = lpDest;
            wBytesBuffered  = 0;
        }
    }   

    /*  Flush buffer if applicable  */

    if (wBytesBuffered > 0)
    {
        wBytes = _lwrite (hFile, lpDest, wBytesBuffered);
        if (wBytes != (int)wBytesBuffered)
            return (FALSE);
    }

    return (TRUE); 
}










int NEAR PASCAL WriteEPSGrayStrip (hFile, lpEPSBuf, lpDataBuf, lpFileInfo)
int     hFile;
LPSTR   lpEPSBuf;
LPSTR   lpDataBuf;
LPFILEINFO lpFileInfo;
{ 
   
    int       wBytes;
    WORD      wEPSLineSize;
    WORD      wRowsThisStrip;
    WORD      wBytesBuffered;
    WORD      wBytesPerRow;
    int       i;

    LPSTR     lpSource;
    LPSTR     lpDest;

    LPSTR     lpSourcePtr; 
    LPSTR     lpDestPtr;

    if (lpFileInfo -> bIsLastStrip)
        wRowsThisStrip = lpFileInfo -> wRowsPerLastStrip;
    else
        wRowsThisStrip = lpFileInfo -> wRowsPerStrip;


    wBytesPerRow = (lpFileInfo -> wScanWidth); 

    wEPSLineSize   = (2 * lpFileInfo -> wBytesPerRow) + 2;




    lpDest      = lpEPSBuf;
    lpSource    = lpDataBuf;

    lpSourcePtr = lpSource;
    lpDestPtr   = lpDest;

    wBytesBuffered  = 0;


    /*  Format this strip into EPS line by line, buffering as we go.. */

    for (i = 0; i < (int)wRowsThisStrip; i++)
    {
        if (wExportClass == IGR)
          fmt_eps_gray (lpDestPtr, lpSourcePtr, lpFileInfo->wScanWidth);
        else
          FormatEPSMono (lpDestPtr, lpSourcePtr, lpFileInfo->wScanWidth);

        lpSourcePtr     += wBytesPerRow;
        lpDestPtr       += wEPSLineSize;
        wBytesBuffered  += wEPSLineSize;

        if (wBytesBuffered > 8000) 
        {
            wBytes = _lwrite (hFile, lpDest, wBytesBuffered);
            if (wBytes != (int)wBytesBuffered)
              return (FALSE);

            /*  Reset output buffer pointer and byte counter  */

            lpDestPtr       = lpDest;
            wBytesBuffered  = 0;
        }
    }   

    /*  Flush buffer if applicable  */

    if (wBytesBuffered > 0)
    {
        wBytes = _lwrite (hFile, lpDest, wBytesBuffered);
        if (wBytes != (int)wBytesBuffered)
            return (FALSE);
    }

    return (TRUE); 
}
