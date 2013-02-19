
#include <windows.h>
#include <imgprep.h>
#include <proto.h>
#include <tgafmt.h>
#include <error.h>
#include <global.h>
#include <io.h>
#include <cpi.h>
#include <memory.h>
#include <prometer.h>
#include <fmtproto.h>
#include <strtable.h>

/*---------------------------------------------------------------------------

  PROCEDURE:      TGAReadHeader
  DESCRIPTION:    Message box procedure for ImgPrep
  DEFINITION:     5.4.4
  START:          David Ison
  MODS:           1/10/90  TBJ
                  2/06/90  David Ison (Read upside down)
                  2/07/90  David Ison
                  3/08/90  David Ison (Fix Targa 16 bug)
                  7/10/90   Corrected "wPaddedScanWidth" mess  D. Ison
                  5/10/91  Read RLE   D. Ison

-----------------------------------------------------------------------------*/

int  NEAR ExpStart (int, int);
void NEAR ExpRead  (LPSTR, int);
void NEAR ExpStop  (void);

int FAR DecodeRLE (int, int, LPTARGAHEADER, LPFILEINFO);

int FAR PASCAL TGAReadHeader (hFile, lpBitmapInfo, lpInputBuf, lpFileInfo)
int             hFile;
LPBITMAPINFO    lpBitmapInfo;
LPSTR           lpInputBuf;
LPFILEINFO      lpFileInfo;
{
    LPBITMAPINFOHEADER  lpbmiHeader; 
    LPSTR               lpTmpBuf;
    LPTGAFIXUP          lpTGAFixup;
    RGBQUAD FAR *       lpRGBQuadPtr;
    TARGAHEADER         TargaHeader;
    WORD                wRowsPerStrip;
    WORD                wBytesPerStrip;
    WORD                wRowsPerLastStrip;
    WORD                wBytesPerLastStrip;
    WORD                wNumStrips;
    WORD                wRowCount;
    WORD                wInputBytesPerRow;
    DWORD               dwTmp;
    int                 hReadFile = hFile;

    if (_llseek (hFile , 0L, 0) != 0L)
        return (EC_MEMORY1);
  
    /*  LOCK INTERNAL POINTERS  */
  
    lpTmpBuf = (LPSTR) ((BYTE huge *) lpInputBuf + BUFOFFSET);
  
    lpRGBQuadPtr = (RGBQUAD FAR *) &lpBitmapInfo ->bmiColors[0];
  
    lpTGAFixup   = (LPTGAFIXUP) ((LPSTR) lpFileInfo + sizeof (FILEINFO));
  
    if (! ReadTargaHeader (hFile, (TARGAHEADER FAR *)&TargaHeader, lpTmpBuf, lpRGBQuadPtr, lpFileInfo))
        return (EC_FILEREAD2);
   
    /*  Setup the file buffering stuff */
  
    if (lpFileInfo->wBitsPerPixel > 8)
    {
        if (lpTGAFixup->bIsTGA16)
            wInputBytesPerRow = 2;
        else
            if (lpTGAFixup->bIsTGA32)
                wInputBytesPerRow = 4;
            else
                wInputBytesPerRow = 3;         
  
        wInputBytesPerRow *= lpFileInfo->wScanWidth;
    }
    else
        wInputBytesPerRow = lpFileInfo->wBytesPerRow;


    /*  Setup buffering stuff for read                     */ 
    {
        WORD wStripSize;

        // Set up max strip size

        switch (wImportClass)
        {
            case IMON:
                wStripSize = MAXBUFSIZE >> 2;
                break;

            case ICM:
            case IGR:
                wStripSize = MAXBUFSIZE / 3;
                break;

            case IRGB:
                wStripSize = MAXBUFSIZE;
                break;
        }
        wRowsPerStrip = wStripSize / lpFileInfo -> wBytesPerRow;
    }

    // Special case: RLE

    if (lpTGAFixup -> bIsRLE)
    {
        LPSTR  lpPath;
        int hTmpFile;
        OFSTRUCT Of;
        int nRetval;

        /*  First make the header look appropriate, then open up temp file (Create one if we don't have one) */
            
        TargaHeader.IDLength   = 0;
        TargaHeader.ImageType -= 8;
        lpTGAFixup -> CMLength = 0;
        lpTGAFixup -> wBytesPerPalEntry = 0;
        lpTGAFixup -> wHeaderSize = sizeof (TARGAHEADER);

        if (! lpTGAFixup -> hRLEFileBuffer)  // Need to create the temp file ?
        {
            HANDLE hRLEPath;
            DWORD  dwBytesNeeded;

            hRLEPath = LocalAlloc (LHND, MAXSTRINGSIZE);
            if (! hRLEPath)
                return (EC_NOMEM);

            lpPath = (LPSTR) LocalLock (hRLEPath);

            GetTempFileName (0, (LPSTR) "TGA", 0, lpPath);

            /*  Need to test disk space on TEMP drive  */

            dwBytesNeeded = (DWORD) wInputBytesPerRow * (DWORD) lpFileInfo -> wScanHeight;
            if (! IsDiskSpaceAvailable (lpPath, dwBytesNeeded))
                return (EC_NODISK);


            /*  Create a whole new tga file, with header and everything.  Plan to read from this file instead of compressed file  */
            
            hTmpFile = OpenFile (lpPath, (LPOFSTRUCT) &Of, OF_CREATE | OF_READWRITE);
            if (hTmpFile <= 0)
                return (EC_FILEREAD2);

            if (WriteFile (hTmpFile, (LPSTR) &TargaHeader, (LONG) sizeof (TargaHeader)) != (LONG) sizeof (TARGAHEADER))
                return (EC_FILEWRITE1);
            
            /*  Decode image  */
            
            nRetval = DecodeRLE (hTmpFile, hReadFile, (LPTARGAHEADER) &TargaHeader, lpFileInfo);
            
            lpTGAFixup -> hDecompressedFile = hTmpFile;
            lpTGAFixup -> hRLEFileBuffer = hRLEPath;
            lpFileInfo -> lpfnFlushRead  = MakeProcInstance (TGAFlushRead, hInstIP);
            LocalUnlock (lpTGAFixup -> hRLEFileBuffer);

            if (nRetval < 0)  // Error on RLE expand
            {
                _lclose  (hTmpFile);
                OpenFile (lpPath, (LPOFSTRUCT) &Of, OF_DELETE);
                return (nRetval);
            }
        }
        else
            if (! lpTGAFixup -> hDecompressedFile)
            {
                lpPath = (LPSTR) LocalLock (lpTGAFixup -> hRLEFileBuffer);
                hTmpFile = OpenFile (lpPath, (LPOFSTRUCT) &Of, OF_READWRITE);
                lpTGAFixup -> hDecompressedFile = hTmpFile;
                LocalUnlock (lpTGAFixup -> hRLEFileBuffer);
            }

            hReadFile = lpTGAFixup -> hDecompressedFile;
            _llseek (hReadFile, (LONG) sizeof (TARGAHEADER), 0);
    }

    if (wRowsPerStrip > lpFileInfo->wScanHeight)
        wRowsPerStrip = lpFileInfo->wScanHeight;
  
    wBytesPerStrip  = wRowsPerStrip * lpFileInfo -> wBytesPerRow;
    wNumStrips      = lpFileInfo->wScanHeight / wRowsPerStrip;
    wRowCount       = wNumStrips * wRowsPerStrip;
  
    if (wRowCount < lpFileInfo->wScanHeight)
    {
        wBytesPerLastStrip  = (lpFileInfo->wScanHeight - wRowCount) * 
                               lpFileInfo -> wBytesPerRow;
  
        wRowsPerLastStrip   = (lpFileInfo->wScanHeight - wRowCount);
        wNumStrips++;
    }
    else
    {
        wBytesPerLastStrip = wBytesPerStrip;
        wRowsPerLastStrip  = wRowsPerStrip;
    }
  
    lpFileInfo->wNumStrips          = wNumStrips;
    lpFileInfo->wBytesPerStrip      = wBytesPerStrip;
    lpFileInfo->wRowsPerStrip       = wRowsPerStrip;
    lpFileInfo->wBytesPerLastStrip  = wBytesPerLastStrip;
    lpFileInfo->wRowsPerLastStrip   = wRowsPerLastStrip;
  
    lpTGAFixup -> wInBytesPerStrip      = wInputBytesPerRow * wRowsPerStrip;
    lpTGAFixup -> wInBytesPerLastStrip  = wInputBytesPerRow * wRowsPerLastStrip;
  
  
     /*  Point to image data (This was moved out of ReadTargaHeader() because
         wBytesPerStrip was unknown.  The purpose:  Reading file backwards.  D. Ison */

    dwTmp = tell (hReadFile);

    lpTGAFixup -> wHeaderSize       = (WORD) dwTmp;
    lpFileInfo -> dwDataOffset      =             
    lpTGAFixup -> dwNextDataOffset  = FindTGAData (hReadFile, lpFileInfo, lpTGAFixup -> wHeaderSize);

    lpbmiHeader = &lpBitmapInfo->bmiHeader;
  
    lpbmiHeader->biSize     = (DWORD) sizeof (BITMAPINFOHEADER);
    lpbmiHeader->biWidth    = (DWORD) lpFileInfo->wScanWidth;
    lpbmiHeader->biHeight   = (DWORD) lpFileInfo->wRowsPerStrip;
    lpbmiHeader->biPlanes   = 1;
    lpbmiHeader->biBitCount = lpFileInfo->wBitsPerPixel;
  
    lpbmiHeader->biCompression    = 0;   
    lpbmiHeader->biSizeImage      = 0;
    lpbmiHeader->biXPelsPerMeter  = 0;
    lpbmiHeader->biYPelsPerMeter  = 0;
    lpbmiHeader->biClrUsed        = 0;
    lpbmiHeader->biClrImportant   = 0;
   
    return (0);
}
  
/*---------------------------------------------------------------------------

  PROCEDURE:      ReadTargaHeader
  DESCRIPTION:    
  DEFINITION:     
  START:          David Ison
  MODS:           1/10/90  Tom Bagford Jr. (to new spec)
                  2/06/90  David Ison - File flipping
                  5/10/91  RLE Read  - D. Ison
-----------------------------------------------------------------------------*/

BOOL ReadTargaHeader (hFile, TgaHdr, TGAPalette, lpRGBQuadPtr, lpFileInfo)
int               hFile;
TARGAHEADER FAR * TgaHdr;
LPSTR             TGAPalette;
RGBQUAD FAR *     lpRGBQuadPtr;
LPFILEINFO        lpFileInfo;
{
    DWORD       dwSeekVal; 
    int         i;
    LPSTR       TmpPtr;
    LPTGAFIXUP  lpTGAFixup;
    WORD        wBytesPerPalEntry;
    WORD        bytes;
    WORD        wReadWriteVal;
    LPDISPINFO    lpDispInfo;

    lpTGAFixup  = (LPTGAFIXUP) ((LPSTR) lpFileInfo + sizeof (FILEINFO));
    lpDispInfo  = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);
  
    bytes =  _lread (hFile, (LPSTR) TgaHdr, sizeof (TARGAHEADER));
    if (FailedLRead( bytes, sizeof( TARGAHEADER)))
       return(  FALSE );
  
    /*  Setup stuff for handling 16 to 24 bit conversion  */
  
    lpTGAFixup -> bIsTGA16 = FALSE;
    lpTGAFixup -> bIsTGA32 = FALSE;
  
    if (TgaHdr->PixelDepth > 8)
    {
        if (TgaHdr->PixelDepth == 16)
            lpTGAFixup->bIsTGA16 = TRUE;
        else
            if (TgaHdr->PixelDepth == 32)
                lpTGAFixup->bIsTGA32 = TRUE;
            else
                TgaHdr->PixelDepth = 24;
    }
  
    /*  Setup public input image descriptor values  */
  
    lpFileInfo->wScanWidth    = TgaHdr->ImageWidth; 
    lpFileInfo->wScanHeight   = TgaHdr->ImageHeight;
    lpFileInfo->wBitsPerPixel = TgaHdr->PixelDepth; 

    if (lpFileInfo -> wBitsPerPixel > 8)
        lpFileInfo -> wBitsPerPixel = 24;


    lpFileInfo->wBytesPerRow  = lpFileInfo->wScanWidth * 
                                (lpFileInfo->wBitsPerPixel >> 3);


    /* (DWORD ALIGN) */
  
    lpFileInfo -> wPaddedBytesPerRow  = (WORD) ((((DWORD) lpFileInfo -> wScanWidth * (DWORD) lpFileInfo -> wBitsPerPixel) + 31L) / 32L * 4L);

    lpDispInfo->wPaddedScanWidth    = ((lpFileInfo -> wScanWidth * INTERNAL_BITS) + 31) / 32 * 4;

    lpDispInfo->wPaddedBytesPerRow  = lpDispInfo -> wPaddedScanWidth;  // FOR NOW THEY ARE EQUAL BUT IN 24 BIT DISPLAY THIS WILL BE REFINED
                                       ( lpFileInfo->wBitsPerPixel >> 3);
  
    /*  See if this image is upside down or rightside up  */
  
    lpTGAFixup -> bIsInverted = (! ((TgaHdr -> ImageDesc & 0x20) >> 5));  //TEST ONLY!
  
  
    /*  Set up some other data that we will need later  */
  
    lpTGAFixup -> IDLength = TgaHdr -> IDLength;
    lpTGAFixup -> CMLength = TgaHdr -> CMLength;
  
    if (lpFileInfo->wBitsPerPixel == 8) 
        SetState (IMPORT_CLASS, ICM);
    else
        SetState (IMPORT_CLASS, IRGB);
  
    if (TgaHdr->CMLength > 0)
    {
        /*  Setup and read palette, if applicable  */
  
        switch( TgaHdr -> CMEntrySize)
        {
            case 15:
            case 16:
                wBytesPerPalEntry = 2;
            break;
      
            case 24:
                wBytesPerPalEntry = 3;
            break;
      
            case 32:
                wBytesPerPalEntry = 4;
            break;
        }
  
        dwSeekVal = (DWORD) (sizeof (TARGAHEADER) + TgaHdr -> IDLength);
  
        if (_llseek (hFile, dwSeekVal, 0) != (LONG)dwSeekVal)
            return (FALSE);
  
        wReadWriteVal = (wBytesPerPalEntry * TgaHdr->CMLength);
        bytes         = _lread (hFile, TGAPalette, wReadWriteVal); 
        if (FailedLRead (bytes, wReadWriteVal))
              return (FALSE);
  
        /*  Reformat the palette to Windows 3.0  */ 
  
        TmpPtr = TGAPalette;
        for (i = 0; i < TgaHdr->CMLength; i++)  // THIS IS 8 BIT only!
        {   
            lpRGBQuadPtr->rgbBlue   = *TmpPtr++;
            lpRGBQuadPtr->rgbGreen  = *TmpPtr++;
            lpRGBQuadPtr->rgbRed    = *TmpPtr++;   
  
            lpRGBQuadPtr++;
        }
      }
    else
        wBytesPerPalEntry = 0;
  
    lpTGAFixup -> wBytesPerPalEntry = wBytesPerPalEntry;

    if (TgaHdr -> ImageType >= 9)
        lpTGAFixup -> bIsRLE = TRUE;

    lpTGAFixup -> wTGAType = TgaHdr -> ImageType;
  
    return (TRUE);
}
  

/*----------------------------------------------------------------------------

  PROCEDURE:      FindTGAData
  DESCRIPTION:    Finds file offset where TGA data starts (top or bottom of file)
  DEFINITION:     
  START:          David Ison
  MODS:           

-----------------------------------------------------------------------------*/

DWORD FindTGAData (hFile, lpFileInfo, dwCurrOffset)
int         hFile;
LPFILEINFO  lpFileInfo;
DWORD       dwCurrOffset;
{
    DWORD       dwSeekVal;
    LPTGAFIXUP  lpTGAFixup;
  
    lpTGAFixup   = (LPTGAFIXUP) ((LPSTR) lpFileInfo + sizeof (FILEINFO));
  
    if (lpTGAFixup -> bIsInverted)
    {
        WORD  wBytesPerPixel;

        /*  Find end of file for reading backwards   */
  
        if (lpFileInfo -> wBitsPerPixel < 24)
            wBytesPerPixel = 1;
        else
            if (lpTGAFixup -> bIsTGA32)
                wBytesPerPixel = 4;
            else
                if (lpTGAFixup -> bIsTGA16)
                    wBytesPerPixel = 2;
                else        
                    wBytesPerPixel = 3;

        dwSeekVal = dwCurrOffset + 
                    ((DWORD) lpFileInfo -> wScanWidth  * 
                     (DWORD) lpFileInfo -> wScanHeight *
                     (DWORD) wBytesPerPixel);
    }
    else   
    {
        WORD  wBytesPerStrip;
  
        /*  The following is a bit of defensive programming.  Its trying to
              guard against images where wBytesPerStrip < wBytesPerLastStrip */
  
        wBytesPerStrip = lpTGAFixup -> wInBytesPerStrip;
  
        if (wBytesPerStrip == 0)
           wBytesPerStrip = lpFileInfo -> wBytesPerLastStrip;
  
        /*  Find actual start of image data */
  
        dwSeekVal = (DWORD) (sizeof (TARGAHEADER) + lpTGAFixup -> IDLength 
              + (lpTGAFixup -> wBytesPerPalEntry * lpTGAFixup -> CMLength));
  
        dwSeekVal -= (DWORD) wBytesPerStrip;
        if (_llseek (hFile, dwSeekVal, 0) != (LONG)dwSeekVal) 
            return (FALSE);
    }
    return (dwSeekVal);
}



int FAR DecodeRLE (hDestFile, hSourceFile, lpTGAHeader, lpFileInfo)
int             hDestFile;
int             hSourceFile;
LPTARGAHEADER   lpTGAHeader;
LPFILEINFO      lpFileInfo;
{
    DWORD dwOffset;
    WORD  wNumStrips;
    WORD  wRowsPerStrip;
    WORD  wBytesPerPixel;
    WORD  wBytesPerRow;

    int   i;
    HANDLE  hBuffer;
    LPSTR   lpBuffer;

    hBuffer = GlobalAlloc (GMEM_MOVEABLE, 32768);
    if (! hBuffer)
        return (-1);
    lpBuffer = GlobalLock (hBuffer);

    /*  Save file pointer of hDestFile  */

    dwOffset = tell (hDestFile);

    wBytesPerPixel = (lpTGAHeader -> PixelDepth >> 3);
    wBytesPerRow   = lpTGAHeader -> ImageWidth * (lpTGAHeader -> PixelDepth >> 3);

    wRowsPerStrip = (WORD) 32768 / wBytesPerRow;
    wNumStrips    = (lpFileInfo -> wScanHeight + (wRowsPerStrip - 1)) / wRowsPerStrip;

    /*  Setup Progress meter to watch.. */

    {
        PSTR pStringBuf;

        pStringBuf  = LocalLock (hStringBuf);
        EnableWindow (hWndIP, FALSE);
        GetNameFromPath (pStringBuf, (LPSTR) szOpenFileName);
        ProOpen (hWndIP, NULL, MakeProcInstance (Abandon, hInstIP), (LPSTR) pStringBuf);
        ProSetBarRange (wNumStrips);
        LoadString (hInstIP, STR_DECODING_RLE_TGA, (LPSTR) pStringBuf, MAXSTRINGSIZE);
        ProSetText (ID_STATUS1, (LPSTR) pStringBuf);
        LocalUnlock (hStringBuf);

    }

    /*  Convert Targa image   */ 

    if (ExpStart (hSourceFile, wBytesPerPixel) < 0)
        return (-1);

    bAbandon = FALSE;
    for (i = 0; (i < (int) wNumStrips && ! bAbandon); i++)
    {
        ExpRead (lpBuffer, lpFileInfo -> wScanWidth * wRowsPerStrip);
        WriteFile (hDestFile, lpBuffer, wBytesPerRow * wRowsPerStrip);
        ProDeltaPos (1);
    }

    ExpStop ();

    GlobalUnlock (hBuffer);
    GlobalFree   (hBuffer);

    EnableWindow (hWndIP, TRUE);
    ProClose ();
    _llseek (hDestFile, dwOffset, 0);
    if (bAbandon)
        return (USER_ABANDON);
    return (0);
}



#define MAXRLEBUFSIZE   16384              /* must be at least 2048 */

HANDLE  hRLEBuff = 0;   // Be sure to set back to NULL on exit...
LPSTR rlBuff;                           /* address of RL buffer */
LPSTR rlEndBuff;                        /* address of last valid byte in rl buffer */
LPSTR rlPointer;                        /* address of next good byte in rl buffer */

unsigned rlBufSize;                     /* number of bytes in RL buffer */
unsigned rlSize;                        /* bytes per output pixel */
int rlPict;                             /* file pointer */
int rlPackFlag = -1;                    /* rl status flag: -1 never set up */
                                        /* 0 - raw packet, 0x80 rl packet */
unsigned rlPackLeft;                    /* number of bytes left in the packet */
char rlValue[4];                        /* value in the array */


int NEAR ExpStart (pict, size)
int pict;           // File handle
int size;           // wBytesPerPixel
{
    rlBufSize = MAXRLEBUFSIZE;

    if (! hRLEBuff)
    {
        hRLEBuff = GlobalAlloc (GHND, (DWORD) rlBufSize);
        rlBuff = GlobalLock (hRLEBuff);
    }

    if (rlBufSize < 128)
    {
        GlobalUnlock (hRLEBuff);
        GLOBALFREE   (hRLEBuff);
        return (-1);
    }

    rlPict     = pict;                                          
    rlSize     = size;                                          
    rlEndBuff  = &rlBuff [rlBufSize];
    rlPointer  = rlEndBuff + 1;
    rlPackLeft = 0;                            /* RL Packet flag */
    return (0);
}
  
void NEAR ExpStop ()
{
    if (rlPackFlag != -1) 
    {
        GlobalUnlock (hRLEBuff);
        GLOBALFREE   (hRLEBuff);
    }
    rlPackFlag = -1;                    
}

/*------------------------------------------------------
        ExpRead --- Read a block of compressed data
        Usage: ExpRead(outBuf, num pixels)
-------------------------------------------------------*/

void NEAR ExpRead (lpBuff, OutPixels)
LPSTR lpBuff;
int  OutPixels;
{
    register int temp, temp2;
    char *tempChar;

    do
    {
        /* Read data if needed            */

        if (rlPointer >= rlEndBuff)
        {       
            ReadFile (rlPict, rlBuff, (LONG) rlBufSize);
            rlPointer = rlBuff;
        }

        if (rlPackLeft == 0)
        {
            rlPackFlag = 0x0080 & ((int)*(rlPointer));
            rlPackLeft = (0x07f & ((int)*(rlPointer++))) + 1;

            /*  Get the repeat value if appropriate     */

            if  (rlPackFlag != 0)
            {
                temp = rlSize;
                tempChar = rlValue;
                do
                {
                    if (rlPointer >= rlEndBuff)
                    {
                        ReadFile (rlPict, rlBuff, (LONG) rlBufSize);
                        rlPointer = rlBuff;
                    }
                    *(tempChar++) = *(rlPointer++);
                } while (--temp > 0); 
            }
    
        }  

        /*  Non-RLE Packet             */

        if (rlPackFlag == 0)
        {
            /*  Load appropriate number of pixels
                either number left in row or number left
                in packet whichever is smaller     */
           
            if (rlPackLeft > (unsigned)OutPixels)
            {
                temp = rlSize*OutPixels;
                rlPackLeft -= OutPixels;
                OutPixels=0;
            }
            else
            {
                temp = rlPackLeft*rlSize;
                OutPixels -= rlPackLeft;
                rlPackLeft = 0;
            }
            do
            {
                /*  If not enough bytes in disk buffer -
                    dump what we have and then go back for more */
                    
                if (temp > (temp2 = (rlEndBuff - rlPointer)))
                {
                    if (temp2 > 0) 
                        _fmemcpy ((LPSTR) lpBuff, (LPSTR) rlPointer, temp2);

                    ReadFile (rlPict, rlBuff, (LONG) rlBufSize);

                    rlPointer = rlBuff;
                    temp -= temp2;
                    lpBuff += temp2;
                }
                else  // Otherwise just load the pixels
                {
                    _fmemcpy ((LPSTR) lpBuff, (LPSTR) rlPointer, temp);
                    rlPointer += temp;
                    lpBuff += temp;
                    temp = 0;
                }       
            } while (temp > 0); 
        }
        else  // Yes, a RLE Packet...
        {
            /* Save min of # of bytes in line or # of bytes in this packet */
        
            if (rlPackLeft > (unsigned)OutPixels)
            {
                temp = OutPixels;
                rlPackLeft -= OutPixels;
                OutPixels = 0;
            }
            else
            {
                temp = rlPackLeft;
                OutPixels -= rlPackLeft;
                rlPackLeft = 0;
            }
                do
                {
                    _fmemcpy ((LPSTR) lpBuff, (LPSTR) rlValue, rlSize);
                    lpBuff += rlSize;
                } while (--temp > 0);
            }
    } while (OutPixels > 0);
}

void FAR PASCAL TGAFlushRead (lpFileInfo)
LPFILEINFO lpFileInfo;
{
    LPTGAFIXUP lpTGAFixup;
    LPSTR       lpPath;
    OFSTRUCT    Of;

    lpTGAFixup   = (LPTGAFIXUP) ((LPSTR) lpFileInfo + sizeof (FILEINFO));

    /*  If RLE, delete temp file and free temp file path buffer  */

    if (lpTGAFixup -> bIsRLE)
    {

        lpPath = (LPSTR) LocalLock (lpTGAFixup -> hRLEFileBuffer);
        OpenFile (lpPath, (LPOFSTRUCT) &Of, OF_DELETE);
        LocalUnlock (lpTGAFixup -> hRLEFileBuffer);
        LocalFree (lpTGAFixup -> hRLEFileBuffer);
    }
}
