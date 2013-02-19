
/* Error messages  */
/* --------------  */

#define   ERR_ERROR                 99
#define   ERR_SETHOT                100
#define   ERR_UFILETYPE             101
#define   ERR_OPENDLG               102
#define   ERR_SAVEDLG               103
#define   ERR_NOMEM                 104
#define   ERR_RDHDR                 105
#define   ERR_GSHADES               106
#define   ERR_RTGALS                107
#define   ERR_RTGAHDR               108
#define   ERR_RTIFLS                109
#define   ERR_RTIFHDR               110
#define   ERR_RTIFTAGS              111
#define   ERR_RPCXLS                112
#define   ERR_RPCXHDR               113
#define   ERR_UNGIF                 114
#define   ERR_WRHDR                 115
#define   ERR_UNTIFMON              116
#define   ERR_UNTIFCOMP             117
#define   ERR_SAMPIX                118
#define   ERR_BIGSTRIPS             119

#define   ERR_MEMORY1               120
#define   ERR_MEMORY2               121
#define   ERR_MEMORY3               122
#define   ERR_MEMORY4               123
#define   ERR_MEMORY5               124
#define   ERR_FILEREAD1             125
#define   ERR_FILEREAD2             126
#define   ERR_FILEWRITE1            127
#define   ERR_FILEWRITE2            128
#define   ERR_PALETTE               129
#define   ERR_INVALIDIMAGE          130
#define   ERR_NOFILENAME            131
#define   ERR_INVALID_FILENAME      132
#define   ERR_INVALID_PATH          133
#define   ERR_IMAGE_TOO_WIDE        134
#define   ERR_OIC_IMAGE_TOO_WIDE    135
#define   ERR_OIC_IMAGE_IS_LINES    136
#define   ERR_MERGE_TOO_WIDE        137
#define   ERR_NODISK                138




/************************** File Format Strings *****************************/

#define  RS_FORMATNAME                560
#define  RS_FMT_CPI24                 561       /*  Increment 1  */
#define  RS_FMT_TIF24                 562
#define  RS_FMT_EPS24                 563
#define  RS_FMT_TGA16                 564
#define  RS_FMT_TGA24                 565
#define  RS_FMT_TGA32                 566
#define  RS_FMT_PMDIB24               567
#define  RS_FMT_WNDIB24               568
#define  RS_FMT_WMF24                 569
#define  RS_FMT_DVA24                 570
#define  RS_FMT_CCPI24                571

#define  RS_FMT_CM                    572                       /* COLORMAP */
#define  RS_FMT_CPI                   572                       /* COLORMAP */
#define  RS_FMT_TIF                   573
#define  RS_FMT_TGA                   574
#define  RS_FMT_PCX                   575
#define  RS_FMT_GIF                   576
#define  RS_FMT_PMDIB                 577      
#define  RS_FMT_WNDIB                 578      
#define  RS_FMT_EPS                   579
#define  RS_FMT_WMF                   580

/************************** File Format EXT *********************************/

#define  RS_FORMATEXT                 600
#define  RS_EXT_CPI24                 600                   /* RGB FORMATS */
#define  RS_EXT_TIF24                 601
#define  RS_EXT_EPS24                 602
#define  RS_EXT_TGA16                 603
#define  RS_EXT_TGA24                 604
#define  RS_EXT_TGA32                 605 
#define  RS_EXT_PMDIB24               606 
#define  RS_EXT_WNDIB24               607
#define  RS_EXT_WMF24                 608
#define  RS_EXT_DVA24                 609
#define  RS_EXT_CCPI24                610

#define  RS_EXT_CM                    612                       /* COLORMAP */
#define  RS_EXT_CPI                   612                       /* COLORMAP */
#define  RS_EXT_TIF                   613
#define  RS_EXT_TGA                   614
#define  RS_EXT_PCX                   615
#define  RS_EXT_GIF                   616
#define  RS_EXT_PMDIB                 617      
#define  RS_EXT_WNDIB                 618      
#define  RS_EXT_EPS                   619
#define  RS_EXT_WMF                   620


// New strings for 3.1

#define  STR_MODES                          677
#define  STR_MODE_REAL                      677 
#define  STR_MODE_STD                       678
#define  STR_MODE_ENH                       679 
                                            
#define  STR_CPUS                           680
#define  STR_CPU_286                        680
#define  STR_CPU_386                        681
#define  STR_CPU_486                        682
                                            
#define  STR_CLASSES                        683
#define  STR_CLS_CLRMAP                     683
#define  STR_CLS_RGB                        684


// String table stuff probably ought to be in imgprep.h because string constants cannot
// be isolated to any given module...

#define  STR_FREE_MEM                       700
#define  STR_DISK_SPACE                     701
#define  STR_SAVE_AS                        702
#define  STR_SAVING_IMAGE                   703
#define  STR_INSUFFICIENT_DISK_SAVE         704
#define  STR_UPIXELS                        705
#define  STR_LBYTES                         706 
#define  STR_MENU_FILEMGR                   708
#define  STR_MENU_CTRLPANEL                 709
#define  STR_MENU_CAPTURE_ON                710
#define  STR_MENU_CAPTURE_OFF               711
#define  STR_DDWINDOWS                      712
#define  STR_NO_IMAGE_ACTIVE                713
#define  STR_DWIDTH                         714
#define  STR_DHEIGHT                        715
#define  STR_OPEN                           716
#define  STR_ABANDON_DISPLAY                717
#define  STR_IMAGE_DISPLAY                  718
#define  STR_GEN_HISTOGRAM                  719
#define  STR_GEN_PALETTE                    720
#define  STR_OPT_COLORS                     721
#define  STR_MAP_COLORSPACE                 722
#define  STR_COLORMAP                       723
#define  STR_GRAY                           724
#define  STR_MONOCHROME                     725
#define  STR_IMAGE_IS_WIDE                  726
#define  STR_IMAGE_SIZE_WARNING             727
#define  STR_MENU_DISPLAY_ON                728
#define  STR_MENU_DISPLAY_OFF               729
#define  STR_PALETTE_OPTIMIZATION           730
#define  STR_DISPLAY_BITS                   731
#define  STR_APPNAME                        732
#define  STR_CONFIRM_EXIT                   733
#define  STR_MENU_EXITWINDOWS               734
#define  STR_COMPRESSING_IMAGE              735
#define  STR_DECOMPRESSING_IMAGE            736
#define  STR_OIC                            737
#define  STR_UNABLE_OPEN_S                  738
#define  STR_NOT_EXIST_S                    739
#define  STR_CANNOT_OVERWRITE_S             740
#define  STR_IMAGE_EXISTS_CONFIRM           741
#define  STR_OIC_RATIO                      742
#define  STR_OIC_RESULTS                    743
#define  STR_OPT_UCOLORS                    744
#define  STR_OPT_FROMU                      745
#define  STR_TIF8_WARNING                   746
#define  STR_LINEAR_DITHER                  747
#define  STR_FS_DITHER                      748
#define  STR_BURKES_DITHER                  749
#define  STR_MERGE                          750
#define  STR_SOURCE_1_EXIST                 751
#define  STR_SOURCE_2_EXIST                 752
#define  STR_MUST_ENTER_SOURCE_1            753
#define  STR_MUST_ENTER_SOURCE_2            754
#define  STR_SOURCE_1_NOT_24                755
#define  STR_SOURCE_2_NOT_24                756
#define  STR_MUST_ENTER_MERGED              757
#define  STR_SOURCE_IMAGE_1                 758
#define  STR_SOURCE_IMAGE_2                 759
#define  STR_MERGED_IMAGE                   760
#define  STR_SOURCE_MERGED_DIFF             761
#define  STR_ORG_NOT_NEGATIVE               762
#define  STR_LENGTH_NOT_NEGATIVE            763 
#define  STR_ORG_LENGTH_TOO_BIG             764
#define  STR_MERGE_POINT_LOG                765
#define  STR_MERGE_POINT_PHYS               766
#define  STR_MERGE_X_NEGATIVE               767
#define  STR_CLOSEAPP                       768
#define  STR_CAUTION                        769
#define  STR_DELETE_FILE_CONFIRM            770
#define  STR_CUT                            771
#define  STR_CUTTING                        772
#define  STR_COPY                           773
#define  STR_COPYING                        774
#define  STR_CONVERTING                     775
#define  STR_DECODING_RLE_TGA               776
#define  STR_D                              777
#define  STR_VGA_LIMITATION                 778
#define  STR_VGA_MESSAGE                    779
#define  STR_PRINT                          780
#define  STR_PRINTING                       781
#define  STR_PREFERENCES                    782
#define  STR_CLIPBOARD_FMT_DIB              783
#define  STR_ABOUT_S                        784
#define  STR_DEFAULT_COLOR                  785
#define  STR_OPR_COLOR                      786
#define  STR_GRAYSCALE                      787
#define  STR_GRAYSCALE_4BIT                 788
#define  STR_BLACK_WHITE                    789
#define  STR_NO_DITHER                      790
#define  STR_ORDERED_DITHER                 791
#define  STR_FS_DITHER2                     792
#define  STR_BK_DITHER2                     793
#define  STR_LIN_DITHER2                    794
