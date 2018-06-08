#include "mouse.h"

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
    offset = 0;
    buttons = 0;
    uint16_t *VideoMemory = (uint16_t *)0xb8000;
    VideoMemory[80 * 12 + 40] = ((VideoMemory[80 * 12 + 40] & 0xF000) >> 4) | ((VideoMemory[80 * 12 + 40] & 0x0F00) << 4) | ((VideoMemory[80 * 12 + 40] & 0x00FF) >> 4);

    commandport.Write(0xA8); // activate interrupts
    commandport.Write(0x20); // give us your current state
    uint8_t status = dataport.Read() | 2;
    commandport.Write(0x60); // set state
    dataport.Write(status);  // write back

    commandport.Write(0xD4);
    dataport.Write(0xF4); // really activate the keyboard
    dataport.Read();
}
void printf(char *);
uint32_t MouseDriver::HandleInterrupt(uint32_t esp)
{
    uint8_t status = commandport.Read();
    // if the sixth bit of the status is one
    // there is actual data to read
    if (!(status & 0x20))
    {
        return esp;
    }

    buffer[offset] = dataport.Read();
    offset = (offset + 1) % 3;

    if (handler == 0)
    {
        return esp;
    }

    if (offset == 0)
    {
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