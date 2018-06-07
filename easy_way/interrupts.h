#ifndef __INTERRUPTS_H
#define __INTERRUPTS_H

#include "types.h"
#include "port.h"
#include "gdt.h"
class InterruptManager
{

  protected:
    static InterruptManager *ActiveInterruptManager;

    struct GateDescriptor
    {
        uint16_t handlerAddressLowBits;
        uint16_t gdt_cideSedmentSelector;
        uint8_t reserved;
        uint8_t access;
        uint16_t handlerAddressHighBits;
    } __attribute__((packed));

    static GateDescriptor interruptDescriptorTable[256];

    struct InterruptDescriptorTablePointer
    {
        uint16_t size;
        uint32_t base;
    } __attribute__((packed));

    static void SetInterruptDescriptorTableEntry(
        uint8_t interruptNumber,
        uint16_t gdt_codeSegmentSelectorOffset,
        void (*handler)(),
        uint8_t DescriptorPrivilegeLevel,
        uint8_t DescriptorType);

    Port_8Bit_Slow picMasterCommand;
    Port_8Bit_Slow picMasterData;

    Port_8Bit_Slow picSlaveCommand;
    Port_8Bit_Slow picSlaveData;

  public:
    InterruptManager(GlobalDescriptorTable *gdt);
    ~InterruptManager();

    void Activate();
    void Deactivate();

    static uint32_t HandleInterrupt(uint8_t interruptNumber, uint32_t esp);
    uint32_t DoHandleInterrupt(uint8_t interruptNumber, uint32_t esp);

    static void IgnoreInterruptRequest();
    // implemented in interruptstub.s
    // this is holt shit
    static void HandleInterruptRequest0x00();
    static void HandleInterruptRequest0x01();
};

#endif