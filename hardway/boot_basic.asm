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


; basic use
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

; learn how the memory works
; first of all, BIOS likes always to load the boot sector to the address >>0x7c00 (magic number too?)

; just load the_secret to al
; what what is loaded into al actually? it load the immediate offset into al ! that is the address
; but we want the content at the offset!
; and [] <- do the help
mov al, the_secret
int 0x10

; but why this is wrong anyway?
; because the CPU treat the the_secret AS the address from the START of the memory(that is 0x0)
; while this the_secret grammar will give the offset from the bootloader!
mov al, [the_secret]
int 0x10

; now this is working!
; bascially it adds the offset address and the start where the offset begins 
mov bx, the_secret
add bx, 0x7c00
mov al, [bx]
int 0x10

; this should work
; but i dont calculate the address...
mov al, [0x7c31]
int 0x10



jmp $ ; Jump to the current address ( i.e. forever ).

the_secret:; which tells the assembler to write the subsequent bytes directly to the binary output file 
           ; not processor instructions !!! just what assembler write at some address (in .bin)
	db "X"

;
; Padding and magic BIOS number.
; Basiclly the remains code is for cpu to verify this is as bootloader

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