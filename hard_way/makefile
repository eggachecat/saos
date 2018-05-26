NASM = nasm
QEMU = qemu-system-x86_64

boot.bin: boot.asm
	$(NASM) -f bin boot.asm -o boot.bin

run: boot.bin
	$(QEMU) boot.bin

clean:
	rm *.o