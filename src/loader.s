# 这是一个计算图（类似TensorFlow中的计算图）

# 多重引导规范(Multiboot)的魔数和标志
.set MAGIC, 0x1badb002      # 多重引导魔数，GRUB用此识别内核
.set FLAGS, (1<<0 | 1<<1)   # 标志位：请求内存信息 + 页面对齐
.set CHECKSUM, -(MAGIC + FLAGS)  # 校验和，确保MAGIC+FLAGS+CHECKSUM=0

# 多重引导头部结构
# 包含RAM大小等信息，存储在ax寄存器中
# 魔数复制到bx寄存器
.section .multiboot
    .long MAGIC      # 魔数
    .long FLAGS      # 标志
    .long CHECKSUM   # 校验和

.section .text
.extern kernelMain        # 外部C++内核主函数
.extern callConstructors  # 外部C++构造函数调用器
.global loader           # 导出loader符号给链接器

loader:
    # 设置内核栈指针 - 为C++代码提供栈空间
    mov $kernel_stack, %esp

    # 调用C++全局对象构造函数（必须在main之前）
    call callConstructors
    
    # 传递GRUB提供的参数给内核主函数
    push %eax   # 多重引导信息结构指针
    push %ebx   # 魔数
    call kernelMain

# 内核主函数返回后的停机循环
_stop:
    cli    # 禁用中断
    hlt    # 停机等待中断
    jmp _stop  # 无限循环（防止意外继续执行）

# BSS段 - 未初始化数据段
.section .bss
.space 2*1024*1024; # 分配2MB内核栈空间
kernel_stack:       # 栈顶标记
