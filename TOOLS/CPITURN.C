/*  
  
 This program rotates CPi files.  
 It is intended to be a model for implementing a general purpose disk
 based image rotation scheme.


 (Somewhat parallels an image conversion)


1)  Before calling:
        Open input and output files, hInFile, hOutFile;

2)  Init Header

3)  Position file pointer to point to image data

4)  Pass to function: 

        hInFile 
        hOutFile
        lpBuff1
        lpBuff2
        wBytesPerRow
        wImageHeight
        wRowsPerStrip
        lpfnRdConvertData (Data Strip Reader)


This program will attempt to emulate this intended architecture

    D. Ison

*/


#include <windows.h>
#include <win30.h>
#include <stdio.h>
#include <malloc.h> 
#include <string.h>
#include <dos.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <fcntl.h>
#include <io.h>
#include "cpifmt.h"

int   _aDBused;
int   bytes;
int   infh, outfh;

#define TRUE 1
#define FALSE 0
#define COLOR 0
#define GRAY  1
char *scan_buf;

char in_filename[13];
char out_filename[13];

int put_line();
void get_img_info();

int ReadCPiHdr();

DWORD   dwCPiPalPos, dwCPiDataPos; 

CPIFILESPEC     CPiFileSpec;
CPIIMAGESPEC    CPiImageSpec;




WORD wImageWidth;
WORD wImageHeight;

WORD wPaddedImgWidth;

WORD wInBitsPerPixel;
WORD wInBytesPerPixel;
WORD wInBytesPerScan;


WORD wOutBitsPerPixel;
WORD wOutBytesPerPixel;
WORD wOutBytesPerScan;
unsigned wBytes;

DWORD dwBytesToRead, dwBytesRead;

WORD  wInByteCount  = 0xFFFF;
WORD  wOutByteCount = 0;

LPSTR lpInData, lpOutData, lpTmpData;

WORD  wByteCount1, wNumBytes;




int RotateImage (int, int, LPSTR, LPSTR, WORD, WORD, WORD, WORD);
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
       fputs ("\nusage:  cpiturn <filename>.cpi  <filename>.cpi\n\n",stdout); 
       exit (0);
   }


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

   if (! cpiturn (Oldname,Newname, ScanBuffer, EncBuffer, Palette)) 
       exit (-1);

}







int cpiturn (oldname, newname, scan_buf, enc_buf, palette) 
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

        lseek (outfh, 960L, SEEK_SET);
        lseek (infh, 960L, SEEK_SET);
        RotateImage (infh, outfh, enc_buf, scan_buf, wBytesPerRow, wImageHeight, wRowsPerStrip, wBytesPerPixel);

    }     

    CPiFixupFile (outfh);


    _dos_close (infh);
    _dos_close (outfh);

    return (retval);
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
    CPiImageSpec.X_Length = wImageHeight;
    CPiImageSpec.Y_Length = wImageWidth;

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


int RotateImage (hInFile, hOutFile, lpBuff1, lpBuff2, wBytesPerRow, wImageHeight, wRowsPerStrip, wBytesPerPixel)
int   hInFile;
int   hOutFile;
LPSTR lpBuff1;
LPSTR lpBuff2;
WORD  wBytesPerRow;
WORD  wImageHeight;
WORD  wRowsPerStrip;
WORD  wBytesPerPixel;
{
    char Buffer [30];
    WORD wBytesTurnedPerRead;
    WORD wRowsPerOutputStrip;
    WORD wBytesPerOutputRow;
    WORD wBytesPerOutputStrip;
    WORD wCurrentOffset;
    WORD wBytesPerStrip;
    unsigned bytes;
    WORD wLinesRead;

    LPSTR  lpInPtr, lpOutPtr;
    LPSTR  lpTmpInPtr, lpTmpOutPtr;
    WORD   i, j, k, l;

    WORD   wRowsThisStrip;

    wBytesPerOutputRow    = wImageHeight * wBytesPerPixel;
    wRowsPerOutputStrip   = 65000 / wBytesPerOutputRow;
    wBytesPerOutputStrip  = wBytesPerOutputRow * wRowsPerOutputStrip;
    wBytesPerStrip        = (wBytesPerRow * wRowsPerStrip);

    wCurrentOffset        = 0;      // From right
    wLinesRead            = 0;


    clr_scrn ();
    say (3,0,"Rotating image... ");

    do
    {
        lpOutPtr   = lpBuff2;
        do   
        {
            lpInPtr   = (lpBuff1 + wBytesPerRow - wCurrentOffset);   // Point to rightmost byte of current box 
          //lpInPtr   -= (wBytesPerPixel - 1); 
            lpInPtr--;
            _dos_read (hInFile, lpBuff1, wBytesPerStrip, &bytes);
    
            if ((wImageHeight - wLinesRead) > wRowsPerStrip)
                wRowsThisStrip = wRowsPerStrip;
            else
                wRowsThisStrip  = (wImageHeight - wLinesRead);


            /*  Transform the "box" into output buffer  */ 
    
            for (j = 0; j < wRowsThisStrip; j++)
            {
    
                lpTmpOutPtr = lpOutPtr;
                lpTmpInPtr  = lpInPtr;

                for (k = 0; k < wRowsPerOutputStrip; k++)
                {

                    if (wBytesPerPixel == 1)
                        *lpTmpOutPtr = *lpTmpInPtr--;
                    else
                    {
                     // *(lpTmpOutPtr    )  = *lpTmpInPtr--;
                     // *(lpTmpOutPtr + 2)  = *lpTmpInPtr--;
                     // *(lpTmpOutPtr + 1)  = *lpTmpInPtr--;


                        *(lpTmpOutPtr + 2)  = *lpTmpInPtr--;
                        *(lpTmpOutPtr + 1)  = *lpTmpInPtr--;
                        *(lpTmpOutPtr    )  = *lpTmpInPtr--;

                    }

                    lpTmpOutPtr += wBytesPerOutputRow;
                }
                lpInPtr   += wBytesPerRow;
                lpOutPtr  += wBytesPerPixel;

            }
    
            wLinesRead += wRowsThisStrip; 

        } while (wLinesRead < wImageHeight);

        /*  At this point, we have read and processed for an entire output strip, so we
            write the strip, reset wLinesRead, pointers, etc. 
        */

        _dos_write (hOutFile, lpBuff2, wBytesPerOutputStrip, &bytes);

        wLinesRead = 0;
        wCurrentOffset += wRowsPerOutputStrip * wBytesPerPixel;

        lseek (hInFile, 960L, SEEK_SET);

        sprintf (Buffer, "%2.0f percent complete\n", (float) ((float) wCurrentOffset / (float) wBytesPerRow * 100.0));
        say (3,20,Buffer);

    }  while (wCurrentOffset < wBytesPerRow);


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

