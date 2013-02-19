

typedef struct
{
    char SourcePathA  [128];
    char SourcePathB  [128];
    char DestPath     [128];
    char FilenameA    [13];
    char FilenameB    [13];
    char DestFilename [13];
    int nX1;
    int nY1;
    int nExtX1;
    int nExtY1;
    int nX2;
    int nY2;
    int nExtX2;
    int nExtY2;
    int nX3;
    int nY3;
    int nPtX1;
    int nPtY1;
    int nPtX2;
    int nPtY2;
    WORD wMode;
    WORD wView;

}  MERGESTRUCT;

typedef MERGESTRUCT     *PMERGESTRUCT;
typedef MERGESTRUCT FAR *LPMERGESTRUCT;


#define UNION       1
#define INTERSECT   2

#define DRAW_AREA   1
#define MERGE_POINT 2



#define     WM_RESETVIEWMODE    WM_USER+1
#define     WM_SETMERGEPOINT    WM_USER+2
#define     WM_SETMERGERECT     WM_USER+3
#define     WM_VALIDATEMERGE    WM_USER+4
#define     WM_SHOWTEXT         WM_USER+5


int FAR PASCAL MergeImages (LPSTR, LPSTR, int, int, int, int, 
                            LPSTR, int, int, int, int, 
                            int, int, WORD);


int GetNextSourceALine (int, LPCPIIMAGESPEC, LPSTR, int, int, int, int, int);
int GetNextSourceBLine (int, LPCPIIMAGESPEC, LPSTR, int, int, int, int, int);
int FAR InitCPIFile (int, LPCPIFILESPEC, LPCPIIMAGESPEC, int, int);
void FAR InitMerge (HWND);

int FAR DoMerge (void);
