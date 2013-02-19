#include <windows.h>
#include <stdio.h>
#include <dos.h>
#include "imgprep.h"
#include "cpifmt.h"

int ReadCPIHdr (infh, lpCPIFileSpec, lpCPIImageSpec) 
int infh;
LPCPIFILESPEC   lpCPIFileSpec;
LPCPIIMAGESPEC  lpCPIImageSpec;
{
    int bytes;

    lseek (infh, 0L, SEEK_SET);

    _dos_read (infh, (LPSTR) lpCPIFileSpec, sizeof (CPIFILESPEC), &bytes);

//  lseek (infh, lpCPIFileSpec -> ImgDescTableOffset, SEEK_SET);

    _dos_read (infh, (LPSTR) lpCPIImageSpec, sizeof (CPIIMAGESPEC), &bytes);

    return (1);
}
}

