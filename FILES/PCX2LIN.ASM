;********************************************************
;            PCX2LIN.ASM
;********************************************************
;
;   Converts 4-plane data into linear data.  EG: PCX to DIB type.
;   This one was specially created for 4 plane .pcx.  The original
;   version was developed for VGA screen capture in imgprep.
;   That version is PLAN2LIN.ASM
;
;   Created:  David Ison
;
;
;            6-26-90:  Put into PCX "DLL" & added CMACROS   D. Ison



       	    DOSSEG
            INCLUDE   CPIASM.MAC
            INCLUDE   CMACROS.INC
            .MODEL    MEDIUM 


            ifndef SEGNAME
                SEGNAME equ <TEXT>
            endif

            createSeg _%SEGNAME, CodeSeg, word, public, CODE



                .code         
 
                public _ToLinear


;     ToLinear (lpDest, lpSource, wScanWidth, wRowsThisStrip, wBytesPerPlane, wBits) 
;                 6 8     10 12       14              16            18         20

BIT_MASK        equ   10000000b

scan_width      equ   Word Ptr [bp + 14]
rows_this_strip equ   Word Ptr [bp + 16]
bytes_per_row   equ   Word Ptr [bp + 18]
bits_to_shift   equ   Byte Ptr [bp + 20]

plane_size      equ   Word Ptr [bp - 4]
plane_count     equ   Word Ptr [bp - 6]
row_offset      equ   Word Ptr [bp - 8]


_ToLinear proc far

                push  bp
                mov   bp,sp

                sub   sp,10

                push  ds
                push  es
                push  si
                push  di

                les   di,DWord Ptr [bp +  6]    ; ES:DI = Dest Buf 
                lds   si,DWord Ptr [bp + 10]    ; DS:SI = Source Buf


                cld
                mov   row_offset,0

; First need to zero out output buffer...
  
                call  zero_outbuf


; Calculate size (in bytes) of a plane (planar mode)

                mov   ax, bytes_per_row
                mov   cx, rows_this_strip
                mul   cx                        ; AX = size of plane
                mov   plane_size,ax

                mov   cx,rows_this_strip
row_loop:       push  cx

                mov   plane_count,0             ; Initialize plane counter
                mov   cx,4  ; 4 PLANES!
plane_loop:     push  cx
                lds   si,DWord Ptr [bp + 10]    ; Point back to input planar dat
                mov   ax,plane_count            ; And adjust pointer to point
                mov   cx,plane_size             ; to 0, 1, 2 or 3rd plane
                mul   cx
                add   si,ax                      
                add   si, row_offset

                mov   bx,0                      ; Counter for pixels across (starts at 0)

                mov   cx,bytes_per_row
pixel_loop:     push  cx

                lodsb
                mov   ah,al                     ; Save in AH
                mov   cx,plane_count            ; Which plane we are now processing
                inc   cx                        ; Do this for ROL instruction instead of rol 1 and shl cl

bit0:           mov   al,ah                     ; AH had planar bits
                mov   dl,Byte Ptr es:[di + bx]  ; Get the output byte
                and   al,BIT_MASK               ; Get leftmost bit of AL
                rol   al,cl                     ; Put into first pos and shift for bit plane
                or    dl,al
                mov   al,dl
                mov   Byte Ptr es:[di + bx],al
                inc   bx
                cmp   bx,scan_width             ; Done with # pixels this row ?
                jb    cont0                     ; Fudge fix for out of range error
                jmp   far ptr end_of_row        ; Yes
cont0:          shl   ah,1                      ; If not, use next bit

bit1:           mov   al,ah                     ; AH had planar bits
                mov   dl,Byte Ptr es:[di + bx]  ; Get the output byte
                and   al,BIT_MASK               ; Get leftmost bit of AL
                rol   al,cl                     ; Put into first pos and shift for bit plane
                or    dl,al
                mov   al,dl
                mov   Byte Ptr es:[di + bx],al
                inc   bx
                cmp   bx,scan_width
                jb    cont1                     ; Fudge fix for out of range error
                jmp   far ptr end_of_row        ; Yes
cont1:          shl   ah,1                      ; If not, use next bit


bit2:           mov   al,ah                     ; AH had planar bits
                mov   dl,Byte Ptr es:[di + bx]  ; Get the output byte
                and   al,BIT_MASK               ; Get leftmost bit of AL
                rol   al,cl                     ; Put into first pos and shift for bit plane
                or    dl,al
                mov   al,dl
                mov   Byte Ptr es:[di + bx],al
                inc   bx
                cmp   bx,scan_width
                jb    cont2                     ; Fudge fix for out of range error
                jmp   far ptr end_of_row        ; Yes
cont2:          shl   ah,1                      ; If not, use next bit

bit3:           mov   al,ah                     ; AH had planar bits
                mov   dl,Byte Ptr es:[di + bx]  ; Get the output byte
                and   al,BIT_MASK               ; Get leftmost bit of AL
                rol   al,cl                     ; Put into first pos and shift for bit plane
                or    dl,al
                mov   al,dl
                mov   Byte Ptr es:[di + bx],al
                inc   bx
                cmp   bx,scan_width
                jae   end_of_row
                shl   ah,1                      ; If not, use next bit

bit4:           mov   al,ah                     ; AH had planar bits
                mov   dl,Byte Ptr es:[di + bx]  ; Get the output byte
                and   al,BIT_MASK               ; Get leftmost bit of AL
                rol   al,cl                     ; Put into first pos and shift for bit plane
                or    dl,al
                mov   al,dl
                mov   Byte Ptr es:[di + bx],al
                inc   bx
                cmp   bx,scan_width
                jae   end_of_row
                shl   ah,1                      ; If not, use next bit

bit5:           mov   al,ah                     ; AH had planar bits
                mov   dl,Byte Ptr es:[di + bx]  ; Get the output byte
                and   al,BIT_MASK               ; Get leftmost bit of AL
                rol   al,cl                     ; Put into first pos and shift for bit plane
                or    dl,al
                mov   al,dl
                mov   Byte Ptr es:[di + bx],al
                inc   bx
                cmp   bx,scan_width
                jae   end_of_row
                shl   ah,1                      ; If not, use next bit

bit6:           mov   al,ah                     ; AH had planar bits
                mov   dl,Byte Ptr es:[di + bx]  ; Get the output byte
                and   al,BIT_MASK               ; Get leftmost bit of AL
                rol   al,cl                     ; Put into first pos and shift for bit plane
                or    dl,al
                mov   al,dl
                mov   Byte Ptr es:[di + bx],al
                inc   bx
                cmp   bx,scan_width
                jae   end_of_row
                shl   ah,1                      ; If not, use next bit


bit7:           mov   al,ah                     ; AH had planar bits
                mov   dl,Byte Ptr es:[di + bx]  ; Get the output byte
                and   al,BIT_MASK               ; Get leftmost bit of AL
                rol   al,cl                     ; Put into first pos and shift for bit plane
                or    dl,al
                mov   al,dl
                mov   Byte Ptr es:[di + bx],al
                inc   bx

                pop   cx
                dec   cx
                cmp   cx,0
                je    bytes_done
                jmp   far ptr pixel_loop

end_of_row:     add   sp,2                      ; Skipped a "pop", need to fixup stack


bytes_done:     inc   plane_count
                pop   cx
                dec   cx
                cmp   cx,0
                je    plane_done
                jmp   far ptr plane_loop


plane_done:     pop   cx
                dec   cx
                cmp   cx,0
                je    all_done

                mov   ax,bytes_per_row
                add   row_offset,ax

                add   di,scan_width 
                jmp   far ptr row_loop

all_done:

                pop   di
                pop   si
                pop   es
                pop   ds

                add   sp,10

                pop   bp
                ret

_ToLinear       endp



zero_outbuf     proc  near


                push  es
                push  di

                mov   ax,scan_width
                mov   cx,rows_this_strip
                mul   cx
                mov   cx,ax

                mov   ax,0
                rep   stosb

                pop   di
                pop   es

                ret

zero_outbuf     endp

                end 

