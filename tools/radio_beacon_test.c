#include <stdint.h>

#define REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))

#define CLOCK_BASE             0x40000000u
#define CLOCK_HFCLKSTART       REG32(CLOCK_BASE + 0x000u)
#define CLOCK_HFSTARTED        REG32(CLOCK_BASE + 0x100u)
#define CLOCK_HFCLKSTAT        REG32(CLOCK_BASE + 0x40Cu)

#define POWER_BASE             0x40000000u
#define POWER_CONSTLAT         REG32(POWER_BASE + 0x078u)

#define RADIO_BASE             0x40001000u
#define RADIO_TASKS_TXEN       REG32(RADIO_BASE + 0x000u)
#define RADIO_TASKS_START      REG32(RADIO_BASE + 0x008u)
#define RADIO_TASKS_DISABLE    REG32(RADIO_BASE + 0x010u)
#define RADIO_EVENTS_READY     REG32(RADIO_BASE + 0x100u)
#define RADIO_EVENTS_END       REG32(RADIO_BASE + 0x10Cu)
#define RADIO_EVENTS_DISABLED  REG32(RADIO_BASE + 0x110u)
#define RADIO_SHORTS           REG32(RADIO_BASE + 0x200u)
#define RADIO_PACKETPTR        REG32(RADIO_BASE + 0x504u)
#define RADIO_FREQUENCY        REG32(RADIO_BASE + 0x508u)
#define RADIO_TXPOWER          REG32(RADIO_BASE + 0x50Cu)
#define RADIO_MODE             REG32(RADIO_BASE + 0x510u)
#define RADIO_PCNF0            REG32(RADIO_BASE + 0x514u)
#define RADIO_PCNF1            REG32(RADIO_BASE + 0x518u)
#define RADIO_BASE0            REG32(RADIO_BASE + 0x51Cu)
#define RADIO_PREFIX0          REG32(RADIO_BASE + 0x524u)
#define RADIO_TXADDRESS        REG32(RADIO_BASE + 0x52Cu)
#define RADIO_RXADDRESSES      REG32(RADIO_BASE + 0x530u)
#define RADIO_CRCCNF           REG32(RADIO_BASE + 0x534u)
#define RADIO_CRCPOLY          REG32(RADIO_BASE + 0x538u)
#define RADIO_CRCINIT          REG32(RADIO_BASE + 0x53Cu)
#define RADIO_TIFS             REG32(RADIO_BASE + 0x544u)
#define RADIO_DATAWHITEIV      REG32(RADIO_BASE + 0x554u)
#define RADIO_MODECNF0         REG32(RADIO_BASE + 0x650u)
#define RADIO_POWER            REG32(RADIO_BASE + 0xFFCu)

#define GPIO_P0_BASE           0x50000000u
#define GPIO_OUTSET            REG32(GPIO_P0_BASE + 0x508u)
#define GPIO_OUTCLR            REG32(GPIO_P0_BASE + 0x50Cu)
#define GPIO_DIRSET            REG32(GPIO_P0_BASE + 0x518u)
#define GPIO_PIN_CNF_15        REG32(GPIO_P0_BASE + 0x700u + 15u * 4u)

static uint8_t __attribute__((aligned(4))) packet[] = {
    0x42u, 0x14u,
    0x24u, 0x2Du, 0x95u, 0x18u, 0xC0u, 0xE8u,
    0x02u, 0x01u, 0x06u,
    0x0Au, 0x09u,
    'R', 'A', 'W', ' ', 'T', 's', 'e', 'h', 'o'
};

typedef struct {
    uint32_t magic;
    uint32_t loops;
    uint32_t ready[3];
    uint32_t end[3];
    uint32_t disabled[3];
    uint32_t last_state;
    uint32_t last_frequency;
} radio_test_status_t;

volatile radio_test_status_t radio_test_status = {
    .magic = 0x52415732u, /* "RAW2" */
};

static void delay_cycles(volatile uint32_t cycles)
{
    while (cycles-- != 0u) {
        __asm volatile ("nop");
    }
}

static void transmit(uint8_t index, uint8_t channel, uint8_t frequency)
{
    RADIO_FREQUENCY = frequency;
    RADIO_DATAWHITEIV = channel;

    RADIO_EVENTS_READY = 0u;
    RADIO_EVENTS_END = 0u;
    RADIO_EVENTS_DISABLED = 0u;

    RADIO_TASKS_TXEN = 1u;

    while (RADIO_EVENTS_READY == 0u) {
    }
    radio_test_status.ready[index]++;

    RADIO_TASKS_START = 1u;
    while (RADIO_EVENTS_END == 0u) {
    }
    radio_test_status.end[index]++;

    RADIO_TASKS_DISABLE = 1u;
    while (RADIO_EVENTS_DISABLED == 0u) {
    }
    radio_test_status.disabled[index]++;
    radio_test_status.last_state = REG32(RADIO_BASE + 0x550u);
    radio_test_status.last_frequency = RADIO_FREQUENCY;
}

int main(void)
{
    GPIO_OUTCLR = 1u << 15;
    GPIO_DIRSET = 1u << 15;
    GPIO_PIN_CNF_15 = 0x00000003u;

    CLOCK_HFSTARTED = 0u;
    CLOCK_HFCLKSTART = 1u;
    while ((CLOCK_HFSTARTED == 0u) || ((CLOCK_HFCLKSTAT & 1u) == 0u)) {
    }

    RADIO_POWER = 1u;
    POWER_CONSTLAT = 1u;
    RADIO_TXPOWER = 0u;
    RADIO_MODE = 3u;                  /* BLE 1 Mbit/s. */
    RADIO_MODECNF0 = 1u;              /* Fast, deterministic radio ramp-up. */
    RADIO_PCNF0 = 0x00000108u;        /* S0=1 byte, LFLEN=8 bits, S1LEN=0. */
    RADIO_PCNF1 = 0x020300FFu;        /* Whitening, BALEN=3, MAXLEN=255. */
    RADIO_BASE0 = 0x89BED600u;
    RADIO_PREFIX0 = 0x0000008Eu;      /* BLE advertising access address. */
    RADIO_TXADDRESS = 0u;
    RADIO_RXADDRESSES = 0u;
    RADIO_CRCCNF = 0x00000103u;       /* Three-byte BLE CRC, skip address. */
    RADIO_CRCPOLY = 0x0000065Bu;
    RADIO_CRCINIT = 0x00555555u;
    RADIO_TIFS = 150u;
    RADIO_PACKETPTR = (uint32_t)(uintptr_t)packet;
    RADIO_SHORTS = 0u;

    for (;;) {
        GPIO_OUTSET = 1u << 15;
        transmit(0u, 37u, 2u);
        transmit(1u, 38u, 26u);
        transmit(2u, 39u, 80u);
        radio_test_status.loops++;
        GPIO_OUTCLR = 1u << 15;
        delay_cycles(240000u);
    }
}
