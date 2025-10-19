#include <drivers/mouse.h>
using namespace saos::common;
using namespace saos::drivers;
using namespace saos::hardwares;

// 声明外部函数
void printf(const char *);
void printfHex(uint8_t);

MouseEventHandler::MouseEventHandler()
{
}
void MouseEventHandler::OnActivate() {}

void MouseEventHandler::OnMouseDown(uint8_t button) {}
void MouseEventHandler::OnMouseUp(uint8_t button) {}
void MouseEventHandler::onMouseMove(int x, int y) {}
MouseDriver::MouseDriver(InterruptManager *manager, MouseEventHandler *handler)
    : InterruptHandler(0x2C, manager),
      dataport(0x60),
      commandport(0x64)
{
    this->handler = handler;
}
MouseDriver::~MouseDriver()
{
}
void MouseDriver::Activate()
{
    printf("Mouse driver activating...\n");
    offset = 0;
    buttons = 0;
    uint16_t *VideoMemory = (uint16_t *)0xb8000;
    VideoMemory[80 * 12 + 40] = ((VideoMemory[80 * 12 + 40] & 0xF000) >> 4) | ((VideoMemory[80 * 12 + 40] & 0x0F00) << 4) | ((VideoMemory[80 * 12 + 40] & 0x00FF) >> 4);

    printf("Activating mouse interrupts...\n");
    commandport.Write(0xA8); // activate interrupts
    commandport.Write(0x20); // give us your current state
    uint8_t status = dataport.Read() | 2;
    printf("Mouse status: 0x");
    printfHex(status);
    printf("\n");
    commandport.Write(0x60); // set state
    dataport.Write(status);  // write back

    printf("Enabling mouse device...\n");
    commandport.Write(0xD4);
    dataport.Write(0xF4); // really activate the mouse
    uint8_t response = dataport.Read();
    printf("Mouse response: 0x");
    printfHex(response);
    printf("\n");
    printf("Mouse driver activated!\n");
}
uint32_t MouseDriver::HandleInterrupt(uint32_t esp)
{
    uint8_t status = commandport.Read();
    printf("Mouse interrupt! Status: 0x");
    printfHex(status);
    printf("\n");
    
    // if the sixth bit of the status is one
    // there is actual data to read
    if (!(status & 0x20))
    {
        printf("No mouse data available\n");
        return esp;
    }

    buffer[offset] = dataport.Read();
    printf("Mouse data[");
    if (offset == 0) printf("0");
    else if (offset == 1) printf("1"); 
    else printf("2");
    printf("]: 0x");
    printfHex(buffer[offset]);
    printf("\n");
    
    offset = (offset + 1) % 3;

    if (handler == 0)
    {
        printf("No mouse handler!\n");
        return esp;
    }

    if (offset == 0)
    {
        printf("Processing mouse packet: [0x");
        printfHex(buffer[0]);
        printf(", 0x");
        printfHex(buffer[1]);
        printf(", 0x");
        printfHex(buffer[2]);
        printf("]\n");
        
        // buffer[1]: move at x-axis
        // buffer[2]: move at y-axis

        handler->onMouseMove(buffer[1], -buffer[2]);

        for (uint8_t i = 0; i < 3; i++)
        {
            if ((buffer[0] & (0x01 << i)) != (buttons & (0x01 << i)))
            { // i th button is pressed
                if (buttons & (0x1 << i))
                {
                    handler->OnMouseUp(i + 1);
                }
                else
                {
                    handler->OnMouseDown(i + 1);
                }
            }
        }
        buttons = buffer[0];
    }

    return esp;
}