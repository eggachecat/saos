# SAOS 操作系统 - 完整中文文档

## 🎉 欢迎！

这是一个基于x86架构的教学型操作系统，采用C++编写，展示了现代操作系统的核心概念和设计模式。

## 📚 文档导航

### 🌟 必读文档（按顺序阅读）

1. **本文件 (README_CN.md)** ← 你在这里
   - 快速开始和项目概览

2. **[ARCHITECTURE.md](ARCHITECTURE.md)** ⭐⭐⭐ 最重要！
   - 系统完整架构
   - 5种设计模式详解（观察者、策略、单例、外观、模板方法）
   - 主循环和事件驱动机制
   - 完整的数据流图（键盘、鼠标、VGA）
   - 启动流程详解（从BIOS到内核）
   - 所有魔数和位操作的详细解释

3. **[NOTES.md](NOTES.md)**
   - 核心技术概念
   - GDT（全局描述符表）
   - IDT（中断描述符表）
   - PCI总线控制
   - 端口通信机制

4. **[代码注释指南.md](代码注释指南.md)**
   - 如何阅读代码
   - 学习路径建议
   - 快速参考和常见问题

---

## 🚀 快速开始

### 构建系统

```bash
# 构建内核
make

# 运行（需要QEMU）
make run

# 清理
make clean
```

### 运行要求
- GCC交叉编译器（i686-elf-gcc）
- GNU Make
- QEMU虚拟机（qemu-system-i386）
- GRUB引导加载器

---

## 🏗️ 系统架构概览

### 核心设计：事件驱动架构

```
┌─────────────────┐
│   硬件设备      │
│  (键盘/鼠标)    │
└────────┬────────┘
         │ 产生中断
         ↓
┌─────────────────┐
│  中断管理器     │ ← 观察者模式的"主题"
│  (IDT + PIC)    │
└────────┬────────┘
         │ 分发事件
         ↓
┌─────────────────┐
│   设备驱动      │ ← 观察者模式的"观察者"
│ (Keyboard/Mouse)│ ← 策略模式的"具体策略"
└────────┬────────┘
         │ 回调通知
         ↓
┌─────────────────┐
│  事件处理器     │ ← 应用层业务逻辑
│ (Event Handlers)│
└────────┬────────┘
         │ 调用输出
         ↓
┌─────────────────┐
│   输出接口      │
│ (printf/VGA)    │
└─────────────────┘
```

### "大循环"的本质

SAOS **没有传统的轮询循环**，而是采用**中断驱动**：

```cpp
// kernelMain() 的最后
while(1);  // 主循环：等待中断
```

**为什么只是空循环？**

系统通过**硬件中断**驱动所有事件：
1. 用户按键 → 键盘中断(IRQ1) → 处理按键
2. 鼠标移动 → 鼠标中断(IRQ12) → 更新光标
3. 定时器滴答 → 定时器中断(IRQ0) → 时间管理

**优势：**
- ⚡ CPU大部分时间处于低功耗等待状态
- 🎯 即时响应（中断延迟<1微秒）
- 🔄 自然的事件驱动模型

---

## 🎨 核心设计模式

### 1. 观察者模式 - 中断系统

**问题：** 如何将硬件中断与处理逻辑解耦？

**解决方案：**
```cpp
// 主题：中断管理器
class InterruptManager {
    InterruptHandler* handlers[256];  // 观察者列表
    
    uint32_t DoHandleInterrupt(uint8_t num, uint32_t esp) {
        if (handlers[num] != 0)
            return handlers[num]->HandleInterrupt(esp);  // 通知
    }
};

// 观察者：键盘驱动
class KeyboardDriver : public InterruptHandler {
    uint32_t HandleInterrupt(uint32_t esp) override {
        uint8_t key = dataport.Read();  // 处理按键
        handler->OnKeyDown(translateKey(key));
        return esp;
    }
};
```

**在大循环中的作用：**
1. 硬件产生中断 → CPU查IDT表
2. InterruptManager收到通知
3. 查找对应的观察者：`handlers[0x21]` → KeyboardDriver
4. 调用观察者的处理方法
5. 返回主循环继续等待

### 2. 策略模式 - 驱动框架

**问题：** 如何统一管理不同类型的驱动？

**解决方案：**
```cpp
// 策略接口
class Driver {
    virtual void Activate() = 0;
    virtual void Deactivate() = 0;
};

// 具体策略
class KeyboardDriver : public Driver { ... };
class MouseDriver : public Driver { ... };

// 上下文：统一管理
class DriverManager {
    Driver* drivers[255];
    
    void ActivateAll() {
        for (int i = 0; i < numDrivers; i++)
            drivers[i]->Activate();  // 多态调用
    }
};
```

**在大循环中的作用：**
- 初始化阶段：`drvManager.ActivateAll()` 激活所有驱动
- 运行阶段：驱动在中断时被动调用
- 无需知道具体驱动类型，统一接口管理

### 3. 单例模式 - 全局中断管理器

**问题：** 汇编中断桩需要访问中断管理器实例

**解决方案：**
```cpp
class InterruptManager {
    static InterruptManager* ActiveInterruptManager;  // 单例
    
    void Activate() {
        ActiveInterruptManager = this;  // 设置全局实例
        asm("sti");  // 启用中断
    }
    
    static uint32_t HandleInterrupt(...) {
        return ActiveInterruptManager->DoHandleInterrupt(...);
    }
};
```

**为什么需要？**
- 汇编代码（interruptstubs.s）调用静态函数
- 静态函数通过单例指针找到实例
- 保证整个系统只有一个活动的中断管理器

---

## 🔄 完整的事件流示例：键盘输入

让我们跟踪一次完整的按键事件（用户按下 'A' 键）：

```
┌────────────────────────────────────────────────────────────┐
│ 第1步：硬件层                                              │
├────────────────────────────────────────────────────────────┤
│ • 用户按下 'A' 键                                          │
│ • 键盘控制器(8042)生成扫描码: 0x1E                        │
│ • 触发 IRQ1 硬件中断信号                                   │
└────────────────────────────────────────────────────────────┘
                         ↓
┌────────────────────────────────────────────────────────────┐
│ 第2步：中断控制器 (PIC)                                   │
├────────────────────────────────────────────────────────────┤
│ • PIC接收 IRQ1 信号                                        │
│ • 转换为中断号: 0x21 (IRQ1 + 0x20偏移)                    │
│ • 向CPU发送中断请求                                        │
└────────────────────────────────────────────────────────────┘
                         ↓
┌────────────────────────────────────────────────────────────┐
│ 第3步：CPU自动处理                                         │
├────────────────────────────────────────────────────────────┤
│ • 保存当前状态（标志寄存器）                               │
│ • 禁用中断（防止嵌套）                                     │
│ • 查找 IDT[0x21] 获取处理函数地址                          │
│ • 跳转到 HandleInterruptRequest0x01 (汇编桩)               │
└────────────────────────────────────────────────────────────┘
                         ↓
┌────────────────────────────────────────────────────────────┐
│ 第4步：汇编中断桩 (interruptstubs.s)                      │
├────────────────────────────────────────────────────────────┤
│ HandleInterruptRequest0x01:                                │
│   movb $0x21, (interruptnumber)  # 保存中断号             │
│   pusha                          # 保存所有通用寄存器      │
│   push %ds, %es, %fs, %gs        # 保存段寄存器           │
│   push %esp                      # 栈指针                  │
│   push (interruptnumber)         # 中断号                  │
│   call InterruptManager::HandleInterrupt                   │
└────────────────────────────────────────────────────────────┘
                         ↓
┌────────────────────────────────────────────────────────────┐
│ 第5步：中断管理器 (InterruptManager)                      │
├────────────────────────────────────────────────────────────┤
│ uint32_t HandleInterrupt(0x21, esp) {                      │
│     return ActiveInterruptManager                          │
│         ->DoHandleInterrupt(0x21, esp);                    │
│ }                                                           │
│                                                             │
│ uint32_t DoHandleInterrupt(0x21, esp) {                    │
│     // 查找观察者                                          │
│     handler = handlers[0x21];  // KeyboardDriver           │
│     esp = handler->HandleInterrupt(esp);                   │
│     // 发送EOI（结束中断）给PIC                            │
│     picMasterCommand.Write(0x20);                          │
│     return esp;                                            │
│ }                                                           │
└────────────────────────────────────────────────────────────┘
                         ↓
┌────────────────────────────────────────────────────────────┐
│ 第6步：键盘驱动 (KeyboardDriver)                          │
├────────────────────────────────────────────────────────────┤
│ uint32_t HandleInterrupt(esp) {                            │
│     uint8_t key = dataport.Read();  // 读端口0x60         │
│     // key = 0x1E                                          │
│                                                             │
│     switch(key) {                                          │
│         case 0x1E:  // 'A'键                               │
│             handler->OnKeyDown('a');                       │
│             break;                                         │
│     }                                                       │
│     return esp;                                            │
│ }                                                           │
└────────────────────────────────────────────────────────────┘
                         ↓
┌────────────────────────────────────────────────────────────┐
│ 第7步：事件处理器 (PrintKeyboardEventHandler)             │
├────────────────────────────────────────────────────────────┤
│ void OnKeyDown('a') {                                      │
│     printf("Key pressed: ");                               │
│     printf("a");                                           │
│     printf("\n");                                          │
│ }                                                           │
└────────────────────────────────────────────────────────────┘
                         ↓
┌────────────────────────────────────────────────────────────┐
│ 第8步：VGA输出 (printf)                                    │
├────────────────────────────────────────────────────────────┤
│ void printf(const char* str) {                             │
│     uint16_t* VideoMemory = 0xB8000;                       │
│     // 写入"Key pressed: a\n"到显存                        │
│     VideoMemory[80*y+x] = 0x0F00 | 'K';  // 白色'K'       │
│     VideoMemory[80*y+x+1] = 0x0F00 | 'e';                  │
│     // ... 继续写入其他字符                                │
│ }                                                           │
└────────────────────────────────────────────────────────────┘
                         ↓
┌────────────────────────────────────────────────────────────┐
│ 第9步：返回路径                                            │
├────────────────────────────────────────────────────────────┤
│ • 返回到汇编桩                                             │
│ • 恢复所有寄存器 (popa, pop段寄存器)                      │
│ • iret 指令返回到中断前的代码                              │
│ • CPU自动恢复标志寄存器和重新启用中断                      │
└────────────────────────────────────────────────────────────┘
                         ↓
┌────────────────────────────────────────────────────────────┐
│ 第10步：回到主循环                                         │
├────────────────────────────────────────────────────────────┤
│ while(1);  ← 继续等待下一个中断                            │
└────────────────────────────────────────────────────────────┘
```

**关键数值：**
- 扫描码：`0x1E`（'A'键的Make Code）
- 中断号：`0x21`（IRQ1 + 0x20）
- 端口地址：`0x60`（键盘数据端口）
- VGA地址：`0xB8000`（文本模式显存）
- 颜色属性：`0x0F00`（白色前景，黑色背景）

**时间消耗：**
- 整个流程 < 10微秒（现代CPU）
- 从按键到显示，用户感觉是即时的

---

## 📁 代码结构

### 目录组织
```
saos_2025/
├── src/                    # 源代码
│   ├── kernel.cpp          # ⭐ 主程序入口
│   ├── loader.s            # 汇编引导代码
│   ├── gdt.cpp             # 全局描述符表
│   ├── hardwares/          # 硬件抽象层
│   │   ├── port.cpp        # ⭐ 端口I/O (已添加详细注释)
│   │   ├── interrupts.cpp  # 中断管理
│   │   ├── interruptstubs.s # 中断汇编桩
│   │   └── pci.cpp         # PCI总线控制
│   └── drivers/            # 设备驱动
│       ├── driver.cpp      # 驱动框架
│       ├── keyboard.cpp    # 键盘驱动
│       ├── mouse.cpp       # 鼠标驱动
│       └── vga.cpp         # 图形驱动
├── include/                # 头文件
│   ├── common/types.h      # ⭐ 基础类型 (已添加详细注释)
│   ├── gdt.h
│   ├── hardwares/
│   └── drivers/
├── ARCHITECTURE.md         # ⭐⭐⭐ 最重要的文档！
├── NOTES.md                # 核心概念详解
├── 代码注释指南.md          # 学习路径
└── README_CN.md            # 本文件
```

### 模块依赖关系
```
应用层: kernel.cpp (main函数, 事件处理器)
         ↓
驱动层: drivers/ (KeyboardDriver, MouseDriver, VGA)
         ↓
抽象层: hardwares/ (Port, InterruptManager, PCI)
         ↓
系统层: gdt.cpp (内存管理)
         ↓
引导层: loader.s (汇编入口)
```

---

## 🔑 关键技术点

### 1. 内存布局

```
物理内存映射:
0x00000000 - 0x000003FF: 实模式中断向量表 (IVT)
0x00000400 - 0x000004FF: BIOS数据区
0x00000500 - 0x00007BFF: 可用空间
0x00007C00 - 0x00007DFF: 引导扇区加载位置
0x00007E00 - 0x0007FFFF: 可用空间
0x00080000 - 0x0009FFFF: 扩展BIOS数据区
0x000A0000 - 0x000BFFFF: 显存（VGA图形）
0x000B8000 - 0x000BFFFF: VGA文本显存 ← 我们用这个
0x000C0000 - 0x000FFFFF: BIOS ROM
0x00100000 - ...       : 扩展内存（内核加载位置）
```

### 2. 段描述符格式（GDT）

```
64位段描述符结构:
┌─────────┬─────────┬─────────┬─────────┐
│ Base[31:24] │G│D│L│AVL│Limit│P│DPL│S│Type│Base[23:16]│
├─────────┴─────────┴─────────┴─────────┤
│         Base[15:0]        │     Limit[15:0]      │
└─────────┴─────────┴─────────┴─────────┘

标志位解释:
G (Granularity): 1=4KB页, 0=字节
D (Default): 1=32位, 0=16位
L (Long): 1=64位代码段
AVL (Available): 系统软件可用
P (Present): 1=段存在
DPL (Privilege): 0-3特权级
S (System): 1=代码/数据, 0=系统
Type: 段类型（可执行、可写等）
```

### 3. IDT门描述符格式

```
64位门描述符:
┌──────────────┬────┬───┬─────┬──────────────┐
│ Offset[31:16]│ P  │DPL│ 0D110│   Reserved   │
├──────────────┴────┴───┴─────┴──────────────┤
│   Segment Selector    │    Offset[15:0]    │
└──────────────┴────┴───┴─────┴──────────────┘

例子（键盘中断）:
Offset: HandleInterruptRequest0x01 的地址
Selector: 代码段选择子 (0x08)
P=1, DPL=00, Type=0xE (32位中断门)
```

### 4. 中断向量表

```
CPU异常 (0x00-0x1F):
0x00: 除零错误
0x01: 调试异常
0x0D: 一般保护错误
0x0E: 页面错误
...

硬件中断 (重新映射后):
0x20 (IRQ0):  定时器
0x21 (IRQ1):  键盘 ← 我们使用
0x22 (IRQ2):  级联（从PIC）
0x2C (IRQ12): 鼠标 ← 我们使用
...
```

### 5. PCI配置空间格式

```
PCI配置地址 (写入0xCF8):
┌──┬──────┬────────┬─────────┬────────┬──────────┬──┐
│31│30-24 │ 23-16  │  15-11  │  10-8  │   7-2    │1-0│
├──┼──────┼────────┼─────────┼────────┼──────────┼──┤
│En│ Res  │  Bus   │ Device  │  Func  │ Register │00│
└──┴──────┴────────┴─────────┴────────┴──────────┴──┘

示例：读取总线0，设备1，功能0的厂商ID
address = 0x80000800
          = 1<<31 | 0<<16 | 1<<11 | 0<<8 | 0x00
commandPort.Write(0x80000800);
vendor_id = dataPort.Read() & 0xFFFF;
```

---

## 💡 学习建议

### 第一天：理解架构（2-3小时）
1. ✅ 阅读本文件（README_CN.md）
2. ✅ 阅读 ARCHITECTURE.md 的前3章
3. ✅ 运行系统，观察行为
4. ✅ 尝试按键，观察输出

### 第二天：深入设计模式（3-4小时）
1. ✅ 阅读 ARCHITECTURE.md 的设计模式部分
2. ✅ 查看观察者模式的代码实现：
   - `src/hardwares/interrupts.cpp`
   - `src/drivers/keyboard.cpp`
3. ✅ 理解策略模式：
   - `include/drivers/driver.h`
   - `src/drivers/driver.cpp`

### 第三天：跟踪事件流（4-5小时）
1. ✅ 阅读 ARCHITECTURE.md 的"键盘输入数据流"
2. ✅ 添加调试输出，跟踪完整流程：
```cpp
// 在 KeyboardDriver::HandleInterrupt 添加：
printf("Scancode: 0x");
printfHex(key);
printf("\n");
```
3. ✅ 观察按键到显示的整个路径

### 第四天：硬件抽象层（3-4小时）
1. ✅ 阅读 `src/hardwares/port.cpp`（已有详细注释）
2. ✅ 理解 outb/inb 指令
3. ✅ 实验：修改端口读写，观察效果

### 第五天：添加新功能（4-6小时）
选择一个小项目：
- 🔨 添加更多键盘扫描码支持
- 🔨 改进鼠标光标显示
- 🔨 添加简单的图形绘制功能
- 🔨 实现定时器驱动

---

## 🔧 调试技巧

### 1. 串口调试
```cpp
// 输出到串口（在终端显示）
serial_printf("Debug: value = ");
printfHex(value);
serial_printf("\n");
```

### 2. QEMU调试参数
```bash
# 显示串口输出到终端
qemu-system-i386 -kernel kernel.bin -serial stdio

# 启用GDB调试
qemu-system-i386 -kernel kernel.bin -s -S
# 然后在另一个终端: gdb kernel.bin
# (gdb) target remote localhost:1234
# (gdb) break kernelMain
# (gdb) continue
```

### 3. 屏幕调试输出
```cpp
// 在关键位置添加
printf("checkpoint 1\n");
```

### 4. 寄存器查看
```cpp
// 在中断处理中打印寄存器
printf("ESP: 0x");
printfHex32(esp);
printf("\n");
```

---

## 🐛 常见问题

### Q: 键盘没有反应？
**A:** 检查：
1. IDT是否正确初始化？
2. `interrupts.Activate()` 是否被调用？
3. PIC是否正确重新映射？（IRQ1应该映射到0x21）
4. 键盘驱动是否正确注册？（`handlers[0x21]`）

调试方法：
```cpp
// 在 InterruptManager::DoHandleInterrupt 添加：
printf("Interrupt ");
printfHex(interruptNumber);
printf("\n");
```

### Q: 鼠标不工作？
**A:** 检查：
1. 从PIC是否启用？（IRQ12 → 0x2C）
2. 鼠标是否被激活？（0xA8命令）
3. 鼠标数据是否正确收集？（3字节数据包）

### Q: 屏幕显示乱码？
**A:** 可能原因：
1. VGA显存地址错误（应该是0xB8000）
2. 颜色属性错误
3. 字符串没有空终止符

### Q: 系统启动后立即重启？
**A:** 检查：
1. GDT是否正确设置？
2. IDT是否正确加载？
3. 栈是否正确初始化？（loader.s中的栈设置）
4. 是否有未处理的异常？

---

## 📚 扩展阅读

### 推荐资源
1. **[OSDev Wiki](https://wiki.osdev.org/)** - 最权威的操作系统开发资源
2. **Intel® 64 and IA-32 Architectures Software Developer Manuals**
   - 卷3A: 系统编程指南
3. **PCI Local Bus Specification** - PCI规范
4. **VGA文档** - https://wiki.osdev.org/VGA_Hardware

### 进阶主题
- 🔹 内存管理：分页、虚拟内存
- 🔹 进程调度：多任务、上下文切换
- 🔹 文件系统：FAT32、ext2
- 🔹 网络协议栈：TCP/IP
- 🔹 图形界面：GUI框架

---

## 🎓 总结

### 核心收获
1. ✅ 理解事件驱动架构
2. ✅ 掌握5种设计模式的实战应用
3. ✅ 了解x86保护模式编程
4. ✅ 学会硬件抽象和驱动开发
5. ✅ 体验完整的系统开发流程

### 设计亮点
- 🌟 清晰的分层架构
- 🌟 优雅的设计模式应用
- 🌟 高效的中断驱动机制
- 🌟 良好的可扩展性

### 下一步
1. 添加新的设备驱动（如网卡、硬盘）
2. 实现内存管理（分页）
3. 添加多任务支持
4. 实现简单的文件系统
5. 开发图形界面

---

## 📞 文档版本

- **版本**: 1.0
- **最后更新**: 2025年10月
- **作者**: SAOS开发团队
- **语言**: 简体中文

---

## 🙏 致谢

感谢以下资源：
- OSDev社区的详细文档
- Intel的官方手册
- GCC和QEMU开发团队

---

**🎉 开始你的操作系统开发之旅吧！**

记住：
1. 📖 先读 `ARCHITECTURE.md`（最重要！）
2. 💻 然后看代码
3. 🔍 遇到问题查 `代码注释指南.md`
4. 🤔 深入研究时参考 `NOTES.md`

**Happy Coding! 🚀**

