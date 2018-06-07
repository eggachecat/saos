#include "mouse.h"

MouseDriver::MouseDriver(InterruptManager *manager)
    : InterruptHandler(0x2C, manager),
      dataport(0x60),
      commandport(0x64)
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
MouseDriver::~MouseDriver()
{
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
    static uint16_t *VideoMemory = (uint16_t *)0xb8000;

    static int8_t x = 40, y = 12; // cursor; init at center ...
    buffer[offset] = dataport.Read();
    offset = (offset + 1) % 3;

    // again
    // each entry in the video memory is two byte
    // the first byte is for the color
    //      where the first four bits are for
    // the second byte is for the character code
    VideoMemory[80 * y + x] = ((VideoMemory[80 * y + x] & 0xF000) >> 4) | ((VideoMemory[80 * y + x] & 0x0F00) << 4) | ((VideoMemory[80 * y + x] & 0x00FF) >> 4);

    if (offset == 0)
    {

        // buffer[1]: move at x-axis
        // buffer[2]: move at y-axis
        x += buffer[1];
        y -= buffer[2];

        if (x < 0)
            x = 0;
        if (x >= 80)
            x = 79;
        if (y < 0)
            y = 0;
        if (y >= 25)
            y = 25;

        VideoMemory[80 * y + x] = ((VideoMemory[80 * y + x] & 0xF000) >> 4) | ((VideoMemory[80 * y + x] & 0x0F00) << 4) | ((VideoMemory[80 * y + x] & 0x00FF) >> 4);

        for (uint8_t i = 0; i < 3; i++)
        {
            if ((buffer[0] & (0x01 << i)) != (buttons & (0x01 << i)))
            { // i th button is pressed
                VideoMemory[80 * y + x] = ((VideoMemory[80 * y + x] & 0xF000) >> 4) | ((VideoMemory[80 * y + x] & 0x0F00) << 4) | ((VideoMemory[80 * y + x] & 0x00FF) >> 4);
            }
        }
        buttons = buffer[0];
    }

    return esp;
}