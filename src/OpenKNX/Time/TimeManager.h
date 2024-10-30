#pragma once
#include "Arduino.h"
#include "TimeClockMillis.h"
#include "TimeClockSystem.h"
#include "hardware.h"
#include "string"
#include "time.h"
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

        class TimeManager
        {
            OPENKNX_TIME_CLOCK _timeClock = OPENKNX_TIME_CLOCK();
            friend Common;
            friend Console;
            friend TimeProvider;
            bool _disableKoRead = false;
            bool _configured = false;
            TimeProvider* _timeProvider = nullptr;
            DaylightSavingMode _daylightSavingMode = DaylightSavingMode::AlwaysStandardTime;
            int _dayLightSavingTimeOffset = 0;
            bool _timeProvideSupportKnxDaylightSavingTimeSwitch = false;
            unsigned long _waitTimerReadKo = 0;
            tm _easter = {0};
            tm _fourthAdvent = {0};

#ifdef OPENKNX_TIME_TESTCOMMAND
            void commandTest();
#endif
            void commandHelp();
            void commandInformation();
#ifdef OPENKNX_TIME_DIGAGNOSTIC
            void commandSetDateTime(std::string& cmd);
#endif
            void setup(bool configured);
            void setDaylightSavingMode(DaylightSavingMode daylightSavingMode);
            void loop();
            void processInputKo(GroupObject& ko);
            bool processCommand(std::string& cmd, bool diagnoseKo);     
            void setLocalTime(tm& tm, unsigned long miilisReceivedTimestamp);
            void setUtcTime(tm& tm, unsigned long miilisReceivedTimestamp);
            void timeSet();
            const std::string logPrefix();
            std::string buildTimezoneString(DaylightSavingMode daylightSavingMode);
          public:
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
            tm getLocalTime();
            /*
             * returns the UTC time
             */
            tm getUtcTime();
            /*
             * returns true, if the time was a least one time set
             */
            bool isTimeValid();

            /*
             * Converts a UTC time to local time
             */
            tm convertUtcToLocalTime(tm& utcTime);

            /*
             * Converts a local time to UTC time
             */
            tm convertLocalTimeToUtc(tm& tmLocalTime);

            /*
             * Returns for the provided local time
             * 1 if it is in daylight saving time
             * 0 if it is in standard time
             * -1 for the switching hour in the auntum which can be summer or winter time
             */
            int isDaylightSavingTime(int year, int month, int day, int hour, int minute);

            /*
             * Returns for the provided local time (current tm_isdst will be ignored)
             * 1 if it is in daylight saving time
             * 0 if it is in standard time
             * -1 for the switching hour in the auntum which can be summer or winter time
             */
            int isDaylightSavingTime(tm tm);

            /*
             * Calculate and set daylight saving flag. The current tm_isdst will be ignored
             */
            void calculateAndSetDstFlag(tm& tm);

            /*
             * Offset daylight saving time
             */
            int daylightSavingTimeOffset();

            /*
             * get easter
             */
            tm getEaster();

            /*
             * get 4th advent
             */
            tm getForthAdvent();
        };
    } // namespace Time
} // namespace OpenKNX