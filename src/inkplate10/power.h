#ifndef POWER_H
#define POWER_H
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
float power_battery_voltage(void);
float power_battery_level(void);
void power_sleep(void);
#ifdef __cplusplus
}
#endif
#endif // POWER_H