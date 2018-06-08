#include <hardwares/port.h>
using namespace saos::common;
using namespace saos::hardwares;
Port::Port(uint16_t portnumber)
{
    this->portnumber = portnumber;
}

Port::~Port() {}

Port_8Bit::Port_8Bit(uint16_t portnumber)
    : Port(portnumber) {}

Port_8Bit::~Port_8Bit() {}

void Port_8Bit::Write(uint8_t data)
{
    asm volatile("outb %0, %1"
                 :
                 : "a"(data), "Nd"(portnumber));
}

uint8_t Port_8Bit::Read()
{
    uint8_t result;
    asm volatile("inb %1, %0"
                 : "=a"(result)
                 : "Nd"(portnumber));
}

Port_8Bit_Slow::Port_8Bit_Slow(uint16_t portnumber)
    : Port_8Bit(portnumber) {}

Port_8Bit_Slow::~Port_8Bit_Slow() {}
void Port_8Bit_Slow::Write(uint8_t data)
{
    asm volatile("outb %0, %1\njmp 1f\n1: jmp 1f\n1:"
                 :
                 : "a"(data), "Nd"(portnumber));
}

Port_16Bit::Port_16Bit(uint16_t portnumber)
    : Port(portnumber) {}

Port_16Bit::~Port_16Bit() {}

void Port_16Bit::Write(uint16_t data)
{
    asm volatile("outw %0, %1"
                 :
                 : "a"(data), "Nd"(portnumber));
}

uint16_t Port_16Bit::Read()
{
    uint16_t result;
    asm volatile("inw %1, %0"
                 : "=a"(result)
                 : "Nd"(portnumber));
}

Port_32Bit::Port_32Bit(uint16_t portnumber)
    : Port(portnumber) {}

Port_32Bit::~Port_32Bit() {}

void Port_32Bit::Write(uint32_t data)
{
    asm volatile("outl %0, %1"
                 :
                 : "a"(data), "Nd"(portnumber));
}

uint32_t Port_32Bit::Read()
{
    uint32_t result;
    asm volatile("inl %1, %0"
                 : "=a"(result)
                 : "Nd"(portnumber));
}