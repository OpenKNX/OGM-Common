#pragma once
#include "Arduino.h"
#include "string"
#include "time.h"
#include "chrono"

class GroupObject;

namespace OpenKNX
{
#ifdef ARDUINO_ARCH_SAMD
struct timezone {
	int	tz_minuteswest;	/* minutes west of Greenwich */
	int	tz_dsttime;	/* type of dst correction */
};
#endif
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
           
            friend Common;
            friend Console;
            friend TimeProvider;
            bool _disableKoRead = false;
            TimeProvider* _timeProvider = nullptr;
            bool _setupCalled = false;
            DaylightSavingMode _daylightSavingMode = DaylightSavingMode::AlwaysStandardTime;
            int _dayLightSavingTimeOffset = 0;
            bool _timeProvideSupportKnxDaylightSavingTimeSwitch = false;
            unsigned long _waitTimerReadKo = 0;
           
            void setup(bool configured);
            void setDaylightSavingMode(DaylightSavingMode daylightSavingMode);
            void loop();
            void processInputKo(GroupObject& ko);
            bool processCommand(std::string& cmd, bool diagnoseKo);
            void setLocalTime(tm& tm, unsigned long miilisReceivedTimestamp);
            void setUtcTime(tm& tm, unsigned long miilisReceivedTimestamp);
            void setTime(std::time_t epoch, timezone* tz, unsigned long miilisReceivedTimestamp);
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
            TimeProvider *getTimeProvder();
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
            int isDayLightSavingTime(int year, int month, int day, int hour, int minute);
            /*
            * Offset daylight saving time
            */
            int daylightSavingTimeOffset();

     
        };
    } // namespace Time
} // namespace OpenKNX