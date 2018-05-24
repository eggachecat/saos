void printf(char* str)
{
    unsigned short* VideoMemory = (unsigned short*) 0xb8000;
    // [color, text]
    // we only edit text
    for(int i = 0; str[i] != '\0'; ++i)
        VideoMemory[i] = (VideoMemory[i] & 0xFF00) | str[i];
}

extern "C" void kernelMain(void* multiboot_structure, unsigned int magicnumber)
{
    printf("Hello world! --- sunao");
    while(1);
}