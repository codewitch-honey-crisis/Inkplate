
#ifndef RTC_TIME_H
#define RTC_TIME_H
#include <stdbool.h>
#include <time.h>
#ifdef __cplusplus
extern "C" {
#endif
bool rtc_time_init(void);
bool rtc_time_now(struct tm* out_tm);
bool rtc_time_set(const struct tm* time);
bool rtc_time_set_tz_offset(long offset);
bool rtc_time_get_tz_offset(long* out_offset);
#ifdef __cplusplus
}
#endif
#endif // RTC_TIME_H