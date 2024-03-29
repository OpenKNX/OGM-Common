#include "Helper.h"
#include "OpenKNX/Facade.h"

#ifndef ARDUINO_ARCH_ESP32

    /*
     * Free Memory
     */
    #ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char *sbrk(int incr);
    #else  // __ARM__
extern char *__brkval;
    #endif // __arm__

#endif // !ARDUINO_ARCH_ESP32

int freeMemory()
{
#if defined(ARDUINO_ARCH_ESP32)
    return ESP.getFreeHeap();
#elif defined(ARDUINO_ARCH_RP2040)
    return rp2040.getFreeHeap();
#else
    char top;
    #ifdef __arm__
    return &top - reinterpret_cast<char *>(sbrk(0));
    #elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
    return &top - __brkval;
    #else  // __arm__
    return __brkval ? &top - __brkval : &top - __malloc_heap_start;
    #endif // __arm__
#endif     // !ARDUINO_ARCH_ESP32
}

/*
 * Uptime in Seconds
 * must be called regularly so that the rollovers can be determined
 */

uint32_t uptime(bool result)
{
    static uint16_t uptimeRolloverCount = 0;
    static uint32_t uptimeLastMillis = 0;

    const uint32_t uptimeCurrentMillis = millis();
    if (uptimeCurrentMillis < uptimeLastMillis)
        uptimeRolloverCount++;
    uptimeLastMillis = uptimeCurrentMillis;

    if (!result)
        return 0;
    return ((uint64_t)uptimeRolloverCount << 32 | uptimeCurrentMillis) / 1000UL;
}

/*
 * Nuker
 */
#ifdef ARDUINO_ARCH_RP2040
bool __no_inline_not_in_flash_func(__nukeFlash)(uint32_t offset, size_t size)
{
    if (offset % 4096 > 0 || size % 4096 > 0)
    {
        return false;
    }
    else
    {
        rp2040.idleOtherCore();
        noInterrupts();
        flash_range_erase(offset, size);
        interrupts();
        rp2040.resumeOtherCore();
        return true;
    }
}

#ifdef SERIAL_DEBUG
void printFreeStackSize()
{
    #ifdef OPENKNX_DUALCORE
    SERIAL_DEBUG.print(rp2040.cpuid() ? "_1> " : "0_> ");
    #endif
    SERIAL_DEBUG.printf("Free stack size: %i bytes\r\n", rp2040.getFreeStack());
}
#endif
#endif