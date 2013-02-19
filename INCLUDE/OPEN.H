/****************************************************************************

   MODULE:           dlgopen.h
   DESCRIPTION:      defines for ImgPrep DLGOPEN
	VERSION:          VERSION 3.0
   DATE:             

*****************************************************************************/

/****************************** DLGOPEN *************************************/

#define  DLGOPEN_EDIT             101
#define  DLGOPEN_FILE_LISTBOX     102
#define  DLGOPEN_DIR_LISTBOX      103
#define  DLGOPEN_PATH             104
#define  DLGOPEN_TEXT             105
#define  DLGOPEN_FOLDOUT          106
#define  DLGOPEN_BIG              107
#define  DLGOPEN_SMALL            108

#define  DLGOPEN_OPTION           0xF000
#define  DLGOPEN_1BPP             0x0001
#define  DLGOPEN_4BPP             0x0002
#define  DLGOPEN_8BPP             0x0004
#define  DLGOPEN_24BPP            0x0008
#define  DLGOPEN_RLE4             0x0010
#define  DLGOPEN_RLE8             0x0020
#define  DLGOPEN_RGB              0x0040

#define  DLGOPEN_OPTION8          0x0080
#define  DLGOPEN_UNUSED           0















































/******************************* IFORMAT.C **********************************/

int  IdentFormat( int, LPSTR );
int  CheckByExt( int, LPSTR );
int  CheckAllTypes( int);
LPSTR GetExtension ( LPSTR );


























