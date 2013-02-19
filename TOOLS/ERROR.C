
/*---------------------------------------------------------------------------

    ERROR.C - 
                
               

    CREATED:    2/91   D. Ison

--------------------------------------------------------------------------*/

/*  Undefs to expedite compilation */

  
#define  NOKANJI
#define  NOPROFILER
#define  NOSOUND
#define  NOCOMM
#define  NOGDICAPMASKS
#define  NOVIRTUALKEYCODES
#define  NOSYSMETRICS
#define  NOKEYSTATES   
#define  NOSYSCOMMANDS 
#define  NORASTEROPS   
#define  OEMRESOURCE   
#define  NOATOM       
#define  NOCLIPBOARD   
#define  NOCOLOR          
#define  NODRAWTEXT   
#define  NOMETAFILE      
#define  NOSCROLL        
#define  NOTEXTMETRIC     
#define  NOWH
#define  NOHELP           
#define  NODEFERWINDOWPOS 

#include <windows.h>
#include "errtable.h"
#include "internal.h"
#include "global.h"

#define MAXERRSIZE  256

int FAR PASCAL ErrorMsg (nErrorCode, nCaption)
int nErrorCode;
int nCaption;
{
    HANDLE hStringBuf1;
    HANDLE hStringBuf2;
    PSTR   pStringBuf1;
    PSTR   pStringBuf2;

    if (nErrorCode == USER_ABANDON)
        return (FALSE);

    if (nCaption == NULL)
        nCaption = ERR_ERROR;

    hStringBuf1 = LocalAlloc (LHND, MAXERRSIZE);
    pStringBuf1 = LocalLock (hStringBuf1);

    hStringBuf2 = LocalAlloc (LHND, MAXERRSIZE);
    pStringBuf2 = LocalLock (hStringBuf2);

    LoadString (hInstance, nErrorCode, (LPSTR) pStringBuf1, MAXERRSIZE);
    LoadString (hInstance, nCaption,  (LPSTR) pStringBuf2, MAXERRSIZE);

    #ifdef NEVER
    {
        HWND hWhatWnd = GetFocus();
        hWhatWnd = GetFocus();
    }
    #endif

    MessageBox (NULL, pStringBuf1, pStringBuf2, MB_OK | MB_ICONSTOP);

    LocalUnlock (hStringBuf1);
    LocalUnlock (hStringBuf2);

    LocalFree   (hStringBuf1);
    LocalFree   (hStringBuf2);

    return (FALSE);
}
