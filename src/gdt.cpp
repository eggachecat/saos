#include <gdt.h>
using namespace saos;
using namespace saos::common;

GlobalDescriptorTable::GlobalDescriptorTable()
    : nullSegmentSelector(0, 0, 0),
      unusedSegmentSelector(0, 0, 0),
      codeSegmentSelector(0, 64 * 1024 * 1024, 0x9A),
      dataSegmentSelector(0, 64 * 1024 * 1024, 0x92)
{
    uint32_t i[2];
    i[0] = sizeof(GlobalDescriptorTable) << 16;
    i[1] = (uint32_t)this;

    asm volatile("lgdt (%0)"
                 :
                 : "p"(((uint8_t *)i) + 2));
}

GlobalDescriptorTable::~GlobalDescriptorTable()
{
}

uint16_t GlobalDescriptorTable::DataSegmentSelector()
{
    return (uint8_t *)&dataSegmentSelector - (uint8_t *)this;
}

uint16_t GlobalDescriptorTable::CodeSegmentSelector()
{
    return (uint8_t *)&codeSegmentSelector - (uint8_t *)this;
}

GlobalDescriptorTable::SegmentDescriptor::SegmentDescriptor(uint32_t base, uint32_t limit, uint8_t flags)
{
    /*
        GDT段描述符的8字节结构（从低位到高位）:
        字节0-1: limit的低16位 (段长度限制)
        字节2-4: base的低24位 (段基址)  
        字节5: flags (访问权限标志)
        字节6: 高4位是limit的高4位，低4位是额外标志
        字节7: base的高8位
    */
    uint8_t *target = (uint8_t *)this;
    
    /*
        处理段长度限制(limit):
        如果limit > 65536，则启用4KB页面粒度（乘以4096）
        否则使用字节粒度
    */
    if (limit <= 65536)
    {
        // 字节粒度：设置G位=0 (0x40 = 01000000b)
        target[6] = 0x40;
    }
    else
    {
        // 4KB页面粒度：设置G位=1 (0xC0 = 11000000b)
        if (limit & 0xFFF != 0xFFF)
            limit = (limit >> 12) - 1;  // 转换为页面数
        else
            limit = limit >> 12;

        target[6] = 0xc0;  // G=1, D/B=1 (32位段)
    }

    // 设置limit的各个位
    target[0] = limit & 0xFF;           // limit[7:0]
    target[1] = (limit >> 8) & 0xFF;    // limit[15:8] 
    target[6] |= (limit >> 16) & 0xF;   // limit[19:16] 存储在字节6的低4位

    // 设置base address的各个位
    target[2] = base & 0xFF;            // base[7:0]
    target[3] = (base >> 8) & 0xFF;     // base[15:8]
    target[4] = (base >> 16) & 0xFF;    // base[23:16]
    target[7] = (base >> 24) & 0xFF;    // base[31:24]

    // 设置访问权限标志
    target[5] = flags;
}

uint32_t GlobalDescriptorTable::SegmentDescriptor::Base()
{
    uint8_t *target = (uint8_t *)this;
    uint32_t result = target[7];
    result = (result << 8) + target[4];
    result = (result << 8) + target[3];
    result = (result << 8) + target[2];
    return result;
}

uint32_t GlobalDescriptorTable::SegmentDescriptor::Limit()
{
    uint8_t *target = (uint8_t *)this;
    uint32_t result = target[6] & 0xF;
    result = (result << 8) + target[1];
    result = (result << 8) + target[0];

    if ((target[6] & 0xc0) == 0xC0)
        result = (result << 12) | 0xFFF;

    return result;
}