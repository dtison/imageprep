#include <windows.h>
#include <imgprep.h>
#include <global.h>
#include <strtable.h>
#include "res\about.h"
#include <cpi.h>
#include <string.h>
#include <version.h>

/*---------------------------------------------------------------------------

   PROCEDURE:         AboutDlg
   DESCRIPTION:       About Dialog Proc
   DEFINITION:        2.3
   START:             1/3/90  TBJ
   MODS:              9-90    Added disk space display (w/free mem display)

                      9-17-90  No more string constants.  D. Ison

----------------------------------------------------------------------------*/


BOOL FAR PASCAL AboutDlgProc (hWnd, Message, wParam, lParam)
HWND      hWnd;
unsigned  Message;
WORD      wParam;
LONG      lParam;
{
    switch (Message)
    {
        case WM_INITDIALOG:
        {
            DWORD dwFreeMemory;
            DWORD dwFreeDisk;
            DWORD dwWinFlags;
            RECT  Rect;
            enum  {REAL, STANDARD, ENHANCED} Mode = REAL;
            enum  {CPU286, CPU386, CPU486}   CPU  = CPU286;
            WORD  wVersion;
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


        //  LoadString (hInstIP, STR_ABOUT_S, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
        //  wsprintf ((LPSTR) pStringBuf, (LPSTR) pStringBuf2, (LPSTR) szAppName);

            SetWindowText (hWnd, szAppName);

            dwFreeMemory = GetFreeSpace (0);
       
            LoadString (hInstIP, STR_FREE_MEM, (LPSTR) pStringBuf, MAXSTRINGSIZE);
            wsprintf ((LPSTR) pStringBuf2, (LPSTR) pStringBuf, (dwFreeMemory >> 10));
            SetDlgItemText (hWnd, IDD_FREEMEM, (LPSTR)pStringBuf2);

            dwFreeDisk = GetDiskSpace (0);
            LoadString (hInstIP, STR_DISK_SPACE, (LPSTR) pStringBuf, MAXSTRINGSIZE);
            wsprintf ((LPSTR)pStringBuf2, (LPSTR) pStringBuf, (dwFreeDisk >> 10));
            SetDlgItemText (hWnd, IDD_DISKSPACE, (LPSTR)pStringBuf2);

            /*  Do Windows version number  */

            wVersion = GetVersion ();

            LoadString (hInstIP, STR_DDWINDOWS, (LPSTR) pStringBuf, MAXSTRINGSIZE);
            wsprintf ((LPSTR) pStringBuf2,(LPSTR) pStringBuf, LOBYTE (wVersion), HIBYTE (wVersion));
            SetDlgItemText (hWnd, IDD_WINVER, (LPSTR) pStringBuf2);

            /*  WinFlags Stuff */

            dwWinFlags = GetWinFlags ();

            /*  Mode  */

            if (dwWinFlags & WF_STANDARD)
                Mode = STANDARD;
            else
                if (dwWinFlags & WF_ENHANCED)
                    Mode = ENHANCED;

            LoadString (hInstIP, (STR_MODES + (int) Mode), (LPSTR) pStringBuf, MAXSTRINGSIZE);
            SetDlgItemText (hWnd, IDD_WINMODE, (LPSTR)pStringBuf);


            /*  CPU   */

            if (dwWinFlags & WF_CPU386)
                CPU = CPU386;
            else
                if (dwWinFlags & WF_CPU486)
                    CPU = CPU486;

            LoadString (hInstIP, (STR_CPUS + (int) CPU), (LPSTR) pStringBuf, MAXSTRINGSIZE);
            SetDlgItemText (hWnd, IDD_CPU, (LPSTR)pStringBuf);

            {
                HDC  hDC;
                WORD wNumPlanes;
                WORD wBitsPerPixel;
                WORD wNumBits;

                hDC = GetDC (NULL);
                wNumPlanes       = GetDeviceCaps (hDC, PLANES);
                wBitsPerPixel    = GetDeviceCaps (hDC, BITSPIXEL);
                ReleaseDC (NULL, hDC);

                wNumBits = wNumPlanes * wBitsPerPixel;
                LoadString (hInstIP, STR_DISPLAY_BITS, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                wsprintf ((LPSTR) pStringBuf2,(LPSTR) pStringBuf, wNumBits);
                SetDlgItemText (hWnd, IDD_DISPLAY, (LPSTR)pStringBuf2);
            }


            _fstrcpy ((LPSTR) pStringBuf, (LPSTR) "Version 04.00.%03d");
            wsprintf ((LPSTR) pStringBuf2, (LPSTR) pStringBuf, VERSION);
            SetDlgItemText (hWnd, 205, (LPSTR)pStringBuf2);

        LocalUnlock (hStringBuf);
      }
      break;
    case WM_COMMAND:
      switch (wParam){
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



