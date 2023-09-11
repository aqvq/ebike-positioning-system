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

mytime_t get_timestamp();
int set_time(int year, int mon, int day, int hour, int min, int sec);
void print_time(uint32_t timestamp, char *dest, size_t len);

#endif // !__TIME_H_SHANG
