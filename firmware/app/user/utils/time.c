#include "time.h"
#include "rtc.h"
#include "log/log.h"
#define TAG "TIME"

extern RTC_HandleTypeDef hrtc;
const short __mday[13] =
    {
        0,
        (31),
        (31 + 28),
        (31 + 28 + 31),
        (31 + 28 + 31 + 30),
        (31 + 28 + 31 + 30 + 31),
        (31 + 28 + 31 + 30 + 31 + 30),
        (31 + 28 + 31 + 30 + 31 + 30 + 31),
        (31 + 28 + 31 + 30 + 31 + 30 + 31 + 31),
        (31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30),
        (31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31),
        (31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30),
        (31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31),
};

int __isleap(int year)
{
    return (!(year % 4) && ((year % 100) || !(year % 400)));
}

// std库的mktime有问题，这里使用自己写的mktime函数
static mytime_t mymktime(struct tm *const t)
{
    mytime_t day;
    mytime_t i;
    mytime_t years = t->tm_year - 70;

    if (t->tm_sec > 60) {
        t->tm_min += t->tm_sec / 60;
        t->tm_sec %= 60;
    }

    if (t->tm_min > 60) {
        t->tm_hour += t->tm_min / 60;
        t->tm_min %= 60;
    }

    if (t->tm_hour > 24) {
        t->tm_mday += t->tm_hour / 24;
        t->tm_hour %= 24;
    }

    if (t->tm_mon > 12) {
        t->tm_year += t->tm_mon / 12;
        t->tm_mon %= 12;
    }

    while (t->tm_mday > __mday[1 + t->tm_mon]) {
        if (t->tm_mon == 1 && __isleap(t->tm_year + 1900)) {
            --t->tm_mday;
        }
        t->tm_mday -= __mday[t->tm_mon];
        ++t->tm_mon;

        if (t->tm_mon > 11) {
            t->tm_mon = 0;
            ++t->tm_year;
        }
    }

    if (t->tm_year < 70)
        return (mytime_t)-1;

    /* 1970年以来的天数等于365 *年数+ 1970年以来的闰年数 */
    day = years * 365 + (years + 1) / 4;

    /* 2100年以后，计算闰年的方式不一样了，每400年减去3个闰年，大多数mktime实现不支持2059年后的日期，所以可以把这个省略掉 */
    if ((int)(years -= 131) >= 0) {
        years /= 100;
        day -= (years >> 2) * 3 + 1;

        if ((years &= 3) == 3)
            years--;

        day -= years;
    }

    day += t->tm_yday = __mday[t->tm_mon] + t->tm_mday - 1 + (__isleap(t->tm_year + 1900) & (t->tm_mon > 1));

    /* 现在是自1970年1月1日以来的天数 */
    i          = 7;
    t->tm_wday = (day + 4) % i; /* 星期天=0, 星期一=1, ..., 星期六=6 */

    i = 24;
    day *= i;
    i = 60;
    return ((day + t->tm_hour) * i + t->tm_min) * i + t->tm_sec;
}

int set_time(int year, int mon, int day, int hour, int min, int sec)
{
    LOGD(TAG, "Set Time");
    LOGD(TAG, "year: %d, mon: %d, day: %d, hour: %d, min: %d, sec: %d", year, mon, day, hour, min, sec);
    RTC_TimeTypeDef RTC_TimeStruct = {0};
    RTC_TimeStruct.Hours           = hour;
    RTC_TimeStruct.Minutes         = min;
    RTC_TimeStruct.Seconds         = sec;
    RTC_DateTypeDef RTC_DateStruct = {0};
    RTC_DateStruct.Year            = year - 2000;
    RTC_DateStruct.Month           = mon;
    RTC_DateStruct.Date            = day;
    HAL_RTC_SetTime(&hrtc, &RTC_TimeStruct, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &RTC_DateStruct, RTC_FORMAT_BIN);

    return 0;
}

mytime_t get_timestamp()
{
    RTC_DateTypeDef sdate_get;
    RTC_TimeTypeDef stime_get;
    /* Get the RTC current Date */
    HAL_RTC_GetDate(&hrtc, &sdate_get, RTC_FORMAT_BIN);
    /* Get the RTC current Time */
    HAL_RTC_GetTime(&hrtc, &stime_get, RTC_FORMAT_BIN);
    struct tm time = {0};
    time.tm_sec    = stime_get.Seconds;
    time.tm_min    = stime_get.Minutes;
    time.tm_hour   = stime_get.Hours;
    time.tm_mday   = sdate_get.Date;
    time.tm_mon    = sdate_get.Month - 1;
    time.tm_year   = sdate_get.Year + 100;
    time.tm_isdst  = -1;
    return mymktime(&time);
}
