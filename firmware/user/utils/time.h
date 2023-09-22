/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 15:27:50
 * @FilePath: \firmware\user\utils\time.h
 * @Description: 自定义time头文件
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */
#ifndef __TIME_H_SHANG
#define __TIME_H_SHANG

#include <stddef.h>
#include <stdint.h>

struct tm {
    int tm_sec;   /* Seconds.	[0-60] (1 leap second) */
    int tm_min;   /* Minutes.	[0-59] */
    int tm_hour;  /* Hours.	[0-23] */
    int tm_mday;  /* Day.		[1-31] */
    int tm_mon;   /* Month.	[0-11] */
    int tm_year;  /* Year - 1900. */
    int tm_wday;  /* Day of week.	[0-6] */
    int tm_yday;  /* Days in year.[0-365]	*/
    int tm_isdst; /* DST.		[-1/0/1]*/

    long int tm_gmtoff;  /* Seconds east of UTC.  */
    const char *tm_zone; /* Timezone abbreviation.  */
};

typedef unsigned long mytime_t;

/// @brief 获取时间戳
/// @return unsigned long
mytime_t get_timestamp();

/// @brief 设置RTC时间
/// @param year 年
/// @param mon 月
/// @param day 日
/// @param hour 时
/// @param min 分
/// @param sec 秒
/// @return int
int set_time(int year, int mon, int day, int hour, int min, int sec);

/// @brief 打印时间
/// @param timestamp 待打印的时间戳
/// @param dest 目的字符串缓冲区
/// @param len 目的字符串缓冲区大小
void print_time(uint32_t timestamp, char *dest, size_t len);

#endif // !__TIME_H_SHANG
