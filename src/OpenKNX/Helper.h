#pragma once

#include <Arduino.h>
#include <cstdint>
#include <stdarg.h>
#include <stdio.h>
#include <string>

#define delayCheckMillis(last, duration) (millis() - last >= duration)
#define delayCheckMicros(last, duration) (micros() - last >= duration)
#define delayCheck(last, duration) delayCheckMillis(last, duration)
#define delayTimerInit() (millis() == 0 ? 1 : millis())

#define NO_NUM -987654321.0F // normal NAN-Handling does not work
#define isNum(value) ((value + 10.0) > NO_NUM)

/*
 * Free Memory
 */
int freeMemory();

/*
 * Nuker
 */
#ifdef ARDUINO_ARCH_RP2040
    #include "hardware/flash.h"
    #include "hardware/sync.h"

/*
 * Erase flash
 */
void __no_inline_not_in_flash_func(__nukeFlash)(uint32_t offset, size_t count);
#endif