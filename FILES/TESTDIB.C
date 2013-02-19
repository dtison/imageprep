/****************************************************************************

  COPYRIGHT:      Copyright (C) 1990, Computer Presentations, Inc.
                  All rights reserved.
  PROJECT:        Image Prep 3.0
  MODULE:         testdib.c
  PROCEDURES:     TestDIB ()       5.5.1
  DATE:           5/20/90
  DEVELOPER:      David W. Mize
*****************************************************************************/
#include <windows.h>
#include "imgprep.h"
#include "dibfmt.h"
#include "error.h"

/****************************************************************************
**  PROCEDURE:      TestWNDIB
**  DESCRIPTION:    Verifies if Passed handle is really a .BMP DIB file
**  DEFINITION:     5.5.1
**  START:          4/31/90  
**  DEVELOPER:      David W. Mize
**  MODS:               
*****************************************************************************/

BOOL TestWNDIB (
  HANDLE  hFile,
  LPSTR   lpBuffer)
{
  LPBITMAPFILEHEADER  lpBitmapFileHeader;    /* internal dib header pointer */
  LPBITMAPINFOHEADER  lpBitmapInfoHeader;
  /*
  **  Verify DIB file by reading BITMAPFILEHEADER and checking 1st WORD
  **  field for 'BM' signature
  */
  lpBitmapFileHeader = (LPBITMAPFILEHEADER)lpBuffer;
  if (_llseek (hFile, 0L, 0) != 0){
    return (FALSE);
  }
  if (_lread ((int)hFile, (LPSTR)lpBitmapFileHeader, 
    sizeof(BITMAPFILEHEADER)) != sizeof(BITMAPFILEHEADER))
    return (FALSE);
  if (lpBitmapFileHeader->bfType != 0x4D42)
    return (FALSE);
  /*
  **  Assume Windows 3.0 DIB and read BITMAPINFOHEADER. Find out whether 
  **  it is a Windows 3.0 or Presentation Manager DIB by checking first
  **  field, biSize to determine if it is the size of a BITMAPINFOHEADER
  **  or the size of a BITMAPCOREHEADER. Header fields are different sizes 
  **  and palettes are formatted differently. Windows 3.0 DIBs have RGBQUAD 
  **  palettes, while PM DIBs have RGBTRIPLE palettes.
  */
  lpBitmapInfoHeader = (LPBITMAPINFOHEADER)lpBuffer;
  if (_lread ((int)hFile, (LPSTR)lpBitmapInfoHeader, 
    sizeof(BITMAPINFOHEADER)) != sizeof(BITMAPINFOHEADER)){
    return (FALSE);
  }
  /*
  **  Determine DIB type
  */
  return (lpBitmapInfoHeader->biSize == sizeof(BITMAPINFOHEADER) ? TRUE :
    FALSE);
}

/****************************************************************************
**  PROCEDURE:      TestPMDIB
**  DESCRIPTION:    Verifies if Passed handle is really a .BMP DIB file
**  DEFINITION:     5.5.1
**  START:          4/31/90  
**  DEVELOPER:      David W. Mize
**  MODS:               
*****************************************************************************/

BOOL TestPMDIB (
  HANDLE    hFile,
  LPSTR     lpBuffer)
{
  LPBITMAPFILEHEADER  lpBitmapFileHeader;    /* internal dib header pointer */
  LPBITMAPCOREHEADER  lpBitmapCoreHeader;
  /*
  **  Verify DIB file by reading BITMAPFILEHEADER and checking 1st WORD
  **  field for 'BM' signature
  */
  lpBitmapFileHeader = (LPBITMAPFILEHEADER)lpBuffer;
  if (_llseek (hFile, 0L, 0) != 0)
    return (FALSE);
  if( _lread ((int)hFile, (LPSTR)lpBitmapFileHeader, 
    sizeof(BITMAPFILEHEADER)) != sizeof(BITMAPFILEHEADER))
    return (FALSE);
  if (lpBitmapFileHeader->bfType != 0x4D42)
    return (FALSE);
  /*
  **  Assume Windows 3.0 DIB and read BITMAPINFOHEADER. Find out whether 
  **  it is a Windows 3.0 or Presentation Manager DIB by checking first
  **  field, biSize to determine if it is the size of a BITMAPINFOHEADER
  **  or the size of a BITMAPCOREHEADER. Header fields are different sizes 
  **  and palettes are formatted differently. Windows 3.0 DIBs have RGBQUAD 
  **  palettes, while PM DIBs have RGBTRIPLE palettes.
  */
  lpBitmapCoreHeader = (LPBITMAPCOREHEADER)lpBuffer;
  if (_lread ((int)hFile, (LPSTR)lpBitmapCoreHeader, 
    sizeof(BITMAPCOREHEADER)) != sizeof(BITMAPCOREHEADER))
    return (FALSE);
  /*
  **  Determine DIB type
  */
  return( lpBitmapCoreHeader->bcSize == sizeof(BITMAPCOREHEADER) ? TRUE : 
    FALSE);
}
