
#include <windows.h>
#include <string.h>
#include <imgprep.h>
#include <resource.h>
#include <proto.h>
#include <error.h>
#include <global.h>
#include <fmtproto.h>


/*----------------------------------------------------------------------------

    PROCEDURE:         GetRCD()
    DESCRIPTION:       Get RdConvertData Proc Pointer

----------------------------------------------------------------------------*/

FARPROC GetRCD (wType)
WORD wType;
{
    FARPROC lpfnReadConvertData;

    switch (wType)
    {
        case IDFMT_CPI:
            lpfnReadConvertData = MakeProcInstance (CPIRdConvertData, hInstIP);
        break;

        case IDFMT_TIF:
            lpfnReadConvertData = MakeProcInstance (TIFRdConvertData, hInstIP);
        break;
        case IDFMT_TGA:
            lpfnReadConvertData = MakeProcInstance (TGARdConvertData, hInstIP);
        break;
        case IDFMT_PCX:
            lpfnReadConvertData = MakeProcInstance (PCXRdConvertData, hInstIP);
        break;
        case IDFMT_GIF:
            lpfnReadConvertData = MakeProcInstance (GIFRdConvertData, hInstIP);
        break;
        case IDFMT_PMDIB:
        case IDFMT_WNDIB:
            lpfnReadConvertData = MakeProcInstance (DIBRdConvertData, hInstIP);
        break;

        case IDFMT_DVA:
            lpfnReadConvertData = MakeProcInstance (DVARdConvertData, hInstIP);
        break;

        default:
            lpfnReadConvertData = 0L;                                   /* none */
        break;
  }
  return (lpfnReadConvertData);
}


FARPROC GetWCD (wType)
WORD wType;
{
    FARPROC lpfnWriteConvertData;

    switch (wType)
    {
        case 1:
            lpfnWriteConvertData = MakeProcInstance (CPIWrConvertData, hInstIP);
        break;
        case IDFMT_TIF:
            lpfnWriteConvertData = MakeProcInstance (TIFWrConvertData, hInstIP);
        break;
        case IDFMT_PS:                   
            lpfnWriteConvertData = MakeProcInstance (EPSWrConvertData, hInstIP);
        break;
        case IDFMT_TGA:
            lpfnWriteConvertData = MakeProcInstance (TGAWrConvertData, hInstIP);
        break;
        case IDFMT_PCX:                   
            lpfnWriteConvertData = MakeProcInstance (PCXWrConvertData, hInstIP);
        break;
        case IDFMT_GIF:
            lpfnWriteConvertData = MakeProcInstance (GIFWrConvertData, hInstIP);
        break;
        case IDFMT_PMDIB:
        case IDFMT_WNDIB:
            lpfnWriteConvertData = MakeProcInstance (DIBWrConvertData, hInstIP);
        break;
        case IDFMT_WMF:
            lpfnWriteConvertData = MakeProcInstance (WMFWrConvertData, hInstIP);
        break;

        case IDFMT_DVA:
            lpfnWriteConvertData = MakeProcInstance (DVAWrConvertData, hInstIP);
        break;

        default:
            lpfnWriteConvertData = (FARPROC)0;
        break;
  }
  return (lpfnWriteConvertData);
}



FARPROC GetIHdr (wType)
WORD wType;
{
    FARPROC lpfnInitHeader;

    switch (wType)
    {
    case 1:
      lpfnInitHeader = MakeProcInstance (CPIInitHeader, hInstIP);
      break;
    case IDFMT_TIF:
      lpfnInitHeader = MakeProcInstance (TIFInitHeader, hInstIP);
      break;
    case IDFMT_PS:
      lpfnInitHeader = MakeProcInstance (EPSInitHeader, hInstIP);
      break;
    case IDFMT_TGA:
      lpfnInitHeader = MakeProcInstance (TGAInitHeader, hInstIP);
      break;
    case IDFMT_PCX:
      lpfnInitHeader = MakeProcInstance (PCXInitHeader, hInstIP);
      break;
    case IDFMT_GIF:
      lpfnInitHeader = MakeProcInstance (GIFInitHeader, hInstIP);
      break;
    case IDFMT_PMDIB:
    case IDFMT_WNDIB:
      lpfnInitHeader = MakeProcInstance (DIBInitHeader, hInstIP);
      break;
    case IDFMT_WMF:
      lpfnInitHeader = MakeProcInstance (WMFInitHeader, hInstIP);
      break;
    case IDFMT_DVA:
      lpfnInitHeader = MakeProcInstance (DVAInitHeader, hInstIP);
      break;

    default:
      lpfnInitHeader = (FARPROC)0;
      break;
  }
  return (lpfnInitHeader);
}


FARPROC GetFHdr (wType)
WORD wType;
{
  FARPROC lpfnFixupHeader;

  switch (wType){
    case 1:   
      lpfnFixupHeader = MakeProcInstance (CPIFixupHeader, hInstIP);
      break;
    case IDFMT_TIF:   
      lpfnFixupHeader = MakeProcInstance (TIFFixupHeader, hInstIP);
      break;
    case IDFMT_PS:   
      lpfnFixupHeader = MakeProcInstance (EPSFixupHeader, hInstIP);
      break;
    case IDFMT_TGA:   
      lpfnFixupHeader = MakeProcInstance (TGAFixupHeader, hInstIP);
      break;
    case IDFMT_PCX:   
      lpfnFixupHeader = MakeProcInstance (PCXFixupHeader, hInstIP);
      break;
    case IDFMT_GIF:   
      lpfnFixupHeader = MakeProcInstance (GIFFixupHeader, hInstIP);
      break;
    case IDFMT_PMDIB:   
    case IDFMT_WNDIB:   
      lpfnFixupHeader = MakeProcInstance (DIBFixupHeader, hInstIP);
      break;
    case IDFMT_WMF:
      lpfnFixupHeader = MakeProcInstance (WMFFixupHeader, hInstIP);
      break;
    case IDFMT_DVA:
      lpfnFixupHeader = MakeProcInstance (DVAFixupHeader, hInstIP);
      break;
    default:
      lpfnFixupHeader = (FARPROC)0;
      break;
  }
  return (lpfnFixupHeader);
}


FARPROC GetRHdr (wType)
WORD wType;
{
  FARPROC lpfnReadHeader;
  /*
  **  INIT READ HEADERPROCS
  */
  switch (wType){
    case 1:  
      lpfnReadHeader = MakeProcInstance (CPIReadHeader, hInstIP);
      break;
    case IDFMT_TIF:  
      lpfnReadHeader = MakeProcInstance (TIFReadHeader, hInstIP);
      break;
    case IDFMT_TGA:  
      lpfnReadHeader = MakeProcInstance (TGAReadHeader, hInstIP);
      break;
    case IDFMT_PCX:  
      lpfnReadHeader = MakeProcInstance (PCXReadHeader, hInstIP);
      break;
    case IDFMT_GIF:  
      lpfnReadHeader = MakeProcInstance (GIFReadHeader, hInstIP);
      break;
    case IDFMT_PMDIB:  
      lpfnReadHeader = MakeProcInstance (DIBReadHeader, hInstIP);
      break;
    case IDFMT_WNDIB:  
      lpfnReadHeader = MakeProcInstance (DIBReadHeader, hInstIP);
      break;
    case IDFMT_DVA:  
      lpfnReadHeader = MakeProcInstance (DVAReadHeader, hInstIP);
      break;
    default:
      lpfnReadHeader = (FARPROC)0;
      break;
  }
  return (lpfnReadHeader);
}







