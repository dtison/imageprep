
/*---------------------------------------------------------------------------

    ROTATEDL.C  Rotate tool interface manager.  Accepts input from the user
                and validates input ...

    CREATED:    3/91   D. Ison

--------------------------------------------------------------------------*/

#include <windows.h>
#include <string.h>
#include <cpi.h>      
#include <cpifmt.h>
#include <malloc.h>
#include <io.h>
#include <prometer.h>
#include <direct.h>
#include "rotatedl.h"
#include "internal.h"
#include "strtable.h"
#include "global.h"

DWORD FAR PASCAL RotateDlgProc (hWnd, Message, wParam, lParam)
HWND      hWnd;
unsigned  Message;
WORD      wParam;
LONG      lParam;
{
    switch (Message)
    {
        case WM_INITDIALOG:
        {

            PSTR            pStringBuf;
            PSTR            pStringBuf2;
            RECT            Rect;
            pStringBuf  = LocalLock (hStringBuf);
            pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

            /* Center the dialog       */

            GetWindowRect (hWnd,&Rect);
            SetWindowPos  (hWnd,NULL,
                          (GetSystemMetrics(SM_CXSCREEN) - (Rect.right - Rect.left)) / 2,
                          (GetSystemMetrics(SM_CYSCREEN) - (Rect.bottom - Rect.top)) / 3,
                           0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

            /*  Save the current drive / dir and other info in a safe place  */

            getcwd (CurrWDir, MAXPATHSIZE);

            InitRotate (hWnd, lParam);
            {
                PROTATE pRotate;        // Temp place for saving current data

                pRotate = (PROTATE) LocalLock (hStructBuf);
                _fmemcpy ((LPSTR) pRotate, (LPSTR) &Rotate, sizeof (ROTATE));
                pRotate -> pSource = pNameBuf;
                pRotate -> pDest   = pRotate -> pSource + MAXPATHSIZE;
                _fstrcpy ((LPSTR) pRotate -> pSource, (LPSTR) SourcePath);
                _fstrcpy ((LPSTR) pRotate -> pDest,   (LPSTR) DestPath);
                LocalUnlock (hStructBuf);
            }
            LoadString (hInstance, STR_ROTATE, (LPSTR) pStringBuf, MAXSTRINGSIZE);
            SetWindowText (hWnd, (LPSTR) pStringBuf); // Caption on dialog

            LocalUnlock (hStringBuf);
        }
        break;

        case WM_COMMAND:
            switch (wParam)
            {
                #ifdef OLDWAY
                case IDD_SOURCE:
                {
                    char        Buffer [128];
                    WORD wTmp = HIWORD (lParam);

                    if (wTmp == CBN_EDITCHANGE)
                        AISMODIFIED;
                    
                    if (wTmp == CBN_KILLFOCUS || wTmp == CBN_SELCHANGE)
                    {
                        int             nIndex;
                        PSTR            pStringBuf;
                        PSTR            pStringBuf2;
                        int             nSelType;

                        pStringBuf  = LocalLock (hStringBuf);
                        pStringBuf2 = pStringBuf + MAXSTRINGSIZE;
                        _fmemset ((LPSTR) pStringBuf, 0, (MAXSTRINGSIZE << 1));

                        /*  Now make sure we are on the right drive  */

                        _chdrive (SourcePath [0] - 96);
                        chdir (&SourcePath[2]);

                        if (wTmp == CBN_SELCHANGE)
                        {
                            if (! (GetKeyState (VK_DOWN) & 0x80 || GetKeyState (VK_UP) & 0x80)) // Was from up/down arrow
                            {
                                nIndex = (int) SendDlgItemMessage (hWnd, IDD_SOURCE, CB_GETCURSEL, NULL, NULL);
                                SendDlgItemMessage (hWnd, IDD_SOURCE, CB_GETLBTEXT, nIndex, (DWORD) (LPSTR) Buffer);
                                nSelType = CleanFileSpec ((LPSTR) pStringBuf, (LPSTR) Buffer);
                                if (nSelType == DIRSPEC || nSelType == DRIVESPEC)
                                {
                                    if (nSelType == DRIVESPEC)
                                    {
                                        int nDrive = *pStringBuf - 96;
                                        _chdrive (nDrive);
                                        getcwd (pStringBuf, MAXSTRINGSIZE);
                                    }
                                    if (pStringBuf [_fstrlen ((LPSTR) pStringBuf) - 1] != '\\')
                                        _fstrcat ((LPSTR) pStringBuf, "\\");
                                    _fstrcat ((LPSTR) pStringBuf, (LPSTR) FileSpec);
                                    DlgDirListComboBox (hWnd, pStringBuf, IDD_SOURCE, IDD_SOURCE_PATH, 0x0000 | 0x0001 | 0x0010 | 0x0020 | 0x4000);  // | 0x8000);
                                    getcwd (pStringBuf, MAXSTRINGSIZE);
                                    _fstrlwr ((LPSTR) pStringBuf);
                                    _fstrcpy ((LPSTR) SourcePath, (LPSTR) pStringBuf); 
                                    PostMessage (hWnd, WM_SHOWTEXT, IDD_SOURCE, (DWORD) (LPSTR) FileSpec);
                                }
                            
                            }
                        }
                        else        // Was a CBN_KILLFOCUS message
                        if (ISAMODIFIED)      // Modified?
                        {
                            GetDlgItemText (hWnd, IDD_SOURCE, (LPSTR) Buffer, sizeof (Buffer));
                            nSelType = CleanFileSpec ((LPSTR) pStringBuf, (LPSTR) Buffer);
                            if (nSelType == DIRSPEC || nSelType == DRIVESPEC)
                            {
                                if (nSelType == DRIVESPEC)
                                {
                                    int nDrive = *pStringBuf - 96;
                                    _chdrive (nDrive);
                                    getcwd (pStringBuf, MAXSTRINGSIZE);
                                }
                                if (pStringBuf [_fstrlen ((LPSTR) pStringBuf) - 1] != '\\')
                                    _fstrcat ((LPSTR) pStringBuf, "\\");
                                _fstrcat ((LPSTR) pStringBuf, (LPSTR) FileSpec);
                                DlgDirListComboBox (hWnd, pStringBuf, IDD_SOURCE, IDD_SOURCE_PATH, 0x0000 | 0x0001 | 0x0010 | 0x0020 | 0x4000); // | 0x8000);
                                getcwd (pStringBuf, MAXSTRINGSIZE);
                                _fstrlwr ((LPSTR) pStringBuf);
                                _fstrcpy ((LPSTR) SourcePath, (LPSTR) pStringBuf); 
                                PostMessage (hWnd, WM_SHOWTEXT, IDD_SOURCE, (DWORD) (LPSTR) FileSpec);
                            }
                            else 
                            {
                                SeparateFile ((LPSTR)pStringBuf2, (LPSTR) pStringBuf, (LPSTR) Buffer);

                                /*  If this path not same as Source Path, change directories and refill l box... */
                        
                                if (_fstrlen (pStringBuf2) > 0)
                                    if (_fstrcmp ((LPSTR) SourcePath, (LPSTR) pStringBuf2) != 0)
                                    {
                                        _fstrcat ((LPSTR) pStringBuf2, (LPSTR) FileSpec);
                                        DlgDirListComboBox (hWnd, pStringBuf2, IDD_SOURCE, IDD_SOURCE_PATH, 0x0000 | 0x0001 | 0x0010 | 0x0020 | 0x4000);  // | 0x8000);
                                        SetDlgItemText (hWnd, IDD_SOURCE,  (LPSTR) Buffer);
                                        GetDlgItemText (hWnd, IDD_SOURCE_PATH, (LPSTR) SourcePath, sizeof (SourcePath));
                                    }
                            }
                        }

                        LocalUnlock (hStringBuf);
                        AISNOTMODIFIED;
                    }
                }
                break;


                case IDD_DEST:
                {
                    char        Buffer [128];
                    WORD wTmp = HIWORD (lParam);

                    if (wTmp == CBN_EDITCHANGE)
                        BISMODIFIED;
                    
                    if (wTmp == CBN_KILLFOCUS || wTmp == CBN_SELCHANGE)
                    {
                        int             nIndex;
                        PSTR            pStringBuf;
                        PSTR            pStringBuf2;
                        int             nSelType;

                        pStringBuf  = LocalLock (hStringBuf);
                        pStringBuf2 = pStringBuf + MAXSTRINGSIZE;
                        _fmemset ((LPSTR) pStringBuf, 0, (MAXSTRINGSIZE << 1));

                        /*  Now make sure we are on the right drive  */

                        _chdrive (DestPath [0] - 96);
                        chdir (&DestPath[2]);

                        if (wTmp == CBN_SELCHANGE)
                        {
                            if (! (GetKeyState (VK_DOWN) & 0x80 || GetKeyState (VK_UP) & 0x80)) // Was from up/down arrow
                            {
                                nIndex = (int) SendDlgItemMessage (hWnd, IDD_DEST, CB_GETCURSEL, NULL, NULL);
                                SendDlgItemMessage (hWnd, IDD_DEST, CB_GETLBTEXT, nIndex, (DWORD) (LPSTR) Buffer);
                                nSelType = CleanFileSpec ((LPSTR) pStringBuf, (LPSTR) Buffer);
                                if (nSelType == DIRSPEC || nSelType == DRIVESPEC)
                                {
                                    if (nSelType == DRIVESPEC)
                                    {
                                        int nDrive = *pStringBuf - 96;
                                        _chdrive (nDrive);
                                        getcwd (pStringBuf, MAXSTRINGSIZE);
                                    }
                                    if (pStringBuf [_fstrlen ((LPSTR) pStringBuf) - 1] != '\\')
                                        _fstrcat ((LPSTR) pStringBuf, "\\");
                                    _fstrcat ((LPSTR) pStringBuf, (LPSTR) FileSpec);
                                    
                                    DlgDirListComboBox (hWnd, pStringBuf, IDD_DEST, IDD_DEST_PATH, 0x0000 | 0x0001 | 0x0010 | 0x0020 | 0x4000);  // | 0x8000);
                                    getcwd (pStringBuf, MAXSTRINGSIZE);
                                    _fstrlwr ((LPSTR) pStringBuf);
                                    _fstrcpy ((LPSTR) DestPath, (LPSTR) pStringBuf); 
                                    PostMessage (hWnd, WM_SHOWTEXT, IDD_DEST, (DWORD) (LPSTR) FileSpec);
                                }
                            
                            }
                        }
                        else        // Was a CBN_KILLFOCUS message
                        if (ISBMODIFIED)      // Modified?
                        {
                            GetDlgItemText (hWnd, IDD_DEST, (LPSTR) Buffer, sizeof (Buffer));
                            nSelType = CleanFileSpec ((LPSTR) pStringBuf, (LPSTR) Buffer);
                            if (nSelType == DIRSPEC || nSelType == DRIVESPEC)
                            {
                                if (nSelType == DRIVESPEC)
                                {
                                    int nDrive = *pStringBuf - 96;
                                    _chdrive (nDrive);
                                    getcwd (pStringBuf, MAXSTRINGSIZE);
                                }
                                if (pStringBuf [_fstrlen ((LPSTR) pStringBuf) - 1] != '\\')
                                    _fstrcat ((LPSTR) pStringBuf, "\\");
                                _fstrcat ((LPSTR) pStringBuf, (LPSTR) FileSpec);
                                DlgDirListComboBox (hWnd, pStringBuf, IDD_DEST, IDD_DEST_PATH, 0x0000 | 0x0001 | 0x0010 | 0x0020 | 0x4000); // | 0x8000);
                                getcwd (pStringBuf, MAXSTRINGSIZE);
                                _fstrlwr ((LPSTR) pStringBuf);
                                _fstrcpy ((LPSTR) DestPath, (LPSTR) pStringBuf); 
                                PostMessage (hWnd, WM_SHOWTEXT, IDD_DEST, (DWORD) (LPSTR) FileSpec);
                            }
                            else 
                            {
                                SeparateFile ((LPSTR)pStringBuf2, (LPSTR) pStringBuf, (LPSTR) Buffer);

                                /*  If this path not same as Source Path, change directories and refill l box... */
                        
                                if (_fstrlen (pStringBuf2) > 0)
                                    if (_fstrcmp ((LPSTR) DestPath, (LPSTR) pStringBuf2) != 0)
                                    {
                                        _fstrcat ((LPSTR) pStringBuf2, (LPSTR) FileSpec);
                                        DlgDirListComboBox (hWnd, pStringBuf2, IDD_DEST, IDD_DEST_PATH, 0x0000 | 0x0001 | 0x0010 | 0x0020 | 0x4000);  // | 0x8000);
                                        SetDlgItemText (hWnd, IDD_DEST,  (LPSTR) Buffer);
                                        GetDlgItemText (hWnd, IDD_DEST_PATH, (LPSTR) DestPath, sizeof (DestPath));
                                    }
                            }
                        }

                        LocalUnlock (hStringBuf);
                        BISNOTMODIFIED;
                    }
                }
                break;
                #endif

                case IDD_CW:
                case IDD_CCW:
                    CheckRadioButton (hWnd, IDD_CW, IDD_CCW, wParam);
                    bDirection = (BYTE) (wParam == IDD_CCW ? CCLOCKWISE : CLOCKWISE);
                    break;

                case IDOK:
                {
                    char            Buffer [128];
                    PSTR            pStringBuf;
                    PSTR            pStringBuf2;
                    HANDLE          hFocusWnd = (HANDLE) NULL;
                    OFSTRUCT        Of;
                    BOOL            bErr = FALSE;
                    char            Source [128];
                    char            Dest   [128];
                    char            Temp   [128];

                    pStringBuf  = LocalLock (hStringBuf);
                    pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

                    /*  Data Validation  */

                    LoadString (hInstance, STR_ROTATE, (LPSTR) pStringBuf2, MAXSTRINGSIZE);

                    #ifdef OLDWAY

                    _fstrcpy ((LPSTR) Source, (LPSTR) SourcePath);
                    if (Source [_fstrlen ((LPSTR) Source) - 1] != '\\')
                        _fstrcat ((LPSTR) Source, "\\");
                    GetDlgItemText (hWnd, IDD_SOURCE, (LPSTR) Buffer, sizeof (Buffer));
                    _fstrcat ((LPSTR) Source, (LPSTR) Buffer);

                    _fstrcpy ((LPSTR) Dest, (LPSTR) DestPath);
                    if (Dest [_fstrlen ((LPSTR) Dest) - 1] != '\\')
                        _fstrcat ((LPSTR) Dest, "\\");
                    _fstrcpy ((LPSTR) Temp, (LPSTR) Dest);
                    GetDlgItemText (hWnd, IDD_DEST, (LPSTR) Buffer, sizeof (Buffer));
                    _fstrcat ((LPSTR) Dest, (LPSTR) Buffer);


                    /*  Test Source Image 1  */

                    if (! bErr)
                    {
                        SeparateFile ((LPSTR) pStringBuf, (LPSTR) Buffer, (LPSTR) Source);

                        if (_fstrlen ((LPSTR) Buffer) == 0)
                        {
                            LoadString (hInstance, STR_MUST_ENTER_SOURCE, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                            hFocusWnd = GetDlgItem (hWnd, IDD_SOURCE);
                            bErr = TRUE;
                        }
                        else
                            if (OpenFile ((LPSTR)Source, (LPOFSTRUCT)&Of, OF_EXIST) <= 0)
                            {
                                LoadString (hInstance, STR_SOURCE_EXIST, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                                hFocusWnd = GetDlgItem (hWnd, IDD_SOURCE);
                                bErr = TRUE;
                            }


                        /*  Test for non-compressed input  */

                        {
                            int hFile;
                            CPIFILESPEC  CPIFileSpec;
                            CPIIMAGESPEC CPIImageSpec;

                            hFile = OpenFile ((LPSTR)Source, (LPOFSTRUCT)&Of, OF_READWRITE);
                            ReadCPIHdr (hFile, (LPCPIFILESPEC) &CPIFileSpec, (LPCPIIMAGESPEC) &CPIImageSpec);
                           _lclose (hFile);

                            if (CPIImageSpec.Compression == CPIFMT_OIC)
                            {
                                LoadString (hInstance, STR_SOURCE_OIC, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                                hFocusWnd = GetDlgItem (hWnd, IDD_SOURCE);
                                bErr = TRUE;
                            }

                            #ifdef RGBONLY
                            if (CPIImageSpec.ImgType != CPIFMT_RGB)
                            {
                                _fstrcpy ((LPSTR) pStringBuf, "Source image is not 24 bit.  I can only work on 24 bit CPI files.");
                                hFocusWnd = GetDlgItem (hWnd, IDD_SOURCE);
                                bErr = TRUE;
                            }
                            #endif
                        }
                    
                    }

                    /*  Test destination image name  */

                    if (! bErr)
                    {
                        SeparateFile ((LPSTR) pStringBuf, (LPSTR) Buffer, (LPSTR) Dest);

                        /*  Go on a little detour to name the "TEMP" file  */

                        _fstrcat ((LPSTR) Temp, (LPSTR) "~tmp____.cpi");
    
                        if (_fstrlen ((LPSTR) Buffer) == 0)
                        {
                            LoadString (hInstance, STR_MUST_ENTER_DEST, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                            hFocusWnd = GetDlgItem (hWnd, IDD_DEST);
                            bErr = TRUE;
                        }
                    }
                    #endif
                    if (bErr)
                    {
                        MessageBeep (NULL);
                        MessageBox (hWnd, pStringBuf, pStringBuf2, MB_OK | MB_ICONQUESTION);
                    }
                    else
                        if (OpenFile ((LPSTR) Dest, (LPOFSTRUCT)&Of, OF_EXIST) > 0)
                        {
                            /*  File Exists, overwrite ? */

                            LoadString (hInstance, STR_IMAGE_EXISTS_CONFIRM, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                            LoadString (hInstance, STR_ROTATE, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
                            if (MessageBox (hWnd, pStringBuf, pStringBuf2, MB_OKCANCEL | MB_ICONQUESTION) == IDCANCEL)
                                bErr = TRUE;
                        }       

                    LocalUnlock (hStringBuf);

                    if (! bErr)
                    {
                        BOOL bTmp;

                        /*  Fill in RotateStruct (and setup the filename ptrs) */

                        Rotate.pSource = pNameBuf;
                        Rotate.pDest   = Rotate.pSource + MAXPATHSIZE;
                        Rotate.pTemp   = Rotate.pDest   + MAXPATHSIZE;

                        _fstrcpy ((LPSTR) Rotate.pSource, (LPSTR) Source);
                        _fstrcpy ((LPSTR) Rotate.pDest,   (LPSTR) Dest);
                        _fstrcpy ((LPSTR) Rotate.pTemp,   (LPSTR) Temp);
                        Rotate.Direction = bDirection;   // Maybe make this a universal variable ???

                        /*  Change the system current drive / directory BACK to what it was  */

                        _chdrive (CurrWDir [0] - 96);
                        chdir (&CurrWDir[2]);

                        b1stRotate = FALSE;
                        EndDialog (hWnd, TRUE);
                    }
                    else
                        SetFocus (hFocusWnd);

                }
                break;


                case IDCANCEL:
                    /*  Change the system current drive / directory and info
                        BACK to what it was  */

                    _chdrive (CurrWDir [0] - 96);
                    chdir (&CurrWDir[2]);

                    {   // Restore previously saved data
                        PROTATE pRotate;        

                        pRotate = (PSMOOTH) LocalLock (hStructBuf);
                        _fmemcpy ((LPSTR) &Rotate, (LPSTR) pRotate, sizeof (SMOOTH));
                        pRotate -> pSource = pNameBuf;
                        pRotate -> pDest   = pRotate -> pSource + MAXPATHSIZE;
                        _fstrcpy ((LPSTR) SourcePath, (LPSTR) pRotate -> pSource);
                        _fstrcpy ((LPSTR) DestPath, (LPSTR) pRotate -> pDest);
                        LocalUnlock (hStructBuf);
                    }
                    EndDialog (hWnd, FALSE);
                    break;

            }
            break;    

        case WM_SHOWTEXT:
        {
            char Buffer [128];

            if (SendDlgItemMessage (hWnd, wParam, CB_GETLBTEXT, 0, (DWORD) (LPSTR) Buffer) != CB_ERR)
                SetDlgItemText (hWnd, wParam, (LPSTR) Buffer);
            SendDlgItemMessage (hWnd, wParam, CB_SETEDITSEL, NULL, MAKELONG (0, 128));
        }

        break;


        default:
            return (FALSE);
    }
    return (TRUE);
}



void NEAR InitRotate (hWnd, lParam)
HWND hWnd;
LONG lParam;
{
    HANDLE  hWndControl;
    char    Buffer [128];
    PSTR            pStringBuf;
    PSTR            pStringBuf2;
    LPSTR           lpPath;

    pStringBuf  = LocalLock (hStringBuf);
    pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

    {
        if (b1stRotate)
        {
            CheckRadioButton (hWnd, IDD_CW, IDD_CCW, IDD_CW);
            bDirection = CLOCKWISE;
        }
        else
        {
            /*  Have already run a rotate.  Put directories and settings 
                back to where they were.  */

            if (Rotate.Direction == 0)
                CheckRadioButton (hWnd, IDD_CW, IDD_CCW, IDD_CW);
            else
                CheckRadioButton (hWnd, IDD_CW, IDD_CCW, (Rotate.Direction == CLOCKWISE ? IDD_CW : IDD_CCW));

            bDirection = Rotate.Direction;
        }

        lpPath = (LPSTR) lParam;

        #ifdef OLDWAY

        /*  First set up source directory / filename ComboBox */

        if (lpPath != NULL)
        {
            _fstrlwr ((LPSTR) lpPath);
            _fstrcpy ((LPSTR) Buffer, (LPSTR) lpPath);
            SeparateFile ((LPSTR)pStringBuf2, (LPSTR) pStringBuf, (LPSTR) Buffer);
            _fstrcpy ((LPSTR) Buffer, (LPSTR) pStringBuf2);
        }
        else
        {
            if (b1stRotate)
                getcwd (pStringBuf, MAXSTRINGSIZE);
            else
                _fstrcpy ((LPSTR) pStringBuf, (LPSTR) SourcePath);

            if (pStringBuf [_fstrlen ((LPSTR) pStringBuf) - 1] != '\\')
                _fstrcat ((LPSTR) pStringBuf, "\\");

            _fstrcpy ((LPSTR) Buffer, (LPSTR) pStringBuf);
        }

        _fstrcat ((LPSTR) Buffer, (LPSTR) FileSpec);
        _fstrlwr ((LPSTR) Buffer);

        DlgDirListComboBox (hWnd, Buffer, IDD_SOURCE, IDD_SOURCE_PATH, 0x0000 | 0x0001 | 0x0010 | 0x0020 | 0x4000);  

        if (lpPath != NULL)
            SetDlgItemText (hWnd, IDD_SOURCE, (LPSTR) pStringBuf);
        else
        {
            if (SendDlgItemMessage (hWnd, IDD_SOURCE, CB_GETLBTEXT, 0, (DWORD) (LPSTR) Buffer) != CB_ERR)
                SetDlgItemText (hWnd, IDD_SOURCE, (LPSTR) Buffer);
        }

        getcwd (pStringBuf, MAXSTRINGSIZE);
        _fstrlwr ((LPSTR) pStringBuf);
        _fstrcpy ((LPSTR) SourcePath, (LPSTR) pStringBuf);


        /*  Now set up dest directory / filename ComboBox  */

        if (b1stRotate)
            getcwd (pStringBuf, MAXSTRINGSIZE);
        else
            _fstrcpy ((LPSTR) pStringBuf, (LPSTR) DestPath);

        if (pStringBuf [_fstrlen ((LPSTR) pStringBuf) - 1] != '\\')
            _fstrcat ((LPSTR) pStringBuf, "\\");

        _fstrcpy ((LPSTR) Buffer, (LPSTR) pStringBuf);
        _fstrcat ((LPSTR) Buffer, (LPSTR) FileSpec);
        _fstrlwr ((LPSTR) Buffer);

        DlgDirListComboBox (hWnd, Buffer, IDD_DEST, IDD_DEST_PATH, 0x0000 | 0x0001 | 0x0010 | 0x0020 | 0x4000);

        SetDlgItemText (hWnd, IDD_DEST, (LPSTR) "temp.cpi");   // Always (?)

        getcwd (pStringBuf, MAXSTRINGSIZE);
        _fstrlwr ((LPSTR) pStringBuf);
        _fstrcpy ((LPSTR) DestPath, (LPSTR) pStringBuf);
        #endif

    }

    LocalUnlock (hStringBuf);
}


