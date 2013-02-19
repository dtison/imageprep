;  1-30-90  Targa 24 to 16 converter 

;  (D. Ison)


       	        DOSSEG
                .MODEL MEDIUM 


                .CODE  UTILS_TEXT 

                public  _TGA24To16


red     equ     Byte Ptr [bp - 4]
green   equ     Byte Ptr [bp - 6]
blue    equ     Byte Ptr [bp - 8]



_TGA24To16      proc    far
 
                push    bp
                mov     bp,sp
                sub     sp,10

                push	  si
                push	  di
                push	  ds
                push	  es
 

                lds     si,DWord Ptr [bp + 6]   ; Make ds:si = source buffer
                les     di,DWord Ptr [bp + 10]  ; Make es:di = dest   buffer
                mov     cx,Word Ptr  [bp + 14]  ; Input bytes to format 
                cld

tgato16loop:    push    cx
                lodsb                           ; Get blue byte 
                mov     blue,al
                lodsb                           ; Get green  
                mov     green,al
                lodsb
                mov     red,al

                xor     bx,bx
                mov     al,blue                 ; Start with blue
                mov     cx,3
                shr     al,cl                   ; Shift to 5 bits

                mov     bl,al                   ; Store blue

                xor     ah,ah
                mov     al,green                ; Next do green
                shr     al,cl                   ; Shift to 5 bits

                mov     cl,5
                shl     ax,cl
                or      bx,ax                   ; Store green

                xor     ah,ah
                mov     al,red                  ; Then red
                mov     cx,3
                shr     al,cl                   ; Shift to 5 bits

                mov     cl,10    
                shl     ax,cl
                or      bx,ax                   ; Store red 
                
                mov     ax,bx
                stosw

                pop     cx                      ; CX had original pixel count
                loop    tgato16loop

                pop     es
                pop     ds
                pop     di
                pop     si
                add     sp,10
                pop     bp   
                ret
                
_TGA24To16      endp


                end
