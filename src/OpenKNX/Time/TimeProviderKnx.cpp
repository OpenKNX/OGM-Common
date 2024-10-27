#include "TimeProviderKnx.h"
#include "OpenKNX.h"

// DPT19 special flags
#ifndef DPT19_FAULT
    #define DPT19_FAULT 0x80
#endif
#ifndef DPT19_WORKING_DAY
    #define DPT19_WORKING_DAY 0x40
#endif
#ifndef DPT19_NO_WORKING_DAY
    #define DPT19_NO_WORKING_DAY 0x20
#endif
#ifndef DPT19_NO_YEAR
    #define DPT19_NO_YEAR 0x10
#endif
#ifndef DPT19_NO_DATE
    #define DPT19_NO_DATE 0x08
#endif
#ifndef DPT19_NO_DAY_OF_WEEK
    #define DPT19_NO_DAY_OF_WEEK 0x04
#endif
#ifndef DPT19_NO_TIME
    #define DPT19_NO_TIME 0x02
#endif
#ifndef DPT19_SUMMERTIME
    #define DPT19_SUMMERTIME 0x01
#endif

#ifndef VAL_STIM_FROM_DPT19
    #define VAL_STIM_FROM_DPT19 1
#endif

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
                    auto prefix = _waitStates == InitialRead ? "initil" : "missing";
                    if (!_hasDate)
                    {
                        if (ParamBASE_CombinedTimeDate)
                            logInfoP("Wait for %s diagram date time", prefix);
                        else
                            logInfoP("Wait for %s diagram date", prefix);
                    }
                    if (!_hasTime)
                        logInfoP("Wait for %s diagram time", prefix);
                    if (!_hasDaylightSavingFlag)
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
                _waitTimerStart = millis() - DelayInitialReadInMs + InitialReadDelayInMs; // first read after 5 seconds
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
                {
                    // read on start
                    if (_waitTimerStart != 0 && millis() - _waitTimerStart >= DelayInitialReadInMs)
                    {
                        logErrorP("Wait end for read on start");
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
                                logErrorP("Read KoBASE_DateTime");
                            }
                        }
                        else
                        {
                            // date and time from separate KOs
                            if (!_hasTime && !_disableKoRead)
                            {
                                KoBASE_Time.requestObjectRead();
                                logErrorP("Read KoBASE_Time");
                            }
                            if (!_hasDate && !_disableKoRead)
                            {
                                KoBASE_Date.requestObjectRead();
                                logErrorP("Read KoBASE_Date");
                            }
                        }
                        if (!_hasDaylightSavingFlag && ParamBASE_SummertimeAll == 0 && !_disableKoRead)
                        {
                            KoBASE_IsSummertime.requestObjectRead();
                            logErrorP("Read KoBASE_IsSummertime");
                        }
                    }
                    break;
                }
                case WaitStates::ReceiveMissingOtherTelegrams:
                {
                    if (_waitTimerStart != 0 && millis() - _waitTimerStart >= WaitTimeMissingOtherDelegramsInMs)
                    {
                        logErrorP("Wait end for ReceiveMissingOtherTelegrams");
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
                auto receiveTimeStamp = millis();
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
                                logErrorP("Read daylight saving time flag from telegram %d", (int)lDST);
                                _dateTime.tm_isdst = lDST;
                                _hasDaylightSavingFlag = true;
                            }
                            checkHasAllDateTimeParts();
                        }
                    }
                }
                else
                {
                    auto receiveTimeStamp = millis();
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
                        if (openknx.time.isTimeValid())
                        {
                            // time is already valid, use current date
                            auto now = openknx.time.getLocalTime();
                            if (knxTime.tm_hour == 0 && now.tm_hour == 23)
                            {
                                logErrorP("New day started");
                                // New day started, correct date to use it
                                now.tm_mday += 1;
                                mktime(&now); // normalize
                            }
                            _dateTime.tm_year = now.tm_year;
                            _dateTime.tm_mon = now.tm_mon;
                            _dateTime.tm_mday = now.tm_mday;
                            _hasDate = true;
                            if (!_hasDaylightSavingFlag)
                            {
                                // use the current DST flag
                                _dateTime.tm_isdst = now.tm_isdst;
                                _hasDaylightSavingFlag = true;
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
                    if (openknx.time.isTimeValid())
                    {
                        auto now = openknx.time.getLocalTime();
                        if (now.tm_year == _dateTime.tm_year && now.tm_mon == _dateTime.tm_mon && now.tm_mday == _dateTime.tm_mday)
                        {
                            // day not changed, wait for receiving time
                        }
                        else
                        {
                            now.tm_mday += 1;
                            mktime(&now);
                            if (now.tm_year == _dateTime.tm_year && now.tm_mon == _dateTime.tm_mon && now.tm_mday == _dateTime.tm_mday && (now.tm_hour == 23 || now.tm_hour == 0))
                            {
                                // Next day has start, correct the time
                                _dateTime.tm_hour = 0;
                                _dateTime.tm_min = 0;
                                _dateTime.tm_sec = 0;
                                _timeStampTimeReceived = millis();
                                _hasTime = true;
                                if (!_hasDaylightSavingFlag)
                                {
                                    _dateTime.tm_isdst = now.tm_isdst;
                                    _hasDaylightSavingFlag = true;
                                }
                            }
                        }
                    }
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
                        if (!_hasTime && openknx.time.isTimeValid())
                        {
                            if (newDaylightSaving)
                                _dateTime.tm_sec -= openknx.time.daylightSavingTimeOffset();
                            else
                                _dateTime.tm_sec += openknx.time.daylightSavingTimeOffset();
                        }
                    }
                     logErrorP("Receive daylight saving time: %d", (int) newDaylightSaving);
                    _hasDaylightSavingFlag = true;
                    checkHasAllDateTimeParts();
                }
            }
#endif
        }
        void TimeProviderKnx::initReceiveDateTimeStructure()
        {
            if (openknx.time.isTimeValid() && !_hasDate && !_hasTime && !_hasDaylightSavingFlag)
            {
                _dateTime = openknx.time.getLocalTime();
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

                logErrorP("Set %04d-%02d-%02d %02d:%02d:%02d (%s) with offset of %dms",
                          _dateTime.tm_year + 1900,
                          _dateTime.tm_mon + 1,
                          _dateTime.tm_mday,
                          _dateTime.tm_hour,
                          _dateTime.tm_min,
                          _dateTime.tm_sec,
                          _dateTime.tm_isdst ? "DST" : "ST",
                          millis() - _timeStampTimeReceived);
                setLocalTime(_dateTime, _timeStampTimeReceived);
                _hasDate = false;
                _hasTime = false;
                _hasDaylightSavingFlag = false;
            }
            else if (openknx.time.isTimeValid() && _waitStates == WaitStates::None)
            {
                logErrorP("Not all parts received, start wait for missing telegrams");
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