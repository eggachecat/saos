.set IRQ_BASE, 0x20
.section .text

.extern _ZN4saos9hardwares16InterruptManager15HandleInterruptEhj

.macro HandleException num
.global _ZN164saos9hardwaresInterruptManager19HandleException\num\()Ev
_ZN4saos9hardwares16InterruptManager19HandleException\num\()Ev:
    movb $\num, (interruptnumber)
    jmp int_bottom
.endm


# I think this is where to implement the method
# HandleInterruptRequest0x00 in interrupts.h
# so num is like 0x00
# and we define HandleInterruptRequest0x00 by:
#   jump to int_bottom
#   execute InterruptManager.HandleInterrupt(interruptnumber, esp)
#   which is also defined in interrupts.h and implemented in interrupts.cpp

.macro HandleInterruptRequest num
.global _ZN4saos9hardwares16InterruptManager26HandleInterruptRequest\num\()Ev
_ZN4saos9hardwares16InterruptManager26HandleInterruptRequest\num\()Ev:
    movb $\num + IRQ_BASE, (interruptnumber)
    jmp int_bottom
.endm

HandleInterruptRequest 0x00
HandleInterruptRequest 0x01 # for the keyboard
HandleInterruptRequest 0x0C # for the mouse

int_bottom:

# push something into the stack
# and call the function

# saving current status
    pusha
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    push %esp
    push (interruptnumber)
    call _ZN4saos9hardwares16InterruptManager15HandleInterruptEhj
    # addl $5, %esp
    movl %eax, %esp

    popl %gs
    popl %fs
    popl %es
    popl %ds
    popa

    iret


.data
    interruptnumber: .byte 0
