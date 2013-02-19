#include <windows.h>
#include "res\pref.h"
#include <imgprep.h>
#include <global.h>
#include "clipimg.h"
#include "strtable.h"

/*---------------------------------------------------------------------------

   PROCEDURE:         PrefDlg
   DESCRIPTION:       Pref Dialog Proc
   START:             5/15/91  D. Ison

----------------------------------------------------------------------------*/

#define CLIP_OPTIONS 0


BOOL FAR PASCAL PrefDlgProc (hWnd, Message, wParam, lParam)
HWND      hWnd;
unsigned  Message;
WORD      wParam;
LONG      lParam;
{
    int  nVal;

    switch (Message)
    {
        case WM_INITDIALOG:
        {
            RECT Rect;

            /* Center the dialog       */

            GetWindowRect (hWnd,&Rect);
            SetWindowPos  (hWnd,NULL,
                          (GetSystemMetrics(SM_CXSCREEN) - (Rect.right - Rect.left)) / 2,
                          (GetSystemMetrics(SM_CYSCREEN) - (Rect.bottom - Rect.top)) / 3,
                           0, 0, SWP_NOSIZE | SWP_NOACTIVATE);


            CheckDlgButton (hWnd, IDD_AUTO_CONVERT, (WORD) bAutoConvert);
            CheckDlgButton (hWnd, IDD_RETAIN_SETTINGS, (WORD) (! bResetProc));

            /*  Hide the Save Defaults check box  */

            ShowWindow (GetDlgItem (hWnd, IDD_SAVE_DEF), FALSE);


            /*  Setup the clipboard checkboxes  */

            nVal = (bClipOptions & CLPF_DIB);
            CheckDlgButton (hWnd, IDD_DIB, (WORD) nVal);

            nVal = (bClipOptions & CLPF_BITMAP);
            CheckDlgButton (hWnd, IDD_DDB, (WORD) nVal);

            nVal = (bClipOptions & CLPF_METAFILE);
            CheckDlgButton (hWnd, IDD_WMF, (WORD) nVal);

//          TmpVals [CLIP_OPTIONS] = (BYTE) bClipOptions;
        }
        break;
        case WM_COMMAND:

        switch (wParam)
        {
            case IDD_DIB:
            case IDD_DDB:
            case IDD_WMF:
            case IDD_AUTO_CONVERT:
            case IDD_RETAIN_SETTINGS:
                CheckDlgButton (hWnd, wParam, (! IsDlgButtonChecked (hWnd, wParam)));
            break;

            case IDOK:

                /*  Detect if user unchecked all clipboard formats  */

                if (! IsDlgButtonChecked (hWnd, IDD_DIB))
                    if (! IsDlgButtonChecked (hWnd, IDD_DDB))
                        if (! IsDlgButtonChecked (hWnd, IDD_WMF))
                        {
                            PSTR pStringBuf;
                            PSTR pStringBuf2;

                            pStringBuf  = LocalLock (hStringBuf);
                            pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

                            LoadString (hInstIP, STR_PREFERENCES, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
                            LoadString (hInstIP, STR_CLIPBOARD_FMT_DIB, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                  
                            MessageBox (hWnd, (LPSTR) pStringBuf, (LPSTR) pStringBuf2, MB_ICONASTERISK | MB_OK);
                            LocalUnlock (hStringBuf);

                            CheckDlgButton (hWnd, IDD_DIB, TRUE);
                        }
                bClipOptions = 0;
                nVal = FALSE;

                if (IsDlgButtonChecked (hWnd, IDD_DIB))
                {
                    bClipOptions |= CLPF_DIB;
                    nVal = TRUE;
                }

                EnableClipFormat (CLPF_DIB, nVal);

                nVal = FALSE;
                if (IsDlgButtonChecked (hWnd, IDD_DDB))
                {
                    bClipOptions |= CLPF_BITMAP;
                    nVal = TRUE;
                }
                EnableClipFormat (CLPF_BITMAP, nVal);

                nVal = FALSE;
                if (IsDlgButtonChecked (hWnd, IDD_WMF))
                {
                    bClipOptions |= CLPF_METAFILE;
                    nVal = TRUE;
                }
                EnableClipFormat (CLPF_METAFILE, nVal);

                bAutoConvert = (BYTE) IsDlgButtonChecked (hWnd, IDD_AUTO_CONVERT);
                bResetProc   = (BYTE) (! IsDlgButtonChecked (hWnd, IDD_RETAIN_SETTINGS));

                bPrefChanged = TRUE;

                EndDialog (hWnd, TRUE);
            break;

            case IDCANCEL:
                EndDialog (hWnd, FALSE);
            break;
            default:
            return (FALSE);
        }
        break;

        default:
            return (FALSE);
    }
    return (TRUE);
}





