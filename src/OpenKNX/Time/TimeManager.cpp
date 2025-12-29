#include "TimeManager.h"
#include "../Log/Logger.h"
#include "DPT19Flags.h"
#include "TimeProvider.h"
#include "TimeProviderKnx.h"
#ifndef ParamBASE_InternalTime
    #define ParamBASE_InternalTime 0
#endif
namespace OpenKNX
{
    namespace Time
    {
        const std::string TimeManager::logPrefix()
        {
            return std::string("Time");
        }
        TimeProvider* TimeManager::getTimeProvder()
        {
            return _timeProvider;
        }
        bool TimeManager::hasTimerProvder()
        {
            return _timeProvider;
        }

#ifdef OPENKNX_TIME_TESTCOMMAND
        void TimeManager::commandTest()
        {
            const char* tz = getenv("TZ");
            if (tz == nullptr)
                tz = "";
            std::string previousTimezone = tz;
            const char* timezoneString = "CET-1CEST,M3.5.0/2:00:00,M10.5.0/3:00:00"; // Germany
            setenv("TZ", timezoneString, 1);
            tzset();

            logDebugP("    Calculated-> X X <- Expected");
            logDebugP("29.3.2024 23:59 %2d  0", (int)isDaylightSavingTime(2024, 3, 29, 23, 59));
            logDebugP("30.3.2024 00:00 %2d  0", (int)isDaylightSavingTime(2024, 3, 30, 0, 0));

            logDebugP("30.3.2024 01:59 %2d  0", (int)isDaylightSavingTime(2024, 3, 30, 1, 59));

            logDebugP("31.3.2024 01:59 %2d  0", (int)isDaylightSavingTime(2024, 3, 31, 1, 59));
            logDebugP("31.3.2024 02:00 %2d  does not exist", (int)isDaylightSavingTime(2024, 3, 31, 2, 00));
            logDebugP("31.3.2024 02:01 %2d  does not exist", (int)isDaylightSavingTime(2024, 3, 31, 2, 01));

            logDebugP("31.3.2024 02:59 %2d  does not exist", (int)isDaylightSavingTime(2024, 3, 31, 2, 59));
            logDebugP("31.3.2024 03:00 %2d  1", (int)isDaylightSavingTime(2024, 3, 31, 3, 00));
            logDebugP("31.3.2024 03:01 %2d  1", (int)isDaylightSavingTime(2024, 3, 31, 4, 01));

            logDebugP("27.10.2024 01:59 %2d  1", (int)isDaylightSavingTime(2024, 10, 27, 1, 59));
            logDebugP("27.10.2024 02:00 %2d -1 could be DST or ST", (int)isDaylightSavingTime(2024, 10, 27, 2, 00));
            logDebugP("27.10.2024 02:01 %2d -1 could be DST or ST", (int)isDaylightSavingTime(2024, 10, 27, 2, 01));

            logDebugP("27.10.2024 02:59 %2d -1 could be DST or ST", (int)isDaylightSavingTime(2024, 10, 27, 2, 59));
            logDebugP("27.10.2024 03:00 %2d  0", (int)isDaylightSavingTime(2024, 10, 27, 3, 00));
            logDebugP("27.10.2024 03:01 %2d  0", (int)isDaylightSavingTime(2024, 10, 27, 3, 01));
            logDebugP("27.10.2024 23:59 %2d  0", (int)isDaylightSavingTime(2024, 10, 27, 23, 59));
            logDebugP("28.10.2024 00:00 %2d  0", (int)isDaylightSavingTime(2024, 10, 28, 00, 00));
            logDebugP("29.10.2024 00:00 %2d  0", (int)isDaylightSavingTime(2024, 10, 29, 00, 00));
            logDebugP("03.11.2024 00:00 %2d  0", (int)isDaylightSavingTime(2024, 11, 3, 00, 00));
            logDebugP("04.11.2024 00:00 %2d  0", (int)isDaylightSavingTime(2024, 11, 4, 00, 00));

            setenv("TZ", previousTimezone.c_str(), 1);
            tzset();

            DateTime dt;
            dt = DateTime(2024, 7, 1, 15, 2, 3, DateTimeTypeLocalTimeDST);
            logDebugP("2024.07.01 15:02:03 DST = %s", dt.toString().c_str());
            dt = dt.toUtc();
            logDebugP("CONVERTED 13:02:03 UTC = %s", dt.toString().c_str());

            dt = DateTime(2024, 7, 1, 15, 2, 3, DateTimeTypeUTC);
            logDebugP("2024.07.01 15:02:03 UTC = %s", dt.toString().c_str());
            dt = dt.toLocalTime();
            logDebugP("CONVERTED 17:02:03 DST = %s", dt.toString().c_str());

            dt.addDays(-1);
            logDebugP("-1d 2024.06.30 17:02:03 ST = %s", dt.toString().c_str());

            dt.addHours(-1);
            logDebugP("-1h 2024.06.30 16:02:03 ST = %s", dt.toString().c_str());

            dt = DateTime(2024, 12, 1, 15, 2, 3, DateTimeTypeLocalTimeST);
            logDebugP("2024.12.01 15:02:03 ST = %s", dt.toString().c_str());
            dt = dt.toUtc();
            logDebugP("CONVERTED 14:02:03 UTC = %s", dt.toString().c_str());

            dt.addDays(-1);
            logDebugP("-1d 2024.11.30 14:02:03 UTC = %s", dt.toString().c_str());

            dt.addHours(-1);
            logDebugP("-1h 2024.11.30 13:02:03 UTC = %s", dt.toString().c_str());

            dt = DateTime(2024, 12, 1, 15, 2, 3, DateTimeTypeUTC);
            logDebugP("2024.12.01 15:02:03 UTC = %s", dt.toString().c_str());
            dt = dt.toLocalTime();
            logDebugP("CONVERTED 16:02:03 ST = %s", dt.toString().c_str());

            dt.addDays(-1);
            logDebugP("-1d 2024.11.30 16:02:03 ST = %s", dt.toString().c_str());

            dt.addHours(-1);
            logDebugP("-1h 2024.11.30 15:02:03 ST = %s", dt.toString().c_str());
        }
#endif

#ifdef OPENKNX_TIME_DIGAGNOSTIC
        void TimeManager::commandHelp()
        {
            openknx.console.printHelpLine("tm", "Show time information");
            openknx.console.printHelpLine("tm setdst", "Set daylight saving time (for testing)");
            openknx.console.printHelpLine("tm setst", "Set standard time (for testing)");
            openknx.console.printHelpLine("tm calcdst", "Calculate daylight saving time (for testing)");
            openknx.console.printHelpLine("tm x", "Set time to 2024-07-01 15:00 UTC (for testing)");
            openknx.console.printHelpLine("tm y", "Set time to 2024-12-01 15:00 UTC (for testing)");
            openknx.console.printHelpLine("tm hhmm", "Set time to hh:mm UTC (for testing)");
            openknx.console.printHelpLine("tm YYMMDDhhmm", "Set time to 20YY-MM-DD hh:mm UTC (for testing)");
    #ifdef OPENKNX_TIME_TESTCOMMAND
            openknx.console.printHelpLine("tm test", "Test some calculation (for testing)");
    #endif
        }
#endif
        void TimeManager::commandInformation()
        {
            if (isValid())
            {
                DateTime time = getUtcTime();
                logInfoP("%s", time.toString().c_str());
                time = time.toLocalTime();
                logInfoP("%s", time.toString().c_str());
                logInfoP("%s", time.dayOfWeekString());
                if (isInaccurate())
                    logInfoP("Time is inaccurate");
#ifdef LOG_HolidayKo
                if (openknx.calendar.isWorkingDayToday())
                    logInfoP("Today is a working day");
                else
                    logInfoP("Today is a non-working day");
                if (openknx.calendar.isWorkingDayTomorrow())
                    logInfoP("Tomorrow is a working day");
                else
                    logInfoP("Tomorrow is a non-working day");
#endif
            }
            else
                logInfoP("No valid time");
            const char* tz = getenv("TZ");
            if (tz == nullptr)
                logInfoP("No timezone set");
            else
            {
                logInfoP("Timezone: %s", buildTimezoneString(DaylightSavingMode::Calculated).c_str());
                if (_daylightSavingMode != DaylightSavingMode::Calculated)
                    logInfoP("Used timezone: %s", tz);
            }
            switch (_daylightSavingMode)
            {
                case DaylightSavingMode::AlwaysDayLightSavingTime:
                    logInfoP("Mode: daylight saving time");
                    break;
                case DaylightSavingMode::AlwaysStandardTime:
                    logInfoP("Mode: standard time");
                    break;
                case DaylightSavingMode::Calculated:
                    logInfoP("Mode: calculate daylight saving time");
                    break;
            }
            logInfoP("Offset for daylight saving time: %ds", (int)_dayLightSavingTimeOffset);
            if (openknx.calendar.isValid())
            {
                DateOnly easter = openknx.calendar.getEaster();
                logInfoP("Easter: %04d-%02d-%02d", easter.year, easter.month, easter.day);
                DateOnly advent = openknx.calendar.getForthAdvent();
                logInfoP("4th advent: %04d-%02d-%02d", advent.year, advent.month, advent.day);
            }

            if (!hasTimerProvder())
                logErrorP("No timeprovider set");
            else
            {
                if (knx.configured())
                    _timeProvider->logInformation();
                else
                    logInfoP("Time provider activated because not ETS programming is missing.");
            }
        }
#ifdef OPENKNX_TIME_DIGAGNOSTIC
        void TimeManager::commandSetDateTime(std::string& cmd)
        {
            logInfoP("Set date/time");
            tm tm = {0};
            if (cmd == "tm x")
            {
                tm.tm_year = 2024 - 1900;
                tm.tm_mon = 7 - 1;
                tm.tm_mday = 1;
                tm.tm_hour = 15;
                tm.tm_min = 0;
            }
            else if (cmd == "tm y")
            {
                tm.tm_year = 2024 - 1900;
                tm.tm_mon = 12 - 1;
                tm.tm_mday = 1;
                tm.tm_hour = 15;
                tm.tm_min = 0;
            }
            else if (cmd.length() == 7)
            {
                tm.tm_year = (isValid() ? getLocalTime().year : 2024) - 1900;
                tm.tm_mon = (isValid() ? getLocalTime().month : 7) - 1;
                tm.tm_mday = isValid() ? getLocalTime().day : 1;
                tm.tm_hour = stoi(cmd.substr(3, 2));
                tm.tm_min = stoi(cmd.substr(5, 2));
            }
            else if (cmd.length() == 13)
            {
                tm.tm_year = stoi(cmd.substr(3, 2)) + 2000 - 1900;
                tm.tm_mon = stoi(cmd.substr(5, 2)) - 1;
                tm.tm_mday = stoi(cmd.substr(7, 2));
                tm.tm_hour = stoi(cmd.substr(9, 2));
                tm.tm_min = stoi(cmd.substr(11, 2));
            }
            else
            {
                logInfoP("Invalid time format");
                return;
            }
            calculateAndSetDstFlag(tm);
            setLocalTime(tm, millis());
        }
#endif
        bool TimeManager::processCommand(std::string& cmd, bool diagnoseKo)
        {
            if (cmd.rfind("tm") == 0)
            {
                if (cmd == "tm")
                {
                    commandInformation();
                    return true;
                }
                if (cmd == "tm unittest")
                {
                    DateTime dt;
                    dt = DateTime(2024, 7, 1, 15, 2, 3, DateTimeTypeLocalTimeDST);
                    logDebugP("2024.07.01 15:02:03 DST = %s", dt.toString().c_str());
                    dt = dt.toUtc();
                    logDebugP("CONVERTED 13:02:03 UTC = %s", dt.toString().c_str());

                    dt = DateTime(2024, 7, 1, 15, 2, 3, DateTimeTypeUTC);
                    logDebugP("2024.07.01 15:02:03 UTC = %s", dt.toString().c_str());
                    dt = dt.toLocalTime();
                    logDebugP("CONVERTED 17:02:03 DST = %s", dt.toString().c_str());

                    dt.addDays(-1);
                    logDebugP("-1d 2024.06.30 17:02:03 ST = %s", dt.toString().c_str());

                    dt.addHours(-1);
                    logDebugP("-1h 2024.06.30 16:02:03 ST = %s", dt.toString().c_str());

                    dt = DateTime(2024, 12, 1, 15, 2, 3, DateTimeTypeLocalTimeST);
                    logDebugP("2024.12.01 15:02:03 ST = %s", dt.toString().c_str());
                    dt = dt.toUtc();
                    logDebugP("CONVERTED 14:02:03 UTC = %s", dt.toString().c_str());

                    dt.addDays(-1);
                    logDebugP("-1d 2024.11.30 14:02:03 UTC = %s", dt.toString().c_str());

                    dt.addHours(-1);
                    logDebugP("-1h 2024.11.30 13:02:03 UTC = %s", dt.toString().c_str());

                    dt = DateTime(2024, 12, 1, 15, 2, 3, DateTimeTypeUTC);
                    logDebugP("2024.12.01 15:02:03 UTC = %s", dt.toString().c_str());
                    dt = dt.toLocalTime();
                    logDebugP("CONVERTED 16:02:03 ST = %s", dt.toString().c_str());

                    dt.addDays(-1);
                    logDebugP("-1d 2024.11.30 16:02:03 ST = %s", dt.toString().c_str());

                    dt.addHours(-1);
                    logDebugP("-1h 2024.11.30 15:02:03 ST = %s", dt.toString().c_str());

                    return true;
                }
                if (cmd == "tm test")
                {
#ifdef OPENKNX_TIME_TESTCOMMAND
                    commandTest();
                    return true;
#else
                    return false;
#endif
                }
#ifdef OPENKNX_TIME_DIGAGNOSTIC
                if (cmd == "tm ?")
                {
                    commandHelp();
                    return true;
                }
                if (cmd == "tm setdst")
                {
                    setDaylightSavingMode(DaylightSavingMode::AlwaysDayLightSavingTime);
                    return true;
                }
                if (cmd == "tm setst")
                {
                    setDaylightSavingMode(DaylightSavingMode::AlwaysStandardTime);
                    return true;
                }
                if (cmd == "tm calcdst")
                {
                    setDaylightSavingMode(DaylightSavingMode::Calculated);
                    return true;
                }

                if (cmd.rfind("tm ") == 0) // must be the last command check
                {
                    commandSetDateTime(cmd);
                    return true;
                }
#endif
            }
            return false;
        }

        int TimeManager::daylightSavingTimeOffset()
        {
            return _dayLightSavingTimeOffset;
        }

        void TimeManager::setTimeProvider(TimeProvider* timeProvider)
        {
            if (_timeProvider != nullptr)
                delete _timeProvider;
            _timeProvideSupportKnxDaylightSavingTimeSwitch = false;
            _timeProvider = timeProvider;
            if (timeProvider != nullptr)
                _timeProvideSupportKnxDaylightSavingTimeSwitch = timeProvider->supportKnxDaylightSavingTimeSwitch();
            if (_configured && _timeProvider != nullptr)
                _timeProvider->setup();
            _waitTimerReadKo = 0;
#ifdef BASE_KoIsSummertime
            if (!ParamBASE_InternalTime && _timeProvideSupportKnxDaylightSavingTimeSwitch && ParamBASE_SummertimeAll == 0 /*Kommunikationsobjekt 'Sommerzeit aktiv'*/ && ParamBASE_ReadTimeDate)
            {
                _waitTimerReadKo = delayTimerInit();
                _intialReadKo = true; // first read after 5 seconds
            }
#endif
        }

        void TimeManager::setup(bool configured)
        {
            _timeLed = openknx.ledFunctions.get(OPENKNX_LEDFUNC_BASE_TIME);

            _configured = configured;
            setDaylightSavingMode(DaylightSavingMode::Calculated);

            _timeClock.setup();
            tm tm = {0};
            tm.tm_year = 2024 - 1900;
            tm.tm_mon = 6 - 1;
            tm.tm_mday = 1;
            tm.tm_hour = 12;
            tm.tm_min = 0;
            time_t st = mktime(&tm);
            tm.tm_hour = 12;
            tm.tm_min = 0;
            tm.tm_isdst = 1;
            time_t dst = mktime(&tm);
            _dayLightSavingTimeOffset = dst - st;

            if (configured)
            {
                // <Enumeration Text="Kommunikationsobjekt 'Sommerzeit aktiv'" Value="0" Id="%ENID%" />
                // <Enumeration Text="Kombiniertes Datum/Zeit-KO (DPT 19)" Value="1" Id="%ENID%" />
                // <Enumeration Text="Interne Berechnung" Value="2" Id="%ENID%" />
                if (ParamBASE_SummertimeAll == 2 || ParamBASE_InternalTime)
                    setDaylightSavingMode(DaylightSavingMode::Calculated);
                else
                    setDaylightSavingMode(DaylightSavingMode::AlwaysStandardTime);

                if (_timeProvider != nullptr)
                    _timeProvider->setup();
            }
        }

        void TimeManager::setDaylightSavingMode(DaylightSavingMode daylightSavingMode)
        {
            _daylightSavingMode = daylightSavingMode;
            std::string timezoneString = buildTimezoneString(daylightSavingMode);
            setenv("TZ", timezoneString.c_str(), 1);
            tzset();
        }

        std::string TimeManager::buildTimezoneString(DaylightSavingMode daylightSavingMode)
        {
            // <Enumeration Text="Amsterdam, Berlin, Bern, Rom, Wien (+1 Stunde)" Value="1" Id="%ENID%" />
            // <Enumeration Text="Benutzerdefiniert" Value="31" Id="%ENID%" />
            // <Enumeration Text="UTC -11 Stunden" Value="27" Id="%ENID%" />
            // <Enumeration Text="UTC -10 Stunden" Value="26" Id="%ENID%" />
            // <Enumeration Text="UTC -9 Stunden" Value="25" Id="%ENID%" />
            // <Enumeration Text="UTC -8 Stunden" Value="24" Id="%ENID%" />
            // <Enumeration Text="UTC -7 Stunden" Value="23" Id="%ENID%" />
            // <Enumeration Text="UTC -6 Stunden" Value="22" Id="%ENID%" />
            // <Enumeration Text="UTC -5 Stunden" Value="21" Id="%ENID%" />
            // <Enumeration Text="UTC -4 Stunden" Value="20" Id="%ENID%" />
            // <Enumeration Text="UTC -3 Stunden" Value="19" Id="%ENID%" />
            // <Enumeration Text="UTC -2 Stunden" Value="18" Id="%ENID%" />
            // <Enumeration Text="UTC -1 Stunde" Value="17" Id="%ENID%" />
            // <Enumeration Text="UTC +0 Stunden" Value="0" Id="%ENID%" />
            // <Enumeration Text="UTC +1 Stunden" Value="28" Id="%ENID%" />
            // <Enumeration Text="UTC +2 Stunden" Value="2" Id="%ENID%" />
            // <Enumeration Text="UTC +3 Stunden" Value="3" Id="%ENID%" />
            // <Enumeration Text="UTC +4 Stunden" Value="4" Id="%ENID%" />
            // <Enumeration Text="UTC +5 Stunden" Value="5" Id="%ENID%" />
            // <Enumeration Text="UTC +6 Stunden" Value="6" Id="%ENID%" />
            // <Enumeration Text="UTC +7 Stunden" Value="7" Id="%ENID%" />
            // <Enumeration Text="UTC +8 Stunden" Value="8" Id="%ENID%" />
            // <Enumeration Text="UTC +9 Stunden" Value="9" Id="%ENID%" />
            // <Enumeration Text="UTC +10 Stunden" Value="10" Id="%ENID%" />
            // <Enumeration Text="UTC +11 Stunden" Value="11" Id="%ENID%" />
            // <Enumeration Text="UTC +12 Stunden" Value="12" Id="%ENID%" />
            const char* timezoneString = "CET-1CEST,M3.5.0/2:00:00,M10.5.0/3:00:00"; // Germany
            if (_configured)
            {
                switch (ParamBASE_Timezone)
                {
                    case 1: // Germany
                        // Use default
                        break;
                    case 31: // Custom timezone
                        timezoneString = (const char*)ParamBASE_TimezoneCustom;
                        break;
                    case 27:
                        timezoneString = "EST11EST10,366,367";
                        break;
                    case 26:
                        timezoneString = "EST10EST9,366,367";
                        break;
                    case 25:
                        timezoneString = "EST9EST8,366,367";
                        break;
                    case 24:
                        timezoneString = "EST8EST7,366,367";
                        break;
                    case 23:
                        timezoneString = "EST7EST6,366,367";
                        break;
                    case 22:
                        timezoneString = "EST6EST5,366,367";
                        break;
                    case 21:
                        timezoneString = "EST5EST4,366,367";
                        break;
                    case 20:
                        timezoneString = "EST4EST3,366,367";
                        break;
                    case 19:
                        timezoneString = "EST3EST2,366,367";
                        break;
                    case 18:
                        timezoneString = "EST2EST1,366,367";
                        break;
                    case 17:
                        timezoneString = "EST1EST0,366,367";
                        break;
                    case 0:
                        timezoneString = "EST0CET-1,366,367";
                        break;
                    case 28:
                        timezoneString = "CET-1CET-2,366,367";
                        break;
                    case 2:
                        timezoneString = "CET-2CET-3,366,367";
                        break;
                    case 3:
                        timezoneString = "CET-3CET-4,366,367";
                        break;
                    case 4:
                        timezoneString = "CET-4CET-5,366,367";
                        break;
                    case 5:
                        timezoneString = "CET-5CET-6,366,367";
                        break;
                    case 6:
                        timezoneString = "CET-CET-7,366,367";
                        break;
                    case 7:
                        timezoneString = "CET-7CET-8,366,367";
                        break;
                    case 8:
                        timezoneString = "CET-8CET-9,366,367";
                        break;
                    case 9:
                        timezoneString = "CET-9CET-10,366,367";
                        break;
                    case 10:
                        timezoneString = "CET-10CET-11,366,367";
                        break;
                    case 11:
                        timezoneString = "CET-11CET-12,366,367";
                        break;
                    case 12:
                        timezoneString = "CET-12CET-13,366,367";
                        break;
                }
            }
            if (daylightSavingMode == DaylightSavingMode::Calculated)
                return std::string(timezoneString);
            const char* seperator = strstr(timezoneString, ",");
            std::string result;
            if (seperator == nullptr)
                result = timezoneString;
            else
                result = std::string(timezoneString).substr(0, seperator - timezoneString + 1);

            if (daylightSavingMode == DaylightSavingMode::AlwaysDayLightSavingTime)
                result += "0,366"; // Always daylight saving tiem
            else
                result += "366,367"; // Always standard time
            return std::string(result);
        }

        void TimeManager::sendTime()
        {
            _lastSendSecond = 255;
        }

        void TimeManager::loop()
        {
            _timeClock.loop();
            _lastTimeStamp = _timeClock.getTime();
            if (_timeProvider != nullptr)
                _timeProvider->loop();

            DateTime localTime = DateTime(_lastTimeStamp);
            TimeChangedEvents timeChangeEvents = (TimeChangedEvents) 0;
            if (_timeSetEventNeeded)
            {
                timeChangeEvents = (TimeChangedEvents) (timeChangeEvents | TimeChangedEventTimeSet);
                _timeSetEventNeeded = false;
            }
            if (_lastLocalTimeInLoop.second != localTime.second)
                timeChangeEvents = (TimeChangedEvents) (timeChangeEvents | TimeChangedEventSecondChanged);
            if (_lastLocalTimeInLoop.minute != localTime.minute)
                timeChangeEvents = (TimeChangedEvents) (timeChangeEvents | TimeChangedEventMinuteChanged);
            if (_lastLocalTimeInLoop.hour != localTime.hour)
                timeChangeEvents = (TimeChangedEvents) (timeChangeEvents | TimeChangedEventHourChanged);
            if (_lastLocalTimeInLoop.day != localTime.day)
                timeChangeEvents = (TimeChangedEvents) (timeChangeEvents | TimeChangedEventDayChanged);
            if (_lastLocalTimeInLoop.month != localTime.month)
                timeChangeEvents = (TimeChangedEvents) (timeChangeEvents | TimeChangedEventMonthChanged);
            if (_lastLocalTimeInLoop.year != localTime.year)
                timeChangeEvents = (TimeChangedEvents) (timeChangeEvents | TimeChangedEventYearChanged);
            
            _lastLocalTimeInLoop = localTime;
            bool isValidState = calculateValid(_lastTimeStamp);
            if (_lastValidInLoop != isValidState)
            {    
                timeChangeEvents = (TimeChangedEvents) (timeChangeEvents | TimeChangedEventValidChanged);
                _lastValidInLoop = isValidState;
            }
            bool isInaccurateState = calculateInaccurate(_lastTimeStamp);
            if (_lastInaccurateInLoop != isInaccurateState)
            {
                timeChangeEvents = (TimeChangedEvents) (timeChangeEvents | TimeChangedEventInaccurateChanged);
                _lastInaccurateInLoop = isInaccurateState;
            }

            TimeChangedArgs timeArgs;
            timeArgs.events = timeChangeEvents;
            timeArgs.localTime = localTime;
            timeArgs.isInaccurate = isInaccurateState;
            timeArgs.isValid = isValidState;
            if (timeChangeEvents != 0)
                sendEvent(timeArgs);
            loopLed(timeArgs);

            if (ParamBASE_InternalTime && isValidState)
            {
                if (localTime.second != _lastSendSecond || localTime.minute != _lastSendMinute || localTime.hour != _lastSendHour)
                {
                    bool forceSend = _lastSendSecond == 255;
                    _lastSendSecond = localTime.second;
                    _lastSendMinute = localTime.minute;
                    _lastSendHour = localTime.hour;

#if defined(KoBASE_Time) || defined(KoBASE_Date) || defined(KoBASE_DateTime)
                    const uint8_t knxDayOfWeek = localTime.dayOfWeek == 0 ? 7 : localTime.dayOfWeek;

                    tm knxTime;
                    knxTime.tm_year = localTime.year;
                    knxTime.tm_mon = localTime.month;
                    knxTime.tm_mday = localTime.day;
                    knxTime.tm_hour = localTime.hour;
                    knxTime.tm_min = localTime.minute;
                    knxTime.tm_sec = localTime.second;
#endif
#ifdef KoBASE_Time
                    // begin updating time KO, and set additional day-of-week after
                    KoBASE_Time.valueNoSend(knxTime, DPT_TimeOfDay);

                    // additional setting of the week-day
                    // DPT10.001 format: 0b DDDH_HHHH 00MM_MMMM 00SS_SSSS
                    //           day-of-week^  ^hour    ^minute   ^second
                    uint8_t* rawDayTime = KoBASE_Time.valueRef();
                    rawDayTime[0] = (rawDayTime[0] & ~0xE0) | ((knxDayOfWeek << 5) & 0xE0);
#endif
#ifdef KoBASE_Date
                    // update date KO
                    KoBASE_Date.valueNoSend(knxTime, DPT_Date);
#endif
#ifdef KoBASE_DateTime
                    // begin updating date-time KO, and set additional fields after
                    KoBASE_DateTime.valueNoSend(knxTime, DPT_DateTime);
                    uint8_t* raw = KoBASE_DateTime.valueRef();

                    // additional setting of the week-day
                    //                      [0] [1] [2] [   3   ] [4] [5] [   6   ] [   7   ]
                    // DPT19.001 format: 0b ... ... ... DDDH_HHHH ... ... BBBB_BBBB BB00_0000
                    //                       day-of-week^  ^hour          ^..flags..^
                    raw[3] = (raw[3] & ~0xE0) | ((knxDayOfWeek << 5) & 0xE0);

                    if (localTime.isDst)
                        raw[6] |= DPT19_SUMMERTIME;
                    else
                        raw[6] &= ~DPT19_SUMMERTIME;

                    raw[6] &= ~DPT19_WORKING_DAY; // not supported, always 0
                    raw[6] |= DPT19_NO_WORKING_DAY;

                    raw[6] &= ~DPT19_FAULT;
                    raw[6] &= ~DPT19_NO_YEAR;
                    raw[6] &= ~DPT19_NO_DATE;
                    raw[6] &= ~DPT19_NO_DAY_OF_WEEK;
                    raw[6] &= ~DPT19_NO_TIME;
#endif
#ifdef KoBASE_IsSummertime
                    KoBASE_IsSummertime.valueNoSend(localTime.isDst != 0, DPT_Switch);
#endif

                    if ((_lastSendMinute % 10 == 0 && _lastSendSecond == 0) || forceSend)
                    {
#ifdef KoBASE_DateTime
                        KoBASE_DateTime.objectWritten();
#endif
#ifdef KoBASE_Time
                        KoBASE_Time.objectWritten();
#endif
#ifdef KoBASE_Date
                        KoBASE_Date.objectWritten();
#endif
#ifdef KoBASE_IsSummertime
                        KoBASE_IsSummertime.objectWritten();
#endif
                    }
                }
            }

#ifdef KoBASE_IsSummertime
            // <Enumeration Text="Kommunikationsobjekt 'Sommerzeit aktiv'" Value="0" Id="%ENID%" />
            // <Enumeration Text="Kombiniertes Datum/Zeit-KO (DPT 19)" Value="1" Id="%ENID%" />
            // <Enumeration Text="Interne Berechnung" Value="2" Id="%ENID%" />
            if (_waitTimerReadKo != 0 && delayCheckMillis(_waitTimerReadKo, (unsigned long)(_intialReadKo ? TimeProviderKnx::InitialReadAfterInMs : TimeProviderKnx::DelayInitialReadInMs)))
            {
                _intialReadKo = false;
                _waitTimerReadKo = millis();
                // Kommunikationsobjekt 'Sommerzeit aktiv'
                KoBASE_IsSummertime.requestObjectRead();
            }
#endif
        }

        void TimeManager::registerCallback(TimeChangedEvents events, TimeChangeCallback callback)
        {
            TimeChangedEventHandler handler;
            handler.events = events;
            handler.callback = callback;
            _callbacks.push_back(handler);
        }

        void TimeManager::sendEvent(TimeChangedArgs args)
        {
            for (auto& handler : _callbacks)
            {
                if ((handler.events & args.events) != 0)
                {
                    handler.callback(args);
                }
            }
        }

        void TimeManager::loopLed(TimeChangedArgs args)
        {
            if (!_timeLed->active())
                return;

            if (_timeUpdatedActivity != 0)
            {
                // time was updated
                if (_ledState != 2)
                {
                    _ledState = 2;
                    _timeLed->color(Led::Color::Blue);
                    _timeLed->on(Led::Capability::COLOR);
                    _timeLed->blinking(100, Led::Capability::MONOCHROME);
                }
                if (delayCheck(_timeUpdatedActivity, 1000))
                {
                    _timeUpdatedActivity = 0;
                }
            }
            else if (args.isValid)
            {
                if (args.isInaccurate)
                {
                    // time is valid but inaccurate
                    if (_ledState != 4)
                    {
                        _ledState = 4;
                        _timeLed->color(Led::Color::Yellow);
                        _timeLed->on(Led::Capability::COLOR);
                        _timeLed->blinking(1000, Led::Capability::MONOCHROME);
                    }
                }
                else
                {
                    // time is valid and accurate
                    if (_ledState != 3)
                    {
                        _ledState = 3;
                        _timeLed->color(Led::Color::Green);
                        _timeLed->on();
                    }
                }
            }
            else
            {
                // time is invalid
                if (_ledState != 1)
                {
                    _ledState = 1;
                    _timeLed->color(Led::Color::Red);
                    _timeLed->on(Led::Capability::COLOR);
                    _timeLed->off(Led::Capability::MONOCHROME);
                }
            }
        }

        bool TimeManager::calculateValid(time_t time)
        {
            return time > 1704070800; // 2024-01-01
        }

        bool TimeManager::isValid()
        {
            time_t now = _timeClock.getTime();
            return calculateValid(now);
        }

        bool TimeManager::calculateInaccurate(time_t time)
        {
            if (!calculateValid(time))
                return true;
            if (_lastUpdatedByProviderTimeStamp == 0)   
                return true;
            return time - _lastUpdatedByProviderTimeStamp > 24 * 60 * 60; // more than 24 hours
        }

        bool TimeManager::isInaccurate()
        {
            time_t now = _timeClock.getTime();
            return calculateInaccurate(now);
        }

        DateTime TimeManager::getLocalTime()
        {
            return DateTime(_timeClock.getTime());
        }

        DateTime TimeManager::getUtcTime()
        {
            return DateTime(_timeClock.getTime(), true);
        }

        void TimeManager::processInputKo(GroupObject& ko)
        {
            if (_timeProvider != nullptr)
            {
                _timeProvider->processInputKo(ko);
            }
#ifdef BASE_KoIsSummertime
            // <Enumeration Text="Kommunikationsobjekt 'Sommerzeit aktiv'" Value="0" Id="%ENID%" />
            // <Enumeration Text="Kombiniertes Datum/Zeit-KO (DPT 19)" Value="1" Id="%ENID%" />
            // <Enumeration Text="Interne Berechnung" Value="2" Id="%ENID%" />
            if (!_timeProvideSupportKnxDaylightSavingTimeSwitch && ko.asap() == BASE_KoIsSummertime && ParamBASE_SummertimeAll == 0)
            {
                _waitTimerReadKo = 0;
                bool dst = (bool)ko.value(DPT_Switch);
                setDaylightSavingMode(dst ? DaylightSavingMode::AlwaysDayLightSavingTime : DaylightSavingMode::AlwaysStandardTime);
            }
#endif
        }

        void TimeManager::setLocalTime(tm& tm, unsigned long millisReceivedTimestamp)
        {
            if (_timeProvideSupportKnxDaylightSavingTimeSwitch)
            {
                // <Enumeration Text="Kommunikationsobjekt 'Sommerzeit aktiv'" Value="0" Id="%ENID%" />
                // <Enumeration Text="Kombiniertes Datum/Zeit-KO (DPT 19)" Value="1" Id="%ENID%" />
                // <Enumeration Text="Interne Berechnung" Value="2" Id="%ENID%" />
                if (ParamBASE_SummertimeAll != 2)
                {
                    DaylightSavingMode mode = tm.tm_isdst == 1 ? DaylightSavingMode::AlwaysDayLightSavingTime : DaylightSavingMode::AlwaysStandardTime;
                    if (mode != _daylightSavingMode)
                        setDaylightSavingMode(mode);
                }
            }
            std::time_t epoch = mktime(&tm);
            _timeClock.setTime(epoch, millisReceivedTimestamp);
            checkChangedTime(tm, false, millisReceivedTimestamp);
        }

        void TimeManager::setUtcTime(tm& tm, unsigned long millisReceivedTimestamp)
        {
            std::time_t epoch = mktime(&tm) - _timezone;
            _timeClock.setTime(epoch, millisReceivedTimestamp);
            checkChangedTime(tm, true, millisReceivedTimestamp);
        }

        void TimeManager::timeSet()
        {
            time_t time = _timeClock.getTime();
            tm tm;
            localtime_r(&time, &tm);
            checkChangedTime(tm, false, millis());
        }

        void TimeManager::checkChangedTime(tm& setTime, bool utc, unsigned long millisReceivedTimestamp)
        {
            _timeUpdatedActivity = millis();
            std::time_t now = _timeClock.getTime();
            _lastUpdatedByProviderTimeStamp = now;
            double offset = difftime(now, _lastTimeStamp);
            bool changed = offset > 2 || offset < -2;
            if (changed)
                logInfoP("Set: %04d-%02d-%02d %02d:%02d:%02d (%s) receive before: %lums",
                         (int)setTime.tm_year + 1900,
                         (int)setTime.tm_mon + 1,
                         (int)setTime.tm_mday,
                         (int)setTime.tm_hour,
                         (int)setTime.tm_min,
                         (int)setTime.tm_sec,
                         utc ? "UTC" : (setTime.tm_isdst ? "DST" : "ST"),
                         millis() - millisReceivedTimestamp);
            else
                logDebugP("Updated: %04d-%02d-%02d %02d:%02d:%02d (%s) receive before: %lums",
                          (int)setTime.tm_year + 1900,
                          (int)setTime.tm_mon + 1,
                          (int)setTime.tm_mday,
                          (int)setTime.tm_hour,
                          (int)setTime.tm_min,
                          (int)setTime.tm_sec, utc ? "UTC" : (setTime.tm_isdst ? "DST" : "ST"),
                          millis() - millisReceivedTimestamp);
            _lastTimeStamp = now;
            sendTime();
            _timeSetEventNeeded = true;
       
        }

        int8_t TimeManager::isDaylightSavingTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute)
        {
            tm timeinfo = {0};
            timeinfo.tm_year = year - 1900; // Year since 1900
            timeinfo.tm_mon = month - 1;    // Month (0-based, January is 0)
            timeinfo.tm_mday = day;         // Day of the month (1-31)
            timeinfo.tm_hour = hour;
            timeinfo.tm_min = minute;
            return isDaylightSavingTime(timeinfo);
        }
        int8_t TimeManager::isDaylightSavingTime(tm timeinfo)
        {
            timeinfo.tm_isdst = 0;
            // Convert the date to time_t
            time_t rawtime = mktime(&timeinfo);
            // Convert back to local time and check DST
            tm localTimeinfo;
            localtime_r(&rawtime, &localTimeinfo);

            // check one hour before
            timeinfo.tm_sec += _dayLightSavingTimeOffset;
            // Convert the date to time_t
            time_t rawtimeMinusOneHour = mktime(&timeinfo);
            // Convert back to local time and check DST
            tm localTimeinfoMinusOnHour;
            localtime_r(&rawtimeMinusOneHour, &localTimeinfoMinusOnHour);
            if (localTimeinfo.tm_isdst != localTimeinfoMinusOnHour.tm_isdst)
                return -1;
            // Check if DST is active (tm_isdst == 1 means DST is active)
            return localTimeinfo.tm_isdst > 0;
        }
        void TimeManager::calculateAndSetDstFlag(tm& tm)
        {
            int isActive = isDaylightSavingTime(tm);
            if (isActive >= 0)
            {
                tm.tm_isdst = isActive == 1;
            }
            else
            {
                // switching hour in autumn, its unknown if daylight saving time or standard time because the local hour exist twice
                if (isValid())
                {
                    DateTime currentLocalTime = getLocalTime();
                    tm.tm_isdst = currentLocalTime.isDst; // asume same time
                    time_t currentTime = currentLocalTime.toTime_t();
                    time_t newTime = mktime(&tm);
                    double seconds = difftime(newTime, currentTime);
                    if (seconds < -2700)
                    {
                        // new time is more then 45 minutes behind current time, assume switch to winter time
                        tm.tm_isdst = 0;
                    }
                }
                else
                {
                    // No information about DST or ST time available. Just guess it's daylight saving time
                    tm.tm_isdst = 1;
                }
            }
        }

    } // namespace Time
} // namespace OpenKNX