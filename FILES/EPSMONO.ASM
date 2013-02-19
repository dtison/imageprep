title epsmono.asm
?PLM=1
?WIN=1
include cmacros.inc

ifndef SEGNAME
  SEGNAME equ <TEXT>
endif

createSeg _%SEGNAME, CodeSeg, word, public, CODE

sBegin Data
sEnd   Data

sBegin CodeSeg
assumes cs, CodeSeg
assumes ds, Data

cProc FormatEPSMono, <PUBLIC, FAR, PASCAL>, <es,ds,di,si,bx>
        parmD   lpDest
        parmD   lpSource
        parmW   wScanWidth

cBegin
        les     di, lpDest
        lds     si, lpSource
        mov     bx, wScanWidth
        cld

monoloop:
        lodsb

        xor     ah, ah
        cmp     al, 0
        je      black
        mov     ah, 'F'               ; 046h
        mov     al, 'F'               ; 046h
        jmp     short chardone

black:  
        mov     ah, '0'               ; 030h
        mov     al, '0'               ; 030h

chardone:
        stosw
        dec     bx
        cmp     bx, 0
        je      done
        jmp     short monoloop

done:
        mov     al, 0Dh                     ; CR
        stosb
        mov     al, 0Ah                     ; LF
        stosb
        mov     ax, 1

cEnd 

sEnd    CodeSeg
end
