



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