
/*---------------------------------------------------------------------------

    SCALEDLG.C  Scaling Tool interface manager.  Accepts input from the user
                and validates input ...

    CREATED:    3/91   D. Ison

--------------------------------------------------------------------------*/

#include <windows.h>
#include <string.h>
#include <cpi.h>      
#include <cpifmt.h>
#include <malloc.h>
#include <io.h>
#include <prometer.h>
#include <direct.h>
#include "scaledlg.h"
#include "internal.h"
#include "strtable.h"
#include "global.h"
#include "stdio.h"
#include "math.h"

#define MAXPERCENT    999
#define MAXPIXELWIDTH 4400

#define STRETCH_INDEX          0
#define AVERAGE_INDEX          1
#define ANTIALIAS_INDEX        2

#define PIXELS_INDEX           0
#define INCHES_INDEX           1
#define CENTIMETERS_INDEX      2
#define MILLIMETERS_INDEX      3
#define POINTS_INDEX           4
#define PICAS_INDEX            5

int  NEAR GetScaleDlgItemInt (HWND, int, BOOL FAR *, BOOL);

void NEAR SetScaleDlgItem    (HWND, int, WORD, BOOL);

DWORD FAR PASCAL ScaleDlgProc (hWnd, Message, wParam, lParam)
HWND      hWnd;
unsigned  Message;
WORD      wParam;
LONG      lParam;
{
    switch (Message)
    {
        case WM_INITDIALOG:
        {

            PSTR            pStringBuf;
            PSTR            pStringBuf2;
            RECT            Rect;
            pStringBuf  = LocalLock (hStringBuf);
            pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

            /* Center the dialog       */

            GetWindowRect (hWnd,&Rect);
            SetWindowPos  (hWnd,NULL,
                          (GetSystemMetrics(SM_CXSCREEN) - (Rect.right - Rect.left)) / 2,
                          (GetSystemMetrics(SM_CYSCREEN) - (Rect.bottom - Rect.top)) / 3,
                           0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

            /*  Save the current drive / dir and other info in a safe place  */

            getcwd (CurrWDir, MAXPATHSIZE);

            InitScale (hWnd, lParam);
            {
                PSCALE pScale;        // Temp place for saving current data

                pScale = (PSCALE) LocalLock (hStructBuf);
                _fmemcpy ((LPSTR) pScale, (LPSTR) &Scale, sizeof (SCALE));
                LocalUnlock (hStructBuf);
            }
            LocalUnlock (hStringBuf);
        }
        break;
    
        case WM_VSCROLL:
        {
            HWND hWndControl;
            int  nScrollPos;
            int  nOldScrollPos;
            int  nControl;
            int  nMin;   
            int  nMax;
            int  nScrollRange;
            double SourcePixels;
            WORD   wDestPixels;

            hWndControl =  (HWND) HIWORD (lParam);

            GetScrollRange (hWndControl, SB_CTL, &nMin, &nMax);
            nScrollRange = nMax - nMin;
            nScrollPos  = -GetScrollPos (hWndControl, SB_CTL);
            nOldScrollPos = nScrollPos;

            switch (wParam)
            {
                case SB_LINEUP:
                nScrollPos++;
                break;

                case SB_LINEDOWN:
                nScrollPos--;
                break;

                case SB_PAGEUP:
                nScrollPos += (nScrollRange >> 3);
                break;

                case SB_PAGEDOWN:
                nScrollPos -= (nScrollRange >> 3);
                break;

              case SB_THUMBTRACK:
                case SB_THUMBPOSITION: 
                nScrollPos = -LOWORD (lParam);
   		        break;
            }
            if (nScrollPos < 1)
                nScrollPos = 1;
            if (nScrollPos > MAXPERCENT)
                nScrollPos = MAXPERCENT;


            SetScrollPos (hWndControl, SB_CTL, -nScrollPos, TRUE);
            nControl = GetDlgCtrlID (hWndControl);
            SetDlgItemInt (hWnd, (nControl + 1), nScrollPos, FALSE);    // Edit box is scroll ID plus 1

            nControl += 5;

            if (nControl == IDD_TARGET_X)
                SourcePixels = (double) Scale.wScanWidth;
            else
                SourcePixels = (double) Scale.wScanHeight;

            wDestPixels = (WORD) ((double) nScrollPos / 100 * SourcePixels);
            SetScaleDlgItem    (hWnd, nControl, wDestPixels, FALSE);

            if (nControl == IDD_TARGET_X)
            {
                Scale.wXPixels  = wDestPixels;
                Scale.wXPercent = (WORD) nScrollPos;
            }
            else
            {
                Scale.wYPixels  = wDestPixels;
                Scale.wYPercent = (WORD) nScrollPos;
            }

            if (bMaintainAspect)
            {
                nControl -= 5;  // Determine whether it was an X or Y scroll

                if (nControl == IDD_XPERCENTSCROLL)
                {
                    hWndControl = GetDlgItem (hWnd, IDD_YPERCENTSCROLL);
                    SetScrollPos (hWndControl, SB_CTL, -nScrollPos, TRUE);
                    SetDlgItemInt (hWnd, IDD_YPERCENT, nScrollPos, FALSE);
                    Scale.wYPercent = (WORD) nScrollPos;

                    /*  Now correct the Y Target Pixels  */

                    SourcePixels = (double) Scale.wScanHeight;
                    wDestPixels = (WORD) ((double) nScrollPos / 100 * SourcePixels);
                    Scale.wYPixels = wDestPixels;
                    SetScaleDlgItem (hWnd, IDD_TARGET_Y, wDestPixels, FALSE);
                }
                else
                {
                    hWndControl = GetDlgItem (hWnd, IDD_XPERCENTSCROLL);
                    SetScrollPos (hWndControl, SB_CTL, -nScrollPos, TRUE);
                    SetDlgItemInt (hWnd, IDD_XPERCENT, nScrollPos, FALSE);
                    Scale.wXPercent = (WORD) nScrollPos;

                    /*  Now correct the X Target Pixels  */

                    SourcePixels = (double) Scale.wScanWidth;
                    wDestPixels = (WORD) ((double) nScrollPos / 100 * SourcePixels);
                    Scale.wXPixels = wDestPixels;
                    SetScaleDlgItem    (hWnd, IDD_TARGET_X, wDestPixels, FALSE);
                }
            }
        }
        break;

        case WM_COMMAND:
            switch (wParam)
            {

                case IDD_XPERCENT: // Edit box
                    if (HIWORD (lParam) == EN_KILLFOCUS)
                    {
                        WORD wScrollVal;
                        BOOL bTmp;
                        HWND hWndControl;
                        double SourcePixels;
                        WORD   wDestPixels;

                        if (SendDlgItemMessage (hWnd, IDD_XPERCENT, EM_GETMODIFY, NULL, NULL) == FALSE)
                            break;

                        wScrollVal  = GetDlgItemInt (hWnd, IDD_XPERCENT, (BOOL FAR *) &bTmp, FALSE);
                        wScrollVal  = min (MAXPERCENT, max (0, wScrollVal));
 
                        /*  Fix up other fields  */

                        SourcePixels    = (double) Scale.wScanWidth;
                        wDestPixels     = (WORD) ((double) wScrollVal / 100 * SourcePixels);

                        if (wDestPixels > MAXPIXELWIDTH)
                        {
                            MessageBeep (NULL);
                            SendDlgItemMessage (hWnd, IDD_XPERCENT, EM_UNDO, NULL, NULL);
                            break;
                        }


                        hWndControl = GetDlgItem (hWnd, IDD_XPERCENTSCROLL);
                        SetScrollPos  (hWndControl, SB_CTL, - (int) wScrollVal, TRUE);
                        SetDlgItemInt (hWnd, IDD_XPERCENT, wScrollVal, FALSE);


                        Scale.wXPixels  = wDestPixels;
                        SetDlgItemInt     (hWnd, IDD_TARGET_X, wDestPixels, FALSE);

                        if (bMaintainAspect)
                        {
                            hWndControl     = GetDlgItem (hWnd, IDD_YPERCENTSCROLL);
                            SetScrollPos      (hWndControl, SB_CTL, - (int) wScrollVal, TRUE);
                            SetDlgItemInt     (hWnd, IDD_YPERCENT, wScrollVal, FALSE);
                            SourcePixels    = (double) Scale.wScanHeight;
                            wDestPixels     = (WORD) ((double) wScrollVal / 100 * SourcePixels);
                            Scale.wYPixels  = wDestPixels;
                            SetDlgItemInt     (hWnd, IDD_TARGET_Y, wDestPixels, FALSE);
                        }

                    }
                    break;


                case IDD_YPERCENT: 
                    if (HIWORD (lParam) == EN_KILLFOCUS)
                    {
                        WORD wScrollVal;
                        BOOL bTmp;
                        HWND hWndControl;
                        double SourcePixels;
                        WORD   wDestPixels;

                        if (SendDlgItemMessage (hWnd, IDD_YPERCENT, EM_GETMODIFY, NULL, NULL) == FALSE)
                            break;

                        wScrollVal  = GetDlgItemInt (hWnd, IDD_YPERCENT, (BOOL FAR *) &bTmp, FALSE);
                        wScrollVal  = min (MAXPERCENT, max (0, wScrollVal));

                        if (bMaintainAspect)  // This test is a look-ahead for the image width to large
                            if (((double) wScrollVal / 100 * Scale.wScanWidth) > MAXPIXELWIDTH)
                            {
                                MessageBeep (NULL);
                                SendDlgItemMessage (hWnd, IDD_YPERCENT, EM_UNDO, NULL, NULL);
                                break;
                            }

                        hWndControl = GetDlgItem (hWnd, IDD_YPERCENTSCROLL);
                        SetScrollPos  (hWndControl, SB_CTL, - (int) wScrollVal, TRUE);
                        SetDlgItemInt (hWnd, IDD_YPERCENT, wScrollVal, FALSE);

                        /*  Fix up other fields  */

                        SourcePixels    = (double) Scale.wScanHeight;
                        wDestPixels     = (WORD) ((double) wScrollVal / 100 * SourcePixels);
                        Scale.wYPixels  = wDestPixels;
                        SetDlgItemInt     (hWnd, IDD_TARGET_Y, wDestPixels, FALSE);

                        if (bMaintainAspect)
                        {
                            hWndControl     = GetDlgItem (hWnd, IDD_XPERCENTSCROLL);
                            SetScrollPos      (hWndControl, SB_CTL, - (int) wScrollVal, TRUE);
                            SetDlgItemInt     (hWnd, IDD_XPERCENT, wScrollVal, FALSE);
                            SourcePixels    = (double) Scale.wScanWidth;
                            wDestPixels     = (WORD) ((double) wScrollVal / 100 * SourcePixels);
                            Scale.wXPixels  = wDestPixels;
                            SetDlgItemInt     (hWnd, IDD_TARGET_X, wDestPixels, FALSE);
                        }

                    }
                    break;


                case IDD_TARGET_X: 
                    if (HIWORD (lParam) == EN_KILLFOCUS)
                    {
                        WORD    wPercent;
                        WORD    wPixelVal;
                        BOOL    bTmp;
                        double  SourcePixels;
                        double  Percent;
                        HWND    hWndControl;

                        if (SendDlgItemMessage (hWnd, IDD_TARGET_X, EM_GETMODIFY, NULL, NULL) == FALSE)
                            break;

                        wPixelVal       = (WORD) GetScaleDlgItemInt (hWnd, IDD_TARGET_X, (BOOL FAR *) &bTmp, FALSE);
                        SetScaleDlgItem (hWnd, IDD_TARGET_X, wPixelVal, FALSE);
                        Scale.wXPixels = wPixelVal;    // NEW!

                        if (wPixelVal > MAXPIXELWIDTH)
                        {
                            MessageBeep (NULL);
                            SetScaleDlgItem    (hWnd, IDD_TARGET_X, MAXPIXELWIDTH, FALSE);
                            break;
                        }

                        SourcePixels    = (double) Scale.wScanWidth;
                        Percent         = ((double) wPixelVal / SourcePixels);
                        wPercent        = (WORD) (Percent * 100);
                        SetDlgItemInt     (hWnd, IDD_XPERCENT, wPercent, FALSE);
                        Scale.wXPercent = wPercent;
                        hWndControl     = GetDlgItem (hWnd, IDD_XPERCENTSCROLL);
                        SetScrollPos      (hWndControl, SB_CTL, - (int) wPercent, TRUE);

                        /*  Fix up Y fields if necessary  */

                        if (bMaintainAspect)
                        {
                            SourcePixels    = (double) Scale.wScanHeight;
                            wPixelVal       = (WORD) (Percent * (SourcePixels + .05));
                            SetScaleDlgItem        (hWnd, IDD_TARGET_Y, wPixelVal, FALSE);
                            SetDlgItemInt     (hWnd, IDD_YPERCENT, wPercent, FALSE);
                            Scale.wYPercent = wPercent;
                            hWndControl     = GetDlgItem (hWnd, IDD_YPERCENTSCROLL);
                            SetScrollPos      (hWndControl, SB_CTL, - (int) wPercent, TRUE);
                        }
                    }
                    break;

                case IDD_TARGET_Y: 
                    if (HIWORD (lParam) == EN_KILLFOCUS)
                    {
                        WORD    wPercent;
                        WORD    wPixelVal;
                        BOOL    bTmp;
                        double  SourcePixels;
                        double  Percent;
                        HWND    hWndControl;

                        if (SendDlgItemMessage (hWnd, IDD_TARGET_Y, EM_GETMODIFY, NULL, NULL) == FALSE)
                            break;

                        wPixelVal       = GetScaleDlgItemInt (hWnd, IDD_TARGET_Y, (BOOL FAR *) &bTmp, FALSE);
                        Scale.wYPixels  = wPixelVal;    // NEW!

                        SetScaleDlgItem (hWnd, IDD_TARGET_Y, wPixelVal, FALSE);

                        SourcePixels    = (double) Scale.wScanHeight;
                        Percent         = ((double) wPixelVal / SourcePixels);
                        wPercent        = (WORD) (Percent * 100);

                        if (bMaintainAspect)  // This test is a look-ahead for the image width to large
                            if ((Percent / 100 * SourcePixels) > MAXPIXELWIDTH)
                            {
                                MessageBeep (NULL);
                                SendDlgItemMessage (hWnd, IDD_TARGET_Y, EM_UNDO, NULL, NULL);
                                break;
                            }

                        SetDlgItemInt     (hWnd, IDD_YPERCENT, wPercent, FALSE);
                        Scale.wXPercent = wPercent;
                        hWndControl     = GetDlgItem (hWnd, IDD_YPERCENTSCROLL);
                        SetScrollPos      (hWndControl, SB_CTL, - (int) wPercent, TRUE);

                        /*  Fix up X fields if necessary  */

                        if (bMaintainAspect)
                        {
                            SourcePixels    = (double) Scale.wScanWidth;
                            wPixelVal       = (WORD) (Percent * (SourcePixels + .05));

                            SetScaleDlgItem     (hWnd, IDD_TARGET_X, wPixelVal, FALSE);
                            SetDlgItemInt     (hWnd, IDD_XPERCENT, wPercent, FALSE);
                            Scale.wXPercent = wPercent;
                            hWndControl     = GetDlgItem (hWnd, IDD_XPERCENTSCROLL);
                            SetScrollPos      (hWndControl, SB_CTL, - (int) wPercent, TRUE);
                        }
                    }
                    break;

                case IDD_PROCESS:

                    if (HIWORD (lParam) == CBN_SELCHANGE)
                    {
                        bScaleProcess = (BYTE) SendDlgItemMessage (hWnd, IDD_PROCESS, CB_GETCURSEL, NULL, NULL);
                        bScaleProcess++;    // Incr one
                    }
                    break;


                case IDD_UNITS:

                    if (HIWORD (lParam) == CBN_SELCHANGE)
                    {

                        HWND hWndControl;

                        bScaleUnits = (BYTE) SendDlgItemMessage (hWnd, IDD_UNITS, CB_GETCURSEL, NULL, NULL);
                        bScaleUnits++;    // Incr one

                        SetScaleDlgItem    (hWnd, IDD_TARGET_X, Scale.wXPixels, TRUE);
                        SetScaleDlgItem    (hWnd, IDD_TARGET_Y, Scale.wYPixels, TRUE);

                        hWndControl = GetDlgItem (hWnd, IDD_RESOLUTION);

                        if (bScaleUnits == PIXELS)
                            EnableWindow (hWndControl, FALSE);
                        else
                            EnableWindow (hWndControl, TRUE);

                    }
                    break;

                case IDD_RESOLUTION:

                    if (HIWORD (lParam) == EN_KILLFOCUS)
                    {
                        WORD wDPI;
                        BOOL bTmp;

                        /*  Test for 30 to 999 DPI ?? */

                        wDPI = (WORD) GetDlgItemInt (hWnd, IDD_RESOLUTION, (BOOL FAR *) &bTmp, FALSE);

                        if (wDPI != 0)
                        {

                            Scale.wResolution = wDPI;
                            
                            SetScaleDlgItem    (hWnd, IDD_TARGET_X, Scale.wXPixels, TRUE);
                            SetScaleDlgItem    (hWnd, IDD_TARGET_Y, Scale.wYPixels, TRUE);
                        }
                    }
                    break;


                case IDD_MAINTAIN_ASPECT:

                    bMaintainAspect = (BYTE) (! bMaintainAspect);
                    CheckDlgButton (hWnd, IDD_MAINTAIN_ASPECT, bMaintainAspect);
                    break;

                case IDOK:
                {
                    PSTR            pStringBuf;
                    PSTR            pStringBuf2;
                    HANDLE          hFocusWnd = (HANDLE) NULL;
                    OFSTRUCT        Of;
                    BOOL            bErr = FALSE;
                    char            Dest   [128];
                    int bTmp;

                    pStringBuf  = LocalLock (hStringBuf);
                    pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

                    /*  Data Validation  */

                    LoadString (hInstance, STR_SCALE, (LPSTR) pStringBuf2, MAXSTRINGSIZE);


                    /*  Test for zero pixel counts  */

                    Scale.wXPixels  = (WORD) GetScaleDlgItemInt (hWnd, IDD_TARGET_X, (BOOL FAR *) &bTmp, FALSE);
                    Scale.wYPixels  = (WORD) GetScaleDlgItemInt (hWnd, IDD_TARGET_Y, (BOOL FAR *) &bTmp, FALSE);

                    if (! bErr)
                    {
                        if (Scale.wXPixels == 0)
                        {
                            LoadString (hInstance, STR_NO_ZERO_PIXELS, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                            hFocusWnd = GetDlgItem (hWnd, IDD_TARGET_X);
                            bErr = TRUE;
                        }
                        else
                            if (Scale.wYPixels == 0)
                            {
                                LoadString (hInstance, STR_NO_ZERO_PIXELS, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                                hFocusWnd = GetDlgItem (hWnd, IDD_TARGET_Y);
                                bErr = TRUE;
                            }
                    }

                    /*  Test dest pixel width  */
                    if (! bErr)
                    {
                        int bTmp;
                    //  Scale.wXPixels  = (WORD) GetDlgItemInt (hWnd, IDD_TARGET_X, (BOOL FAR *) &bTmp, FALSE);
                        if (Scale.wXPixels > 4400)
                        {
                            LoadString (hInstance, STR_SCALED_TOO_WIDE, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                            hFocusWnd = GetDlgItem (hWnd, IDD_TARGET_X);
                            bErr = TRUE;
                        }
                    }

                    if (bErr)
                    {
                        MessageBeep (NULL);
                        MessageBox (hWnd, pStringBuf, pStringBuf2, MB_OK | MB_ICONQUESTION);
                    }
                    else
                        if (OpenFile ((LPSTR) Dest, (LPOFSTRUCT)&Of, OF_EXIST) > 0)
                        {
                            /*  File Exists, overwrite ? */

                            LoadString (hInstance, STR_IMAGE_EXISTS_CONFIRM, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                            LoadString (hInstance, STR_SCALE, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
                            if (MessageBox (hWnd, pStringBuf, pStringBuf2, MB_OKCANCEL | MB_ICONQUESTION) == IDCANCEL)
                                bErr = TRUE;
                        }       

                    LocalUnlock (hStringBuf);

                    if (! bErr)
                    {
                        BOOL bTmp;

                        /*  Fill in ScaleStruct (and setup the filename ptrs) */

                        Scale.wXPercent = (WORD) GetDlgItemInt (hWnd, IDD_XPERCENT, (BOOL FAR *) &bTmp, FALSE);
                        Scale.wYPercent = (WORD) GetDlgItemInt (hWnd, IDD_YPERCENT, (BOOL FAR *) &bTmp, FALSE);
                     // Scale.wXPixels  = (WORD) GetDlgItemInt (hWnd, IDD_TARGET_X, (BOOL FAR *) &bTmp, FALSE);
                     // Scale.wYPixels  = (WORD) GetDlgItemInt (hWnd, IDD_TARGET_Y, (BOOL FAR *) &bTmp, FALSE);
                        Scale.ScaleProcess = bScaleProcess;   // Maybe make this a universal variable ???
                        Scale.ScaleUnits   = bScaleUnits  ; 

                        /*  Change the system current drive / directory BACK to what it was  */

                        _chdrive (CurrWDir [0] - 96);
                        chdir (&CurrWDir[2]);

                        b1stScale = FALSE;
                        EndDialog (hWnd, TRUE);
                    }
                    else
                        SetFocus (hFocusWnd);

                }
                break;


                case IDCANCEL:
                    /*  Change the system current drive / directory and info
                        BACK to what it was  */

                    _chdrive (CurrWDir [0] - 96);
                    chdir (&CurrWDir[2]);

                    {   // Restore previously saved data
                        PSCALE pScale;        

                        pScale = (PSCALE) LocalLock (hStructBuf);
                        _fmemcpy ((LPSTR) &Scale, (LPSTR) pScale, sizeof (SCALE));
                        pScale -> pSource = pNameBuf;
                        pScale -> pDest   = pScale -> pSource + MAXPATHSIZE;
                        _fstrcpy ((LPSTR) SourcePath, (LPSTR) pScale -> pSource);
                        _fstrcpy ((LPSTR) DestPath, (LPSTR) pScale -> pDest);
                        LocalUnlock (hStructBuf);
                    }
                    EndDialog (hWnd, FALSE);
                    break;

            }
            break;    

        case WM_SHOWTEXT:
        {
            char Buffer [128];

            if (SendDlgItemMessage (hWnd, wParam, CB_GETLBTEXT, 0, (DWORD) (LPSTR) Buffer) != CB_ERR)
                SetDlgItemText (hWnd, wParam, (LPSTR) Buffer);
            SendDlgItemMessage (hWnd, wParam, CB_SETEDITSEL, NULL, MAKELONG (0, 128));
        }

        break;


        default:
            return (FALSE);
    }
    return (TRUE);
 }




void NEAR InitScale (hWnd, lParam)
HWND hWnd;
LONG lParam;
{
    HANDLE  hWndControl;
    PSTR            pStringBuf;
    PSTR            pStringBuf2;
    LPSTR           lpPath;
    CPIFILESPEC     CPIFileSpec;
    CPIIMAGESPEC    CPIImageSpec;
    OFSTRUCT        Of;
    int             hFile;
    WORD            wXMaxPercent;
    WORD            wYMaxPercent;
    HDC             hDC;
    int             nHorizRes;
//  int             nVertRes;       // Not used now

    pStringBuf  = LocalLock (hStringBuf);
    pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

    /*  Grab the screen resolution for a default output DPI value  */

    hDC = GetDC (hWnd);
    nHorizRes = GetDeviceCaps (hDC, LOGPIXELSX);
    ReleaseDC (hWnd, hDC);
    Scale.wResolution = (WORD) nHorizRes;
    SetDlgItemInt (hWnd, IDD_RESOLUTION, Scale.wResolution, FALSE);    
    hWndControl = GetDlgItem (hWnd, IDD_RESOLUTION);
    EnableWindow (hWndControl, FALSE);


    CheckDlgButton (hWnd, IDD_MAINTAIN_ASPECT, bMaintainAspect);

    /*  Fill in combo boxes  */

    LoadString (hInstance, STR_STRETCH, (LPSTR) pStringBuf, MAXSTRINGSIZE);
    SendDlgItemMessage (hWnd, IDD_PROCESS, CB_ADDSTRING, 0, (DWORD) (LPSTR) pStringBuf);
    LoadString (hInstance, STR_AVERAGE, (LPSTR) pStringBuf, MAXSTRINGSIZE);
    SendDlgItemMessage (hWnd, IDD_PROCESS, CB_ADDSTRING, 0, (DWORD) (LPSTR) pStringBuf);
    LoadString (hInstance, STR_ANTIALIAS, (LPSTR) pStringBuf, MAXSTRINGSIZE);
    SendDlgItemMessage (hWnd, IDD_PROCESS, CB_ADDSTRING, 0, (DWORD) (LPSTR) pStringBuf);

    LoadString (hInstance, STR_PIXELS, (LPSTR) pStringBuf, MAXSTRINGSIZE);
    SendDlgItemMessage (hWnd, IDD_UNITS, CB_ADDSTRING, 0, (DWORD) (LPSTR) pStringBuf);
    LoadString (hInstance, STR_INCHES, (LPSTR) pStringBuf, MAXSTRINGSIZE);
    SendDlgItemMessage (hWnd, IDD_UNITS, CB_ADDSTRING, 0, (DWORD) (LPSTR) pStringBuf);
    LoadString (hInstance, STR_CENTIMETERS, (LPSTR) pStringBuf, MAXSTRINGSIZE);
    SendDlgItemMessage (hWnd, IDD_UNITS, CB_ADDSTRING, 0, (DWORD) (LPSTR) pStringBuf);
    LoadString (hInstance, STR_MILLIMETERS, (LPSTR) pStringBuf, MAXSTRINGSIZE);
    SendDlgItemMessage (hWnd, IDD_UNITS, CB_ADDSTRING, 0, (DWORD) (LPSTR) pStringBuf);
    LoadString (hInstance, STR_POINTS, (LPSTR) pStringBuf, MAXSTRINGSIZE);
    SendDlgItemMessage (hWnd, IDD_UNITS, CB_ADDSTRING, 0, (DWORD) (LPSTR) pStringBuf);
    LoadString (hInstance, STR_PICAS, (LPSTR) pStringBuf, MAXSTRINGSIZE);
    SendDlgItemMessage (hWnd, IDD_UNITS, CB_ADDSTRING, 0, (DWORD) (LPSTR) pStringBuf);

    {
        if (b1stScale)
        {
            SendDlgItemMessage (hWnd, IDD_PROCESS, CB_SETCURSEL, ANTIALIAS_INDEX, NULL);
            SendDlgItemMessage (hWnd, IDD_UNITS  , CB_SETCURSEL, PIXELS_INDEX, NULL);

            bScaleProcess   = AVGINTERPOLATE;
            bScaleUnits     = PIXELS;
            Scale.wXPercent = 100;
            Scale.wYPercent = 100;
        }
        else
        {
            /*  Have already run a scale.  Put directories and settings 
                back to where they were.  */

            if (Scale.ScaleProcess == 0)
            {
                SendDlgItemMessage (hWnd, IDD_PROCESS, CB_SETCURSEL, ANTIALIAS_INDEX, NULL);
                SendDlgItemMessage (hWnd, IDD_PROCESS, CB_SETCURSEL, PIXELS_INDEX, NULL);
            }
            else
            {
                BYTE Offset = (BYTE) (Scale.ScaleProcess - TRUNCATION);
                SendDlgItemMessage (hWnd, IDD_PROCESS, CB_SETCURSEL, (STRETCH_INDEX + Offset), NULL);
                Offset = (BYTE) (Scale.ScaleUnits - PIXELS);
                SendDlgItemMessage (hWnd, IDD_UNITS  , CB_SETCURSEL, (PIXELS_INDEX  + Offset), NULL);
            }

            bScaleProcess = Scale.ScaleProcess;
            bScaleUnits   = Scale.ScaleUnits;
        }


        lpPath = (LPSTR) lParam;

/*  Put in stuff for finding out scanwidth and scanheight so stupid target fields have something  */

        hFile = OpenFile ((LPSTR) lpPath, (LPOFSTRUCT)&Of, OF_READWRITE);
        if (hFile > 0)
        {
            ReadCPIHdr (hFile, (LPCPIFILESPEC) &CPIFileSpec, (LPCPIIMAGESPEC) &CPIImageSpec);
                
            /*  Set default dialog values  */
                
//          Scale.wXPixels    = CPIImageSpec.X_Length;
//          Scale.wYPixels    = CPIImageSpec.Y_Length;

            Scale.wXPixels    = (WORD) ((double) CPIImageSpec.X_Length * Scale.wXPercent / 100);
            Scale.wYPixels    = (WORD) ((double) CPIImageSpec.Y_Length * Scale.wYPercent / 100);

            Scale.wScanWidth  = CPIImageSpec.X_Length;
            Scale.wScanHeight = CPIImageSpec.Y_Length;
    
            _lclose (hFile);
        }
        else
        {
            Scale.wXPixels = 0;
            Scale.wYPixels = 0;
        }   

//      Scale.wXPercent = 100;
//      Scale.wYPercent = 100;

        /*  Set up scroll ranges for percent based on size of image  */

        wXMaxPercent = (WORD) ((double) MAXPIXELWIDTH / Scale.wScanWidth * 100);
        wXMaxPercent = min (wXMaxPercent, MAXPERCENT);
        wYMaxPercent = wXMaxPercent;        // Has to be this way..

        hWndControl = GetDlgItem (hWnd, IDD_XPERCENTSCROLL);
        SetScrollRange (hWndControl, SB_CTL, -wXMaxPercent, -1, FALSE);
        hWndControl = GetDlgItem (hWnd, IDD_YPERCENTSCROLL);
        SetScrollRange (hWndControl, SB_CTL, -wYMaxPercent, -1, FALSE);


        hWndControl = GetDlgItem (hWnd, IDD_XPERCENTSCROLL);
        SetScrollPos (hWndControl, SB_CTL, - (int) Scale.wXPercent, FALSE);
        hWndControl = GetDlgItem (hWnd, IDD_YPERCENTSCROLL);
        SetScrollPos (hWndControl, SB_CTL, - (int) Scale.wYPercent, FALSE);

//      SetDlgItemInt (hWnd, IDD_TARGET_X, Scale.wXPixels, TRUE);
//      SetDlgItemInt (hWnd, IDD_TARGET_Y, Scale.wYPixels, TRUE);

        SetScaleDlgItem    (hWnd, IDD_TARGET_X, Scale.wXPixels, TRUE);
        SetScaleDlgItem    (hWnd, IDD_TARGET_Y, Scale.wYPixels, TRUE);

        SetDlgItemInt (hWnd, IDD_XPERCENT, (Scale.wXPercent), TRUE);
        SetDlgItemInt (hWnd, IDD_YPERCENT, (Scale.wYPercent), TRUE);

    }

    LocalUnlock (hStringBuf);
}

         
void NEAR SetScaleDlgItem    (HWND hWnd, int nControl, WORD wPixels, BOOL Flags)
{
 
    LONG   lValue;
    double dValue;
    PSTR   pStringBuf;
    BOOL   bUseDecimal = TRUE;
    BOOL   bUseThousandths = TRUE;

    dValue = (double) wPixels;
    dValue += .05;

    pStringBuf  = LocalLock (hStringBuf);

    switch (bScaleUnits)
    {
        case PIXELS:
            bUseDecimal = FALSE;
        break;

        case INCHES:
            dValue = (dValue / Scale.wResolution);
        break;

        case CENTIMETERS:
            dValue = (dValue / Scale.wResolution * 2.54);
            bUseThousandths = FALSE;
        break;

        case MILLIMETERS:
            dValue = (dValue / Scale.wResolution * 25.4);
            bUseThousandths = FALSE;
        break;                          

        case POINTS:
            dValue = (dValue / Scale.wResolution * 72);
            bUseThousandths = FALSE;
        break;

        case PICAS:
            dValue = (dValue / Scale.wResolution * 12);
            bUseThousandths = FALSE;
        break;
    }

    lValue = (LONG) (dValue        * 1000.0);

    if (bUseDecimal)
        if (bUseThousandths)
            wsprintf ((LPSTR) pStringBuf, "%4ld.%-3ld", (lValue / 1000), (lValue % 1000));
        else
            wsprintf ((LPSTR) pStringBuf, "%4ld.%-2ld", (lValue / 1000), (lValue % 100));
    else
        wsprintf ((LPSTR) pStringBuf, "%4ld", (lValue / 1000));


    SetDlgItemText (hWnd, nControl, (LPSTR) pStringBuf);

    LocalUnlock (hStringBuf);
}


int  NEAR GetScaleDlgItemInt (HWND hWnd, int nControl, BOOL FAR *bTmp, BOOL Flags)
{
    int  nValue;
    double dValue;
    PSTR   pStringBuf;


    pStringBuf  = LocalLock (hStringBuf);

    GetDlgItemText (hWnd, nControl, (LPSTR) pStringBuf, MAXSTRINGSIZE);
    dValue = atof (pStringBuf);

//  dValue += .05;

    switch (bScaleUnits)
    {
        case PIXELS:
            nValue = (int) dValue;
        break;

        case INCHES:
            nValue = (int) (dValue * Scale.wResolution);
        break;

        case CENTIMETERS:
            nValue = (int) (dValue * Scale.wResolution / 2.54);
        break;        

        case MILLIMETERS:
            nValue = (int) (dValue * Scale.wResolution / 25.4);
        break;

        case POINTS:
            nValue = (int) (dValue * Scale.wResolution / 72);
        break;

        case PICAS:
            nValue = (int) (dValue * Scale.wResolution / 12);
        break;

    }
    LocalUnlock (hStringBuf);

    return (nValue);
}




