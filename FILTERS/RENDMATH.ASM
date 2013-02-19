; Special modifications for ImagePrep
;
;   10-21-89:   Took out decrement one convention as was in old colorlab
;   11-21-89:  Created RGBToGraySum to perform gray summing 
;
;        
            DOSSEG
            .MODEL MEDIUM 
 

            .CODE  REDUCE_TEXT

            public  _get_squares
            public  _process_scan
            public  _count_unique
            public  _find_closesta

            public  _get_hista


;
;   get_squares (int, int, int, long *, long *, long *);
;


_get_squares    proc    near

                push    bp
                mov     bp,sp
 
                mov     ax,Word Ptr [bp + 4]    ; First integer to square
                mov     cx,Word Ptr [bp + 4]    
                imul    cx

                mov     bx,Word Ptr [bp + 10]
                mov     Word Ptr [bx],ax        ; First long integer passed
                mov     Word Ptr [bx + 2],dx


                mov     ax,Word Ptr [bp + 6]    ; Second integer to square
                mov     cx,Word Ptr [bp + 6]    
                imul    cx

                mov     bx,Word Ptr [bp + 12]
                mov     Word Ptr [bx],ax        ; Second long integer passed
                mov     Word Ptr [bx + 2],dx

                mov     ax,Word Ptr [bp +  8]    ; Third integer to square
                mov     cx,Word Ptr [bp +  8]    
                imul    cx

                mov     bx,Word Ptr [bp + 14]
                mov     Word Ptr [bx],ax        ; Third long integer passed
                mov     Word Ptr [bx + 2],dx


                pop     bp
                ret
_get_squares    endp





;
;   process_scan (lpData, lpbHist, lpwHist, wBitsShift, wBytesPerStrip,
;                 int *, int *, int *, int *, int *, int *);
;




curr_rmin      equ     Word Ptr [bp - 6] 
curr_rmax      equ     Word Ptr [bp - 8] 
curr_gmin      equ     Word Ptr [bp - 10] 
curr_gmax      equ     Word Ptr [bp - 12] 
curr_bmin      equ     Word Ptr [bp - 14] 
curr_bmax      equ     Word Ptr [bp - 16] 
curr_ds        equ     Word Ptr [bp - 18] 


_process_scan proc    far

                push    bp
                mov     bp,sp
                sub     sp,24                   ; 24 bytes local space

                push    ds
                push    es
                push    si
                push    di
                
                
                mov     ax,ds
                mov     curr_ds,ax              ; Save this ds
                
; Setup min/max  for each color
                
; Red            
                mov     bx,Word Ptr [bp + 22]    ; &rmin
                mov     ax,Word Ptr [bx]
                mov     curr_rmin,ax             ; Save current min
                
                mov     bx,Word Ptr [bp + 24]    ; &rmax
                mov     ax,Word Ptr [bx]
                mov     curr_rmax,ax             ; Save current max
                
                
; Green         
                mov     bx,Word Ptr [bp + 26]    ; &gmin
                mov     ax,Word Ptr [bx]
                mov     curr_gmin,ax             ; Save current min
                
                mov     bx,Word Ptr [bp + 28]    ; &gmax
                mov     ax,Word Ptr [bx]
                mov     curr_gmax,ax             ; Save current max
                
; Blue          
                mov     bx,Word Ptr [bp + 30]    ; &bmin
                mov     ax,Word Ptr [bx]
                mov     curr_bmin,ax             ; Save current min
                
                mov     bx,Word Ptr [bp + 32]    ; &bmax
                mov     ax,Word Ptr [bx]
                mov     curr_bmax,ax             ; Save current max
                
                
                
; Setup far buf fers
                
                lds     si,DWord Ptr [bp + 6]
                les     di,DWord Ptr [bp + 14]
                
                
                cld
                
                xor     bx,bx                    ; Clear counter
                
process_loop:   push    bx
                
                xor     bx,bx
                
; First - get i ndex into histogram (and compare min/max stuff). . .
; Do red * 1024  
                
                xor     ax,ax
                mov     cx,Word Ptr [bp + 18]    ; Bits to shift (right)
                shr     Byte Ptr [si],cl
                lodsb
                
                
red_test:       cmp     ax,curr_rmin
                jl      set_rmin
                cmp     ax,curr_rmax
                jg      set_rmax
                
red_ok:         mov     cx,10
                shl     ax,cl
                add     bx,ax
                
; Do green * 32  
                
                xor     ax,ax
                mov     cx,Word Ptr [bp + 18]    ; Bits to shift (right)
                shr     Byte Ptr [si],cl
                lodsb
                
grn_test:       cmp     ax,curr_gmin
                jl      set_gmin
                cmp     ax,curr_gmax
                jg      set_gmax
                
grn_ok:         mov     cx,5  
                shl     ax,cl
                add     bx,ax
                
                
; Add blue      
                xor     ax,ax
                mov     cx,Word Ptr [bp + 18]    ; Bits to shift (right)
                shr     Byte Ptr [si],cl
                lodsb
                
blu_test:       cmp     ax,curr_bmin
                jl      set_bmin
                cmp     ax,curr_bmax
                jg      set_bmax
                
blu_ok:         add     bx,ax
                
; BX now points  to correct histogram index (0..32k)
                
                shl     bx,1                            ; Make 0..64k 
                cmp     Word Ptr es:[di + bx],0FFFFh    ; Already = 64k ?
                jne     no_byte_inc
                
; Otherwise nee d to increment byte portion of histogram
                
                push    es
                push    di
                
                
                les     di,DWord Ptr [bp + 10]
                
                shr     bx,1                     ; Make 0..32k
                inc     Byte Ptr es:[di + bx]    
                shl     bx,1                     ; Make 0..64k again
                   
                pop     di
                pop     es
                
                mov     Word Ptr es:[di + bx],0   ; Zero word portion
                jmp     short word_zero           ; Do not increment word portion
                
no_byte_inc:    inc     Word Ptr es:[di + bx]     ; Increment word portion
word_zero:      pop     bx                        ; Restore counter
                add     bx,3
                cmp     bx,Word Ptr [bp + 20]
                jb      process_loop
                jmp     short do_minmax
                
; These are for  setting min / max values for r, g & b.  (Within short distance)
                
set_rmin:       mov     curr_rmin,ax
                jmp     red_ok
set_rmax:       mov     curr_rmax,ax
                jmp     red_ok
                
set_gmin:       mov     curr_gmin,ax
                jmp     grn_ok
set_gmax:       mov     curr_gmax,ax
                jmp     grn_ok
                
set_bmin:       mov     curr_bmin,ax
                jmp     blu_ok
set_bmax:       mov     curr_bmax,ax
                jmp     blu_ok
                
                
; Place min/max  values in addresses
                
do_minmax:      mov     ax,curr_ds
                mov     ds,ax
                
; Red           xor     ax,ax
                
                mov     ax,curr_rmin
                mov     bx,Word Ptr [bp + 22]    ; &rmin
                mov     Word Ptr [bx],ax         ; Save current min
                
                mov     ax,curr_rmax
                mov     bx,Word Ptr [bp + 24]    ; &rmin
                mov     Word Ptr [bx],ax         ; Save current max
                
; Green         
                mov     ax,curr_gmin
                mov     bx,Word Ptr [bp + 26]    ; &gmin
                mov     Word Ptr [bx],ax         ; Save current min
                
                mov     ax,curr_gmax
                mov     bx,Word Ptr [bp + 28]    ; &gmax
                mov     Word Ptr [bx],ax         ; Save current max
                
; Blue          
                mov     ax,curr_bmin
                mov     bx,Word Ptr [bp + 30]    ; &bmin
                mov     Word Ptr [bx],ax         ; Save current min
                
                mov     ax,curr_bmax
                mov     bx,Word Ptr [bp + 32]    ; &bmax
                mov     Word Ptr [bx],ax         ; Save current max
                
                
                pop     di
                pop     si
                pop     es
                pop     ds
                
                add     sp,24                     ; Restore 24 bytes local space
                pop     bp  
                ret

_process_scan   endp


;
;        count_unique (char far *, char far *);
;                      lpbHist      lpwHist
;


_count_unique  proc    far

               push    bp
               mov     bp,sp
               push    ds
               push    es
               push    si
               push    di

               lds     si,DWord Ptr [bp + 6]
               les     di,DWord Ptr [bp + 10]

               xor     ax,ax                   ; Clear counter
               mov     cx,32768                ; Do 32k times
cnt_loop:      cmp     Word Ptr es:[di],0      ; If not 0, is a color
               jne     add_count 
               cmp     Byte Ptr [si],0         ; If 0, not a color
               je      next_hist
add_count:     inc     ax


next_hist:     inc     di
               inc     di
               inc     si

               loop    cnt_loop 

; (AX contains return value)

               pop     di
               pop     si
               pop     es
               pop     ds
        pop     bp
               ret

_count_unique  endp





; find_closesta (red, green, blue, (char far *) Palette, (char far *) squares, 16);


 tmp_dist       equ     Word Ptr [bp - 2]
 curr_dist      equ     Word Ptr [bp - 4]
 red            equ     Byte Ptr [bp - 6]
 green          equ     Byte Ptr [bp - 8]
 blue           equ     Byte Ptr [bp - 10]
 smallest       equ     Word Ptr [bp - 12]
 tmp_pos        equ     Word Ptr [bp - 14]


_find_closesta proc    far

                push    bp
                mov     bp,sp
                sub     sp,18
                push    si
                push    di
                push    ds
                push    es

                mov     ax,Word Ptr [bp + 6]
                mov     red,al

                mov     ax,Word Ptr [bp + 8]
                mov     green,al

                mov     ax,Word Ptr [bp + 10]
                mov     blue,al

                lds     si,DWord Ptr [bp + 12]  ; Make ds:si = palette
                les     di,DWord Ptr [bp + 16]  ; Make es:di = square table 



CONTINUE:

                mov     cx,0
                cld
                mov     curr_dist,0FFFFH        ; Max val for curr dist

findloop:       push    cx
                mov     tmp_pos,cx
                mov     ax,3
                mul     cx

                add     ax,si                   ; SI points to palette
                mov     bx,ax                   ; Index into palette
                mov     cl,Byte Ptr [bx]
                inc     bx
                mov     ch,Byte Ptr [bx]
                inc     bx
                mov     dl,Byte Ptr [bx]
;  Now cl = red , ch = green and dl = blue from palette
;  Get differences

                sub     cl,red
                sub     ch,green
                sub     dl,blue


; Convert any negatives to positive
                cmp     cl,0
                jge     tst_green
                not     cl
                inc     cl
tst_green:      cmp     ch,0
                jge     tst_blue
                not     ch
                inc     ch
tst_blue:       cmp     dl,0
                jge     square_diffs
                not     dl
                inc     dl


square_diffs:   xor     bh,bh
                mov     bl,cl                           ; Red diff
                shl     bx,1                            ; Index to word
                mov     ax,Word Ptr es:[di + bx]        ; Make ax = red_err ** 2
                
                xor     bh,bh
                mov     bl,ch                           ; Green diff
                shl     bx,1                            ; Index to word
                mov     cx,Word Ptr es:[di + bx]        ; Make ax = green_err ** 2
                
                xor     bh,bh
                mov     bl,dl                           ; Blue diff
                shl     bx,1                            ; Index to word
                mov     dx,Word Ptr es:[di + bx]        ; Make ax = blue_err ** 2
                
; Now ax = square red diff
;     cx = square green diff
;     dx = square blue diff

add_diffs:      add     ax,cx
                add     ax,dx                           ; AX now = tmp_dist
                mov     cx,curr_dist
                cmp     ax,cx
                jae     dist_high
                
                mov     curr_dist,ax
                mov     ax,tmp_pos
                mov     smallest,ax
                
dist_high:      pop     cx
                inc     cx
                cmp     cx,Word Ptr [bp + 20]   ; Input positions to search
                jb      findloop

                mov     ax,smallest 

                pop     es
                pop     ds
                pop     di
                pop     si
                add     sp,18
                pop     bp
                ret

_find_closesta endp




;get_hista (lpbHist, lpwHist, HistIdx++);   
;           4        8          12

_get_hista     proc    near

               push    bp
               mov     bp,sp
               push    ds
               push    es
               push    si
               push    di

               lds     si,DWord Ptr [bp + 4]           ; Point to bhist
               les     di,DWord ptr [bp + 8]           ; Point to whist
               mov     bx,Word Ptr [bp + 12]           ; Offset

               xor     dh,dh
               mov     dl,Byte Ptr [si + bx]           ; Low order bits

               shl     bx,1
               mov     ax,Word Ptr es:[di + bx]        ; High order bits

               pop     di
               pop     si
               pop     es
               pop     ds
               pop     bp
               ret

_get_hista     endp

               end
