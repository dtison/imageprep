#include <windows.h>
#define MARGIN 16

BOOL ShowLogo (HWND hWnd)
{
  HDC         hDC;
  HANDLE      hInstance;
  HANDLE      hMF;
  HANDLE      hResource;
  HANDLE      hMemMF;
  RECT        rcClient;
  BOOL        bSuccess = 0;
  /*
  **  Get instance
  */
  hInstance = GetWindowWord (hWnd, GWW_HINSTANCE);
  /*
  **  Get DC and save current settings
  */
  if (hDC = GetDC (hWnd)){
    SaveDC (hDC);
    GetClientRect (hWnd, (LPRECT)&rcClient);
    /*
    **  Set DC attributes
    */
    SetMapMode (hDC, MM_ISOTROPIC);
    SetStretchBltMode (hDC, COLORONCOLOR);
//    SetWindowOrg (hDC, rcClient.right / 4, rcClient.bottom / 4);
//    SetWindowExt (hDC, rcClient.right * 4, rcClient.bottom * 4);
    SetWindowOrg (hDC, (rcClient.right - rcClient.left) / MARGIN ,
        (rcClient.bottom - rcClient.top) / MARGIN);
    SetWindowExt (hDC,
        rcClient.right - (rcClient.right - rcClient.left) / MARGIN ,
        rcClient.bottom - (rcClient.bottom - rcClient.top) / MARGIN);
    SetViewportOrg (hDC, 0, 0);
    SetViewportExt (
      hDC, 
      rcClient.right  - rcClient.left, 
      rcClient.bottom - rcClient.top);
    /*  
    **  Find logo metafile resource 
    */
    hResource = FindResource (hInstance, (LPSTR)"LOGO", (LPSTR)"METAFILE");
    /*  
    **  Load metafile resource: not actually loaded until LockResource 
    */
    hMemMF = LoadResource (hInstance, hResource);
    /*  
    **  Physically load resource with LockResource
    */
    LockResource (hMemMF);
    /*  
    **  Set resource to metafile and play it 
    */
    hMF = SetMetaFileBits (hMemMF);
    bSuccess = PlayMetaFile (hDC, hMF);
    /*  
    **  Unlock and free resource memory 
    */
    UnlockResource (hMemMF);
    FreeResource (hMemMF);
    /* 
    **  Restore device context 
    */
    /*  
    **  Invalidate metafile handle   
    */
    /*
    **  this seems to cause lockups because hMF is no longer valid
    DeleteMetaFile (hMF);
    */
    RestoreDC (hDC, -1);
    ReleaseDC (hWnd, hDC);
  }
  return (bSuccess);
}
