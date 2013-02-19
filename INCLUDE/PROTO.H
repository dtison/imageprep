/*
   MODULE:        proto.h

   DESCRIPTION:   Prototypes and structure definitions for ImgPrep
*/

void  ReportError (int, LPSTR);

long FAR PASCAL DisplayWndProc (HWND, unsigned, WORD, LONG);
long FAR PASCAL PreviewWndProc (HWND, unsigned, WORD, LONG);

void  SetState (int, int);

int   UpdateMenus (HWND, int, int);
void  CheckMenuCmd (HWND, WORD, BOOL);
void  menu_init(void);       

//BOOL FAR PASCAL DoDisplayStateDlg (HWND, unsigned, WORD, LONG);
  int  FAR PASCAL ProcSpecDlgProc (HWND, unsigned, WORD, LONG);
int SetDisplayDlg (HWND);
void SetRadios (HWND, WORD, WORD);
int FreePrevDispPalMem (WORD, WORD);
void RadioGray (HWND, int);
void RadioAvailable (HWND, int);

BOOL FAR PASCAL ImportImgDlg  (HWND, unsigned, WORD, LONG);
BOOL FAR PASCAL DithOptionsDlg(HWND, unsigned, WORD, LONG);
BOOL FAR PASCAL ExportImgDlg  (HWND, unsigned, WORD, LONG);
BOOL FAR PASCAL DisplayImgDlg (HWND, unsigned, WORD, LONG);
BOOL FAR PASCAL SaveOptDlg    (HWND, unsigned, WORD, LONG);  

BOOL FAR PASCAL AboutDlgProc (HWND, unsigned, WORD, LONG);
BOOL FAR PASCAL InfoDlgProc (HWND, unsigned, WORD, LONG);
BOOL FAR PASCAL OPRDlgProc  (HWND, unsigned, WORD, LONG);
BOOL FAR PASCAL OICInfoDlg  (HWND, unsigned, WORD, LONG);
void FAR SetupDispWnd       (HWND, HANDLE);
void FAR PASCAL GetNameFromPath (LPSTR, LPSTR);


DWORD FAR PASCAL MergeDlgProc (HWND, unsigned, WORD, LONG);

/******************************* IFORMAT.C **********************************/

int  IdentFormat   (int, LPSTR);
int  CheckByExt    (int, LPSTR);
int  CheckAllTypes (int);
LPSTR GetExtension (LPSTR);

#ifdef NEVER
/************************** DLGOPEN.C ***************************************/

int DlgOpenFile (HWND, LPSTR, DWORD, LPSTR, LPSTR, WORD FAR *);
int FAR PASCAL DlgfnOpen(HWND, unsigned, WORD, LONG);
void DlgCheckOkEnable   (HWND, int, unsigned);
void AddExt             (LPSTR, LPSTR);
BOOL FSearchSpec        (LPSTR);
LPSTR  FillListBox      (HWND, LPSTR);
void ChangeDefExt       (LPSTR, LPSTR);
BOOL CheckFileName      (HWND, LPSTR, LPSTR);
#endif

/***************************** IMPORT.C *************************************/

int   ImportFile  (void);
HWND  CreateDisplayWindow (HANDLE);  
int   InitDispInfo  (HANDLE);

/*************************** INVESTIGATE ************************************/
int   OpenImage (int, LONG);
int   DoSave (int);
int 	SetImportStates (void);

BOOL FAR PASCAL SetHotKeyDlg (HWND, unsigned, WORD, LONG);

/******************************** PAINT.C ***********************************/

int FAR PASCAL PaintPicture (HWND);

int NEAR PASCAL CreateDisplayBitmap (LPSTR FAR *, LPSTR FAR *, LPFILEINFO, LPBITMAPINFO);

HANDLE GenPal (int, LPFILEINFO, LPBITMAPINFO, int);

int SetExportStates (void);

int GenDefaultPal   (LPSTR, int);



/**************************** SaveDlg.c *************************************/

HANDLE FAR PASCAL SaveDlg (HWND, unsigned, WORD, LONG);
int   SetSaveDlg (HWND);
int   SetSaveRadios (HWND, WORD);
                    
/******************************** EXPORT.C **********************************/

int 	ExportFile (int);


FARPROC GetRCD  (WORD);
FARPROC GetWCD  (WORD);
FARPROC GetIHdr (WORD);
FARPROC GetRHdr (WORD);
FARPROC GetFHdr (WORD);

BOOL FAR PASCAL SaveReportProc (HWND, unsigned, WORD, LONG);
BOOL FAR PASCAL SaveStatusProc (HWND, unsigned, WORD, LONG);
BOOL FAR PASCAL SquashProc     (HWND, unsigned, WORD, LONG);
BOOL FAR PASCAL UnSquashProc   (HWND, unsigned, WORD, LONG);
BOOL FAR PASCAL CompDlg        (HWND, unsigned, WORD, LONG);
BOOL FAR PASCAL CompDlgTest    (HWND, unsigned, WORD, LONG);

int  InitImageFileFPB          (void);
int  Get_FPB_ExpFileState      (HANDLE, LPSTR);
int  Get_FPB_ImpFileState      (HANDLE, LPSTR);
int  MakeFileSpec       (HANDLE);
int  InitComboSaveType  (HANDLE);
int FreeComboSaveType   (HANDLE);

FARPROC GetFilterProc   (int);

/*
**  Open/Save
*/
int  FAR PASCAL DU_WndProc (HWND, unsigned, WORD, LONG);

#ifdef NEVER
BOOL IsWildCard         ( LPSTR );
BOOL IsPath             ( LPSTR );
BOOL DoesFileExist      ( LPSTR, LPSTR );
BOOL DoesPathExist      ( LPSTR, LPSTR );

int ProcessPathEntry    ( LPSTR, LPSTR );
int ProcessFileEntry    ( LPSTR );
int ProcessExtensionEntry( LPSTR );
int PathFileExt         ( LPSTR );
#endif

int InitExpFileInfo (LPFILEINFO, LPFILEINFO);
int FreeFilters (HANDLE, int);

int FAR PASCAL OpenDlgProc (HWND, unsigned, WORD, LONG);
int FAR PASCAL SaveDlgProc (HWND, unsigned, WORD, LONG);


int InitFileTypeCombo (HWND);
WORD SetFileType (LPSTR);

int ChangeFileExt (LPSTR, LPSTR);
int ChangeFile (LPSTR, LPSTR);
int ProcessFileType (HWND);
int ProcessExtension (HWND, LPSTR);
int ProcessFile (HWND, LPSTR);

HWND LaunchSaveReport (HWND);
int SR_Percent (HWND, WORD, WORD);
int RemoveSaveReport (HWND);


BOOL  IsDiskSpaceAvailable (LPSTR lpszPath, DWORD dwBytesNeeded);

BOOL FAR PASCAL RealizeSystemPalette (HDC, HPALETTE, LPINT, LPDWORD, WORD);


void FAR PASCAL UpdateCorrectionTable (HANDLE);

// Gamma, RGB & BC implementation

void FAR PASCAL ColorCorrectRGB (LPSTR, LPSTR, LPFILEINFO);
void FAR PASCAL ColorCorrectPalette (RGBQUAD FAR *);
void FAR RGBFilter (LPSTR, LPSTR, RGBQUAD FAR *, WORD, WORD, WORD);  // Internal assembler function


void FAR PASCAL Abandon (void);  // Abandon callback
int  FAR ValidateImage (LPFILEINFO);

int FAR PASCAL PreviewPicture (HWND);


LPSTR FAR GlobalLockDiscardable (HANDLE);

int   FAR ConfirmExit (HWND);
DWORD FAR PASCAL PrefDlgProc (HWND, unsigned, WORD, LONG);

int FAR CopyClipboard (HWND, BOOL);
int FAR CopyPrint     (HWND, BOOL);

void FAR  ConformMenu   (WORD);
void FAR RGBClassMenu  (HWND);
void FAR CMAPClassMenu (HWND);
void FAR GRAYClassMenu (HWND);
void FAR MONOClassMenu (HWND);


int  FAR SelPalette (HWND, WORD);
int  FAR SelDither  (HWND, WORD);
int  FAR SelGray16  (HWND, WORD);

void FAR CleanFiles (void);

LONG FAR MenuCommand (HWND,  unsigned, WORD, LONG);

