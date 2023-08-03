#include "Helper.h"
#include "OpenKNX/Facade.h"

#ifndef ARDUINO_ARCH_ESP32

/*
 * Free Memory
 */
#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char* __brkval;
#endif // __arm__

#endif // !ARDUINO_ARCH_ESP32

int freeMemory()
{
#ifdef ARDUINO_ARCH_ESP32
    return ESP.getFreeHeap();
#else
    char top;
#ifdef __arm__
    return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
    return &top - __brkval;
#else  // __arm__
    return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif // __arm__
#endif // !ARDUINO_ARCH_ESP32
}

/*
 * Nuker
 */
#ifdef ARDUINO_ARCH_RP2040
void __no_inline_not_in_flash_func(__nukeFlash)(uint32_t offset, size_t size)
{
    if (offset % 4096 > 0 || size % 4096 > 0)
    {
        openknx.logger.log("Fatal: nuke paramters invalid");
    }
    else
    {
        rp2040.idleOtherCore();
        noInterrupts();
        flash_range_erase(offset, offset + size);
        interrupts();
        rp2040.resumeOtherCore();
    }
}
#endif