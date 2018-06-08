#include "types.h"
#include "gdt.h"
#include "port.h"
#include "interrupts.h"
#include "keyboard.h"
#include "mouse.h"
#include "driver.h"
void printf(char *str)
{
    static uint16_t *VideoMemory = (uint16_t *)0xb8000;
    static uint8_t x = 0, y = 0;
    // [color, text]
    // we only edit text
    for (int i = 0; str[i] != '\0'; ++i)
    {
        switch (str[i])
        {
        case '\n':
            y++;
            x = 0;
            break;
        default:
            VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0xFF00) | str[i];
            x++;
            break;
        }

        if (x >= 80)
        {
            y++;
            x = 0;
        }
        if (y >= 25)
        {
            for (y = 0; y < 25; y++)
                for (x = 0; x < 90; x++)
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
        char *foo = "";
        foo[0] = c;
        printf(foo);
    }
};

class MouseToConsole : public MouseEventHandler
{

public:
    MouseToConsole() : x(40), y(12)
    {

        static uint16_t *VideoMemory = (uint16_t *)0xb8000;

        // again
        // each entry in the video memory is two byte
        // the first byte is for the color
        //      where the first four bits are for
        // the second byte is for the character code
        VideoMemory[80 * y + x] = ((VideoMemory[80 * y + x] & 0xF000) >> 4) | ((VideoMemory[80 * y + x] & 0x0F00) << 4) | ((VideoMemory[80 * y + x] & 0x00FF) >> 4);
    }
    void OnActivate()
    {
    }
    int8_t x, y; // cursor; init at center ...

    void OnMouseDown(uint8_t button)
    {
    }
    void OnMouseUp(uint8_t button)
    {
    }
    void onMouseMove(int x_offset, int y_offset)
    {
        static uint16_t *VideoMemory = (uint16_t *)0xb8000;

        // buffer[1]: move at x-axis
        // buffer[2]: move at y-axis
        x += x_offset;
        if (x < 0)
            x = 0;
        if (x >= 80)
            x = 79;
        y += y_offset;
        if (y < 0)
            y = 0;
        if (y >= 25)
            y = 25;

        VideoMemory[80 * y + x] = ((VideoMemory[80 * y + x] & 0xF000) >> 4) | ((VideoMemory[80 * y + x] & 0x0F00) << 4) | ((VideoMemory[80 * y + x] & 0x00FF) >> 4);
    }
};

/*
    I do not know what the hack is this..
*/
typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for (constructor *i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}

extern "C" void kernelMain(void *multiboot_structure, uint32_t magicnumber)
{
    printf("Hello world! \n--- sunao");

    GlobalDescriptorTable gdt;
    InterruptManager interrupts(&gdt);

    printf("I hate you forever ! \n--- chenhao - 0");

    DriverManager drvManager;

    PrintKeyboardEventHandler kbhandler;
    KeyboardDriver keyboard(&interrupts, &kbhandler);
    drvManager.AddDriver(&keyboard);

    MouseToConsole mousehandler;
    MouseDriver mouse(&interrupts, &mousehandler);
    drvManager.AddDriver(&mouse);

    printf("I hate you forever ! \n--- chenhao - 1");

    drvManager.ActivateAll();
    printf("I hate you forever ! \n--- chenhao - 2");
    interrupts.Activate();

    while (1)
        ;
}