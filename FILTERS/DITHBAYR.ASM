/***  This file has been taken out of service.   3/91  D. Ison  ***/



; Special modifications for ImagePrep
;
;   12-29-89:  Changed over to new Filter (LPLPSTR, LPLPSTR, LPSTR) format. D. Ison
;
;   01-09-90:  Changed over to FAR PASCAL parameter convention.    D. Ison
;
;   07-09-90:  Made parameters in DITHERBAYER  struct wPaddedScanWidth (from lpDispInfo)
;              And straightened out more "Padded ScanWidth" stuff.

        extrn         _Pattern        :  WORD

        include cmacros.inc

        ifndef SEGNAME
            SEGNAME equ <TEXT>
        endif
        createSeg _%SEGNAME, CodeSeg, word, public, CODE

        INCLUDE CPIASM.MAC
        .MODEL  MEDIUM

        .CODE   


        public  DitherBayer_A


      

; DitherBayer_A ((LPLSTR) &lpDest, (LPLPSTR) &lpSource, LPSTR);
; (PASCAL)                  14                  10         6


; (PaddedWidth = wPaddedBytesPerRow)


; Internal duplicate values from structure

wBytesPerRow      equ     Word Ptr [bp - 4]
wRowsThisStrip    equ     Word Ptr [bp - 6]
wScrnY            equ     Word Ptr [bp - 8]
wPaddedScanWidth  equ     Word Ptr [bp - 10]
                
; Other Local variables

wRed              equ     Word Ptr [bp - 12]
wGreen            equ     Word Ptr [bp - 14]
wBlue             equ     Word Ptr [bp - 16]
TmpPtr            equ     Word Ptr [bp - 18]
i                 equ     Word Ptr [bp - 24]
j                 equ     Word Ptr [bp - 26]
old_ds            equ     Word Ptr [bp - 28]
                  

dithbayr STRUC 

    bytes_per_row     dw ?    ; (Input 24 bit DWORD aligned bytes per row)
    padded_scanwidth  dw ?    ; Output DIB aligned pixel resolution (colormapped display)
    rows_this_strip   dw ?
    scrn_y            dw ?    ; Vertical index into image (for bayer dither) 

dithbayr ENDS


                
DitherBayer_A   proc    far



                @WinPrologue 30                         ; 30 local bytes  

; First save the current DS
  
                mov     ax,ds
                mov     old_ds,ax                       ; Save segment for later  


; Next copy structure info to local variables

                lds     si,DWord Ptr [bp + 6]           ; DS:SI = Structure ptr

                mov     ax,Word Ptr [si].bytes_per_row
                mov     wBytesPerRow,ax

                mov     ax,Word Ptr [si].padded_scanwidth
                mov     wPaddedScanWidth,ax             ; (This is also output bytes / row for 8 bit output)

                mov     ax,Word Ptr [si].rows_this_strip
                mov     wRowsThisStrip,ax

                mov     ax,Word Ptr [si].scrn_y
                mov     wScrnY,ax

 
; Then setup IO Buffers

                @GetIOBufs                              

                cld
                
                
                mov     cx,0                            ; From 0..wRowsThisStrip
next_row:       push    cx
                mov     i,cx                            ; Save the row count

                mov     TmpPtr,si                       ; Save the current input pointer
                
                mov     cx,0                            ; From 0..wPaddedScanWidth
next_pixel:     push    cx
                mov     j,cx                            ; Save the current pixel count
                
                mov     cx,2
                
                xor     ax,ax
                lodsb                                   ; Get next byte  (red) 
                shr     al,cl
                mov     wRed,ax
                
                lodsb                                   ;
                shr     al,cl
                mov     wGreen,ax
                
                lodsb                                   ;
                shr     al,cl
                mov     wBlue,ax
                
                xor     bx,bx
                mov     ax,j
                
                and     ax,0007h;                       ; (j & 7)
                mov     cx,4                            ; 3 bits mul by 8, 1 bit for WORD size
                shl     ax,cl                           ; 
                
                
                mov     bx,ax                           ; Now have [(j & 7)]
                
                mov     ax,i
                add     ax,wScrnY
                and     ax,0007h;                       ; (i & 7)
                shl     ax,1                            ; Double for WORD element size
                add     bx,ax                           ; Now have [(i & 7)]
                
                add     bx,offset _Pattern
                
                push    ds
                mov     ax,old_ds
                mov     ds,ax
                
                mov     cx, Word Ptr [bx]               ; Int value at Pattern [(j & 7)] [(i & 7)]
                mov     ch,cl                           ; Save the byte in ch
                
                pop     ds
                
                
; (Variables bRedIdx, bGreenIdx, bBlueIdx have been replaced with al, ah  and dl.)
                
                
                mov     cl,15
                xor     bh,bh
                
                mov     bl,ch                   ; Put bTestVal into bl
                sub     bx,wRed                 ; Is wRed > bTestval ?
                shr     bx,cl                   ; If so, bx will contain negative
                mov     al,bl                   ; bRedIdx 
                
                mov     bl,ch                   ; Put bTestVal into bl
                sub     bx,wGreen               ; Is wGreen > bTestval ?
                shr     bx,cl
                mov     ah,bl                   ; bBlueIdx
                
                mov     bl,ch                   ; Put bTestVal into bl
                sub     bx,wBlue                ; Is wBlue  > bTestval ?
                shr     bx,cl
                mov     dl,bl                   ; bBlueIdx
                
                
 
index:          shl     al,1                    ; bRedIdx 
                shl     al,1
                
                shl     ah,1                    ; bGreenIdx
                
                xor     bx,bx
                or      bl,al                   ; bRedIdx
                or      bl,ah                   ; bGreenIdx;
                or      bl,dl                   ; bBlueIdx
                
                
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
                je      bayer_done
                jmp     far ptr next_row

bayer_done:
                @WinEpilogue  30, 6             ; 28 local bytes; 6 parm bytes

DitherBayer_A   endp



                end

