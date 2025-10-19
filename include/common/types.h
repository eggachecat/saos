/**
 * @file types.h
 * @brief SAOS操作系统基础数据类型定义
 * 
 * 【大局定位】
 * 这是整个操作系统的基石文件，定义了所有模块使用的统一数据类型。
 * 在C++中，基本类型（int, short等）的大小依赖于编译器和平台。
 * 为了保证跨平台的一致性和可移植性，我们显式定义固定大小的类型。
 * 
 * 【设计模式】类型别名模式（Type Aliasing Pattern）
 * - 将平台相关的类型（char, int等）映射为平台无关的类型（uint8_t等）
 * - 提供统一的抽象层，隔离底层差异
 * 
 * 【使用示例】
 * ```cpp
 * uint8_t port_number = 0x60;     // 明确是8位无符号整数
 * uint32_t memory_address = 0xB8000; // 32位地址
 * int16_t mouse_offset = -5;      // 带符号的16位偏移量
 * ```
 * 
 * 【为什么需要？】
 * 1. 明确性：uint8_t比unsigned char更清楚表达"8位无符号整数"的意图
 * 2. 可移植性：在不同架构上保持相同大小
 * 3. 可读性：代码意图更清晰，减少歧义
 */

#ifndef __SAOS__COMMON__TYPES_H
#define __SAOS__COMMON__TYPES_H

namespace saos
{
namespace common
{
/**
 * 8位整数类型
 * - int8_t: 有符号 (-128 到 127)
 * - uint8_t: 无符号 (0 到 255)
 * 
 * 使用场景：
 * - 端口数据读写（I/O操作通常是字节级）
 * - 扫描码、ASCII字符
 * - 颜色分量（RGB值）
 */
typedef char int8_t;
typedef unsigned char uint8_t;

/**
 * 16位整数类型
 * - int16_t: 有符号 (-32768 到 32767)
 * - uint16_t: 无符号 (0 到 65535)
 * 
 * 使用场景：
 * - 端口号（0x0000-0xFFFF）
 * - GDT/IDT段选择子
 * - VGA文本模式字符（属性+字符，2字节）
 * - 鼠标/键盘的相对偏移量
 */
typedef short int16_t;
typedef unsigned short uint16_t;

/**
 * 32位整数类型
 * - int32_t: 有符号 (-2^31 到 2^31-1)
 * - uint32_t: 无符号 (0 到 2^32-1)
 * 
 * 使用场景：
 * - 内存地址（32位保护模式）
 * - PCI配置地址
 * - 段描述符的基址和限长
 * - 栈指针（esp）
 * - VGA像素坐标
 */
typedef int int32_t;
typedef unsigned int uint32_t;

/**
 * 64位整数类型
 * - int64_t: 有符号 (-2^63 到 2^63-1)
 * - uint64_t: 无符号 (0 到 2^64-1)
 * 
 * 使用场景：
 * - 未来扩展（64位模式）
 * - 大数计算
 * - 时间戳（毫秒级精度）
 * 
 * 注意：当前32位保护模式下，64位操作可能需要多个CPU指令
 */
typedef long long int int64_t;
typedef unsigned long long int uint64_t;

} // namespace common
} // namespace saos

#endif