#include <windows.h>
#include <string.h>
#include "imgprep.h"
#include "resource.h"
#include "strtable.h"
#include "proto.h"
#include "global.h"
#include "capture.h"
#include "error.h"
#include "gamma.h"
#include "color.h"
#include "helpids.h"
#include "prometer.h"
#include "balance.h"
#include "startup.h"
#include "direct.h"
#include "menu.h"
#include "tools.h"
#include "clipimg.h"
#include "print.h"


#define MAXUPDATECOUNT  3
int  nUpdateCount = 0;

#ifdef COLORLAB
#include "scanlib.h"
#endif


/*---------------------------------------------------------------------------

   PROCEDURE:        WinMain()
   DESCRIPTION:      Main entry procedure for ImgPrep
   DEFINITION:       1.0
                            
   MODS:					
                     9-17-90  No more string constants.  D. Ison

-----------------------------------------------------------------------------*/
   
int FAR PASCAL WinMain (hInstance, hPrevInstance, lpszCmdLine, nCmdShow)
HANDLE  hInstance;
HANDLE  hPrevInstance;
LPSTR   lpszCmdLine;
int     nCmdShow;
{
  MSG msg;
  LPSTR lpTmpPtr;

//if (! (GetKeyState (VK_CONTROL) & 0x80))
  if (! CheckCopyProtect (hInstance))
    return (FALSE);


  /*  Look at command line to see if we need to alter our behavior here.. */

  {
    LPSTR   lpPtr;
    enum    {MIN, MAX, NORMAL} MinMax = NORMAL;
    lpPtr = lpszCmdLine;
    lpTmpPtr = lpPtr;

    while (*lpPtr)
    {
      if (*lpPtr == '/' || *lpPtr == '-')
      {
        lpPtr++;
        if (_fmemicmp (lpPtr, (LPSTR) "max", 3) == 0)
        {
          MinMax   = MAX;
          lpTmpPtr = lpPtr + 3;
        }
        else
          if (_fmemicmp (lpPtr, (LPSTR) "min", 3) == 0)
          {
            MinMax = MIN;       
            lpTmpPtr = lpPtr + 3;
          }
          else
            if (_fmemicmp (lpPtr, (LPSTR) "nologo",6) == 0)
            {
              bLogo = TRUE;  // Treat as if already been displayed
              lpTmpPtr = lpPtr + 6;
            }
      }
      lpPtr++;
    }

    if (MinMax == MAX)  //  User wants initial display maximized
      nCmdShow = SW_SHOWMAXIMIZED;
    else
      if (MinMax == MIN)  //  User wants initial display maximized
        nCmdShow = SW_SHOWMINIMIZED;
  }

  #ifdef DVAFIX
  /*  Grab the current directory.  (To pacify DVA FFT library only)  */
  {
    char Buffer [MAXPATHSIZE];

    getcwd (Buffer, MAXPATHSIZE);
    _fstrcpy ((LPSTR) szAppPath, (LPSTR) Buffer);

  }
  #endif


  #ifdef COLORLAB
  if (lstrlen (szAppName) == 0)
    lstrcpy (szAppName, "ColorLab");
  #else
  #ifdef DECIPHER
    if (lstrlen (szAppName) == 0)
        lstrcpy (szAppName, "Decipher");
  #else
    if (lstrlen (szAppName) == 0 )
        lstrcpy (szAppName, "ImagePrep");
  #endif
  #endif

  hInstIP = hInstance;
  if (!hPrevInstance)
  {
    if (!ImagePrepInit (hInstance, hPrevInstance, lpszCmdLine, nCmdShow))
      return (FALSE);
  }
  hWndIP = CreateWindow (
    (LPSTR)szAppName, 
    (LPSTR)szAppName, 
    WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,  
    0,
    0,
    CW_USEDEFAULT,                                /*  Default        */
    CW_USEDEFAULT,                                /*  Default        */
    NULL,                                         /*  No parent      */
    NULL,                                         /*  Use class menu */
    hInstance, 
    lpszCmdLine);      

  if (! hWndIP) 
    return (FALSE);


  /*  Customize menu with stuff that won't change anytime in the program  */
  /*  (Add program mgr and ctrl panel)    */
  {
    char szStringBuf [256];
    HMENU hMenu;

    hMenu = GetSystemMenu (hWndIP, NULL);
    AppendMenu (hMenu, MF_SEPARATOR, NULL, NULL);
    LoadString (hInstIP, STR_MENU_FILEMGR, (LPSTR) szStringBuf, sizeof (szStringBuf) - 1);
    AppendMenu (hMenu, MF_ENABLED, IDM_FILEMGR, (LPSTR)szStringBuf);
    LoadString (hInstIP, STR_MENU_CTRLPANEL, (LPSTR) szStringBuf, sizeof (szStringBuf) - 1);
    AppendMenu (hMenu, MF_ENABLED, IDM_CTRLPANEL,(LPSTR)szStringBuf);
  }

  ShowWindow (hWndIP, nCmdShow);

  /*  Send command-line passed filenames directly as if scanned  */

  {
    OFSTRUCT Of;

    while (*lpTmpPtr == ' ' && *lpTmpPtr != 0)
      lpTmpPtr++;

    if (OpenFile (lpTmpPtr, (LPOFSTRUCT)&Of, OF_EXIST) != -1) // Does this thing exist
      PostMessage (hWndIP, WM_SHOWIMAGE, COMMAND_LINE_OPEN, (DWORD) lpTmpPtr); 
  }

//MakeHelpPathName(szHelpFileName);   (Future)
  hHelpCursor = LoadCursor (hInstance,"HelpCursor");
    
  hAccelerate = LoadAccelerators (hInstIP, "ACCEL");

  while (GetMessage (&msg, NULL, 0, 0))
  {
    if (TranslateAccelerator (hWndIP, hAccelerate, (LPMSG) &msg) == 0)
    {
      TranslateMessage (&msg);
      DispatchMessage (&msg);
    }
  }

  #ifdef COLORLAB
  FlushEugene ();
  #endif
  FlushTools();


  return (msg.wParam);
}

/*-----------------------------------------------------------------------------

   PROCEDURE:        IPWndProc
   DESCRIPTION:      Initialization procedure for ImgPrep
   DEFINITION:       2.0
   START:            D. Ison & T. Bagman
                     
   MODS:             TBjr 4/17/90  stubbed for expansion as testable

------------------------------------------------------------------------------*/

long FAR PASCAL IPWndProc (hWnd, Message, wParam, lParam)
HWND        hWnd;
unsigned    Message;
WORD        wParam;
LONG        lParam;
{
  #ifdef HELP
  DWORD     dwHelpContextId;                 // Context-sensitve help placeholder
  #endif

  switch (Message)
  {
    case WM_CREATE:
    {
      LPGAMMATABLE lpGammaTable;
      LPCOLORTABLE lpColorTable;

      if (! (hGlobalBufs = GlobalAlloc (GMEM_MOVEABLE, GLOBALBUFSIZE * 2L)))
      {
        ReportError (EC_NOMEM, NULL);
        DestroyWindow (hWnd);
      }

      /*  Setup temp file path to be used  (Do this ONCE only!)  */

      GetTempFileName (0, (LPSTR) "HST", 0, (LPSTR)HistFile);

      InitSettings (hWnd);
    
            /*  Allocate a Global Gamma Table  */

      hGlobalGamma = GlobalAlloc (GHND, (DWORD) sizeof (GAMMATABLE) + 256 * sizeof (RGBQUAD));
      if (! hGlobalGamma)
      {
        ReportError (EC_NOMEM, NULL);
        DestroyWindow (hWnd);
      }
    
      lpGammaTable = (LPGAMMATABLE) GlobalLock (hGlobalGamma);

      CreateGammaTable (100, 0, 0, lpGammaTable);
      GlobalUnlock (hGlobalGamma);
    
      /*  Allocate a Color Table (for RGB and Brightness/Contrast */
    
      hGlobalColor = GlobalAlloc (GHND, (DWORD) sizeof (COLORTABLE) + 256 * sizeof (RGBQUAD));
      if (! hGlobalColor)
      {
        ReportError (EC_NOMEM, NULL);
        DestroyWindow (hWnd);
      }
    
      lpColorTable = (LPCOLORTABLE) GlobalLock (hGlobalColor);
    
      CreateColorTable (0, 255, 0, lpColorTable);
    
      GlobalUnlock (hGlobalColor);

      /*  Allocate a Color Translation (to combine other color correctors) */
    
      hGlobalCTable = GlobalAlloc (GHND, (DWORD)  256 * sizeof (RGBQUAD));
      if (! hGlobalColor)
      {
        ReportError (EC_NOMEM, NULL);
        DestroyWindow (hWnd);
      }
    
      UpdateCorrectionTable (hGlobalCTable);
    
      /*  Register with capture                */
 
      if (! (hInstCapture = RegisterCapApp (FALSE, hWnd, wDevice, hInstIP)))
      {
        ReportError (EC_ERROR, NULL);
        DestroyWindow (hWnd);
      }
      
      hStringBuf = LocalAlloc (LHND, (MAXSTRINGSIZE << 1));
      if (! hStringBuf)
        return (1);
    
        if (! bLogo)          // (FALSE means cmd line de-activated logo)
          PostMessage (hWnd, WM_COMMAND, IDM_LOGO, 0L); 
        else
          bLogo = FALSE;      // Pretend as if logo displayed & then cleared.  FALSE will prevent InvalidateRect()
    
      szUndoFileName [0] = 0; // No undo file

      getcwd (szOpenDir, sizeof (szOpenDir));
      _fstrcpy ((LPSTR) szSaveDir, (LPSTR) szOpenDir);

    }
    break;

    case WM_INITMENU:
    {
      HMENU hMenu;
      HMENU hSysMenu;
      HMENU hEditMenu;
      HMENU hRotateMenu;
      HMENU hProcessMenu;
      HMENU hFiltersMenu;
      char  szStringBuf [256];
      WORD  wStringID;

      hMenu = GetMenu (hWnd);
      hSysMenu = GetSystemMenu (hWnd, FALSE);

      if (bIsPainting)
      {
        EnableMenuItem (hMenu, IDM_EXIT,  MF_GRAYED);
        EnableMenuItem (hMenu, IDM_CAP_AREA,  MF_GRAYED);
        EnableMenuItem (hMenu, IDM_PASTE, MF_GRAYED);
        EnableMenuItem (hMenu, IDM_MERGE, MF_GRAYED);
        EnableMenuItem (hSysMenu, SC_CLOSE, MF_GRAYED);
      }
      else
      {
        EnableMenuItem (hMenu, IDM_EXIT,  MF_ENABLED);
        EnableMenuItem (hMenu, IDM_CAP_AREA,  MF_ENABLED);
        if (IsClipboardFormatAvailable (CF_DIB) || IsClipboardFormatAvailable (CF_BITMAP) || IsClipboardFormatAvailable (CF_METAFILEPICT))
          EnableMenuItem (hMenu, IDM_PASTE, MF_ENABLED);
        else
          EnableMenuItem (hMenu, IDM_PASTE, MF_GRAYED);
        EnableMenuItem (hMenu, IDM_MERGE, MF_ENABLED);
        EnableMenuItem (hSysMenu, SC_CLOSE, MF_ENABLED);
      }

      if (image_active && ! bIsPainting)
      {
        EnableMenuItem (hMenu, IDM_SAVE,  MF_ENABLED);
        EnableMenuItem (hMenu, IDM_CUT,   MF_ENABLED);
        EnableMenuItem (hMenu, IDM_COPY,  MF_ENABLED);
        EnableMenuItem (hMenu, IDM_PRINT, MF_ENABLED);
      }
      else
      {
        EnableMenuItem (hMenu, IDM_SAVE , MF_GRAYED);
        EnableMenuItem (hMenu, IDM_CUT,   MF_GRAYED);
        EnableMenuItem (hMenu, IDM_COPY,  MF_GRAYED);
        EnableMenuItem (hMenu, IDM_PRINT, MF_GRAYED);
      }

      if (image_active)
      {
        #ifdef COLORLAB
        EnableMenuItem (hMenu, IDM_SCAN, MF_GRAYED);
        #endif    
        EnableMenuItem (hMenu, IDM_INFO, MF_ENABLED);
        if (_fstrlen ((LPSTR) szUndoFileName) > 0)
          EnableMenuItem (hMenu, IDM_UNDO, MF_ENABLED);
        EnableMenuItem (hMenu, IDM_CLOSE, MF_ENABLED);
        EnableMenuItem (hMenu, IDM_FULLSCREEN, MF_ENABLED);
        EnableMenuItem (hMenu, IDM_PRINTSETUP, MF_ENABLED);
      }
      else
      {
        #ifdef COLORLAB
        EnableMenuItem (hMenu, IDM_SCAN, MF_ENABLED);
        #endif    
        EnableMenuItem (hMenu, IDM_INFO, MF_GRAYED);
        EnableMenuItem (hMenu, IDM_UNDO, MF_GRAYED);
        EnableMenuItem (hMenu, IDM_CLOSE, MF_GRAYED);
        EnableMenuItem (hMenu, IDM_FULLSCREEN, MF_GRAYED);
        EnableMenuItem (hMenu, IDM_PRINTSETUP, MF_GRAYED);
      }

      if (bCaptureOn)
        wStringID = STR_MENU_CAPTURE_ON;
      else
        wStringID = STR_MENU_CAPTURE_OFF;

      LoadString (hInstIP, wStringID, (LPSTR) szStringBuf, sizeof (szStringBuf) - 1);
    
      ChangeMenu (hMenu, IDM_ACTIVATE_TOGGLE, (LPSTR)szStringBuf, IDM_ACTIVATE_TOGGLE, MF_BYCOMMAND | MF_CHANGE | MF_ENABLED);
      CheckMenuItem (hMenu, IDM_ACTIVATE_TOGGLE, bCaptureOn ? MF_CHECKED : MF_UNCHECKED);
      CheckMenuItem (hMenu, wRegion + IDM_CAPAREAMASK, MF_CHECKED);

      if (bLogo)
      {
        /*  Erase logo  */

        InvalidateRect (hWnd, (LPRECT)NULL, TRUE);
        bLogo = FALSE;
      }

#define EDIT_MENU     1
#define ROTATE_MENU   8
#define PROCESS_MENU  3
#define FILTERS_MENU  5
            
      /*  Tools menu setup  */

      if (image_active && wImportType == IDFMT_CPI)
      {
        EnableMenuItem (hMenu, IDM_SHARPEN, MF_ENABLED);
        EnableMenuItem (hMenu, IDM_SMOOTH, MF_ENABLED);
        EnableMenuItem (hMenu, IDM_REMOVE_NOISE, MF_ENABLED);
        EnableMenuItem (hMenu, IDM_ENHANCE_EDGE, MF_ENABLED);
        EnableMenuItem (hMenu, IDM_SCALE  , MF_ENABLED);
        EnableMenuItem (hMenu, IDM_FLIP   , MF_ENABLED);
        EnableMenuItem (hMenu, IDM_MIRROR , MF_ENABLED);

        /*  Enable rotate and filters popups  */

        hProcessMenu = GetSubMenu (hMenu, PROCESS_MENU);
        hFiltersMenu = GetSubMenu (hProcessMenu, FILTERS_MENU);

        hEditMenu    = GetSubMenu (hMenu, EDIT_MENU);
        hRotateMenu  = GetSubMenu (hEditMenu, ROTATE_MENU);

        EnableMenuItem (hEditMenu, ROTATE_MENU, MF_BYPOSITION | MF_ENABLED);
        EnableMenuItem (hProcessMenu, FILTERS_MENU, MF_BYPOSITION | MF_ENABLED);
      }
      else
      {

        EnableMenuItem (hMenu, IDM_SHARPEN, MF_GRAYED);
        EnableMenuItem (hMenu, IDM_SMOOTH , MF_GRAYED);
        EnableMenuItem (hMenu, IDM_REMOVE_NOISE, MF_GRAYED);
        EnableMenuItem (hMenu, IDM_ENHANCE_EDGE, MF_GRAYED);
        EnableMenuItem (hMenu, IDM_SCALE  , MF_GRAYED);
        EnableMenuItem (hMenu, IDM_FLIP   , MF_GRAYED);
        EnableMenuItem (hMenu, IDM_MIRROR , MF_GRAYED);

        /*  Gray out rotate and filters popups  */

        hProcessMenu = GetSubMenu (hMenu, PROCESS_MENU);
        hFiltersMenu = GetSubMenu (hProcessMenu, FILTERS_MENU);

        hEditMenu    = GetSubMenu (hMenu, EDIT_MENU);
        hRotateMenu  = GetSubMenu (hEditMenu, ROTATE_MENU);

        EnableMenuItem (hEditMenu, ROTATE_MENU, MF_BYPOSITION | MF_GRAYED);
        EnableMenuItem (hProcessMenu, FILTERS_MENU, MF_BYPOSITION | MF_GRAYED);

      }

      #ifdef HELP
        if (bHelp) 
          SetCursor (hHelpCursor);
      #endif



      /*  Finally setup processing menu for current image / selections  */

      ConformMenu (wImportClass);

            #ifdef NEWMENU
            {
                HMENU hFileMenu = 0;
                HMENU hPopMenu;

                hFileMenu = GetSubMenu (hMenu, 0);
            //  hPopMenu  = GetSubMenu (hFileMenu, 10);

            //  EnableMenuItem (hFileMenu, 9, MF_BYPOSITION | MF_GRAYED);
                EnableMenuItem (hFileMenu, 10, MF_BYPOSITION | MF_GRAYED);
            }
            #endif

      CheckMenuItem (hMenu, IDM_INVERTCOLORS, bInvertColors ? MF_CHECKED : MF_UNCHECKED);

      if (wDithQuality == 1)
      {
        CheckMenuItem (hMenu, IDM_STRICT, MF_CHECKED);
        CheckMenuItem (hMenu, IDM_RELAXED, MF_UNCHECKED);
      }
      else
      {
        CheckMenuItem (hMenu, IDM_RELAXED, MF_CHECKED);
        CheckMenuItem (hMenu, IDM_STRICT, MF_UNCHECKED);
      }

    }
    break;
    
    #ifdef HELP
    case WM_ENTERIDLE:
      if ((wParam == MSGF_MENU) && (GetKeyState(VK_F1) & 0x8000)) 
      {
        bHelp = TRUE;
        PostMessage(hWnd, WM_KEYDOWN, VK_RETURN, 0L);
      }
      return (DefWindowProc(hWnd, Message, wParam, lParam));
    break;
    
    case WM_KEYDOWN:
      if (wParam == VK_F1) 
      {
    
        /* If Shift-F1, turn help mode on and set help cursor */ 
    
        if (GetKeyState(VK_SHIFT)<0) 
        {
          bHelp = TRUE;
          SetCursor (hHelpCursor);
          return (DefWindowProc(hWnd, Message, wParam, lParam));
        }
        else /* If F1 without shift, then call up help main index topic */ 
          WinHelp (hWnd,(LPSTR) "helpex.hlp",HELP_INDEX,0L);
    
      }
      else 
        if (wParam == VK_ESCAPE && bHelp) 
        {
          /* Escape during help mode: turn help mode off */
          bHelp = FALSE;
          SetCursor ((HCURSOR) GetClassWord (hWnd,GCW_HCURSOR));
        }
        return (DefWindowProc(hWnd, Message, wParam, lParam));
      break;
    
    case WM_SETCURSOR:
                
        /*  In help mode it is necessary to reset the cursor in response */
        /*  to every WM_SETCURSOR message.Otherwise, by default, Windows */
        /*  will reset the cursor to that of the window class. */
    
        if (bHelp) 
        {
          SetCursor (hHelpCursor);
          break;
        }
        return (DefWindowProc(hWnd, Message, wParam, lParam));
          break;

    case WM_NCLBUTTONDOWN:

        /* If we are in help mode (Shift-F1) then display context- */
        /* sensitive help for non-client area. */

        if (bHelp) 
        {
          dwHelpContextId =
                   (wParam == HTCAPTION)     ? (DWORD) HELPID_TITLE_BAR     :
                   (wParam == HTSIZE)        ? (DWORD) HELPID_SIZE_BOX      :
                   (wParam == HTREDUCE)      ? (DWORD) HELPID_MINIMIZE_ICON :
                   (wParam == HTZOOM)        ? (DWORD) HELPID_MAXIMIZE_ICON :
                   (wParam == HTSYSMENU)     ? (DWORD) HELPID_SYSTEM_MENU   :
                   (wParam == HTBOTTOM)      ? (DWORD) HELPID_SIZING_BORDER :
                   (wParam == HTBOTTOMLEFT)  ? (DWORD) HELPID_SIZING_BORDER :
                   (wParam == HTBOTTOMRIGHT) ? (DWORD) HELPID_SIZING_BORDER :
                   (wParam == HTTOP)         ? (DWORD) HELPID_SIZING_BORDER :
                   (wParam == HTLEFT)        ? (DWORD) HELPID_SIZING_BORDER :
                   (wParam == HTRIGHT)       ? (DWORD) HELPID_SIZING_BORDER :
                   (wParam == HTTOPLEFT)     ? (DWORD) HELPID_SIZING_BORDER :
                   (wParam == HTTOPRIGHT)    ? (DWORD) HELPID_SIZING_BORDER :
                                               (DWORD) 0L; 

          if (!((BOOL)dwHelpContextId))
            return (DefWindowProc(hWnd, Message, wParam, lParam));   

            bHelp = FALSE;
            WinHelp (hWnd,(LPSTR) "helpex.hlp",HELP_INDEX,NULL);
            break;
         }
         return (DefWindowProc (hWnd, Message, wParam, lParam));

    case WM_LBUTTONDOWN:
    	if (bHelp)
	    {
	        bHelp = FALSE;
	        WinHelp (hWnd, (LPSTR) "helpex.hlp",HELP_CONTEXT,(DWORD) HELPID_EDIT_WINDOW);
	    }
    break;
    #endif

    case WM_SIZE:
      if (wParam == SIZEFULLSCREEN || wParam == SIZENORMAL)
        if (image_active && IsWindow (hWndDisplay))  // Safety test..
          SetupDispWnd (hWndDisplay, hFileInfo);
    break;

    case WM_SYSCOMMAND:
      switch (wParam)
      {
        case IDM_FILEMGR:
  	      WinExec ((LPSTR)"WINFILE", SW_SHOWNORMAL);
        break;

        case IDM_CTRLPANEL:
  	      WinExec ((LPSTR)"CONTROL", SW_SHOWNORMAL);
        break;

        default:
          return (DefWindowProc (hWnd, Message, wParam, lParam));
      }
      break; 

    case WM_COMMAND:

      MenuCommand (hWnd, Message, wParam, lParam);
    break;


    #ifdef NEWWAY
    case WM_ACTIVATEAPP:

          if (bDisplayQuality)
          {
            HDC hDC;
            HPALETTE hPalette;
            hDC = GetDC (hWnd);
            if (wParam)
            {
              SetSystemPaletteUse (hDC, SYSPAL_NOSTATIC);
              if (hDispPalette)
              {
                UnrealizeObject (hDispPalette);
                hPalette = SelectPalette (hDC, hDispPalette, 0);
                RealizePalette (hDC);
                hDispPalette = SelectPalette (hDC, hPalette, 0);
              }
              ReleaseDC (hWnd, hDC);
              SetSysColors (NUMSYSCOLORS, (LPINT)&SysColorIndices,(LPDWORD)&SysColorValues);
          }
          else
          {
            SetSystemPaletteUse (hDC, SYSPAL_STATIC);
            if (hDispPalette)
            {
              UnrealizeObject (hDispPalette);
              hPalette = SelectPalette (hDC, hDispPalette, 0);
              RealizePalette (hDC);
              hDispPalette = SelectPalette (hDC, hPalette, 0);
            }
            ReleaseDC (hWnd, hDC);
            SetSysColors (NUMSYSCOLORS, (LPINT)&SysColorIndices,(LPDWORD)&DefaultSysColorValues);
          }
          SendMessage (-1, WM_SYSCOLORCHANGE, (WORD)NULL, (LONG)NULL);
        }
        return (DefWindowProc(hWnd, Message, wParam, lParam));
        break;

  #ifdef USEQUERYNEWPALETTE
      case WM_QUERYNEWPALETTE:

      if (hDispPalette)
      {
        HDC       hDC;
        HPALETTE  hPalette;
///     SetCapture (hWnd);
        hDC = GetDC (hWnd);
        hPalette = SelectPalette (hDC, hDispPalette, 0);
        RealizePalette (hDC);
        hDispPalette = SelectPalette (hDC, hPalette, 0);
        ReleaseDC (hWnd, hDC);
        if (image_active)
//        PostMessage (hWnd, WM_USER + 102, 0, 0);
          InvalidateRect (hWndDisplay, (LPRECT)NULL, FALSE);

///     ReleaseCapture();
        return (TRUE);
      }
      break;

      case WM_USER + 102:
        InvalidateRect (hWndDisplay, (LPRECT)NULL, FALSE);
        break;

  #endif

      case WM_PALETTECHANGED:
      if ((wParam != hWnd) && (wParam != hWndDisplay))
      {
        if (image_active && hDispPalette)
        {
          HDC       hDC;
          HPALETTE  hPalette;

          hDC = GetDC (hWndDisplay);
          hPalette = SelectPalette (hDC, hDispPalette, 0);
          RealizePalette (hDC);
          if (nUpdateCount < MAXUPDATECOUNT)
          {
            UpdateColors(hDC);
            nUpdateCount++;
          }
          else
          {
            InvalidateRect (hWndDisplay, (LPRECT)NULL, FALSE);
            nUpdateCount = 0;
          }
          hDispPalette = SelectPalette (hDC, hPalette, 0);
          ReleaseDC (hWndDisplay, hDC);
        }
      }
      break;

    #endif


    case WM_ACTIVATEAPP:

      if (bDisplayQuality){
        HDC hDC;
        HPALETTE hPalette;
        hDC = GetDC (hWnd);
        if (wParam){
          SetSystemPaletteUse (hDC, SYSPAL_NOSTATIC);
          if (hDispPalette){
            UnrealizeObject (hDispPalette);
            hPalette = SelectPalette (hDC, hDispPalette, 0);
            RealizePalette (hDC);
            hDispPalette = SelectPalette (hDC, hPalette, 0);
          }
          ReleaseDC (hWnd, hDC);
          SetSysColors (NUMSYSCOLORS, (LPINT)&SysColorIndices,
            (LPDWORD)&SysColorValues);
        }
        else{
          SetSystemPaletteUse (hDC, SYSPAL_STATIC);
          if (hDispPalette){
            UnrealizeObject (hDispPalette);
            hPalette = SelectPalette (hDC, hDispPalette, 0);
            RealizePalette (hDC);
            hDispPalette = SelectPalette (hDC, hPalette, 0);
          }
          ReleaseDC (hWnd, hDC);
          SetSysColors (NUMSYSCOLORS, (LPINT)&SysColorIndices,
            (LPDWORD)&DefaultSysColorValues);
        }
//      SendMessage (-1, WM_SYSCOLORCHANGE, (WORD)NULL, (LONG)NULL);
      PostMessage (-1, WM_SYSCOLORCHANGE, (WORD)NULL, (LONG)NULL);
      }
      break;

    case WM_QUERYNEWPALETTE:


      if (hDispPalette){
        HDC       hDC;
        HPALETTE  hPalette;
        hDC = GetDC (hWnd);
        hPalette = SelectPalette (hDC, hDispPalette, 0);
        RealizePalette (hDC);
        hDispPalette = SelectPalette (hDC, hPalette, 0);
        ReleaseDC (hWnd, hDC);
        return (TRUE);
      }
      break;


    case WM_SETFOCUS:

      /* 
             DM: Added parent window test to exclude dll dialogs
             DI: Added (* loop *) to find actual highest level window  */

      if (wParam)
      {
        HANDLE hTestWnd;
        HANDLE hWndLostFocus;

        // Find highest-level (* parent *) of window that lost focus 

        hWndLostFocus = hTestWnd = wParam;
        while (hTestWnd != NULL)
        {
            hWndLostFocus = hTestWnd;
            hTestWnd = GetWindowWord (hTestWnd, GWW_HWNDPARENT);
        }

        if ((GetWindowWord (wParam, GWW_HINSTANCE) != 
          GetWindowWord (hWnd, GWW_HINSTANCE)) &&
          (hWndLostFocus != hWnd)){
          if (IsWindow (hWndDisplay))
          {
//          InvalidateRect (hWndDisplay, (LPRECT)NULL, FALSE);
          PostMessage (hWnd, WM_USER + 102, 0, 0);
            nUpdateCount = 0;
          }
        }
      }
      break;

    case WM_PALETTECHANGED:
      if ((wParam != hWnd) && (wParam != hWndDisplay)){
        if (image_active && hDispPalette){
          HDC       hDC;
          HPALETTE  hPalette;

          hDC = GetDC (hWndDisplay);
          hPalette = SelectPalette (hDC, hDispPalette, 0);
          RealizePalette (hDC);
          if (nUpdateCount < MAXUPDATECOUNT){
            UpdateColors(hDC);
            nUpdateCount++;
          }
          else
          {
//          InvalidateRect (hWndDisplay, (LPRECT)NULL, FALSE);
          PostMessage (hWnd, WM_USER + 102, 0, 0);
            nUpdateCount = 0;
          }
          hDispPalette = SelectPalette (hDC, hPalette, 0);
          ReleaseDC (hWndDisplay, hDC);
        }
      }
      break;

      case WM_USER + 102:
        InvalidateRect (hWndDisplay, (LPRECT)NULL, FALSE);
        break;

      case WM_QUERYENDSESSION:
      {
        OFSTRUCT Of;
        int nRetval;

        if (nRetval = ConfirmExit (hWnd))
        {
            SaveSettings (hWnd);
            if (hHistFile)
                _lclose (hHistFile);
            if (OpenFile ((LPSTR)HistFile, (LPOFSTRUCT)&Of, OF_EXIST) != -1)
               OpenFile ((LPSTR)HistFile, (LPOFSTRUCT)&Of, OF_DELETE);
            CleanFiles ();
            return ((LONG) nRetval);
        }
      }
      break;

      case WM_CLOSE:
        
        if (ConfirmExit (hWnd))
        {
          SaveSettings (hWnd);
          DestroyWindow (hWnd);
        }
      break;

      case WM_SHOWIMAGE:
        OpenImage (wParam, lParam);
      break;

      case WM_DESTROY:
      {
        OFSTRUCT Of;

        if (image_active)
          SendMessage (hWndDisplay, WM_CLOSE, 0, 0);    // Close in well-behaved form

//      #ifdef OLDPLACE
        if (hHistFile)
            _lclose (hHistFile);
        if (OpenFile ((LPSTR)HistFile, (LPOFSTRUCT)&Of, OF_EXIST) != -1)
            OpenFile ((LPSTR)HistFile, (LPOFSTRUCT)&Of, OF_DELETE);

        /*   Release capture hook       */
        if (bCaptureOn)
            SetCaptureOn (FALSE);
        /*   Unregister capture       */
        UnRegisterCapApp (hInstCapture);
        GLOBALFREE (hGlobalBufs);
        GLOBALFREE (hGlobalGamma);
        GLOBALFREE (hGlobalColor);
        LocalFree  (hStringBuf);
        #ifdef HELP
        WinHelp (hWnd, (LPSTR) "helpex.hlp", HELP_QUIT, 0L);
        #endif

        CleanFiles ();
//      #endif
        PostQuitMessage (0);
      }
      break;   

      default:
        return (DefWindowProc (hWnd, Message, wParam, lParam));
  }
  return (0L);
}


int FAR ConfirmExit (HWND hWnd)
{
  int nRetval = TRUE;

  if (bImageModified)
  {
    int nResponse;
    PSTR pStringBuf;
    PSTR pStringBuf2;

    pStringBuf  = LocalLock (hStringBuf);
    pStringBuf2 = pStringBuf + MAXSTRINGSIZE;
                
    LoadString (hInstIP, STR_CONFIRM_EXIT, (LPSTR) pStringBuf, MAXSTRINGSIZE);
    LoadString (hInstIP, STR_APPNAME, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
                
    nResponse = MessageBox (hWnd, (LPSTR) pStringBuf, (LPSTR) pStringBuf2, MB_YESNOCANCEL | MB_ICONQUESTION);
    switch (nResponse)
    {
      case IDYES:
        SendMessage (hWnd, WM_COMMAND, IDM_SAVE, NULL);
        nRetval = 1;
      break;
      case IDNO:
        nRetval = 1;
      break;
      case IDCANCEL:
        nRetval = 0L;
      break;
    }
    LocalUnlock (hStringBuf);
  }

  return (nRetval);

}

