#pragma once
#include "hardware.h"
#include "knxprod.h"

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

#ifndef KNX_SERIAL
#define KNX_SERIAL Serial1
#endif