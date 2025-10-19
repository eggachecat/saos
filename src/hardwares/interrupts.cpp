#include <hardwares/interrupts.h>
using namespace saos::common;
using namespace saos::hardwares;
/*
    esp: 
        current stack pointer
        CPU will get this for us
        
*/

void printf(const char *str);
void printfHex(uint8_t);

InterruptHandler::InterruptHandler(uint8_t interruptNumber, InterruptManager *interruptManager)
{
    this->interruptNumber = interruptNumber;
    this->interruptManager = interruptManager;
    interruptManager->handlers[interruptNumber] = this;
}
InterruptHandler::~InterruptHandler()
{
    if (interruptManager->handlers[interruptNumber] == this)
    {
        interruptManager->handlers[interruptNumber] = 0;
    }
}

uint32_t InterruptHandler::HandleInterrupt(uint32_t esp)
{
    return esp;
}

InterruptManager::GateDescriptor InterruptManager::interruptDescriptorTable[256];

InterruptManager *InterruptManager::ActiveInterruptManager = 0;

void InterruptManager::IgnoreInterruptRequest()
{
    // ignore all interrupt not implemented
}

void InterruptManager::SetInterruptDescriptorTableEntry(
    uint8_t interruptNumber,
    uint16_t gdt_codeSegmentSelectorOffset,
    void (*handler)(),
    uint8_t DescriptorPrivilegeLevel,
    uint8_t DescriptorType)
{
    const uint8_t IDT_DESC_PRESENT = 0x80;
    interruptDescriptorTable[interruptNumber].handlerAddressLowBits = ((uint32_t)handler) & 0xFFFF;
    interruptDescriptorTable[interruptNumber].handlerAddressHighBits = (((uint32_t)handler >> 16)) & 0xFFFF;
    interruptDescriptorTable[interruptNumber].gdt_cideSedmentSelector = gdt_codeSegmentSelectorOffset;
    interruptDescriptorTable[interruptNumber].access = IDT_DESC_PRESENT | DescriptorType | ((DescriptorPrivilegeLevel & 3) << 5);
    interruptDescriptorTable[interruptNumber].reserved = 0;
}

InterruptManager::InterruptManager(GlobalDescriptorTable *gdt)
    : picMasterCommand(0x20),      // 主PIC命令端口
      picMasterData(0x21),         // 主PIC数据端口  
      picSlaveCommand(0xA0),       // 从PIC命令端口
      picSlaveData(0xA1)           // 从PIC数据端口
{
    /*
     * 初始化中断描述符表(IDT)
     * IDT告诉CPU当收到特定中断号时应该调用哪个处理函数
     * 每个中断号对应一个8字节的门描述符
     */
    uint16_t CodeSegment = gdt->CodeSegmentSelector();
    const uint8_t IDT_INTERRUPT_GATE = 0xE;  // 中断门类型

    // 初始化所有256个中断向量，默认都指向忽略处理函数
    for (uint16_t i = 0; i < 256; i++)
    {
        handlers[i] = 0;
        SetInterruptDescriptorTableEntry(i, CodeSegment, &IgnoreInterruptRequest, 0, IDT_INTERRUPT_GATE);
    }
    
    /*
     * 设置硬件中断处理函数:
     * 0x20: 定时器中断 (IRQ0)
     * 0x21: 键盘中断 (IRQ1)  
     * 0x2C: 鼠标中断 (IRQ12)
     * 这些是重新映射后的中断号，避免与CPU异常冲突
     */
    SetInterruptDescriptorTableEntry(0x20, CodeSegment, &HandleInterruptRequest0x00, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x21, CodeSegment, &HandleInterruptRequest0x01, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x2C, CodeSegment, &HandleInterruptRequest0x0C, 0, IDT_INTERRUPT_GATE);

    /*
     * 重新配置PIC (可编程中断控制器)
     * 默认情况下IRQ0-7映射到中断0x08-0x0F，与CPU异常重叠
     * 我们将其重新映射到0x20-0x27 (主PIC) 和 0x28-0x2F (从PIC)
     */
    
    // 开始初始化序列 (ICW1)
    picMasterCommand.Write(0x11);  // 0x11 = 需要ICW4 + 级联模式
    picSlaveCommand.Write(0x11);

    // 设置中断向量偏移 (ICW2)  
    picMasterData.Write(0x20);     // 主PIC: IRQ0-7 -> 中断0x20-0x27
    picSlaveData.Write(0x28);      // 从PIC: IRQ8-15 -> 中断0x28-0x2F

    // 设置级联连接 (ICW3)
    picMasterData.Write(0x04);     // 主PIC: 从PIC连接到IRQ2 (位2=1)
    picSlaveData.Write(0x02);      // 从PIC: 级联标识为2

    // 设置操作模式 (ICW4)
    picMasterData.Write(0x01);     // 8086模式
    picSlaveData.Write(0x01);

    // 清除中断屏蔽，允许所有中断
    picMasterData.Write(0x00);
    picSlaveData.Write(0x00);

    // 将IDT加载到CPU
    InterruptDescriptorTablePointer idt;
    idt.size = 256 * sizeof(GateDescriptor) - 1;  // IDT大小-1
    idt.base = (uint32_t)interruptDescriptorTable; // IDT基址
    asm volatile("lidt %0" ::"m"(idt));            // 使用lidt指令加载IDT
}

InterruptManager::~InterruptManager()
{
}

void InterruptManager::Activate()
{
    if (ActiveInterruptManager != 0)
        ActiveInterruptManager->Deactivate();

    // deactivate the old one and activate the new one
    ActiveInterruptManager = this;
    // make os instantiate all the hardware after IDT table created!
    // then activate IDT
    // we dont want CPU should handlers interrupt before all hardware all set
    asm("sti"); // start interrupt
}

void InterruptManager::Deactivate()
{
    if (ActiveInterruptManager == this)
    {
        ActiveInterruptManager = 0;
        asm("cli");
    }
}

uint32_t InterruptManager::HandleInterrupt(uint8_t interruptNumber, uint32_t esp)
{
    if (ActiveInterruptManager != 0)
    {
        return ActiveInterruptManager->DoHandleInterrupt(interruptNumber, esp);
    }
    printf("Interrupt");
    return esp;
}

uint32_t InterruptManager::DoHandleInterrupt(uint8_t interruptNumber, uint32_t esp)
{
    // 如果有注册的中断处理程序，调用它
    if (handlers[interruptNumber] != 0)
    {
        esp = handlers[interruptNumber]->HandleInterrupt(esp);
    }
    else if (interruptNumber != 0x20) // 忽略定时器中断的未处理消息
    {
        printf("UNHANDLED Interrupt");
        printfHex(interruptNumber);
    }
    
    /*
     * 发送EOI (End of Interrupt) 信号给PIC
     * 告诉PIC当前中断已处理完毕，可以继续处理下一个中断
     * 硬件中断范围: 0x20-0x2F
     */
    if (0x20 <= interruptNumber && interruptNumber < 0x30)
    {
        picMasterCommand.Write(0x20); // 向主PIC发送EOI
        if (0x28 <= interruptNumber)  // 如果是从PIC的中断(IRQ8-15)
        {
            picSlaveCommand.Write(0x20); // 也要向从PIC发送EOI
        }
    }

    return esp;
}