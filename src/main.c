#include <stdint.h>

#define WATCH_DEBUG_MAGIC 0x57415443u /* "WATC" */

typedef struct {
    uint32_t magic;
    uint32_t boot_count;
    uint32_t heartbeat;
    uint32_t last_checkpoint;
} watch_debug_state_t;

/*
 * Kept across a debugger reset when RAM is preserved. It gives GDB a safe,
 * board-independent way to prove that the application is running.
 */
__attribute__((section(".noinit"), used))
volatile watch_debug_state_t watch_debug_state;

static void delay(void)
{
    for (volatile uint32_t i = 0; i < 400000u; ++i) {
        __asm volatile ("nop");
    }
}

int main(void)
{
    if (watch_debug_state.magic != WATCH_DEBUG_MAGIC) {
        watch_debug_state.magic = WATCH_DEBUG_MAGIC;
        watch_debug_state.boot_count = 0;
    }

    watch_debug_state.boot_count++;
    watch_debug_state.heartbeat = 0;
    watch_debug_state.last_checkpoint = 1;

    for (;;) {
        watch_debug_state.heartbeat++;
        watch_debug_state.last_checkpoint = 2;
        delay();
    }
}

