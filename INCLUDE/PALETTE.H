int NEAR PASCAL MakeBinaryPalette (LPSTR);
int NEAR PASCAL MakeDefaultPalette (LPSTR, WORD, LPFILEINFO);
int NEAR PASCAL MakeGrayPalette (LPSTR, int);
int NEAR PASCAL MakeImportPalette (LPSTR, LPBITMAPINFO);

// No more HANDLES  3.1..

int NEAR PASCAL MakeOptimizedPalette (LPSTR, int, LPFILEINFO, LPBITMAPINFO, LPSTR, int, BOOL);
int ReduceRGBToCM (HWND, int, LPFILEINFO, LPBITMAPINFO, LPSTR, LPSTR, RGBQUAD FAR *, BOOL, int);

int NEAR PASCAL GenerateDefaultPalette (LPSTR, int);




