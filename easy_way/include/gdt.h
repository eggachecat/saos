#ifndef __SAOS__GDT_H
#define __SAOS__GDT_H

#include <common/types.h>
namespace saos
{
class GlobalDescriptorTable
{
  public:
    class SegmentDescriptor
    {
      private:
        saos::common::uint16_t limit_lo;
        saos::common::uint16_t base_lo;
        saos::common::uint8_t base_hil;
        saos::common::uint8_t type;
        saos::common::uint8_t flags_limit_hi;
        saos::common::uint8_t base_vhi;

      public:
        SegmentDescriptor(saos::common::uint32_t base, saos::common::uint32_t limit, saos::common::uint8_t type);
        saos::common::uint32_t Base();
        saos::common::uint32_t Limit();

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

    saos::common::uint16_t CodeSegmentSelector();
    saos::common::uint16_t DataSegmentSelector();
};
} // namespace saos

#endif