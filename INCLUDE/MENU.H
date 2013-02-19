
/*------------------------------ FILE MENU ---------------------------------*/

#define IDM_OPEN                  103                   
#define IDM_CLOSE                 104                   
#define IDM_SAVE                  105                   
#define IDM_EXIT                  106                   
#define IDM_SCAN                  107
#define IDM_LOGO                  108
#define IDM_PRINT                 109 
#define IDM_PRINTSETUP            110

/*------------------------------ EDIT MENU ---------------------------------*/
#define IDM_UNDO                  120
#define IDM_CUT                   121
#define IDM_COPY                  122
#define IDM_PASTE                 123
#define IDM_FLIP                  124
#define IDM_MIRROR                125
#define IDM_SCALE                 126
#define IDM_ROTATE                127
#define IDM_ROTATECCW             128
#define IDM_MERGE                 129

/*------------------------------ VIEW MENU ---------------------------------*/

#define IDM_DISPLAY_TOGGLE        130
#define IDM_ACTUALSIZE            131
#define IDM_FITINWINDOW           132
#define IDM_VIEWNORMAL            133
#define IDM_VIEWQUALITY           134
#define IDM_VIEWTRUECOLOR         135
#define IDM_PREFERENCES           136
#define IDM_INFO                  137
#define IDM_FULLSCREEN            138

/*------------------------------ PROCESS MENU ------------------------------*/

#define IDM_SET_DISP              140
#define IDM_RETAIN_TOGGLE         141
#define IDM_DEFAULT_COLOR         142 
#define IDM_8BIT_OPR              143   /*  Same as old IDM_OPR_COLOR  */
#define IDM_GRAYSCALE             144
#define IDM_MONOCHROME            145
#define IDM_INVERTCOLORS          146

#define IDM_GRAYSCALE16           147
#define IDM_4BIT_OPR              148

#define IDM_NO_DITHER             150   
#define IDM_ORDERED               151
#define IDM_FS                    152
#define IDM_BURKE                 153 
#define IDM_FAST                  154
#define IDM_RELAXED               155
#define IDM_STRICT                156 
#define IDM_SETUP_OPR             157
#define IDM_LINE_DRAWING          158 

#define IDM_4BIT_COLOR            9000
#define IDM_8BIT_COLOR            9001

#define IDM_PROCESSES             142
#define IDM_DITHERS               150
#define OLD

#ifdef OLD
#define IDM_OPR_COLOR             143
#endif

#define IDM_SHARPEN               160
#define IDM_SMOOTH                161
#define IDM_REMOVE_NOISE          162
#define IDM_ENHANCE_EDGE          163

/*------------------------------ ENHANCE MENU --------------------------------**/

#define IDM_GAMMA                 180 
#define IDM_RGB                   181 
#define IDM_BC                    182 
#define IDM_BALANCE               183

/*------------------------------ CAPTURE MENU ------------------------------*/

#define IDM_ACTIVATE_TOGGLE       200        

#define IDM_CAPAREAMASK           201        
#define IDM_CAP_SCREEN            202        
#define IDM_CAP_WINDOW            203        
#define IDM_CAP_CLIENT            204        
#define IDM_CAP_AREA              205        
                                     
#define IDM_SETHOTKEY             206        
                                      
#define IDM_CAPMASK               207        
#define IDM_CAPCPI                208        
#define IDM_CAPTIF                209        
#define IDM_CAPTGA                210        
#define IDM_CAPPCX                211        
#define IDM_CAPGIF                212        
#define IDM_CAPDIB                213        
#define IDM_HIDE_WINDOW           214
#define IDM_CAP_TO_CLIPBOARD      215
#define IDM_NAME_CAPFILE          216


/*------------------------------ HELP MENU --------------------------------**/

#define IDM_HELPINDEX             220
#define IDM_HELPKEYBOARD          221
#define IDM_HELPCOMMANDS          222
#define IDM_HELPPROCEDURES        223
#define IDM_HELPGLOSSARY          224
#define IDM_HELPHELP              225
#define IDM_ABOUT                 226


/*------------------------------ SYSTEM MENU --------------------------------**/

#define IDM_FILEMGR               240 
#define IDM_CTRLPANEL             241  
#define IDM_EXITWINDOWS           242
