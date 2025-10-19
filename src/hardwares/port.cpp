/**
 * @file port.cpp
 * @brief x86端口I/O通信的实现
 * 
 * 【大局定位】
 * 这是硬件抽象层的核心实现，封装了x86架构的I/O端口访问指令。
 * 所有硬件驱动（键盘、鼠标、PCI等）都依赖这个模块与硬件通信。
 * 
 * 【关键概念：内联汇编】
 * C++无法直接执行CPU指令，必须通过内联汇编（inline assembly）：
 * ```cpp
 * __asm__ volatile("指令" : 输出操作数 : 输入操作数 : 破坏列表);
 * ```
 * 
 * 【GCC内联汇编约束】
 * - "a": 使用 EAX/AX/AL 寄存器
 * - "Nd": 使用立即数或DX寄存器（端口号限制）
 * - "=a": 输出约束，结果写入 EAX/AX/AL
 * - volatile: 告诉编译器不要优化掉这段代码（硬件操作有副作用）
 * 
 * 【端口地址限制】
 * x86端口地址范围：0x0000-0xFFFF（16位）
 * - 0x0000-0x00FF: 可以使用立即数寻址（快速）
 * - 0x0100-0xFFFF: 必须使用DX寄存器寻址
 * 
 * 【常用端口地址】
 * 0x20, 0x21: 主PIC（中断控制器）
 * 0x60, 0x64: 键盘/鼠标控制器
 * 0xA0, 0xA1: 从PIC
 * 0x3C0-0x3DF: VGA显卡寄存器
 * 0x3F8-0x3FF: COM1串口
 * 0xCF8, 0xCFC: PCI配置空间
 * 
 * 【与主循环的交互】
 * 1. 系统初始化时，各驱动创建Port对象（指定端口号）
 * 2. 中断发生时，HandleInterrupt调用Port::Read()读取数据
 * 3. 驱动激活时，Activate()调用Port::Write()配置硬件
 * 4. 主循环等待中断 → 中断处理 → 端口读写 → 返回主循环
 */

#include <hardwares/port.h>
using namespace saos::common;
using namespace saos::hardwares;

/**
 * 【构造函数】Port基类
 * 
 * 职责：保存端口号，供子类使用
 * 
 * @param portnumber 端口地址（0x0000-0xFFFF）
 * 
 * 【设计考虑】
 * - 端口号在对象生命周期内不变，因此在构造时初始化
 * - 使用初始化列表（更高效，避免默认构造+赋值）
 * - 不进行有效性检查（假设调用者提供正确地址）
 */
Port::Port(uint16_t portnumber)
{
    this->portnumber = portnumber;
}

Port::~Port() {}

// ============================================================================
// 8位端口实现
// ============================================================================

/**
 * 【构造函数】Port_8Bit
 * 
 * @param portnumber 8位端口地址
 * 
 * 【使用示例】
 * ```cpp
 * // 键盘数据端口
 * Port_8Bit keyboardData(0x60);
 * 
 * // PIC主控制器命令端口
 * Port_8Bit picMasterCmd(0x20);
 * ```
 */
Port_8Bit::Port_8Bit(uint16_t portnumber)
    : Port(portnumber) {}

Port_8Bit::~Port_8Bit() {}

/**
 * 【核心函数】向8位端口写入数据
 * 
 * @param data 要写入的字节（0x00-0xFF）
 * 
 * 【底层原理】
 * 使用x86的 outb 指令（Output Byte）：
 * - 将AL寄存器的值输出到指定端口
 * - 格式：outb AL, DX 或 outb AL, imm8
 * 
 * 【内联汇编详解】
 * ```asm
 * outb %0, %1
 * ```
 * - %0: 第一个操作数（data），约束"a"表示放入AL寄存器
 * - %1: 第二个操作数（portnumber），约束"Nd"表示：
 *   - N: 0-255范围的立即数（快速）
 *   - d: 如果超出范围，使用DX寄存器
 * 
 * 【具体例子】
 * ```cpp
 * Port_8Bit keyboardCmd(0x64);
 * keyboardCmd.Write(0xAE);  // 发送"启用键盘接口"命令
 * ```
 * 
 * 执行过程：
 * 1. data=0xAE 加载到 AL 寄存器
 * 2. portnumber=0x64 作为立即数或加载到 DX
 * 3. CPU执行 outb 0xAE, 0x64
 * 4. 键盘控制器接收到命令，启用键盘接口
 * 
 * 【volatile的作用】
 * 告诉编译器：这段代码有副作用（硬件状态改变），不能：
 * - 优化掉（即使结果未使用）
 * - 重排序（顺序很重要）
 * - 合并多次写入（每次都有意义）
 */
void Port_8Bit::Write(uint8_t data)
{
    asm volatile("outb %0, %1"
                 :                          // 无输出操作数
                 : "a"(data), "Nd"(portnumber)); // 输入：data→AL, portnumber→端口
}

/**
 * 【核心函数】从8位端口读取数据
 * 
 * @return 从端口读取的字节值
 * 
 * 【底层原理】
 * 使用x86的 inb 指令（Input Byte）：
 * - 从指定端口读取1字节到AL寄存器
 * - 格式：inb DX, AL 或 inb imm8, AL
 * 
 * 【内联汇编详解】
 * ```asm
 * inb %1, %0
 * ```
 * - %0: 输出操作数（result），约束"=a"表示从AL寄存器读取结果
 * - %1: 输入操作数（portnumber）
 * 
 * 【具体例子：键盘中断处理】
 * ```cpp
 * uint32_t KeyboardDriver::HandleInterrupt(uint32_t esp) {
 *     Port_8Bit dataport(0x60);
 *     uint8_t scancode = dataport.Read();  // ← 调用这个函数
 *     
 *     // scancode可能是：
 *     // 0x1E: 'A'键按下
 *     // 0x9E: 'A'键释放（0x1E + 0x80）
 *     // 0xFA: 键盘确认码
 *     
 *     if (scancode == 0x1E) {
 *         handler->OnKeyDown('a');
 *     }
 *     return esp;
 * }
 * ```
 * 
 * 执行过程：
 * 1. CPU执行 inb 0x60, AL
 * 2. 键盘控制器将输出缓冲区的数据放到数据总线上
 * 3. CPU从数据总线读取数据到AL寄存器
 * 4. AL的值复制到result变量
 * 5. 返回result（扫描码）
 * 
 * 【时序考虑】
 * 某些设备需要时间准备数据，读取前应检查状态寄存器：
 * ```cpp
 * // 等待键盘输出缓冲区有数据
 * while (!(commandport.Read() & 0x01)) {
 *     // 位0=1表示有数据可读
 * }
 * uint8_t data = dataport.Read();
 * ```
 */
uint8_t Port_8Bit::Read()
{
    uint8_t result;
    asm volatile("inb %1, %0"
                 : "=a"(result)              // 输出：AL→result
                 : "Nd"(portnumber));        // 输入：portnumber→端口
    return result;
}

// ============================================================================
// 8位慢速端口实现（带延迟）
// ============================================================================

/**
 * 【构造函数】Port_8Bit_Slow
 * 
 * 【使用场景】
 * - PIC中断控制器（老旧硬件，响应慢）
 * - ISA总线设备
 * - 某些老式声卡/网卡
 * 
 * 【为什么慢速版本只用于写入？】
 * - 写入命令需要设备处理，可能需要时间
 * - 读取通常是读取已准备好的数据，不需要额外延迟
 * - 读取前通常会检查状态位，已经有了隐式等待
 */
Port_8Bit_Slow::Port_8Bit_Slow(uint16_t portnumber)
    : Port_8Bit(portnumber) {}

Port_8Bit_Slow::~Port_8Bit_Slow() {}

/**
 * 【核心函数】向慢速8位端口写入数据（带延迟）
 * 
 * @param data 要写入的字节
 * 
 * 【延迟机制】
 * ```asm
 * outb %0, %1          ; 写入数据
 * jmp 1f               ; 跳转到标签1（消耗时钟周期）
 * 1: jmp 1f            ; 再跳一次（继续消耗时钟）
 * 1:                   ; 继续执行后续代码
 * ```
 * 
 * 【标签解释】
 * "1f"表示"向前查找最近的标签1"
 * - 第一个 jmp 1f 跳转到第一个标签1
 * - 第二个 jmp 1f 跳转到第二个标签1
 * - 两次跳转共消耗约2-3个时钟周期
 * 
 * 【延迟时间】
 * 在现代CPU上（3GHz）：
 * - 每个时钟周期 ≈ 0.33纳秒
 * - 2个jmp指令 ≈ 2-3时钟周期 ≈ 1纳秒
 * 
 * 对于老式8259A PIC（响应时间≥1微秒），这个延迟足够。
 * 
 * 【实际应用：PIC初始化】
 * ```cpp
 * Port_8Bit_Slow picMasterCommand(0x20);
 * Port_8Bit_Slow picMasterData(0x21);
 * 
 * // 初始化序列（每步都需要延迟）
 * picMasterCommand.Write(0x11);  // ICW1: 初始化
 * picMasterData.Write(0x20);     // ICW2: 中断向量偏移
 * picMasterData.Write(0x04);     // ICW3: 从PIC连接到IRQ2
 * picMasterData.Write(0x01);     // ICW4: 8086模式
 * ```
 * 
 * 如果没有延迟，PIC可能还没处理完ICW1就收到ICW2，导致初始化失败。
 * 
 * 【性能影响】
 * - PIC初始化只在系统启动时执行一次（8次写入 × 1纳秒 = 8纳秒）
 * - 运行时PIC操作（EOI确认）也是偶尔执行
 * - 对整体性能影响可忽略不计
 */
void Port_8Bit_Slow::Write(uint8_t data)
{
    asm volatile("outb %0, %1\njmp 1f\n1: jmp 1f\n1:"
                 :
                 : "a"(data), "Nd"(portnumber));
}

// ============================================================================
// 16位端口实现
// ============================================================================

/**
 * 【构造函数】Port_16Bit
 * 
 * 【使用场景】
 * - IDE硬盘数据传输（0x1F0数据端口）
 * - 某些声卡采样数据
 * - 网络设备的16位寄存器
 * 
 * 【为什么需要16位？】
 * - 效率：一次传输2字节，减少指令数
 * - 原子性：某些寄存器必须一次性读写完整的16位值
 * - 兼容性：某些老式设备只支持16位访问
 */
Port_16Bit::Port_16Bit(uint16_t portnumber)
    : Port(portnumber) {}

Port_16Bit::~Port_16Bit() {}

/**
 * 【核心函数】向16位端口写入数据
 * 
 * @param data 要写入的16位值（0x0000-0xFFFF）
 * 
 * 【底层原理】
 * 使用 outw 指令（Output Word，word=2字节）：
 * - 将AX寄存器（16位）的值输出到端口
 * 
 * 【字节序】
 * x86是小端序（Little Endian）：
 * 写入 0x1234 时：
 * - 第一个时钟周期：写入 0x34（低字节）
 * - 第二个时钟周期：写入 0x12（高字节）
 * 
 * 【示例：IDE硬盘读取】
 * ```cpp
 * Port_16Bit ideData(0x1F0);  // IDE主控制器数据端口
 * 
 * // 读取一个扇区（512字节 = 256个16位字）
 * uint16_t buffer[256];
 * for (int i = 0; i < 256; i++) {
 *     buffer[i] = ideData.Read();
 * }
 * ```
 */
void Port_16Bit::Write(uint16_t data)
{
    asm volatile("outw %0, %1"
                 :
                 : "a"(data), "Nd"(portnumber));
}

/**
 * 【核心函数】从16位端口读取数据
 * 
 * @return 读取的16位值
 * 
 * 【底层原理】
 * 使用 inw 指令（Input Word）
 */
uint16_t Port_16Bit::Read()
{
    uint16_t result;
    asm volatile("inw %1, %0"
                 : "=a"(result)
                 : "Nd"(portnumber));
    return result;
}

// ============================================================================
// 32位端口实现
// ============================================================================

/**
 * 【构造函数】Port_32Bit
 * 
 * 【主要使用场景：PCI配置空间】
 * x86架构提供两个特殊的32位端口用于PCI总线配置：
 * - 0xCF8（命令端口）：写入要访问的PCI设备地址
 * - 0xCFC（数据端口）：读写PCI配置寄存器
 * 
 * 【其他使用场景】
 * - 高级网络设备（千兆以太网卡）
 * - 现代GPU的配置寄存器
 * - 高性能RAID控制器
 */
Port_32Bit::Port_32Bit(uint16_t portnumber)
    : Port(portnumber) {}

Port_32Bit::~Port_32Bit() {}

/**
 * 【核心函数】向32位端口写入数据
 * 
 * @param data 要写入的32位值（0x00000000-0xFFFFFFFF）
 * 
 * 【底层原理】
 * 使用 outl 指令（Output Long，long=4字节=32位）：
 * - 将EAX寄存器（32位）的值输出到端口
 * 
 * 【典型应用：PCI设备访问】
 * ```cpp
 * Port_32Bit commandPort(0xCF8);
 * Port_32Bit dataPort(0xCFC);
 * 
 * // 读取PCI设备的厂商ID和设备ID
 * uint32_t address = 0x80000000  // 位31=1：使能配置空间访问
 *                  | (bus << 16)  // 位23-16：总线号
 *                  | (device << 11) // 位15-11：设备号
 *                  | (function << 8) // 位10-8：功能号
 *                  | (0x00 & 0xFC);  // 位7-2：寄存器号（4字节对齐）
 * 
 * commandPort.Write(address);  // ← 调用这个函数
 * uint32_t value = dataPort.Read();
 * 
 * // 提取信息
 * uint16_t vendor_id = value & 0xFFFF;          // 低16位
 * uint16_t device_id = (value >> 16) & 0xFFFF;  // 高16位
 * ```
 * 
 * 【原子性的重要性】
 * PCI地址必须一次性写入完整的32位：
 * - 如果分成4次8位写入，中间状态会导致错误
 * - 如果分成2次16位写入，可能访问到错误的设备
 * 
 * 【字节序】
 * 写入 0x80001000 时（小端序）：
 * - 字节0：0x00
 * - 字节1：0x10
 * - 字节2：0x00
 * - 字节3：0x80
 * 
 * 但outl指令是原子的，一次性传输所有4字节。
 */
void Port_32Bit::Write(uint32_t data)
{
    asm volatile("outl %0, %1"
                 :
                 : "a"(data), "Nd"(portnumber));
}

/**
 * 【核心函数】从32位端口读取数据
 * 
 * @return 读取的32位值
 * 
 * 【底层原理】
 * 使用 inl 指令（Input Long）
 * 
 * 【典型应用：读取PCI BAR】
 * ```cpp
 * // 读取基址寄存器（Base Address Register）
 * commandPort.Write(addressForBAR0);
 * uint32_t bar = dataPort.Read();  // ← 调用这个函数
 * 
 * // 解析BAR
 * if (bar & 0x01) {
 *     // I/O空间
 *     uint16_t ioBase = bar & ~0x03;
 * } else {
 *     // 内存映射空间
 *     uint32_t memBase = bar & ~0x0F;
 *     bool prefetchable = (bar >> 3) & 0x01;
 * }
 * ```
 */
uint32_t Port_32Bit::Read()
{
    uint32_t result;
    asm volatile("inl %1, %0"
                 : "=a"(result)
                 : "Nd"(portnumber));
    return result;
}
