#pragma once
#include <hardware.h>
#include <knxprod.h>
#include <versions.h>

// see README for available defines

#ifndef OPENKNX_MAX_MODULES
    #define OPENKNX_MAX_MODULES 9
#endif

#ifndef OPENKNX_MAX_LOOPTIME // US
    #define OPENKNX_MAX_LOOPTIME 4000
#endif

#ifndef OPENKNX_LOOPTIME_WARNING // MS
    #define OPENKNX_LOOPTIME_WARNING 7
#endif

#ifndef OPENKNX_LOOPTIME_WARNING_INTERVAL // MS
    #define OPENKNX_LOOPTIME_WARNING_INTERVAL 1000
#endif

#ifndef OPENKNX_WAIT_FOR_SERIAL
    #define OPENKNX_WAIT_FOR_SERIAL 2000
#endif

// Priority active?
#ifdef OPENKNX_HEARTBEAT_PRIO

    // Remove double define
    #undef OPENKNX_HEARTBEAT
    #ifndef OPENKNX_HEARTBEAT
        #define OPENKNX_HEARTBEAT OPENKNX_HEARTBEAT_PRIO
    #endif

#endif

// Heartbeat active? (with priority)
#ifdef OPENKNX_HEARTBEAT

    // set default value for OPENKNX_HEARTBEAT timeout to 1000ms
    #if OPENKNX_HEARTBEAT <= 1
        #undef OPENKNX_HEARTBEAT
        #define OPENKNX_HEARTBEAT 1000
    #endif

    // set default frequencies (normal)
    #ifndef OPENKNX_HEARTBEAT_FREQ
        #define OPENKNX_HEARTBEAT_FREQ 200
    #endif

    // set default frequencies (prio with active forceOn)
    #ifndef OPENKNX_HEARTBEAT_PRIO_ON_FREQ
        #define OPENKNX_HEARTBEAT_PRIO_ON_FREQ 200
    #endif

    // set default frequencies (prio with inactive forceOn)
    #ifndef OPENKNX_HEARTBEAT_PRIO_OFF_FREQ
        #define OPENKNX_HEARTBEAT_PRIO_OFF_FREQ 1000
    #endif

#endif

#ifndef OPENKNX_WATCHDOG_MAX_PERIOD
    #define OPENKNX_WATCHDOG_MAX_PERIOD 16384
#endif

#ifndef OPENKNX_RECOVERY_TIME
    #define OPENKNX_RECOVERY_TIME 6000
#endif

#ifndef KNX_SERIAL
    #define KNX_SERIAL Serial1
#endif

// Fallback for old defines
#ifdef INFO_LED_PIN
    #define INFO1_LED_PIN INFO_LED_PIN
    #define INFO1_LED_PIN_ACTIVE_ON INFO_LED_PIN_ACTIVE_ON
#endif

// __time_critical_func fallback
#ifndef ARDUINO_ARCH_RP2040
    #define __time_critical_func(X) X
    #define __isr
#endif

#if defined(OPENKNX_DUALCORE) && defined(ARDUINO_ARCH_ESP32)
    #ifndef ARDUINO_LOOP1_STACK_SIZE
        #ifndef CONFIG_ARDUINO_LOOP1_STACK_SIZE
            #define ARDUINO_LOOP1_STACK_SIZE 8192
        #else
            #define ARDUINO_LOOP1_STACK_SIZE CONFIG_ARDUINO_LOOP1_STACK_SIZE
        #endif
    #endif
#endif

#ifdef ARDUINO_ARCH_ESP32
    typedef char pin_size_t;
#endif