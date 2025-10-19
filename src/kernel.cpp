#include <common/types.h>
#include <gdt.h>
#include <hardwares/port.h>
#include <hardwares/interrupts.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/driver.h>
#include <hardwares/pci.h>
#include <drivers/vga.h>
using namespace saos;
using namespace saos::common;
using namespace saos::drivers;
using namespace saos::hardwares;

// 串口调试输出函数 - 调试信息会显示在终端中
void serial_printf(const char *str)
{
    Port_8Bit serial_port(0x3F8);  // COM1串口 - 移除static关键字
    
    for (int i = 0; str[i] != '\0'; ++i)
    {
        serial_port.Write(str[i]);
    }
}

// 使用vga来打印
// vag的打印方法很简单: 在特定的内存地址放入你要写的即可
void printf(const char *str)
{
    // VGA文本模式显存起始地址：0xB8000
    // 每个字符占用2字节：[属性字节][字符字节]
    static uint16_t *VideoMemory = (uint16_t *)0xb8000;
    static uint8_t x = 0, y = 0;  // 当前光标位置
    
    for (int i = 0; str[i] != '\0'; ++i)
    {
        switch (str[i])
        {
        case '\n':  // 换行符处理
            y++;
            x = 0;
            break;
        default:
            // 保持原有颜色属性(高8位)，只更新字符(低8位)
            VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0xFF00) | str[i];
            x++;
            break;
        }

        // 处理行末换行
        if (x >= 80)
        {
            y++;
            x = 0;
        }
        
        // 处理屏幕满的情况：清屏并重置光标
        if (y >= 25)
        {
            for (y = 0; y < 25; y++)
                for (x = 0; x < 80; x++)  // 修复：应该是80列而不是90
                    VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0xFF00) | ' ';

            x = 0;
            y = 0;
        }
    }
}

void printfHex(uint8_t key)
{
    char *foo = "00";
    char *hex = "0123456789ABCDEF";
    foo[0] = hex[(key >> 4) & 0x0F];
    foo[1] = hex[key & 0x0F];

    printf(foo);
}

class PrintKeyboardEventHandler : public KeyboardEventHandler
{
  public:
    void OnKeyDown(char c)
    {
        printf("Key pressed: ");
        serial_printf("Key pressed: ");
        char key_str[2] = {c, '\0'};
        printf(key_str);
        serial_printf(key_str);
        printf("\n");
        serial_printf("\n");
    }
};

class MouseToConsole : public MouseEventHandler
{
public:
    MouseToConsole() : x(40), y(12)  // 初始化鼠标光标在屏幕中央
    {
        static uint16_t *VideoMemory = (uint16_t *)0xb8000;

        /*
         * VGA文本模式颜色格式 (每个字符2字节):
         * 高字节: [闪烁][背景色3位][前景色4位] 
         * 低字节: [字符ASCII码]
         * 
         * 这里通过颜色反转来创建光标效果：
         * - 交换前景色和背景色的位置来实现反色显示
         */
        VideoMemory[80 * y + x] = ((VideoMemory[80 * y + x] & 0xF000) >> 4) | 
                                  ((VideoMemory[80 * y + x] & 0x0F00) << 4) | 
                                  ((VideoMemory[80 * y + x] & 0x00FF));
    }
    
    void OnActivate()
    {
        // 鼠标激活时的处理（当前为空）
    }
    
    int8_t x, y; // 鼠标光标位置

    void OnMouseDown(uint8_t button)
    {
        // 鼠标按键按下事件
        printf("Mouse button ");
        char btn[2] = {'0' + button, '\0'};
        printf(btn);
        printf(" pressed\n");
    }
    
    void OnMouseUp(uint8_t button)
    {
        // 鼠标按键释放事件
        printf("Mouse button ");
        char btn[2] = {'0' + button, '\0'};
        printf(btn);
        printf(" released\n");
    }
    
    void onMouseMove(int x_offset, int y_offset)
    {
        static uint16_t *VideoMemory = (uint16_t *)0xb8000;

        // 添加调试信息：显示鼠标移动偏移量（同时输出到屏幕和串口）
        if (x_offset != 0 || y_offset != 0) {
            printf("Mouse moved: dx=");
            serial_printf("Mouse moved: dx=");
            
            // 简单的数字转字符串并打印
            char dx_str[10];
            int dx = x_offset;
            if (dx < 0) { 
                printf("-"); 
                serial_printf("-");
                dx = -dx; 
            }
            int i = 0;
            if (dx == 0) { dx_str[i++] = '0'; }
            else {
                while (dx > 0) {
                    dx_str[i++] = '0' + (dx % 10);
                    dx /= 10;
                }
            }
            // 反向打印数字
            for (int j = i-1; j >= 0; j--) {
                char temp[2] = {dx_str[j], '\0'};
                printf(temp);
                serial_printf(temp);
            }
            
            printf(" dy=");
            serial_printf(" dy=");
            int dy = y_offset;
            if (dy < 0) { 
                printf("-"); 
                serial_printf("-");
                dy = -dy; 
            }
            i = 0;
            if (dy == 0) { dx_str[i++] = '0'; }
            else {
                while (dy > 0) {
                    dx_str[i++] = '0' + (dy % 10);
                    dy /= 10;
                }
            }
            // 反向打印数字
            for (int j = i-1; j >= 0; j--) {
                char temp[2] = {dx_str[j], '\0'};
                printf(temp);
                serial_printf(temp);
            }
            printf("\n");
            serial_printf("\n");
        }

        // 根据鼠标移动偏移量更新光标位置
        x += x_offset;
        if (x < 0) x = 0;        // 防止超出左边界
        if (x >= 80) x = 79;     // 防止超出右边界
        
        y += y_offset;
        if (y < 0) y = 0;        // 防止超出上边界  
        if (y >= 25) y = 24;     // 防止超出下边界 (修复：应该是24而不是25)

        // 在新位置显示反色光标
        VideoMemory[80 * y + x] = ((VideoMemory[80 * y + x] & 0xF000) >> 4) | 
                                  ((VideoMemory[80 * y + x] & 0x0F00) << 4) | 
                                  ((VideoMemory[80 * y + x] & 0x00FF));
    }
};

/*
 * C++全局构造函数调用机制
 * 在C++中，全局对象的构造函数需要在main函数之前调用
 * 链接器会将所有构造函数指针放在特殊段中(.init_array)
 * start_ctors和end_ctors标记了这个数组的开始和结束
 */
typedef void (*constructor)();
extern "C" constructor start_ctors;  // 构造函数数组开始标记
extern "C" constructor end_ctors;    // 构造函数数组结束标记

extern "C" void callConstructors()
{
    // 遍历并调用所有全局对象的构造函数
    for (constructor *i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}

extern "C" void kernelMain(void *multiboot_structure, uint32_t magicnumber)
{
    printf("Fuck what the fuck Hello world! \n--- sunao");

    // 初始化全局描述符表(GDT) - 内存段管理
    GlobalDescriptorTable gdt;
    
    // 初始化中断管理器 - 处理硬件中断
    InterruptManager interrupts(&gdt);

    // 创建驱动管理器 - 统一管理所有设备驱动
    DriverManager drvManager;
    printf("Driver manager created\n");
    serial_printf("Driver manager created\n");

    // 初始化键盘驱动
    printf("Initializing keyboard driver...\n");
    serial_printf("Initializing keyboard driver...\n");
    PrintKeyboardEventHandler kbhandler;  // 键盘事件处理器
    KeyboardDriver keyboard(&interrupts, &kbhandler);
    drvManager.AddDriver(&keyboard);
    printf("Keyboard driver added\n");
    serial_printf("Keyboard driver added\n");

    // 初始化鼠标驱动  
    printf("Initializing mouse driver...\n");
    serial_printf("Initializing mouse driver...\n");
    MouseToConsole mousehandler;          // 鼠标事件处理器
    MouseDriver mouse(&interrupts, &mousehandler);
    drvManager.AddDriver(&mouse);
    printf("Mouse driver added\n");
    serial_printf("Mouse driver added\n");

    // 初始化PCI控制器 - 自动检测和配置PCI设备
    PeripheralComponentInterconnectController PCIController;
    PCIController.SelectDrivers(&drvManager, &interrupts);

    // 初始化VGA图形驱动
    VideoGraphicsArray vga;

    // 激活所有驱动程序
    printf("Activating all drivers...\n");
    serial_printf("Activating all drivers...\n");
    drvManager.ActivateAll();
    printf("All drivers activated!\n");
    serial_printf("All drivers activated!\n");
    
    // 启用中断处理 - 开始响应硬件事件
    printf("Enabling interrupts...\n");
    serial_printf("Enabling interrupts...\n");
    interrupts.Activate();
    printf("Interrupts enabled! System ready for input.\n");
    serial_printf("Interrupts enabled! System ready for input.\n");
    
    // 测试中断系统是否工作
    printf("Testing interrupt system...\n");
    serial_printf("Testing interrupt system...\n");
    printf("Please try pressing keys or moving mouse now!\n");
    serial_printf("Please try pressing keys or moving mouse now!\n");
    
    // 切换到VGA图形模式：320x200像素，8位色彩
    vga.SetMode(320, 200, 8);

    // 绘制蓝色背景
    for (int32_t y = 0; y < 200; y++)
    {
        for (int32_t x = 0; x < 320; x++)
        {
            vga.PutPixel(x, y, 0x00, 0x00, 0xA8);  // RGB: (0, 0, 168) 蓝色
        }
    }

    // 主循环 - 系统保持运行状态
    while (1)
        ;
}