#pragma once
#include "TimeProvider.h"

namespace OpenKNX
{
    namespace Time
    {
        class TimeProviderKnx : public TimeProvider
        {
            const static int WaitTimeMissingOtherDelegramsInMs = 900;

            enum WaitStates
            {
                None,
                InitialRead,
                InitialReadRepeat,
                ReceiveMissingOtherTelegrams
            };
            WaitStates _waitStates = WaitStates::None;
            bool _configured = false;
            bool _hasDate = false;
            bool _hasTime = false;
            unsigned long _timeStampTimeReceived = 0;
            bool _hasDaylightSavingFlag = false;
            tm _dateTime = {0};
            unsigned long _waitTimerStart = 0;
            void checkHasAllDateTimeParts();
            void initReceiveDateTimeStructure();

          public:
            const static int InitialReadAfterInMs = 5000;
            const static int DelayInitialReadInMs = 32000;
            /*
             * Return the prefix for logging
             */
            const std::string logPrefix() override;
            /*
             * Overload this function to log the name of the provider and other usefull information
             */
            void logInformation() override;
            /*
             * called by the framework after the knx configuration was loaded
             */
            void setup() override;
            /*
             * called by the framework in the loop for core0
             */
            void loop() override;
            /*
             * Called on incoming/changing GroupObject
             * @param GroupObject
             */
            void processInputKo(GroupObject& ko) override;
            /*
             * return true, if the provider support setting daylight saving time
             */
            bool supportKnxDaylightSavingTimeSwitch() override;
        };
    } // namespace Time
} // namespace OpenKNX