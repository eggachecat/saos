[org 0x7c00] ; no more pain for offsetting


mov ah, 0x0e


; we want to play with stack!
; two register: bp and sp for pointers at stack Base and stack Top(why not tp...)
; bp stays still and sp is moving (the same as we know in data structure class)


mov bp, 0x8000 ; as we says we set the base pointer at some free address
mov sp, bp ; so now the top of stack is actually the base of the stack

push 'A' ; push some data on the stack for later use
push 'B' ; note, there are pushed on as 16-bit values
push 'C' ; so the most significant byte will be added by our assembler as 0x00
         ; and 'A' is something like 0x0(8 bits) + 'A'(8 bits)

pop bx ;  Note , we can only pop 16-bits , so pop data to bx (cx dx are all fine!) bx has nothing to do with bp
mov al, bl ; then copy bl ( i.e. 8-bit char ) to al
int 0x10 ; print(al)

the_secret:
	db "X"

jmp $ ; Jump to the current address ( i.e. forever ).


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