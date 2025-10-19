#ifndef __SAOS__HARDWARES__INTERRUPTS_H
#define __SAOS__HARDWARES__INTERRUPTS_H

#include <common/types.h>
#include <hardwares/port.h>
#include <gdt.h>
namespace saos
{
namespace hardwares
{
class InterruptManager;
class InterruptHandler
{
  protected:
    saos::common::uint8_t interruptNumber;
    InterruptManager *interruptManager;
    InterruptHandler(saos::common::uint8_t interruptNumber, InterruptManager *interruptManager);
    ~InterruptHandler();

  public:
    virtual saos::common::uint32_t HandleInterrupt(saos::common::uint32_t esp);
};
class InterruptManager
{
    friend class InterruptHandler;

  protected:
    static InterruptManager *ActiveInterruptManager;
    InterruptHandler *handlers[256];
    struct GateDescriptor
    {
        saos::common::uint16_t handlerAddressLowBits;
        saos::common::uint16_t gdt_cideSedmentSelector;
        saos::common::uint8_t reserved;
        saos::common::uint8_t access;
        saos::common::uint16_t handlerAddressHighBits;
    } __attribute__((packed));

    static GateDescriptor interruptDescriptorTable[256];

    struct InterruptDescriptorTablePointer
    {
        saos::common::uint16_t size;
        saos::common::uint32_t base;
    } __attribute__((packed));

    static void SetInterruptDescriptorTableEntry(
        saos::common::uint8_t interruptNumber,
        saos::common::uint16_t gdt_codeSegmentSelectorOffset,
        void (*handler)(),
        saos::common::uint8_t DescriptorPrivilegeLevel,
        saos::common::uint8_t DescriptorType);

    saos::hardwares::Port_8Bit_Slow picMasterCommand;
    saos::hardwares::Port_8Bit_Slow picMasterData;

    saos::hardwares::Port_8Bit_Slow picSlaveCommand;
    saos::hardwares::Port_8Bit_Slow picSlaveData;

  public:
    InterruptManager(saos::GlobalDescriptorTable *gdt);
    ~InterruptManager();

    void Activate();
    void Deactivate();

    static saos::common::uint32_t HandleInterrupt(saos::common::uint8_t interruptNumber, saos::common::uint32_t esp);
    saos::common::uint32_t DoHandleInterrupt(saos::common::uint8_t interruptNumber, saos::common::uint32_t esp);

    static void IgnoreInterruptRequest();
    // implemented in interruptstub.s
    // this is holt shit
    static void HandleInterruptRequest0x00();
    static void HandleInterruptRequest0x01();
    static void HandleInterruptRequest0x0C();
};
} // namespace hardwares
} // namespace saos
#endif