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
#ifdef ARDUINO_ARCH_ESP32
    return ESP.getFreeHeap();
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
        flash_range_erase(offset, size);
        interrupts();
        rp2040.resumeOtherCore();
    }
}

uint32_t stackPointer()
{
    uint32_t *sp;
    asm volatile("mov %0, sp"
                 : "=r"(sp));
    return (uint32_t)sp;
}

int freeStackSize()
{
    unsigned int sp = stackPointer();
    uint32_t ref = 0x20041000;
    #ifdef OPENKNX_DUALCORE
    if (rp2040.cpuid() == 1) ref = 0x20040000;
    #endif

    return sp - ref;
}

void printFreeStackSize()
{
    #ifdef OPENKNX_DUALCORE
    SERIAL_DEBUG.print(rp2040.cpuid() ? "_1> " : "0_> ");
    #endif
    SERIAL_DEBUG.printf("Free stack size: %i bytes\r\n", freeStackSize());
}
#endif