#include <windows.h>
#include <imgprep.h>
#include <proto.h>
#include <global.h>
#include <error.h>
#include <reduce.h>
#include <stdio.h>
#include <memory.h>
#include <cpi.h>
#include <prometer.h>
#include <strtable.h>


WORD                wNumReductionColors;
LPCOLORBOX          lpGlobalColorBoxes;
WORD                wGlobalBoxCount;
HANDLE              hSmallhist;
long                lHistIndex;
LPCOLORBOX          lpFreeColorBoxes;
LPCOLORBOX          lpUsedColorBoxes;
LPCOLORCELL FAR *   lplpColorCells;
LPCOLORCELL         lpGlobalColorCells;
WORD                wColorCellCount;
int                 nFreeBoxCount=0;
int                 Dither;

LPCOLORCELL NEAR PASCAL CreateColorCell (DWORD);
LPCOLORBOX  NEAR PASCAL GetLargestColorBox(void);

int     NEAR PASCAL MapColorTable(void);
int     NEAR PASCAL MapDitherColorTable(void);
void    NEAR PASCAL SplitColorBox(LPCOLORBOX);
void    NEAR PASCAL ShrinkColorBox(LPCOLORBOX);
int     NEAR PASCAL AssignColor (LPCOLORBOX, LPBYTE, LPBYTE, LPBYTE, WORD);
DWORD   NEAR PASCAL GetColorBoxMedian (LPCOLORBOX);

DWORD FAR  PASCAL   GetColorDistance (DWORD, DWORD);


int GenerateHistogram (hWnd, hFile, lpFileInfo, lpBitmapInfo, lpInputBuf, lpTmpBuf)
HWND          hWnd;
int           hFile;
LPFILEINFO    lpFileInfo;
LPBITMAPINFO  lpBitmapInfo;
LPSTR         lpInputBuf;
LPSTR         lpTmpBuf;
{
  int (FAR PASCAL *lpfnRdConvertData)(int, LPSTR FAR *, LPSTR FAR *, LPFILEINFO, LPBITMAPINFO) = 0;

  HDC         hDC;
  int         i;
  int         nRedMin;
  int         nRedMax;
  int         nGreenMin;
  int         nGreenMax;
  int         nBlueMin;
  int         nBlueMax;

  WORD        wRowsPerStrip;
  WORD        wBytesPerStrip;
  WORD        wRowsPerLastStrip;
  WORD        wBytesPerLastStrip;
  WORD        wNumStrips;
  LPCOLORBOX  lpColorbox; 
  LPSTR       lpDest;
  LPSTR       lpSource;
  WORD        wPaddedBytesPerRow;   // 24 bit pixels * 3 (DIB aligned)

  PSTR        pStringBuf;
  PSTR        pStringBuf2;
  /*
  **  Set internal number of colors to reduce to
  */
  wNumberColors  += 2;
  wNumReductionColors = wNumberColors;
  pStringBuf          = LocalLock (hStringBuf);
  pStringBuf2         = pStringBuf + MAXSTRINGSIZE;

  hDC = GetDC (hWnd);
  /*
  **  WAS: Get pointers from handles and allocate memory used by this window
  */
  lpfnRdConvertData = GetRCD (wImportType);
  /*
  **  Setup the Colorbox data structure. Put COLORBOX in lpInputBuf
  */
  lpColorbox = (LPCOLORBOX)lpInputBuf;
  /*
  **  Set these
  */
  nRedMin = nGreenMin = nBlueMin = 999;       /* 0xFFFF; */
  nRedMax = nGreenMax = nBlueMax = -1;        /* 0x0000; */

  _fmemset ((LPSTR)lpwHist, 0, (size_t)65535);
  _fmemset ((LPSTR)lpbHist, 0, (size_t)32768);
  /*
  **  Do the Histogram generation from DIB format
  */
  wNumStrips          = lpFileInfo->wNumStrips;
  wBytesPerStrip      = lpFileInfo->wBytesPerStrip;
  wRowsPerStrip       = lpFileInfo->wRowsPerStrip;
  wBytesPerLastStrip  = lpFileInfo->wBytesPerLastStrip;
  wRowsPerLastStrip   = lpFileInfo->wRowsPerLastStrip;
  wPaddedBytesPerRow  = ((((lpFileInfo->wScanWidth * 24) + 31) / 32) * 4);
      
  _llseek (hFile, lpFileInfo->dwDataOffset, 0);

  lpFileInfo->bIsLastStrip = FALSE;
  /*
  **  Progress meter to watch..
  */
#ifndef NOPRO
  EnableWindow (hWndIP, FALSE);
  LoadString (hInstIP, STR_PALETTE_OPTIMIZATION, (LPSTR) pStringBuf, MAXSTRINGSIZE);
  ProOpen (hWndIP, NULL, MakeProcInstance (Abandon, hInstIP), (LPSTR) pStringBuf);
  ProSetBarRange (wNumStrips);

  LoadString (hInstIP, STR_OPT_UCOLORS, (LPSTR) pStringBuf, MAXSTRINGSIZE);
  wsprintf (pStringBuf2, pStringBuf, (wNumReductionColors));
  ProSetText (ID_STATUS1, (LPSTR) pStringBuf2);

  LoadString (hInstIP, STR_GEN_HISTOGRAM, (LPSTR) pStringBuf, MAXSTRINGSIZE);
  ProSetText (ID_STATUS2, (LPSTR) pStringBuf);
#endif

  bAbandon = FALSE;

  for (i = 0; (i < (int)(wNumStrips - 1)) && !bAbandon; i++){
    lpSource = lpInputBuf;  
    lpDest   = lpTmpBuf;

    (*lpfnRdConvertData)(hFile, &lpDest, &lpSource, lpFileInfo, lpBitmapInfo);

    ColorCorrectRGB (lpSource, lpDest, lpFileInfo);
    /*
    **  For now, always clip
    */
    ClipScanlines (lpDest, lpSource, lpFileInfo->wBytesPerRow, wPaddedBytesPerRow, lpFileInfo->wRowsPerStrip);

    RGBToBGR (lpSource, lpDest, lpFileInfo->wRowsPerStrip, lpFileInfo->wScanWidth, lpFileInfo->wBytesPerRow);

    process_scan (lpSource, lpbHist, lpwHist, 3, lpFileInfo->wBytesPerStrip, 
      &nRedMin, &nRedMax, &nGreenMin, &nGreenMax, &nBlueMin, &nBlueMax);

#ifndef NOPRO
    ProDeltaPos (1);
#endif
  }
  /*
  **  Fixup last strip
  */
  if (!bAbandon){
    /*
    **  Mark the last strip for the formats that need to know
    */ 
    lpFileInfo->bIsLastStrip = TRUE;

    lpSource = lpInputBuf;
    lpDest   = lpTmpBuf;

    (*lpfnRdConvertData)(hFile, &lpDest, &lpSource, lpFileInfo, lpBitmapInfo);

    ColorCorrectRGB (lpSource, lpDest, lpFileInfo);

    ClipScanlines (lpDest, lpSource, lpFileInfo->wBytesPerRow, wPaddedBytesPerRow, lpFileInfo->wRowsPerLastStrip);

    RGBToBGR (lpSource, lpDest, lpFileInfo->wRowsPerLastStrip, lpFileInfo -> wScanWidth, lpFileInfo->wBytesPerRow);

    process_scan (lpSource, lpbHist, lpwHist, 3, lpFileInfo->wBytesPerLastStrip, 
      &nRedMin, &nRedMax, &nGreenMin, &nGreenMax, &nBlueMin, &nBlueMax);

#ifndef NOPRO
    ProDeltaPos (1);
#endif
  }
  /*  
  **  Set default min/max values in colorboxes
  */
  lpColorbox->RedMin    = nRedMin;
  lpColorbox->RedMax    = nRedMax;
  lpColorbox->GreenMin  = nGreenMin;
  lpColorbox->GreenMax  = nGreenMax;
  lpColorbox->BlueMin   = nBlueMin;
  lpColorbox->BlueMax   = nBlueMax;
  lpColorbox->Total     = (DWORD)lpFileInfo->wScanWidth * (DWORD)lpFileInfo->wScanHeight;

#ifndef NOPRO
  {
    WORD wUnique;
    char szBuffer [256];

    wUnique = (WORD)count_unique (lpbHist, lpwHist);

    LoadString (hInstIP, STR_OPT_UCOLORS, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
    wsprintf (pStringBuf, pStringBuf2, (wNumReductionColors));

    LoadString (hInstIP, STR_OPT_FROMU, (LPSTR) szBuffer, MAXSTRINGSIZE);
    wsprintf ((LPSTR) pStringBuf2, (LPSTR) szBuffer,wUnique);

    lstrcat ((LPSTR) pStringBuf, (LPSTR) pStringBuf2);
    ProSetText (ID_STATUS1, (LPSTR) pStringBuf);
  }
#endif

  ReleaseDC (hWnd, hDC);
  LocalUnlock (hStringBuf);

  if (bAbandon){
#ifndef NOPRO
    EnableWindow (hWndIP, TRUE);
    ProClose ();  
#endif
    return (FALSE);
  }
  else
    return (TRUE);
}

int GeneratePalette (hWnd, lpInputBuf, lpTmpBuf, lpRGBQuad)
HWND            hWnd;
LPSTR           lpInputBuf;
LPSTR           lpTmpBuf;
RGBQUAD FAR *   lpRGBQuad;
{
  int           i;
  LPCOLORBOX    lpColorBoxList;
  LPCOLORBOX    lpColorBox;
  LPCOLORBOX    lpColorBoxTemp;
  RGBQUAD FAR * lpRGBQuadTemp;

#ifndef NOPRO
  {
    char szStringBuf[256];

    ProSetBarRange (wNumReductionColors >> 4);
    ProSetBarPos (1);
    LoadString (hInstIP, STR_GEN_PALETTE, (LPSTR) szStringBuf, MAXSTRINGSIZE);
    ProSetText (ID_STATUS3, (LPSTR) szStringBuf);
  }
#endif
  /*
  **  Initialize out palette structure to be all whites...
  */
  for (i = 0; i < 256; i++)
    rm[i] = gm[i] = bm[i] = 255;
  /*
  **  Initialize buffer to 0s
  */
  _fmemset ((LPSTR)lpTmpBuf, 0, 512 * sizeof(COLORBOX));
  /*
  for (i = 0; i < (512 * sizeof(COLORBOX)); i++)
    *(lpTmpBuf + i) = 0;
  */
  /*
  **  (At this point lpInputBuf contains the Colorbox min and max values data...)
  **  Start initialization of Box list from stuff set in get_histogram
  */
  _fmemcpy (lpTmpBuf, lpInputBuf, sizeof(COLORBOX));
  /*
  **  Point to beginning of an empty memory block & initialize linked list pattern
  */
  lpGlobalColorBoxes = (LPCOLORBOX)lpTmpBuf;

  lpColorBoxList   = lpFreeColorBoxes = (LPCOLORBOX)lpTmpBuf;

  lpFreeColorBoxes[0].next = &lpFreeColorBoxes[1];
  lpFreeColorBoxes[0].prev = NULL;
  for (i = 1; i < (int)wNumReductionColors - 3; i++){
    lpFreeColorBoxes[i].next = &lpFreeColorBoxes[i + 1];
    lpFreeColorBoxes[i].prev = &lpFreeColorBoxes[i - 1];
  }
  lpFreeColorBoxes[wNumReductionColors - 3].next = NULL;
  lpFreeColorBoxes[wNumReductionColors - 3].prev = &lpFreeColorBoxes[wNumReductionColors - 4];

  lpColorBox       = lpFreeColorBoxes;
  lpFreeColorBoxes = lpColorBox->next;

  if (lpFreeColorBoxes)
    lpFreeColorBoxes->prev = NULL;

  lpColorBox->next = lpUsedColorBoxes;
  lpUsedColorBoxes = lpColorBox;

  if (lpColorBox->next)
    lpColorBox->next->prev = lpColorBox;
  /*
  **  This equates to calling get_histogram with a lpColorBox 
  */
  lpColorBoxTemp        = (LPCOLORBOX)lpInputBuf;
  lpColorBox->Total     = lpColorBoxTemp->Total; 
  lpColorBox->RedMin    = lpColorBoxTemp->RedMin;
  lpColorBox->RedMax    = lpColorBoxTemp->RedMax;
  lpColorBox->GreenMin  = lpColorBoxTemp->GreenMin;
  lpColorBox->GreenMax  = lpColorBoxTemp->GreenMax;
  lpColorBox->BlueMin   = lpColorBoxTemp->BlueMin;
  lpColorBox->BlueMax   = lpColorBoxTemp->BlueMax;

  wGlobalBoxCount = 0;

  while (lpFreeColorBoxes != NULL){
    lpColorBox = GetLargestColorBox();       /* find largest colorbox */
    if (lpColorBox != NULL)
      SplitColorBox (lpColorBox);          /* split largest colorbox */
    else
      lpFreeColorBoxes = NULL;
#ifndef NOPRO
    if ((nFreeBoxCount % 16) == 0)
       ProDeltaPos (1);
#endif
    nFreeBoxCount++;
  }
#ifndef NOPRO
  /*
  **  Set to all colors
  */
  ProSetBarPos (wNumReductionColors >> 2);
#endif
  /*
  **  Assign palette colors from median of colors in colorboxes
  **  Start at 2 instead of 0 to allow space for black and white
  */
  rm[0] = gm[0] = bm[0] = 0;
  for (i = 1, lpColorBox = lpUsedColorBoxes; lpColorBox != NULL; i++, lpColorBox = lpColorBox->next){
    AssignColor (lpColorBox, &rm[i], &gm[i], &bm[i], PAL_MEDIAN);
  }
  rm[wNumReductionColors - 1] = gm[wNumReductionColors - 1] = bm[wNumReductionColors - 1] = 255;
  lpColorBoxList = lpFreeColorBoxes = lpUsedColorBoxes = NULL;
  /*
  **  Sort the palette
  */
  SortPalette ();
  /*
  **  Reformat and copy to Windows 3.0 palette
  */
  lpRGBQuadTemp = lpRGBQuad;
  for (i = 0; i < MAXCOLORMAPSIZE; i++){         /* copy all 256 entries */
    lpRGBQuad->rgbRed      = (BYTE)rm[i];
    lpRGBQuad->rgbGreen    = (BYTE)gm[i];
    lpRGBQuad->rgbBlue     = (BYTE)bm[i];
    lpRGBQuad->rgbReserved = 0;    
    lpRGBQuad++;   
  }
  return (TRUE);
}

int MapRGBSpace (hWnd, lpInputBuf, lpTmpBuf)
HWND  hWnd;
LPSTR lpInputBuf;
LPSTR lpTmpBuf;
{
  int nError;

#ifndef NOPRO
  {
    char szStringBuf [256];

    ProSetBarRange (32);
    ProSetBarPos (0);
    LoadString (hInstIP, STR_OPT_COLORS, (LPSTR) szStringBuf, MAXSTRINGSIZE);
    ProSetText (ID_STATUS4, (LPSTR) szStringBuf);
  }
#endif
  /*
  **  Use lpTmpBuf for Cells; lpInputBuf for smallHist
  **
  **  5a: Create color cell list as described in Heckbert[2]
  */
  lplpColorCells = (LPCOLORCELL FAR *)(lpTmpBuf);

  _fmemset ((LPSTR)lplpColorCells, 0, (size_t)1024); 

  lpSmallhist = lpInputBuf;
  nError      = MapColorTable();

#ifndef NOPRO
  EnableWindow (hWndIP, TRUE);
  ProClose ();
#endif

  return (nError);
}

int NEAR PASCAL MapColorTable(void)
{
  WORD          wRed;
  WORD          wGreen;
  WORD          wBlue;
  WORD          wRedIndex;
  WORD          wGreenIndex;
  WORD          wBlueIndex;
  WORD          wIndex;
  BYTE          bColorIndex;
  DWORD         dwDistanceTemp;
  DWORD         dwDistance;
  LPCOLORCELL   lpColorCell;
  WORD          wCellNumEntries;
  LPSTR         lpHistogram;
  int           i;
  int           j;
  /*
  **  Set color cells
  */
  wColorCellCount = 0;
  /*
  **  Set index histogram
  */
  for (wIndex = 0, lpHistogram = lpbHist; wIndex < 32768; wIndex++)
    *lpHistogram++ = (char)(get_hist((long)wIndex) ? 255 : 0);
  /*
  **  Create colorcells and set small histogram
  */
  lpGlobalColorCells = (LPCOLORCELL)lpwHist;
//  _fmemset ((LPSTR)lplpColorCells, 0, (size_t)65535);
  lpHistogram = lpSmallhist = lpbHist;
  for (wRed = 0; wRed < BITLENGTH && !bAbandon; wRed++){
#ifndef NOPRO
    ProDeltaPos (1);
#endif
    wRedIndex   = (wRed >> (BITDEPTH - CELLDEPTH)) << (CELLDEPTH * 2);
    for (wGreen = 0; wGreen < BITLENGTH; wGreen++){
      wGreenIndex = (wGreen >> (BITDEPTH - CELLDEPTH)) << CELLDEPTH;
      for (wBlue = 0; wBlue < BITLENGTH; wBlue++){
        bColorIndex = (BYTE)255;
        if (*lpHistogram){
          wBlueIndex  = wBlue >> (BITDEPTH - CELLDEPTH);
          lpColorCell = *(lplpColorCells + wRedIndex + wGreenIndex + wBlueIndex);
          if (!lpColorCell)
            lpColorCell = CreateColorCell(
              RGB((BYTE)(wRed   << (COLORDEPTH - BITDEPTH)),
                  (BYTE)(wGreen << (COLORDEPTH - BITDEPTH)),
                  (BYTE)(wBlue  << (COLORDEPTH - BITDEPTH))));
          if (!lpColorCell)
            return (-1);
          /*
          **  Find closest color palette index in colorcell
          */
          dwDistance = 0xFFFFFFFF;
          wCellNumEntries = lpColorCell->ccNumEntries;
          for (i = 0; i < (int)wCellNumEntries && dwDistance > (DWORD)lpColorCell->ccEntries[i][1]; i++){
            j = lpColorCell->ccEntries[i][0];
            dwDistanceTemp = GetColorDistance(
              RGB((BYTE)rm[j], (BYTE)gm[j], (BYTE)bm[j]),
              RGB((BYTE)(wRed   << (COLORDEPTH - BITDEPTH)),
                  (BYTE)(wGreen << (COLORDEPTH - BITDEPTH)),
                  (BYTE)(wBlue  << (COLORDEPTH - BITDEPTH))));
            if (dwDistanceTemp < dwDistance){
              dwDistance  = dwDistanceTemp;
              bColorIndex = (BYTE)j;
            }
          }
        }
        *lpHistogram++ = bColorIndex;
      }  
    }
  }
  return (!bAbandon);  // If user abandons, return FALSE..
}

LPCOLORCELL NEAR PASCAL CreateColorCell (rgbColor)
DWORD         rgbColor;
{
  LPCOLORCELL lpColorCell;
  WORD        wRed;
  WORD        wGreen;
  WORD        wBlue;
  WORD        wRedCenter;
  WORD        wGreenCenter;
  WORD        wBlueCenter;
  WORD        wRedEntry;
  WORD        wGreenEntry;
  WORD        wBlueEntry;
  WORD        wRedTemp;
  WORD        wGreenTemp;
  WORD        wBlueTemp;
  DWORD       dwMinDistance;
  DWORD       dwDistance;
  WORD        wTemp;
  WORD        wNext;
  WORD        w;
  WORD        n;

  wRed   = wRedCenter   = (WORD)GetRValue(rgbColor);
  wGreen = wGreenCenter = (WORD)GetGValue(rgbColor);
  wBlue  = wBlueCenter  = (WORD)GetBValue(rgbColor);
  /*
  **  ColorCell
  */
  wRed   >>= COLORDEPTH - CELLDEPTH;
  wGreen >>= COLORDEPTH - CELLDEPTH;
  wBlue  >>= COLORDEPTH - CELLDEPTH;
  /*
  ** ColorCell center
  */
  wRedCenter   = wRed << (COLORDEPTH - CELLDEPTH);
  wGreenCenter = wGreen << (COLORDEPTH - CELLDEPTH);
  wBlueCenter  = wBlue  << (COLORDEPTH - CELLDEPTH);
  wRedCenter   += ((MAXCOLOR / CELLLENGTH / 2) - 1);
  wGreenCenter += ((MAXCOLOR / CELLLENGTH / 2) - 1);
  wBlueCenter  += ((MAXCOLOR / CELLLENGTH / 2) - 1);
  /*
  wRedCenter   &= ~0 << (COLORDEPTH - CELLDEPTH);
  wGreenCenter &= ~0 << (COLORDEPTH - CELLDEPTH);
  wBlueCenter  &= ~0 << (COLORDEPTH - CELLDEPTH);
  wRedCenter   += (MAXCOLOR / CELLLENGTH / 2) - 1;
  wGreenCenter += (MAXCOLOR / CELLLENGTH / 2) - 1;
  wBlueCenter  += (MAXCOLOR / CELLLENGTH / 2) - 1;
  */
  /*
  **  Allocate Color cell from table
  */
  if (wColorCellCount == 64){       /* only 64 color cells possible */
    MessageBeep (NULL);
    return (0);
  }
  lpColorCell = (LPCOLORCELL)&lpGlobalColorCells[wColorCellCount];
  *(lplpColorCells + (wRed << (CELLDEPTH * 2)) + (wGreen << CELLDEPTH) + wBlue) = lpColorCell;
  lpColorCell->ccNumEntries = 0;
  wColorCellCount++;
  /*
  **  Step 1: Find all colors inside this cell; while we're at
  **          it, find the distance to the furthest corner of the 
  **          centermost point.
  */
  dwMinDistance = 0xFFFFFFFF;

  for (w = 0; w < wNumReductionColors; w++){
    wRedEntry   = (WORD)rm[w];
    wGreenEntry = (WORD)gm[w];
    wBlueEntry  = (WORD)bm[w];

    if ((wRedEntry   >> (COLORDEPTH - CELLDEPTH)) == wRed &&
        (wGreenEntry >> (COLORDEPTH - CELLDEPTH)) == wGreen &&
        (wBlueEntry  >> (COLORDEPTH - CELLDEPTH)) == wBlue){
      
      lpColorCell->ccEntries[lpColorCell->ccNumEntries][0] = w;
      lpColorCell->ccEntries[lpColorCell->ccNumEntries][1] = 0;
      lpColorCell->ccNumEntries++;
      /*
      **  if distance is left of center, distance is to furthest corner
      */
      if (wRedEntry > wRedCenter)
        wRedTemp = wRedCenter - ((MAXCOLOR / CELLLENGTH / 2) - 1);
      else
        wRedTemp = wRedCenter + (MAXCOLOR / CELLLENGTH / 2);

      if (wGreenEntry > wGreenCenter)
        wGreenTemp = wGreenCenter - ((MAXCOLOR / CELLLENGTH / 2) - 1);
      else
        wGreenTemp = wGreenCenter + (MAXCOLOR / CELLLENGTH / 2);

      if (wBlueEntry > wBlueCenter)
        wBlueTemp = wBlueCenter - ((MAXCOLOR / CELLLENGTH / 2) - 1);
      else
        wBlueTemp = wBlueCenter + (MAXCOLOR / CELLLENGTH / 2);

      dwDistance = GetColorDistance(
        RGB((BYTE)wRedEntry, (BYTE)wGreenEntry, (BYTE)wBlueEntry),
        RGB((BYTE)wRedTemp, (BYTE)wGreenTemp, (BYTE)wBlueTemp));

      if (dwDistance < dwMinDistance)
        dwMinDistance = dwDistance;
    }
  }
  /*
  **  Step 2: Find all other points within that distance to box.
  **          Don't set the ones that are already in the color cell.
  */
  for (w = 0; w < wNumReductionColors; w++){
    wRedEntry   = (WORD)rm[w];
    wGreenEntry = (WORD)gm[w];
    wBlueEntry  = (WORD)bm[w];
    /*
    **  If distance between color and colorcell is positive, distance is
    **  from outer edge of color cell and color, otherwise from colorcell and
    **  color
    */
    if ((wRedEntry   >> (COLORDEPTH - CELLDEPTH)) != wRed ||
        (wGreenEntry >> (COLORDEPTH - CELLDEPTH)) != wGreen ||
        (wBlueEntry  >> (COLORDEPTH - CELLDEPTH)) != wBlue){

      if (wRedEntry > wRedCenter)
        wRedTemp = wRedCenter + (MAXCOLOR / CELLLENGTH / 2);
      else
        wRedTemp = (wRedCenter - (MAXCOLOR / CELLLENGTH / 2) - 1);

      if (wGreenEntry > wGreenCenter)
        wGreenTemp = wGreenCenter + (MAXCOLOR / CELLLENGTH / 2);
      else
        wGreenTemp = wGreenCenter - ((MAXCOLOR / CELLLENGTH / 2) - 1);

      if (wBlueEntry > wBlueCenter)
        wBlueTemp = wBlueCenter + (MAXCOLOR / CELLLENGTH / 2);
      else
        wBlueTemp = wBlueCenter - ((MAXCOLOR / CELLLENGTH / 2) - 1);

      dwDistance = GetColorDistance(
        RGB((BYTE)wRedEntry, (BYTE)wGreenEntry, (BYTE)wBlueEntry),
        RGB((BYTE)wRedTemp, (BYTE)wGreenTemp, (BYTE)wBlueTemp));

      if (dwDistance < dwMinDistance){
        lpColorCell->ccEntries[lpColorCell->ccNumEntries][0] = w;
        lpColorCell->ccEntries[lpColorCell->ccNumEntries][1] = (WORD)dwDistance;
        lpColorCell->ccNumEntries++;
      }
    }
  }
  /*
  **  Step 3: Sort color cells by distance using cheap exchange sort
  */
  n = lpColorCell->ccNumEntries - 1;
  while (n > 0){
    wNext = 0;
    for (w = 0; w < n; w++){
      if (lpColorCell->ccEntries[w][1] >= lpColorCell->ccEntries[w + 1][1]){

        wTemp = lpColorCell->ccEntries[w][0];
        lpColorCell->ccEntries[w][0]     = lpColorCell->ccEntries[w + 1][0];
        lpColorCell->ccEntries[w + 1][0] = wTemp;

        wTemp = lpColorCell->ccEntries[w][1];
        lpColorCell->ccEntries[w][1]     = lpColorCell->ccEntries[w + 1][1];
        lpColorCell->ccEntries[w + 1][1] = wTemp;

        wNext = w;
      }
    }
    n = wNext;
  }
  return (lpColorCell);
}


/****************************************************************************
**
**  FUNCTION:     MapDithRGBSpace(HWND hWnd, LPSTR lpInputBuf, LPSTR lpTmpBuf)
**
**  DESCRIPTION:  Map dither histogram using colorcells
**
****************************************************************************/

int MapDithRGBSpace (hWnd, lpInputBuf, lpTmpBuf)
HWND  hWnd;
LPSTR lpInputBuf;
LPSTR lpTmpBuf;
{
  int nError;

#ifndef NOPRO
  {
    char szStringBuf [256];

    ProSetBarRange (32);
    ProSetBarPos (0);
    LoadString (hInstIP, STR_OPT_COLORS, (LPSTR) szStringBuf, MAXSTRINGSIZE);
    ProSetText (ID_STATUS4, (LPSTR) szStringBuf);
  }
#endif
  /*
  **  Use lpTmpBuf for Cells; lpInputBuf for smallHist
  **
  **  5a: Create color cell list as described in Heckbert[2]
  */
  lplpColorCells = (LPCOLORCELL FAR *)(lpTmpBuf);

  _fmemset ((LPSTR)lplpColorCells, 0, (size_t)1024); 

  lpSmallhist = lpInputBuf;
  nError      = MapDitherColorTable();

#ifndef NOPRO
  EnableWindow (hWndIP, TRUE);
  ProClose ();
#endif

  return (nError);
}

/***************************************************************************
**
**  FUNCTION:     MapDitherColorTable (void)
**
**  DESCRIPTION:  Map dither histogram using colorcells
**
**  NOTES:        Needs more work
**
***************************************************************************/
int NEAR PASCAL MapDitherColorTable(void)
{
  WORD          wRed;
  WORD          wGreen;
  WORD          wBlue;
  WORD          wRedIndex;
  WORD          wGreenIndex;
  WORD          wBlueIndex;
  WORD          wIndex;
  BYTE          bColorIndex;
  DWORD         dwDistanceTemp;
  DWORD         dwDistance;
  LPCOLORCELL   lpColorCell;
  WORD          wCellNumEntries;
  LPSTR         lpHistogram;
  int           i;
  int           j;
  /*
  **  Set color cells
  */
  wColorCellCount = 0;
  /*
  **  Set index histogram
  */
  for (wIndex = 0, lpHistogram = lpbHist; wIndex < 32768; wIndex++)
    *lpHistogram++ = (char)(get_hist((long)wIndex) ? 255 : 0);
  /*
  **  Create colorcells and set small histogram
  */
  lpGlobalColorCells = (LPCOLORCELL)lpwHist;
//  _fmemset ((LPSTR)lplpColorCells, 0, (size_t)65535);
  /*
  **  Create colorcells and set small histogram
  */
  lpHistogram = lpSmallhist = lpbHist;
  for (wRed = 0; wRed < BITLENGTH && !bAbandon; wRed++){
#ifndef NOPRO
    ProDeltaPos (1);
#endif
    wRedIndex   = (wRed >> (BITDEPTH - CELLDEPTH)) << (CELLDEPTH * 2);
    for (wGreen = 0; wGreen < BITLENGTH; wGreen++)
    {
      wGreenIndex = (wGreen >> (BITDEPTH - CELLDEPTH)) << CELLDEPTH;
      for (wBlue = 0; wBlue < BITLENGTH; wBlue += wDithQuality){
//      for (wBlue = 0; wBlue < BITLENGTH; wBlue++){
        wBlueIndex  = wBlue >> (BITDEPTH - CELLDEPTH);
        lpColorCell = *(lplpColorCells + wRedIndex + wGreenIndex + wBlueIndex);
        if (!lpColorCell)
          lpColorCell = CreateColorCell(
            RGB((BYTE)(wRed   << (COLORDEPTH - BITDEPTH)),
                (BYTE)(wGreen << (COLORDEPTH - BITDEPTH)),
                (BYTE)(wBlue  << (COLORDEPTH - BITDEPTH))));
        if (!lpColorCell)
          return (-1);
        /*
        **  Find closest color palette index in colorcell
        */
        dwDistance = 0xFFFFFFFF;
        wCellNumEntries = lpColorCell->ccNumEntries;
        bColorIndex = (BYTE)255;
        /*
        for (i = 0; i < wNumReductionColors; i++){
          dwDistanceTemp = GetColorDistance(
            RGB((BYTE)rm[i], (BYTE)gm[i], (BYTE)bm[i]),
            RGB((BYTE)(wRed   << (COLORDEPTH - BITDEPTH)),
                (BYTE)(wGreen << (COLORDEPTH - BITDEPTH)),
                (BYTE)(wBlue  << (COLORDEPTH - BITDEPTH))));
          if (dwDistanceTemp < dwDistance){
            dwDistance  = dwDistanceTemp;
            bColorIndex = (BYTE)i;
          }
        }
        */
        for (i = 0; i < (int)wCellNumEntries && dwDistance > (DWORD)lpColorCell->ccEntries[i][1]; i++){
          j = lpColorCell->ccEntries[i][0];
          dwDistanceTemp = GetColorDistance(
            RGB((BYTE)rm[j], (BYTE)gm[j], (BYTE)bm[j]),
            RGB((BYTE)(wRed   << (COLORDEPTH - BITDEPTH)),
                (BYTE)(wGreen << (COLORDEPTH - BITDEPTH)),
                (BYTE)(wBlue  << (COLORDEPTH - BITDEPTH))));
          if (dwDistanceTemp < dwDistance){
            dwDistance  = dwDistanceTemp;
            bColorIndex = (BYTE)j;
          }
        }
        lpHistogram = lpSmallhist + (wRed << 10) + (wGreen << 5) + wBlue;
        if (wDithQuality == 8)
          _fmemset (lpHistogram, bColorIndex, 8); /* Set 8 at a time */
        *lpHistogram = bColorIndex;
      }  
    }
  }
  return (!bAbandon);  // If user abandons, return FALSE..
}
#ifdef OLD
int MapDithRGBSpace (hWnd, lpInputBuf, lpTmpBuf) 
HWND      hWnd;
LPSTR     lpInputBuf;
LPSTR     lpTmpBuf;
{
  WORD    wRed;
  WORD    wGreen;
  WORD    wBlue;
  WORD    wIndex;
  LPSTR   lpPalette;
  LPSTR   lpPtr;
  LPSTR   lpCurrHistPtr;
  LPWORD  lpSquares;
  BYTE    bColor;
  WORD    w;
  /*
  **
  */
  lpSmallhist = lpbHist;
  /*
  **  Prepare squares table to avoid doing multiplys
  */
  lpSquares = (WORD FAR *)lpInputBuf;
  for (w = 0; w < 64; w++)
    lpSquares[w] = (w * w);

  lpPtr = lpPalette = lpTmpBuf;
      
  for (w = 0; w < 256; w++){
    *lpPtr++ = (BYTE)((WORD)rm[w] >> 2);
    *lpPtr++ = (BYTE)((WORD)gm[w] >> 2);
    *lpPtr++ = (BYTE)((WORD)bm[w] >> 2);
  }

#ifndef NOPRO
  {
    char szStringBuf [256];

    ProSetBarRange (32);
    ProSetBarPos (0);
    LoadString (hInstIP, STR_MAP_COLORSPACE, (LPSTR) szStringBuf, sizeof (szStringBuf) - 1);
    ProSetText (ID_STATUS4, (LPSTR) szStringBuf);
  }
#endif
  bAbandon = FALSE;
  for (wRed = 0; (wRed < 32 && ! bAbandon); wRed++){
    ProDeltaPos (1);
    for (wGreen = 0; wGreen < 32; wGreen++)
      for (wBlue = 0; wBlue < 32; wBlue += wDithQuality){

        bColor     = find_closesta (wRed << 1, wGreen << 1, wBlue << 1, lpPalette, lpSquares, (WORD)(wNumReductionColors));
        wIndex = (WORD)((wRed << 10) + (wGreen << 5) + wBlue);
        lpCurrHistPtr = (lpSmallhist + wIndex);
        if (wDithQuality == 8)
          _fmemset (lpCurrHistPtr, bColor, 8);  // Set 8 of them and save time */ 
        *lpCurrHistPtr = bColor;
      }
  }
#ifndef NOPRO
  EnableWindow (hWndIP, TRUE);
  ProClose ();
#endif
  return (!bAbandon);  // If user abandons, return FALSE..
}
#endif

/****************************************************************************
**
**  FUNCTION:     GetLargestColorBox (void)
**
**  DESCRIPTION:  Find the largest color box according to number of entries
**
****************************************************************************/

LPCOLORBOX NEAR PASCAL GetLargestColorBox(void)
{
  LPCOLORBOX  lpColorBoxTemp = lpUsedColorBoxes;
  LPCOLORBOX  lpColorBox     = NULL;
  DWORD       dwSize         = 0;
  /*
  **  Find the largest colorbox
  */
  while (lpColorBoxTemp != NULL){
    if ((lpColorBoxTemp->RedMax   > lpColorBoxTemp->RedMin  ||
         lpColorBoxTemp->GreenMax > lpColorBoxTemp->GreenMin  ||
         lpColorBoxTemp->BlueMax  > lpColorBoxTemp->BlueMin) &&
         lpColorBoxTemp->Total    > dwSize){

      lpColorBox  = lpColorBoxTemp;
      dwSize      = lpColorBoxTemp->Total;
    }
    lpColorBoxTemp = lpColorBoxTemp->next;
  }
  return(lpColorBox);
}

/****************************************************************************
**
**  FUNCTION:     SplitColorBox (void)
**
**  DESCRIPTION:  Divide color box
**
****************************************************************************/
void NEAR PASCAL SplitColorBox(lpColorBox)
LPCOLORBOX lpColorBox;
{
  LPCOLORBOX  lpNewColorBox;
  DWORD *     lpHistPtr;
  DWORD       dwHist2[BITLENGTH];
  WORD        wRed;
  WORD        wGreen;
  WORD        wBlue;
  WORD        wTemp;
  DWORD       dwSum;
  DWORD       dwSum2;
  WORD        wFirst;
  WORD        wLast;
  WORD        i;
  WORD        j;
  enum        {RED,GREEN,BLUE} nMajorAxis;
  /*
  **  Determine largest axis in colorbox: red, green or blue
  */
  if ((i = lpColorBox->RedMax - lpColorBox->RedMin) >= (lpColorBox->GreenMax - lpColorBox->GreenMin) &&
    i >= (lpColorBox->BlueMax - lpColorBox->BlueMin))
    nMajorAxis = RED;
  else 
    if ((lpColorBox->GreenMax - lpColorBox->GreenMin) >= (lpColorBox->BlueMax - lpColorBox->BlueMin))
      nMajorAxis = GREEN;
    else
      nMajorAxis = BLUE;
  /*
  **  Get histogram along longest axis into hist2
  */
  switch(nMajorAxis){
    case RED:
      lpHistPtr = &dwHist2[lpColorBox->RedMin];
      for (wRed = (WORD)lpColorBox->RedMin; wRed <= (WORD)lpColorBox->RedMax; wRed++){
        *lpHistPtr = 0;
        for (wGreen = (WORD)lpColorBox->GreenMin; wGreen <= (WORD)lpColorBox->GreenMax; wGreen++){
          lHistIndex = ((wRed << 10) + (wGreen << 5) + lpColorBox->BlueMin);
          for (wBlue = (WORD)lpColorBox->BlueMin; wBlue <= (WORD)lpColorBox->BlueMax; wBlue++)
            *lpHistPtr += (DWORD)get_hist (lHistIndex++);
        }
        lpHistPtr++;            /*  Count all the counts - and keep counting */ 
      }
      wFirst = lpColorBox->RedMin;    /*  First r value  */
      wLast  = lpColorBox->RedMax;    /*  Last r value   */
      break;
    case GREEN:
      lpHistPtr = &dwHist2[lpColorBox->GreenMin];
      for (wGreen = (WORD)lpColorBox->GreenMin; wGreen <= (WORD)lpColorBox->GreenMax; wGreen++){
        *lpHistPtr = 0;
        for (wRed = (WORD)lpColorBox->RedMin; wRed <= (WORD)lpColorBox->RedMax; wRed++){
          lHistIndex = ((wRed << 10) + (wGreen << 5) + lpColorBox -> BlueMin); 
          for (wBlue = (WORD)lpColorBox->BlueMin; wBlue <= (WORD)lpColorBox->BlueMax; wBlue++)
            *lpHistPtr += (DWORD)get_hist (lHistIndex++);
        }
        lpHistPtr++;
      }
      wFirst = lpColorBox->GreenMin;  
      wLast  = lpColorBox->GreenMax;
      break;
    case BLUE:
      lpHistPtr = &dwHist2[lpColorBox->BlueMin];
      for (wBlue = (WORD)lpColorBox->BlueMin; wBlue <= (WORD)lpColorBox->BlueMax; wBlue++){
        *lpHistPtr = 0;
        for (wRed = (WORD)lpColorBox->RedMin; wRed <= (WORD)lpColorBox->RedMax; wRed++){       
          lHistIndex = ((wRed << 10) + (lpColorBox->GreenMin << 5) + wBlue); 
          for (wGreen = (WORD)lpColorBox->GreenMin; wGreen <= (WORD)lpColorBox->GreenMax; wGreen++){
            *lpHistPtr += (DWORD)get_hist (lHistIndex);
            lHistIndex += BITLENGTH;
          }
        }
        lpHistPtr++;
      }
      wFirst = lpColorBox->BlueMin;  
      wLast  = lpColorBox->BlueMax;
      break;
  }
  /*
  **  Find median point
  */
  dwSum2     = (DWORD)lpColorBox->Total >> 1;
  lpHistPtr  = &dwHist2[wFirst];
  for (i = wFirst, dwSum = 0; i <= wLast && (dwSum += *lpHistPtr++) < dwSum2; i++);
  if (i == wFirst)
    i++;
  wTemp = (WORD)i;

  lpHistPtr = &dwHist2[wLast];
  for (i = wLast, dwSum = 0; i >= wFirst && (dwSum += *lpHistPtr--) < dwSum2; i--);
  if (i == wFirst)
    i++;
  wTemp += (WORD)i;
  wTemp >>= 1;
  i = wTemp;
  /*
  **  Create new box, re-allocate points
  */
  lpNewColorBox    = lpFreeColorBoxes;
  lpFreeColorBoxes = lpNewColorBox->next;
  if (lpFreeColorBoxes)
    lpFreeColorBoxes->prev = NULL;
  if (lpUsedColorBoxes)
    lpUsedColorBoxes->prev = lpNewColorBox;
  lpNewColorBox->next = lpUsedColorBoxes;
  lpUsedColorBoxes    = lpNewColorBox;
  /*
  **  Reset total entries in new boxes
  */
  lpHistPtr  = &dwHist2[wFirst];
  for (j = wFirst, dwSum = 0; j < i; j++)
    dwSum += *lpHistPtr++;
  lpColorBox->Total    -= (long)dwSum;
  lpNewColorBox->Total  = (long)dwSum;
  /*
  **  New inherits min/max values from parent colorbox.
  */
  lpNewColorBox->RedMin   = lpColorBox->RedMin;
  lpNewColorBox->RedMax   = lpColorBox->RedMax;
  lpNewColorBox->GreenMin = lpColorBox->GreenMin;
  lpNewColorBox->GreenMax = lpColorBox->GreenMax;
  lpNewColorBox->BlueMin  = lpColorBox->BlueMin;
  lpNewColorBox->BlueMax  = lpColorBox->BlueMax;
  /*
  **  Now set min/max for new from box just split
  */
  switch (nMajorAxis){
    case RED:
      lpNewColorBox->RedMax = i - 1;      /* i was an index into sub-histogram */
      lpColorBox->RedMin    = i;          /* an is the median ptr              */
      break;
    case GREEN:
      lpNewColorBox->GreenMax = i - 1;
      lpColorBox->GreenMin    = i;
      break;
    case BLUE:
      lpNewColorBox->BlueMax = i - 1;
      lpColorBox->BlueMin    = i;
      break;
  }
  ShrinkColorBox (lpColorBox); 
  ShrinkColorBox (lpNewColorBox);
}

/****************************************************************************
**
**  FUNCTION:     ShrinkColorBox (LPCOLORBOX lpColorBox);
**
**  DESCRIPTION:  Shrink Color box dimensions to enclose sub-histogram
**                color space
**
****************************************************************************/

void NEAR PASCAL ShrinkColorBox(lpColorBox)
LPCOLORBOX lpColorBox;
{
  WORD  wRed;
  WORD  wGreen;
  WORD  wBlue;
  long  lHistIndex;
  /*
  **  This can be replaced with callable functions to get min/max values
  */
  if (lpColorBox->RedMax > lpColorBox->RedMin){
    for (wRed = lpColorBox->RedMin; wRed <= lpColorBox->RedMax; wRed++)
      for (wGreen = lpColorBox->GreenMin; wGreen <= lpColorBox->GreenMax; wGreen++){
        lHistIndex = ((wRed << 10) + (wGreen << 5) + lpColorBox->BlueMin); 
        for (wBlue = lpColorBox->BlueMin; wBlue <= lpColorBox->BlueMax; wBlue++)
        if (get_hist (lHistIndex++)){
          lpColorBox->RedMin = wRed;
          goto have_RedMin;
        }  
      }
have_RedMin:
    if (lpColorBox->RedMax > lpColorBox->RedMin)
      for (wRed = lpColorBox->RedMax; wRed >= lpColorBox->RedMin; wRed--)
        for (wGreen = lpColorBox->GreenMin; wGreen <= lpColorBox->GreenMax; wGreen++){
          lHistIndex = ((wRed << 10) + (wGreen << 5) + lpColorBox->BlueMin); 
          for (wBlue = lpColorBox->BlueMin; wBlue <= lpColorBox->BlueMax; wBlue++)
            if (get_hist (lHistIndex++)){
              lpColorBox->RedMax = wRed;
              goto have_RedMax;
            }  
        }
  }
have_RedMax:
  if (lpColorBox->GreenMax > lpColorBox->GreenMin){
    for (wGreen = lpColorBox->GreenMin; wGreen <= lpColorBox->GreenMax; wGreen++)
      for(wRed = lpColorBox->RedMin; wRed <= lpColorBox->RedMax; wRed++){
        lHistIndex = ((wRed << 10) + (wGreen << 5) + lpColorBox->BlueMin); 
        for (wBlue = lpColorBox->BlueMin; wBlue <= lpColorBox->BlueMax; wBlue++)
          if (get_hist (lHistIndex++)){
            lpColorBox->GreenMin = wGreen;
            goto have_GreenMin;
          }
      }
have_GreenMin:
    if (lpColorBox->GreenMax > lpColorBox->GreenMin)
      for (wGreen = lpColorBox->GreenMax; wGreen >= lpColorBox->GreenMin; wGreen--)
        for(wRed = lpColorBox->RedMin; wRed <= lpColorBox->RedMax; wRed++){
          lHistIndex = ((wRed << 10) + (wGreen << 5) + lpColorBox->BlueMin);
          for (wBlue = lpColorBox->BlueMin; wBlue <= lpColorBox->BlueMax; wBlue++)
            if (get_hist (lHistIndex++)){
              lpColorBox->GreenMax = wGreen;
              goto have_GreenMax;
            }
        }
  }
have_GreenMax:
  if (lpColorBox->BlueMax > lpColorBox->BlueMin){
    for (wBlue = lpColorBox->BlueMin; wBlue <= lpColorBox->BlueMax; wBlue++)
      for (wRed = lpColorBox->RedMin; wRed <= lpColorBox->RedMax; wRed++){
        lHistIndex = ((wRed << 10) + (lpColorBox->GreenMin << 5) + wBlue); 
        for (wGreen = lpColorBox->GreenMin; wGreen <= lpColorBox->GreenMax; wGreen++){
          if (get_hist (lHistIndex)){
            lpColorBox->BlueMin = wBlue;
            goto have_BlueMin;
          }
          lHistIndex += BITLENGTH;
        }
      }
have_BlueMin:
    if (lpColorBox->BlueMax > lpColorBox->BlueMin )
      for (wBlue = lpColorBox->BlueMax; wBlue >= lpColorBox->BlueMin; wBlue--)
        for (wRed = lpColorBox->RedMin; wRed <= lpColorBox->RedMax; wRed++){
          lHistIndex = ((wRed << 10) + (lpColorBox->GreenMin << 5) + wBlue); 
          for (wGreen = lpColorBox->GreenMin; wGreen <= lpColorBox->GreenMax; wGreen++){
            if (get_hist (lHistIndex)){
              lpColorBox->BlueMax = wBlue;
              goto have_BlueMax;
            }
            lHistIndex += BITLENGTH;
          }
        }
  }
have_BlueMax: 
  return;
}

/****************************************************************************
**
**  FUNCTION:     SortPalette(void)
**
**  DESCRIPTION:  Sort palette based on intensity
**
****************************************************************************/

int SortPalette(void) 
{
  long  lTemp1;
  long  lTemp2;
  WORD  wGap;
  BYTE  bRed;
  BYTE  bGreen;
  BYTE  bBlue;
  int   nChanges;
  int   i;
  /*
  **  Sort based on intensity
  */
  wGap = MAXCOLORMAPSIZE / 2;         // 128;  /* 256/2  */
  while (wGap != 0){
    do{
      nChanges = 0;
      for (i = 0; i < (int)(MAXCOLORMAPSIZE - wGap); i++){
        /*
        **  Get intensity value
        */
        lTemp1 = (30 * rm[i])        + (59 * gm[i])        + (11 * bm[i]);
        lTemp2 = (30 * rm[i + wGap]) + (59 * gm[i + wGap]) + (11 * bm[i + wGap]);
        if (lTemp1 > lTemp2){
          bRed   = rm[i];
          bGreen = gm[i];
          bBlue  = bm[i];

          rm[i]  = rm[i + wGap];
          gm[i]  = gm[i + wGap];
          bm[i]  = bm[i + wGap];

          rm[i + wGap] = bRed;
          gm[i + wGap] = bGreen;
          bm[i + wGap] = bBlue;

          nChanges = 1;
        }
      }
    }while(nChanges);
    wGap >>= 1;
  }
  return (1);
}

/*  NOT USED  */
BYTE find_closestc (nRed, nGreen, nBlue, lpPalette, wCount)
int     nRed;
int     nGreen;
int     nBlue;
LPSTR   lpPalette;
WORD    wCount;
{
  WORD  w;
  char  bSmallest;
  long  lCurrentDistance;
  long  lTempDistance;
  int   nRedErr;
  int   nGreenErr;
  int   nBlueErr;
  int   nRedValue;
  int   nGreenValue;
  int   nBlueValue;
  int   nPalPtr;
  /*
  **
  */
  bSmallest        = 0;
  lCurrentDistance = 9999999;

  for (w = 0; w < wCount; w++){
    nPalPtr   = (3 * w);
    nRedValue = lpPalette[nPalPtr];
    nRedErr   = nRed - nRedValue;
    nRedErr  *= nRedErr;

    nGreenValue = lpPalette[nPalPtr + 1];
    nGreenErr   = nGreen - nGreenValue;
    nGreenErr  *= nGreenErr;

    nBlueValue  = lpPalette[nPalPtr + 2];
    nBlueErr    = nBlue - nBlueValue;
    nBlueErr   *= nBlueErr;

    lTempDistance  = (long)nRedErr + (long)nGreenErr + (long)nBlueErr;

    if (lTempDistance < lCurrentDistance){
      lCurrentDistance = lTempDistance;
      bSmallest  = (BYTE)w;
    }
  }   
  return (bSmallest);
}
/*****************************************************************************
**
**  FUNCTION:     get_hist (long lPosition)
**
**  DESCRIPTION:  get long histogram value
****************************************************************************/

long near get_hist (lPosition)
long    lPosition;
{
  long  lValue;
  /*
  **  Logical shifts for NON reversed bytes...
  */
  lValue    = *(lpbHist + (WORD)lPosition);
  lValue  <<= 16;
  lValue   += *(lpwHist + (WORD)lPosition);
  return (lValue);
}
/****************************************************************************
**
**  FUNCTION:     set_hist (long lPos, long lValue)
**
**  DESCRIPTION:  set long histogram value
**
****************************************************************************/

void set_hist (lPos, lValue)
long    lPos;
long    lValue;
{
  *(lpbHist + (WORD)lPos) = LOBYTE(HIWORD(lValue));
  *(lpwHist + (WORD)lPos) = LOWORD(lValue);
}

/****************************************************************************
**
**  FUNCTION:     AssignColor()
**
**  DESCRIPTION:  Assign palette entry for colorbox
**
****************************************************************************/

int NEAR PASCAL AssignColor (lpColorBox, lpRed, lpGreen, lpBlue, wMethod)
LPCOLORBOX  lpColorBox;
LPBYTE      lpRed;
LPBYTE      lpGreen;
LPBYTE      lpBlue;
WORD        wMethod;
{
  WORD      wRed              = 0;
  WORD      wGreen            = 0;
  WORD      wBlue             = 0;
  WORD      wRedPop           = 0;
  WORD      wGreenPop         = 0;
  WORD      wBluePop          = 0;
  DWORD     dwRed             = 0;
  DWORD     dwGreen           = 0;
  DWORD     dwBlue            = 0;
  DWORD     dwRedAvg          = 0;
  DWORD     dwGreenAvg        = 0;
  DWORD     dwBlueAvg         = 0;
  DWORD     dwHistValue       = 0;
  DWORD     dwHistValueTemp   = 0;
  WORD      wNumEntries       = 0;
  DWORD     rgbColor          = 0;
  /*
  **  Assign palette representative to a color box based on the passed
  **  parameter for method of determination
  */
  switch (wMethod){
    case PAL_MEDIAN:
      /*
      **  Straight Median Cut: pick median color
      */
      *lpRed   = (BYTE)(((lpColorBox->RedMin + lpColorBox->RedMax) << (COLORDEPTH - BITDEPTH)) / 2);
      *lpGreen = (BYTE)(((lpColorBox->GreenMin + lpColorBox->GreenMax) << (COLORDEPTH - BITDEPTH)) / 2);
      *lpBlue  = (BYTE)(((lpColorBox->BlueMin + lpColorBox->BlueMax) << (COLORDEPTH - BITDEPTH)) / 2);
      break;
    case PAL_AVERAGE:
      /*
      **  Average of colors in color box region: pick average color
      */
      for (wRed = (WORD)lpColorBox->RedMin; wRed <= (WORD)lpColorBox->RedMax; wRed++){
        for (wGreen = (WORD)lpColorBox->GreenMin; wGreen <= (WORD)lpColorBox->GreenMax; wGreen++){
          for (wBlue = (WORD)lpColorBox->BlueMin; wBlue <= (WORD)lpColorBox->BlueMax; wBlue++){
            dwHistValue = (DWORD)get_hist ((wRed << 10) + (wGreen << 5) + (wBlue));
            if (dwHistValue){
              wNumEntries++;
              dwRed   += (DWORD)wRed;
              dwGreen += (DWORD)wGreen;
              dwBlue  += (DWORD)wBlue;
            }
          }
        }
      }
      *lpRed   = (BYTE)((dwRed   / (DWORD)wNumEntries) << (COLORDEPTH - BITDEPTH));
      *lpGreen = (BYTE)((dwGreen / (DWORD)wNumEntries) << (COLORDEPTH - BITDEPTH));
      *lpBlue  = (BYTE)((dwBlue  / (DWORD)wNumEntries) << (COLORDEPTH - BITDEPTH));
      break;
    case PAL_POPULARITY:
      /*
      **  Straight popularity: pick most popular color
      */
      for (wRed = (WORD)lpColorBox->RedMin; wRed <= (WORD)lpColorBox->RedMax; wRed++){
        for (wGreen = (WORD)lpColorBox->GreenMin; wGreen <= (WORD)lpColorBox->GreenMax; wGreen++){
          for (wBlue = (WORD)lpColorBox->BlueMin; wBlue <= (WORD)lpColorBox->BlueMax; wBlue++){
            dwHistValueTemp = (DWORD)get_hist ((wRed << 10) + (wGreen << 5) + (wBlue));
            if (dwHistValueTemp > dwHistValue){
              dwHistValue  = dwHistValueTemp;
              dwRed   = (DWORD)wRed;
              dwGreen = (DWORD)wGreen;
              dwBlue  = (DWORD)wBlue;
            }
          }
        }
      }
      *lpRed   = (BYTE)(dwRed   << (COLORDEPTH - BITDEPTH));
      *lpGreen = (BYTE)(dwGreen << (COLORDEPTH - BITDEPTH));
      *lpBlue  = (BYTE)(dwBlue  << (COLORDEPTH - BITDEPTH));
      break;
    case PAL_MEAN:
      /*
      **  Popularity averaging: pick color based on population density
      **  NOTE: potential for overflow
      */
      for (wRed = (WORD)lpColorBox->RedMin; wRed <= (WORD)lpColorBox->RedMax; wRed++){
        for (wGreen = (WORD)lpColorBox->GreenMin; wGreen <= (WORD)lpColorBox->GreenMax; wGreen++){
          for (wBlue = (WORD)lpColorBox->BlueMin; wBlue <= (WORD)lpColorBox->BlueMax; wBlue++){
            dwHistValue = (DWORD)get_hist ((wRed << 10) + (wGreen << 5) + (wBlue));
            if (dwHistValue){
              dwRed   += (DWORD)wRed   * dwHistValue;
              dwGreen += (DWORD)wGreen * dwHistValue;
              dwBlue  += (DWORD)wBlue  * dwHistValue;
            }
          }
        }
      }
      *lpRed   = (BYTE)((dwRed   / (DWORD)lpColorBox->Total) << (COLORDEPTH - BITDEPTH));
      *lpGreen = (BYTE)((dwGreen / (DWORD)lpColorBox->Total) << (COLORDEPTH - BITDEPTH));
      *lpBlue  = (BYTE)((dwBlue  / (DWORD)lpColorBox->Total) << (COLORDEPTH - BITDEPTH));
      break;
    case PAL_MEDPOP:
      /*
      **  Median Cut weighted by popularity. Find color with most occurrences.
      */
      for (wRed = (WORD)lpColorBox->RedMin; wRed <= (WORD)lpColorBox->RedMax; wRed++){
        for (wGreen = (WORD)lpColorBox->GreenMin; wGreen <= (WORD)lpColorBox->GreenMax; wGreen++){
          for (wBlue = (WORD)lpColorBox->BlueMin; wBlue <= (WORD)lpColorBox->BlueMax; wBlue++){
            dwHistValueTemp = get_hist ((wRed << 10) + (wGreen << 5) + (wBlue));
            if (dwHistValueTemp > dwHistValue){
              dwHistValue = dwHistValueTemp;
              dwRed       = (DWORD)wRed;
              dwGreen     = (DWORD)wGreen;
              dwBlue      = (DWORD)wBlue;
            }
          }
        }
      }
      /*
      **  Find representative
      */
      dwRed   <<= (COLORDEPTH - BITDEPTH);
      dwRed    += (DWORD)((lpColorBox->RedMin + lpColorBox->RedMax) << (COLORDEPTH - BITDEPTH)) / 2;
      dwRed   >>= 1;
      dwGreen <<= (COLORDEPTH - BITDEPTH);
      dwGreen  += (DWORD)((lpColorBox->GreenMin + lpColorBox->GreenMax) << (COLORDEPTH - BITDEPTH)) / 2;
      dwGreen >>= 1;
      dwBlue  <<= (COLORDEPTH - BITDEPTH);
      dwBlue   += (DWORD)((lpColorBox->BlueMin + lpColorBox->BlueMax) << (COLORDEPTH - BITDEPTH)) / 2;
      dwBlue  >>= 1;
      *lpRed    = (BYTE)dwRed;
      *lpGreen  = (BYTE)dwGreen;
      *lpBlue   = (BYTE)dwBlue;
      break;
    case PAL_MEDAVG:
      /*
      **  Average of colors in color box region with median
      */
      for (wRed = (WORD)lpColorBox->RedMin; wRed <= (WORD)lpColorBox->RedMax; wRed++){
        for (wGreen = (WORD)lpColorBox->GreenMin; wGreen <= (WORD)lpColorBox->GreenMax; wGreen++){
          for (wBlue = (WORD)lpColorBox->BlueMin; wBlue <= (WORD)lpColorBox->BlueMax; wBlue++){
            dwHistValue = (DWORD)get_hist ((wRed << 10) + (wGreen << 5) + (wBlue));
            if (dwHistValue){
              wNumEntries++;
              dwRed   += (DWORD)wRed;
              dwGreen += (DWORD)wGreen;
              dwBlue  += (DWORD)wBlue;
            }
          }
        }
      }
      /*
      **  Find representative
      */
      dwRed     = (dwRed   / (DWORD)wNumEntries) << (COLORDEPTH - BITDEPTH);
      dwRed    += (DWORD)((lpColorBox->RedMin + lpColorBox->RedMax) << (COLORDEPTH - BITDEPTH)) / 2;
      dwRed   >>= 1;
      dwGreen   = (dwGreen / (DWORD)wNumEntries) << (COLORDEPTH - BITDEPTH);
      dwGreen  += (DWORD)((lpColorBox->GreenMin + lpColorBox->GreenMax) << (COLORDEPTH - BITDEPTH)) / 2;
      dwGreen >>= 1;
      dwBlue    = (dwBlue  / (DWORD)wNumEntries) << (COLORDEPTH - BITDEPTH);
      dwBlue   += ((lpColorBox->BlueMin + lpColorBox->BlueMax) << (COLORDEPTH - BITDEPTH)) / 2;
      dwBlue  >>= 1;
      *lpRed    = (BYTE)dwRed;
      *lpGreen  = (BYTE)dwGreen;
      *lpBlue   = (BYTE)dwBlue;
      break;
    case PAL_MEDPOPAVG:
      /*
      **  Median Cut weighted by popularity. Find color with most occurrences.
      */
      for (wRed = (WORD)lpColorBox->RedMin; wRed <= (WORD)lpColorBox->RedMax; wRed++){
        for (wGreen = (WORD)lpColorBox->GreenMin; wGreen <= (WORD)lpColorBox->GreenMax; wGreen++){
          for (wBlue = (WORD)lpColorBox->BlueMin; wBlue <= (WORD)lpColorBox->BlueMax; wBlue++){
            dwHistValueTemp = (DWORD)get_hist ((wRed << 10) + (wGreen << 5) + (wBlue));
            if (dwHistValueTemp){
              wNumEntries++;
              dwRed   += (DWORD)wRed;
              dwGreen += (DWORD)wGreen;
              dwBlue  += (DWORD)wBlue;
              if (dwHistValueTemp > dwHistValue){
                dwHistValue = dwHistValueTemp;
                wRedPop    = wRed;
                wGreenPop  = wGreen;
                wBluePop   = wBlue;
              }
            }
          }
        }
      }
      /*
      **  Find representative
      */
      dwRed     = (dwRed / (DWORD)wNumEntries) << (COLORDEPTH - BITDEPTH);
      dwRed    += (DWORD)wRedPop << (COLORDEPTH - BITDEPTH);
      dwRed   >>= 1;
      dwRed    += (DWORD)((lpColorBox->RedMin + lpColorBox->RedMax) << (COLORDEPTH - BITDEPTH)) / 2;
      dwRed   >>= 1;
      dwGreen   = (dwGreen / (DWORD)wNumEntries) << (COLORDEPTH - BITDEPTH);
      dwGreen  += (DWORD)(wGreenPop << (COLORDEPTH - BITDEPTH));
      dwGreen >>= 1;
      dwGreen  += (DWORD)((lpColorBox->GreenMin + lpColorBox->GreenMax) << (COLORDEPTH - BITDEPTH)) / 2;
      dwGreen >>= 1;
      dwBlue    = (dwBlue / (DWORD)wNumEntries) << (COLORDEPTH - BITDEPTH);
      dwBlue   += (DWORD)(wBluePop << (COLORDEPTH - BITDEPTH));
      dwBlue  >>= 1;
      dwBlue   += (DWORD)((lpColorBox->BlueMin + lpColorBox->BlueMax) << (COLORDEPTH - BITDEPTH)) / 2;
      dwBlue  >>= 1;
      *lpRed    = (BYTE)dwRed;
      *lpGreen  = (BYTE)dwGreen;
      *lpBlue   = (BYTE)dwBlue;
      break;
    case PAL_MEDMEANPOP:
      /*
      **  Popularity averaging: NOTE: potential for overflow
      */
      for (wRed = (WORD)lpColorBox->RedMin; wRed <= (WORD)lpColorBox->RedMax; wRed++){
        for (wGreen = (WORD)lpColorBox->GreenMin; wGreen <= (WORD)lpColorBox->GreenMax; wGreen++){
          for (wBlue = (WORD)lpColorBox->BlueMin; wBlue <= (WORD)lpColorBox->BlueMax; wBlue++){
            dwHistValueTemp = get_hist ((wRed << 10) + (wGreen << 5) + (wBlue));
            if (dwHistValueTemp){
              dwRed   += (DWORD)wRed   * dwHistValueTemp;
              dwGreen += (DWORD)wGreen * dwHistValueTemp;
              dwBlue  += (DWORD)wBlue  * dwHistValueTemp;
              if (dwHistValueTemp > dwHistValue){
                wRedPop   = wRed;
                wGreenPop = wGreen;
                wBluePop  = wBlue;
              }
            }
          }
        }
      }
      dwRed     = (dwRed   / lpColorBox->Total) << (COLORDEPTH - BITDEPTH);
      dwRed    += ((DWORD)(lpColorBox->RedMin + lpColorBox->RedMax) << (COLORDEPTH - BITDEPTH)) >> 1;
      dwRed    += (DWORD)wRedPop << (COLORDEPTH - BITDEPTH);
      dwRed    /= 3;
      dwGreen   = (dwGreen / lpColorBox->Total) << (COLORDEPTH - BITDEPTH);
      dwGreen  += ((DWORD)(lpColorBox->GreenMin + lpColorBox->GreenMax) << (COLORDEPTH - BITDEPTH)) >> 1;
      dwGreen  += (DWORD)wGreenPop << (COLORDEPTH - BITDEPTH);
      dwGreen  /= 3;
      dwBlue    = (dwBlue  / lpColorBox->Total) << (COLORDEPTH - BITDEPTH);
      dwBlue   += ((DWORD)(lpColorBox->BlueMin + lpColorBox->BlueMax) << (COLORDEPTH - BITDEPTH)) >> 1;
      dwBlue   += (DWORD)wBluePop << (COLORDEPTH - BITDEPTH);
      dwBlue   /= 3;
      *lpRed    = (BYTE)dwRed;
      *lpGreen  = (BYTE)dwGreen;
      *lpBlue   = (BYTE)dwBlue;
      break;
    case PAL_MEDMEANPOPAVG:
      /*
      **  Popularity averaging: NOTE: potential for overflow
      */
      for (wRed = (WORD)lpColorBox->RedMin; wRed <= (WORD)lpColorBox->RedMax; wRed++){
        for (wGreen = (WORD)lpColorBox->GreenMin; wGreen <= (WORD)lpColorBox->GreenMax; wGreen++){
          for (wBlue = (WORD)lpColorBox->BlueMin; wBlue <= (WORD)lpColorBox->BlueMax; wBlue++){
            dwHistValueTemp = get_hist ((wRed << 10) + (wGreen << 5) + (wBlue));
            if (dwHistValueTemp){
              wNumEntries++;
              dwRedAvg   += (DWORD)wRed;
              dwGreenAvg += (DWORD)wGreen;
              dwBlueAvg  += (DWORD)wBlue;
              dwRed      += (DWORD)wRed   * dwHistValueTemp;
              dwGreen    += (DWORD)wGreen * dwHistValueTemp;
              dwBlue     += (DWORD)wBlue  * dwHistValueTemp;
              if (dwHistValueTemp > dwHistValue){
                dwHistValue = dwHistValueTemp;
                wRedPop    = wRed;
                wGreenPop  = wGreen;
                wBluePop   = wBlue;
              }
            }
          }
        }
      }
      wRedPop   <<= (COLORDEPTH - BITDEPTH);
      wGreenPop <<= (COLORDEPTH - BITDEPTH);
      wBluePop  <<= (COLORDEPTH - BITDEPTH);

      dwRedAvg     = (dwRedAvg   / (DWORD)wNumEntries) << (COLORDEPTH - BITDEPTH);
      dwGreenAvg   = (dwGreenAvg / (DWORD)wNumEntries) << (COLORDEPTH - BITDEPTH);
      dwBlueAvg    = (dwBlueAvg  / (DWORD)wNumEntries) << (COLORDEPTH - BITDEPTH);

      dwRed        = (dwRed / lpColorBox->Total) << (COLORDEPTH - BITDEPTH);
      dwRed       += ((DWORD)(lpColorBox->RedMin + lpColorBox->RedMax) << (COLORDEPTH - BITDEPTH)) >> 1;
      dwRed       += dwRedAvg + (DWORD)wRedPop;
      dwRed      >>= 2;

      dwGreen      = (dwGreen / lpColorBox->Total) << (COLORDEPTH - BITDEPTH);
      dwGreen     += ((DWORD)(lpColorBox->GreenMin + lpColorBox->GreenMax) << (COLORDEPTH - BITDEPTH)) >> 1;
      dwGreen     += dwGreenAvg + (DWORD)wGreenPop;
      dwGreen    >>= 2;

      dwBlue       = (dwBlue / lpColorBox->Total) << (COLORDEPTH - BITDEPTH);
      dwBlue      += ((DWORD)(lpColorBox->BlueMin + lpColorBox->BlueMax) << (COLORDEPTH - BITDEPTH)) >> 1;
      dwBlue      += dwBlueAvg + (DWORD)wBluePop;
      dwBlue     >>= 2;

      *lpRed       = (BYTE)dwRed;
      *lpGreen     = (BYTE)dwGreen;
      *lpBlue      = (BYTE)dwBlue;
      break;
    case PAL_NEWMEDIAN:
      /*
      **  Pick color based on colorbox median. Average of color so that approx.
      **  half fall on either side of median
      */
      rgbColor     = GetColorBoxMedian (lpColorBox);
      wRed         = GetRValue(rgbColor);
      wRed        += ((WORD)lpColorBox->RedMin + (WORD)lpColorBox->RedMax) / 2;
      wRed        /= 2;
      wGreen       = GetGValue(rgbColor);
      wGreen      += ((WORD)lpColorBox->GreenMin + (WORD)lpColorBox->GreenMax) / 2;
      wGreen      /= 2;
      wBlue        = GetBValue(rgbColor);
      wBlue       += ((WORD)lpColorBox->BlueMin + (WORD)lpColorBox->BlueMax) / 2;
      *lpRed       = (BYTE)wRed;
      *lpGreen     = (BYTE)wGreen;
      *lpBlue      = (BYTE)wBlue;
      break;
    default:
      return (-1);
      break;
  }
  return (0);
}

/****************************************************************************
**
**  FUNCTION:     GetColorBoxMedain (LPCOLORBOX lpColorBox)
**
**  DESCRIPTION:  Get median of colors in a colorbox
**
***************************************************************************/

DWORD NEAR PASCAL GetColorBoxMedian (lpColorBox)
LPCOLORBOX lpColorBox;
{
  DWORD      Hist[BITLENGTH];
  DWORD *    lpHist;
  WORD       wRed;
  WORD       wGreen;
  WORD       wBlue;
  WORD       wRedValue;
  WORD       wGreenValue;
  WORD       wBlueValue;
  WORD       wRedMin;
  WORD       wRedMax;
  WORD       wGreenMin;
  WORD       wGreenMax;
  WORD       wBlueMin;
  WORD       wBlueMax;
  DWORD      dwTotalTemp;
  DWORD      dwTotal;
  WORD       w;
  /*
  **  Get Red median
  */
  dwTotal   = (DWORD)lpColorBox->Total / 2;
  wRedMin   = (WORD)lpColorBox->RedMin;
  wRedMax   = (WORD)lpColorBox->RedMax;
  wGreenMin = (WORD)lpColorBox->GreenMin;
  wGreenMax = (WORD)lpColorBox->GreenMax;
  wBlueMin  = (WORD)lpColorBox->BlueMin;
  wBlueMax  = (WORD)lpColorBox->BlueMax;
  /*
  **
  */
  lpHist = &Hist[wRedMin];
  for (wRed = wRedMin; wRed <= wRedMax; wRed++){
    *lpHist = 0;
    for (wGreen = wGreenMin; wGreen <= wGreenMax; wGreen++){
      for (wBlue = wBlueMin; wBlue <= wBlueMax; wBlue++){
        *lpHist += get_hist ((wRed << 10) + (wGreen << 5) + wBlue);
      }
    }
    lpHist++;            /*  Count all the counts - and keep counting */ 
  }
  /*
  **  Find red median point. Search forward
  */
  lpHist = &Hist[wRedMin];
  dwTotalTemp = 0;
  for (w = wRedMin; w <= wRedMax && (dwTotalTemp += (DWORD)*lpHist++) < dwTotal; w++);
  wRedValue = w << 3;
  /*
  **  Search backward
  */
  lpHist = &Hist[wRedMax];
  dwTotalTemp = 0;
  for (w = wRedMax; w >= wRedMin && (dwTotalTemp += (DWORD)*lpHist--) < dwTotal; w--);
  wRedValue  += w << 3;
  wRedValue >>= 1;
  /*
  **  Get Green median.
  */
  lpHist = &Hist[wGreenMin];
  for (wGreen = wGreenMin; wGreen <= wGreenMax; wGreen++){
    *lpHist = 0;
    for (wRed = wRedMin; wRed <= wRedMax; wRed++){
      for (wBlue = wBlueMin; wBlue <= wBlueMax; wBlue++){
        *lpHist += get_hist ((wRed << 10) + (wGreen << 5) + wBlue);
      }
    }
    lpHist++;
  }
  /*
  **  Find green median point. Search forward
  */
  lpHist = &Hist[wGreenMin];
  dwTotalTemp = 0;
  for (w = wGreenMin; w <= wGreenMax && (dwTotalTemp += (DWORD)*lpHist++) < dwTotal; w++);
  wGreenValue = w << 3;
  /*
  **  Search backward
  */
  lpHist = &Hist[wGreenMax];
  dwTotalTemp = 0;
  for (w = wGreenMax; w >= wGreenMin && (dwTotalTemp += (DWORD)*lpHist--) < dwTotal; w--);
  wGreenValue  += w << 3;
  wGreenValue >>= 1;
  /*
  **  Get Blue median
  */
  lpHist = &Hist[wBlueMin];
  for (wBlue = wBlueMin; wBlue <= wBlueMax; wBlue++){
    *lpHist = 0;
    for (wRed = wRedMin; wRed <= wRedMax; wRed++){       
      for (wGreen = wGreenMin; wGreen <= wGreenMax; wGreen++){
        *lpHist += get_hist ((wRed << 10) + (wGreen << 5) + wBlue);
      }
    }
    lpHist++;
  }
  /*
  **  Find blue median point. Search forward
  */
  lpHist = &Hist[wBlueMin];
  dwTotalTemp = 0;
  for (w = wBlueMin; w <= wBlueMax && (dwTotalTemp += (DWORD)*lpHist++) < dwTotal; w++);
  wBlueValue = w << 3;
  /*
  **  Search backward
  */
  lpHist = &Hist[wBlueMax];
  dwTotalTemp = 0;
  for (w = wBlueMax; w >= wBlueMin && (dwTotalTemp += (DWORD)*lpHist--) < dwTotal; w--);
  wBlueValue  += w << 3;
  wBlueValue >>= 1;
  /*
  **  Find median between colorbox median and median calculations
  */
  return (RGB((BYTE)wRedValue, (BYTE)wGreenValue, (BYTE)wBlueValue));
}

DWORD FAR PASCAL GetColorDistance (rgbColor1, rgbColor2)
DWORD rgbColor1;
DWORD rgbColor2;
{
  WORD  wRed;
  WORD  wGreen;
  WORD  wBlue;
  short nTemp;
  WORD  wDistance;
  DWORD dwDistance;
  /*
  **  Find cubic distance between two V3 vectors. Truncate to 6-bit space for
  **  color reduction.
  */
  nTemp      = (short)GetRValue(rgbColor1) >> 2;
  nTemp     -= (short)GetRValue(rgbColor2) >> 2;
  wRed       = nTemp < 0 ? (WORD)(-nTemp) : (WORD)nTemp;
  nTemp      = (short)GetGValue(rgbColor1) >> 2;
  nTemp     -= (short)GetGValue(rgbColor2) >> 2;
  wGreen     = nTemp < 0 ? (WORD)(-nTemp) : (WORD)nTemp;
  nTemp      = (short)GetBValue(rgbColor1) >> 2;
  nTemp     -= (short)GetBValue(rgbColor2) >> 2;
  wBlue      = nTemp < 0 ? (WORD)(-nTemp) : (WORD)nTemp;
  wDistance  = wRed   * wRed;
  wDistance += wGreen * wGreen;
  wDistance += wBlue  * wBlue;
  dwDistance = (DWORD)wDistance;
  return (dwDistance);
}
