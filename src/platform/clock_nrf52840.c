#include "watch/clock.h"

#include <stdint.h>

#define REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))

#define CLOCK_BASE              0x40000000u
#define CLOCK_TASKS_LFCLKSTART  REG32(CLOCK_BASE + 0x008u)
#define CLOCK_EVENTS_LFSTARTED  REG32(CLOCK_BASE + 0x104u)
#define CLOCK_LFCLKSRC          REG32(CLOCK_BASE + 0x518u)

#define RTC1_BASE        0x40011000u
#define RTC1_TASKS_START REG32(RTC1_BASE + 0x000u)
#define RTC1_TASKS_CLEAR REG32(RTC1_BASE + 0x008u)
#define RTC1_COUNTER     REG32(RTC1_BASE + 0x504u)
#define RTC1_PRESCALER   REG32(RTC1_BASE + 0x508u)

#define SECONDS_PER_DAY  86400u
#define TICKS_PER_DAY    (SECONDS_PER_DAY * WATCH_CLOCK_SUBSECOND_HZ)
#define RTC_COUNTER_MASK 0x00FFFFFFu

static uint32_t start_ticks;
static uint32_t elapsed_ticks;
static uint32_t previous_counter;

void watch_clock_init(uint8_t hour, uint8_t minute, uint8_t second)
{
    uint32_t start_seconds =
        ((uint32_t)(hour % 24u) * 3600u) +
        ((uint32_t)(minute % 60u) * 60u) +
        (uint32_t)(second % 60u);
    start_ticks = start_seconds * WATCH_CLOCK_SUBSECOND_HZ;
    elapsed_ticks = 0u;
    previous_counter = 0u;

#ifndef WATCH_ENABLE_BLE
    /* In a non-BLE build the application owns LFCLK directly. */
    CLOCK_LFCLKSRC = 0u;
    CLOCK_EVENTS_LFSTARTED = 0u;
    CLOCK_TASKS_LFCLKSTART = 1u;
    while (CLOCK_EVENTS_LFSTARTED == 0u) {
    }
#endif

    RTC1_TASKS_CLEAR = 1u;
    /* 32,768 Hz / (4095 + 1) = eight animation ticks per second. */
    RTC1_PRESCALER = 4095u;
    RTC1_TASKS_START = 1u;
}

watch_time_t watch_clock_get(void)
{
    uint32_t counter = RTC1_COUNTER & RTC_COUNTER_MASK;
    uint32_t delta = (counter - previous_counter) & RTC_COUNTER_MASK;
    previous_counter = counter;
    elapsed_ticks = (elapsed_ticks + delta) % TICKS_PER_DAY;

    uint32_t ticks = (start_ticks + elapsed_ticks) % TICKS_PER_DAY;
    uint32_t seconds = ticks / WATCH_CLOCK_SUBSECOND_HZ;
    watch_time_t time;

    time.subsecond = (uint8_t)(ticks % WATCH_CLOCK_SUBSECOND_HZ);
    time.hour = (uint8_t)(seconds / 3600u);
    seconds %= 3600u;
    time.minute = (uint8_t)(seconds / 60u);
    time.second = (uint8_t)(seconds % 60u);
    return time;
}
