#pragma once
#include "../DateTime.h"
#include "Arduino.h"
#include "OpenKNX/Led/Base.h"
#include "OpenKNX/Led/FunctionManager.h"
#include "OpenKNX/Led/Manager.h"
#include "OpenKNX/Led/RGB.h"
#include "TimeClockMillis.h"
#include "TimeClockSystem.h"
#include "hardware.h"
#include "string"
#include "time.h"
#include <ctime>

#ifndef OPENKNX_TIME_DIGAGNOSTIC
    #ifdef OPENKNX_DEBUG
        #define OPENKNX_TIME_DIGAGNOSTIC
    #endif
#endif

// #define OPENKNX_TIME_CLOCK OpenKNX::Time::TimeClockMillis
#ifndef OPENKNX_TIME_CLOCK
    #ifndef ARDUINO_ARCH_SAMD
        #define OPENKNX_TIME_CLOCK OpenKNX::Time::TimeClockSystem
    #else
        #define OPENKNX_TIME_CLOCK OpenKNX::Time::TimeClockMillis
    #endif
#endif

class GroupObject;

namespace OpenKNX
{
    class Common;
    class Console;

    namespace Time
    {
        class TimeProvider;

        enum DaylightSavingMode
        {
            AlwaysStandardTime,
            AlwaysDayLightSavingTime,
            Calculated
        };

        enum TimeChangedEvents
        {
            TimeChangedEventSecondChanged = 1,
            TimeChangedEventMinuteChanged = 2,
            TimeChangedEventHourChanged = 4,
            TimeChangedEventDayChanged = 8,
            TimeChangedEventMonthChanged = 16,
            TimeChangedEventYearChanged = 32,
            TimeChangedEventTimeSet = 64,
            TimeChangedEventValidChanged = 128,
            TimeChangedEventInaccurateChanged = 256
        };

        struct TimeChangedArgs
        {
            TimeChangedEvents events;
            DateTime localTime;
            bool isValid;
            bool isInaccurate;
        };

        typedef std::function<void(TimeChangedArgs)> TimeChangeCallback;
       
        class TimeManager
        {
            struct TimeChangedEventHandler
            {
                TimeChangedEvents events;
                TimeChangeCallback callback;
            };
            OPENKNX_TIME_CLOCK _timeClock = OPENKNX_TIME_CLOCK();
            friend Common;
            friend Console;
            friend TimeProvider;
            uint8_t _lastSendSecond = 0;
            uint8_t _lastSendMinute = 0;
            uint8_t _lastSendHour = 0;
            bool _configured = false;
            TimeProvider* _timeProvider = nullptr;
            DaylightSavingMode _daylightSavingMode = DaylightSavingMode::AlwaysStandardTime;
            int _dayLightSavingTimeOffset = 0;
            bool _timeProvideSupportKnxDaylightSavingTimeSwitch = false;
            unsigned long _waitTimerReadKo = 0;
            bool _intialReadKo = false;
            DateTime _lastLocalTimeInLoop = DateTime();
            bool _lastInaccurateInLoop = false;
            bool _lastValidInLoop = false;
            time_t _lastUpdatedByProviderTimeStamp =0;
            time_t _lastTimeStamp = 0;
            uint8_t _ledState = 0;
            uint8_t _lastSecondChange = 0;
            unsigned long _timerLedOn = 0;
            unsigned long _timeUpdatedActivity = 0;
            Led::FunctionGroup* _timeLed = nullptr;
            bool _timeSetEventNeeded = false;
            std::vector<TimeChangedEventHandler> _callbacks = std::vector<TimeChangedEventHandler>();

#ifdef OPENKNX_TIME_TESTCOMMAND
            void commandTest();
#endif
            void commandHelp();
            void commandInformation();
#ifdef OPENKNX_TIME_DIGAGNOSTIC
            void commandSetDateTime(std::string& cmd);
#endif
            void sendEvent(TimeChangedArgs args);
            void setup(bool configured);
            void setDaylightSavingMode(DaylightSavingMode daylightSavingMode);
            void loop();
            void loopLed(TimeChangedArgs args);
            void processInputKo(GroupObject& ko);
            bool processCommand(std::string& cmd, bool diagnoseKo);
            void setLocalTime(tm& tm, unsigned long miilisReceivedTimestamp);
            void setUtcTime(tm& tm, unsigned long miilisReceivedTimestamp);
            void timeSet();
            void checkChangedTime(tm& setTime, bool utc, unsigned long millisReceivedTimestamp);
            void sendTime();
            const std::string logPrefix();
            std::string buildTimezoneString(DaylightSavingMode daylightSavingMode);
            bool calculateValid(time_t now);
            bool calculateInaccurate(time_t now);

          public:
            /*
            Register a callback for time changed events. Specify the events to listen for and the callback function.
            */
            void registerCallback(TimeChangedEvents events, TimeChangeCallback callback);
            /*
            Returns true, if a time provider was set
            */
            bool hasTimerProvder();
            /*
            Returns the timerprovider if set, otherwise nullptr
            */
            TimeProvider* getTimeProvder();
            /*
             * set a time provider, a previous set time provider will be deleted
             */
            void setTimeProvider(TimeProvider* timeProvider);
            /*
             * returns the local time
             */
            DateTime getLocalTime();
            /*
             * returns the UTC time
             */
            DateTime getUtcTime();
            /*
             * returns true, if the time was a least one time set
             */
            bool isValid();

            /*
             * returns true, if the last time from time provider was set more than 24 hours ago
             */
            bool isInaccurate();

            /*
             * Returns for the provided local time
             * 1 if it is in daylight saving time
             * 0 if it is in standard time
             * -1 for the switching hour in the auntum which can be summer or winter time
             */
            int8_t isDaylightSavingTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute);

            /*
             * Returns for the provided local time (current tm_isdst will be ignored)
             * 1 if it is in daylight saving time
             * 0 if it is in standard time
             * -1 for the switching hour in the auntum which can be summer or winter time
             */
            int8_t isDaylightSavingTime(tm tm);

            /*
             * Calculate and set daylight saving flag 'tm_isdst' in tm struct. The current 'tm_isdst' will be ignored
             */
            void calculateAndSetDstFlag(tm& tm);

            /*
             * Offset daylight saving time, independent of the current time.
             * Note: this is not the offset to UTC, it is the offset between standard time and daylight saving time
             */
            int daylightSavingTimeOffset();
        };
    } // namespace Time
} // namespace OpenKNX