#pragma once
/**
 * @file        Wrapper.h
 * @brief       I2C Wrapper for OpenKNX - Unified Hardware and PIO I2C Management
 * @version     0.0.1
 * @date        2025-10-30
 * @copyright   Copyright (c) 2025, Erkan Çolak (erkan@çolak.de)
 *              Licensed under GNU GPL v3.0
 */

#include "OpenKNX/defines.h"
#include <Wire.h>
#include <string>

/*
 * Check if PIO I2C is requested on non-RP2040/RP2350 platforms
 */
#if !defined(ARDUINO_ARCH_RP2040) && ANY(OPENKNX_WIRE_PIO, OPENKNX_WIRE1_PIO)
    //#warning "OPENKNX WIRE_PIO or WIRE1_PIO defined but platform is not RP2040/RP2350!"
    //#warning "PIO is only available on RP2040/RP2350 platforms!" // Warn user, but allow compilation to continue
#endif

/*
 * PIO I2C Support (RP2040/RP2350 only)
 */
#if defined(ARDUINO_ARCH_RP2040) && ANY(OPENKNX_WIRE_PIO, OPENKNX_WIRE1_PIO)
    #define OPENKNX_I2C_USE_PIO
    #include "PIOI2CWire.h"
#endif

/* ****************************************************************************
 * Check Configuration Defines
 * @note Throw errors and stop compilation if required defines are missing
 * ***************************************************************************/

/*
 * Validate WIRE_PIO pin configuration
 */
#ifdef OPENKNX_WIRE_PIO
    #ifndef OPENKNX_WIRE_PIO_SDA_PIN
        #error "OPENKNX_WIRE_PIO defined but OPENKNX_WIRE_PIO_SDA_PIN not set!"
    #endif
    #ifndef OPENKNX_WIRE_PIO_SCL_PIN
        #error "OPENKNX_WIRE_PIO defined but OPENKNX_WIRE_PIO_SCL_PIN not set!"
    #endif
    #ifndef OPENKNX_WIRE_PIO_CLOCK
        #error "OPENKNX_WIRE_PIO defined but OPENKNX_WIRE_PIO_CLOCK not set!"
    #endif
    #if (OPENKNX_WIRE_PIO_SCL_PIN != OPENKNX_WIRE_PIO_SDA_PIN + 1)
        #error "OPENKNX_WIRE_PIO: SCL pin must be SDA pin + 1!"
    #endif
extern OpenKNX::I2C::PIOI2CWire WIRE_PIO; // Declare global WIRE_PIO instance
#endif

/*
 * Validate WIRE1_PIO pin configuration
 */
#ifdef OPENKNX_WIRE1_PIO
    #ifndef OPENKNX_WIRE1_PIO_SDA_PIN
        #error "OPENKNX_WIRE1_PIO defined but OPENKNX_WIRE1_PIO_SDA_PIN not set!"
    #endif
    #ifndef OPENKNX_WIRE1_PIO_SCL_PIN
        #error "OPENKNX_WIRE1_PIO defined but OPENKNX_WIRE1_PIO_SCL_PIN not set!"
    #endif
    #ifndef OPENKNX_WIRE1_PIO_CLOCK
        #error "OPENKNX_WIRE1_PIO defined but OPENKNX_WIRE1_PIO_CLOCK not set!"
    #endif
    #if (OPENKNX_WIRE1_PIO_SCL_PIN != OPENKNX_WIRE1_PIO_SDA_PIN + 1)
        #error "OPENKNX_WIRE1_PIO: SCL pin must be SDA pin + 1!"
    #endif
extern OpenKNX::I2C::PIOI2CWire WIRE1_PIO; // Declare global WIRE1_PIO instance
#endif

/*
 * Validate WIRE pin configuration
 */
#ifdef OPENKNX_WIRE
    #ifndef OPENKNX_WIRE_SDA_PIN
        #error "OPENKNX_WIRE defined but OPENKNX_WIRE_SDA_PIN not set!"
    #endif
    #ifndef OPENKNX_WIRE_SCL_PIN
        #error "OPENKNX_WIRE defined but OPENKNX_WIRE_SCL_PIN not set!"
    #endif
    #ifndef OPENKNX_WIRE_CLOCK
        #error "OPENKNX_WIRE defined but OPENKNX_WIRE_CLOCK not set!"
    #endif
extern TwoWire& WIRE; // Declare global WIRE instance
#endif

/*
 * Validate WIRE1 pin configuration
 */
#ifdef OPENKNX_WIRE1
    #ifndef OPENKNX_WIRE1_SDA_PIN
        #error "OPENKNX_WIRE1 defined but OPENKNX_WIRE1_SDA_PIN not set!"
    #endif
    #ifndef OPENKNX_WIRE1_SCL_PIN
        #error "OPENKNX_WIRE1 defined but OPENKNX_WIRE1_SCL_PIN not set!"
    #endif
    #ifndef OPENKNX_WIRE1_CLOCK
        #error "OPENKNX_WIRE1 defined but OPENKNX_WIRE1_CLOCK not set!"
    #endif
extern TwoWire& WIRE1; // Declare global WIRE1 instance
#endif

/* ****************************************************************************
 * OpenKNX I2C Wrapper Class Definition
 * ***************************************************************************/
namespace OpenKNX
{
    namespace I2C
    {
        /**
         * @brief I2C Wrapper for unified Hardware and PIO I2C management
         *
         * Manages up to 4 I2C buses:
         * - WIRE_PIO / WIRE1_PIO (PIO I2C, RP2040 only)
         * - WIRE / WIRE1 (Hardware I2C, all platforms)
         *
         * The wrapper handles initialization and provides optional diagnostics.
         */
        class WireWrapper
        {
          public:
            WireWrapper();
            ~WireWrapper();
            void init();
            void loop();
            void scanBus(TwoWire& wire, const char* name);
            void scanAllBuses();
#ifdef OPENKNX_I2C_BENCHMARK
            void runBenchmarks();
#endif
            const std::string logPrefix() { return "I2C"; }

          private:
            bool _initialized = false;
        }; // class WireWrapper
    } // namespace I2C
} // namespace OpenKNX