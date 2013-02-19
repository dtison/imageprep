#include <windows.h>
#include <imgprep.h>
#include <proto.h>
#include <global.h>
#include <startup.h>
#include <menu.h>
#include <string.h>
#include <direct.h>
#include "clipimg.h"

#ifndef COLORLAB
#ifndef DECIPHER
static char *IniName = "Imgprep";
#endif
#endif

void FAR InitSettings (hWnd)
HWND hWnd;
{

    int nValue;
    HMENU  hMenu;
    WORD wNumPlanes;
    WORD wBitsPerPixel;
    WORD wNumColors;
    HDC  hDC;
    int  nPalette = 0;
    int  nDither  = 0;
    int i;
    char  ApplName [80];
    char Path [MAXPATHSIZE];
    char  szTempName [13];

    hMenu = GetMenu (hWnd);
    hDC   = GetDC (hWnd);


    _fstrcpy ((LPSTR) szTempName, (LPSTR) szAppName);
    #ifndef COLORLAB
    #ifndef DECIPHER
    _fstrcpy ((LPSTR) szTempName, (LPSTR) IniName);
    #endif
    #endif

    GetWindowsDirectory ((LPSTR) Path, MAXPATHSIZE);

    if (Path [_fstrlen ((LPSTR) Path) - 1] != '\\')
        _fstrcat ((LPSTR) Path, "\\");

    _fstrcat ((LPSTR) Path, (LPSTR) szTempName);
    _fstrcat ((LPSTR) Path, (LPSTR) ".ini");

    _fstrcpy ((LPSTR) ApplName, (LPSTR) "Settings");

    /*---  Clipboard Cut/Copy Formats  ---*/

    nValue = GetPrivateProfileInt   ((LPSTR) ApplName,
             (LPSTR) "CopyDIB",
             1,
             (LPSTR) Path);

    if (nValue)
        bClipOptions |= CLPF_DIB;
    else
        EnableClipFormat (CLPF_DIB, FALSE);

    nValue = GetPrivateProfileInt   ((LPSTR) ApplName,
             (LPSTR) "CopyBitmap",
             1,
             (LPSTR) Path);

    if (nValue)
        bClipOptions |= CLPF_BITMAP;
    else
        EnableClipFormat (CLPF_BITMAP, FALSE);

    nValue = GetPrivateProfileInt   ((LPSTR) ApplName,
             (LPSTR) "CopyMetafile",
             1,
             (LPSTR) Path);

    if (nValue)
        bClipOptions |= CLPF_METAFILE;
    else
        EnableClipFormat (CLPF_METAFILE, FALSE);


    /*---  Retain Settings  ---*/

    nValue = GetPrivateProfileInt   ((LPSTR) ApplName,
             (LPSTR) "RetainSettings",
             0,
             (LPSTR) Path);

    bResetProc = (BYTE) (! nValue);
//  CheckMenuItem (hMenu, IDM_RETAIN_TOGGLE, (bResetProc ? MF_UNCHECKED : MF_CHECKED));


    /*---  Auto-Convert     ---*/

    nValue = GetPrivateProfileInt   ((LPSTR) ApplName,
             (LPSTR) "UseTemp",
             1,
             (LPSTR) Path);

    bAutoConvert = (BYTE) nValue;

    CheckMenuItem (hMenu, IDM_RETAIN_TOGGLE, (bResetProc ? MF_UNCHECKED : MF_CHECKED));


    wFullView    = FALSE;
    CheckMenuItem (hMenu, IDM_ACTUALSIZE, MF_CHECKED);
    CheckMenuItem (hMenu, IDM_FITINWINDOW, MF_UNCHECKED);

    if (nDither == 0 && nPalette == 0)  // No settings or set to none, so determine a default
    {
        /*  Test for True color display  */

        wNumPlanes       = GetDeviceCaps (hDC, PLANES);
        wBitsPerPixel    = GetDeviceCaps (hDC, BITSPIXEL);

        if ((wNumPlanes * wBitsPerPixel) <= 8)
        {
            nDither  = IBAY;
            nPalette = INONE;
            EnableMenuItem (hMenu, IDM_VIEWTRUECOLOR, MF_BYCOMMAND | MF_GRAYED);
        }
        else
        {
            nDither  = INONE;
            nPalette = INONE;

            EnableMenuItem (hMenu, IDM_CAPPCX, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_CAPGIF, MF_BYCOMMAND | MF_GRAYED);
            PostMessage (hWnd, WM_COMMAND, IDM_VIEWTRUECOLOR, NULL);  
        }

    }

    SetState (DITH_STATE, nDither);
    SetState (PAL_STATE,  nPalette);

    /*  Save default settings for 24 bit display, even if set to none  */

    bDefDither  = (BYTE) nDither;
    bDefPalette = (BYTE) nPalette;

    /*  Set up the rest of the stuff...  */

    wNumColors = GetDeviceCaps (hDC, NUMCOLORS);
    SetState (DISPLAY_STATE, wNumColors > 16 ? IEVGA : IVGA);
    SetState (DEVICE_STATE, wNumColors > 16 ? DEVGA : DVGA);
    SetState (REGION_STATE, REGION_SCREEN);   /* capture region set   */

    SetState (IMPORT_CLASS, IRGB);            
    SetState (EXPORT_CLASS, IRGB);            
    SetState (EXPORT_TYPE, IDFMT_CPI);        
    SetState (IMPORT_TYPE, 0);                

    /*  Setup default system colors         */

    for (i = 0; i < NUMSYSCOLORS; i++)
        DefaultSysColorValues[i] = GetSysColor(SysColorIndices[i]);

    ReleaseDC (hWnd, hDC);

}


void FAR SaveSettings (hWnd)
HWND hWnd;
{

    int nValue;
    char  ApplName [80];
    char Path [MAXPATHSIZE];
    char Buffer [128];
    char  szTempName [13];

    if (! bPrefChanged)
        return;


    /*  RETURN CODES and LOADSTRINGS FOR ALL THESE STRINGS (AND HSTRINGBUFS) */

    _fstrcpy ((LPSTR) szTempName, (LPSTR) szAppName);

    #ifndef COLORLAB
    #ifndef DECIPHER
    _fstrcpy ((LPSTR) szTempName, (LPSTR) IniName);
    #endif
    #endif

    GetWindowsDirectory ((LPSTR) Path, MAXPATHSIZE);

    if (Path [_fstrlen ((LPSTR) Path) - 1] != '\\')
        _fstrcat ((LPSTR) Path, "\\");

    _fstrcat ((LPSTR) Path, (LPSTR) szTempName);
    _fstrcat ((LPSTR) Path, (LPSTR) ".ini");

    _fstrcpy ((LPSTR) ApplName, (LPSTR) "Settings");


    /*---  Clipboard Cut/Copy Formats  ---*/

    nValue = ((bClipOptions & CLPF_DIB) > 0);
    wsprintf (Buffer, "%d", nValue);
    WritePrivateProfileString ((LPSTR) ApplName,
            (LPSTR) "CopyDIB",
            (LPSTR) Buffer,
            (LPSTR) Path);

    nValue = ((bClipOptions & CLPF_BITMAP) > 0);
    wsprintf (Buffer, "%d", nValue);
    WritePrivateProfileString ((LPSTR) ApplName,
            (LPSTR) "CopyBitmap",
            (LPSTR) Buffer,
            (LPSTR) Path);

    nValue = ((bClipOptions & CLPF_METAFILE) > 0);
    wsprintf (Buffer, "%d", nValue);
    WritePrivateProfileString ((LPSTR) ApplName,
            (LPSTR) "CopyMetafile",
            (LPSTR) Buffer,
            (LPSTR) Path);

    /*---  Retain Settings  ---*/

    wsprintf (Buffer, "%d", (! bResetProc));
    WritePrivateProfileString ((LPSTR) ApplName,
            (LPSTR) "RetainSettings",
            (LPSTR) Buffer,
            (LPSTR) Path);

    /*---  Auto-Convert     ---*/

    wsprintf (Buffer, "%d", (BYTE) bAutoConvert);
    WritePrivateProfileString ((LPSTR) ApplName,
            (LPSTR) "UseTemp",
            (LPSTR) Buffer,
            (LPSTR) Path);

}

