print_hex:
; receiving the data in 'dx'
; For the examples we'll assume that we're called with dx=0x1234
; Strategy: get the last char of 'dx', then convert to ASCII and store the ascii somewhere (location HEX_OUT)
; Numeric ASCII values: '0' (ASCII 0x30) to '9' (0x39), so just add 0x30 to byte N.
; For alphabetic characters A-F: 'A' (ASCII 0x41) to 'F' (0x46) we'll add 0x40
; Then, move the ASCII byte to the correct position on the resulting string !!

    pusha

    mov cx, 0 ; our index variable
	
hex_loop:
    cmp cx, 4 ; loop 4 times
    je end_block
    
    ; 1. convert [last] char of 'dx' to ascii
    mov ax, dx ; we will use 'ax' as our working register
    and ax, 0x000f ; 0x1234 -> 0x0004 by masking first three to zeros <- now we map 4 to a number!
    add al, 0x30 ; add 0x30 to N to convert it to ASCII "N"
    cmp al, 0x39 ; if > 9, add extra 8 to represent 'A' to 'F'
    jle mov_char ; go first 
    add al, 7 ; 'A' is ASCII 65 instead of 58, so 65-58=7 then mov_char

mov_char:
    ; 2. get the correct position of the string to place our ASCII char
    ; bx <- base address + string length - index of char
    mov bx, HEX_OUT + 5 ; base + length
    sub bx, cx  ; our index variable
    mov [bx], al ; copy the ASCII char on 'al' to the position pointed by 'bx'
    ror dx, 4 ; 0x1234 -> 0x4123 -> 0x3412 -> 0x2341 -> 0x1234

    ; increment index and loop
    add cx, 1
    jmp hex_loop

end_block:
    ; prepare the parameter and call the function
	; we already put the string to be put into the HEX_OUT location
    ; remember that print receives parameters in 'bx'
    mov bx, HEX_OUT
    call print_string

    popa
    ret

HEX_OUT:
    db '0x0000', 0 ; reserve memory for our new string

print_string:
	; bx is the parameter string address
	; this is function for print string
	pusha ; push all register values to the stack

	call_print_char_block:
		mov cx, [bx] ; and compare if the char is 0
		cmp cl, 0 ; cl! not cx!! because the higher of cx may mot be zero!
		je print_string_done_block ; if it is 0 then done

		mov al, [bx]
		call print_char
		add bx, 1 ; move to the next byte
		
		jmp call_print_char_block ; else we keep printing


	print_string_done_block:
		popa ; resume the registers
		ret


print_char:
	; so what we can do is actaully storing all the global registers on a stack
	; then we can use it locally
	; after the function being done, we can resume all registers from the stack to resume the state
	pusha ; push all register values to the stack
	
	mov ah, 0x0e
	int 0x10
	popa ; resume the registers
	ret
