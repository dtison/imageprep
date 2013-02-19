; Special modifications for ImagePrep
;
;   11-21-89:  To  fmt_to24 to work with a palette 4 bytes/entry instead of 3 bytes/entry
;              And to pass parameters as Dest, Source  D. Ison
;
;   12-29-89:  Changed over to new Filter (LPLPSTR, LPLPSTR, LPSTR) format. D. Ison
;
;   01-09-90:  Changed over to FAR PASCAL parameter convention  D. Ison
;              (What a hassle!)


        include cmacros.inc

        ifndef SEGNAME
            SEGNAME equ <TEXT>
        endif
        createSeg _%SEGNAME, CodeSeg, word, public, CODE

        INCLUDE CPIASM.MAC
        .MODEL  MEDIUM

        .DATA

        db      49h,6Dh,61h,67h,65h,50h,72h,65h,70h,2Fh,43h,6Fh,6Ch,6Fh,72h,6Ch
        db      61h,62h,20h,34h,2Eh,30h,20h,61h,6Eh,64h,20h,54h,4Fh,4Fh,4Ch,53h
        db      2Eh,44h,4Ch,4Ch,20h,63h,72h,65h,61h,74h,65h,64h,20h,53h,70h,72h
        db      69h,6Eh,67h,20h,31h,39h,39h,31h,2Ch,20h,44h,61h,76h,69h,64h,20h
        db      54h,2Eh,20h,49h,73h,6Fh,6Eh,20h,20h
        db      4Fh,49h,43h,2Ch,20h,53h,70h,72h,69h,6Eh,67h,20h,31h,39h,39h,30h

        .CODE   
 
        public  Format_8To24
;
;      Format_8To24 ((LPLPSTR) &lpDest, (LPLPSTR) &lpSource, lpStruct);
;      (PASCAL)                  14                  10         6

; Internal duplicate values from structure

wPalSegment   equ     Word Ptr [bp - 4]
wPalOffset    equ     Word Ptr [bp - 6]
wScanWidth    equ     Word Ptr [bp - 8]

; Other Local variables

red           equ     Byte Ptr [bp - 10]
grn           equ     Byte Ptr [bp - 12]
blu           equ     Byte Ptr [bp - 14]


fmt8to24 STRUC 

    paloffset       dw ?    
    palsegment      dw ?    
    scan_width      dw ?

fmt8to24 ENDS



Format_8To24  proc    far


              @WinPrologue 20                         ; 20 local bytes



; First copy structure info to local variables

              lds     si,DWord Ptr [bp + 6]           ; DS:SI = Structure ptr

              mov     ax,Word Ptr [si].paloffset
              mov     wPalOffset,ax

              mov     ax,Word Ptr [si].palsegment
              mov     wPalSegment,ax

              mov     ax,Word Ptr [si].scan_width
              mov     wScanWidth,ax
 
; Then setup IO Buffers

              @GetIOBufs                              

              mov     cx,Word Ptr  wScanWidth
              cld

toloop:       push    cx
              lodsb                           ; Get (next) input byte

; Now need to map to R, G or B values from palette

              push    es                      ; Save current es:di
              push    di                      
 
              mov     es,wPalSegment
              mov     di,wPalOffset

              xor     ah,ah
              mov     cl,2
              shl     ax,cl                   ; Index to 4 bytes / entry
              mov     bx,ax
 
              add     bx,di                   ; Add offset

              mov     al,Byte Ptr es:[bx]
              mov     red,al                  ; Save the red byte after shift 
 
              inc     bx
              mov     al,Byte Ptr es:[bx]
              mov     grn,al                  ; Save the green byte after shift9

              inc     bx
              mov     al,Byte Ptr es:[bx]
              mov     blu,al

              pop     di
              pop     es                      ; Restore es= output buffer

              mov     al,red
              stosb                           ; Store blue first
              mov     al,grn 
              stosb                           ; Store green
              mov     al,blu 
              stosb                           ; Store red
                    
to24_done:    pop     cx                      ; CX had original counter

              loop    toloop

              @WinEpilogue  20, 6             ; 20 local bytes; 6 parm bytes
 
Format_8To24  endp


              end

