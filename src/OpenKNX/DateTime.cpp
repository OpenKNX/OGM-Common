#include "DateTime.h"
#include "OpenKNX.h"

namespace OpenKNX
{
    const char* DateOnly::dayOfWeekString() const
    {
        switch (dayOfWeek)
        {
        case 0:
            return "Sunday";
        case 1:
            return "Monday";
        case 2:
            return "Tuesday";
        case 3:
            return "Wednesday";
        case 4:
            return "Thursday";
        case 5:
            return "Friday";
        case 6:
            return "Saturday";
        default:
            return "Unknown";
        }
    }

    DateTime::DateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second, DateTimeType type)
    {
        this->year = year;
        this->month = month;
        this->day = day;
        this->hour = hour;
        this->minute = minute;
        this->second = second;
        this->isDst = type == DateTimeTypeLocalTimeDST;
        this->isUtc = type == DateTimeTypeUTC;
        this->isLocalTime = type != DateTimeTypeUTC;
    }

    DateTime::DateTime(time_t time, bool createUtc)
    {
        tm tm;
        if (createUtc)
            gmtime_r(&time, &tm);
        else
            localtime_r(&time, &tm);
        year = tm.tm_year + 1900;
        month = tm.tm_mon + 1;
        day = tm.tm_mday;
        dayOfWeek = tm.tm_wday;
        hour = tm.tm_hour;
        minute = tm.tm_min;
        second = tm.tm_sec;
        isDst = tm.tm_isdst;
        isLocalTime = !createUtc;
        isUtc = createUtc;
    }

    DateTime::DateTime(tm& tmStruct, bool isUtc, bool tmIsNormalized)
    {
        tm* fillout = &tmStruct;
        tm tmCopy;
        if (!tmIsNormalized)
        {
            if (isUtc)
            {
                // normalize
                tm tmUtc = tmStruct;
                time_t utc = mktime(&tmUtc) - _timezone;
                gmtime_r(&utc, &tmCopy);
                fillout = &tmCopy;
            }
            else
            {
                tmCopy = tmStruct;
                mktime(&tmCopy); // normalize an fill out day of week
                fillout = &tmCopy;
            }
        }
        year = fillout->tm_year + 1900;
        month = fillout->tm_mon + 1;
        day = fillout->tm_mday;
        dayOfWeek = fillout->tm_wday;
        hour = fillout->tm_hour;
        minute = fillout->tm_min;
        second = fillout->tm_sec;
        isDst = fillout->tm_isdst;
        isLocalTime = !isUtc;
        this->isUtc = isUtc;
    }

    void DateTime::toTm(tm& tm) const
    {
        tm = {0};
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min = minute;
        tm.tm_sec = second;
        tm.tm_isdst = isDst;
    }

    tm DateTime::toTm() const
    {
        tm tm;
        toTm(tm);
        return tm;
    }

    time_t DateTime::toTime_t() const
    {
        if (isUtc)
            return toLocalTime().toTime_t();
        tm tm;
        toTm(tm);
        return mktime(&tm);
    }

    DateTime DateTime::toUtc() const
    {
        if (isUtc)
            return *this;
        tm tmLocal;
        toTm(tmLocal);
        time_t localTime = mktime(&tmLocal);
        tm utc;
        gmtime_r(&localTime, &utc);
        return DateTime(utc, true, true);
    }

    DateTime DateTime::toLocalTime() const
    {
        if (isLocalTime)
            return *this;
        tm tmUtc;
        toTm(tmUtc);
        time_t utc = mktime(&tmUtc) - _timezone;
        tm localTime;
        localtime_r(&utc, &localTime);         
        return DateTime(localTime, false, true);
    }

    void DateTime::addSeconds(int seconds)
    {
        time_t time = toTime_t() + seconds;
        if (isUtc)
            *this = DateTime(time).toUtc();
        else
            *this = DateTime(time);
    }

    void DateTime::addMinutes(int minutes)
    {
        addSeconds(minutes * 60);
    }

    void DateTime::addHours(int hours)
    {
        addMinutes(hours * 60);
    }

    void DateTime::addDays(int days)
    {
        addHours(days * 24);
    }

    void DateTime::addMonths(int months)
    {
        tm tmLocal;
        toTm(tmLocal);
        tmLocal.tm_mon += months;
        mktime(&tmLocal);
        if (isUtc)
            *this = DateTime(tmLocal).toUtc();
        else
            *this = DateTime(tmLocal);
    }

    std::string DateTime::toString() const
    {
        char buffer[26];
        snprintf(buffer, sizeof(buffer),
                    "%04d-%02d-%02d %02d:%02d:%02d (%s)", year, month, day, hour, minute, second, isUtc ? "UTC" : (isDst ? "DST"
                                                                                                                : "ST"));
        return std::string(buffer);
    }
} // namespace OpenKNX