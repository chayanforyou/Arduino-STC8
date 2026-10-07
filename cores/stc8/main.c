#include "Arduino.h"

extern void setup(void);
extern void loop(void);

// Initialize system
void init(void)
{
    clock_init();
}

// Main entry point
void main(void)
{
    init();
    setup();

    while (1)
    {
        loop();
    }
}

// Provides __sdcc_call_dptr for indirect function pointer calls
void _sdcc_call_dptr(void) __naked {
    __asm
        clr a
        jmp @a+dptr
    __endasm;
}
