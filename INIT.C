
#include <windows.h>
#include <imgprep.h>
#include <resource.h>
#include <proto.h>
#include <startup.h>

/*--- Scalars  ---*/

HWND        hWndIP          = (HWND)0;      /* app window handle          */
HWND        hWndDisplay     = (HWND)0;      /* display window handle      */

HANDLE      hBhist          = (HANDLE)0;    /* Histogram buffer           */
HANDLE      hBitmapInfo     = (HANDLE)0;    /* Bitmap info struct         */
HANDLE      hExpPalette     = (HANDLE)0;    /* export palette             */
HANDLE      hExpBitmapInfo  = (HANDLE)0;    /* export bitmapinfo          */
HANDLE      hExpFileInfo    = (HANDLE)0;    /* export fileinfo            */
HANDLE      hFileInfo       = (HANDLE)0;    /* file info structure        */
HANDLE      hGlobalBufs     = (HANDLE)0;    /* image processing buffers   */
HANDLE      hGlobalPalette  = (HANDLE)0;    /* Saves images original palette */
HANDLE      hImpPalette     = (HANDLE)0; 
HANDLE      hInstIP         = (HANDLE)0;    /* application instance       */
HANDLE      hStringBuf;                     // Global string message handle
HANDLE      hGlobalGamma;
HANDLE      hGlobalColor;
HANDLE      hGlobalCTable;                  // Global color-correct translate table
HANDLE      hGlobalImageBuf;
HANDLE      hInstCapture;
HANDLE      hAccelerate;
HANDLE      hGlobalString;                  /* For strings used in PROCSPEC dialog or wherever else needed  */
            
HPALETTE    hOldPalette;
HPALETTE    hDispPalette    = (HPALETTE)0;  /* display palette            */
HCURSOR     hHelpCursor;                    // Cursor displayed when in help mode*/

WORD        int_dither;                     /* internal temp copy of dither state for Dialogs etc.*/
WORD        int_pal;                        /* internal temp copy of palette state for Dialogs etc. */    
WORD        old_pal;                        /* temp copy of Pal Type for change in palette state tests */
WORD        wDevice;                        /* Device Type */
WORD        wDisplay;                       /* Display Type */
WORD        wDither;                        /* Dither Process type */
WORD        wDithQuality = 1;               /* Quality or Speed FS Dither indication */
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
WORD        wOPRColors  = 256;
WORD        wCurrDither = DITHER_FS_COLOR;  // Currently selected dither filter  (maybe separate for color/gray later)
WORD        wFullView   = FALSE;            // Default w/scrollbars enabled
            
            
int         hExportFile = (int)0;           /* current Export file handle */
int         dPalType    = 0;                /* for use with GenPal        */
int         ePalType    = 0;                /* for use with GenPal        */
int         CompLevel   = 55;               /* default = 79 EFI scale */
int         CompFactor;                     /*  (Scaled) */
int         hHistFile;                      /* histogram file handle */
int         hImportFile     = (int)0;       /* Import file handle         */
            
            
BYTE        bCaptureOn      = FALSE;
BYTE        bDoDisplay      = TRUE;
BYTE        bDisplayQuality = FALSE;
BYTE        bIsCapturing    = FALSE;
BYTE        bIsSaving       = FALSE;
BYTE        bIsTooBig       = FALSE;
BYTE        bNewDefault     = TRUE;
BYTE        bNewOpt         = TRUE;
BYTE        bOptDither      = FALSE;
BYTE        bResetProc      = TRUE; 
BYTE        bRestoreDPal    = FALSE;
BYTE        image_active    = FALSE;
BYTE        bIsScanFile     = FALSE;
BYTE        bGetEPal;
BYTE        bLogo           = FALSE;
BYTE        bHelp           = FALSE;        // Help mode flag; TRUE = "ON"
BYTE        bIsPainting     = FALSE;        // Indicates if in middle of processing WM_PAINT message
BYTE        bTrueColor      = FALSE;
BYTE        bInvertColors   = FALSE;
BYTE        bMemoryPaint    = FALSE;
BYTE        bSaveSettings   = FALSE;
BYTE        bFullScreen     = FALSE;
BYTE        bEditActive     = FALSE;
BYTE        bFiltersActive  = FALSE;        // For import filters (Stabilizes interruptable repaints)
BYTE        bDefDither;
BYTE        bDefPalette;
BYTE        bAutoConvert    = TRUE;
BYTE        bIsCurrTemp     = FALSE;        // Is Current image a temp file ?
BYTE        bIsUndoTemp     = FALSE;        // Is Undo image a temp file ?
BYTE        bClipOptions;                   // Flags for what formats we cut/copy as
BYTE        bPrefChanged    = FALSE;
BYTE        bPollMessages   = TRUE;         // Poll for messages during repaints, etc or not
BYTE        bImageModified  = FALSE;        // Flags the confirm save box
BYTE        bToolsActive    = FALSE;
//BYTE        bPadding;

BYTE        bAbandon        = FALSE;
BOOL        bIsFirstPaint   = 0x8001;
BYTE FAR    *lpSmallhist;
BYTE FAR    *lpbHist; 


/*--- Non-Scalars, structures  ---*/

HANDLE      ImportFilterStruct[6][2];
HANDLE      ExportFilterStruct[6][2]; /* pointer to filter structures */
            
int         ExportFilters[6];
int         ImportFilters[6] = { 0,0,0,0,0,0 };

BYTE        rm[MAX_CMAP_SIZE + 2];
BYTE        gm[MAX_CMAP_SIZE + 2];
BYTE        bm[MAX_CMAP_SIZE + 2];
BYTE        TmpVals [16];                   // Temporary storage space for data.  Esp for dialog boxes.

char        HistFile[128];
char        szOpenFileName[128];
char        szSaveFileName[128];
char        szAppName[10];
char        szAppPath [MAXPATHSIZE];
char        szDefaultExts[5] = "*.*";       // WAS 128 (saved 123)
char        szOpenFile[128];
char        szSaveFile[128];
char        szTempPath[128];
char        szTempFile[128];
char        szTempExt[128];
char        szCurrFileName [128];           
char        szUndoFileName [128];           // For undo capability
char        szImageName [13];               // For banner and save as dialog
char        szOpenDir [128];                // Saves last open directory
char        szSaveDir [128];                // Saves last save directory
char        szLastSaveFormat [128];         // Saves last saved format (string)

OFSTRUCT    of_hist;                        /* Information from OpenFile() histogram */
RECT        EditRect;
LPSTR       lpStrings [11];

/*  Dither patterns for ordered dither  */

#define BAYER
#ifdef BAYER
BYTE Pattern2[8][8] = { 
             0, 32,  8, 40,  2, 34, 10, 42,
            48, 16, 56, 24, 50, 18, 58, 26,
            12, 44,  4, 36, 14, 46,  6, 38,
            60, 28, 52, 20, 62, 30, 54, 22,
             3, 35, 11, 43,  1, 33,  9, 41,
            51, 19, 59, 27, 49, 17, 57, 25,
            15, 27,  7, 39, 13, 45,  5, 37,
            63, 31, 55, 23, 61, 29, 53, 21};
#else  

// --- Test spiral

BYTE Pattern2[8][8] = { 
      35, 12, 31, 23, 15, 33, 41, 49,
      8, 32, 54, 46, 38, 10,18, 26,
      17, 40, 62, 60, 52, 29, 7,  5,
      25, 48, 58, 56, 44, 21, 1,  3,
      14, 34, 42, 50, 36, 13, 30, 22,
      37, 11, 19, 27,  9, 32, 53, 45,
      51, 28,  6,  4, 16, 39, 61, 59,
      43, 20,  1,  2, 24, 47, 57, 55};

#endif

BYTE Pattern [16][16] = 
{
       0, 191,  47, 239,  11, 203,  59, 251,   2, 194,  50, 242,  14, 206,  62, 254,
     127,  63, 175, 111, 139,  75, 187, 123, 130,  66, 178, 114, 142,  78, 190, 126,
      31, 223,  15, 207,  43, 235,  27, 219,  34, 226,  18, 210,  46, 238,  30, 222,
     159,  95, 143,  79, 171, 107, 155,  91, 162,  98, 146,  82, 174, 110, 158,  94,
       7, 199,  55, 247,   3, 195,  51, 243,  10, 202,  58, 250,   6, 198,  54, 246,
     135,  71, 183, 119, 131,  67, 179, 115, 138,  74, 186, 122, 134,  70, 182, 118,
      39, 231,  23, 215,  35, 227,  19, 211,  42, 234,  26, 218,  38, 230,  22, 214,
     167, 103, 151,  87, 163,  99, 147,  83, 170, 106, 154,  90, 166, 102, 150,  86,
       1, 193,  49, 241,  13, 205,  61, 253,   0, 192,  48, 240,  12, 204,  60, 252,
     129,  65, 177, 113, 141,  77, 189, 125, 128,  64, 176, 112, 140,  76, 188, 124,
      33, 225,  17, 209,  45, 237,  29, 221,  32, 224,  16, 208,  44, 236,  28, 220,
     161,  97, 145,  81, 173, 109, 157,  93, 160,  96, 144,  80, 172, 108, 156,  92,
       9, 201,  57, 249,   5, 197,  53, 245,   8, 200,  56, 248,   4, 196,  52, 244,
     137,  73, 185, 121, 133,  69, 181, 117, 136,  72, 184, 120, 132,  68, 180, 116,
      41, 233,  25, 217,  37, 229,  21, 213,  40, 232,  24, 216,  36, 228,  20, 212,
     169, 105, 153,  89, 165, 101, 149,  85, 168, 104, 152,  88, 164, 100, 148,  84

}; 


DWORD DefaultSysColorValues[NUMSYSCOLORS];
DWORD SysColorValues[NUMSYSCOLORS]=
  { 0x00000000,                   /* COLOR_SCROLLBAR        0   */
    0x00FFFFFF,                   /* COLOR_BACKGROUND       1   */
    0x00000000,                   /* COLOR_ACTIVECAPTION    2   */
    0x00FFFFFF,                   /* COLOR_INACTIVECAPTION  3   */
    0x00FFFFFF,                   /* COLOR_MENU             4   */
    0x00FFFFFF,                   /* COLOR_WINDOW           5   */
    0x00000000,                   /* COLOR_WINDOWFRAME      6   */
    0x00000000,                   /* COLOR_MENUTEXT         7   */
    0x00000000,                   /* COLOR_WINDOWTEXT       8   */
    0x00FFFFFF,                   /* COLOR_CAPTIONTEXT      9   */
    0x00000000,                   /* COLOR_ACTIVEBORDER     10  */
    0x00FFFFFF,                   /* COLOR_INACTIVEBORDER   11  */
    0x00FFFFFF,                   /* COLOR_APPWORKSPACE     12  */
    0x00000000,                   /* COLOR_HIGHLIGHT        13  */
    0x00FFFFFF,                   /* COLOR_HIGHLIGHTTEXT    14  */
    0x00FFFFFF,                   /* COLOR_BTNFACE          15  */
    0x00000000,                   /* COLOR_BTNSHADOW        16  */
    0x00000000,                   /* COLOR_GRAYTEXT         17  */
    0x00000000,                   /* COLOR_BTNTEXT          18  */
    0x00000000                    /* COLOR_ENDCOLORS        18  */
  };

int   SysColorIndices[NUMSYSCOLORS] = {
    COLOR_SCROLLBAR,
    COLOR_BACKGROUND,     
    COLOR_ACTIVECAPTION,  
    COLOR_INACTIVECAPTION,
    COLOR_MENU,           
    COLOR_WINDOW,         
    COLOR_WINDOWFRAME,    
    COLOR_MENUTEXT,       
    COLOR_WINDOWTEXT,     
    COLOR_CAPTIONTEXT,    
    COLOR_ACTIVEBORDER,   
    COLOR_INACTIVEBORDER, 
    COLOR_APPWORKSPACE,   
    COLOR_HIGHLIGHT,      
    COLOR_HIGHLIGHTTEXT,  
    COLOR_BTNFACE,        
    COLOR_BTNSHADOW,      
    COLOR_GRAYTEXT,       
    COLOR_BTNTEXT,
    COLOR_ENDCOLORS
  };



/*********************** DISPLAY DEFAULT STATE MATRIX *********************/

BYTE DDSM [MAXDISP] [MAXCLASS] [MAXPAL] [MAXDITH-1] [2] = {

{   /* DISP = evga */

{   /*************CLASS rgb evga *******************/

{   /* PAL = NORM  , 5 DITHERS */
    
{   INORM,  INONE  },       /* NORM */
{   INORM,  IBAY   },
{   INORM,  IFS    },
{   INORM,  IFS16  },
{   INORM,  IFS16  }

},

{
{   IOPT,       INONE },        /* OPT */
{   INORM,      INONE },
{   IOPT,       IFS   },
{   IOPT,       INONE },
{   IOPT,       IFS16 }
},  

{
{   IGRA,       INONE },        /* GRAY */
{   IGRA,       INONE },
{   IGRA,       INONE },
{   IGRA,       INO16 },
{   IGRA,       INO16 }
},

{
{   IBW,        INONE },        /* BW */
{   IBW,        IBAY  },
{   IBW,        IFS   },
{   IBW,        INONE },
{   IBW,        IFS   }
}

},

{   /************** COLORMAP evga *******************/

{       
{   INORM,  INONE },        /* NORM */
{   INORM,  INONE },
{   INORM,  INONE },
{   INORM,  INONE },
{   INORM,  INONE }
},

{
{   INORM,  INONE },        /* OPT */
{   INORM,  INONE },
{   INORM,  INONE },
{   INORM,  INONE },
{   INORM,  INONE }
},  

{
{   IGRA,       INONE },        /* GRAY */
{   IGRA,       INONE },
{   IGRA,       INONE },
{   IGRA,       INO16 },
{   IGRA,       INO16 }
},

{
{   IBW,        INONE },        /* BW */
{   IBW,        IBAY  },
{   IBW,        IFS   },
{   IBW,        INONE },
{   IBW,        IFS   }
}
},

{   /************** gray evga *******************/

{       
{   IGRA,       INONE },        /* NORM */
{   IGRA,       INONE },
{   IGRA,       INONE },
{   IGRA,       INO16 },
{   IGRA,       INO16 }
},

{
{   IGRA,       INONE },        /* OPT */
{   IGRA,       INONE },
{   IGRA,       INONE },
{   IGRA,       INO16 },
{   IGRA,       INO16 }
},

{   
{   IGRA,       INONE },        /* GRAY */
{   IGRA,       INONE },
{   IGRA,       INONE },
{   IGRA,       INO16 },
{   IGRA,       INO16 }
},

{
{   IBW,        INONE },        /* BW */
{   IBW,        IBAY  },
{   IBW,        IFS   },
{   IBW,        INONE },
{   IBW,        IFS   }
}
},

{   /************** BW evga *******************/
 
{       
{   IBW,        INONE },        /* NORM */
{   IBW,        INONE },
{   IBW,        INONE },
{   IBW,        INONE },
{   IBW,        INONE }
},

{
{   IBW,        INONE },        /* OPT */
{   IBW,        INONE },
{   IBW,        INONE },
{   IBW,        INONE },
{   IBW,        INONE }
},

{
{   IBW,        INONE },        /* GRAY */
{   IBW,        INONE },
{   IBW,        INONE },
{   IBW,        INONE },
{   IBW,        INONE }
},

{
{   IBW,        INONE },        /* BW */
{   IBW,        INONE },
{   IBW,        INONE },
{   IBW,        INONE },
{   IBW,        INONE }
}

}
},

{   /* DISP = vga */

{   /************** rgb vga *******************/

{       
{   INORM,  INONE },        /* NORM */
{   INORM,  IBAY  },
{   INORM,  IFS   },
{   INORM,  INONE },
{   INORM,  IFS   }
},

{
{   INORM,  INONE },        /* OPT */
{   INORM,  INONE  },
{   IOPT,       IFS   },
{   INORM,  INONE },
{   IOPT,       IFS16 }
},

{   
{   IGRA,       INONE },        /* GRAY */
{   IGRA,       INONE }, 
{   IGRA,       INONE },
{   IGRA,       INO16 },
{   IGRA,       INO16 }
},

{
{   IBW,        INONE },        /* BW */
{   IBW,        IBAY  },
{   IBW,        IFS   },
{   IBW,        INONE },
{   IBW,        IFS   }
}
},

{   /************** COLORMAP vga *******************/

{       
{   INORM,  INONE },        /* NORM */
{   INORM,  INONE },
{   INORM,  INONE },
{   INORM,  INONE },
{   INORM,  INONE }
},

{
{   INORM,  INONE },        /* OPT */
{   INORM,  INONE },
{   INORM,  INONE },
{   INORM,  INONE },
{   INORM,  INONE }
},

{   
{   IGRA,       INONE },        /* GRAY */
{   IGRA,       INONE },
{   IGRA,       INONE },
{   IGRA,       INO16 },
{   IGRA,       INO16 }
},

{
{   IBW,        INONE },        /* BW */
{   IBW,        IBAY  },
{   IBW,        IFS   },
{   IBW,        INONE },
{   IBW,        IFS   }
}
},

{   /************** gray vga *******************/

{       
{   IGRA,       INONE },        /* NORM */
{   IGRA,       INONE },
{   IGRA,       INONE },
{   IGRA,       INO16 },
{   IGRA,       INO16 }
},

{
{   IGRA,       INONE },        /* OPT */
{   IGRA,       INONE },
{   IGRA,       INONE },
{   IGRA,       INO16 },
{   IGRA,       INO16 }
},

{   
{   IGRA,       INONE },        /* GRAY */
{   IGRA,       INONE }, 
{   IGRA,       INONE },
{   IGRA,       INO16 },
{   IGRA,       INO16 }
},

{
{   IBW,        INONE },        /* BW */
{   IBW,        IBAY  },
{   IBW,        IFS   },
{   IBW,        INONE },
{   IBW,        IFS   }
}
},
 
{  /************** BW vga *******************/

{       
{   IBW,        INONE },        /* NORM */
{   IBW,        INONE },
{   IBW,        INONE },
{   IBW,        INONE },
{   IBW,        INONE }
},

{
{   IBW,        INONE },        /* OPT */
{   IBW,        INONE },
{   IBW,        INONE },
{   IBW,        INONE },
{   IBW,        INONE }
},

{
{   IBW,        INONE },        /* GRAY */
{   IBW,        INONE },
{   IBW,        INONE },
{   IBW,        INONE },
{   IBW,        INONE }
},

{
{   IBW,        INONE },        /* BW */
{   IBW,        INONE },
{   IBW,        INONE },
{   IBW,        INONE },
{   IBW,        INONE   }
}
}
    }

};

/******************** DISPLAY STATE FILTER MATRIX **************************/

/* Thus initial is EVGA, RGB, NORM, NONE, 0 */

/*           2          4          4         5           6           
 int    DIS[ MAXDISP ][ MAXCLASS ][ MAXPAL ][ MAXDITH-1 ][ MAXFILTER+ 1 ] = {
*/
BYTE DIS[ 2 ][ 4 ][ 4 ][ 5 ][ 6 ] = {

{   /* EVGA */
    { /* raw */
        {
            {   UQUA,       0,          0,          0,          0,          0 },        /* RAW, NORM */
            {   BAY8,       0,          0,          0,          0,          0 },
            {   FS,         0,          0,          0,          0,          0 },                        
            {   FS,         0,          0,          0,          0,          0 },                        
            {   FS,         0,          0,          0,          0,          0 }                     
        },
        {
            {   REDU,       0,          0,          0,          0,          0 },        /* RAW, OPT */
            {   BAY8,       0,          0,          0,          0,          0 },                        
            {   FS,         0,          0,          0,          0,          0 },                        
            {   REDU,       0,          0,          0,          0,          0 },                        
            {   FS,         0,          0,          0,          0,          0 }                     
        },
        {   
            {   GSUM,       0,          0,          0,          0,          0 },        /* RAW, GRAY */
            {   GSUM,       0,          0,          0,          0,          0 },                        
            {   GSUM,       0,          0,          0,          0,          0 },                        
            {   GSUM,       LEVL,       0,          0,          0,          0 },                        
            {   GSUM,       LEVL,       0,          0,          0,          0 }                     
        },
        {   
            {   GSUM,       G2BW,       0,          0,          0,          0 },        /* RAW, BW */
            {   GSUM,       GBAY,       0,          0,          0,          0 },                        
            {   GSUM,       GFS,        0,          0,          0,          0 },                        
            {   GSUM,       G2BW,       0,          0,          0,          0 },                        
            {   GSUM,       GFS,        0,          0,          0,          0 }                     
        }
    }, /* end raw */

    { /* cmap */
        {
            {   LIT,        0,          0,          0,          0,          0 },        /* CMAP, NORM */
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 }
        },
        {
            {   LIT,        0,          0,          0,          0,          0 },        /* CMAP, OPT */
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 }
        },
        {
            {   DIDX,       GSUM,       0,      0,          0,          0 },        /* CMAP, GRAY */
            {   DIDX,       GSUM,       0,      0,          0,          0 },
            {   DIDX,       GSUM,       0,      0,          0,          0 },
            {   DIDX,       GSUM,       LEVL,       0,          0,          0 },
            {   DIDX,       GSUM,       LEVL,   0,          0,          0 }
        },
        {
            {   DIDX,       GSUM,       G2BW,       0,          0,          0 },        /* CMAP, BW */
            {   DIDX,       GSUM,       GBAY,       0,          0,          0 },
            {   DIDX,       GSUM,       GFS,        0,          0,          0 },
            {   DIDX,       GSUM,       G2BW,       0,          0,          0 },
            {   DIDX,       GSUM,       GFS,        0,          0,          0 }
        }
    }, /* end cmap */

    { /* gray */
        {
            {   LIT,        0,          0,          0,          0,          0 },        /* GRAY, NORM */
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 },
            {   LEVL,       0,          0,          0,          0,          0 },
            {   LEVL,       0,          0,          0,          0,          0 }
        },
        {
            {   LIT,        0,          0,          0,          0,          0 },        /* GRAY, NORM */
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 },
            {   LEVL,       0,          0,          0,          0,          0 },
            {   LEVL,       0,          0,          0,          0,          0 }
        },
        {
            {   LIT,        0,          0,          0,          0,          0 },        /* GRAY, GRAY */
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 },
            {   LEVL,       0,          0,          0,          0,          0 },
            {   LEVL,       0,          0,          0,          0,          0 }
        },
        {
            {   G2BW,       0,          0,          0,          0,          0 },        /* GRAY, BW */
            {   GBAY,       0,          0,          0,          0,          0 },
            {   GFS,        0,          0,          0,          0,          0 },
            {   G2BW,       0,          0,          0,          0,          0 },
            {   GBAY,       0,          0,          0,          0,          0 }
        }
    }, /* end gray */

    { /* bw */
        {
            {   LIT,        0,          0,          0,          0,          0 },        /* BW, NORM */
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 }
        },
        {
            {   LIT,        0,          0,          0,          0,          0 },        /* BW, OPT */
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 },
        },
        {
            {   LIT,        0,          0,          0,          0,          0 },        /* BW, GRAY */
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 },
        },
        {
            {   LIT,        0,          0,          0,          0,          0 },        /* BW, BW */
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 }
        }
    } /* end bw */
},               /* end evga */


{ /************ VGA, RAW, NORM,  NONE **********/
    {
        {
            {   UQUA8,  0,          0,          0,          0,          0 },        /* RAW, NORM */
            {   BAY8,       0,          0,          0,          0,          0 },
            {   FS,     0,          0,          0,          0,          0 },                        
            {   UQUA8,  0,          0,          0,          0,          0 },                        
            {   FS,     0,          0,          0,          0,          0 }                     
        },
        {
            {   UQUA8,  0,          0,          0,          0,          0 },        /* RAW, OPT */
            {   BAY8,       0,          0,          0,          0,          0 },                        
            {   FS,     0,          0,          0,          0,          0 },                        
            {   UQUA8,  0,          0,          0,          0,          0 },                        
            {   FS,     0,          0,          0,          0,          0 }                     
        },
        {   
            {   GSUM,       LEVL,           0,          0,          0,          0 },        /* RAW, GRAY */
            {   GSUM,       LEVL,           0,          0,          0,          0 },                        
            {   GSUM,       LEVL,           0,          0,          0,          0 },                        
            {   GSUM,       LEVL,       0,          0,          0,          0 },                        
            {   GSUM,       LEVL,       0,          0,          0,          0 }                     
        },
        {   
            {   GSUM,       G2BW,       0,          0,          0,          0 },        /* RAW, BW */
            {   GSUM,       GBAY,       0,          0,          0,          0 },                        
            {   GSUM,       GFS,        0,          0,          0,          0 },                        
            {   GSUM,       G2BW,       0,          0,          0,          0 },                        
            {   GSUM,       GFS,        0,          0,          0,          0 }                     
        }
    },
    {
        {
            {   DIDX,       FS,     0,          0,          0,          0 },        /* CMAP, NORM */
            {   DIDX,       FS,     0,          0,          0,          0 },
            {   DIDX,       FS,     0,          0,          0,          0 },
            {   DIDX,       FS,     0,          0,          0,          0 },
            {   DIDX,       FS,     0,          0,          0,          0 }
        },
        {
            {   DIDX,       FS,     0,          0,          0,          0 },        /* CMAP, OPT */
            {   DIDX,       FS,     0,          0,          0,          0 },
            {   DIDX,       FS,     0,          0,          0,          0 },
            {   DIDX,       FS,     0,          0,          0,          0 },
            {   DIDX,       FS,     0,          0,          0,          0 }
        },
        {
            {   DIDX,       GSUM,       LEVL,           0,          0,          0 },        /* CMAP, GRAY */
            {   DIDX,       GSUM,       LEVL,           0,          0,          0 },
            {   DIDX,       GSUM,       LEVL,           0,          0,          0 },
            {   DIDX,       GSUM,       LEVL,           0,          0,          0 },
            {   DIDX,       GSUM,       LEVL,           0,          0,          0 }
        },
        {
            {   DIDX,       GSUM,       G2BW,       0,          0,          0 },        /* CMAP, BW */
            {   DIDX,       GSUM,       GBAY,       0,          0,          0 },
            {   DIDX,       GSUM,       GFS,        0,          0,          0 },
            {   DIDX,       GSUM,       G2BW,       0,          0,          0 },
            {   DIDX,       GSUM,       GFS,        0,          0,          0 }
        }
    },
    {
        {
            {   LEVL,       0,          0,          0,          0,          0 },        /* GRAY, NORM */
            {   LEVL,       0,          0,          0,          0,          0 },
            {   LEVL,       0,          0,          0,          0,          0 }, 
            {   LEVL,       0,          0,          0,          0,          0 },
            {   LEVL,       0,          0,          0,          0,          0 }
        },
        {
            {   LEVL,       0,          0,          0,          0,          0 },        /* GRAY, NORM */
            {   LEVL,       0,          0,          0,          0,          0 },
            {   LEVL,       0,          0,          0,          0,          0 },
            {   LEVL,       0,          0,          0,          0,          0 },
            {   LEVL,       0,          0,          0,          0,          0 }
        },
        {
            {   LEVL,       0,          0,          0,          0,          0 },        /* GRAY, GRAY */
            {   LEVL,       0,          0,          0,          0,          0 },
            {   LEVL,       0,          0,          0,          0,          0 },
            {   LEVL,       0,          0,          0,          0,          0 },
            {   LEVL,       0,          0,          0,          0,          0 }
        },
        {
            {   G2BW,       0,          0,          0,          0,          0 },        /* GRAY, BW */
            {   GBAY,       0,          0,          0,          0,          0 },
            {   GFS,        0,          0,          0,          0,          0 },
            {   G2BW,       0,          0,          0,          0,          0 },
            {   GBAY,       0,          0,          0,          0,          0 }
        }
    },
    {
        {
            {   LIT,        0,          0,          0,          0,          0 },        /* BW, NORM */
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 }
        },
        {
            {   LIT,        0,          0,          0,          0,          0 },        /* BW, OPT */
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 }
        },
        {
            {   LIT,        0,          0,          0,          0,          0 },        /* BW, GRAY */
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 }
        },
        {
            {   LIT,        0,          0,          0,          0,          0 },        /* BW, BW */
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 },
            {   LIT ,       0,          0,          0,          0,          0 },
            {   LIT,        0,          0,          0,          0,          0 }
        }
    }    /* end bw */
}  /* end vga */
};

/********************** EXPORT STATE FILTER MATRIX ************************/

/* Thus initial is RGB, NORM, NONE, 0 */

BYTE ESF [MAXCLASS] [MAXPAL] [MAXDITH] [MAXFILTER + 1] = {

    UQUA,       0,          0,          0,          0,          0,      /* RAW, NORM */
    BAY8,       0,          0,          0,          0,          0,
    FS,         0,          0,          0,          0,          0,                      
    FS,         0,          0,          0,          0,          0,                      
    FS,         0,          0,          0,          0,          0,                      
    LIT,        0,          0,          0,          0,          0,      /* .., RAW */

    REDU,       0,          0,          0,          0,          0,      /* RAW, OPT */
    BAY8,       0,          0,          0,          0,          0,                      
    FS,         0,          0,          0,          0,          0,                      
    FS,         0,          0,          0,          0,          0,      /* REDU16 ? */              
    FS,         0,          0,          0,          0,          0,                      
    LIT,        0,          0,          0,          0,          0,      /* .., RAW */
    
    GSUM,       0,          0,          0,          0,          0,      /* RAW, GRAY */
    GSUM,       0,          0,          0,          0,          0,                      
    GSUM,       0,          0,          0,          0,          0,                      
    GSUM,       LEVL,       0,          0,          0,          0,      /* + LEVL ? */
    GSUM,       LEVL,       0,          0,          0,          0,      /* + LEVL ? */              
    0,          0,          0,          0,          0,          0,      /* .., RAW */
    
    GSUM,       G2BW,       0,          0,          0,          0,      /* RAW, BW */
    GSUM,       GBAY,       0,          0,          0,          0,                      
    GSUM,       GFS,        0,          0,          0,          0,                      
    GSUM,       G2BW,       0,          0,          0,          0,                      
    GSUM,       GFS,        0,          0,          0,          0,                      
    0,          0,          0,          0,          0,          0,      /* .., RAW */

    LIT,        0,          0,          0,          0,          0,      /* CMAP, NORM */
    LIT,        0,          0,          0,          0,          0,
    LIT,        0,          0,          0,          0,          0,
    LIT,        0,          0,          0,          0,          0,
    LIT,        0,          0,          0,          0,          0,
    DIDX,       0,          0,          0,          0,          0,      /* .. , RAW */

    LIT,        0,          0,          0,          0,          0,      /* CMAP, OPT */
    LIT,        0,          0,          0,          0,          0,
    LIT,        0,          0,          0,          0,          0,
    LIT,        0,          0,          0,          0,          0,
    LIT,        0,          0,          0,          0,          0,
    DIDX,       0,          0,          0,          0,          0,      /* .. , RAW */

    DIDX,       GSUM,       0,          0,          0,          0,      /* CMAP, GRAY */
    DIDX,       GSUM,       0,          0,          0,          0,
    DIDX,       GSUM,       0,          0,          0,          0,
    DIDX,       GSUM,       LEVL,       0,          0,          0,      /* + LEVL ? */
    DIDX,       GSUM,       LEVL,       0,          0,          0,      /* + LEVL ? */
    0,          0,          0,          0,          0,          0,      /* .., RAW */

    DIDX,       GSUM,       G2BW,       0,          0,          0,      /* CMAP, BW */
    DIDX,       GSUM,       GBAY,       0,          0,          0,
    DIDX,       GSUM,       GFS,        0,          0,          0,
    DIDX,       GSUM,       G2BW,       0,          0,          0,
    DIDX,       GSUM,       GFS,        0,          0,          0,
    0,          0,          0,          0,          0,          0,      /* .., RAW */

    LIT,        0,          0,          0,          0,          0,      /* GRAY, NORM */
    LIT,        0,          0,          0,          0,          0,
    LIT,        0,          0,          0,          0,          0,
    LIT,        0,          0,          0,          0,          0,      /* + LEVL ? */
    LIT,        0,          0,          0,          0,          0,      /* + LEVL ? */
    0,          0,          0,          0,          0,          0,      /* .., RAW */

    LIT,        0,          0,          0,          0,          0,      /* GRAY, OPT */
    LIT,        0,          0,          0,          0,          0,
    LIT,        0,          0,          0,          0,          0,
    LIT,        0,          0,          0,          0,          0,
    LIT,        0,          0,          0,          0,          0,
    0,          0,          0,          0,          0,          0,      /* .., RAW */

    LIT,        0,          0,          0,          0,          0,      /* GRAY, GRAY */
    LIT,        0,          0,          0,          0,          0,
    LIT,        0,          0,          0,          0,          0,
    LEVL,       0,          0,          0,          0,          0, // Was lit
    LEVL,       0,          0,          0,          0,          0, // Was lit
    0,          0,          0,          0,          0,          0,      /* .., RAW */

    G2BW,       0,          0,          0,          0,          0,      /* GRAY, BW */
    GBAY,       0,          0,          0,          0,          0,
    GFS,        0,          0,          0,          0,          0,
    G2BW,       0,          0,          0,          0,          0,
    GBAY,       0,          0,          0,          0,          0,
    0,          0,          0,          0,          0,          0,      /* .., RAW */

    LIT,        0,          0,          0,          0,          0,      /* BW, NORM */
    LIT,        0,          0,          0,          0,          0,
    LIT,        0,          0,          0,          0,          0,
    LIT,        0,          0,          0,          0,          0,
    LIT,        0,          0,          0,          0,          0,
    0,          0,          0,          0,          0,          0,      /* .., RAW */

    LIT,        0,          0,          0,          0,          0,      /* BW, OPT */
    LIT,        0,          0,          0,          0,          0,
    LIT,        0,          0,          0,          0,          0,
    LIT,        0,          0,          0,          0,          0,
    LIT,        0,          0,          0,          0,          0,
    0,          0,          0,          0,          0,          0,      /* .., RAW */

    LIT,        0,          0,          0,          0,          0,      /* BW, GRAY */
    LIT,        0,          0,          0,          0,          0,
    LIT,        0,          0,          0,          0,          0,
    LIT,        0,          0,          0,          0,          0,
    LIT,        0,          0,          0,          0,          0,
    0,          0,          0,          0,          0,          0,      /* .., RAW */

    LIT,        0,          0,          0,          0,          0,      /* BW, BW */
    LIT,        0,          0,          0,          0,          0,
    LIT,        0,          0,          0,          0,          0,
    LIT,        0,          0,          0,          0,          0,
    LIT,        0,          0,          0,          0,          0,
    0,          0,          0,          0,          0,          0,      /* .., RAW */
};

/******************** DISPLAY STATE PAL MATRIX **************************/

// int  DPAL[ MAXDISP ][ MAXCLASS ][ MAXPAL ][ MAXDITH -1 ][ PALTYPE ]= {

BYTE DPAL [2] [4] [4] [5] [1]= {

{ /******************************** EVGA ************************************/
{ /* raw */

//{   DP256,  DP8,    DP256,  DP16,   DP16 },     /* RAW , NORM */  // HERE!
  {   DP256,  DP256,    DP256,  DP16,   DP16 },     /* RAW , NORM */
{   OP256,  OP256,  OP256,  OP16,   OP16 },     /* RAW , OPT */
{   GP256,  GP256,  GP256,  GP16,   GP16 },
{   BP,     BP,     BP,     BP,     BP   }      /* RAW , BW */
}, /* end raw */

{   /* CMAP */
{   IP256,  IP256,      IP256,  IP256,  IP256 },    /* CMAP , NORM */
{   IP256,  IP256,      IP256,  IP256,  IP256 },    /* CMAP , OPT */
{ GP256,  GP256,  GP256,  GP16,   GP16 },
{   BP,     BP,     BP,         BP,     BP   }      /* CMAP , BW */
}, /* CMAP */

{
{ GP256,  GP256,  GP256,  GP16,   GP16 },
{ GP256,  GP256,  GP256,  GP16,   GP16 },
{ GP256,  GP256,  GP256,  GP16,   GP16 },
{   BP,     BP,         BP,     BP,     BP   }      /* GRAY , BW */
},

{
{   BP,     BP,     BP,     BP,     BP },       /* BW , NORM */
{   BP,     BP,     BP,     BP,     BP },       /* BW , OPT */
{   BP,     BP,     BP,     BP,     BP },       /* BW , GRAY */
{   BP,     BP,     BP,     BP,     BP }        /* BW , BW */
}
}, /* end evga */

{ /******************************** VGA *************************************/
{ /* raw */
{   DP8,        DP8,        DP8,        DP8,        DP8 },      /* RAW , NORM */
{   DP8,        DP8,        DP8,        DP8,        DP8 },      /* RAW , OPT */
{   GP8,        GP8,        GP8,        GP8,        GP8 },      /* RAW , GRAY */
{   BP,         BP,         BP,         BP,         BP }        /* RAW , BW */
},
{
{   DP8,        DP8,    DP8,        DP8,        DP8 },      /* CMAP , NORM */
{   DP8,        DP8,    DP8,        DP8,        DP8 },      /* CMAP , OPT */
{   GP8,        GP8,        GP8,        GP8,        GP8 },      /* CMAP , GRAY */
{   BP,         BP,         BP,         BP,         BP }        /* CMAP , BW */
},
{
{   GP8,        GP8,        GP8,        GP8,        GP8 },      /* GRAY , NORM */
{   GP8,        GP8,        GP8,        GP8,        GP8 },      /* GRAY , OPT */
{   GP8,        GP8,        GP8,        GP8,        GP8 },      /* GRAY , GRAY */
{   BP,         BP,         BP,         BP,         BP }        /* GRAY , BW */
},
{
{   BP,     BP,     BP,     BP,     BP },       /* BW , NORM */
{   BP,     BP,     BP,     BP,     BP },       /* BW , OPT */
{   BP,     BP,     BP,     BP,     BP },       /* BW , GRAY */
{   BP,     BP,     BP,     BP,     BP }        /* BW , BW */
}
} /* end vga */
};

/********************** EXPORT STATE PAL MATRIX ************************/
// int  EPAL[ MAXCLASS ][ MAXPAL ][ MAXDITH ][ PALTYPE ] =

BYTE EPAL [4] [4] [5] [1] =  {
{
//{   DP256,  DP8,        DP256,  DP16,       DP16 },     /* RAW , NORM */  //HERE!
{   DP256,  DP256,        DP256,  DP16,       DP16 },     /* RAW , NORM */
{   OP256,  OP256,  OP256,  OP16,       OP16 },     /* RAW , OPT */
{   GP256,  GP256,  GP256,  GP16,   GP16 },     /* RAW , GRAY */
{   BP,     BP,     BP,     BP,     BP   }      /* RAW , BW */
},
{
{   IP256,  IP256,      IP256,  IP256,  IP256 },    /* CMAP , NORM */
{   IP256,  IP256,      IP256,  IP256,  IP256 },    /* CMAP , OPT */
{   GP256,  GP256,  GP256,  GP16,   GP16 },     /* CMAP , GRAY */
{   BP,         BP,             BP,         BP,         BP    }     /* CMAP , BW */
},
{
{   GP256,  GP256,  GP256,  GP16,   GP16 },     /* GRAY , NORM */
{   GP256,  GP256,  GP256,  GP16,   GP16 },     /* GRAY , OPT */
{   GP256,  GP256,  GP256,  GP16,   GP16 },     /* GRAY , GRAY */
{   BP,         BP,         BP,         BP,         BP   }      /* GRAY , BW */
},
{
{   BP,         BP,         BP,         BP,         BP },       /* BW , NORM */
{   BP,         BP,         BP,         BP,         BP },       /* BW , OPT */
{   BP,         BP,         BP,         BP,         BP },       /* BW , GRAY */
{   BP,         BP,         BP,         BP,         BP }        /* BW , BW */
}
};

/*---  Save Formats Matrix               ---*/


BYTE SFM [4][XALLFORMATS] = {

/*---  NON INDEXED                                         ---  INDEXED ---     
   NONE, CPI TIFF EPS TG16 TGA TG32 PD  WD WMF24 DVA CCPI CPI TIF TGA PCX GIF PD WD EPS WMF       */

{   0,    1,  1,   1,  1,  1,   1,  1,  1,  1,    1,  1,   1,  1,  1,  1,  1, 1, 1, 0,  1,  },  /* NORM */
{   0,    1,  1,   1,  1,  1,   1,  1,  1,  1,    0,  1,   1,  1,  1,  1,  1, 1, 1, 0,  1,  },  /* OPT */
{   0,    0,  0,   0,  0,  0,   0,  0,  0,  0,    0,  0,   1,  1,  1,  1,  1, 1, 1, 1,  1,  },  /* GRAY */
{   0,    0,  0,   0,  0,  0,   0,  0,  0,  0,    0,  0,   1,  1,  0,  0,  0, 1, 1, 1,  1,  }   /* BW */
};


/****************************************************************************

   PROCEDURE:      ImagePrepInit
   DESCRIPTION:    Initialization procedure for ImgPrep
   DEFINITION:     1.1
   START:          1/2/90  Tom Bagford Jr.   ( based on David Ison's 
                                               original work )
   MODS:                 4/16/90  3.0 review, annotated

*****************************************************************************/

BOOL FAR PASCAL ImagePrepInit (hInstance, hPrevInstance, lpszCmdLine, nCmdShow)
HANDLE      hInstance;
HANDLE      hPrevInstance;
LPSTR       lpszCmdLine;
int         nCmdShow;
{
  WNDCLASS  wc;
  /*
  **  save instance handle 
  */
  wc.style          = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc    = IPWndProc;
  wc.cbClsExtra     = 0;
  wc.cbWndExtra     = 8;
  wc.hInstance      = hInstance;
  wc.hIcon          = LoadIcon (hInstance, MAKEINTRESOURCE (IMGPREP_ICON));
  wc.hCursor        = LoadCursor (NULL, IDC_ARROW);
  wc.hbrBackground  = GetStockObject (LTGRAY_BRUSH);
  wc.lpszMenuName   = (LPSTR)"ImgPrepMenu";
  wc.lpszClassName  = (LPSTR)szAppName;
  if (!RegisterClass ((LPWNDCLASS)&wc))
    return (FALSE);

  wc.style          = CS_HREDRAW | CS_VREDRAW;

  /* Change only necessary parameters  */

  wc.lpfnWndProc    = DisplayWndProc;
  wc.cbWndExtra     = 0;
  wc.hbrBackground  = GetStockObject (LTGRAY_BRUSH);
  wc.lpszMenuName   = NULL;
  wc.lpszClassName  = (LPSTR)"Display";
  if (!RegisterClass ((LPWNDCLASS)&wc))
    return (FALSE);


  return (TRUE);
}


