void exit() 
{
    asm("mov $60, %eax\n"
        "mov $24567837, %edi\n"
        "syscall\n");
}

extern "C" void _start()
{
    exit();
}
