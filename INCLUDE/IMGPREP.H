/*-----------------------------------------------------------------------------

   MODULE:         imgprep.h

-----------------------------------------------------------------------------*/

/*  Macro for freeing global memory handles saved as global data */

#define GLOBALFREE(h) (h) = (h) ? (GlobalFree ((h)) ? (h) : 0): 0

#ifdef WHATHAPPENED
#ifdef DEBUG
#define WinAssert(exp,sz)                       \
  {                                             \
    if (!(exp)){                                \
      char szBuffer[40];                        \
      wsprintf ((LPSTR)szBuffer,                \
        (LPSTR)"ERROR! %s  %d",                 \
        (LPSTR)__FILE__, __LINE__);             \
      MessageBeep(0);                           \
      MessageBox (NULL, (LPSTR)(sz),            \
        (LPSTR)szBuffer, MB_OK | MB_ICONHAND);  \
    }                                           \
  }                                             \
#else
#define WinAssert (exp, sz)
#endif
#endif

#define NUMSYSCOLORS    20
#define MAXBUFSIZE      20480  // Current max strip size on file I/O
#define MAXSTRINGSIZE   256
#define  MAX_CMAP_SIZE  256
#define MAXPATHSIZE     144

#define SAVE_TO_CPI     0
#define SAVE_NORMAL     1

#define FailedLWrite(bytes,length) (bytes) == -1 || (bytes) != (length) 
#define FailedLRead(bytes,length)  (bytes) == -1 || (bytes) != (length) 


#define   WM_SHOWIMAGE     WM_USER + 100


/*--- File Formats ---

               FOR USE WITHIN wImportType and wExportType
               and OFFSETS for STATE MATRICES

*/

#define IDFMT_NONE                0
#define IDFMT_CPI                 1             
#define IDFMT_TIF                 2
#define IDFMT_PS                  3
#define IDFMT_TGA                 4
#define IDFMT_PCX                 5
#define IDFMT_GIF                 6
#define IDFMT_PMDIB               7
#define IDFMT_WNDIB               8
#define IDFMT_WMF                 9
#define IDFMT_DVA                 10
#define IDFMT_CCPI                11
#define MAXFORMATS                12


/*--- wDither Values  ---*/

#define DITHER_NONE               1     /* No Dither */
#define DITHER_BAYER              2     /* Ordered Dither, 8 color default */
#define DITHER_FS                 3     /* Floyd-Steinberg Dither 256 shade */
#define DITHER_NONE16             4     /* No Dither, reduce to 16 shades */
#define DITHER_FS16               5     /* FLoyd Steinberg Dither 16 shades */
#define DITHER_24BIT              6   

/*--- wPal Values  ---*/

#define PAL_NORM                  0
#define PAL_OPT                   1
#define PAL_GRAY                  2
#define PAL_BW                    3

/*--- SetState ID's ---*/

#define   EXPORT_TYPE             1
#define   IMPORT_TYPE             2
#define   EXPORT_CLASS            3
#define   IMPORT_CLASS            4
#define   REGION_STATE            5
#define   HOTKEY_STATE            6
#define   DITH_STATE              7
#define   PAL_STATE               8
#define   DISPLAY_STATE           9
#define   INPUT_CLASS             10
#define   SAVE_TYPE               11
#define   PAINT_STATE             12
#define   DEVICE_STATE            13
#define   CAPTURE_STATE           14


/*--- Attributes for DlgDirLst() ---*/

#define  ATTRFILELIST    0x0000 

#define  ATTRDIRLIST     0x4000 | 0x0010 | 0x8000       /* directories and drives ONLY */
#define  CBEXTMAX        6                              /* Number of bytes in "\*.txt" */

#define IDF(id)  ((id) & ~DLGOPEN_OPTION)               /* Extracts flag from control ID */
#define FID(f)   ((f)  |  DLGOPEN_OPTION)               /* Extracts control ID from flag */

/* Save Dialog   */

#define  XCPI24                   1                     /* RGB FORMATS */
#define  XTIF24                   2
#define  XEPS24                   3
#define  XTGA16                   4
#define  XTGA24                   5
#define  XTGA32                   6
#define  XPMDIB24                 7                         
#define  XWNDIB24                 8                          
#define  XWMF24                   9 
#define  XDVA24                   10
#define  XCCPI24                  11                         

#define  XCM                      12                    /* COLORMAP */
#define  XCPI                     12 
#define  XTIF                     13
#define  XTGA                     14
#define  XPCX                     15
#define  XGIF                     16
#define  XPMDIB                   17 
#define  XWNDIB                   18 
#define  XEPS                     19
#define  XWMF                     20
#define  XALLFORMATS              21

/*   GenPal   */

#define  IMPORT_PAL               1
#define  EXPORT_PAL               2

/*  State Control   

   Array indexes are based on the state variables:

   wDisplay, wClass, wPal, wDither 

                                            */

/* Display / Export Filter State Matrix     */

#define  MAXDISP       2                    /* wDisplay states */      
#define  I8514         0                    /* extended vga display */
#define  IEVGA         0                    /* extended vga display */
#define  IVGA          1                    /* standard vga display */

#define  MAXCLASS      4                    /* wClass states */   
#define  IRGB          0                    /* 24 bit RGB color file class */
#define  ICM           1                    /* Colormaped, 8bit indexed class */
#define  IGR           2                    /* Gray 8bit value class */
#define  IMON          3                    /* Monochrome 1bit class */

#define  MAXPAL        4                    /* wPal states */
#define  INORM         0                    /* Normal: image or default pal d( 256/8 ) */
#define  IOPT          1                    /* Optimized pal disp( 256/8 ) exp( 256/16 )*/
#define  IGRA          2                    /* Gray:   disp (64/8) export (256/16) level */
#define  IBW           3                    /* BW palette disp (2) levels */

#define  MAXDITH       6                    /* wDither states */
#define  INONE         0                    /* No dither */
#define  IBAY          1                    /* Bayer dither */
#define  IFS           2                    /* Floyd Steinberg dither */
#define  INO16         3                    /* No dither VGA 16 color */
#define  IFS16         4                    /* Floyd Steinberg dither VGA 16 color */
#define  RAW           5                    /* Raw 24 bit, no palette (for export only)*/

/*  Filters  */

#define  MAXFILTER     5                    /* Max # filter procs called from array */

#define  LIT           0                    /* Literal colormapping */
#define  REDU          1                    /* Reduction: OPR */
#define  FS            2                    /* Floyd-Steinberg with palette provided */
#define  BAY8          3                    /* Bayer with 8 colors VGA */
#define  UQUA          4                    /* Uniform Quantize with pal provided */
#define  UQUA8         5                    /* Uniform Quantize with 8 color VGA */
#define  GSUM          6                    /* gray sum 24bit to 8 bit values */
#define  LEVL          7                    /* shift gray levels */
#define  DIDX          8                    /* de-index: 8 to 24 bits */
#define  G2BW          9                    /* Gray to Black&White thresholded */
#define  GBAY          10                   /* gray to BW with Bayer dither */
#define  GFS           11                   /* Gray to BW with Floyd-Steinberg dither */
#define  ALLFILT       12                   /* total filters */

/*  Palette  */

#define  DP256         0                    /* Default palette 256 */
#define  DP8           1                    /* default palette 8 */
#define  DP16          2                    /* default palette 16 */
#define  OP256         3                    /* optimized palette 256 */
#define  OP8           4                    /* optimized palette 8 */
#define  OP16          5                    /* optimized palette 16 */
#define  GP256         6                    /* gray palette 256 */
#define  GP64          7                    /* gray palette 64 */
#define  GP16          8                    /* gray palette 16 */
#define  GP8           8                    /* gray palette 8 */
//#define  GP8           9                    /* gray palette 8 */
#define  BP            10                   /* bw palette 2 color 8 bit */
#define  IP256         11                   /* image palette 256 color */

#define  PALTYPE       1

#define  SETPAL        0
#define  SETDITH       1

/*--- Devices --- */

#define  MAXDEV        3                    /* wDevice states */      
#define  D8514         0                    /* 8514 extended vga display */
#define  DEVGA         1                    /* extended vga display */
#define  DVGA          2                    /* standard vga display */
#define  DUNKNOWN      3                    /* unsupported display device */

/*---  Global Buffers & Capture ---*/


#define     BUFOFFSET          65536L
#define     GLOBALBUFSIZE      65536L
#define     SMALLBUFOFFSET     32768L
#define     LARGEBUFOFFSET     65536L

#define CAP_SCREEN                1
#define CAP_WINDOW                2
#define CAP_CLIENT                3
#define CAP_AREA                  4
#define REGION_SCREEN             1
#define REGION_WINDOW             2
#define REGION_CLIENT             3
#define REGION_AREA               4

#define  CAPBUFOFFSET        4096L


/*--- FILEINFO Structure --- */

typedef struct tagFileInfo
{
    WORD wScanWidth;
    WORD wScanHeight;
    WORD wBitsPerPixel;
    WORD wPaddedScanWidth;
    WORD wBytesPerRow;
    WORD wPaddedBytesPerRow;
    WORD wRowsPerStrip;
    WORD wRowsPerLastStrip;
    WORD wBytesPerLastStrip;
    WORD wBytesPerStrip;
    WORD wNumStrips;
    DWORD dwDataOffset;
    BOOL bIsLastStrip;
    char HistFile [128];
    FARPROC lpfnFlushRead;
}   FILEINFO;

typedef FILEINFO FAR * LPFILEINFO;

/*--- DISPLAY INFO Structure ---*/

typedef struct tagDispInfo
{
    WORD wBitsPerPixel;
    WORD wPaddedScanWidth;
    WORD wBytesPerRow;
    WORD wPaddedBytesPerRow;
    WORD wBytesPerPlane;
    BOOL bIsPalette; 
    WORD wDisplayColorsMode;
    WORD wColorToGrayMode;
    HANDLE hDispPalette;
    HANDLE hErrorBuf;
} DISPINFO;

typedef DISPINFO FAR * LPDISPINFO;


#define INTERNAL_BITS               8

#define DITHER_FS_COLOR             1   
#define DITHER_BURKE_COLOR          2   
#define DITHER_FAST_COLOR           3
#define DITHER_JARVIS_COLOR         4
#define DITHER_STUCKI_COLOR         5
#define DITHER_STEVENSON_COLOR      6

#define SCAN_OPEN                   0
#define USER_OPEN                   1       // User opens from menu
#define COMMAND_LINE_OPEN           2       // User opens from command line
#define TOOLS_OPEN                  3       // Tools opens with its output
#define OIC_OPEN                    4
#define AUTO_CONVERT_OPEN           5
#define CLIPBOARD_OPEN              6
#define UNDO_OPEN                   7
#define GENERIC_OPEN                8

// What do we do with this ????

int FAR PASCAL CompressCPI (int, LPSTR);

int FAR PASCAL DecompressCPI (int, LPSTR);
                                   
BOOL FAR PASCAL TestCompressedCPI (int, LPSTR);


#define WINDOW_TOP          0
#define WINDOW_LEFT         2
#define WINDOW_BOTTOM       4
#define WINDOW_RIGHT        6

