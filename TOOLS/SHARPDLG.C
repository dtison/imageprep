
/*---------------------------------------------------------------------------

    SHARPDLG.C  Sharpen Tool interface manager.  Accepts input from the user
                and validates input ...

    CREATED:    2/91   D. Ison

--------------------------------------------------------------------------*/

#include <windows.h>
#include <string.h>
#include <cpi.h>      
#include <cpifmt.h>
#include <malloc.h>
#include <io.h>
#include <prometer.h>
#include <direct.h>
#include <sharpdlg.h>
#include "internal.h"
#include "strtable.h"
#include "global.h"

#define SHARPLEVELS 20

DWORD FAR PASCAL SharpDlgProc (hWnd, Message, wParam, lParam)
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

            InitSharpen (hWnd, lParam);
            {
                PSHARP  pSharp;        // Temp place for saving current data

                pSharp = (PSHARP) LocalLock (hStructBuf);
                _fmemcpy ((LPSTR) pSharp, (LPSTR) &Sharp, sizeof (SHARP));
                LocalUnlock (hStructBuf);
            }

            LoadString (hInstance, STR_SHARPEN, (LPSTR) pStringBuf, MAXSTRINGSIZE);
            SetWindowText (hWnd, (LPSTR) pStringBuf); // Caption on dialog
            LocalUnlock (hStringBuf);
        }
        break;
    
        case WM_VSCROLL:
        {
            HWND hWndControl;
            int  nScrollPos;

            hWndControl = GetDlgItem (hWnd, IDD_LEVELSCROLL);
            nScrollPos  = -GetScrollPos (hWndControl, SB_CTL);

            switch (wParam)
            {
                case SB_LINEUP:
                nScrollPos++;
                break;

                case SB_LINEDOWN:
                nScrollPos--;
                break;

                case SB_PAGEUP:
                nScrollPos += 5;
                break;

                case SB_PAGEDOWN:
                nScrollPos -= 5;
                break;

              case SB_THUMBTRACK:
                case SB_THUMBPOSITION: 
                nScrollPos = -LOWORD (lParam);
   		        break;
            }
            if (nScrollPos < 1)
                nScrollPos = 1;
            if (nScrollPos > SHARPLEVELS)
                nScrollPos = SHARPLEVELS;

            SetScrollPos (hWndControl, SB_CTL, -nScrollPos, TRUE);
            SetDlgItemInt (hWnd, IDD_LEVEL, nScrollPos, FALSE);
        }
        break;

        case WM_COMMAND:
            switch (wParam)
            {
                case IDD_LEVEL: // Edit box
                    if (HIWORD (lParam) == EN_KILLFOCUS)
                    {
                        WORD wScrollVal;
                        BOOL bTmp;
                        HWND hWndControl;

                        wScrollVal = GetDlgItemInt (hWnd, IDD_LEVEL, (BOOL FAR *) &bTmp, FALSE);
                        wScrollVal = min (SHARPLEVELS, max (0, wScrollVal));
                        hWndControl = GetDlgItem (hWnd, IDD_LEVELSCROLL);
                        SetScrollPos (hWndControl, SB_CTL, - (int) wScrollVal, TRUE);
                        SetDlgItemInt (hWnd, IDD_LEVEL, wScrollVal, FALSE);

                    }
                    break;

                case IDD_UNSHARP:
                case IDD_HIGHPASS:
                    CheckRadioButton (hWnd, IDD_UNSHARP, IDD_HIGHPASS, wParam);
                    bSharpFilter = (BYTE) (wParam == IDD_HIGHPASS ? HIGHPASS : UNSHARP);
                    break;

                case IDOK:
                {
                    char            Buffer [128];
                    PSTR            pStringBuf;
                    PSTR            pStringBuf2;
                    HANDLE          hFocusWnd = (HANDLE) NULL;
                    OFSTRUCT        Of;
                    BOOL            bErr = FALSE;
                    char            Source [128];
                    char            Dest   [128];
                    char            Temp   [128];

                    pStringBuf  = LocalLock (hStringBuf);
                    pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

                    /*  Data Validation  */

                    LoadString (hInstance, STR_SHARPEN, (LPSTR) pStringBuf2, MAXSTRINGSIZE);


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
                            LoadString (hInstance, STR_SHARPEN, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
                            if (MessageBox (hWnd, pStringBuf, pStringBuf2, MB_OKCANCEL | MB_ICONQUESTION) == IDCANCEL)
                                bErr = TRUE;
                        }       

                    LocalUnlock (hStringBuf);

                    if (! bErr)
                    {
                        BOOL bTmp;

                        /*  Fill in SharpenStruct (and setup the filename ptrs) */

                        Sharp.Level = (BYTE) GetDlgItemInt (hWnd, IDD_LEVEL, (BOOL FAR *) &bTmp, FALSE);
                        Sharp.SharpFilter = bSharpFilter;   // Maybe make this a universal variable ???

                        /*  Change the system current drive / directory BACK to what it was  */

                        _chdrive (CurrWDir [0] - 96);
                        chdir (&CurrWDir[2]);

                        b1stSharpen = FALSE;
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
                        PSHARP pSharp;        

                        pSharp = (PSHARP) LocalLock (hStructBuf);
                        _fmemcpy ((LPSTR) &Sharp, (LPSTR) pSharp, sizeof (SHARP));
                        pSharp -> pSource = pNameBuf;
                        pSharp -> pDest   = pSharp -> pSource + MAXPATHSIZE;
                        _fstrcpy ((LPSTR) SourcePath, (LPSTR) pSharp -> pSource);
                        _fstrcpy ((LPSTR) DestPath, (LPSTR) pSharp -> pDest);
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



void NEAR InitSharpen (hWnd, lParam)
HWND hWnd;
LONG lParam;
{
    HANDLE  hWndControl;
    char    Buffer [128];
    PSTR            pStringBuf;
    PSTR            pStringBuf2;
    LPSTR           lpPath;

    pStringBuf  = LocalLock (hStringBuf);
    pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

    hWndControl = GetDlgItem (hWnd, IDD_LEVELSCROLL);
    SetScrollRange (hWndControl, SB_CTL, -SHARPLEVELS, -1, FALSE);

    {
        if (b1stSharpen)
        {
            CheckRadioButton (hWnd, IDD_UNSHARP, IDD_HIGHPASS, IDD_UNSHARP);
            bSharpFilter = UNSHARP;

            SetScrollPos (hWndControl, SB_CTL, -1, FALSE);
            SetDlgItemInt (hWnd, IDD_LEVEL, 1, TRUE);
        }
        else
        {
            /*  Have already run a sharpen.  Put directories and settings 
                back to where they were.  */

            if (Sharp.SharpFilter == 0)
                CheckRadioButton (hWnd, IDD_UNSHARP, IDD_HIGHPASS, IDD_UNSHARP);
            else
                CheckRadioButton (hWnd, IDD_UNSHARP, IDD_HIGHPASS, (Sharp.SharpFilter == UNSHARP ? IDD_UNSHARP : IDD_HIGHPASS));

            bSharpFilter = Sharp.SharpFilter;

            SetScrollPos (hWndControl, SB_CTL, Sharp.Level == 0 ? -1 : -(Sharp.Level), FALSE);
            SetDlgItemInt (hWnd, IDD_LEVEL, (Sharp.Level), TRUE);
        }
    }
    

    LocalUnlock (hStringBuf);
}


