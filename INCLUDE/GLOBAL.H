/*--- Scalars  ---*/

HWND        hWndIP;
HWND        hWndDisplay;

HANDLE      hBhist;
HANDLE      hBitmapInfo;
HANDLE      hExpPalette;
HANDLE      hExpBitmapInfo;
HANDLE      hExpFileInfo;
HANDLE      hFileInfo;
HANDLE      hGlobalBufs;
HANDLE      hGlobalPalette;
HANDLE      hImpPalette;
HANDLE      hInstIP;
HANDLE      hStringBuf;                     // Global string message handle
HANDLE      hGlobalGamma;
HANDLE      hGlobalColor;
HANDLE      hGlobalCTable;                  // Global color-correct translate table
HANDLE      hGlobalImageBuf;
HANDLE      hInstCapture;
HANDLE      hAccelerate;
HANDLE      hGlobalString; 

HPALETTE    hOldPalette;
HPALETTE    hDispPalette;
HCURSOR     hHelpCursor;                    // Cursor displayed when in help mode*/

WORD        int_dither;                     /* internal temp copy of dither state for Dialogs etc.*/
WORD        int_pal;                        /* internal temp copy of palette state for Dialogs etc. */    
WORD        old_pal;                        /* temp copy of Pal Type for change in palette state tests */
WORD        wDevice;                        /* Device Type */
WORD        wDisplay;                       /* Display Type */
WORD        wDither;                        /* Dither Process type */
WORD        wDithQuality;
WORD        wExportType;                    /* the current Export File Type */
WORD        wHotKey;                        /* current HOTKEY */
WORD        wImportType;                    /* the current Import File Type */
WORD        wUserImportType;                /* What the user thinks is the import File Type */
WORD        wExportClass;                   /* the current Export Image Class */
WORD        wImportClass;                   /* the current Import Image Class */
WORD        wPal;                           /* Palette Process Type */
WORD        wRegion;                        /* current capture Region type */
WORD        wSaveType;                      /* Save Type */
WORD FAR *  lpwHist;                        
WORD        wNumberColors;
WORD        wOPRColors ;
WORD        wCurrDither;
WORD        wFullView  ;
            
            
int         hExportFile;
int         dPalType   ;
int         ePalType   ;
int         CompLevel  ;
int         CompFactor;                     /*  (Scaled) */
int         hHistFile;                      /* histogram file handle */
int         hImportFile;
            
            
BYTE        bCaptureOn  ;
BYTE        bDoDisplay  ;
BYTE        bDisplayQuality;
BYTE        bIsCapturing   ;
BYTE        bIsSaving      ;
BYTE        bIsTooBig      ;
BYTE        bNewDefault    ;
BYTE        bNewOpt        ;
BYTE        bOptDither     ;
BYTE        bResetProc     ;
BYTE        bRestoreDPal   ;
BYTE        image_active   ;
BYTE        bIsScanFile    ;
BYTE        bGetEPal;
BYTE        bLogo          ;
BYTE        bHelp          ;
BYTE        bIsPainting    ;
BYTE        bTrueColor     ;
BYTE        bInvertColors  ;
BYTE        bMemoryPaint   ;
BYTE        bSaveSettings  ;
BYTE        bFullScreen    ;
BYTE        bEditActive    ;
BYTE        bFiltersActive ;
BYTE        bDefDither     ;
BYTE        bDefPalette    ;
BYTE        bAutoConvert   ;
BYTE        bIsCurrTemp    ;        
BYTE        bIsUndoTemp    ;
BYTE        bClipOptions;                   
BYTE        bPrefChanged   ;
BYTE        bPollMessages  ;
BYTE        bImageModified ;
BYTE        bToolsActive   ;
//BYTE        bPadding;

BYTE        bAbandon    ;
BOOL        bIsFirstPaint  ;
BYTE FAR    *lpSmallhist;
BYTE FAR    *lpbHist; 


/*--- Non-Scalars, structures  ---*/

HANDLE      ImportFilterStruct[6][2];
HANDLE      ExportFilterStruct[6][2]; /* pointer to filter structures */
            
int         ExportFilters[6];
int         ImportFilters[6];

BYTE        rm[MAX_CMAP_SIZE + 2];
BYTE        gm[MAX_CMAP_SIZE + 2];
BYTE        bm[MAX_CMAP_SIZE + 2];
BYTE        TmpVals [16];                   // Temporary storage space for data.  Esp for dialog boxes.

char        HistFile[128];
char        szOpenFileName[128];
char        szSaveFileName[128];
char        szAppName[10];
char        szAppPath [MAXPATHSIZE];
char        szDefaultExts[5];
char        szOpenFile[128];
char        szSaveFile[128];
char        szTempPath[128];
char        szTempFile[128];
char        szTempExt[128];
char        szCurrFileName [128];           
char        szUndoFileName [128];
char        szImageName [13];
char        szOpenDir [128];
char        szSaveDir [128];
char        szLastSaveFormat [128];

OFSTRUCT    of_hist;                        
RECT        EditRect;
LPSTR       lpStrings [11];


/*  Dither patterns for ordered dither  */


BYTE Pattern2[8][8];
BYTE Pattern [16][16];

DWORD DefaultSysColorValues[NUMSYSCOLORS];
DWORD SysColorValues[NUMSYSCOLORS];
int   SysColorIndices[NUMSYSCOLORS];

/*  --- State matrices  ---*/


extern BYTE  DIS[2][4][4][5][6];
extern BYTE  ESF[MAXCLASS][MAXPAL][MAXDITH][MAXFILTER + 1];
extern BYTE  DPAL[2][4][4][5][1];
extern BYTE  EPAL[4][4][5][1];
extern BYTE  DDSM[MAXDISP][MAXCLASS][MAXPAL][MAXDITH - 1][2];
extern BYTE  SFM[4][XALLFORMATS];



