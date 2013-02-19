
#include <windows.h>
#include <memory.h>
#include <imgprep.h>
#include <proto.h>
#include <global.h>
#include <error.h>
#include <gamma.h>
#include <color.h>
#define BOUND(x,min,max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
#include <cpifmt.h>
#include <merge.h>
//#include <areatool.h>

int nLinesScrolled = 0;

extern HANDLE hWndMerge;
extern BOOL   bIsMerge;


/*--- Stuff used only in this module  --- */

static  HMENU   hIPMenu;
static  DWORD   dwIPClass;
static  DWORD   dwDispClass;


/*---------------------------------------------------------------------------------

   PROCEDURE:        DisplayWndProc
   DESCRIPTION:      Initialization procedure for ImgPrep
   DEFINITION:       3.0  
   START:            TBJ
                     
   MODS:             9-90     Full view and scroll stuff  D. Ison
                     9-17-90  No more string constants.  D. Ison

------------------------------------------------------------------------------*/

long FAR PASCAL DisplayWndProc (hWnd, Message, wParam, lParam)
HWND        hWnd;
unsigned    Message;
WORD        wParam;
LONG        lParam;
{


    switch (Message)
    {
        case WM_CREATE:
        {
            #ifdef OLDWAY
            RECT Rect;
            #endif

            SetupDispWnd (hWnd, hFileInfo);

            #ifdef OLDWAY
            /*  Reset color correction tables if not retaining settings    */
            if (bResetProc)
            {
                LPGAMMATABLE lpGammaTable;
                LPCOLORTABLE lpColorTable;

                lpGammaTable = (LPGAMMATABLE)GlobalLock (hGlobalGamma);
                lpColorTable = (LPCOLORTABLE)GlobalLock (hGlobalColor);
                CreateGammaTable (100, 0, 0, lpGammaTable);
                CreateColorTable (0, 255, 0, lpColorTable);
                UpdateCorrectionTable(hGlobalCTable);
                GlobalUnlock (hGlobalColor);
                GlobalUnlock (hGlobalGamma);
            }
            #endif
            image_active = TRUE;  // Why not here ?
        }

        case WM_PAINT:
        {
            int     err;
  
            if ((!wDither) && (!wPal))
            {
              if ((wImportClass == IRGB) && bTrueColor)
                err = PreviewPicture (hWnd);
              else
                err = PaintPicture (hWnd);
            }
            else
              err = PaintPicture (hWnd);

            if (err < 0)
            {
              ReportError (err, NULL);
              SendMessage (hWnd, WM_CLOSE, 0, 0);
            }

//          return (DefWindowProc (hWnd, Message, wParam, lParam));

        }
        break;

        case WM_LBUTTONDOWN:
            if (bIsMerge)
            {
                if (TmpVals [1] == MERGE_POINT)
                {
                    DWORD       dwPoint;
                    POINT       Point;
                    HDC         hDC;
                    LPFILEINFO  lpFileInfo;
                    RECT        rcClient;

                    Point.x = LOWORD (lParam);
                    Point.y = HIWORD (lParam);

                    /*  For some reason, Windows always returns (1,1) as the 
                        window and viewport extents unless the following steps
                        are taken  */

                    hDC = GetDC (hWnd);

                    GetClientRect (hWnd, (LPRECT) &rcClient);
                    SetMapMode (hDC, MM_ANISOTROPIC);
                    SetWindowOrg (hDC, 0, 0);
                    SetViewportOrg (hDC, 0, 0);
                    lpFileInfo = (LPFILEINFO) GlobalLock (hFileInfo);
                    SetWindowExt (hDC, lpFileInfo->wScanWidth, lpFileInfo->wScanHeight);
                    GlobalUnlock (hFileInfo);
                    SetViewportExt (hDC, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);

                    DPtoLP (hDC, (LPPOINT) &Point, 1);
                    ReleaseDC (hWnd, hDC);
                    dwPoint = MAKELONG (Point.x, Point.y);
                    SendMessage (hWndMerge, WM_SETMERGEPOINT, NULL, dwPoint);
                }
            }
            else
        //      #define EXPERIMENT

                #ifdef EXPERIMENT
                {
                    SetRectEmpty ((LPRECT) &EditRect);
                    RubberBand (hWnd, lParam, (LPRECT) &EditRect, SELECT_AREA);

                    bEditActive = TRUE;
                //  SendMessage (hWndMerge, WM_SETMERGERECT, NULL, (DWORD) (LPRECT) &Rect);
                }
                #endif

        return (DefWindowProc (hWnd, Message, wParam, lParam));
        break;

        case WM_VSCROLL:
        {
            int iMin;   
            int iMax;
            RECT rc;
            int iPos;
            int dn = 0;
            int nScrollRange;

            /* Calculate new vertical scroll position */
        
            GetScrollRange (hWnd, SB_VERT, &iMin, &iMax);
            nScrollRange = iMax - iMin;
            iPos = GetScrollPos (hWnd, SB_VERT);
            GetClientRect (hWnd, &rc);
        
            switch (wParam) 
            {
                case SB_LINEDOWN:      
                    dn =  1;
                    break;
        
                case SB_LINEUP:
                    dn = -1;
                    break;
        
                case SB_PAGEDOWN:
                    dn = (nScrollRange >> 3);
                    break;
        
                case SB_PAGEUP:        
                    dn = -(nScrollRange >> 3);
                    break;
        
                case SB_THUMBTRACK:
                case SB_THUMBPOSITION: 
                    dn = LOWORD(lParam)-iPos;
                    break;

                case SB_ENDSCROLL:
                    /* Limit scrolling to current scroll range */
                        ScrollWindow (hWnd, 0, -nLinesScrolled, NULL, NULL);
                        nLinesScrolled = 0;
                break;

                default:
                   dn = 0;
            }

            if (dn = BOUND (iPos + dn, iMin, iMax) - iPos)
            {
                nLinesScrolled += dn;
                SetScrollPos (hWnd, SB_VERT, iPos + dn, TRUE);
            }
        }
        break;

        case WM_HSCROLL:
        {
            int iMin;   
            int iMax;
            RECT rc;
            int iPos;
            int dn = 0;
            int nScrollRange;

            /* Calculate new horizontal scroll position */
            GetScrollRange (hWnd, SB_HORZ, &iMin, &iMax);
            nScrollRange = iMax - iMin;
            iPos = GetScrollPos (hWnd, SB_HORZ);
            GetClientRect (hWnd, &rc);
            
            switch (wParam) 
            {
                case SB_LINEDOWN:      
                    dn =  1;
                    break;
            
                case SB_LINEUP:        
                    dn = -1;
                    break;
            
                case SB_PAGEDOWN:      
                    dn = (nScrollRange >> 3);
                    break;
            
                case SB_PAGEUP:        
                    dn = -(nScrollRange >> 3);
                    break;
            
                case SB_THUMBTRACK:
                case SB_THUMBPOSITION: 
                    dn = LOWORD (lParam) - iPos;
                    break;
            
            
                    case SB_ENDSCROLL:
                        /* Limit scrolling to current scroll range */
                            ScrollWindow (hWnd, -nLinesScrolled, 0, NULL, NULL);
                            nLinesScrolled = 0;
                    break;
            
            
                default:           
                    dn = 0;
            }

            if (dn = BOUND (iPos + dn, iMin, iMax) - iPos)
            {
                nLinesScrolled += dn;
                SetScrollPos (hWnd, SB_HORZ, iPos + dn, TRUE);
            }
            
    }
    break;

    case WM_RBUTTONDOWN:
    {
        if (bFullScreen)
        {
            bFullScreen = FALSE;
            SetWindowLong (hWnd, GWL_STYLE, dwDispClass);
            SetWindowLong (hWndIP, GWL_STYLE, dwIPClass);
            SetClassLong (hWnd,   GCW_HBRBACKGROUND, GetStockObject (LTGRAY_BRUSH));
            SetClassLong (hWndIP, GCW_HBRBACKGROUND, GetStockObject (LTGRAY_BRUSH));
            SetMenu (hWndIP, hIPMenu);
            ShowWindow (hWndIP, SW_SHOW);
            ShowWindow (hWnd, SW_SHOW);
        }
        else
        {
            RECT Rect;

            bFullScreen = TRUE;

            hIPMenu = GetMenu (hWndIP);
            SetMenu (hWndIP, NULL);
            SetClassLong (hWnd  , GCW_HBRBACKGROUND, GetStockObject (BLACK_BRUSH));
            SetClassLong (hWndIP, GCW_HBRBACKGROUND, GetStockObject (BLACK_BRUSH));
            dwIPClass   = SetWindowLong (hWndIP, GWL_STYLE, WS_POPUP);
            dwDispClass = SetWindowLong (hWnd, GWL_STYLE, WS_POPUP);

            ShowWindow (hWndIP, SW_SHOWMAXIMIZED);
            SetupDispWnd (hWnd, hFileInfo);

            /* Center the window       */

            GetWindowRect (hWnd,&Rect);
            SetWindowPos  (hWnd,NULL,
                          (GetSystemMetrics(SM_CXSCREEN) - (Rect.right - Rect.left)) / 2,
                          (GetSystemMetrics(SM_CYSCREEN) - (Rect.bottom - Rect.top)) / 3,
                           0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

            ShowWindow (hWnd, SW_SHOWNORMAL);
        }
    }
    break;


    case WM_CLOSE:

        if (bIsPainting)
        {
            image_active = FALSE;
            PostMessage (hWnd, WM_CLOSE, 0, 0);
        }
        else
            DestroyWindow (hWnd);

        break;

    case WM_DESTROY:
    {


      /*  Restore to initial states  */

      SetState (EXPORT_CLASS, IRGB);        
      SetState (IMPORT_CLASS, IRGB);        
      SetState (IMPORT_TYPE, 0);           

        if (bResetProc)
        {
            LPGAMMATABLE lpGammaTable;
            LPCOLORTABLE lpColorTable;

// OLD WAY  SetState (DITH_STATE, INONE);

            #ifdef OLDWAY2
            if (! bTrueColor)
                SetState (DITH_STATE, IBAY);
            else
                SetState (DITH_STATE, INONE);
            SetState (PAL_STATE, INORM);
            #endif

            SetState (DITH_STATE, (int) bDefDither);
            SetState (PAL_STATE,  (int) bDefPalette);

            /*  Reset color correction tables if not retaining settings    */

            lpGammaTable = (LPGAMMATABLE)GlobalLock (hGlobalGamma);
            lpColorTable = (LPCOLORTABLE)GlobalLock (hGlobalColor);

            CreateGammaTable (100, 0, 0, lpGammaTable);
            CreateColorTable (0, 255, 0, lpColorTable);

            UpdateCorrectionTable(hGlobalCTable);
            GlobalUnlock (hGlobalColor);
            GlobalUnlock (hGlobalGamma);

        }

      /*   INITIALIZE FLAGS     */

      image_active = FALSE;
      bOptDither   = FALSE;
      bNewDefault  = FALSE;
      bIsTooBig    = FALSE;

      {
          LPFILEINFO lpFileInfo;
          int (FAR PASCAL *lpfnFlushRead)(LPFILEINFO);

          lpFileInfo = (LPFILEINFO) GlobalLock (hFileInfo);
          lpfnFlushRead = lpFileInfo -> lpfnFlushRead;
          if (lpFileInfo -> lpfnFlushRead != NULL)
                (*lpfnFlushRead)(lpFileInfo);
          GlobalUnlock (hFileInfo);
      }

      GLOBALFREE (hBhist);
      GLOBALFREE (hFileInfo);
      GLOBALFREE (hBitmapInfo);
      GLOBALFREE (hImpPalette);

      if (wImportClass == ICM)
        GLOBALFREE (hGlobalPalette);

      GLOBALFREE (hGlobalImageBuf);

      bIsScanFile     = FALSE;
      bImageModified  = FALSE;
      wUserImportType = 0;
      if (hImportFile)
      {
        _lclose (hImportFile);
        hImportFile = 0;
      }

    }
    break;

    default:
      return (DefWindowProc (hWnd, Message, wParam, lParam));
  }
  return (0L);
}


