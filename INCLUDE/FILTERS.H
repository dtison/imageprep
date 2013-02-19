
/************************** FILTER STRUCTS **********************************/



typedef struct
{
    WORD wInputBytesPerRow;   //  Bytes per row on input side  (May be true 24 bit DIB data
                              //  or 3 * lpDispInfo -> wPaddedScanWidth)
    WORD wPaddedScanWidth;    //  Pixel Resolution for DIB aligned version of this data
    WORD wRowsThisStrip;      //  Number scanlines this strip

}   UNIFORMQUANT;

UNIFORMQUANT FAR *lpUniformQuantPtr;


typedef struct
{

    WORD wInputBytesPerRow;   //  Bytes per row on input side  (May be true 24 bit DIB data
                              //  or 3 * lpDispInfo -> wPaddedScanWidth)
    WORD wPaddedScanWidth;    //  Pixel Resolution for DIB aligned version of this data
    WORD wRowsThisStrip;      //  Number scanlines this strip
    WORD wScrnY;
    WORD wColors;

}   DITHERBAYER;

DITHERBAYER FAR *lpDitherBayerPtr;


typedef struct
{

    WORD    wInputBytesPerRow;
    WORD    wPaddedScanWidth;
    WORD    wRowsThisStrip;
    HANDLE  hErrBuf;

}   QUANTFSDITHER;

typedef struct
{

    WORD  wInputBytesPerRow;
    WORD  wPaddedScanWidth;
    WORD  wRowsThisStrip;
 
}   QUANTOPR;




typedef struct
{

    WORD  wInputBytesPerRow;
    WORD  wPaddedScanWidth;
    WORD  wRowsThisStrip;

}   RGBTOGRAY;

RGBTOGRAY FAR *lpRGBToGrayPtr;

typedef struct
{

    WORD wBytesThisStrip;
    WORD wBitsToShift;

}   SHIFTGRAY;

SHIFTGRAY FAR *lpShiftGrayPtr;


typedef struct
{

    LPSTR lpPalette;
    WORD  wBytesThisStrip;

}   FMT8TO24;

typedef struct
{

    WORD  wBytesThisStrip;

}   GRAYTOBW;



/***************************** FILTERS.C ************************************/

int    SetupFilters (LPFILEINFO, int );
int    ResetFilters( HANDLE, int );
HANDLE GetLEVL( LPFILEINFO, BOOL );
HANDLE GetGSUM( LPFILEINFO, BOOL );
HANDLE GetDIDX( LPFILEINFO, BOOL );
HANDLE GetBAY8( LPFILEINFO, BOOL, BOOL);
HANDLE GetGBAY( LPFILEINFO, BOOL );
HANDLE GetUQUA( LPFILEINFO, BOOL );
HANDLE GetREDU( LPFILEINFO, BOOL );
HANDLE GetUQUA8( LPFILEINFO, BOOL );
HANDLE GetG2BW( LPFILEINFO, BOOL );
HANDLE GetFS( LPFILEINFO, BOOL , HANDLE );
HANDLE GetDGFS( LPFILEINFO, BOOL , HANDLE );


/************************** FILTERS LIBRARY *********************************/

int FAR PASCAL QuantColorReduced( LPSTR FAR *, LPSTR FAR *, LPSTR );
int FAR PASCAL QuantFSDither( LPSTR FAR *, LPSTR FAR *, LPSTR );

int FAR PASCAL DitherBayer_A( LPSTR FAR *, LPSTR FAR *, LPSTR );
int FAR PASCAL UniformQuant8_A( LPSTR FAR *, LPSTR FAR *, LPSTR );
int FAR PASCAL UniformQuant_A( LPSTR FAR *, LPSTR FAR *, LPSTR );
int FAR PASCAL DitherGrayBayer( LPSTR FAR *, LPSTR FAR *, LPSTR );
int FAR PASCAL DitherGrayFS( LPSTR FAR *, LPSTR FAR *, LPSTR );
int FAR PASCAL GrayToBW( LPSTR FAR *, LPSTR FAR *, LPSTR );
int FAR PASCAL Format_8To24( LPSTR FAR *, LPSTR FAR *, LPSTR );
int FAR PASCAL GraySum( LPSTR FAR *, LPSTR FAR *, LPSTR );
int FAR PASCAL ShiftGrayLevels( LPSTR FAR *, LPSTR FAR *, LPSTR );
int FAR PASCAL BytesTo3Plane( LPSTR FAR *, LPSTR FAR *, LPSTR );



int FAR dither_fsgray   (LPSTR, LPSTR, WORD, WORD, WORD, char *);
void FAR dither_fscolor (LPSTR, LPSTR, WORD, WORD, WORD, PSTR, LPSTR, 
                         RGBQUAD FAR *, WORD);

void FAR dither_facolor (LPSTR, LPSTR, WORD, WORD, WORD, PSTR, LPSTR, 
                         RGBQUAD FAR *, WORD);


void FAR dither_bkcolor (LPSTR, LPSTR, WORD, WORD, WORD, PSTR, LPSTR, 
                          RGBQUAD FAR *, WORD);
