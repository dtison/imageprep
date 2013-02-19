#include <windows.h>
#include <memory.h>
#include <io.h>
#include <imgprep.h>
#include <strtable.h>
#include <proto.h>
#include <reduce.h>
#include <global.h>
#include <error.h>
#include <memory.h>   // For C 6.0 memory copy etc.
#include <cpi.h>      // CPI library tools
#include <palette.h>

/*---------------------------------------------------------------------------

   PROCEDURE:         GenPal
   DESCRIPTION:       Generate appropriate palette for (display or export)
                      as indicated by the typepal and return the handle to
                      the palette
   DEFINITION:        5.7.4
   START:             1/6/90  TBJ.   
   MODS:              3/23/90 Port to Win 3.0  David Ison 

----------------------------------------------------------------------------*/

HANDLE   GenPal (nPaletteType, lpFileInfo, lpBitmapInfo, hFile)
int             nPaletteType;
LPFILEINFO      lpFileInfo;
LPBITMAPINFO    lpBitmapInfo;
int             hFile;
{
  BOOL          bIsDither;
  HANDLE        hTmpPal;
  int           nReturnPal;
  int           nPalType;
  LPDISPINFO    lpDispInfo;
  LPSTR         lpInputBuf;
  LPSTR         lpTmpPalette;



  if (! (lpInputBuf = (LPSTR)GlobalLockDiscardable (hGlobalBufs)))
  {
    return (0);
  }

  lpDispInfo = (LPDISPINFO)((LPSTR)lpFileInfo + 512);

  if (!(hTmpPal = GlobalAlloc (GHND, 1024L)))
  {
    GlobalUnlock (hGlobalBufs);
    return (0);
  }   

  if (!(lpTmpPalette = (LPSTR)GlobalLock (hTmpPal)))
  {
    GlobalUnlock (hGlobalBufs);
    GLOBALFREE (hTmpPal);
    return (0);
  }

  if (wDither == IFS || wDither == IFS16)
    bIsDither = TRUE;
  else
    bIsDither = FALSE;
  switch (nPaletteType){
    case IMPORT_PAL:
      SetImportStates();
      nPalType = dPalType;
      break;
    case EXPORT_PAL:
      SetExportStates();
      nPalType = ePalType;
      bRestoreDPal = bGetEPal = (ePalType != dPalType) ? (BYTE) TRUE : (BYTE) FALSE;
      break;
  }
  switch (nPalType)
  {
    case   DP256:     
      nReturnPal = MakeDefaultPalette (lpTmpPalette, 256, lpFileInfo);
      break;
    case   DP8: 
      nReturnPal = MakeDefaultPalette (lpTmpPalette, 8, lpFileInfo);
      break;            
    case   DP16:
      nReturnPal = MakeDefaultPalette (lpTmpPalette, 16, lpFileInfo);
      break;
    case   OP256:    
      nReturnPal = MakeOptimizedPalette (lpTmpPalette, 256, lpFileInfo, lpBitmapInfo, lpInputBuf, hFile, bIsDither);
      break;
    case   OP8: 
      nReturnPal = MakeOptimizedPalette (lpTmpPalette, 8, lpFileInfo, lpBitmapInfo, lpInputBuf, hFile, bIsDither);
      break;
    case   OP16:   
      nReturnPal = MakeOptimizedPalette (lpTmpPalette, 256, lpFileInfo, lpBitmapInfo, lpInputBuf, hFile, bIsDither);
      break;
    case   GP256: 
    case   GP64:    
      nReturnPal = MakeGrayPalette (lpTmpPalette, 256);
      break;
    case   GP16:
      nReturnPal = MakeGrayPalette (lpTmpPalette, 16);
      break;

    #ifdef NEVER
    case   GP8: 
//    nReturnPal = MakeGrayPalette (lpTmpPalette, 8);
    nReturnPal = MakeGrayPalette (lpTmpPalette, 256);
      break;
    #endif
    case   BP:
      nReturnPal = MakeBinaryPalette (lpTmpPalette);
      break;
    case   IP256: 
      nReturnPal = MakeImportPalette (lpTmpPalette, lpBitmapInfo);
      break;
  }
  if (nReturnPal < 0)
  {
    GlobalUnlock (hGlobalBufs);
    GlobalUnlock (hTmpPal);
    GLOBALFREE (hTmpPal);
    return (nReturnPal);
  }
  GlobalUnlock (hGlobalBufs);
  GlobalUnlock (hTmpPal);
  return (hTmpPal);
}


#ifdef NEVER
int SetAllDacs (hInternalPal, lpBuffer, hDC)
HANDLE  hInternalPal;
LPSTR   lpBuffer;
HDC     hDC;
{
  HPALETTE        hPalette;
  HPALETTE        hOldPalette;
  LPLOGPALETTE    lpLogPalette;
  RGBQUAD  FAR *  lpRGBQuadPtr;

  lpRGBQuadPtr = (RGBQUAD FAR *)GlobalLock (hInternalPal);
  lpLogPalette = (LPLOGPALETTE)lpBuffer;
  CreateLogPalFromRGBQuad( 
    lpLogPalette, 
    lpRGBQuadPtr, 
    (wDevice == DVGA ? 8 : 256) ); 
  hPalette = CreatePalette( lpLogPalette );
  hOldPalette = SelectPalette( hDC, hPalette, FALSE );
  RealizePalette( hDC );
  GlobalUnlock (hInternalPal);
  return (hPalette);     
}
#endif

HPALETTE NEAR PASCAL SetPalette (hDC, hInternalPal, lpBufs)
HDC           hDC;
HANDLE        hInternalPal;
LPSTR         lpBufs;
{
    HPALETTE      hPalette;
    LPLOGPALETTE  lpLogPalette;
    RGBQUAD FAR * lpRGBQuadPtr;

    if (bIsFirstPaint)
    {
        lpRGBQuadPtr = (RGBQUAD FAR *) GlobalLock (hInternalPal);
        lpLogPalette = (LPLOGPALETTE)lpBufs;
        CreateLogPalFromRGBQuad (lpLogPalette, lpRGBQuadPtr, (wDevice == DVGA ? 8 : 256));
        hPalette = CreatePalette (lpLogPalette);
        hOldPalette = SelectPalette (hDC, hPalette, 0);
        RealizePalette (hDC);
        hDispPalette = hPalette;
        GlobalUnlock (hInternalPal);
    }
    else
    {
        SelectPalette (hDC, hDispPalette, 0);
        hPalette = hDispPalette;
    }
    return (hPalette);
}

int NEAR PASCAL MakeBinaryPalette (lpTmpPalette)
LPSTR lpTmpPalette;
{
    int           i;
    RGBQUAD FAR * lpRGBQuadPtr;

    lpRGBQuadPtr = (RGBQUAD FAR *)lpTmpPalette;
    _fmemset ((LPSTR)lpRGBQuadPtr, 255, 1024);
    for (i = 0; i < 256; i += 255)
    {
        lpRGBQuadPtr->rgbRed    = (BYTE) i;
        lpRGBQuadPtr->rgbGreen  = (BYTE) i;
        lpRGBQuadPtr->rgbBlue   = (BYTE) i;
        lpRGBQuadPtr->rgbReserved = 0; 
        lpRGBQuadPtr++;
    }
    return (0);
}


int NEAR PASCAL MakeDefaultPalette (lpTmpPalette, wNumColors, lpInFileInfo)
LPSTR         lpTmpPalette;
WORD          wNumColors;
LPFILEINFO    lpInFileInfo;
{                      
    HANDLE          hResPalMem;
    HANDLE          hResHistMem;
    HANDLE          hResHist;
    HANDLE          hResPal;
    LPSTR           lpResPalette;
    LPSTR           lpResHist;
    LPSTR           lpPtr;
    WORD            wMapLen;
    WORD            wPaletteLen;
    RGBQUAD FAR *   lpRGBQuadPtr;

    lpRGBQuadPtr = (RGBQUAD FAR *)lpTmpPalette;
    bNewDefault = FALSE;

    if (! (hBhist = GlobalAlloc (GHND, 32768L)))
        return (EC_NOMEM);

    if (! (lpSmallhist = lpbHist = (BYTE FAR *)GlobalLock (hBhist)))
    {
        GLOBALFREE (hBhist);
        return (EC_MEMORY1);
    }

    /*  Read the (* smallhist *) and palette info from the disk   */

    if (wNumColors == 256)
    {
        GenerateDefaultPalette (lpTmpPalette, 256);
        hResPal  = FindResource (hInstIP, (LPSTR)"quadpal8",  (LPSTR)"DEFPALETTE");
        hResHist = FindResource (hInstIP, (LPSTR)"smallh8", (LPSTR)"SMALLHIST");
        wPaletteLen   = 1024;
        wMapLen       = 256;
    }
    else
    {
        /*   8 or 16 colors     */

        if (wNumColors == 8)
        {
            GenerateDefaultPalette (lpTmpPalette, 8);
            hResPal     = FindResource (hInstIP, (LPSTR)"quadpal3", (LPSTR)"DEFPALETTE");
            hResHist    = FindResource (hInstIP, (LPSTR)"smallh3", (LPSTR)"SMALLHIST");
            wPaletteLen = 32;
            wMapLen     = 8;
        }
        else
        {
            hResPal  = FindResource (hInstIP, (LPSTR)"quadpal4", (LPSTR)"DEFPALETTE");
            hResHist = FindResource (hInstIP, (LPSTR)"smallh4", (LPSTR)"SMALLHIST");
            wPaletteLen = 1024;
            wMapLen     = 16;
        }
    }
    if (! (hResPalMem = LoadResource (hInstIP, hResPal)))
    {
        GlobalUnlock (hBhist);
        GLOBALFREE (hBhist);
        return (EC_MEMORY2);
    }
    if (! (hResHistMem = LoadResource (hInstIP, hResHist)))
    {
        GlobalUnlock (hBhist);
        GLOBALFREE (hBhist);
        return (EC_MEMORY2);
    }

    lpResPalette  = LockResource (hResPalMem);
    lpResHist     = LockResource (hResHistMem);
    lpPtr         = lpResPalette;

//  if ((wNumColors == 256) && (wDither == IFS))
//    _fmemcpy ((LPSTR)lpRGBQuadPtr, lpPtr, wPaletteLen);

    if (wNumColors == 16) 
        _fmemcpy ((LPSTR)lpRGBQuadPtr, lpPtr, wPaletteLen);

//  if ((wNumColors == 8) && (wDither == IFS) && (wDevice == DVGA))
//      _fmemcpy ((LPSTR)lpRGBQuadPtr, lpPtr, wPaletteLen);

    lpPtr += wPaletteLen;

    #ifdef NEVER
    _fmemcpy ((LPSTR)rm, lpPtr, wMapLen);
    lpPtr += wMapLen;
      
    _fmemcpy ((LPSTR)gm, lpPtr, wMapLen);
    lpPtr += wMapLen;
        
    _fmemcpy ((LPSTR)bm, lpPtr, wMapLen);
    #endif

    _fmemcpy (lpSmallhist, lpResHist, (WORD) 32768);

    UnlockResource (hResPalMem);
    FreeResource (hResPalMem);
    UnlockResource (hResHistMem);
    FreeResource (hResHistMem);

    _fmemcpy ((LPSTR) lpInFileInfo -> HistFile, (LPSTR) HistFile, 128);

    return (0);
}


int NEAR PASCAL GenerateDefaultPalette (lpTmpPalette, colors)
LPSTR lpTmpPalette;
int   colors;
{
    RGBQUAD FAR *   lpRGBQuadPtr;
    WORD w;

    lpRGBQuadPtr = (RGBQUAD FAR *)lpTmpPalette;

    _fmemset ((LPSTR)lpRGBQuadPtr, 255, 1024);

    /*  New default palette stuff.  4/91.  D. Ison  */

    if (colors == 256)
    {
        for (w = 0; w < 256; w++)
        {
            lpRGBQuadPtr [w].rgbRed   = (BYTE)(((w & 0x00E0) >> 5) * 36);
            lpRGBQuadPtr [w].rgbGreen = (BYTE)(((w & 0x001C) >> 2) * 36);
            lpRGBQuadPtr [w].rgbBlue  = (BYTE)((w & 0x0003) * 85);
            lpRGBQuadPtr [w].rgbReserved = 0;
        }
    //  #define FH
        #ifdef FH
        {
            int fh;

            fh = _lcreat ("pal256.dat", 0);

            _lwrite (fh, lpRGBQuadPtr, 1024);
            _lclose (fh);
        }
        #endif
    }
    else
    {
        for (w = 0; w < 8; w++)
        {
            lpRGBQuadPtr [w].rgbRed   = (BYTE)((w & 0x0004) ? 255 : 0);
            lpRGBQuadPtr [w].rgbGreen = (BYTE)((w & 0x0002) ? 255 : 0);
            lpRGBQuadPtr [w].rgbBlue  = (BYTE)((w & 0x0001) ? 255 : 0);
            lpRGBQuadPtr [w].rgbReserved = 0;
        }

        #ifdef FH
        /*  Let's write out the default palette mapping table (NO) */
        {
            int fh;

            fh = _lcreat ("pal8.dat", 0);

            _lwrite (fh, lpRGBQuadPtr, 1024);
            _lclose (fh);
        }
        #endif
    }
    return (0);
}

int NEAR PASCAL MakeGrayPalette (lpTmpPalette, shades)
LPSTR      lpTmpPalette;
int        shades;
{
    int              i;
    int              wInterval;
    RGBQUAD FAR *    lpRGBQuadPtr;

    lpRGBQuadPtr = (RGBQUAD FAR *) lpTmpPalette;

    switch (shades)
    {
        case 256:
        case 64:       
            wInterval = 1;             
            break;

        case 16:       
            wInterval = 16;            
            break;

        case 8:        
            wInterval = 32;            
            break;

        default:       
            return (EC_GSHADES);
            break;
    }

    _fmemset ((LPSTR) lpRGBQuadPtr, 255, 1024);

    for (i = 0; i < 256; i += wInterval)
    {
        lpRGBQuadPtr -> rgbRed      = (BYTE) i;
        lpRGBQuadPtr -> rgbGreen    = (BYTE) i;
        lpRGBQuadPtr -> rgbBlue     = (BYTE) i;     
        lpRGBQuadPtr -> rgbReserved = 0; 
        lpRGBQuadPtr++;
    }

    return (0);
}


/*----------------------------------------------------------------------------

   PROCEDURE:         MakeImportPalette()
   DESCRIPTION:       get image palette
   DEFINITION:        5.7.4.4
   MODS:

----------------------------------------------------------------------------*/

int NEAR PASCAL MakeImportPalette (lpTmpPalette, lpBitmapInfo)
LPSTR              lpTmpPalette;
LPBITMAPINFO       lpBitmapInfo;
{
    _fmemcpy ((LPSTR)lpTmpPalette, (LPSTR)&lpBitmapInfo->bmiColors[0], 1024);
    return (0);
}



int NEAR PASCAL MakeOptimizedPalette (
  LPSTR         lpTmpPalette,
  int           nNumColors,
  LPFILEINFO    lpFileInfo,
  LPBITMAPINFO  lpBitmapInfo,
  LPSTR         lpInputBuf,
  int           hFile,
  BOOL          bIsDither)
{
    RGBQUAD FAR * lpRGBQuadPtr;
    unsigned      bytes;

  
    lpRGBQuadPtr = (RGBQUAD FAR *)lpTmpPalette;

    /*  New 3.1 memory mgt stuff  ...  */

    if (bIsFirstPaint || bNewOpt || bGetEPal || bRestoreDPal)
    {
        bNewOpt = FALSE;

        #ifdef NEVER
        // This is no longer needed since we expanded the size of
        // hGlobalBufs by 64k and can just use some of that memory instead.
        if (!(hWhist = GlobalAlloc (GHND, 65536L)))
        {
            return (EC_NOMEM);
        }
        if (!(lpwHist = (WORD FAR *)GlobalLock (hWhist)))
        {
            GLOBALFREE (hWhist);
            return (EC_MEMORY1);
        }
        #endif
    
        if (!(hBhist = GlobalAlloc (GHND, 32768L)))
        {
            
            return (EC_NOMEM);
        }
        if (!(lpSmallhist = lpbHist = GlobalLock (hBhist)))
        {
            
            GLOBALFREE (hBhist);
            return (EC_MEMORY1);
        }
        _fmemset ((LPSTR) lpRGBQuadPtr, 255, 1024);

// Test new mem-mgt scheme.  Make lpwhist 2nd half of hGlobalBufs...
// At this point, lpInputBuf points to 1st half of 128k buffer.

        lpwHist = (WORD FAR *) ((BYTE huge *) lpInputBuf + LARGEBUFOFFSET);


        if (! ReduceRGBToCM (hWndDisplay, 
                       hFile, 
                       lpFileInfo,
                       lpBitmapInfo,
                       lpInputBuf, 
                       (LPSTR)((BYTE huge *) lpInputBuf + SMALLBUFOFFSET), 
                       lpRGBQuadPtr, 
                       bIsDither,
                       nNumColors))
        {
            GlobalUnlock (hBhist);
            GLOBALFREE (hBhist);
            return (USER_ABANDON);
        }


        #ifdef NEVER
        /* Get rid of word histogram     */
        GlobalUnlock (hWhist);
        GLOBALFREE (hWhist);
        #endif
    /*
    **  Save the smallhist in a temp file and free up the memory
    */

    if (hHistFile > 0)
        _lclose (hHistFile);

    hHistFile = _lcreat ((LPSTR) HistFile, 0);

    _fmemcpy ((LPSTR) lpFileInfo -> HistFile, (LPSTR) HistFile, 128);




      bytes = _lwrite ((int)hHistFile, (LPSTR)lpSmallhist, (WORD)32768);
      if (((int)bytes == -1) || (bytes != 32768)){
        GlobalUnlock (hBhist);
        GLOBALFREE (hBhist);
        return (EC_PALETTE);
      }
      bytes = _lwrite (hHistFile, (LPSTR)lpRGBQuadPtr, 1024);
      if (((int)bytes == -1) || (bytes != 1024)){
        GlobalUnlock (hBhist);
        GLOBALFREE (hBhist);
        return (EC_PALETTE);
      }

      #ifdef NEVER
      bytes = _lwrite (hHistFile, (LPSTR)OldPaletteRegs, 16);
      if (((int)bytes == -1) || (bytes != 16 )){
        GlobalUnlock (hBhist);
        GLOBALFREE (hBhist);
        return (EC_PALETTE);
      }
      #endif

      bytes = _lwrite (hHistFile, (LPSTR)rm, 256);
      if (((int)bytes == -1) || (bytes != 256)){
        GlobalUnlock (hBhist);
        GLOBALFREE (hBhist);
        return (EC_PALETTE);
      }
      bytes = _lwrite (hHistFile, (LPSTR)gm, 256);
      if (((int)bytes == -1) || (bytes != 256)){
        GlobalUnlock (hBhist);
        GLOBALFREE (hBhist);
        return (EC_PALETTE);
      }
      bytes = _lwrite (hHistFile, (LPSTR)bm, 256);
      if (((int)bytes == -1) || (bytes != 256)){
        GlobalUnlock (hBhist);
        GLOBALFREE (hBhist);
        return (EC_PALETTE);
      }

      _lclose (hHistFile);
      hHistFile = 0;


      if (bGetEPal)
        bGetEPal = FALSE;
      else
        bRestoreDPal = FALSE;
  }
  /*
  **  NOT FIRST PAINT
  */
  else
  {

    /*    Grab small histogram and map info associated with this file    */

    if (hHistFile > 0)
        _lclose (hHistFile);

    hHistFile = _lopen ((LPSTR) HistFile, 0);

    // (No error checking yet.  Is this necessary ?  The file has already been created and closed by now...

    if (!(hBhist = GlobalAlloc (GHND, 32768L)))
      return (EC_MEMORY2);

    if (!(lpSmallhist = (LPSTR)GlobalLock (hBhist)))
    {
      GLOBALFREE (hBhist);
      return (EC_MEMORY1);
    }
    bytes = _lread (hHistFile, lpSmallhist, (WORD)32768);
    if (((int)bytes == -1) || (bytes != 32768)){
      GlobalUnlock (hBhist);
      GLOBALFREE (hBhist);
      return (EC_PALETTE);
    }
    bytes = _lread (hHistFile, (LPSTR)lpRGBQuadPtr, 1024);
    if (((int)bytes == -1) || (bytes != 1024)){
      GlobalUnlock (hBhist);
      GLOBALFREE (hBhist);
      return (EC_PALETTE);
    }

    #ifdef NEVER
    bytes = _lread (hHistFile, (LPSTR)OldPaletteRegs, 16);
    if (((int)bytes == -1) || (bytes != 16)){
      GlobalUnlock (hBhist);
      GLOBALFREE (hBhist);
      return (EC_PALETTE);
    }
    #endif

    bytes = _lread (hHistFile, (LPSTR)rm, 256);
    if (((int)bytes == -1) || (bytes != 256)){
      GlobalUnlock (hBhist);
      GLOBALFREE (hBhist);
      return (EC_PALETTE);
    }
    bytes = _lread (hHistFile, (LPSTR)gm, 256);
    if (((int)bytes == -1) || (bytes != 256)){
      GlobalUnlock (hBhist);
      GLOBALFREE (hBhist);
      return (EC_PALETTE);
    }
    bytes = _lread( hHistFile, (LPSTR)bm, 256 );
    if (((int)bytes == -1) || (bytes != 256)){
      GlobalUnlock (hBhist);
      GLOBALFREE (hBhist);
      return (EC_PALETTE);
    }
    _lclose (hHistFile);
    hHistFile = 0;

  }
  GlobalUnlock (hBhist);
  return (0);
}



int  ReduceRGBToCM (HWND          hWnd,
                    int           hFile,
                    LPFILEINFO    lpFileInfo,
                    LPBITMAPINFO  lpBitmapInfo,
                    LPSTR         lpInputBuf,
                    LPSTR         lpTmpBuf, 
                    RGBQUAD FAR * lpRGBQuadPtr,
                    BOOL          bIsDither,
                    int           nNumColors)
{
  
    /*   Go back out to file and generate histogram data   */
    switch (dPalType)
    {
        case OP256:
            wNumberColors = 254;
        break;

        case OP8:
            wNumberColors = 6;
        break;

        case OP16:
            wNumberColors = 14;
            break;
    }

    if (bIsSaving)
        switch (ePalType)
        {
            case OP256:
                wNumberColors = 254;
            break;

            case OP8:
                wNumberColors = 6;
            break;

            case OP16:
                wNumberColors = 14;
            break;
        }

    if (wPal == IOPT && wDither != IFS16)
            wNumberColors = wOPRColors - 2;



    if (wDither == IFS) 
        bOptDither = TRUE;

    if (! GenerateHistogram (hWnd, 
                             hFile, 
                             lpFileInfo,
                             lpBitmapInfo,
                             lpInputBuf, 
                             lpTmpBuf))
        return (FALSE);  // Currently, the only way we can (* fail *) is w/a 
                         // user-abandon.  Future might also extend to 
                         // disk reads, etc.


  
    if (! GeneratePalette (hWnd, 
                          lpInputBuf, 
                          lpTmpBuf,        // (Use lpTmpBuf for colorbox allocs)
                          lpRGBQuadPtr))
        return (FALSE);

  
    if (bIsDither)
        if (! MapDithRGBSpace (hWnd, lpInputBuf, lpTmpBuf))
            return (FALSE);
        else;
    else
        if (! MapRGBSpace (hWnd, lpInputBuf, lpTmpBuf))
            return (FALSE);

    return (TRUE);
}
  
  
  
  
  
BOOL FAR PASCAL RealizeSystemPalette (hDC, hPalette, lpSysColorIndices, lpSysColorValues, wPaletteUsage)
HDC         hDC;
HPALETTE    hPalette;
LPINT       lpSysColorIndices;
LPDWORD     lpSysColorValues;
WORD        wPaletteUsage;
{
    HPALETTE    hTempPalette;

    SetSystemPaletteUse (hDC, wPaletteUsage);
    UnrealizeObject (hPalette);
    hTempPalette = SelectPalette (hDC, hPalette, 0);
    RealizePalette (hDC);
    SetSysColors (NUMSYSCOLORS, lpSysColorIndices, lpSysColorValues);
    SelectPalette (hDC, hTempPalette, 0);
    SendMessage (-1, WM_SYSCOLORCHANGE, (WORD)NULL, (LONG)NULL);
    return (TRUE);
}


















