#include <windows.h>
#include <cpi.h>
#include <io.h>
#include <string.h>
#include "internal.h"
#include "global.h"

int FAR PASCAL CleanFileSpec (lpDest, lpSource)
LPSTR lpDest;
LPSTR lpSource;
{
    int     i;
    char    ch;
    int     nRetval = FILESPEC;         // Assume it is a file spec
    BOOL    bIsFileSpec = TRUE;
    BOOL    bIsDriveSpec = FALSE;

    i = 0;

    while (*lpSource != 0 && i < 128 && ! bIsDriveSpec)
    {
        ch = *lpSource++;

        if (ch != '[' && ch != ']')  // Skip over [ or ]
        {
            lpDest [i] = ch;
            i++;    
        }
        else
        {
            if (*lpSource == '-')           // Is a drive spec ?
                if (*(lpSource + 2) == '-') // Make sure
                {
                    lpDest [0] = *(lpSource + 1);
                    lpDest [1] = ':';
                    lpDest [2] = 0;
                    bIsFileSpec  = FALSE;
                    bIsDriveSpec = TRUE;
                }
            bIsFileSpec = FALSE;
        }
    }

    if (! bIsDriveSpec)
        lpDest [i] = 0;
                                
    if (bIsDriveSpec)
        nRetval = DRIVESPEC;
    else
        if (! bIsFileSpec)
            nRetval = DIRSPEC;


    return (nRetval);

}

int FAR PASCAL CheckDiskSpace (hFile, dwSize)
int   hFile;
DWORD dwSize;
{
    DWORD dwTmp;
    char Buffer [4];
    int  nRetval;

    dwTmp = tell (hFile);

    _llseek (hFile, 0L, 0);
    _llseek (hFile, (dwSize - 4L), 1);

    if (WriteFile (hFile, (LPSTR) Buffer, 4L) != 4L)
        nRetval = FALSE;
    else 
        nRetval = TRUE;

    _llseek (hFile, dwTmp, 0);

    return (nRetval);
}




void FAR PASCAL GetNameFromPath (lpFileName, lpPathName)
LPSTR lpFileName;
LPSTR lpPathName;
{
    PSTR          pStringBuf;
    PSTR          pStringBuf2;

    pStringBuf  = LocalLock (hStringBuf);
    pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

    SeparateFile ((LPSTR)pStringBuf2, (LPSTR) pStringBuf, (LPSTR) lpPathName);

    _fstrncpy (lpFileName, (LPSTR) pStringBuf, 12);

    LocalUnlock (hStringBuf);
}
