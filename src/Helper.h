#pragma once

#include <Arduino.h>
#include <cstdint>
#include <stdarg.h>
#include <stdio.h>
#include <string>

#define DelayCheckMillis(last, duration) (millis() - last >= duration)
#define DelayCheckMicros(last, duration) (micros() - last >= duration)
#define DelayCheck(last, duration) DelayCheckMillis(last, duration)

#define NO_NUM -987654321.0F // normal NAN-Handling does not work

/*********************
 * Helper for any module
 * *******************/

// generic helper for formatted debug output
// int printDebug(const char *format, ...);
// void printHEX(const char* iPrefix, const uint8_t *iData, size_t iLength);
// void printResult(bool iResult);

// ensure correct time delta check
// important by using interrupt handler
//   The called method millis() are not incremented further in the interrupt!
bool delayCheck(uint32_t iOldTimer, uint32_t iDuration);
// init delay timer with millis, ensure that it is not 0
uint32_t delayTimerInit();
// check for float number
bool isNum(float iNumber);

int freeMemory();

/*
 * Nuker
 */
#ifdef ARDUINO_ARCH_RP2040
#include "hardware/flash.h"
#include "hardware/sync.h"

#ifndef NUKE_FLASH_SIZE_BYTES
#define NUKE_FLASH_SIZE_BYTES (16 * 1024 * 1024)
#endif

void nukeFlashKnxOnly();
void nukeFlash();
#endif