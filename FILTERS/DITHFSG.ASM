;  1-10-90  This is a "lower level" filter invoked by DitherGrayFS (..)
;  1-24-90  Added a slight contrast adjustment to input data before
;           dithering.  Improved output noticeably.

;  (D. Ison)

        include cmacros.inc

        ifndef SEGNAME
            SEGNAME equ <TEXT>
        endif
        createSeg _%SEGNAME, CodeSeg, word, public, CODE

        INCLUDE CPIASM.MAC
        .MODEL  MEDIUM

        .CODE   

        public  _dither_fsgray

; dither_fsgray (lnline, err_buf, scanbytes, rows_this_strip, errbuf_len, clip_table);
;                 6       10      14           16             18          20         

; Added serpentine raster   1-2-90  D. Ison

scanbytes       equ     Word Ptr [bp + 14]
rows_this_strip equ     Word Ptr [bp + 16]
errbuf_len      equ     Word Ptr [bp + 18]
clip_table      equ     Word Ptr [bp + 20]


err_next        equ     Byte Ptr [bp - 2]
err             equ     Word Ptr [bp - 4]
err_ptr         equ     Word Ptr [bp - 6]
gray            equ     Byte Ptr [bp - 8]
old_ds          equ     Word Ptr [bp - 10]
total_err       equ     Byte Ptr [bp - 12]
curr_dotcount   equ     Word Ptr [bp - 14]
outptr          equ     Word Ptr [bp - 16]
true_lvl        equ     Byte Ptr [bp - 18]
is_zero         equ     Word Ptr [bp - 20]
is_scanbytes    equ     Word Ptr [bp - 22]
is_forward      equ     Word Ptr [bp - 24]

_dither_fsgray  proc    far

                push    bp
                mov     bp,sp
                sub     sp,26
                push    si
                push    di
                push    ds
                push    es

                mov     ax,ds
                mov     old_ds,ax                       ; Save segment for later  

                lds     si,DWord Ptr [bp + 6]           ; DS:SI = input buffer
                les     di,DWord Ptr [bp + 10]          ; ES:DI = error buffer

                mov     err_next,0                      ; Init next err for this
                mov     is_forward,1                    ; Start with left to right traversal

                mov     cx,0                            ; From 0..rows_this_strip 
next_row2lvl:   push    cx


; Setup serpentine raster

                cmp     is_forward,1
                je      go_forward
                std                                     ; Go in reverse
                add     si,scanbytes


go_forward:     cld                                     ; Go forward

                mov     outptr,si                       ; Point to buffer 

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
                mov     err,ax
                
; (Current pointers) 

                mov     ax,err
                sub     ax,errbuf_len
                mov     err_ptr,ax


; Add errors from previous line to each dot in current line

                mov     cx,0                            ; From 0..scanbytes
next_dot2lvl:   push    cx
                mov     curr_dotcount,cx                ; Save the dot count
                
                lodsb                                   ; Get next byte  
                shr     al,1
                shr     al,1

; Add a little contrast to the data   1-23-90  D. Ison
; Maybe later I'll take this jump out just for the fun of it
; Could be done with pair of values indexed in memory...

;               cmp     al,31
;               jg      hi  
;               sub     al,2
;               jmp     short cont_done
;hi:             add     al,2



cont_done:      add     al,err_next
                mov     bx,err_ptr
                mov     ah,Byte Ptr es:[bx]
                add     al,ah
                mov     cl,al                           ; CL = Gray level 
                inc     err_ptr 


; Clip using clip table to save compare/jumps

                push    ds
                mov     ax,old_ds  
                mov     ds,ax
                mov     bx,clip_table  
                mov     al,cl                           ; Red
                xlat
                pop     ds


; Now have adjusted and clipped gray values, now truncate to 1 bit 





; Now al = level
               mov     gray,al

               cmp     al,31  
               ja      is_one
               mov     al,0                            ; AL is palette value (level)
               mov     cl,0                            ; CL is palette position
               jmp     short store_2lvl
is_one:        mov     al,63                           ; Palette value (level)
               mov     cl,1                            ; Palette position


store_2lvl:    mov     bx,si
               cmp     is_forward,1
               je      is_fwd
               inc     bx
               jmp     short store2

is_fwd:        dec     bx      
store2:        mov     Byte Ptr [bx],cl





; Calculate error amount


                sub     gray,al

; gray now contains error amount, now add to err lists    
; if  0 < count < scanbytes

                
                mov     err_next,0                  ; Init next errs for this
                mov     is_zero,0                   ; Init special case flags
                mov     is_scanbytes,0              ; for 0 and n.  (n = scanbytes)
                
                mov     cx,curr_dotcount
                cmp     cx,0
                jne     chk_cnt2l
                mov     is_zero,1
                jmp     short chk_done2l
                
chk_cnt2l:      cmp     cx,scanbytes
                jne     chk_done2l
                mov     is_scanbytes,1
                
chk_done2l:

do_errs2lvl:    mov     cx,4                            ; Bits to shift

; Do error distribution (gray). . .
; Will always do "5" position, so add here first

                mov     ax,5
                imul    gray
                sar     ax,cl
                mov     bx,err
                add     bx,curr_dotcount                ; (same as "j" in C code)
                add     Byte Ptr es:[bx],al
                mov     total_err,al                    ; Track total error

; Do "7" and "1" positions only if curr_dotcount < scanbytes
; ("7" position)

                mov     ax,7
                imul    gray
                sar     ax,cl                           ; AL now is error to add
                cmp     is_scanbytes,1                  ; If yes, skip error adding
                je      tot_7_2lvl
                mov     err_next,al
tot_7_2lvl:     add     total_err,al                    ; Track total error

; ("1" Position)
                mov     al,gray                          ; Just (1 * err) for this one
                sar     ax,cl                           ; AL now is error to add
                cmp     is_scanbytes,1                  ; If yes, skip error adding
                je      tot_1_2lvl
                mov     bx,err
                add     bx,curr_dotcount                ; (same as "j" in C code)
                inc     bx                              ; (j + 1)
                add     Byte Ptr es:[bx],al
tot_1_2lvl:     add     total_err,al                    ; Track total error


; ("3" Position)

                mov     ax,3
                imul    gray
                sar     ax,cl                           ; AL now is error to add

; Add "3" position only if curr_dotcount > 0

                cmp     is_zero,1                       ; If yes, skip error adding
                je      tot_3_2lvl
                mov     bx,err
                add     bx,curr_dotcount                ; (same as "j" in C code)
                dec     bx                              ; (j - 1)
                add     Byte Ptr es:[bx],al
tot_3_2lvl:     add     total_err,al                    ; Track total error
   


; Make sure dispersed error = difference.  Find gray_err - dispersed_err 
; and put into "1" position  (if curr_dotcount < scanbytes)

                cmp     is_scanbytes,1
                je      skip_total2l
                
                mov     al,total_err
                sub     gray,al
                mov     al,gray
                
                mov     bx,err
                add     bx,curr_dotcount                ; (same as "j" in C code)
                inc     bx                              ; (j + 1)
                add     Byte Ptr es:[bx],al
                

skip_total2l:

skip_err2lvl:  
                pop     cx 
                inc     cx
                cmp     cx,scanbytes
                je      scanline_done2lvl
                jmp     far ptr next_dot2lvl

scanline_done2lvl:

                pop     cx
                inc     cx
                cmp     cx,rows_this_strip
                je      fs_done2lvl
                jmp     far ptr next_row2lvl

fs_done2lvl:    cld
                pop     es
                pop     ds
                pop     di
                pop     si
                add     sp,26
                pop     bp
                ret

_dither_fsgray  endp





                end

