#include <windows.h>

HANDLE hInstance;
HWND  hWndIP;

BOOL FAR PASCAL Notify (HANDLE);
int  FAR PASCAL SetupNotify (HWND, HANDLE);
HANDLE hGlobalBufs;

int FAR PASCAL LibMain (hModule, wDataSegment, wHeapSize, lpszCmdLine)
HANDLE  hModule;
WORD    wDataSegment;
WORD    wHeapSize;
LPSTR   lpszCmdLine;
{
    if (wHeapSize)
    {
        hInstance = hModule;
        UnlockData(0);
    }

	return (1);

}

int FAR PASCAL WEP (bSystemExit)
BOOL  bSystemExit;
{
    return (1);
}


int FAR PASCAL SetupNotify (hWnd, hBuf)
HWND hWnd;
HANDLE hBuf;
{
    hWndIP = hWnd;
    hGlobalBufs = hBuf;
    GlobalNotify (MakeProcInstance (Notify, hInstance));
    return (TRUE);
}


BOOL FAR PASCAL Notify (hMem)
HANDLE hMem;
{
    if (hMem == hGlobalBufs)
    {
        MessageBeep (NULL);
        if (IsWindow (hWndIP))
        {
            MessageBox (hWndIP, "Have been notified of a discard", "Discard", MB_OK);

            if (SendMessage (hWndIP, WM_USER+7, NULL, NULL)) // Is image active ?
                return (FALSE);
            else
                return (TRUE);
        }
    }
    return (TRUE);
}


