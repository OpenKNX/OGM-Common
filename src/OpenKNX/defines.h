#pragma once
#include <hardware.h>
#include <knxprod.h>
#include <versions.h>

// see README for available defines

#ifndef OPENKNX_MAX_MODULES
    #define OPENKNX_MAX_MODULES 9
#endif

#ifndef OPENKNX_MAX_LOOPTIME
    #define OPENKNX_MAX_LOOPTIME 4000
#endif

#ifndef OPENKNX_LOOPTIME_WARNING
    #define OPENKNX_LOOPTIME_WARNING 5
#endif

#ifndef OPENKNX_LOOPTIME_WARNING_INTERVAL
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

#ifndef KNX_SERIAL
    #define KNX_SERIAL Serial1
#endif

#ifdef ARDUINO_ARCH_RP2040
    #define OPENKNX_DUALCORE
#endif