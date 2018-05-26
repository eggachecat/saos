gcc -ffreestanding -c basic.c -o tmp.o
ld -o tmp.tmp tmp.o
objcopy -O binary tmp.tmp output.bin