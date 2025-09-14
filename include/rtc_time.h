
#ifndef RTC_TIME_H
#define RTC_TIME_H
#include <stdbool.h>
#include <time.h>
#ifdef __cplusplus
extern "C" {
#endif
/// @brief Initializes the clock
/// @return True if successful, otherwise false
bool rtc_time_init(void);
/// @brief Gets the current time
/// @param out_tm The tm structure to fill
/// @return true if successful, otherwise false
bool rtc_time_now(struct tm* out_tm);
/// @brief Sets the current time
/// @param time The tm structure to set with
/// @return True if successful, otherwise false
bool rtc_time_set(const struct tm* time);
/// @brief Sets the number of seconds in offset for the current timezone
/// @param offset The offset to set
/// @return True if successful, otherwise false
bool rtc_time_set_tz_offset(long offset);
/// @brief Gets the number of seconds in offset for the current timezone
/// @param out_offset The offset
/// @return True if successful, otherwise false
bool rtc_time_get_tz_offset(long* out_offset);
#ifdef __cplusplus
}
#endif
#endif // RTC_TIME_H