
#include <windows.h>
#include <io.h>
#include <cpi.h>
#include <imgprep.h>
#include <proto.h>
#include <error.h>
#include <cpifmt.h>
#include <memory.h>
#include <global.h>
#include "fmtproto.h"
#include "dvafmt.h"
#include "error.h"

#ifdef DVAFIX
void NEAR GetDVALibraryPath (LPSTR);
#endif

int FAR PASCAL DVAReadHeader (hFile, lpBitmapInfo, lpInputBuf, lpFileInfo)
int             hFile;
LPBITMAPINFO    lpBitmapInfo;
LPSTR           lpInputBuf;
LPFILEINFO      lpFileInfo;
{
    int                 nRetVal= 0;                                   /* OK */
    WORD                wBytesPerStrip;
    WORD                wRowsPerStrip;
    WORD                wNumStrips;
    WORD                wBytesPerLastStrip;
    WORD                wRowsPerLastStrip;
    WORD                wRowCount;
	  HANDLE              hLibrary;
    int                 (FAR PASCAL *lpImportImg)(int, LPWORD);
    BOOL                (FAR PASCAL *lpRecognizeImg)(int);
    HANDLE              RetHandle;

    LPBITMAPINFO        lpDVABitmapInfo;
    LPBITMAPINFOHEADER  lpDVAbmiHeader;
    LPDISPINFO          lpDispInfo;
    DWORD               dwOffset;
    LPDVAFIXUP          lpDVAFixup;
    #ifdef DVAFIX
    char                DVALibraryPath [MAXPATHSIZE + 14];
    #endif

    lpDVAFixup  = (LPDVAFIXUP) ((LPSTR) lpFileInfo + sizeof (FILEINFO));
    lpDispInfo  = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);

    if (lpDVAFixup -> hDVABitmap)     // Already have converted DVA file
    {
        lpDVAFixup -> dwDataOffset = lpFileInfo -> dwDataOffset;
        return (nRetVal);
    }

    #ifdef DVAFIX
    GetDVALibraryPath (DVALibraryPath);
    #endif

    hLibrary = LoadLibrary ("DVA.FFT");
    if (hLibrary >= 32)
    {
        HCURSOR hCurrentCursor;

        hCurrentCursor  = SetCursor (LoadCursor (NULL, IDC_WAIT));

        lpRecognizeImg = GetProcAddress(hLibrary, "FFTRecogniseFile");
        if (lpRecognizeImg != NULL)
        {
            if (! (*lpRecognizeImg)(hFile))
               nRetVal = EC_RDHDR;
        }

        if (nRetVal == 0)     // File checks OK
        {
            lpImportImg = GetProcAddress (hLibrary, "FFTImportFile");
            if (lpImportImg != NULL)
            {
                _llseek (hFile, 0L, 0);
                (*lpImportImg) (hFile, (LPWORD) &RetHandle);

                if (RetHandle == NULL)
                    nRetVal = EC_RDHDR;
                else
                    lpDVAFixup -> hDVABitmap = RetHandle;
            }
        }
        FreeLibrary (hLibrary); 

        SetCursor (hCurrentCursor);
    }
    else
        nRetVal = EC_INVALIDIMAGE;

    if (nRetVal != 0)         // Check for a failure to import
        return (nRetVal);
    

    lpDVABitmapInfo = (LPBITMAPINFO) GlobalLock (lpDVAFixup -> hDVABitmap);
    lpDVAbmiHeader  = &lpDVABitmapInfo->bmiHeader;


    lpFileInfo -> wBitsPerPixel       = lpDVAbmiHeader -> biBitCount;
    lpFileInfo -> wScanWidth          = (WORD) lpDVAbmiHeader -> biWidth;
    lpFileInfo -> wScanHeight         = (WORD) lpDVAbmiHeader -> biHeight;

    lpFileInfo -> wBytesPerRow = lpFileInfo -> wScanWidth * (lpFileInfo -> wBitsPerPixel >> 3);
    lpFileInfo -> wPaddedBytesPerRow  = (WORD) ((((DWORD) lpFileInfo -> wScanWidth * (DWORD) lpFileInfo -> wBitsPerPixel) + 31L) / 32L * 4L);

    wRowsPerStrip = MAXBUFSIZE / lpFileInfo -> wBytesPerRow;

    lpDispInfo -> wPaddedScanWidth    = ((lpFileInfo -> wScanWidth * INTERNAL_BITS) + 31) / 32 * 4;

    lpDispInfo -> wPaddedBytesPerRow  = lpDispInfo -> wPaddedScanWidth;  // FOR NOW THEY ARE EQUAL BUT IN 24 BIT DISPLAY THIS WILL BE REFINED
                  

    /*  Also find where data begins  */

    dwOffset = lpDVAbmiHeader -> biSize + (DWORD) lpFileInfo -> wScanHeight * (DWORD) lpFileInfo -> wPaddedBytesPerRow; // Point to end of strip
    lpFileInfo -> dwDataOffset = dwOffset;
    lpDVAFixup -> dwDataOffset = dwOffset;

    lpFileInfo -> lpfnFlushRead  = MakeProcInstance (DVAFlushRead, hInstIP);

    GlobalUnlock (lpDVAFixup -> hDVABitmap);

    SetState (IMPORT_CLASS, IRGB);

    /*---  Little Picture Fix  ---*/

    if (wRowsPerStrip > lpFileInfo->wScanHeight)
       wRowsPerStrip = lpFileInfo->wScanHeight;


    wBytesPerStrip = wRowsPerStrip * lpFileInfo->wBytesPerRow;

    wNumStrips = lpFileInfo->wScanHeight / wRowsPerStrip;
    wRowCount = wNumStrips * wRowsPerStrip;

    if (wRowCount < lpFileInfo->wScanHeight)
    {
       wBytesPerLastStrip  = (lpFileInfo -> wScanHeight - wRowCount) * lpFileInfo->wBytesPerRow;
       wRowsPerLastStrip   = (lpFileInfo -> wScanHeight - wRowCount);
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
    
    
    lpFileInfo->bIsLastStrip        = FALSE;
    return (nRetVal);
}   



void FAR PASCAL DVAFlushRead (lpFileInfo)
LPFILEINFO lpFileInfo;
{
    LPDVAFIXUP          lpDVAFixup;

    lpDVAFixup  = (LPDVAFIXUP) ((LPSTR) lpFileInfo + sizeof (FILEINFO));

    GLOBALFREE (lpDVAFixup -> hDVABitmap);  

}

#ifdef DVAFIX

#include <string.h>

void NEAR GetDVALibraryPath (LPSTR lpLibraryPath)
{

    _fstrcpy ((LPSTR) lpLibraryPath, (LPSTR) szAppPath);

    if (lpLibraryPath [_fstrlen ((LPSTR) lpLibraryPath) - 1] != '\\')
        _fstrcat ((LPSTR) lpLibraryPath, "\\");
    _fstrcat ((LPSTR) lpLibraryPath, (LPSTR) "DVA.FFT");
}

#endif
