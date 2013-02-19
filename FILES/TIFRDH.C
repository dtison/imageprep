#include <windows.h>
#include <cpi.h>
#include <imgprep.h>
#include <global.h> // Need to access some of ImagePrep's global data..
#include <strtable.h>   // And its resources.  How about making these local to the .DLL ?
#include <proto.h>
#include <tiffmt.h>
#include <error.h>

/*-----------------------------------------------------------------------------

   PROCEDURE:        TIFReadHeader
   DESCRIPTION:      File "can opener" for ImgPrep (TIF files)
   DEFINITION:       5.4.2
   START:            David Ison
   MODS:             1/8/90  David Ison        
                     10/16/90  1 bit tiff read
----------------------------------------------------------------------------*/

BOOL    bIsMotorola;

int FAR PASCAL TIFReadHeader (hFile, lpBitmapInfo, lpInputBuf, lpFileInfo)
int         hFile;
LPBITMAPINFO  lpBitmapInfo;
LPSTR       lpInputBuf;
LPFILEINFO  lpFileInfo;
{
    int           nRetval = 0;
    LPSTR         lpTmpBuf;
    RGBQUAD FAR * lpRGBQuadPtr;
    TIFFHEADER    TiffHeader;
    TIFFTAGS      TiffTags;
      
    if (_llseek ((int) hFile, 0L, 0))
       return (EC_RTIFLS);
      
      
    
    lpTmpBuf = (LPSTR) ((BYTE huge *) lpInputBuf + BUFOFFSET);
      
    lpRGBQuadPtr = (RGBQUAD FAR *) &lpBitmapInfo->bmiColors[0];
      
    /* Read TIF file header  */
    
    if (ReadTiffHeader (hFile, (LPTIFFHEADER) &TiffHeader, lpFileInfo) < 0)
    {
        return (EC_FILEREAD2);
    }
      
    /* Read TIF tags, filling out lpFileInfo and tif specific data */
      
    nRetval = ReadTiffTags (hFile, (LPTIFFTAGS) &TiffTags, lpTmpBuf, 
                            lpInputBuf, lpRGBQuadPtr, lpFileInfo);
    
    return (nRetval);
}   

/*----------------------------------------------------------------------------

   PROCEDURE:         ReadTiffHeader
   DESCRIPTION:       
   DEFINITION:        5.4.2.1
   START:             David Ison
   MODS:              

----------------------------------------------------------------------------*/

int ReadTiffHeader (hFile, lpTiffHeader, lpFileInfo)
int             hFile;
LPTIFFHEADER    lpTiffHeader;
LPFILEINFO      lpFileInfo;
{
    LPTIFFIXUP lpTIFFixup;
    WORD       wBytes;
    
    lpTIFFixup = (LPTIFFIXUP) ((LPSTR) lpFileInfo + sizeof (FILEINFO));
    
    wBytes = _lread (hFile, (LPSTR) lpTiffHeader, sizeof (TIFFHEADER));
    
    if (((int) wBytes == -1 ) || (wBytes != sizeof (TIFFHEADER)))
        return (EC_FILEREAD2);

    lpTIFFixup->dwIFDOffset = ByteOrderLong (&lpTiffHeader->IFDOffset, bIsMotorola);
    
    return (0);
}   

/*----------------------------------------------------------------------------

   PROCEDURE:        ReadTiffTags
   DESCRIPTION:       
   DEFINITION:       5.4.2.2
   START:            David Ison
   MODS:             1/8/90  David Ison        (To new spec)
                     7/10/90   Corrected "wPaddedScanWidth" mess

----------------------------------------------------------------------------*/

int ReadTiffTags (hFile, lpTiffTags, lpTmpBuf, lpPalette, lpRGBQuadPtr, lpFileInfo) 
int             hFile;
LPTIFFTAGS      lpTiffTags;  // NOT USED YET 
LPSTR           lpTmpBuf;
LPSTR           lpPalette;
RGBQUAD FAR *   lpRGBQuadPtr;
LPFILEINFO      lpFileInfo;
{
    char            Buffer [4];
    int             err;
    int             i;
    LPIFDSTRUCT     lpIFDPtr;
    LPTIFFIXUP      lpTIFFixup;
    WORD            wBytes; 
    WORD            wBytesPerPalEntry;
    WORD *          pWord;
    WORD            wNumIFDs;
    WORD            wSize;
    WORD            wBitsPerPixel;
    WORD FAR *      lpwRedPtr;
    WORD FAR *      lpwGreenPtr;
    WORD FAR *      lpwBluePtr;
    WORD            wTag;
    
    LPDISPINFO    lpDispInfo;
    
    
    lpTIFFixup = (LPTIFFIXUP) ((LPSTR) lpFileInfo + sizeof (FILEINFO));
    lpDispInfo  = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);
      
    if (_llseek ((int)hFile, (long)lpTIFFixup->dwIFDOffset, 0) == -1) 
        return (EC_FILEREAD1);
    
    /*  Initialize fields that will need to be tested later  */
      
    lpFileInfo -> wScanWidth        = 0;
    lpFileInfo -> wScanHeight       = 0;
    lpTIFFixup -> wBitsPerSample    = 0;
    lpTIFFixup -> wSamplesPerPixel  = 1;    // Assume default of 1 Sample / Pixel
      
    /*  Switch tags, fill in what you want, ignore what u dont, 
        check what u want afterwards and return error if not valid  */
      
    pWord  = (WORD *) Buffer;
    wBytes = _lread ((int) hFile, (LPSTR) pWord, 2);
    
    if (FailedLRead (wBytes, 2))
        return (EC_FILEREAD2);
    
    wNumIFDs = ByteOrderWord (*pWord, bIsMotorola);
    wSize    = wNumIFDs * sizeof (IFDSTRUCT);
    wBytes   = _lread (hFile, lpTmpBuf, wSize);
      
    if (FailedLRead (wBytes, wSize))
        return (EC_FILEREAD2);
      
    lpIFDPtr = (LPIFDSTRUCT) lpTmpBuf;
      
    /*--Tags we will be looking for: ------------------------------------
      
        DWORD  NewSubFileType;
        WORD   SubFileType;
        DWORD  ImageWidth;
        DWORD  ImageLength;
        WORD   BitsPerSample;
        WORD   Compression;
        WORD   PhotometricInterpretation;
        HANDLE StripOffsets;
        WORD   SamplesPerPixel;
        DWORD  RowsPerStrip;
        HANDLE StripByteCounts;
        HANDLE Colormap;                   
    --------------------------------------------------------------------*/
      
    for (i = 0; i < (int)wNumIFDs; i++)
    {
        wTag = ByteOrderWord (lpIFDPtr -> wTag, bIsMotorola);
        switch (wTag)
        {
            case 254:        // NEWSUBFILETYPE
                if ((err = NewSubFileType (hFile, lpIFDPtr, lpFileInfo)) < 0)
                    return (err);
                break;
            case 256:        // IMAGEWIDTH
                if ((err = ImageWidth (hFile, lpIFDPtr, lpFileInfo)) < 0)
                    return (err);
                break;
            case 257:        // IMAGELENGTH
                if ((err = ImageLength (hFile, lpIFDPtr, lpFileInfo)) < 0)
                    return (err);
                break;
            case 258:        // BITSPERSAMPLE
                if ((err = BitsPerSample (hFile, lpIFDPtr, lpFileInfo)) < 0)
                    return (err);
                break;
            case 259:        // COMPRESSION
                if ((err = Compression (hFile, lpIFDPtr, lpFileInfo)) < 0)
                    return (err);
                break;
            case 262:        // PHOTOMETRICINTERPRETATION
                if ((err = PhotometricInterpretation (hFile, lpIFDPtr, lpFileInfo)) < 0)
                    return (err);
                break;
            case 273:        // STRIPOFFSETS
                if ((err = StripOffsets (hFile, lpIFDPtr, lpFileInfo)) < 0)
                    return (err);
                break;
            case 277:        // SAMPLESPERPIXEL
                if ((err = SamplesPerPixel (hFile, lpIFDPtr, lpFileInfo))< 0)
                    return (err);
                break;
            case 278:        // ROWSPERSTRIP
                if ((err =  RowsPerStrip (hFile, lpIFDPtr, lpFileInfo)) < 0)
                    return (err);
                break;
            case 279:        // STRIPBYTECOUNTS
                if ((err =  StripByteCounts (hFile, lpIFDPtr, lpFileInfo)) < 0)
                    return (err);
                break;
            case 320:        // COLORMAP
                if ((err = Colormap (hFile, lpIFDPtr, lpFileInfo)) < 0)
                    return (err);
                break;
        }
        lpIFDPtr++;
    }
      
    /*  First test to see if we found all the important values  */
      
    if (lpFileInfo->wScanWidth == 0)
        return (EC_INVALIDIMAGE);
      
    if (lpFileInfo->wScanHeight == 0)
        return (EC_INVALIDIMAGE);
      
    if (lpTIFFixup->wBitsPerSample == 0)
        return (EC_INVALIDIMAGE);
      
    if (lpTIFFixup->wSamplesPerPixel != 1 && lpTIFFixup->wSamplesPerPixel != 3)
        return (EC_INVALIDIMAGE);

    /*  Now generate a grayscale (8 bit) palette if the image was grayscale */

    /*  Setup FILEINFO stuff  */
      
    wBitsPerPixel = lpTIFFixup->wBitsPerSample * lpTIFFixup->wSamplesPerPixel;
    
    lpFileInfo -> wBitsPerPixel       = wBitsPerPixel;


    lpFileInfo -> wBytesPerRow        = ((lpFileInfo->wScanWidth * wBitsPerPixel) + 7) >> 3;
    
    lpFileInfo -> wPaddedBytesPerRow  = (WORD) ((((DWORD) lpFileInfo -> wScanWidth * (DWORD) lpFileInfo -> wBitsPerPixel) + 31L) / 32L * 4L);

    lpDispInfo -> wPaddedScanWidth    = ((lpFileInfo -> wScanWidth * INTERNAL_BITS) + 31) / 32 * 4;
    lpDispInfo -> wPaddedBytesPerRow  = lpDispInfo -> wPaddedScanWidth;  // FOR NOW THEY ARE EQUAL BUT IN 24 BIT DISPLAY THIS WILL BE REFINED
    

    if (lpTIFFixup->bIsSingleStrip)
    {
        lpFileInfo->wRowsPerStrip     = 1;
        lpFileInfo->wRowsPerLastStrip = 1;
    }
    
    lpFileInfo->wBytesPerStrip      = lpFileInfo->wBytesPerRow * lpFileInfo->wRowsPerStrip;
    lpFileInfo->wBytesPerLastStrip  = lpFileInfo->wBytesPerRow * lpFileInfo->wRowsPerLastStrip;
    lpFileInfo->wNumStrips          = lpTIFFixup->wStripsPerImage;

    
    /*  If necessary, now calculate wNumStrips, wRowsPerLastStrip,  wBytesPerLastStrip  */
    
    if (! lpTIFFixup->bIsSingleStrip)
    {
        WORD wRowsPerLastStrip; 
        WORD wBytesPerLastStrip;
        WORD wRowCount;
    
        wRowCount  = lpFileInfo->wNumStrips * lpFileInfo->wRowsPerStrip;
    
        if (wRowCount > lpFileInfo->wScanHeight)
        {
            wRowsPerLastStrip   = lpFileInfo->wRowsPerStrip -
                                  (wRowCount - lpFileInfo->wScanHeight);
            wBytesPerLastStrip  = wRowsPerLastStrip * lpFileInfo->wBytesPerRow;
        }
        else
        {
            wBytesPerLastStrip = lpFileInfo->wBytesPerStrip;
            wRowsPerLastStrip  = lpFileInfo->wRowsPerStrip;
        }
    
        lpFileInfo->wRowsPerLastStrip   = wRowsPerLastStrip;
        lpFileInfo->wBytesPerLastStrip  = wBytesPerLastStrip;
    }
      
    if (lpFileInfo->wBitsPerPixel == 24)
    {
        SetState (IMPORT_CLASS, IRGB);
        lpTIFFixup -> wTiffClass = TIF_RGB;
    }
    
    if (lpFileInfo->wBitsPerPixel == 8) 
    {
        BOOL bIsPalette = FALSE;

        if (lpTIFFixup->wPhotometric == TIF_INVERTED || lpTIFFixup->wPhotometric == TIF_NORMAL)
        {
            SetState (IMPORT_CLASS, IGR);  
            lpTIFFixup -> wTiffClass = TIF_GRAYSCALE;
        }
        else
            if (lpTIFFixup->wPhotometric == TIF_PALETTE)
            {
                SetState (IMPORT_CLASS, ICM);  
                lpTIFFixup -> wTiffClass = TIF_PALETTE;
                bIsPalette = TRUE;
            }
            else
                return (EC_INVALIDIMAGE);

          /*  Other modules read the palette here, but the TIFF version 
            has already read the palette */ 
    

        if (bIsPalette)
        {
            if (! (lpPalette = GlobalLock (lpTIFFixup->hColormap)))
                return (EC_MEMORY1);

            /*  Reformat the palette to Windows 3.0  */ 
    
            lpwRedPtr    = (WORD FAR *)lpPalette;
            lpwGreenPtr  = lpwRedPtr   + (lpTIFFixup->wNumberColors);
            lpwBluePtr   = lpwGreenPtr + (lpTIFFixup->wNumberColors);
    
            for (i = 0; i < (int)lpTIFFixup->wNumberColors; i++) 
            {   
                lpRGBQuadPtr->rgbRed    = (BYTE) (*lpwRedPtr   >> 8);
                lpRGBQuadPtr->rgbGreen  = (BYTE) (*lpwGreenPtr >> 8);
                lpRGBQuadPtr->rgbBlue   = (BYTE) (*lpwBluePtr  >> 8);
    
                lpRGBQuadPtr++;
                lpwRedPtr++;
                lpwGreenPtr++;
                lpwBluePtr++;
            }
            GlobalUnlock (lpTIFFixup->hColormap);
        }

        #ifdef NEVER
        /*  Check for an 8 bit with strip size > 6826  */
        if (lpFileInfo -> wBytesPerStrip > 6826 && bIsPainting)  // Kludge "bIsPainting" test...
        {
            PSTR pStringBuf;
            PSTR pStringBuf2;

            pStringBuf  = LocalLock (hStringBuf);
            pStringBuf2 = pStringBuf + MAXSTRINGSIZE;
            LoadString (hInstIP, STR_TIF8_WARNING, (LPSTR) pStringBuf, MAXSTRINGSIZE);
            LoadString (hInstIP, STR_IMAGE_SIZE_WARNING, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
            MessageBox (hWndIP, (LPSTR) pStringBuf, (LPSTR) pStringBuf2, MB_OK);
            LocalUnlock (hStringBuf);
        }
        #endif

    }
    else
        wBytesPerPalEntry = 0;
    
    if (lpFileInfo->wBitsPerPixel == 1)
    {
        SetState (IMPORT_CLASS, IMON);
        lpTIFFixup -> wTiffClass = TIF_BILEVEL;

        /*  Correct BitsPerPixel (for MONO)  */
        lpFileInfo -> wBitsPerPixel = 8;
    }




    /*  Other modules "Point to image data" here; 
        All we do is set the wCurrentStrip counter  */
    
    lpTIFFixup->wCurrentStrip = 0;

// Last-minute Kludge!

    if (lpTIFFixup -> wTiffClass == TIF_BILEVEL)
    {
        lpFileInfo -> wBytesPerRow = (lpFileInfo -> wScanWidth + 7) / 8;
        lpFileInfo -> wPaddedBytesPerRow  = lpFileInfo -> wBytesPerRow;
    }

    lpTIFFixup->bIsFirstStrip = TRUE;
    
    return (TRUE);
}   


int ImageWidth (hFile, lpIFDPtr, lpFileInfo)
int            hFile;
LPIFDSTRUCT    lpIFDPtr;
LPFILEINFO     lpFileInfo;
{
    WORD    wSize;

    wSize = TiffSize (lpIFDPtr -> nType);

    if (wSize == 2)
        lpFileInfo -> wScanWidth = ByteOrderWord ((WORD) lpIFDPtr -> ValueOffset, bIsMotorola);
    else
        if (wSize == 4)
            lpFileInfo -> wScanWidth = (WORD) ByteOrderLong (&lpIFDPtr -> ValueOffset, bIsMotorola);

    return (0);
}

int ImageLength (hFile, lpIFDPtr, lpFileInfo)
int            hFile;
LPIFDSTRUCT    lpIFDPtr;
LPFILEINFO       lpFileInfo;
{
    WORD    wSize;

    wSize = TiffSize (lpIFDPtr -> nType);

    if (wSize == 2)
        lpFileInfo -> wScanHeight = ByteOrderWord ((WORD) lpIFDPtr->ValueOffset, bIsMotorola);
    else
        if (wSize == 4)
            lpFileInfo -> wScanHeight = (WORD) ByteOrderLong (&lpIFDPtr -> ValueOffset, bIsMotorola);

    return (0);
}


int NewSubFileType (hFile, lpIFDPtr, lpFileInfo)
int            hFile;
LPIFDSTRUCT    lpIFDPtr;
LPFILEINFO     lpFileInfo;
{
    return (0); 
}


int BitsPerSample (hFile, lpIFDPtr, lpFileInfo)
int           hFile;
LPIFDSTRUCT   lpIFDPtr;
LPFILEINFO    lpFileInfo;
{
    LPTIFFIXUP lpTIFFixup;
    WORD       wBitsPerSample;

    lpTIFFixup = (LPTIFFIXUP) ((LPSTR) lpFileInfo + sizeof (FILEINFO));
    
    if (ByteOrderLong (&lpIFDPtr->Length, bIsMotorola) == 1L)
        wBitsPerSample = ByteOrderWord ((WORD) lpIFDPtr->ValueOffset, bIsMotorola);
    else
    {
        DWORD      dwSeekPos;
        WORD       FAR *lpBitsPerSample;
        WORD       wReadSize;
        char       Buffer [20];           //  (For reading in BitsPerSample values) 
        WORD       wBytes;
    
        /*  Go find it the "hard" way  */
          
        wReadSize = (WORD) ByteOrderLong (&lpIFDPtr->Length, bIsMotorola) * (WORD) TiffSize (lpIFDPtr->nType);
        dwSeekPos = ByteOrderLong (&lpIFDPtr->ValueOffset, bIsMotorola);
    
        if (_llseek ((int) hFile, dwSeekPos, 0) != (LONG)dwSeekPos)
            return (EC_FILEREAD1);
          
        lpBitsPerSample = (WORD FAR *) Buffer;
    
        wBytes = _lread (hFile, (LPSTR) lpBitsPerSample, wReadSize);
        if (FailedLRead (wBytes, wReadSize))
            return (EC_MEMORY1);
    
        wBitsPerSample = ByteOrderWord (*lpBitsPerSample, bIsMotorola);
    }
    
    lpTIFFixup->wBitsPerSample  = wBitsPerSample;
    lpTIFFixup->wNumberColors   = (1 << wBitsPerSample);

    switch (lpTIFFixup->wBitsPerSample)
    {
        case 8:
        break;
    
        case 1:
        break;

        default:
        return (EC_UFILETYPE);
    }
    return (0);
}


int Compression (hFile, lpIFDPtr, lpFileInfo)
int            hFile;
LPIFDSTRUCT    lpIFDPtr;
LPFILEINFO     lpFileInfo;
{
    LPTIFFIXUP  lpTIFFixup;
    WORD        wSize;

    lpTIFFixup = (LPTIFFIXUP) ((LPSTR) lpFileInfo + sizeof (FILEINFO));

    wSize = TiffSize (lpIFDPtr -> nType);

    if (wSize == 2)
        lpTIFFixup -> wCompression = ByteOrderWord ((WORD) lpIFDPtr -> ValueOffset, bIsMotorola);
    else
        if (wSize == 4)
            lpTIFFixup -> wCompression = (WORD) ByteOrderLong (&lpIFDPtr -> ValueOffset, bIsMotorola);

    switch (lpTIFFixup -> wCompression)
    {
        case TIF_UNCOMPRESSED:
            break;

        default:
            return (EC_UNTIFCOMP);
    }
    return (0);
}

int PhotometricInterpretation (hFile, lpIFDPtr, lpFileInfo)
int           hFile;
LPIFDSTRUCT   lpIFDPtr;
LPFILEINFO    lpFileInfo;
{
    int           Retval = 0; 
    LPTIFFIXUP    lpTIFFixup;
    
    lpTIFFixup = (LPTIFFIXUP) ((LPSTR) lpFileInfo + sizeof (FILEINFO));
    
    lpTIFFixup -> wPhotometric = ByteOrderWord ((WORD) lpIFDPtr -> ValueOffset, bIsMotorola);

    switch (ByteOrderWord ((WORD) lpIFDPtr->ValueOffset, bIsMotorola))
    {
        case TIF_INVERTED:      // Inverted Grayscale (and BW)  (OK)
        break;

        case TIF_NORMAL:        // "Normal" Grayscale (BW)   (OK)
        break;

        case TIF_RGB:           //  RGB (OK)
        break;

        case TIF_PALETTE:       // Colormap (OK)
        break;

        default:                // Anything else (NOT OK)
            Retval = EC_INVALIDIMAGE;
    }

/*  PALETTE NOTES:

    The bIsColormap flag denoted the existance of a palette buffer for the 
    TIF import "fixup" routine not to GlobalFree an invalid handle.  Actually
    palettes are generated for photometric 3 AND grayscale class images.  (As
    well as 1 bit)   */

    return (Retval);
}


int StripOffsets (hFile, lpIFDPtr, lpFileInfo)  
int           hFile;
LPIFDSTRUCT   lpIFDPtr;
LPFILEINFO    lpFileInfo;
{
    DWORD FAR * lpStripOffsets;
    HANDLE      hStripOffsets;
    LPTIFFIXUP  lpTIFFixup;
    WORD        wSize;
    WORD        wBufferSize;
    WORD        wBytes;
    
    lpTIFFixup = (LPTIFFIXUP) ((LPSTR) lpFileInfo + sizeof (FILEINFO));
    
    if (ByteOrderLong (&lpIFDPtr->Length, bIsMotorola) == 1L)   // We have a *stupid* single strip tif.
    {
        lpTIFFixup->bIsSingleStrip      = TRUE;
        lpTIFFixup->wStripsPerImage     = lpFileInfo->wScanHeight;
        lpFileInfo->dwDataOffset        = ByteOrderLong (&lpIFDPtr  ->ValueOffset, bIsMotorola);
    }
    else
    {
        lpTIFFixup->bIsSingleStrip  = FALSE;
        lpTIFFixup->wStripsPerImage = (WORD) lpIFDPtr->Length;
            
        wSize = TiffSize (lpIFDPtr->nType);
    
        /*
        **  Need to keep track of size of fields, i.e., SHORT or LONG
        */

        lpTIFFixup->wStripOffsetSize = wSize;
    
        wBufferSize = lpTIFFixup->wStripsPerImage * wSize;
        
        /*  Need to allocate a memory handle here.  I am not comfortable with 
            this kind of architecture, but the TIFF specification seems to 
            necessitate it...  */
          
        if (! (hStripOffsets = GlobalAlloc (GHND, (long) wBufferSize)))
            return (EC_NOMEM);
          
        if ((lpStripOffsets = (DWORD FAR *) GlobalLock (hStripOffsets)) == 0)
        {
            GlobalFree (hStripOffsets);
            return (EC_NOMEM);
        }
          
        /*  Now have a buffer, seek to correct offset and read StripOffsets 
               from file  */ 
          
        if (_llseek ((int) hFile, (DWORD) lpIFDPtr->ValueOffset, 0) == -1)
           return ( EC_FILEREAD1);
          
        wBytes = _lread (hFile, (LPSTR) lpStripOffsets, wBufferSize);
          
        if (FailedLRead (wBytes, wBufferSize))
            return (EC_FILEREAD2);
          
        /*  Save the handle and free  */
          
        lpTIFFixup->hStripOffsets = hStripOffsets;
        GlobalUnlock (hStripOffsets);
    }
    return (0);
}   
    

int SamplesPerPixel (hFile, lpIFDPtr, lpFileInfo)
int            hFile;
LPIFDSTRUCT    lpIFDPtr;
LPFILEINFO     lpFileInfo;
{
    LPTIFFIXUP lpTIFFixup;
    WORD    wSize;

    wSize = TiffSize (lpIFDPtr -> nType);

    /*  If it ain't 1 or 3 samples/pixel, we don't fool with it  */

    if (wSize == 2)
    {
        if (ByteOrderWord ((WORD) lpIFDPtr->ValueOffset, bIsMotorola) != 1L && ByteOrderWord ((WORD) lpIFDPtr->ValueOffset, bIsMotorola) != 3)
            return (EC_INVALIDIMAGE);
    }
    else
    {
        if (ByteOrderLong (&lpIFDPtr->ValueOffset, bIsMotorola) != 1L && ByteOrderLong (&lpIFDPtr->ValueOffset, bIsMotorola) != 3)
            return (EC_INVALIDIMAGE);
    }

    lpTIFFixup = (LPTIFFIXUP) ((LPSTR) lpFileInfo + sizeof (FILEINFO));

    lpTIFFixup->wSamplesPerPixel = ByteOrderWord ((WORD) lpIFDPtr->ValueOffset, bIsMotorola);

    return (0);
}


int RowsPerStrip (hFile, lpIFDPtr, lpFileInfo)  
int            hFile;
LPIFDSTRUCT    lpIFDPtr;
LPFILEINFO     lpFileInfo;
{
    LPTIFFIXUP lpTIFFixup;

    lpTIFFixup = (LPTIFFIXUP) ((LPSTR) lpFileInfo + sizeof (FILEINFO));

    lpFileInfo->wRowsPerStrip   = ByteOrderWord ((WORD) lpIFDPtr->ValueOffset, bIsMotorola);
    lpFileInfo->wBytesPerStrip  = lpFileInfo->wRowsPerStrip * 
                                  lpFileInfo->wBytesPerRow;

    return (0);
}


int StripByteCounts (hFile, lpIFDPtr, lpFileInfo)  
int         hFile;
LPIFDSTRUCT lpIFDPtr;
LPFILEINFO  lpFileInfo;
{
    DWORD FAR * lpdwStripByteCounts;
    HANDLE      hStripByteCounts;
    LPTIFFIXUP  lpTIFFixup;
    WORD        wBytes;
    WORD        wSize;
    WORD        wBufferSize;
    
    lpTIFFixup = (LPTIFFIXUP) ((LPSTR) lpFileInfo + sizeof (FILEINFO));
    
    /*  If a 1 strip tiff, just skip this tag  */
    /*  Future:  Just look at the "Length" field.  If is 1, set direct */
    
    if (lpTIFFixup->bIsSingleStrip)
        return (0);
    
    wSize = TiffSize (lpIFDPtr->nType);
    
    /*  Need to keep track of size of fields, i.e., SHORT or LONG   */

    lpTIFFixup->wStripByteSize = wSize;
    
    wBufferSize = (WORD) ByteOrderLong (&lpIFDPtr->Length, bIsMotorola) * wSize;
    
    /*  Need to allocate a memory handle here.  I am not comfortable 
         with this kind of architecture, but the TIFF specification seems 
           to necessitate it...  */
    
    if (! (hStripByteCounts = GlobalAlloc (GHND, (long)wBufferSize)))
        return (EC_NOMEM);
    
    if ((lpdwStripByteCounts = (DWORD FAR *)GlobalLock (hStripByteCounts)) == 0)
    {
        GlobalFree (hStripByteCounts);
        return (EC_NOMEM);
    }
    
    /*  Now have a buffer, seek to correct offset and read from file  */ 
    
    if (_llseek ((int) hFile, (DWORD) ByteOrderLong (&lpIFDPtr->ValueOffset, bIsMotorola), 0) == -1)
        return (EC_FILEREAD1);
    
    wBytes = _lread (hFile, (LPSTR) lpdwStripByteCounts, wBufferSize);
    
    if ((int) wBytes == -1  || wBytes != wBufferSize)
        return (EC_FILEREAD2);
    
    lpFileInfo->wBytesPerStrip = (WORD) ByteOrderLong (lpdwStripByteCounts, bIsMotorola);
    
    GlobalUnlock (hStripByteCounts);
    
    /*  Save the handle  */
    
    lpTIFFixup->hStripByteCounts = hStripByteCounts;
    
      // New max image size...
    if (lpFileInfo->wBytesPerStrip > MAXBUFSIZE)
      return (EC_BIGSTRIPS);
    
    return (0);
}   

int Colormap (hFile, lpIFDPtr, lpFileInfo)  
int           hFile;
LPIFDSTRUCT   lpIFDPtr;
LPFILEINFO    lpFileInfo;
{
    HANDLE      hColormap;
    LPSTR       lpColormap;
    LPTIFFIXUP  lpTIFFixup;
    WORD        wBufferSize;
    WORD        wSize;
    WORD        wBytes;
    
    lpTIFFixup = (LPTIFFIXUP) ((LPSTR) lpFileInfo + sizeof (FILEINFO));
    
    wSize = TiffSize (lpIFDPtr->nType);
    
    wBufferSize = (lpTIFFixup->wNumberColors * 3) * wSize;
    
    /*  Need to allocate a memory handle here.  I am not comfortable 
         with this kind of architecture, but the TIFF specification seems 
           to necessitate it...  */
    
    if (! (hColormap = GlobalAlloc (GHND, (long) wBufferSize)))
        return (EC_NOMEM);
    
    if ((lpColormap = GlobalLock (hColormap)) == 0)
    {
        GlobalFree (hColormap);
        return (EC_NOMEM);
    }
    
    /*  Now have a buffer, seek to correct offset and read from file  */ 
                                
    if (_llseek ((int) hFile, (DWORD) ByteOrderLong (&lpIFDPtr->ValueOffset, bIsMotorola), 0) == -1)
    {
        GlobalUnlock (hColormap);
        GlobalFree (hColormap);
        return (EC_FILEREAD1);
    }
    
    wBytes = _lread (hFile, (LPSTR) lpColormap, wBufferSize);
    
    if ((int) wBytes == -1  || wBytes != wBufferSize)
    {
        GlobalUnlock (hColormap);
        GlobalFree (hColormap);
        return (EC_FILEREAD2);
    }
    
    GlobalUnlock (hColormap);
    
    /*  Save the handle  */
    
    lpTIFFixup->hColormap = hColormap;
    return (0);
}   


int TiffSize (nType)
int nType; 
{
    int nRetval;
    
    nType = ByteOrderWord (nType, bIsMotorola);

    switch (nType)
    {
        case IFDBYTE:
          nRetval = IFDBYTESIZE;
        break;
    
        case IFDASCII:
          nRetval = IFDASCIISIZE;
        break;
    
        case IFDSHORT: 
          nRetval = IFDSHORTSIZE;
        break;
    
        case IFDLONG:
          nRetval = IFDLONGSIZE;
        break;
    
        case IFDRATIONAL:
          nRetval = IFDRATIONALSIZE;
        break;
    }
    return (nRetval);
}

#ifdef NEVER   
/*------------------------------------------------------------------------------

   PROCEDURE:         GetTIFGrayPal
   DESCRIPTION:       
   DEFINITION:         5.4.2.15
   START:              David Ison
   MODS:               1/8/90  David Ison        (To new spec)

----------------------------------------------------------------------------*/
int GetTIFGrayPal (lpTIFFixup) 
LPTIFFIXUP  lpTIFFixup;
{
    HANDLE         hColormap;
    int            i;
    LPSTR          lpColormap;
    WORD           wBufferSize;
    WORD FAR *     lpRedPtr;
    WORD FAR *     lpGreenPtr;
    WORD FAR *     lpBluePtr;
    
    wBufferSize = 1536;
    
    /*  Need to allocate a memory handle here.  I am not comfortable with 
         this kind of architecture, but the TIFF specification seems to 
           necessitate it...  */
    
    if (! (hColormap = GlobalAlloc (GHND, (long)wBufferSize)))
        return (FALSE);
    
    if (! (lpColormap = GlobalLock (hColormap)))
    {
        GlobalFree (hColormap);
        return (FALSE);
    }
    
    /*  Now have a buffer, place in grayscale palette   */ 
    
    lpRedPtr    = (WORD FAR *)lpColormap;
    lpGreenPtr  = lpRedPtr + 256;  // (256 entries is 2 * sizeof (WORD) = 512 bytes)
    lpBluePtr   = lpGreenPtr + 256;
    
    for (i = 0; i < 256; i++) 
    {
        *lpRedPtr++   = i << 8; 
        *lpGreenPtr++ = i << 8; 
        *lpBluePtr++  = i << 8; 
    }
    
    GlobalUnlock (hColormap);    
    
    /*  Save the handle  */
    
    lpTIFFixup->wNumberColors = 256;
    lpTIFFixup->hColormap     = hColormap;
    return (TRUE);
}   

#endif


WORD FAR ByteOrderWord (wValue, bIsMotorola)
WORD   wValue;
BOOL   bIsMotorola;
{
    WORD wRetval;

    if (! bIsMotorola)
        wRetval = wValue;
    else
    {
        // Swap bytes

        wRetval = LOBYTE (wValue);
        wRetval <<= 8;
        wRetval |= HIBYTE (wValue);
    }

    return (wRetval);
}


DWORD FAR ByteOrderLong (dwValue, bIsMotorola)
DWORD   FAR *dwValue;
BOOL   bIsMotorola;
{
    DWORD dwRetval;
    LPSTR  pBytes1;
    LPSTR  pBytes2;
    BYTE  bTmp;

    dwRetval = *dwValue;

    if (! bIsMotorola)
        return (dwRetval);
    else
    {
        // Swap bytes

        pBytes1 = (LPSTR) &dwRetval;
        pBytes2 = (LPSTR) &dwRetval + 3;

        bTmp     = *pBytes1;
        *pBytes1 = *pBytes2;
        *pBytes2 = bTmp;

        pBytes1++;
        pBytes2--;

        bTmp     = *pBytes1;
        *pBytes1 = *pBytes2;
        *pBytes2 = bTmp;


    }
    return (dwRetval);
}



