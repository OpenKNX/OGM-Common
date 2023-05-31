#include "Helper.h"

/*
 * Free Memory
 */
#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char* __brkval;
#endif // __arm__

int freeMemory()
{
    char top;
#ifdef __arm__
    return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
    return &top - __brkval;
#else  // __arm__
    return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif // __arm__
}

/*
 * Nuker
 */
#ifdef ARDUINO_ARCH_RP2040
void __no_inline_not_in_flash_func(__nukeFlash)(uint32_t offset, size_t size)
{
    volatile uint32_t pos = offset;
    volatile uint32_t end = offset + size;
    if (offset % 4096 > 0 || size % 4096 > 0)
    {
        SERIAL_DEBUG.println("Fatal: nuke paramters invalid");
    }
    else
    {
        rp2040.idleOtherCore();
        while (pos < end)
        {
            noInterrupts();
            flash_range_erase(pos, 4096);
            interrupts();
            pos += 4096;
            SERIAL_DEBUG.printf(".");
        }
        SERIAL_DEBUG.println("");
    }
}
#endif