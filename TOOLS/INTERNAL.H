
/*--------------------------------------------------------------------

    TOOLS.DLL internal include file  

    2/21/91  D. Ison

    All kinds of stuff that ain't exported or made available to other
    applications.

---------------------------------------------------------------------*/


/*  Constants and other defines  */

#define MAXSTRINGSIZE   256
#define MAXSTRUCTSIZE   64  
#define MAXPATHSIZE     128
#define ISAMODIFIED     (ModiFlags &  0x1)
#define AISMODIFIED     (ModiFlags |= 0x1)
#define AISNOTMODIFIED  (ModiFlags &= 0xFFFE)

#define ISBMODIFIED     ((ModiFlags &  0x2) >> 1)
#define BISMODIFIED     (ModiFlags |= 0x2)
#define BISNOTMODIFIED  (ModiFlags &= 0x0)

#define TOOLS_OPEN      3

#define FILESPEC        1
#define DRIVESPEC       2
#define DIRSPEC         3

#define UNSHARP         1
#define HIGHPASS        2
#define MEDIAN          1
#define LOWPASS         2
#define CLOCKWISE       1
#define CCLOCKWISE      2

#define TRUNCATION      1
#define AVERAGING       2
#define AVGINTERPOLATE  3

#define PIXELS          1
#define INCHES          2
#define CENTIMETERS     3
#define MILLIMETERS     4
#define POINTS          5
#define PICAS           6




#define WM_SHOWTEXT     WM_USER + 1
#define WM_SHOWIMAGE    WM_USER + 100
#define IDM_CLOSE       101

#define CMAP_TO_8       1
#define CMAP_TO_24      3 

#define MONOCHROME      1
#define GRAYSCALE       2
#define COLORMAP        3
#define TRUECOLOR       4

#define CI_FLIP         0x01
#define CI_MIRROR       0x02 


/*  Filter Structures & Prototypes */

/* --- Sharpen ---  */

typedef struct tagSHARPEN
{
    PSTR pSource;
    PSTR pDest;
    PSTR pTemp;
    BYTE Level;
    BYTE SharpFilter;  
}   SHARP;

typedef SHARP NEAR *PSHARP;
typedef SHARP FAR  *LPSHARP;

int  NEAR SharpenImage (HWND, PSHARP);
void NEAR SharpenLine (LPSTR, LPSTR, LPSTR, LPSTR, WORD, WORD, WORD, PSHARP);
void NEAR InitSharpen (HWND, LONG);


/* --- Smooth  ---  */

typedef struct tagSMOOTH 
{
    PSTR pSource;
    PSTR pDest;
    PSTR pTemp;
    BYTE SmoothFilter;  
    BYTE Level;
}   SMOOTH;

typedef SMOOTH NEAR *PSMOOTH;
typedef SMOOTH FAR  *LPSMOOTH;

int  NEAR SmoothImage (HWND, PSMOOTH);
void NEAR SmoothLine  (LPSTR, LPSTR, LPSTR, LPSTR, WORD, WORD, WORD, PSMOOTH);
void NEAR InitSmooth  (HWND, LONG);


/* --- Edge  ---  */

typedef struct tagEDGE
{
    PSTR pSource;
    PSTR pDest;
    PSTR pTemp;
    BYTE Level;
    BYTE Sensitivity;  
    BYTE Filter;        // Edge enhance or edge detect
}   EDGE;

typedef EDGE NEAR *PEDGE;
typedef EDGE FAR  *LPEDGE;

int  NEAR EnhanceEdgeImage (HWND, PEDGE);
void NEAR EnhanceEdgeLine  (LPSTR, LPSTR, LPSTR, LPSTR, WORD, WORD, WORD, PEDGE);
void NEAR InitEnhanceEdge  (HWND, LONG, int);


/* --- Rotate  ---  */

typedef struct tagROTATE  
{
    PSTR pSource;
    PSTR pDest;
    PSTR pTemp;
    BYTE Direction;
}   ROTATE;

typedef ROTATE NEAR *PROTATE;
typedef ROTATE FAR  *LPROTATE;

int  NEAR RotateImage (HWND, PROTATE);
void NEAR InitRotate (HWND, LONG);


/* --- Scale  ---  */

typedef struct tagSCALE
{
    PSTR pSource;
    PSTR pDest;
    PSTR pTemp;
    BYTE ScaleProcess;
    BYTE ScaleUnits;
    WORD wResolution;           // Output resolution
    WORD wXPercent;
    WORD wYPercent;
    WORD wXPixels;
    WORD wYPixels;
    WORD wScanWidth;
    WORD wScanHeight;

    WORD wTmpXPixels;
    WORD wTmpYPixels;

}   SCALE;

typedef SCALE NEAR *PSCALE;
typedef SCALE FAR  *LPSCALE;

int  NEAR ScaleImage (HWND, PSCALE);
void NEAR InitScale  (HWND, LONG);


/* --- Flip  ---  */

typedef struct tagFLIP 
{
    PSTR pSource;
    PSTR pDest;
    PSTR pTemp;
}   FLIP;

typedef FLIP NEAR *PFLIP;
typedef FLIP FAR  *LPFLIP;

int  NEAR FlipImage (HWND, PFLIP);
void NEAR InitFlip   (HWND, LONG);


/* --- Mirror ---  */

typedef struct tagMIRROR 
{
    PSTR pSource;
    PSTR pDest;
    PSTR pTemp;
}   MIRROR;

typedef MIRROR NEAR *PMIRROR;
typedef MIRROR FAR  *LPMIRROR;

int  NEAR MirrorImage (HWND, PFLIP);  // FLIP because flip & mirror are exactly the same.
void NEAR InitMirror (HWND, LONG);


/*  Exports...   */

DWORD FAR PASCAL SharpDlgProc (HWND, unsigned, WORD, LONG);
DWORD FAR PASCAL SmoothDlgProc (HWND, unsigned, WORD, LONG);
DWORD FAR PASCAL EdgeDlgProc (HWND, unsigned, WORD, LONG);
DWORD FAR PASCAL RotateDlgProc (HWND, unsigned, WORD, LONG);
DWORD FAR PASCAL ScaleDlgProc (HWND, unsigned, WORD, LONG);
DWORD FAR PASCAL FlipDlgProc  (HWND, unsigned, WORD, LONG);
DWORD FAR PASCAL MirrorDlgProc  (HWND, unsigned, WORD, LONG);
void  FAR PASCAL Abandon (void);


/*  Non exports... */

int  FAR PASCAL CleanFileSpec (LPSTR, LPSTR);
int  FAR PASCAL CheckDiskSpace (int, DWORD);
int  FAR PASCAL ErrorMsg (int, int);
void FAR PASCAL GetNameFromPath (LPSTR, LPSTR);



