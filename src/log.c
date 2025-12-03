#include <stdint.h>
#include <stdio.h>
#include "timing.h"
void log_timestamp(void) {
    uint32_t ms = timing_get_ms();
    float sec = ms/1000.f;
    printf("@ %03.1fs: ",sec);
}
