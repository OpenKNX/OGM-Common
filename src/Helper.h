#pragma once

#include <cstdint>
#include <stdio.h>
#include <stdarg.h>
#include <Arduino.h>
#include <string>

#define NO_NUM -987654321.0F // normal NAN-Handling does not work

/*********************
 * Helper for any module
 * *******************/

// generic helper for formatted debug output
int printDebug(const char *format, ...);
void printHEX(const char* iPrefix, const uint8_t *iData, size_t iLength);
void printResult(bool iResult);

// ensure correct time delta check
// cannot be used in interrupt handler
bool delayCheck(uint32_t iOldTimer, uint32_t iDuration);
// init delay timer with millis, ensure that it is not 0
uint32_t delayTimerInit();
// check for float number
bool isNum(float iNumber);

int freeMemory();
