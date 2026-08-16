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

    if (!result)
    {
        // Only this path writes uptimeLastMillis — called exclusively from Common::loop() (single writer).
        if (uptimeCurrentMillis < uptimeLastMillis)
            uptimeRolloverCount++;
        uptimeLastMillis = uptimeCurrentMillis;
        return 0;
    }

    // Read path: no writes to shared state, safe to call from any context.
    return ((uint64_t)uptimeRolloverCount << 32 | uptimeCurrentMillis) / 1000UL;
}

/*
 * Human readable duration, same layout as the log timestamp ("2d 03:11:07").
 */
std::string humanDuration(uint32_t seconds)
{
    const uint16_t days = (uint16_t)(seconds / 86400);
    seconds -= (uint32_t)days * 86400;
    const uint8_t hours = (uint8_t)(seconds / 3600);
    seconds -= (uint32_t)hours * 3600;
    const uint8_t mins = (uint8_t)(seconds / 60);
    seconds -= (uint32_t)mins * 60;

    char result[26] = {};
    snprintf(result, sizeof(result), "%dd %2.2d:%2.2d:%2.2d", (days % 10000), hours, mins, (uint8_t)seconds);
    return result;
}

/*
 * Compact count. Values below 10000 stay exact - on a quiet bus the real number is what
 * you want to read; above that the magnitude matters more than the last digits. Divisions
 * truncate, so the shown value never rounds up past a decade boundary.
 */
std::string humanCount(uint64_t value)
{
    char b[12] = {};
    if (value < 10000ULL)
        snprintf(b, sizeof(b), "%u", (unsigned)value);
    else if (value < 100000ULL)
        snprintf(b, sizeof(b), "%u.%uk", (unsigned)(value / 1000ULL), (unsigned)((value % 1000ULL) / 100ULL));
    else if (value < 1000000ULL)
        snprintf(b, sizeof(b), "%uk", (unsigned)(value / 1000ULL));
    else if (value < 10000000ULL)
        snprintf(b, sizeof(b), "%u.%02uM", (unsigned)(value / 1000000ULL), (unsigned)((value % 1000000ULL) / 10000ULL));
    else if (value < 100000000ULL)
        snprintf(b, sizeof(b), "%u.%uM", (unsigned)(value / 1000000ULL), (unsigned)((value % 1000000ULL) / 100000ULL));
    else if (value < 1000000000ULL)
        snprintf(b, sizeof(b), "%uM", (unsigned)(value / 1000000ULL));
    else if (value < 10000000000ULL)
        snprintf(b, sizeof(b), "%u.%02uG", (unsigned)(value / 1000000000ULL), (unsigned)((value % 1000000000ULL) / 10000000ULL));
    else
    {
        uint64_t g = value / 1000000000ULL;
        if (g > 0xFFFFFFFFULL) g = 0xFFFFFFFFULL; // keep the 32-bit cast honest
        snprintf(b, sizeof(b), "%luG", (unsigned long)g);
    }
    return b;
}

/*
 * Compact count without decimals, for rows that pack several values into one line.
 */
std::string humanCountShort(uint64_t value)
{
    char b[12] = {};
    if (value < 1000ULL)
        snprintf(b, sizeof(b), "%u", (unsigned)value);
    else if (value < 1000000ULL)
        snprintf(b, sizeof(b), "%uk", (unsigned)(value / 1000ULL));
    else if (value < 1000000000ULL)
        snprintf(b, sizeof(b), "%uM", (unsigned)(value / 1000000ULL));
    else
    {
        uint64_t g = value / 1000000000ULL;
        if (g > 0xFFFFFFFFULL) g = 0xFFFFFFFFULL;
        snprintf(b, sizeof(b), "%luG", (unsigned long)g);
    }
    return b;
}

/*
 * Byte amount in SI units. Traffic volume is counted in decades; the IEC units (KiB/MiB)
 * stay reserved for memory, where the powers of two are the real quantity.
 */
std::string humanBytes(uint64_t value)
{
    char b[16] = {};
    if (value < 10000ULL)
        snprintf(b, sizeof(b), "%u B", (unsigned)value);
    else if (value < 100000ULL)
        snprintf(b, sizeof(b), "%u.%u kB", (unsigned)(value / 1000ULL), (unsigned)((value % 1000ULL) / 100ULL));
    else if (value < 1000000ULL)
        snprintf(b, sizeof(b), "%u kB", (unsigned)(value / 1000ULL));
    else if (value < 10000000ULL)
        snprintf(b, sizeof(b), "%u.%02u MB", (unsigned)(value / 1000000ULL), (unsigned)((value % 1000000ULL) / 10000ULL));
    else if (value < 100000000ULL)
        snprintf(b, sizeof(b), "%u.%u MB", (unsigned)(value / 1000000ULL), (unsigned)((value % 1000000ULL) / 100000ULL));
    else if (value < 1000000000ULL)
        snprintf(b, sizeof(b), "%u MB", (unsigned)(value / 1000000ULL));
    else if (value < 10000000000ULL)
        snprintf(b, sizeof(b), "%u.%02u GB", (unsigned)(value / 1000000000ULL), (unsigned)((value % 1000000000ULL) / 10000000ULL));
    else
    {
        uint64_t g = value / 1000000000ULL;
        if (g > 0xFFFFFFFFULL) g = 0xFFFFFFFFULL;
        snprintf(b, sizeof(b), "%lu GB", (unsigned long)g);
    }
    return b;
}

#ifndef KNX_IS_ROUTER
void writeDpt16Ko(GroupObject &ko, const char *message, va_list &values)
{
    char buffer[19] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x06,0x9F,0xFD,0xCC}; // Last byte must be zero!
    uint8_t len = vsnprintf(buffer, 15, message, values); 
    if (len >= 15)
        if (buffer[15] != 0x06 || buffer[16] != 0x9F || buffer[17] != 0xFD || buffer[18] != 0xCC) 
            openknx.hardware.fatalError(FATAL_SYSTEM, "BufferOverflow: writeDpt16Ko message too long");
        else
            logDebug("writeDpt16Ko", "BufferOverflow: Diagnose length is %u (exceeds 14)", len);
    ko.value(buffer, Dpt(16, 1));
}

void writeDpt16Ko(GroupObject &ko, const char *message, ...)
{
    va_list values;
    va_start(values, message);
    writeDpt16Ko(ko, message, values);
    va_end(values);
}
#endif

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