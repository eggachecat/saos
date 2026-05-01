ENTRY(loader)
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)

SECTIONS
{
  . = 0x0100000;

  .text :
  {
    *(.multiboot)
    *(.text*)
    *(.rodata)
  }

  .data  :
  {
    /* ====================================================================
     * C++全局构造函数数组
     * ====================================================================
     * 链接器脚本(Linker Script)的作用：
     *   定义程序在内存中的布局，以及如何组合各个目标文件的段
     *
     * 这里创建了一个特殊的数组，存储所有全局对象的构造函数指针
     */
    
    /* start_ctors 是一个符号(Symbol)，不是变量！
     * "start_ctors = ." 表示：
     *   - 创建一个名为 start_ctors 的符号
     *   - 它的值是当前位置计数器(.)的地址
     *   - 这标记了构造函数数组的开始地址
     * 
     * C++代码可以用 extern "C" 声明这个符号，然后获取它的地址
     */
    start_ctors = .;
    
    /* KEEP() 告诉链接器：即使这些段看起来没被使用，也不要丢弃
     * 
     * *( .init_array ) 表示：
     *   - 从所有输入文件(*)中
     *   - 收集名为 .init_array 的段
     *   - 放在这里
     * 
     * .init_array 段是什么？
     *   当你在C++中写：
     *     MyClass globalObject;  // 全局对象
     *   
     *   编译器会：
     *   1. 生成一个构造函数: __static_initialization_0()
     *   2. 创建一个函数指针指向这个构造函数
     *   3. 将这个函数指针放入 .init_array 段
     * 
     * 所以这一行把所有全局对象的构造函数指针收集到这里
     */
    KEEP(*( .init_array ));
    
    /* 这行处理优先级构造函数
     * SORT_BY_INIT_PRIORITY 会按照初始化优先级排序
     * .init_array.* 匹配如 .init_array.00100 这样的段
     * 
     * 某些构造函数可以指定优先级：
     *   __attribute__((init_priority(200)))
     * 这样可以控制初始化顺序
     */
    KEEP(*(SORT_BY_INIT_PRIORITY( .init_array.* )));
    
    /* end_ctors 标记构造函数数组的结束地址
     * 此时，start_ctors 和 end_ctors 之间的内存区域包含了
     * 所有全局对象构造函数的指针
     * 
     * 内存布局示意：
     *   start_ctors → [函数指针1] [函数指针2] [函数指针3] ... ← end_ctors
     *                 4字节       4字节       4字节
     */
    end_ctors = .;

    /* 然后是普通的已初始化数据段 */
    *(.data)
  }

  .bss  :
  {
    *(.bss)
  }

  /DISCARD/ : { *(.fini_array*) *(.comment) }
}