# SAOS 操作系统架构详解

## 📋 目录
- [系统概览](#系统概览)
- [设计模式](#设计模式)
- [主循环和事件流](#主循环和事件流)
  - [🎡 系统的"巨大循环"](#系统的巨大循环---the-grand-loop)
  - [🌀 深入理解：主循环的哲学](#深入理解主循环的哲学)
- [进程/任务调度：从单任务到多任务](#进程任务调度从单任务到多任务)
- [模块交互图](#模块交互图)
- [启动流程](#启动流程)
- [数据流详解](#数据流详解)

---

## 🎯 系统概览

**SAOS** 是一个基于x86架构的教学型操作系统，采用事件驱动架构，支持基本的硬件抽象和中断处理。

### 核心功能
- ✅ 保护模式内存管理（GDT）
- ✅ 硬件中断处理（IDT + PIC）
- ✅ 设备驱动框架
- ✅ PCI总线设备发现
- ✅ VGA图形显示
- ✅ 键盘/鼠标输入

---

## 🎨 设计模式

### 1. **观察者模式 (Observer Pattern)** - 事件处理系统

**位置：** `InterruptHandler` 和各种 `EventHandler`

**作用：** 将硬件中断（主题）与具体的事件处理器（观察者）解耦

```cpp
// 主题：中断管理器
class InterruptManager {
    InterruptHandler* handlers[256];  // 观察者列表
    
    uint32_t DoHandleInterrupt(uint8_t interruptNumber, uint32_t esp) {
        if (handlers[interruptNumber] != 0) {
            return handlers[interruptNumber]->HandleInterrupt(esp);  // 通知观察者
        }
    }
};

// 抽象观察者
class InterruptHandler {
    virtual uint32_t HandleInterrupt(uint32_t esp);
};

// 具体观察者
class KeyboardDriver : public InterruptHandler {
    uint32_t HandleInterrupt(uint32_t esp) override {
        // 处理键盘中断
    }
};
```

**具体例子：**
- 当用户按下键盘按键时：
  1. 硬件产生IRQ1中断（中断号0x21）
  2. CPU查找IDT，跳转到 `HandleInterruptRequest0x01`
  3. `InterruptManager::DoHandleInterrupt(0x21, esp)` 被调用
  4. 查找 `handlers[0x21]`，找到 `KeyboardDriver` 实例
  5. 调用 `KeyboardDriver::HandleInterrupt()`
  6. 读取扫描码，解析后调用 `KeyboardEventHandler::OnKeyDown('a')`

---

### 2. **策略模式 (Strategy Pattern)** - 驱动框架

**位置：** `Driver` 基类和具体驱动实现

**作用：** 定义统一的驱动接口，允许运行时切换不同的驱动实现

```cpp
// 策略接口
class Driver {
    virtual void Activate();
    virtual int Reset();
    virtual void Deactivate();
};

// 具体策略
class KeyboardDriver : public Driver {
    void Activate() override {
        // 键盘特定的激活逻辑
    }
};

class MouseDriver : public Driver {
    void Activate() override {
        // 鼠标特定的激活逻辑
    }
};

// 上下文：统一管理所有驱动
class DriverManager {
    Driver* drivers[255];
    void ActivateAll() {
        for (int i = 0; i < numDrivers; i++)
            drivers[i]->Activate();  // 多态调用
    }
};
```

**具体例子：**
```cpp
// kernelMain中：
DriverManager drvManager;
drvManager.AddDriver(&keyboard);  // 添加键盘驱动策略
drvManager.AddDriver(&mouse);     // 添加鼠标驱动策略
drvManager.ActivateAll();         // 统一激活，无需知道具体类型
```

---

### 3. **单例模式 (Singleton Pattern)** - 中断管理器

**位置：** `InterruptManager::ActiveInterruptManager`

**作用：** 确保整个系统只有一个活动的中断管理器

```cpp
class InterruptManager {
    static InterruptManager* ActiveInterruptManager;  // 单例指针
    
    void Activate() {
        if (ActiveInterruptManager != 0)
            ActiveInterruptManager->Deactivate();  // 停用旧的
        ActiveInterruptManager = this;              // 设置新的
        asm("sti");  // 启用中断
    }
    
    static uint32_t HandleInterrupt(uint8_t num, uint32_t esp) {
        if (ActiveInterruptManager != 0)
            return ActiveInterruptManager->DoHandleInterrupt(num, esp);
    }
};
```

**为什么需要单例？**
- 硬件中断处理函数是全局静态函数（由汇编代码调用）
- 必须通过全局指针找到当前活动的中断管理器实例
- 保证中断处理的一致性和可预测性

---

### 4. **外观模式 (Facade Pattern)** - PCI控制器

**位置：** `PeripheralComponentInterconnectController`

**作用：** 为复杂的PCI总线操作提供简化接口

```cpp
class PeripheralComponentInterconnectController {
    Port_32Bit dataPort;     // 底层端口操作
    Port_32Bit commandPort;
    
public:
    // 简化的高层接口
    uint32_t Read(uint16_t bus, uint16_t device, uint16_t function, uint32_t reg);
    PeripheralComponentInterconnectDeviceDescriptor GetDeviceDescriptor(...);
    void SelectDrivers(DriverManager* drvManager, InterruptManager* interrupts);
};
```

**隐藏的复杂性：**
```cpp
// 用户只需调用：
PeripheralComponentInterconnectDeviceDescriptor dev = 
    PCIController.GetDeviceDescriptor(bus, device, function);

// 内部处理：
// 1. 构造PCI配置地址（位操作）
// 2. 写入命令端口（0xCF8）
// 3. 从数据端口读取（0xCFC）
// 4. 提取和解析多个寄存器
// 5. 填充设备描述符结构
```

---

### 5. **模板方法模式 (Template Method Pattern)** - 端口通信

**位置：** `Port` 类层次结构

**作用：** 定义端口操作的通用算法骨架，子类实现具体细节

```cpp
// 抽象类定义算法骨架
class Port {
protected:
    uint16_t portnumber;
public:
    Port(uint16_t portnumber);
};

// 具体实现
class Port_8Bit : public Port {
    virtual void Write(uint8_t data) {
        __asm__ volatile("outb %0, %1" : : "a" (data), "Nd" (portnumber));
    }
};

class Port_8Bit_Slow : public Port_8Bit {
    void Write(uint8_t data) override {
        __asm__ volatile("outb %0, %1\njmp 1f\n1: jmp 1f\n1:" 
                        : : "a" (data), "Nd" (portnumber));
        // 额外的延迟，适用于慢速设备
    }
};
```

---

## 🔄 主循环和事件流

### 🎡 系统的"巨大循环" - The Grand Loop

SAOS的核心是一个**看似空洞却极其强大的无限循环**：

```cpp
// kernel.cpp 第304-305行：系统的心脏
while (1)
    ;
```

**这就是整个操作系统的主循环！** 看起来什么都没做？这正是它的精妙之处。

#### 为什么是空循环？

```
传统轮询架构 (❌ 不推荐):
┌─────────────────────────┐
│ while(1) {              │
│   if (键盘有数据?)      │ ← 浪费CPU周期不断检查
│     处理键盘();         │
│   if (鼠标有数据?)      │ ← 大部分时间都在无意义轮询
│     处理鼠标();         │
│   if (网络有数据?)      │
│     处理网络();         │
│   // ... 检查更多设备   │
│ }                       │
└─────────────────────────┘
效率：约5-10% (大量时间浪费在空检查)

SAOS事件驱动架构 (✅ 推荐):
┌─────────────────────────┐
│ while(1)                │
│     ;  // 什么都不做!   │ ← CPU处于HLT状态(低功耗)
│ }                       │ ← 硬件中断时CPU自动唤醒
└─────────────────────────┘
效率：接近100% (仅在有事件时工作)
```

### 🎭 主循环的真实状态

```
时间轴视图：

T=0ms    ┌──────────────────────────────────┐
         │ kernelMain() 启动流程            │
         ├──────────────────────────────────┤
         │ 1. 初始化 GDT（内存段）         │
         │ 2. 初始化 IDT（中断表）         │
         │ 3. 创建并注册所有驱动           │
         │ 4. 扫描 PCI 设备                │
         │ 5. 激活所有驱动                 │
         │ 6. 启用中断（sti指令）          │
         │ 7. 设置 VGA 图形模式            │
         │ 8. 绘制蓝色背景                 │
         └──────────┬───────────────────────┘
                    │
T=100ms             ↓
         ╔═══════════════════════════════════╗
         ║  进入主循环: while(1);            ║
         ║  CPU状态: 空转 (实际可用HLT指令)  ║
         ║  功耗: 极低                       ║
         ║  等待: 硬件中断                   ║
         ╚═══════════════════════════════════╝
                    │
                    │ [系统看似"冻结"，实则待命]
                    │
T=150ms             │ ⚡ 用户按下键盘!
                    ↓
         ┌────────────────────────────────┐
         │ 硬件中断触发 (IRQ1)            │
         │ CPU自动：                      │
         │  - 暂停while循环               │
         │  - 保存寄存器状态              │
         │  - 查找IDT[0x21]               │
         │  - 跳转到中断处理函数          │
         └──────────┬─────────────────────┘
                    ↓
         ┌────────────────────────────────┐
         │ KeyboardDriver::HandleInterrupt│
         │  - 读取扫描码                  │
         │  - 识别字符 'A'                │
         │  - 调用事件处理器              │
         │  - printf("Key pressed: A")    │
         └──────────┬─────────────────────┘
                    │
T=151ms             ↓
         ╔═══════════════════════════════════╗
         ║  返回主循环: while(1);            ║
         ║  iret指令恢复之前的状态           ║
         ║  继续"空转"等待下一个中断         ║
         ╚═══════════════════════════════════╝
                    │
T=200ms             │ ⚡ 鼠标移动!
                    ↓
         ┌────────────────────────────────┐
         │ 硬件中断触发 (IRQ12)           │
         │ MouseDriver::HandleInterrupt   │
         │  - 读取3字节数据包             │
         │  - 解析移动偏移                │
         │  - 更新屏幕光标                │
         └──────────┬─────────────────────┘
                    ↓
T=201ms  ╔═══════════════════════════════════╗
         ║  返回主循环: while(1);            ║
         ║  永不退出，永不停止               ║
         ╚═══════════════════════════════════╝
                    │
                    └→ [循环继续，直到关机...]
```

### 🔍 主循环的底层实现

```asm
; while(1); 编译后的汇编代码：

_main_loop:
    jmp _main_loop    ; 无条件跳转到自己
                      ; 每条指令约1-2个CPU周期
                      ; 每秒执行数亿次！

; 优化版本（如果使用HLT指令）：
_main_loop_optimized:
    hlt               ; 暂停CPU直到中断
    jmp _main_loop_optimized
                      ; 功耗降低90%+
```

### 🎯 主循环的关键特性

| 特性 | 说明 | 影响 |
|------|------|------|
| **永不退出** | `while(1)`保证系统持续运行 | 操作系统永不"完成" |
| **可中断** | `sti`指令启用中断标志位 | 硬件可随时"打断" |
| **低优先级** | 主循环是最低优先级任务 | 任何中断都能抢占 |
| **无状态** | 循环本身不维护状态 | 所有状态在中断处理器中 |
| **零开销** | 空循环无实际操作 | CPU可进入节能模式 |

### 系统的"大循环"概览

```
┌─────────────────────────────────────┐
│      kernelMain() 启动流程          │
├─────────────────────────────────────┤
│ 1. 初始化 GDT（内存段）             │
│ 2. 初始化 IDT（中断表）             │
│ 3. 创建并注册所有驱动               │
│ 4. 扫描 PCI 设备                    │
│ 5. 激活所有驱动                     │
│ 6. 启用中断（sti指令）              │
│ 7. 设置 VGA 图形模式                │
│ 8. 进入无限循环 while(1);           │
└─────────────────────────────────────┘
                ↓
        系统进入"等待态"
                ↓
    ┌───────────────────────┐
    │   硬件产生中断信号     │
    └───────────────────────┘
                ↓
```

### 事件驱动流程图

```
[硬件事件] → [CPU中断] → [IDT查表] → [汇编Stub] → [InterruptManager]
                                                          ↓
                                                    [查找Handler]
                                                          ↓
                                              ┌───────────┴───────────┐
                                              ↓                       ↓
                                      [KeyboardDriver]        [MouseDriver]
                                              ↓                       ↓
                                        [解析扫描码]            [解析数据包]
                                              ↓                       ↓
                                    [KeyboardEventHandler]   [MouseEventHandler]
                                              ↓                       ↓
                                        [OnKeyDown]            [onMouseMove]
                                              ↓                       ↓
                                         [printf]               [更新屏幕]
```

### 具体事件流示例：键盘输入

```
步骤 1: 用户按下 'A' 键
   ↓
步骤 2: 键盘控制器产生 IRQ1 硬件中断
   ↓
步骤 3: PIC (可编程中断控制器) 将 IRQ1 转换为中断号 0x21
   ↓
步骤 4: CPU 自动：
   - 保存当前状态（标志寄存器）
   - 查找 IDT[0x21]
   - 跳转到 HandleInterruptRequest0x01（汇编代码）
   ↓
步骤 5: interruptstubs.s 中的汇编代码：
   - 保存所有寄存器（pusha, push段寄存器）
   - 调用 InterruptManager::HandleInterrupt(0x21, esp)
   ↓
步骤 6: InterruptManager::DoHandleInterrupt()
   - 查找 handlers[0x21] → 找到 KeyboardDriver 实例
   - 调用 keyboardDriver->HandleInterrupt(esp)
   ↓
步骤 7: KeyboardDriver::HandleInterrupt()
   - 读取端口 0x60，获取扫描码（例如：0x1E）
   - 识别为 'A' 键
   - 调用 handler->OnKeyDown('a')
   ↓
步骤 8: PrintKeyboardEventHandler::OnKeyDown('a')
   - 调用 printf("Key pressed: a")
   - 在 VGA 文本缓冲区写入字符
   ↓
步骤 9: 返回路径：
   - HandleInterrupt 返回 esp
   - DoHandleInterrupt 发送 EOI 到 PIC（0x20命令）
   - 汇编代码恢复所有寄存器（popa, pop段寄存器）
   - iret 指令返回到中断前的代码
   ↓
步骤 10: CPU 继续执行 while(1) 循环，等待下一个中断
```

---

## 🗺️ 模块交互图

```
┌──────────────────────────────────────────────────────────────────────┐
│                           kernelMain (主程序)                         │
│  - 系统初始化协调者                                                   │
│  - 创建所有核心对象                                                   │
│  - 进入无限等待循环                                                   │
└────────────┬─────────────────────────────────────────────────────────┘
             │ 创建并配置
    ┌────────┼────────┬────────────┬──────────────┐
    ↓        ↓        ↓            ↓              ↓
┌────────┐ ┌─────────────┐ ┌──────────────┐ ┌──────────┐
│  GDT   │ │   IDT/PIC   │ │DriverManager │ │   PCI    │
│内存段表│ │  中断系统   │ │  驱动管理器  │ │ 设备总线 │
└────────┘ └──────┬──────┘ └───────┬──────┘ └─────┬────┘
                  │                │               │
                  │ 注册Handler    │ 添加驱动      │
                  ↓                ↓               ↓
            ┌─────────────────────────────────────────┐
            │         硬件驱动层（Driver）            │
            ├──────────┬──────────┬──────────────────┤
            │ Keyboard │  Mouse   │       VGA        │
            │ Driver   │ Driver   │     Driver       │
            └─────┬────┴────┬─────┴──────────────────┘
                  │         │
                  │ 通知事件 │
                  ↓         ↓
            ┌────────────────────┐
            │   Event Handlers   │
            │  (应用层回调)      │
            ├────────────────────┤
            │ KeyboardEvent-     │
            │   Handler          │
            │ MouseEvent-        │
            │   Handler          │
            └─────────┬──────────┘
                      │
                      │ 调用输出
                      ↓
            ┌────────────────────┐
            │  输出接口          │
            ├────────────────────┤
            │ printf (VGA)       │
            │ serial_printf      │
            │ VGA PutPixel       │
            └────────────────────┘
```

### 各模块职责

| 模块 | 职责 | 与大循环的交互 |
|------|------|----------------|
| **GDT** | 定义内存段，初始化后保持静态 | 初始化阶段配置，之后无交互 |
| **InterruptManager** | 管理IDT表，分发中断到Handler | 每次中断时作为中央调度器 |
| **DriverManager** | 统一管理所有驱动的生命周期 | 初始化时批量激活驱动 |
| **PCI Controller** | 扫描PCI总线，发现设备 | 初始化时一次性扫描 |
| **KeyboardDriver** | 处理键盘中断，解析扫描码 | 每次键盘中断时被调用 |
| **MouseDriver** | 处理鼠标中断，解析移动/按键 | 每次鼠标中断时被调用 |
| **VGA Driver** | 提供图形绘制接口 | 主动调用绘制像素 |
| **Event Handlers** | 应用层的事件响应逻辑 | 被驱动回调时执行 |

---

## 🚀 启动流程

### 完整的系统启动序列

```
1. BIOS/UEFI 阶段
   └─> 硬件自检 (POST)
   └─> 加载引导加载器 (GRUB)

2. GRUB 引导阶段
   └─> 加载内核到内存 (loader.s)
   └─> 传递多重引导信息
   └─> 跳转到 loader 入口

3. loader.s (汇编入口)
   ├─> 设置内核栈 (2MB)
   │   mov $kernel_stack, %esp
   ├─> 调用全局构造函数
   │   call callConstructors
   └─> 跳转到 C++ 主函数
       call kernelMain

4. callConstructors() 函数
   └─> 遍历 .init_array 段
   └─> 调用所有全局对象的构造函数

5. kernelMain() 主函数
   ├─> 打印欢迎信息
   │   printf("Hello world!")
   │
   ├─> 初始化 GDT
   │   GlobalDescriptorTable gdt;
   │   ├─> 创建4个段描述符（null, unused, code, data）
   │   ├─> 使用 lgdt 指令加载到CPU
   │   └─> 完成：内存段管理就绪
   │
   ├─> 初始化中断系统
   │   InterruptManager interrupts(&gdt);
   │   ├─> 创建256个IDT表项
   │   ├─> 设置关键中断处理函数
   │   │   - 0x20: 定时器
   │   │   - 0x21: 键盘
   │   │   - 0x2C: 鼠标
   │   ├─> 重新映射PIC（避免与CPU异常冲突）
   │   │   主PIC: IRQ0-7  → 0x20-0x27
   │   │   从PIC: IRQ8-15 → 0x28-0x2F
   │   └─> 使用 lidt 指令加载IDT（但尚未启用中断）
   │
   ├─> 创建驱动管理器
   │   DriverManager drvManager;
   │
   ├─> 初始化键盘驱动
   │   PrintKeyboardEventHandler kbhandler;
   │   KeyboardDriver keyboard(&interrupts, &kbhandler);
   │   ├─> 注册到 InterruptManager (handlers[0x21] = &keyboard)
   │   └─> 添加到 DriverManager
   │
   ├─> 初始化鼠标驱动
   │   MouseToConsole mousehandler;
   │   MouseDriver mouse(&interrupts, &mousehandler);
   │   ├─> 注册到 InterruptManager (handlers[0x2C] = &mouse)
   │   └─> 添加到 DriverManager
   │
   ├─> 扫描PCI设备
   │   PeripheralComponentInterconnectController PCIController;
   │   PCIController.SelectDrivers(&drvManager, &interrupts);
   │   ├─> 遍历所有总线（0-8）
   │   ├─> 遍历所有设备（0-31）
   │   ├─> 读取厂商ID/设备ID
   │   ├─> 识别设备类型（网卡、显卡等）
   │   └─> 自动加载对应驱动（如果有）
   │
   ├─> 初始化VGA驱动
   │   VideoGraphicsArray vga;
   │
   ├─> 激活所有驱动
   │   drvManager.ActivateAll();
   │   ├─> keyboard.Activate()
   │   │   ├─> 清空键盘缓冲区
   │   │   ├─> 启用键盘接口 (0xAE)
   │   │   ├─> 配置控制器状态
   │   │   └─> 发送启用扫描命令 (0xF4)
   │   │
   │   └─> mouse.Activate()
   │       ├─> 激活鼠标中断 (0xA8)
   │       ├─> 读取并修改控制器状态
   │       └─> 发送启用鼠标命令 (0xF4)
   │
   ├─> 启用中断
   │   interrupts.Activate();
   │   ├─> 设置为全局活动中断管理器
   │   └─> asm("sti");  // 启用CPU中断标志位
   │
   ├─> 切换到图形模式
   │   vga.SetMode(320, 200, 8);
   │   ├─> 写入VGA寄存器配置
   │   │   - 杂项寄存器 (0x3C2)
   │   │   - 序列器寄存器 (0x3C4/0x3C5)
   │   │   - CRTC寄存器 (0x3D4/0x3D5)
   │   │   - 图形控制器 (0x3CE/0x3CF)
   │   │   - 属性控制器 (0x3C0/0x3C1)
   │   └─> 显存映射到 0xA0000
   │
   ├─> 绘制背景
   │   for (y: 0->200)
   │       for (x: 0->320)
   │           vga.PutPixel(x, y, 0, 0, 0xA8);  // 蓝色
   │
   └─> 进入主循环
       while(1);  // 系统在这里等待中断
```

---

## 📊 数据流详解

### 1. 键盘输入数据流

```
┌──────────────┐
│  物理按键    │
│   (硬件)     │
└──────┬───────┘
       │ 产生电信号
       ↓
┌─────────────────────┐
│  键盘控制器 (8042)  │
│  - 将按键转换为     │
│    扫描码           │
│  - 存入输出缓冲区   │
└──────┬──────────────┘
       │ 触发 IRQ1
       ↓
┌──────────────────────┐
│ PIC (中断控制器)     │
│ - IRQ1 → 中断号0x21  │
└──────┬───────────────┘
       │ 中断信号
       ↓
┌───────────────────────┐
│  CPU                  │
│  - 查找 IDT[0x21]     │
│  - 跳转处理函数       │
└──────┬────────────────┘
       │
       ↓
┌────────────────────────────┐
│ interruptstubs.s           │
│ HandleInterruptRequest0x01 │
│  - 保存寄存器状态          │
│  - mov $0x21, interrupt_num│
└──────┬─────────────────────┘
       │ call HandleInterrupt
       ↓
┌────────────────────────────┐
│ InterruptManager           │
│ ::HandleInterrupt(0x21)    │
│  - handlers[0x21] 是谁？   │
│  → KeyboardDriver          │
└──────┬─────────────────────┘
       │
       ↓
┌─────────────────────────────┐
│ KeyboardDriver              │
│ ::HandleInterrupt()         │
│  - dataport.Read()  读0x60  │
│  - 获取扫描码：0x1E         │
└──────┬──────────────────────┘
       │ 解析扫描码
       ↓
┌──────────────────────────────┐
│  switch (0x1E)               │
│  case 0x1E: // 'A'键         │
│    handler->OnKeyDown('a');  │
└──────┬───────────────────────┘
       │
       ↓
┌────────────────────────────────┐
│ PrintKeyboardEventHandler      │
│ ::OnKeyDown('a')               │
│  - printf("Key pressed: a")    │
└──────┬─────────────────────────┘
       │
       ↓
┌───────────────────────────────┐
│  printf() 函数                │
│  - VideoMemory[pos] = 'a'     │
│  - 写入VGA文本缓冲区0xB8000   │
└──────┬────────────────────────┘
       │
       ↓
┌──────────────────┐
│  屏幕显示 'a'    │
└──────────────────┘
```

**数据值示例：**
- 物理按键：'A' 键
- 扫描码（Make Code）：`0x1E`
- 中断号：`0x21`（IRQ1 + 0x20偏移）
- 字符值：`'a'` (0x61)
- VGA内存地址：`0xB8000 + (80*y + x)*2`
- VGA数据格式：`[属性字节][字符字节]`

---

### 2. 鼠标移动数据流

```
┌──────────────┐
│  物理鼠标    │
│   移动       │
└──────┬───────┘
       │ 光电/机械传感器
       ↓
┌─────────────────────┐
│ 鼠标控制器 (8042)   │
│ - 生成3字节数据包： │
│   [按键状态]        │
│   [X轴移动]         │
│   [Y轴移动]         │
└──────┬──────────────┘
       │ 触发 IRQ12（每字节一次中断）
       ↓
┌──────────────────────┐
│ PIC 从控制器         │
│ - IRQ12 → 0x2C       │
└──────┬───────────────┘
       │
       ↓
┌────────────────────────────┐
│ MouseDriver                │
│ ::HandleInterrupt()        │
│  - buffer[offset++] = data │
│  - 收集3字节完整数据包     │
└──────┬─────────────────────┘
       │ offset == 0 时数据包完整
       ↓
┌─────────────────────────────┐
│ 解析鼠标数据包              │
│                             │
│ buffer[0]: 按键和状态标志   │
│  ┌─┬─┬─┬─┬─┬─┬─┬─┐         │
│  │Y│X│Y│X│1│M│R│L│         │
│  │O│O│S│S│ │B│B│B│         │
│  └─┴─┴─┴─┴─┴─┴─┴─┘         │
│   7 6 5 4 3 2 1 0 位        │
│                             │
│ buffer[1]: X轴移动量        │
│  - 有符号8位整数            │
│  - 正值：向右               │
│  - 负值：向左               │
│                             │
│ buffer[2]: Y轴移动量        │
│  - 有符号8位整数            │
│  - 正值：向下（硬件坐标）   │
│  - 负值：向上               │
└──────┬──────────────────────┘
       │
       ↓
┌─────────────────────────────┐
│ handler->onMouseMove(       │
│   buffer[1],   // X偏移     │
│   -buffer[2]   // Y反转     │
│ )                           │
└──────┬──────────────────────┘
       │
       ↓
┌─────────────────────────────────┐
│ MouseToConsole::onMouseMove()   │
│  x += x_offset;  // 更新位置    │
│  y += y_offset;                 │
│  // 边界检查                    │
│  if (x < 0) x = 0;              │
│  if (x >= 80) x = 79;           │
│  // 更新屏幕显示（颜色反转）    │
│  VideoMemory[80*y+x] = 反色值;  │
└─────────────────────────────────┘
```

**数据包示例：**
```
假设：鼠标向右移动5像素，向上移动3像素，左键按下

buffer[0] = 0b00001001 = 0x09
  - 位0 (L): 1 = 左键按下
  - 位1 (R): 0 = 右键释放
  - 位2 (M): 0 = 中键释放
  - 位3: 1 (固定)
  - 位4 (XS): 0 = X正数
  - 位5 (YS): 0 = Y正数

buffer[1] = 0x05 (5)
  - X轴：向右移动5单位

buffer[2] = 0xFD (-3的补码)
  - Y轴：向上移动-3单位（硬件坐标）
  - 代码中取反：-(-3) = 3（屏幕坐标向上）
```

---

### 3. VGA图形绘制数据流

```
┌──────────────────────┐
│ 应用代码             │
│ vga.PutPixel(        │
│   x=100, y=50,       │
│   r=0, g=0, b=0xA8   │
│ );                   │
└──────┬───────────────┘
       │
       ↓
┌─────────────────────────────┐
│ GetColorIndex(r, g, b)      │
│  - 将RGB转换为8位调色板索引 │
│  - (0, 0, 0xA8) → 0x01      │
└──────┬──────────────────────┘
       │
       ↓
┌──────────────────────────────┐
│ GetFrameBufferSegment()      │
│  - 读取图形控制器寄存器0x06  │
│  - 确定显存基址：0xA0000     │
└──────┬───────────────────────┘
       │
       ↓
┌───────────────────────────────┐
│ 计算像素地址                  │
│ pixelAddr = 0xA0000           │
│           + 320 * y           │
│           + x                 │
│           = 0xA0000           │
│           + 320 * 50          │
│           + 100               │
│           = 0xA3F64           │
└──────┬────────────────────────┘
       │
       ↓
┌───────────────────────────────┐
│ 写入显存                      │
│ *pixelAddr = colorIndex;      │
│ *(0xA3F64) = 0x01;            │
└──────┬────────────────────────┘
       │ VGA硬件监视显存变化
       ↓
┌───────────────────────────────┐
│ VGA 图形控制器                │
│  - 读取显存数据               │
│  - 查找调色板寄存器           │
│  - palette[0x01] → RGB(0,0,168)│
└──────┬────────────────────────┘
       │
       ↓
┌───────────────────────────────┐
│ DAC (数模转换器)              │
│  - 将数字RGB转换为模拟信号    │
│  - R: 0/255 * 0.7V            │
│  - G: 0/255 * 0.7V            │
│  - B: 168/255 * 0.7V          │
└──────┬────────────────────────┘
       │
       ↓
┌───────────────────────────────┐
│ 显示器                        │
│  - 在 (100, 50) 显示蓝色像素  │
└───────────────────────────────┘
```

**内存映射：**
```
320x200x8位模式：

显存起始：0xA0000
显存大小：320 * 200 = 64000 字节

像素(x, y)的地址 = 0xA0000 + y * 320 + x

例子：
- (0, 0) → 0xA0000
- (319, 0) → 0xA013F (第一行最后一个)
- (0, 1) → 0xA0140 (第二行开始)
- (319, 199) → 0xAF9FF (右下角)
```

---

## 🔧 关键数据结构

### IDT 门描述符（8字节）

```
 63                           48 47      40 39    32
+-------------------------------+---------+--------+
|  Handler Address [31:16]      | Access  | Reserv |
+-------------------------------+---------+--------+
 31                           16 15               0
+-------------------------------+------------------+
| GDT Code Segment Selector     | Handler Addr[15:0]|
+-------------------------------+------------------+

Access Byte (位39-32):
  ┌─┬─┬───┬─┬────┐
  │P│DPL│0│Type│
  └─┴─┴───┴─┴────┘
  P: Present (1=有效)
  DPL: 特权级 (0-3)
  Type: 门类型 (0xE=32位中断门)

例子：KeyboardDriver (IRQ1 → 0x21)
  Handler Address: KeyboardDriver::HandleInterrupt 的地址
  Segment Selector: gdt.CodeSegmentSelector() = 0x08
  Access: 0x8E = 10001110b
    P=1 (存在)
    DPL=00 (内核级)
    Type=1110 (32位中断门)
```

### PCI 配置地址格式（32位）

```
 31  30-24 23-16  15-11   10-8    7-2      1-0
┌───┬─────┬──────┬───────┬──────┬────────┬────┐
│En │Rsv  │ Bus  │Device │ Func │Register│ 00 │
└───┴─────┴──────┴───────┴──────┴────────┴────┘
 │    │      │       │       │       │      │
 │    │      │       │       │       │      └─ 必须为0（4字节对齐）
 │    │      │       │       │       └─ 寄存器号（0-63）
 │    │      │       │       └─ 功能号（0-7）
 │    │      │       └─ 设备号（0-31）
 │    │      └─ 总线号（0-255）
 │    └─ 保留（必须为0）
 └─ 使能位（必须为1）

例子：读取总线0，设备1，功能0，寄存器0x00（厂商ID）
  地址 = 0x80000000 | (0 << 16) | (1 << 11) | (0 << 8) | 0x00
       = 0x80000800

  写入到端口 0xCF8: commandPort.Write(0x80000800)
  从端口 0xCFC 读取: vendor_id = dataPort.Read()
```

---

## 💡 关键魔数和位操作解释

### 1. GDT 段描述符标志

```cpp
codeSegmentSelector(0, 64*1024*1024, 0x9A)
dataSegmentSelector(0, 64*1024*1024, 0x92)

0x9A = 10011010b (代码段)
  ┌─────┬────┬─┬─┬─┬─┬─┬─┐
  │  1  │ 0  │0│1│1│0│1│0│
  └─────┴────┴─┴─┴─┴─┴─┴─┘
    │     │    │ │ │ │ │ └─ A (Accessed) = 0
    │     │    │ │ │ │ └─── R (Readable) = 1 (代码段可读)
    │     │    │ │ │ └───── C (Conforming) = 0 (不一致)
    │     │    │ │ └─────── E (Executable) = 1 (可执行)
    │     │    │ └───────── S (System) = 1 (代码/数据段)
    │     │    └─────────── DPL = 00 (特权级0，内核)
    │     └──────────────── P (Present) = 1 (段存在)
    └────────────────────── (高位)

0x92 = 10010010b (数据段)
  E = 0 (不可执行)
  R = 1 (可写)
```

**为什么需要？**
- 保护模式要求明确定义每个段的权限
- 防止代码段被写入（安全性）
- 防止数据段被执行（避免恶意代码）

### 2. PIC 重新映射

```cpp
// ICW1: 初始化命令字1
picMasterCommand.Write(0x11);  // 0x11 = 00010001b
                                //   位0=1: 需要ICW4
                                //   位4=1: 初始化

// ICW2: 中断向量偏移
picMasterData.Write(0x20);  // IRQ0-7 → 中断号 0x20-0x27
picSlaveData.Write(0x28);   // IRQ8-15 → 中断号 0x28-0x2F

// ICW3: 级联设置
picMasterData.Write(0x04);  // 0x04 = 00000100b
                            //   位2=1: 从PIC连接到IRQ2

picSlaveData.Write(0x02);   // 0x02: 从PIC的级联标识号

// ICW4: 操作模式
picMasterData.Write(0x01);  // 0x01 = 00000001b
                            //   位0=1: 8086/8088模式
```

**为什么重新映射？**
```
默认映射（不兼容）:
  IRQ0-7  → 中断号 0x08-0x0F
  IRQ8-15 → 中断号 0x70-0x77
  
问题：0x08-0x0F 与 CPU异常冲突！
  0x08: 双重故障异常
  0x0D: 一般保护错误
  
重新映射后（兼容）:
  IRQ0-7  → 0x20-0x27
  IRQ8-15 → 0x28-0x2F
  
好处：完全避开 CPU 保留的异常号（0x00-0x1F）
```

### 3. VGA 颜色反转（鼠标光标）

```cpp
VideoMemory[80*y+x] = 
    ((VideoMemory[80*y+x] & 0xF000) >> 4) |  // 提取背景色 → 前景色位置
    ((VideoMemory[80*y+x] & 0x0F00) << 4) |  // 提取前景色 → 背景色位置
    ((VideoMemory[80*y+x] & 0x00FF));        // 保持字符不变

// VGA 文本模式的16位格式:
// ┌────────┬────────┬────────┬────────┐
// │ 闪烁+背景 │  前景  │     字符ASCII   │
// │  4位   │  4位   │      8位        │
// └────────┴────────┴────────┴────────┘
//  15-12     11-8      7-0

例子：原始值 = 0x0F41 (白色'A'在黑色背景)
  0x0F41 & 0xF000 = 0x0000  // 提取背景(黑色=0)
  0x0000 >> 4     = 0x0000
  
  0x0F41 & 0x0F00 = 0x0F00  // 提取前景(白色=F)
  0x0F00 << 4     = 0xF000
  
  0x0F41 & 0x00FF = 0x0041  // 保持字符('A')
  
  结果 = 0x0000 | 0xF000 | 0x0041 = 0xF041
       (黑色'A'在白色背景) → 反色效果！
```

### 4. PCI 设备检测位操作

```cpp
// 检查设备是否有多个功能
bool DeviceHasFunction(uint16_t bus, uint16_t device) {
    return Read(bus, device, 0, 0x0E) & (1 << 7);
    //                              位7: 多功能标志
}

// 读取时的对齐处理
uint32_t result = dataPort.Read();
return result >> (8 * (register_offset % 4));

例子：读取寄存器 0x02（设备ID，2字节）
  register_offset = 0x02
  
  1. 构造对齐地址: 0x02 & 0xFC = 0x00
     （读取4字节块：0x00-0x03）
  
  2. 读取整个4字节: dataPort.Read()
     返回: [0x03][0x02][0x01][0x00]
           厂商ID  设备ID
  
  3. 右移: result >> (8 * (0x02 % 4))
          = result >> 16
          = 提取 [0x03][0x02]
          = 设备ID
```

---

## 🎓 总结

### 系统的核心设计理念

1. **事件驱动而非轮询**
   - 系统大部分时间处于 `while(1)` 等待状态
   - 所有实际工作由中断触发
   - 高效利用CPU资源

2. **分层架构**
   ```
   应用层 (Event Handlers) ← 用户逻辑
        ↕
   驱动层 (Drivers) ← 设备抽象
        ↕
   硬件抽象层 (Port, InterruptManager) ← 底层访问
        ↕
   硬件层 (物理设备)
   ```

3. **单向依赖**
   - 上层依赖下层，下层通过回调通知上层
   - 避免循环依赖
   - 清晰的模块职责

4. **设计模式的实战应用**
   - 观察者模式：中断→驱动→应用的事件传递
   - 策略模式：统一的驱动接口
   - 单例模式：全局中断管理器
   - 外观模式：简化复杂硬件操作

### 数据流的核心路径

```
硬件 → 中断 → 驱动 → 事件处理器 → 输出
 │      │      │        │          │
物理   CPU    解析    业务逻辑    显示
事件   IDT   协议     回调       结果
```

这个架构为未来扩展提供了良好的基础：
- 添加新硬件？实现新的Driver子类
- 添加新功能？实现新的EventHandler
- 修改行为？替换EventHandler实现

---

## 🌀 深入理解：主循环的哲学

### "什么都不做"的艺术

```cpp
while (1);  // 这是操作系统最重要的一行代码
```

这个看似荒谬的空循环，实际上体现了现代操作系统设计的核心哲学：

#### 1️⃣ **响应式编程思想**

```
传统程序流程（主动）:
  开始 → 处理 → 结束
  ✓ 有明确的生命周期
  ✗ 不适合持续运行的系统

操作系统流程（被动）:
  开始 → 等待 → [事件] → 处理 → 等待 → [事件] → ...
  ✓ 永不结束
  ✓ 完全事件驱动
  ✓ 资源高效利用
```

#### 2️⃣ **控制反转 (Inversion of Control)**

```
传统应用程序：
  main() {
      data = readKeyboard();    // 程序主动拉取数据
      processData(data);
  }
  问题：如果没有数据，程序会阻塞

SAOS操作系统：
  main() {
      while(1);                 // 程序被动等待
  }
  
  KeyboardDriver::HandleInterrupt() {
      data = port.Read();       // 硬件推送数据
      handler->OnKeyDown(data); // 回调通知应用
  }
  优势：永不阻塞，多设备并发
```

#### 3️⃣ **主循环 vs 中断优先级**

```
CPU执行优先级（从高到低）：

优先级 0 (最高): NMI (不可屏蔽中断)
  └─> 关键硬件错误，必须立即处理

优先级 1: 硬件中断 (IRQ)
  ├─> IRQ0: 定时器 (每秒触发18.2次)
  ├─> IRQ1: 键盘
  ├─> IRQ12: 鼠标
  └─> ...其他设备

优先级 2: 软件中断 (系统调用)
  └─> 应用程序请求内核服务

优先级 3 (最低): 主循环 while(1)
  └─> 仅在没有任何中断时执行
  └─> 可以被任何中断抢占

这意味着：
- 键盘中断可以随时打断 while(1)
- while(1) 永远给中断让路
- 系统响应时间 < 1微秒
```

### 💡 如果没有主循环会怎样？

```cpp
// 反面教材：kernelMain直接返回
extern "C" void kernelMain(...) {
    // ... 初始化代码 ...
    interrupts.Activate();
    
    return;  // ❌ 错误！内核结束了
}

结果：
1. CPU继续执行下一条指令（未定义行为）
2. 可能跳转到随机内存地址
3. 触发保护错误，系统崩溃
4. 即使中断仍然启用，也没有"地方"可以返回
```

**正确做法：永不返回**
```cpp
extern "C" void kernelMain(...) {
    // ... 初始化代码 ...
    interrupts.Activate();
    
    while (1);  // ✅ 正确：给中断一个"家"
    
    // 这行永远不会执行
}
```

### 🎪 完整的循环生命周期

```
阶段1: 启动准备（一次性）
┌──────────────────────────────┐
│ BIOS → GRUB → loader.s       │
│ callConstructors()           │
│ kernelMain() 开始            │
│  ├─ 初始化GDT                │
│  ├─ 初始化IDT                │
│  ├─ 创建驱动                 │
│  ├─ 激活驱动                 │
│  └─ interrupts.Activate()    │
│     └─> asm("sti");  ⚡      │ ← 关键时刻！启用中断
└──────────────────────────────┘
          │
          ↓
阶段2: 进入主循环（永久态）
╔══════════════════════════════╗
║  while (1);                  ║ ← CPU在此无限循环
║    ↓                         ║
║    └→ jmp (循环自己)         ║
╚══════════════════════════════╝
    ↑       ↓
    │       │ 中断发生时：
    │       ↓
    │  ┌────────────────────┐
    │  │ CPU硬件自动：      │
    │  │ 1. 保存PC寄存器    │ ← 记住while循环的地址
    │  │ 2. 保存标志寄存器  │
    │  │ 3. 查IDT表         │
    │  │ 4. 跳转中断处理    │
    │  └────────────────────┘
    │          ↓
    │  ┌────────────────────┐
    │  │ 中断处理器工作：   │
    │  │ - 读取硬件数据     │
    │  │ - 调用驱动         │
    │  │ - 执行回调         │
    │  │ - 发送EOI          │
    │  └────────────────────┘
    │          ↓
    │  ┌────────────────────┐
    │  │ iret指令：         │
    │  │ 1. 恢复标志寄存器  │
    │  │ 2. 恢复PC寄存器    │ ← 返回while循环
    └──│ 3. 继续执行        │
       └────────────────────┘
```

### 🔬 实验：观察主循环

如果你想"看到"主循环在工作，可以在QEMU中启用调试：

```bash
# 1. 编译带符号的内核
make

# 2. 使用GDB调试QEMU
qemu-system-i386 -kernel kernel.bin -s -S &
gdb kernel.bin
(gdb) target remote localhost:1234
(gdb) break kernelMain
(gdb) continue
(gdb) finish  # 运行到while(1)

# 3. 查看汇编代码
(gdb) disassemble
   ... 初始化代码 ...
   0x00102345:  jmp 0x00102345  ← 这就是while(1)!
   
# 4. 单步执行
(gdb) stepi
   → 看到PC寄存器一直在0x00102345跳转

# 5. 设置中断断点
(gdb) break KeyboardDriver::HandleInterrupt
(gdb) continue
   → 按下键盘，GDB会停在中断处理器
   → 完成后，返回0x00102345继续循环
```

### 🎯 关键要点总结

| 概念 | 传统程序 | SAOS主循环 |
|------|----------|------------|
| **生命周期** | 有限（开始→结束） | 无限（永不退出） |
| **控制流** | 顺序执行 | 事件驱动 |
| **资源使用** | 主动轮询（低效） | 被动等待（高效） |
| **并发模型** | 单线程阻塞 | 中断驱动并发 |
| **返回值** | 有意义的返回 | 永不返回 |
| **主要工作** | 在main()中完成 | 在中断处理器中完成 |

### 📊 主循环的完整视图

```
═══════════════════════════════════════════════════════════════════════
                         SAOS 系统架构全景图
═══════════════════════════════════════════════════════════════════════

                        ┌─────────────────┐
                        │   物理硬件层    │
                        │  键盘/鼠标/VGA  │
                        └────────┬────────┘
                                 │ IRQ信号
                                 ↓
                        ┌─────────────────┐
                        │  中断控制器(PIC)│
                        │  IRQ → 中断号   │
                        └────────┬────────┘
                                 │ 硬件中断
                                 ↓
        ╔═══════════════════════════════════════════════════╗
        ║              CPU 中断处理机制                     ║
        ║                                                   ║
        ║  ┌─────────────────────────────────────────────┐ ║
        ║  │          IDT (中断描述符表)                 │ ║
        ║  │  [0x21] → KeyboardDriver::HandleInterrupt   │ ║
        ║  │  [0x2C] → MouseDriver::HandleInterrupt      │ ║
        ║  │  ...                                        │ ║
        ║  └─────────────────────────────────────────────┘ ║
        ║                      ↓                            ║
        ║            【中断处理执行】                       ║
        ║                      ↓                            ║
        ║            【iret返回】                           ║
        ╚═══════════════════════╦═══════════════════════════╝
                                ↓
        ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
        ┃        主循环层 - kernel.cpp 第304行          ┃
        ┃                                                ┃
        ┃          ╔════════════════════╗                ┃
        ┃          ║    while (1);      ║  ← 永恒的支点  ┃
        ┃          ║       ↓↑           ║                ┃
        ┃          ║    jmp loop        ║                ┃
        ┃          ╚════════════════════╝                ┃
        ┃                                                ┃
        ┃  特性：                                        ┃
        ┃  • 永不退出                                    ┃
        ┃  • 最低优先级 (任何中断都能打断)              ┃
        ┃  • 零资源消耗 (CPU可HLT休眠)                  ┃
        ┃  • 中断的"返回地址"                           ┃
        ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
                                │
                        系统持续运行
                        直到断电/重启
                                
═══════════════════════════════════════════════════════════════════════
                        时间线示意图
═══════════════════════════════════════════════════════════════════════

T0    [启动] → [初始化] → [进入主循环]
T1-∞  ───────────────────────────────────────────────────────────────→
      ↑                ↑              ↑              ↑
      │ 键盘中断       │ 鼠标中断     │ 定时器中断   │ ...更多中断
      └─[处理]─回到主循环
                       └─[处理]─回到主循环
                                      └─[处理]─回到主循环

规律：
  - 主循环始终在运行（除了处理中断的短暂时刻）
  - 99.9%的时间：CPU在主循环中"空转"
  - 0.1%的时间：CPU处理中断
  - 响应延迟：< 1微秒（从硬件信号到开始执行中断处理器）

═══════════════════════════════════════════════════════════════════════
```

### 💭 哲学思考

> "最好的循环是看不见的循环。"
> 
> "最强大的系统是什么都不做的系统——因为真正的工作由事件驱动。"

SAOS的 `while(1);` 不是程序的终点，而是系统的**支撑点**——就像地球自转的轴心，看似静止，实则支撑了整个系统的运转。

所有的键盘输入、鼠标移动、硬件中断，都像舞者围绕这个静止的中心旋转。主循环不参与表演，但没有它，舞台就会坍塌。

---

## 🔄 进程/任务调度：从单任务到多任务

### 🎯 SAOS 当前状态：单任务内核

**重要说明**：SAOS 目前是一个**单任务（Single-Tasking）系统**，没有实现进程/任务调度。

```cpp
// 当前的 SAOS 系统结构：
┌────────────────────────────────────┐
│         内核空间 (唯一任务)        │
│                                    │
│  ┌──────────────────────────────┐ │
│  │   kernelMain()               │ │
│  │   ├─ 初始化代码              │ │
│  │   ├─ 驱动管理               │ │
│  │   └─ while(1);  ← 唯一运行态 │ │
│  └──────────────────────────────┘ │
│                                    │
│  【中断处理器】                    │
│  - KeyboardDriver                  │
│  - MouseDriver                     │
│  - 直接在内核上下文执行            │
└────────────────────────────────────┘

特点：
✓ 简单直接，无调度开销
✓ 响应延迟极低（< 1μs）
✗ 无法运行用户程序
✗ 无法并发多个任务
✗ 没有进程隔离
```

### 🚀 进化路径：引入任务调度

如果 SAOS 要升级为**多任务系统**，主循环会发生根本性变化：

#### 📊 架构对比：单任务 vs 多任务

```
┌─────────────────────────────────────────────────────────────────┐
│                    单任务 SAOS (当前)                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   while(1);  ← 空循环，等待中断                                 │
│                                                                 │
│   工作模式：100% 中断驱动                                       │
│   CPU状态：空闲时完全空转                                       │
│   用户程序：无                                                  │
└─────────────────────────────────────────────────────────────────┘

                            VS

┌─────────────────────────────────────────────────────────────────┐
│                   多任务 OS (未来扩展)                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   while(1) {                                                    │
│       Task* next = scheduler.PickNextTask();  ← 调度算法        │
│       SwitchTo(next);  ← 上下文切换                             │
│   }                                                             │
│                                                                 │
│   工作模式：调度驱动 + 中断驱动                                 │
│   CPU状态：始终有任务在运行                                     │
│   用户程序：多个进程并发执行                                    │
└─────────────────────────────────────────────────────────────────┘
```

### 🧩 任务/进程如何融入主循环

#### 方案 1：协作式多任务（Cooperative Multitasking）

```cpp
// 任务结构定义
struct Task {
    uint32_t id;
    uint32_t esp;      // 栈指针
    uint32_t ebp;      // 基址指针
    uint32_t eip;      // 指令指针
    TaskState state;   // RUNNING, READY, BLOCKED
    Task* next;        // 任务链表
};

class TaskManager {
    Task* current_task;
    Task* task_list;
    
    void Yield() {
        // 当前任务主动让出CPU
        SaveContext(current_task);
        current_task = current_task->next;
        RestoreContext(current_task);
    }
};

// 改造后的主循环：
extern "C" void kernelMain(...) {
    // ... 初始化代码 ...
    
    TaskManager taskManager;
    
    // 创建任务
    taskManager.CreateTask(task1_entry);
    taskManager.CreateTask(task2_entry);
    taskManager.CreateTask(task3_entry);
    
    interrupts.Activate();
    
    // 主循环变成调度循环
    while(1) {
        Task* task = taskManager.GetNextTask();
        if (task != nullptr) {
            taskManager.SwitchTo(task);  // 运行任务
        }
        // 如果没有就绪任务，进入低功耗模式
        asm("hlt");
    }
}

// 示例任务函数
void task1_entry() {
    while(1) {
        printf("Task 1 running\n");
        taskManager.Yield();  // 主动让出CPU
    }
}

void task2_entry() {
    while(1) {
        printf("Task 2 running\n");
        taskManager.Yield();
    }
}
```

**执行流程：**
```
T=0    主循环: 调度Task1
T=10   Task1: printf → Yield()
       ↓
       主循环: 调度Task2
T=20   Task2: printf → Yield()
       ↓
       主循环: 调度Task3
T=30   Task3: printf → Yield()
       ↓
       主循环: 回到Task1
       ... 循环往复 ...
```

#### 方案 2：抢占式多任务（Preemptive Multitasking）⭐ 推荐

```cpp
class TaskScheduler {
    Task* current_task;
    Task* ready_queue;
    uint32_t time_slice;  // 时间片（例如10ms）
    
    // 定时器中断处理器
    class TimerInterruptHandler : public InterruptHandler {
        TaskScheduler* scheduler;
        
        uint32_t HandleInterrupt(uint32_t esp) override {
            scheduler->tick_count++;
            
            // 时间片用完，强制切换任务
            if (scheduler->tick_count >= scheduler->time_slice) {
                scheduler->tick_count = 0;
                esp = scheduler->Schedule(esp);  // 返回新任务的栈指针
            }
            
            return esp;
        }
    };
    
    uint32_t Schedule(uint32_t current_esp) {
        // 保存当前任务状态
        current_task->esp = current_esp;
        SaveRegisters(current_task);
        
        // 选择下一个任务（Round-Robin算法）
        current_task = PickNextTask();
        
        // 恢复新任务状态
        RestoreRegisters(current_task);
        return current_task->esp;  // 返回新栈指针
    }
};

// 改造后的主循环：
extern "C" void kernelMain(...) {
    // ... 初始化代码 ...
    
    TaskScheduler scheduler;
    
    // 注册定时器中断（IRQ0，每秒触发约18.2次）
    TimerInterruptHandler timerHandler(&scheduler);
    interrupts.SetHandler(0x20, &timerHandler);
    
    // 创建任务
    scheduler.CreateTask(task1_entry, PRIORITY_NORMAL);
    scheduler.CreateTask(task2_entry, PRIORITY_HIGH);
    scheduler.CreateTask(idle_task, PRIORITY_LOWEST);
    
    interrupts.Activate();
    
    // 主循环变成"空闲任务"
    while(1) {
        asm("hlt");  // CPU休眠，等待下一次定时器中断
    }
}
```

**执行时间线：**
```
时间线（抢占式多任务）：

T=0ms     主循环: CPU休眠 (hlt)
          └→ current_task = Task1

T=10ms    ⚡ 定时器中断 (IRQ0)
          ├→ 保存Task1上下文
          ├→ 选择Task2
          ├→ 恢复Task2上下文
          └→ iret返回到Task2的代码

T=15ms    ⚡ 键盘中断 (IRQ1)
          ├→ 暂停Task2
          ├→ KeyboardDriver::HandleInterrupt()
          └→ iret返回到Task2继续执行

T=20ms    ⚡ 定时器中断 (IRQ0)
          ├→ 保存Task2上下文
          ├→ 选择Task3
          └→ 恢复Task3上下文

T=30ms    ⚡ 定时器中断 (IRQ0)
          └→ 切换回Task1

... 循环往复，每个任务都能获得CPU时间 ...
```

### 🎭 完整的多任务系统架构

```
═══════════════════════════════════════════════════════════════════
                      多任务操作系统全景图
═══════════════════════════════════════════════════════════════════

                         用户空间
    ┌──────────────┬──────────────┬──────────────┬──────────────┐
    │   Task 1     │   Task 2     │   Task 3     │   Task N     │
    │  (用户程序)  │  (用户程序)  │  (用户程序)  │  (空闲任务)  │
    └──────┬───────┴──────┬───────┴──────┬───────┴──────┬───────┘
           │              │              │              │
           └──────────────┴──────────────┴──────────────┘
                          系统调用
                             ↓
    ═══════════════════════════════════════════════════════════════
                          内核空间
    ┌─────────────────────────────────────────────────────────────┐
    │                    任务调度器                               │
    │  ┌─────────────────────────────────────────────────────┐   │
    │  │ Ready Queue:  [Task1] → [Task2] → [Task3]           │   │
    │  │ Blocked Queue: [Task4等待IO] → [Task5等待信号]      │   │
    │  │ Current Task: Task2                                 │   │
    │  └─────────────────────────────────────────────────────┘   │
    │                             ↓                               │
    │            【上下文切换 Context Switch】                     │
    │                             ↓                               │
    │  ┌─────────────────────────────────────────────────────┐   │
    │  │  保存当前任务：                                      │   │
    │  │   - 通用寄存器 (EAX, EBX, ECX, ...)                │   │
    │  │   - 栈指针 (ESP, EBP)                               │   │
    │  │   - 指令指针 (EIP)                                  │   │
    │  │   - 标志寄存器 (EFLAGS)                             │   │
    │  │   - 页表指针 (CR3)                                  │   │
    │  └─────────────────────────────────────────────────────┘   │
    │                             ↓                               │
    │  ┌─────────────────────────────────────────────────────┐   │
    │  │  恢复下一个任务的所有寄存器                          │   │
    │  └─────────────────────────────────────────────────────┘   │
    └─────────────────────────────────────────────────────────────┘
                             ↑
                    【定时器中断触发】
                      每10ms一次
                             ↑
    ┌─────────────────────────────────────────────────────────────┐
    │                       主循环层                              │
    │                                                             │
    │   while(1) {                                                │
    │       hlt;  // CPU休眠，等待中断                            │
    │   }                                                         │
    │                                                             │
    │   主循环角色变化：                                          │
    │   - 单任务系统：主循环是唯一的"任务"                       │
    │   - 多任务系统：主循环是"空闲任务"（idle task）            │
    │                 只在没有其他任务就绪时运行                 │
    └─────────────────────────────────────────────────────────────┘

═══════════════════════════════════════════════════════════════════
```

### 🔑 关键概念：任务上下文切换

```cpp
// 上下文切换的核心代码
uint32_t ContextSwitch(Task* old_task, Task* new_task) {
    // 第1步：保存旧任务的CPU状态
    asm volatile(
        "pushal"              // 保存所有通用寄存器
    );
    old_task->esp = getCurrentESP();
    old_task->ebp = getCurrentEBP();
    
    // 第2步：切换内存空间（如果有虚拟内存）
    if (old_task->page_directory != new_task->page_directory) {
        LoadPageDirectory(new_task->page_directory);
    }
    
    // 第3步：恢复新任务的CPU状态
    setESP(new_task->esp);
    setEBP(new_task->ebp);
    asm volatile(
        "popal"               // 恢复所有通用寄存器
    );
    
    // 第4步：跳转到新任务的代码位置
    // 当函数返回时，CPU会继续执行新任务的代码
    return new_task->esp;
}
```

**上下文切换的时间成本：**
```
典型的上下文切换耗时：
- 保存寄存器：    ~50个CPU周期
- 切换页表：      ~100个CPU周期
- 恢复寄存器：    ~50个CPU周期
- TLB刷新开销：   ~1000个CPU周期

总计：约 1-2 微秒 @ 1GHz CPU
```

### 📊 主循环的三种形态对比

```
┌──────────────────┬───────────────────┬─────────────────────┬─────────────────────┐
│                  │  裸机/单任务系统  │  协作式多任务       │  抢占式多任务       │
│                  │  (当前SAOS)       │                     │  (推荐)             │
├──────────────────┼───────────────────┼─────────────────────┼─────────────────────┤
│ 主循环代码       │ while(1);         │ while(1) {          │ while(1) {          │
│                  │                   │   scheduler.Run();  │   hlt;              │
│                  │                   │ }                   │ }                   │
├──────────────────┼───────────────────┼─────────────────────┼─────────────────────┤
│ 任务切换时机     │ 无任务切换        │ 任务主动调用Yield() │ 定时器中断强制切换  │
├──────────────────┼───────────────────┼─────────────────────┼─────────────────────┤
│ 公平性           │ N/A               │ ❌ 取决于任务配合   │ ✅ 强制保证         │
├──────────────────┼───────────────────┼─────────────────────┼─────────────────────┤
│ 响应时间         │ < 1μs             │ 不确定（可能很长）  │ 可控（时间片大小）  │
├──────────────────┼───────────────────┼─────────────────────┼─────────────────────┤
│ 实现复杂度       │ 低                │ 中                  │ 高                  │
├──────────────────┼───────────────────┼─────────────────────┼─────────────────────┤
│ 主循环角色       │ 唯一执行路径      │ 任务调度循环        │ 空闲任务            │
├──────────────────┼───────────────────┼─────────────────────┼─────────────────────┤
│ 适用场景         │ 嵌入式/教学       │ 实时系统            │ 通用操作系统        │
└──────────────────┴───────────────────┴─────────────────────┴─────────────────────┘
```

### 🎯 实际例子：任务调度的工作流程

```
场景：3个任务在运行

┌────────────────────────────────────────────────────────────────┐
│ Task 1: 文本编辑器                                             │
│   while(1) {                                                   │
│     ProcessKeyInput();  // 处理用户输入                        │
│     UpdateScreen();     // 更新显示                            │
│   }                                                            │
└────────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────────┐
│ Task 2: 音乐播放器                                             │
│   while(1) {                                                   │
│     DecodeAudio();      // 解码音频                            │
│     SendToSoundCard();  // 输出到声卡                          │
│   }                                                            │
└────────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────────┐
│ Task 3: 后台下载                                               │
│   while(1) {                                                   │
│     ReadNetworkData();  // 读取网络数据                        │
│     WriteToFile();      // 写入文件                            │
│   }                                                            │
└────────────────────────────────────────────────────────────────┘

时间线（CPU视角）：
═══════════════════════════════════════════════════════════════════

T=0ms     执行Task1 (文本编辑器)
T=10ms    ⚡ 定时器中断 → 保存Task1 → 切换到Task2
T=20ms    ⚡ 定时器中断 → 保存Task2 → 切换到Task3
T=25ms    ⚡ 键盘中断 → Task3被暂停 → 处理键盘输入 → 恢复Task3
T=30ms    ⚡ 定时器中断 → 保存Task3 → 切换到Task1
...

用户感受：三个程序"同时"运行（实际是快速切换的错觉）
```

### 💡 为什么 SAOS 目前不需要任务调度？

```
当前 SAOS 的设计哲学：

✓ 简单性优先
  - 没有任务调度，代码量大幅减少
  - 易于理解和调试
  - 适合教学和原型开发

✓ 专注于硬件抽象
  - 核心目标是展示驱动框架
  - 演示中断处理机制
  - 展示设计模式应用

✓ 性能优势
  - 零上下文切换开销
  - 中断响应延迟最小化
  - 适合实时性要求高的场景

但如果要运行用户程序，任务调度是必须的！
```

### 🚀 如何为 SAOS 添加任务支持？

**实现路线图：**

```
阶段 1: 基础任务结构
  ├─ 定义 Task 结构体
  ├─ 实现任务创建/销毁
  └─ 实现简单的任务列表管理

阶段 2: 上下文切换
  ├─ 保存/恢复寄存器状态
  ├─ 实现栈切换
  └─ 测试基本的任务切换

阶段 3: 调度算法
  ├─ 实现 Round-Robin 调度
  ├─ 添加优先级支持
  └─ 集成到定时器中断

阶段 4: 高级特性
  ├─ 任务同步（信号量、互斥锁）
  ├─ 进程间通信（IPC）
  └─ 虚拟内存隔离

改造主循环：
  从： while(1);
  到： while(1) { scheduler.RunNextTask(); }
```

### 📝 总结

| 概念 | SAOS 当前 | 多任务扩展 |
|------|-----------|------------|
| **主循环** | 空循环，纯粹等待中断 | 调度循环或空闲任务 |
| **执行流** | 单一内核上下文 | 多个任务上下文切换 |
| **并发性** | 仅中断处理器并发 | 任务级并发 |
| **用户程序** | 不支持 | 支持多个用户进程 |
| **CPU利用率** | 事件驱动时工作 | 始终有任务运行 |

**关键洞察**：
- 单任务系统：主循环**就是**整个系统
- 多任务系统：主循环**承载**多个系统

主循环从"唯一的主角"变成了"舞台管理者"——它不再亲自表演，而是决定谁上台表演。

---

*📝 本文档详细解释了SAOS的完整架构、设计模式、数据流和关键实现细节。配合代码注释阅读效果更佳！*

