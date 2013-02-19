;----------------------------------------------------------------------------;
;
;
;  (They are not yet setup for exporting however)  
;
;
;   Hello CPI
;----------------------------------------------------------------------------;

        .xlist
        include cmacros.inc
        .list





ifndef SEGNAME
  SEGNAME equ <TEXT>
endif

; (.286p)
  createSeg _%SEGNAME, CodeSeg, word, public, CODE



sBegin  Code

assumes cs,Code
assumes ds,nothing



;----------------------------------------------------------------------------;
;
;   QuantizeOPR   (lpData,             
;                  lpSmallhist, 
;                  wPaddedScanWidth,
;                  wRowsThisStrip,
;                  wInputBytesPerRow,
;                  3);
;
;----------------------------------------------------------------------------;



cProc   QuantizeOPR, <FAR,PUBLIC,PASCAL>, <ds, es, si, di>

parmD   lpData 
parmD   lpSmallhist
parmW   wPaddedScanWidth
parmW   wRowsThisStrip 
parmW   wInputBytesPerRow
parmW   wNumBits 
                
localW  OutPtr
localW  TmpPtr   
                

cBegin  


                lds     si,DWord Ptr lpData 
                les     di,DWord Ptr lpSmallhist

                mov     OutPtr,si           ; Store output pointer

                cld

                mov     cx,wRowsThisStrip
row_loop:       push    cx

                mov     TmpPtr,si

                mov     cx,wPaddedScanWidth
pixel_loop:     push    cx


                xor     bx,bx

; Do red * 1024   (red << 10)

                mov     cx, Word Ptr wNumBits  
                xor     ax,ax
                lodsb
                shr     al,cl
                mov     cx,10
                shl     ax,cl   
                add     bx,ax



; Do green * 32  (green << 10)

                mov     cx, wNumBits
                xor     ax,ax
                lodsb
                shr     al,cl
                mov     cx,5
                shl     ax,cl   
                add     bx,ax




; Add blue
                mov     cx, Word Ptr wNumBits 
                xor     ax,ax
                lodsb
                shr     al,cl
                add     bx,ax

; BX now points to correct histogram index

                xor     ax,ax
                mov     al,Byte Ptr es:[di + bx]

                mov     bx,OutPtr
                mov     Byte Ptr [bx],al
                inc     OutPtr

                pop     cx
                loop    pixel_loop

                mov     si,TmpPtr
                add     si,wInputBytesPerRow

                pop     cx
                loop    row_loop



cEnd



sEnd  




end



