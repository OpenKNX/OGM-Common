#include "TimeProviderKnx.h"
#include "DPT19Flags.h"
#include "OpenKNX.h"

#define TIME_TOLERANCE_CHECK_MIN 2

namespace OpenKNX
{
    namespace Time
    {
        const std::string TimeProviderKnx::logPrefix()
        {
            return std::string("KnxTime");
        }

        void TimeProviderKnx::logInformation()
        {
            logInfoP("Timeprovider: KNX");
            switch (_waitStates)
            {
                case WaitStates::InitialRead:
                case WaitStates::ReceiveMissingOtherTelegrams:
                {
                    const char *prefix = _waitStates == InitialRead ? "initial" : "missing";
                    if (!_hasDate)
                    {
                        if (ParamBASE_CombinedTimeDate)
                            logInfoP("Wait for %s diagram date time", prefix);
                        else
                            logInfoP("Wait for %s diagram date", prefix);
                    }
                    if (!_hasTime)
                        logInfoP("Wait for %s diagram time", prefix);
                    if (!_hasDaylightSavingFlag && ParamBASE_SummertimeAll == 0)
                        logInfoP("Wait for %s diagram daylightsaving", prefix);
                }
                break;
            }
        }

        void TimeProviderKnx::setup()
        {
#ifdef BASE_KoTime
            if (KoBASE_Time.initialized())
                processInputKo(KoBASE_Time);
            if (!ParamBASE_CombinedTimeDate && KoBASE_Date.initialized())
                processInputKo(KoBASE_Date);
            // <Enumeration Text="Kommunikationsobjekt 'Sommerzeit aktiv'" Value="0" Id="%ENID%" />
            // <Enumeration Text="Kombiniertes Datum/Zeit-KO (DPT 19)" Value="1" Id="%ENID%" />
            // <Enumeration Text="Interne Berechnung (nur in Deutschland)" Value="2" Id="%ENID%" />
            if (ParamBASE_SummertimeAll == 0 && KoBASE_IsSummertime.initialized())
                processInputKo(KoBASE_IsSummertime);

            if ((!_hasDate || !_hasTime || !_hasDaylightSavingFlag) && ParamBASE_ReadTimeDate)
            {
                _waitTimerStart = delayTimerInit();
                _waitStates = WaitStates::InitialRead;
            }
#endif
        }

        void TimeProviderKnx::loop()
        {
#ifdef BASE_KoTime
            switch (_waitStates)
            {
                case WaitStates::InitialRead:
                case WaitStates::InitialReadRepeat:
                {
                    // read on start
                    if (_waitTimerStart != 0 && delayCheckMillis(_waitTimerStart,  (unsigned long) (_waitStates == WaitStates::InitialRead ? InitialReadAfterInMs : DelayInitialReadInMs)))
                    {
                        _waitStates = WaitStates::InitialReadRepeat;
                        if (_hasDate && _hasTime)
                        {
                            // Not daylight saving information, assue standard time
                            _dateTime.tm_isdst = 0;
                            _hasDaylightSavingFlag = true;
                            checkHasAllDateTimeParts();
                            return;
                        }
                        else
                        {
                            _waitTimerStart = millis();
                        }
                        if (ParamBASE_CombinedTimeDate)
                        {
                            // combined date and time
                            if (!_hasTime && !_disableKoRead)
                            {
                                KoBASE_Time.requestObjectRead();
                            }
                        }
                        else
                        {
                            // date and time from separate KOs
                            if (!_hasTime && !_disableKoRead)
                            {
                                KoBASE_Time.requestObjectRead();
                            }
                            if (!_hasDate && !_disableKoRead)
                            {
                                KoBASE_Date.requestObjectRead();
                            }
                        }
                        if (!_hasDaylightSavingFlag && ParamBASE_SummertimeAll == 0 && !_disableKoRead)
                        {
                            KoBASE_IsSummertime.requestObjectRead();
                        }
                    }
                    break;
                }
                case WaitStates::ReceiveMissingOtherTelegrams:
                {
                    if (_waitTimerStart != 0 && delayCheckMillis(_waitTimerStart, WaitTimeMissingOtherDelegramsInMs))
                    {
                        _waitStates = WaitStates::None;
                        // Use the already loaded internal time for all time parts
                        _hasDate = true;
                        _hasTime = true;
                        _hasDaylightSavingFlag = true;
                        checkHasAllDateTimeParts();
                    }
                    break;
                }
            }
#endif
        }

        void TimeProviderKnx::processInputKo(GroupObject &ko)
        {
#ifdef BASE_KoTime
            if (ko.asap() == BASE_KoTime)
            {
                unsigned long receiveTimeStamp = millis();
                if (ParamBASE_CombinedTimeDate)
                {
                    KNXValue value = "";
                    // first ensure we have a valid data-time content
                    // (including the correct length)
                    if (ko.tryValue(value, DPT_DateTime))
                    {

                        // use raw value, as current version of knx do not provide access to all fields
                        // TODO DPT19: check integration of extended DPT19 access into knx or OpenKNX-Commons
                        // size is ensured to be 8 Byte
                        uint8_t *raw = ko.valueRef();

                        /*
                        const bool flagFault = raw[6] & 0x80;
                        // ignore working day (WD, NWD): raw[6] & 0x40, raw[6] & 0x20
                        const bool flagNoYear = raw[6] & 0x10;
                        const bool flagNoDate = raw[6] & 0x08;
                        // ignore NDOW: raw[6] & 0x04
                        const bool flagNoTime = raw[6] & 0x02;
                        const bool flagSuti = raw[6] & 0x01;
                        // ignore quality of clock (CLQ): raw[7] & 0x80
                        // ignore synchronisation source reliablity (SRC): raw[7] & 0x40
                        */

                        // ignore inputs with:
                        // * F - fault
                        // * NY - missing year
                        // * ND - missing date
                        // * NT - missing time
                        if ((raw[6] & (DPT19_FAULT | DPT19_NO_YEAR | DPT19_NO_DATE | DPT19_NO_TIME)))
                        {
                            logErrorP("Unvalid date time received");
                        }
                        else
                        {
                            initReceiveDateTimeStructure();
                            _timeStampTimeReceived = receiveTimeStamp;
                            struct tm knxDateTime = value;
                            _dateTime.tm_year = knxDateTime.tm_year - 1900;
                            _dateTime.tm_mon = knxDateTime.tm_mon - 1;
                            _dateTime.tm_mday = knxDateTime.tm_mday;
                            _hasDate = true;
                            _dateTime.tm_hour = knxDateTime.tm_hour;
                            _dateTime.tm_min = knxDateTime.tm_min;
                            _dateTime.tm_sec = knxDateTime.tm_sec;
                            _hasTime = true;

                            const bool lDST = raw[6] & DPT19_SUMMERTIME;
                            // <Enumeration Text="Kommunikationsobjekt 'Sommerzeit aktiv'" Value="0" Id="%ENID%" />
                            // <Enumeration Text="Kombiniertes Datum/Zeit-KO (DPT 19)" Value="1" Id="%ENID%" />
                            // <Enumeration Text="Interne Berechnung" Value="2" Id="%ENID%" />
                            if (ParamBASE_SummertimeAll == 1)
                            {
                                _dateTime.tm_isdst = lDST;
                                _hasDaylightSavingFlag = true;
                            }
                            checkHasAllDateTimeParts();
                        }
                    }
                }
                else
                {
                    unsigned long receiveTimeStamp = millis();
                    KNXValue value = "";
                    // ensure we have a valid time content
                    if (ko.tryValue(value, DPT_TimeOfDay))
                    {
                        initReceiveDateTimeStructure();
                        _timeStampTimeReceived = receiveTimeStamp;
                        struct tm knxTime = value;
                        _dateTime.tm_hour = knxTime.tm_hour;
                        _dateTime.tm_min = knxTime.tm_min;
                        _dateTime.tm_sec = knxTime.tm_sec;
                        _hasTime = true;
                        if (openknx.time.isValid())
                        {
                            tm now;
                            openknx.time.getLocalTime().toTm(now);
                            if (!_hasDate)
                            {
                                // date is already valid, use current date
                                if (knxTime.tm_hour == 0 && knxTime.tm_min <= TIME_TOLERANCE_CHECK_MIN &&
                                    now.tm_hour == 23 && now.tm_min >= 60 - TIME_TOLERANCE_CHECK_MIN)
                                {
                                    // New day started, correct date to use it
                                    now.tm_mday += 1;
                                    mktime(&now); // normalize
                                }
                                else if (knxTime.tm_hour == 23 && knxTime.tm_min >= 60 - TIME_TOLERANCE_CHECK_MIN &&
                                         now.tm_hour == 0 && now.tm_min <= TIME_TOLERANCE_CHECK_MIN)
                                {
                                    // New day started, correct date to use it
                                    now.tm_mday -= 1;
                                    mktime(&now); // normalize
                                }
                                _dateTime.tm_year = now.tm_year;
                                _dateTime.tm_mon = now.tm_mon;
                                _dateTime.tm_mday = now.tm_mday;
                                _hasDate = true;
                            }
                        }
                        checkHasAllDateTimeParts();
                    }
                }
            }
            else if (ko.asap() == BASE_KoDate)
            {
                KNXValue value = "";
                // ensure we have a valid date content
                if (ko.tryValue(value, DPT_Date))
                {
                    initReceiveDateTimeStructure();
                    struct tm knxTime = value;
                    _dateTime.tm_year = knxTime.tm_year - 1900;
                    _dateTime.tm_mon = knxTime.tm_mon - 1;
                    _dateTime.tm_mday = knxTime.tm_mday;
                    _hasDate = true;
                    if (openknx.time.isValid())
                    {
                        if (!_hasTime)
                        {
                            // use current time
                            tm now;
                            openknx.time.getLocalTime().toTm(now);
                            if (now.tm_year == _dateTime.tm_year && now.tm_mon == _dateTime.tm_mon && now.tm_mday == _dateTime.tm_mday)
                            {
                                // day not changed, wait for receiving time
                            }
                            else
                            {
                                tm next = now;
                                next.tm_mday += 1;
                                mktime(&next);
                                if (next.tm_year == _dateTime.tm_year && next.tm_mon == _dateTime.tm_mon && next.tm_mday == _dateTime.tm_mday &&
                                    now.tm_hour == 23 && now.tm_min >= 60 - TIME_TOLERANCE_CHECK_MIN)
                                {
                                    // Next day has start, correct the time
                                    _dateTime.tm_hour = 0;
                                    _dateTime.tm_min = 0;
                                    _dateTime.tm_sec = 0;
                                    _timeStampTimeReceived = millis();
                                    _hasTime = true;
                                }
                                else
                                {
                                    tm previous = now;
                                    previous.tm_mday -= 1;
                                    mktime(&previous);
                                    if (previous.tm_year == _dateTime.tm_year && previous.tm_mon == _dateTime.tm_mon && previous.tm_mday == _dateTime.tm_mday &&
                                        now.tm_hour == 0 && now.tm_min <= TIME_TOLERANCE_CHECK_MIN)
                                    {
                                        // Still previous day
                                        _dateTime.tm_hour = 23;
                                        _dateTime.tm_min = 59;
                                        _dateTime.tm_sec = 59;
                                        _timeStampTimeReceived = millis();
                                        _hasTime = true;
                                    }
                                }
                            }
                        }
                    }
                    checkHasAllDateTimeParts();
                }
            }
            else if (ko.asap() == BASE_KoIsSummertime)
            {
                // <Enumeration Text="Kommunikationsobjekt 'Sommerzeit aktiv'" Value="0" Id="%ENID%" />
                // <Enumeration Text="Kombiniertes Datum/Zeit-KO (DPT 19)" Value="1" Id="%ENID%" />
                // <Enumeration Text="Interne Berechnung (nur in Deutschland)" Value="2" Id="%ENID%" />
                if (ParamBASE_SummertimeAll == 0)
                {
                    initReceiveDateTimeStructure();
                    bool newDaylightSaving = (bool)ko.value(DPT_Switch);
                    if (newDaylightSaving != _dateTime.tm_isdst)
                    {
                        _dateTime.tm_isdst = newDaylightSaving;
                        if (!_hasTime && openknx.time.isValid())
                        {
                            if (newDaylightSaving)
                                _dateTime.tm_sec -= openknx.time.daylightSavingTimeOffset();
                            else
                                _dateTime.tm_sec += openknx.time.daylightSavingTimeOffset();
                        }
                    }
                    _hasDaylightSavingFlag = true;
                    checkHasAllDateTimeParts();
                }
            }
#endif
        }
        void TimeProviderKnx::initReceiveDateTimeStructure()
        {
            if (openknx.time.isValid() && !_hasDate && !_hasTime && !_hasDaylightSavingFlag)
            {
                openknx.time.getLocalTime().toTm(_dateTime);
                _timeStampTimeReceived = millis();
            }
        }

        void TimeProviderKnx::checkHasAllDateTimeParts()
        {
            // <Enumeration Text="Kommunikationsobjekt 'Sommerzeit aktiv'" Value="0" Id="%ENID%" />
            // <Enumeration Text="Kombiniertes Datum/Zeit-KO (DPT 19)" Value="1" Id="%ENID%" />
            // <Enumeration Text="Interne Berechnung" Value="2" Id="%ENID%" />
            if (ParamBASE_SummertimeAll == 2 && _hasDate && _hasTime)
            {
                openknx.time.calculateAndSetDstFlag(_dateTime);
                _hasDaylightSavingFlag = true;
            }
            if (_hasDate && _hasTime && _hasDaylightSavingFlag)
            {
                _waitStates = WaitStates::None;
                _waitTimerStart = 0;

                setLocalTime(_dateTime, _timeStampTimeReceived);
                _hasDate = false;
                _hasTime = false;
                _hasDaylightSavingFlag = false;
            }
            else if (openknx.time.isValid() && _waitStates == WaitStates::None)
            {
                _waitTimerStart = millis();
                _waitStates = WaitStates::ReceiveMissingOtherTelegrams;
            }
        }

        bool TimeProviderKnx::supportKnxDaylightSavingTimeSwitch()
        {
            return true;
        }

    } // namespace Time
} // namespace OpenKNX