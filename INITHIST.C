#include <windows.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <malloc.h> 
#include <cpifmt.h>

char filename [13];

int hFile;
int hOutFile;
unsigned bytes;

void FAR find_closesta ();


typedef struct 
{
    BYTE NumEntries;
    BYTE Entries [256];
}   COLORCELL;


typedef COLORCELL FAR *LPCOLORCELL;

BYTE UsedList [256];

int  main (argc,argv)
int argc;
char *argv[];
{

    LPSTR lpInputBuf;
    LPSTR lpTmpBuf;
    LPSTR lpSmallhist;

    unsigned ir, ig, ib;
    unsigned space_idx, small_color;
    LPSTR lpPalette, lpPtr;
    LPSTR lpHistPtr;
    LPSTR lpPalBuf;
    LPSTR lpBuf1;

    WORD  FAR *lpwSquares;
    BYTE color;
    WORD i, j;
    LPSTR  lpCurrHistPtr;
    RGBQUAD FAR *lpRGBQuadPtr;
    RGBQUAD FAR *lpRGBQuadPal;

    LPCOLORCELL lpColorCells;
    LPCOLORCELL lpCellPtr;

    WORD wIndex = 0;
    WORD wNumColors;
    int  Red, Green, Blue;
    int  TmpRed, TmpGreen, TmpBlue;

    int  nTmp;

    WORD wDist;
    WORD wMinDist;
    WORD wTotal = 0;



    lpBuf1      = _fmalloc (512);
    lpInputBuf  = _fmalloc (32768);
    lpPalBuf    = _fmalloc (1024);
    lpTmpBuf    = _fmalloc (65500);
    lpSmallhist = _fmalloc (32768);


    _fmemset (lpTmpBuf, 0, 65500);

    /*  Prepare squares table to avoid doing multiplys  */

    lpwSquares = (WORD FAR *) lpBuf1;
    for (i = 0; i < 64; i++)
        lpwSquares [i] = (i * i);


    lpRGBQuadPal = (RGBQUAD FAR *) lpInputBuf;
    lpRGBQuadPtr = lpRGBQuadPal;
    lpPtr = lpPalette = lpPalBuf;
    lpColorCells = (LPCOLORCELL) lpTmpBuf;
      
    wNumColors = atoi (argv [2]);

    switch (wNumColors)
    {
    
        case 256:
        case 16:
        case 8:
        case 2:
        break;

        default:
        exit (0);
    }

    strcpy (filename, argv [1]);

    if (_dos_open (filename, O_RDONLY, &hFile) != 0)
        return (0);

    

    _dos_read (hFile, lpRGBQuadPtr, 1024, &bytes);

/*  Need 6 bit palette  */


    for (i = 0; i < wNumColors; i++)
    {
        *lpPtr++ = lpRGBQuadPtr -> rgbRed   >> 2;
        *lpPtr++ = lpRGBQuadPtr -> rgbGreen >> 2;
        *lpPtr++ = lpRGBQuadPtr -> rgbBlue  >> 2;
        lpRGBQuadPtr++;
    }

    lpRGBQuadPtr = lpRGBQuadPal;


    printf ("Creating cells\n");


    /*  Go thru all colors in "org" colorspace */

    for (i = 0; i < 256; i++)
    {

        Red   = (BYTE) (lpRGBQuadPtr -> rgbRed   >> 6);
        Green = (BYTE) (lpRGBQuadPtr -> rgbGreen >> 6);
        Blue  = (BYTE) (lpRGBQuadPtr -> rgbBlue  >> 6);


        lpCellPtr = &lpColorCells [(Red << 4) + (Green << 2) + Blue];

        wMinDist = 0xFFFF;

        /*  Now find all entries in palette having these same first 2 bits in each plane  */

        for (j = 0; j < 256; j++)
        {
        
            if (! UsedList [j])
            {
                TmpRed   = (BYTE) (lpRGBQuadPal [j].rgbRed >> 6);
                if (TmpRed == Red)
                {
                    TmpGreen = (BYTE) (lpRGBQuadPal [j].rgbGreen >> 6);
                    if (TmpGreen == Green)
                    {

                        TmpBlue = (BYTE) (lpRGBQuadPal [j].rgbBlue >> 6);
                        if (TmpBlue == Blue)
                        {

                            /*  We have a match to put in this cell */

                            lpCellPtr -> Entries [lpCellPtr -> NumEntries] = j;
                            lpCellPtr -> NumEntries++;
                            UsedList [j] = TRUE;
                            wTotal++;


                            /*  Calculate actual 5 bit distance */

                            Red   = (BYTE) (lpRGBQuadPtr -> rgbRed   >> 3);
                            Green = (BYTE) (lpRGBQuadPtr -> rgbGreen >> 3);
                            Blue  = (BYTE) (lpRGBQuadPtr -> rgbBlue  >> 3);

                            TmpRed   = (BYTE) (lpRGBQuadPal [j].rgbRed >> 3);
                            nTmp   = TmpRed - Red;
                            wDist  = (WORD) (nTmp * nTmp);

                            TmpGreen   = (BYTE) (lpRGBQuadPal [j].rgbGreen >> 3);
                            nTmp   = TmpGreen - Green;
                            wDist  += (WORD) (nTmp * nTmp);

                            TmpBlue = (BYTE) (lpRGBQuadPal [j].rgbBlue >> 3);
                            nTmp   = TmpBlue - Blue;
                            wDist  += (WORD) (nTmp * nTmp);

                            if (wDist < wMinDist)
                                wMinDist = wDist;

                        }
                    }
                }
            }
        }


        /*  Find all OTHER indexes within that distance from this color */

        for (j = 0; j < 256; j++)
        {
        
            Red   = (BYTE) (lpRGBQuadPtr -> rgbRed   >> 6);
            Green = (BYTE) (lpRGBQuadPtr -> rgbGreen >> 6);
            Blue  = (BYTE) (lpRGBQuadPtr -> rgbBlue  >> 6);

            if (! UsedList [j])
            {
                TmpRed   = (BYTE) (lpRGBQuadPal [j].rgbRed >> 6);
                if (TmpRed != Red)
                {
                    TmpGreen = (BYTE) (lpRGBQuadPal [j].rgbGreen >> 6);
                    if (TmpGreen != Green)
                    {

                        TmpBlue = (BYTE) (lpRGBQuadPal [j].rgbBlue >> 6);
                        if (TmpBlue != Blue)
                        {

                            /*  Calculate actual 5 bit distance */

                            Red   = (BYTE) (lpRGBQuadPtr -> rgbRed   >> 3);
                            Green = (BYTE) (lpRGBQuadPtr -> rgbGreen >> 3);
                            Blue  = (BYTE) (lpRGBQuadPtr -> rgbBlue  >> 3);

                            TmpRed   = (BYTE) (lpRGBQuadPal [j].rgbRed >> 3);
                            nTmp   = TmpRed - Red;
                            wDist  = (WORD) (nTmp * nTmp);

                            TmpGreen   = (BYTE) (lpRGBQuadPal [j].rgbGreen >> 3);
                            nTmp   = TmpGreen - Green;
                            wDist  += (WORD) (nTmp * nTmp);

                            TmpBlue   = (BYTE) (lpRGBQuadPal [j].rgbBlue >> 3);
                            nTmp   = TmpBlue - Blue;
                            wDist  += (WORD) (nTmp * nTmp);

                            if (wDist < wMinDist)
                            {
                                lpCellPtr -> Entries [lpCellPtr -> NumEntries] = j;
                                lpCellPtr -> NumEntries++;
                                UsedList [j] = TRUE;
                                wTotal++;
                            }



                        }
                    }
                }
            }

        }
        lpRGBQuadPtr++;

    }


    printf ("Total: %u \n",wTotal);


    /*  Now that we know what colors belong in what "cells", search for
        closest color in much smaller range  */



    for (ir = 0; ir < 32; ir++)
    {
        for (ig = 0; ig < 32; ig++)
            for (ib = 0; ib < 32; ib++)
            {


                lpCellPtr = &lpColorCells [((ir >> 3) << 4) + ((ig >> 3) << 2) + (ib >> 3)];

                /*  Now we know what cell to search in.  Search for closest color  */

//              if (0)
                if (lpCellPtr -> NumEntries > 0)
                {

                    wMinDist = 0xFFFF;
    
                    for (j = 0; j < lpCellPtr -> NumEntries; j++)
                    {
    
                        Red   = (BYTE) lpRGBQuadPal [lpCellPtr -> Entries [j]].rgbRed;
                        Green = (BYTE) lpRGBQuadPal [lpCellPtr -> Entries [j]].rgbGreen;
                        Blue  = (BYTE) lpRGBQuadPal [lpCellPtr -> Entries [j]].rgbBlue;
    
    
                        Red   >>= 3;
                        Green >>= 3;
                        Blue  >>= 3;
    
                        nTmp   = ir - Red;
                        wDist  = (WORD) (nTmp * nTmp);
    
                        nTmp   = ig - Green;
                        wDist  += (WORD) (nTmp * nTmp);
    
                        nTmp   = ib -  Blue;
                        wDist  += (WORD) (nTmp * nTmp);
    
                        if (wDist < wMinDist)
                        {
                            color = lpCellPtr -> Entries [j];
                            wMinDist = wDist;
    
                        }

                    }

                }
                else
                    color = find_closesta ((ir << 1), (ig << 1), (ib << 1), lpPalette, lpwSquares, (WORD) wNumColors);

                space_idx = (unsigned) ((ir << 10) + (ig << 5) + ib);

                lpCurrHistPtr = (lpSmallhist + space_idx);
                *lpCurrHistPtr   = color;
                wIndex++;

            }
            printf ("Index %u\r",wIndex);
    }

    if (_dos_creat ("hist", _A_NORMAL, &hOutFile) != 0)
        return (0);


    _dos_write (hOutFile, lpSmallhist, 32768, &bytes);


    return (TRUE);


}


