/* CPI file reader/writer stuff  */


typedef struct tagIMAGEINFO
{
    int         hFile;
    WORD        wScanWidth;
    WORD        wScanHeight;
    WORD        wImageClass;
    WORD        wBytesPerPixel;
    WORD        wBytesPerRow;
    WORD        wCMAP;
    HANDLE      hPalette;
    LPSTR       lpPalette;
    WORD        wCPIImageType;
    WORD        wCPIBitsPerPixel;
    WORD        wCPIBytesPerRow;
    DWORD       dwCPIPalOffset;
    DWORD       dwCPIImgOffset;
}   IMAGEINFO;

typedef IMAGEINFO NEAR  *PIMAGEINFO;
typedef IMAGEINFO FAR   *LPIMAGEINFO;



typedef HANDLE  HIMAGE;

HIMAGE FAR  OpenImage     (LPSTR, WORD);
HIMAGE FAR  CreateImage   (LPSTR, HIMAGE, WORD, WORD, WORD);
int FAR     ReadLine      (HIMAGE, LPSTR);
int FAR     WriteLine     (HIMAGE, LPSTR);
int FAR     CloseImage    (HIMAGE, BOOL);
WORD NEAR   GetImageClass (WORD);
BOOL NEAR   TestCPI       (int, LPSTR);
int FAR     GetImageInfo  (HIMAGE, LPIMAGEINFO);
int FAR     SetImageInfo  (HIMAGE, LPIMAGEINFO);
int FAR     RewindImage   (HIMAGE);

