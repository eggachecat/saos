# ============================================================================
# 中断存根(Interrupt Stubs)汇编文件
# ============================================================================
# 文件目的：
#   这个文件是操作系统中断处理的底层实现，使用汇编语言编写。
#   它提供了从硬件中断到C++中断处理函数的桥梁。
#
# 为什么需要这个文件？
#   当CPU收到中断(如键盘按键、鼠标移动)时，它会跳转到预先设置好的地址。
#   这些地址必须指向汇编代码，因为我们需要精确控制寄存器和栈的状态。
#   然后我们才能安全地调用高级语言(C++)编写的中断处理逻辑。
# ============================================================================

# ============================================================================
# x86寄存器完全指南
# ============================================================================
# 寄存器是CPU内部的高速存储单元，比内存快得多。
# 在32位x86架构中，主要有以下几类寄存器：
#
# ┌─────────────────────────────────────────────────────────────────────────┐
# │ 1. 通用寄存器 (General Purpose Registers) - 8个，每个32位             │
# └─────────────────────────────────────────────────────────────────────────┘
#
# EAX - Extended Accumulator Register (扩展累加器寄存器)
#   • 全称: Accumulator (累加器)
#   • 主要用途: 算术运算、函数返回值
#   • 特殊用途: 乘法和除法指令的隐式操作数
#   • 在函数调用中: 存储返回值
#   • 历史: 16位时代叫AX，8位部分是AH(高8位)和AL(低8位)
#
# EBX - Extended Base Register (扩展基址寄存器)
#   • 全称: Base (基址)
#   • 主要用途: 数据指针、数组索引的基址
#   • 特点: 在某些调用约定中是"被调用者保存"(callee-saved)
#   • 常用于: 指向数据结构的基地址
#
# ECX - Extended Counter Register (扩展计数器寄存器)
#   • 全称: Counter (计数器)
#   • 主要用途: 循环计数器
#   • 特殊用途: 字符串操作(rep指令)和位移操作的计数
#   • 在循环中: for(int i=0; i<100; i++) 的 i 经常用ECX
#
# EDX - Extended Data Register (扩展数据寄存器)
#   • 全称: Data (数据)
#   • 主要用途: 数据存储、I/O端口访问
#   • 特殊用途: 与EAX配合进行64位乘除法(EDX:EAX表示64位)
#   • 在函数调用中: 可能用于传递参数或扩展返回值
#
# ESI - Extended Source Index (扩展源索引寄存器)
#   • 全称: Source Index (源索引)
#   • 主要用途: 字符串操作和内存复制的源地址
#   • 字符串操作: movs, lods等指令隐式使用ESI作为源
#   • 例如: memcpy(dest, src, len) 中的 src 地址常用ESI
#
# EDI - Extended Destination Index (扩展目标索引寄存器)
#   • 全称: Destination Index (目标索引)
#   • 主要用途: 字符串操作和内存复制的目标地址
#   • 字符串操作: movs, stos等指令隐式使用EDI作为目标
#   • 例如: memcpy(dest, src, len) 中的 dest 地址常用EDI
#
# EBP - Extended Base Pointer (扩展基址指针寄存器)
#   • 全称: Base Pointer (基址指针)
#   • 主要用途: 栈帧指针，指向当前函数栈帧的基址
#   • 作用: 访问函数的局部变量和参数
#   • 典型用法: [EBP-4]访问局部变量, [EBP+8]访问函数参数
#   • 函数序言: push %ebp; mov %esp, %ebp (建立新栈帧)
#
# ESP - Extended Stack Pointer (扩展栈指针寄存器)
#   • 全称: Stack Pointer (栈指针)
#   • 主要用途: 永远指向栈顶
#   • 特点: 由push/pop/call/ret指令自动维护
#   • 栈增长: x86栈向下增长，push使ESP减小，pop使ESP增大
#   • 重要性: 如果ESP损坏，系统会崩溃！
#
# ┌─────────────────────────────────────────────────────────────────────────┐
# │ 2. 段寄存器 (Segment Registers) - 6个，每个16位                        │
# └─────────────────────────────────────────────────────────────────────────┘
# 在保护模式下，段寄存器存储"段选择子"，指向GDT/LDT中的段描述符
#
# CS - Code Segment (代码段寄存器)
#   • 用途: 指向当前执行代码所在的段
#   • 与EIP配合: CS:EIP 形成完整的代码地址
#   • 特殊性: 不能直接mov修改，只能通过far jmp/call或中断修改
#   • 特权级: CS的低2位(CPL)表示当前特权级(0=内核,3=用户)
#
# DS - Data Segment (数据段寄存器)
#   • 用途: 指向数据段，默认的数据访问段
#   • 访问数据: mov %eax, [地址] 默认使用DS段
#   • 可以修改: mov $0x10, %ax; mov %ax, %ds
#
# ES - Extra Segment (额外段寄存器)
#   • 用途: 额外的数据段，用于字符串操作的目标
#   • 字符串指令: movs, stos等使用ES:EDI作为目标地址
#
# FS - F Segment (附加段寄存器F)
#   • 用途: 附加数据段
#   • 现代用法: 在Linux中用于线程局部存储(TLS - Thread Local Storage)
#   • 例如: mov %fs:0x10, %eax 访问线程特定的数据
#
# GS - G Segment (附加段寄存器G)
#   • 用途: 附加数据段
#   • 现代用法: 在Windows x64中用于TEB(线程环境块)
#
# SS - Stack Segment (栈段寄存器)
#   • 用途: 指向栈所在的段
#   • 与ESP配合: SS:ESP 形成完整的栈地址
#   • 自动使用: push/pop/call/ret指令隐式使用SS段
#
# ┌─────────────────────────────────────────────────────────────────────────┐
# │ 3. 特殊寄存器 (Special Registers)                                      │
# └─────────────────────────────────────────────────────────────────────────┘
#
# EIP - Extended Instruction Pointer (扩展指令指针)
#   • 全称: Instruction Pointer (指令指针)
#   • 用途: 指向下一条要执行的指令的地址
#   • 特点: 不能直接访问(不能mov %eip, %eax)
#   • 修改方式: 通过jmp, call, ret, int, iret等指令间接修改
#   • 中断时: CPU自动保存EIP到栈上
#
# EFLAGS - Extended Flags Register (扩展标志寄存器)
#   • 用途: 存储CPU的状态标志位，32位，每一位有特定含义
#   • 重要标志位:
#     - CF (bit 0): Carry Flag (进位标志) - 无符号运算溢出
#     - PF (bit 2): Parity Flag (奇偶标志) - 结果最低字节1的个数是否为偶数
#     - ZF (bit 6): Zero Flag (零标志) - 运算结果是否为0
#     - SF (bit 7): Sign Flag (符号标志) - 运算结果的符号(最高位)
#     - IF (bit 9): Interrupt Flag (中断标志) - 是否允许硬件中断
#                   cli指令清除IF(禁用中断)，sti指令设置IF(启用中断)
#     - DF (bit 10): Direction Flag (方向标志) - 字符串操作的方向
#                    cld清除DF(向上)，std设置DF(向下)
#     - OF (bit 11): Overflow Flag (溢出标志) - 有符号运算溢出
#   • 条件跳转: je, jne, jg, jl等指令根据EFLAGS的标志位判断
#   • 中断时: CPU自动保存EFLAGS到栈上
#
# ┌─────────────────────────────────────────────────────────────────────────┐
# │ 寄存器命名规则 (16位 → 32位 → 64位)                                    │
# └─────────────────────────────────────────────────────────────────────────┘
# 8位时代:   AL, AH, BL, BH, CL, CH, DL, DH
# 16位时代:  AX, BX, CX, DX, SI, DI, BP, SP
# 32位时代:  EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP (E = Extended)
# 64位时代:  RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP (R = Register)
#            还新增了 R8-R15
#
# ┌─────────────────────────────────────────────────────────────────────────┐
# │ 为什么中断处理要保存所有这些寄存器？                                    │
# └─────────────────────────────────────────────────────────────────────────┘
# 中断可能在任何时刻发生，打断正在运行的程序。为了让程序能无缝恢复，
# 我们必须保存它的完整状态。如果不保存：
#   - 被中断的程序的变量(存在寄存器中)会丢失
#   - 循环计数器会错乱
#   - 函数返回值会被覆盖
#   - 栈帧指针会损坏
# 所以我们用 pusha + 段寄存器保存 = 完整的CPU上下文保护
# ============================================================================

# IRQ_BASE = 0x20 (32)
# 这是硬件中断请求(IRQ)的基地址。在x86架构中，前32个中断号(0-31)
# 被保留给CPU异常(如除零错误、缺页错误等)。硬件中断从32开始编号。
.set IRQ_BASE, 0x20

.section .text

# ============================================================================
# C++函数声明（通过名字修饰后的符号）
# ============================================================================
# 不寻常之处：C++名字修饰(Name Mangling)
#
# 在C++中，函数名 saos::hardwares::InterruptManager::HandleInterrupt
# 会被编译器"修饰"(mangle)成一个复杂的符号名。这是因为C++支持：
#   - 命名空间 (namespace)
#   - 类 (class)
#   - 函数重载 (overloading)
#
# 修饰后的名字包含了完整的类型信息，格式大致是：
#   _Z  N  4saos  9hardwares  16InterruptManager  15HandleInterrupt  E  h  j
#   |   |  |      |            |                   |                  |  |  |
#   |   |  |      |            |                   |                  |  |  参数类型: unsigned int (j)
#   |   |  |      |            |                   |                  |  参数类型: unsigned char (h)
#   |   |  |      |            |                   |                  结束嵌套名称 (E)
#   |   |  |      |            |                   函数名(15个字符)
#   |   |  |      |            类名(16个字符)
#   |   |  |      命名空间名(9个字符)
#   |   |  命名空间名(4个字符)
#   |   开始嵌套名称 (N)
#   表示这是修饰过的名字 (Z)
#
# 你可以使用 "c++filt" 工具来解码这些符号：
#   echo "_ZN4saos9hardwares16InterruptManager15HandleInterruptEhj" | c++filt
#   输出: saos::hardwares::InterruptManager::HandleInterrupt(unsigned char, unsigned int)
#
.extern _ZN4saos9hardwares16InterruptManager15HandleInterruptEhj

# ============================================================================
# 汇编宏模式：HandleException
# ============================================================================
# 汇编宏(Macro)模式说明：
#   宏是一种代码生成技术。当你写 "HandleException 5" 时，汇编器会
#   展开这个宏，自动生成一个完整的异常处理函数。
#
# 这个宏的作用：
#   为每个CPU异常(0-31号中断)生成一个小的处理函数
#
# 参数：
#   \num - 异常编号(如 0=除零错误, 13=一般保护错误, 14=缺页错误等)
#
# 宏参数语法详解：
#   .macro HandleException num    <- 定义宏，参数名是 num
#       movb $\num, ...           <- 使用时必须写 \num (带反斜杠)
#   
#   为什么需要反斜杠 \ ？
#     反斜杠是"参数替换符"，告诉汇编器"把这里替换成参数的值"
#   
#   例子：
#     调用: HandleException 5
#     
#     如果写: movb $num, (interruptnumber)
#       结果: movb $num, (interruptnumber)  <- 错误！字面的"num"字符串
#     
#     如果写: movb $\num, (interruptnumber)  
#       结果: movb $5, (interruptnumber)    <- 正确！5被替换进来了
#   
#   类比：
#     C/C++中:     #define MACRO(x) x*2      使用: MACRO(5) → 5*2
#     汇编宏中:    .macro MACRO x            使用: \x 来引用参数
#   
#   特殊用法 \num\()：
#     有时你会看到 \num\()，这是因为宏参数后面直接跟着其他字符时，
#     需要用 \() 来明确参数的边界。
#     例如: HandleInterruptRequest\num\()Ev
#           如果写成 HandleInterruptRequest\numEv，汇编器会认为参数
#           名是 "numEv" 而不是 "num"，所以用 \() 来分隔。
#
.macro HandleException num
.global _ZN164saos9hardwaresInterruptManager19HandleException\num\()Ev
_ZN4saos9hardwares16InterruptManager19HandleException\num\()Ev:
    # 保存中断编号到内存变量
    # movb = move byte (移动一个字节)
    # $\num = 立即数，即中断编号
    # (interruptnumber) = 直接内存寻址，存储到interruptnumber变量
    movb $\num, (interruptnumber)
    
    # 跳转到通用的中断处理代码
    # 所有中断最终都会汇聚到 int_bottom，那里有统一的处理逻辑
    jmp int_bottom
.endm


# ============================================================================
# 汇编宏模式：HandleInterruptRequest
# ============================================================================
# 这个宏的作用：
#   为每个硬件中断请求(IRQ)生成一个小的处理函数
#
# IRQ和异常的区别：
#   - 异常：CPU内部产生(如除零、非法指令)，编号0-31
#   - IRQ：外部硬件产生(如键盘、鼠标、硬盘)，编号32+
#
# 参数：
#   \num - IRQ编号(如 0=定时器, 1=键盘, 12=鼠标等)
#   实际中断号 = IRQ_BASE + \num = 32 + \num
#
# 注意：这里同样使用 \num 来引用宏参数(参见上面的详细语法说明)
#
.macro HandleInterruptRequest num
.global _ZN4saos9hardwares16InterruptManager26HandleInterruptRequest\num\()Ev
_ZN4saos9hardwares16InterruptManager26HandleInterruptRequest\num\()Ev:
    # 计算实际中断号 = IRQ_BASE(32) + IRQ编号
    # 例如：键盘是IRQ1，实际中断号是33
    movb $\num + IRQ_BASE, (interruptnumber)
    
    # 跳转到通用中断处理代码
    jmp int_bottom
.endm

# 使用宏生成具体的中断处理函数：
HandleInterruptRequest 0x00  # IRQ0: 定时器中断(每秒触发多次，用于任务调度)
HandleInterruptRequest 0x01  # IRQ1: 键盘中断(每次按键/释放键时触发)
HandleInterruptRequest 0x0C  # IRQ12: PS/2鼠标中断(鼠标移动或点击时触发)

# ============================================================================
# 通用中断处理入口：int_bottom
# ============================================================================
# 所有的中断(无论是异常还是硬件中断)都会跳转到这里。
# 这里实现了一个关键的汇编模式：中断上下文保存与恢复
#
# 中断处理的三个阶段：
#   1. 保存当前程序的状态(所有寄存器)
#   2. 调用C++编写的中断处理函数
#   3. 恢复之前的状态，返回到被中断的程序
#
int_bottom:

# ============================================================================
# 阶段1：保存CPU状态到栈上
# ============================================================================
# 汇编模式：完整的上下文保存(Full Context Save)
#
# 为什么要保存？
#   中断可能在任何时刻发生，打断正在运行的程序。我们必须保存所有
#   寄存器的值，这样中断处理完成后，原程序可以无缝继续执行，
#   就像中断从未发生过一样。
#
# 栈的增长方向：
#   在x86架构中，栈向下增长(从高地址到低地址)
#   push操作会先减小ESP(栈指针)，再写入数据

    # pusha - Push All
    # 这是一个特殊的x86指令，一次性保存8个通用寄存器：
    # EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    # 这是保存上下文的常见模式
    #
    # 为什么pusha没有目标操作数(dst)？
    #   push类指令永远推送到"栈(Stack)"，目标是隐含的！
    #   栈的位置由 ESP 寄存器(栈指针)决定。
    #
    # pusha的工作原理：
    #   1. ESP = ESP - 4  (栈指针向下移动4字节，栈向低地址增长)
    #   2. [ESP] = EAX    (将EAX的值写入ESP指向的内存地址)
    #   3. ESP = ESP - 4
    #   4. [ESP] = ECX    (将ECX的值写入新的ESP位置)
    #   5. ... 重复8次，依次保存所有寄存器
    #
    # 压栈顺序(从最后压入到最先压入)：
    #   [ESP]     ← 栈顶(低地址)
    #   EDI       ← 最后压入(最先被pop出来)
    #   ESI
    #   EBP
    #   ESP (原始值)
    #   EBX
    #   EDX
    #   ECX
    #   EAX       ← 最先压入(最后被pop出来)
    #   [ESP-32]  ← pusha前的栈顶位置
    #
    # 类比：
    #   pusha 相当于手动执行：
    #     push %eax
    #     push %ecx
    #     push %edx
    #     push %ebx
    #     push %esp
    #     push %ebp
    #     push %esi
    #     push %edi
    #   但pusha只需要1条指令，更快更简洁！
    #
    # 内存中的实际情况：
    #   假设执行pusha前，ESP = 0x00100000
    #   执行pusha后，ESP = 0x000FFFE0 (减少了32字节 = 8寄存器 × 4字节)
    #   这32字节的内存现在存储着所有寄存器的值
    pusha
    
    # 保存段寄存器(Segment Registers)
    # 在保护模式下，这些寄存器存储段选择子，指向GDT/LDT中的段描述符
    # 
    # pushl同样推送到栈(由ESP隐式指定)：
    #   pushl = push long (推送一个长整型，4字节)
    pushl %ds   # 数据段寄存器，同样推到[ESP]位置，然后ESP-=4
    pushl %es   # 额外段寄存器  
    pushl %fs   # 附加段寄存器
    pushl %gs   # 附加段寄存器
    
    # 此时栈的布局(从栈顶到栈底，低地址到高地址)：
    #   内存地址        内容
    #   [ESP]      ←   GS    ← 栈顶(当前ESP指向这里)
    #   [ESP+4]        FS
    #   [ESP+8]        ES
    #   [ESP+12]       DS
    #   [ESP+16]       EDI
    #   [ESP+20]       ESI
    #   [ESP+24]       EBP
    #   [ESP+28]       ESP(原始值)
    #   [ESP+32]       EBX
    #   [ESP+36]       EDX
    #   [ESP+40]       ECX
    #   [ESP+44]       EAX   ← 栈底(相对于我们保存的数据)
    #   
    # 总共保存了12个值，占用48字节

# ============================================================================
# 阶段2：调用C++中断处理函数
# ============================================================================
# 汇编模式：C调用约定(C Calling Convention)
#
# x86 C调用约定的规则：
#   1. 参数从右到左压入栈
#   2. 调用者负责清理栈(caller cleanup)
#   3. 返回值通过EAX寄存器传递

    # 准备第二个参数：当前栈指针
    # ESP现在指向我们刚才保存的所有寄存器，C++函数可以访问这些数据
    # push同样没有dst，隐式推送到栈上
    push %esp
    
    # 准备第一个参数：中断编号
    # (interruptnumber)是直接内存寻址，读取我们之前保存的中断号
    # 这个push把interruptnumber变量的值(1字节)推送到栈上
    push (interruptnumber)
    
    # 调用C++函数：
    # saos::hardwares::InterruptManager::HandleInterrupt(unsigned char num, unsigned int esp)
    call _ZN4saos9hardwares16InterruptManager15HandleInterruptEhj
    
    # 注意：这里有个被注释掉的清理栈的指令
    # addl $5, %esp  # 这个指令是错误的！应该是 addl $8, %esp
    #                # 因为我们压了2个参数，每个4字节，共8字节
    
    # 不寻常之处：直接使用返回值作为新的栈指针
    # 通常中断处理函数返回后，我们会继续使用原来的栈。
    # 但这里允许C++函数返回一个新的ESP值(通过EAX)，这样可以实现：
    #   - 任务切换(Task Switching)
    #   - 改变返回地址
    #   - 切换到不同的栈
    # 这是实现多任务操作系统的关键技术！
    movl %eax, %esp

# ============================================================================
# 阶段3：恢复CPU状态
# ============================================================================
# 汇编模式：上下文恢复(Context Restore)
#
# pop指令的顺序必须与push相反(后进先出，LIFO - Last In First Out)
#
# pop指令的工作原理：
#   pop也没有源操作数(src)！它隐式地从栈上读取数据。
#   1. 读取 [ESP] 指向的内存值
#   2. 将该值存入目标寄存器
#   3. ESP = ESP + 4 (栈指针向上移动，释放栈空间)
#
# 例如：popl %gs
#   1. %gs = [ESP]    (从栈顶读取值到GS寄存器)
#   2. ESP = ESP + 4  (栈指针上移4字节)

    # 恢复段寄存器(顺序与push相反)
    # 必须按照 LIFO 原则：最后push的最先pop
    popl %gs    # 从[ESP]恢复GS，然后ESP+=4
    popl %fs    # 从[ESP]恢复FS，然后ESP+=4
    popl %es    # 从[ESP]恢复ES，然后ESP+=4
    popl %ds    # 从[ESP]恢复DS，然后ESP+=4
    
    # popa - Pop All
    # 恢复所有通用寄存器(与pusha相反)
    # 一次性执行8次pop操作，顺序与pusha相反：
    #   pop %edi, pop %esi, pop %ebp, (跳过ESP), pop %ebx, pop %edx, pop %ecx, pop %eax
    # 执行后，ESP增加32字节，所有通用寄存器恢复到中断前的值
    popa

    # iret - Interrupt Return
    # 这是一个特殊的x86指令，专门用于从中断返回
    # iret也隐式地使用栈(通过ESP)！
    #
    # 当中断发生时，CPU自动将这些值压栈：
    #   [老的EFLAGS]  ← CPU自动push
    #   [老的CS]      ← CPU自动push
    #   [老的EIP]     ← CPU自动push，然后跳转到中断处理函数
    #
    # iret会从栈上依次弹出(隐式使用ESP)：
    #   1. pop EIP (指令指针) - 恢复到被中断的代码位置
    #   2. pop CS (代码段选择子) 
    #   3. pop EFLAGS (标志寄存器) - 恢复所有CPU标志位
    #      (如果涉及特权级切换，还会弹出 SS 和 ESP)
    #
    # iret相当于：
    #   pop %eip
    #   pop %cs  
    #   pop %eflags
    #   然后跳转到EIP指向的地址
    #
    # iret执行后，CPU就像从未被中断过一样，继续执行原来的程序
    iret


# ============================================================================
# 数据段：存储中断编号
# ============================================================================
.data
    # 定义一个1字节的变量，用于临时存储当前处理的中断编号
    # 所有中断处理函数共享这个变量
    # .byte 0 表示初始化为0
    interruptnumber: .byte 0
