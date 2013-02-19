
/*---------------------------------------------------------------------------

    EDGEDLG.C   Enhance Edge Tool interface manager.  Accepts input from the user
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
#include "edgedlg.h"
#include "internal.h"
#include "strtable.h"
#include "global.h"
#include <tools.h>

DWORD FAR PASCAL EdgeDlgProc (hWnd, Message, wParam, lParam)
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

            InitEnhanceEdge (hWnd, lParam, Edge.Filter);
            {
                PEDGE pEdge;        // Temp place for saving current data

                pEdge = (PEDGE) LocalLock (hStructBuf);
                _fmemcpy ((LPSTR) pEdge, (LPSTR) &Edge, sizeof (EDGE));

                LocalUnlock (hStructBuf);
            }
            if (Edge.Filter == EDGE_ENHANCE)
                LoadString (hInstance, STR_ENHANCE_EDGE, (LPSTR) pStringBuf, MAXSTRINGSIZE);
            else
            {   
                HWND hWndControl;

                /*  Also hide the "Level" fields  */

                LoadString (hInstance, STR_CREATE_LINE_DRAWING, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                #ifdef NEVER
                hWndControl = GetDlgItem (hWnd, IDD_LEVELSCROLL);
                ShowWindow (hWndControl, SW_HIDE);
                hWndControl = GetDlgItem (hWnd, IDD_LEVEL);
                ShowWindow (hWndControl, SW_HIDE);
                hWndControl = GetDlgItem (hWnd, 113);
                ShowWindow (hWndControl, SW_HIDE);
                #endif
            }
            SetWindowText (hWnd, (LPSTR) pStringBuf); // Caption on dialog
            LocalUnlock (hStringBuf);
        }
        break;
    
        case WM_VSCROLL:
        {
            HWND hWndControl;
            int  nScrollPos;
            int  nControl;

            hWndControl =  (HWND) HIWORD (lParam);
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
                nScrollPos += 20;
                break;

                case SB_PAGEDOWN:
                nScrollPos -= 20;
                break;

              case SB_THUMBTRACK:
                case SB_THUMBPOSITION: 
                nScrollPos = -LOWORD (lParam);
   		        break;
            }
            if (nScrollPos < 1)
                nScrollPos = 1;
            if (nScrollPos > 100)
                nScrollPos = 100;

            SetScrollPos (hWndControl, SB_CTL, -nScrollPos, TRUE);
            nControl = GetDlgCtrlID (hWndControl);
            SetDlgItemInt (hWnd, (nControl + 1), nScrollPos, FALSE);    // Edit box is scroll ID plus 1
        }
        break;

        case WM_COMMAND:
            switch (wParam)
            {

                case IDD_LEVEL: // Edit box(es)
                    if (HIWORD (lParam) == EN_KILLFOCUS)
                    {
                        WORD wScrollVal;
                        BOOL bTmp;
                        HWND hWndControl;

                        wScrollVal = GetDlgItemInt (hWnd, IDD_LEVEL, (BOOL FAR *) &bTmp, FALSE);
                        wScrollVal =  min (100, max (1, wScrollVal));
                        hWndControl = GetDlgItem (hWnd, IDD_LEVELSCROLL);
                        SetScrollPos (hWndControl, SB_CTL, - (int) wScrollVal, TRUE);
                        SetDlgItemInt (hWnd, IDD_LEVEL, wScrollVal, FALSE);

                    }
                    break;

                case IDD_SENSITIVITY: 
                    if (HIWORD (lParam) == EN_KILLFOCUS)
                    {
                        WORD wScrollVal;
                        BOOL bTmp;
                        HWND hWndControl;

                        wScrollVal = GetDlgItemInt (hWnd, IDD_SENSITIVITY, (BOOL FAR *) &bTmp, FALSE);
                        wScrollVal =  min (100, max (1, wScrollVal));
                        hWndControl = GetDlgItem (hWnd, IDD_SENSITIVITY_SCROLL);
                        SetScrollPos (hWndControl, SB_CTL, - (int) wScrollVal, TRUE);
                        SetDlgItemInt (hWnd, IDD_SENSITIVITY, wScrollVal, FALSE);

                    }
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

                    LoadString (hInstance, STR_ENHANCE_EDGE, (LPSTR) pStringBuf2, MAXSTRINGSIZE);


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
                            LoadString (hInstance, STR_ENHANCE_EDGE, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
                            if (MessageBox (hWnd, pStringBuf, pStringBuf2, MB_OKCANCEL | MB_ICONQUESTION) == IDCANCEL)
                                bErr = TRUE;
                        }       

                    LocalUnlock (hStringBuf);

                    if (! bErr)
                    {
                        BOOL bTmp;

                        /*  Fill in EdgeenStruct (and setup the filename ptrs) */

                        Edge.Level       = (BYTE) GetDlgItemInt (hWnd, IDD_LEVEL, (BOOL FAR *) &bTmp, FALSE);
                        Edge.Sensitivity = (BYTE) GetDlgItemInt (hWnd, IDD_SENSITIVITY, (BOOL FAR *) &bTmp, FALSE);

                        /*  Change the system current drive / directory BACK to what it was  */

                        _chdrive (CurrWDir [0] - 96);
                        chdir (&CurrWDir[2]);

                        b1stEnhanceEdge = FALSE;
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
                        PEDGE pEdge;

                        pEdge = (PEDGE) LocalLock (hStructBuf);
                        _fmemcpy ((LPSTR) &Edge, (LPSTR) pEdge, sizeof (EDGE));
                        pEdge -> pSource = pNameBuf;
                        pEdge -> pDest   = pEdge -> pSource + MAXPATHSIZE;
                        _fstrcpy ((LPSTR) SourcePath, (LPSTR) pEdge -> pSource);
                        _fstrcpy ((LPSTR) DestPath, (LPSTR) pEdge -> pDest);
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



void NEAR InitEnhanceEdge (hWnd, lParam, nFilter)
HWND hWnd;
LONG lParam;
int  nFilter;
{
    HANDLE  hWndLevelControl;
    HANDLE  hWndSensControl;
    char    Buffer [128];
    PSTR            pStringBuf;
    PSTR            pStringBuf2;
    LPSTR           lpPath;


    pStringBuf  = LocalLock (hStringBuf);
    pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

    if (nFilter == EDGE_ENHANCE)
    {
        hWndLevelControl = GetDlgItem (hWnd, IDD_LEVELSCROLL);
        SetScrollRange (hWndLevelControl, SB_CTL, -100, -1, FALSE);
    }

    hWndSensControl = GetDlgItem (hWnd, IDD_SENSITIVITY_SCROLL);
    SetScrollRange (hWndSensControl, SB_CTL, -100, -1, FALSE);

    {
        if (b1stEnhanceEdge)
        {
            if (nFilter == EDGE_ENHANCE)
            {
                SetScrollPos (hWndLevelControl, SB_CTL, -30, FALSE);
                SetDlgItemInt (hWnd, IDD_LEVEL, 30, TRUE);
            }

            SetScrollPos (hWndSensControl, SB_CTL, -50, FALSE);
            SetDlgItemInt (hWnd, IDD_SENSITIVITY, 50, TRUE);
        }
        else
        {
            /*  Have already run an edge.  Put directories and settings 
                back to where they were.  */

            if (nFilter == EDGE_ENHANCE)
            {
                SetScrollPos (hWndLevelControl, SB_CTL, Edge.Level == -30 ? -1 : -(Edge.Level), FALSE);
                SetDlgItemInt (hWnd, IDD_LEVEL, (Edge.Level), TRUE);
            }

            SetScrollPos (hWndSensControl, SB_CTL, Edge.Sensitivity == -50 ? -1 : -(Edge.Sensitivity), FALSE);
            SetDlgItemInt (hWnd, IDD_SENSITIVITY, (Edge.Sensitivity), TRUE);
        }


    }

    LocalUnlock (hStringBuf);
}

