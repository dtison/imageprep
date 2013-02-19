
#include <windows.h>
#include <string.h>
#include <imgprep.h>
#include <resource.h>
#include <strtable.h>
#include <proto.h>
#include <global.h>
#include <save.h>
#include <error.h>
#include <cpi.h>      // CPI library tools


static int NEAR InitSaveTypes (HWND, WORD);
static int NEAR SetFileExtension (LPSTR, WORD);

void FAR ToCPIName (LPSTR, LPSTR);

#define TMP_SAVE_TYPE 0

int DoSave (int nFlags)
{
    int       hOutFile = 0;        /* Temporary storage for saved file */
    HCURSOR   hOldCursor;
    int       nError = EC_ERROR;                              
    FARPROC   lpfnSaveDlg;
    WORD      wSaveDither;
    OFSTRUCT  Of;
    WORD      wTmpPal;
    WORD      wTmpDither;


    /*  Test "auto-save" to cpi file  */

    if (nFlags == SAVE_TO_CPI)
    {
        char Buffer [128];
        LPFILEINFO lpFileInfo;
        DWORD      dwBytes;
        WORD        wTmpSaveType;

        /*  First get temp path and check the disk space */

        GetTempFileName (0, (LPSTR) "AUTO", 0, (LPSTR) Buffer);

        lpFileInfo = (LPFILEINFO) GlobalLock (hFileInfo);
        dwBytes    = (DWORD) lpFileInfo -> wScanWidth * (DWORD) lpFileInfo -> wScanHeight * (DWORD) (lpFileInfo -> wBitsPerPixel >> 3);
        GlobalUnlock (hFileInfo);

        if (! IsDiskSpaceAvailable ((LPSTR) Buffer, dwBytes))
            return (EC_NODISK);

        hExportFile  = hOutFile = OpenFile ((LPSTR) Buffer, (LPOFSTRUCT)&Of, OF_CREATE | OF_READWRITE);
        wTmpSaveType = wSaveType;               // Rememember old save type, palette and dither
        wTmpPal      = wPal;
        wTmpDither   = wDither;

        wSaveType    = (wImportClass == IRGB) ? XCPI24 : XCPI;
        wPal         = (WORD) bDefPalette;
        wDither      = (WORD) bDefDither;
        SetState (SAVE_TYPE, wSaveType);
        
        nError = ExportFile (nFlags);

        wDither   = wTmpDither;
        wPal      = wTmpPal;
        wSaveType = wTmpSaveType;               // Restore old save type 

        /*  Clean stuff from reader  (this time)  */

        _lclose (hOutFile);
        _lclose (hImportFile);

        hImportFile = 0;
        hExportFile = 0;

        if (nError == 0)
        {
            int (FAR PASCAL *lpfnFlushRead)(LPFILEINFO);

            lpFileInfo = (LPFILEINFO) GlobalLock (hFileInfo);
            lpfnFlushRead = lpFileInfo -> lpfnFlushRead;
            if (lpFileInfo -> lpfnFlushRead != NULL)
                (*lpfnFlushRead)(lpFileInfo);
            GlobalUnlock (hFileInfo);
        }

        CleanFiles ();    // This SHOULD work...

        if (nError == 0)
            OpenImage (AUTO_CONVERT_OPEN, (DWORD) (LPSTR) Buffer);
        else
        {
            OFSTRUCT Of;

            OpenFile ((LPSTR) Buffer, (LPOFSTRUCT)&Of, OF_DELETE);
        }

        return (nError);
    }
    
    /* Save Dither State   */

    wSaveDither = wDither;                        
    
    lpfnSaveDlg = MakeProcInstance ((FARPROC)SaveDlgProc, hInstIP);
    hOutFile = DialogBox (hInstIP, "FILEDLG", hWndIP, lpfnSaveDlg);
    FreeProcInstance (lpfnSaveDlg);
    if (hOutFile <= 0)
    {
        if (hOutFile == 0)
            hOutFile = IDCANCEL;
        nError = hOutFile;
        return (nError);
    }

    hExportFile = hOutFile;
    SetState (SAVE_TYPE, wSaveType);
    
    /*  Special case VGA.. display dif from exp    */
    
    if ((wDevice == DVGA) && (wPal == IOPT))
        GLOBALFREE (hBhist);
    
    #ifdef DEMO
    if (((wSaveType != XCCPI24) && (wSaveType != XCPI24)) && (wSaveType != XCPI)){
      MessageBox (hWndIP, (LPSTR)"Can only save CPI files", (LPSTR)"ImagePrep Demo Version", MB_OK);
      _lclose (hOutFile);
      OpenFile ((LPSTR)szSaveFileName, (LPOFSTRUCT)&Of, OF_DELETE);
      hExportFile = 0;
      return (0);
    }
    #endif
    
    if (wSaveType == XCCPI24)
    {
        _lclose(hOutFile);
        hExportFile = 0;
        nError = CompressCPI (hImportFile, (LPSTR)szSaveFileName);
        return (nError);
    }
    hOldCursor  = SetCursor (LoadCursor (NULL, IDC_WAIT));

    if (hOutFile)
    { 
        nError = ExportFile (nFlags);
        _lclose (hOutFile);
        hExportFile = 0;
        if (bAbandon || nError)
        {
            OpenFile ((LPSTR)szSaveFileName, (LPOFSTRUCT)&Of, OF_DELETE);
            bAbandon = FALSE;
        }
    }
    else        
        nError = EC_SAVEDLG;

    SetCursor (hOldCursor);
    #ifdef NEVER
    SetState (DITH_STATE, wSaveDither);
    if ((wDevice == DVGA) && (wPal == IOPT))                   /* VGA FIX */
        bNewOpt = TRUE;
    #endif
    return (nError);
}   


int FAR PASCAL SaveDlgProc (hWnd, Message, wParam, lParam)
HWND        hWnd;
unsigned    Message;
WORD        wParam;
LONG        lParam;
{
    int       hFile;
    OFSTRUCT  Of;
    extern    WORD wSaveType;
    int       nIndex;
    char    szBuffer[128];                  /* temporary buffer           */

    char szStringBuf1 [128];
    char szStringBuf2 [128];

    switch (Message)
    {
        case WM_INITDIALOG:

            /* Center the dialog       */
            {
                RECT Rect;
        
                GetWindowRect (hWnd,&Rect);
                SetWindowPos  (hWnd,NULL,
                          (GetSystemMetrics(SM_CXSCREEN) - (Rect.right - Rect.left)) / 2,
                          (GetSystemMetrics(SM_CYSCREEN) - (Rect.bottom - Rect.top)) / 3,
                           0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

            }

            LoadString (hInstIP, STR_SAVE_AS, (LPSTR) szStringBuf1, sizeof (szStringBuf1) - 1);
            SetWindowText (hWnd, szStringBuf1);

            /*  New way !!  */

            _fstrcpy ((LPSTR) szSaveFile, (LPSTR) szImageName);
            _fstrcpy ((LPSTR)szTempExt, (LPSTR)szDefaultExts);
            _fstrcpy ((LPSTR) szTempPath, (LPSTR) szSaveDir);

            if (szTempPath [_fstrlen ((LPSTR) szTempPath) - 1] != '\\')
                _fstrcat ((LPSTR) szTempPath, "\\");

            _fstrcat ((LPSTR) szTempPath, (LPSTR) szDefaultExts);  // TEST

            DlgDirList (hWnd, (LPSTR)szTempPath, DU_DIRLIST, DU_PATH, ATTRDIRLIST);
            DlgDirList (hWnd, (LPSTR)szTempExt, DU_FILELIST, DU_PATH, ATTRFILELIST);


            /*  Initialize file types and default save type   */
      
            InitSaveTypes (hWnd, DU_FILETYPE);

            TmpVals [TMP_SAVE_TYPE] = (BYTE) wSaveType;
            _fstrcpy ((LPSTR) rm, (LPSTR) szLastSaveFormat);        // Let's fix this the next release!!  rm is only a temp holding place.
            if (wSaveType == NULL)
            {
                wSaveType = XCPI24;
                LoadString (hInstIP, RS_FMT_CPI24,(LPSTR)szBuffer, sizeof(szBuffer));
            }
            else
                _fstrcpy ((LPSTR) szBuffer, (LPSTR) szLastSaveFormat);

            SetFileExtension ((LPSTR)szSaveFile, wSaveType);
            SetDlgItemText (hWnd, DU_FILENAME, (LPSTR)szSaveFile);

            nIndex = (int)SendDlgItemMessage (hWnd, DU_FILETYPE, CB_FINDSTRING, 0,(LONG)((LPSTR)szBuffer));
            if (nIndex != CB_ERR)
                SendDlgItemMessage (hWnd, DU_FILETYPE, CB_SETCURSEL, nIndex, 0L);
            else
            {
                /*  Another small kludge.  Re-try everything with 8 bit CPI file  */

                wSaveType = XCPI;
                LoadString (hInstIP, RS_FMT_CPI,(LPSTR)szBuffer, sizeof(szBuffer));

                SetFileExtension ((LPSTR)szSaveFile, wSaveType);
                SetDlgItemText (hWnd, DU_FILENAME, (LPSTR)szSaveFile);
                nIndex = (int)SendDlgItemMessage (hWnd, DU_FILETYPE, CB_FINDSTRING, 0,(LONG)((LPSTR)szBuffer));
                SendDlgItemMessage (hWnd, DU_FILETYPE, CB_SETCURSEL, nIndex, 0L);
            }

            ShowWindow (GetDlgItem (hWnd, 113), SW_HIDE);
            ShowWindow (GetDlgItem (hWnd, 114), SW_HIDE);
            break;

        case WM_COMMAND:
            switch (wParam)
            {
                case DU_FILETYPE:
                switch (HIWORD(lParam))
                {
                    case CBN_SELCHANGE:
                    {
                        DWORD   dwIndex;
                        int     i;

                        dwIndex = SendDlgItemMessage (hWnd, wParam, CB_GETCURSEL, 0, 0L);
                        SendDlgItemMessage (hWnd, wParam, CB_GETLBTEXT, (WORD)dwIndex,(LONG)((LPSTR)szBuffer));

                        _fstrcpy ((LPSTR) szLastSaveFormat, (LPSTR) szBuffer);

                        for (i = 0; i < XALLFORMATS; i++)
                        {
                            LoadString (hInstIP, RS_FORMATNAME + i, (LPSTR)szTempFile, sizeof(szTempFile));
                            if (_fstrcmp ((LPSTR)szTempFile, (LPSTR)szBuffer) == 0)
                                wSaveType = i;
                        }
                        if (GetDlgItemText (hWnd, DU_FILENAME, szBuffer, sizeof(szBuffer)))
                            SetFileExtension ((LPSTR)szBuffer, wSaveType);
                        SetDlgItemText (hWnd, DU_FILENAME, szBuffer);
                    }
                    break;
                }
                break;

        case DU_DIRLIST:
          switch (HIWORD(lParam))
          {
            case LBN_DBLCLK:
              DlgDirSelect (hWnd, (LPSTR)szTempPath, wParam);
              _fstrcat ((LPSTR)szTempPath, (LPSTR)szTempExt);
              DlgDirList (hWnd, (LPSTR)szTempPath, DU_DIRLIST, DU_PATH, ATTRDIRLIST);
              DlgDirList (hWnd, (LPSTR)szTempPath, DU_FILELIST, DU_PATH, ATTRFILELIST);
              break;
          } 
          break;
        case DU_FILELIST:
          switch (HIWORD(lParam)){
            case LBN_SELCHANGE:
              DlgDirSelect (hWnd, (LPSTR)szTempFile, wParam);
              SetDlgItemText (hWnd, DU_FILENAME, (LPSTR)szTempFile);
              break;
            case LBN_DBLCLK:
              PostMessage (hWnd, WM_COMMAND, ID_OK, 0L);
              break;
          } 
          break;
        case DU_FILENAME:
          break;
        case DU_PATH:
          break;

        case ID_OK:
        case IDOK:

          /* Get filename and separate into path and filename  */

          if (!GetDlgItemText (hWnd, DU_FILENAME, (LPSTR)szBuffer,
            sizeof(szBuffer)))
            {
                LoadString (hInstIP, ERR_NOFILENAME, (LPSTR) szStringBuf1, sizeof (szStringBuf1) - 1);
                LoadString (hInstIP, STR_SAVE_AS, (LPSTR) szStringBuf2, sizeof (szStringBuf2) - 1);
                MessageBox (hWnd, (LPSTR) szStringBuf1, (LPSTR) szStringBuf2, MB_OK);
                break;
            }

          SeparateFile ((LPSTR)szTempPath, (LPSTR)szTempFile, (LPSTR)szBuffer);
          /*  If there is a filename      */
          if (szTempFile[0])
          { 
            /*  Check for wildcards in filename         */
            if (_fstrchr ((LPSTR)szTempFile, '*') || _fstrchr ((LPSTR)szTempFile, '?')){
              /*  check for path in filename       */
              if (!szTempPath[0])
              {
                  if (GetDlgItemText (hWnd, DU_PATH, (LPSTR)szTempPath,sizeof(szTempPath)))
                     if (szTempPath[0])
                       if (szTempPath[lstrlen(szTempPath)-1] != '\\')
                      _fstrcat ((LPSTR)szTempPath, (LPSTR)"\\");
              }
              _fstrcat ((LPSTR)szTempPath, (LPSTR)szTempExt);

              /*  Update directory listings             */
              if (DlgDirList (hWnd, (LPSTR)szTempPath, DU_DIRLIST, DU_PATH, ATTRDIRLIST)){
                if (DlgDirList (hWnd, (LPSTR)szTempPath, DU_FILELIST, DU_PATH, ATTRFILELIST))
                  SetDlgItemText (hWnd, DU_FILENAME, (LPSTR)szTempPath);
                else{
                  /*
                  **  Process file name error
                  */
                    LoadString (hInstIP, ERR_INVALID_FILENAME, (LPSTR) szStringBuf1, sizeof (szStringBuf1) - 1);
                    LoadString (hInstIP, STR_SAVE_AS, (LPSTR) szStringBuf2, sizeof (szStringBuf2) - 1);
                    MessageBox (hWnd, (LPSTR) szStringBuf1, (LPSTR) szStringBuf2, MB_OK);
                    SetFocus (GetDlgItem (hWnd, DU_FILENAME));
                }
              }
              else{
                /*
                **  Process file path error
                */
                LoadString (hInstIP, ERR_INVALID_PATH, (LPSTR) szStringBuf1, sizeof (szStringBuf1) - 1);
                LoadString (hInstIP, STR_SAVE_AS, (LPSTR) szStringBuf2, sizeof (szStringBuf2) - 1);
                MessageBox (hWnd, (LPSTR) szStringBuf1, (LPSTR) szStringBuf2, MB_OK);
                SetFocus (GetDlgItem (hWnd, DU_PATH));
              }
            }
            else{
              if (szTempPath[0]){
                _fstrcpy ((LPSTR)szSaveFile, (LPSTR)szTempPath);
              }
              else{
                /*
                **  no path, get path from path edit
                */
                GetDlgItemText (hWnd, DU_PATH, (LPSTR)szSaveFile, sizeof(szSaveFile));
                if (szSaveFile[0]){
                  if (szSaveFile[lstrlen(szSaveFile) - 1] != '\\')
                    _fstrcat ((LPSTR)szSaveFile, (LPSTR)"\\");
                }
              }
              _fstrcat ((LPSTR)szSaveFile, (LPSTR)szTempFile);

              /*  Check for duplicate name  */
              
              if (_fstricmp (szSaveFile, szOpenFileName) == 0)
              {
                LoadString (hInstIP, STR_CANNOT_OVERWRITE_S, (LPSTR) szStringBuf1, sizeof (szStringBuf1) - 1);
                LoadString (hInstIP, STR_SAVE_AS, (LPSTR) szStringBuf2, sizeof (szStringBuf2) - 1);
                wsprintf ((LPSTR)szBuffer, szStringBuf1, (LPSTR)szSaveFile);
                MessageBox (hWnd, (LPSTR)szBuffer, szStringBuf2, MB_OK | MB_ICONQUESTION);
              }
              else
              {
                if (OpenFile ((LPSTR)szSaveFile, (LPOFSTRUCT)&Of, OF_EXIST) > 0)
                {
                    LoadString (hInstIP, STR_IMAGE_EXISTS_CONFIRM, (LPSTR) szStringBuf1, sizeof (szStringBuf1) - 1);
                    LoadString (hInstIP, STR_SAVE_AS, (LPSTR) szStringBuf2, sizeof (szStringBuf2) - 1);
                    if (MessageBox (hWnd, (LPSTR) szStringBuf1, szStringBuf2, MB_OKCANCEL | MB_ICONQUESTION) == IDCANCEL)
                      break;
                }
                hFile = OpenFile ((LPSTR)szSaveFile, (LPOFSTRUCT)&Of, OF_READWRITE | OF_CREATE);
                if (hFile <= 0)
                {
                  LoadString (hInstIP, STR_UNABLE_OPEN_S, (LPSTR) szStringBuf1, sizeof (szStringBuf1) - 1);
                  wsprintf ((LPSTR)szBuffer, szStringBuf1, (LPSTR)szSaveFile);
                  LoadString (hInstIP, STR_SAVE_AS, (LPSTR) szStringBuf2, sizeof (szStringBuf2) - 1);
                  MessageBox (hWnd, szBuffer, szStringBuf2, MB_OK);
                }
                else
                {
                  _fstrcpy ((LPSTR)szSaveFileName, (LPSTR)Of.szPathName);
                  EndDialog (hWnd, hFile);
                }
              }
            }
          }
          break;
        case ID_CANCEL:
          wSaveType = (WORD) TmpVals [TMP_SAVE_TYPE];
          _fstrcpy ((LPSTR) szLastSaveFormat, (LPSTR) rm);     //  rm was only a temp holding place.
          EndDialog(hWnd, 0);             /* CANCEL, return no handle */
          break;
      }
      break;
    default:
      return (FALSE);
  }
  return (TRUE);
}



static int NEAR InitSaveTypes (hWnd, nIDComboListBox)
HWND      hWnd;                           /* Dialog window handle       */
WORD      nIDComboListBox;                /* combo list box ID          */
{
    char    szBuffer[128];                  /* temporary buffer           */
    HANDLE  hInstance;                      /* instance handle            */
    int     nEntries = 0;                   /* # of entries in list box   */
    int     i;                              /* temporary counter          */

    hInstance = GetWindowWord (hWnd, GWW_HINSTANCE);

    /* For all available save formats, load corresponding resource string   */

    for (i = 0; i < XALLFORMATS; i++)
    {
        if (SFM[wPal][i])
        {
            #ifdef DECIPHER
            if (i == XCCPI24)
                i = i;
            #else
            if (i == XCCPI24)
            {
                if ((wImportClass == IRGB) && (wImportType == IDFMT_CPI))
                {
                    if (LoadString (hInstance, RS_FORMATNAME + i, (LPSTR)szBuffer, sizeof(szBuffer)))
                    {
                        SendDlgItemMessage (hWnd, nIDComboListBox, CB_ADDSTRING, 0, (LONG)((LPSTR)szBuffer));
                        nEntries++;
                    }
                }
            }
            #endif
            else
            {
                if (LoadString (hInstance, RS_FORMATNAME + i, (LPSTR)szBuffer,sizeof(szBuffer)))
                {
                    SendDlgItemMessage (hWnd, nIDComboListBox, CB_ADDSTRING, 0, (LONG)((LPSTR)szBuffer));
                    nEntries++;
                }
            }
        }
    }

    /*  Set default selection   */

    SendDlgItemMessage (hWnd, nIDComboListBox, CB_SETCURSEL, 0, 0L);
    return (nEntries);
}


static int NEAR SetFileExtension (lpFilename, wSaveType)
LPSTR lpFilename;                   /* filename string to parse       */
WORD  wSaveType;                    /* type of file to be saved       */
{
  char  szExt[13];                  /* buffer for file extension      */
  int   nStrlen;                    /* length of input string         */
  LPSTR lpPtr;                      /* input string ->                */

  if (!lpFilename[0])
    return (0);

  switch (wSaveType){
    case XCPI:
    case XCPI24:
    case XCCPI24:
      _fstrcpy ((LPSTR)szExt, (LPSTR)".cpi");
      break;
    case XWNDIB:
    case XPMDIB:
    case XWNDIB24:
    case XPMDIB24:
      _fstrcpy ((LPSTR)szExt, (LPSTR)".bmp");
      break;
    case XTIF:
    case XTIF24:
      _fstrcpy ((LPSTR)szExt, (LPSTR)".tif");
      break;
    case XTGA:
    case XTGA16:
    case XTGA24:
    case XTGA32:
      _fstrcpy ((LPSTR)szExt, (LPSTR)".tga");
      break;
    case XPCX:
      _fstrcpy ((LPSTR)szExt, (LPSTR)".pcx");
      break;
    case XGIF:
      _fstrcpy ((LPSTR)szExt, (LPSTR)".gif");
      break;
    case XEPS:
    case XEPS24:
      _fstrcpy ((LPSTR)szExt, (LPSTR)".eps");
      break;
    case XWMF:
    case XWMF24:
      _fstrcpy ((LPSTR)szExt, (LPSTR)".wmf");
      break;
    case XDVA24:
      _fstrcpy ((LPSTR)szExt, (LPSTR)".dva");
      break;

  }
  nStrlen = _fstrlen (lpFilename);
  lpPtr = lpFilename + nStrlen;
  while (*lpPtr != '.') 
    lpPtr--;
  if (lpPtr > lpFilename)
    _fstrcpy (lpPtr, szExt);
  else
    _fstrcat (lpFilename, szExt);
  return (1);
}


void FAR ToCPIName (lpDest, lpSource)
LPSTR lpDest;
LPSTR lpSource;
{
    char ch;
    LPSTR lpSourcePtr = lpSource;

    ch = *lpSourcePtr;

    while (ch != 0 && ch != '.')
        ch = *lpSourcePtr++;


    if (ch == '.')
        *(lpSourcePtr - 1) = 0;

    _fstrcpy (lpDest, lpSource);
    _fstrcat (lpDest, (LPSTR) ".CPI");
}

