#include <windows.h>
#include <cpi.h>
#include <string.h>
#include <imgprep.h>
#include <strtable.h>
#include <global.h>
#include <resource.h>
#include "res\info.h"

BOOL FAR PASCAL InfoDlgProc (hWnd, Message, wParam, lParam)
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
            int   nClass;
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


            /*  Now do image info stuff */

            #ifdef OLDWAY
            if (! image_active)
            {

                LoadString (hInstIP, STR_NO_IMAGE_ACTIVE, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                SetDlgItemText (hWnd, 102, (LPSTR)pStringBuf); 

                pStringBuf [0] = 0;
                SetDlgItemText (hWnd, 103, (LPSTR)pStringBuf); 
                SetDlgItemText (hWnd, 104, (LPSTR)pStringBuf); 

            }
            else
            #endif

            {
                LPFILEINFO lpFileInfo;
                PSTR pString;

                lpFileInfo = (LPFILEINFO) GlobalLock (hFileInfo);

                LoadString (hInstIP, STR_D, (LPSTR) pStringBuf, MAXSTRINGSIZE);

                wsprintf ((LPSTR)pStringBuf2, (LPSTR)pStringBuf,lpFileInfo -> wScanWidth);
                SetDlgItemText (hWnd, IDD_WIDTH, (LPSTR)pStringBuf2); 

                LoadString (hInstIP, STR_D, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                wsprintf ((LPSTR)pStringBuf2, (LPSTR)pStringBuf,lpFileInfo -> wScanHeight);
                SetDlgItemText (hWnd, IDD_HEIGHT, (LPSTR)pStringBuf2); 

                switch (wImportClass)
                {
                    case IRGB:
                        nClass = STR_CLS_RGB;
                        break;

                    case ICM:
                        nClass = STR_CLS_CLRMAP;
                        break;

                    case IGR:
                        nClass = STR_GRAY;
                        break;

                    case IMON:
                        nClass = STR_MONOCHROME;
                        break;
                }
                LoadString (hInstIP, nClass, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                SetDlgItemText (hWnd, IDD_TYPE, (LPSTR)pStringBuf); 

                /*  Format  */

                LoadString (hInstIP, (RC_TYPE_BASE + wUserImportType), (LPSTR) pStringBuf, MAXSTRINGSIZE);
                SetDlgItemText (hWnd, IDD_FORMAT, (LPSTR)pStringBuf); 

                GlobalUnlock (hFileInfo);
            }
            LocalUnlock (hStringBuf);
        }
        break;
    
        case WM_COMMAND:
        switch (wParam)
        {
            case IDOK:
                EndDialog (hWnd, TRUE);
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


