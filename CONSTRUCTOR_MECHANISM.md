# C++ 全局构造函数机制详解

## 概述
这个文档解释了 `start_ctors` 和 `end_ctors` 是如何工作的。

## 完整流程

```
┌─────────────────────────────────────────────────────────────────────────┐
│ 1. 编译阶段 (Compilation Phase)                                        │
└─────────────────────────────────────────────────────────────────────────┘

你的 C++ 代码:
    class MyClass {
    public:
        MyClass() { /* 构造函数 */ }
    };
    
    MyClass globalObject;  // 全局对象

编译器做什么:
    1. 编译 MyClass 的构造函数
    2. 生成一个特殊的初始化函数 (如 __cxx_global_var_init_0)
    3. 在 .init_array 段中放入这个函数的指针

生成的目标文件 (.o):
    .text 段:         [MyClass::MyClass() 的机器码]
                     [__cxx_global_var_init_0 的机器码]
    .init_array 段:  [指向 __cxx_global_var_init_0 的指针]
    .data 段:        [globalObject 的存储空间]


┌─────────────────────────────────────────────────────────────────────────┐
│ 2. 链接阶段 (Linking Phase)                                            │
└─────────────────────────────────────────────────────────────────────────┘

链接器读取 linker.ld 脚本:
    .data :
    {
        start_ctors = .;                          ← 创建符号，值 = 当前地址
        KEEP(*( .init_array ));                   ← 收集所有 .init_array
        KEEP(*(SORT_BY_INIT_PRIORITY(...)));
        end_ctors = .;                            ← 创建符号，值 = 当前地址
        *(.data)
    }

链接器将多个 .o 文件的 .init_array 合并:
    driver.o   →  .init_array: [函数指针A]
    keyboard.o →  .init_array: [函数指针B]        合并
    mouse.o    →  .init_array: [函数指针C]    ─────────→
    kernel.o   →  .init_array: [函数指针D]

最终的可执行文件内存布局:
    地址 0x00102000: [其他 .data]
    地址 0x00102100: ← start_ctors 指向这里
                     [函数指针A] (4字节)
    地址 0x00102104: [函数指针B] (4字节)
    地址 0x00102108: [函数指针C] (4字节)
    地址 0x0010210C: [函数指针D] (4字节)
    地址 0x00102110: ← end_ctors 指向这里
                     [其他 .data]


┌─────────────────────────────────────────────────────────────────────────┐
│ 3. 运行阶段 (Runtime Phase)                                            │
└─────────────────────────────────────────────────────────────────────────┘

loader.s:
    call callConstructors    ← 在 kernelMain 之前调用

callConstructors() 执行:
    
    Step 1: i = &start_ctors = 0x00102100
            *i = [函数指针A]
            (*i)()  → 调用函数A，初始化 driver 中的全局对象
    
    Step 2: i++, i = 0x00102104
            *i = [函数指针B]
            (*i)()  → 调用函数B，初始化 keyboard 中的全局对象
    
    Step 3: i++, i = 0x00102108
            *i = [函数指针C]
            (*i)()  → 调用函数C，初始化 mouse 中的全局对象
    
    Step 4: i++, i = 0x0010210C
            *i = [函数指针D]
            (*i)()  → 调用函数D，初始化 kernel 中的全局对象
    
    Step 5: i++, i = 0x00102110 = &end_ctors
            循环结束

    返回到 loader.s

loader.s:
    call kernelMain    ← 现在所有全局对象都已初始化，可以安全使用了！
```

## 关键概念

### 1. 链接器符号 (Linker Symbol)
- **不是变量**：`start_ctors` 和 `end_ctors` 不占用存储空间
- **是地址标记**：它们只是内存地址的名字
- **由链接器创建**：在链接过程中根据 linker.ld 脚本生成

### 2. extern "C" 声明
```cpp
extern "C" constructor start_ctors;  // 声明一个链接器符号
```
- `extern "C"` 告诉编译器：这个符号不是在 C++ 代码中定义的
- 编译器不会期望找到 `start_ctors` 的定义
- 链接器会在链接阶段提供这个符号的地址

### 3. 获取地址
```cpp
&start_ctors  // 获取 start_ctors 符号的地址
```
- 因为 `start_ctors` 本身就是一个地址标记
- `&start_ctors` 获取的是数组首元素的地址

## 实际例子

假设你的代码中有：
```cpp
// keyboard.cpp
class Keyboard {
public:
    Keyboard() { /* 初始化键盘 */ }
};
Keyboard globalKeyboard;

// mouse.cpp  
class Mouse {
public:
    Mouse() { /* 初始化鼠标 */ }
};
Mouse globalMouse;
```

编译后：
1. `keyboard.o` 包含一个 `.init_array` 条目指向 Keyboard 的初始化函数
2. `mouse.o` 包含一个 `.init_array` 条目指向 Mouse 的初始化函数

链接后：
```
内存地址:
0x00102100: [指向 Keyboard 初始化函数]  ← start_ctors
0x00102104: [指向 Mouse 初始化函数]
0x00102108:                             ← end_ctors
```

运行时：
```cpp
callConstructors() {
    // i 从 0x00102100 遍历到 0x00102108
    // 调用 Keyboard 初始化函数 → globalKeyboard 被构造
    // 调用 Mouse 初始化函数 → globalMouse 被构造
}
```

## 为什么需要这个机制？

在普通的应用程序中，C++ 运行时库会自动处理全局对象的构造。但在操作系统内核中：
- **没有运行时库**：我们不能依赖标准的 C++ 运行时
- **必须手动初始化**：我们需要显式调用所有构造函数
- **使用链接器魔法**：通过 linker.ld 脚本和符号来实现

## 调试技巧

如果你想看实际的构造函数地址，可以在 `callConstructors()` 中添加：
```cpp
extern "C" void callConstructors()
{
    serial_printf("start_ctors address: 0x%x\n", &start_ctors);
    serial_printf("end_ctors address: 0x%x\n", &end_ctors);
    serial_printf("Number of constructors: %d\n", 
                  ((uint32_t)&end_ctors - (uint32_t)&start_ctors) / sizeof(constructor));
    
    for (constructor *i = &start_ctors; i != &end_ctors; i++)
    {
        serial_printf("Calling constructor at: 0x%x\n", *i);
        (*i)();
    }
}
```

## 参考资料
- [GNU ld 链接器脚本文档](https://sourceware.org/binutils/docs/ld/Scripts.html)
- [C++ ABI 规范](https://itanium-cxx-abi.github.io/cxx-abi/abi.html)

