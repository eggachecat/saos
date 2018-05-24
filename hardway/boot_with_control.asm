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

; now we play with some control structures
; that is loop. if-else

; Conditional jumps are achieved in assembly language by 
; first running a comparisoninstruction, 
; then by issuing a specific conditional jump instruction.

; comparation: 
;	cmp v1, v2
; jump:
;	je      x == y
;	jne     x != y
;	jl      x < y
;	jle     x <= y
;	jg      x > y
;	jge     x >= y
; example:
;	should output CG since 30 > 4s
mov bx, 30
cmp bx, 4
jl smaller_than_10
jg greater_than_10


smaller_than_10:
	mov al, 'S'
	jmp continue
greater_than_10:
	mov al, 'G'
	jmp continue

continue:
	int 0x10


; calling function
; use the keyword: call
; The CPU keeps track of the current instruction being executed in the special register ip (instruction pointer), 
; which, sadly, we cannot access directly.
; we can use call and ret





; this is how we derfine a funtion
; the ret will let the assembler konw jmp where the caller is
print_function_global:
	; always the register? always global?!
	; that is before we call this function we must have set sth. on al(what we want to print)
	; but things always go messy when every thing is global
	; ex. if we want use bx in the function , the global bx will overwritten
	mov ah, 0x0e
	int 0x10
	ret

print_function_local:
	; so what we can do is actaully storing all the global registers on a stack
	; then we can use it locally
	; after the function being done, we can resume all registers from the stack to resume the state
	pusha ; push all register values to the stack
	
	mov bx, 10 ; these two instructions have no meaning
	add bx, 20 ; just to demostrate that we can manipulate registers locall without polluting the previous

	mov ah, 0x0e
	int 0x10
	popa ; resume the registers
	ret


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