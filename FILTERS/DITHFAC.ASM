; Special modifications for ImagePrep
;
; 7-10-90   Changed this dither to be more DIB-like in its approach.  Added
;           the "wInputBytesPerRow" parameter to allow Padded Scanwidth dithers..
;
; 8-22-90   Unified interface with (* other *) err-dist dithers



        include cmacros.inc

        ifndef SEGNAME
            SEGNAME equ <TEXT>
        endif
        createSeg _%SEGNAME, CodeSeg, word, public, CODE

        INCLUDE CPIASM.MAC
        .MODEL  MEDIUM

        .CODE   


; (FAST FILTER)

        public  _dither_facolor

;       

; dither_facolor (lpData, lpErrBuf, scanbytes, wRowsThisStrip, errbuf_len, clip_table, lpSmallhist, lpPalette, wBytesPerRow);
;                 6       10        14          16              18            20         22  24     26  28       30

scanbytes      equ     Word Ptr [bp + 14]
rows_this_strip equ    Word Ptr [bp + 16]
errbuf_len     equ     Word Ptr [bp + 18]
clip_table     equ     Word Ptr [bp + 20]
; (Smallhist is 22-24)
lpPalette      equ     DWord Ptr [bp + 26]
wBytesPerRow   equ     Word Ptr [bp + 30]  ;  Input bytes per row

red_err_next   equ     Byte Ptr [bp - 2]
green_err_next equ     Byte Ptr [bp - 4]
blue_err_next  equ     Byte Ptr [bp - 6]
red_err        equ     Word Ptr [bp - 8]
green_err      equ     Word Ptr [bp - 10]
blue_err       equ     Word Ptr [bp - 12]
red_ptr        equ     Word Ptr [bp - 14]
green_ptr      equ     Word Ptr [bp - 16]
blue_ptr       equ     Word Ptr [bp - 18]
red            equ     Byte Ptr [bp - 20]
green          equ     Byte Ptr [bp - 22]
blue           equ     Byte Ptr [bp - 24]
old_ds         equ     Word Ptr [bp - 26]
total_err      equ     Byte Ptr [bp - 28]
curr_dotcount  equ     Word Ptr [bp - 30]
outptr         equ     Word Ptr [bp - 32]
true_red       equ     Byte Ptr [bp - 34]
true_blue      equ     Byte Ptr [bp - 36]
true_green     equ     Byte Ptr [bp - 38]
is_zero        equ     Word Ptr [bp - 40]
is_scanbytes   equ     Word Ptr [bp - 42]
TmpPtr         equ     Word Ptr [bp - 44]

rgb      STRUC 

    rgbblue     db      ?
    rgbgreen    db      ?
    rgbred      db      ?
    rgbgray     db      ?

rgb      ENDS

_dither_facolor proc    far

                push    bp
                mov     bp,sp
                sub     sp,46
                push    si
                push    di
                push    ds
                push    es

                mov     ax,ds
                mov     old_ds,ax                       ; Save segment for later  

                lds     si,DWord Ptr [bp + 6]           ; DS:SI = input buffer
                les     di,DWord Ptr [bp + 22]          ; ES:DI = lpSmallhist
                cld

                mov     outptr,si                       ; Point to buffer 
                
                mov     cx,0                            ; From 0..rows_this_strip 
next_row:       push    cx


                mov     TmpPtr,si                       ; Save si for wInputBytesPerRow stuff

                mov     cx,0                            ; From 0..scanbytes
next_dot:       push    cx
                
blu:            lodsb                                   ; Get next byte  (blue) 
                shr     al,1
                shr     al,1
                shr     al,1
                
                mov     dl,al                           ; DL = blue
                mov     blue,al

                
grn:            lodsb                                   ; Get next byte  (green) 
                shr     al,1
                shr     al,1
                shr     al,1
                mov     green,al
                mov     ch,al                           ; CH = green
                

                lodsb                                   ; Get next byte  (red) 
                shr     al,1
                shr     al,1
                shr     al,1
                mov     red,al
                mov     cl,al                           ; CL = red



get_clr:

; Now do (red << 10) + (green << 5) + (blue)

                xor     ah,ah
                mov     al,red
                mov     cl,10
                shl     ax,cl
                mov     dx,ax
                
                xor     ah,ah 
                mov     al,green
                mov     cl,5
                shl     ax,cl
                add     dx,ax
                
                xor     ah,ah
                mov     al,blue
                add     dx,ax
                
                mov     bx,dx
                mov     al,Byte Ptr es:[di + bx]        ; Closest color value

                mov     bx,outptr
                mov     Byte Ptr [bx],al
                inc     outptr 
                
                pop     cx 
                inc     cx
                cmp     cx,scanbytes
                je      scanline_done
                jmp     far ptr next_dot

scanline_done:


;  Fixup stuff  for input DIB alignment stuff

                mov     si,TmpPtr
                add     si,wBytesPerRow


                pop     cx
                inc     cx
                cmp     cx,rows_this_strip
                je      fa_done
                jmp     far ptr next_row

fa_done:
                pop     es
                pop     ds
                pop     di
                pop     si
                add     sp,46
                pop     bp
                ret

_dither_facolor endp


                end
