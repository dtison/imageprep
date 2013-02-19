; Special modifications for ImagePrep
;
;   12-29-89:  Changed over to new Filter (LPLPSTR, LPLPSTR, LPSTR) format. D. Ison
;
;   01-09-90:  Changed over to FAR PASCAL parameter convention.    D. Ison
;
;   01-30-90:  Fixed bug that made 0's and 8's instead of 1's.     D. Ison


        include cmacros.inc

        ifndef SEGNAME
            SEGNAME equ <TEXT>
        endif
        createSeg _%SEGNAME, CodeSeg, word, public, CODE

        INCLUDE CPIASM.MAC
        .MODEL  MEDIUM

        .CODE   

        public  GrayToBW 


;**********************************************************
;  12-29-89  (This program assumes input is GRAYSCALE data)

;
;       GrayToBW    ((LPLPSTR) &lpDest, (LPLPSTR) &lpSource, lpDest);
;       (PASCAL)                 14                  10        6                


; Internal duplicate values from structure

wBytesThisStrip  equ     Word Ptr [bp - 4]

; Other Local variables
    
gray2bw  STRUC 

    bytes_this_strip  dw ? 

gray2bw  ENDS

GrayToBW        proc    far


                @WinPrologue 2                          ; 2 local bytes  

; First copy structure info to local variables

                lds     si,DWord Ptr [bp + 6]          ; DS:SI = Structure ptr

                mov     ax,Word Ptr [si].bytes_this_strip
                mov     wBytesThisStrip,ax

 
; Then setup IO Buffers

                @GetIOBufs                              


                cld
                xor     cx,cx                   ; Clear counter

GrayToBWLoop:   push    cx

                mov     cx,7
                lodsb                           
                and     al,10000000b
                shr     al,cl
                stosb                           ; Store the value in out buffer

                pop     cx
                inc     cx

                cmp     cx,wBytesThisStrip
                jb      GrayToBWLoop

                @WinEpilogue  2, 6              ; 2 local bytes; 6 parm bytes

GrayToBW        endp



                end

