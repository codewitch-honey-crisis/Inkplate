#ifndef TIMING_H
#define TIMING_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/// @brief Gets the number of milliseconds since boot
/// @return The number of milliseconds since boot
uint32_t timing_get_ms();
/// @brief Delay for the specified number of miliseconds
/// @param ms The number of milliseconds
void timing_delay_ms(uint32_t ms);
/// @brief Delay for the specified number of microseconds
/// @param ms The number of microseconds
void timing_delay_us(uint32_t us);
#ifdef __cplusplus
}
#endif

#endif // TIMING_H