
#include <windows.h>
#include "cpi.h"
#include "imgprep.h"
#include "error.h"
#include "cpifmt.h"
#include <memory.h>
#include "dvafmt.h"


int FAR PASCAL DVARdConvertData (hFile, lpfpDest, lpfpSource, lpFileInfo, lpBitmapInfo)
int           hFile;
LPSTR FAR *   lpfpDest;                                            
LPSTR FAR *   lpfpSource;                    
LPFILEINFO    lpFileInfo;
LPBITMAPINFO  lpBitmapInfo;
{

    /*  "lpSource" may actually be a misnomer.  Maybe we should say lpTmp or something..
         The important thing is that lpfpDest receives the output data  */

    int                       nRetval = 0;
    LPDVAFIXUP                lpDVAFixup;
    LPDISPINFO                lpDispInfo;
    LPSTR                     lpDest;
    LPSTR                     lpSource;
    WORD                      wBytesThisStrip;
    WORD                      wRowsThisStrip;
    char huge                 *hpData;
    LPBITMAPINFO              lpDVABitmapInfo;

    lpSource = *lpfpDest;
    lpDest   = *lpfpSource;

    
    lpDVAFixup  = (LPDVAFIXUP) ((LPSTR) lpFileInfo + sizeof (FILEINFO));
    lpDispInfo  = (LPDISPINFO) ((LPSTR) lpFileInfo + 512);
    
    if (lpFileInfo -> bIsLastStrip)
    {
        wBytesThisStrip = lpFileInfo -> wPaddedBytesPerRow * lpFileInfo -> wRowsPerLastStrip;
        wRowsThisStrip  = lpFileInfo -> wRowsPerLastStrip;
    }
    else
    {
        wBytesThisStrip = lpFileInfo -> wPaddedBytesPerRow * lpFileInfo -> wRowsPerStrip;
        wRowsThisStrip  = lpFileInfo->wRowsPerStrip;
    }
    
    /*  Read some data into the buffer    */

    lpDVABitmapInfo = (LPBITMAPINFO) GlobalLock (lpDVAFixup -> hDVABitmap);
    hpData = (char huge *) ((char huge *) lpDVABitmapInfo + lpDVAFixup -> dwDataOffset - (DWORD) wBytesThisStrip);


    /*  Just copy from our packed DIB buffer  */

    _fmemcpy (lpSource, hpData, wBytesThisStrip);

    InvertScanlines (lpDest, lpSource, lpFileInfo -> wPaddedBytesPerRow, wRowsThisStrip);
    
    lpDVAFixup -> dwDataOffset -= (DWORD) wBytesThisStrip;


    /*  Set the data offset back in case we need it  */

    if (lpFileInfo -> bIsLastStrip)
        lpDVAFixup -> dwDataOffset = lpFileInfo -> dwDataOffset;

    GlobalUnlock (lpDVAFixup -> hDVABitmap);

    *lpfpSource  = lpSource;
    *lpfpDest    = lpDest;
    

    return (nRetval);
}   
    
    
    
    
    
    
