#ifndef WATCH_CLOCK_H
#define WATCH_CLOCK_H

#include <stdint.h>

enum {
    WATCH_CLOCK_SUBSECOND_HZ = 8
};

typedef struct {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t subsecond;
} watch_time_t;

void watch_clock_init(uint8_t hour, uint8_t minute, uint8_t second);
watch_time_t watch_clock_get(void);

#endif
