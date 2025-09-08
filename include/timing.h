#ifndef TIMING_H
#define TIMING_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
uint32_t timing_get_ms();
void timing_delay_ms(uint32_t ms);
void timing_delay_us(uint32_t us);
#ifdef __cplusplus
}
#endif

#endif // TIMING_H