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
    rp2040.idleOtherCore();
    while (size > offset)
    {
        noInterrupts();
        size -= 4096;
        flash_range_erase(size, 4096);
        interrupts();
        SERIAL_DEBUG.printf(".%i \n", size);
    }
    SERIAL_DEBUG.println("");
    SERIAL_DEBUG.print("done");
    watchdog_reboot(0, 0, 0);
}
#endif