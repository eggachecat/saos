;
; A simple boot sector that prints a message to the screen using a BIOS routine.
;

; something about assemble
; registers: ax cs dx bv <- they can be accessed by cpu and us
; EAX is the full 32-bit value

; AX(BX CX DX) is the lower 16-bits registers
; AL is the lower 8 bits of AX
; AH is the bits 8 through 15 (zero-based) of AX

;DB - Define Byte. 8 bits
;DW - Define Word. Generally 2 bytes on a typical x86 32-bit system
;DD - Define double word. Generally 4 bytes on a typical x86 32-bit system



mov ah , 0x0e ; int(interrupt) which tells we want to use tty mode
mov al , 'H'
int 0x10 ; bascially if we interrupt with 0x10(this is general for video service) the cpu will verify the ah 
		 ; and if it is 0x0e, the cpu will print the ascii in al; this routine is hard-coded by cpu

mov al , 'e'
int 0x10
mov al , 'l'
int 0x10
mov al , 'l'
int 0x10
mov al , 'o'
int 0x10
jmp $ ; Jump to the current address ( i.e. forever ).



;
; Padding and magic BIOS number.
;

; When compiled , our program must fit into 512 bytes , 
; with the last two bytes being the magic number , 
; so here , tell our assembly compiler to pad out our 
; program with enough zero bytes (db 0) to bring us to 
; 510 th byte.

; total 512 reserve 2 bytes ===> 510
; $-$$ for statment size


times 510-($-$$) db 0

; Magic number
dw 0xaa55 