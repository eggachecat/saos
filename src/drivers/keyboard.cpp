#include <drivers/keyboard.h>
using namespace saos::common;
using namespace saos::drivers;
using namespace saos::hardwares;

// 声明外部函数
void printf(const char *);
void printfHex(uint8_t);

KeyboardEventHandler::KeyboardEventHandler()
{
}
void KeyboardEventHandler::OnKeyUp(char)
{
}
void KeyboardEventHandler::OnKeyDown(char)
{
}
KeyboardDriver::KeyboardDriver(InterruptManager *manager, KeyboardEventHandler *handler)
    : InterruptHandler(0x21, manager),
      dataport(0x60),
      commandport(0x64)
{
    this->handler = handler;
}

void KeyboardDriver::Activate()
{
    printf("Keyboard driver activating...\n");
    /*
     * 激活键盘驱动的步骤:
     * 1. 清空键盘缓冲区中的所有旧数据
     * 2. 启用键盘中断
     * 3. 配置键盘控制器
     * 4. 启用键盘设备
     */
    
    // 清空键盘缓冲区：读取所有待处理的数据直到缓冲区为空
    printf("Clearing keyboard buffer...\n");
    while (commandport.Read() & 0x1)  // 检查输出缓冲区状态位
    {
        dataport.Read();  // 读取并丢弃缓冲区中的数据
    }
    
    printf("Enabling keyboard interface...\n");
    commandport.Write(0xAE); // 发送"启用键盘接口"命令
    commandport.Write(0x20); // 请求读取当前配置字节
    
    // 读取当前状态并修改：启用中断，禁用鼠标中断
    uint8_t status = (dataport.Read() | 1) & ~0x10;
    printf("Keyboard status configured: 0x");
    printfHex(status);
    printf("\n");
    
    commandport.Write(0x60); // 发送"写入配置字节"命令  
    dataport.Write(status);  // 写入新的配置

    printf("Enabling keyboard scanning...\n");
    dataport.Write(0xF4);    // 向键盘发送"启用扫描"命令
    printf("Keyboard driver activated!\n");
}
KeyboardDriver::~KeyboardDriver()
{
}

uint32_t KeyboardDriver::HandleInterrupt(uint32_t esp)
{
    // 从键盘数据端口读取扫描码
    uint8_t key = dataport.Read();

    // 添加调试信息：显示所有接收到的扫描码
    printf("Keyboard interrupt! Scancode: 0x");
    printfHex(key);
    printf("\n");

    if (handler == 0)
    {
        printf("No keyboard handler!\n");
        return esp;  // 没有事件处理程序，直接返回
    }

    /*
     * 键盘扫描码处理:
     * - 扫描码 < 0x80: 按键按下 (make code)
     * - 扫描码 >= 0x80: 按键释放 (break code = make code + 0x80)
     * 我们只处理按键按下事件
     */
    if (key < 0x80) 
    {
        printf("Key pressed! Processing...\n");
        switch (key)
        {
        case 0xFA:  // 键盘确认码，忽略
            printf("Keyboard ACK received\n");
            break;

        case 0x45:  // Num Lock 或其他特殊键，暂时忽略
            printf("Special key (Num Lock)\n");
            break;

        case 0x1E:  // 'A'键的扫描码
            printf("A key detected!\n");
            this->handler->OnKeyDown('a');
            break;
            
        default:
            // 显示未处理的扫描码（用于调试）
            printf("Unhandled key: 0x");
            printfHex(key);
            printf("\n");
            break;
        }
    } else {
        printf("Key released (scancode >= 0x80)\n");
    }

    return esp;
}