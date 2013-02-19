; Special modifications for ImagePrep
;
;   12-29-89:  Changed over to new Filter (LPLPSTR, LPLPSTR, LPSTR) format. D. Ison
;
;   01-09-90:  Changed over to FAR PASCAL parameter convention.    D. Ison
;
;   07-10-90:  Made parameters in UNIFORMQUANT struct wPaddedScanWidth (from lpDispInfo)
;              And straightened out more "Padded ScanWidth" stuff.

        include cmacros.inc

        ifndef SEGNAME
            SEGNAME equ <TEXT>
        endif
        createSeg _%SEGNAME, CodeSeg, word, public, CODE

        INCLUDE CPIASM.MAC
        .MODEL  MEDIUM

        .CODE   
		

        public  UniformQuant8_A


      


; UniformQuant8_A ((LPLPSTR) &lpDest, (LPLPSTR) &lpSource, lpSource);
; (PASCAL)                      14                10           6                

; Internal duplicate values from structure

wBytesPerRow      equ     Word Ptr [bp - 4]    ; (Input bytes per row)
wRowsThisStrip    equ     Word Ptr [bp - 6]
wPaddedScanWidth  equ     Word Ptr [bp - 8]

; Other Local variables

i                 equ     Word Ptr [bp - 10]
j                 equ     Word Ptr [bp - 12]
TmpPtr            equ     Word Ptr [bp - 14]


uniquant STRUC 

    bytes_per_row     dw ?    ; (Input 24 bit DWORD aligned bytes per row)
    padded_scanwidth  dw ?    ; Output DIB aligned pixel resolution (colormapped display)
    rows_this_strip   dw ?

uniquant ENDS


UniformQuant8_A proc    far
  


                @WinPrologue 18                         ; 18 local bytes  

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

                mov     cx,0                            ; From 0..wRowsThisStrip
next_row:       push    cx
                mov     i,cx                            ; Save the row count

                mov     TmpPtr,si                       ; Save the current input pointer

                mov     cx,0                            ; From 0..wScanPaddedWidth
next_pixel:     push    cx
                mov     j,cx                            ; Save the current pixel count

                lodsb       
                and     al,80h                          ; Take top bit
                mov     cl,7
                shr     al,cl
                mov     ch,al

                lodsb                                   ;
                and     al,80h                          ; Take top bit 
                mov     cl,6
                shr     al,cl
                mov     dl,al 

                lodsb                                   ; Get next byte  (red) 
                and     al,80h                          ; Take top (most significant) bit 
                mov     cl,5
                shr     al,cl
                mov     dh,al

                xor     bx,bx
                or      bl,ch                          ; Blue
                or      bl,dl                          ; Green
                or      bl,dh                          ; Red  

                mov     al,bl
                stosb

                pop     cx
                inc     cx
                cmp     cx,wPaddedScanWidth
                je      scanline_done
                jmp     far ptr next_pixel

scanline_done:


;  Fixup stuff  for input DIB alignment stuff

                mov     si,TmpPtr

                add     si,wBytesPerRow

                pop     cx
                inc     cx
                cmp     cx,wRowsThisStrip 
                je      uni16_done
                jmp     far ptr next_row

uni16_done:
                @WinEpilogue  18, 6             ; 18 local bytes; 6 parm bytes

UniformQuant8_A endp



                end




