
#include <windows.h>
#include "imgprep.h"
#include "resource.h"
#include "strtable.h"
#include "proto.h"
#include "global.h"
#include "save.h"
#include "res\procspec.h"
#include "setup.h"
#include "error.h"
#include "string.h"
#include "menu.h"

/*---------------------------------------------------------------------------

   PROCEDURE:         ProcSpecDlgProc   
   DESCRIPTION:       Setup image Processing dialog
   DEFINITION:        2.8
   START:             5/15/91  D. Ison

----------------------------------------------------------------------------*/

void FAR ConformDialog  (HWND, WORD);
void FAR RGBClassDialog (HWND);
void FAR CMAPClassDialog (HWND);
void FAR GRAYClassDialog (HWND);
void FAR MONOClassDialog (HWND);

#define INITIALIZE_CLASS     0   
#define INITIALIZE_PALETTE   1


#define DEFAULT_COLOR       0
#define OPR_COLOR           1
#define GRAYSCALE           2
#define BLACK_WHITE         3

#define NO_DITHER           4
#define ORDERED_DITHER      5
#define FS_DITHER           6
#define BK_DITHER           7
#define LIN_DITHER          8  
#define GRAYSCALE_4BIT      9
#define FS_DITHER_4BIT      10

#define PALETTES            0
#define DITHERS             4


#define DEFAULT_COLOR_INDEX     0
#define OPR_COLOR_INDEX         1
#define GRAYSCALE_INDEX         2 
#define GRAYSCALE_4BIT_INDEX    3
#define BLACK_WHITE_INDEX       4

#define NO_DITHER_INDEX         0
#define ORDERED_DITHER_INDEX    1
#define FS_DITHER_INDEX         2
#define BK_DITHER_INDEX         3
#define LIN_DITHER_INDEX        4

#define INVERT_COLORS           0
#define DITHER_QUALITY          1
#define DITHER_RESOLUTION       2

BYTE Palettes [5];
BYTE Dithers  [5];

void NEAR SetupClassCombos (HWND, LPSTR, LPSTR);


int FAR PASCAL ProcSpecDlgProc (hWnd, Message, wParam, lParam)
HWND        hWnd;
unsigned    Message;
WORD        wParam;
LONG        lParam;
{
    BOOL        bReturn = TRUE;                          
    RECT        Rect;
    LPSTR       lpString;

    switch (Message)
    {
        case WM_INITDIALOG:

        int_dither = wDither; 
        int_pal    = wPal;    
        old_pal    = dPalType;


        hGlobalString = GlobalAlloc (GHND, 4096);
        if (! hGlobalString)
            return (EC_NOMEM);

        lpString = GlobalLock (hGlobalString);


        /* Center the window       */

        GetWindowRect (hWnd,&Rect);
        SetWindowPos  (hWnd,NULL,
                      (GetSystemMetrics (SM_CXSCREEN) - (Rect.right - Rect.left)) / 2,
                      (GetSystemMetrics (SM_CYSCREEN) - (Rect.bottom - Rect.top)) / 3,
                       0, 0, SWP_NOSIZE | SWP_NOACTIVATE);


        /*  Setup Selection Strings  */


        lpStrings [DEFAULT_COLOR] = lpString;
        LoadString (hInstIP, STR_DEFAULT_COLOR, lpString, MAXSTRINGSIZE);
        lpString += _fstrlen (lpString) + 1;

        lpStrings [OPR_COLOR] = lpString;
        LoadString (hInstIP, STR_OPR_COLOR, lpString, MAXSTRINGSIZE);
        lpString += _fstrlen (lpString) + 1;

        lpStrings [GRAYSCALE] = lpString;
        LoadString (hInstIP, STR_GRAYSCALE, lpString, MAXSTRINGSIZE);
        lpString += _fstrlen (lpString) + 1;

        lpStrings [GRAYSCALE_4BIT] = lpString;
        LoadString (hInstIP, STR_GRAYSCALE_4BIT, lpString, MAXSTRINGSIZE);
        lpString += _fstrlen (lpString) + 1;

        lpStrings [BLACK_WHITE] = lpString;
        LoadString (hInstIP, STR_BLACK_WHITE, lpString, MAXSTRINGSIZE);
        lpString += _fstrlen (lpString) + 1;

        lpStrings [NO_DITHER] = lpString;
        LoadString (hInstIP, STR_NO_DITHER, lpString, MAXSTRINGSIZE);
        lpString += _fstrlen (lpString) + 1;

        lpStrings [ORDERED_DITHER] = lpString;
        LoadString (hInstIP, STR_ORDERED_DITHER, lpString, MAXSTRINGSIZE);
        lpString += _fstrlen (lpString) + 1;

        lpStrings [FS_DITHER] = lpString;
        LoadString (hInstIP, STR_FS_DITHER2, lpString, MAXSTRINGSIZE);
        lpString += _fstrlen (lpString) + 1;

        lpStrings [BK_DITHER] = lpString;
        LoadString (hInstIP, STR_BK_DITHER2, lpString, MAXSTRINGSIZE);
        lpString += _fstrlen (lpString) + 1;

        lpStrings [LIN_DITHER] = lpString;
        LoadString (hInstIP, STR_LIN_DITHER2, lpString, MAXSTRINGSIZE);
        lpString += _fstrlen (lpString) + 1;

        ConformDialog (hWnd, wImportClass);

        int_pal    = (int) wPal;        // Remember old palette and dither settings
        int_dither = (int) wDither;

        CheckDlgButton (hWnd, IDD_INVERT_COLORS, (WORD) bInvertColors);
        TmpVals [INVERT_COLORS]  = bInvertColors;

        TmpVals [DITHER_QUALITY] = (BYTE) ((wDithQuality == 1) ? TRUE : FALSE);
        CheckRadioButton (hWnd, IDD_RELAXED, IDD_STRICT, (IDD_RELAXED + TmpVals [DITHER_QUALITY]));

        TmpVals [DITHER_RESOLUTION] = (BYTE) ((wDither != IFS16) ? TRUE : FALSE);

        CheckRadioButton (hWnd, IDD_4BIT, IDD_8BIT, (IDD_4BIT + TmpVals [DITHER_RESOLUTION]));

        EnableWindow (GetDlgItem (hWnd, IDD_4BIT), FALSE);
        EnableWindow (GetDlgItem (hWnd, IDD_8BIT), FALSE);
        EnableWindow (GetDlgItem (hWnd, IDD_OPR_SETUP), (wPal == IOPT && wDither != IFS16) ? TRUE : FALSE);

        GlobalUnlock (hGlobalString);

        break;

        case WM_COMMAND:
            switch (wParam)
            {
                case IDD_PALETTES:
                {
                    WORD wTmp = HIWORD (lParam);
                    WORD wTmpIndex;
                    WORD wParam;

                    if (wTmp == CBN_SELCHANGE)
                    {
                        /*  Get the new palette selection  */

                        wTmpIndex = (WORD) SendDlgItemMessage (hWnd, IDD_PALETTES, CB_GETCURSEL, NULL, NULL);


                        wParam = Palettes [wTmpIndex];

                        switch (wParam)
                        {
                            case DEFAULT_COLOR_INDEX:
                                wParam = (wDither == IFS16 ? IDM_4BIT_COLOR : IDM_8BIT_COLOR);
                                break;
                            case OPR_COLOR_INDEX:
                                wParam = (wDither == IFS16 ? IDM_4BIT_OPR : IDM_8BIT_OPR);
                                break;
                            case GRAYSCALE_INDEX:
                                wParam = IDM_GRAYSCALE;
                                break;
                            case GRAYSCALE_4BIT_INDEX:
                                wParam = IDM_GRAYSCALE16;
                                break;
                            case BLACK_WHITE_INDEX:
                                wParam = IDM_MONOCHROME;
                                break;
                        }

                        if (wParam == IDM_GRAYSCALE16)
                            SelGray16  (hWndIP, wParam);
                        else
                            SelPalette (hWndIP, wParam);

                        EnableWindow (GetDlgItem (hWnd, IDD_OPR_SETUP), (wPal == IOPT && wDither != IFS16) ? TRUE : FALSE);
                        


                        /*  Now reset all the dropdown contents stuff  */

                        ConformDialog (hWnd, wImportClass);

                    }
                }
                break;

                case IDD_DITHERS:
                {
                    WORD wTmp = HIWORD (lParam);
                    WORD wTmpIndex;
                    WORD wParam;

                    if (wTmp == CBN_SELCHANGE)
                    {
                        /*  Get the new dither selection  */

                        wTmpIndex = (WORD) SendDlgItemMessage (hWnd, IDD_DITHERS, CB_GETCURSEL, NULL, NULL);

                        wParam = Dithers [wTmpIndex];

                        switch (wParam + DITHERS)
                        {
                            case NO_DITHER:
                                wParam = IDM_NO_DITHER;
                                break;
                            case ORDERED_DITHER:
                                wParam = IDM_ORDERED;
                                break;
                            case FS_DITHER:
                                wParam = IDM_FS;
                                break;
                            case BK_DITHER:
                                wParam = IDM_BURKE;
                                break;
                            case LIN_DITHER:
                                wParam = IDM_FAST;
                                break;
                        }

                        SelDither (hWndIP, wParam);

                        /*  Now reset all the dropdown contents stuff  */

                        ConformDialog (hWnd, wImportClass);
                    }
                }
                break;

                case IDD_INVERT_COLORS:

                    CheckDlgButton (hWnd, wParam, (! IsDlgButtonChecked (hWnd, wParam)));
                    break;

                case IDD_RELAXED:
                case IDD_STRICT:

                    TmpVals [DITHER_QUALITY] = (BYTE) (wParam == IDD_STRICT);
                    CheckRadioButton (hWnd, IDD_RELAXED, IDD_STRICT, (IDD_RELAXED + TmpVals [DITHER_QUALITY]));
                    break;

                case IDD_OPR_SETUP:
                {
                    FARPROC lpfnDlg;
                    BOOL    bSuccess;

                    lpfnDlg = MakeProcInstance ((FARPROC)OPRDlgProc, hInstIP);
                    bSuccess = DialogBox (hInstIP, (LPSTR)"OPRSETUP", hWndIP, lpfnDlg);
                    if (bSuccess)
                        SendMessage (hWnd, WM_COMMAND, IDM_8BIT_OPR, NULL);

                }
                break;

                case IDOK:

                    SetState (PAINT_STATE, TRUE);

                    if ((wDither == IFS) && (wPal == IOPT))
                    {
                        if ((int_dither != IFS) && (int_pal == IOPT))
                            bNewOpt = TRUE;
                    }
                    bReturn = TRUE;

                    if (image_active)
                    {
                        GLOBALFREE (hImpPalette);

                        /*  This put in to allow interruptable paints.  4/91  D.Ison  */

                        FreeFilters (hFileInfo, IMPORT_CLASS);

                        SetImportStates ();
                        FreePrevDispPalMem (dPalType, old_pal);
                        if ((wDither == IFS) && (wPal == IOPT)) 
                            if (! bOptDither)
                            bNewOpt = TRUE;
                    }
                    bInvertColors = (BYTE) IsDlgButtonChecked (hWnd, IDD_INVERT_COLORS);
                    if (TmpVals [INVERT_COLORS] != bInvertColors)   // Then a change was made
                        UpdateCorrectionTable (hGlobalCTable);

                    wDithQuality = TmpVals [DITHER_QUALITY] ? 1 : 8;

                    SendMessage (hWndIP, WM_COMMAND, IDM_4BIT_COLOR + TmpVals [DITHER_RESOLUTION], 1L);

                    EndDialog (hWnd, bReturn);
                    break;

                case IDCANCEL:
                    SetState (DITH_STATE, int_dither);      // Restore old palette and dither settings
                    SetState (PAL_STATE, int_pal);
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


void FAR ConformDialog (hWnd, wImportClass)
HWND    hWnd;
WORD    wImportClass;
{

    /*  First reset all   */

    SendDlgItemMessage (hWnd, IDD_PALETTES, CB_RESETCONTENT, 0, 0L);
    SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_RESETCONTENT, 0, 0L);

    switch (wImportClass)
    {
        case IRGB:
            RGBClassDialog (hWnd);
            break;

        case ICM:
            CMAPClassDialog (hWnd);
            break;

        case IGR:
            GRAYClassDialog (hWnd);
            break;

        case IMON:
            MONOClassDialog (hWnd);
            break;
    }


    if (wDither == INO16 || wDither == IFS16)
        CheckRadioButton (hWnd, IDD_4BIT, IDD_8BIT, IDD_4BIT);
    else
        CheckRadioButton (hWnd, IDD_4BIT, IDD_8BIT, IDD_8BIT);

}

void FAR RGBClassDialog (hWnd)
HWND hWnd;
{
    BYTE  TmpPalettes [5];
    BYTE  TmpDithers  [5];
    WORD  wTmpPal;
    WORD  wTmpDith;
    int   i;

    SendDlgItemMessage (hWnd, IDD_PALETTES, WM_SETREDRAW, FALSE, 0L);
    SendDlgItemMessage (hWnd, IDD_DITHERS , WM_SETREDRAW, FALSE, 0L);


    SetupClassCombos (hWnd, (LPSTR) TmpPalettes, (LPSTR) TmpDithers);

    /*  Now delete everything that doesn't apply to current import class / palette selection  */

    switch (wPal)
    {
        /*  Alway delete strings in reverse order so indexes will be correct  */
        case IOPT:
            SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, (ORDERED_DITHER_INDEX), NULL);
            TmpDithers [ORDERED_DITHER_INDEX] = (BYTE) FALSE;
        break;
    
        case IGR:
            SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, LIN_DITHER_INDEX, NULL);
            SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, BK_DITHER_INDEX, NULL);
            SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, FS_DITHER_INDEX, NULL);
            SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, ORDERED_DITHER_INDEX, NULL);

            TmpDithers [LIN_DITHER_INDEX]       = (BYTE) FALSE;
            TmpDithers [BK_DITHER_INDEX]        = (BYTE) FALSE;
            TmpDithers [FS_DITHER_INDEX]        = (BYTE) FALSE;
            TmpDithers [ORDERED_DITHER_INDEX]   = (BYTE) FALSE;


            // Grayed 4bit 8 bit color stuff...
        break;
    
        case IBW:
            SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, LIN_DITHER_INDEX, NULL);            
            SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, BK_DITHER_INDEX, NULL);

            TmpDithers [LIN_DITHER_INDEX]       = (BYTE) FALSE;
            TmpDithers [BK_DITHER_INDEX]        = (BYTE) FALSE;

            // Grayed 4bit 8 bit color stuff... also
        break;
    }

    if (wDither == IFS16)
    {
        /*  Alway delete strings in reverse order so indexes will be correct  */

        SendDlgItemMessage (hWnd, IDD_PALETTES, CB_DELETESTRING, BLACK_WHITE_INDEX, NULL);
        SendDlgItemMessage (hWnd, IDD_PALETTES, CB_DELETESTRING, GRAYSCALE_4BIT_INDEX, NULL);
        SendDlgItemMessage (hWnd, IDD_PALETTES, CB_DELETESTRING, GRAYSCALE_INDEX, NULL);

        if (wPal != IOPT)
            SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, ORDERED_DITHER_INDEX, NULL);

        SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, NO_DITHER_INDEX, NULL);

        TmpPalettes [GRAYSCALE_INDEX]       = (BYTE) FALSE;
        TmpPalettes [GRAYSCALE_4BIT_INDEX]  = (BYTE) FALSE;
        TmpPalettes [BLACK_WHITE_INDEX]     = (BYTE) FALSE;

        if (wPal != IOPT)
            TmpDithers [ORDERED_DITHER_INDEX]   = (BYTE) FALSE;

        TmpDithers [NO_DITHER_INDEX]        = (BYTE) FALSE;
    }

    /*  Now setup main Palette and Dither remap tables  */

    wTmpPal  = 0;
    wTmpDith = 0;

    for (i = 0; i < 5; i++)
    {
        if (TmpPalettes [i])
        {
            Palettes [wTmpPal] = (BYTE) i;
            wTmpPal++;
        }

        if (TmpDithers [i])
        {
            Dithers [wTmpDith] = (BYTE) i;
            wTmpDith++;
        }
    }

    SendDlgItemMessage (hWnd, IDD_DITHERS , WM_SETREDRAW, TRUE, 0L);
    SendDlgItemMessage (hWnd, IDD_PALETTES, WM_SETREDRAW, TRUE, 0L);
}



void FAR CMAPClassDialog (hWnd)
HWND hWnd;
{
    BYTE  TmpPalettes [5];
    BYTE  TmpDithers  [5];
    WORD  wTmpPal;
    WORD  wTmpDith;
    int   i;

    SendDlgItemMessage (hWnd, IDD_PALETTES, WM_SETREDRAW, FALSE, 0L);
    SendDlgItemMessage (hWnd, IDD_DITHERS , WM_SETREDRAW, FALSE, 0L);

    SetupClassCombos (hWnd, (LPSTR) TmpPalettes, (LPSTR) TmpDithers);

    /*  Now delete everything that doesn't apply to current import class / palette selection  */

    switch (wPal)
    {
        case IBW:
            SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, LIN_DITHER_INDEX, NULL);
            SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, BK_DITHER_INDEX, NULL);

            TmpDithers [LIN_DITHER_INDEX]       = (BYTE) FALSE;
            TmpDithers [BK_DITHER_INDEX]        = (BYTE) FALSE;
        break;


        default:

            SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, LIN_DITHER_INDEX, NULL);
            SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, BK_DITHER_INDEX, NULL);
            SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, FS_DITHER_INDEX, NULL);
            SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, ORDERED_DITHER_INDEX, NULL);

            TmpDithers [LIN_DITHER_INDEX]       = (BYTE) FALSE;
            TmpDithers [BK_DITHER_INDEX]        = (BYTE) FALSE;
            TmpDithers [FS_DITHER_INDEX]        = (BYTE) FALSE;
            TmpDithers [ORDERED_DITHER_INDEX]   = (BYTE) FALSE;

        break;
    }

    SendDlgItemMessage (hWnd, IDD_PALETTES,  CB_DELETESTRING, OPR_COLOR_INDEX, NULL);            
    TmpPalettes [OPR_COLOR_INDEX] = (BYTE) FALSE;


    /*  Now setup main Palette and Dither remap tables  */

    wTmpPal  = 0;
    wTmpDith = 0;

    for (i = 0; i < 5; i++)
    {
        if (TmpPalettes [i])
        {
            Palettes [wTmpPal] = (BYTE) i;
            wTmpPal++;
        }

        if (TmpDithers [i])
        {
            Dithers [wTmpDith] = (BYTE) i;
            wTmpDith++;
        }
    }

    SendDlgItemMessage (hWnd, IDD_DITHERS , WM_SETREDRAW, TRUE, 0L);
    SendDlgItemMessage (hWnd, IDD_PALETTES, WM_SETREDRAW, TRUE, 0L);
}

void FAR GRAYClassDialog (hWnd)
HWND hWnd;
{
    BYTE  TmpPalettes [5];
    BYTE  TmpDithers  [5];
    WORD  wTmpPal;
    WORD  wTmpDith;
    int   i;

    SendDlgItemMessage (hWnd, IDD_PALETTES, WM_SETREDRAW, FALSE, 0L);
    SendDlgItemMessage (hWnd, IDD_DITHERS , WM_SETREDRAW, FALSE, 0L);

    SetupClassCombos (hWnd, (LPSTR) TmpPalettes, (LPSTR) TmpDithers);

    /*  Now delete everything that doesn't apply to current import class / palette selection  */

    switch (wPal)
    {
        case IBW:
            SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, LIN_DITHER_INDEX, NULL);
            SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, BK_DITHER_INDEX, NULL);

            TmpDithers [LIN_DITHER_INDEX]       = (BYTE) FALSE;
            TmpDithers [BK_DITHER_INDEX]        = (BYTE) FALSE;
        break;


        default:

            SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, LIN_DITHER_INDEX, NULL);
            SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, BK_DITHER_INDEX, NULL);
            SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, FS_DITHER_INDEX, NULL);
            SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, ORDERED_DITHER_INDEX, NULL);

            TmpDithers [LIN_DITHER_INDEX]       = (BYTE) FALSE;
            TmpDithers [BK_DITHER_INDEX]        = (BYTE) FALSE;
            TmpDithers [FS_DITHER_INDEX]        = (BYTE) FALSE;
            TmpDithers [ORDERED_DITHER_INDEX]   = (BYTE) FALSE;

        break;
    }

    SendDlgItemMessage (hWnd, IDD_PALETTES,  CB_DELETESTRING, OPR_COLOR_INDEX, NULL); 
    TmpPalettes [OPR_COLOR_INDEX] = (BYTE) FALSE;

    SendDlgItemMessage (hWnd, IDD_PALETTES,  CB_DELETESTRING, DEFAULT_COLOR_INDEX, NULL); 
    TmpPalettes [DEFAULT_COLOR_INDEX] = (BYTE) FALSE;

    /*  Now setup main Palette and Dither remap tables  */

    wTmpPal  = 0;
    wTmpDith = 0;

    for (i = 0; i < 5; i++)
    {
        if (TmpPalettes [i])
        {
            Palettes [wTmpPal] = (BYTE) i;
            wTmpPal++;
        }

        if (TmpDithers [i])
        {
            Dithers [wTmpDith] = (BYTE) i;
            wTmpDith++;
        }
    }

    SendDlgItemMessage (hWnd, IDD_DITHERS , WM_SETREDRAW, TRUE, 0L);
    SendDlgItemMessage (hWnd, IDD_PALETTES, WM_SETREDRAW, TRUE, 0L);
}



void FAR MONOClassDialog (hWnd)
HWND hWnd;
{
    BYTE  TmpPalettes [5];
    BYTE  TmpDithers  [5];
    WORD  wTmpPal;
    WORD  wTmpDith;
    int   i;

    SendDlgItemMessage (hWnd, IDD_PALETTES, WM_SETREDRAW, FALSE, 0L);
    SendDlgItemMessage (hWnd, IDD_DITHERS , WM_SETREDRAW, FALSE, 0L);

    SetupClassCombos (hWnd, (LPSTR) TmpPalettes, (LPSTR) TmpDithers);

    /*  Now delete everything that doesn't apply to current import class / palette selection  */


    SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, LIN_DITHER_INDEX, NULL);
    SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, BK_DITHER_INDEX, NULL);
    SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, FS_DITHER_INDEX, NULL);
    SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_DELETESTRING, ORDERED_DITHER_INDEX, NULL);

    TmpDithers [LIN_DITHER_INDEX]       = (BYTE) FALSE;
    TmpDithers [BK_DITHER_INDEX]        = (BYTE) FALSE;
    TmpDithers [FS_DITHER_INDEX]        = (BYTE) FALSE;
    TmpDithers [ORDERED_DITHER_INDEX]   = (BYTE) FALSE;

    SendDlgItemMessage (hWnd, IDD_PALETTES,  CB_DELETESTRING, GRAYSCALE_4BIT_INDEX, NULL);
    SendDlgItemMessage (hWnd, IDD_PALETTES,  CB_DELETESTRING, GRAYSCALE_INDEX, NULL);
    SendDlgItemMessage (hWnd, IDD_PALETTES,  CB_DELETESTRING, OPR_COLOR_INDEX, NULL);
    SendDlgItemMessage (hWnd, IDD_PALETTES,  CB_DELETESTRING, DEFAULT_COLOR_INDEX, NULL);

    TmpPalettes [GRAYSCALE_4BIT_INDEX] = (BYTE) FALSE;
    TmpPalettes [GRAYSCALE_INDEX]      = (BYTE) FALSE;
    TmpPalettes [OPR_COLOR_INDEX]      = (BYTE) FALSE;
    TmpPalettes [DEFAULT_COLOR_INDEX]  = (BYTE) FALSE;

    /*  Now setup main Palette and Dither remap tables  */

    wTmpPal  = 0;
    wTmpDith = 0;

    for (i = 0; i < 5; i++)
    {
        if (TmpPalettes [i])
        {
            Palettes [wTmpPal] = (BYTE) i;
            wTmpPal++;
        }

        if (TmpDithers [i])
        {
            Dithers [wTmpDith] = (BYTE) i;
            wTmpDith++;
        }
    }

    SendDlgItemMessage (hWnd, IDD_DITHERS , WM_SETREDRAW, TRUE, 0L);
    SendDlgItemMessage (hWnd, IDD_PALETTES, WM_SETREDRAW, TRUE, 0L);
}


int FreePrevDispPalMem (wNewPal, wOldPal)
WORD wNewPal;
WORD wOldPal;
{
  if (wOldPal != wNewPal){
    SetState (PAINT_STATE, TRUE);
    switch (wOldPal){
      case DP256:    
      case DP8:
      case DP16:
        bNewDefault = TRUE;
        break;
      case OP256:
      case OP8:
      case OP16:
        bNewOpt    = TRUE;
        bOptDither = FALSE;
// HIST TEMP delete was here 
        break;
      case GP256:
      case GP64:
      case GP16:
      #ifdef NEVER
      case GP8:
      #endif
      case BP:
      case IP256:
        break;
    }
  }
  return (0);
}


void NEAR SetupClassCombos (HWND hWnd, LPSTR lpTmpPalettes, LPSTR lpTmpDithers)
{

    /*  Palettes  */

    SendDlgItemMessage (hWnd, IDD_PALETTES, CB_ADDSTRING, 0, (DWORD) lpStrings [DEFAULT_COLOR]);
    SendDlgItemMessage (hWnd, IDD_PALETTES, CB_ADDSTRING, 0, (DWORD) lpStrings [OPR_COLOR]);
    SendDlgItemMessage (hWnd, IDD_PALETTES, CB_ADDSTRING, 0, (DWORD) lpStrings [GRAYSCALE]);
    SendDlgItemMessage (hWnd, IDD_PALETTES, CB_ADDSTRING, 0, (DWORD) lpStrings [GRAYSCALE_4BIT]);
    SendDlgItemMessage (hWnd, IDD_PALETTES, CB_ADDSTRING, 0, (DWORD) lpStrings [BLACK_WHITE]);

    /*  Mark default states  */

    lpTmpPalettes [DEFAULT_COLOR_INDEX]   = (BYTE) TRUE;
    lpTmpPalettes [OPR_COLOR_INDEX]       = (BYTE) TRUE;
    lpTmpPalettes [GRAYSCALE_INDEX]       = (BYTE) TRUE;
    lpTmpPalettes [GRAYSCALE_4BIT_INDEX]  = (BYTE) TRUE;
    lpTmpPalettes [BLACK_WHITE_INDEX]     = (BYTE) TRUE;

    /*  Dithers  */

    SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_ADDSTRING, 0, (DWORD) lpStrings [NO_DITHER]);
    SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_ADDSTRING, 0, (DWORD) lpStrings [ORDERED_DITHER]);
    SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_ADDSTRING, 0, (DWORD) lpStrings [FS_DITHER]);
    SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_ADDSTRING, 0, (DWORD) lpStrings [BK_DITHER]);
    SendDlgItemMessage (hWnd, IDD_DITHERS,  CB_ADDSTRING, 0, (DWORD) lpStrings [LIN_DITHER]);

    /*  Mark default states  */

    lpTmpDithers [NO_DITHER_INDEX]        = (BYTE) TRUE;
    lpTmpDithers [ORDERED_DITHER_INDEX]   = (BYTE) TRUE;
    lpTmpDithers [FS_DITHER_INDEX]        = (BYTE) TRUE;
    lpTmpDithers [BK_DITHER_INDEX]        = (BYTE) TRUE;
    lpTmpDithers [LIN_DITHER_INDEX]       = (BYTE) TRUE;


    /*  Setup current wPal and wDither selections  */

    switch (wPal)
    {
        case INORM:
            SendDlgItemMessage (hWnd, IDD_PALETTES, CB_SETCURSEL, DEFAULT_COLOR_INDEX, NULL);
            break;

        case IOPT:
            SendDlgItemMessage (hWnd, IDD_PALETTES, CB_SETCURSEL, OPR_COLOR_INDEX, NULL);
            break;

        case IGRA:
            SendDlgItemMessage (hWnd, IDD_PALETTES, CB_SETCURSEL, wDither == INO16 ? GRAYSCALE_4BIT_INDEX : GRAYSCALE_INDEX, NULL);
            break;

        case IMON:
            SendDlgItemMessage (hWnd, IDD_PALETTES, CB_SETCURSEL, BLACK_WHITE_INDEX, NULL);

    }

    switch (wDither)
    {
        case INONE:
            SendDlgItemMessage (hWnd, IDD_DITHERS , CB_SETCURSEL, NO_DITHER_INDEX, NULL);
            break;

        case IBAY:
            SendDlgItemMessage (hWnd, IDD_DITHERS , CB_SETCURSEL, ORDERED_DITHER_INDEX, NULL);
            break;

        case IFS:  
        case IFS16:
            SendDlgItemMessage (hWnd, IDD_DITHERS , CB_SETCURSEL, (FS_DITHER_INDEX + wCurrDither - 1), NULL);
            break;

        case INO16:
            SendDlgItemMessage (hWnd, IDD_DITHERS , CB_SETCURSEL, NO_DITHER_INDEX, NULL);
            break;

    }
}
