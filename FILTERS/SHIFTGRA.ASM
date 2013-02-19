; Special modifications for ImagePrep
;
;   12-29-89:  Changed over to new Filter (LPLPSTR, LPLPSTR, LPSTR) format. D. Ison
;
;   01-09-90:  Changed over to FAR PASCAL parameter convention.   D. Ison


        include cmacros.inc

        ifndef SEGNAME
            SEGNAME equ <TEXT>
        endif
        createSeg _%SEGNAME, CodeSeg, word, public, CODE

        INCLUDE CPIASM.MAC
        .MODEL  MEDIUM

        .CODE   

        public    ShiftGrayLevels 


;
;       ShiftGray  ((LPLPSTR) &lpDest, (LPLPSTR) &lpSource, lpSource);
;       (PASCAL)                14                  10        6                

; Internal duplicate values from structure

wBytesThisStrip equ     Word Ptr [bp - 4]
wBitsToShift    equ     Word Ptr [bp - 6]

shgray STRUC 

    bytes_this_strip  dw ? 
    bits_to_shift     dw ?

shgray ENDS
    

ShiftGrayLevels proc    far

                @WinPrologue 4                          ; 4 local bytes  


; First copy structure info to local variables

                lds     si,DWord Ptr [bp + 6]           ; DS:SI = Structure ptr

                mov     ax,Word Ptr [si].bytes_this_strip
                mov     wBytesThisStrip,ax
                mov     ax,Word Ptr [si].bits_to_shift 
                mov     wBitsToShift,ax

 
; Then setup IO Buffers

                @GetIOBufs                              

                cld
                mov     cx,wBitsToShift

; Use DX as counter: 0..wBytesThisStrip

                xor     dx,dx                   ; Clear counter

ShiftGrayLoop:  push    dx

                lodsb                           ; Get the byte
                shr     al,cl
                stosb                           ; Store the value in out buffer

                pop     dx
                inc     dx

                cmp     dx,wBytesThisStrip
                jb      ShiftGrayLoop

                @WinEpilogue  4, 6              ; 4 local bytes; 6 parm bytes

ShiftGrayLevels endp



                end
