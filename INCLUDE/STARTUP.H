int FAR PASCAL  WinMain (HANDLE, HANDLE, LPSTR, int);
BOOL FAR PASCAL ImagePrepInit (HANDLE, HANDLE, LPSTR, int);
long FAR PASCAL IPWndProc (HWND, unsigned, WORD, LONG);
void FAR InitSettings (HWND);
void FAR SaveSettings (HWND);

/*
**  Display application logo on startup
*/
BOOL ShowLogo (HWND);
