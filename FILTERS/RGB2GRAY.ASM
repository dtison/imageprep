;********************************************************
;  11-22-89  (This program assumes input is in BGR order)
;********************************************************
; Special modifications for ImagePrep
;
;   12-29-89:  Changed over to new Filter (LPLPSTR, LPLPSTR, LPSTR) format. D. Ison
;
;   01-09-90:  Changed over to FAR PASCAL parameter convention.    D. Ison
;
;   07-09-90:  Straightened out more "Padded ScanWidth" stuff.



        include cmacros.inc

        ifndef SEGNAME
            SEGNAME equ <TEXT>
        endif
        createSeg _%SEGNAME, CodeSeg, word, public, CODE

        INCLUDE CPIASM.MAC
        .MODEL  MEDIUM

        .CODE   

        public  GraySum


;
;       GraySum        ((LPLPSTR) &lpDest, (LPLPSTR) &lpSource, lpSource);
;       (PASCAL)                     14                  10        6                



; Internal duplicate values from structure

wBytesPerRow      equ     Word Ptr [bp - 4]    ; (Input bytes per row)
wRowsThisStrip    equ     Word Ptr [bp - 6]
wPaddedScanWidth  equ     Word Ptr [bp - 8]

; Other Local variables

red             equ     Byte Ptr [bp - 10] 
green           equ     Byte Ptr [bp - 12] 
blue            equ     Byte Ptr [bp - 14] 
redsum          equ     Word Ptr [bp - 16] 
greensum        equ     Word Ptr [bp - 18] 
bluesum         equ     Word Ptr [bp - 20] 
TmpPtr          equ     Word Ptr [bp - 22] 

rgb2gray STRUC 

    bytes_per_row     dw ?    ; (Input 24 bit DWORD aligned bytes per row)
    padded_scanwidth  dw ?    ; Output DIB aligned pixel resolution (colormapped display)
    rows_this_strip   dw ?

rgb2gray ENDS


GraySum         proc    far


                @WinPrologue 26                         ; 26 local bytes  


; First copy structure info to local variables

                lds     si,DWord Ptr [bp + 6]           ; DS:SI = Structure ptr

                mov     ax,Word Ptr [si].bytes_per_row
                mov     wBytesPerRow,ax

                mov     ax,Word Ptr [si].padded_scanwidth
                mov     wPaddedScanWidth,ax             ; (This is also output bytes / row for 8 bit output)

                mov     ax,Word Ptr [si].rows_this_strip
                mov     wRowsThisStrip,ax
 


; Then setup IO Buffers

                @GetIOBufs                              


                cld
                mov     cx,wRowsThisStrip       ; Init row counter

RowLoop:        push    cx

                mov     cx,wPaddedScanWidth     ; Init pixel counter

                mov     TmpPtr,si               ; Save the current input pointer

PixelLoop:      push    cx


                lodsb                           ; Get blue component
                mov     blue,al 
                lodsb                           ; Get green component
                mov     green,al
                lodsb
                mov     red,al                  ; Get red 

; Do grayscale summing

                xor     ax,ax
                mov     al,green
                mov     cl,59
                mul     cl
                mov     greensum,ax

                xor     ax,ax
                mov     al,red  
                mov     cl,30
                mul     cl
                mov     redsum,ax

                xor     ax,ax
                mov     al,blue 
                mov     cl,11
                mul     cl

                add     ax,redsum
                add     ax,greensum

; AX now is weighted, so divide by 100

                mov     cl,100
                div     cl
                xor     ah,ah

; AX now is AL = Gray level (0..255)

                stosb                           ; Store the value in out buffer

                pop     cx
                loop    PixelLoop


;  Fixup stuff  for input DIB alignment stuff

                mov     si,TmpPtr
                add     si,wBytesPerRow
                

                pop     cx
                loop    RowLoop

                @WinEpilogue  26, 6             ; 26 local bytes; 6 parm bytes


GraySum	        endp



                end
