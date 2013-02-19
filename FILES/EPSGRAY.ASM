;********************************************************
;            EPSGRAY.ASM
;********************************************************
;
;   Formats data .EPS style.
;
;   Created:  David Ison
;
;            6-26-90:  Put into EPS "DLL" & added CMACROS   D. Ison
;            9-5-90:   Modified version for Imageprep does not use "trans table"


            DOSSEG
            INCLUDE   CPIASM.MAC
            INCLUDE   CMACROS.INC
            .MODEL    MEDIUM 


            ifndef SEGNAME
                SEGNAME equ <TEXT>
            endif

            createSeg _%SEGNAME, CodeSeg, word, public, CODE


        public _fmt_eps_gray



.CODE

;
;      fmt_eps_gray (lpDest, lpSource, wScanWidth);
;

_fmt_eps_gray   proc    far

                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                push    es


                les     di,DWord Ptr [bp +  6]  ; Make es:di = output buffer
                lds     si,DWord Ptr [bp + 10]  ; Make ds:si = input buffer
                mov     bx,Word Ptr [bp + 18]   ; Input bytes to format 
                mov     cx,4                    ; Bytes to shift  (bits ?)
                cld

grayloop:       lodsb                           ; Get (next) byte

                
; Translate from translation table
                
;               push    ds                      ; Preserve input buffer ds
;               push    si                      ; Preserve input buffer si
;               push    bx                      ; Preserve count
;               
;               lds     si,DWord Ptr [bp + 14]  ; Point to translation table
;               
;               xor     bx,bx                   ; Make bh=bl=0
;               mov     bl,al                   ; Grayscale offset starts at 1
;               mov     al,Byte Ptr [si + bx]   ; Get translated byte
;               
;               pop     bx                      ; Restore preserved registers
;               pop     si
;               pop     ds
;               


                mov     ah,al                   ; Save the byte 
                shr     al,cl                   ; Do high order bits first
                cmp     al,0Ah                          
                jb      do_10s                  ; If < 10 do as 10's
                add     al,55                   ; Translate to A..F ascii
                stosb
                jmp     short do_lobits
do_10s:         add     al,48   
                stosb 
                   
do_lobits:      mov     al,ah
                shl     al,cl                   ; Shift left to remove hi order bits
                shr     al,cl                   ; Shift back to low order
                cmp     al,0Ah                          
                jb      do_10s2                 ; If < 10 do as 10's
                add     al,55                   ; Translate to A..F ascii
                stosb
                jmp     short chk_gr_cnt   
do_10s2:        add     al,48   
                stosb 
                
chk_gr_cnt:     dec     bx
                cmp     bx,0
                je      done
                jmp     grayloop
                
done:           
;  Add cr/lf to  end of this line
                mov     al,0Dh                  ; CR
                stosb
                mov     al,0Ah                  ; LF
                stosb
                
                pop     es
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret

_fmt_eps_gray   endp
       




        end






