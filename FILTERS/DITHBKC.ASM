; Special modifications for ImagePrep
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

;  (BURKE FILTER)

        public  _dither_bkcolor


.286P
;       

; dither_bkcolor (lpData, lpErrBuf, scanbytes, wRowsThisStrip, errbuf_len, clip_table, lpSmallhist, lpPalette, wBytesPerRow);
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
red_err_next2   equ     Byte Ptr [bp - 46]
green_err_next2 equ     Byte Ptr [bp - 48]
blue_err_next2  equ     Byte Ptr [bp - 50]


rgb      STRUC 

    rgbblue     db      ?
    rgbgreen    db      ?
    rgbred      db      ?
    rgbgray     db      ?

rgb      ENDS

_dither_bkcolor proc    far

                push    bp
                mov     bp,sp
                sub     sp,56
                push    si
                push    di
                push    ds
                push    es

                mov     ax,ds
                mov     old_ds,ax                       ; Save segment for later  

                lds     si,DWord Ptr [bp + 6]           ; DS:SI = input buffer
                les     di,DWord Ptr [bp + 10]          ; ES:DI = error buffer
                cld

                mov     red_err_next,0                  ; Init next errs for this
                mov     green_err_next,0                ; strip
                mov     blue_err_next,0


                mov     red_err_next2,0                  ; Init next errs for this
                mov     green_err_next2,0                ; strip
                mov     blue_err_next2,0


                mov     outptr,si                       ; Point to buffer 
                
                mov     cx,0                            ; From 0..rows_this_strip 
next_row:       push    cx

; Transfer previous error to current error line

                push    ds
                push    si
                push    di
                lds     si,DWord Ptr [bp + 10]          ; Point to error buffer
                add     si,errbuf_len                   ; Point to previous portion
                mov     cx,errbuf_len
                rep     movsb
                pop     di
                pop     si
                pop     ds

; Zero out previous error line

                push    di
                add     di,errbuf_len                   ; Point to previous portion
                mov     cx,errbuf_len
                mov     al,0
                rep     stosb
                pop     di


; Setup forward and current error lists

                mov     ax,di
                add     ax,errbuf_len                   ; Make fwd err list
                mov     red_err,ax
                add     ax,scanbytes  
                inc     ax
                mov     green_err,ax
                add     ax,scanbytes 
                inc     ax
                mov     blue_err,ax

; (Current pointers) 

                mov     ax,red_err
                sub     ax,errbuf_len
                mov     red_ptr,ax
                
                mov     ax,green_err
                sub     ax,errbuf_len
                mov     green_ptr,ax
                
                mov     ax,blue_err
                sub     ax,errbuf_len
                mov     blue_ptr,ax


                mov     TmpPtr,si                       ; Save si for wInputBytesPerRow stuff

; Add errors from previous line to each dot in current line

                mov     cx,0                            ; From 0..scanbytes
next_dot:       push    cx
                mov     curr_dotcount,cx                ; Save the dot count
                
blu:            lodsb                                   ; Get next byte  (blue) 
                shr     al,1
                shr     al,1
                
                add     al,blue_err_next
                mov     bx,blue_ptr
                mov     ah,Byte Ptr es:[bx]
                add     al,ah
                mov     dl,al                           ; DL = blue
                inc     blue_ptr 


                
                
grn:            lodsb                                   ; Get next byte  (green) 
                shr     al,1
                shr     al,1
                
                add     al,green_err_next
                mov     bx,green_ptr
                mov     ah,Byte Ptr es:[bx]
                add     al,ah
                mov     ch,al                           ; CH = green
                inc     green_ptr 
                



                lodsb                                   ; Get next byte  (red) 
                shr     al,1
                shr     al,1

                add     al,red_err_next
                mov     bx,red_ptr
                mov     ah,Byte Ptr es:[bx]
                add     al,ah
                mov     cl,al                           ; CL = red
                inc     red_ptr 

clip:
; Clip using clip table to save compare/jumps


table_clip:

                push    ds
                mov     ax,old_ds  
                mov     ds,ax
                mov     bx,clip_table  
                mov     al,cl                           ; Red
                xlat
                mov     red,al
                
                mov     al,ch                           ; Green
                xlat
                mov     green,al
                
                mov     al,dl                           ; Blue
                xlat
                mov     blue,al
                
                pop     ds

; Now have adjusted and clipped red, green and blue values, so find color in table

get_clr:
                push    es
                push    di
                les     di,DWord Ptr [bp + 22]          ; ES:DI = Smallhist

; Now do (red << 10) + (green << 5) + (blue)

                xor     ah,ah
                mov     al,red
                shr     ax,1                            ; Go to 5 bits first
                mov     cl,10
                shl     ax,cl
                mov     dx,ax
                
                xor     ah,ah 
                mov     al,green
                shr     ax,1                            
                mov     cl,5
                shl     ax,cl
                add     dx,ax
                
                xor     ah,ah
                mov     al,blue
                shr     ax,1                            
                add     dx,ax
                
                mov     bx,dx
                mov     al,Byte Ptr es:[di + bx]        ; Closest color value

                mov     bx,outptr
                mov     Byte Ptr [bx],al
                inc     outptr 
                

                pop     di
                pop     es
 

; Calculate error amounts..


                push    ds
                push    si

                lds     si,lpPalette                    ; Point to windows quad palette 

                mov     cl,2                            ; Bits to shift windows pal vals
                xor     bx,bx

                mov     bl,al                           ; AL = color

                shl     bx,1
                shl     bx,1                            

                mov     al,Byte Ptr [si + bx].rgbred
                shr     al,cl
                mov     true_red,al
                sub     red,al
                
                mov     al,Byte Ptr [si + bx].rgbgreen
                shr     al,cl
                mov     true_green,al
                sub     green,al
                
                mov     al,Byte Ptr [si + bx].rgbblue
                shr     al,cl
                mov     true_blue,al
                sub     blue,al

                pop     si
                pop     ds

; Red, green and blue now contain error amounts; now add to err lists    
; if  0 < count < scanbytes

                mov     red_err_next,0                  ; Init next errs for this
                mov     green_err_next,0                ; dot   
                mov     blue_err_next,0
                mov     red_err_next2,0                 ; Init next errs for this
                mov     green_err_next2,0               ; dot   
                mov     blue_err_next2,0

                mov     is_zero,0                       ; Init special case flags
                mov     is_scanbytes,0                  ; for 0 and n.  (n = scanbytes)
                
                mov     cx,curr_dotcount
                cmp     cx,0
                jne     chk_cnt
                mov     is_zero,1
                jmp     short chk_done   
                
chk_cnt:        cmp     cx,scanbytes
                jne     chk_done  
                mov     is_scanbytes,1

chk_done:  



do_errs:        mov     cx,5                            ; Bits to shift (Burkes)


;---------------------------
; Do error distribution. . .
;---------------------------

; (Red) 
; Will always do "8" position, so add here first  (was 5)

                mov     al,red
                sar     al,2                            ; 8 / 32 = 1 / 4

                mov     bx,red_err
                add     bx,curr_dotcount                ; (same as "j" in C code)
                add     Byte Ptr es:[bx],al
                mov     total_err,al                    ; Track total error

; Do "8" and "4" positions only if curr_dotcount < scanbytes (was "7" and "1")
; ("8" position)


                mov     al,red
                sar     al,2

                cmp     is_scanbytes,1                  ; If yes, skip error adding
                je      tot_8_red
                mov     red_err_next,al
tot_8_red:      add     total_err,al                    ; Track total error

; ("4" Position)


                mov     al,red
                sar     al,3                            ; 4 / 32 = 1 / 8


                cmp     is_scanbytes,1                  ; If yes, skip error adding
                je      tot_4_red  
                mov     bx,red_err
                add     bx,curr_dotcount                ; (same as "j" in C code)
                inc     bx                              ; (j + 1)
                add     Byte Ptr es:[bx],al
tot_4_red:      add     total_err,al                    ; Track total error


; Do "4" and "2" positions only if curr_dotcount < scanbytes (FS has none)
; ("4" position)

                mov     al,red
                sar     al,3

                cmp     is_scanbytes,1                  ; If yes, skip error adding
                je      tot_4_red2
                mov     red_err_next2,al
tot_4_red2:     add     total_err,al                    ; Track total error

; ("2" Position)

                mov     al,red
                sar     al,4                            ; 2 / 32 = 1 / 16

                cmp     is_scanbytes,1                  ; If yes, skip error adding
                je      tot_2_red  
                mov     bx,red_err
                add     bx,curr_dotcount                ; (same as "j" in C code)
                inc     bx                              ; (j + 1)
                inc     bx                              ; (j + 1)
                add     Byte Ptr es:[bx],al
tot_2_red:      add     total_err,al                    ; Track total error







; ("4" Position)  (was 3)

                mov     al,red
                sar     al,3



; Add "4" position only if curr_dotcount > 0

                cmp     is_zero,1                       ; If yes, skip error adding
                je      tot_4_red3
                mov     bx,red_err
                add     bx,curr_dotcount                ; (same as "j" in C code)
                dec     bx                              ; (j - 1)
                add     Byte Ptr es:[bx],al
tot_4_red3:     add     total_err,al                    ; Track total error
   


; ("2" Position)  (was none in FS)

                mov     al,red
                sar     al,4



; Add "2" position only if curr_dotcount > 0

                cmp     is_zero,1                       ; If yes, skip error adding
                je      tot_2_red2
                mov     bx,red_err
                add     bx,curr_dotcount                ; (same as "j" in C code)
                dec     bx                              ; (j - 1)
                dec     bx                              ; (j - 1)
                add     Byte Ptr es:[bx],al
tot_2_red2:     add     total_err,al                    ; Track total error



; Make sure dispersed error = difference.  Find red_err - dispersed_err 
; and put into "4" position  (if curr_dotcount < scanbytes)

                cmp     is_scanbytes,1
                je      skip_total_red
                
                mov     al,total_err
                sub     red,al
                mov     al,red
                
                mov     bx,red_err
                add     bx,curr_dotcount                ; (same as "j" in C code)
                inc     bx                              ; (j + 1)
                add     Byte Ptr es:[bx],al
                

skip_total_red:




;***************************



; (Green)
; Will always do "8" position, so add here first  (was 5)

                mov     al,green
                sar     al,2


                mov     bx,green_err
                add     bx,curr_dotcount                ; (same as "j" in C code)
                add     Byte Ptr es:[bx],al
                mov     total_err,al                    ; Track total error

; Do "8" and "4" positions only if curr_dotcount < scanbytes (was "7" and "1")
; ("8" position)

                mov     al,green
                sar     al,2

                cmp     is_scanbytes,1                  ; If yes, skip error adding
                je      tot_8_green
                mov     green_err_next,al
tot_8_green:      add     total_err,al                    ; Track total error

; ("4" Position)

                mov     al,green
                sar     al,3


                cmp     is_scanbytes,1                  ; If yes, skip error adding
                je      tot_4_green  
                mov     bx,green_err
                add     bx,curr_dotcount                ; (same as "j" in C code)
                inc     bx                              ; (j + 1)
                add     Byte Ptr es:[bx],al
tot_4_green:      add     total_err,al                    ; Track total error


; Do "4" and "2" positions only if curr_dotcount < scanbytes (FS has none)
; ("4" position)


                mov     al,green
                sar     al,3

                cmp     is_scanbytes,1                  ; If yes, skip error adding
                je      tot_4_green2
                mov     green_err_next2,al
tot_4_green2:     add     total_err,al                    ; Track total error

; ("2" Position)

                mov     al,green
                sar     al,4

                cmp     is_scanbytes,1                  ; If yes, skip error adding
                je      tot_2_green  
                mov     bx,green_err
                add     bx,curr_dotcount                ; (same as "j" in C code)
                inc     bx                              ; (j + 1)
                inc     bx                              ; (j + 1)
                add     Byte Ptr es:[bx],al
tot_2_green:      add     total_err,al                    ; Track total error







; ("4" Position)  (was 3)

                mov     al,green
                sar     al,3



; Add "4" position only if curr_dotcount > 0

                cmp     is_zero,1                       ; If yes, skip error adding
                je      tot_4_green3
                mov     bx,green_err
                add     bx,curr_dotcount                ; (same as "j" in C code)
                dec     bx                              ; (j - 1)
                add     Byte Ptr es:[bx],al
tot_4_green3:     add     total_err,al                    ; Track total error
   


; ("2" Position)  (was none in FS)

                mov     al,green
                sar     al,4



; Add "2" position only if curr_dotcount > 0

                cmp     is_zero,1                       ; If yes, skip error adding
                je      tot_2_green2
                mov     bx,green_err
                add     bx,curr_dotcount                ; (same as "j" in C code)
                dec     bx                              ; (j - 1)
                dec     bx                              ; (j - 1)
                add     Byte Ptr es:[bx],al
tot_2_green2:     add     total_err,al                    ; Track total error



; Make sure dispersed error = difference.  Find green_err - dispersed_err 
; and put into "4" position  (if curr_dotcount < scanbytes)

                cmp     is_scanbytes,1
                je      skip_total_green
                
                mov     al,total_err
                sub     green,al
                mov     al,green
                
                mov     bx,green_err
                add     bx,curr_dotcount                ; (same as "j" in C code)
                inc     bx                              ; (j + 1)
                add     Byte Ptr es:[bx],al
                

skip_total_green:



;***************************


; (Blue)
; Will always do "8" position, so add here first  (was 5)

 
                mov     al,blue
                sar     al,2


                mov     bx,blue_err
                add     bx,curr_dotcount                ; (same as "j" in C code)
                add     Byte Ptr es:[bx],al
                mov     total_err,al                    ; Track total error

; Do "8" and "4" positions only if curr_dotcount < scanbytes (was "7" and "1")
; ("8" position)


                mov     al,blue
                sar     al,2

                cmp     is_scanbytes,1                  ; If yes, skip error adding
                je      tot_8_blue
                mov     blue_err_next,al
tot_8_blue:      add     total_err,al                    ; Track total error

; ("4" Position)


                mov     al,blue
                sar     al,3


                cmp     is_scanbytes,1                  ; If yes, skip error adding
                je      tot_4_blue  
                mov     bx,blue_err
                add     bx,curr_dotcount                ; (same as "j" in C code)
                inc     bx                              ; (j + 1)
                add     Byte Ptr es:[bx],al
tot_4_blue:      add     total_err,al                    ; Track total error


; Do "4" and "2" positions only if curr_dotcount < scanbytes (FS has none)
; ("4" position)


                mov     al,blue
                sar     al,3

                cmp     is_scanbytes,1                  ; If yes, skip error adding
                je      tot_4_blue2
                mov     blue_err_next2,al
tot_4_blue2:     add     total_err,al                    ; Track total error

; ("2" Position)

                mov     al,blue
                sar     al,4

                cmp     is_scanbytes,1                  ; If yes, skip error adding
                je      tot_2_blue  
                mov     bx,blue_err
                add     bx,curr_dotcount                ; (same as "j" in C code)
                inc     bx                              ; (j + 1)
                inc     bx                              ; (j + 1)
                add     Byte Ptr es:[bx],al
tot_2_blue:      add     total_err,al                    ; Track total error







; ("4" Position)  (was 3)


                mov     al,blue
                sar     al,3



; Add "4" position only if curr_dotcount > 0

                cmp     is_zero,1                       ; If yes, skip error adding
                je      tot_4_blue3
                mov     bx,blue_err
                add     bx,curr_dotcount                ; (same as "j" in C code)
                dec     bx                              ; (j - 1)
                add     Byte Ptr es:[bx],al
tot_4_blue3:     add     total_err,al                    ; Track total error
   


; ("2" Position)  (was none in FS)


                mov     al,blue
                sar     al,4



; Add "2" position only if curr_dotcount > 0

                cmp     is_zero,1                       ; If yes, skip error adding
                je      tot_2_blue2
                mov     bx,blue_err
                add     bx,curr_dotcount                ; (same as "j" in C code)
                dec     bx                              ; (j - 1)
                dec     bx                              ; (j - 1)
                add     Byte Ptr es:[bx],al
tot_2_blue2:     add     total_err,al                    ; Track total error



; Make sure dispersed error = difference.  Find blue_err - dispersed_err 
; and put into "4" position  (if curr_dotcount < scanbytes)

                cmp     is_scanbytes,1
                je      skip_total_blue
                
                mov     al,total_err
                sub     blue,al
                mov     al,blue
                
                mov     bx,blue_err
                add     bx,curr_dotcount                ; (same as "j" in C code)
                inc     bx                              ; (j + 1)
                add     Byte Ptr es:[bx],al
                

skip_total_blue:




;***************************



skip_errdist:  
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
                je      bk_done
                jmp     far ptr next_row

bk_done:
                pop     es
                pop     ds
                pop     di
                pop     si
                add     sp,56
                pop     bp
                ret

_dither_bkcolor endp


                end
