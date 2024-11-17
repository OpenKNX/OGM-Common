#pragma once
#include "knxprod.h"
#include "string"
#include "time.h"

namespace OpenKNX
{
    enum DateTimeType : uint8_t
    {
        DateTimeTypeLocalTimeST,
        DateTimeTypeLocalTimeDST,
        DateTimeTypeUTC
    };

    struct TimeOnly
    {
        uint8_t hour;
        uint8_t minute;
        uint8_t second;
        bool isDst : 1;
        bool isUtc : 1;
        bool isLocalTime : 1;
    };

    struct DateOnly
    {
        uint16_t year;
        uint8_t month;
        uint8_t day;
        /*
        * 0 = Sunday, 1 = Monday, 2 = Tuesday, 3 = Wednesday, 4 = Thursday, 5 = Friday, 6 = Saturday
        */
        uint8_t dayOfWeek;
        const char* dayOfWeekString() const;     
     };

    struct DateTime : public DateOnly, public TimeOnly
    {
        DateTime() = default;
        DateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second, DateTimeType type);

        DateTime(time_t time, bool createUtc = false);

        DateTime(tm& tm, bool isUtc = false, bool tmIsNormalized = false);

        void toTm(tm& tm) const;
        tm toTm() const;

        time_t toTime_t() const;

        DateTime toUtc() const;

        DateTime toLocalTime() const;

        void addSeconds(int seconds);

        void addMinutes(int minutes);

        void addHours(int hours);

        void addDays(int days);

        void addMonths(int months);

        std::string toString() const;
    };
} // namespace OpenKNX