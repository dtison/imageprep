#include <windows.h>
#include <string.h>
#include "imgprep.h"
#include "proto.h"
#include "global.h"
#include "clipimg.h"
#include "print.h"
#include "menu.h"
#include "capture.h"
#include "gamma.h"
#include "color.h"
#include "balance.h"
#include "strtable.h"
#include "tools.h"
#include "startup.h"
#include "cpifmt.h"
#include "merge.h"

#define PROCESS_MENU  3
#define PALETTE_MENU  0
#define DEF_MENU      0
#define OPR_MENU      1
#define GRAY_MENU     2


#ifdef COLORLAB
#include "scanlib.h"
#endif

#ifdef DEMO
char *szDemoVersion = "ImagePrep Demo Version";
#endif


int NEAR CPITools (HWND, int (FAR PASCAL *) (HWND, LPSTR, LPSTR, int), int);

LONG FAR MenuCommand (HWND hWnd, unsigned Message, WORD wParam, LONG lParam)
{
    BOOL      bSuccess;
    FARPROC   lpfnDlg;                         
    int       nErr = 0;                        

      #ifdef HELP
      /* Was F1 just pressed in a menu, or are we in help mode */
      /* (Shift-F1)? */
      if (bHelp) 
      {
        dwHelpContextId =
                   (wParam == IDM_OPEN)   ? (DWORD) HELPID_FILE_OPEN    :
                   (wParam == IDM_SAVE)   ? (DWORD) HELPID_FILE_SAVE    :
                   (wParam == IDM_EXIT)   ? (DWORD) HELPID_FILE_EXIT    :
                                            (DWORD) 0L;

        if (!dwHelpContextId)
	      {
          MessageBox (hWnd, "Help not available for Help Menu item", "Help Example", MB_OK);  // REPORT ERR
          return (DefWindowProc(hWnd, Message, wParam, lParam));
	      }

        bHelp = FALSE;
        WinHelp(hWnd,(LPSTR) "helpex.hlp",HELP_CONTEXT,dwHelpContextId);
      }
      else
      #endif

      switch (wParam)   
      {
        case IDM_LOGO:
            bLogo = (BYTE) ShowLogo (hWnd);
            if (wDisplay == IVGA)
            {
              PSTR pStringBuf;
              PSTR pStringBuf2;

              pStringBuf  = LocalLock (hStringBuf);
              pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

              LoadString (hInstIP, STR_VGA_LIMITATION, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
              LoadString (hInstIP, STR_VGA_MESSAGE, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                  
              MessageBox (hWnd, (LPSTR) pStringBuf, (LPSTR) pStringBuf2, MB_ICONHAND | MB_OK);
              LocalUnlock (hStringBuf);
            }
            break;

          #ifdef COLORLAB
          case IDM_SCAN:
          {
            OFSTRUCT Of;
            char Buffer [MAXPATHSIZE];

            bIsScanFile = FALSE;

            GetTempFileName (0, (LPSTR) "IMG", 0, (LPSTR) Buffer);

            OpenFile ((LPSTR) Buffer, (LPOFSTRUCT)&Of, OF_PARSE);
            bSuccess = ScanImage (hWndIP, (LPSTR)&Of.szPathName);

          }
          break;
          #endif

          case IDM_OPEN:

            nErr = OpenImage (USER_OPEN, NULL);
            if (nErr < 0)
                ReportError (nErr, NULL);

            break;

            #ifdef DISPOFF
              if (nErr < 0)
              {
                ReportError (nErr, NULL);
                break;
              }

              if (nErr != IDCANCEL)
              {
                if (! bDoDisplay)
                    PostMessage (hWnd, WM_COMMAND, IDM_SAVE, 0L);
                else
            #endif

          break;

          case IDM_SAVE:

          bIsSaving = TRUE;
          nErr = DoSave (SAVE_NORMAL);
          bIsSaving = FALSE;
          if (nErr < 0) 
            ReportError (nErr, NULL);
          if (image_active)
          {
            if (nErr != IDCANCEL)
            {
              RECT  rcUpdate;
              GetUpdateRect (hWndDisplay, (LPRECT)&rcUpdate, FALSE);
              if (!IsRectEmpty ((LPRECT)&rcUpdate))
                InvalidateRect (hWndDisplay, (LPRECT)NULL, FALSE);
              
            }
          }
          break;   
          
          case IDM_CLOSE:

          if (image_active)
              SendMessage (hWndDisplay, WM_CLOSE, NULL, 0L);
          break;
          case IDM_ABOUT:
          lpfnDlg = MakeProcInstance ((FARPROC)AboutDlgProc, hInstIP);
          DialogBox (hInstIP, (LPSTR)"ABOUTBOX", hWndIP, lpfnDlg);
          FreeProcInstance (lpfnDlg);
          break;


          case IDM_DISPLAY_TOGGLE:
          {
            WORD   wChecked;
            WORD   wStringID;
            char   szStringBuf [256];
            HMENU  hMenu = GetMenu (hWnd);

            bDoDisplay = (BYTE) ! bDoDisplay;

            if (bDoDisplay)
            {
                wChecked  = MF_CHECKED;
                wStringID = STR_MENU_DISPLAY_ON;

                /*  Set palette and dither back to defaults  */

                wDither = (WORD) bDefDither;
                wPal     = (WORD) bDefPalette;
            }

            else
            {
                wChecked = MF_UNCHECKED;
                wStringID = STR_MENU_DISPLAY_OFF;
            }

            LoadString (hInstIP, wStringID, (LPSTR) szStringBuf, sizeof (szStringBuf) - 1);
    
            ChangeMenu (hMenu, 
                        IDM_DISPLAY_TOGGLE,
                        (LPSTR)szStringBuf,
                        IDM_DISPLAY_TOGGLE, 
                        MF_BYCOMMAND | MF_CHANGE | MF_ENABLED);

            CheckMenuItem (hMenu, IDM_DISPLAY_TOGGLE, wChecked);

          }
          break;

          case IDM_ACTUALSIZE:
          {
            HMENU  hMenu = GetMenu (hWnd);
            wFullView    = FALSE;

            CheckMenuItem (hMenu, IDM_ACTUALSIZE, MF_CHECKED);
            CheckMenuItem (hMenu, IDM_FITINWINDOW, MF_UNCHECKED);

            if (image_active && IsWindow (hWndDisplay))  // Safety test..
            {
                /*  Need to put on some scrollbars  and InvalidateRect....  */

                SetupDispWnd (hWndDisplay, hFileInfo);
            //  bIsFirstPaint = TRUE;
                InvalidateRect (hWndDisplay, (LPRECT) NULL, FALSE);
            }
          }
          break;

          case IDM_FITINWINDOW:
          {
            HMENU  hMenu = GetMenu (hWnd);
            wFullView    = TRUE;

            CheckMenuItem (hMenu, IDM_ACTUALSIZE, MF_UNCHECKED);
            CheckMenuItem (hMenu, IDM_FITINWINDOW, MF_CHECKED);

            /*  Scrollbars go away if window displayed  */

            if (image_active && IsWindow (hWndDisplay))
            {
                SetupDispWnd (hWndDisplay, hFileInfo);
            //  bIsFirstPaint = TRUE;
                InvalidateRect (hWndDisplay, (LPRECT) NULL, FALSE);
            }
          }
          break;

          case IDM_VIEWNORMAL:
          {
            HDC       hDC;
            HPALETTE  hPalette;
            HMENU     hMenu = GetMenu (hWnd);

            if (! wPal)
                bIsFirstPaint = TRUE;

            bDisplayQuality = FALSE;
            bTrueColor      = FALSE;

            /*  Setup palette usage  */

            CheckMenuItem (hMenu, IDM_VIEWNORMAL, MF_CHECKED);
            CheckMenuItem (hMenu, IDM_VIEWQUALITY, MF_UNCHECKED);
            CheckMenuItem (hMenu, IDM_VIEWTRUECOLOR, MF_UNCHECKED);

            SetClassLong (hWnd, GCW_HBRBACKGROUND, GetStockObject (LTGRAY_BRUSH));
            if (image_active)
              SetClassLong (hWndDisplay, GCW_HBRBACKGROUND, GetStockObject (LTGRAY_BRUSH));

            hDC = GetDC (hWnd);
            SetSystemPaletteUse (hDC, SYSPAL_STATIC);
            if (hDispPalette)
            {
                UnrealizeObject (hDispPalette);
                hPalette = SelectPalette (hDC, hDispPalette, 0);
                RealizePalette (hDC);
                hDispPalette = SelectPalette (hDC, hPalette, 0);
            }
            SetSysColors (NUMSYSCOLORS, (LPINT)&SysColorIndices,(LPDWORD)&DefaultSysColorValues);
            ReleaseDC (hWnd, hDC);
            SendMessage (-1, WM_SYSCOLORCHANGE, (WORD)NULL, (LONG)NULL);
          }
          break;

          case IDM_VIEWQUALITY:
          {
            HDC       hDC;
            HPALETTE  hPalette;
            HMENU     hMenu = GetMenu (hWnd);

            if (! wPal)
                bIsFirstPaint = TRUE;

            bDisplayQuality = TRUE;
            bTrueColor      = FALSE;

            /*  Setup palette usage  */

            CheckMenuItem (hMenu, IDM_VIEWNORMAL, MF_UNCHECKED);
            CheckMenuItem (hMenu, IDM_VIEWQUALITY, MF_CHECKED);
            CheckMenuItem (hMenu, IDM_VIEWTRUECOLOR, MF_UNCHECKED);

//          SetClassLong (hWnd, GCW_HBRBACKGROUND, GetStockObject (LTGRAY_BRUSH));

            SetClassLong (hWnd, GCW_HBRBACKGROUND, GetStockObject (WHITE_BRUSH));
            if (image_active)
              SetClassLong (hWndDisplay, GCW_HBRBACKGROUND, GetStockObject (WHITE_BRUSH));


            hDC = GetDC (hWnd);
            SetSystemPaletteUse (hDC, SYSPAL_NOSTATIC);
            if (hDispPalette)
            {
                UnrealizeObject (hDispPalette);
                hPalette = SelectPalette (hDC, hDispPalette, 0);
                RealizePalette (hDC);
                hDispPalette = SelectPalette (hDC, hPalette, 0);
            }
            SetSysColors (NUMSYSCOLORS, (LPINT)&SysColorIndices, (LPDWORD)&SysColorValues);
            ReleaseDC (hWnd, hDC);
            SendMessage (-1, WM_SYSCOLORCHANGE, (WORD)NULL, (LONG)NULL);
          }
          break;

          case IDM_VIEWTRUECOLOR:
          {
            HMENU     hMenu = GetMenu (hWnd);

            CheckMenuItem (hMenu, IDM_VIEWNORMAL, MF_UNCHECKED);
            CheckMenuItem (hMenu, IDM_VIEWQUALITY, MF_UNCHECKED);
            CheckMenuItem (hMenu, IDM_VIEWTRUECOLOR, MF_CHECKED);
            bTrueColor = TRUE;

            if (image_active)
                if (IsWindow (hWndDisplay))  // Safety test..
                    InvalidateRect (hWndDisplay, (LPRECT) NULL, FALSE);

          }
          break;

          case IDM_FULLSCREEN:

              PostMessage (hWndDisplay, WM_RBUTTONDOWN, NULL, NULL);
              break;

          case IDM_RETAIN_TOGGLE:
          {
            WORD   wChecked;
            HMENU  hMenu = GetMenu (hWnd);

            bResetProc = (BYTE) ! bResetProc;
            wChecked   = (bResetProc ? MF_UNCHECKED : MF_CHECKED);
            CheckMenuItem (hMenu, IDM_RETAIN_TOGGLE, wChecked);
          }
          break;

          case IDM_RELAXED:
          {
            HMENU  hMenu = GetMenu (hWnd);

            wDithQuality = 8;
            if (image_active && wDither >= IFS)
            {
              bIsFirstPaint = TRUE;
              InvalidateRect (hWndDisplay, (LPRECT)NULL, FALSE);
            }

          }
          break;

          case IDM_STRICT:
          {
            HMENU  hMenu = GetMenu (hWnd);

            wDithQuality = 1;
            if (image_active && wDither >= IFS)
            {
              bIsFirstPaint = TRUE;
              InvalidateRect (hWndDisplay, (LPRECT)NULL, FALSE);
            }
          }
          break;

          case IDM_INFO:
          lpfnDlg  = MakeProcInstance ((FARPROC)InfoDlgProc, hInstIP);
          bSuccess = DialogBox (hInstIP, (LPSTR)"INFO", hWndIP, lpfnDlg);
          FreeProcInstance (lpfnDlg);
          break;

          case IDM_GAMMA:
          {    
            LPGAMMATABLE lpGammaTable;

            lpGammaTable = (LPGAMMATABLE) GlobalLock (hGlobalGamma);

            if (AdjustGamma (hWnd, lpGammaTable))
            {
                UpdateCorrectionTable (hGlobalCTable);
                if (image_active && IsWindow (hWndDisplay))  // Safety test..
                {
                    bIsFirstPaint = TRUE;
                    InvalidateRect (hWndDisplay, (LPRECT)NULL, FALSE);
                }
            }
            GlobalUnlock (hGlobalGamma);
          }
          break;

          case IDM_RGB:
          {    
            LPCOLORTABLE lpColorTable;

            lpColorTable = (LPCOLORTABLE) GlobalLock (hGlobalColor);

            if (AdjustRGB (hWnd, lpColorTable))
            {
                UpdateCorrectionTable (hGlobalCTable);
                if (image_active && IsWindow (hWndDisplay))  // Safety test..
                {
                    bIsFirstPaint = TRUE;
                    InvalidateRect (hWndDisplay, (LPRECT)NULL, FALSE);
                }
            }
            GlobalUnlock (hGlobalColor);
          }
          break;

          case IDM_BALANCE:{    
            LPCOLORTABLE lpColorTable;

            lpColorTable = (LPCOLORTABLE) GlobalLock (hGlobalColor);
            if (AdjustBalance (hWnd, lpColorTable)){
              UpdateCorrectionTable (hGlobalCTable);
              if (image_active && IsWindow (hWndDisplay)){
                bIsFirstPaint = TRUE;
                InvalidateRect (hWndDisplay, (LPRECT)NULL, FALSE);
              }
            }
            GlobalUnlock (hGlobalColor);
          }
          break;

          case IDM_BC:
          {    
            LPCOLORTABLE lpColorTable;

            lpColorTable = (LPCOLORTABLE) GlobalLock (hGlobalColor);

            if (AdjustBC (hWnd, lpColorTable))
            {
                UpdateCorrectionTable (hGlobalCTable);
                if (image_active && IsWindow (hWndDisplay))  // Safety test..
                {
                    bIsFirstPaint = TRUE;
                    InvalidateRect (hWndDisplay, (LPRECT)NULL, FALSE);
                }
            }

            GlobalUnlock (hGlobalColor);
          }
          break;

          case IDM_DEFAULT_COLOR:
          case IDM_8BIT_OPR:          //  Was: IDM_OPR_COLOR:
          case IDM_4BIT_OPR:  
          case IDM_GRAYSCALE:
          case IDM_MONOCHROME:
          {
            SelPalette (hWnd, wParam);

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

                bIsFirstPaint = TRUE;
                InvalidateRect (hWndDisplay, NULL, FALSE);
            } 
          }
          break;

          case IDM_NO_DITHER:
          case IDM_ORDERED:
          case IDM_FS:
          case IDM_BURKE:
          case IDM_FAST:
          {
            BOOL bRegenPal = (BOOL) image_active;

            if (image_active)
                if (wPal == IOPT && wParam > IDM_ORDERED && wDither > IBAY)
                    bRegenPal = FALSE;

            SelDither (hWnd, wParam);

            if (image_active)
            {
                if (bRegenPal)
                {
                    GLOBALFREE (hImpPalette);

                    /*  This put in to allow interruptable paints.  4/91  D.Ison  */
                    
                    FreeFilters (hFileInfo, IMPORT_CLASS);

                    SetImportStates ();
                    FreePrevDispPalMem (dPalType, old_pal);
                    if ((wDither == IFS) && (wPal == IOPT)) 
                        if (! bOptDither)
                            bNewOpt = TRUE;
                    bIsFirstPaint = TRUE;
                } 
                InvalidateRect (hWndDisplay, NULL, FALSE);
            }
          }
          break;

          case IDM_GRAYSCALE16:  //  A Special case
          {
            SelGray16 (hWnd, wParam);

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
                bIsFirstPaint = TRUE;
                InvalidateRect (hWndDisplay, NULL, FALSE);
            } 
          }
          break;

          case IDM_8BIT_COLOR:
          case IDM_4BIT_COLOR:   // Sets up VGA 16 stuff
          {
            if (wParam == IDM_8BIT_COLOR)
                if (wDither == IFS16)
                    SetState (DITH_STATE, IFS);

            if (wParam == IDM_4BIT_COLOR)
                SetState (DITH_STATE, IFS16);
            else
                if (wDither == IFS16)
                    SetState (DITH_STATE, IBAY);


                if (lParam == 1L)
                    break;

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
                    bIsFirstPaint = TRUE;
                    InvalidateRect (hWndDisplay, NULL, FALSE);
                } 

          }
          break;

          case IDM_SETUP_OPR:

            lpfnDlg = MakeProcInstance ((FARPROC)OPRDlgProc, hInstIP);
            bSuccess = DialogBox (hInstIP, (LPSTR)"OPRSETUP", hWndIP, lpfnDlg);
            if (bSuccess)
            {
                SendMessage (hWnd, WM_COMMAND, IDM_8BIT_OPR, NULL);
                if (image_active)
                    if (wPal == IOPT)
                    {
                        bIsFirstPaint = TRUE;
                        InvalidateRect (hWndDisplay, (LPRECT)NULL, FALSE);
                    }
            }
          break;


          #ifdef HELP
          case IDM_HELPINDEX:
            WinHelp (hWnd, (LPSTR) "helpex.hlp", HELP_INDEX, NULL);
          break;
          #endif

          case IDM_EXIT:

            PostMessage (hWnd, WM_CLOSE, NULL, NULL);
            break;

          case IDM_MERGE:
          {
            int nRetval;

            nRetval = DoMerge();

            if (nRetval < 0)
                ReportError (nRetval, NULL);
          }
          break;

          case IDM_SHARPEN:
          {
            int nRetval;

            nRetval = CPITools (hWndDisplay, SharpenCPI, 0);
            if (nRetval < 0)
              ReportError (nRetval, NULL);
          }
          break;

          case IDM_SMOOTH:
          {
            int nRetval;

            nRetval = CPITools (hWndDisplay, SmoothCPI, 0);
            if (nRetval < 0)
              ReportError (nRetval, NULL);
          }
          break;

          case IDM_REMOVE_NOISE:
          {
            int nRetval;

            nRetval = CPITools (hWndDisplay, SmoothCPI, 1);
            if (nRetval < 0)
              ReportError (nRetval, NULL);
          }
          break;

          case IDM_ENHANCE_EDGE:
          {
            int nRetval;

            nRetval = CPITools (hWndDisplay, EnhanceEdgeCPI, 0);
            if (nRetval < 0)
              ReportError (nRetval, NULL);
          }
          break;

          case IDM_LINE_DRAWING:
          {
            int nRetval;

            nRetval = CPITools (hWndDisplay, EnhanceEdgeCPI, 1);
            if (nRetval < 0)
              ReportError (nRetval, NULL);
          }
          break;

          case IDM_ROTATE:
          {
            int nRetval;

            nRetval = CPITools (hWndDisplay, RotateCPI, 0);
            if (nRetval < 0)
              ReportError (nRetval, NULL);
          }
          break;

          case IDM_ROTATECCW:
          {
            int nRetval;

            nRetval = CPITools (hWndDisplay, RotateCPI, 1);
            if (nRetval < 0)
              ReportError (nRetval, NULL);
          }
          break;
        
          case IDM_SCALE:
          {
            int nRetval;

            nRetval = CPITools (hWndDisplay, ScaleCPI, 0);
            if (nRetval < 0)
              ReportError (nRetval, NULL);
          }
          break;

          case IDM_FLIP:
          {
            int nRetval;

            nRetval = CPITools (hWndDisplay, FlipCPI, 0);
            if (nRetval < 0)
              ReportError (nRetval, NULL);
          }
          break;

          case IDM_MIRROR:
          {
            int nRetval;

            nRetval = CPITools (hWndDisplay, MirrorCPI, 0);
            if (nRetval < 0)
              ReportError (nRetval, NULL);
          }
          break;

          case IDM_INVERTCOLORS:
          {
            HMENU  hMenu = GetMenu (hWnd);

            bInvertColors = (BYTE) ! bInvertColors;
            UpdateCorrectionTable (hGlobalCTable);
            if (image_active && IsWindow (hWndDisplay))  // Safety test..
            {
                bIsFirstPaint = TRUE;
                InvalidateRect (hWndDisplay, (LPRECT)NULL, FALSE);
            }

          }
          break;


          case IDM_SET_DISP:
          lpfnDlg = MakeProcInstance ((FARPROC)ProcSpecDlgProc, hInstIP);
          bSuccess = DialogBox (hInstIP, (LPSTR)"PROCSPEC", hWndIP, lpfnDlg);
          FreeProcInstance (lpfnDlg);
          if (bSuccess && image_active)
            InvalidateRect (hWndDisplay, (LPSTR)NULL , FALSE);
          break;

          case IDM_ACTIVATE_TOGGLE:  
            bCaptureOn = (BYTE) ! bCaptureOn;
            SetCaptureOn (bCaptureOn);
          break;

          case IDM_CAP_CLIENT: 
          case IDM_CAP_WINDOW: 
          case IDM_CAP_SCREEN: 
          case IDM_CAP_AREA:
            CheckMenuItem (GetMenu (hWnd), wRegion + IDM_CAPAREAMASK, MF_UNCHECKED);
            SetCaptureType (wParam - IDM_CAPAREAMASK);
            wRegion = wParam - IDM_CAPAREAMASK;
          break;   

          case IDM_SETHOTKEY:   
            DoSetHot (hWnd);
          break;

          case IDM_CAPTIF: 
          case IDM_CAPTGA: 
          case IDM_CAPPCX:
          case IDM_CAPGIF:
          case IDM_CAPDIB:

          #ifdef DEMO
          MessageBox (hWnd, (LPSTR)"Can only capture CPI images", (LPSTR) szDemoVersion, MB_OK);
          break;
          #endif
          case IDM_CAPCPI:
          {
            WORD  wFileType;
            HMENU hMenu;
            int   i;

            /*   Set capture file type    */

            switch (wParam)
            {
              case IDM_CAPCPI:
                wFileType = IDFMT_CPI;
                break;
              case IDM_CAPTIF:
                wFileType = IDFMT_TIF;
                break;
              case IDM_CAPTGA:
                wFileType = IDFMT_TGA;
                break;
              case IDM_CAPPCX:
                wFileType = IDFMT_PCX;
                break;
              case IDM_CAPGIF:
                wFileType = IDFMT_GIF;
                break;
              case IDM_CAPDIB:
                wFileType = IDFMT_WNDIB;
                break;
              default:
                wFileType = IDFMT_CPI;
                break;
            }
            SetCaptureFileType (wFileType);
            SetState (EXPORT_TYPE, wParam - IDM_CAPMASK);
            hMenu = GetMenu (hWnd);
            for (i = IDM_CAPCPI; i <= IDM_CAPDIB; i++)
              CheckMenuItem (hMenu, i, MF_UNCHECKED);
            CheckMenuItem (hMenu, wParam, MF_CHECKED);
          }
          break;   

          case IDM_HIDE_WINDOW:
            CheckMenuItem (GetMenu (hWnd), IDM_HIDE_WINDOW, ToggleHide() ? MF_CHECKED : MF_UNCHECKED);
            break;

          case IDM_NAME_CAPFILE:
            CheckMenuItem (GetMenu (hWnd), IDM_NAME_CAPFILE, ToggleNameCapFile() ? MF_CHECKED : MF_UNCHECKED);
            break;

          case IDM_CAP_TO_CLIPBOARD:
          {
            BOOL bCapToClipboard;
            HMENU hMenu = GetMenu (hWnd);

            #ifdef DEMO
            MessageBox (hWnd, (LPSTR)"Can only capture CPI images", (LPSTR)szDemoVersion, MB_OK);
            break;
            #endif


            bCapToClipboard = ToggleCapToClipboard ();
            CheckMenuItem (GetMenu (hWnd), IDM_CAP_TO_CLIPBOARD, bCapToClipboard ? MF_CHECKED : MF_UNCHECKED);
            if (bCapToClipboard)    // Kludge!
            {
                EnableMenuItem (hMenu, IDM_CAPCPI, MF_GRAYED);
                EnableMenuItem (hMenu, IDM_CAPTIF, MF_GRAYED);
                EnableMenuItem (hMenu, IDM_CAPTGA, MF_GRAYED);
                EnableMenuItem (hMenu, IDM_CAPPCX, MF_GRAYED);
                EnableMenuItem (hMenu, IDM_CAPGIF, MF_GRAYED);
                EnableMenuItem (hMenu, IDM_CAPDIB, MF_GRAYED);
            }
            else
            {
                WORD wNumPlanes;
                WORD wBitsPerPixel;
                HDC  hDC;

                hDC = GetDC (NULL);
                wNumPlanes    = GetDeviceCaps (hDC, PLANES);
                wBitsPerPixel = GetDeviceCaps (hDC, BITSPIXEL);
                ReleaseDC (NULL, hDC);

                EnableMenuItem (hMenu, IDM_CAPCPI, MF_ENABLED);
                EnableMenuItem (hMenu, IDM_CAPTIF, MF_ENABLED);
                EnableMenuItem (hMenu, IDM_CAPTGA, MF_ENABLED);
                EnableMenuItem (hMenu, IDM_CAPDIB, MF_ENABLED);
                if ((wNumPlanes * wBitsPerPixel) <= 8)
                {
                  EnableMenuItem (hMenu, IDM_CAPPCX, MF_BYCOMMAND | MF_ENABLED);
                  EnableMenuItem (hMenu, IDM_CAPGIF, MF_BYCOMMAND | MF_ENABLED);
                }

            }
          }
          break;

          #ifdef DEMO
          case IDM_CUT:
          case IDM_COPY:

          MessageBox (hWnd, (LPSTR)"Can only paste from Clipboard", (LPSTR)szDemoVersion, MB_OK);
          break;
          #else  

          case IDM_CUT:
            if (CopyClipboard (hWnd, FALSE) == 0)
                PostMessage(hWndDisplay, WM_CLOSE, 0, 0L);
          break;

          case IDM_COPY:
            CopyClipboard (hWnd, TRUE);
          break;

          #endif

          case IDM_PASTE: 
          {
            ATOM    pasteAtom;
            int     dibImage;
            PSTR    fileName;
            HANDLE  hFileName;

            hFileName = LocalAlloc (LPTR, 256);

            fileName = (PSTR) LocalLock (hFileName);
            
            GetTempFileName (0, (LPSTR) "CLP", 0, (LPSTR) fileName);

            dibImage = PasteFromClipboard(hWnd, 256,
                (LPSTR) fileName);

            if (dibImage == 0)
            {
              pasteAtom = GlobalAddAtom((LPSTR) fileName);
              PostMessage(hWnd, WM_SHOWIMAGE, CLIPBOARD_OPEN,  MAKELONG(pasteAtom, 0));

            }
            LocalUnlock (hFileName);
            LocalFree(hFileName);
          }
          break;

          case IDM_PREFERENCES:
            lpfnDlg = MakeProcInstance ((FARPROC)PrefDlgProc, hInstIP);
            DialogBox (hInstIP, (LPSTR)"PREF", hWndIP, lpfnDlg);
            FreeProcInstance (lpfnDlg);
            break;

          case IDM_UNDO:
          {
            PSTR pStringBuf;
            BYTE bOldCurrTemp;

            /*  Here current becomes undo, undo becomes current and opened
                with attributes based on if undo was temp or not.  Confusing?  */

            pStringBuf = LocalLock (hStringBuf);
            _fstrcpy ((LPSTR) pStringBuf, (LPSTR) szUndoFileName);
            _fstrcpy ((LPSTR) szUndoFileName, (LPSTR) szCurrFileName);


            bOldCurrTemp = (BYTE) bIsCurrTemp;
            bIsCurrTemp  = (BYTE) bIsUndoTemp;
            bIsUndoTemp  = (BYTE) bOldCurrTemp;

            SendMessage (hWndDisplay, WM_CLOSE, 0, 0);
            PostMessage (hWnd, WM_SHOWIMAGE, UNDO_OPEN, (LONG) (LPSTR) pStringBuf);
            LocalUnlock (hStringBuf);
          }
          break;

          case IDM_PRINT:
            CopyPrint (hWnd, FALSE);
          break;

          case IDM_PRINTSETUP:
          {
            LPFILEINFO lpFileInfo;

            lpFileInfo = (LPFILEINFO) GlobalLock (hFileInfo);
            DisplayPageSetup (hWnd, lpFileInfo -> wScanWidth, lpFileInfo -> wScanHeight);
            GlobalUnlock (hFileInfo);
          }
          break;

          default:
            return (DefWindowProc (hWnd, Message, wParam, lParam));
      }     /*  For switch (wParam)  */

}

int NEAR CPITools (HWND hWnd, int (FAR PASCAL *lpfnTool) (HWND, LPSTR, LPSTR, int), int nFlags)
{
  int nRetval;
  BOOL bDefault = FALSE;
  PSTR pStringBuf;
  OFSTRUCT Of;

  pStringBuf  = LocalLock (hStringBuf);

  /*  New temp file will become current file if tool succeeds */

  GetTempFileName (0, (LPSTR) "CPI", 0, (LPSTR) pStringBuf);

  if (image_active)
    if (wImportType == IDFMT_CPI)
      bDefault = TRUE;

  LockData (0);       // We will be passing a far ptr to our Data Segment
//bToolsActive = (BYTE) TRUE;
  bPollMessages = (BYTE) FALSE;
  nRetval = (*lpfnTool) (hWnd, (LPSTR) pStringBuf, (LPSTR) szCurrFileName, nFlags);
//bToolsActive = (BYTE) FALSE;
  bPollMessages = (BYTE) TRUE;
  UnlockData (0);

  if (nRetval)   // Tool processed successfully, update filenames, delete files, etc. */
  {
    if (szUndoFileName [0] && bIsUndoTemp)
    {
      if (OpenFile ((LPSTR)szUndoFileName, (LPOFSTRUCT)&Of, OF_EXIST) != -1)
        OpenFile ((LPSTR)szUndoFileName, (LPOFSTRUCT)&Of, OF_DELETE);
      szUndoFileName [0] = 0; 
    }

    /*  Then transfer current file to undo file  */

    _fstrcpy ((LPSTR) szUndoFileName, (LPSTR) szCurrFileName);
    bIsUndoTemp = (BYTE) bIsCurrTemp;
    bImageModified = TRUE;
  }
  LocalUnlock (hStringBuf);
  return (nRetval);      

}





void FAR ConformMenu (wImportClass)
WORD wImportClass;
{
    HMENU hMenu;
    HMENU hProcessMenu;
    HMENU hPalMenu;

    /*  First turn em all back on  */

    hMenu = GetMenu (hWndIP);

    hProcessMenu = GetSubMenu (hMenu, PROCESS_MENU);
    hPalMenu     = GetSubMenu (hProcessMenu, PALETTE_MENU);
    EnableMenuItem (hPalMenu, OPR_MENU, MF_BYPOSITION | MF_ENABLED);


    EnableMenuItem (hMenu, IDM_DEFAULT_COLOR, MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem (hMenu, IDM_GRAYSCALE, MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem (hMenu, IDM_GRAYSCALE16, MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem (hMenu, IDM_MONOCHROME, MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem (hMenu, IDM_NO_DITHER, MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem (hMenu, IDM_ORDERED, MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem (hMenu, IDM_FS, MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem (hMenu, IDM_BURKE, MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem (hMenu, IDM_FAST, MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem (hMenu, IDM_8BIT_COLOR, MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem (hMenu, IDM_4BIT_COLOR, MF_BYCOMMAND | MF_ENABLED);

    /*  Then uncheck everything  */

    CheckMenuItem (hMenu, IDM_DEFAULT_COLOR, MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_8BIT_OPR, MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_4BIT_OPR, MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_GRAYSCALE, MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_GRAYSCALE16, MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_MONOCHROME, MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_NO_DITHER, MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_ORDERED, MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_FS, MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_BURKE, MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_FAST, MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_8BIT_COLOR, MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_4BIT_COLOR, MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_SETUP_OPR, MF_UNCHECKED);

    switch (wImportClass)
    {
        case IRGB:
            RGBClassMenu (hWndIP);
            break;

        case ICM:
            CMAPClassMenu (hWndIP);
            break;

        case IGR:
            GRAYClassMenu (hWndIP);
            break;

        case IMON:
            MONOClassMenu (hWndIP);
            break;
    }
}

void FAR RGBClassMenu (hWnd)
HWND hWnd;
{
    HMENU hMenu;
    WORD  wDithVal;

    hMenu = GetMenu (hWnd);

    /*  Now Gray out everything that doesn't apply to current 
        import class / selection  */
    
    switch (wPal)
    {

    /*  Then gray out stuff based on this  */

        case IOPT:
            EnableMenuItem (hMenu, IDM_ORDERED, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_8BIT_COLOR, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_4BIT_COLOR, MF_BYCOMMAND | MF_GRAYED);
        break;

        case IGR:
            EnableMenuItem (hMenu, IDM_ORDERED, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_FS, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_BURKE, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_FAST, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_8BIT_COLOR, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_4BIT_COLOR, MF_BYCOMMAND | MF_GRAYED);
        break;

        case IBW:
            EnableMenuItem (hMenu, IDM_BURKE, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_FAST, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_8BIT_COLOR, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_4BIT_COLOR, MF_BYCOMMAND | MF_GRAYED);
        break;
    }

    /*  Now have available menu selections set up; check best selections we 
        can based on old selections  */

    
            

    if (wDither == INO16)   // Special case (4 bit gray)
        CheckMenuItem (hMenu, IDM_GRAYSCALE16, MF_CHECKED);
    else
        if (wPal == IOPT)       // Special case for OPR popup menu
        {
            if (wDither == IFS16)
                CheckMenuItem (hMenu, IDM_4BIT_OPR, MF_CHECKED);
            else
                if (wOPRColors != 256 && wOPRColors != 16)
                    CheckMenuItem (hMenu, IDM_SETUP_OPR, MF_CHECKED);
                else
                    CheckMenuItem (hMenu, IDM_8BIT_OPR, MF_CHECKED);
        }
        else
            CheckMenuItem (hMenu, (IDM_PROCESSES + wPal), MF_CHECKED);

    wDithVal = IDM_DITHERS + wDither;

    if (wDither > IFS)      // Definitely a 16 color dither
       wDithVal = wDithVal + wCurrDither - 3;  // Correct offset...
    else
        if (wDither == IFS) // Definitely err-distr variety
            wDithVal += wCurrDither - 1;

    if (wDither == INO16)   // Override if gray 16
        wDithVal = IDM_DITHERS + INONE;

    CheckMenuItem (hMenu, wDithVal, MF_CHECKED);

    if (wDither == IFS16)
    {
        CheckMenuItem  (hMenu, IDM_4BIT_COLOR, MF_CHECKED);
        EnableMenuItem (hMenu, IDM_GRAYSCALE, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem (hMenu, IDM_GRAYSCALE16, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem (hMenu, IDM_GRAYSCALE, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem (hMenu, IDM_MONOCHROME, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem (hMenu, IDM_NO_DITHER, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem (hMenu, IDM_ORDERED, MF_BYCOMMAND | MF_GRAYED);
    }
    else
        CheckMenuItem (hMenu, IDM_8BIT_COLOR, MF_CHECKED);


    if (wDither <= IBAY)
    {
        EnableMenuItem (hMenu, IDM_8BIT_COLOR, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem (hMenu, IDM_4BIT_COLOR, MF_BYCOMMAND | MF_GRAYED);
    }


}

void FAR CMAPClassMenu (hWnd)
HWND hWnd;
{
    HMENU hMenu;
    HMENU hProcessMenu;
    HMENU hPalMenu;
    WORD  wDithVal;

    hMenu = GetMenu (hWnd);

    /*  Now Gray out everything that doesn't apply to current 
        import class / selection  */
    
    hProcessMenu = GetSubMenu (hMenu, PROCESS_MENU);
    hPalMenu     = GetSubMenu (hProcessMenu, PALETTE_MENU);
    EnableMenuItem (hPalMenu, OPR_MENU, MF_BYPOSITION | MF_GRAYED);

    EnableMenuItem (hMenu, IDM_BURKE, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem (hMenu, IDM_FAST, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem (hMenu, IDM_4BIT_COLOR, MF_BYCOMMAND | MF_GRAYED);


    switch (wPal)
    {

    /*  Then gray out stuff based on this  */

        case INORM:
            EnableMenuItem (hMenu, IDM_FS, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_ORDERED, MF_BYCOMMAND | MF_GRAYED);
            break;

        case IGR:
            EnableMenuItem (hMenu, IDM_ORDERED, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_FS, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_BURKE, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_FAST, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_8BIT_COLOR, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_4BIT_COLOR, MF_BYCOMMAND | MF_GRAYED);
        break;

        case IBW:
            EnableMenuItem (hMenu, IDM_BURKE, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_FAST, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_8BIT_COLOR, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_4BIT_COLOR, MF_BYCOMMAND | MF_GRAYED);
        break;
    }

    if (wDither == INO16)   // Special case (4 bit gray)
        CheckMenuItem (hMenu, IDM_GRAYSCALE16, MF_CHECKED);
    else
        CheckMenuItem (hMenu, (IDM_PROCESSES + wPal), MF_CHECKED);



    wDithVal = IDM_DITHERS + wDither;

    if (wDither > IFS)      // Definitely a 16 color dither
       wDithVal = wDithVal + wCurrDither - 3;  // Correct offset...
    else
        if (wDither == IFS) // Definitely err-distr variety
            wDithVal += wCurrDither - 1;

    if (wDither == INO16)   // Override if gray 16
        wDithVal = IDM_DITHERS + INONE;

    CheckMenuItem (hMenu, wDithVal, MF_CHECKED);

    EnableMenuItem (hMenu, IDM_8BIT_COLOR, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem (hMenu, IDM_4BIT_COLOR, MF_BYCOMMAND | MF_GRAYED);

}

void FAR GRAYClassMenu (hWnd)
HWND hWnd;
{
    HMENU hMenu;
    HMENU hProcessMenu;
    HMENU hPalMenu;
    WORD  wDithVal;

    hMenu = GetMenu (hWnd);

    /*  Now Gray out everything that doesn't apply to current 
        import class / selection  */
    
    EnableMenuItem (hMenu, IDM_DEFAULT_COLOR, MF_BYCOMMAND | MF_GRAYED);
    hProcessMenu = GetSubMenu (hMenu, PROCESS_MENU);
    hPalMenu     = GetSubMenu (hProcessMenu, PALETTE_MENU);
    EnableMenuItem (hPalMenu, OPR_MENU, MF_BYPOSITION | MF_GRAYED);
    EnableMenuItem (hMenu, IDM_BURKE, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem (hMenu, IDM_FAST, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem (hMenu, IDM_4BIT_COLOR, MF_BYCOMMAND | MF_GRAYED);

    switch (wPal)
    {

    /*  Then gray out stuff based on this  */

        case IGR:
            EnableMenuItem (hMenu, IDM_ORDERED, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_FS, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_BURKE, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_FAST, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_8BIT_COLOR, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_4BIT_COLOR, MF_BYCOMMAND | MF_GRAYED);
        break;

        case IBW:
            EnableMenuItem (hMenu, IDM_BURKE, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_FAST, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_8BIT_COLOR, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hMenu, IDM_4BIT_COLOR, MF_BYCOMMAND | MF_GRAYED);
        break;
    }

    if (wDither == INO16)   // Special case (4 bit gray)
        CheckMenuItem (hMenu, IDM_GRAYSCALE16, MF_CHECKED);
    else
        CheckMenuItem (hMenu, (IDM_PROCESSES + wPal), MF_CHECKED);


    wDithVal = IDM_DITHERS + wDither;

    if (wDither > IFS)      // Definitely a 16 color dither
       wDithVal = wDithVal + wCurrDither - 3;  // Correct offset...
    else
        if (wDither == IFS) // Definitely err-distr variety
            wDithVal += wCurrDither - 1;

    if (wDither == INO16)   // Override if gray 16
        wDithVal = IDM_DITHERS + INONE;

    CheckMenuItem (hMenu, wDithVal, MF_CHECKED);



}

void FAR MONOClassMenu (hWnd)
HWND hWnd;
{
    HMENU hMenu;
    HMENU hProcessMenu;
    HMENU hPalMenu;

    hMenu = GetMenu (hWnd);

    /*  Now Gray out everything that doesn't apply to current 
        import class / selection  */
    
    EnableMenuItem (hMenu, IDM_DEFAULT_COLOR, MF_BYCOMMAND | MF_GRAYED);

    hProcessMenu = GetSubMenu (hMenu, PROCESS_MENU);
    hPalMenu     = GetSubMenu (hProcessMenu, PALETTE_MENU);
    EnableMenuItem (hPalMenu, OPR_MENU, MF_BYPOSITION | MF_GRAYED);

    EnableMenuItem (hMenu, IDM_BURKE, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem (hMenu, IDM_FAST, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem (hMenu, IDM_4BIT_COLOR, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem (hMenu, IDM_8BIT_COLOR, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem (hMenu, IDM_GRAYSCALE, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem (hMenu, IDM_GRAYSCALE16, MF_BYCOMMAND | MF_GRAYED);

    EnableMenuItem (hMenu, IDM_ORDERED, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem (hMenu, IDM_FS, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem (hMenu, IDM_BURKE, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem (hMenu, IDM_FAST, MF_BYCOMMAND | MF_GRAYED);

    CheckMenuItem (hMenu, (IDM_PROCESSES + wPal), MF_CHECKED);
    CheckMenuItem (hMenu, IDM_DITHERS + wDither, MF_CHECKED);
}


int  FAR SelPalette (HWND hWnd, WORD wParam)
{
    int nDither;

    /*  Now this emulates OK on setup processing dialog  */

    /*  If already in FS 16 mode, and switching to FS    */

    if (wParam == IDM_8BIT_OPR)
        if (wDither == IFS16)
            SetState (DITH_STATE, IFS);

    /*  Handle 4BIT_OPR as special case  */

    if (wParam == IDM_4BIT_OPR)
    {
        SetState (PAL_STATE, IOPT);
        SendMessage (hWnd, WM_COMMAND, IDM_4BIT_COLOR, 1L);
        return (0);
    }

    if (wParam == IDM_4BIT_COLOR)
    {
        SetState (PAL_STATE, INORM);
        SendMessage (hWnd, WM_COMMAND, IDM_4BIT_COLOR, 1L);
        return (0);
    }
    else
        if (wParam == IDM_8BIT_COLOR)
            wParam = IDM_DEFAULT_COLOR;
    
    
    SetState (PAINT_STATE, TRUE);

    SetState (PAL_STATE, (wParam - IDM_PROCESSES));

    if (wParam == IDM_GRAYSCALE)        // To grayscale, set dither to none
        nDither = INONE;
    else
        nDither = DDSM [0][wImportClass][wPal][wDither][SETDITH];
                    
    /*  Possible override if import class is true color AND default palette */

    if (wImportClass == IRGB)
        if (wPal == INORM)
            nDither = (int) bDefDither;

    SetState (DITH_STATE, nDither);

    return (0);

}


int FAR SelDither (HWND hWnd, WORD wParam)
{
    int nDither;

    nDither = (int) (wParam - IDM_DITHERS);

    if (nDither > IFS)   
    {
        /*  User is asking for a Burkes or Linear Dither  
            So we fake it here  */
        int nExtDither = nDither - IFS;
        nDither = IFS;
        wCurrDither = (WORD) nExtDither + 1;
    }
    else
        wCurrDither = DITHER_FS_COLOR;

    if (wDither != IFS16)  // Don't override the 4 bit color setting
        SetState (DITH_STATE, nDither);
    return (0);
}


int FAR SelGray16 (HWND hWnd, WORD wParam)
{
    SetState (PAL_STATE, IGRA);
    SetState (DITH_STATE, INO16);
    return (0);
}
