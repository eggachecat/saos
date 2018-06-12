



# Memory Segments

[---] [---] [---]

Code Segment ---  Data Segment

## GDT

### some thing like this

| starting | length | flags(code/data; executable/...) |

### Each entry contrains 8 bytes:(order from lower to higher)

    16bit: limit
    16bit: starting pointer(<===>64kb not enough!) 
    8bit: more ponter
    8bit: flags(access)
    4bit: more limit
    4bit: more flags
    8bit: more pointer

summary: 
    total: 64-bits
    --limit: 20bits
    --starting pointer: 32bits
    --flags: 12

*wtf*:
    limit would be multiply 2^12 (so together 32bits)
    iff the last bits are all set to 1

What a pain ! : we need to define this data structure


### Multipliter demultiplixer 

Bascially a switch


## IDT

### Each entry
    interrupt number
    handler(Ram where to jump to) 
    flags
    segments(related to handler?)
    access right(kernel space\user sapce)

*wtf*
    each function for each interrupt



## PCI



## Base Address registers

### Purpose

    set base address registers to communicate with different ports

    [] [] [] [ |type]

    type: 1 bit:
        1: 
            send byte individually

    port number:
        numer of 4