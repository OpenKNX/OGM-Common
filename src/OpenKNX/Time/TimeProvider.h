#pragma once
#include "OpenKNX.h"

namespace OpenKNX
{
    namespace Time
    {
        /*
         * This is the base class for time providers like KNX, GPS, ...
         */
        class TimeProvider
        {
            friend Common;

          protected:
            bool _disableKoRead = false;

          public:
            /*
             * called by the framework after the knx configuration was loaded
             */
            virtual void setup() {}
            /*
             * called by the framework in the loop for core0
             */
            virtual void loop() {}
            /*
             * Called on incoming/changing GroupObject
             * @param GroupObject
             */
            virtual void processInputKo(GroupObject& ko) {}

            /*
             * return true, if the provider support setting daylight saving time
             */
            virtual bool supportKnxDaylightSavingTimeSwitch() { return false; }

          protected:
            /*
             * Return the prefix for logging
             */
            virtual const std::string logPrefix() = 0;
            /*
             * The time provide implementation can use this function to set the local time
             * By calling this function, the UTC time will be calculated
             */
            void setLocalTime(tm& localTime, unsigned long millisReceivedTimestamp = millis());
            /*
             * The time provide implementation can use this function to set the UTC time
             * By calling this function, the local time will be calculated
             */
            void setUtcTime(tm& utcTime, unsigned long millisReceivedTimestamp = millis());
            /*
             * The time provide implementation can use this function to notify the framework that the time was set
             */
            void timeSet();

          public:
            /*
             * Overload this function to log the name of the provider and other usefull information
             */
            virtual void logInformation() = 0;
            /*
             * Overload this function to delete resources
             */
            virtual ~TimeProvider() {}
        };
    } // namespace Time
} // namespace OpenKNX