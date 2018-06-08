#include <hardwares/interrupts.h>
using namespace saos::common;
using namespace saos::hardwares;
/*
    esp: 
        current stack pointer
        CPU will get this for us
        
*/

void printf(char *str);
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
    : picMasterCommand(0x20),
      picMasterData(0x21),
      picSlaveCommand(0xA0),
      picSlaveData(0xA1)
{
    // in this function
    // we define the IDT
    // bascially we write on a special section of memory
    // to tell the cpu
    // what will it do when recevie a special interrupt with
    // a special interrupt number
    uint16_t CodeSegment = gdt->CodeSegmentSelector();
    const uint8_t IDT_INTERRUPT_GATE = 0xE;

    for (uint16_t i = 0; i < 256; i++)
    {
        handlers[i] = 0;
        SetInterruptDescriptorTableEntry(i, CodeSegment, &IgnoreInterruptRequest, 0, IDT_INTERRUPT_GATE);
    }
    // see
    // we set a magic number 0x20
    // and a handler function HandleInterruptRequest0x00 to handle this interrupt
    // that is
    // call HandleInterruptRequest0x00 when interrupt number is 0x20
    // CodeSegment, 0, IDT_INTERRUPT_GATE are special parameters to
    // locate where the IDT table is
    SetInterruptDescriptorTableEntry(0x20, CodeSegment, &HandleInterruptRequest0x00, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x21, CodeSegment, &HandleInterruptRequest0x01, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x2C, CodeSegment, &HandleInterruptRequest0x0C, 0, IDT_INTERRUPT_GATE);

    // wtf starts
    // I think this is something about hardware setting
    // need to know the tf
    picMasterCommand.Write(0x11);
    picSlaveCommand.Write(0x11);

    picMasterData.Write(0x20);
    picSlaveData.Write(0x28);

    picMasterData.Write(0x04);
    picSlaveData.Write(0x02);

    picMasterData.Write(0x01);
    picSlaveData.Write(0x01);

    picMasterData.Write(0x00);
    picSlaveData.Write(0x00);
    // wtf ends
    InterruptDescriptorTablePointer idt;
    idt.size = 256 * sizeof(GateDescriptor) - 1;
    idt.base = (uint32_t)interruptDescriptorTable;
    asm volatile("lidt %0" ::"m"(idt)); // cpu instruction set; tell cpu use this table
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
    if (handlers[interruptNumber] != 0)
    {
        esp = handlers[interruptNumber]->HandleInterrupt(esp);
    }
    else if (interruptNumber != 0x20) // if the interrupt is not the timer interrupt!
    {
        printf("UNHANDLED Interrupt");
        printfHex(interruptNumber);
    }
    // we tell the pic that it can continue listening (not blocking)

    if (0x20 <= interruptNumber && interruptNumber < 0x30)
    {
        picMasterCommand.Write(0x20); // answer to the master pic
        if (0x28 <= interruptNumber)  // if request is from the slave
        {
            picSlaveCommand.Write(0x20);
        }
    }

    return esp;
}