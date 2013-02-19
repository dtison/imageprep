
#include <windows.h>
#include <imgprep.h>
#include <proto.h>
#include <global.h>
#include <cpifmt.h>
#include <io.h>
#include <stdio.h>
#include <cpi.h>
#include <oic.h>
#include <error.h>
#include <stdlib.h>
#include <strtable.h>

#define  MSG_SAVE_PERCENT    0x0411
#define  MSG_DONE_REPORT     0x0412

BOOL bTestMode = FALSE;

LPSTR lpHashTable;
LPSTR lpLZWIOBuf;
LPSTR lpLZWDecodeBuf;
LPSTR lpXFRMBuf;

DWORD   dwCPIPalPos;
DWORD   dwCPIDataPos; 


#define GOFAST
#define OPT_OICMEMORY

#define SquashDiscardResource(r)      _asm    mov   ax,r     \
                                      _asm    jmp   SquashDiscardResource

int FAR CPIFixupFile (int, LPSTR, LPCPIIMAGESPEC);

int FAR PASCAL CompressCPI (hInFile, lpDestPath) 
int   hInFile; 
LPSTR lpDestPath;
{
    HANDLE       hBuffer1    = (HANDLE) 0;
    HANDLE       hBuffer2    = (HANDLE) 0;
    HANDLE       hPalBuffer  = (HANDLE) 0;
    HANDLE       hHashTable  = (HANDLE) 0;

    LPSTR        lpBuffer1   = (LPSTR) 0;
    LPSTR        lpBuffer2   = (LPSTR) 0;
    LPSTR        lpPalBuffer = (LPSTR) 0;
    int          hOutFile    = (int)   0;
    PSTR         pStringBuf  = (PSTR)  0;
    PSTR         pStringBuf2 = (PSTR)  0;

    int          Retval = -1;
    int          nTmpLevel;
    CPIFILESPEC  CPIFileSpec;
    CPIIMAGESPEC CPIImageSpec;

    WORD         wScanWidth;
    WORD         wScanHeight;
    WORD         wBytesPerRow;
    WORD         wBytesPerPixel;
    LONG         lNumBytes;
    OICMETER     OICMeter;

    /*  Initialize to flag locks  */

    lpHashTable     = (LPSTR) 0;


  
    /*  First let's do our memory allocations  */
  

    #ifndef OPT_OICMEMORY
    hBuffer1 = GlobalAlloc (GHND, 32768);
    if (! hBuffer1)
        return (Retval);
  
    lpBuffer1 = GlobalLock (hBuffer1);  
    if (lpBuffer1 == NULL)
    {
        GlobalFree (hBuffer1);
        return (Retval);
    }
  
    hBuffer2 = GlobalAlloc (GHND, 32768);
    if (! hBuffer2)
    {
        GlobalUnlock (hBuffer1);
        GlobalFree (hBuffer1);
        return (Retval);
    }

    lpBuffer2 = GlobalLock (hBuffer2);  
    if (lpBuffer2 == NULL)
    {
        GlobalUnlock (hBuffer1);
        GlobalFree (hBuffer1);
        GlobalFree (hBuffer2);
        return (Retval);
    }
    #else
    
    lpBuffer1 = GlobalWire (hGlobalBufs);
    if (! lpBuffer1)
        SquashDiscardResource (EC_NOMEM);


// New 3.1 memory mgt stuff

//  hBuffer2 = GlobalAlloc (GHND, 65535);
//  if (! hBuffer2)
//      SquashDiscardResource (-1);

//  lpBuffer2 = GlobalWire (hBuffer2);  
//  if (lpBuffer2 == NULL)
//      SquashDiscardResource (EC_NOMEM);

    lpBuffer2 = (BYTE huge *) lpBuffer1 + LARGEBUFOFFSET;

    #endif

  
    hPalBuffer = GlobalAlloc (GHND, 1024);
    if (! hPalBuffer)
        SquashDiscardResource (EC_NOMEM);
  
    lpPalBuffer = GlobalLock (hPalBuffer);  
    if (lpPalBuffer == NULL)
        SquashDiscardResource (EC_NOMEM);
  
    hHashTable = GlobalAlloc (GHND, 40962);
    if (! hHashTable)
        SquashDiscardResource (EC_NOMEM);

    lpHashTable = GlobalLock (hHashTable);  
    if (lpHashTable == NULL)
        SquashDiscardResource (EC_NOMEM);

    pStringBuf  = LocalLock (hStringBuf);
    pStringBuf2 = pStringBuf + MAXSTRINGSIZE;


    hOutFile = _lcreat (lpDestPath, 0);
        
    /*  Get system metrics & image file sizes  */
        
    ReadCPIHdr (hInFile, (LPCPIFILESPEC) &CPIFileSpec,(LPCPIIMAGESPEC) &CPIImageSpec);
        
    wScanWidth  = CPIImageSpec.X_Length;
    wScanHeight = CPIImageSpec.Y_Length;


    /*  Make sure we can actually compress the thing by checking size 
        and data format  */    

    if (wScanWidth > 2000)
        SquashDiscardResource (EC_OIC_IMAGE_TOO_WIDE);

    if (CPIImageSpec.StorageType != CPIFMT_TRIPLETS) 
        SquashDiscardResource (EC_OIC_IMAGE_IS_LINES);

    {
        FARPROC   lpfnDlg;              
        BOOL         success;
  
        lpfnDlg = MakeProcInstance (CompDlg, hInstIP);
        success = DialogBox (hInstIP, (LPSTR) "COMPSETUP", hWndIP, lpfnDlg);
        FreeProcInstance (lpfnDlg);
  
        if (! success)
            SquashDiscardResource (USER_ABANDON);

    }     
  
    nTmpLevel = CompLevel;              // Save for prometer message
    CompLevel = (CompLevel - 1) * 5;    // Scale to 0 - 19, then to 0 - 95.  (95 is max compression)

    /*  Point to actual image data  */

    lseek (hInFile, CPIImageSpec.ImgDataOffset, SEEK_SET);

        
    /*  Initialize Header  */
        
    InitCPIFileSpec ((LPCPIFILESPEC) &CPIFileSpec);
        
    lNumBytes = (LONG) sizeof (CPIFILESPEC);

    if (WriteFile (hOutFile, (LPSTR) &CPIFileSpec, lNumBytes) != lNumBytes)
        SquashDiscardResource (EC_WRHDR);
            
        
    /*  Write out Image Spec stuff  */

    InitCPIImageSpec ((LPCPIIMAGESPEC) &CPIImageSpec);
            
    CPIImageSpec.ImgDescTag       = 1;
    CPIImageSpec.ImgSpecLength    = sizeof (CPIIMAGESPEC);
    CPIImageSpec.X_Origin         = 0;
    CPIImageSpec.Y_Origin         = 0;   
    CPIImageSpec.X_Length         = wScanWidth;
    CPIImageSpec.Y_Length         = wScanHeight;
    CPIImageSpec.BitsPerPixel     = 24;
    CPIImageSpec.NumberPlanes     = 1;
    CPIImageSpec.NumberColors     = 256;
    CPIImageSpec.Orientation      = 0;    
    CPIImageSpec.Compression      = 2;    // OIC
    CPIImageSpec.ImgSize          = (DWORD) ((DWORD) wScanWidth * 
                                            (DWORD) wScanHeight * 3L);
    CPIImageSpec.ImgSequence      = 0;
    CPIImageSpec.NumberPalEntries = 0;
    CPIImageSpec.NumberBitsPerEntry = 24;
    CPIImageSpec.AttrBitsPerEntry = 0;
    CPIImageSpec.FirstEntryIndex  = NULL;
    CPIImageSpec.RGBSequence      = 0;
    CPIImageSpec.NextDescriptor   = NULL;
    CPIImageSpec.ApplSpecReserved = NULL;        
            
    CPIImageSpec.CompLevel        = (BYTE) CompLevel;
    CPIImageSpec.CodeSize         = 8;
            
    lNumBytes = (LONG) sizeof (CPIIMAGESPEC);

    if (WriteFile (hOutFile, (LPSTR) &CPIImageSpec, lNumBytes) != lNumBytes)
        SquashDiscardResource (EC_WRHDR);
    
            
    /*  Find out how many strips so we know how big to make RLECounts Buffer  */
            
    {     
        WORD wRLECountsSize;
  
        wRLECounts = (wScanHeight + 7) >> 3;  // wRLECounts is same as number of strips and is a counter 
                                                     // of how many RLE byte counts will be used in the compression.
        wRLECountsSize = (wRLECounts << 1);
        lmemset (lpPalBuffer, 0, wRLECountsSize);
        dwCPIPalPos = tell (hOutFile);

        if (WriteFile  (hOutFile, lpPalBuffer, wRLECountsSize) != (LONG) wRLECountsSize)
            SquashDiscardResource (EC_WRHDR);

        dwCPIDataPos = tell (hOutFile);
    }
  
    wBytesPerPixel  = CPIImageSpec.BitsPerPixel >> 3;
    wBytesPerRow    = wScanWidth * wBytesPerPixel;

  
    CalcMatrices (CompLevel);

    LoadString (hInstIP, STR_COMPRESSING_IMAGE, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
    wsprintf (pStringBuf, pStringBuf2, nTmpLevel);
    LoadString (hInstIP, STR_OIC, (LPSTR) pStringBuf2, MAXSTRINGSIZE);

    OICMeter.hWnd        = hWndIP;
    OICMeter.hInst       = hInstIP;
    OICMeter.lpfnAbandon = Abandon;
    OICMeter.lpMessage1  = (LPSTR) pStringBuf;   
    OICMeter.lpCaption   = (LPSTR) pStringBuf2;  
    bAbandon             = FALSE;

    if (! CompressOIC (hInFile, hOutFile, lpBuffer1, lpBuffer2, wBytesPerRow, wScanWidth, wScanHeight, (WORD FAR *) lpPalBuffer, lpHashTable, (LPOICMETER) &OICMeter))
        if (bAbandon)           // User abandoned compress, return W/O err code
            SquashDiscardResource (USER_ABANDON);
        else
            SquashDiscardResource (EC_WRHDR);

  
    CPIFixupFile (hOutFile, lpPalBuffer, (LPCPIIMAGESPEC) &CPIImageSpec);


    /*  Calculate and display compression ratio  */
    {
        FARPROC   lpfnDlg;              
        BOOL         success;
        struct 
        {
            int hInFile;
            int hOutFile;
            LPSTR lpPath;
            int   nLevel;
        }   OICInfo;

        OICInfo.hInFile  = hInFile;
        OICInfo.hOutFile = hOutFile;
        OICInfo.lpPath   = lpDestPath;
        OICInfo.nLevel   = nTmpLevel;
        lpfnDlg = MakeProcInstance (OICInfoDlg, hInstIP);
        success = DialogBoxParam (hInstIP, (LPSTR) "OICINFO", hWndIP, lpfnDlg, (DWORD) (LPSTR) &OICInfo);
        FreeProcInstance (lpfnDlg);

    }     

    SquashDiscardResource (0);


/*---  Squash Resource Discard Section  ---*/

    {

        int nRetVal;

        SquashDiscardResource:

        _asm    mov nRetVal,ax


        /*  Unlocks  */


        if (lpBuffer1)
            GlobalUnWire (hGlobalBufs);

        if (lpPalBuffer)
            GlobalUnlock (hPalBuffer);

        if (lpHashTable)
            GlobalUnlock (hHashTable);

        if (pStringBuf)
            LocalUnlock (hStringBuf);


        /*  Frees */

        if (hPalBuffer)
            GlobalFree (hPalBuffer);

        if (hHashTable)
            GlobalFree (hHashTable);


        /*  File close(s)  */

        if (hOutFile){
          OFSTRUCT Of;

          _lclose (hOutFile);

          if (nRetVal < 0)
            OpenFile ((LPSTR)lpDestPath, (LPOFSTRUCT)&Of, OF_DELETE);
        }

        return (nRetVal);
    }
}


BOOL FAR PASCAL CompDlg (hDlg, message, wParam, lParam)
HWND        hDlg;
unsigned    message;
WORD        wParam;
LONG        lParam;
{



    int   Level;
    HWND  hWndControl = 0; 
    int  nScrollPos; 

    switch (message) 
    {
        case WM_INITDIALOG:
        {
            RECT rc;

            /* Center the dialog.      */

            GetWindowRect (hDlg,&rc);
            SetWindowPos  (hDlg,NULL,
                          (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2,
                          (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 3,
                           0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

        }


        CompLevel = 1;
        hWndControl = GetDlgItem (hDlg, 100);
        SetDlgItemInt (hDlg, 107, CompLevel, FALSE);
        SetScrollRange (hWndControl, SB_CTL, 1, 20, FALSE);
        SetScrollPos (hWndControl, SB_CTL, CompLevel, FALSE);
        break;

        case WM_VSCROLL:
          hWndControl = GetDlgItem (hDlg, 100);
          nScrollPos  = GetScrollPos (hWndControl, SB_CTL);

          switch (wParam)
            {
              case SB_LINEUP:
                nScrollPos--;
                break;

              case SB_LINEDOWN:
                nScrollPos++;
                break;

              case SB_PAGEUP:
                nScrollPos -= 5;
                break;

              case SB_PAGEDOWN:
                nScrollPos += 5;
                break;

         		  case SB_THUMBTRACK:
              case SB_THUMBPOSITION: 
                 nScrollPos = LOWORD (lParam);
   		          break;
            }
            if (nScrollPos < 1)
              nScrollPos = 1;
            if (nScrollPos > 20)
              nScrollPos = 20;
            /*
            nScrollPos = min (20, max (0, nScrollPos));
            CompLevel = nScrollPos == 20 ? 19 : nScrollPos;
            */
            CompLevel = nScrollPos;

          SetScrollPos (hWndControl, SB_CTL, nScrollPos, TRUE);
          SetDlgItemInt (hDlg, 107, nScrollPos, FALSE);
        break;

        /*
        case WM_PAINT:
        break;
        */
        case WM_COMMAND:
          switch (wParam)
            {
              case 107:
                if (HIWORD (lParam) == EN_KILLFOCUS)
                  {
                    Level = GetDlgItemInt (hDlg, 107, (BOOL FAR *) &Level, FALSE);
                    Level = min (20, max (1, Level));
                    CompLevel = Level;
                    /*
                    CompLevel = Level == 20 ? 19 : Level;
                    */
                    hWndControl = GetDlgItem (hDlg, 100);
                    SetScrollPos (hWndControl, SB_CTL, Level, TRUE);
                    SetDlgItemInt (hDlg, 107, Level, FALSE);
                  }
               break;
               /*
               case IDOK:
               */
               case 98:
                EndDialog (hDlg, TRUE);
                break;
               /*
               case IDCANCEL:
               */
               case 99:
                 EndDialog (hDlg, FALSE);
               break;

               default:
                 return (FALSE);
             }
           break;   

         default:
           return  (FALSE);
      }
    return (TRUE);
}






  
void InitCPIFileSpec (lpCPIFileSpec)
LPCPIFILESPEC lpCPIFileSpec;
{


    CPIFILESPEC CPIFileSpec;

 
    /*  Use a local copy to save code size  */

    lmemcpy ((LPSTR) CPIFileSpec.CPiSignature, (LPSTR) "CPI", 4);
    CPIFileSpec.VersionNumber       = 0x200;
    CPIFileSpec.FileSpecLength      = sizeof (CPIFILESPEC);
    CPIFileSpec.ImageCount          = 1;
    CPIFileSpec.ImgDescTableOffset  = sizeof (CPIFILESPEC);
    CPIFileSpec.ApplSpecReserved    = NULL;

    lmemset ((LPSTR) CPIFileSpec.Filler, 0xFF, sizeof (CPIFileSpec.Filler));

    /*  Now initialized, copy to dest buffer  */

    lmemcpy ((LPSTR) lpCPIFileSpec, (LPSTR) &CPIFileSpec, sizeof (CPIFILESPEC));


}




void InitCPIImageSpec (lpCPIImageSpec)
LPCPIIMAGESPEC lpCPIImageSpec;
{


    CPIIMAGESPEC CPIImageSpec;

 
    /*  Use a local copy to save code size  */

    /*  First set whole thing to zero  */

    lmemset ((LPSTR) &CPIImageSpec, 0, sizeof (CPIIMAGESPEC));


    /*  Now fill in non-zero default values  */


    CPIImageSpec.ImgSequence = 1;  // This is all we have now



    lmemset ((LPSTR) CPIImageSpec.Filler, 0xFF, sizeof (CPIImageSpec.Filler));

    /*  Now initialized, copy to dest buffer  */

    lmemcpy ((LPSTR) lpCPIImageSpec, (LPSTR) &CPIImageSpec, sizeof (CPIIMAGESPEC));


}


int FAR CPIFixupFile (hFile, lpPalette, lpCPIImageSpec)
int hFile;
LPSTR lpPalette;
LPCPIIMAGESPEC lpCPIImageSpec;
{


    int nRetval = TRUE;

  /*  Finish up CPI file header information and close files  */


    lpCPIImageSpec -> ImgDataOffset = dwCPIDataPos; 
    lpCPIImageSpec -> PalDataOffset = dwCPIPalPos; 

    _llseek (hFile, 64L, SEEK_SET);

    WriteFile (hFile, (LPSTR) lpCPIImageSpec, sizeof (CPIIMAGESPEC));


    if (lpCPIImageSpec -> ImgDataOffset > lpCPIImageSpec -> PalDataOffset)  
        WriteFile (hFile, lpPalette, (wRLECounts << 1));


    return (nRetval);
}



int ReadCPIHdr (infh, lpCPIFileSpec, lpCPIImageSpec) 
int infh;
LPCPIFILESPEC   lpCPIFileSpec;
LPCPIIMAGESPEC  lpCPIImageSpec;
{


    _llseek (infh, 0L, SEEK_SET);

    ReadFile (infh, (LPSTR) lpCPIFileSpec, sizeof (CPIFILESPEC));

    ReadFile  (infh, (LPSTR) lpCPIImageSpec, sizeof (CPIIMAGESPEC));

    return (1);
}


BOOL FAR PASCAL OICInfoDlg (hDlg, Message, wParam, lParam)
HWND      hDlg;
unsigned  Message;
WORD      wParam;
LONG      lParam;
{

    switch (Message)
    {
        case WM_INITDIALOG:
        {
            RECT rc;
            PSTR pStringBuf;
            PSTR pStringBuf2;
            DWORD        dwInFileSize;
            DWORD        dwOutFileSize;
            double       Ratio; 
            int          nRatio;
            int          nRatio2;
            struct OICINFO
            {
                int   hInFile;
                int   hOutFile;
                LPSTR lpPath;
                int   nLevel;
            }   FAR   *lpOICInfo;

            /* Center the dialog.      */

            GetWindowRect (hDlg,&rc);
            SetWindowPos  (hDlg,NULL,
                          (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2,
                          (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 3,
                           0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

            pStringBuf  = LocalLock (hStringBuf);
            pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

            lpOICInfo = (struct OICINFO FAR *) lParam;

            LoadString (hInstIP, STR_OIC_RESULTS, (LPSTR) pStringBuf, MAXSTRINGSIZE);
            SetWindowText (hDlg, pStringBuf);

            SetDlgItemText (hDlg, 105, (LPSTR) lpOICInfo -> lpPath);

            dwInFileSize  = lseek (lpOICInfo -> hInFile, 0L, SEEK_END);
            dwOutFileSize = lseek (lpOICInfo -> hOutFile, 0L, SEEK_END);
    
            Ratio   = ((double) dwInFileSize / (double) dwOutFileSize) * 100.0;
            nRatio  = (int) Ratio / 100;
            nRatio2 = (int) Ratio % 100;
            nRatio2 /= 10;

            wsprintf (pStringBuf, "%ld", dwInFileSize);
            SetDlgItemText (hDlg, 106, (LPSTR) pStringBuf);

            wsprintf (pStringBuf, "%ld", dwOutFileSize);
            SetDlgItemText (hDlg, 107, (LPSTR) pStringBuf);

            LoadString (hInstIP, STR_OIC_RATIO, (LPSTR) pStringBuf2, MAXSTRINGSIZE);
            wsprintf (pStringBuf, pStringBuf2, nRatio, nRatio2);
            SetDlgItemText (hDlg, 108, (LPSTR) pStringBuf);
 
            wsprintf (pStringBuf, "%d", lpOICInfo -> nLevel);
            SetDlgItemText (hDlg, 109, (LPSTR) pStringBuf);

            LocalUnlock (hStringBuf);

        }
        break;

        case WM_COMMAND:
            switch (wParam)
            {
                case IDOK:
                    EndDialog (hDlg, TRUE);
                break;

            }
            break;    

        default:
            return (FALSE);
    }
    return (TRUE);
}
