        .xlist
        include cmacros.inc
        .list


?PLM=0                            ; 'C'naming

ifndef SEGNAME
  SEGNAME equ <TEXT>
endif

createSeg _%SEGNAME, CodeSeg, word, public, CODE

sBegin  Code


assumes cs,Code
assumes ds,nothing





;
;       void FAR RGBFilter (LPSTR, LPSTR, LPSTR, WORD, WORD);






RGBFilt STRUC 

    blue        db      ?
    green       db      ?
    red         db      ?
    unused      db      ?

RGBFilt ENDS


cProc           RGBFilter, <FAR,PUBLIC>, <es, ds, si, di>

                parmD   lpDest
                parmD   lpSource
                parmD   lpTransTable
                parmW   wScanWidth
                parmW   wRowsThisStrip
                parmW   wBytesPerRow

                localW  TmpSourcePtr
                localD  TmpDestPtr
                localW  wOutIndex

cBegin          

                les     di,DWord Ptr lpDest
                lds     si,DWord Ptr lpSource

                mov     Word Ptr TmpDestPtr,di
                mov     Word Ptr TmpDestPtr + 2,es


                mov     cx,wRowsThisStrip
RowLoop:        push    cx


                mov     Word Ptr TmpSourcePtr,si
                mov     Word Ptr TmpDestPtr,di

                mov     wOutIndex,0

                mov     cx,wScanWidth
PixLoop:        push    cx


                les     di,DWord Ptr lpTransTable

; Process three values

                mov     cl,2            ; Bits to shift

                lodsw                   ; Get blue and green
                push    ax
                lodsb                   ; Get red

                xor     bh,bh
                mov     bl,al           ; Translate red 
                shl     bx,cl

                mov     dl,Byte Ptr es:[di + bx].red 

                pop     ax

                xor     bh,bh
                mov     bl,ah           ; Translate green
                shl     bx,cl


             mov ah,0

                mov     ah,Byte Ptr es:[di + bx].green


                xor     bh,bh
                mov     bl,al           ; Translate blue
                shl     bx,cl

             mov al,0

                mov     al,Byte Ptr es:[di + bx].blue


                les     di,DWord Ptr TmpDestPtr
                add     di,wOutIndex

                stosw

                mov     al,dl

                stosb

                add     wOutIndex,3

                pop     cx

                loop    PixLoop

                mov     si,Word Ptr TmpSourcePtr
                add     si,wBytesPerRow

                mov     di,Word Ptr TmpDestPtr
                add     di,wBytesPerRow

                pop     cx

                loop    RowLoop



cEnd            



sEnd  




end
