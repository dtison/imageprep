;********************************************************
;            EPSCOLOR.ASM
;********************************************************
;
;   Formats data .EPS style.
;
;   Created:  David Ison
;
;            6-26-90:  Put into EPS "DLL" & added CMACROS   D. Ison



       	    DOSSEG
            INCLUDE   CPIASM.MAC
            INCLUDE   CMACROS.INC
            .MODEL    MEDIUM 


            ifndef SEGNAME
                SEGNAME equ <TEXT>
            endif

            createSeg _%SEGNAME, CodeSeg, word, public, CODE


        public _fmt_rgb_eps

;
;      fmt_rgb_eps (scan_buf_ptr, enc_buf_ptr, scanbytes, rows_this_strip);  /*  Format one rgb input strip to color EPS  */
;
; scan_buf_ptr:     (Far) Pointer to input buffer
; enc_buf_ptr:      (Far) Pointer to output buffer
; scanbytes:        Pixel resolution of scanline
; rows_this_strip:  No. scanlines this strip

inp_buf_ptr    equ     Word Ptr [bp - 4]
index          equ     Word Ptr [bp - 6]
byte_count     equ     Word Ptr [bp - 8]


.CODE
_fmt_rgb_eps    proc    far

	              push    bp
                mov     bp,sp
                sub     sp,14                   ; 14 bytes local space
                push  	si
                push	  di
                push  	ds
                push	  es
 
                cld

		            lds     si,DWord Ptr [bp + 6]   ; Pointer to input buffer 
		            les     di,DWord Ptr [bp + 10]  ; Pointer to output buffer 
                mov     inp_buf_ptr, si         ; Save pointer to input buffer

; Calculate bytes per row (per strip)

                mov     ax,Word Ptr [bp + 14]   ; Pixels / row
                mov     cx,3
                mul     cx
                mov     byte_count,ax

                mov     cx,Word Ptr [bp + 16]   ; Input rows to format 
fmt_eps_rgb:    push    cx
                mov     cx,3                    ; R line, G line and B line
                mov     index,0
fmt_eps_ln:     push    cx
                mov     si,inp_buf_ptr          ; Reset input pointer
                add     si,index                ; Index to R, G or B
                mov     cx,Word Ptr [bp + 14]   ; Pixels / row
fmt_eps_dot:    push    cx
                lodsb                           ; Get (next) input byte
                add     si,2                    ; Skip over other 2 components
                mov     bl,al                   ; Save byte for later use

; Transform byte to ascii

                mov     cl,4                    ; # bits to shift
                shr     al,cl                   ; Do high order bits first
                cmp     al,0Ah                          
                jb      do_eps10s               ; If < 10 do as 10's
                add     al,55                   ; Translate to A..F ascii
                stosb
                jmp     short eps_lobits
do_eps10s:      add     al,48   
                stosb 
                  
eps_lobits:     mov     al,bl                   ; BL had the data byte after shift
                shl     al,cl                   ; Shift left to remove hi order bits
                shr     al,cl                   ; Shift back to low order
                cmp     al,0Ah                          
                jb      do_eps10s2              ; If < 10 do as 10's
                add     al,55                   ; Translate to A..F ascii
                stosb
                jmp     short eps_bt_done 
do_eps10s2:     add     al,48   
                stosb 


eps_bt_done:    pop     cx
                loop    fmt_eps_dot  

;  Add cr/lf to end of this line  (R, G or B)  
                mov     al,0Dh                  ; CR
                stosb
                mov     al,0Ah                  ; LF
                stosb
                
                inc     index
                pop     cx 
                loop    fmt_eps_ln 


;  Add to our counter the no. of bytes moved

                mov     ax,byte_count
                add     inp_buf_ptr,ax
                pop     cx
                loop    fmt_eps_rgb 

	              pop     es
	              pop     ds
	              pop     di
	              pop     si
                add     sp,14                    ; Restore 14 local bytes
                pop     bp
                ret

_fmt_rgb_eps    endp

       	end






