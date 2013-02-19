#include <windows.h>
#include "imgprep.h"
#include "resource.h"
#include "strtable.h"
#include "proto.h"
#include "global.h"
#include "save.h"
#include <setup.h>

/*---------------------------------------------------------------------------

   PROCEDURE:         DoDisplayStateDlg
   DESCRIPTION:       DoDisplayState Dialog Proc
   DEFINITION:        2.8
   START:             1/3/90  Tom Bagford Jr.   
   MODS:              4/18/90 TBjr 3.0 review & annotate

----------------------------------------------------------------------------*/

BOOL FAR PASCAL DoDisplayStateDlg(hWnd, Message, wParam, lParam)
HWND        hWnd;
unsigned    Message;
WORD        wParam;
LONG        lParam;
{
    BOOL      bReturn = TRUE;                          
    FARPROC   lpfnDlg;                      
    WORD      wSetDither;
    WORD      wSetPal;


    switch (Message)
    {
        case WM_INITDIALOG:
      int_dither = wDither;         /* Save current Dither for CANCEL */
      int_pal    = wPal;            /* Process Pal Type save for CANCEL */
      old_pal    = dPalType;        /* Save Pal Type for CANCEL & Change Test */

      /*  What radios to check ?   */

      SetDisplayDlg (hWnd);

      /* Check the radios  */

      CheckRadioButton (hWnd, ID_NORM, ID_BW, ID_PALBASE  + int_pal);
      CheckRadioButton (hWnd, ID_NONE, ID_FS16, ID_DITHBASE + int_dither);
      break;

    case WM_COMMAND:
      switch (wParam)
      {
        case ID_NORM:
        case ID_OPT:
        case ID_GRAY:
        case ID_BW:
          int_pal   = wParam - ID_PALBASE;
          SetDisplayDlg (hWnd);
          CheckRadioButton (hWnd, ID_NORM, ID_BW, ID_PALBASE  + int_pal);
          CheckRadioButton (hWnd, ID_NONE, ID_FS16, ID_DITHBASE + int_dither);
          break;
        case ID_NONE:
        case ID_BAYER:
        case ID_FS:
        case ID_NONE16:
        case ID_FS16:
          int_dither = wParam - ID_DITHBASE;
          //SetDisplayDlg( hWnd, (WORD FAR *)&int_dither, (WORD FAR *)&int_pal ); 
          SetDisplayDlg (hWnd);
          CheckRadioButton (hWnd, ID_NORM, ID_BW, ID_PALBASE  + int_pal);
          CheckRadioButton (hWnd, ID_NONE, ID_FS16, ID_DITHBASE + int_dither);
          break;
        case ID_IMPORTIMG:
          lpfnDlg = MakeProcInstance ((FARPROC) ImportImgDlg, hInstIP);
          DialogBox (hInstIP, (LPSTR)"INPUTIMG", hWnd, lpfnDlg);
          FreeProcInstance (lpfnDlg);
          break;
        case ID_EXPORTIMG:
          lpfnDlg = MakeProcInstance ((FARPROC) ExportImgDlg, hInstIP);
          DialogBox (hInstIP, (LPSTR)"PROIMG", hWnd, lpfnDlg);
          FreeProcInstance (lpfnDlg);
          break;
        case ID_DISPLAYIMG:
          lpfnDlg = MakeProcInstance ((FARPROC) DisplayImgDlg, hInstIP);
          DialogBox (hInstIP, (LPSTR)"DISPIMG", hWnd, lpfnDlg);
          FreeProcInstance (lpfnDlg);
          break;
        case 142:   /* dither quality options */
          lpfnDlg = MakeProcInstance ((FARPROC)DithOptionsDlg, hInstIP);
          DialogBox (hInstIP, (LPSTR)"DOPTIONS", hWnd, lpfnDlg);
          FreeProcInstance (lpfnDlg);
          break;
        case ID_SAVEOPS:
          lpfnDlg = MakeProcInstance ((FARPROC) SaveOptDlg, hInstIP);
          DialogBox (hInstIP, (LPSTR)"SAVEOPS", hWnd, lpfnDlg);
          FreeProcInstance (lpfnDlg);
          break;
        case IDOK:
          SetState (PAINT_STATE, TRUE);
          wSetPal  = DDSM[wDisplay][wImportClass][int_pal][int_dither][SETPAL];
          wSetDither = DDSM[wDisplay][wImportClass][int_pal][int_dither][SETDITH];
          /*
          **  3/7/90 fix FS to NONE
          */
          if ((wDither == IFS) && (wPal == IOPT)){
            if ((int_dither != IFS) && (int_pal == IOPT))
              bNewOpt = TRUE;
          }
          SetState (DITH_STATE, int_dither);
          SetState (PAL_STATE, int_pal);
          bReturn = TRUE;
          if (image_active)
          {
            GLOBALFREE (hImpPalette);

            /*  This put in to allow interruptable paints.  4/91  D.Ison  */

            FreeFilters (hFileInfo, IMPORT_CLASS);

            SetImportStates ();
            FreePrevDispPalMem (dPalType, old_pal);
            if ((wDither == IFS) && (wPal == IOPT)) 
              if (! bOptDither)
                bNewOpt = TRUE;
              
          }
          EndDialog (hWnd, bReturn);
          break;
        case IDCANCEL:
          EndDialog (hWnd, FALSE);
          break;
        default:
          return (FALSE);
      }
      break;
    default:
     return (FALSE);
  }
  return (TRUE);
}


/*---------------------------------------------------------------------------

   PROCEDURE:        SetDisplayDlg
   DESCRIPTION:      Determine what radio button set should exist 
                     with the values provided

                     Grays/ungrays appropriate radios
                     sets the requested or only defaults allowed

   DEFINITION:       2.8.1
   START:            1/3/90  Tom Bagford Jr.   
                     temporary
   MODS:             4/19/90 TBjr ... 3.0 Mods & annotate
                     4/91    D. Ison  - Fix the Setup Processing Dialog

----------------------------------------------------------------------------*/

#define GRAYOUT

int SetDisplayDlg (hWnd)
HWND          hWnd;                                        /* Dialog to set */
{
    #ifdef GRAYOUT
    int p,d;
    #endif

    WORD dith;                        /* proc copy of int_dither */
    WORD i;
    WORD pal;                         /* proc copy of int_pal */

    /*  Copy temp variables for use   */

    pal  = int_pal;
    dith = int_dither; 

    /* Find new value of pal & dith   */

    pal  = DDSM[0][wImportClass][pal][dith][SETPAL];     /* Display */
    dith = DDSM[0][wImportClass][pal][dith][SETDITH];    /* Display */

    #ifdef GRAYOUT
    /*  Process Radio Button Palette options  */

    for (i = 0; i < MAXPAL; i++)
    {
        if (i == (WORD) (p = DDSM[0][wImportClass][i][dith ][SETPAL]))
//          EnableWindow (GetDlgItem (hWnd, ID_PALBASE + i), TRUE);
            ShowWindow (GetDlgItem (hWnd, ID_PALBASE + i), SW_SHOW);
        else
//          EnableWindow (GetDlgItem (hWnd, ID_PALBASE + i), FALSE);
            ShowWindow (GetDlgItem (hWnd, ID_PALBASE + i), SW_HIDE);
    }

    /*  Process Radio Button Dither options   */

    for (i = 0; i < MAXDITH -1; i++)
    {
        if (i == (WORD) (d = DDSM[0][wImportClass][pal][i][SETDITH]))
//          EnableWindow (GetDlgItem (hWnd, ID_DITHBASE + i), TRUE);
            ShowWindow (GetDlgItem (hWnd, ID_DITHBASE + i), SW_SHOW);
        else
//          EnableWindow (GetDlgItem (hWnd, ID_DITHBASE + i), FALSE);
            ShowWindow (GetDlgItem (hWnd, ID_DITHBASE + i), SW_HIDE);
    }
    #else

    for (i = 0; i < MAXPAL; i++)
        EnableWindow (GetDlgItem (hWnd, ID_PALBASE + i), TRUE);
    for (i = 0; i < MAXDITH -1; i++)
        EnableWindow (GetDlgItem (hWnd, ID_DITHBASE + i), TRUE);

    #endif
    int_pal    = pal;   
    int_dither = dith;  
    return (0);
}

/****************************************************************************

   PROCEDURE:         FreePrevDispPalMem
   DESCRIPTION:       
   DEFINITION:         
   START:              1/17/90  Tom Bagford Jr.   
                       mod for old pal dds change image
   MODS:

*****************************************************************************/
int FreePrevDispPalMem (wNewPal, wOldPal)
WORD wNewPal;
WORD wOldPal;
{
  if (wOldPal != wNewPal){
    SetState (PAINT_STATE, TRUE);
    switch (wOldPal){
      case DP256:    
      case DP8:
      case DP16:
        bNewDefault = TRUE;
        break;
      case OP256:
      case OP8:
      case OP16:
        bNewOpt    = TRUE;
        bOptDither = FALSE;
// HIST TEMP delete was here 
        break;
      case GP256:
      case GP64:
      case GP16:
      case GP8:
      case BP:
      case IP256:
        break;
    }
  }
  return (0);
}

/**************************************************************************

   PROCEDURE:         ImportImgDlg
   DESCRIPTION:       
   DEFINITION:         2.8.1
   START:              1/27/90  Tom Bagford Jr.   
   MODS:

***************************************************************************/

BOOL FAR PASCAL ImportImgDlg(hWnd, Message, wParam, lParam)
HWND      hWnd;
unsigned  Message;
WORD      wParam;
LONG      lParam;
{
  LPFILEINFO      lpF;

  switch (Message){
    case WM_INITDIALOG:
      if (image_active){
        PSTR pStringBuf;
        PSTR pStringBuf2;

        pStringBuf  = LocalLock (hStringBuf);
        pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

        /*
        **  type
        */
        LoadString (hInstIP, RC_TYPE_BASE + wImportType, (LPSTR)pStringBuf, MAXSTRINGSIZE);
        SetDlgItemText (hWnd, 600, (LPSTR)pStringBuf);
        /*
        **  class
        */
        LoadString (hInstIP, RC_CLASS_BASE + wImportClass, (LPSTR)pStringBuf, MAXSTRINGSIZE);
        SetDlgItemText (hWnd, 601, (LPSTR)pStringBuf);
        lpF = (LPFILEINFO)GlobalLock (hFileInfo);
        /*
        **  width
        */
        LoadString (hInstIP, STR_UPIXELS, (LPSTR)pStringBuf, MAXSTRINGSIZE);
        wsprintf ((LPSTR)pStringBuf2, (LPSTR)pStringBuf, lpF->wScanWidth);
        SetDlgItemText (hWnd, 602, (LPSTR)pStringBuf2);
        /*
        **  height
        */
        LoadString (hInstIP, STR_UPIXELS, (LPSTR)pStringBuf, MAXSTRINGSIZE);
        wsprintf ((LPSTR)pStringBuf2, (LPSTR)pStringBuf, lpF->wScanHeight);
        SetDlgItemText (hWnd, 603, (LPSTR)pStringBuf2);
        /*
        **  size
        */
        LoadString (hInstIP, STR_LBYTES, (LPSTR)pStringBuf, MAXSTRINGSIZE);
        wsprintf ((LPSTR)pStringBuf2, (LPSTR)pStringBuf, 
          (long)lpF->wScanHeight * (long)lpF->wBytesPerRow);
        SetDlgItemText (hWnd, 604, (LPSTR)pStringBuf2);
        GlobalUnlock (hFileInfo);
        LocalUnlock (hStringBuf);
      }
      break;
    case WM_COMMAND:
      switch (wParam){
        case IDOK:
          EndDialog (hWnd, TRUE);
          break;
        default:
          return (FALSE);
      }
      break;
    default:
      return (FALSE);
  }
  return (TRUE);
}


BOOL FAR PASCAL ExportImgDlg(hWnd, Message, wParam, lParam)
HWND hWnd;
unsigned Message;
WORD wParam;
LONG lParam;
{
  switch (Message){
    case WM_INITDIALOG:
    {
        PSTR pStringBuf;
        PSTR pStringBuf2;
        pStringBuf  = LocalLock (hStringBuf);
        pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

      switch (int_pal){
        case PAL_NORM:
        case PAL_OPT:
          LoadString (hInstIP, RC_CLASS_CM, (LPSTR)pStringBuf, MAXSTRINGSIZE);
          SetDlgItemText (hWnd, 605, (LPSTR)pStringBuf);
          break;
        case PAL_GRAY:
          LoadString (hInstIP, RC_CLASS_GRAY, (LPSTR)pStringBuf, MAXSTRINGSIZE);
          SetDlgItemText (hWnd, 605, (LPSTR)pStringBuf); 
          break;
        case PAL_BW:
          LoadString (hInstIP, RC_CLASS_BW, (LPSTR)pStringBuf, MAXSTRINGSIZE);
          SetDlgItemText (hWnd, 605, (LPSTR)pStringBuf);
          break;
      }
      /*
      **  do Processed Image palette
      */
      LoadString (hInstIP, RCP_DP256 + EPAL[wImportClass][int_pal][int_dither][0],
                        (LPSTR)pStringBuf, MAXSTRINGSIZE);


            /* Create a number string here for lstrcat if OPT palette */

            pStringBuf2 [0] = 0;
            if (int_pal == PAL_OPT)
                wsprintf (pStringBuf2, "%d ",wNumberColors);

            lstrcat (pStringBuf2, pStringBuf);


      SetDlgItemText (hWnd, 606, (LPSTR)pStringBuf2);
      /*
      **  do Processed Image dith 1
      */
      switch (int_dither)
      {
        int nString;

        case INONE:
        case INO16:
          LoadString (hInstIP, RC_DITH_NO, (LPSTR)pStringBuf, MAXSTRINGSIZE);
          SetDlgItemText (hWnd, 607, (LPSTR)pStringBuf);
          LoadString (hInstIP, RC_BLANK, (LPSTR)pStringBuf, MAXSTRINGSIZE);
          SetDlgItemText (hWnd, 608, (LPSTR)pStringBuf);
          break;
        case IBAY:
          LoadString (hInstIP, RC_DITH_BAY, (LPSTR)pStringBuf, MAXSTRINGSIZE);
          SetDlgItemText (hWnd, 607, (LPSTR)pStringBuf);
          LoadString (hInstIP, RC_BLANK, (LPSTR)pStringBuf, MAXSTRINGSIZE);
          SetDlgItemText (hWnd, 608, (LPSTR)pStringBuf);
          break;
        case IFS:
        case IFS16:
          LoadString (hInstIP, RC_DITH_FS, (LPSTR)pStringBuf, MAXSTRINGSIZE);
          SetDlgItemText (hWnd, 607, (LPSTR)pStringBuf);

        switch (wCurrDither)
        {
              
            case DITHER_FAST_COLOR:
                nString = STR_LINEAR_DITHER;
                break;

            case DITHER_FS_COLOR:
                nString = STR_FS_DITHER;
                break;


            case DITHER_BURKE_COLOR:
                nString = STR_BURKES_DITHER;
                break;
        }

          LoadString (hInstIP, nString, (LPSTR)pStringBuf, MAXSTRINGSIZE);
 
          SetDlgItemText (hWnd, 608, (LPSTR)pStringBuf);
          break;
      }
      LocalUnlock (hStringBuf);
    }
    break;
    case WM_COMMAND:
      switch (wParam){
        case IDOK:
          EndDialog (hWnd, TRUE);
          break;
        default:
          return (FALSE);
      }
      break;
    default:
      return (FALSE);
  }
  return (TRUE);
}


BOOL FAR PASCAL DisplayImgDlg(hWnd, Message, wParam, lParam)
HWND      hWnd;
unsigned  Message;
WORD      wParam;
LONG      lParam;
{
  WORD    wSetPal;
  WORD    wSetDith;


  switch (Message){
    case WM_INITDIALOG:
    {
        PSTR pStringBuf;
        PSTR pStringBuf2;
        pStringBuf  = LocalLock (hStringBuf);
        pStringBuf2 = pStringBuf + MAXSTRINGSIZE;

      wSetPal = DDSM[wDisplay][wImportClass][int_pal][int_dither][SETPAL];
      wSetDith = DDSM[wDisplay][wImportClass][int_pal][int_dither][SETDITH];
      switch (wSetPal){
        case PAL_NORM:
        case PAL_OPT:
          LoadString (hInstIP, RC_CLASS_CM, (LPSTR)pStringBuf, MAXSTRINGSIZE);
          SetDlgItemText (hWnd, 605, (LPSTR)pStringBuf);
          break;
        case PAL_GRAY :
          LoadString (hInstIP, RC_CLASS_GRAY, (LPSTR)pStringBuf, MAXSTRINGSIZE);
          SetDlgItemText (hWnd, 605, (LPSTR)pStringBuf);
          break;
        case PAL_BW:
          LoadString (hInstIP, RC_CLASS_BW, (LPSTR)pStringBuf, MAXSTRINGSIZE);
          SetDlgItemText (hWnd, 605, (LPSTR)pStringBuf);
          break;
      }
      /*
      **  do Processed Image palette
      */
      wSetPal = DPAL[wDisplay][wImportClass][int_pal][int_dither][0];
      switch (wSetPal){
        case DP256:
        case DP8:
        case DP16:
        case OP256:
        case OP8:
        case OP16:
        case GP256:
        case GP64:
        case GP16:
        case GP8:
        case BP:
        case IP256:

          LoadString (hInstIP, RCP_DP256 + wSetPal, (LPSTR)pStringBuf, MAXSTRINGSIZE);

            /* Create a number string here for lstrcat if OPT palette */

            pStringBuf2 [0] = 0;
            if (int_pal == PAL_OPT)
                wsprintf (pStringBuf2, "%d ",wNumberColors);

            lstrcat (pStringBuf2, pStringBuf);
	
          SetDlgItemText (hWnd, 606, (LPSTR)pStringBuf2);
          break;
      }
      /*
      **  do Processed Image dith 1
      */
      switch (wSetDith)
      {
        int nString;

        case INONE:
        case INO16:
          LoadString (hInstIP, RC_DITH_NO, (LPSTR)pStringBuf, MAXSTRINGSIZE);
          SetDlgItemText (hWnd, 607, (LPSTR)pStringBuf); 
          LoadString (hInstIP, RC_BLANK, (LPSTR)pStringBuf, MAXSTRINGSIZE);
          SetDlgItemText (hWnd, 608, (LPSTR)pStringBuf);
          break;
        case IBAY:
          LoadString (hInstIP, RC_DITH_BAY, (LPSTR)pStringBuf, MAXSTRINGSIZE);
          SetDlgItemText (hWnd, 607, (LPSTR)pStringBuf);
          LoadString (hInstIP, RC_BLANK, (LPSTR)pStringBuf, MAXSTRINGSIZE);
          SetDlgItemText (hWnd, 608, (LPSTR)pStringBuf);
          break;
        case IFS:
        case IFS16:
          LoadString (hInstIP, RC_DITH_FS, (LPSTR)pStringBuf, MAXSTRINGSIZE);
          SetDlgItemText (hWnd, 607, (LPSTR)pStringBuf);

        switch (wCurrDither)
        {
            case DITHER_FAST_COLOR:
                nString = STR_LINEAR_DITHER;
                break;

            case DITHER_FS_COLOR:
                nString = STR_FS_DITHER;
                break;

            case DITHER_BURKE_COLOR:
                nString = STR_BURKES_DITHER;
                break;
        }

          LoadString (hInstIP, nString, (LPSTR)pStringBuf, MAXSTRINGSIZE);

          SetDlgItemText (hWnd, 608, (LPSTR)pStringBuf);
          break;
      }
      LocalUnlock (hStringBuf);
    }
    break;
    case WM_COMMAND:
      switch (wParam){
        case IDOK:
          EndDialog (hWnd, TRUE);
          break;
        default:
          return (FALSE);
      }
      break;
    default:
      return (FALSE);
  }
  return (TRUE);
}


BOOL FAR PASCAL SaveOptDlg(hWnd, Message, wParam, lParam)
HWND      hWnd;
unsigned  Message;
WORD      wParam;
LONG      lParam;
{
  WORD    i;


  switch (Message){
    case WM_INITDIALOG:
    {
        PSTR pStringBuf;
        pStringBuf  = LocalLock (hStringBuf);

      switch (int_pal){
        case PAL_NORM:
        case PAL_OPT :
          lstrcpy ((LPSTR)pStringBuf, (LPSTR)"ColorMap");
          break;
        case PAL_GRAY:
          lstrcpy ((LPSTR)pStringBuf, (LPSTR)"Gray");
          break;
        case PAL_BW:
          lstrcpy ((LPSTR)pStringBuf, (LPSTR)"Monochrome");
          break;
      }
      SetDlgItemText (hWnd, CLASSEDIT, (LPSTR)pStringBuf);
      for (i = 1; i < XALLFORMATS ; i++){
        if (SFM[int_pal][i]){
          EnableWindow (GetDlgItem (hWnd, RGBBASE + i), TRUE);
          if ((wImportType != IDFMT_CPI) && (i == 8))
            EnableWindow (GetDlgItem (hWnd, RGBBASE + i), FALSE);
        }      
        else
          EnableWindow (GetDlgItem (hWnd, RGBBASE + i), FALSE);
      }
      LocalUnlock (hStringBuf);
    }
    break;

    case WM_COMMAND:
      switch (wParam){
        case IDOK:
          EndDialog (hWnd, TRUE);
          break;
        default:
          return (FALSE);
      }
      break;
    default:
      return (FALSE);
  }
  return (TRUE);
}

/*-------------------------------------------------------------------------

      PROCEDURE:      DithOptionsDlg

--------------------------------------------------------------------------*/

BOOL FAR PASCAL DithOptionsDlg(hWnd, Message, wParam, lParam)
HWND         hWnd;
unsigned     Message;
WORD         wParam;
LONG         lParam;
{
  BOOL bChanged;

  switch (Message){
    case WM_INITDIALOG:
      bChanged = FALSE;
      if (wDithQuality == 8)
        CheckRadioButton (hWnd, 200, 201, 200);
      else
        CheckRadioButton (hWnd, 200, 201, 201);
      break;
    case WM_COMMAND:
      switch (wParam){
        case 200:
        case 201:
          CheckRadioButton (hWnd, 200, 201, wParam);
          wDithQuality = (wParam == 200) ? 8 : 1;
          bChanged = !bChanged;
          break;
        case IDOK:
          if (bChanged)
            bNewOpt = (BYTE) bIsFirstPaint = TRUE;
          EndDialog (hWnd, TRUE);
          break;
        default:
          return (FALSE);
      }
      break;
    default:
      return (FALSE);
  }
  return (TRUE);
}


