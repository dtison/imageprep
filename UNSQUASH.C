
#include <windows.h>
#include <imgprep.h>
#include <proto.h>
#include <global.h>
#include <io.h>
#include <cpifmt.h>
#include <cpi.h>
#include <oic.h>
#include <error.h>
#include <strtable.h>

#define  MSG_SAVE_PERCENT    0x0411
#define  MSG_DONE_REPORT     0x0412
#define GOFAST

extern DWORD   dwCPIPalPos;
extern DWORD   dwCPIDataPos; 

#define OPT_OICMEMORY
#define UnsquashDiscardResource(r)      _asm    mov   ax,r     \
                                        _asm    jmp   UnsquashDiscardResource


int FAR CPIFixupFile (int, LPSTR, LPCPIIMAGESPEC);

int FAR PASCAL DecompressCPI (hCompressedFile, lpDestPath) 
int   hCompressedFile;
LPSTR lpDestPath;
{
    HANDLE  hBuffer1         = (HANDLE) 0;
    HANDLE  hBuffer2         = (HANDLE) 0;
    HANDLE  hPalBuffer       = (HANDLE) 0;
    HANDLE  hXFRMBuf         = (HANDLE) 0;
    HANDLE  hLZWDecodeBuf    = (HANDLE) 0;

    LPSTR   lpXFRMBuf        = (LPSTR) 0L;
    LPSTR   lpLZWDecodeBuf   = (LPSTR) 0L;
    LPSTR   lpBuffer1        = (LPSTR) 0L;
    LPSTR   lpBuffer2        = (LPSTR) 0L;
    LPSTR   lpPalBuffer      = (LPSTR) 0L;
    PSTR    pStringBuf       = (PSTR)  0;
    PSTR    pStringBuf2      = (PSTR)  0;

    int     hOutFile  = 0;

    WORD    wScanWidth;
    WORD    wScanHeight;
    WORD    wBytesPerRow;
    WORD    wBytesPerPixel;

    LONG    lNumBytes;
    int Retval = -1;
    CPIFILESPEC  CPIFileSpec;
    CPIIMAGESPEC CPIImageSpec;
    OICMETER    OICMeter;
    OFSTRUCT Of;


   /*  First let's do our memory allocations  */

    #ifndef OPT_OICMEMORY

    hBuffer1 = GlobalAlloc (GHND, 32768);
    if (! hBuffer1)
        return (Retval);

    lpBuffer1 = GlobalLock (hBuffer1);  
    if (lpBuffer1 == NULL)
        return (Retval);


    hBuffer2 = GlobalAlloc (GHND, 32768);
    if (! hBuffer2)
        return (Retval);

    lpBuffer2 = GlobalLock (hBuffer2);  
    if (lpBuffer2 == NULL)
        return (Retval);


    #else

    lpBuffer1 = GlobalWire (hGlobalBufs);
    if (! lpBuffer1)
        UnsquashDiscardResource (EC_NOMEM);


    // New 3.1 memory mgt stuff

//  hBuffer2  = GlobalAlloc (GHND, 65535);
//  if (! hBuffer2)
//      UnsquashDiscardResource (EC_NOMEM);

//  lpBuffer2 = GlobalWire (hBuffer2);
//  if (! lpBuffer2)
//      UnsquashDiscardResource (EC_NOMEM);

    lpBuffer2 = (BYTE huge *) lpBuffer1 + LARGEBUFOFFSET;

    #endif



    hPalBuffer = GlobalAlloc (GHND, 1024);
    if (! hPalBuffer)
        UnsquashDiscardResource (EC_NOMEM);

    lpPalBuffer = GlobalLock (hPalBuffer);  
    if (! lpPalBuffer)
        UnsquashDiscardResource (EC_NOMEM);


    hLZWDecodeBuf = GlobalAlloc (GHND, 32768);
    if (! hLZWDecodeBuf)
        UnsquashDiscardResource (EC_NOMEM);

    lpLZWDecodeBuf = GlobalLock (hLZWDecodeBuf);  
    if (lpLZWDecodeBuf == NULL)
        UnsquashDiscardResource (EC_NOMEM);


    hXFRMBuf = GlobalAlloc (GHND, 43960);
    if (! hXFRMBuf)
        UnsquashDiscardResource (EC_NOMEM);

    lpXFRMBuf = GlobalLock (hXFRMBuf);  
    if (lpXFRMBuf == NULL)
        UnsquashDiscardResource (EC_NOMEM);

    pStringBuf  = LocalLock (hStringBuf);
    pStringBuf2 = pStringBuf + MAXSTRINGSIZE;


    #ifdef NEVER
    #ifdef DEBUG
    {
        DWORD dwNumBytes;
        char message [80];

        dwNumBytes = GlobalCompact (-1);
        
        wsprintf (message, "Memory after allocations: %ld ",dwNumBytes);
        WinAssert (NULL, message);
    }
    #endif
    #endif


    /*  Next go read from the already open file to get critical goodies like          
        image dimensions, etc..  */  // Still need ERROR checking



    /*  Get system metrics & image file sizes  */
    
    if (! ReadCPIHdr (hCompressedFile, (LPCPIFILESPEC) &CPIFileSpec, (LPCPIIMAGESPEC) &CPIImageSpec))
        UnsquashDiscardResource (EC_SAVEDLG);


    wScanWidth  = CPIImageSpec.X_Length;
    wScanHeight = CPIImageSpec.Y_Length;

    CalcMatrices (CPIImageSpec.CompLevel);

    /*  Find out how many strips so we know how many RLECounts are in the palette position  */

    {

        WORD wRLECountsSize;

        wRLECounts = (wScanHeight + 7) >> 3;  // wRLECounts is same as number of strips and is a counter 
                                                   // of how many RLE byte counts will be used in the compression.
        wRLECountsSize = (wRLECounts << 1);

        SeekFile (hCompressedFile,CPIImageSpec.PalDataOffset, 0);

        if (ReadFile (hCompressedFile, lpPalBuffer, wRLECountsSize) != (LONG) wRLECountsSize)
            UnsquashDiscardResource (EC_FILEREAD2);
      
        /*  Point to input data  */

        SeekFile (hCompressedFile, CPIImageSpec.ImgDataOffset, 0);

    }

    /*  Initialize Output file  */


    hOutFile = OpenFile (lpDestPath, (LPOFSTRUCT)&Of, OF_READWRITE | OF_CREATE);

    InitCPIFileSpec ((LPCPIFILESPEC) &CPIFileSpec);

    lNumBytes = (LONG) sizeof (CPIFILESPEC);

    if (WriteFile (hOutFile, (LPSTR) &CPIFileSpec, lNumBytes) != lNumBytes)
        UnsquashDiscardResource (EC_SAVEDLG);

        /*  Write out Image Spec stuff  */

    InitCPIImageSpec ((LPCPIIMAGESPEC) &CPIImageSpec);


    CPIImageSpec.ImgType          = CPIFMT_RGB;
    CPIImageSpec.ImgDescTag       = 1;
    CPIImageSpec.ImgSpecLength    = sizeof (CPIIMAGESPEC);
    CPIImageSpec.X_Origin         = 0;
    CPIImageSpec.Y_Origin         = 0;   
    CPIImageSpec.X_Length         = wScanWidth;
    CPIImageSpec.Y_Length         = wScanHeight;


    CPIImageSpec.BitsPerPixel     = 24;

    CPIImageSpec.NumberPlanes     = 1;
    CPIImageSpec.NumberColors     = 256;
    CPIImageSpec.Orientation      = 0;    
    CPIImageSpec.Compression      = 0;    // Un-OIC'ed
    CPIImageSpec.ImgSize          = (DWORD) ((DWORD) wScanWidth * 
                                            (DWORD) wScanHeight * 3L);

    CPIImageSpec.ImgSequence      = 0;
    CPIImageSpec.NumberPalEntries = 0;
    CPIImageSpec.NumberBitsPerEntry = 24;
    CPIImageSpec.AttrBitsPerEntry = 0;
    CPIImageSpec.FirstEntryIndex  = NULL;
    CPIImageSpec.RGBSequence      = 0;
    CPIImageSpec.NextDescriptor   = NULL;
    CPIImageSpec.ApplSpecReserved = NULL;        
 
    CPIImageSpec.CompLevel        = 0;
    CPIImageSpec.CodeSize         = 0;

    lNumBytes = (LONG)sizeof(CPIIMAGESPEC);

    if (WriteFile (hOutFile, (LPSTR) &CPIImageSpec, lNumBytes) != lNumBytes)
      UnsquashDiscardResource (EC_SAVEDLG);

    dwCPIPalPos = dwCPIDataPos  = tell (hOutFile);


    wBytesPerPixel  = CPIImageSpec.BitsPerPixel >> 3;
    wBytesPerRow    = wScanWidth * wBytesPerPixel;

    LoadString (hInstIP, STR_DECOMPRESSING_IMAGE, (LPSTR) pStringBuf, MAXSTRINGSIZE);
    LoadString (hInstIP, STR_OIC, (LPSTR) pStringBuf2, MAXSTRINGSIZE);

    OICMeter.hWnd        = hWndIP;
    OICMeter.hInst       = hInstIP;
    OICMeter.lpfnAbandon = Abandon;
    OICMeter.lpMessage1  = (LPSTR) pStringBuf;
    OICMeter.lpCaption   = (LPSTR) pStringBuf2;
    bAbandon             = FALSE;

    /*  Make sure there is enough space to decompress  */
    {
        DWORD dwImageSize;

        dwImageSize = (DWORD) wScanWidth * (DWORD) wScanHeight * 3L;    // 3 could be 1 also if we decompressed grayscale files...

//      if (GetDiskSpace ((*lpDestPath - 96)) < dwImageSize)
        if (! IsDiskSpaceAvailable (lpDestPath,  dwImageSize))
            UnsquashDiscardResource (EC_NODISK);
    }   

    if (! DecompressOIC (hCompressedFile, hOutFile, lpBuffer1, lpBuffer2, wBytesPerRow, wScanWidth, wScanHeight, (WORD FAR *) lpPalBuffer, lpXFRMBuf, lpLZWDecodeBuf, (LPOICMETER) &OICMeter))
        if (bAbandon)           // User abandoned compress, return W/O err code
            UnsquashDiscardResource (USER_ABANDON);
        else
            UnsquashDiscardResource (EC_SAVEDLG);


    CPIFixupFile (hOutFile, lpPalBuffer, (LPCPIIMAGESPEC) &CPIImageSpec);

    UnsquashDiscardResource (hOutFile);


/*---  Unsquash Resource Discard Section  ---*/

    {

        int nRetVal;

        UnsquashDiscardResource:

        _asm    mov nRetVal,ax


        _lclose (hCompressedFile);

        if (nRetVal < 0){
          OFSTRUCT Of;

          if (hOutFile){
            _lclose (hOutFile);
            OpenFile (lpDestPath, (LPOFSTRUCT)&Of, OF_DELETE);
          }
        }
          

        /*  Unlocks  */


        if (lpBuffer1)
            GlobalUnWire (hGlobalBufs);

//      if (lpBuffer2)
//          GlobalUnWire (hBuffer2);

        if (lpPalBuffer)
            GlobalUnlock (hPalBuffer);

        if (lpXFRMBuf)
            GlobalUnlock (hXFRMBuf);

        if (lpLZWDecodeBuf)
            GlobalUnlock (hLZWDecodeBuf);

        if (pStringBuf)
            LocalUnlock (hStringBuf);

        /*  Frees */

//      if (hBuffer2)
//          GlobalFree (hBuffer2);

        if (hPalBuffer)
            GlobalFree (hPalBuffer);

        if (hXFRMBuf)
            GlobalFree (hXFRMBuf);

        if (hLZWDecodeBuf)
            GlobalFree (hLZWDecodeBuf);


        /*  Close and reopen file to avoid lost clusters and other read problems  */

        _lclose (hOutFile);
        OpenFile (lpDestPath, (LPOFSTRUCT)&Of, OF_READWRITE);

        return (nRetVal);
    }


}

