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


// New ETS Base View
// CHeck on Param is available

// #if defined(BASE_StartupDelayBase) && defined(LOG_StartupDelayBase)
// #undef LOG_StartupDelayBase
// #undef ParamLOG_StartupDelayBase
// #define LOG_StartupDelayBase BASE_StartupDelayBase
// #define ParamLOG_StartupDelayBase ParamBASE_StartupDelayBase
// #undef LOG_StartupDelayTime
// #undef ParamLOG_StartupDelayTime
// #define LOG_StartupDelayTime BASE_StartupDelayTime
// #define ParamLOG_StartupDelayTime ParamBASE_StartupDelayTime
// #undef LOG_StartupDelayTimeMS
// #undef ParamLOG_StartupDelayTimeMS
// #define LOG_StartupDelayTimeMS BASE_StartupDelayTimeMS
// #define ParamLOG_StartupDelayTimeMS ParamBASE_StartupDelayTimeMS
// #endif

// #if defined(BASE_HeartbeatDelayBase) && defined(LOG_HeartbeatDelayBase)
// #undef KoLOG_Heartbeat
// #define KoLOG_Heartbeat KoBASE_Heartbeat
// #undef LOG_HeartbeatDelayBase
// #undef ParamLOG_HeartbeatDelayBase
// #define LOG_HeartbeatDelayBase BASE_HeartbeatDelayBase
// #define ParamLOG_HeartbeatDelayBase ParamBASE_HeartbeatDelayBase
// #undef LOG_HeartbeatDelayTime
// #undef ParamLOG_HeartbeatDelayTime
// #define LOG_HeartbeatDelayTime BASE_HeartbeatDelayTime
// #define ParamLOG_HeartbeatDelayTime ParamBASE_HeartbeatDelayTime
// #undef LOG_HeartbeatDelayTimeMS
// #undef ParamLOG_HeartbeatDelayTimeMS
// #define LOG_HeartbeatDelayTimeMS BASE_HeartbeatDelayTimeMS
// #define ParamLOG_HeartbeatDelayTimeMS ParamBASE_HeartbeatDelayTimeMS
// #endif

// #if defined(BASE_Diagnose) && defined(LOG_Diagnose)
// #undef KoLOG_Diagnose
// #define KoLOG_Diagnose KoBASE_Diagnose
// #undef LOG_Diagnose
// #undef ParamLOG_Diagnose
// #define LOG_Diagnose BASE_Diagnose
// #define ParamLOG_Diagnose ParamBASE_Diagnose
// #endif

// #if defined(BASE_ReadTimeDate) && defined(LOG_ReadTimeDate)
// #undef LOG_ReadTimeDate
// #undef ParamLOG_ReadTimeDate
// #define LOG_ReadTimeDate BASE_ReadTimeDate
// #define ParamLOG_ReadTimeDate ParamBASE_ReadTimeDate
// #endif

// #if defined(BASE_CombinedTimeDate) && defined(LOG_CombinedTimeDate)
// #undef KoLOG_Time
// #define KoLOG_Time KoBASE_Time
// #undef KoLOG_Date
// #define KoLOG_Date KoBASE_Date
// #undef LOG_CombinedTimeDate
// #undef ParamLOG_CombinedTimeDate
// #define LOG_CombinedTimeDate BASE_CombinedTimeDate
// #define ParamLOG_CombinedTimeDate ParamBASE_CombinedTimeDate
// #endif
