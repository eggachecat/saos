[org 0x7c00]
	mov [BOOT_DRIVE], dl ; BIOS stores our boot drive in DL , so it 's
						 ; best to remember this for later.
    mov bp, 0x8000 ; set the stack safely away from us
    mov sp, bp

    mov bx, 0x9000 ; es:bx = 0x0000:0x9000 = 0x09000
    mov dh, 2 ; read 2 sectors
			  ; the bios sets 'dl' for our boot disk number
			  ; if you have trouble, use the '-fda' flag: 'qemu -fda file.bin'
    mov dl , [BOOT_DRIVE]
    call disk_load

    mov dx, [0x9000] ; retrieve the first loaded word, 0xdada
    call print_hex

    mov dx, [0x9000 + 512] ; first word from second loaded sector, 0xface
    call print_hex

    jmp $

%include "./libs/__init__.asm"

BOOT_DRIVE : 
	db 0

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

; Magic number
times 510 - ($-$$) db 0
dw 0xaa55

; boot sector = sector 1 of cyl 0 of head 0 of hdd 0
; from now on = sector 2 ...
times 256 dw 0xdada ; sector 2 = 512 bytes
times 256 dw 0xface ; sector 3 = 512 bytes






