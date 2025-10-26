# Linux适配的SAOS操作系统Makefile
# 需要安装: sudo apt install gcc g++ binutils nasm qemu-system-x86 grub-pc-bin xorriso mtools

# Linux工具链配置
CC = gcc
AS = as
LD = ld
GPPPARAMS = -m32 -Iinclude -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-leading-underscore
ASPARAMS = --32
LDPARAMS = -melf_i386
QEMU = qemu-system-i386
GRUB_MKRESCUE = grub-mkrescue

objects = obj/loader.o \
		  obj/gdt.o \
		  obj/hardwares/port.o \
		  obj/hardwares/interruptstubs.o \
		  obj/hardwares/interrupts.o \
		  obj/hardwares/pci.o \
		  obj/drivers/driver.o \
		  obj/drivers/keyboard.o \
		  obj/drivers/mouse.o \
		  obj/drivers/vga.o\
		  obj/kernel.o

# 编译C++源文件
obj/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CC) $(GPPPARAMS) -o $@ -c $<

# 编译汇编源文件  
obj/%.o: src/%.s
	@mkdir -p $(@D)
	$(AS) $(ASPARAMS) -o $@ $<

# 链接生成内核二进制文件
sakernel.bin: linker.ld $(objects)
	$(LD) $(LDPARAMS) -T $< -o $@ $(objects)

# 创建ISO镜像文件
sakernel.iso: sakernel.bin
	@mkdir -p iso/boot/grub
	@cp $< iso/boot/
	@echo 'set timeout=0' > iso/boot/grub/grub.cfg
	@echo 'set default=0' >> iso/boot/grub/grub.cfg
	@echo '' >> iso/boot/grub/grub.cfg
	@echo 'menuentry "SAOS" {' >> iso/boot/grub/grub.cfg
	@echo '  multiboot /boot/sakernel.bin' >> iso/boot/grub/grub.cfg
	@echo '  boot' >> iso/boot/grub/grub.cfg
	@echo '}' >> iso/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) --output=$@ iso
	@rm -rf iso

# 在QEMU中运行操作系统
run: sakernel.iso
	$(QEMU) -cdrom sakernel.iso -m 32M -serial stdio

# 在QEMU中运行（调试模式，带串口输出）
debug: sakernel.iso
	$(QEMU) -cdrom sakernel.iso -m 32M -serial stdio -d int

# VirtualBox运行方式（需要预先创建名为"saos"的虚拟机）
run-vbox: sakernel.iso
	@(killall VirtualBoxVM && sleep 1) || true
	@VBoxManage startvm "saos" --type gui || VirtualBoxVM --startvm "saos" &

# 检查依赖工具是否安装
check-deps:
	@echo "检查Linux上的依赖工具..."
	@which $(CC) > /dev/null || (echo "❌ $(CC) 未安装. 运行: sudo apt install gcc" && exit 1)
	@which $(AS) > /dev/null || (echo "❌ $(AS) 未安装. 运行: sudo apt install binutils" && exit 1) 
	@which $(LD) > /dev/null || (echo "❌ $(LD) 未安装. 运行: sudo apt install binutils" && exit 1)
	@which nasm > /dev/null || (echo "❌ nasm 未安装. 运行: sudo apt install nasm" && exit 1)
	@which $(QEMU) > /dev/null || (echo "❌ $(QEMU) 未安装. 运行: sudo apt install qemu-system-x86" && exit 1)
	@which $(GRUB_MKRESCUE) > /dev/null || (echo "❌ $(GRUB_MKRESCUE) 未安装. 运行: sudo apt install grub-pc-bin xorriso" && exit 1)
	@which mformat > /dev/null || (echo "❌ mtools 未安装. 运行: sudo apt install mtools" && exit 1)
	@echo "✅ 所有依赖工具已安装!"

# 安装Linux依赖
install-deps:
	@echo "安装Linux依赖工具..."
	sudo apt update
	sudo apt install -y gcc g++ binutils nasm qemu-system-x86 grub-pc-bin xorriso mtools

# 显示帮助信息
help:
	@echo "SAOS操作系统 - Linux构建系统"
	@echo ""
	@echo "可用命令:"
	@echo "  make run          - 在QEMU中运行操作系统"
	@echo "  make debug        - 在QEMU中运行（调试模式）"  
	@echo "  make run-vbox     - 在VirtualBox中运行"
	@echo "  make sakernel.iso - 只构建ISO镜像"
	@echo "  make sakernel.bin - 只构建内核二进制文件"
	@echo "  make check-deps   - 检查依赖工具"
	@echo "  make install-deps - 安装依赖工具"
	@echo "  make clean        - 清理构建文件"
	@echo "  make help         - 显示此帮助"

.PHONY: clean run run-vbox debug check-deps install-deps help
clean:
	rm -rf obj sakernel.bin sakernel.iso