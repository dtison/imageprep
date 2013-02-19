
#include <windows.h>
#include <imgprep.h>
#include <global.h>
#include <strtable.h>


#define wTmpOPRColors TmpValPtr [0]

BOOL FAR PASCAL OPRDlgProc(hWnd, Message, wParam, lParam)
HWND      hWnd;
unsigned  Message;
WORD      wParam;
LONG      lParam;
{

    HWND  hWndControl = 0; 
    int  nScrollPos; 
    WORD *TmpValPtr = (WORD *) &TmpVals [0];


    switch (Message)
    {
        case WM_INITDIALOG:

            wTmpOPRColors = wOPRColors;
            if (wTmpOPRColors == 16)
                CheckDlgButton (hWnd, 101, 1);
            else
                if (wTmpOPRColors == 256)
                    CheckDlgButton (hWnd, 102, 1);
            SetDlgItemInt (hWnd, 103, wTmpOPRColors, FALSE);

            hWndControl = GetDlgItem (hWnd, 105);
            SetScrollRange (hWndControl, SB_CTL, -256, -4, FALSE);
            SetScrollPos (hWndControl, SB_CTL, -wTmpOPRColors, FALSE);

            break;


        case WM_VSCROLL:

            hWndControl = GetDlgItem (hWnd, 105);
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
                    nScrollPos += 50;
                    break;

                case SB_PAGEDOWN:
                    nScrollPos -= 50;
                    break;

         	    case SB_THUMBTRACK:
                case SB_THUMBPOSITION: 
                    nScrollPos = -LOWORD (lParam);
   		            break;
            }
            if (nScrollPos < 4)
                nScrollPos = 4;
            if (nScrollPos > 256)
                nScrollPos = 256;

            wTmpOPRColors = nScrollPos;

            SetScrollPos (hWndControl, SB_CTL, -nScrollPos, TRUE);
            SetDlgItemInt (hWnd, 103, nScrollPos, FALSE);

            if (wTmpOPRColors == 16)
                CheckDlgButton (hWnd, 101, 1);
            else
                CheckDlgButton (hWnd, 101, 0);

            if (wTmpOPRColors == 256)
                CheckDlgButton (hWnd, 102, 1);
            else
                CheckDlgButton (hWnd, 102, 0);

        break;



        case WM_COMMAND:
            switch (wParam)
            {
                case 101:
                    CheckDlgButton (hWnd, 102, 0);
                    CheckDlgButton (hWnd, 101, 1);
                    wTmpOPRColors = 16;
                    SetDlgItemInt (hWnd, 103, wTmpOPRColors, FALSE);
                    hWndControl = GetDlgItem (hWnd, 105);
                    SetScrollPos (hWndControl, SB_CTL, wTmpOPRColors, TRUE);

                    break;

                case 102:
                    CheckDlgButton (hWnd, 101, 0);
                    CheckDlgButton (hWnd, 102, 1);
                    wTmpOPRColors = 256;
                    SetDlgItemInt (hWnd, 103, wTmpOPRColors, FALSE);
                    hWndControl = GetDlgItem (hWnd, 105);
                    SetScrollPos (hWndControl, SB_CTL, wTmpOPRColors, TRUE);
                    break;

                case 103:   // Edit box
                    if (HIWORD (lParam) == EN_KILLFOCUS)
                    {
                        wTmpOPRColors = GetDlgItemInt (hWnd, 103, (WORD FAR *) &wTmpOPRColors, FALSE);
                        wTmpOPRColors = min (256, max (4, wTmpOPRColors));
                        hWndControl = GetDlgItem (hWnd, 105);
                        SetScrollPos (hWndControl, SB_CTL, -(int) wTmpOPRColors, TRUE);
                        SetDlgItemInt (hWnd, 103, wTmpOPRColors, FALSE);

                        if (wTmpOPRColors == 16)
                            CheckDlgButton (hWnd, 101, 1);
                        else
                            CheckDlgButton (hWnd, 101, 0);
                        if (wTmpOPRColors == 256)
                            CheckDlgButton (hWnd, 102, 1);
                        else
                            CheckDlgButton (hWnd, 102, 0);

                    #ifdef NEVER
                    {
                        char szMessage [80];
                        wsprintf (szMessage, "Dialog: wTmpOPRColors is: %d",wTmpOPRColors);
                        MessageBox (hWnd, szMessage, NULL, MB_OK);   
                    }
                    #endif

                    }

                    break;


                case IDOK:

                    wOPRColors = (WORD) GetDlgItemInt (hWnd, 103, (WORD FAR *) &wTmpOPRColors, FALSE);
                    wTmpOPRColors = 0;

                    EndDialog (hWnd, TRUE);
                    break;

                case IDCANCEL:
                    wTmpOPRColors = 0;
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
