/*  Hello world  */

#define EDGE_ENHANCE   0
#define EDGE_LINE      1

int FAR PASCAL SharpenCPI      (HWND, LPSTR, LPSTR, int);
int FAR PASCAL SmoothCPI       (HWND, LPSTR, LPSTR, int);
int FAR PASCAL EnhanceEdgeCPI  (HWND, LPSTR, LPSTR, int);
int FAR PASCAL RotateCPI       (HWND, LPSTR, LPSTR, int);
int FAR PASCAL ScaleCPI        (HWND, LPSTR, LPSTR, int);
int FAR PASCAL FlipCPI         (HWND, LPSTR, LPSTR, int);
int FAR PASCAL MirrorCPI       (HWND, LPSTR, LPSTR, int);

int FAR PASCAL FlushTools (void);
