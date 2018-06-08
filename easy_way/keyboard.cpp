#include "keyboard.h"

void printf(char *);
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
    while (commandport.Read() & 0x1)
    {
        dataport.Read();
    }
    commandport.Write(0xAE); // activate interrupts
    commandport.Write(0x20); // give us your current state
    uint8_t status = (dataport.Read() | 1) & ~0x10;
    commandport.Write(0x60); // set state
    dataport.Write(status);  // write back

    dataport.Write(0xF4); // really activate the keyboard
}
KeyboardDriver::~KeyboardDriver()
{
}

uint32_t KeyboardDriver::HandleInterrupt(uint32_t esp)
{
    uint8_t key = dataport.Read();

    if (handler == 0)
    {
        return esp;
    }

    if (key < 0x80) // only release ignore keep pressing
    {
        switch (key)
        {
        case 0xFA:
            break;

        case 0x45:
            break;

        case 0x1E:
            printf("a");
            this->handler->OnKeyDown('a');
            break;
        default:
            char *foo = "KEYBOARD 0x00 ";
            printfHex(key);
            break;
        }
    }

    return esp;
}