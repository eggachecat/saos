#ifndef __GDT_H
#define __GDT_H

#include "types.h"

class GlobalDescriptorTable
{
public:
    class SegmentDescriptor
    {
    private:
        uint16_t limit_lo;
        uint16_t base_lo;
        uint8_t base_hil;
        uint8_t type;
        uint8_t flags_limit_hi;
        uint8_t base_vhi;
    public:
        SegmentDescriptor(uint32_t base, uint32_t limit, uint8_t type);
        uint32_t Base();
        uint32_t Limit();

    } __attribute__((packed)); // wtf
/* wtf -- oh yes
    When a modern computer reads from or writes to a memory address, it will do this in word sized chunks (e.g. 4 byte chunks on a 32-bit system). 
    Data alignment means putting the data at a memory offset equal to some multiple of the word size, 
    which increases the system's performance due to the way the CPU handles memory.

    To align the data, it may be necessary to insert some meaningless bytes 
    between the end of the last data structure and the start of the next, which is data structure padding.

    No padding! (exact size not block-atom-size)
*/


SegmentDescriptor nullSegmentSelector;
SegmentDescriptor unusedSegmentSelector;
SegmentDescriptor codeSegmentSelector;
SegmentDescriptor dataSegmentSelector;

public:
    GlobalDescriptorTable();
    ~GlobalDescriptorTable();

    uint16_t CodeSegmentSelector();
    uint16_t DataSegmentSelector();

};

#endif