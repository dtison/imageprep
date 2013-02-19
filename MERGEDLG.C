
/*---------------------------------------------------------------------------

    MERGEDLG.C  Merge Tool interface manager.  Accepts input from the user
                and does its best to validate every conceivable kind of 
                nonsense that might be entered.

    CREATED:    12/90   D. Ison

--------------------------------------------------------------------------*/

#include <windows.h>
#include <string.h>
#include <imgprep.h>
#include <resource.h>
#include <proto.h>
#include <global.h>
#include <save.h>
#include <error.h>
#include <cpi.h>      // CPI library tools
#include <cpifmt.h>
#include <merge.h>
#include "res\mergedlg.h"
#include <malloc.h>
#include <io.h>
#include <prometer.h>
#include <direct.h>
#include <menu.h>
#include "strtable.h"


char    *FileSpec = "*.cpi";
BYTE    ModiFlags;

#define ISAMODIFIED     (ModiFlags &  0x1)
#define AISMODIFIED     (ModiFlags |= 0x1)
#define AISNOTMODIFIED  (ModiFlags &= 0xFFFE)

#define ISBMODIFIED     ((ModiFlags &  0x2) >> 1)
#define BISMODIFIED     (ModiFlags |= 0x2)
#define BISNOTMODIFIED  (ModiFlags &= 0x0)

#define ISMERGEDMODIFIED     ((ModiFlags &  0x4) >> 2)
#define MERGEDISMODIFIED     (ModiFlags |= 0x4)
#define MERGEDISNOTMODIFIED  (ModiFlags &= 0x4)



char PathA [128];
char PathB [128];
char DestPath [128];

extern MERGESTRUCT MergeStruct;

int     nXMerge1;
int     nYMerge1;
int     nXMerge2;
int     nYMerge2;
 
int     X_Length1;
int     Y_Length1;
int     X_Length2;
int     Y_Length2;

BOOL    bMergeActive;        // These two will be combined later...
BOOL    bIsMerge;
HANDLE  hWndMerge;


#define FILESPEC    1
#define DRIVESPEC   2
#define DIRSPEC     3

int FAR PASCAL CleanFileSpec (LPSTR, LPSTR);

DWORD FAR PASCAL MergeDlgProc(hWnd, Message, wParam, lParam)
HWND      hWnd;
unsigned  Message;
WORD      wParam;
LONG      lParam;
{
    MERGESTRUCT TmpMergeStruct;
    int             nTestval;

    switch (Message)
    {
        case WM_INITDIALOG:
        {

            char    Buffer [128];
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

            #define MODELESS

            #ifndef MODELESS
                hWndMerge = hWnd;   // Save window handle
            #endif

            if (! bMergeActive)
                InitMerge (hWnd);
            else
            {       // Reset values that were there before

                _fmemcpy ((LPSTR) &TmpMergeStruct, (LPSTR) &MergeStruct, sizeof (MERGESTRUCT));

                CheckRadioButton (hWnd, IDD_UNION, IDD_INTERSECT, TmpMergeStruct.wMode == INTERSECT ? IDD_INTERSECT : IDD_UNION);
                TmpVals [0] = (BYTE) (TmpMergeStruct.wMode == IDD_MERGE_POINT ? MERGE_POINT : DRAW_AREA);
                CheckRadioButton (hWnd, IDD_DRAW_AREA, IDD_MERGE_POINT, TmpMergeStruct.wView == MERGE_POINT ? IDD_MERGE_POINT : IDD_DRAW_AREA);

                TmpVals [0] = (BYTE) TmpMergeStruct.wMode;
                #ifdef RUBBER
                TmpVals [1] = (BYTE) TmpMergeStruct.wView;
                #endif

                _fstrcpy ((LPSTR) Buffer, (LPSTR) PathA);
                _fstrcat ((LPSTR) Buffer, (LPSTR) "\\");
                _fstrcat ((LPSTR) Buffer, (LPSTR) FileSpec);
                SeparateFile ((LPSTR)pStringBuf2, (LPSTR) pStringBuf, (LPSTR) Buffer);
                _fstrcat ((LPSTR) pStringBuf2, (LPSTR) FileSpec);
                DlgDirListComboBox (hWnd, pStringBuf2, IDD_SOURCEA, IDD_SOURCEA_PATH, 0x0000 | 0x0001 | 0x0010 | 0x0020 | 0x4000 | 0x8000);
                SetDlgItemText (hWnd, IDD_SOURCEA,  (LPSTR) TmpMergeStruct.FilenameA);

                _fstrcpy ((LPSTR) Buffer, (LPSTR) PathB);
                _fstrcat ((LPSTR) Buffer, (LPSTR) "\\");
                _fstrcat ((LPSTR) Buffer, (LPSTR) FileSpec);
                SeparateFile ((LPSTR)pStringBuf2, (LPSTR) pStringBuf, (LPSTR) Buffer);
                _fstrcat ((LPSTR) pStringBuf2, (LPSTR) FileSpec);
                DlgDirListComboBox (hWnd, pStringBuf2, IDD_SOURCEB, IDD_SOURCEB_PATH, 0x0000 | 0x0001 | 0x0010 | 0x0020 | 0x4000 | 0x8000);
                SetDlgItemText (hWnd, IDD_SOURCEB,  (LPSTR) TmpMergeStruct.FilenameB);

                _fstrcpy ((LPSTR) Buffer, (LPSTR) DestPath);
                _fstrcat ((LPSTR) Buffer, (LPSTR) "\\");
                _fstrcat ((LPSTR) Buffer, (LPSTR) FileSpec);
                SeparateFile ((LPSTR)pStringBuf2, (LPSTR) pStringBuf, (LPSTR) Buffer);
                _fstrcat ((LPSTR) pStringBuf2, (LPSTR) FileSpec);
                DlgDirListComboBox (hWnd, pStringBuf2, IDD_MERGED, IDD_MERGED_PATH, 0x0000 | 0x0001 | 0x0010 | 0x0020 | 0x4000 | 0x8000);
                SetDlgItemText (hWnd, IDD_MERGED,  (LPSTR) TmpMergeStruct.DestFilename);


                SetDlgItemText (hWnd, IDD_SOURCEA_PATH,  (LPSTR) PathA);
                SetDlgItemText (hWnd, IDD_SOURCEB_PATH,  (LPSTR) PathB);
                SetDlgItemText (hWnd, IDD_MERGED_PATH,  (LPSTR) DestPath);

                SetDlgItemInt (hWnd, IDD_X1, TmpMergeStruct.nPtX1, TRUE);
                SetDlgItemInt (hWnd, IDD_Y1, TmpMergeStruct.nPtY1, TRUE);
                #ifdef RUBBER
                SetDlgItemInt (hWnd, IDD_EXTX1, TmpMergeStruct.nExtX1, TRUE);
                SetDlgItemInt (hWnd, IDD_EXTY1, TmpMergeStruct.nExtY1, TRUE);
                #endif
                SetDlgItemInt (hWnd, IDD_X2, TmpMergeStruct.nPtX2, TRUE);
                SetDlgItemInt (hWnd, IDD_Y2, TmpMergeStruct.nPtY2, TRUE);
                #ifdef RUBBER
                SetDlgItemInt (hWnd, IDD_EXTX2, TmpMergeStruct.nExtX2, TRUE);
                SetDlgItemInt (hWnd, IDD_EXTY2, TmpMergeStruct.nExtY2, TRUE);
                #endif
                SetDlgItemInt (hWnd, IDD_X3, TmpMergeStruct.nX3, TRUE);
                SetDlgItemInt (hWnd, IDD_Y3, TmpMergeStruct.nY3, TRUE);

            }

            /*  Setup Icon  */
            {
                HANDLE hIcon;

                hIcon = LoadIcon (hInstIP, MAKEINTRESOURCE (MERGE_ICON));
                SetClassWord (hWnd, GCW_HICON, hIcon);
            }
            LocalUnlock (hStringBuf);

        }
        break;
    
        case WM_COMMAND:
            switch (wParam)
            {
                case IDD_SOURCEA:
                {
                    char        Buffer [128];
                    WORD wTmp = HIWORD (lParam);

                    if (wTmp == CBN_EDITCHANGE)
                        AISMODIFIED;
                    
                    if (wTmp == CBN_KILLFOCUS || wTmp == CBN_SELCHANGE)
                    {
                        OFSTRUCT    Of;
                        CPIFILESPEC  CPIFileSpec;
                        CPIIMAGESPEC CPIImageSpec;
                        int         hFile;
                        
                        int         nIndex;
                        PSTR            pStringBuf;
                        PSTR            pStringBuf2;
                        int             nSelType;

                        pStringBuf  = LocalLock (hStringBuf);
                        pStringBuf2 = pStringBuf + MAXSTRINGSIZE;
                        _fmemset ((LPSTR) pStringBuf, 0, (MAXSTRINGSIZE << 1));

                        /*  Now make sure we are on the right drive  */

                        _chdrive (PathA [0] - 96);
                        chdir (&PathA[2]);

                        if (wTmp == CBN_SELCHANGE)
                        {
                            if (! (GetKeyState (VK_DOWN) & 0x80 || GetKeyState (VK_UP) & 0x80)) // Was from up/down arrow
                            {
                                nIndex = (int) SendDlgItemMessage (hWnd, IDD_SOURCEA, CB_GETCURSEL, NULL, NULL);
                                SendDlgItemMessage (hWnd, IDD_SOURCEA, CB_GETLBTEXT, nIndex, (DWORD) (LPSTR) Buffer);
                                nSelType = CleanFileSpec ((LPSTR) pStringBuf, (LPSTR) Buffer);
                                if (nSelType == DIRSPEC || nSelType == DRIVESPEC)
                                {
                                    if (nSelType == DRIVESPEC)
                                    {
                                        int nDrive = *pStringBuf - 96;
                                        _chdrive (nDrive);
                                        getcwd (pStringBuf, MAXSTRINGSIZE);
                                    }
                                    _fstrcat ((LPSTR) pStringBuf, (LPSTR) "\\");
                                    _fstrcat ((LPSTR) pStringBuf, (LPSTR) FileSpec);
                                    
                                    DlgDirListComboBox (hWnd, pStringBuf, IDD_SOURCEA, IDD_SOURCEA_PATH, 0x0000 | 0x0001 | 0x0010 | 0x0020 | 0x4000 | 0x8000 | 0x2000);
                                    getcwd (Buffer, sizeof (Buffer));
                                    _fstrlwr ((LPSTR) Buffer);
                                    _fstrcpy ((LPSTR) PathA, (LPSTR) Buffer); 
                                    PostMessage (hWnd, WM_SHOWTEXT, IDD_SOURCEA, (DWORD) (LPSTR) FileSpec);
                                }
                            
                            }
                        }
                        else        // Was a CBN_KILLFOCUS message
                        if (ISAMODIFIED)      // Modified?
                        {
                            GetDlgItemText (hWnd, IDD_SOURCEA, (LPSTR) Buffer, sizeof (Buffer));
                            nSelType = CleanFileSpec ((LPSTR) pStringBuf, (LPSTR) Buffer);
                            if (nSelType == DIRSPEC || nSelType == DRIVESPEC)
                            {
                                if (nSelType == DRIVESPEC)
                                {
                                    int nDrive = *pStringBuf - 96;
                                    _chdrive (nDrive);
                                    getcwd (pStringBuf, MAXSTRINGSIZE);
                                }
                                _fstrcat ((LPSTR) pStringBuf, (LPSTR) "\\");
                                _fstrcat ((LPSTR) pStringBuf, (LPSTR) FileSpec);
                                DlgDirListComboBox (hWnd, pStringBuf, IDD_SOURCEA, IDD_SOURCEA_PATH, 0x0000 | 0x0001 | 0x0010 | 0x0020 | 0x4000 | 0x8000 | 0x2000);
                                getcwd (Buffer, sizeof (Buffer));
                                _fstrlwr ((LPSTR) Buffer);
                                _fstrcpy ((LPSTR) PathA, (LPSTR) Buffer); 
                                PostMessage (hWnd, WM_SHOWTEXT, IDD_SOURCEA, (DWORD) (LPSTR) FileSpec);
                            }
                            else 
                            {
                                SeparateFile ((LPSTR)pStringBuf2, (LPSTR) pStringBuf, (LPSTR) Buffer);

                                /*  If this path not same as A Path, change directories and refill l box... */
                        
                                if (_fstrlen (pStringBuf2) > 0)
                                    if (_fstrcmp ((LPSTR) PathA, (LPSTR) pStringBuf2) != 0)
                                    {
                                        _fstrcat ((LPSTR) pStringBuf2, (LPSTR) FileSpec);
                                        DlgDirListComboBox (hWnd, pStringBuf2, IDD_SOURCEA, IDD_SOURCEA_PATH, 0x0000 | 0x0001 | 0x0010 | 0x0020 | 0x4000 | 0x8000);
                                        SetDlgItemText (hWnd, IDD_SOURCEA,  (LPSTR) Buffer);
                                        GetDlgItemText (hWnd, IDD_SOURCEA_PATH, (LPSTR) PathA, sizeof (PathA));
                                    }
                            }
                        }

                        if (_fstrlen (pStringBuf) > 0)
                        {
                            hFile = OpenFile ((LPSTR) pStringBuf, (LPOFSTRUCT)&Of, OF_READWRITE);
                            if (hFile > 0)
                            {
                                ReadCPIHdr (hFile, (LPCPIFILESPEC) &CPIFileSpec, (LPCPIIMAGESPEC) &CPIImageSpec);

                                /*  Set default dialog values  */
                        
                                SetDlgItemInt (hWnd, IDD_X1, CPIImageSpec.X_Length, TRUE);
                                SetDlgItemInt (hWnd, IDD_Y1, 0, TRUE);
                                SetDlgItemInt (hWnd, IDD_X3, CPIImageSpec.X_Length, TRUE);
                                SetDlgItemInt (hWnd, IDD_Y3, 0, TRUE);

                                X_Length1 = CPIImageSpec.X_Length;
                                Y_Length1 = CPIImageSpec.Y_Length;
                                _lclose (hFile);
                            }
                            else
                            {
                                SetDlgItemInt (hWnd, IDD_X1, 0, TRUE);
                                SetDlgItemInt (hWnd, IDD_Y1, 0, TRUE);
                                SetDlgItemInt (hWnd, IDD_X3, 0, TRUE);
                                SetDlgItemInt (hWnd, IDD_Y3, 0, TRUE);
                            }
                        }
                        LocalUnlock (hStringBuf);
                        AISNOTMODIFIED;
                    }
                }
                break;

                case IDD_SOURCEB:
                {
                    char        Buffer [128];
                    WORD wTmp = HIWORD (lParam);

                    if (wTmp == CBN_EDITCHANGE)
                        BISMODIFIED;
                    
                    if (wTmp == CBN_KILLFOCUS || wTmp == CBN_SELCHANGE)
                    {
                        OFSTRUCT    Of;
                        CPIFILESPEC  CPIFileSpec;
                        CPIIMAGESPEC CPIImageSpec;
                        int         hFile;
                        HANDLE      
                        int         nIndex;
                        PSTR            pStringBuf;
                        PSTR            pStringBuf2;
                        int             nSelType;

                        pStringBuf  = LocalLock (hStringBuf);
                        pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

                        /*  Now make sure we are on the right drive  */

                        _chdrive (PathB [0] - 96);
                        chdir (&PathB[2]);

                        if (wTmp == CBN_SELCHANGE)
                        {
                            if (! (GetKeyState (VK_DOWN) & 0x80 || GetKeyState (VK_UP) & 0x80)) // Was from up/down arrow
                            {
                                nIndex = (int) SendDlgItemMessage (hWnd, IDD_SOURCEB, CB_GETCURSEL, NULL, NULL);
                                SendDlgItemMessage (hWnd, IDD_SOURCEB, CB_GETLBTEXT, nIndex, (DWORD) (LPSTR) Buffer);
                                nSelType = CleanFileSpec ((LPSTR) pStringBuf, (LPSTR) Buffer);
                                if (nSelType == DIRSPEC || nSelType == DRIVESPEC)
                                {
                                    if (nSelType == DRIVESPEC)
                                    {
                                        int nDrive = *pStringBuf - 96;
                                        _chdrive (nDrive);
                                        getcwd (pStringBuf, MAXSTRINGSIZE);
                                    }
                                    _fstrcat ((LPSTR) pStringBuf, (LPSTR) "\\");
                                    _fstrcat ((LPSTR) pStringBuf, (LPSTR) FileSpec);
                                    
                                    DlgDirListComboBox (hWnd, pStringBuf, IDD_SOURCEB, IDD_SOURCEB_PATH, 0x0000 | 0x0001 | 0x0010 | 0x0020 | 0x4000 | 0x8000 | 0x2000);
                                    getcwd (Buffer, sizeof (Buffer));
                                    _fstrlwr ((LPSTR) Buffer);
                                    _fstrcpy ((LPSTR) PathB, (LPSTR) Buffer); 
                                    PostMessage (hWnd, WM_SHOWTEXT, IDD_SOURCEB, (DWORD) (LPSTR) FileSpec);
                                }
                            
                            }
                        }
                        else        // Was a CBN_KILLFOCUS message
                        if (ISBMODIFIED)      // Modified?
                        {
                            GetDlgItemText (hWnd, IDD_SOURCEB, (LPSTR) Buffer, sizeof (Buffer));
                            nSelType = CleanFileSpec ((LPSTR) pStringBuf, (LPSTR) Buffer);
                            if (nSelType == DIRSPEC || nSelType == DRIVESPEC)
                            {
                                if (nSelType == DRIVESPEC)
                                {
                                    int nDrive = *pStringBuf - 96;
                                    _chdrive (nDrive);
                                    getcwd (pStringBuf, MAXSTRINGSIZE);
                                }
                                _fstrcat ((LPSTR) pStringBuf, (LPSTR) "\\");
                                _fstrcat ((LPSTR) pStringBuf, (LPSTR) FileSpec);
                                DlgDirListComboBox (hWnd, pStringBuf, IDD_SOURCEB, IDD_SOURCEB_PATH, 0x0000 | 0x0001 | 0x0010 | 0x0020 | 0x4000 | 0x8000 | 0x2000);
                                getcwd (Buffer, sizeof (Buffer));
                                _fstrlwr ((LPSTR) Buffer);
                                _fstrcpy ((LPSTR) PathB, (LPSTR) Buffer); 
                                PostMessage (hWnd, WM_SHOWTEXT, IDD_SOURCEB, (DWORD) (LPSTR) FileSpec);
                            }
                            else 
                            {
                                SeparateFile ((LPSTR)pStringBuf2, (LPSTR) pStringBuf, (LPSTR) Buffer);

                                /*  If this path not same as B Path, change directories and refill l box... */
                        
                                if (_fstrlen (pStringBuf2) > 0)
                                    if (_fstrcmp ((LPSTR) PathB, (LPSTR) pStringBuf2) != 0)
                                    {
                                        _fstrcat ((LPSTR) pStringBuf2, (LPSTR) FileSpec);
                                        DlgDirListComboBox (hWnd, pStringBuf2, IDD_SOURCEB, IDD_SOURCEB_PATH, 0x0000 | 0x0001 | 0x0010 | 0x0020 | 0x4000 | 0x8000);
                                        SetDlgItemText (hWnd, IDD_SOURCEB,  (LPSTR) Buffer);
                                        GetDlgItemText (hWnd, IDD_SOURCEB_PATH, (LPSTR) PathB, sizeof (PathB));
                                    }
                            }
                        }

                        hFile = OpenFile ((LPSTR) pStringBuf, (LPOFSTRUCT)&Of, OF_READWRITE);
                        if (hFile > 0)
                        {
                            ReadCPIHdr (hFile, (LPCPIFILESPEC) &CPIFileSpec, (LPCPIIMAGESPEC) &CPIImageSpec);

                            /*  Set default dialog values  */
                        
                            SetDlgItemInt (hWnd, IDD_X2, 0, TRUE);
                            SetDlgItemInt (hWnd, IDD_Y2, 0, TRUE);

                            X_Length2 = CPIImageSpec.X_Length;
                            Y_Length2 = CPIImageSpec.Y_Length;
                            _lclose (hFile);
                        }
                        else
                        {
                            SetDlgItemInt (hWnd, IDD_X2, 0, TRUE);
                            SetDlgItemInt (hWnd, IDD_Y2, 0, TRUE);
                        }
                        LocalUnlock (hStringBuf);
                        BISNOTMODIFIED;
                    }
                }
                break;

                case IDD_MERGED:
                {
                    char        Buffer [128];
                    WORD wTmp = HIWORD (lParam);

                    if (wTmp == CBN_EDITCHANGE)
                        MERGEDISMODIFIED;
                    
                    if (wTmp == CBN_KILLFOCUS || wTmp == CBN_SELCHANGE)
                    {
                        int         nIndex;
                        PSTR            pStringBuf;
                        PSTR            pStringBuf2;
                        int             nSelType;

                        pStringBuf  = LocalLock (hStringBuf);
                        pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

                        /*  Now make sure we are on the right drive  */

                        _chdrive (DestPath [0] - 96);
                        chdir (&DestPath[2]);

                        if (wTmp == CBN_SELCHANGE)
                        {
                            if (! (GetKeyState (VK_DOWN) & 0x80 || GetKeyState (VK_UP) & 0x80)) // Was from up/down arrow
                            {
                                nIndex = (int) SendDlgItemMessage (hWnd, IDD_MERGED, CB_GETCURSEL, NULL, NULL);
                                SendDlgItemMessage (hWnd, IDD_MERGED, CB_GETLBTEXT, nIndex, (DWORD) (LPSTR) Buffer);
                                nSelType = CleanFileSpec ((LPSTR) pStringBuf, (LPSTR) Buffer);
                                if (nSelType == DIRSPEC || nSelType == DRIVESPEC)
                                {
                                    if (nSelType == DRIVESPEC)
                                    {
                                        int nDrive = *pStringBuf - 96;
                                        _chdrive (nDrive);
                                        getcwd (pStringBuf, MAXSTRINGSIZE);
                                    }
                                    _fstrcat ((LPSTR) pStringBuf, (LPSTR) "\\");
                                    _fstrcat ((LPSTR) pStringBuf, (LPSTR) FileSpec);
                                    
                                    DlgDirListComboBox (hWnd, pStringBuf, IDD_MERGED, IDD_MERGED_PATH, 0x0000 | 0x0001 | 0x0010 | 0x0020 | 0x4000 | 0x8000 | 0x2000);
                                    getcwd (Buffer, sizeof (Buffer));
                                    _fstrlwr ((LPSTR) Buffer);
                                    _fstrcpy ((LPSTR) DestPath, (LPSTR) Buffer); 
                                    PostMessage (hWnd, WM_SHOWTEXT, IDD_MERGED, (DWORD) (LPSTR) FileSpec);
                                }
                            
                            }
                        }
                        else        // Was a CBN_KILLFOCUS message
                        if (ISMERGEDMODIFIED)      // Modified?
                        {
                            GetDlgItemText (hWnd, IDD_MERGED, (LPSTR) Buffer, sizeof (Buffer));
                            nSelType = CleanFileSpec ((LPSTR) pStringBuf, (LPSTR) Buffer);
                            if (nSelType == DIRSPEC || nSelType == DRIVESPEC)
                            {
                                if (nSelType == DRIVESPEC)
                                {
                                    int nDrive = *pStringBuf - 96;
                                    _chdrive (nDrive);
                                    getcwd (pStringBuf, MAXSTRINGSIZE);
                                }
                                _fstrcat ((LPSTR) pStringBuf, (LPSTR) "\\");
                                _fstrcat ((LPSTR) pStringBuf, (LPSTR) FileSpec);
                                DlgDirListComboBox (hWnd, pStringBuf, IDD_MERGED, IDD_MERGED_PATH, 0x0000 | 0x0001 | 0x0010 | 0x0020 | 0x4000 | 0x8000 | 0x2000);
                                getcwd (Buffer, sizeof (Buffer));
                                _fstrlwr ((LPSTR) Buffer);
                                _fstrcpy ((LPSTR) DestPath, (LPSTR) Buffer); 
                                PostMessage (hWnd, WM_SHOWTEXT, IDD_MERGED, (DWORD) (LPSTR) FileSpec);
                            }
                            else 
                            {
                                SeparateFile ((LPSTR)pStringBuf2, (LPSTR) pStringBuf, (LPSTR) Buffer);

                                /*  If this path not same as B Path, change directories and refill l box... */
                        
                                if (_fstrlen (pStringBuf2) > 0)
                                    if (_fstrcmp ((LPSTR) DestPath, (LPSTR) pStringBuf2) != 0)
                                    {
                                        _fstrcat ((LPSTR) pStringBuf2, (LPSTR) FileSpec);
                                        DlgDirListComboBox (hWnd, pStringBuf2, IDD_MERGED, IDD_MERGED_PATH, 0x0000 | 0x0001 | 0x0010 | 0x0020 | 0x4000 | 0x8000);
                                        SetDlgItemText (hWnd, IDD_MERGED,  (LPSTR) Buffer);
                                        GetDlgItemText (hWnd, IDD_MERGED_PATH, (LPSTR) DestPath, sizeof (DestPath));
                                    }
                            }
                        }
                        MERGEDISNOTMODIFIED;
                        LocalUnlock (hStringBuf);
                    }
                }
                break;


                case IDD_RESET:
                    InitMerge (hWnd);
                break;


                case IDD_UNION:
                case IDD_INTERSECT:
                    CheckRadioButton (hWnd, IDD_UNION, IDD_INTERSECT, wParam);
                    TmpVals [0] = (BYTE) (wParam == IDD_INTERSECT ? INTERSECT : UNION);
                    break;

                #ifdef RUBBER
                case IDD_DRAW_AREA:
                case IDD_MERGE_POINT:
                    CheckRadioButton (hWnd, IDD_DRAW_AREA, IDD_MERGE_POINT, wParam);
                    TmpVals [1] = (BYTE) (wParam == IDD_MERGE_POINT ? MERGE_POINT : DRAW_AREA);
                    break;
                #endif

                case IDD_VIEW_A:
                {
                    char Buffer [128];
                
                    int hFile;
                    PSTR pStringBuf;
                    PSTR pStringBuf2;
                    OFSTRUCT Of;
                    CPIFILESPEC     CPIFileSpec;
                    CPIIMAGESPEC    CPIImageSpec;

                    _chdrive (PathA [0] - 96);
                    chdir (&PathA[2]);

                    GetDlgItemText (hWnd, IDD_SOURCEA, (LPSTR) Buffer, sizeof (Buffer));

                    /*  First see if the image is merge-able */

                    hFile = OpenFile ((LPSTR) Buffer, (LPOFSTRUCT)&Of, OF_READWRITE);
                    ReadCPIHdr (hFile, (LPCPIFILESPEC) &CPIFileSpec, (LPCPIIMAGESPEC) &CPIImageSpec);
                    _lclose (hFile);
                    if (CPIImageSpec.ImgType != CPIFMT_RGB)
                    {
                        pStringBuf  = LocalLock (hStringBuf);
                        pStringBuf2 = pStringBuf + MAXSTRINGSIZE;
                        LoadString (hInstIP, STR_MERGE, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
                        LoadString (hInstIP, STR_SOURCE_1_NOT_24, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                        MessageBox (hWnd, pStringBuf, pStringBuf2, MB_OK | MB_ICONQUESTION);
                        LocalUnlock (hStringBuf);
                    }
                    else
                    {
                        if ((TmpVals [3] & 0x80) != 0x80) // Test whether value has already been saved
                        {
                            TmpVals [3] = (BYTE) wFullView;
                            TmpVals [3] |= 0x80;
                        }

                        /*  Then go ahead and bring the image up */

                        if (image_active)
                        {
                            DestroyWindow (hWndDisplay);
                            image_active = FALSE;
                        }

                        bIsScanFile = TRUE;
                        bIsMerge    = 1;
                        wFullView   = TRUE;       // Always set TRUE
//                      PostMessage (hWndIP, WM_SHOWIMAGE, 1, (DWORD) (LPSTR) Buffer);
                        PostMessage (hWndIP, WM_SHOWIMAGE, COMMAND_LINE_OPEN, (DWORD) (LPSTR) Buffer);
                    }
                }
                break;
                case IDD_VIEW_B:
                {
                    char Buffer [128];
                
                    int hFile;
                    PSTR pStringBuf;
                    PSTR pStringBuf2;
                    OFSTRUCT Of;
                    CPIFILESPEC     CPIFileSpec;
                    CPIIMAGESPEC    CPIImageSpec;

                    _chdrive (PathB [0] - 96);
                    chdir (&PathB[2]);

                    GetDlgItemText (hWnd, IDD_SOURCEB, (LPSTR) Buffer, sizeof (Buffer));

                    /*  First see if the image is merge-able */

                    hFile = OpenFile ((LPSTR) Buffer, (LPOFSTRUCT)&Of, OF_READWRITE);
                    ReadCPIHdr (hFile, (LPCPIFILESPEC) &CPIFileSpec, (LPCPIIMAGESPEC) &CPIImageSpec);
                    _lclose (hFile);
                    if (CPIImageSpec.ImgType != CPIFMT_RGB)
                    {
                        pStringBuf  = LocalLock (hStringBuf);
                        pStringBuf2 = pStringBuf + MAXSTRINGSIZE;
                        LoadString (hInstIP, STR_MERGE, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
                        LoadString (hInstIP, STR_SOURCE_2_NOT_24, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                        MessageBox (hWnd, pStringBuf, pStringBuf2, MB_OK | MB_ICONQUESTION);
                        LocalUnlock (hStringBuf);
                    }
                    else
                    {
                        if ((TmpVals [3] & 0x80) != 0x80) // Test whether value has already been saved
                        {
                            TmpVals [3] = (BYTE) wFullView;
                            TmpVals [3] |= 0x80;
                        }

                        /*  Then go ahead and bring the image up */

                        if (image_active)
                        {
                            DestroyWindow (hWndDisplay);
                            image_active = FALSE;
                        }

                        _fstrcpy ((LPSTR)szOpenFileName, (LPSTR) Buffer);
                        bIsScanFile = TRUE;
                        bIsMerge    = 2;
                        wFullView   = TRUE;       // Always set TRUE
//                      PostMessage (hWndIP, WM_COMMAND, IDM_OPEN, 0L);
                        PostMessage (hWndIP, WM_SHOWIMAGE, COMMAND_LINE_OPEN, (DWORD) (LPSTR) Buffer);

                    }
                }
                break;

                case IDD_X1:
                {
                    PSTR    pStringBuf;
                    PSTR    pStringBuf2;

                    int     nUserVal;
                    BOOL    bErr = FALSE;
                    int     nTmp;

                    if (HIWORD (lParam) == EN_KILLFOCUS)
                        if (SendMessage (LOWORD (lParam), EM_GETMODIFY, NULL, NULL))
                        {
                            pStringBuf  = LocalLock (hStringBuf);
                            pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

                            nUserVal = GetDlgItemInt (hWnd, IDD_X1, (LPINT) &nTestval, TRUE);
                            nTmp = (nUserVal - nXMerge2);

                            if (nUserVal > X_Length1 || nUserVal < 0)
                            {
                                LoadString (hInstIP, STR_MERGE_POINT_PHYS, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                                LoadString (hInstIP, STR_MERGE, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
                                MessageBox (hWnd, pStringBuf, pStringBuf2, MB_OK | MB_ICONQUESTION);

                                SendDlgItemMessage (hWnd, IDD_X1, EM_UNDO, NULL, NULL);
                                SendDlgItemMessage (hWnd, IDD_X1, EM_SETMODIFY, FALSE, NULL);
                                bErr = TRUE;
                            }
                            else
                                if (nTmp < 0)
                                {
                                    LoadString (hInstIP, STR_MERGE_X_NEGATIVE, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                                    LoadString (hInstIP, STR_MERGE, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
                                    MessageBox (hWnd, pStringBuf, pStringBuf2, MB_OK | MB_ICONQUESTION);
                        
                                    SendDlgItemMessage (hWnd, IDD_X1, EM_UNDO, NULL, NULL);
                                    SendDlgItemMessage (hWnd, IDD_X1, EM_SETMODIFY, FALSE, NULL);
                                    bErr = TRUE;
                                }

                            LocalUnlock (hStringBuf);

                            if (! bErr)
                            {
                                /*  Update the merge position */
            
                                SetDlgItemInt (hWnd, IDD_X3, nTmp, TRUE);
                                nXMerge1 = nUserVal;
                            }
                        }
                    
                }
                break;

                case IDD_Y1:
                {
                    PSTR    pStringBuf;
                    PSTR    pStringBuf2;

                    int     nUserVal;
                    BOOL    bErr = FALSE;
                    int     nTmp;

                    if (HIWORD (lParam) == EN_KILLFOCUS)
                        if (SendMessage (LOWORD (lParam), EM_GETMODIFY, NULL, NULL))
                        {
                            pStringBuf  = LocalLock (hStringBuf);
                            pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

                            nUserVal = GetDlgItemInt (hWnd, IDD_Y1, (LPINT) &nTestval, TRUE);
                            if (nUserVal > Y_Length1 || nUserVal < 0)
                            {
                                LoadString (hInstIP, STR_MERGE_POINT_PHYS, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                                LoadString (hInstIP, STR_MERGE, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
                                MessageBox (hWnd, pStringBuf, pStringBuf2, MB_OK | MB_ICONQUESTION);

                                SendDlgItemMessage (hWnd, IDD_Y1, EM_UNDO, NULL, NULL);
                                SendDlgItemMessage (hWnd, IDD_Y1, EM_SETMODIFY, FALSE, NULL);
                                bErr = TRUE;
                            }
                            LocalUnlock (hStringBuf);

                            if (! bErr)
                            {
                                /*  Update the merge position */
            
                                nTmp = (nUserVal - nYMerge2);
                                SetDlgItemInt (hWnd, IDD_Y3, nTmp, TRUE);
                                nYMerge1 = nUserVal;
                            }
                        }
                    
                }
                break;

                case IDD_X2:
                {
                    PSTR    pStringBuf;
                    PSTR    pStringBuf2;

                    int     nUserVal;
                    BOOL    bErr = FALSE;
                    int     nTmp;
                    int     nTmpXMerge3;

                    if (HIWORD (lParam) == EN_KILLFOCUS)
                        if (SendMessage (LOWORD (lParam), EM_GETMODIFY, NULL, NULL))
                        {
                            pStringBuf  = LocalLock (hStringBuf);
                            pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

                            nUserVal = GetDlgItemInt (hWnd, IDD_X2, (LPINT) &nTestval, TRUE);
                            nTmp     = GetDlgItemInt (hWnd, IDD_X1, (LPINT) &nTestval, TRUE);
                            nTmpXMerge3 = (nTmp - nUserVal);
                            if (nUserVal > X_Length2 || nUserVal < 0)
                            {
                                LoadString (hInstIP, STR_MERGE_POINT_PHYS, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                                LoadString (hInstIP, STR_MERGE, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
                                MessageBox (hWnd, pStringBuf, pStringBuf2, MB_OK | MB_ICONQUESTION);

                                SendDlgItemMessage (hWnd, IDD_X2, EM_UNDO, NULL, NULL);
                                SendDlgItemMessage (hWnd, IDD_X2, EM_SETMODIFY, FALSE, NULL);
                                bErr = TRUE;
                            }
                            else
                                if (nTmpXMerge3 < 0)
                                {
                                    LoadString (hInstIP, STR_MERGE_X_NEGATIVE, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                                    LoadString (hInstIP, STR_MERGE, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
                                    MessageBox (hWnd, pStringBuf, pStringBuf2, MB_OK | MB_ICONQUESTION);
                        
                                    SendDlgItemMessage (hWnd, IDD_X2, EM_UNDO, NULL, NULL);
                                    SendDlgItemMessage (hWnd, IDD_X2, EM_SETMODIFY, FALSE, NULL);
                                    bErr = TRUE;
                                }
                            LocalUnlock (hStringBuf);

                            if (! bErr)
                            {
                                /*  Update the merge position */
            
                                nXMerge2 = nUserVal;
                                SetDlgItemInt (hWnd, IDD_X3, nTmpXMerge3, TRUE);
                            }
                        }
                }
                break;

                case IDD_Y2:
                {
                    PSTR    pStringBuf;
                    PSTR    pStringBuf2;

                    int     nUserVal;
                    BOOL    bErr = FALSE;
                    int     nTmp;

                    if (HIWORD (lParam) == EN_KILLFOCUS)
                        if (SendMessage (LOWORD (lParam), EM_GETMODIFY, NULL, NULL))
                        {
                            pStringBuf  = LocalLock (hStringBuf);
                            pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

                            nUserVal = GetDlgItemInt (hWnd, IDD_Y2, (LPINT) &nTestval, TRUE);
                            if (nUserVal > Y_Length2 || nUserVal < 0)
                            {
                                LoadString (hInstIP, STR_MERGE_POINT_PHYS, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                                LoadString (hInstIP, STR_MERGE, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
                                MessageBox (hWnd, pStringBuf, pStringBuf2, MB_OK | MB_ICONQUESTION);

                                SendDlgItemMessage (hWnd, IDD_Y2, EM_UNDO, NULL, NULL);
                                SendDlgItemMessage (hWnd, IDD_Y2, EM_SETMODIFY, FALSE, NULL);
                                bErr = TRUE;

                            }
                            LocalUnlock (hStringBuf);

                            if (! bErr)
                            {
                                /*  Update the merge position */
            
                                nYMerge2 = nUserVal;
                                nTmp = nYMerge1 - nYMerge2;
                                SetDlgItemInt (hWnd, IDD_Y3, nTmp, TRUE);
                            }
                        }
                    
                }
                break;

                case IDD_X3:
                {
                    PSTR    pStringBuf;
                    PSTR    pStringBuf2;

                    int     nUserVal;
                    BOOL    bErr = FALSE;
                    int     nTmp;
                    int     nTmpXMerge2;

                    if (HIWORD (lParam) == EN_KILLFOCUS)
                        if (SendMessage (LOWORD (lParam), EM_GETMODIFY, NULL, NULL))
                        {
                            pStringBuf  = LocalLock (hStringBuf);
                            pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

                            nUserVal    = GetDlgItemInt (hWnd, IDD_X3, (LPINT) &nTestval, TRUE);
                            nTmp        = GetDlgItemInt (hWnd, IDD_X1, (LPINT) &nTestval, TRUE);
                            nTmpXMerge2 = (nTmp - nUserVal);
    
                            if (nUserVal > X_Length1 || nUserVal > X_Length2 || nUserVal < 0)
                            {
                                LoadString (hInstIP, STR_MERGE_POINT_PHYS, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                                LoadString (hInstIP, STR_MERGE, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
                                MessageBox (hWnd, pStringBuf, pStringBuf2, MB_OK | MB_ICONQUESTION);

                                SendDlgItemMessage (hWnd, IDD_X3, EM_UNDO, NULL, NULL);
                                SendDlgItemMessage (hWnd, IDD_X3, EM_SETMODIFY, FALSE, NULL);
                                bErr = TRUE;
                            }
                            else
                                if (nTmpXMerge2 < 0)
                                {
                                    LoadString (hInstIP, STR_MERGE_X_NEGATIVE, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                                    LoadString (hInstIP, STR_MERGE, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
                                    MessageBox (hWnd, pStringBuf, pStringBuf2, MB_OK | MB_ICONQUESTION);
                        
                                    SendDlgItemMessage (hWnd, IDD_X3, EM_UNDO, NULL, NULL);
                                    SendDlgItemMessage (hWnd, IDD_X3, EM_SETMODIFY, FALSE, NULL);
                                    bErr = TRUE;
                                }

                            LocalUnlock (hStringBuf);

                            if (! bErr)
                            {
                                /*  Update the X2 merge position */

                                nXMerge2 = nTmpXMerge2;
                                SetDlgItemInt (hWnd, IDD_X2, nXMerge2, TRUE);
                            }
                        }
                }
                break;

                case IDD_Y3:
                {
                    PSTR    pStringBuf;
                    PSTR    pStringBuf2;

                    int     nUserVal;
                    BOOL    bErr = FALSE;
                    int     nTmp;
                    int     nTmpYMerge2;

                    if (HIWORD (lParam) == EN_KILLFOCUS)
                        if (SendMessage (LOWORD (lParam), EM_GETMODIFY, NULL, NULL))
                        {
                            pStringBuf  = LocalLock (hStringBuf);
                            pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

                            nUserVal    = GetDlgItemInt (hWnd, IDD_Y3, (LPINT) &nTestval, TRUE);
                            nTmp        = GetDlgItemInt (hWnd, IDD_Y1, (LPINT) &nTestval, TRUE);
                            nTmpYMerge2 = (nTmp - nUserVal);

                            if (nUserVal > Y_Length1 || nUserVal > Y_Length2 || nUserVal < 0)
                            {
                                LoadString (hInstIP, STR_MERGE_POINT_PHYS, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                                LoadString (hInstIP, STR_MERGE, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
                                MessageBox (hWnd, pStringBuf, pStringBuf2, MB_OK | MB_ICONQUESTION);

                                SendDlgItemMessage (hWnd, IDD_Y3, EM_UNDO, NULL, NULL);
                                SendDlgItemMessage (hWnd, IDD_Y3, EM_SETMODIFY, FALSE, NULL);
                                bErr = TRUE;

                            }
                            LocalUnlock (hStringBuf);
                            if (! bErr)
                            {
                                /*  Update the Y2 merge position */

                                nYMerge2 = nTmpYMerge2;
                                SetDlgItemInt (hWnd, IDD_Y2, nYMerge2, TRUE);
                            }
                        }
                    
                }
                break;

                case IDOK:
                {
                    char            Buffer [128];
                    PSTR            pStringBuf;
                    PSTR            pStringBuf2;
                    HANDLE          hFocusWnd = (HANDLE) NULL;
                    OFSTRUCT        Of;
                    CPIFILESPEC     CPIFileSpec1;
                    CPIIMAGESPEC    CPIImageSpec1;
                    CPIFILESPEC     CPIFileSpec2;
                    CPIIMAGESPEC    CPIImageSpec2;
                    BOOL            bErr = FALSE;
                    char            SourceA [128];
                    char            SourceB [128];
                    char            Dest    [128];
                    char            FilenameA [13];
                    char            FilenameB [13];
                    char            DestFilename [13];
                    int             nX1 = 0;
                    int             nY1 = 0;
                    int             nExtX1 = X_Length1;
                    int             nExtY1 = Y_Length1;
                    int             nX2 = 0;
                    int             nY2 = 0;
                    int             nExtX2 = X_Length2;
                    int             nExtY2 = Y_Length2;
                    int             nX3;
                    int             nY3;

                    pStringBuf  = LocalLock (hStringBuf);
                    pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

                    /*  Data Validation  */

                    LoadString (hInstIP, STR_MERGE, (LPSTR) pStringBuf2, MAXSTRINGSIZE);

                    GetDlgItemText (hWnd, IDD_SOURCEB, (LPSTR) SourceB, sizeof (SourceB));
                    GetDlgItemText (hWnd, IDD_SOURCEA, (LPSTR) SourceA, sizeof (SourceA));
                    GetDlgItemText (hWnd, IDD_MERGED,  (LPSTR) Dest, sizeof (Dest));
                    GetDlgItemText (hWnd, IDD_SOURCEA, (LPSTR) FilenameA, sizeof (FilenameA));
                    GetDlgItemText (hWnd, IDD_SOURCEB, (LPSTR) FilenameB, sizeof (FilenameB));
                    GetDlgItemText (hWnd, IDD_MERGED,  (LPSTR) DestFilename, sizeof (DestFilename));

                    if (_fstrcmp ((LPSTR) SourceA, (LPSTR) Dest) == 0)
                    {
                        LoadString (hInstIP, STR_SOURCE_MERGED_DIFF, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                        hFocusWnd = GetDlgItem (hWnd, IDD_SOURCEA);
                        bErr = TRUE;
                    }

                    if (_fstrcmp ((LPSTR) SourceB, (LPSTR) Dest) == 0)
                    {
                        LoadString (hInstIP, STR_SOURCE_MERGED_DIFF, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                        hFocusWnd = GetDlgItem (hWnd, IDD_SOURCEB);
                        bErr = TRUE;
                    }


                    /*  Test Source Image 1  */

                    if (! bErr)
                    {
                        SeparateFile ((LPSTR) pStringBuf, (LPSTR) Buffer, (LPSTR) SourceA);

                        _fstrcpy ((LPSTR) SourceA, (LPSTR) PathA);
                        if (SourceA [_fstrlen ((LPSTR) SourceA) - 1] != '\\')
                            _fstrcat ((LPSTR) SourceA, "\\");
                        _fstrcat ((LPSTR) SourceA, (LPSTR) Buffer);
            

                        if (_fstrlen ((LPSTR) Buffer) == 0)
                        {
                            LoadString (hInstIP, STR_MUST_ENTER_SOURCE_1, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                            hFocusWnd = GetDlgItem (hWnd, IDD_SOURCEA);
                            bErr = TRUE;
                        }
                        else
                            if (OpenFile ((LPSTR)SourceA, (LPOFSTRUCT)&Of, OF_EXIST) <= 0)
                            {
                                LoadString (hInstIP, STR_SOURCE_1_EXIST, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                                hFocusWnd = GetDlgItem (hWnd, IDD_SOURCEA);
                                bErr = TRUE;
                            }
                            else
                            {
                                int hFile;

                                hFile = OpenFile ((LPSTR) SourceA, (LPOFSTRUCT)&Of, OF_READWRITE);

                                ReadCPIHdr (hFile, (LPCPIFILESPEC) &CPIFileSpec1, (LPCPIIMAGESPEC) &CPIImageSpec1);
                            
                                if (CPIImageSpec1.ImgType != CPIFMT_RGB)
                                {
                                    LoadString (hInstIP, STR_SOURCE_1_NOT_24, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                                    hFocusWnd = GetDlgItem (hWnd, IDD_SOURCEA);
                                    bErr = TRUE;
                                }
                                _lclose (hFile);
                            }
                    }

                    /*  Test Source Image 2  */

                    if (! bErr)
                    {
                        SeparateFile ((LPSTR) pStringBuf, (LPSTR) Buffer, (LPSTR) SourceB);

                        _fstrcpy ((LPSTR) SourceB, (LPSTR) PathB);
                        if (SourceB [_fstrlen ((LPSTR) SourceB) - 1] != '\\')
                            _fstrcat ((LPSTR) SourceB, "\\");
                        _fstrcat ((LPSTR) SourceB, (LPSTR) Buffer);

                        if (_fstrlen ((LPSTR) Buffer) == 0)
                        {
                            LoadString (hInstIP, STR_MUST_ENTER_SOURCE_2, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                            hFocusWnd = GetDlgItem (hWnd, IDD_SOURCEB);
                            bErr = TRUE;
                        }
                        else
                            if (OpenFile ((LPSTR)SourceB, (LPOFSTRUCT)&Of, OF_EXIST) <= 0)
                            {
                                LoadString (hInstIP, STR_SOURCE_2_EXIST, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                                hFocusWnd = GetDlgItem (hWnd, IDD_SOURCEB);
                                bErr = TRUE;
                            }
                            else
                            {
                                int hFile;
    
                                hFile = OpenFile ((LPSTR) SourceB, (LPOFSTRUCT)&Of, OF_READWRITE);
    
                                ReadCPIHdr (hFile, (LPCPIFILESPEC) &CPIFileSpec2, (LPCPIIMAGESPEC) &CPIImageSpec2);
                                
                                if (CPIImageSpec2.ImgType != CPIFMT_RGB)
                                {
                                    LoadString (hInstIP, STR_SOURCE_2_NOT_24, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                                    hFocusWnd = GetDlgItem (hWnd, IDD_SOURCEB);
                                    bErr = TRUE;
                                }
                                _lclose (hFile);
                            }

                    }

                    /*  Test destination image name  */

                    if (! bErr)
                    {
                        GetDlgItemText (hWnd, IDD_MERGED, (LPSTR) Dest, sizeof (Dest));
                        SeparateFile ((LPSTR) pStringBuf, (LPSTR) Buffer, (LPSTR) Dest);

                        _fstrcpy ((LPSTR) Dest, (LPSTR) DestPath);
                        if (Dest [_fstrlen ((LPSTR) Dest) - 1] != '\\')
                            _fstrcat ((LPSTR) Dest, "\\");
                        _fstrcat ((LPSTR) Dest, (LPSTR) Buffer);
    
                        if (_fstrlen ((LPSTR) Buffer) == 0)
                        {
                            LoadString (hInstIP, STR_MUST_ENTER_MERGED, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                            hFocusWnd = GetDlgItem (hWnd, IDD_MERGED);
                            bErr = TRUE;
                        }
                    }


                    /*  Test all the other mess the user might have entered  */

    
                    #define ORIGIN_NEG  2
                    #define EXTENT_NEG  3
                    #define ORG_EXT_IMAGE_SIZE 4
                    #define MERGE_OUT_RANGE_LOG 5
                    #define MERGE_OUT_RANGE_PHYS 6

                    if (! bErr)
                    {

                        /*  Many of these values have assumed entirely different 
                            meanings because of last-minute changes in the 
                            interface of the dialog.  D. Ison  2-1-91  */

                        nX1     = GetDlgItemInt (hWnd, IDD_X1, (LPINT) &nTestval, TRUE);
                        nY1     = GetDlgItemInt (hWnd, IDD_Y1, (LPINT) &nTestval, TRUE);
                        nX2     = GetDlgItemInt (hWnd, IDD_X2, (LPINT) &nTestval, TRUE);
                        nY2     = GetDlgItemInt (hWnd, IDD_Y2, (LPINT) &nTestval, TRUE);
                        nX3     = GetDlgItemInt (hWnd, IDD_X3, (LPINT) &nTestval, TRUE);
                        nY3     = GetDlgItemInt (hWnd, IDD_Y3, (LPINT) &nTestval, TRUE);

                        /*  Merge point must be "in" desired rectangle  */
                        /*  and in physical size of image A */

                        if (! bErr)
                        {
                            if (nX3 > CPIImageSpec1.X_Length)
                            {
                                bErr = MERGE_OUT_RANGE_PHYS;
                                hFocusWnd = GetDlgItem (hWnd, IDD_X3);
                            }
                            if (nY3 > CPIImageSpec1.Y_Length)
                            {
                                bErr = MERGE_OUT_RANGE_PHYS;
                                hFocusWnd = GetDlgItem (hWnd, IDD_Y3);
                            }
                        }
    
                        switch (bErr)   // Look for an error
                        {
                            case ORIGIN_NEG:
                                LoadString (hInstIP, STR_ORG_NOT_NEGATIVE, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                            break;
                            case EXTENT_NEG:
                                LoadString (hInstIP, STR_LENGTH_NOT_NEGATIVE, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                            break;
                            case ORG_EXT_IMAGE_SIZE:
                                LoadString (hInstIP, STR_ORG_LENGTH_TOO_BIG, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                            break;
                            case MERGE_OUT_RANGE_LOG:
                                LoadString (hInstIP, STR_MERGE_POINT_LOG , (LPSTR) pStringBuf, MAXSTRINGSIZE);
                            break;
                            case MERGE_OUT_RANGE_PHYS:
                                LoadString (hInstIP, STR_MERGE_POINT_PHYS , (LPSTR) pStringBuf, MAXSTRINGSIZE);
                            break;
                        }
                    }    
    
                    if (bErr)
                    {
                        MessageBeep (NULL);
                        MessageBox (hWnd, pStringBuf, pStringBuf2, MB_OK | MB_ICONQUESTION);
                    }
                    else
                        if (OpenFile ((LPSTR) Dest, (LPOFSTRUCT)&Of, OF_EXIST) > 0)
                        {
                            /*  File Exists, overwrite ? */

                            LoadString (hInstIP, STR_IMAGE_EXISTS_CONFIRM, (LPSTR) pStringBuf, MAXSTRINGSIZE);
                            LoadString (hInstIP, STR_MERGE, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
                            if (MessageBox (hWnd, pStringBuf, pStringBuf2, MB_OKCANCEL | MB_ICONQUESTION) == IDCANCEL)
                                bErr = TRUE;
                        }       

                    LocalUnlock (hStringBuf);

                    if (! bErr)
                    {
                        MergeStruct.nX1     = 0;
                        MergeStruct.nY1     = 0;
                        MergeStruct.nExtX1  = X_Length1;
                        MergeStruct.nExtY1  = Y_Length1;
                        MergeStruct.nX2     = 0;
                        MergeStruct.nY2     = 0;

                        MergeStruct.nPtX1   = nX1;      // Change of interpretation 2/1/91
                        MergeStruct.nPtY1   = nY1; 
                        MergeStruct.nPtX2   = nX2; 
                        MergeStruct.nPtY2   = nY2;

                        MergeStruct.nExtX2  = X_Length2;
                        MergeStruct.nExtY2  = Y_Length2;
                        
                        MergeStruct.nX3     = nX3;
                        MergeStruct.nY3     = nY3;
                        MergeStruct.wMode   = (WORD) TmpVals [0];
                        #ifdef RUBBER
                        MergeStruct.wView   = (WORD) TmpVals [1];
                        #endif

                        _fstrcpy ((LPSTR) MergeStruct.SourcePathA, (LPSTR) SourceA);
                        _fstrcpy ((LPSTR) MergeStruct.SourcePathB, (LPSTR) SourceB);
                        _fstrcpy ((LPSTR) MergeStruct.DestPath, (LPSTR) Dest);

                        _fstrcpy ((LPSTR) MergeStruct.FilenameA, (LPSTR) FilenameA);
                        _fstrcpy ((LPSTR) MergeStruct.FilenameB, (LPSTR) FilenameB);
                        _fstrcpy ((LPSTR) MergeStruct.DestFilename, (LPSTR) DestFilename);

                        bIsMerge    = 3;    // "Return" value from this dialog
                        DestroyWindow (hWndMerge);
                        if (image_active)
                        {
                            DestroyWindow (hWndDisplay);
                            image_active = FALSE;
                        }
                        hWndMerge = 0;
                    }
                    else
                        SetFocus (hFocusWnd);

                }
                break;


                case IDCANCEL:
                    bIsMerge = FALSE;           // Flag merge dialog cancel
                    bMergeActive = FALSE;       // Ignore past settings...
                    DestroyWindow (hWndMerge);
                    if (image_active)
                    {
                        DestroyWindow (hWndDisplay);
                        image_active = FALSE;
                    }
                    hWndMerge = 0;
                    break;


            }
            break;    

        case WM_RESETVIEWMODE:
            if (! bIsMerge)     // If merge active, ignore
            {
                wFullView = (TmpVals [3] & 1);  // Only look at low bits
                TmpVals [3] = 0;
            }
            break;

        case WM_SETMERGEPOINT:
        {
            RECT    VRect;
            POINT   Point;
            int     nTmpX;
            int     nTmpY;

            Point.x = LOWORD (lParam);
            Point.y = HIWORD (lParam);

            if (bIsMerge == 1)      // Image 1 up
            {
                /*  Validate requested merge point.  (1) Must be in selected area  (2) X merge point can NOT be negative */

                #ifdef RUBBER
                VRect.top       = GetDlgItemInt (hWnd, IDD_Y1, (LPINT) &nTestval, TRUE);
                VRect.left      = GetDlgItemInt (hWnd, IDD_Y1, (LPINT) &nTestval, TRUE);
                VRect.bottom    = VRect.top  + GetDlgItemInt (hWnd, IDD_EXTY1, (LPINT) &nTestval, TRUE);
                VRect.right     = VRect.left + GetDlgItemInt (hWnd, IDD_EXTX1, (LPINT) &nTestval, TRUE);
                #else
                VRect.top       = 0;
                VRect.left      = 0;
                VRect.bottom    = Y_Length1;
                VRect.right     = X_Length1;
                #endif

                if (PtInRect ((LPRECT) &VRect, Point))
                {
                    nXMerge1 = Point.x;
                    nYMerge1 = Point.y;
                    SetDlgItemInt (hWnd, IDD_X1, nXMerge1, TRUE);
                    SetDlgItemInt (hWnd, IDD_Y1, nYMerge1, TRUE);

                    if (IsIconic (hWnd))
                        ShowWindow (hWnd, SW_SHOWNORMAL);
                }
                else
                    MessageBeep (NULL);
            }
            else
            {
                int nX3, nY3;
                BOOL bIsValid = FALSE;

                #ifdef RUBBER
                VRect.top       = GetDlgItemInt (hWnd, IDD_Y2, (LPINT) &nTestval, TRUE);
                VRect.left      = GetDlgItemInt (hWnd, IDD_X2, (LPINT) &nTestval, TRUE);
                VRect.bottom    = VRect.top  + GetDlgItemInt (hWnd, IDD_EXTY2, (LPINT) &nTestval, TRUE);
                VRect.right     = VRect.left + GetDlgItemInt (hWnd, IDD_EXTX2, (LPINT) &nTestval, TRUE);
                #else
                VRect.top       = 0;
                VRect.left      = 0;
                VRect.bottom    = Y_Length2;
                VRect.right     = X_Length2;
                #endif

                /*  Validate requested merge point.  (1) Must be in selected area  (2) X merge point can NOT be negative */

                if (PtInRect ((LPRECT) &VRect, Point))
                {

                    /*  Now have both points, can set up translation 
                        and put in dialog box  */

                    nTmpX = Point.x;
                    nTmpY = Point.y;

                    nX3 = (nXMerge1 - nTmpX);
                    nY3 = (nYMerge1 - nTmpY);

                    if (nX3 > 0)
                    {
                        nXMerge2 = nTmpX;
                        nYMerge2 = nTmpY;

                        SetDlgItemInt (hWnd, IDD_X2, nXMerge2, TRUE);
                        SetDlgItemInt (hWnd, IDD_Y2, nYMerge2, TRUE);
                        SetDlgItemInt (hWnd, IDD_X3, nX3, TRUE);
                        SetDlgItemInt (hWnd, IDD_Y3, nY3, TRUE);
                        bIsValid = TRUE;
                        if (IsIconic (hWnd))
                            ShowWindow (hWnd, SW_SHOWNORMAL);
                    }
                }
                if (! bIsValid)
                   MessageBeep (NULL);
            }
        }
        break;

        #ifdef NOMORE
        case WM_SETMERGERECT:
        {
            LPRECT lpRect = (LPRECT) lParam;
            ShowWindow (hWndMerge, SW_SHOWNORMAL);
            if (bIsMerge == 1)      // Image 1 up
            {
                SetDlgItemInt (hWnd, IDD_X1, lpRect -> left, TRUE);
                SetDlgItemInt (hWnd, IDD_Y1, lpRect -> top  , TRUE);
                SetDlgItemInt (hWnd, IDD_EXTX1, (lpRect -> right  - lpRect -> left), TRUE);
                SetDlgItemInt (hWnd, IDD_EXTY1, (lpRect -> bottom - lpRect -> top), TRUE);
            }
            else                    // Image 2 up
            {
                SetDlgItemInt (hWnd, IDD_X2, lpRect -> left, TRUE);
                SetDlgItemInt (hWnd, IDD_Y2, lpRect -> top  , TRUE);
                SetDlgItemInt (hWnd, IDD_EXTX2, (lpRect -> right  - lpRect -> left), TRUE);
                SetDlgItemInt (hWnd, IDD_EXTY2, (lpRect -> bottom - lpRect -> top), TRUE);
            }
        }
        break;
        #endif
        case WM_VALIDATEMERGE:      // May have to use Globals, instead!
        {
            if (TmpVals [1] == MERGE_POINT)
                if (bIsMerge == 2)  // Image 2 up
                {
                    int nTmpX1;
                    int nTmpY1;

                    nTmpX1 = LOWORD (lParam);
                    nTmpY1 = HIWORD (lParam);

                    if ((nXMerge1 - nXMerge2) < 0)
                        return (FALSE);
                }

            return (TRUE);      // Always by default;
        }
        break;

        case WM_SHOWTEXT:
        {
            char Buffer [128];
            OFSTRUCT    Of;
            CPIFILESPEC  CPIFileSpec;
            CPIIMAGESPEC CPIImageSpec;
            int         hFile;

            if (SendDlgItemMessage (hWnd, wParam, CB_GETLBTEXT, 0, (DWORD) (LPSTR) Buffer) != CB_ERR)
                SetDlgItemText (hWnd, wParam, (LPSTR) Buffer);
            SendDlgItemMessage (hWnd, wParam, CB_SETEDITSEL, NULL, MAKELONG (0, 128));


            /*  This stuff was put in after the combos were changed to CBS_SORT type because the
                X_Length and Y_Length values were not being set properly anymore.  D. Ison  6/91 */

            hFile = OpenFile ((LPSTR) Buffer, (LPOFSTRUCT)&Of, OF_READWRITE);

            if (hFile > 0)
            {                   
                ReadCPIHdr (hFile, (LPCPIFILESPEC) &CPIFileSpec, (LPCPIIMAGESPEC) &CPIImageSpec);

                /*  Set default dialog values  */

                if (wParam == IDD_SOURCEA)
                {
                    SetDlgItemInt (hWnd, IDD_X1, CPIImageSpec.X_Length, TRUE);
                    SetDlgItemInt (hWnd, IDD_Y1, 0, TRUE);
                    SetDlgItemInt (hWnd, IDD_X3, CPIImageSpec.X_Length, TRUE);
                    SetDlgItemInt (hWnd, IDD_Y3, 0, TRUE);
                        
                    X_Length1 = CPIImageSpec.X_Length;
                    Y_Length1 = CPIImageSpec.Y_Length;
                }
                else
                {
                    SetDlgItemInt (hWnd, IDD_X2, CPIImageSpec.X_Length, TRUE);
                    SetDlgItemInt (hWnd, IDD_Y2, 0, TRUE);
                        
                    X_Length2 = CPIImageSpec.X_Length;
                    Y_Length2 = CPIImageSpec.Y_Length;
                }
                _lclose (hFile);

            }
            else
                if (wParam == IDD_SOURCEA)
                {
                    SetDlgItemInt (hWnd, IDD_X1, 0, TRUE);
                    SetDlgItemInt (hWnd, IDD_Y1, 0, TRUE);
                    SetDlgItemInt (hWnd, IDD_X3, 0, TRUE);
                    SetDlgItemInt (hWnd, IDD_Y3, 0, TRUE);
                }
                else
                {
                    SetDlgItemInt (hWnd, IDD_X2, 0, TRUE);
                    SetDlgItemInt (hWnd, IDD_Y2, 0, TRUE);
                }

        }

        break;


        default:
            return (FALSE);
    }
    return (TRUE);
}



void FAR InitMerge (hWnd)
HWND hWnd;
{
    HANDLE  hWndControl;
    char    Buffer [128];
    PSTR            pStringBuf;
    PSTR            pStringBuf2;
    int             hFile;
    OFSTRUCT        Of;
    CPIFILESPEC     CPIFileSpec;
    CPIIMAGESPEC    CPIImageSpec;

    pStringBuf  = LocalLock (hStringBuf);
    pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

    CheckRadioButton (hWnd, IDD_UNION, IDD_INTERSECT, IDD_UNION);
    TmpVals [0] = UNION;

    #ifdef RUBBER
    CheckRadioButton (hWnd, IDD_DRAW_AREA, IDD_MERGE_POINT, IDD_MERGE_POINT);
    #endif
    TmpVals [1] = (BYTE) MERGE_POINT;

    getcwd (Buffer, sizeof (Buffer));
    _fstrcat ((LPSTR) Buffer, (LPSTR) "\\");
    _fstrcat ((LPSTR) Buffer, (LPSTR) FileSpec);
    _fstrlwr ((LPSTR) Buffer);
    SeparateFile ((LPSTR)pStringBuf2, (LPSTR) pStringBuf, (LPSTR) Buffer);
    _fstrcat ((LPSTR) pStringBuf2, (LPSTR) FileSpec);

    DlgDirListComboBox (hWnd, pStringBuf2, IDD_SOURCEA, IDD_SOURCEA_PATH, 0x0000 | 0x0001 | 0x0010 | 0x0020 | 0x4000 | 0x8000);
    GetDlgItemText (hWnd, IDD_SOURCEA_PATH, (LPSTR) PathA, sizeof (PathA));

    hWndControl = GetDlgItem (hWnd, IDD_SOURCEA);
    if (SendMessage (hWndControl, CB_GETLBTEXT, 0, (DWORD) (LPSTR) Buffer) != CB_ERR)
    SetDlgItemText (hWnd, IDD_SOURCEA, (LPSTR) Buffer);

    hFile = OpenFile ((LPSTR) Buffer, (LPOFSTRUCT)&Of, OF_READWRITE);
    if (hFile > 0)
    {
        ReadCPIHdr (hFile, (LPCPIFILESPEC) &CPIFileSpec, (LPCPIIMAGESPEC) &CPIImageSpec);
                
        /*  Set default dialog values  */
        #ifdef RUBBER
        SetDlgItemInt (hWnd, IDD_EXTX1, CPIImageSpec.X_Length, TRUE);
        SetDlgItemInt (hWnd, IDD_EXTY1, CPIImageSpec.Y_Length, TRUE);
        #endif
        SetDlgItemInt (hWnd, IDD_X3, CPIImageSpec.X_Length, TRUE);
        SetDlgItemInt (hWnd, IDD_Y3, 0, TRUE);
                
        X_Length1 = CPIImageSpec.X_Length;
        Y_Length1 = CPIImageSpec.Y_Length;
        nXMerge1  = CPIImageSpec.X_Length;
        nYMerge1  = 0;
    
        _lclose (hFile);
    }
                
                
    /*  Set up source B  */
                
    DlgDirListComboBox (hWnd, pStringBuf2, IDD_SOURCEB, IDD_SOURCEB_PATH, 0x0000 | 0x0001 | 0x0010 | 0x0020 | 0x4000 | 0x8000);
    GetDlgItemText (hWnd, IDD_SOURCEB_PATH, (LPSTR) PathB, sizeof (PathB));
                
    hWndControl = GetDlgItem (hWnd, IDD_SOURCEA);
    if (SendMessage (hWndControl, CB_GETLBTEXT, 0, (DWORD) (LPSTR) Buffer) != CB_ERR)
    SetDlgItemText (hWnd, IDD_SOURCEB, (LPSTR) Buffer);
                
    hFile = OpenFile ((LPSTR) Buffer, (LPOFSTRUCT)&Of, OF_READWRITE);
    if (hFile > 0)
    {
        ReadCPIHdr (hFile, (LPCPIFILESPEC) &CPIFileSpec, (LPCPIIMAGESPEC) &CPIImageSpec);
                
        /*  Set default dialog values  */
    
        #ifdef RUBBER
        SetDlgItemInt (hWnd, IDD_EXTX2, CPIImageSpec.X_Length, TRUE);
        SetDlgItemInt (hWnd, IDD_EXTY2, CPIImageSpec.Y_Length, TRUE);
        #endif
        X_Length2 = CPIImageSpec.X_Length;
        Y_Length2 = CPIImageSpec.Y_Length;
        nXMerge2  = 0;
        nYMerge2  = 0;
    
        _lclose (hFile);
    }
                
    DlgDirListComboBox (hWnd, pStringBuf2, IDD_MERGED, IDD_MERGED_PATH, 0x0000 | 0x0001 | 0x0010 | 0x0020 | 0x4000 | 0x8000);
    SetDlgItemText (hWnd, IDD_MERGED,  (LPSTR) "merged.cpi");

    getcwd (Buffer, 128);


    _fstrlwr ((LPSTR) Buffer);
    _fstrcpy ((LPSTR) PathA, (LPSTR) Buffer);
    _fstrcpy ((LPSTR) PathB, (LPSTR) Buffer);
    _fstrcpy ((LPSTR) DestPath, (LPSTR) Buffer);

    SetDlgItemText (hWnd, IDD_SOURCEA_PATH,  (LPSTR) PathA);
    SetDlgItemText (hWnd, IDD_SOURCEB_PATH,  (LPSTR) PathB);
    SetDlgItemText (hWnd, IDD_MERGED_PATH,  (LPSTR) DestPath);

    /*  Init all remaining edit boxes */

    SetDlgItemInt (hWnd, IDD_X1, nXMerge1, TRUE);
    SetDlgItemInt (hWnd, IDD_Y1, nYMerge1, TRUE);
    SetDlgItemInt (hWnd, IDD_X2, nXMerge2, TRUE);
    SetDlgItemInt (hWnd, IDD_Y2, nYMerge2, TRUE);

    bMergeActive = TRUE;

    LocalUnlock (hStringBuf);
}


int FAR PASCAL CleanFileSpec (lpDest, lpSource)
LPSTR lpDest;
LPSTR lpSource;
{
    int     i;
    char    ch;
    int     nRetval = FILESPEC;         // Assume it is a file spec
    BOOL    bIsFileSpec = TRUE;
    BOOL    bIsDriveSpec = FALSE;

    i = 0;

    while (*lpSource != 0 && i < 128 && ! bIsDriveSpec)
    {
        ch = *lpSource++;

        if (ch != '[' && ch != ']')  // Skip over [ or ]
        {
            lpDest [i] = ch;
            i++;    
        }
        else
        {
            if (*lpSource == '-')           // Is a drive spec ?
                if (*(lpSource + 2) == '-') // Make sure
                {
                    lpDest [0] = *(lpSource + 1);
                    lpDest [1] = ':';
                    lpDest [2] = 0;
                    bIsFileSpec  = FALSE;
                    bIsDriveSpec = TRUE;
                }
            bIsFileSpec = FALSE;
        }
    }

    if (! bIsDriveSpec)
        lpDest [i] = 0;
                                
    if (bIsDriveSpec)
        nRetval = DRIVESPEC;
    else
        if (! bIsFileSpec)
            nRetval = DIRSPEC;


    return (nRetval);

}
