/*---------------------------------------------------------------------------

    CPIFILE.C 
                
               

    CREATED:    2/91   D. Ison

--------------------------------------------------------------------------*/

/*  Undefs to expedite compilation */
  
  
#define  NOKANJI
#define  NOPROFILER
#define  NOSOUND
#define  NOCOMM
#define  NOGDICAPMASKS
#define  NOVIRTUALKEYCODES
#define  NOSYSMETRICS
#define  NOKEYSTATES   
#define  NOSYSCOMMANDS 
#define  NORASTEROPS   
#define  OEMRESOURCE   
#define  NOATOM       
#define  NOCLIPBOARD   
#define  NOCOLOR          
#define  NODRAWTEXT   
#define  NOMETAFILE      
#define  NOSCROLL        
#define  NOTEXTMETRIC     
#define  NOWH
#define  NOHELP           
#define  NODEFERWINDOWPOS 
  

#include <windows.h>
#include <cpi.h>      
#include <cpifmt.h>
#include <prometer.h>
#include <tools.h>
#include <io.h>
#include <string.h>
#include "internal.h"
#include "cpifile.h"
#include "global.h"

IMAGEINFO Images [2];   // One to read, one to write for now...

        
HANDLE  hInBuf;
HANDLE  hOutBuf;
HANDLE  hReformBuf;   
HANDLE  hAuxBuf;
HANDLE  hFlipBuf;
LPSTR   lpInBuf;      // For buffering input
LPSTR   lpOutBuf;     // For buffering output
LPSTR   lpReformBuf;  // For reformatting input
LPSTR   lpAuxBuf;     // For reformatting output
LPSTR   lpFlipBuf;    // For inverting scanlines
WORD    wReadBytes;   // ""
WORD    wWriteBytes;  // ""
WORD    wInByteCount  = 0;
WORD    wOutByteCount = 0;
WORD    wInByteOffset = 0;
DWORD   dwBytesToRead = 0L;

BYTE    bFlip   = 0;       // Flags that affect processing
BYTE    bMirror = 0;
BYTE    bIsLines = 0;

HIMAGE FAR OpenImage (lpPath, wCMAP)
LPSTR       lpPath;
WORD        wCMAP;
{
    OFSTRUCT        Of;
    int             hFile;
    CPIFILESPEC     CPIFileSpec;
    CPIIMAGESPEC    CPIImageSpec;
    HANDLE          hPalette;
    LPSTR           lpPalette;
    WORD            wImageType;

    hFile = OpenFile (lpPath, (LPOFSTRUCT)&Of, OF_READWRITE);
    if (hFile < 0)
        return (FALSE);

    /*  Go out and grab some memory  */

    hPalette  = GlobalAlloc (GHND, 768L);
    lpPalette = GlobalWire (hPalette);


    /*  Make sure the opened file is actually a CPI file  */

    if (! TestCPI (hFile, lpPalette))   // lpPalette is a temp buffer 
        return (FALSE);

    ReadCPIHdr (hFile, (LPCPIFILESPEC) &CPIFileSpec, (LPCPIIMAGESPEC) &CPIImageSpec);

    /*  Grab the palette, if applicable */

    if (CPIImageSpec.ImgType != CPIFMT_RGB)
    {
        _llseek (hFile, CPIImageSpec.PalDataOffset, 0);
        if (ReadFile (hFile, lpPalette, 768L) != 768L)
            return (FALSE);
    }

    _fmemset ((LPSTR) &Images [0], 0, sizeof (IMAGEINFO));

    Images [0].wScanWidth       = CPIImageSpec.X_Length;
    Images [0].wScanHeight      = CPIImageSpec.Y_Length;
    Images [0].wImageClass      = GetImageClass (CPIImageSpec.ImgType);
    Images [0].wBytesPerPixel   = (CPIImageSpec.BitsPerPixel >> 3);  // TEMP ONLY
    Images [0].wBytesPerRow     = Images [0].wScanWidth * Images [0].wBytesPerPixel;
    Images [0].wCMAP            = wCMAP;
    Images [0].hPalette         = hPalette;
    Images [0].lpPalette        = lpPalette;
    Images [0].wCPIImageType    = CPIImageSpec.ImgType;
    Images [0].wCPIBitsPerPixel = CPIImageSpec.BitsPerPixel;
    Images [0].wCPIBytesPerRow  = Images [0].wScanWidth * (CPIImageSpec.BitsPerPixel >> 3);  // TEMP
    Images [0].dwCPIPalOffset   = CPIImageSpec.PalDataOffset;
    Images [0].dwCPIImgOffset   = CPIImageSpec.ImgDataOffset;
    Images [0].hFile            = hFile;


    wImageType = Images [0].wCPIImageType;


    /*  Special case:  Colormap input (adjust bytes per row values if we
        will be de-indexing.)                                               */

    if (wImageType == CPIFMT_COLOR)
        if (wCMAP == CMAP_TO_24)
        {
            Images [0].wBytesPerRow   *= 3;
            Images [0].wBytesPerPixel *= 3;
        }

    /*  Special case:  1 bit mono input.  Set CPIBytesPerRow to appropriate value for ReadFile() call */

    if (wImageType == CPIFMT_MONO)
    {
        Images [0].wCPIBitsPerPixel = 1;
        Images [0].wCPIBytesPerRow  = (Images [0].wScanWidth + 31) / 32 * 4;    // DWORD aligned data
    }

    /*  Setup input buffer stuff  */
    {
        DWORD dwSize;

        dwSize = GetFreeSpace (0);

        if (dwSize > 65535L)
            dwSize = 65535L;

        if (dwSize < (DWORD) Images [0].wCPIBytesPerRow)  // Can we get at least 1 scanline ?
            return (FALSE);

        hInBuf = GlobalAlloc (GHND, dwSize);
        if (! hInBuf)
            return (FALSE);
        lpInBuf = GlobalWire (hInBuf);

        wReadBytes    = ((WORD) dwSize / Images [0].wCPIBytesPerRow * Images [0].wCPIBytesPerRow);
        wInByteCount  = 0;
        dwBytesToRead = (DWORD) (Images [0].wCPIBytesPerRow * (DWORD) Images [0].wScanHeight);

        if (wImageType == CPIFMT_RGB)
            if (CPIImageSpec.StorageType == CPIFMT_LINES)
                bIsLines = TRUE;

    }

    _llseek (hFile, CPIImageSpec.ImgDataOffset, 0);

    return (1);
}



HIMAGE FAR CreateImage (lpPath, hImage, wScanWidth, wScanHeight, wFlags)
LPSTR       lpPath;
HIMAGE      hImage;
WORD        wScanWidth;
WORD        wScanHeight;
WORD        wFlags;         // Flip / Mirror flags
{
    OFSTRUCT Of;
    int   hFile;
    CPIFILESPEC  CPIFileSpec;
    CPIIMAGESPEC  CPIImageSpec;
    WORD    wPaletteLen;
    BOOL    bAuxNeeded = FALSE;

    hImage--;       // Use for array index...

    hFile = OpenFile (lpPath, (LPOFSTRUCT)&Of, OF_CREATE | OF_READWRITE);
    if (hFile < 0)
        return (FALSE);

    if (wFlags & CI_FLIP)
        bFlip   = TRUE;
    if (wFlags & CI_MIRROR)
        bMirror = TRUE;

    /*  Copy structure data over  */

    _fmemcpy ((LPSTR) &Images [1], (LPSTR) &Images [hImage], sizeof (IMAGEINFO));
    Images [1].hFile             = hFile;

    InitCPIFileSpec ((LPCPIFILESPEC) &CPIFileSpec);
    _lwrite (hFile, (LPSTR) &CPIFileSpec, sizeof (CPIFILESPEC));

    InitCPIImageSpec ((LPCPIIMAGESPEC) &CPIImageSpec);

    /*  Setup width / height info */

    if (wScanWidth != NULL)
    {
        Images [1].wScanWidth  = wScanWidth;
        Images [1].wScanHeight = wScanHeight;
        Images [1].wCPIBytesPerRow  = wScanWidth * (Images [1].wCPIBitsPerPixel >> 3);  // TEMP
        Images [1].wBytesPerRow     = wScanWidth * Images [1].wBytesPerPixel;
    }


    /*  Special case:  Colormap input (adjust bytes per row values if we
        will be de-indexing.  This time on output file side)                */
  
    if (Images [1].wCPIImageType == CPIFMT_COLOR)
        if (Images [1].wCMAP == CMAP_TO_24)
        {
            Images [1].wCPIImageType = CPIFMT_RGB;
            Images [1].wCPIBytesPerRow  *= 3;   // Could also be Images[0].wBytesPerRow;....
            Images [1].wCPIBitsPerPixel = 24;
        }

    /*  Special case:  8 bit Mono input, 8 bit "gray" output  */
  
    if (Images [1].wCPIImageType == CPIFMT_BW || Images [1].wCPIImageType == CPIFMT_MONO)
    {
        Images [1].wCPIBytesPerRow  = Images [1].wScanWidth;
        Images [1].wCPIBitsPerPixel = 8;
        Images [1].wBytesPerPixel   = 1;
        Images [1].wBytesPerRow     = Images [1].wScanWidth;
        Images [1].wCPIImageType    = CPIFMT_BW;            // Always..
    }

    CPIImageSpec.ImgType        = Images [1].wCPIImageType;

    CPIImageSpec.ImgDescTag     = 1;
    CPIImageSpec.ImgSpecLength  = sizeof (CPIIMAGESPEC);
    CPIImageSpec.X_Length       = Images [1].wScanWidth;
    CPIImageSpec.Y_Length       = Images [1].wScanHeight;
    CPIImageSpec.BitsPerPixel   = (unsigned char) Images [1].wCPIBitsPerPixel;

    CPIImageSpec.NumberPlanes   = 1;
    CPIImageSpec.NumberColors   = 256;
    CPIImageSpec.ImgSize        = (DWORD)  ((DWORD) Images [1].wScanWidth * 
                                            (DWORD) Images [1].wScanHeight * (DWORD) (Images[1].wCPIBitsPerPixel >> 3));  // TEMP
    CPIImageSpec.NumberBitsPerEntry = 24;


    _lwrite (hFile, (LPSTR) &CPIImageSpec, sizeof (CPIIMAGESPEC));

    /*  Also stuff for palette... */

    wPaletteLen = (WORD) (Images[1].dwCPIImgOffset - Images[1].dwCPIPalOffset);

    if (wPaletteLen > 0)
        if (WriteFile (hFile, Images [1].lpPalette, (LONG) wPaletteLen) != (LONG) wPaletteLen)
            return (FALSE);

    /*  Let's allocate the AuxBuf, now that we have all the information that
        we will possibly need.                                             */

    if (Images[0].wCPIImageType == CPIFMT_BW)
        bAuxNeeded = TRUE;
    else
        if (Images[0].wCPIImageType == CPIFMT_MONO)
            bAuxNeeded = TRUE;
        else
            if (Images[0].wCPIImageType == CPIFMT_COLOR)
                bAuxNeeded = TRUE;
            else
                if (Images[0].wCPIImageType == CPIFMT_RGB || bIsLines)
                    bAuxNeeded = TRUE;

    if (bAuxNeeded)
    {
        WORD wSrcBytesPerRow = 0;
        WORD wSrcBytesPerCol = 0;
        WORD wDstBytesPerRow = 0;
        WORD wDstBytesPerCol = 0;
        WORD wBytesPerRow;
        WORD wBytesPerCol;

        WORD wAuxBytes;

      	wSrcBytesPerRow = Images [0].wBytesPerRow;
      	wDstBytesPerRow = Images [1].wBytesPerRow;

        // This is done in the case of a rotation because width/height get reversed..
        
        wSrcBytesPerCol = Images [0].wScanHeight * Images [0].wBytesPerPixel;
        wDstBytesPerCol = Images [1].wScanHeight * Images [1].wBytesPerPixel;

        wBytesPerRow = max (wSrcBytesPerRow, wDstBytesPerRow);
        wBytesPerCol = max (wSrcBytesPerCol, wDstBytesPerCol);

        /*  Then we will also need more memory for re-formatting  */

            wAuxBytes = max (wBytesPerRow, wBytesPerCol);
            hAuxBuf = GlobalAlloc (GHND, (DWORD) wAuxBytes);
            if (! hAuxBuf)
                return (FALSE);
            lpAuxBuf = GlobalWire (hAuxBuf);
    }

    /*  Now the output buffer stuff  */
    {
        DWORD dwSize;

        dwSize = GetFreeSpace (0);

        if (dwSize > 65535L)
            dwSize = 65535L;

        if (dwSize < (DWORD) Images [1].wCPIBytesPerRow)
            return (FALSE);

        hOutBuf = GlobalAlloc (GHND, dwSize);
        if (! hOutBuf)
            return (FALSE);
        lpOutBuf = GlobalWire (hOutBuf);

        wWriteBytes = ((WORD) dwSize / Images [1].wCPIBytesPerRow * Images [1].wCPIBytesPerRow);
        wOutByteCount = 0;
    }

    /*  And the flip buffer  */

    {
        DWORD dwSize;
        dwSize = tell (hFile);
        dwSize += (DWORD) ((DWORD) Images [1].wBytesPerRow * (DWORD) Images [1].wScanHeight);
        if (! CheckDiskSpace (hFile, dwSize))
            return (FALSE);

        if (bFlip)
        {
            /*  Get some "flip" memory  */

            hFlipBuf = GlobalAlloc (GHND, (DWORD) wWriteBytes);
            if (! hFlipBuf)
                return (FALSE);
            lpFlipBuf = GlobalWire (hFlipBuf);
            _llseek (hFile, dwSize, 0);
        }

        if (bMirror)  // For now, flip and mirror won't be on simultaneously
        {
            hFlipBuf = GlobalAlloc (GHND, (DWORD) wWriteBytes);
            if (! hFlipBuf)
                return (FALSE);
            lpFlipBuf = GlobalWire (hFlipBuf);
        }

    }
    return (2);
}

int FAR ReadLine (hImage, lpDest)
HIMAGE hImage;
LPSTR  lpDest;
{
    LPIMAGEINFO lpImageInfo;
    WORD  wNumBytes;

    lpImageInfo = &Images [hImage - 1];
    wNumBytes   = lpImageInfo -> wCPIBytesPerRow;

    if (wInByteCount == 0)  // Need some data
    {
        if (ReadFile (lpImageInfo -> hFile, lpInBuf, (LONG) wReadBytes) != (LONG) wReadBytes)
            if (dwBytesToRead > (DWORD) wReadBytes)
                return (FALSE);

        wInByteCount  = wReadBytes;
        wInByteOffset = 0;
        dwBytesToRead -= (DWORD) wReadBytes;
    }

    _fmemcpy (lpDest, (lpInBuf + wInByteOffset), wNumBytes);


    /*  Special case 1:  Colormapped images  */

    if (Images [0].wCPIImageType == CPIFMT_COLOR)  
        if (Images [0].wCMAP == CMAP_TO_24)     // Supposed to turn in into 24 bit ?
        {
            LPSTR   lpPalette;
            LPSTR   lpDestPtr;
            LPSTR   lpPtr;
            WORD    i;
            WORD    wScanWidth;
            int     nIndex;
    
            /*  Need to de-index palette */
    
            wScanWidth  = Images [0].wScanWidth;
            lpPalette   = Images [0].lpPalette;
            lpPtr       = lpAuxBuf;
            lpDestPtr   = lpDest;

            {
                DWORD dwAux;
                DWORD dwDest;

                dwAux  = GlobalSize (hAuxBuf);
                dwDest = GlobalSize (hInBuf);   
                dwDest--;
            }
    
            for (i = 0; i < wScanWidth; i++)
            {
                nIndex = (BYTE) *lpDestPtr++;
                nIndex *= 3;
                *lpPtr++ = (BYTE) (lpPalette [nIndex++] << 2);
                *lpPtr++ = (BYTE) (lpPalette [nIndex++] << 2);
                *lpPtr++ = (BYTE) (lpPalette [nIndex  ] << 2);
            }
            _fmemcpy (lpDest, lpAuxBuf, Images [0].wBytesPerRow);
        }


    /*  Special case 2:  BW Images  (8 bit)  */

    if (Images [0].wCPIImageType == CPIFMT_BW)
    {
        LPSTR   lpDestPtr;
        LPSTR   lpPtr;
        WORD    i;
        WORD    wScanWidth;

        wScanWidth  = Images [0].wScanWidth;
        lpPtr       = lpAuxBuf;
        lpDestPtr   = lpDest;

        for (i = 0; i < wScanWidth; i++)
            *lpPtr++ = (BYTE) (*lpDestPtr++ == 0 ? 0 : 255);

        _fmemcpy (lpDest, lpAuxBuf, wNumBytes);     // Using BytesPerRow produced GP fault, so this value is ok.
    }



    /*  Special case 3:  BW Images  (1 bit)  */

    if (Images [0].wCPIImageType == CPIFMT_MONO)
    {
        LPSTR lpSourcePtr;
        LPSTR lpDestPtr;
        WORD  i, j, k;
        WORD  wSourceIndex = 0;
        WORD  wDestIndex   = 0;
        BYTE  TmpVal;
        WORD  wCPIBytesPerRow;

        wCPIBytesPerRow = Images [0].wCPIBytesPerRow;

        /*  Reformat out to 8 bits per pixel */

        lpSourcePtr = lpDest;
        lpDestPtr   = lpAuxBuf;

            for (j = 0; j < wCPIBytesPerRow; j++)
            {

                TmpVal = (BYTE) *lpSourcePtr++;

                for (k = 0; k < 8; k++)
                {
                    *lpDestPtr++ = (BYTE) (((TmpVal & 0x80) >> 7) == 0 ? 0 : 255);
                    TmpVal <<= 1;
                }
            }

        _fmemcpy (lpDest, lpAuxBuf, Images [1].wBytesPerRow);     // Using wNumBytes was wrong because it refers to 1 bit/pixel not 8 bit/pixel.  This should be ok for all cases.
    }



    /*  Special case 4:  RGB Images  (Convert lines to triplets)  */

    if (Images [0].wCPIImageType == CPIFMT_RGB)
        if (bIsLines)
        {
            WORD wScanWidth  = Images [0].wScanWidth;

            LinesToTriplets (lpAuxBuf, lpDest, wScanWidth, 1);
            _fmemcpy (lpDest, lpAuxBuf, wNumBytes);     

        }

    wInByteCount  -= wNumBytes;
    wInByteOffset += wNumBytes;
    return (TRUE);

}



int FAR WriteLine (hImage, lpSource)
HIMAGE hImage;
LPSTR  lpSource;
{
    LPIMAGEINFO lpImageInfo;
    WORD  wNumBytes;

    lpImageInfo = &Images [hImage - 1];
    wNumBytes   = lpImageInfo -> wCPIBytesPerRow;

    if (wOutByteCount == wWriteBytes)  // Need to write the data
    {
        if (bFlip)
        {
            WORD wNumRows;

            wNumRows = (wWriteBytes / lpImageInfo -> wCPIBytesPerRow);
            _llseek (lpImageInfo -> hFile, -(DWORD) wWriteBytes, 1);
            InvertScanlines (lpFlipBuf, lpOutBuf, lpImageInfo -> wCPIBytesPerRow, wNumRows);
            WriteFile (lpImageInfo -> hFile, lpFlipBuf, (LONG) wWriteBytes);
            _llseek (lpImageInfo -> hFile, -(DWORD) wWriteBytes, 1);
        }
        else
            if (WriteFile (lpImageInfo -> hFile, lpOutBuf, (LONG) wWriteBytes) != (LONG) wWriteBytes)
                return (FALSE);

        wOutByteCount  = 0;
    }

    /*  Special case 1:  BW Images written from 8 bit gray  */

    if (Images [0].wCPIImageType == CPIFMT_BW)
    {
        LPSTR   lpSourcePtr;
        LPSTR   lpPtr;
        WORD    i;
        WORD    wScanWidth;

        wScanWidth  = Images [1].wScanWidth;
        lpPtr       = lpAuxBuf;
        lpSourcePtr = lpSource;

        for (i = 0; i < wScanWidth; i++)
            *lpPtr++ = (BYTE) ((BYTE) *lpSourcePtr++ > 128 ? 255 : 0);

        _fmemcpy (lpSource, lpAuxBuf, Images [1].wBytesPerRow);
    }

    if (bMirror)
    {
        LPSTR   lpSourcePtr;
        LPSTR   lpPtr;
        WORD    wScanWidth;
        WORD    i;
        WORD    wBytesPerPixel;

        wScanWidth  = Images [1].wScanWidth;
        lpPtr       = lpFlipBuf;
        lpSourcePtr = lpSource + Images [1].wBytesPerRow - 1;

        wBytesPerPixel = lpImageInfo -> wBytesPerPixel;

        for (i = 0; i < wScanWidth; i++)
            if (wBytesPerPixel == 1)
                *lpPtr++ = (BYTE) *lpSourcePtr--;
            else
            {
                lpPtr [2] = (BYTE) *lpSourcePtr--;
                lpPtr [1] = (BYTE) *lpSourcePtr--;
                lpPtr [0] = (BYTE) *lpSourcePtr--;

                lpPtr += 3;
            }

        _fmemcpy (lpSource, lpFlipBuf, Images [1].wBytesPerRow);
    }

    _fmemcpy ((lpOutBuf + wOutByteCount), lpSource, wNumBytes);
    wOutByteCount  += wNumBytes;
    return (TRUE);
}



int FAR GetImageInfo (hImage, lpImageInfo)
HIMAGE hImage;
LPIMAGEINFO lpImageInfo;
{
    _fmemcpy ((LPSTR) lpImageInfo, (LPSTR) &Images [hImage - 1], sizeof (IMAGEINFO));

    return (TRUE);
}

int FAR SetImageInfo (hImage, lpImageInfo)
HIMAGE      hImage;
LPIMAGEINFO lpImageInfo;
{

    hImage--;

    /*  Just transfer dimensions info for now  */

    Images [hImage].wScanWidth  = lpImageInfo -> wScanWidth;
    Images [hImage].wScanHeight = lpImageInfo -> wScanHeight;

    return (TRUE);
}


int FAR RewindImage (hImage)
HIMAGE      hImage;
{
    int nRetval = TRUE;
    hImage--;

    /*  Just lseek to top and setup buffering  */

    if (hImage == 0)
    {
        wInByteCount  = 0;
        wInByteOffset = 0;
        dwBytesToRead = (DWORD) (Images [0].wCPIBytesPerRow * (DWORD) Images [0].wScanHeight);
        _llseek (Images [0].hFile, Images [0].dwCPIImgOffset, 0);
    }
    else
        nRetval = FALSE;

    return (nRetval);
}



int FAR CloseImage (hImage, bHeaderFlag)
HIMAGE hImage;
BOOL   bHeaderFlag;
{
    CPIIMAGESPEC CPIImageSpec;

    hImage--;
    if (bHeaderFlag) // Need to write offset information and flush memory
    {
        if (wOutByteCount > 0)
            if (bFlip)
            {
                WORD wNumRows;

                wNumRows = (wOutByteCount / Images [hImage].wCPIBytesPerRow);
                _llseek (Images [hImage].hFile, -(DWORD) wOutByteCount, 1);

                InvertScanlines (lpFlipBuf, lpOutBuf, Images [hImage].wCPIBytesPerRow, wNumRows);
                WriteFile (Images [hImage].hFile, lpFlipBuf, (LONG) wOutByteCount);

                _llseek (Images [hImage].hFile, -(DWORD) wOutByteCount, 1);
            }
            else
                WriteFile (Images [hImage].hFile, lpOutBuf, (LONG) wOutByteCount);

        _llseek (Images [hImage].hFile, 64L, 0);
        _lread  (Images [hImage].hFile, (LPSTR) &CPIImageSpec, sizeof (CPIIMAGESPEC));

        CPIImageSpec.PalDataOffset = Images [hImage].dwCPIPalOffset;
        CPIImageSpec.ImgDataOffset = Images [hImage].dwCPIImgOffset;

        _llseek (Images [hImage].hFile, 64L, 0);
        _lwrite (Images [hImage].hFile, (LPSTR) &CPIImageSpec, sizeof (CPIIMAGESPEC));

        if (lpInBuf)
        {
            GlobalUnWire (hInBuf);
            lpInBuf = NULL;
        }
        if (hInBuf)
        {
            GlobalFree (hInBuf);
            hInBuf = NULL;
        }
        if (lpOutBuf)
        {
            GlobalUnWire (hOutBuf);
            lpOutBuf = NULL;
        }
        if (hOutBuf)
        {
            GlobalFree (hOutBuf);
            hOutBuf = NULL;
        }
        if (lpAuxBuf)
        {
            GlobalUnWire (hAuxBuf);
            lpAuxBuf = NULL;
        }	
        if (hAuxBuf)
        {
            GlobalFree (hAuxBuf);
            hAuxBuf = NULL;
        }
        if (lpFlipBuf)
        {
            GlobalUnWire (hFlipBuf);
            lpFlipBuf = NULL;
        }	
        if (hFlipBuf)
        {
            GlobalFree (hFlipBuf);
            hFlipBuf = NULL;
        }
        if (Images [0].lpPalette)
            GlobalUnWire (Images [0].hPalette);
            
        if (Images [0].hPalette)
            GlobalFree (Images[0].hPalette);

        wInByteCount  = 0;
        wOutByteCount = 0;
        wInByteOffset = 0;
        dwBytesToRead = 0L;

        bFlip   = FALSE;
        bMirror = FALSE;
        bIsLines = FALSE;
    }

    _lclose (Images [hImage].hFile);
    return (TRUE);
}




BOOL NEAR TestCPI (hFile, lpBuffer) 
int      hFile;
LPSTR    lpBuffer;
{
    BOOL             Retval = TRUE;
    LPCPIFILESPEC    lpFileSpecPtr;
    LONG             lBytes; 

    if (_llseek (hFile, 0L, 0))
        return (FALSE);
  
    lpFileSpecPtr = (LPCPIFILESPEC) lpBuffer;

  /*  Verify file first by reading it  */

    lBytes = ReadFile (hFile, (LPSTR)lpFileSpecPtr, sizeof (CPIFILESPEC));

    _llseek (hFile, 0L, 0);

    if (lBytes != (LONG) sizeof (CPIFILESPEC))
        Retval = FALSE;
    else
    {      
        /*  Verify header   */
        if (_fstrcmp (lpFileSpecPtr -> CPiSignature, (LPSTR) "CPI") != 0)
          Retval = FALSE;
    }

    return (Retval);
}

WORD NEAR GetImageClass (wImageType)
WORD wImageType;
{
    //  (nRetval is public)

    switch (wImageType)
    {
        case CPIFMT_RGB:
            nRetval = TRUECOLOR;
            break;

        case CPIFMT_COLOR:
            nRetval = COLORMAP;
            break;

        case CPIFMT_GRAY:
        case CPIFMT_OPTGRAY:
            nRetval = GRAYSCALE;
            break;

        case CPIFMT_BW:
        case CPIFMT_MONO:
            nRetval = MONOCHROME;
            break;
    }

    return (nRetval);
}





