[org 0x7c00]

; mov bx, HELLO_MSG
; call print_string

mov dx, 0x12fe
call print_hex


; now we play with segments!
; cs <--- code segment
; ds <--- data segment
; ss <--- stack segment: used to modify bp! 
; es <---  extra segment
; the real address is segment * 16 + address

jmp $ ; Jump to the current address ( i.e. forever ).


; print functions
%include "print_function.asm"

; Data
HELLO_MSG:
	db 'Hello, World!', 0 ; zero on the end tells our routine when to stop printing chars




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