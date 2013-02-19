/*  
  
 This program sharpens 24 bit CPI files.  


    D. Ison

*/


#include <windows.h>
#include <stdio.h>
#include <malloc.h> 
#include <string.h>
#include <dos.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <fcntl.h>
#include <io.h>
#include "cpifmt.h"


#define SHARPNESS   8
int sharpness;

int   bytes;
int   infh, outfh;

#define COLOR 0
#define GRAY  1
char *scan_buf;

char in_filename[13];
char out_filename[13];

void get_img_info();

int ReadCPiHdr();

DWORD   dwCPiPalPos, dwCPiDataPos; 

CPIFILESPEC     CPiFileSpec;
CPIIMAGESPEC    CPiImageSpec;

LPSTR lpInBuf;      // For buffering input
LPSTR lpOutBuf;     // For buffering output
WORD  wReadBytes;   // ""
WORD  wWriteBytes;  // ""
WORD  wInByteCount  = 0;
WORD  wOutByteCount = 0;
WORD  wInByteOffset = 0;

WORD wImageWidth;
WORD wImageHeight;


unsigned wBytes;




int SharpenImage (int, int, LPSTR, LPSTR, WORD, WORD, WORD, WORD, WORD);
int CPiFixupFile (int);



main (argc,argv)
int argc;
char *argv[];
{
   char *result;
   short i;
   unsigned char ch;
   char far*ScanBuffer, far *Palette, far *EncBuffer;
   char *Oldname, *Newname;


   if (argc < 3)
   {   
       fputs ("\nusage:  unsharp <filename>.cpi  <filename>.cpi  <sharpness 1..16>\n\n",stdout); 
       exit (0);
   }

    sharpness = atoi (argv [3]);
    if (sharpness <= 0)
        sharpness = 2;  // Default;

   if ((ScanBuffer = _fmalloc (65000)) == NULL)
   { 
       fputs ("\nCan't allocate scanline buffer  Exiting to DOS\n\n",stdout); 
       exit (1);
   }


   if ((Palette = _fmalloc (1024)) == NULL)
   { 
       fputs ("\nCan't allocate palette buffer  Exiting to DOS\n\n",stdout); 
       exit (1);
   }

   if ((EncBuffer = _fmalloc (65000)) == NULL)
   { 
       fputs ("\nCan't allocate encode buffer  Exiting to DOS\n\n",stdout); 
       exit (1);
   }

   strncpy (in_filename,argv[1],12);
   strncpy (out_filename,argv[2],12);


   if ((result = (strchr (in_filename,'.'))) != NULL)
       *result = '\0';
   strncat (in_filename,".CPI",4);

   if ((result = (strchr (out_filename,'.'))) != NULL)
       *result = '\0';
   strncat (out_filename,".CPI",4);


   if (_dos_open (in_filename,O_RDONLY,&infh) != 0)
   {
       fputs ("\nCan't open the input file.  Exiting to DOS\n\n",stdout); 
       exit (1);
   }




   lseek (infh, 192L, SEEK_SET );  
   if (_dos_read (infh, (char far *) Palette, 768, &bytes) != 0)     
      return (-1);
   _dos_close (infh);      

   Oldname = in_filename;
   Newname = out_filename;

   if (! cpiunsharp (Oldname,Newname, ScanBuffer, EncBuffer, Palette)) 
       exit (-1);

}







int cpiunsharp (oldname, newname, scan_buf, enc_buf, palette) 
char *oldname;
char *newname;
char far *scan_buf;
char far *enc_buf;
char far *palette;
{
    int bytes;
    int scanbytes;
    int scanlines; 
    int i;
    char retval = 0;
    int count;
    char *ptr;
    int *int_ptr;
    int num_lines;               
    long curr_offset = 0;
    long file_length;
    long next_buf_size;
    char *buf_ptr;
    int done = FALSE;
    int read_size;
    int linecount = 0;
    int lastbuf = FALSE;
    
    if (palette == NULL)      /*  Means use input file's palette  */
        return (retval);  
    else
    { 
        WORD wRowsPerStrip;
        WORD wBytesPerRow;
        WORD wBytesPerPixel;

  
        if (_dos_open (oldname, O_RDONLY, &infh) != 0)
            return (retval);

        if (_dos_creat (newname, _A_NORMAL, &outfh) != 0)
            return (retval);

             
        /*  Get system metrics & image file sizes  */
    
        get_img_info (infh, &scanbytes, &scanlines);
        wImageWidth = scanbytes;
        wImageHeight = scanlines;



        /*  Initialize Header  */
        WriteCPiFileSpec (outfh);
        WriteCPiImgSpec  (outfh);

        /*  Write palette  */

        if (_dos_write (outfh, palette, 768, &bytes) != 0 || bytes != 768)
            retval = FALSE;

        wBytesPerPixel  = CPiImageSpec.BitsPerPixel >> 3;
        wBytesPerRow    = wImageWidth * wBytesPerPixel;
        wRowsPerStrip   = 65000 / wBytesPerRow;
        wRowsPerStrip   = wRowsPerStrip / 3 * 3;        // Make evenly divisible by 3
        if (wRowsPerStrip < 3)
        {
            printf ("Can't sharpen the image because it is too big \n");
            exit (-1);
        }

        lseek (outfh, 960L, SEEK_SET);
        lseek (infh, 960L, SEEK_SET);

        SharpenImage (infh, outfh, enc_buf, scan_buf, wBytesPerRow, wImageWidth, wImageHeight, wRowsPerStrip, wBytesPerPixel);
    }     

    CPiFixupFile (outfh);


    _dos_close (infh);
    _dos_close (outfh);

    return (retval);
}

int SharpenImage (hInFile, hOutFile, lpBuff1, lpBuff2, wBytesPerRow, wImageWidth, wImageHeight, wRowsPerStrip, wBytesPerPixel)
int   hInFile;
int   hOutFile;
LPSTR lpBuff1;
LPSTR lpBuff2;
WORD  wBytesPerRow;
WORD  wImageWidth;
WORD  wImageHeight;
WORD  wRowsPerStrip;
WORD  wBytesPerPixel;
{
    char Buffer [30];
    WORD wCurrentOffset;
    WORD wBytesPerStrip;
    unsigned bytes;

    LPSTR   lpPrev;
    LPSTR   lpCurr;
    LPSTR   lpNext;
    WORD    i;

    WORD    wRowsThisStrip;
    WORD    wNumGroups;
    WORD    wBytesPerGroup;
    WORD    wLinesSharpened = 0;

    wBytesPerStrip        = (wBytesPerRow * wRowsPerStrip);

    printf ("Sharpening image at %d... \n",sharpness);

    wNumGroups = wRowsPerStrip / 3;
    wBytesPerGroup = (3 * wBytesPerRow); // Bytes in 3 lines..

    lpPrev = lpBuff1;
    lpCurr = lpPrev + wBytesPerRow;
    lpNext = lpCurr + wBytesPerRow;

    lpInBuf  = lpBuff1 + 32000;
    lpOutBuf = lpBuff2 + 32000;
    wReadBytes  = 16384 / wBytesPerRow * wBytesPerRow;  // Read / Write 16k at a time
    wWriteBytes = 16384 / wBytesPerRow * wBytesPerRow;

    /*  Skip first line for now */

    _dos_read (hInFile, lpPrev, wBytesPerRow, &bytes);
    _dos_write (hOutFile, lpPrev, wBytesPerRow, &bytes);

    /*  Skip second line for now */

    _dos_read (hInFile, lpCurr, wBytesPerRow, &bytes);
    _dos_write (hOutFile, lpCurr, wBytesPerRow, &bytes);

    for (i = 2; i < wImageHeight; i++)  // Skipped first 2 lines....
    {

        GetLine (hInFile, lpNext, wBytesPerRow);

        SharpenLine (lpBuff2, lpPrev, lpCurr, lpNext, wImageWidth, wBytesPerRow);

        PutLine (hOutFile, lpBuff2, wBytesPerRow);

        printf ("%2.0f percent complete\r", (float) ((float) i / (float) wImageHeight * 100.0));

        _fmemcpy (lpPrev, lpCurr, wBytesPerRow);
        _fmemcpy (lpCurr, lpNext, wBytesPerRow);

    }

    if (wOutByteCount > 0)
        _dos_write (hOutFile, lpOutBuf, wOutByteCount, &bytes);
 
    return (1);
}





void get_img_info (infh, scanbytes, scanlines) 
int infh;
int *scanbytes;
int *scanlines;
{
  int *tmp_ptr;
  char tmp_buf[8];
  int bytes;

  ReadCPiHdr (infh);
  *scanbytes = CPiImageSpec.X_Length;
  *scanlines = CPiImageSpec.Y_Length;

}



int ReadCPiHdr (infh) 
int infh;
{
    LPCPIIMAGESPEC DescHdrPtr;
    LPCPIFILESPEC  SpecHdrPtr;
    int bytes;

    SpecHdrPtr = &CPiFileSpec; 

    if (_dos_read (infh, (char far *) SpecHdrPtr, 64, &bytes) != NULL || bytes != 64)
        return (0);


    DescHdrPtr = &CPiImageSpec;

    if (_dos_read (infh, (char far *) DescHdrPtr, 128, &bytes) != NULL || bytes != 128)
        return (0);

    return (1);
}


int WriteCPiFileSpec (outfh) 
int outfh;
{
    int i, num_written;
    char *ptr;

    strcpy (CPiFileSpec.CPiSignature,"CPI");
    CPiFileSpec.VersionNumber = 0x0100;
    CPiFileSpec.FileSpecLength = sizeof (CPIFILESPEC);
    CPiFileSpec.ImageCount = 1;
    CPiFileSpec.ImgDescTableOffset = sizeof (CPIFILESPEC);
    CPiFileSpec.ApplSpecReserved = NULL;
    ptr = CPiFileSpec.Filler;
    for (i = 0; i < sizeof (CPiFileSpec.Filler); i++)
        *ptr++ = 0xFF;

    if (_dos_write (outfh, (char far *) &CPiFileSpec, sizeof (CPIFILESPEC),&num_written) != NULL || num_written != sizeof (CPIFILESPEC))
        return (0);

    return (1);
}

int WriteCPiImgSpec (outfh) 
int outfh;
{
    int i, num_written;
    char *ptr;
    strcpy (CPiImageSpec.ImgName, "1.1");


    CPiImageSpec.ImgDescTag = 1;
    CPiImageSpec.ImgSpecLength = sizeof (CPIIMAGESPEC);
    CPiImageSpec.X_Origin = 0;
    CPiImageSpec.Y_Origin = 0;   
    CPiImageSpec.X_Length = wImageWidth;
    CPiImageSpec.Y_Length = wImageHeight;

    CPiImageSpec.NumberPlanes = 1;
    CPiImageSpec.NumberColors = 256;
    CPiImageSpec.Orientation = 0;
    CPiImageSpec.Compression = 0;
    CPiImageSpec.ImgSize = (DWORD) ((DWORD) wImageWidth * (DWORD) wImageHeight * 3L);

    CPiImageSpec.ImgSequence = 0;
    CPiImageSpec.NumberPalEntries = 256;
    CPiImageSpec.NumberBitsPerEntry = 24;
    CPiImageSpec.AttrBitsPerEntry = 0;
    CPiImageSpec.FirstEntryIndex = NULL;
    CPiImageSpec.RGBSequence = 0;
    CPiImageSpec.NextDescriptor = NULL;
    CPiImageSpec.ApplSpecReserved = NULL;        
 
    ptr = CPiImageSpec.Filler;

    for (i = 0; i < sizeof (CPiImageSpec.Filler); i++)
     *ptr++ = 0xFF;

    if (_dos_write (outfh, (char far *) &CPiImageSpec, sizeof (CPIIMAGESPEC), &num_written) != NULL || num_written != sizeof (CPIIMAGESPEC))
        return (0);

    return (1);
}


int CPiFixupFile (hFile)
int hFile;
{


    int nRetval = TRUE;

  /*  Finish up CPi file header information and close files  */


    dwCPiDataPos  = 960L;
    dwCPiPalPos   = 192L;

    CPiImageSpec.ImgDataOffset = dwCPiDataPos; 
    CPiImageSpec.PalDataOffset = dwCPiPalPos; 
    lseek (hFile, 64L, SEEK_SET);
    if (_dos_write (hFile, (char far *) &CPiImageSpec,sizeof (CPIIMAGESPEC),&bytes) != NULL || bytes != sizeof (CPIIMAGESPEC))
        nRetval = FALSE;


    _dos_close (hFile); 

    return (nRetval);
}



int GetLine (hFile, lpData, wNumBytes)
int hFile;
LPSTR lpData;
WORD wNumBytes;
{
    if (wInByteCount == 0)  // Need some data
    {
        _dos_read (hFile, lpInBuf, wReadBytes, &bytes);
        wInByteCount = wReadBytes;
        wInByteOffset = 0;
    }

    _fmemcpy (lpData, (lpInBuf + wInByteOffset), wNumBytes);
    wInByteCount -= wNumBytes;
    wInByteOffset += wNumBytes;
    return (1);
}

	


int PutLine (hFile, lpData, wNumBytes)
int hFile;
LPSTR lpData;
WORD wNumBytes;
{
    if (wOutByteCount == wWriteBytes)  // Need to write the data
    {
        _dos_write (hFile, lpOutBuf, wWriteBytes, &bytes);
        wOutByteCount  = 0;
    }

    _fmemcpy ((lpOutBuf + wOutByteCount), lpData, wNumBytes);
    wOutByteCount  += wNumBytes;
    return (1);
}


int SharpenLine (lpDest, lpPrev, lpCurr, lpNext, wScanWidth, wBytesPerRow)
LPSTR lpDest;
LPSTR lpPrev;
LPSTR lpCurr;
LPSTR lpNext;
WORD  wScanWidth;
WORD  wBytesPerRow;
{
    int i;
    int nRedVal;
    int nGrnVal;
    int nBluVal;
    int nTmp;
    int nDelta;

    LPSTR  lpPrevPtr = lpPrev;
    LPSTR  lpCurrPtr = lpCurr;
    LPSTR  lpNextPtr = lpNext;


    /*  For each pixel, look at 8 surrounding and exaggerate the difference  
        We are processing a 3 X 3 matrix  */

    for (i = 0; i < wScanWidth; i++)
    {
        nRedVal = (BYTE) *lpCurrPtr;

        /*  Gen an average for the 8 surrounding pixels  */

        nTmp    = ((BYTE)*(lpCurrPtr - 3) + (BYTE)*(lpCurrPtr + 3));
        nTmp   += ((BYTE)*(lpPrevPtr - 3) + (BYTE)*lpPrevPtr  + (BYTE)*(lpPrevPtr + 3));
        nTmp   += ((BYTE)*(lpNextPtr - 3) + (BYTE)*lpNextPtr  + (BYTE)*(lpNextPtr + 3));
        nTmp   >>= 3;

        nTmp   = (nRedVal - nTmp);
        if (nTmp < 0)
            nTmp -= sharpness;
        else
            nTmp += sharpness;
        
        nRedVal += nTmp;

        if (nRedVal < 0)
            nRedVal = 0;

        if (nRedVal > 255)
            nRedVal = 255;

        *lpDest++ = (BYTE) nRedVal;

        lpCurrPtr++;
        lpPrevPtr++;
        lpNextPtr++;


        nGrnVal = (BYTE) *lpCurrPtr;
        nTmp    = ((BYTE)*(lpCurrPtr - 3) + (BYTE)*(lpCurrPtr + 3));
        nTmp   += ((BYTE)*(lpPrevPtr - 3) + (BYTE)*lpPrevPtr  + (BYTE)*(lpPrevPtr + 3));
        nTmp   += ((BYTE)*(lpNextPtr - 3) + (BYTE)*lpNextPtr  + (BYTE)*(lpNextPtr + 3));
        nTmp   >>= 3;

        nTmp   = (nGrnVal - nTmp);
        if (nTmp < 0)
            nTmp -= sharpness;
        else
            nTmp += sharpness;
        
        nGrnVal += nTmp;

        if (nGrnVal < 0)
            nGrnVal = 0;

        if (nGrnVal > 255)
            nGrnVal = 255;

        *lpDest++ = (BYTE) nGrnVal;

        lpCurrPtr++;
        lpPrevPtr++;
        lpNextPtr++;



        nBluVal = (BYTE) *lpCurrPtr;
        nTmp    = ((BYTE)*(lpCurrPtr - 3) + (BYTE)*(lpCurrPtr + 3));
        nTmp   += ((BYTE)*(lpPrevPtr - 3) + (BYTE)*lpPrevPtr  + (BYTE)*(lpPrevPtr + 3));
        nTmp   += ((BYTE)*(lpNextPtr - 3) + (BYTE)*lpNextPtr  + (BYTE)*(lpNextPtr + 3));
        nTmp   >>= 3;

        nTmp   = (nBluVal - nTmp);
        if (nTmp < 0)
            nTmp -= sharpness;
        else
            nTmp += sharpness;
        
        nBluVal += nTmp;

        if (nBluVal < 0)
            nBluVal = 0;

        if (nBluVal > 255)
            nBluVal = 255;

        *lpDest++ = (BYTE) nBluVal;

        lpCurrPtr++;
        lpPrevPtr++;
        lpNextPtr++;


    }

    return (1);
}


