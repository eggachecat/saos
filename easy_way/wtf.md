# WTF wiki

## wtf is `_ZN16InterruptManager15HandleInterruptEhj`

### Reference

    [Name Managling](https://en.wikipedia.org/wiki/Name_mangling)

### SimpleTF

bascially:
    leading _Z 
    lengthOf(identifier) + identifier
    ...

so...
    function `InterruptManager.HandleInterruptEhj` becomes..
    _Z 
    16(which is the length of word `InterruptManager`) InterruptManager
    15(which is the length of word `HandleInterruptEhj`) HandleInterruptEhj

## wtf is `macro`

### Reference:

    [Marco](https://en.wikipedia.org/wiki/Macro_(computer_science))

### SimpleTF
    