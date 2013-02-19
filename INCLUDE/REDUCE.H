/*--- Reduction ---*/

#define  COLOR_DEPTH    5
#define  B_DEPTH        5                   /* # bits/pixel to use */
#define  B_LEN          (1<<B_DEPTH)
#define  C_DEPTH        2
#define  C_LEN          (1<<C_DEPTH)        /* # cells/color to use (4) */
#define  WAS_NUM_COLORS 254 


#define MAXDISTANCE       65535
#define MAXCOLOR          256
#define MAXCOLORMAPSIZE   256 
#define COLORDEPTH        8         // 5
#define CELLDEPTH         2
#define CELLLENGTH        (1 << CELLDEPTH)
#define BITDEPTH          5
#define BITLENGTH         (1 << BITDEPTH)

#define PAL_MEDIAN            1             /* Median Cut                 */
#define PAL_AVERAGE           2             /* Region averaging           */
#define PAL_POPULARITY        3             /* Popularity                 */
#define PAL_MEAN              4             /* Popularity Averaging       */
#define PAL_MEDAVG            5             /* Median Cut with popularity */
#define PAL_MEDPOP            6
#define PAL_MEDPOPAVG         7
#define PAL_MEDMEANPOP        8
#define PAL_MEDMEANPOPAVG     9
#define PAL_NEWMEDIAN         10            /* ColorBox median            */
#define PAL_TEST              99
#define PAL_REDUCTION         PAL_MEDIAN

typedef struct tagColorbox                 /*  There are 256 "color boxes"  */
{
  struct tagColorbox FAR *next;
  struct tagColorbox FAR *prev;
  WORD   RedMin;
  WORD   RedMax;
  WORD   GreenMin;
  WORD   GreenMax;
  WORD   BlueMin;
  WORD   BlueMax;
  DWORD  Total;
}COLORBOX;                                /*  represents.  (Sampling size) */

typedef COLORBOX      *PCOLORBOX;
typedef COLORBOX NEAR *NPCOLORBOX;
typedef COLORBOX FAR  *LPCOLORBOX;

typedef struct tagCOLORCELL
{
  WORD  ccNumEntries;
  WORD  ccEntries[MAXCOLORMAPSIZE][2];

}COLORCELL;
typedef COLORCELL      *PCOLORCELL;
typedef COLORCELL NEAR *NPCOLORCELL;
typedef COLORCELL FAR  *LPCOLORCELL;





/***************************** REDUCTION PROTOTYPES   *******************************/

static      assign_color (LPCOLORBOX, unsigned char *, unsigned char *,
                      unsigned char * );
LONG        count_unique (LPSTR, WORD FAR *);
LPCOLORCELL create_colorcell ( long, long, long );
BYTE 				 find_closesta (WORD, WORD, WORD, LPSTR, WORD FAR *, WORD);


int          GenerateHistogram (HWND, int, LPFILEINFO, LPBITMAPINFO, LPSTR, LPSTR);


int          GeneratePalette (HWND, LPSTR, LPSTR, RGBQUAD FAR *);
long near    get_hist ( long );
LPCOLORBOX   largest_box( void );
int          map_colortable( void );
int          MapDithRGBSpace ( HWND, LPSTR, LPSTR );
int          MapRGBSpace ( HWND, LPSTR, LPSTR );
void         process_scan( LPSTR, LPSTR, WORD FAR *, int, WORD, int *, 
                           int *, int *, int *, int *, int *);


void         shrinkbox (COLORBOX FAR *);
int          SortPalette (void);
void         splitbox  (COLORBOX FAR *);
void         set_hist ( long, long );

void FAR quantize_line (LPSTR, LPSTR, WORD, WORD, WORD);


void near get_squares( int,int,int,long*,long*,long* );


