#include <windows.h>
#include <string.h>
#include <imgprep.h>
#include <resource.h>
#include <proto.h>
#include <global.h>
#include <error.h>
#include <fmtproto.h>

int IdentFormat (hFile, lpFilename)
int   hFile;
LPSTR lpFilename;
{
  int  nReturn; 
  _fstrupr (lpFilename);
  nReturn = CheckByExt (hFile, lpFilename);
  if (nReturn < 0)
      return (nReturn);
  if (! nReturn)
    nReturn = CheckAllTypes (hFile);
  if (nReturn == 0) 
    nReturn = EC_UFILETYPE;
  return (nReturn);
}


int CheckByExt (hFile, lpFilename)
int   hFile;
LPSTR lpFilename;
{
  LPSTR lpBuffer;    
  int   nReturn = 0; 

  if (!(lpBuffer = (LPSTR)GlobalLockDiscardable (hGlobalBufs)))
    return (EC_MEMORY1);
  if (lstrcmp (GetExtension (lpFilename), (LPSTR)".CPI") == 0)
  {
    if (TestCompressedCPI (hFile, lpBuffer))
    {
      #ifdef DECIPHER
      nReturn = EC_UFILETYPE;
      #else
      nReturn =  IDFMT_CCPI;
      #endif
    }
    else
    { 
      if (TestCPI (hFile, lpBuffer))
        nReturn = IDFMT_CPI;
    }
  }
  else if (lstrcmp (GetExtension (lpFilename), (LPSTR)".TIF") == 0){
    if (TestTIF (hFile, lpBuffer))
      nReturn = IDFMT_TIF ;
  }
  else if (lstrcmp (GetExtension (lpFilename), (LPSTR)".TGA") == 0){
    if (TestTGA( hFile, lpBuffer))
      nReturn = IDFMT_TGA ;
  }
  else if (lstrcmp (GetExtension (lpFilename), (LPSTR)".PCX") == 0){
    if (TestPCX (hFile, lpBuffer))
      nReturn = IDFMT_PCX ;
  }
  else if (lstrcmp (GetExtension (lpFilename), (LPSTR)".GIF") == 0){
    if (TestGIF (hFile, lpBuffer))
      nReturn = IDFMT_GIF ;
  }
  else 
  if (lstrcmp (GetExtension (lpFilename), (LPSTR)".BMP") == 0)
  {
    if (TestPMDIB (hFile, lpBuffer))
      nReturn = IDFMT_PMDIB ;
    else 
      if (TestWNDIB (hFile, lpBuffer))
        nReturn = IDFMT_WNDIB ;
  }
  else
  if (lstrcmp (GetExtension (lpFilename), (LPSTR)".WMF") == 0)
  {
    if (TestWMF (hFile, lpBuffer))
      nReturn = IDFMT_WMF;
  }
  else
  if (lstrcmp (GetExtension (lpFilename), (LPSTR)".DVA") == 0)
  {
//  if (TestDVA (hFile, lpBuffer))
      nReturn = IDFMT_DVA;
  }


  GlobalUnlock (hGlobalBufs);
  return (nReturn);
}

/****************************************************************************

   PROCEDURE:        CheckAllTypes
   DESCRIPTION:      Find file format type from all known list
   DEFINITION:       2.1.2.2
   START:            1/3/90  Tom Bagford Jr.   
   MODS:             4/19/90 TBjr 3.0 review & annotated 

*****************************************************************************/

int CheckAllTypes (hFile)
int hFile;
{
  LPSTR lpBuffer;
  int   nReturn = 0;


  if (!(lpBuffer = (LPSTR)GlobalLockDiscardable (hGlobalBufs)))
    return (EC_MEMORY1);
  if (TestCompressedCPI (hFile, lpBuffer)){
#ifndef DECIPHER
    nReturn =  IDFMT_CCPI;
#else
    nReturn = EC_UFILETYPE;
#endif
  }
  else if (TestCPI (hFile, lpBuffer)){
    nReturn = IDFMT_CPI;
  }
  else if (TestTIF( hFile, lpBuffer)){
    nReturn = IDFMT_TIF;
  }
  else if (TestTGA (hFile, lpBuffer)){
    nReturn = IDFMT_TGA;
  }
  else if (TestPCX (hFile, lpBuffer)){
    nReturn = IDFMT_PCX;
  }
  else if (TestGIF (hFile, lpBuffer)){
    nReturn = IDFMT_GIF;
  }
  else if (TestPMDIB (hFile, lpBuffer)){
    nReturn = IDFMT_PMDIB;
  }
  else if (TestWNDIB (hFile, lpBuffer)){
    nReturn = IDFMT_WNDIB ;
  }  
  else if (TestWMF   (hFile, lpBuffer)){
    nReturn = IDFMT_WMF;
  }  
  else if (TestDVA   (hFile, lpBuffer)){
    nReturn = IDFMT_DVA;
  }  
  GlobalUnlock (hGlobalBufs);
  return( nReturn ); 
}


/****************************************************************************

   PROCEDURE:        GetExtension
   DESCRIPTION:      get extension from filename
   DEFINITION:         
   START:            1/4/90  Tom Bagford Jr.   
   MODS:

*****************************************************************************/

LPSTR GetExtension (lpFilename)
LPSTR   lpFilename;
{
  LPSTR lptr;

  lptr = lpFilename;
  while (*lptr && *lptr != '.')
    lptr++;
  return (lptr);
}


