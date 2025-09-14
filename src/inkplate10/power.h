#ifndef POWER_H
#define POWER_H
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
// TODO: TEST THE BATTERY FUNCTIONS

/// @brief Reads the battery voltage
/// @return The voltage, or zero if no battery
float power_battery_voltage(void);
/// @brief Reads the battery level
/// @return The level, or 0 if no battery
float power_battery_level(void);
/// @brief Sleeps the device
void power_sleep(void);
#ifdef __cplusplus
}
#endif
#endif // POWER_H