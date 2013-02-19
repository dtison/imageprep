
#include <windows.h>
#include <imgprep.h>
#include <global.h>
#include <error.h>
#include <cpifmt.h>
#include <io.h>       // Using C library "tell" function
#include <memory.h>   // For C 6.0 memory copy etc.
#include <cpi.h>      // CPI library tools
#include "dvafmt.h"


int FAR PASCAL DVAInitHeader (
int           hFile,
LPSTR         lpBuffer,
HANDLE        hIFInfo,
HANDLE        hIBInfo,
HANDLE        hEFInfo,  
HANDLE        hEBInfo        )
{
    LPBITMAPINFO        lpDVABitmapInfo;
    LPBITMAPINFOHEADER  lpDVAbmiHeader;
    DWORD               dwDIBSize;
    LPFILEINFO          lpEFileInfo;
    HANDLE              hDVABitmap;
    LPDVAFIXUP          lpDVAFixup;
    int                 nRetVal = 0;
    WORD                wDIBBytesPerRow;

    if (! (lpEFileInfo = (LPFILEINFO) GlobalLock (hEFInfo)))
        return (EC_MEMORY1);

    lpDVAFixup  = (LPDVAFIXUP) ((LPSTR) lpEFileInfo + sizeof (FILEINFO));

    wDIBBytesPerRow = (WORD) ((((DWORD) lpEFileInfo -> wScanWidth * 24L) + 31L) / 32L * 4L);

    /*  Allocate a packed DIB buffer */


    dwDIBSize       = (DWORD) lpEFileInfo -> wScanHeight * (DWORD) wDIBBytesPerRow;
    dwDIBSize      += (DWORD) sizeof (BITMAPINFOHEADER);

    hDVABitmap = GlobalAlloc (GHND, dwDIBSize);
    if (! hDVABitmap)
    {
        GlobalUnlock (hEFInfo);
        return (EC_MEMORY1);
    }

    lpDVABitmapInfo = (LPBITMAPINFO) GlobalLock (hDVABitmap);
    lpDVAbmiHeader  = &lpDVABitmapInfo->bmiHeader;

    lpDVAbmiHeader -> biSize   = sizeof (BITMAPINFOHEADER);
    lpDVAbmiHeader -> biWidth  = lpEFileInfo->wScanWidth;
    lpDVAbmiHeader -> biHeight = lpEFileInfo->wScanHeight;
    lpDVAbmiHeader -> biPlanes = 1;

    lpDVAbmiHeader -> biBitCount = 24;

    lpDVAbmiHeader -> biSizeImage      = (DWORD)WIDTHBYTES((DWORD)lpDVAbmiHeader -> biWidth 
                                      * (DWORD)lpDVAbmiHeader -> biBitCount) 
                                      * (DWORD)lpDVAbmiHeader -> biHeight;


    lpDVAbmiHeader -> biCompression    = 0; 
    lpDVAbmiHeader -> biXPelsPerMeter  = 0;
    lpDVAbmiHeader -> biYPelsPerMeter  = 0;
    lpDVAbmiHeader -> biClrUsed        = 0;
    lpDVAbmiHeader -> biClrImportant   = 0;

    lpDVAFixup -> hDVABitmap   = hDVABitmap;
    lpDVAFixup -> dwDataOffset = dwDIBSize;


    /*  Get the export options  */
    {
        HANDLE (FAR PASCAL *lpOFunc)(HANDLE, HANDLE);
        HANDLE hLibrary;

        hLibrary = LoadLibrary ("DVA.FFT");

        if (hLibrary >= 32)
        {
            lpOFunc = GetProcAddress (hLibrary, "FFTGetExportOptions");
            if (lpOFunc != NULL)
        	    lpDVAFixup -> hExportOptions = (*lpOFunc)(hDVABitmap, hWndIP);
        }
        else
            nRetVal = USER_ABANDON;

        FreeLibrary (hLibrary);
    }


    GlobalUnlock (hDVABitmap);
    GlobalUnlock (hEFInfo);

    return (nRetVal);

}


int FAR PASCAL DVAWrConvertData (hFile, lppT, lppD, lpEFileInfo, lpBitmapInfo)
int           hFile;
LPSTR FAR *   lppT;
LPSTR FAR *   lppD;
LPFILEINFO    lpEFileInfo;
LPBITMAPINFO  lpBitmapInfo;
{
    int              nRetval = TRUE;
    LPSTR            lpSource;
    LPSTR            lpDest;
    LPSTR            lpTemp;
    WORD             wBytesThisStrip;
    WORD             wPaddedBytesThisStrip;
    WORD             wRowsThisStrip;
    WORD             wDIBBytesPerRow;
    WORD             wDIBBytesThisStrip;

    char huge                 *hpData;
    LPBITMAPINFO              lpDVABitmapInfo;
    LPDVAFIXUP                  lpDVAFixup;

    lpSource = *lppD;
    lpDest   = *lppT;

    lpDVAFixup  = (LPDVAFIXUP) ((LPSTR) lpEFileInfo + sizeof (FILEINFO));

    wDIBBytesPerRow = (WORD) ((((DWORD) lpEFileInfo -> wScanWidth * 24L) + 31L) / 32L * 4L);


    if (lpEFileInfo->bIsLastStrip)
    {
        wPaddedBytesThisStrip = lpEFileInfo->wPaddedBytesPerRow *
                                lpEFileInfo->wRowsPerLastStrip;
        wBytesThisStrip       = lpEFileInfo->wBytesPerRow *
                                lpEFileInfo->wRowsPerLastStrip;
        wRowsThisStrip        = lpEFileInfo->wRowsPerLastStrip;

        wDIBBytesThisStrip    = wDIBBytesPerRow * lpEFileInfo->wRowsPerLastStrip;

    }
    else
    {
        wBytesThisStrip       = lpEFileInfo->wBytesPerRow *
                                lpEFileInfo->wRowsPerStrip;
        wPaddedBytesThisStrip = lpEFileInfo->wPaddedBytesPerRow *
                                lpEFileInfo->wRowsPerStrip;
        wRowsThisStrip        = lpEFileInfo->wRowsPerStrip;
        wDIBBytesThisStrip    = wDIBBytesPerRow * lpEFileInfo->wRowsPerStrip;
    }
  

    /*  Put data in packed DIB  */

    lpDVABitmapInfo = (LPBITMAPINFO) GlobalLock (lpDVAFixup -> hDVABitmap);

    hpData = (char huge *) ((char huge *) lpDVABitmapInfo + lpDVAFixup -> dwDataOffset - (DWORD) wDIBBytesThisStrip);


    if (wDIBBytesPerRow != lpEFileInfo -> wPaddedBytesPerRow)
    {
        ClipScanlines (lpDest, lpSource, wDIBBytesPerRow, lpEFileInfo -> wPaddedBytesPerRow, wRowsThisStrip);
        lpTemp   = lpSource;
        lpSource = lpDest;
        lpDest   = lpTemp;
    }

    /*  Just copy from our packed DIB buffer  */

    InvertScanlines (lpDest, lpSource, wDIBBytesPerRow, wRowsThisStrip);

    _fmemcpy (hpData, lpDest, wDIBBytesThisStrip);
    
    lpDVAFixup -> dwDataOffset -= (DWORD) wDIBBytesThisStrip;

//  #define TEST
    #ifndef TEST
    GlobalUnlock (lpDVAFixup -> hDVABitmap);
    #endif

    /*  Finally, ask DLL to convert DIB to DVA  */

    if (lpEFileInfo -> bIsLastStrip && ! bAbandon) 
    {
        int (FAR PASCAL *lpEFunc)(int, HANDLE, HANDLE);
        HANDLE hLibrary;
        HCURSOR hCurrentCursor;

        hCurrentCursor  = SetCursor (LoadCursor (NULL, IDC_WAIT));

        hLibrary = LoadLibrary ("DVA.FFT");
        lpEFunc = GetProcAddress (hLibrary, "FFTExportFile");
        if (lpEFunc != NULL)
        {
            (*lpEFunc) (hFile, lpDVAFixup -> hDVABitmap, lpDVAFixup -> hExportOptions);

        }
        FreeLibrary (hLibrary);

        SetCursor (hCurrentCursor);

        #ifdef  TEST
        {
            BITMAPFILEHEADER  BitmapFileHeader;
            LPBITMAPINFOHEADER lpDVAbmiHeader;
            OFSTRUCT Of;
            int hTmpFile;

            lpDVAbmiHeader  = &lpDVABitmapInfo->bmiHeader;



            BitmapFileHeader.bfType             = 0x4D42;         /* 'BM' */
            BitmapFileHeader.bfReserved1        = 0;
            BitmapFileHeader.bfReserved2        = 0;
            BitmapFileHeader.bfOffBits          = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

            BitmapFileHeader.bfSize = BitmapFileHeader.bfOffBits + 
                                      lpDVAbmiHeader -> biSizeImage;

            
            hTmpFile = OpenFile ((LPSTR) "1dva.bmp", (LPOFSTRUCT)&Of, OF_CREATE | OF_READWRITE);

            WriteFile (hTmpFile, (LPSTR) &BitmapFileHeader, (DWORD) sizeof (BitmapFileHeader));
            WriteFile (hTmpFile, (LPSTR) lpDVABitmapInfo, (DWORD) sizeof (BITMAPINFOHEADER));
            WriteFile (hTmpFile, ((LPSTR) lpDVABitmapInfo + sizeof (BITMAPINFOHEADER)), lpDVAbmiHeader -> biSizeImage);

            _lclose (hTmpFile);

            GlobalUnlock (lpDVAFixup -> hDVABitmap);

        }
        #endif


    }
    return (nRetval);
}


int FAR PASCAL  DVAFixupHeader  (hFile, lpBuffer, hIFInfo, hIBInfo, hEFInfo, hEBInfo )
int      hFile;
LPSTR    lpBuffer;
HANDLE   hIFInfo;
HANDLE   hEFInfo;
HANDLE   hIBInfo;
HANDLE   hEBInfo;
{
    LPDVAFIXUP lpDVAFixup;
    LPFILEINFO lpFileInfo;

    lpFileInfo = (LPFILEINFO) GlobalLock (hEFInfo);

    lpDVAFixup  = (LPDVAFIXUP) ((LPSTR) lpFileInfo + sizeof (FILEINFO));

    GLOBALFREE (lpDVAFixup -> hDVABitmap);

    GlobalUnlock (hEFInfo);

    return (0);
}

 


