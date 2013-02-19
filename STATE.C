
#include <windows.h>
#include "imgprep.h"
#include "strtable.h"
#include "proto.h"
#include "global.h"


int SetImportStates()
{
    int i;

    /* Setup Display Filter Set   */

    for (i = 0; i < 5; i++)
        ImportFilters[i] = DIS[wDisplay][wImportClass][wPal][wDither][i];

    /* Setup Palette Type Flag    */

    dPalType = DPAL[wDisplay][wImportClass][wPal][wDither][0];
    return (0);
}


void SetState (nStateID, nValue)
int  nStateID;                               
int  nValue;                      
{
    switch (nStateID)
    {
        case EXPORT_TYPE:
            wExportType = (WORD)nValue;
        break;
        case IMPORT_TYPE:
            wImportType = (WORD)nValue;
        break;
        case EXPORT_CLASS:
            wExportClass = (WORD)nValue;
        break;
        case IMPORT_CLASS:
            wImportClass = (WORD)nValue;
        break;
        case REGION_STATE:
            wRegion = (WORD)nValue;
        break;
        case HOTKEY_STATE:
            wHotKey = (WORD)nValue;
        break;
        case DITH_STATE:
            wDither = (WORD)nValue;
            if (image_active)
                bImageModified = TRUE;
        break;
        case PAL_STATE:
            wPal = (WORD)nValue;
            if (image_active)
                bImageModified = TRUE;
        break;
        case DISPLAY_STATE:
            wDisplay = (WORD)nValue;
        break;
        case DEVICE_STATE:
            wDevice = (WORD)nValue;
        break;
        case PAINT_STATE:
            bIsFirstPaint = (BYTE) nValue;
        break;
        case CAPTURE_STATE:
            bIsCapturing = (BYTE) nValue;
        break;

        case SAVE_TYPE:

            wSaveType = (WORD)nValue;

          /* If SaveType changed, change export TYPE & CLASS       */

            switch (wSaveType)
            {
                case XCPI24:
                    SetState (EXPORT_TYPE, IDFMT_CPI);
                    SetState (EXPORT_CLASS, IRGB);
                break;

                case XTIF24:
                    SetState (EXPORT_TYPE, IDFMT_TIF);
                    SetState (EXPORT_CLASS, IRGB);
                    break;

                case XEPS24:
                    SetState (EXPORT_TYPE, IDFMT_PS);
                    SetState (EXPORT_CLASS, IRGB);
                break;

                case XTGA16:
                    SetState (EXPORT_TYPE, IDFMT_TGA);
                    SetState (EXPORT_CLASS, IRGB);
                break;

                case XTGA24:
                    SetState (EXPORT_TYPE, IDFMT_TGA);
                    SetState (EXPORT_CLASS, IRGB);
                break;

                case XTGA32:
                    SetState (EXPORT_TYPE, IDFMT_TGA);
                    SetState (EXPORT_CLASS, IRGB);
                break;

                case XPMDIB24:
                    SetState (EXPORT_TYPE, IDFMT_PMDIB);
                    SetState (EXPORT_CLASS, IRGB);
                break;

                case XWNDIB24:
                    SetState (EXPORT_TYPE, IDFMT_WNDIB);
                    SetState (EXPORT_CLASS, IRGB);
                break;

                case XCCPI24:
                    SetState (EXPORT_TYPE, IDFMT_CCPI);
                    SetState (EXPORT_CLASS, IRGB);
                break;

                case XWMF24:
                    SetState (EXPORT_TYPE, IDFMT_WMF);
                    SetState (EXPORT_CLASS, IRGB);
                break;

                case XDVA24:
                    SetState (EXPORT_TYPE, IDFMT_DVA);
                    SetState (EXPORT_CLASS, IRGB);
                break;

                case XCPI:
                    SetState (EXPORT_TYPE, IDFMT_CPI);
                    switch (wPal)
                    {
                        case INORM:
                        case IOPT:
                            SetState (EXPORT_CLASS, ICM);
                        break;
                        case IGRA:
                            SetState (EXPORT_CLASS, IGRA);
                        break;
                        case IBW:
                           SetState (EXPORT_CLASS, IMON);
                        break;
                    }
                break;

        case XTIF:
          SetState (EXPORT_TYPE, IDFMT_TIF);
          switch (wPal){
            case INORM:
            case IOPT:
              SetState (EXPORT_CLASS, ICM);
              break;
            case IGRA:
              SetState (EXPORT_CLASS, IGRA);
              break;
            case IBW:
              SetState (EXPORT_CLASS, IMON);
              break;
          }
          break;
        case XTGA:
          SetState (EXPORT_TYPE, IDFMT_TGA);
          switch (wPal){
            case INORM:
            case IOPT:
              SetState (EXPORT_CLASS, ICM);
              break;
            case IGRA:
              SetState (EXPORT_CLASS, IGRA);
              break;
            case IBW:
              SetState (EXPORT_CLASS, IMON);
              break;
          }
          break;
        case XPCX:
          SetState (EXPORT_TYPE, IDFMT_PCX);
          switch (wPal){
            case INORM:
            case IOPT:
              SetState (EXPORT_CLASS, ICM);
              break;
            case IGRA:
              SetState (EXPORT_CLASS, IGRA);
              break;
            case IBW:
              SetState (EXPORT_CLASS, IMON);
              break;
          }
          break;
        case XGIF:
          SetState (EXPORT_TYPE, IDFMT_GIF);
          switch (wPal){
            case INORM:
            case IOPT:
              SetState (EXPORT_CLASS, ICM);
              break;
            case IGRA:
              SetState (EXPORT_CLASS, IGRA);
              break;
            case IBW:
              SetState (EXPORT_CLASS, IMON);
              break;
          }
          break;
        case XPMDIB:
          SetState (EXPORT_TYPE, IDFMT_PMDIB);
          switch (wPal){
            case INORM:
            case IOPT:
              SetState (EXPORT_CLASS, ICM);
              break;
            case IGRA:
              SetState (EXPORT_CLASS, IGRA);
              break;
            case IBW:
              SetState (EXPORT_CLASS, IMON);
              break;
          }
          break;
        case XWNDIB:
          SetState (EXPORT_TYPE, IDFMT_WNDIB);
          switch (wPal){
            case INORM:
            case IOPT:
              SetState (EXPORT_CLASS, ICM);
              break;
            case IGRA:
              SetState (EXPORT_CLASS, IGRA);
              break;
            case IBW:
              SetState (EXPORT_CLASS, IMON);
              break;
          }
          break;
        case XEPS:
          SetState (EXPORT_TYPE, IDFMT_PS);
          switch (wPal){
            case INORM:
            case IOPT:
              SetState (EXPORT_CLASS, IRGB);
              break;
            case IGRA:
              SetState (EXPORT_CLASS, IGRA);
              break;
            case IBW:
              SetState (EXPORT_CLASS, IMON);
              break;
          }
          break;

                case XWMF:
                    SetState (EXPORT_TYPE, IDFMT_WMF);
                    switch (wPal)
                    {
                        case INORM:
                        case IOPT:
                            SetState (EXPORT_CLASS, ICM);
                        break;
                        case IGRA:
                            SetState (EXPORT_CLASS, IGRA);
                        break;
                        case IBW:
                           SetState (EXPORT_CLASS, IMON);
                        break;
                    }
                break;

      }
      break;
    default:
      break;
  }
}

int  SetExportStates ( )
{
    int i;
    int nFilter;

    /* Setup Export Filter Set   */

    switch (wSaveType)
    {
        case XCPI24:
        case XTIF24:
        case XEPS24:
        case XTGA16:
        case XTGA24:
        case XTGA32:
        case XPMDIB24:
        case XWNDIB24:
        case XCCPI24:
        case XWMF24:
        case XDVA24:
            nFilter = 5;                        /* RGB save filter set */
        break;
        case XCPI:
        case XTIF:
        case XTGA:
        case XPCX:
        case XGIF:
        case XPMDIB:
        case XWNDIB:
        case XEPS:
        case XWMF:
            nFilter = wDither;                  /* Use current dither/matrix set */
        break;
    }
    for (i = 0; i< 5; i++)
        ExportFilters[i] = ESF[wImportClass][wPal][nFilter][i];
  
    /*  Setup Palette Type Flag   */
    ePalType = EPAL[wImportClass][wPal][wDither][0];
    return (0);
}












