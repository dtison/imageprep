
#include <windows.h>
#include <string.h>
#include <imgprep.h>
#include <resource.h>
#include <strtable.h>
#include <proto.h>
#include <global.h>
#include <error.h>
#include <cpi.h>      // CPI library tools
#include <menu.h>
#include "metafile.h"


char *Formats [10] = {"BMP","CPI","TGA","TIF","GIF","PCX","DVA","WMF"};
char *Exts    [10] = {"*.BMP","*.CPI","*.TGA","*.TIF","*.GIF","*.PCX","*.DVA","*.WMF"};
char Selected [10] = {1,1,1,1,1,1,1,1,1,1};

static LPSTR NEAR FillListBox (HWND, LPSTR, int);
static int NEAR UpdateFileList (HWND);

/*---------------------------------------------------------------------------
   PROCEDURE:         DoOpen
   DESCRIPTION:       Response to Open Menu selection
   DEFINITION:        2.1
   MODS:              
----------------------------------------------------------------------------*/


int OpenImage (int nFlags, LONG lParam)
{
    FARPROC   lpfnOpenDlg;
    int       hInFile = 0;                   
    int       nError = EC_ERROR;             
    int       nFileType;                
    OFSTRUCT  Of;
    ATOM  Atom;
    WORD  wBytes;
    int         hTempFile;
    int         nErr = FALSE;

    /*  First get szOpenFileName filled in   */

    switch (nFlags)
    {
        case SCAN_OPEN:
            CleanFiles ();
            Atom = (ATOM) LOWORD (lParam);
            wBytes = GlobalGetAtomName (Atom, szOpenFileName, 128);
            GlobalDeleteAtom (Atom);
            GetNameFromPath ((LPSTR) szImageName, (LPSTR) szOpenFileName);
            bIsCurrTemp = FALSE;
            bImageModified = FALSE;
        break;

        case USER_OPEN:
    //      CleanFiles ();  BUG HERE????
            lpfnOpenDlg = MakeProcInstance ((FARPROC)OpenDlgProc, hInstIP);
            hInFile = DialogBox (hInstIP, "FILEDLG", hWndIP, lpfnOpenDlg);
            FreeProcInstance (lpfnOpenDlg);
            if (hInFile)
            {
                CleanFiles ();          // Put here instead of before call to dialog
                _lclose (hInFile);      // Kludge..

                /*  Always get image name from path.  Optionally also update the szOpenDir  */

                GetNameFromPath ((LPSTR) szImageName, (LPSTR) szOpenFileName);

                if (szOpenFileName [1] == ':')  // Look for A: or B:, if neither, remember open drive/directory
                {
                    if (! (szOpenFileName [0] == 'A' || szOpenFileName[0] == 'B' || szOpenFileName[0] == 'a' || szOpenFileName[0] == 'b'))
                    {
                        SeparateFile ((LPSTR) szOpenDir, (LPSTR) szImageName, (LPSTR)szOpenFileName);
                        _fstrcpy ((LPSTR) szSaveDir, (LPSTR) szOpenDir);
                    }
                }

                bIsCurrTemp = FALSE;


            }
            else
                nErr = USER_ABANDON;
            bImageModified = FALSE;
        break;

        case COMMAND_LINE_OPEN:
            CleanFiles ();
            _fstrcpy ((LPSTR)szOpenFileName, (LPSTR) lParam);
            GetNameFromPath ((LPSTR) szImageName, (LPSTR) szOpenFileName);
            bIsCurrTemp = FALSE;
            bImageModified = FALSE;
        break;

        case TOOLS_OPEN:
            _fstrcpy ((LPSTR)szOpenFileName, (LPSTR) lParam);
            bIsCurrTemp = TRUE;
        break;

        case OIC_OPEN:
        case AUTO_CONVERT_OPEN:
            _fstrcpy ((LPSTR)szOpenFileName, (LPSTR) lParam);
            bIsCurrTemp = TRUE;
            bImageModified = FALSE;
        break;

        case CLIPBOARD_OPEN:
            CleanFiles ();
            Atom = (ATOM) LOWORD (lParam);
            wBytes = GlobalGetAtomName (Atom, szOpenFileName, 128);
            GlobalDeleteAtom (Atom);
            _fstrcpy ((LPSTR) szImageName, (LPSTR) "Untitled");
            bIsCurrTemp = TRUE;
            bImageModified = FALSE;
        break;

        case GENERIC_OPEN:
        case UNDO_OPEN:
            _fstrcpy ((LPSTR)szOpenFileName, (LPSTR) lParam);
        break;

    }

    if (nErr != 0)
        return (nErr);

    hInFile = OpenFile ((LPSTR)szOpenFileName, (LPOFSTRUCT)&Of, OF_READWRITE);

    if (hInFile <= 0)
    {
        if (hInFile == 0)
        hInFile = IDCANCEL;
        nError = hInFile;
        return (nError);
    }

    /* Identify and Import File    */

    if (image_active)
    {
        _fstrcpy ((LPSTR) szTempPath,  (LPSTR) szOpenFileName);     // Store it somewhere

        SendMessage (hWndDisplay, WM_CLOSE, 0, 0);    // Close that baby down!

        if (nFlags == USER_OPEN)                      // So we don't re-display dialog
            nFlags = COMMAND_LINE_OPEN;
        else
            if (nFlags == CLIPBOARD_OPEN)
                nFlags = GENERIC_OPEN;                   

        _lclose (hInFile);

        PostMessage (hWndIP, WM_SHOWIMAGE, nFlags, (LONG) (LPSTR) szTempPath); // Re-prime the pump

        return (0);
    }

    _fstrcpy ((LPSTR) szCurrFileName, (LPSTR) szOpenFileName);  // For having UNDO cap.  5/91

    _llseek (hInFile, 0L, 0);

    nFileType = IdentFormat (hInFile, (LPSTR) szOpenFileName);
    if (nFileType < 0)
    {
        /* File type unknown    */

        _lclose (hInFile);
        hImportFile = 0; 
        return (nFileType);
    }

        switch (nFileType)
        {
            case IDFMT_CCPI:
            {
                char Buffer [128];

                GetTempFileName (0, (LPSTR) "OIC", 0, (LPSTR) Buffer);
                #ifndef DECIPHER
                /* If file type is compressed cpi, decompress first, then re-open     */
                hTempFile = DecompressCPI (hInFile ,(LPSTR) Buffer);
                if (hTempFile > 0)
                {
                    _lclose (hInFile);
                    return (OpenImage (OIC_OPEN, (LONG) (LPSTR) Buffer));
                }
                else
                    return (hTempFile);
    
            }
                #else
            return (EC_UFILETYPE);
                #endif
            break;
    
            case IDFMT_WMF:
            {
                char Buffer [128];
                WORD Retval;

                wUserImportType = (WORD) nFileType;
                GetTempFileName (0, (LPSTR) "WMF", 0, (LPSTR) Buffer);
                Retval = wmfImportMetaFile (hWndIP, hInFile, (LPSTR) Buffer);
                if (Retval == 0)
                {
                    _lclose (hInFile);
                    return (OpenImage (OIC_OPEN, (LONG) (LPSTR) Buffer));
                }
                else
                    return (USER_ABANDON);
    
            }
            break;

            case IDFMT_CPI:
            break;
    
            default:
    
            if (bDoDisplay)           // If we are going to actually display the thing, we may need to do some stuff first..
            if (bAutoConvert)
            {
                /*  Now if opened image is not CPI, save as CPI and re-open as CPI  */
    
                if (wImportType != IDFMT_CPI)
                    if (bAutoConvert)
                    {
                        wUserImportType = (WORD) nFileType;
                        hImportFile = hInFile;
                        SetState (IMPORT_TYPE, nFileType);  
                        nError = ImportFile();
                        if (nError != 0)
                            return (nError);
                        else
                            return (DoSave (SAVE_TO_CPI)); 
                    }
            }
            break;
        }

    hImportFile = hInFile;      
    SetState (IMPORT_TYPE, nFileType);  
    if (! wUserImportType)
        wUserImportType = wImportType;
    nError = ImportFile();

    if (nError >= 0)
        if (! bDoDisplay)
            PostMessage (hWndIP, WM_COMMAND, IDM_SAVE, 0L);

    return (nError);
}


int FAR PASCAL OpenDlgProc (hWnd, Message, wParam, lParam)
HWND        hWnd;
unsigned    Message;
WORD        wParam;
LONG        lParam;
{
  OFSTRUCT  Of;
  int       hFile;
  int      i;

  switch (Message)
  {
    case WM_INITDIALOG:
    {
      PSTR            pStringBuf;
      PSTR            pStringBuf2;

      pStringBuf  = LocalLock (hStringBuf);
      pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

      /* Center the dialog       */
      {
        RECT Rect;
        
        GetWindowRect (hWnd,&Rect);
        SetWindowPos  (hWnd,NULL,
                      (GetSystemMetrics(SM_CXSCREEN) - (Rect.right - Rect.left)) / 2,
                      (GetSystemMetrics(SM_CYSCREEN) - (Rect.bottom - Rect.top)) / 3,
                      0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

      }
      LoadString (hInstIP, STR_OPEN, (LPSTR) pStringBuf, MAXSTRINGSIZE);
      SetWindowText (hWnd, pStringBuf);

      /*  Load default profile information         */

      _fstrcpy ((LPSTR) szTempPath, (LPSTR) szOpenDir);

      if (szTempPath [_fstrlen ((LPSTR) szTempPath) - 1] != '\\')
        _fstrcat ((LPSTR) szTempPath, "\\");

      _fstrcat ((LPSTR) szTempPath, (LPSTR) szDefaultExts);  // TEST

      DlgDirList (hWnd, (LPSTR)szTempPath, DU_DIRLIST, DU_PATH, ATTRDIRLIST);

      ShowWindow (GetDlgItem (hWnd, DU_FILETYPE), SW_HIDE);
      ShowWindow (GetDlgItem (hWnd, 105), SW_HIDE);

      EnableWindow (GetDlgItem (hWnd, DU_FILETYPE), FALSE);


      /*  Test setup of formats listbox  */
  
      {
        int i;
        for (i = 0; i < 8; i++)
          SendMessage (GetDlgItem (hWnd, 113), LB_ADDSTRING, 0, (DWORD) (LPSTR) Formats [i]);
      }

      /*  Put in the formats that were last selected  */

      for (i = 0; i < 8; i++)
        if (Selected [i])
          SendDlgItemMessage (hWnd, 113, LB_SETSEL, TRUE, MAKELONG (i, 0));

      UpdateFileList (hWnd);

//    SendDlgItemMessage (hWnd, DU_FILENAME, EM_SETSEL, NULL, MAKELONG (0, 10));

//    SetFocus (GetDlgItem (hWnd, 113));

      LocalUnlock (hStringBuf);

    }
    break;

    case WM_VKEYTOITEM:
    if (wParam == VK_DELETE)
    {
      PSTR pStringBuf;
      PSTR pStringBuf2;
            char Buffer [128];

      pStringBuf  = LocalLock (hStringBuf);
      pStringBuf2 = pStringBuf + MAXSTRINGSIZE;
            
      LoadString (hInstIP, STR_DELETE_FILE_CONFIRM, (LPSTR) pStringBuf2, MAXSTRINGSIZE);

      GetDlgItemText (hWnd, DU_FILENAME, (LPSTR) Buffer, sizeof (Buffer));

      wsprintf (pStringBuf, pStringBuf2, (LPSTR) Buffer);

      LoadString (hInstIP, STR_OPEN, (LPSTR) pStringBuf2, MAXSTRINGSIZE);

      if (MessageBox (hWnd,
                      (LPSTR)pStringBuf,
                      (LPSTR)pStringBuf2, MB_YESNO) == IDYES)
                      {
                          OFSTRUCT Of;
                          DlgDirSelect (hWnd, (LPSTR)pStringBuf, DU_FILELIST);
                          MessageBeep (NULL);
                          OpenFile ((LPSTR) pStringBuf, (LPOFSTRUCT)&Of, OF_DELETE);
                          UpdateFileList (hWnd);
                          SetDlgItemText (hWnd, DU_FILENAME, (LPSTR)szTempExt);
                      }
      LocalUnlock (hStringBuf);
      return (-2);
    }
    else
      return (-1);
    break;

    case WM_COMMAND:
      switch (wParam)
      {
        case 113:
          switch (HIWORD(lParam))
          {
            case LBN_SELCHANGE:
              UpdateFileList (hWnd);
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
                        #ifdef OLDWAY
                        DlgDirList (hWnd, (LPSTR)szTempPath, DU_FILELIST, DU_PATH, ATTRFILELIST);
                        #else
                        UpdateFileList (hWnd);
                        #endif
                    break;
            } 
            break;

          case DU_FILELIST:
            switch (HIWORD(lParam))
            {
                    case LBN_SELCHANGE:
                        DlgDirSelect (hWnd, (LPSTR)szTempFile, wParam);
                        SetDlgItemText (hWnd, DU_FILENAME, (LPSTR)szTempFile);
                    break;

                    case LBN_DBLCLK:
                        PostMessage (hWnd, WM_COMMAND, ID_OK, 0L);
                    break;

                    case WM_KEYUP:
                        MessageBeep (NULL);
                    break;

            } 
            break;

          case ID_OK:
          case IDOK:
          {

            PSTR pStringBuf;
            PSTR pStringBuf2;

            pStringBuf  = LocalLock (hStringBuf);
            pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

            /*   Get filename and separate into path and filename      */

            if (! GetDlgItemText (hWnd, DU_FILENAME, (LPSTR)pStringBuf2, MAXSTRINGSIZE))
            {
              char Buffer [128];

              LoadString (hInstIP, STR_OPEN, (LPSTR) Buffer, sizeof (Buffer) - 1);
              ReportError (EC_NOFILENAME, (LPSTR) Buffer);
              break;
            }
            SeparateFile ((LPSTR)szTempPath, (LPSTR)szTempFile, (LPSTR)pStringBuf2);

            if (szTempFile[0])
            {
              /*  Check for wildcards in filename       */

              if (_fstrchr ((LPSTR)szTempFile, '*') || _fstrchr ((LPSTR)szTempFile, '?'))
              {
                _fstrcpy ((LPSTR)szTempExt, (LPSTR)szTempFile);
                if (!szTempPath[0])
                {
                  if (GetDlgItemText (hWnd, DU_PATH, (LPSTR)szTempPath, sizeof(szTempPath)))
                    if (szTempPath[0])
                    {
                      if (szTempPath[lstrlen(szTempPath) - 1] != '\\')
                        _fstrcat ((LPSTR)szTempPath, (LPSTR)"\\");
                    }

                }
                _fstrcat ((LPSTR)szTempPath, (LPSTR)szTempExt);

                /* Update directory listings               */
                if (DlgDirList (hWnd, (LPSTR)szTempPath, DU_DIRLIST, DU_PATH, ATTRDIRLIST))
                {
                  if (DlgDirList (hWnd, (LPSTR)szTempPath, DU_FILELIST, DU_PATH, ATTRFILELIST))
                    SetDlgItemText (hWnd, DU_FILENAME, (LPSTR)szTempPath);
                  else
                  {
                    /*   Process file name error            */

                    char Buffer [128];
    
                    LoadString (hInstIP, STR_OPEN, (LPSTR) Buffer, sizeof (Buffer) - 1);
                    ReportError (EC_INVALID_FILENAME, (LPSTR) Buffer);
                    SetFocus (GetDlgItem (hWnd, DU_FILENAME));
                  }

                }
                else
                {

                  char Buffer [128];

                  LoadString (hInstIP, STR_OPEN, (LPSTR) Buffer, sizeof (Buffer) - 1);
                  ReportError (EC_INVALID_PATH, (LPSTR) Buffer);
                  SetFocus (GetDlgItem (hWnd, DU_PATH));
                }
              }
              else
              {
                /*  Filename OK, now check path            */

                if (szTempPath[0])
                  _fstrcpy ((LPSTR)szOpenFile, (LPSTR)szTempPath);
                else
                {
                  /*  No path, get path from path edit                 */

                  GetDlgItemText (hWnd, DU_PATH, (LPSTR)szOpenFile, sizeof(szOpenFile));
                  if (szOpenFile[0])
                    if (szOpenFile[lstrlen(szOpenFile) - 1] != '\\')
                      _fstrcat ((LPSTR)szOpenFile, (LPSTR)"\\");
                }
                _fstrcat ((LPSTR)szOpenFile, (LPSTR)szTempFile);

                /*   Attempt to open file         */

                if (OpenFile ((LPSTR)szOpenFile, (LPOFSTRUCT)&Of, OF_EXIST) > 0)
                {
                  hFile = OpenFile ((LPSTR)szOpenFile, (LPOFSTRUCT)&Of, OF_READWRITE);
                  if (hFile <= 0)
                  {
                    LoadString (hInstIP, STR_UNABLE_OPEN_S, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                    wsprintf ((LPSTR)pStringBuf2, (LPSTR) pStringBuf, (LPSTR)szOpenFile);
                    LoadString (hInstIP, STR_OPEN, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                    MessageBox (hWnd, pStringBuf2, (LPSTR)pStringBuf, MB_OK);
                  }
                  else
                  {
                    _fstrcpy ((LPSTR)szOpenFileName, (LPSTR)Of.szPathName);
                    EndDialog (hWnd, hFile);
                  }
                }
                else
                {
                  /* File must not exist                */

                  LoadString (hInstIP, STR_NOT_EXIST_S, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                  wsprintf ((LPSTR)pStringBuf2, (LPSTR) pStringBuf, (LPSTR)szOpenFile);
                  LoadString (hInstIP, STR_OPEN, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                  MessageBox (hWnd, pStringBuf2, (LPSTR) pStringBuf, MB_OK);
                  SetFocus (GetDlgItem (hWnd, DU_FILENAME));
                }
              }
            }   

            LocalUnlock (hStringBuf);

          } /*  For case IDOK  */
          break;

          case ID_CANCEL:
            EndDialog (hWnd, 0);    
            break;

          default:
            return (FALSE);
      } /*  For switch (wParam)   */ 
      break;

      default:
        return (FALSE);

  }
  return (TRUE);
}


int NEAR UpdateFileList (HWND hWnd)
{
    int i;
    char Buffer [128];

    Buffer [0] = 0;

    for (i = 0; i < 8; i++)
        if (SendDlgItemMessage (hWnd, 113, LB_GETSEL, i, NULL))
        {
            _fstrcat ((LPSTR) Buffer, (LPSTR) Exts [i]);
            _fstrcat ((LPSTR) Buffer, (LPSTR) ";");
            Selected [i] = 1;
        }
        else
            Selected [i] = 0;

        if ((i = _fstrlen ((LPSTR) Buffer)) > 0)
        {
            Buffer [i - 1] = 0;
            SetDlgItemText (hWnd, DU_FILENAME,  (LPSTR) Buffer);
        }

    /*  Now have string separated with semicolons.  Update files listbox. (Old Showdib code) */

    FillListBox (hWnd, (LPSTR) Buffer, DU_FILELIST);

    return (TRUE);
}


/*-----------------------------------------------------------------------------
    FUNCTION   : static char * NEAR FillListBox (hDlg,pFile)            
    PURPOSE    : Fill list box with filenames that match specifications, and
                 fills the static field with the path name.         
    RETURNS    : A pointer to the pathname.                 

----------------------------------------------------------------------------*/

static LPSTR NEAR FillListBox (hDlg, lpExts, nListBox)

HWND    hDlg;
LPSTR   lpExts;        /* [path]{list of file wild cards, separated by ';'} */
int     nListBox;
{
    char    ach[20];
    LPSTR   lpCH;
    LPSTR   lpDir;      /* Directory name or path */

    lpCH  = lpExts;
    lpDir = (LPSTR) ach;

    while (*lpCH && *lpCH != ';')
    lpCH++;
    while ((lpCH > lpExts) && (*lpCH != '/') && (*lpCH != '\\'))
    lpCH--;
    if (lpCH > lpExts) 
    {
       *lpCH = 0;
       lstrcpy (lpDir, lpExts);
       lpExts = lpCH + 1;
    }
    else 
       lstrcpy (lpDir,".");

    SendDlgItemMessage (hDlg, nListBox, LB_RESETCONTENT, 0, 0L);
    SendDlgItemMessage (hDlg, nListBox, WM_SETREDRAW, FALSE, 0L);

    lpDir = lpExts;        /* Save lpExts to return */

    while (*lpExts) 
    {
        lpCH = ach;
        while (*lpExts == ' ')
            lpExts++;
        while (*lpExts && *lpExts != ';')
            *lpCH++ = *lpExts++;
        *lpCH = 0;
        if (*lpExts)
            lpExts++;

        SendDlgItemMessage (hDlg, nListBox, LB_DIR, ATTRFILELIST, (LONG)(LPSTR)ach);
    }
    SendDlgItemMessage (hDlg, nListBox, WM_SETREDRAW, TRUE, 0L);
    InvalidateRect (GetDlgItem (hDlg, nListBox), NULL, TRUE);

    return lpDir;
}

