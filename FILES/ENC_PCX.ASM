;********************************************************
;            ENC_PCX.ASM
;********************************************************
;
;   Encodes data .PCX style.
;
;   Created:  David Ison
;
;   Call as   enc_pcx (scan_buf,scan_bytes,outfh,buffer)
;
;            scan_buf - buffer containing one scan line data
;            scan_bytes - # bytes per scan line (length of scan line)
;            outfh - Handle of output file
;            buffer - Pointer to      buffer for data (must be 4k)

;            1/30/90 - David Ison (Fixed offset problem <buf_offset> that
;                      caused last strip problem in saves)
;
;
;            6-26-90:  Put into PCX "DLL" & added CMACROS   D. Ison



       	    DOSSEG
            INCLUDE  CPIASM.MAC
            INCLUDE CMACROS.INC
            .MODEL MEDIUM 


            ifndef SEGNAME
                SEGNAME equ <TEXT>
            endif

            createSeg _%SEGNAME, CodeSeg, word, public, CODE




.data

pcx_buf_cnt     dw      0                       ; An associated counter
.code       

PUBLIC          _enc_pcx
PUBLIC          _pcxbuf_flush

wTotal          equ     Word  Ptr  [bp - 4]
CurrDS          equ     Word  Ptr  [bp - 6]

lpData          equ     DWord Ptr  [bp + 6]
wScanBytes      equ     Word  Ptr  [bp + 10]
hFile           equ     Word  Ptr  [bp + 12]
lpBuffer        equ     DWord Ptr  [bp + 14]
buf_offset      equ     Word  Ptr  [bp + 14]

_enc_pcx        proc    far 

                push    bp
                mov     bp,sp

                sub     sp,10                   ; Reserve 10 bytes local space

                push    ds                      ; Save current ds
                push    es                      ; Save current es
                push    si
                push    di

                mov     ax,ds
                mov     CurrDS,ax               ; Store our current ds

                lds     si,lpData
                les     di,lpBuffer             ; Make ES:DI = outbuf buffer

                mov     cx,wScanBytes           ; Bytes to process
                dec     cx


                mov     dx,1                    ; Init run count

                lodsb                           ; Get first byte
                mov     bl,al

enc_loop1:      lodsb
                cmp     al,bl
                jne     not_equal   
  
; Otherwise inc the count 

                inc     dx                      ; Run count
                cmp     dx,3Fh                  ; Or 63 decimal
                jne     test_byte_cnt
                  
; Save data and count

                call    far ptr enc_put
                xor     dx,dx                   ; Zero Run count

                jmp     short test_byte_cnt

not_equal:      cmp     dx,0
                je      no_put
                call    far ptr enc_put

no_put:         mov     bl,al
                mov     dx,1 

test_byte_cnt:  loop    enc_loop1


; Finish up

                cmp     dx,0
                je      no_more
                call    far ptr enc_put

no_more:        mov     ax,wTotal


                pop     di
                pop     si
                pop     es                      ; Restore es
                pop     ds                      ; Restore ds

                add     sp,10                    ; De-allocate local space

                pop     bp
                ret

_enc_pcx        endp



;************************************************************* 
; ENC_PUT       Buffers file writing
; On entry      bp - Points to stack frame of enc_pcx
;               dx - Run count
;               bl - Data to encode
;               pcx_buf_cnt contains current byte count

enc_put         proc    far 

                push    ax
                push    bx
                push    di

                mov     al,bl                   ; bl = last

                mov     di,buf_offset           ; Point back to beginning
                push    ds
                push    ax
                mov     ax,CurrDS
                mov     ds,ax
                mov     bx,pcx_buf_cnt          ; bx is bytes written so far
                pop     ax
                pop     ds

                cmp     dx,1                    ; Test count value
                jl      put_exit                ; We missed it somewhere
                jg      put_some

; Otherwise, see if data looks like a count to put one

                mov     ah,al
                and     ah,0C0h
                cmp     ah,0C0h
                je      put_some                ; Data looks like a count

; Data does not look like count, so put one

                mov     Byte Ptr es:[di + bx],al   ; Store the data
                inc     bx
                jmp     short put_done

put_some:       or      dl,0C0h                    ; Mask counter
                mov     Byte Ptr es:[di + bx],dl   ; Store the count
                inc     bx
                mov     Byte Ptr es:[di + bx],al   ; Store the data
                inc     bx

put_done:       cmp     bx,4000                    ; 4000 bytes written so far?
                jle     put_exit 

; Need to flush buffer

                push    ds
                push    ax
                mov     ax,CurrDS
                mov     ds,ax                   ; Make ds=default seg
                mov     pcx_buf_cnt,bx          ; Store current count
                xor     bx,bx                   ; Reset # bytes written
                push    es                      ; Pass segment to buffer
                push    di                      ; Pass pointer to buffer
                mov     ax,hFile
                push    ax                      ; Pass file handle 
                call    far ptr _pcxbuf_flush
                add     sp,6
                pop     ax
                pop     ds

put_exit:       push    ds
                push    ax
                mov     ax,CurrDS
                mov     ds,ax
                mov     pcx_buf_cnt,bx          ; Remember current byte count
                pop     ax
                pop     ds

                pop     di
                pop     bx
                pop     ax
                ret
enc_put         endp



; Call as:   pcxbuf_flush (handle,buffer);

_pcxbuf_flush   proc    far 

                push    bp
                mov     bp,sp

                push    bx
                push    cx
                push    dx
                push    ds


                mov     cx,pcx_buf_cnt          ; CX is wTotal bytes buffered
                mov     ax,[bp+10]              ; Passed seg to buffer
                mov     ds,ax
                mov     dx,[bp+8]               ; Passed pointer to buffer
                mov     bx,[bp+6]               ; BX is file handle
                mov     ah,40h                  ; Write file
                int     21h


                pop     ds                      ; Restore default ds
                mov     pcx_buf_cnt,0           ; Re-init the buff counter 

                pop     dx
                pop     cx
                pop     bx
                pop     bp

                ret

_pcxbuf_flush   endp


                end
