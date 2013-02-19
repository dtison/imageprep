
#include <windows.h>
#include <imgprep.h>
#include <proto.h>
#include <error.h>
#include <global.h>
#include <filters.h>

/*---------------------------------------------------------------------------

   PROCEDURE:         SetupFilters
   DESCRIPTION:       Setup filter structure array
   DEFINITION:         
   START:             1/9/90  TBJ
   MODS:              8/90    NO HANDLES  D. Ison     (Just say no!)

----------------------------------------------------------------------------*/

int SetupFilters (lpFileInfo, state)
LPFILEINFO    lpFileInfo;
int           state;
{
  HANDLE      hErrBuf;       /* temp holder of buffer handle for filters */
  int         i;                                        /* loop variable */

  LPDISPINFO          lpDispInfo;

  /*  Initialize Import Filter Structs   */

  bFiltersActive = TRUE;

  if (state == IMPORT_CLASS){
    for (i = 0; i < 5; i++){
      switch (ImportFilters[i]){
        case 0: 
          /*
          **  Case 0 signifies either no filter or LITERAL translation, 
          **  init struct = 0 ptrs
          */
          ImportFilterStruct[i][0] = (HANDLE)0;
          ImportFilterStruct[i][1] = (HANDLE)0;
          break;
        case   REDU:      
          /*
          **  This Case = Reduction Process 
          **  The only mem resources this filter needs initialized is the
          **  filter parameter structure itself ( obtained from GetREDU )
          */
          ImportFilterStruct[i][0] = GetREDU (lpFileInfo, FALSE);
          ImportFilterStruct[i][1] = GetREDU (lpFileInfo, TRUE);
          break;
        case   FS:      

        {

          WORD wNumBytes;  
          WORD wRowsPerStrip;

          lpDispInfo  = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);

          /* This is a patch to prevent UAE's when dithering 1 row/strip images.  
             Because the errbuf_len in QUANTIZE.C is always for one line, if
             wRowsPerStrip is also 1, there is no room for fwd and current error buffers.

             D. Ison  */

          wRowsPerStrip = lpFileInfo -> wRowsPerStrip;
          if (wRowsPerStrip == 1)
            wRowsPerStrip = 2;      

          //  "1" is FS, "3" is RGB planes, "2" is Curr and fwd err lists

          wNumBytes = (lpDispInfo -> wPaddedScanWidth + 2) * 3 * wRowsPerStrip;


          if (! (hErrBuf = GlobalAlloc (GHND, (DWORD) wNumBytes)))
              return (EC_NOMEM);

        }

          ImportFilterStruct[i][0] = GetFS (lpFileInfo, FALSE, hErrBuf);
          ImportFilterStruct[i][1] = GetFS (lpFileInfo, TRUE, hErrBuf);
          break;
        case   BAY8:   
          /*
          **  This Case = Bayer Dither Process
          **  The mem resources this filter needs initialized is the
          **  filter parameter structure itself (obtained from GetBAY8)
          */
          ImportFilterStruct[i][0] = GetBAY8 (lpFileInfo, FALSE, FALSE);
          ImportFilterStruct[i][1] = GetBAY8 (lpFileInfo, TRUE, FALSE);
          break;
        case   UQUA:   
          /*
          **  This Case = Uniform Quantize Process
          **  The mem resources this filter needs initialized is the
          **  filter parameter structure itself ( obtained from GetUQUA )
          */
          ImportFilterStruct[i][0] = GetUQUA (lpFileInfo, FALSE);
          ImportFilterStruct[i][1] = GetUQUA (lpFileInfo, TRUE);
          break;
        case   UQUA8:   
          /*
          **  This Case = Uniform Quantize 8 Color Process
          **  The mem resources this filter needs initialized is the
          **  filter parameter structure itself ( obtained from GetUQUA8 )
          */
          ImportFilterStruct[i][0] = GetUQUA8 (lpFileInfo, FALSE);
          ImportFilterStruct[i][1] = GetUQUA8 (lpFileInfo, TRUE);
          break;
        case   GSUM:   
          /*
          **  This Case = GraySum 24 bit to Gray Process
          **  The mem resources this filter needs initialized is the
          **  filter parameter structure itself ( obtained from GetGSum )
          */
          ImportFilterStruct[i][0] = GetGSUM (lpFileInfo, FALSE);
          ImportFilterStruct[i][1] = GetGSUM (lpFileInfo, TRUE);
          break;
        case   LEVL:   
          /*
          **  This Case = Level Shift for Gray Process
          **  The mem resources this filter needs initialized is the
          **  filter parameter structure itself ( obtained from GetLEVL )
          */
          ImportFilterStruct[i][0] = GetLEVL (lpFileInfo, FALSE);
          ImportFilterStruct[i][1] = GetLEVL (lpFileInfo, TRUE);
          break;
        case   DIDX:   
          /*
          **  This Case = De-Index 8 bit to 24 bit Process
          **  The mem resources this filter needs initialized is the
          **  filter parameter structure itself ( obtained from GetDIDX )
          */
          ImportFilterStruct[i][0] = GetDIDX (lpFileInfo, FALSE);
          ImportFilterStruct[i][1] = GetDIDX (lpFileInfo, TRUE);
          break;
        case   G2BW:   
          /*
          **  This Case = Gray to Black & White Process
          **  The mem resources this filter needs initialized is the
          **  filter parameter structure itself ( obtained from GetG2BW )
          */
          ImportFilterStruct[i][0] = GetG2BW (lpFileInfo, FALSE);
          ImportFilterStruct[i][1] = GetG2BW (lpFileInfo, TRUE);
          break;
        case   GBAY:   
          /*
          **  This Case = Gray Bayer Dither to BW Process
          **  The mem resources this filter needs initialized is the
          **  filter parameter structure itself ( obtained from GetGBAY )
          */
          ImportFilterStruct[i][0] = GetGBAY (lpFileInfo, FALSE);
          ImportFilterStruct[i][1] = GetGBAY (lpFileInfo, TRUE);
          break;
        case   GFS:      
          /*
          **  This Case = Gray Floyd Steinberg Dither to BW Process
          **  The mem resources this filter needs initialized is the
          **  filter parameter structure itself ( obtained from GetGFS )
          */
          if (!(hErrBuf = GlobalAlloc (GHND, 
            (DWORD)(lpFileInfo->wPaddedBytesPerRow *  6 *
            lpFileInfo->wRowsPerStrip)))){
            return (EC_NOMEM);
          }
          ImportFilterStruct[i][0] = GetDGFS (lpFileInfo, FALSE, hErrBuf);
          ImportFilterStruct[i][1] = GetDGFS (lpFileInfo, TRUE, hErrBuf);
          break;
      }
    }
  }

  /*  Initialize Export Filter Structs   */

  if (state == EXPORT_CLASS)
  {
    for (i = 0; i < 5; i++)
    {
      switch (ExportFilters[i])
      {
        case 0:   
          /*
          **  Case 0 signifies either no filter or 
          **  LITERAL translation, init struct = 0 ptrs
          */
          ExportFilterStruct[i][0] = (HANDLE)0;
          ExportFilterStruct[i][1] = (HANDLE)0;
          break;
        case   REDU:   
          /*
          **  This Case = Reduction Process
          **  The only mem resources this filter needs initialized is the
          **  filter parameter structure itself ( obtained from GetREDU )
          */
          ExportFilterStruct[i][0] = GetREDU (lpFileInfo, FALSE);
          ExportFilterStruct[i][1] = GetREDU (lpFileInfo, TRUE);
          break;
        case   FS:      
        {

          WORD wNumBytes;  
          WORD wRowsPerStrip;

          lpDispInfo  = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);
          /* This is a patch.  See note in FS of IMPORT switch.. */
          wRowsPerStrip = lpFileInfo -> wRowsPerStrip;
          if (wRowsPerStrip == 1)
            wRowsPerStrip = 2;      
          //  "1" is FS, "3" is RGB planes, "2" is Curr and fwd err lists
          wNumBytes = (lpDispInfo -> wPaddedScanWidth + 2) * 3 * wRowsPerStrip;
          if (! (hErrBuf = GlobalAlloc (GHND, (DWORD) wNumBytes)))
              return (EC_NOMEM);
        }
          ExportFilterStruct[i][0] = GetFS (lpFileInfo, FALSE, hErrBuf);
          ExportFilterStruct[i][1] = GetFS (lpFileInfo, TRUE, hErrBuf);
          break;
        case   BAY8:   
          /*
          **  This Case = Bayer Dither Process
          **  The mem resources this filter needs initialized is the
          **  filter parameter structure itself (obtained from GetBAY8)
          */
          ExportFilterStruct[i][0] = GetBAY8 (lpFileInfo, FALSE, TRUE);
          ExportFilterStruct[i][1] = GetBAY8 (lpFileInfo, TRUE, TRUE);
          break;
        case   UQUA:   
          /*
          **  This Case = Uniform Quantize Process
          **  The mem resources this filter needs initialized is the
          **  filter parameter structure itself ( obtained from GetUQUA )
          */
          ExportFilterStruct[i][0] = GetUQUA (lpFileInfo, FALSE);
          ExportFilterStruct[i][1] = GetUQUA (lpFileInfo, TRUE);
          break;
        case   UQUA8:   
          /*
          **  This Case = Uniform Quantize 8 Color Process
          **  The mem resources this filter needs initialized is the
          **  filter parameter structure itself ( obtained from GetUQUA8 )
          */
          ExportFilterStruct[i][0] = GetUQUA8 (lpFileInfo, FALSE);
          ExportFilterStruct[i][1] = GetUQUA8 (lpFileInfo, TRUE);
          break;
        case   GSUM:   
          /*
          **  This Case = GraySum 24 bit to Gray Process
          **  The mem resources this filter needs initialized is the
          **  filter parameter structure itself ( obtained from GetGSum )
          */
          ExportFilterStruct[i][0] = GetGSUM (lpFileInfo, FALSE);
          ExportFilterStruct[i][1] = GetGSUM (lpFileInfo, TRUE);
          break;
        case   LEVL:   
          /*
          **  This Case = Level Shift for Gray Process
          **  The mem resources this filter needs initialized is the
          **  filter parameter structure itself ( obtained from GetLEVL )
          */
          ExportFilterStruct[i][0] = GetLEVL (lpFileInfo, FALSE);
          ExportFilterStruct[i][1] = GetLEVL (lpFileInfo, TRUE);
          break;
        case   DIDX:   
          /*
          **  This Case = De-Index 8 bit to 24 bit Process
          **  The mem resources this filter needs initialized is the
          **  filter parameter structure itself ( obtained from GetDIDX )
          */
          ExportFilterStruct[i][0] = GetDIDX (lpFileInfo, FALSE);
          ExportFilterStruct[i][1] = GetDIDX (lpFileInfo, TRUE);
          break;
        case   G2BW:   
          /*
          **  This Case = Gray to Black & White Process
          **  The mem resources this filter needs initialized is the
          **  filter parameter structure itself ( obtained from GetG2BW )
          */
          ExportFilterStruct[i][0] = GetG2BW (lpFileInfo, FALSE);
          ExportFilterStruct[i][1] = GetG2BW (lpFileInfo, TRUE);
          break;
        case   GBAY:   
          /*
          **  This Case = Gray Bayer Dither to BW Process
          **  The mem resources this filter needs initialized is the
          **  filter parameter structure itself ( obtained from GetGBAY )
          */
          ExportFilterStruct[i][0] = GetGBAY (lpFileInfo, FALSE);
          ExportFilterStruct[i][1] = GetGBAY (lpFileInfo, TRUE);
          break;
        case   GFS:      
          /*
          **  This Case = Gray Floyd Steinberg Dither to BW Process
          **  The mem resources this filter needs initialized is the
          **  filter parameter structure itself ( obtained from GetGFS )
          */
          if (!(hErrBuf = GlobalAlloc (GHND, 
            (DWORD)(lpFileInfo->wPaddedBytesPerRow *    6 *
            lpFileInfo->wRowsPerStrip)))){
            return (EC_NOMEM);
          }
          ExportFilterStruct[i][0] = GetDGFS (lpFileInfo, FALSE, hErrBuf);
          ExportFilterStruct[i][1] = GetDGFS (lpFileInfo, TRUE, hErrBuf);
          break;
      }
    }
  }

  return (0);
}

/*----------------------------------------------------------------------------

   PROCEDURE:        GetBAY8
   DESCRIPTION:      Alloc structs & init for BAY8 filter (DitherBayer_A)
   DEFINITION:         
   START:            1/10/90  Tom Bagford Jr.   
   MODS:             4/23/90  3.0 TBjr
                     7/10/90  Corrected "wPaddedScanWidth" mess  D. Ison

-----------------------------------------------------------------------------*/

HANDLE GetBAY8 (lpFileInfo, bIsLast, bExport)
LPFILEINFO  lpFileInfo;
BOOL        bIsLast;
BOOL        bExport;
{
    HANDLE              hTmp;
    DITHERBAYER FAR *   lpF;

    LPDISPINFO          lpDispInfo;
    
    lpDispInfo  = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);


    hTmp  = GlobalAlloc (GHND, (DWORD) sizeof (DITHERBAYER)); 
    lpF   = (DITHERBAYER FAR *) GlobalLock (hTmp);

    if (bIsLast)
        lpF->wRowsThisStrip = lpFileInfo->wRowsPerLastStrip;
    else
        lpF->wRowsThisStrip = lpFileInfo->wRowsPerStrip;

    if (wImportClass == IRGB)
    {
        lpF -> wInputBytesPerRow  = lpFileInfo -> wPaddedBytesPerRow;
        lpF -> wPaddedScanWidth   = lpDispInfo -> wPaddedScanWidth;
    }
    else  // Colormap being deindexed
    {
        lpF -> wInputBytesPerRow  = lpDispInfo -> wPaddedScanWidth * 3;
        lpF -> wPaddedScanWidth   = lpDispInfo -> wPaddedScanWidth;  // I do believe...
    }

    lpF->wScrnY = 0;
    lpF->wColors = ((wDisplay == IVGA && ! bExport) ? 8 : 256);

    GlobalUnlock (hTmp);
    return (hTmp);
}


/*---------------------------------------------------------------------------

   PROCEDURE:        GetUQUA8
   DESCRIPTION:      Alloc structs & init for UQUA8 filter ( UniformQuant8_A )
   DEFINITION:         
   START:            1/9/90  Tom Bagford Jr.   
   MODS:             7/10/90  Corrected "wPaddedScanWidth" mess  D. Ison


----------------------------------------------------------------------------*/

HANDLE GetUQUA8 (lpFileInfo, bIsLast)
LPFILEINFO  lpFileInfo;
BOOL        bIsLast;
{
    HANDLE            hTmp;
    UNIFORMQUANT FAR *    lpF;
    LPDISPINFO          lpDispInfo;
    
    lpDispInfo  = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);
    
    hTmp  = GlobalAlloc (GHND, (DWORD) sizeof (UNIFORMQUANT)); 
    lpF   = (UNIFORMQUANT FAR *) GlobalLock (hTmp);

    if (bIsLast)
        lpF->wRowsThisStrip = lpFileInfo->wRowsPerLastStrip;
    else
        lpF->wRowsThisStrip   = lpFileInfo->wRowsPerStrip;

    switch (wImportClass)
    {
        case IRGB:
        {
            lpF -> wInputBytesPerRow  = lpFileInfo -> wPaddedBytesPerRow;
            lpF -> wPaddedScanWidth   = lpDispInfo -> wPaddedScanWidth;
        }
        break;

        default:
        {
            lpF -> wInputBytesPerRow  = lpDispInfo -> wPaddedScanWidth * 3;
            lpF -> wPaddedScanWidth   = lpDispInfo -> wPaddedScanWidth;
        }
        break;
    }

    GlobalUnlock (hTmp);
    return (hTmp);
}

/*--------------------------------------------------------------------------     

   PROCEDURE:        GetUQUA
   DESCRIPTION:      Alloc structs & init for UQUA filter (UniformQuant_A)
   DEFINITION:         
   MODS:             7/10/90  Corrected "wPaddedScanWidth" mess  D. Ison

-----------------------------------------------------------------------------*/

HANDLE GetUQUA (lpFileInfo, bIsLast)
LPFILEINFO  lpFileInfo;
BOOL        bIsLast;
{
    HANDLE            hTmp;
    UNIFORMQUANT FAR *    lpF;
    LPDISPINFO          lpDispInfo;
    
    lpDispInfo  = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);

    hTmp  = GlobalAlloc (GHND, (DWORD) sizeof (UNIFORMQUANT)); 
    lpF   = (UNIFORMQUANT FAR *) GlobalLock (hTmp);

    if (bIsLast)
        lpF->wRowsThisStrip = lpFileInfo->wRowsPerLastStrip;
    else
        lpF->wRowsThisStrip = lpFileInfo->wRowsPerStrip;


    /*  wImportClass will ALWAYS be RGB for this function to have been used,
        therefore these will always be the case  */

    {
        lpF -> wInputBytesPerRow  = lpFileInfo -> wPaddedBytesPerRow;
        lpF -> wPaddedScanWidth   = lpDispInfo -> wPaddedScanWidth;
    }

    GlobalUnlock (hTmp);
    return (hTmp);
}


/*---------------------------------------------------------------------------

   PROCEDURE:        GetREDU
   DESCRIPTION:      Alloc structs & init for REDU filter (  )
   DEFINITION:         
   START:            1/9/90  Tom Bagford Jr.   
   MODS:             7/10/90  Corrected "wPaddedScanWidth" mess  D. Ison

-----------------------------------------------------------------------------*/

HANDLE GetREDU (lpFileInfo, bIsLast)
LPFILEINFO  lpFileInfo;
BOOL        bIsLast;
{
    HANDLE            hTmp;
    QUANTOPR FAR *    lpF;
    LPDISPINFO          lpDispInfo;
    
    lpDispInfo  = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);

    hTmp  = GlobalAlloc (GHND, (DWORD) sizeof (QUANTOPR)); 
    lpF   = (QUANTOPR FAR *) GlobalLock (hTmp);

    if (bIsLast)
        lpF->wRowsThisStrip = lpFileInfo->wRowsPerLastStrip;
    else
        lpF->wRowsThisStrip = lpFileInfo->wRowsPerStrip;


            {
                lpF -> wInputBytesPerRow = lpFileInfo -> wPaddedBytesPerRow;
                lpF -> wPaddedScanWidth  = lpDispInfo -> wPaddedScanWidth;
            }


/*  switch (wImportClass)
    {
        case IRGB:
            if ((wImportClass == wExportClass) && (bIsSaving))
                lpF->wInputBytesPerRow = lpFileInfo->wPaddedBytesPerRow;
            else
            {   
                if (bIsSaving)
                    lpF->wInputBytesPerRow = lpFileInfo->wPaddedBytesPerRow * 3;
                else
                {
                    lpF -> wInputBytesPerRow = lpFileInfo -> wPaddedBytesPerRow;
                    lpF -> wPaddedScanWidth  = lpDispInfo -> wPaddedScanWidth;
                }
            }
            break;
        default:
            lpF->wInputBytesPerRow = lpFileInfo->wPaddedBytesPerRow * 3;
          break;
    } */


    GlobalUnlock (hTmp);
    return (hTmp);
}

/*-------------------------------------------------------------------------- 

   PROCEDURE:        GetFS
   DESCRIPTION:      Alloc structs & init for FS filter (QUANTDITHERFS)
   DEFINITION:         
   START:            1/9/90  Tom Bagford Jr.   
   MODS:             7/10/90  Corrected "wPaddedScanWidth" mess  D. Ison

----------------------------------------------------------------------------*/

HANDLE GetFS (lpFileInfo, bIsLast, hErrBuf)
LPFILEINFO  lpFileInfo;
BOOL        bIsLast;
HANDLE      hErrBuf;
{
    HANDLE                  hTmp;
    QUANTFSDITHER FAR *     lpF;

    LPDISPINFO          lpDispInfo;
    
    lpDispInfo  = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);

    hTmp  = GlobalAlloc (GHND, (DWORD) sizeof (QUANTFSDITHER)); 
    lpF   = (QUANTFSDITHER FAR *) GlobalLock (hTmp);

    if (bIsLast)
        lpF->wRowsThisStrip = lpFileInfo->wRowsPerLastStrip;
    else
        lpF->wRowsThisStrip = lpFileInfo->wRowsPerStrip;

    {
 
        {
            if (wImportClass == ICM)
            {
                lpF -> wInputBytesPerRow = lpFileInfo -> wPaddedBytesPerRow * 3;
                lpF -> wPaddedScanWidth  = lpDispInfo -> wPaddedScanWidth;
            }
            else  // RGB   
            {
                lpF -> wInputBytesPerRow = lpFileInfo -> wPaddedBytesPerRow;
                lpF -> wPaddedScanWidth  = lpDispInfo -> wPaddedScanWidth;
            }
        }
    }
    lpF -> hErrBuf = hErrBuf;
    GlobalUnlock (hTmp);
    return (hTmp);
}


/*---------------------------------------------------------------------------

   PROCEDURE:        GetLEVL
   DESCRIPTION:      Alloc structs & init for LEVEL filter (ShiftGrayLevels)
   DEFINITION:         
   START:            1/9/90  Tom Bagford Jr.   
   MODS:             7/11/90  Corrected "wPaddedScanWidth" mess  D. Ison

   NOTES:            The input data must be 8bit gray code
                           
----------------------------------------------------------------------------*/

HANDLE GetLEVL (lpFileInfo, bIsLast)
LPFILEINFO  lpFileInfo;
BOOL        bIsLast;
{
    HANDLE             hTmp;
    SHIFTGRAY FAR *    lpF;
    LPDISPINFO          lpDispInfo;
    WORD    wRowsThisStrip;


    hTmp  = GlobalAlloc (GHND, (long)sizeof(SHIFTGRAY)); 
    lpF   = (SHIFTGRAY FAR *)GlobalLock (hTmp);


    lpDispInfo  = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);

    wRowsThisStrip = ((bIsLast) ? lpFileInfo -> wRowsPerLastStrip : lpFileInfo -> wRowsPerStrip);


    lpF -> wBytesThisStrip = lpDispInfo -> wPaddedScanWidth * wRowsThisStrip;


    switch (dPalType)
    {
        case GP256:
            lpF->wBitsToShift    = 0;
        break;

        case GP64:
//          lpF->wBitsToShift    = 2;
            lpF->wBitsToShift    = 0;
        break;
  
        case GP16:
            lpF->wBitsToShift    = 4;
        break;
  
//      case GP8:
//          lpF->wBitsToShift    = 5;  // OK D. Ison  Looks bad because we can't set the palette yet.  7-90
//          lpF->wBitsToShift    = 0;
//      break;
 
    }
    GlobalUnlock (hTmp);
    return (hTmp);
}


/*---------------------------------------------------------------------------

   PROCEDURE:        GetG2BW
   DESCRIPTION:      Alloc structs & init for G2BW filter (GrayToBW)
   DEFINITION:         
   START:            1/9/90  Tom Bagford Jr.   
   MODS:             7/11/90  Corrected "wPaddedScanWidth" mess  D. Ison

----------------------------------------------------------------------------*/

HANDLE GetG2BW (lpFileInfo, bIsLast)
LPFILEINFO    lpFileInfo;
BOOL          bIsLast;
{
    HANDLE            hTmp;
    GRAYTOBW  FAR *   lpF;
    LPDISPINFO          lpDispInfo;
    WORD    wRowsThisStrip;

    hTmp  = GlobalAlloc (GHND, (DWORD) sizeof (GRAYTOBW));
    lpF = (GRAYTOBW FAR *) GlobalLock (hTmp);

    lpDispInfo  = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);

    wRowsThisStrip = ((bIsLast) ? lpFileInfo -> wRowsPerLastStrip : lpFileInfo -> wRowsPerStrip);

    lpF -> wBytesThisStrip = lpDispInfo -> wPaddedScanWidth * wRowsThisStrip;


    GlobalUnlock (hTmp);
    return (hTmp);
}



/*---------------------------------------------------------------------------

   PROCEDURE:        GetGSUM
   DESCRIPTION:      Alloc structs & init for GSUM filter (GraySum)
   DEFINITION:         
   START:            1/9/90  Tom Bagford Jr.   
   MODS:             7/10/90 "PaddedScanwidth" correction  D. Ison

   NOTES:            24 bit data coming in, 8 bit going out

-----------------------------------------------------------------------------*/

HANDLE GetGSUM (lpFileInfo, bIsLast)
LPFILEINFO    lpFileInfo;
BOOL          bIsLast;
{
    HANDLE               hTmp;
    RGBTOGRAY FAR *      lpF;
    LPDISPINFO          lpDispInfo;

    lpDispInfo  = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);

    hTmp  = GlobalAlloc (GHND, (DWORD) sizeof(RGBTOGRAY)); 
    lpF   = (RGBTOGRAY FAR *) GlobalLock (hTmp);


    if (bIsLast)
        lpF->wRowsThisStrip = lpFileInfo->wRowsPerLastStrip;
    else
        lpF->wRowsThisStrip = lpFileInfo->wRowsPerStrip;

    if (wImportClass == IRGB)
    {
        lpF -> wInputBytesPerRow  = lpFileInfo -> wPaddedBytesPerRow;
        lpF -> wPaddedScanWidth   = lpDispInfo -> wPaddedScanWidth;
    }
    else  // Colormap being deindexed
    {
        lpF -> wInputBytesPerRow  = lpDispInfo -> wPaddedScanWidth * 3;
        lpF -> wPaddedScanWidth   = lpDispInfo -> wPaddedScanWidth;  // I do believe...
    }

    GlobalUnlock (hTmp);
    return (hTmp);

}

/*----------------------------------------------------------------------------

   PROCEDURE:        GetDIDX
   DESCRIPTION:      Alloc structs & init for DIDX filter ( Fmt8To24 )
   DEFINITION:         
   START:            1/9/90  Tom Bagford Jr.   
   MODS:    Ison fixed the PaddedScanWidth mess! 


   Only a CM could be DIDXed !

-----------------------------------------------------------------------------*/

HANDLE GetDIDX (lpFileInfo, bIsLast)
LPFILEINFO       lpFileInfo;
BOOL             bIsLast;
{
    HANDLE            hTmp;
    FMT8TO24 FAR *    lpF;
//  HANDLE              hPalette;

    hTmp  = GlobalAlloc (GHND, (long)sizeof(FMT8TO24)); 
    lpF   = (FMT8TO24 FAR *)GlobalLock (hTmp);

    #ifdef NOTYET
    if (! (hPalette = GlobalAlloc (GHND, (DWORD) 1024)))
        return (EC_NOMEM);

    lpF->lpPalette = GlobalLock (hPalette);
    lpF->hPalette = hPalette;
    #else
//  lpF->lpPalette = NULL;
    lpF->lpPalette = GlobalLock (hGlobalPalette);

    #endif

    {
        if (bIsLast)
            lpF->wBytesThisStrip = lpFileInfo->wPaddedBytesPerRow *
                                   lpFileInfo->wRowsPerLastStrip;
        else
            lpF->wBytesThisStrip = lpFileInfo->wPaddedBytesPerRow *
                                   lpFileInfo->wRowsPerStrip;
    }
    GlobalUnlock (hTmp);
    return (hTmp);
}

/*---------------------------------------------------------------------------

   PROCEDURE:        GetDGFS
   DESCRIPTION:      Alloc structs & init for DGFS filter (QUANTDITHERFS)
   DEFINITION:         
   START:            1/9/90  Tom Bagford Jr.   
   MODS:             7/11/90  Corrected "wPaddedScanWidth" mess  D. Ison

----------------------------------------------------------------------------*/

HANDLE GetDGFS (lpFileInfo , bIsLast, hErrBuf)
LPFILEINFO  lpFileInfo;
BOOL        bIsLast;
HANDLE      hErrBuf;
{
    HANDLE                  hTmp;
    QUANTFSDITHER FAR *     lpF;

    LPDISPINFO          lpDispInfo;
    
    lpDispInfo  = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);

    hTmp  = GlobalAlloc (GHND, (DWORD) sizeof (QUANTFSDITHER)); 
    lpF   = (QUANTFSDITHER FAR *) GlobalLock (hTmp);

    if (bIsLast)
        lpF->wRowsThisStrip = lpFileInfo->wRowsPerLastStrip;
    else
        lpF->wRowsThisStrip = lpFileInfo->wRowsPerStrip;

    lpF -> wInputBytesPerRow = lpDispInfo -> wPaddedScanWidth;
    lpF -> wPaddedScanWidth  = lpDispInfo -> wPaddedScanWidth;

    lpF -> hErrBuf = hErrBuf;
    GlobalUnlock (hTmp);
    return (hTmp);
}

/*----------------------------------------------------------------------------

   PROCEDURE:        GetGBAY
   DESCRIPTION:      Alloc structs & init for GBAY filter ( DitherBayer_A )
   DEFINITION:         
   START:            1/10/90  Tom Bagford Jr.   
   MODS:             7/11/90  Corrected "wPaddedScanWidth" mess  D. Ison

-----------------------------------------------------------------------------*/

HANDLE GetGBAY (lpFileInfo, bIsLast)
LPFILEINFO  lpFileInfo;
BOOL        bIsLast;
{
    HANDLE              hTmp;
    DITHERBAYER FAR *   lpF;

    LPDISPINFO          lpDispInfo;
    
    lpDispInfo  = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);


    hTmp  = GlobalAlloc (GHND, (DWORD) sizeof (DITHERBAYER)); 
    lpF   = (DITHERBAYER FAR *) GlobalLock (hTmp);

    if (bIsLast)
        lpF->wRowsThisStrip = lpFileInfo->wRowsPerLastStrip;
    else
        lpF->wRowsThisStrip = lpFileInfo->wRowsPerStrip;


    {
        lpF -> wInputBytesPerRow  = lpDispInfo -> wPaddedBytesPerRow;
        lpF -> wPaddedScanWidth   = lpDispInfo -> wPaddedScanWidth;
    }

    lpF->wScrnY = 0;


    GlobalUnlock (hTmp);
    return (hTmp);
}


int FreeFilters (hFInfo, nState)
HANDLE  hFInfo;
int     nState;
{
  LPFILEINFO          lpFileInfo;
  DITHERBAYER FAR *   lpDB;
  QUANTFSDITHER FAR * lpFSD;
  FMT8TO24 FAR *      lpFmt8To24;
  int                 i;

  lpFileInfo = (LPFILEINFO)GlobalLock (hFInfo);
  switch (nState)
  {
    case IMPORT_CLASS:
        bFiltersActive = FALSE;
        if (! bFiltersActive)  // Save a UAE
        {
            return (0);
        }
      lpFileInfo->bIsLastStrip = FALSE;
      for (i = 0; i < 5; i++){
        switch (ImportFilters[i]){
          case FS:
            lpFSD = (QUANTFSDITHER FAR *)GlobalLock (ImportFilterStruct[i][0]);

            // This test necessary on optimized err-dist dither settings because palette
            // generation might have been user-abandoned but the filters were never set.  D. Ison  9/90

            if (lpFSD != NULL) 
                GLOBALFREE (lpFSD->hErrBuf);
            break;

          case BAY8:
            lpDB = (DITHERBAYER FAR *)GlobalLock (ImportFilterStruct[i][0]);
            lpDB->wScrnY = 0;
            GlobalUnlock (ImportFilterStruct[i][0]);
            lpDB = (DITHERBAYER FAR *)GlobalLock (ImportFilterStruct[i][1]);
            lpDB->wScrnY = 0;
            GlobalUnlock (ImportFilterStruct[i][0]);
            break;
        
          case DIDX:
            lpFmt8To24 = (FMT8TO24 FAR *)GlobalLock (ImportFilterStruct[i][0]);

            if (lpFmt8To24 != NULL) 
                GlobalUnlock (hGlobalPalette);

            break;
        

        }
        GLOBALFREE (ImportFilterStruct[i][0]);
        GLOBALFREE (ImportFilterStruct[i][1]);
      }
      break;
    case EXPORT_CLASS:
      for (i = 0; i < 5; i++){
        switch (ExportFilters[i]){
          case FS:
            lpFSD = (QUANTFSDITHER FAR *)GlobalLock (ExportFilterStruct[i][0]);

            if (lpFSD != NULL) 
            {
                int i;
                WORD wLockCount;

                wLockCount = (GlobalFlags (lpFSD->hErrBuf) & GMEM_LOCKCOUNT);
                for (i = 0; i < (int) wLockCount; i++)
                    GlobalUnlock (lpFSD->hErrBuf);
    
                GLOBALFREE (lpFSD->hErrBuf);
            }
            break;
          case BAY8:
            lpDB = (DITHERBAYER FAR *)GlobalLock (ExportFilterStruct[i][0]);
            lpDB->wScrnY = 0;
            GlobalUnlock (ExportFilterStruct[i][0]);
            lpDB = (DITHERBAYER FAR *)GlobalLock (ExportFilterStruct[i][1]);
            lpDB->wScrnY = 0;
            break;
        }
        GLOBALFREE (ExportFilterStruct[i][0]);
        GLOBALFREE (ExportFilterStruct[i][1]);
      }
      break;
  }
  GlobalUnlock (hFInfo);
  return (0);
}


