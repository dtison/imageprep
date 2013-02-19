/*---------------------------------------------------------------------------

    EXIT.C - 
               

    CREATED:    2/91   D. Ison

--------------------------------------------------------------------------*/

/*  Undefs to expedite compilation */

  
#define  NOKANJI
#define  NOPROFILER
#define  NOSOUND
#define  NOCOMM
#define  NOGDICAPMASKS
#define  NOVIRTUALKEYCODES
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
#include <imgprep.h>
#include <proto.h>
#include <global.h>
#include <cpi.h>      
#include "res\exitdlg.h"
#include <strtable.h>
#include <string.h>

DWORD FAR ConfirmExit ()
{
    FARPROC     lpfnExitDlg;

    lpfnExitDlg = MakeProcInstance ((FARPROC)ExitDlgProc, hInstIP);

    if (DialogBox (hInstIP, "EXITDLG", hWndIP, lpfnExitDlg))
        return (1);
    else
        return (0);

    FreeProcInstance (lpfnExitDlg);

    return (1);
}


DWORD FAR PASCAL ExitDlgProc (hWnd, Message, wParam, lParam)
HWND      hWnd;
unsigned  Message;
WORD      wParam;
LONG      lParam;
{
    switch (Message)
    {
        case WM_INITDIALOG:
        {
            RECT  Rect;

            PSTR pStringBuf;
            PSTR pStringBuf2;

            pStringBuf  = LocalLock (hStringBuf);
            pStringBuf2 = pStringBuf + MAXSTRINGSIZE;


            /* Center the dialog       */

            GetWindowRect (hWnd,&Rect);
            SetWindowPos  (hWnd,NULL,
                          (GetSystemMetrics(SM_CXSCREEN) - (Rect.right - Rect.left)) / 2,
                          (GetSystemMetrics(SM_CYSCREEN) - (Rect.bottom - Rect.top)) / 3,
                           0, 0, SWP_NOSIZE | SWP_NOACTIVATE);


            if (image_active)   
            {
                MessageBeep (0);
                LoadString (hInstIP, STR_CAUTION, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                #ifdef COLORS
                {
                    HDC hDC;
                    HWND hWndControl;

                    hWndControl = GetDlgItem (hWnd, IDD_CAUTION);
                    hDC = GetDC (hWndControl);

                    SetTextColor (hDC, RGB (255, 0, 0));

                    TextOut (hDC, 0, 0, (LPSTR) pStringBuf, _fstrlen (pStringBuf));
                    ReleaseDC (hWnd, hDC);
                }
                #endif
                SetDlgItemText (hWnd, IDD_CAUTION,  (LPSTR) pStringBuf);
            }

            LoadString (hInstIP, STR_CLOSEAPP, (LPSTR) pStringBuf, MAXSTRINGSIZE);
            wsprintf ((LPSTR) pStringBuf2, (LPSTR) pStringBuf, (LPSTR) szAppName);
            SetDlgItemText (hWnd, IDD_CLOSEAPP,  (LPSTR) pStringBuf2);

            CheckDlgButton (hWnd, IDD_SAVESETTINGS, bSaveSettings);

            LocalUnlock (hStringBuf);
        }
        break;
        case WM_COMMAND:

        switch (wParam)
        {
            case IDOK:
                EndDialog (hWnd, TRUE);
            break;

            case IDD_SAVESETTINGS:
                bSaveSettings = (BYTE) (! bSaveSettings);
                CheckDlgButton (hWnd, IDD_SAVESETTINGS, bSaveSettings);
                break;

            case IDCANCEL:
                EndDialog (hWnd, FALSE);
            break;
        }
        break;

        default:
            return (FALSE);
    }
    return (TRUE);
}



