#pragma once
#include "OpenKNX/Button.h"
#include "OpenKNX/Led.h"
#include <Arduino.h>

#ifdef ARDUINO_ARCH_RP2040
    #include <hardware.h>
    #include <hardware/adc.h>
    #include <pico/stdlib.h>
#endif

// fatal error codes
#define FATAL_FLASH_PARAMETERS 2        // I2C busy during startup
#define FATAL_I2C_BUSY 3                // I2C busy during startup
#define FATAL_LOG_WRONG_CHANNEL_COUNT 4 // knxprod contains more channels than logic supports
#define FATAL_SENS_UNKNOWN 5            // unknown or unsupported sensor
#define FATAL_SCHEDULE_MAX_CALLBACKS 6  // Too many callbacks in scheduler
#define FATAL_NETWORK 7                 // Network error
#define FATAL_INIT_FILESYSTEM 10        // LittleFS.begin() failed
#define FATAL_SYSTEM 20                 // Systemerror (e.g. buffer overrun)

// really needed?
#define BOARD_HW_EEPROM 0x01
#define BOARD_HW_LED 0x02
#define BOARD_HW_ONEWIRE 0x04
#define BOARD_HW_NCN5130 0x08

namespace OpenKNX
{
    class Hardware
    {
      private:
        uint8_t features = 0;

      public:
        // Initialize or HW detection
        void init();
        // Fatal Error
        void fatalError(uint8_t code, const char* message = 0);
        // CPU Temperatur
        float cpuTemperature();

#ifdef ARDUINO_ARCH_RP2040
        // Filesystem
        void initFilesystem();
#endif
        void initFlash();
        void initLeds();
        void initButtons();
        void initKnxRxISR();
    };
} // namespace OpenKNX